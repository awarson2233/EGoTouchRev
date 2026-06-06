#include "ServiceProxyInternal.h"
#include "Ipc/IpcProtocol.h"
#include "Logger.h"
#include "SolverBuildConfig.h"
#include "config/ConfigBinder.h"
#include "config/ConfigKeyMap.h"
#include "config/ConfigPath.h"
#include "config/ConfigTlv.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cstring>
#include <cstdint>
#include <exception>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
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
    std::replace(value.begin(), value.end(), '-', '_');
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
        if (*numeric >= 0 && *numeric <= 2) {
            return static_cast<PenButtonMode>(*numeric);
        }
    }
    return std::nullopt;
}

std::optional<PenButtonRoute> ParsePenButtonRouteConfig(const Config::ConfigValue& value) {
    if (const auto* text = std::get_if<std::string>(&value)) {
        const auto normalized = NormalizeConfigToken(*text);
        if (normalized == "vhf_only") return PenButtonRoute::VhfOnly;
        if (normalized == "win32_only") return PenButtonRoute::Win32Only;
        if (normalized == "vhf_and_win32" || normalized == "vhf_win32") return PenButtonRoute::VhfAndWin32;
    }
    if (const auto* numeric = std::get_if<int32_t>(&value)) {
        if (*numeric >= 0 && *numeric <= 2) {
            return static_cast<PenButtonRoute>(*numeric);
        }
    }
    return std::nullopt;
}

void ApplyLegacyServiceModeMigration(Config::ConfigStore& target, const Config::ConfigStore& source) {
    if (source.has("service.mode") || !source.has("service.mode.full")) {
        return;
    }

    target.set<Config::ConfigValue>(
        "service.mode",
        ToConfigValue(source.getOr<bool>("service.mode.full", true) ? "full" : "touch_only"));
}

