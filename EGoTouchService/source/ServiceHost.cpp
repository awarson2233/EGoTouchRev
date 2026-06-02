#include "ServiceHost.h"

#include "SystemStateMonitor.h"
#include "runtime/DeviceRuntime.h"
#include "penevt/PenEventBridge.h"
#include "penpress/PenPressureReader.h"
#include "Ipc/IpcPipeServer.h"
#include "Ipc/IpcSecurity.h"
#include "Ipc/SharedFrameBuffer.h"
#include "Ipc/ConfigSync.h"
#include "SolverBuildConfig.h"
#include "SolverTypes.h"

#include "GuiLogSink.h"
#include "Logger.h"
#include "Ipc/IpcProtocol.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <ctime>
#include <utility>
#include <vector>

#if EGOTOUCH_CONFIG_ENABLED
static std::string g_overrideConfigPath;
static bool g_commandLineParsed = false;

static std::string WStringToUtf8(const std::wstring& input) {
    if (input.empty()) return {};
    const int required = WideCharToMultiByte(CP_UTF8, 0, input.data(), static_cast<int>(input.size()), nullptr, 0, nullptr, nullptr);
    if (required <= 0) return {};
    std::string output(static_cast<size_t>(required), '\0');
    WideCharToMultiByte(CP_UTF8, 0, input.data(), static_cast<int>(input.size()), output.data(), required, nullptr, nullptr);
    return output;
}

static std::vector<std::wstring> TokenizeCommandLine(const wchar_t* commandLine) {
    std::vector<std::wstring> args;
    if (!commandLine) return args;

    const wchar_t* p = commandLine;
    while (*p) {
        while (*p && std::iswspace(*p)) ++p;
        if (!*p) break;

        std::wstring arg;
        bool inQuotes = false;
        while (*p) {
            if (*p == L'"') {
                inQuotes = !inQuotes;
                ++p;
                continue;
            }
            if (!inQuotes && std::iswspace(*p)) break;
            arg.push_back(*p++);
        }
        args.push_back(std::move(arg));
        while (*p && std::iswspace(*p)) ++p;
    }

    return args;
}

static void ParseCommandLine(int argc, wchar_t* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (wcscmp(argv[i], L"--config") == 0 && i + 1 < argc) {
            const wchar_t* val = argv[i + 1];
            if (val[0] == L'-' && val[1] == L'-') {
                LOG_WARN("ServiceHost", "CLI", "Args", "--config expects a path, got flag: {}", WStringToUtf8(val));
                continue;
            }
            g_overrideConfigPath = WStringToUtf8(val);
            ++i;
            LOG_INFO("ServiceHost", "CLI", "Args", "Overriding config path: {}", g_overrideConfigPath);
        }
    }
}

static void EnsureCommandLineParsed() {
    if (g_commandLineParsed) return;
    g_commandLineParsed = true;

    auto args = TokenizeCommandLine(GetCommandLineW());
    std::vector<wchar_t*> argv;
    argv.reserve(args.size());
    for (auto& arg : args) {
        argv.push_back(arg.data());
    }
    ParseCommandLine(static_cast<int>(argv.size()), argv.data());
}

static std::string GetConfigPath() {
    EnsureCommandLineParsed();
    if (!g_overrideConfigPath.empty()) {
        return g_overrideConfigPath;
    }
    return "C:/ProgramData/EGoTouchRev/config.ini";
}
#else
// Release: returns empty string — callers should skip file I/O when path is empty
static std::string GetConfigPath() {
    return "";
}
#endif

namespace Service {

struct ServiceHost::Impl {
    std::unique_ptr<Host::SystemStateMonitor> m_sysMonitor;
    // BT MCU 事件通道 (col00)：设备发现 + 握手 + ACK + 0x7D01 回显 —— 仅 Full 模式
    std::unique_ptr<Himax::Pen::PenEventBridge> m_penEventBridge;
    // BT MCU 压力通道 (col01)：'U' 报文读取 + 频率 / 压感数据 —— 仅 Full 模式
    std::unique_ptr<Himax::Pen::PenPressureReader> m_penPressureReader;

    // IPC
    Ipc::IpcPipeServer m_ipcServer;
    Ipc::SharedFrameWriter m_frameWriter;
    Ipc::ConfigDirtyFlag m_configDirty;
    bool m_debugMode = false;
    HANDLE m_logEvent = nullptr;
    HANDLE m_penEvent = nullptr;

