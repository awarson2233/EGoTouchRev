#include "ServiceProxy.h"
#include "Logger.h"
#include "GuiLogSink.h"
#include "IpcProtocol.h"
#include <sstream>
#include <chrono>
#include <fstream>
#include <string>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string_view>
#include <filesystem>
#include "AsaTypes.hpp"

namespace App {

static const std::string kConfigPath = "C:/ProgramData/EGoTouchRev/config.ini";

namespace {

std::string TrimCopy(std::string_view input) {
    const size_t start = input.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) return {};
    const size_t end = input.find_last_not_of(" \t\r\n");
    return std::string(input.substr(start, end - start + 1));
}

bool ParseIniKeyValue(std::string_view line, std::string& key, std::string& value) {
    const size_t eq = line.find('=');
    if (eq == std::string_view::npos) return false;
    key = TrimCopy(line.substr(0, eq));
    value = TrimCopy(line.substr(eq + 1));
    return !key.empty();
}

bool IsLegacyTouchSection(const std::string& section) {
    return section == "Master Frame Parser" ||
           section == "Baseline Subtraction" ||
           section == "CMF Processor" ||
           section == "Grid IIR Processor" ||
           section == "Feature Extractor (4.1/4.2)" ||
           section == "Touch Tracker (IDT)" ||
           section == "Coordinate Filter (1 Euro)" ||
           section == "TouchGestureStateMachine";
}

std::optional<std::string> MapLegacyTouchKey(const std::string& section,
                                             const std::string& key) {
    if (section == "Master Frame Parser") {
        if (key == "Enabled") return std::string("FrameParserEnabled");
        return std::nullopt;
    }
    if (section == "Baseline Subtraction") {
        if (key == "Enabled") return std::string("BaselineEnabled");
        return key;
    }
    if (section == "CMF Processor") {
        if (key == "Enabled") return std::string("CMFEnabled");
        if (key == "DimensionMode") return std::string("CMFDimensionMode");
        if (key == "ExclusionThreshold") return std::string("CMFExclusionThreshold");
        if (key == "MaxCorrection") return std::string("CMFMaxCorrection");
        return key;
    }
    if (section == "Grid IIR Processor") {
        if (key == "Enabled") return std::string("GridIIREnabled");
        return key;
    }
    if (section == "Feature Extractor (4.1/4.2)") {
        if (key == "Enabled") return std::nullopt;
        return key;
    }
    if (section == "Touch Tracker (IDT)") {
        if (key == "Enabled") return std::string("TrackerEnabled");
        return key;
    }
    if (section == "Coordinate Filter (1 Euro)") {
        if (key == "Enabled") return std::string("CoordFilterEnabled");
        return key;
    }
    if (section == "TouchGestureStateMachine") {
        if (key == "Enabled") return std::string("GestureEnabled");
        return key;
    }
    return std::nullopt;
}

} // namespace

ServiceProxy::ServiceProxy()
    : m_dvrBuffer(std::make_unique<RingBuffer<Dvr::DvrFrameSlot, 480>>()) {
    // TouchPipeline is self-contained — no processor registration needed.
    LoadConfig();
}

ServiceProxy::~ServiceProxy() {
    // Join any in-flight DVR export before tearing down resources
    if (m_dvrThread.joinable()) m_dvrThread.join();
    StopAutoDiscovery();
    Disconnect();
}

