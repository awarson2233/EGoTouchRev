#include "ServiceProxy.h"
#include "Logger.h"
#include "GuiLogSink.h"
#include "IFrameProcessor.h"
#include "IpcProtocol.h"
#include <sstream>

// Pipeline Processors (local copy for GUI parameter editing)
#include "MasterFrameParser.h"
#include "BaselineSubtraction.h"
#include "CMFProcessor.h"
#include "GridIIRProcessor.h"
#include "FeatureExtractor.h"
#include "TouchTracker.h"
#include "CoordinateFilter.h"
#include "TouchGestureStateMachine.h"
#include <chrono>
#include <fstream>
#include <string>
#include <iomanip>
#include <sstream>
#include <filesystem>

namespace App {

static const std::string kConfigPath = "C:/ProgramData/EGoTouchRev/config.ini";

ServiceProxy::ServiceProxy()
    : m_dvrBuffer(std::make_unique<RingBuffer<Engine::HeatmapFrame, 480>>()) {
    // Build local pipeline (mirrors Service pipeline for GUI config editing)
    m_pipeline.AddProcessor(std::make_unique<Engine::MasterFrameParser>());
    m_pipeline.AddProcessor(std::make_unique<Engine::BaselineSubtraction>());
    m_pipeline.AddProcessor(std::make_unique<Engine::CMFProcessor>());
    m_pipeline.AddProcessor(std::make_unique<Engine::GridIIRProcessor>());
    m_pipeline.AddProcessor(std::make_unique<Engine::FeatureExtractor>());
    // NOTE: StylusProcessor removed — stylus is now handled by
    // independent StylusPipeline in DeviceRuntime.
    m_pipeline.AddProcessor(std::make_unique<Engine::TouchTracker>());
    m_pipeline.AddProcessor(std::make_unique<Engine::CoordinateFilter>());
    m_pipeline.AddProcessor(std::make_unique<Engine::TouchGestureStateMachine>());
    LoadConfig();
}

ServiceProxy::~ServiceProxy() {
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

bool ServiceProxy::GetLatestFrame(Engine::HeatmapFrame& out) {
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
    for (auto& p : m_pipeline.GetProcessors()) {
        out << "[" << p->GetName() << "]\n";
        p->SaveConfig(out);
        out << "\n";
    }
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
    Engine::IFrameProcessor* cur = nullptr;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == ';') continue;
        if (line.front() == '[' && line.back() == ']') {
            section = line.substr(1, line.size() - 2);
            cur = nullptr;
            for (auto& p : m_pipeline.GetProcessors()) {
                if (p->GetName() == section) {
                    cur = p.get(); break;
                }
            }
        } else if (section == "Service") {
            auto eq = line.find('=');
            if (eq != std::string::npos) {
                std::string k = line.substr(0, eq);
                std::string v = line.substr(eq + 1);
                if (k == "mode") m_srvModeFull = (v == "full");
                else if (k == "auto_mode") m_srvAutoMode = (v == "1" || v == "true");
                else if (k == "stylus_vhf_enabled") m_srvStylusVhfEnabled = (v == "1" || v == "true");
            }
        } else if (section == "StylusPipeline") {
            auto eq = line.find('=');
            if (eq != std::string::npos) {
                m_stylusPipeline.LoadConfig(
                    line.substr(0, eq), line.substr(eq + 1));
            }
        } else if (cur) {
            auto eq = line.find('=');
            if (eq != std::string::npos) {
                cur->LoadConfig(line.substr(0, eq),
                                line.substr(eq + 1));
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
    auto& procs = m_pipeline.GetProcessors();
    if (enabled && !m_masterParserOnly) {
        m_savedProcessorStates.clear();
        for (size_t i = 0; i < procs.size(); ++i) {
            m_savedProcessorStates.push_back(procs[i]->IsEnabled());
            if (i > 0) procs[i]->SetEnabled(false);
        }
    } else if (!enabled && m_masterParserOnly) {
        for (size_t i = 0; i < procs.size() &&
             i < m_savedProcessorStates.size(); ++i) {
            procs[i]->SetEnabled(m_savedProcessorStates[i]);
        }
    }
    m_masterParserOnly = enabled;
}

// ── DVR export (local) ──
void ServiceProxy::TriggerDVRExport(bool heatmap, bool master, bool slave) {
    if (!m_dvrBuffer) return;
    namespace fs = std::filesystem;
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{}; localtime_s(&tm, &t);
    std::ostringstream ts;
    ts << std::put_time(&tm, "%Y%m%d_%H%M%S");
    std::string dir = "dvr_" + ts.str();
    fs::create_directories(dir);
    // Snapshot frames from ring buffer
    auto frames = m_dvrBuffer->GetSnapshot();
    for (size_t i = 0; i < frames.size(); ++i) {
        if (heatmap) {
            std::ofstream f(dir + "/frame_" + std::to_string(i) + ".csv");
            for (int r = 0; r < 40; ++r) {
                for (int c = 0; c < 60; ++c) {
                    if (c) f << ',';
                    f << frames[i].heatmapMatrix[r][c];
                }
                f << '\n';
            }
        }
    }
    LOG_INFO("App", __func__, "IPC", "Exported {} frames to {}/", frames.size(), dir);
}

// ── Poll loop with FPS measurement ──
void ServiceProxy::PollLoop() {
    m_latestFrame.rawData.reserve(5402);

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
                    m_dvrBuffer->PushOverwriting(m_latestFrame);
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