    std::vector<Ipc::DebugFieldSchemaWire> m_debugSchema;
    uint16_t m_debugSchemaVersion = 0;
    uint32_t m_debugSchemaHash = 0;
    std::mutex m_debugFrameMutex;
    Solvers::HeatmapFrame m_latestDebugFrame;
    bool m_hasLatestDebugFrame = false;
};

// ── 设备路径 ──
static const std::wstring kDevicePathMaster    = L"\\\\.\\Global\\SPBTESTTOOL_MASTER";
static const std::wstring kDevicePathSlave     = L"\\\\.\\Global\\SPBTESTTOOL_SLAVE";
static const std::wstring kDevicePathInterrupt = L"\\\\.\\Global\\SPBTESTTOOL_MASTER";

namespace {

enum class DebugDerivedSourceIndex : int16_t {
    MasterWasRead = 0,
    ContactCount = 1,
    PeakCount = 2,
    FrameTimestamp = 3,
};

constexpr uint8_t ToPersistedFieldBits() {
    return Ipc::ToBits(Ipc::ServiceConfigFieldWire::Mode) |
           Ipc::ToBits(Ipc::ServiceConfigFieldWire::AutoMode) |
           Ipc::ToBits(Ipc::ServiceConfigFieldWire::StylusVhfEnabled) |
           Ipc::ToBits(Ipc::ServiceConfigFieldWire::PenButtonMode) |
           Ipc::ToBits(Ipc::ServiceConfigFieldWire::PenButtonRoute);
}

constexpr uint8_t ToWireServiceMode(ServiceMode mode) {
    switch (mode) {
    case ServiceMode::Full:
        return static_cast<uint8_t>(Ipc::ServiceModeWire::Full);
    case ServiceMode::TouchOnly:
        return static_cast<uint8_t>(Ipc::ServiceModeWire::TouchOnly);
    }
    return static_cast<uint8_t>(Ipc::ServiceModeWire::Full);
}

bool TryParseWireServiceMode(uint8_t wireValue, ServiceMode& out) {
    switch (static_cast<Ipc::ServiceModeWire>(wireValue)) {
    case Ipc::ServiceModeWire::Full:
        out = ServiceMode::Full;
        return true;
    case Ipc::ServiceModeWire::TouchOnly:
        out = ServiceMode::TouchOnly;
        return true;
    default:
        return false;
    }
}

uint64_t EncodeU32(uint32_t value) {
    return static_cast<uint64_t>(value);
}

uint64_t EncodeI32(int32_t value) {
    return static_cast<uint64_t>(static_cast<uint32_t>(value));
}

uint64_t EncodeBool(bool value) {
    return value ? 1ull : 0ull;
}

uint64_t EncodeF32(float value) {
    uint32_t bits = 0;
    static_assert(sizeof(bits) == sizeof(value), "float/u32 size mismatch");
    std::memcpy(&bits, &value, sizeof(bits));
    return static_cast<uint64_t>(bits);
}

template <size_t N>
std::string_view CStrArrayView(const char (&value)[N]) {
    const auto* end = std::find(value, value + N, '\0');
    return std::string_view(value, static_cast<size_t>(end - value));
}

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

bool HasIniSection(const std::string& configPath, std::string_view sectionName) {
    std::ifstream cfg(configPath);
    if (!cfg.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(cfg, line)) {
        const std::string trimmed = TrimCopy(line);
        if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#') continue;
        if (trimmed.front() != '[' || trimmed.back() != ']') continue;
        const std::string current = TrimCopy(std::string_view(trimmed).substr(1, trimmed.size() - 2));
        if (current == sectionName) {
            return true;
        }
    }
    return false;
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
                          PenButtonMode penButtonMode,
                          PenButtonRoute penButtonRoute,
                          bool penButtonRouteExplicit,
                          const DeviceRuntime& runtime) {
    if (configPath.empty()) {
        return true;
    }

    std::ofstream out(configPath, std::ios::trunc);
    if (!out.is_open()) return false;

    out << "[Service]\n";
    out << "mode=" << ServiceModeToConfig(mode) << "\n";
    out << "auto_mode=" << (autoMode ? "1" : "0") << "\n";
    out << "stylus_vhf_enabled=" << (stylusVhfEnabled ? "1" : "0") << "\n";
    out << "pen_button_mode="
        << static_cast<int>(penButtonMode) << "\n";
    if (penButtonRouteExplicit) {
        out << "pen_button_route="
            << static_cast<int>(penButtonRoute) << "\n";
    }
    out << "\n";

    out << "[TouchPipeline]\n";
    runtime.SavePipelineConfig(out);
    out << "\n";

    out << "[StylusPipeline]\n";
    runtime.SaveStylusPipelineConfig(out);
    out << "\n";
    return true;
}

RuntimePolicyEvent TranslateSystemStateEvent(const Host::SystemStateEvent& event) {
    RuntimePolicyEvent translated{};
    translated.timestamp = event.timestamp;
    translated.rawIndex = event.raw_index;

    switch (event.type) {
    case Host::SystemStateEventType::DisplayOn:
        translated.type = RuntimePolicyEvent::Type::DisplayOn;
        break;
    case Host::SystemStateEventType::DisplayOff:
        translated.type = RuntimePolicyEvent::Type::DisplayOff;
        break;
    case Host::SystemStateEventType::LidOn:
        translated.type = RuntimePolicyEvent::Type::LidOn;
        break;
    case Host::SystemStateEventType::LidOff:
        translated.type = RuntimePolicyEvent::Type::LidOff;
        break;
    case Host::SystemStateEventType::Suspend:
        translated.type = RuntimePolicyEvent::Type::Suspend;
        break;
    case Host::SystemStateEventType::Shutdown:
        translated.type = RuntimePolicyEvent::Type::Shutdown;
        break;
    case Host::SystemStateEventType::ResumeAutomatic:
        translated.type = RuntimePolicyEvent::Type::ResumeAutomatic;
        break;
    default:
        translated.type = RuntimePolicyEvent::Type::Unknown;
        break;
    }

    return translated;
}

} // namespace

ServiceHost::ServiceHost()
    : m_impl(std::make_unique<Impl>()) {}

ServiceHost::~ServiceHost() {
    Stop();
}

// ── 模式解析 ──────────────────────────────────────────
ServiceConfigState ServiceHost::ParseServiceConfig(const std::string& configPath) const {
    return Service::ParseServiceConfig(configPath);
}

void ServiceHost::ApplyServiceConfigToRuntime(const ServiceConfigState& config) {
    if (!m_deviceRuntime) return;

    m_deviceRuntime->ApplyServicePolicy(
        config.autoMode, config.stylusVhfEnabled,
        config.penButtonMode, config.penButtonRoute,
        config.penButtonRouteExplicit);
}

ReloadServiceConfigResult ServiceHost::HandleReloadServiceConfig(
    const ServiceConfigState& reloadedConfig) {
    ReloadServiceConfigResult result{};

    const bool modeChanged = (m_configState.mode != reloadedConfig.mode);
    const bool autoModeChanged = (m_configState.autoMode != reloadedConfig.autoMode);
    const bool stylusVhfChanged = (m_configState.stylusVhfEnabled != reloadedConfig.stylusVhfEnabled);

    if (modeChanged) {
        result.changedFields |= ToServiceConfigFieldBit(ServiceConfigField::Mode);
        result.restartRequiredFields |= ToServiceConfigFieldBit(ServiceConfigField::Mode);
        LOG_WARN("Service", __func__, "IPC",
                 "[Service].mode changed from {} to {}; runtime topology remains {} until service restart.",
                 ServiceModeToConfig(m_configState.mode),
                 ServiceModeToConfig(reloadedConfig.mode),
                 ServiceModeToConfig(m_runtimeMode));
    }

    if (autoModeChanged) {
        result.changedFields |= ToServiceConfigFieldBit(ServiceConfigField::AutoMode);
        LOG_INFO("Service", __func__, "IPC",
                 "[Service].auto_mode reloaded to {} (immediate apply).",
                 reloadedConfig.autoMode ? 1 : 0);
    }

    if (stylusVhfChanged) {
        result.changedFields |= ToServiceConfigFieldBit(ServiceConfigField::StylusVhfEnabled);
        LOG_INFO("Service", __func__, "IPC",
                 "[Service].stylus_vhf_enabled reloaded to {} (immediate apply).",
                 reloadedConfig.stylusVhfEnabled ? 1 : 0);
    }

    const bool penButtonModeChanged = (m_configState.penButtonMode != reloadedConfig.penButtonMode);
    const bool penButtonRouteChanged =
        (m_configState.penButtonRoute != reloadedConfig.penButtonRoute) ||
        (m_configState.penButtonRouteExplicit != reloadedConfig.penButtonRouteExplicit);

    if (penButtonModeChanged) {
        result.changedFields |= ToServiceConfigFieldBit(ServiceConfigField::PenButtonMode);
        LOG_INFO("Service", __func__, "IPC",
                 "[Service].pen_button_mode reloaded to {} (immediate apply).",
                 static_cast<int>(reloadedConfig.penButtonMode));
    }

    if (penButtonRouteChanged) {
        result.changedFields |= ToServiceConfigFieldBit(ServiceConfigField::PenButtonRoute);
        LOG_INFO("Service", __func__, "IPC",
                 "[Service].pen_button_route reloaded to {} (immediate apply).",
                 static_cast<int>(reloadedConfig.penButtonRoute));
    }

    const bool policyChanged =
        autoModeChanged || stylusVhfChanged ||
        penButtonModeChanged || penButtonRouteChanged;
    if (m_deviceRuntime && policyChanged) {
        m_deviceRuntime->ApplyServicePolicy(
            reloadedConfig.autoMode, reloadedConfig.stylusVhfEnabled,
            reloadedConfig.penButtonMode, reloadedConfig.penButtonRoute,
            reloadedConfig.penButtonRouteExplicit);
        result.appliedFields |= static_cast<uint8_t>(
            (autoModeChanged ? ToServiceConfigFieldBit(ServiceConfigField::AutoMode) : 0u) |
            (stylusVhfChanged ? ToServiceConfigFieldBit(ServiceConfigField::StylusVhfEnabled) : 0u) |
            (penButtonModeChanged ? ToServiceConfigFieldBit(ServiceConfigField::PenButtonMode) : 0u) |
            (penButtonRouteChanged ? ToServiceConfigFieldBit(ServiceConfigField::PenButtonRoute) : 0u));
    }

    m_configState = reloadedConfig;
    return result;
}

bool ServiceHost::StartRuntimeAndPipeline(const std::string& configPath) {
    m_deviceRuntime = std::make_unique<DeviceRuntime>(
        kDevicePathMaster, kDevicePathSlave, kDevicePathInterrupt);
    ApplyServiceConfigToRuntime(m_configState);
    BuildDefaultPipeline(configPath);
    BuildDebugSchema();

    if (!m_deviceRuntime->Start()) {
        LOG_ERROR("Service", __func__, "Boot", "DeviceRuntime::Start() failed.");
        return false;
    }

    LOG_INFO("Service", __func__, "Boot", "DeviceRuntime started (auto mode).");
    return true;
}

void ServiceHost::StartSystemStateMonitor() {
    m_impl->m_sysMonitor = std::make_unique<Host::SystemStateMonitor>();
    const bool monitorOk = m_impl->m_sysMonitor->Start(
        [this](const Host::SystemStateEvent& ev) {
            LOG_INFO("Service", __func__, "Event", "System event: type={}", Host::ToString(ev.type));
            m_deviceRuntime->IngestPolicyEvent(TranslateSystemStateEvent(ev));
        });

    if (!monitorOk) {
        LOG_WARN("Service", __func__, "Monitor", "SystemStateMonitor failed to start; running without system state detection.");
        m_impl->m_sysMonitor.reset();
        return;
    }

    LOG_INFO("Service", __func__, "Monitor", "SystemStateMonitor started.");
}

void ServiceHost::StartIpcSubsystem() {
#ifdef _DEBUG
    // Service creates Global\\ mapping in debug builds.
    if (!m_impl->m_frameWriter.Create(Ipc::kSharedFrameName)) {
        LOG_WARN("Service", __func__, "IPC", "Failed to create shared memory; App debug will be disabled.");
    } else {
        LOG_INFO("Service", __func__, "IPC", "Shared memory created for App connection.");
    }
#endif

    {
        Ipc::ScopedSecurityDescriptor sd;
        SECURITY_ATTRIBUTES sa{};
        if (!Ipc::BuildAdminOnlySecurityAttributes(sa, sd)) {
            LOG_WARN("Service", __func__, "IPC", "Build event security descriptor failed: {}", GetLastError());
        } else {
            m_impl->m_logEvent = CreateEventW(&sa, FALSE, FALSE, Ipc::kLogReadyEventName);
            if (!m_impl->m_logEvent) {
                LOG_WARN("Service", __func__, "IPC", "CreateEvent failed for LogReadyEvent: {}", GetLastError());
            } else {
                Common::GuiLogSink::Instance()->SetNotifyEvent(m_impl->m_logEvent);
            }

            m_impl->m_penEvent = CreateEventW(&sa, FALSE, FALSE, Ipc::kPenReadyEventName);
            if (!m_impl->m_penEvent) {
                LOG_WARN("Service", __func__, "IPC", "CreateEvent failed for PenReadyEvent: {}", GetLastError());
            }
        }
    }

    m_impl->m_configDirty.Open();
    m_impl->m_ipcServer.SetCommandHandler(
        [this](const Ipc::IpcRequest& req) {
            return HandleIpcCommand(req);
        });
    m_impl->m_ipcServer.Start();
    LOG_INFO("Service", __func__, "Boot", "IPC pipe server started.");
}

void ServiceHost::StartPenSubsystem() {
    if (m_runtimeMode != ServiceMode::Full) {
        LOG_INFO("Service", __func__, "MCU", "Pen modules skipped (touch_only mode).");
        return;
    }

    m_impl->m_penEventBridge = std::make_unique<Himax::Pen::PenEventBridge>();
    if (m_impl->m_penEvent) {
        m_impl->m_penEventBridge->SetNotifyEvent(m_impl->m_penEvent);
    }
    m_impl->m_penEventBridge->SetEventCallback(
        [this](const Himax::Pen::PenEvent& ev) {
            if (m_deviceRuntime) {
                m_deviceRuntime->IngestPenEvent(ev);
            }
        });
    m_impl->m_penEventBridge->Start();
    LOG_INFO("Service", __func__, "MCU", "PenEventBridge started (col00 event channel).");

    m_impl->m_penPressureReader = std::make_unique<Himax::Pen::PenPressureReader>();
    if (m_impl->m_penEvent) {
        m_impl->m_penPressureReader->SetNotifyEvent(m_impl->m_penEvent);
    }
    m_impl->m_penPressureReader->SetPressureCallback(
        [this](const Himax::Pen::PenPressureStats& stats) {
            if (m_deviceRuntime) {
                m_deviceRuntime->IngestBtMcuPressurePacket(
                    std::array<uint16_t, 4>{stats.press[0], stats.press[1], stats.press[2], stats.press[3]},
                    std::array<uint16_t, 4>{stats.rawPress[0], stats.rawPress[1], stats.rawPress[2], stats.rawPress[3]},
                    stats.freq1,
                    stats.freq2);
            }
        });
    m_impl->m_penPressureReader->Start();
    LOG_INFO("Service", __func__, "MCU", "PenPressureReader started (col01 pressure channel).");
}

bool ServiceHost::Start() {
    const std::string configPath = GetConfigPath();
    if (!configPath.empty()) {
        m_configState = ParseServiceConfig(configPath);
    }
    m_runtimeMode = m_configState.mode;
    LOG_INFO("Service", __func__, "Boot", "Service mode: {}, AutoMode: {}",
             ServiceModeToConfig(m_configState.mode), m_configState.autoMode);

    if (!StartRuntimeAndPipeline(configPath)) {
        return false;
    }

    StartSystemStateMonitor();
    StartIpcSubsystem();
    StartPenSubsystem();

    LOG_INFO("Service", __func__, "Boot", "All modules started.");
    return true;
}

void ServiceHost::StopIpcSubsystem() {
    m_impl->m_ipcServer.Stop();
#ifdef _DEBUG
    if (m_deviceRuntime) {
        m_deviceRuntime->SetFramePushCallback(nullptr);
    }
#endif
    {
        std::lock_guard<std::mutex> lk(m_impl->m_debugFrameMutex);
        m_impl->m_hasLatestDebugFrame = false;
        m_impl->m_latestDebugFrame = Solvers::HeatmapFrame{};
    }
    m_impl->m_debugMode = false;

    m_impl->m_frameWriter.Close();
    m_impl->m_configDirty.Close();

    if (m_impl->m_logEvent) {
        Common::GuiLogSink::Instance()->SetNotifyEvent(nullptr);
        CloseHandle(m_impl->m_logEvent);
        m_impl->m_logEvent = nullptr;
    }

    if (m_impl->m_penEvent) {
        CloseHandle(m_impl->m_penEvent);
        m_impl->m_penEvent = nullptr;
    }
}

void ServiceHost::StopPenSubsystem() {
    if (m_impl->m_penPressureReader) {
        m_impl->m_penPressureReader->SetNotifyEvent(nullptr);
        m_impl->m_penPressureReader->Stop();
        m_impl->m_penPressureReader.reset();
        LOG_INFO("Service", __func__, "MCU", "PenPressureReader stopped.");
    }

    if (m_impl->m_penEventBridge) {
        m_impl->m_penEventBridge->SetNotifyEvent(nullptr);
        m_impl->m_penEventBridge->Stop();
        m_impl->m_penEventBridge.reset();
        LOG_INFO("Service", __func__, "MCU", "PenEventBridge stopped.");
    }
}

void ServiceHost::StopSystemStateMonitor() {
    if (!m_impl->m_sysMonitor) {
        return;
    }

    m_impl->m_sysMonitor->Stop();
    m_impl->m_sysMonitor.reset();
    LOG_INFO("Service", __func__, "Monitor", "SystemStateMonitor stopped.");
}

void ServiceHost::StopRuntimeSubsystem() {
    if (!m_deviceRuntime) {
        return;
    }

    m_deviceRuntime->Stop();
    m_deviceRuntime.reset();
    LOG_INFO("Service", __func__, "Device", "DeviceRuntime stopped.");
}

void ServiceHost::Stop() {
    StopPenSubsystem();
    StopIpcSubsystem();
    StopSystemStateMonitor();
    StopRuntimeSubsystem();

    LOG_INFO("Service", __func__, "Shutdown", "All modules stopped.");
}

// ── Shared config loader ────────────────────────────────────────────
template <typename TouchLoader, typename StylusLoader>
LoadPipelineConfigResult LoadPipelineConfig(
    const std::string& configPath,
    TouchLoader&& loadTouch,
    StylusLoader&& loadStylus)
{
    LoadPipelineConfigResult result;
    if (configPath.empty()) {
        return result;
    }

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
            loadTouch(key, val);
            result.touchLoaded = true;
            continue;
        }
        if (section == "StylusPipeline") {
            loadStylus(key, val);
            result.stylusLoaded = true;
            continue;
        }
        if (IsLegacyTouchSection(section)) {
            const auto mappedKey = MapLegacyTouchKey(section, key);
            if (!mappedKey.has_value()) continue;
            loadTouch(*mappedKey, val);
            result.touchLoaded = true;
            result.migrated = true;
        }
    }
    return result;
}

