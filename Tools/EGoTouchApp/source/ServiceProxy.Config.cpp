#include "ServiceProxyInternal.h"
#include "Ipc/IpcProtocol.h"
#include "Logger.h"
#include "SolverBuildConfig.h"
#include "config/ConfigBinder.h"
#include "config/ConfigKeyMap.h"
#include "config/ConfigPath.h"
#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstring>
#include <exception>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>


namespace App {

namespace {

Config::ConfigValue ToConfigValue(bool value) {
    return Config::ConfigValue(value);
}

Config::ConfigValue ToConfigValue(std::string_view value) {
    return Config::ConfigValue(std::string(value));
}

const char* ToServiceModeConfig(bool full) {
    return full ? "full" : "touch_only";
}

std::string NormalizeConfigToken(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    std::replace(value.begin(), value.end(), ' ', '_');
    std::replace(value.begin(), value.end(), '+', '_');
    while (value.find("__") != std::string::npos) {
        value.replace(value.find("__"), 2, "_");
    }
    return value;
}

const char* ToPenButtonModeConfig(PenButtonMode mode) {
    switch (mode) {
    case PenButtonMode::OemCustom: return "oem_custom";
    case PenButtonMode::NativeBarrel: return "native_barrel";
    case PenButtonMode::NativeEraser: return "native_eraser";
    }
    return "oem_custom";
}

const char* ToPenButtonRouteConfig(PenButtonRoute route) {
    switch (route) {
    case PenButtonRoute::VhfOnly: return "vhf_only";
    case PenButtonRoute::Win32Only: return "win32_only";
    case PenButtonRoute::VhfAndWin32: return "vhf_and_win32";
    }
    return "vhf_only";
}

std::optional<PenButtonMode> ParsePenButtonModeConfig(const Config::ConfigValue& value) {
    if (const auto* text = std::get_if<std::string>(&value)) {
        const auto normalized = NormalizeConfigToken(*text);
        if (normalized == "oem_custom") return PenButtonMode::OemCustom;
        if (normalized == "native_barrel") return PenButtonMode::NativeBarrel;
        if (normalized == "native_eraser") return PenButtonMode::NativeEraser;
    }
    if (const auto* numeric = std::get_if<int32_t>(&value)) {
        return static_cast<PenButtonMode>(std::clamp(*numeric, 0, 2));
    }
    return std::nullopt;
}

std::optional<PenButtonRoute> ParsePenButtonRouteConfig(const Config::ConfigValue& value) {
    if (const auto* text = std::get_if<std::string>(&value)) {
        const auto normalized = NormalizeConfigToken(*text);
        if (normalized == "vhf_only") return PenButtonRoute::VhfOnly;
        if (normalized == "win32_only") return PenButtonRoute::Win32Only;
        if (normalized == "vhf_and_win32") return PenButtonRoute::VhfAndWin32;
    }
    if (const auto* numeric = std::get_if<int32_t>(&value)) {
        return static_cast<PenButtonRoute>(std::clamp(*numeric, 0, 2));
    }
    return std::nullopt;
}

bool ServiceModeFullFromConfig(const Config::ConfigStore& store, bool fallback) {
    if (store.has("service.mode")) {
        const auto value = store.get<Config::ConfigValue>("service.mode");
        if (const auto* text = std::get_if<std::string>(&value)) {
            std::string normalized = *text;
            std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });
            if (normalized == "full") return true;
            if (normalized == "touch_only") return false;
        }
    }

    if (store.has("service.mode.full")) {
        return store.getOr<bool>("service.mode.full", fallback);
    }

    return fallback;
}

Config::ConfigValue ToConfigValue(PenButtonMode value) {
    return Config::ConfigValue(std::string(ToPenButtonModeConfig(value)));
}

Config::ConfigValue ToConfigValue(PenButtonRoute value) {
    return Config::ConfigValue(std::string(ToPenButtonRouteConfig(value)));
}

void SetValue(Config::ConfigStore& store, std::string_view path, Config::ConfigValue value) {
    store.set<Config::ConfigValue>(path, std::move(value));
}

void PopulateServiceDefaults(Config::ConfigStore& store) {
    SetValue(store, "service.mode", ToConfigValue("full"));
    SetValue(store, "service.auto_mode", ToConfigValue(true));
    SetValue(store, "service.stylus_vhf_enabled", ToConfigValue(true));
    SetValue(store, "service.pen_button_mode", ToConfigValue(PenButtonMode::OemCustom));
    SetValue(store, "service.pen_button_route", ToConfigValue(PenButtonRoute::VhfOnly));
}

