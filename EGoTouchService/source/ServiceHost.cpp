#include "ServiceHost.h"
#include "GuiLogSink.h"
#include "Logger.h"
#include "IpcProtocol.h"
#include "IFrameProcessor.h"
#include <fstream>
#include <string>
#include <algorithm>

// Engine Pipeline Processors
#include "MasterFrameParser.h"
#include "BaselineSubtraction.h"
#include "CMFProcessor.h"
#include "GridIIRProcessor.h"
#include "FeatureExtractor.h"
#include "TouchTracker.h"
#include "CoordinateFilter.h"
#include "TouchGestureStateMachine.h"

namespace Service {

// ── 固化路径 ──
static const std::string kConfigPath  = "C:/ProgramData/EGoTouchRev/config.ini";

// ── 设备路径 ──
static const std::wstring kDevicePathMaster    = L"\\\\.\\Global\\SPBTESTTOOL_MASTER";
static const std::wstring kDevicePathSlave     = L"\\\\.\\Global\\SPBTESTTOOL_SLAVE";
static const std::wstring kDevicePathInterrupt = L"\\\\.\\Global\\SPBTESTTOOL_MASTER";

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
        if (line.empty() || line[0] == ';') continue;
        if (line.front() == '[' && line.back() == ']') {
            inServiceSection = (line == "[Service]");
            continue;
        }
        if (inServiceSection) {
            auto eq = line.find('=');
            if (eq != std::string::npos) {
                std::string key = line.substr(0, eq);
                std::string val = line.substr(eq + 1);
                // trim whitespace
                val.erase(0, val.find_first_not_of(" \t\r\n"));
                val.erase(val.find_last_not_of(" \t\r\n") + 1);

                if (key == "mode") {
                    if (val == "touch_only") m_mode = ServiceMode::TouchOnly;
                    else m_mode = ServiceMode::Full;
                } else if (key == "auto_mode") {
                    m_autoMode = (val != "0" && val != "false");
                } else if (key == "stylus_vhf_enabled") {
                    m_stylusVhfEnabled = (val != "0" && val != "false");
                }
            }
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
    m_deviceRuntime->SetTouchOnlyMode(m_mode == ServiceMode::TouchOnly);
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
                using EC = Himax::Pen::PenUsbEventCode;
                switch (ev.code) {

                case EC::PenConnStatus: {
                    const bool connected = !ev.payload.empty() && ev.payload[0] != 0;
                    LOG_INFO("Service", __func__, "MCU", "PenConnStatus: {}.", connected ? "connected" : "disconnected");
                    if (m_deviceRuntime) {
                        command cmd{};
                        if (connected) {
                            cmd.type  = AFE_Command::InitStylus;
                            cmd.param = 0;  // freq pair由0x73 SetStylusId绑定
                            m_deviceRuntime->SubmitCommand(
                                cmd, CommandSource::SystemPolicy, "PenConnStatus→Init");
                        } else {
                            cmd.type  = AFE_Command::DisconnectStylus;
                            cmd.param = 0;
                            m_deviceRuntime->SubmitCommand(
                                cmd, CommandSource::SystemPolicy, "PenConnStatus→Disconnect");
                        }
                    }
                    break;
                }

                case EC::PenFreqJump: {
                    std::string hexDump;
                    for (size_t i = 0; i < ev.payload.size(); ++i) {
                        char buf[8];
                        snprintf(buf, sizeof(buf), "%02X ", ev.payload[i]);
                        hexDump += buf;
                    }
                    LOG_INFO("Service", __func__, "MCU", "PenFreqJump (payload[{}]: {})", ev.payload.size(), hexDump);

                    if (m_deviceRuntime) {
                        command cmd{};
                        cmd.type  = AFE_Command::EnableFreqShift;
                        cmd.param = 0;
                        m_deviceRuntime->SubmitCommand(
                            cmd, CommandSource::SystemPolicy, "PenFreqJump");
                    }
                    break;
                }

                case EC::PenTypeInfo: {
                    // 0x73: 笔类型 → set_stylus_id(pen_type) 频率表绑定
                    uint8_t penType = ev.payload.empty() ? 0 : ev.payload[0];
                    LOG_INFO("Service", __func__, "MCU", "PenTypeInfo: pen_type={}.", penType);
                    if (m_deviceRuntime) {
                        command cmd{};
                        cmd.type  = AFE_Command::SetStylusId;
                        cmd.param = penType;
                        m_deviceRuntime->SubmitCommand(
                            cmd, CommandSource::SystemPolicy, "PenTypeInfo→SetStylusId");
                    }
                    break;
                }

                case EC::PenCurStatus: {
                    // 0x72: 笔工作模式 (1=书写, 2=悬停, 3=橡皮擦)
                    uint8_t mode = ev.payload.empty() ? 0 : ev.payload[0];
                    const char* modeStr = "unknown";
                    if (mode == 1) modeStr = "writing";
                    else if (mode == 2) modeStr = "hovering";
                    else if (mode == 3) modeStr = "eraser";
                    LOG_INFO("Service", __func__, "MCU", "PenCurStatus: mode={} ({}).", mode, modeStr);
                    break;
                }

                default:
                    LOG_INFO("Service", __func__, "MCU", "MCU event 0x{:02X} received.", static_cast<uint8_t>(ev.code));
                    break;
                }
            });
        m_penEventBridge->Start();
        LOG_INFO("Service", __func__, "MCU", "PenEventBridge started (col00 event channel).");

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