// ── Pipeline 构建 ──────────────────────────────
void ServiceHost::CopyCString(char* dst, size_t dstSize, std::string_view src) {
    if (!dst || dstSize == 0) return;
    std::memset(dst, 0, dstSize);
    if (src.empty()) return;
    const size_t n = std::min(dstSize - 1, src.size());
    std::memcpy(dst, src.data(), n);
}

uint32_t ServiceHost::HashDebugSchema(const std::vector<Ipc::DebugFieldSchemaWire>& defs) {
    uint32_t h = 2166136261u;
    auto hashBytes = [&h](const void* p, size_t n) {
        const auto* b = reinterpret_cast<const uint8_t*>(p);
        for (size_t i = 0; i < n; ++i) {
            h ^= b[i];
            h *= 16777619u;
        }
    };

    for (const auto& d : defs) {
        hashBytes(&d.fieldId, sizeof(d.fieldId));
        hashBytes(&d.valueType, sizeof(d.valueType));
        hashBytes(&d.sourceKind, sizeof(d.sourceKind));
        hashBytes(&d.sourceIndex, sizeof(d.sourceIndex));
        hashBytes(&d.uiOrder, sizeof(d.uiOrder));
        hashBytes(&d.dvrTarget, sizeof(d.dvrTarget));
        hashBytes(&d.dvrPositionMode, sizeof(d.dvrPositionMode));
        hashBytes(&d.dvrIndex, sizeof(d.dvrIndex));

        const std::string_view key = CStrArrayView(d.key);
        const std::string_view displayName = CStrArrayView(d.displayName);
        const std::string_view unit = CStrArrayView(d.unit);
        const std::string_view uiGroup = CStrArrayView(d.uiGroup);
        const std::string_view dvrColumnName = CStrArrayView(d.dvrColumnName);
        const std::string_view dvrAnchor = CStrArrayView(d.dvrAnchor);

        hashBytes(key.data(), key.size());
        hashBytes(displayName.data(), displayName.size());
        hashBytes(unit.data(), unit.size());
        hashBytes(uiGroup.data(), uiGroup.size());
        hashBytes(dvrColumnName.data(), dvrColumnName.size());
        hashBytes(dvrAnchor.data(), dvrAnchor.size());
    }

    return h;
}