bool ServiceProxy::Connect() {
    // 1. Open shared memory (Service owns the Global\\ mapping)
    if (!m_frameReader.Open(kSharedMemName)) {
        LOG_ERROR("App", __func__, "IPC", "Failed to open shared memory (Service not running?).");
        return false;
    }
    // 2. Open config dirty flag
    m_configDirty.Open();

    // 3. Connect pipe to Service
    if (!m_client.Connect(3000)) {
        LOG_ERROR("App", __func__, "IPC", "Pipe connection failed.");
        m_frameReader.Close();
        return false;
    }
    // 4. Tell Service to enter debug mode
    auto resp = m_client.EnterDebugMode(kSharedMemName);
    if (!resp.success) {
        LOG_ERROR("App", __func__, "IPC", "EnterDebugMode rejected.");
        m_client.Disconnect();
        m_frameReader.Close();
        return false;
    }
    if (!m_logEvent) {
        m_logEvent = OpenEventW(SYNCHRONIZE, FALSE, Ipc::kLogReadyEventName);
        if (!m_logEvent) {
            LOG_WARN("App", __func__, "IPC", "OpenEvent failed for LogReadyEvent: {}", GetLastError());
        }
    }
    if (!m_penEvent) {
        m_penEvent = OpenEventW(SYNCHRONIZE, FALSE, Ipc::kPenReadyEventName);
        if (!m_penEvent) {
            LOG_WARN("App", __func__, "IPC", "OpenEvent failed for PenReadyEvent: {}", GetLastError());
        }
    }
    if (!m_pollStopEvent) {
        m_pollStopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!m_pollStopEvent) {
            LOG_WARN("App", __func__, "IPC", "CreateEvent failed for PollStopEvent: {}", GetLastError());
        }
    }
    // 5. Start polling thread
    m_polling.store(true);
    m_pollThread = std::thread(&ServiceProxy::PollLoop, this);

    LOG_INFO("App", __func__, "IPC", "Connected to EGoTouchService.");
    return true;
}

void ServiceProxy::Disconnect() {
    // Stop polling
    m_polling.store(false);
    if (m_pollStopEvent) {
        SetEvent(m_pollStopEvent);
    }
    if (m_pollThread.joinable()) m_pollThread.join();
    if (m_pollStopEvent) {
        CloseHandle(m_pollStopEvent);
        m_pollStopEvent = nullptr;
    }
    if (m_logEvent) {
        CloseHandle(m_logEvent);
        m_logEvent = nullptr;
    }
    if (m_penEvent) {
        CloseHandle(m_penEvent);
        m_penEvent = nullptr;
    }

    // Tell Service to exit debug mode
    if (m_client.IsConnected()) {
        m_client.ExitDebugMode();
        m_client.Disconnect();
    }
    m_frameReader.Close();
    m_configDirty.Close();
    m_fps.store(0);
    m_slaveFps.store(0);
    LOG_INFO("App", __func__, "IPC", "Disconnected.");
}

// ── Auto-discovery ──
void ServiceProxy::StartAutoDiscovery(int intervalMs) {
    if (m_discovering.load()) return;
    m_discoveryIntervalMs = intervalMs;
    m_discovering.store(true);
    m_discoveryThread = std::thread(&ServiceProxy::DiscoveryLoop, this);
    LOG_INFO("App", __func__, "IPC", "Auto-discovery started (interval={}ms).", intervalMs);
}

void ServiceProxy::StopAutoDiscovery() {
    m_discovering.store(false);
    if (m_discoveryThread.joinable()) m_discoveryThread.join();
}

bool ServiceProxy::TryConnect() {
    if (IsConnected()) return true;
    return Connect();
}

