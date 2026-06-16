#include "ServiceProxyInternal.h"
#include "GuiLogSink.h"
#include "FrameLayout.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <sstream>
#include <string>
#include <windows.h>

namespace App {

namespace {

void PopulateRawHeatmapFromFrameData(Solvers::HeatmapFrame& frame) {
    if (!frame.rawPtr || frame.rawLen < Frame::kMasterFrameSize) return;
    const uint8_t* raw = frame.rawPtr + Frame::kMatrixOffset;
    int16_t* heat = reinterpret_cast<int16_t*>(frame.heatmapMatrix);
    for (int i = 0; i < Frame::kMatrixCells; ++i) {
        const uint16_t value = static_cast<uint16_t>(raw[i * 2]) |
            (static_cast<uint16_t>(raw[i * 2 + 1]) << 8);
        heat[i] = static_cast<int16_t>(value);
    }
}

uint64_t CaptureSystemEpochUs() {
    static LARGE_INTEGER frequency = [] {
        LARGE_INTEGER value{};
        QueryPerformanceFrequency(&value);
        return value;
    }();
    static const uint64_t qpcAtEpochUs = [] {
        LARGE_INTEGER counter{};
        QueryPerformanceCounter(&counter);
        const auto nowUs = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        const uint64_t elapsedUs = (static_cast<uint64_t>(counter.QuadPart) * 1000000ull) /
            static_cast<uint64_t>(frequency.QuadPart);
        return static_cast<uint64_t>(nowUs) - elapsedUs;
    }();

    LARGE_INTEGER counter{};
    QueryPerformanceCounter(&counter);
    const uint64_t elapsedUs = (static_cast<uint64_t>(counter.QuadPart) * 1000000ull) /
        static_cast<uint64_t>(frequency.QuadPart);
    return qpcAtEpochUs + elapsedUs;
}

} // namespace

// ── Poll loop with FPS measurement ──
void ServiceProxy::PollLoop() {

    uint64_t lastFpsFrameId = m_frameReader.LastFrameId();
    uint64_t lastSlaveFpsFrameId = m_frameReader.LastSlaveFrameId();
    uint64_t lastMasterFpsFrameId = m_frameReader.LastMasterFrameId();
    auto lastFpsTick = std::chrono::steady_clock::now();
    auto lastLogPoll = std::chrono::steady_clock::now();
    auto lastPenPoll = std::chrono::steady_clock::now();
    auto lastDynamicDebugPoll = std::chrono::steady_clock::now() - std::chrono::milliseconds(250);
    uint64_t latestDvrSeq = 0;
    uint64_t latestDvrTimestamp = 0;
    HANDLE frameEvent = m_frameReader.FrameReadyEvent();
    HANDLE stopEvent = m_pollStopEvent;
    while (m_polling.load()) {
        auto now = std::chrono::steady_clock::now();
        auto nextLogDue = lastLogPoll + std::chrono::milliseconds(1000);
        auto nextPenDue = lastPenPoll + std::chrono::milliseconds(500);
        auto nextDynamicDebugDue = lastDynamicDebugPoll + std::chrono::milliseconds(250);
        auto nextDue = std::min(nextLogDue, std::min(nextPenDue, nextDynamicDebugDue));
        DWORD timeoutMs = 1000;
        if (nextDue <= now) {
            timeoutMs = 0;
        } else {
            timeoutMs = static_cast<DWORD>(
                std::chrono::duration_cast<std::chrono::milliseconds>(nextDue - now).count());
        }

        DWORD waitRes = WAIT_TIMEOUT;
        HANDLE handles[4];
        enum class WaitType { Stop, Frame, Log, Pen };
        WaitType types[4];
        DWORD count = 0;
        if (stopEvent) {
            handles[count] = stopEvent;
            types[count] = WaitType::Stop;
            ++count;
        }
        if (frameEvent) {
            handles[count] = frameEvent;
            types[count] = WaitType::Frame;
            ++count;
        }
        if (m_logEvent) {
            handles[count] = m_logEvent;
            types[count] = WaitType::Log;
            ++count;
        }
        if (m_penEvent) {
            handles[count] = m_penEvent;
            types[count] = WaitType::Pen;
            ++count;
        }

        if (count > 0) {
            waitRes = WaitForMultipleObjects(count, handles, FALSE, timeoutMs);
        } else {
            Sleep(std::min<DWORD>(timeoutMs, 50));
        }

        if (count > 0 && waitRes >= WAIT_OBJECT_0 && waitRes < WAIT_OBJECT_0 + count) {
            const WaitType wt = types[waitRes - WAIT_OBJECT_0];
            if (wt == WaitType::Stop) {
                break;
            }
            if (wt == WaitType::Frame) {
                bool gotFrame = false;
                {
                    std::lock_guard<std::mutex> lk(m_frameMutex);
                    if (m_frameReader.Read(m_latestFrame)) {
                        if (m_masterParserOnly.load(std::memory_order_relaxed)) {
                            PopulateRawHeatmapFromFrameData(m_latestFrame);
                        }
                        m_latestFrame.receiveSystemEpochUs = CaptureSystemEpochUs();
                        m_hasNewFrame.store(true, std::memory_order_release);
                        gotFrame = true;
                    }
                }
                if (gotFrame && m_dvrBuffer) {
                    Dvr::DvrFrameSlot slot;
                    slot.CopyFrom(m_latestFrame);
                    slot.dvrSeq = m_dvrSeqCounter.fetch_add(1, std::memory_order_relaxed) + 1;
                    m_dvrBuffer->PushOverwriting(slot);
                    latestDvrSeq = slot.dvrSeq;
                    latestDvrTimestamp = slot.timestamp;
                }
            }
            if (wt == WaitType::Log) {
                lastLogPoll = std::chrono::steady_clock::now() - std::chrono::milliseconds(1000);
            }
            if (wt == WaitType::Pen) {
                lastPenPoll = std::chrono::steady_clock::now() - std::chrono::milliseconds(500);
            }
        }

        now = std::chrono::steady_clock::now();
        // FPS counter
        auto fpsElapsed = std::chrono::duration_cast<
            std::chrono::milliseconds>(now - lastFpsTick);
        if (fpsElapsed.count() >= 1000) {
            // Master FPS: only counts frames where master was actually read
            uint64_t currentMasterId = m_frameReader.LastMasterFrameId();
            m_fps.store(static_cast<int>(currentMasterId - lastMasterFpsFrameId));
            lastMasterFpsFrameId = currentMasterId;

            // Slave FPS: counts every GetFrame() cycle (240Hz when stylus connected)
            uint64_t currentSlaveId = m_frameReader.LastSlaveFrameId();
            m_slaveFps.store(static_cast<int>(currentSlaveId - lastSlaveFpsFrameId));
            lastSlaveFpsFrameId = currentSlaveId;

            // Keep lastFpsFrameId in sync (used for frame-ready detection)
            lastFpsFrameId = m_frameReader.LastFrameId();

            lastFpsTick = now;
        }
        // Dynamic debug snapshot polling (~every 250ms). This keeps UI values fresh
        // without binding the shared-memory frame hot path to synchronous pipe IPC.
        auto dynamicDebugElapsed = std::chrono::duration_cast<
            std::chrono::milliseconds>(now - lastDynamicDebugPoll);
        if (dynamicDebugElapsed.count() >= 250 && m_client.IsConnected()) {
            uint64_t snapshotTimestamp = 0;
            if (RefreshDynamicDebugSnapshot(&snapshotTimestamp) &&
                snapshotTimestamp != 0 &&
                latestDvrSeq != 0 &&
                snapshotTimestamp == latestDvrTimestamp &&
                m_dvrDynamicDebugBuffer) {
                m_dvrDynamicDebugBuffer->PushOverwriting(CaptureDynamicDebugFrameSlot(latestDvrSeq));
            }
            lastDynamicDebugPoll = now;
        }

        // Service log polling (~every 1s)
        auto logElapsed = std::chrono::duration_cast<
            std::chrono::milliseconds>(now - lastLogPoll);
        if (logElapsed.count() >= 1000 && m_client.IsConnected()) {
            Ipc::IpcRequest req{};
            req.command = Ipc::IpcCommand::GetLogs;
            auto resp = m_client.Send(req);
            if (resp.success && resp.dataLen > 0) {
                std::string packed(
                    reinterpret_cast<const char*>(resp.data), resp.dataLen);
                std::istringstream iss(packed);
                std::string line;
                while (std::getline(iss, line)) {
                    if (line.empty()) continue;
                    // Service 日志格式: [timestamp] [level] [layer] [method] [state] msg
                    // GUI 只保留 [level] 之后的部分，去掉时间戳（首个 ']' 之后）
                    std::string display = line;
                    auto bracket = line.find("] ");  // 找时间戳末尾
                    if (bracket != std::string::npos)
                        display = line.substr(bracket + 2);  // 跳过 "] "
                    Common::GuiLogSink::Instance()->PushRaw("[Svc] " + display);
                }
            }
            lastLogPoll = now;
        }
        // PenBridge status polling (~every 500ms for responsive pressure bars)
        auto penElapsed = std::chrono::duration_cast<
            std::chrono::milliseconds>(now - lastPenPoll);
        if (penElapsed.count() >= 500 && m_client.IsConnected()) {
            Ipc::IpcRequest penReq{};
            penReq.command = Ipc::IpcCommand::GetPenBridgeStatus;
            auto penResp = m_client.Send(penReq);
            if (penResp.success && penResp.dataLen >= 13) {
                const uint8_t* d = penResp.data;
                PenBridgeStatus s;
                s.evtRunning   = d[0] != 0;
                s.pressRunning = d[1] != 0;
                s.reportType   = d[2];
                s.freq1        = d[3];
                s.freq2        = d[4];
                for (int k = 0; k < 4; ++k)
                    s.press[k] = static_cast<uint16_t>(d[5 + k * 2]) |
                                 (static_cast<uint16_t>(d[6 + k * 2]) << 8);
                if (penResp.dataLen >= 24) {
                    s.pressureMode = d[13];
                    s.pressureMax = static_cast<uint16_t>(d[14]) |
                                    (static_cast<uint16_t>(d[15]) << 8);
                    for (int k = 0; k < 4; ++k) {
                        s.rawPress[k] = static_cast<uint16_t>(d[16 + k * 2]) |
                                        (static_cast<uint16_t>(d[17 + k * 2]) << 8);
                    }
                }
                std::lock_guard<std::mutex> lk(m_penMutex);
                m_penStatus = s;
            }

            Ipc::IpcRequest identityReq{};
            identityReq.command = Ipc::IpcCommand::GetPenIdentityStatus;
            auto identityResp = m_client.Send(identityReq);
            if (identityResp.success && identityResp.dataLen >= sizeof(Ipc::PenIdentityStatusWire)) {
                Ipc::PenIdentityStatusWire wire{};
                std::memcpy(&wire, identityResp.data, sizeof(wire));
                if (wire.wireVersion == Ipc::kIpcProtocolVersion) {
                    PenIdentityStatus identity{};
                    identity.hasConnectionState = (wire.flags & Ipc::kPenIdentityHasConnectionState) != 0;
                    identity.connected = (wire.flags & Ipc::kPenIdentityConnected) != 0;
                    identity.hasStylusId = (wire.flags & Ipc::kPenIdentityHasStylusId) != 0;
                    identity.stylusId = wire.stylusId;
                    identity.hasPenModuleModelId = (wire.flags & Ipc::kPenIdentityHasPenModuleModelId) != 0;
                    identity.penModuleModelId = wire.penModuleModelId;
                    identity.hasProtocolHint = (wire.flags & Ipc::kPenIdentityHasProtocolHint) != 0;
                    switch (static_cast<Ipc::PenIdentityProtocolHint>(wire.protocolHint)) {
                    case Ipc::PenIdentityProtocolHint::Hpp2:
                        identity.protocolHint = PenProtocolHint::Hpp2;
                        break;
                    case Ipc::PenIdentityProtocolHint::Hpp3:
                        identity.protocolHint = PenProtocolHint::Hpp3;
                        break;
                    default:
                        identity.protocolHint = PenProtocolHint::Auto;
                        break;
                    }
                    identity.protocolHintFromPenModule =
                        (wire.protocolFlags & Ipc::kPenIdentityProtocolFromPenModule) != 0;
                    identity.factoryStatusFlags = wire.factoryStatusFlags;
                    auto assignUtf8Field = [](bool& present,
                                               std::string& target,
                                               uint16_t textLenWire,
                                               const auto& textBuffer) {
                        const auto textLen = std::min<std::size_t>(
                            textLenWire, sizeof(textBuffer));
                        if (present && textLen > 0) {
                            target.assign(textBuffer, textBuffer + textLen);
                        } else {
                            present = false;
                            target.clear();
                        }
                    };

                    identity.hasHardwareVersion = (wire.flags & Ipc::kPenIdentityHasHardwareVersion) != 0;
                    assignUtf8Field(identity.hasHardwareVersion,
                                    identity.hardwareVersion,
                                    wire.hardwareVersionUtf8Len,
                                    wire.hardwareVersionUtf8);
                    identity.hasSerialNumber = (wire.flags & Ipc::kPenIdentityHasSerialNumber) != 0;
                    assignUtf8Field(identity.hasSerialNumber,
                                    identity.serialNumber,
                                    wire.serialNumberUtf8Len,
                                    wire.serialNumberUtf8);
                    identity.hasFirmwareVersion = (wire.flags & Ipc::kPenIdentityHasFirmwareVersion) != 0;
                    assignUtf8Field(identity.hasFirmwareVersion,
                                    identity.firmwareVersion,
                                    wire.firmwareVersionUtf8Len,
                                    wire.firmwareVersionUtf8);

                    std::lock_guard<std::mutex> lk(m_penMutex);
                    m_penIdentityStatus = std::move(identity);
                }
            }
            lastPenPoll = now;
        }
    }
}

} // namespace App