uint16_t ServiceHost::DeriveDebugSchemaVersion(uint32_t schemaHash) {
    // Update policy: schemaVersion is deterministically derived from descriptor content.
    // Any descriptor change that affects schemaHash automatically changes schemaVersion.
    constexpr uint16_t kVersionSalt = 0xA5C3u;
    const uint16_t folded = static_cast<uint16_t>((schemaHash & 0xFFFFu) ^ (schemaHash >> 16));
    const uint16_t version = static_cast<uint16_t>(folded ^ kVersionSalt);
    return version == 0 ? 1 : version;
}

uint64_t ServiceHost::EncodePenValue(const Himax::Pen::PenPressureStats& s,
                                     bool evtRunning,
                                     bool pressRunning,
                                     int16_t sourceIndex,
                                     bool& valid) {
    valid = true;
    switch (static_cast<Ipc::DebugPenSourceIndex>(sourceIndex)) {
    case Ipc::DebugPenSourceIndex::EvtRunning: return EncodeBool(evtRunning);
    case Ipc::DebugPenSourceIndex::PressRunning: return EncodeBool(pressRunning);
    case Ipc::DebugPenSourceIndex::ReportType: return EncodeU32(s.reportType);
    case Ipc::DebugPenSourceIndex::Freq1: return EncodeU32(s.freq1);
    case Ipc::DebugPenSourceIndex::Freq2: return EncodeU32(s.freq2);
    case Ipc::DebugPenSourceIndex::Press0: return EncodeU32(s.press[0]);
    case Ipc::DebugPenSourceIndex::Press1: return EncodeU32(s.press[1]);
    case Ipc::DebugPenSourceIndex::Press2: return EncodeU32(s.press[2]);
    case Ipc::DebugPenSourceIndex::Press3: return EncodeU32(s.press[3]);
    default:
        valid = false;
        return 0;
    }
}

uint64_t ServiceHost::EncodeDebugValue(const Solvers::HeatmapFrame& frame,
                                       const Ipc::DebugFieldSchemaWire& def,
                                       bool& valid) {
    valid = true;
    const auto sourceKind = static_cast<Ipc::DebugSourceKind>(def.sourceKind);

    switch (sourceKind) {
    case Ipc::DebugSourceKind::MasterSuffixWord:
        if (!frame.masterSuffixValid || def.sourceIndex < 0 || def.sourceIndex >= Frame::kMasterSuffixWords) {
            valid = false;
            return 0;
        }
        return EncodeU32(frame.masterSuffix.words[def.sourceIndex]);
    case Ipc::DebugSourceKind::SlaveSuffixWord:
        if (!frame.slaveSuffixValid || def.sourceIndex < 0 || def.sourceIndex >= Frame::kSlaveSuffixWords) {
            valid = false;
            return 0;
        }
        return EncodeU32(frame.slaveSuffix.words[def.sourceIndex]);
    case Ipc::DebugSourceKind::StylusField: {
        const auto& s = frame.stylus;
        const auto& point = s.output.point;
        const auto& press = s.runtime.pressure;
        switch (static_cast<Ipc::DebugStylusSourceIndex>(def.sourceIndex)) {
        case Ipc::DebugStylusSourceIndex::Pressure: return EncodeU32(s.output.pressure);
        case Ipc::DebugStylusSourceIndex::SignalX: return EncodeU32(s.interop.signalX);
        case Ipc::DebugStylusSourceIndex::SignalY: return EncodeU32(s.interop.signalY);
        case Ipc::DebugStylusSourceIndex::MaxRawPeak: return EncodeU32(s.interop.maxRawPeak);
        case Ipc::DebugStylusSourceIndex::Status: return EncodeU32(s.input.status);
        case Ipc::DebugStylusSourceIndex::PipelineStage: return EncodeU32(s.output.pipelineStage);
        case Ipc::DebugStylusSourceIndex::PointX: return EncodeF32(point.x);
        case Ipc::DebugStylusSourceIndex::PointY: return EncodeF32(point.y);
        case Ipc::DebugStylusSourceIndex::RawPressure: return EncodeU32(point.rawPressure);
        case Ipc::DebugStylusSourceIndex::MappedPressure: return EncodeU32(point.mappedPressure);
        case Ipc::DebugStylusSourceIndex::NoPressInkActive:
            valid = false;
            return 0;
        case Ipc::DebugStylusSourceIndex::TouchSuppressActive: return EncodeBool(s.interop.touchSuppressActive);
        case Ipc::DebugStylusSourceIndex::BtSeq: return EncodeU32(press.btSeq);
        case Ipc::DebugStylusSourceIndex::PredictedAgeFrames: return EncodeU32(press.predictedAgeFrames);
        case Ipc::DebugStylusSourceIndex::PressureIsReal: return EncodeBool(press.pressureIsReal);
        default:
            valid = false;
            return 0;
        }
    }
    case Ipc::DebugSourceKind::PenBridgeField:
        valid = false;
        return 0;
    case Ipc::DebugSourceKind::DerivedField:
        switch (static_cast<DebugDerivedSourceIndex>(def.sourceIndex)) {
        case DebugDerivedSourceIndex::MasterWasRead:
            return EncodeBool(frame.masterWasRead);
        case DebugDerivedSourceIndex::ContactCount:
            return EncodeU32(static_cast<uint32_t>(frame.touch.output.contacts.size()));
        case DebugDerivedSourceIndex::PeakCount:
#if EGOTOUCH_DIAG
            return EncodeU32(static_cast<uint32_t>(frame.touch.debug.peaks.size()));
#else
            return EncodeU32(0);
#endif
        case DebugDerivedSourceIndex::FrameTimestamp:
            return frame.timestamp;
        default:
            valid = false;
            return 0;
        }
    default:
        valid = false;
        return 0;
    }
}