void PopulateServiceValues(Config::ConfigStore& store,
                           bool desiredModeFull,
                           bool autoMode,
                           bool stylusVhfEnabled,
                           PenButtonMode penButtonMode,
                           PenButtonRoute penButtonRoute) {
    SetValue(store, "service.mode", ToConfigValue(ToServiceModeConfig(desiredModeFull)));
    SetValue(store, "service.auto_mode", ToConfigValue(autoMode));
    SetValue(store, "service.stylus_vhf_enabled", ToConfigValue(stylusVhfEnabled));
    SetValue(store, "service.pen_button_mode", ToConfigValue(penButtonMode));
    SetValue(store, "service.pen_button_route", ToConfigValue(penButtonRoute));
}

void SyncServiceMirrorsFromStore(Config::ConfigStore& store,
                                 std::atomic<bool>& desiredModeFull,
                                 std::atomic<bool>& activeModeFull,
                                 std::atomic<bool>& autoMode,
                                 std::atomic<bool>& stylusVhfEnabled,
                                 std::atomic<PenButtonMode>& penButtonMode,
                                 std::atomic<PenButtonRoute>& penButtonRoute) {
    const bool full = ServiceModeFullFromConfig(store, desiredModeFull.load(std::memory_order_relaxed));
    desiredModeFull.store(full, std::memory_order_relaxed);
    activeModeFull.store(full, std::memory_order_relaxed);
    autoMode.store(store.getOr<bool>("service.auto_mode", autoMode.load(std::memory_order_relaxed)), std::memory_order_relaxed);
    stylusVhfEnabled.store(store.getOr<bool>("service.stylus_vhf_enabled", stylusVhfEnabled.load(std::memory_order_relaxed)), std::memory_order_relaxed);
    if (store.has("service.pen_button_mode")) {
        const auto value = store.get<Config::ConfigValue>("service.pen_button_mode");
        const auto parsed = ParsePenButtonModeConfig(value);
        penButtonMode.store(parsed.value_or(penButtonMode.load(std::memory_order_relaxed)), std::memory_order_relaxed);
    }
    if (store.has("service.pen_button_route")) {
        const auto value = store.get<Config::ConfigValue>("service.pen_button_route");
        const auto parsed = ParsePenButtonRouteConfig(value);
        penButtonRoute.store(parsed.value_or(penButtonRoute.load(std::memory_order_relaxed)), std::memory_order_relaxed);
    }

    PopulateServiceValues(
        store,
        desiredModeFull.load(std::memory_order_relaxed),
        autoMode.load(std::memory_order_relaxed),
        stylusVhfEnabled.load(std::memory_order_relaxed),
        penButtonMode.load(std::memory_order_relaxed),
        penButtonRoute.load(std::memory_order_relaxed));
}

} // namespace

void ServiceProxy::InitConfigSchema() {
    Config::ConfigBinder binder;
    m_pipeline.registerBindings(binder);
    m_stylusPipeline.registerBindings(binder);

    m_configDefaults = Config::ConfigStore{};
    PopulateServiceDefaults(m_configDefaults);
    binder.writeDefaults(m_configDefaults);

    m_configStore = Config::ConfigStore{};
    m_configStore.mergeFrom(m_configDefaults);
    binder.writeCurrent(m_configStore);

    const auto paths = Config::resolve();
    if (paths.has_value()) {
        try {
            Config::ConfigStore yamlStore;
            yamlStore.loadFromYaml(paths->defaultConfig);
            m_configStore.mergeFrom(yamlStore);
            if (paths->overrideExists) {
                Config::ConfigStore overrides;
                overrides.loadFromYaml(paths->overrideConfig);
                m_configStore.mergeFrom(overrides);
            }
            LOG_INFO("App", __func__, "Config", "Loaded app-local preview config: default='{}' overrides='{}' overrideExists={}",
                     paths->defaultConfig, paths->overrideConfig, paths->overrideExists);
        } catch (const std::exception& ex) {
            LOG_WARN("App", __func__, "Config", "Failed to load YAML config for app-local preview; using binder defaults: {}", ex.what());
        }
    } else {
        LOG_WARN("App", __func__, "Config", "config/default.yaml not found; using binder defaults for app-local preview.");
    }

    SyncServiceMirrorsFromStore(
        m_configStore,
        m_srvDesiredModeFull,
        m_srvActiveModeFull,
        m_srvAutoMode,
        m_srvStylusVhfEnabled,
        m_srvPenButtonMode,
        m_srvPenButtonRoute);
    ApplyConfigStoreToLocalRuntime();

    m_configSchema = Config::BuildMergedSchema(m_configDefaults, binder);
}