// ── Pipeline 构建 ──────────────────────────────
void ServiceHost::BuildDefaultPipeline(const std::string& configPath) {
    auto& pl = m_deviceRuntime->GetPipeline();
    pl.AddProcessor(std::make_unique<Engine::MasterFrameParser>());
    pl.AddProcessor(std::make_unique<Engine::BaselineSubtraction>());
    pl.AddProcessor(std::make_unique<Engine::CMFProcessor>());
    pl.AddProcessor(std::make_unique<Engine::GridIIRProcessor>());
    pl.AddProcessor(std::make_unique<Engine::FeatureExtractor>());
    pl.AddProcessor(std::make_unique<Engine::TouchTracker>());
    pl.AddProcessor(std::make_unique<Engine::CoordinateFilter>());
    pl.AddProcessor(std::make_unique<Engine::TouchGestureStateMachine>());
    LOG_INFO("Service", __func__, "Boot", "Registered {} pipeline processors.", pl.GetProcessors().size());

    // Load saved config
    std::ifstream cfgIn(configPath);
    if (cfgIn.is_open()) {
        std::string line, section;
        Engine::IFrameProcessor* cur = nullptr;
        while (std::getline(cfgIn, line)) {
            if (line.empty() || line[0] == ';') continue;
            if (line.front() == '[' && line.back() == ']') {
                section = line.substr(1, line.size() - 2);
                cur = nullptr;
                for (auto& p : pl.GetProcessors()) {
                    if (p->GetName() == section) {
                        cur = p.get(); break;
                    }
                }
            } else if (cur) {
                auto eq = line.find('=');
                if (eq != std::string::npos) {
                    cur->LoadConfig(line.substr(0, eq),
                                    line.substr(eq + 1));
                }
            }
        }
        LOG_INFO("Service", __func__, "Boot", "Loaded config from {}.", configPath);
    } else {
        LOG_WARN("Service", __func__, "Boot", "Config file not found: {}", configPath);
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
                [this](const Engine::HeatmapFrame& f) {
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
            // Apply touchonly mode and VHF stylus config dynamically to the runtime
            m_deviceRuntime->SetTouchOnlyMode(m_mode == ServiceMode::TouchOnly);
            m_deviceRuntime->SetStylusVhfEnabled(m_stylusVhfEnabled);

            auto& pl = m_deviceRuntime->GetPipeline();
            std::ifstream in(kConfigPath);
            if (in.is_open()) {
                std::string line, section;
                Engine::IFrameProcessor* cur = nullptr;
                while (std::getline(in, line)) {
                    if (line.empty() || line[0] == ';') continue;
                    if (line.front() == '[' && line.back() == ']') {
                        section = line.substr(1, line.size() - 2);
                        cur = nullptr;
                        for (auto& p : pl.GetProcessors()) {
                            if (p->GetName() == section) {
                                cur = p.get(); break;
                            }
                        }
                    } else if (cur) {
                        auto eq = line.find('=');
                        if (eq != std::string::npos) {
                            cur->LoadConfig(line.substr(0, eq),
                                            line.substr(eq + 1));
                        }
                    }
                }
                resp.success = true;
                LOG_INFO("Service", __func__, "IPC", "Config reloaded from {}.", kConfigPath);
            }
        }
        break;

    case Ipc::IpcCommand::SaveConfig:
        if (m_deviceRuntime) {
            auto& pl = m_deviceRuntime->GetPipeline();
            std::ofstream out(kConfigPath);
            if (out.is_open()) {
                // Preserve [Service] section
                out << "[Service]\n";
                out << "mode=" << (m_mode == ServiceMode::Full ? "full" : "touch_only") << "\n";
                out << "auto_mode=" << (m_autoMode ? "1" : "0") << "\n";
                out << "stylus_vhf_enabled=" << (m_stylusVhfEnabled ? "1" : "0") << "\n\n";
                for (auto& p : pl.GetProcessors()) {
                    out << "[" << p->GetName() << "]\n";
                    p->SaveConfig(out);
                    out << "\n";
                }
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