void ServiceHost::BuildDebugSchema() {
    m_impl->m_debugSchema.clear();

    auto add = [this](uint16_t fieldId,
                      Ipc::DebugValueType valueType,
                      Ipc::DebugSourceKind sourceKind,
                      int16_t sourceIndex,
                      uint8_t uiOrder,
                      Ipc::DebugDvrTarget dvrTarget,
                      Ipc::DebugDvrPositionMode dvrPositionMode,
                      int16_t dvrIndex,
                      std::string_view key,
                      std::string_view displayName,
                      std::string_view unit,
                      std::string_view uiGroup,
                      std::string_view dvrColumnName,
                      std::string_view dvrAnchor) {
        Ipc::DebugFieldSchemaWire w{};
        w.fieldId = fieldId;
        w.valueType = static_cast<uint8_t>(valueType);
        w.sourceKind = static_cast<uint8_t>(sourceKind);
        w.sourceIndex = sourceIndex;
        w.uiOrder = uiOrder;
        w.dvrTarget = static_cast<uint8_t>(dvrTarget);
        w.dvrPositionMode = static_cast<uint8_t>(dvrPositionMode);
        w.dvrIndex = dvrIndex;
        CopyCString(w.key, sizeof(w.key), key);
        CopyCString(w.displayName, sizeof(w.displayName), displayName);
        CopyCString(w.unit, sizeof(w.unit), unit);
        CopyCString(w.uiGroup, sizeof(w.uiGroup), uiGroup);
        CopyCString(w.dvrColumnName, sizeof(w.dvrColumnName), dvrColumnName);
        CopyCString(w.dvrAnchor, sizeof(w.dvrAnchor), dvrAnchor);
        m_impl->m_debugSchema.push_back(w);
    };

    add(1, Ipc::DebugValueType::UInt32, Ipc::DebugSourceKind::MasterSuffixWord,
        static_cast<int16_t>(Frame::MasterWord::kTpFreq1), 1,
        Ipc::DebugDvrTarget::MasterStatus, Ipc::DebugDvrPositionMode::AfterAnchor, -1,
        "master_tp_freq1", "Master TpFreq1", "", "MasterSuffix",
        "DBG_MasterTpFreq1", "MasterSuffixValid");

    add(2, Ipc::DebugValueType::UInt32, Ipc::DebugSourceKind::MasterSuffixWord,
        static_cast<int16_t>(Frame::MasterWord::kTpFreq2), 2,
        Ipc::DebugDvrTarget::MasterStatus, Ipc::DebugDvrPositionMode::AfterAnchor, -1,
        "master_tp_freq2", "Master TpFreq2", "", "MasterSuffix",
        "DBG_MasterTpFreq2", "DBG_MasterTpFreq1");

    add(3, Ipc::DebugValueType::UInt32, Ipc::DebugSourceKind::StylusField,
        static_cast<int16_t>(Ipc::DebugStylusSourceIndex::Pressure), 1,
        Ipc::DebugDvrTarget::SlaveSuffix, Ipc::DebugDvrPositionMode::AfterAnchor, -1,
        "stylus_pressure", "Stylus Pressure", "", "Stylus",
        "DBG_StylusPressure", "Pressure");

    add(4, Ipc::DebugValueType::Float32, Ipc::DebugSourceKind::StylusField,
        static_cast<int16_t>(Ipc::DebugStylusSourceIndex::PointX), 2,
        Ipc::DebugDvrTarget::SlaveSuffix, Ipc::DebugDvrPositionMode::AfterAnchor, -1,
        "stylus_point_x", "Stylus Point X", "grid", "Stylus",
        "DBG_StylusPointX", "PointX");

    add(5, Ipc::DebugValueType::Float32, Ipc::DebugSourceKind::StylusField,
        static_cast<int16_t>(Ipc::DebugStylusSourceIndex::PointY), 3,
        Ipc::DebugDvrTarget::SlaveSuffix, Ipc::DebugDvrPositionMode::AfterAnchor, -1,
        "stylus_point_y", "Stylus Point Y", "grid", "Stylus",
        "DBG_StylusPointY", "DBG_StylusPointX");

    add(13, Ipc::DebugValueType::UInt32, Ipc::DebugSourceKind::StylusField,
        static_cast<int16_t>(Ipc::DebugStylusSourceIndex::BtSeq), 4,
        Ipc::DebugDvrTarget::SlaveSuffix, Ipc::DebugDvrPositionMode::AfterAnchor, -1,
        "stylus_bt_seq", "Stylus BT Seq", "", "Stylus",
        "DBG_StylusBtSeq", "DBG_StylusPointY");

    add(14, Ipc::DebugValueType::UInt32, Ipc::DebugSourceKind::StylusField,
        static_cast<int16_t>(Ipc::DebugStylusSourceIndex::PredictedAgeFrames), 5,
        Ipc::DebugDvrTarget::SlaveSuffix, Ipc::DebugDvrPositionMode::AfterAnchor, -1,
        "stylus_predicted_age", "Predicted Age", "frames", "Stylus",
        "DBG_StylusPredictedAge", "DBG_StylusBtSeq");

    add(15, Ipc::DebugValueType::Bool, Ipc::DebugSourceKind::StylusField,
        static_cast<int16_t>(Ipc::DebugStylusSourceIndex::PressureIsReal), 6,
        Ipc::DebugDvrTarget::SlaveSuffix, Ipc::DebugDvrPositionMode::AfterAnchor, -1,
        "stylus_pressure_is_real", "Pressure Is Real", "", "Stylus",
        "DBG_StylusPressureIsReal", "DBG_StylusPredictedAge");

    add(6, Ipc::DebugValueType::UInt32, Ipc::DebugSourceKind::PenBridgeField,
        static_cast<int16_t>(Ipc::DebugPenSourceIndex::Freq1), 1,
        Ipc::DebugDvrTarget::DynamicDebug, Ipc::DebugDvrPositionMode::Append, -1,
        "pen_freq1", "Pen Freq1", "", "PenBridge",
        "DBG_PenFreq1", "");

    add(7, Ipc::DebugValueType::UInt32, Ipc::DebugSourceKind::PenBridgeField,
        static_cast<int16_t>(Ipc::DebugPenSourceIndex::Freq2), 2,
        Ipc::DebugDvrTarget::DynamicDebug, Ipc::DebugDvrPositionMode::Append, -1,
        "pen_freq2", "Pen Freq2", "", "PenBridge",
        "DBG_PenFreq2", "");

    add(8, Ipc::DebugValueType::UInt32, Ipc::DebugSourceKind::PenBridgeField,
        static_cast<int16_t>(Ipc::DebugPenSourceIndex::Press0), 3,
        Ipc::DebugDvrTarget::DynamicDebug, Ipc::DebugDvrPositionMode::Append, -1,
        "pen_press0", "Pen Press0", "", "PenBridge",
        "DBG_PenPress0", "");

    add(9, Ipc::DebugValueType::UInt32, Ipc::DebugSourceKind::PenBridgeField,
        static_cast<int16_t>(Ipc::DebugPenSourceIndex::Press1), 4,
        Ipc::DebugDvrTarget::DynamicDebug, Ipc::DebugDvrPositionMode::Append, -1,
        "pen_press1", "Pen Press1", "", "PenBridge",
        "DBG_PenPress1", "");

    add(16, Ipc::DebugValueType::UInt32, Ipc::DebugSourceKind::PenBridgeField,
        static_cast<int16_t>(Ipc::DebugPenSourceIndex::Press2), 5,
        Ipc::DebugDvrTarget::DynamicDebug, Ipc::DebugDvrPositionMode::Append, -1,
        "pen_press2", "Pen Press2", "", "PenBridge",
        "DBG_PenPress2", "");

    add(17, Ipc::DebugValueType::UInt32, Ipc::DebugSourceKind::PenBridgeField,
        static_cast<int16_t>(Ipc::DebugPenSourceIndex::Press3), 6,
        Ipc::DebugDvrTarget::DynamicDebug, Ipc::DebugDvrPositionMode::Append, -1,
        "pen_press3", "Pen Press3", "", "PenBridge",
        "DBG_PenPress3", "");

    add(10, Ipc::DebugValueType::UInt32, Ipc::DebugSourceKind::DerivedField,
        static_cast<int16_t>(DebugDerivedSourceIndex::ContactCount), 1,
        Ipc::DebugDvrTarget::MasterStatus, Ipc::DebugDvrPositionMode::AfterAnchor, -1,
        "contact_count", "Contact Count", "", "Frame",
        "DBG_ContactCount", "ContactCount");

    add(11, Ipc::DebugValueType::UInt32, Ipc::DebugSourceKind::DerivedField,
        static_cast<int16_t>(DebugDerivedSourceIndex::PeakCount), 2,
        Ipc::DebugDvrTarget::MasterStatus, Ipc::DebugDvrPositionMode::AfterAnchor, -1,
        "peak_count", "Peak Count", "", "Frame",
        "DBG_PeakCount", "DBG_ContactCount");

    add(12, Ipc::DebugValueType::Bool, Ipc::DebugSourceKind::DerivedField,
        static_cast<int16_t>(DebugDerivedSourceIndex::MasterWasRead), 3,
        Ipc::DebugDvrTarget::MasterStatus, Ipc::DebugDvrPositionMode::AfterAnchor, -1,
        "master_was_read", "Master Was Read", "", "Frame",
        "DBG_MasterWasRead", "DBG_PeakCount");

    m_impl->m_debugSchemaHash = HashDebugSchema(m_impl->m_debugSchema);
    m_impl->m_debugSchemaVersion = DeriveDebugSchemaVersion(m_impl->m_debugSchemaHash);
}