void ServiceProxy::ApplyConfigStoreToLocalRuntime() {
    m_pipeline.applyConfig(m_configStore);
    m_stylusPipeline.applyConfig(m_configStore);
}

std::vector<std::string> ServiceProxy::GetConfigModuleTags() const {
    std::vector<std::string> tags;
    tags.reserve(m_configSchema.entries.size());
    for (const auto& entry : m_configSchema.entries) {
        if (!entry.moduleTag.empty()) {
            tags.push_back(entry.moduleTag);
        }
    }
    std::sort(tags.begin(), tags.end());
    tags.erase(std::unique(tags.begin(), tags.end()), tags.end());
    return tags;
}

void ServiceProxy::SaveConfig() {
#ifndef _DEBUG
    static std::atomic_bool loggedReleaseNoOp{false};
    if (!loggedReleaseNoOp.exchange(true, std::memory_order_relaxed)) {
        LOG_WARN("App", __func__, "Config", "Release builds read YAML only at Service startup; live config mutation is not supported.");
    }
    return;
#else
    if (!IsLiveControlAllowed() || !m_client.IsConnected()) return;

    Ipc::ApplyConfigPatchRequestWire patch{};
    patch.fieldMask = Ipc::ToBits(Ipc::ServiceConfigFieldWire::Mode) |
                      Ipc::ToBits(Ipc::ServiceConfigFieldWire::AutoMode) |
                      Ipc::ToBits(Ipc::ServiceConfigFieldWire::StylusVhfEnabled) |
                      Ipc::ToBits(Ipc::ServiceConfigFieldWire::PenButtonMode) |
                      Ipc::ToBits(Ipc::ServiceConfigFieldWire::PenButtonRoute);
    patch.desiredMode = static_cast<uint8_t>(
        m_srvDesiredModeFull.load(std::memory_order_relaxed)
            ? Ipc::ServiceModeWire::Full
            : Ipc::ServiceModeWire::TouchOnly);
    patch.autoMode = m_srvAutoMode.load(std::memory_order_relaxed) ? 1 : 0;
    patch.stylusVhfEnabled = m_srvStylusVhfEnabled.load(std::memory_order_relaxed) ? 1 : 0;
    patch.penButtonMode = static_cast<uint8_t>(m_srvPenButtonMode.load(std::memory_order_relaxed));
    patch.penButtonRoute = static_cast<uint8_t>(m_srvPenButtonRoute.load(std::memory_order_relaxed));

    const auto applyResp = m_client.ApplyConfigPatch(patch);
    if (!applyResp.success) {
        LOG_WARN("App", __func__, "IPC", "ApplyConfigPatch failed with status={}", static_cast<unsigned int>(applyResp.status));
        return;
    }

    const auto persistResp = m_client.PersistConfig();
    if (!persistResp.success) {
        LOG_WARN("App", __func__, "IPC", "PersistConfig failed with status={}", static_cast<unsigned int>(persistResp.status));
        return;
    }

    const auto snapshotResp = m_client.GetConfigSnapshot();
    if (snapshotResp.success && snapshotResp.dataLen >= sizeof(Ipc::ConfigSnapshotWire)) {
        Ipc::ConfigSnapshotWire snapshot{};
        std::memcpy(&snapshot, snapshotResp.data, sizeof(snapshot));
        m_srvDesiredModeFull.store(snapshot.desiredMode == static_cast<uint8_t>(Ipc::ServiceModeWire::Full), std::memory_order_relaxed);
        m_srvActiveModeFull.store(snapshot.activeMode == static_cast<uint8_t>(Ipc::ServiceModeWire::Full), std::memory_order_relaxed);
        m_srvAutoMode.store(snapshot.autoMode != 0, std::memory_order_relaxed);
        m_srvStylusVhfEnabled.store(snapshot.stylusVhfEnabled != 0, std::memory_order_relaxed);
        m_srvPenButtonMode.store(static_cast<PenButtonMode>(snapshot.penButtonMode), std::memory_order_relaxed);
        m_srvPenButtonRoute.store(static_cast<PenButtonRoute>(snapshot.penButtonRoute), std::memory_order_relaxed);
        PopulateServiceValues(
            m_configStore,
            m_srvDesiredModeFull.load(std::memory_order_relaxed),
            m_srvAutoMode.load(std::memory_order_relaxed),
            m_srvStylusVhfEnabled.load(std::memory_order_relaxed),
            m_srvPenButtonMode.load(std::memory_order_relaxed),
            m_srvPenButtonRoute.load(std::memory_order_relaxed));
    }

    LOG_INFO("App", __func__, "IPC", "Config patch persisted through service IPC; local INI persistence is disabled.");
#endif
}