void ServiceProxy::DiscoveryLoop() {
    while (m_discovering.load()) {
        if (!IsConnected()) {
            if (Connect()) {
                LOG_INFO("App", __func__, "IPC", "Service discovered and connected.");
            }
        }
        // Sleep in small increments so we can stop quickly
        for (int i = 0; i < m_discoveryIntervalMs / 100 && m_discovering.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

bool ServiceProxy::IsConnected() const {
    return m_client.IsConnected();
}

bool ServiceProxy::GetLatestFrame(Solvers::HeatmapFrame& out) {
    if (!m_hasNewFrame.load()) return false;
    std::lock_guard<std::mutex> lk(m_frameMutex);
    out = m_latestFrame;
    m_hasNewFrame.store(false);
    return true;
}

bool ServiceProxy::SwitchAfeMode(uint8_t afeCmd, uint8_t param) {
    auto resp = m_client.SendAfeCommand(afeCmd, param);
    return resp.success;
}

bool ServiceProxy::StartRemoteRuntime() {
    return m_client.StartRuntime().success;
}

bool ServiceProxy::StopRemoteRuntime() {
    return m_client.StopRuntime().success;
}

void ServiceProxy::SaveConfig() {
    // 1. 生成服务层的配置段（[Service]段）
    std::string serviceBlock = "[Service]\n";
    serviceBlock += "mode=" + std::string(m_srvModeFull ? "full" : "touch_only") + "\n";
    serviceBlock += "auto_mode=" + std::string(m_srvAutoMode ? "1" : "0") + "\n";
    serviceBlock += "stylus_vhf_enabled=" + std::string(m_srvStylusVhfEnabled ? "1" : "0") + "\n";

    // 2. 将全量配置写回
    std::ofstream out(kConfigPath);
    if (!out.is_open()) return;

    out << serviceBlock << "\n";
    // TouchPipeline (unified section)
    out << "[TouchPipeline]\n";
    m_pipeline.SaveConfig(out);
    out << "\n";
    // Write stylus pipeline config
    out << "[StylusPipeline]\n";
    m_stylusPipeline.SaveConfig(out);
    out << "\n";
    out.close();
    // Notify Service to reload from config.ini
    m_configDirty.SetDirty();
    m_client.ReloadConfig();
    LOG_INFO("App", __func__, "IPC", "Config saved and Service notified to reload.");
}

void ServiceProxy::LoadConfig() {
    std::ifstream in(kConfigPath);
    if (!in.is_open()) return;
    std::string line, section;
    while (std::getline(in, line)) {
        const std::string trimmed = TrimCopy(line);
        if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#') continue;
        if (trimmed.front() == '[' && trimmed.back() == ']') {
            section = TrimCopy(std::string_view(trimmed).substr(1, trimmed.size() - 2));
            continue;
        }

        std::string key;
        std::string value;
        if (!ParseIniKeyValue(trimmed, key, value)) continue;

        if (section == "Service") {
            if (key == "mode") m_srvModeFull = (value == "full");
            else if (key == "auto_mode") m_srvAutoMode = (value == "1" || value == "true");
            else if (key == "stylus_vhf_enabled") m_srvStylusVhfEnabled = (value == "1" || value == "true");
        } else if (section == "TouchPipeline") {
            m_pipeline.LoadConfig(key, value);
        } else if (section == "StylusPipeline") {
            m_stylusPipeline.LoadConfig(key, value);
        } else if (IsLegacyTouchSection(section)) {
            const auto mappedKey = MapLegacyTouchKey(section, key);
            if (mappedKey.has_value()) {
                m_pipeline.LoadConfig(*mappedKey, value);
            }
        }
    }
}

void ServiceProxy::NotifyConfigDirty() {
    m_configDirty.SetDirty();
}

// ── VHF control ──
bool ServiceProxy::SetVhfEnabled(bool enabled) {
    Ipc::IpcRequest req{};
    req.command = Ipc::IpcCommand::SetVhfEnabled;
    req.param[0] = enabled ? 1 : 0; req.paramLen = 1;
    bool ok = m_client.Send(req).success;
    if (ok) m_vhfEnabled.store(enabled);
    return ok;
}

bool ServiceProxy::SetVhfTranspose(bool enabled) {
    Ipc::IpcRequest req{};
    req.command = Ipc::IpcCommand::SetVhfTranspose;
    req.param[0] = enabled ? 1 : 0; req.paramLen = 1;
    bool ok = m_client.Send(req).success;
    if (ok) m_vhfTranspose.store(enabled);
    return ok;
}

bool ServiceProxy::SetAutoAfeSync(bool enabled) {
    Ipc::IpcRequest req{};
    req.command = Ipc::IpcCommand::SetAutoAfeSync;
    req.param[0] = enabled ? 1 : 0; req.paramLen = 1;
    bool ok = m_client.Send(req).success;
    if (ok) m_autoAfeSync.store(enabled);
    return ok;
}

// ── MasterParser-only mode (local) ──
void ServiceProxy::SetMasterParserOnlyMode(bool enabled) {
    // With new TouchPipeline, toggle individual module enables
    if (enabled && !m_masterParserOnly) {
        m_savedMasterOnly = true;
        // Disable all signal conditioning + processing beyond frame parse
        m_pipeline.m_baseline.m_enabled = false;
        m_pipeline.m_cmf.m_enabled = false;
        m_pipeline.m_gridIIR.m_enabled = false;
        m_pipeline.m_tracker.m_enabled = false;
        m_pipeline.m_coordFilter.m_enabled = false;
        m_pipeline.m_gesture.m_enabled = false;
    } else if (!enabled && m_masterParserOnly) {
        // Restore defaults
        m_pipeline.m_baseline.m_enabled = true;
        m_pipeline.m_cmf.m_enabled = true;
        m_pipeline.m_gridIIR.m_enabled = true;
        m_pipeline.m_tracker.m_enabled = true;
        m_pipeline.m_coordFilter.m_enabled = true;
        m_pipeline.m_gesture.m_enabled = true;
    }
    m_masterParserOnly = enabled;
}

// ── DVR export (async) ──
void ServiceProxy::TriggerDVRExport(bool heatmap, bool master, bool slave) {
    if (!m_dvrBuffer) return;
    if (m_dvrExporting.load()) return; // Already exporting

    // Join any completed previous export thread
    if (m_dvrThread.joinable()) m_dvrThread.join();

    m_dvrExporting.store(true);
    m_dvrThread = std::thread([this, heatmap, master, slave]() {
        // Snapshot under mutex (brief hold — copies data, then releases)
        auto frames = m_dvrBuffer->GetSnapshot();
        if (frames.empty()) {
            m_dvrExporting.store(false);
            return;
        }

        namespace fs = std::filesystem;
        auto now = std::chrono::system_clock::now();
        auto t   = std::chrono::system_clock::to_time_t(now);
        std::tm tm{}; localtime_s(&tm, &t);
        std::ostringstream ts;
        ts << std::put_time(&tm, "%Y%m%d_%H%M%S");
        std::string dir = "exports/dvr/dvr_" + ts.str();
        fs::create_directories(dir);

        // ── Helper: write standard DVR metadata header ──
        auto writeDvrHeader = [&](std::ostream& out, const char* sectionName, bool includeTouchXY = true) {
            out << "# DVR Export: " << sectionName << '\n';
            out << "# ExportTime: " << ts.str() << '\n';
            out << "# FrameCount: " << frames.size() << '\n';
            if (!frames.empty()) {
                out << "# FirstFrameTimestamp: " << frames.front().timestamp << '\n';
                out << "# LastFrameTimestamp: "  << frames.back().timestamp << '\n';
                // Master Suffix snapshot from first frame
                const auto& ms = frames.front().masterSuffix;
                out << "# MasterSuffix.freqShiftDone: "  << ms.words[Frame::MasterWord::kFreqShiftDone] << '\n';
                out << "# MasterSuffix.tpFreq1: "        << ms.words[Frame::MasterWord::kTpFreq1] << '\n';
                out << "# MasterSuffix.tpFreq2: "        << ms.words[Frame::MasterWord::kTpFreq2] << '\n';
                out << "# MasterSuffix.penF0NoiseCount: " << ms.words[Frame::MasterWord::kPenF0NoiseCount] << '\n';
                out << "# MasterSuffix.penF1NoiseCount: " << ms.words[Frame::MasterWord::kPenF1NoiseCount] << '\n';
                if (includeTouchXY) {
                    out << "# MasterSuffix.touchX: "      << ms.words[Frame::MasterWord::kTouchX] << '\n';
                    out << "# MasterSuffix.touchY: "      << ms.words[Frame::MasterWord::kTouchY] << '\n';
                }
            }
        };

        // ── Heatmap: per-frame 40x60 matrix CSV ──
        if (heatmap) {
            for (size_t i = 0; i < frames.size(); ++i) {
                std::ofstream f(dir + "/frame_" + std::to_string(i) + ".csv");
                // Per-frame header with that frame's master suffix
                f << "# Frame " << i << " | Timestamp: " << frames[i].timestamp << '\n';
                const auto& ms = frames[i].masterSuffix;
                f << "# freqShiftDone=" << ms.words[Frame::MasterWord::kFreqShiftDone]
                  << " tpFreq1=" << ms.words[Frame::MasterWord::kTpFreq1]
                  << " tpFreq2=" << ms.words[Frame::MasterWord::kTpFreq2]
                  << " F0Noise=" << ms.words[Frame::MasterWord::kPenF0NoiseCount]
                  << " F1Noise=" << ms.words[Frame::MasterWord::kPenF1NoiseCount]
                  << " touchXY=(" << ms.words[Frame::MasterWord::kTouchX]
                  << "," << ms.words[Frame::MasterWord::kTouchY] << ")\n";
                for (int r = 0; r < 40; ++r) {
                    for (int c = 0; c < 60; ++c) {
                        if (c) f << ',';
                        f << frames[i].heatmapMatrix[r][c];
                    }
                    f << '\n';
                }
            }
        }

        // ── Master status: contacts/peaks summary per frame ──
        if (master) {
            std::ofstream mf(dir + "/master_status.csv");
            writeDvrHeader(mf, "master_status");
            mf << "Frame,Timestamp,MasterWasRead,PeakCount,ContactCount,"
                  "P0_R,P0_C,P0_Z,P1_R,P1_C,P1_Z,"
                  "C0_ID,C0_X,C0_Y,C0_State,C0_Area,C0_SigSum,"
                  "C1_ID,C1_X,C1_Y,C1_State,C1_Area,C1_SigSum\n";
            for (size_t i = 0; i < frames.size(); ++i) {
                const auto& fr = frames[i];
                mf << i << ',' << fr.timestamp << ','
                   << (fr.masterWasRead ? 1 : 0) << ','
                   << static_cast<int>(fr.peakCount) << ',' << static_cast<int>(fr.contactCount);
                // Up to 2 peaks
                for (int p = 0; p < 2; ++p) {
                    if (p < fr.peakCount) {
                        mf << ',' << fr.peaks[p].r << ',' << fr.peaks[p].c
                           << ',' << fr.peaks[p].z;
                    } else {
                        mf << ",,,";
                    }
                }
                // Up to 2 contacts
                for (int ci = 0; ci < 2; ++ci) {
                    if (ci < fr.contactCount) {
                        const auto& ct = fr.contacts[ci];
                        mf << ',' << ct.id << ',' << ct.x << ',' << ct.y
                           << ',' << ct.state << ',' << ct.area << ',' << ct.signalSum;
                    } else {
                        mf << ",,,,,,";
                    }
                }
                mf << '\n';
            }
        }

        // ── Slave status: raw 166-word suffix per frame ──
        if (slave) {
            std::ofstream wf(dir + "/slave_suffix.csv");
            writeDvrHeader(wf, "slave_suffix", false);
            wf << "Frame,Timestamp";
            for (int w = 0; w < 166; ++w) {
                if (w == 0) wf << ",TX1_Y";
                else if (w == 1) wf << ",TX1_X";
                else if (w == 83) wf << ",TX2_Y";
                else if (w == 84) wf << ",TX2_X";
                else wf << ",W" << w;
            }
            wf << '\n';
            for (size_t i = 0; i < frames.size(); ++i) {
                wf << i << ',' << frames[i].timestamp;
                if (frames[i].slaveSuffixValid) {
                    for (int w = 0; w < Frame::kSlaveSuffixWords; ++w) {
                        wf << ',' << frames[i].slaveSuffix.words[w];
                    }
                } else {
                    for (int w = 0; w < Frame::kSlaveSuffixWords; ++w) wf << ",0";
                }
                wf << '\n';
            }
        }

        LOG_INFO("App", "TriggerDVRExport", "IPC",
                 "Exported {} frames to {}/ (heatmap={}, master={}, slave={})",
                 frames.size(), dir, heatmap, master, slave);
        m_dvrExporting.store(false);
    });
}

// ── Poll loop with FPS measurement ──
void ServiceProxy::PollLoop() {

    uint64_t lastFpsFrameId = m_frameReader.LastFrameId();
    uint64_t lastSlaveFpsFrameId = m_frameReader.LastSlaveFrameId();
    uint64_t lastMasterFpsFrameId = m_frameReader.LastMasterFrameId();
    auto lastFpsTick = std::chrono::steady_clock::now();
    auto lastLogPoll = std::chrono::steady_clock::now();
    auto lastPenPoll = std::chrono::steady_clock::now();
    HANDLE frameEvent = m_frameReader.FrameReadyEvent();
    HANDLE stopEvent = m_pollStopEvent;
    while (m_polling.load()) {
        auto now = std::chrono::steady_clock::now();
        auto nextLogDue = lastLogPoll + std::chrono::milliseconds(1000);
        auto nextPenDue = lastPenPoll + std::chrono::milliseconds(500);
        auto nextDue = (nextLogDue < nextPenDue) ? nextLogDue : nextPenDue;
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
                        m_hasNewFrame.store(true, std::memory_order_release);
                        gotFrame = true;
                    }
                }
                if (gotFrame && m_dvrBuffer) {
                    Dvr::DvrFrameSlot slot;
                    slot.CopyFrom(m_latestFrame);
                    m_dvrBuffer->PushOverwriting(slot);
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
                std::lock_guard<std::mutex> lk(m_penMutex);
                m_penStatus = s;
            }
            lastPenPoll = now;
        }
    }
}

} // namespace App