void ServiceHost::BuildDefaultPipeline(const std::string& configPath) {
    if (configPath.empty()) {
        LOG_INFO("Service", __func__, "Boot", "Config file I/O disabled; using built-in pipeline defaults.");
        return;
    }

    // TouchPipeline is self-contained: no processor registration needed.
    // Just load config.
    auto loadTouch = [this](const std::string& key, const std::string& value) {
        m_deviceRuntime->LoadPipelineConfig(key, value);
    };
    auto loadStylus = [this](const std::string& key, const std::string& value) {
        m_deviceRuntime->LoadStylusPipelineConfig(key, value);
    };
    LOG_INFO("Service", __func__, "Boot", "TouchPipeline initialized (linear orchestrator).");

    // Load saved config
    const auto loadResult = LoadPipelineConfig(configPath, loadTouch, loadStylus);
    if (!loadResult.fileOpened) {
        LOG_WARN("Service", __func__, "Boot", "Config file not found: {}", configPath);
        return;
    }

    if (loadResult.migrated) {
        std::string backupPath;
        if (BackupConfigFile(configPath, backupPath)) {
            if (WriteCanonicalConfig(configPath, m_configState.mode, m_configState.autoMode, m_configState.stylusVhfEnabled, m_configState.penButtonMode, m_configState.penButtonRoute, m_configState.penButtonRouteExplicit, *m_deviceRuntime)) {
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

// ── IPC helpers ──────────────────────────────
void ServiceHost::HandleIpcEnterDebugMode(Ipc::IpcResponse& resp) {
#ifdef _DEBUG
    // Shared memory is already created at startup.
    // Just activate the frame push callback.
    if (m_impl->m_frameWriter.IsOpen()) {
        m_deviceRuntime->SetFramePushCallback(
            [this](const Solvers::HeatmapFrame& f) {
                {
                    std::lock_guard<std::mutex> lk(m_impl->m_debugFrameMutex);
                    m_impl->m_latestDebugFrame = f;
                    m_impl->m_hasLatestDebugFrame = true;
                }
                Ipc::SharedFrameData sharedFrame{};
                Ipc::PopulateSharedFrameDataFromSolverFrame(sharedFrame, f);
                const RuntimeSnapshot runtime = m_deviceRuntime->GetSnapshot();
                sharedFrame.workerState = static_cast<int8_t>(runtime.state);
                sharedFrame.streaming = runtime.state == workerState::streaming;
                sharedFrame.lastFrameProcessUs = -1;
                sharedFrame.avgFrameProcessUs = -1;
                sharedFrame.acquisitionFps = -1;
                sharedFrame.slaveAcquisitionFps = -1;
                sharedFrame.vhfEnabled = m_deviceRuntime->IsVhfEnabled();
                sharedFrame.vhfDeviceOpen = m_deviceRuntime->IsVhfDeviceOpen();
                sharedFrame.vhfTranspose = m_deviceRuntime->IsVhfTransposeEnabled();
                m_impl->m_frameWriter.Write(sharedFrame);
            });
        m_impl->m_debugMode = true;
        Ipc::MarkSuccess(resp);
        LOG_INFO("Service", __func__, "IPC", "Entered debug mode.");
    } else {
        Ipc::MarkFailure(resp, Ipc::IpcStatusCode::InvalidState);
        LOG_ERROR("Service", __func__, "IPC", "EnterDebugMode rejected: shared memory not available.");
    }
#else
    Ipc::MarkFailure(resp, Ipc::IpcStatusCode::UnsupportedCommand);
    LOG_WARN("Service", __func__, "IPC", "EnterDebugMode not available in release build.");
#endif
}

void ServiceHost::HandleIpcExitDebugMode(Ipc::IpcResponse& resp) {
#ifdef _DEBUG
    m_deviceRuntime->SetFramePushCallback(nullptr);
    {
        std::lock_guard<std::mutex> lk(m_impl->m_debugFrameMutex);
        m_impl->m_hasLatestDebugFrame = false;
        m_impl->m_latestDebugFrame = Solvers::HeatmapFrame{};
    }
    m_impl->m_debugMode = false;
#endif

    Ipc::MarkSuccess(resp);
    LOG_INFO("Service", __func__, "IPC", "Exited debug mode.");
}

void ServiceHost::HandleIpcGetConfigSnapshot(Ipc::IpcResponse& resp) {
    Ipc::ConfigSnapshotWire snapshot{};
    snapshot.definedFields = ToPersistedFieldBits();
    snapshot.desiredMode = ToWireServiceMode(m_configState.mode);
    snapshot.activeMode = ToWireServiceMode(m_runtimeMode);
    snapshot.autoMode = m_configState.autoMode ? 1 : 0;
    snapshot.stylusVhfEnabled = m_configState.stylusVhfEnabled ? 1 : 0;
    snapshot.penButtonMode = static_cast<uint8_t>(m_configState.penButtonMode);
    snapshot.penButtonRoute = static_cast<uint8_t>(m_configState.penButtonRoute);

    std::memcpy(resp.data, &snapshot, sizeof(snapshot));
    resp.dataLen = static_cast<uint16_t>(sizeof(snapshot));
    Ipc::MarkSuccess(resp);
}

void ServiceHost::HandleIpcApplyConfigPatch(const Ipc::IpcRequest& req, Ipc::IpcResponse& resp) {
    if (!m_deviceRuntime) {
        Ipc::MarkFailure(resp, Ipc::IpcStatusCode::InvalidState);
        return;
    }
    if (req.paramLen < sizeof(Ipc::ApplyConfigPatchRequestWire)) {
        Ipc::MarkFailure(resp, Ipc::IpcStatusCode::InvalidRequest);
        return;
    }

    Ipc::ApplyConfigPatchRequestWire patch{};
    std::memcpy(&patch, req.param, sizeof(patch));
    if (patch.wireVersion != Ipc::kIpcProtocolVersion) {
        Ipc::MarkFailure(resp, Ipc::IpcStatusCode::InvalidRequest);
        return;
    }

    ServiceConfigState desired = m_configState;
    if (Ipc::HasField(patch.fieldMask, Ipc::ServiceConfigFieldWire::Mode)) {
        if (!TryParseWireServiceMode(patch.desiredMode, desired.mode)) {
            Ipc::MarkFailure(resp, Ipc::IpcStatusCode::InvalidRequest);
            return;
        }
    }
    if (Ipc::HasField(patch.fieldMask, Ipc::ServiceConfigFieldWire::AutoMode)) {
        desired.autoMode = patch.autoMode != 0;
    }
    if (Ipc::HasField(patch.fieldMask, Ipc::ServiceConfigFieldWire::StylusVhfEnabled)) {
        desired.stylusVhfEnabled = patch.stylusVhfEnabled != 0;
    }
    if (Ipc::HasField(patch.fieldMask, Ipc::ServiceConfigFieldWire::PenButtonMode)) {
        desired.penButtonMode = static_cast<PenButtonMode>(
            std::clamp(static_cast<int>(patch.penButtonMode), 0, 2));
    }
    if (Ipc::HasField(patch.fieldMask, Ipc::ServiceConfigFieldWire::PenButtonRoute)) {
        desired.penButtonRoute = static_cast<PenButtonRoute>(
            std::clamp(static_cast<int>(patch.penButtonRoute), 0, 2));
        desired.penButtonRouteExplicit = true;
    }

    const auto reloadState = HandleReloadServiceConfig(desired);

    Ipc::ConfigMutationResultWire result{};
    result.changedFields = reloadState.changedFields;
    result.appliedFields = reloadState.appliedFields;
    result.restartRequiredFields = reloadState.restartRequiredFields;

    std::memcpy(resp.data, &result, sizeof(result));
    resp.dataLen = static_cast<uint16_t>(sizeof(result));
    Ipc::MarkSuccess(resp);
}

void ServiceHost::HandleIpcPersistConfig(Ipc::IpcResponse& resp) {
    if (!m_deviceRuntime) {
        Ipc::MarkFailure(resp, Ipc::IpcStatusCode::InvalidState);
        return;
    }

    const std::string configPath = GetConfigPath();
    if (!WriteCanonicalConfig(configPath, m_configState.mode, m_configState.autoMode, m_configState.stylusVhfEnabled, m_configState.penButtonMode, m_configState.penButtonRoute, m_configState.penButtonRouteExplicit, *m_deviceRuntime)) {
        Ipc::MarkFailure(resp, Ipc::IpcStatusCode::InternalError);
        return;
    }

    Ipc::PersistConfigResponseWire result{};
    result.persistedFields = ToPersistedFieldBits();
    std::memcpy(resp.data, &result, sizeof(result));
    resp.dataLen = static_cast<uint16_t>(sizeof(result));
    Ipc::MarkSuccess(resp);
    if (configPath.empty()) {
        LOG_INFO("Service", __func__, "IPC", "PersistConfig skipped; config file I/O disabled.");
    } else {
        LOG_INFO("Service", __func__, "IPC", "Canonical config persisted to {}.", configPath);
    }
}

void ServiceHost::HandleIpcReloadConfig(Ipc::IpcResponse& resp) {
    if (!m_deviceRuntime) {
        Ipc::MarkFailure(resp, Ipc::IpcStatusCode::InvalidState);
        return;
    }

    const std::string configPath = GetConfigPath();
    if (configPath.empty()) {
        Ipc::ReloadConfigSummaryWire summary{};
        std::memcpy(resp.data, &summary, sizeof(summary));
        resp.dataLen = static_cast<uint16_t>(sizeof(summary));
        Ipc::MarkSuccess(resp);
        LOG_INFO("Service", __func__, "IPC", "ReloadConfig skipped; config file I/O disabled.");
        return;
    }

    auto loadTouch = [this](const std::string& key, const std::string& value) {
        m_deviceRuntime->LoadPipelineConfig(key, value);
    };
    auto loadStylus = [this](const std::string& key, const std::string& value) {
        m_deviceRuntime->LoadStylusPipelineConfig(key, value);
    };
    const auto loadResult = LoadPipelineConfig(configPath, loadTouch, loadStylus);
    if (!loadResult.fileOpened) {
        LOG_WARN("Service", __func__, "IPC", "ReloadConfig failed: config file not found: {}", configPath);
        Ipc::MarkFailure(resp, Ipc::IpcStatusCode::NotFound);
        return;
    }

    const auto reloadedConfig = ParseServiceConfig(configPath);
    const auto reloadState = HandleReloadServiceConfig(reloadedConfig);
    const bool missingCanonicalServiceSection = !HasIniSection(configPath, "Service");

    if (loadResult.migrated || missingCanonicalServiceSection) {
        std::string backupPath;
        if (BackupConfigFile(configPath, backupPath)) {
            if (WriteCanonicalConfig(configPath, m_configState.mode, m_configState.autoMode, m_configState.stylusVhfEnabled, m_configState.penButtonMode, m_configState.penButtonRoute, m_configState.penButtonRouteExplicit, *m_deviceRuntime)) {
                LOG_INFO("Service", __func__, "IPC",
                         "Reloaded legacy config from {} and rewrote canonical sections. Backup: {}",
                         configPath, backupPath);
            } else {
                LOG_WARN("Service", __func__, "IPC",
                         "Reloaded legacy config from {} but failed to rewrite canonical config.",
                         configPath);
            }
        } else {
            LOG_WARN("Service", __func__, "IPC",
                     "Reloaded legacy config from {} but failed to create backup before migration.",
                     configPath);
        }
    } else {
        LOG_INFO("Service", __func__, "IPC", "Config reloaded from {}.", configPath);
    }

    if (reloadState.restartRequiredFields != 0u) {
        LOG_WARN("Service", __func__, "IPC",
                 "ReloadConfig completed with restart-required fields. changed=0x{:02X} applied_now=0x{:02X} restart_required=0x{:02X}.",
                 static_cast<unsigned int>(reloadState.changedFields),
                 static_cast<unsigned int>(reloadState.appliedFields),
                 static_cast<unsigned int>(reloadState.restartRequiredFields));
    } else {
        LOG_INFO("Service", __func__, "IPC",
                 "ReloadConfig completed. changed=0x{:02X} applied_now=0x{:02X} restart_required=0x{:02X}.",
                 static_cast<unsigned int>(reloadState.changedFields),
                 static_cast<unsigned int>(reloadState.appliedFields),
                 static_cast<unsigned int>(reloadState.restartRequiredFields));
    }

    Ipc::ReloadConfigSummaryWire summary{};
    summary.changedFields = reloadState.changedFields;
    summary.appliedFields = reloadState.appliedFields;
    summary.restartRequiredFields = reloadState.restartRequiredFields;
    std::memcpy(resp.data, &summary, sizeof(summary));
    resp.dataLen = static_cast<uint16_t>(sizeof(summary));
    Ipc::MarkSuccess(resp);
}

void ServiceHost::HandleIpcSaveConfig(Ipc::IpcResponse& resp) {
    HandleIpcPersistConfig(resp);
}

void ServiceHost::HandleIpcGetLogs(Ipc::IpcResponse& resp) {
    auto lines = Common::GuiLogSink::Instance()->DrainNewLines();
    std::string packed;
    for (const auto& l : lines) {
        if (packed.size() + l.size() + 1 > sizeof(resp.data)) {
            break;
        }
        packed += l;
        packed += '\n';
    }

    resp.dataLen = static_cast<uint16_t>(std::min(packed.size(), sizeof(resp.data)));
    std::memcpy(resp.data, packed.data(), resp.dataLen);
    Ipc::MarkSuccess(resp);
}

void ServiceHost::HandleIpcGetPenBridgeStatus(Ipc::IpcResponse& resp) {
    // Pack: [evtRunning:1][pressRunning:1][reportType:1][freq1:1][freq2:1]
    //       [p0..p3 u16 scaled][mode:1][max u16][raw0..raw3 u16]
    // Total: 24 bytes
    uint8_t buf[24] = {};
    buf[0] = (m_impl->m_penEventBridge && m_impl->m_penEventBridge->IsRunning()) ? 1 : 0;
    buf[1] = (m_impl->m_penPressureReader && m_impl->m_penPressureReader->IsRunning()) ? 1 : 0;
    if (m_impl->m_penPressureReader) {
        auto s = m_impl->m_penPressureReader->GetPressureStats();
        buf[2] = s.reportType;
        buf[3] = s.freq1;
        buf[4] = s.freq2;
        for (int k = 0; k < 4; ++k) {
            buf[5 + k * 2] = static_cast<uint8_t>(s.press[k] & 0xFF);
            buf[5 + k * 2 + 1] = static_cast<uint8_t>(s.press[k] >> 8);
        }
        buf[13] = static_cast<uint8_t>(s.pressureMode);
        buf[14] = static_cast<uint8_t>(s.pressureMax & 0xFF);
        buf[15] = static_cast<uint8_t>(s.pressureMax >> 8);
        for (int k = 0; k < 4; ++k) {
            buf[16 + k * 2] = static_cast<uint8_t>(s.rawPress[k] & 0xFF);
            buf[16 + k * 2 + 1] = static_cast<uint8_t>(s.rawPress[k] >> 8);
        }
    }

    std::memcpy(resp.data, buf, sizeof(buf));
    resp.dataLen = sizeof(buf);
    Ipc::MarkSuccess(resp);
}

void ServiceHost::HandleIpcGetDebugSchema(const Ipc::IpcRequest& req, Ipc::IpcResponse& resp) {
    Ipc::DebugSchemaRequest reqSchema{};
    if (req.paramLen >= sizeof(Ipc::DebugSchemaRequest)) {
        std::memcpy(&reqSchema, req.param, sizeof(Ipc::DebugSchemaRequest));
    }

    const uint16_t total = static_cast<uint16_t>(m_impl->m_debugSchema.size());
    const uint16_t offset = std::min<uint16_t>(reqSchema.offset, total);
    const uint16_t maxByPayload = static_cast<uint16_t>(
        (sizeof(resp.data) - sizeof(Ipc::DebugSchemaResponseHeader)) / sizeof(Ipc::DebugFieldSchemaWire));
    uint16_t requested = reqSchema.limit == 0 ? maxByPayload : reqSchema.limit;
    requested = std::min<uint16_t>(requested, maxByPayload);
    const uint16_t available = static_cast<uint16_t>(total - offset);
    const uint16_t take = std::min<uint16_t>(requested, available);

    Ipc::DebugSchemaResponseHeader hdr{};
    hdr.schemaVersion = m_impl->m_debugSchemaVersion;
    hdr.totalFields = total;
    hdr.returnedFields = take;
    hdr.recordSize = static_cast<uint16_t>(sizeof(Ipc::DebugFieldSchemaWire));
    hdr.schemaHash = m_impl->m_debugSchemaHash;

    std::memcpy(resp.data, &hdr, sizeof(hdr));
    size_t cursor = sizeof(hdr);
    for (uint16_t i = 0; i < take; ++i) {
        const auto& def = m_impl->m_debugSchema[offset + i];
        std::memcpy(resp.data + cursor, &def, sizeof(def));
        cursor += sizeof(def);
    }

    resp.dataLen = static_cast<uint16_t>(cursor);
    Ipc::MarkSuccess(resp);
}

void ServiceHost::HandleIpcGetDebugSnapshot(Ipc::IpcResponse& resp) {
    Solvers::HeatmapFrame frame{};
    bool hasFrame = false;
    {
        std::lock_guard<std::mutex> lk(m_impl->m_debugFrameMutex);
        if (m_impl->m_hasLatestDebugFrame) {
            frame = m_impl->m_latestDebugFrame;
            hasFrame = true;
        }
    }

    const bool evtRunning = (m_impl->m_penEventBridge && m_impl->m_penEventBridge->IsRunning());
    const bool pressRunning = (m_impl->m_penPressureReader && m_impl->m_penPressureReader->IsRunning());
    Himax::Pen::PenPressureStats penStats{};
    if (m_impl->m_penPressureReader) {
        penStats = m_impl->m_penPressureReader->GetPressureStats();
    }

    const uint16_t maxByPayload = static_cast<uint16_t>(
        (sizeof(resp.data) - sizeof(Ipc::DebugSnapshotHeader)) / sizeof(Ipc::DebugSnapshotValueWire));
    const uint16_t take = std::min<uint16_t>(static_cast<uint16_t>(m_impl->m_debugSchema.size()), maxByPayload);

    Ipc::DebugSnapshotHeader hdr{};
    hdr.schemaVersion = m_impl->m_debugSchemaVersion;
    hdr.fieldCount = take;
    hdr.recordSize = static_cast<uint16_t>(sizeof(Ipc::DebugSnapshotValueWire));
    std::memcpy(resp.data, &hdr, sizeof(hdr));

    size_t cursor = sizeof(hdr);
    for (uint16_t i = 0; i < take; ++i) {
        const auto& def = m_impl->m_debugSchema[i];
        bool valid = true;
        uint64_t raw = 0;

        const auto sourceKind = static_cast<Ipc::DebugSourceKind>(def.sourceKind);
        if (sourceKind == Ipc::DebugSourceKind::PenBridgeField) {
            raw = EncodePenValue(penStats, evtRunning, pressRunning, def.sourceIndex, valid);
        } else if (hasFrame) {
            raw = EncodeDebugValue(frame, def, valid);
        } else {
            valid = false;
        }

        Ipc::DebugSnapshotValueWire v{};
        v.fieldId = def.fieldId;
        v.valueType = def.valueType;
        v.flags = valid ? 0x1 : 0x0;
        v.rawValue = raw;
        std::memcpy(resp.data + cursor, &v, sizeof(v));
        cursor += sizeof(v);
    }

    resp.dataLen = static_cast<uint16_t>(cursor);
    Ipc::MarkSuccess(resp);
}

// ── IPC Command Handler ──────────────────────────────
Ipc::IpcResponse ServiceHost::HandleIpcCommand(const Ipc::IpcRequest& req) {
    Ipc::IpcResponse resp{};

    switch (req.command) {
    case Ipc::IpcCommand::Ping:
        Ipc::MarkSuccess(resp);
        break;

    case Ipc::IpcCommand::EnterDebugMode:
        HandleIpcEnterDebugMode(resp);
        break;

    case Ipc::IpcCommand::ExitDebugMode:
        HandleIpcExitDebugMode(resp);
        break;

    case Ipc::IpcCommand::AfeCommand:
        if (req.paramLen < 2) {
            Ipc::MarkFailure(resp, Ipc::IpcStatusCode::InvalidRequest);
        } else if (!m_deviceRuntime) {
            Ipc::MarkFailure(resp, Ipc::IpcStatusCode::InvalidState);
        } else if (m_deviceRuntime->SubmitExternalAfeCommand(static_cast<AFE_Command>(req.param[0]), req.param[1])) {
            Ipc::MarkSuccess(resp);
        } else {
            Ipc::MarkFailure(resp, Ipc::IpcStatusCode::InvalidState);
        }
        break;

    case Ipc::IpcCommand::StartRuntime:
        if (m_deviceRuntime) {
            switch (m_deviceRuntime->RequestStart()) {
            case DeviceRuntime::StartRequestResult::Started:
                Ipc::MarkSuccess(resp);
                LOG_INFO("Service", __func__, "IPC", "StartRuntime accepted: runtime started.");
                break;
            case DeviceRuntime::StartRequestResult::AlreadyRunning:
                Ipc::MarkSuccess(resp);
                LOG_INFO("Service", __func__, "IPC", "StartRuntime accepted: runtime already running (idempotent no-op).");
                break;
            case DeviceRuntime::StartRequestResult::Failed:
            default:
                Ipc::MarkFailure(resp, Ipc::IpcStatusCode::InternalError);
                LOG_WARN("Service", __func__, "IPC", "StartRuntime failed: runtime did not start.");
                break;
            }
        } else {
            Ipc::MarkFailure(resp, Ipc::IpcStatusCode::InvalidState);
        }
        break;

    case Ipc::IpcCommand::StopRuntime:
        if (m_deviceRuntime) {
            switch (m_deviceRuntime->RequestStop()) {
            case DeviceRuntime::StopRequestResult::Stopped:
                Ipc::MarkSuccess(resp);
                LOG_INFO("Service", __func__, "IPC", "StopRuntime accepted: runtime stopped.");
                break;
            case DeviceRuntime::StopRequestResult::AlreadyStopped:
            default:
                Ipc::MarkSuccess(resp);
                LOG_INFO("Service", __func__, "IPC", "StopRuntime accepted: runtime already stopped (idempotent no-op).");
                break;
            }
        } else {
            Ipc::MarkFailure(resp, Ipc::IpcStatusCode::InvalidState);
        }
        break;

    case Ipc::IpcCommand::GetConfigSnapshot:
        HandleIpcGetConfigSnapshot(resp);
        break;

    case Ipc::IpcCommand::ApplyConfigPatch:
        HandleIpcApplyConfigPatch(req, resp);
        break;

    case Ipc::IpcCommand::PersistConfig:
        HandleIpcPersistConfig(resp);
        break;

    case Ipc::IpcCommand::ReloadConfig:
        HandleIpcReloadConfig(resp);
        break;

    case Ipc::IpcCommand::SaveConfig:
        HandleIpcSaveConfig(resp);
        break;

    case Ipc::IpcCommand::SetVhfEnabled:
        if (req.paramLen < 1) {
            Ipc::MarkFailure(resp, Ipc::IpcStatusCode::InvalidRequest);
        } else if (!m_deviceRuntime) {
            Ipc::MarkFailure(resp, Ipc::IpcStatusCode::InvalidState);
        } else {
            m_deviceRuntime->SetVhfEnabled(req.param[0] != 0);
            Ipc::MarkSuccess(resp);
        }
        break;

    case Ipc::IpcCommand::SetVhfTranspose:
        if (req.paramLen < 1) {
            Ipc::MarkFailure(resp, Ipc::IpcStatusCode::InvalidRequest);
        } else if (!m_deviceRuntime) {
            Ipc::MarkFailure(resp, Ipc::IpcStatusCode::InvalidState);
        } else {
            m_deviceRuntime->SetVhfTransposeEnabled(req.param[0] != 0);
            Ipc::MarkSuccess(resp);
        }
        break;

    case Ipc::IpcCommand::GetLogs:
        HandleIpcGetLogs(resp);
        break;

    case Ipc::IpcCommand::GetPenBridgeStatus:
        HandleIpcGetPenBridgeStatus(resp);
        break;

    case Ipc::IpcCommand::SetMasterParserOnly:
        if (req.paramLen >= 1 && m_deviceRuntime) {
            m_deviceRuntime->SetMasterParserOnlyMode(req.param[0] != 0);
            Ipc::MarkSuccess(resp);
            LOG_INFO("Service", __func__, "IPC", "Master parser only mode {}.", req.param[0] != 0 ? "enabled" : "disabled");
        } else {
            Ipc::MarkFailure(resp, Ipc::IpcStatusCode::InvalidState);
        }
        break;

    case Ipc::IpcCommand::SetPenPressureMode:
        if (req.paramLen >= 1 && m_impl->m_penPressureReader) {
            const auto mode = req.param[0] == 0
                ? Himax::Pen::PenPressureRangeMode::Raw12Bit4096
                : Himax::Pen::PenPressureRangeMode::Raw14Bit16382;
            m_impl->m_penPressureReader->SetPressureRangeMode(mode);
            Ipc::MarkSuccess(resp);
            LOG_INFO("Service", __func__, "MCU", "Pen pressure mode set to {}.", req.param[0] == 0 ? "4096" : "16382/4");
        } else {
            Ipc::MarkFailure(resp, Ipc::IpcStatusCode::InvalidState);
        }
        break;

    case Ipc::IpcCommand::GetDebugSchema:
        HandleIpcGetDebugSchema(req, resp);
        break;

    case Ipc::IpcCommand::GetDebugSnapshot:
        HandleIpcGetDebugSnapshot(resp);
        break;

    default:
        break;
    }

    return resp;
}

} // namespace Service