void ServiceProxy::RefreshConfigSnapshot() {
    if (!m_client.IsConnected()) return;

    const auto resp = m_client.GetConfigSnapshot();
    if (resp.success && resp.dataLen >= sizeof(Ipc::ConfigSnapshotWire)) {
        Ipc::ConfigSnapshotWire snapshot{};
        std::memcpy(&snapshot, resp.data, sizeof(snapshot));
        m_srvDesiredModeFull.store(
            snapshot.desiredMode == static_cast<uint8_t>(Ipc::ServiceModeWire::Full),
            std::memory_order_relaxed);
        m_srvActiveModeFull.store(
            snapshot.activeMode == static_cast<uint8_t>(Ipc::ServiceModeWire::Full),
            std::memory_order_relaxed);
        m_srvAutoMode.store(snapshot.autoMode != 0, std::memory_order_relaxed);
        m_srvStylusVhfEnabled.store(snapshot.stylusVhfEnabled != 0, std::memory_order_relaxed);
        m_srvPenButtonMode.store(
            static_cast<PenButtonMode>(snapshot.penButtonMode),
            std::memory_order_relaxed);
        m_srvPenButtonRoute.store(
            static_cast<PenButtonRoute>(snapshot.penButtonRoute),
            std::memory_order_relaxed);
        PopulateServiceValues(
            m_configStore,
            m_srvDesiredModeFull.load(std::memory_order_relaxed),
            m_srvAutoMode.load(std::memory_order_relaxed),
            m_srvStylusVhfEnabled.load(std::memory_order_relaxed),
            m_srvPenButtonMode.load(std::memory_order_relaxed),
            m_srvPenButtonRoute.load(std::memory_order_relaxed));
    }
}

void ServiceProxy::SetSrvModeFull(bool full) {
    if (!IsLiveControlAllowed()) return;
    m_srvDesiredModeFull.store(full, std::memory_order_relaxed);
    SetValue(m_configStore, "service.mode", ToConfigValue(ToServiceModeConfig(full)));
}

void ServiceProxy::SetSrvStylusVhfEnabled(bool enabled) {
    if (!IsLiveControlAllowed()) return;
    m_srvStylusVhfEnabled.store(enabled, std::memory_order_relaxed);
    SetValue(m_configStore, "service.stylus_vhf_enabled", ToConfigValue(enabled));
}

void ServiceProxy::SetSrvAutoMode(bool enabled) {
    if (!IsLiveControlAllowed()) return;
    m_srvAutoMode.store(enabled, std::memory_order_relaxed);
    SetValue(m_configStore, "service.auto_mode", ToConfigValue(enabled));
}

void ServiceProxy::SetPenButtonMode(PenButtonMode m) {
    if (!IsLiveControlAllowed()) return;
    m_srvPenButtonMode.store(m, std::memory_order_relaxed);
    SetValue(m_configStore, "service.pen_button_mode", ToConfigValue(m));
}

void ServiceProxy::SetPenButtonRoute(PenButtonRoute r) {
    if (!IsLiveControlAllowed()) return;
    m_srvPenButtonRoute.store(r, std::memory_order_relaxed);
    SetValue(m_configStore, "service.pen_button_route", ToConfigValue(r));
}

// ── MasterParser-only mode (local) ──
void ServiceProxy::SetMasterParserOnlyMode(bool enabled) {
    if (enabled == m_masterParserOnly.load(std::memory_order_relaxed)) return;

    if (IsConnected() && IsLiveControlAllowed()) {
        Ipc::IpcRequest req{};
        req.command = Ipc::IpcCommand::SetMasterParserOnly;
        req.param[0] = enabled ? 1 : 0;
        req.paramLen = 1;
        m_client.Send(req);
    }

    if (enabled) {
        m_masterParserOnlySnapshot = CaptureTouchPipelineModuleEnableState(m_pipeline);
        TouchPipelineModuleEnableState parserOnlyState = *m_masterParserOnlySnapshot;
        parserOnlyState.baselineEnabled = false;
        parserOnlyState.cmfEnabled = false;
        parserOnlyState.gridIIREnabled = false;
        parserOnlyState.trackerEnabled = false;
        parserOnlyState.coordFilterEnabled = false;
        parserOnlyState.gestureEnabled = false;
        ApplyTouchPipelineModuleEnableState(m_pipeline, parserOnlyState);
    } else if (m_masterParserOnlySnapshot.has_value()) {
        ApplyTouchPipelineModuleEnableState(m_pipeline, *m_masterParserOnlySnapshot);
        m_masterParserOnlySnapshot.reset();
    }

    m_masterParserOnly.store(enabled, std::memory_order_relaxed);
}

} // namespace App