bool ServiceModeFullFromConfig(const Config::ConfigStore& store, bool fallback) {
    if (store.has("service.mode")) {
        const auto value = store.get<Config::ConfigValue>("service.mode");
        if (const auto* text = std::get_if<std::string>(&value)) {
            const auto normalized = NormalizeConfigToken(*text);
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

Config::ConfigValueType ValueTypeFor(const Config::ConfigValue& value) {
    return std::visit([](const auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, bool>) {
            return Config::ConfigValueType::Bool;
        } else if constexpr (std::is_same_v<T, int32_t>) {
            return Config::ConfigValueType::Int32;
        } else if constexpr (std::is_same_v<T, float>) {
            return Config::ConfigValueType::Float;
        } else {
            return Config::ConfigValueType::String;
        }
    }, value);
}

Config::ConfigTlvEntry BuildTlvEntry(Config::ConfigKeyId keyId, const Config::ConfigValue& value) {
    return Config::ConfigTlvEntry{
        .keyId = keyId,
        .valueType = ValueTypeFor(value),
        .stringValue = Config::toString(value),
    };
}

bool IsLivePatchableEntry(const Config::ConfigSchemaEntry& entry) {
    return entry.boundToRuntime &&
           (entry.runtimeBinding == Config::ConfigRuntimeBinding::LiveSetter ||
            entry.runtimeBinding == Config::ConfigRuntimeBinding::ManualLiveApply);
}

enum class ServiceModeSchema {
    Full,
    TouchOnly,
};

struct ServiceSchemaState {
    ServiceModeSchema mode = ServiceModeSchema::Full;
    bool autoMode = true;
    bool stylusVhfEnabled = true;
    PenButtonMode penButtonMode = PenButtonMode::OemCustom;
    PenButtonRoute penButtonRoute = PenButtonRoute::VhfOnly;
};

void RegisterServiceConfigSchemaBindings(Config::ConfigBinder& binder,
                                         ServiceSchemaState& state) {
    static const std::array<std::pair<ServiceModeSchema, std::string>, 2> kModeMapping{{
        {ServiceModeSchema::Full, "full"},
        {ServiceModeSchema::TouchOnly, "touch_only"},
    }};
    static const std::array<std::pair<PenButtonMode, std::string>, 3> kPenButtonModeMapping{{
        {PenButtonMode::OemCustom, "oem_custom"},
        {PenButtonMode::NativeBarrel, "native_barrel"},
        {PenButtonMode::NativeEraser, "native_eraser"},
    }};
    static const std::array<std::pair<PenButtonRoute, std::string>, 3> kPenButtonRouteMapping{{
        {PenButtonRoute::VhfOnly, "vhf_only"},
        {PenButtonRoute::Win32Only, "win32_only"},
        {PenButtonRoute::VhfAndWin32, "vhf_and_win32"},
    }};

    constexpr auto runtimeBinding = Config::ConfigRuntimeBinding::ManualLiveApply;
    binder.bindEnum("service.mode", &ServiceSchemaState::mode, state,
                    ServiceModeSchema::Full, std::span<const std::pair<ServiceModeSchema, std::string>>(kModeMapping), "Service runtime topology", runtimeBinding);
    binder.bind("service.auto_mode", &ServiceSchemaState::autoMode, state,
                true, {}, "Enable automatic runtime start/init", runtimeBinding);
    binder.bind("service.stylus_vhf_enabled", &ServiceSchemaState::stylusVhfEnabled, state,
                true, {}, "Enable stylus VHF output", runtimeBinding);
    binder.bindEnum("service.pen_button_mode", &ServiceSchemaState::penButtonMode, state,
                    PenButtonMode::OemCustom, std::span<const std::pair<PenButtonMode, std::string>>(kPenButtonModeMapping), "Pen button semantic mode", runtimeBinding);
    binder.bindEnum("service.pen_button_route", &ServiceSchemaState::penButtonRoute, state,
                    PenButtonRoute::VhfOnly, std::span<const std::pair<PenButtonRoute, std::string>>(kPenButtonRouteMapping), "Pen button injection route", runtimeBinding);
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
    ServiceSchemaState serviceSchemaState;
    RegisterServiceConfigSchemaBindings(binder, serviceSchemaState);
    m_pipeline.registerBindings(binder);
    m_stylusPipeline.registerBindings(binder);
    Config::registerRuntimeKeyMappings(binder);

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
            ApplyLegacyServiceModeMigration(m_configStore, yamlStore);
            if (paths->overrideExists) {
                Config::ConfigStore overrides;
                overrides.loadFromYaml(paths->overrideConfig);
                m_configStore.mergeFrom(overrides);
                ApplyLegacyServiceModeMigration(m_configStore, overrides);
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

void ServiceProxy::MarkConfigPathsDirty(const std::vector<std::string>& paths) {
    for (const auto& path : paths) {
        m_dirtyConfigPaths.insert(path);
    }
}

ApplyConfigResult ServiceProxy::GetLastApplyConfigResult() const {
    ApplyConfigResult result{};
    result.liveApplied = m_lastApplyConfigLiveApplied.load(std::memory_order_relaxed);
    result.persistAttempted = m_lastApplyConfigPersistAttempted.load(std::memory_order_relaxed);
    result.persisted = m_lastApplyConfigPersisted.load(std::memory_order_relaxed);
    result.unpersistedLiveChanges = m_hasUnpersistedLiveConfigChanges.load(std::memory_order_relaxed);
    result.persistStatus = static_cast<Ipc::IpcStatusCode>(m_lastApplyConfigPersistStatus.load(std::memory_order_relaxed));
    return result;
}

bool ServiceProxy::ApplyConfigStoreGlobally() {
    m_lastApplyConfigLiveApplied.store(false, std::memory_order_relaxed);
    m_lastApplyConfigPersistAttempted.store(false, std::memory_order_relaxed);
    m_lastApplyConfigPersisted.store(false, std::memory_order_relaxed);
    m_lastApplyConfigPersistStatus.store(static_cast<uint8_t>(Ipc::IpcStatusCode::InternalError), std::memory_order_relaxed);
#if !EGOTOUCH_CONFIG_ENABLED
    return false;
#endif
    if (!IsLiveControlAllowed() || !m_client.IsConnected()) {
        LOG_WARN("App", __func__, "IPC", "Global config apply skipped; live control is not allowed or IPC is disconnected.");
        return false;
    }

    Config::ConfigPatchTlv patch{};
    size_t skippedKeys = 0;
    std::vector<std::string> appliedPaths;
    std::vector<std::string> dirtyPaths(m_dirtyConfigPaths.begin(), m_dirtyConfigPaths.end());
    std::sort(dirtyPaths.begin(), dirtyPaths.end());
    for (const auto& path : dirtyPaths) {
        auto keyId = Config::tryKeyIdForPath(path);
        const auto schemaIt = std::find_if(m_configSchema.entries.begin(), m_configSchema.entries.end(),
            [&path](const Config::ConfigSchemaEntry& entry) { return entry.yamlPath == path; });
        if (!keyId.has_value() || !m_configStore.has(path) ||
            schemaIt == m_configSchema.entries.end() || !IsLivePatchableEntry(*schemaIt)) {
            ++skippedKeys;
            continue;
        }

        const Config::ConfigValue value = m_configStore.get<Config::ConfigValue>(path);
        patch.entries.push_back(BuildTlvEntry(*keyId, value));
        appliedPaths.push_back(path);
    }

    if (patch.entries.empty()) {
        LOG_WARN("App", __func__, "Config", "Global config apply produced no TLV entries; skippedKeys={}", skippedKeys);
        return false;
    }

    std::vector<uint8_t> payload;
    try {
        payload = Config::serializePatch(patch);
    } catch (const std::exception& ex) {
        LOG_WARN("App", __func__, "Config", "Failed to serialize global config patch: {}", ex.what());
        return false;
    }

    if (payload.size() > Ipc::kConfigTlvMaxPayloadBytes) {
        LOG_WARN("App", __func__, "IPC", "Global config patch too large: {} bytes", payload.size());
        return false;
    }

    static std::atomic<uint16_t> nextSession{1};
    uint16_t sessionId = nextSession.fetch_add(1, std::memory_order_relaxed);
    if (sessionId == 0) {
        sessionId = nextSession.fetch_add(1, std::memory_order_relaxed);
    }

    size_t offset = 0;
    Ipc::IpcResponse applyResp{};
    while (offset < payload.size()) {
        const size_t take = std::min<size_t>(Ipc::kConfigTlvChunkPayloadBytes, payload.size() - offset);
        Ipc::ConfigTlvChunkRequestWire chunk{};
        chunk.sessionId = sessionId;
        chunk.totalLen = static_cast<uint16_t>(payload.size());
        chunk.offset = static_cast<uint16_t>(offset);
        chunk.chunkLen = static_cast<uint16_t>(take);
        chunk.flags = 0;
        if (offset == 0) {
            chunk.flags |= Ipc::kConfigTlvChunkFirst;
        }
        if (offset + take == payload.size()) {
            chunk.flags |= Ipc::kConfigTlvChunkLast;
        }
        std::memcpy(chunk.bytes, payload.data() + offset, take);

        applyResp = m_client.ApplyConfigTlvChunk(chunk);
        if (!applyResp.success) {
            LOG_WARN("App", __func__, "IPC", "ApplyConfigTlvChunk failed with status={} offset={}", static_cast<unsigned int>(applyResp.status), offset);
            return false;
        }
        offset += take;
    }

    m_lastApplyConfigLiveApplied.store(true, std::memory_order_relaxed);

    const auto persistResp = m_client.PersistConfig();
    m_lastApplyConfigPersistAttempted.store(true, std::memory_order_relaxed);
    m_lastApplyConfigPersisted.store(persistResp.success, std::memory_order_relaxed);
    m_lastApplyConfigPersistStatus.store(static_cast<uint8_t>(persistResp.status), std::memory_order_relaxed);
    if (!persistResp.success) {
        m_hasUnpersistedLiveConfigChanges.store(true, std::memory_order_relaxed);
        LOG_WARN("App", __func__, "IPC", "PersistConfig failed after live apply with status={}; keeping live apply success and dirty paths for retry", static_cast<unsigned int>(persistResp.status));
    }

    RefreshConfigSnapshot();
    ApplyConfigStoreToLocalRuntime();
    if (persistResp.success) {
        for (const auto& path : appliedPaths) {
            m_dirtyConfigPaths.erase(path);
        }
        if (m_dirtyConfigPaths.empty()) {
            m_hasUnpersistedLiveConfigChanges.store(false, std::memory_order_relaxed);
        }
    }
    LOG_INFO("App", __func__, "IPC", "Global config live-applied entries={} skippedKeys={} payloadBytes={} persisted={} unpersistedLiveChanges={}",
             patch.entries.size(), skippedKeys, payload.size(), persistResp.success ? 1 : 0,
             m_hasUnpersistedLiveConfigChanges.load(std::memory_order_relaxed) ? 1 : 0);
    return true;
}

void ServiceProxy::SaveConfig() {
    (void)ApplyConfigStoreGlobally();
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
    m_dirtyConfigPaths.insert("service.mode");
}

void ServiceProxy::SetSrvStylusVhfEnabled(bool enabled) {
    if (!IsLiveControlAllowed()) return;
    m_srvStylusVhfEnabled.store(enabled, std::memory_order_relaxed);
    SetValue(m_configStore, "service.stylus_vhf_enabled", ToConfigValue(enabled));
    m_dirtyConfigPaths.insert("service.stylus_vhf_enabled");
}

void ServiceProxy::SetSrvAutoMode(bool enabled) {
    if (!IsLiveControlAllowed()) return;
    m_srvAutoMode.store(enabled, std::memory_order_relaxed);
    SetValue(m_configStore, "service.auto_mode", ToConfigValue(enabled));
    m_dirtyConfigPaths.insert("service.auto_mode");
}

void ServiceProxy::SetPenButtonMode(PenButtonMode m) {
    if (!IsLiveControlAllowed()) return;
    m_srvPenButtonMode.store(m, std::memory_order_relaxed);
    SetValue(m_configStore, "service.pen_button_mode", ToConfigValue(m));
    m_dirtyConfigPaths.insert("service.pen_button_mode");
}

void ServiceProxy::SetPenButtonRoute(PenButtonRoute r) {
    if (!IsLiveControlAllowed()) return;
    m_srvPenButtonRoute.store(r, std::memory_order_relaxed);
    SetValue(m_configStore, "service.pen_button_route", ToConfigValue(r));
    m_dirtyConfigPaths.insert("service.pen_button_route");
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
