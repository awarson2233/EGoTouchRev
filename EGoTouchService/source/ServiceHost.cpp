#include "ServiceHost.h"
#include "GuiLogSink.h"
#include "Logger.h"
#include "IpcProtocol.h"
#include <fstream>
#include <string>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string_view>

// Engine Pipeline Processors
#include "TouchSolver/TouchPipeline.h"

namespace Service {

// ── 固化路径 ──
static const std::string kConfigPath  = "C:/ProgramData/EGoTouchRev/config.ini";

// ── 设备路径 ──
static const std::wstring kDevicePathMaster    = L"\\\\.\\Global\\SPBTESTTOOL_MASTER";
static const std::wstring kDevicePathSlave     = L"\\\\.\\Global\\SPBTESTTOOL_SLAVE";
static const std::wstring kDevicePathInterrupt = L"\\\\.\\Global\\SPBTESTTOOL_MASTER";

namespace {

struct LoadPipelineConfigResult {
    bool fileOpened = false;
    bool migrated = false;
    bool touchLoaded = false;
    bool stylusLoaded = false;
};

std::string TrimCopy(std::string_view input) {
    const size_t start = input.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) return {};
    const size_t end = input.find_last_not_of(" \t\r\n");
    return std::string(input.substr(start, end - start + 1));
}

std::string ToLowerCopy(std::string_view input) {
    std::string out;
    out.reserve(input.size());
    for (char ch : input) {
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return out;
}

bool ParseBoolValue(std::string_view value) {
    const std::string lowered = ToLowerCopy(TrimCopy(value));
    return lowered == "1" || lowered == "true" || lowered == "yes" || lowered == "on";
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

const char* ServiceModeToConfig(ServiceMode mode) {
    return mode == ServiceMode::Full ? "full" : "touch_only";
}

std::string BuildBackupPath(const std::string& configPath) {
    namespace fs = std::filesystem;
    const fs::path source(configPath);
    std::time_t now = std::time(nullptr);
    std::tm localTm{};
    localtime_s(&localTm, &now);

    std::ostringstream stamp;
    stamp << std::put_time(&localTm, "%Y%m%d-%H%M%S");

    const std::string backupName =
        source.stem().string() + ".legacy-" + stamp.str() + source.extension().string() + ".bak";
    return (source.parent_path() / backupName).string();
}

bool BackupConfigFile(const std::string& configPath, std::string& outBackupPath) {
    namespace fs = std::filesystem;
    const std::string backupPath = BuildBackupPath(configPath);
    std::error_code ec;
    fs::copy_file(configPath, backupPath, fs::copy_options::overwrite_existing, ec);
    if (ec) return false;
    outBackupPath = backupPath;
    return true;
}

bool WriteCanonicalConfig(const std::string& configPath,
                          ServiceMode mode,
                          bool autoMode,
                          bool stylusVhfEnabled,
                          const Solvers::TouchPipeline& touchPipe,
                          const Solvers::StylusPipeline& stylusPipe) {
    std::ofstream out(configPath, std::ios::trunc);
    if (!out.is_open()) return false;

    out << "[Service]\n";
    out << "mode=" << ServiceModeToConfig(mode) << "\n";
    out << "auto_mode=" << (autoMode ? "1" : "0") << "\n";
    out << "stylus_vhf_enabled=" << (stylusVhfEnabled ? "1" : "0") << "\n\n";

    out << "[TouchPipeline]\n";
    touchPipe.SaveConfig(out);
    out << "\n";

    out << "[StylusPipeline]\n";
    stylusPipe.SaveConfig(out);
    out << "\n";
    return true;
}

} // namespace

ServiceHost::~ServiceHost() {
    Stop();
}

// ── 模式解析 ──────────────────────────────────────────
void ServiceHost::ParseServiceConfig(const std::string& configPath) {
    std::ifstream cfg(configPath);
    if (!cfg.is_open()) {
        m_mode = ServiceMode::Full;
        m_autoMode = true;
        m_stylusVhfEnabled = true;
        return;
    }

    std::string line;
    bool inServiceSection = false;
    while (std::getline(cfg, line)) {
        const std::string trimmed = TrimCopy(line);
        if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#') continue;
        if (trimmed.front() == '[' && trimmed.back() == ']') {
            inServiceSection = (trimmed == "[Service]");
            continue;
        }
        if (!inServiceSection) continue;

        std::string key;
        std::string val;
        if (!ParseIniKeyValue(trimmed, key, val)) continue;

        if (key == "mode") {
            m_mode = (val == "touch_only") ? ServiceMode::TouchOnly : ServiceMode::Full;
        } else if (key == "auto_mode") {
            m_autoMode = ParseBoolValue(val);
        } else if (key == "stylus_vhf_enabled") {
            m_stylusVhfEnabled = ParseBoolValue(val);
        }
    }
}

bool ServiceHost::Start() {
    // ── 0. 解析运行模式 ──────────────────────────────────
    ParseServiceConfig(kConfigPath);
    LOG_INFO("Service", __func__, "Boot", "Service mode: {}, AutoMode: {}", m_mode == ServiceMode::Full ? "full" : "touch_only", m_autoMode);

    // ── 1. DeviceRuntime（先创建，后续模块依赖它） ─────────
    m_deviceRuntime = std::make_unique<DeviceRuntime>(
        kDevicePathMaster, kDevicePathSlave, kDevicePathInterrupt);
    m_deviceRuntime->SetAutoMode(m_autoMode);
    m_deviceRuntime->SetStylusVhfEnabled(m_stylusVhfEnabled);
    BuildDefaultPipeline(kConfigPath);

    if (!m_deviceRuntime->Start()) {
        LOG_ERROR("Service", __func__, "Boot", "DeviceRuntime::Start() failed.");
        return false;
    }
    LOG_INFO("Service", __func__, "Boot", "DeviceRuntime started (auto mode).");

    // ── 2. SystemStateMonitor（事件 → DeviceRuntime 命令队列） ─
    m_sysMonitor = std::make_unique<Host::SystemStateMonitor>();
    bool monitorOk = m_sysMonitor->Start(
        [this](const Host::SystemStateEvent& ev) {
            LOG_INFO("Service", __func__, "Event", "System event: type={}", Host::ToString(ev.type));
            m_deviceRuntime->IngestSystemEvent(ev);
        });

    if (!monitorOk) {
        LOG_WARN("Service", __func__, "Monitor", "SystemStateMonitor failed to start; running without system state detection.");
        m_sysMonitor.reset();
    } else {
        LOG_INFO("Service", __func__, "Monitor", "SystemStateMonitor started.");
    }

#ifdef _DEBUG
    // ── 3. Shared Memory (Service creates Global\\ mapping, debug only) ──
    if (!m_frameWriter.Create(Ipc::kSharedFrameName)) {
        LOG_WARN("Service", __func__, "IPC", "Failed to create shared memory; App debug will be disabled.");
    } else {
        LOG_INFO("Service", __func__, "IPC", "Shared memory created for App connection.");
    }
#endif

    // ── 4. IPC Pipe Server ──────────────────────────────────
    // Create log/pen status events for App (cross-session)
    {
        SECURITY_DESCRIPTOR sd{};
        InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(&sd, TRUE, nullptr, FALSE);
        SECURITY_ATTRIBUTES sa{};
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = &sd;
        sa.bInheritHandle = FALSE;
        m_logEvent = CreateEventW(&sa, FALSE, FALSE, Ipc::kLogReadyEventName);
        if (!m_logEvent) {
            LOG_WARN("Service", __func__, "IPC", "CreateEvent failed for LogReadyEvent: {}", GetLastError());
        } else {
            Common::GuiLogSink::Instance()->SetNotifyEvent(m_logEvent);
        }
        m_penEvent = CreateEventW(&sa, FALSE, FALSE, Ipc::kPenReadyEventName);
        if (!m_penEvent) {
            LOG_WARN("Service", __func__, "IPC", "CreateEvent failed for PenReadyEvent: {}", GetLastError());
        }
    }
    m_configDirty.Open();
    m_ipcServer.SetCommandHandler(
        [this](const Ipc::IpcRequest& req) {
            return HandleIpcCommand(req);
        });
    m_ipcServer.Start();
    LOG_INFO("Service", __func__, "Boot", "IPC pipe server started.");

    LOG_INFO("Service", __func__, "Boot", "All modules started.");

    // ── 4. BT MCU（仅 Full 模式启用） ─────────────────────
    if (m_mode == ServiceMode::Full) {
        // ── 4a. 事件通道 (col00): 握手 + ACK + 0x7D01 回显 ──
        m_penEventBridge = std::make_unique<Himax::Pen::PenEventBridge>();
        if (m_penEvent) m_penEventBridge->SetNotifyEvent(m_penEvent);
        m_penEventBridge->SetEventCallback(
            [this](const Himax::Pen::PenEvent& ev) {
                if (m_deviceRuntime)
                    m_deviceRuntime->OnPenEvent(ev);
            });
        m_penEventBridge->Start();
        LOG_INFO("Service", __func__, "MCU", "PenEventBridge started (col00 event channel).");

        // BT 笔切频命令发送器：DeviceRuntime 在检测到频率不匹配时调用
        if (m_deviceRuntime) {
            m_deviceRuntime->SetBtScanModeSender(
                [this](uint8_t freq1, uint8_t freq2) -> bool {
                    if (!m_penEventBridge) return false;
                    return m_penEventBridge->SendScanMode(freq1, freq2);
                });
            LOG_INFO("Service", __func__, "MCU", "BtScanModeSender injected via PenEventBridge (col00).");
        }

        // ── 4b. 压力通道 (col01): 'U' 报文频率 + 压感 ──────
        m_penPressureReader = std::make_unique<Himax::Pen::PenPressureReader>();
        if (m_penEvent) m_penPressureReader->SetNotifyEvent(m_penEvent);
        m_penPressureReader->SetPressureCallback(
            [this](uint16_t press) {
                if (m_deviceRuntime) m_deviceRuntime->SetBtMcuPressure(press);
            });

        // BT 频率提供者：DeviceRuntime 每帧 poll 获取最新 BT MCU 频率
        if (m_deviceRuntime) {
            m_deviceRuntime->SetBtFreqProvider(
                [this]() -> std::pair<uint8_t, uint8_t> {
                    if (!m_penPressureReader) return {0, 0};
                    auto s = m_penPressureReader->GetPressureStats();
                    return {s.freq1, s.freq2};
                });
        }

        m_penPressureReader->Start();
        LOG_INFO("Service", __func__, "MCU", "PenPressureReader started (col01 pressure channel).");
    } else {
        LOG_INFO("Service", __func__, "MCU", "Pen modules skipped (touch_only mode).");
    }

    return true;
}

void ServiceHost::Stop() {
    // 逆序停止（后启动的先停止）

    m_ipcServer.Stop();
    m_frameWriter.Close();
    m_configDirty.Close();
    m_debugMode = false;
    if (m_logEvent) {
        Common::GuiLogSink::Instance()->SetNotifyEvent(nullptr);
        CloseHandle(m_logEvent);
        m_logEvent = nullptr;
    }
    if (m_penEvent) {
        CloseHandle(m_penEvent);
        m_penEvent = nullptr;
    }

    // Pen 通道（先停，避免回调中仍访问 DeviceRuntime）
    if (m_penPressureReader) {
        m_penPressureReader->Stop();
        m_penPressureReader.reset();
        LOG_INFO("Service", __func__, "MCU", "PenPressureReader stopped.");
    }
    if (m_penEventBridge) {
        m_penEventBridge->Stop();
        m_penEventBridge.reset();
        LOG_INFO("Service", __func__, "MCU", "PenEventBridge stopped.");
    }

    if (m_sysMonitor) {

        m_sysMonitor->Stop();
        m_sysMonitor.reset();
        LOG_INFO("Service", __func__, "Monitor", "SystemStateMonitor stopped.");
    }

    if (m_deviceRuntime) {
        m_deviceRuntime->Stop();
        m_deviceRuntime.reset();
        LOG_INFO("Service", __func__, "Device", "DeviceRuntime stopped.");
    }

    LOG_INFO("Service", __func__, "Shutdown", "All modules stopped.");
}

// ── Shared config loader ────────────────────────────────────────────
static LoadPipelineConfigResult LoadPipelineConfig(
    const std::string& configPath,
    Solvers::TouchPipeline& touchPipe,
    Solvers::StylusPipeline* stylusPipe = nullptr)
{
    LoadPipelineConfigResult result;
    std::ifstream in(configPath);
    if (!in.is_open()) return result;
    result.fileOpened = true;

    std::string line, section;
    while (std::getline(in, line)) {
        const std::string trimmed = TrimCopy(line);
        if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#') continue;
        if (trimmed.front() == '[' && trimmed.back() == ']') {
            section = TrimCopy(std::string_view(trimmed).substr(1, trimmed.size() - 2));
            continue;
        }

        std::string key;
        std::string val;
        if (!ParseIniKeyValue(trimmed, key, val)) continue;

        if (section == "TouchPipeline") {
            touchPipe.LoadConfig(key, val);
            result.touchLoaded = true;
            continue;
        }
        if (stylusPipe && section == "StylusPipeline") {
            stylusPipe->LoadConfig(key, val);
            result.stylusLoaded = true;
            continue;
        }
        if (IsLegacyTouchSection(section)) {
            const auto mappedKey = MapLegacyTouchKey(section, key);
            if (!mappedKey.has_value()) continue;
            touchPipe.LoadConfig(*mappedKey, val);
            result.touchLoaded = true;
            result.migrated = true;
        }
    }
    return result;
}

// ── Pipeline 构建 ──────────────────────────────
void ServiceHost::BuildDefaultPipeline(const std::string& configPath) {
    // TouchPipeline is self-contained: no processor registration needed.
    // Just load config.
    auto& tp = m_deviceRuntime->GetPipeline();
    auto& sp = m_deviceRuntime->GetStylusPipeline();
    LOG_INFO("Service", __func__, "Boot", "TouchPipeline initialized (linear orchestrator).");

    // Load saved config
    const auto loadResult = LoadPipelineConfig(configPath, tp, &sp);
    if (!loadResult.fileOpened) {
        LOG_WARN("Service", __func__, "Boot", "Config file not found: {}", configPath);
        return;
    }

    if (loadResult.migrated) {
        std::string backupPath;
        if (BackupConfigFile(configPath, backupPath)) {
            if (WriteCanonicalConfig(configPath, m_mode, m_autoMode, m_stylusVhfEnabled, tp, sp)) {
                LOG_INFO("Service", __func__, "Boot",
                         "Migrated legacy pipeline config from {} and rewrote canonical sections. Backup: {}",
                         configPath, backupPath);
            } else {
                LOG_WARN("Service", __func__, "Boot",
                         "Loaded legacy pipeline config from {} but failed to rewrite canonical config.",
                         configPath);
            }
        } else {
            LOG_WARN("Service", __func__, "Boot",
                     "Loaded legacy pipeline config from {} but failed to create backup before migration.",
                     configPath);
        }
    } else {
        LOG_INFO("Service", __func__, "Boot", "Loaded canonical config from {}.", configPath);
    }

    if (!loadResult.touchLoaded && !loadResult.stylusLoaded) {
        LOG_WARN("Service", __func__, "Boot",
                 "Config file {} opened, but no pipeline sections were applied; keeping defaults.",
                 configPath);
    }
}

// ── IPC Command Handler ──────────────────────────────
Ipc::IpcResponse ServiceHost::HandleIpcCommand(
        const Ipc::IpcRequest& req) {
    Ipc::IpcResponse resp{};
    switch (req.command) {
    case Ipc::IpcCommand::Ping:
        resp.success = true;
        break;

    case Ipc::IpcCommand::EnterDebugMode: {
#ifdef _DEBUG
        // Shared memory is already created at startup.
        // Just activate the frame push callback.
        if (m_frameWriter.IsOpen()) {
            m_deviceRuntime->SetFramePushCallback(
                [this](const Solvers::HeatmapFrame& f) {
                    m_frameWriter.Write(f);
                });
            m_debugMode = true;
            resp.success = true;
            LOG_INFO("Service", __func__, "IPC", "Entered debug mode.");
        } else {
            LOG_ERROR("Service", __func__, "IPC", "EnterDebugMode rejected: shared memory not available.");
        }
#else
        LOG_WARN("Service", __func__, "IPC", "EnterDebugMode not available in release build.");
#endif
        break;
    }

    case Ipc::IpcCommand::ExitDebugMode:
#ifdef _DEBUG
        m_deviceRuntime->SetFramePushCallback(nullptr);
        m_debugMode = false;
#endif
        resp.success = true;
        LOG_INFO("Service", __func__, "IPC", "Exited debug mode.");
        break;

    case Ipc::IpcCommand::AfeCommand:
        if (req.paramLen >= 2 && m_deviceRuntime) {
            command cmd{};
            cmd.type = static_cast<AFE_Command>(req.param[0]);
            cmd.param = req.param[1];
            m_deviceRuntime->SubmitCommand(
                cmd, CommandSource::External, "IPC AFE");
            resp.success = true;
        }
        break;

    case Ipc::IpcCommand::StartRuntime:
        if (m_deviceRuntime) {
            resp.success = m_deviceRuntime->Start();
        }
        break;

    case Ipc::IpcCommand::StopRuntime:
        if (m_deviceRuntime) {
            m_deviceRuntime->Stop();
            resp.success = true;
        }
        break;

    case Ipc::IpcCommand::ReloadConfig:
        if (m_deviceRuntime) {
            // First, re-read [Service] global configs
            ParseServiceConfig(kConfigPath);
            m_deviceRuntime->SetStylusVhfEnabled(m_stylusVhfEnabled);

            auto& tp = m_deviceRuntime->GetPipeline();
            auto& sp = m_deviceRuntime->GetStylusPipeline();
            const auto loadResult = LoadPipelineConfig(kConfigPath, tp, &sp);
            if (loadResult.fileOpened) {
                if (loadResult.migrated) {
                    std::string backupPath;
                    if (BackupConfigFile(kConfigPath, backupPath)) {
                        if (WriteCanonicalConfig(kConfigPath, m_mode, m_autoMode, m_stylusVhfEnabled, tp, sp)) {
                            LOG_INFO("Service", __func__, "IPC",
                                     "Reloaded legacy config from {} and rewrote canonical sections. Backup: {}",
                                     kConfigPath, backupPath);
                        } else {
                            LOG_WARN("Service", __func__, "IPC",
                                     "Reloaded legacy config from {} but failed to rewrite canonical config.",
                                     kConfigPath);
                        }
                    } else {
                        LOG_WARN("Service", __func__, "IPC",
                                 "Reloaded legacy config from {} but failed to create backup before migration.",
                                 kConfigPath);
                    }
                } else {
                    LOG_INFO("Service", __func__, "IPC", "Config reloaded from {}.", kConfigPath);
                }
                resp.success = true;
            }
        }
        break;

    case Ipc::IpcCommand::SaveConfig:
        if (m_deviceRuntime) {
            auto& tp = m_deviceRuntime->GetPipeline();
            auto& sp = m_deviceRuntime->GetStylusPipeline();
            if (WriteCanonicalConfig(kConfigPath, m_mode, m_autoMode, m_stylusVhfEnabled, tp, sp)) {
                resp.success = true;
                LOG_INFO("Service", __func__, "IPC", "Config saved to {}.", kConfigPath);
            }
        }
        break;

    case Ipc::IpcCommand::SetVhfEnabled:
        if (m_deviceRuntime && req.paramLen >= 1) {
            m_deviceRuntime->GetVhfReporter().SetEnabled(req.param[0] != 0);
            resp.success = true;
        }
        break;

    case Ipc::IpcCommand::SetVhfTranspose:
        if (m_deviceRuntime && req.paramLen >= 1) {
            m_deviceRuntime->GetVhfReporter().SetTransposeEnabled(req.param[0] != 0);
            resp.success = true;
        }
        break;

    case Ipc::IpcCommand::SetAutoAfeSync:
        resp.success = true; // placeholder — future DeviceRuntime integration
        break;

    case Ipc::IpcCommand::GetLogs: {
        auto lines = Common::GuiLogSink::Instance()->DrainNewLines();
        std::string packed;
        for (const auto& l : lines) {
            if (packed.size() + l.size() + 1 > sizeof(resp.data)) break;
            packed += l;
            packed += '\n';
        }
        resp.dataLen = static_cast<uint16_t>(
            std::min(packed.size(), sizeof(resp.data)));
        std::memcpy(resp.data, packed.data(), resp.dataLen);
        resp.success = true;
        break;
    }

    case Ipc::IpcCommand::GetPenBridgeStatus: {
        // Pack: [evtRunning:1][pressRunning:1][reportType:1][freq1:1][freq2:1]
        //       [p0L:1][p0H:1][p1L:1][p1H:1][p2L:1][p2H:1][p3L:1][p3H:1]
        // Total: 13 bytes
        uint8_t buf[13] = {};
        buf[0] = (m_penEventBridge && m_penEventBridge->IsRunning()) ? 1 : 0;
        buf[1] = (m_penPressureReader && m_penPressureReader->IsRunning()) ? 1 : 0;
        if (m_penPressureReader) {
            auto s = m_penPressureReader->GetPressureStats();
            buf[2]  = s.reportType;
            buf[3]  = s.freq1;
            buf[4]  = s.freq2;
            for (int k = 0; k < 4; ++k) {
                buf[5 + k * 2]     = static_cast<uint8_t>(s.press[k] & 0xFF);
                buf[5 + k * 2 + 1] = static_cast<uint8_t>(s.press[k] >> 8);
            }
        }
        std::memcpy(resp.data, buf, sizeof(buf));
        resp.dataLen = sizeof(buf);
        resp.success = true;
        break;
    }

    default:
        break;
    }
    return resp;
}

} // namespace Service
