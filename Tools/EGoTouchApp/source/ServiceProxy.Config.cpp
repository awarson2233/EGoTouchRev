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

Config::ConfigValue ToConfigValue(const char* value) {
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

// App-local YAML compatibility only: accepts pre-catalog default/override files
// that still used service.mode.full. Connected Service sync never uses this path.
void ApplyAppLocalYamlServiceModeMigration(Config::ConfigStore& target, const Config::ConfigStore& source) {
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
            entry.runtimeBinding == Config::ConfigRuntimeBinding::ManualLiveApply) &&
           Config::isLiveApplyTiming(entry.applyTiming);
}

bool IsV3PatchableEntry(const Config::ConfigSchemaEntry& entry) {
    return entry.boundToRuntime &&
           (entry.runtimeBinding == Config::ConfigRuntimeBinding::LiveSetter ||
            entry.runtimeBinding == Config::ConfigRuntimeBinding::ManualLiveApply) &&
           (Config::isLiveApplyTiming(entry.applyTiming) ||
            entry.applyTiming == Config::ConfigApplyTiming::RestartRequired);
}

std::optional<Config::ConfigValue> NormalizeSnapshotValueForSchema(const Config::ConfigSchemaEntry& schemaEntry,
                                                                 const Config::ConfigValue& value) {
    if (schemaEntry.uiType != Config::ConfigUiType::Enum && schemaEntry.enumMapping.empty()) {
        return value;
    }

    const auto* numeric = std::get_if<int32_t>(&value);
    if (numeric == nullptr) {
        return value;
    }

    const auto it = std::ranges::find_if(schemaEntry.enumMapping, [numeric](const auto& item) {
        return item.first == *numeric;
    });
    if (it == schemaEntry.enumMapping.end()) {
        LOG_WARN("App", __func__, "Config", "Skipping unmapped numeric enum snapshot value path={} value={}",
                 schemaEntry.yamlPath, *numeric);
        return std::nullopt;
    }

    return Config::ConfigValue(it->second);
}

bool IsKnownConfigV3MutationStatus(uint8_t status) {
    switch (static_cast<Ipc::ConfigV3MutationStatus>(status)) {
    case Ipc::ConfigV3MutationStatus::Ok:
    case Ipc::ConfigV3MutationStatus::NoChanges:
    case Ipc::ConfigV3MutationStatus::VersionMismatch:
    case Ipc::ConfigV3MutationStatus::Rejected:
    case Ipc::ConfigV3MutationStatus::PersistFailed:
        return true;
    }
    return false;
}

std::optional<Ipc::ConfigV3ApplyResultWire> DecodeConfigV3ApplyResult(const Ipc::IpcResponse& response) {
    if (!response.success || response.dataLen < sizeof(Ipc::ConfigV3ApplyResultWire)) {
        return std::nullopt;
    }

    Ipc::ConfigV3ApplyResultWire result{};
    std::memcpy(&result, response.data, sizeof(result));
    if (result.wireVersion != Ipc::kIpcProtocolVersion || !IsKnownConfigV3MutationStatus(result.status)) {
        return std::nullopt;
    }
    return result;
}

std::optional<Ipc::PersistConfigV3ResponseWire> DecodeConfigV3PersistResult(const Ipc::IpcResponse& response) {
    if (!response.success || response.dataLen < sizeof(Ipc::PersistConfigV3ResponseWire)) {
        return std::nullopt;
    }

    Ipc::PersistConfigV3ResponseWire result{};
    std::memcpy(&result, response.data, sizeof(result));
    if (result.wireVersion != Ipc::kIpcProtocolVersion || !IsKnownConfigV3MutationStatus(result.status)) {
        return std::nullopt;
    }
    return result;
}

const Config::ConfigSchemaEntry* FindSchemaEntryByPath(const Config::ConfigSchemaSnapshot& schema,
                                                       std::string_view path) {
    const auto it = std::ranges::find_if(schema.entries, [path](const Config::ConfigSchemaEntry& entry) {
        return entry.yamlPath == path;
    });
    return it == schema.entries.end() ? nullptr : &*it;
}

std::optional<Config::ConfigValue> TryGetStoreValue(const Config::ConfigStore& store,
                                                    std::string_view path) {
    if (!store.has(path)) {
        return std::nullopt;
    }
    return store.get<Config::ConfigValue>(path);
}

bool StoreValueEquals(const Config::ConfigStore& store,
                      std::string_view path,
                      const Config::ConfigValue& value) {
    const auto current = TryGetStoreValue(store, path);
    return current.has_value() && *current == value;
}

bool IsAppliedButUnpersisted(const ConfigDraftPathState& state) {
    const bool applied = state.applyState == ConfigDraftApplyState::LiveApplied ||
                         state.applyState == ConfigDraftApplyState::StagedRestartRequired;
    const bool unpersisted = state.persistState == ConfigDraftPersistState::Unpersisted ||
                             state.persistState == ConfigDraftPersistState::Failed;
    return state.dirty && applied && unpersisted;
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

void SyncServiceDraftMirrorsFromStore(const Config::ConfigStore& store,
                                      std::atomic<bool>& desiredModeFull,
                                      std::atomic<bool>& autoMode,
                                      std::atomic<bool>& stylusVhfEnabled,
                                      std::atomic<PenButtonMode>& penButtonMode,
                                      std::atomic<PenButtonRoute>& penButtonRoute) {
    desiredModeFull.store(
        ServiceModeFullFromConfig(store, desiredModeFull.load(std::memory_order_relaxed)),
        std::memory_order_relaxed);
    autoMode.store(store.getOr<bool>("service.auto_mode", autoMode.load(std::memory_order_relaxed)),
                   std::memory_order_relaxed);
    stylusVhfEnabled.store(store.getOr<bool>("service.stylus_vhf_enabled", stylusVhfEnabled.load(std::memory_order_relaxed)),
                           std::memory_order_relaxed);
    if (store.has("service.pen_button_mode")) {
        const auto parsed = ParsePenButtonModeConfig(store.get<Config::ConfigValue>("service.pen_button_mode"));
        penButtonMode.store(parsed.value_or(penButtonMode.load(std::memory_order_relaxed)), std::memory_order_relaxed);
    }
    if (store.has("service.pen_button_route")) {
        const auto parsed = ParsePenButtonRouteConfig(store.get<Config::ConfigValue>("service.pen_button_route"));
        penButtonRoute.store(parsed.value_or(penButtonRoute.load(std::memory_order_relaxed)), std::memory_order_relaxed);
    }
}

void SyncServiceActiveMirrorFromSnapshot(const Config::ConfigStore& store,
                                         std::atomic<bool>& activeModeFull) {
    activeModeFull.store(
        ServiceModeFullFromConfig(store, activeModeFull.load(std::memory_order_relaxed)),
        std::memory_order_relaxed);
}

} // namespace

void ServiceProxy::InitConfigSchema() {
    Config::ConfigBinder binder;
    ServiceSchemaState serviceSchemaState;
    RegisterServiceConfigSchemaBindings(binder, serviceSchemaState);
    m_pipeline.registerBindings(binder);
    m_stylusPipeline.registerBindings(binder);
    Config::registerRuntimeKeyMappings(binder);

    m_configDraft = ConfigDraft{};
    m_configV3CatalogReady = false;
    PopulateServiceDefaults(m_configDraft.catalogDefaults);
    binder.writeDefaults(m_configDraft.catalogDefaults);

    m_configDraft.editableDraft = Config::ConfigStore{};
    m_configDraft.editableDraft.mergeFrom(m_configDraft.catalogDefaults);
    binder.writeCurrent(m_configDraft.editableDraft);

    const auto paths = Config::resolve();
    if (paths.has_value()) {
        try {
            Config::ConfigStore yamlStore;
            yamlStore.loadFromYaml(paths->defaultConfig);
            m_configDraft.editableDraft.mergeFrom(yamlStore);
            ApplyAppLocalYamlServiceModeMigration(m_configDraft.editableDraft, yamlStore);
            if (paths->overrideExists) {
                Config::ConfigStore overrides;
                overrides.loadFromYaml(paths->overrideConfig);
                m_configDraft.editableDraft.mergeFrom(overrides);
                ApplyAppLocalYamlServiceModeMigration(m_configDraft.editableDraft, overrides);
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
        m_configDraft.editableDraft,
        m_srvDesiredModeFull,
        m_srvActiveModeFull,
        m_srvAutoMode,
        m_srvStylusVhfEnabled,
        m_srvPenButtonMode,
        m_srvPenButtonRoute);
    ApplyConfigStoreToLocalRuntime();

    m_configSchema = Config::BuildMergedSchema(m_configDraft.catalogDefaults, binder);
}

Config::ConfigSchemaSnapshot BuildServiceProxyConfigSchemaSnapshotForTest() {
    Config::ConfigBinder binder;
    ServiceSchemaState serviceSchemaState;
    Solvers::TouchPipeline touchPipeline;
    Solvers::StylusPipeline stylusPipeline;

    RegisterServiceConfigSchemaBindings(binder, serviceSchemaState);
    touchPipeline.registerBindings(binder);
    stylusPipeline.registerBindings(binder);
    Config::registerRuntimeKeyMappings(binder);

    Config::ConfigStore defaults;
    PopulateServiceDefaults(defaults);
    binder.writeDefaults(defaults);
    return Config::BuildMergedSchema(defaults, binder);
}

void ServiceProxy::ApplyConfigStoreToLocalRuntime() {
    m_pipeline.applyConfig(m_configDraft.editableDraft);
    m_stylusPipeline.applyConfig(m_configDraft.editableDraft);
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
        if (path.empty()) {
            continue;
        }

        const auto draftValue = TryGetStoreValue(m_configDraft.editableDraft, path);
        const auto existingIt = m_configDraft.pathStates.find(path);
        const bool appliedButUnpersisted = existingIt != m_configDraft.pathStates.end() &&
                                           IsAppliedButUnpersisted(existingIt->second);
        if (draftValue.has_value() &&
            StoreValueEquals(m_configDraft.serviceSnapshot, path, *draftValue)) {
            auto& state = m_configDraft.pathStates[path];
            if (appliedButUnpersisted) {
                state.hasServiceSnapshot = true;
                state.hasDirtyBaseline = m_configDraft.dirtyBaseline.contains(path);
                continue;
            }
            state.dirty = false;
            state.hasServiceSnapshot = true;
            state.hasDirtyBaseline = false;
            state.applyState = ConfigDraftApplyState::Clean;
            state.persistState = ConfigDraftPersistState::NotAttempted;
            state.persistStatus = Ipc::IpcStatusCode::InternalError;
            state.mutationStatus = static_cast<uint8_t>(Ipc::ConfigV3MutationStatus::Ok);
            state.failedKeyId = Config::ConfigKeyId::MaxKeyId;
            state.errorMessage.clear();
            m_configDraft.dirtyBaseline.erase(path);
            continue;
        }

        auto& state = m_configDraft.pathStates[path];
        if (!state.dirty) {
            if (const auto baseline = TryGetStoreValue(m_configDraft.serviceSnapshot, path)) {
                m_configDraft.dirtyBaseline[path] = *baseline;
            } else if (const auto fallback = TryGetStoreValue(m_configDraft.editableDraft, path)) {
                m_configDraft.dirtyBaseline[path] = *fallback;
            }
            state.baselineSchemaVersion = m_configDraft.snapshotSchemaVersion;
            state.baselineSnapshotVersion = m_configDraft.snapshotVersion;
        }
        state.dirty = true;
        state.hasServiceSnapshot = m_configDraft.serviceSnapshot.has(path);
        state.hasDirtyBaseline = m_configDraft.dirtyBaseline.contains(path);
        state.applyState = ConfigDraftApplyState::Pending;
        state.persistState = ConfigDraftPersistState::NotAttempted;
        state.persistStatus = Ipc::IpcStatusCode::InternalError;
        state.mutationStatus = static_cast<uint8_t>(Ipc::ConfigV3MutationStatus::Ok);
        state.failedKeyId = Config::ConfigKeyId::MaxKeyId;
        state.errorMessage.clear();
    }
}

void ServiceProxy::SetConfigDraftValue(std::string_view path, Config::ConfigValue value) {
    m_configDraft.editableDraft.set<Config::ConfigValue>(path, std::move(value));
    CommitConfigDraftEdits({std::string(path)});
}

void ServiceProxy::CommitConfigDraftEdits(const std::vector<std::string>& paths) {
    if (paths.empty()) {
        return;
    }

    MarkConfigPathsDirty(paths);
    SyncServiceDraftMirrorsFromStore(
        m_configDraft.editableDraft,
        m_srvDesiredModeFull,
        m_srvAutoMode,
        m_srvStylusVhfEnabled,
        m_srvPenButtonMode,
        m_srvPenButtonRoute);
}

ConfigDraftPathState ServiceProxy::GetConfigDraftPathState(std::string_view path) const {
    ConfigDraftPathState result{};
    result.hasServiceSnapshot = m_configDraft.serviceSnapshot.has(path);
    result.hasDirtyBaseline = m_configDraft.dirtyBaseline.contains(std::string(path));

    const auto it = m_configDraft.pathStates.find(std::string(path));
    if (it != m_configDraft.pathStates.end()) {
        result = it->second;
        result.hasServiceSnapshot = m_configDraft.serviceSnapshot.has(path);
        result.hasDirtyBaseline = m_configDraft.dirtyBaseline.contains(std::string(path));
    }
    return result;
}

ApplyConfigResult ServiceProxy::GetLastApplyConfigResult() const {
    ApplyConfigResult result{};
    result.status = m_lastApplyConfigStatus.load(std::memory_order_relaxed);
    result.liveApplied = m_lastApplyConfigLiveApplied.load(std::memory_order_relaxed);
    result.restartRequired = m_lastApplyConfigRestartRequired.load(std::memory_order_relaxed);
    result.persistAttempted = m_lastApplyConfigPersistAttempted.load(std::memory_order_relaxed);
    result.persisted = m_lastApplyConfigPersisted.load(std::memory_order_relaxed);
    result.unpersistedLiveChanges = m_hasUnpersistedLiveConfigChanges.load(std::memory_order_relaxed);
    result.persistStatus = static_cast<Ipc::IpcStatusCode>(m_lastApplyConfigPersistStatus.load(std::memory_order_relaxed));
    return result;
}

bool ServiceProxy::ApplyConfigStoreGlobally() {
    auto setApplyOutcome = [this](ApplyConfigStatus status, bool liveApplied, bool restartRequired) {
        m_lastApplyConfigStatus.store(status, std::memory_order_relaxed);
        m_lastApplyConfigLiveApplied.store(liveApplied, std::memory_order_relaxed);
        m_lastApplyConfigRestartRequired.store(restartRequired, std::memory_order_relaxed);
        m_lastApplyConfigPersistAttempted.store(false, std::memory_order_relaxed);
        m_lastApplyConfigPersisted.store(false, std::memory_order_relaxed);
        m_lastApplyConfigPersistStatus.store(static_cast<uint8_t>(Ipc::IpcStatusCode::InternalError), std::memory_order_relaxed);
    };
    struct PreparedPatch {
        Config::ConfigPatchTlv patch;
        std::vector<std::string> patchPaths;
        std::vector<std::string> persistOnlyPaths;
        std::vector<std::string> noChangePaths;
        size_t skippedKeys = 0;
        bool hasRestartRequiredEntry = false;
    };

    auto hasUnpersistedDirtyState = [this]() {
        return std::ranges::any_of(m_configDraft.pathStates, [](const auto& item) {
            return IsAppliedButUnpersisted(item.second);
        });
    };

    auto hasDirtyDraftState = [this]() {
        return std::ranges::any_of(m_configDraft.pathStates, [](const auto& item) {
            return item.second.dirty;
        });
    };

    auto clearNoChangePaths = [this](const std::vector<std::string>& paths) {
        for (const auto& path : paths) {
            auto it = m_configDraft.pathStates.find(path);
            if (it != m_configDraft.pathStates.end() && !IsAppliedButUnpersisted(it->second)) {
                it->second.dirty = false;
                it->second.applyState = ConfigDraftApplyState::Clean;
                it->second.persistState = ConfigDraftPersistState::NotAttempted;
                it->second.errorMessage.clear();
                m_configDraft.dirtyBaseline.erase(path);
            }
        }
    };

    auto preparePatch = [this]() {
        PreparedPatch prepared;
        std::vector<std::string> dirtyPaths;
        dirtyPaths.reserve(m_configDraft.pathStates.size());
        for (const auto& [path, state] : m_configDraft.pathStates) {
            if (state.dirty) {
                dirtyPaths.push_back(path);
            }
        }
        std::sort(dirtyPaths.begin(), dirtyPaths.end());

        for (const auto& path : dirtyPaths) {
            const auto* schemaEntry = FindSchemaEntryByPath(m_configSchema, path);
            if (schemaEntry == nullptr ||
                schemaEntry->keyId == Config::ConfigKeyId::MaxKeyId ||
                !m_configDraft.editableDraft.has(path) ||
                !IsV3PatchableEntry(*schemaEntry)) {
                ++prepared.skippedKeys;
                continue;
            }

            const Config::ConfigValue value = m_configDraft.editableDraft.get<Config::ConfigValue>(path);
            const auto stateIt = m_configDraft.pathStates.find(path);
            const bool sameAsServiceSnapshot = StoreValueEquals(m_configDraft.serviceSnapshot, path, value);
            if (sameAsServiceSnapshot) {
                if (stateIt != m_configDraft.pathStates.end() && IsAppliedButUnpersisted(stateIt->second)) {
                    prepared.persistOnlyPaths.push_back(path);
                    prepared.hasRestartRequiredEntry = prepared.hasRestartRequiredEntry ||
                        schemaEntry->applyTiming == Config::ConfigApplyTiming::RestartRequired;
                } else {
                    prepared.noChangePaths.push_back(path);
                }
                continue;
            }

            prepared.patch.entries.push_back(BuildTlvEntry(schemaEntry->keyId, value));
            prepared.patchPaths.push_back(path);
            prepared.hasRestartRequiredEntry = prepared.hasRestartRequiredEntry ||
                schemaEntry->applyTiming == Config::ConfigApplyTiming::RestartRequired;
        }
        return prepared;
    };

    auto markPathsApplyFailed = [this](const std::vector<std::string>& paths,
                                       Ipc::ConfigV3MutationStatus status,
                                       Config::ConfigKeyId failedKeyId,
                                       std::string_view message) {
        for (const auto& path : paths) {
            auto& state = m_configDraft.pathStates[path];
            state.dirty = true;
            state.applyState = ConfigDraftApplyState::Failed;
            state.persistState = ConfigDraftPersistState::NotAttempted;
            state.mutationStatus = static_cast<uint8_t>(status);
            state.failedKeyId = failedKeyId;
            state.errorMessage = std::string(message);
        }
    };

    auto markRejected = [this, &markPathsApplyFailed](const Ipc::ConfigV3ApplyResultWire& applyResult) {
        const auto failedKeyId = static_cast<Config::ConfigKeyId>(applyResult.failedKeyId);
        std::vector<std::string> failedPaths;
        if (failedKeyId != Config::ConfigKeyId::MaxKeyId) {
            for (const auto& entry : m_configSchema.entries) {
                if (entry.keyId == failedKeyId && !entry.yamlPath.empty()) {
                    failedPaths.push_back(entry.yamlPath);
                    break;
                }
            }
        }
        if (failedPaths.empty()) {
            for (const auto& [path, state] : m_configDraft.pathStates) {
                if (state.dirty) {
                    failedPaths.push_back(path);
                }
            }
        }
        markPathsApplyFailed(
            failedPaths,
            static_cast<Ipc::ConfigV3MutationStatus>(applyResult.status),
            failedKeyId,
            "Service rejected this config value.");
    };

    auto markPatchPathsApplied = [this](const std::vector<std::string>& paths,
                                        const Ipc::ConfigV3ApplyResultWire& applyResult) {
        for (const auto& path : paths) {
            const auto* schemaEntry = FindSchemaEntryByPath(m_configSchema, path);
            auto& state = m_configDraft.pathStates[path];
            state.dirty = true;
            state.applyState = (schemaEntry != nullptr &&
                                schemaEntry->applyTiming == Config::ConfigApplyTiming::RestartRequired)
                ? ConfigDraftApplyState::StagedRestartRequired
                : ConfigDraftApplyState::LiveApplied;
            state.persistState = ConfigDraftPersistState::NotAttempted;
            state.mutationStatus = applyResult.status;
            state.failedKeyId = Config::ConfigKeyId::MaxKeyId;
            state.errorMessage.clear();
        }
    };

    auto markPersistOutcome = [this](const std::vector<std::string>& paths,
                                     bool persisted,
                                     Ipc::IpcStatusCode persistStatus) {
        for (const auto& path : paths) {
            auto& state = m_configDraft.pathStates[path];
            state.persistStatus = persistStatus;
            if (persisted) {
                state.dirty = false;
                state.persistState = ConfigDraftPersistState::Persisted;
                state.errorMessage.clear();
                m_configDraft.dirtyBaseline.erase(path);
            } else {
                state.dirty = true;
                state.persistState = ConfigDraftPersistState::Unpersisted;
                state.errorMessage = "Applied in Service but not persisted.";
            }
        }
    };

    const bool liveControlAllowed = IsLiveControlAllowed();
    const bool connectedForApply = IsConfigIpcConnectedForApply();
    const bool connectedLiveApply = liveControlAllowed && connectedForApply;
    const bool hasDirtyDraft = hasDirtyDraftState();
    if (connectedLiveApply && hasDirtyDraft && !m_configV3CatalogReady) {
        setApplyOutcome(ApplyConfigStatus::LiveApplyFailed, false, false);
        LOG_WARN("App", __func__, "Config", "Cannot apply config v3 patch/persist without a ready catalog; dirty draft retained for retry.");
        return false;
    }

    if (connectedLiveApply && hasDirtyDraft &&
        (m_configDraft.snapshotSchemaVersion == 0 || m_configDraft.snapshotVersion == 0) &&
        !RefreshConfigSnapshotV3()) {
        setApplyOutcome(ApplyConfigStatus::LiveApplyFailed, false, false);
        LOG_WARN("App", __func__, "Config", "Cannot apply config v3 patch without a snapshot baseline.");
        return false;
    }

    PreparedPatch prepared = preparePatch();
    clearNoChangePaths(prepared.noChangePaths);

    if (prepared.patch.entries.empty() && prepared.persistOnlyPaths.empty()) {
        setApplyOutcome(ApplyConfigStatus::NoChanges, false, false);
        m_hasUnpersistedLiveConfigChanges.store(hasUnpersistedDirtyState(), std::memory_order_relaxed);
        LOG_INFO("App", __func__, "Config", "Global config apply produced no TLV entries; skippedKeys={}", prepared.skippedKeys);
        return true;
    }

    if (!liveControlAllowed || !connectedForApply) {
        ApplyConfigStoreToLocalRuntime();
        Ipc::ConfigV3ApplyResultWire localApplyResult{};
        localApplyResult.status = static_cast<uint8_t>(Ipc::ConfigV3MutationStatus::Ok);
        markPatchPathsApplied(prepared.patchPaths, localApplyResult);
        markPersistOutcome(prepared.patchPaths, false, Ipc::IpcStatusCode::InvalidState);
        setApplyOutcome(prepared.hasRestartRequiredEntry ? ApplyConfigStatus::RestartRequired : ApplyConfigStatus::LiveApplied,
                        !prepared.hasRestartRequiredEntry,
                        prepared.hasRestartRequiredEntry);
        m_hasUnpersistedLiveConfigChanges.store(true, std::memory_order_relaxed);
        LOG_WARN("App", __func__, "IPC", "Global config apply used app-local fallback; live control unavailable or IPC disconnected.");
        return true;
    }

    auto serializePreparedPatch = [&](const PreparedPatch& patchToSerialize) -> std::optional<std::vector<uint8_t>> {
        try {
            auto payload = Config::serializePatch(patchToSerialize.patch);
            if (payload.empty() || payload.size() > Ipc::kConfigPatchV3PayloadBytes) {
                LOG_WARN("App", __func__, "IPC", "Global config v3 patch too large: {} bytes", payload.size());
                return std::nullopt;
            }
            return payload;
        } catch (const std::exception& ex) {
            LOG_WARN("App", __func__, "Config", "Failed to serialize global config patch: {}", ex.what());
            return std::nullopt;
        }
    };

    auto sendApply = [&](std::span<const uint8_t> payload) {
        Ipc::ApplyConfigPatchV3RequestWire request{};
        request.baseSchemaVersion = m_configDraft.snapshotSchemaVersion;
        request.baseSnapshotVersion = m_configDraft.snapshotVersion;
        request.payloadBytes = static_cast<uint16_t>(payload.size());
        std::memcpy(request.bytes, payload.data(), payload.size());
        return SendApplyConfigPatchV3Request(request);
    };

    Ipc::ConfigV3ApplyResultWire applyResult{};
    if (!prepared.patch.entries.empty()) {
        auto payload = serializePreparedPatch(prepared);
        if (!payload.has_value()) {
            setApplyOutcome(ApplyConfigStatus::LiveApplyFailed, false, false);
            markPathsApplyFailed(prepared.patchPaths, Ipc::ConfigV3MutationStatus::Rejected,
                                 Config::ConfigKeyId::MaxKeyId, "Config patch could not be serialized.");
            return false;
        }

        auto applyResp = sendApply(std::span<const uint8_t>(payload->data(), payload->size()));
        auto decodedApply = DecodeConfigV3ApplyResult(applyResp);
        if (!decodedApply.has_value()) {
            setApplyOutcome(ApplyConfigStatus::LiveApplyFailed, false, false);
            markPathsApplyFailed(prepared.patchPaths, Ipc::ConfigV3MutationStatus::Rejected,
                                 Config::ConfigKeyId::MaxKeyId, "ApplyConfigPatchV3 did not return a valid result.");
            LOG_WARN("App", __func__, "IPC", "ApplyConfigPatchV3 failed with status={}; dirty paths retained for retry",
                     static_cast<unsigned int>(applyResp.status));
            return false;
        }
        applyResult = *decodedApply;

        if (static_cast<Ipc::ConfigV3MutationStatus>(applyResult.status) == Ipc::ConfigV3MutationStatus::VersionMismatch) {
            if (!RefreshConfigSnapshotV3()) {
                setApplyOutcome(ApplyConfigStatus::LiveApplyFailed, false, false);
                markPathsApplyFailed(prepared.patchPaths, Ipc::ConfigV3MutationStatus::VersionMismatch,
                                     Config::ConfigKeyId::MaxKeyId, "Snapshot refresh failed after version mismatch.");
                LOG_WARN("App", __func__, "Config", "ApplyConfigPatchV3 version mismatch and snapshot refresh failed.");
                return false;
            }

            prepared = preparePatch();
            clearNoChangePaths(prepared.noChangePaths);
            if (!prepared.patch.entries.empty()) {
                payload = serializePreparedPatch(prepared);
                if (!payload.has_value()) {
                    setApplyOutcome(ApplyConfigStatus::LiveApplyFailed, false, false);
                    markPathsApplyFailed(prepared.patchPaths, Ipc::ConfigV3MutationStatus::Rejected,
                                         Config::ConfigKeyId::MaxKeyId, "Rebased config patch could not be serialized.");
                    return false;
                }
                applyResp = sendApply(std::span<const uint8_t>(payload->data(), payload->size()));
                decodedApply = DecodeConfigV3ApplyResult(applyResp);
                if (!decodedApply.has_value()) {
                    setApplyOutcome(ApplyConfigStatus::LiveApplyFailed, false, false);
                    markPathsApplyFailed(prepared.patchPaths, Ipc::ConfigV3MutationStatus::Rejected,
                                         Config::ConfigKeyId::MaxKeyId, "ApplyConfigPatchV3 retry did not return a valid result.");
                    LOG_WARN("App", __func__, "IPC", "ApplyConfigPatchV3 retry failed with status={}", static_cast<unsigned int>(applyResp.status));
                    return false;
                }
                applyResult = *decodedApply;
            } else if (prepared.persistOnlyPaths.empty()) {
                setApplyOutcome(ApplyConfigStatus::NoChanges, false, false);
                m_hasUnpersistedLiveConfigChanges.store(hasUnpersistedDirtyState(), std::memory_order_relaxed);
                LOG_INFO("App", __func__, "Config", "VersionMismatch rebase produced no remaining dirty patch entries.");
                return true;
            } else {
                applyResult = Ipc::ConfigV3ApplyResultWire{};
                LOG_INFO("App", __func__, "Config", "VersionMismatch rebase produced persist-only paths; skipping retry apply and retrying persist.");
            }
        }

        const auto mutationStatus = static_cast<Ipc::ConfigV3MutationStatus>(applyResult.status);
        if (mutationStatus != Ipc::ConfigV3MutationStatus::Ok &&
            mutationStatus != Ipc::ConfigV3MutationStatus::NoChanges) {
            setApplyOutcome(ApplyConfigStatus::LiveApplyFailed, false, false);
            markRejected(applyResult);
            LOG_WARN("App", __func__, "IPC", "ApplyConfigPatchV3 rejected status={} failedKeyId=0x{:04X}",
                     static_cast<unsigned int>(applyResult.status), applyResult.failedKeyId);
            return false;
        }

        markPatchPathsApplied(prepared.patchPaths, applyResult);
    }

    auto hasPathApplyState = [this](const std::vector<std::string>& paths, ConfigDraftApplyState applyState) {
        return std::ranges::any_of(paths, [this, applyState](const auto& path) {
            const auto it = m_configDraft.pathStates.find(path);
            return it != m_configDraft.pathStates.end() &&
                   it->second.applyState == applyState;
        });
    };

    const bool liveApplied = applyResult.appliedCount != 0 ||
        hasPathApplyState(prepared.patchPaths, ConfigDraftApplyState::LiveApplied) ||
        hasPathApplyState(prepared.persistOnlyPaths, ConfigDraftApplyState::LiveApplied);
    const bool restartRequired = applyResult.restartRequiredCount != 0 ||
        hasPathApplyState(prepared.patchPaths, ConfigDraftApplyState::StagedRestartRequired) ||
        hasPathApplyState(prepared.persistOnlyPaths, ConfigDraftApplyState::StagedRestartRequired);
    const auto mutationStatus = static_cast<Ipc::ConfigV3MutationStatus>(applyResult.status);
    const auto applyStatus = prepared.patch.entries.empty() && !prepared.persistOnlyPaths.empty()
        ? (restartRequired ? ApplyConfigStatus::RestartRequired : ApplyConfigStatus::LiveApplied)
        : (mutationStatus == Ipc::ConfigV3MutationStatus::NoChanges
            ? ApplyConfigStatus::NoChanges
            : (restartRequired ? ApplyConfigStatus::RestartRequired : ApplyConfigStatus::LiveApplied));
    setApplyOutcome(applyStatus, liveApplied, restartRequired);

    std::vector<std::string> persistTrackedPaths = prepared.patchPaths;
    persistTrackedPaths.insert(persistTrackedPaths.end(), prepared.persistOnlyPaths.begin(), prepared.persistOnlyPaths.end());

    auto isUserOverridePersistable = [this](std::string_view path) {
        const auto* schemaEntry = FindSchemaEntryByPath(m_configSchema, path);
        return schemaEntry != nullptr &&
               schemaEntry->persistPolicy == Config::ConfigPersistPolicy::UserOverride;
    };

    std::vector<std::string> userOverridePersistPaths;
    std::vector<std::string> nonUserOverridePersistPaths;
    userOverridePersistPaths.reserve(persistTrackedPaths.size());
    for (const auto& path : persistTrackedPaths) {
        if (isUserOverridePersistable(path)) {
            userOverridePersistPaths.push_back(path);
        } else {
            nonUserOverridePersistPaths.push_back(path);
        }
    }

    const auto persistResp = SendPersistConfigV3Request();
    m_lastApplyConfigPersistAttempted.store(true, std::memory_order_relaxed);
    m_lastApplyConfigPersistStatus.store(static_cast<uint8_t>(persistResp.status), std::memory_order_relaxed);
    const auto decodedPersist = DecodeConfigV3PersistResult(persistResp);
    Ipc::PersistConfigV3ResponseWire persistResult{};
    if (decodedPersist.has_value()) {
        persistResult = *decodedPersist;
    }
    const bool persistResponseOk = decodedPersist.has_value() &&
        static_cast<Ipc::ConfigV3MutationStatus>(persistResult.status) == Ipc::ConfigV3MutationStatus::Ok;
    const bool persisted = persistResponseOk && nonUserOverridePersistPaths.empty();
    m_lastApplyConfigPersisted.store(persisted, std::memory_order_relaxed);
    markPersistOutcome(userOverridePersistPaths, persistResponseOk, persistResp.status);
    markPersistOutcome(nonUserOverridePersistPaths, false, persistResp.status);
    if (!persisted) {
        m_hasUnpersistedLiveConfigChanges.store(true, std::memory_order_relaxed);
        if (persistResponseOk) {
            LOG_WARN("App", __func__, "IPC", "PersistConfigV3 skipped {} non-user-override path(s); keeping dirty paths for retry",
                     nonUserOverridePersistPaths.size());
        } else {
            LOG_WARN("App", __func__, "IPC", "PersistConfigV3 failed after apply with status={}; keeping dirty paths for retry",
                     static_cast<unsigned int>(persistResp.status));
        }
    } else {
        m_hasUnpersistedLiveConfigChanges.store(hasUnpersistedDirtyState(), std::memory_order_relaxed);
    }

    RefreshConfigSnapshot();
    ApplyConfigStoreToLocalRuntime();
    LOG_INFO("App", __func__, "IPC", "Global config v3 applied entries={} skippedKeys={} liveApplied={} restartRequired={} persisted={} persistedCount={} skippedPersist={} unpersistedLiveChanges={}",
             prepared.patch.entries.size(), prepared.skippedKeys, liveApplied ? 1 : 0, restartRequired ? 1 : 0,
             persisted ? 1 : 0, persistResult.persistedCount, persistResult.skippedCount,
             m_hasUnpersistedLiveConfigChanges.load(std::memory_order_relaxed) ? 1 : 0);
    return true;
}

void ServiceProxy::SaveConfig() {
    (void)ApplyConfigStoreGlobally();
}

bool ServiceProxy::IsConfigIpcConnectedForApply() const {
#if defined(EGOTOUCH_APP_SERVICE_PROXY_TEST)
    if (m_configV3IpcTestConnected) {
        return true;
    }
#endif
    return m_client.IsConnected();
}

Ipc::IpcResponse ServiceProxy::SendApplyConfigPatchV3Request(const Ipc::ApplyConfigPatchV3RequestWire& request) {
#if defined(EGOTOUCH_APP_SERVICE_PROXY_TEST)
    if (m_configV3IpcTestConnected) {
        m_lastConfigV3ApplyRequestForTest = request;
        m_hasLastConfigV3ApplyRequestForTest = true;
        ++m_configV3ApplyRequestCountForTest;
        if (m_configV3IpcTestApplyResponseIndex < m_configV3IpcTestApplyResponses.size()) {
            return m_configV3IpcTestApplyResponses[m_configV3IpcTestApplyResponseIndex++];
        }
        return m_configV3IpcTestApplyResponse;
    }
#endif
    return m_client.ApplyConfigPatchV3(request);
}

Ipc::IpcResponse ServiceProxy::SendPersistConfigV3Request() {
#if defined(EGOTOUCH_APP_SERVICE_PROXY_TEST)
    if (m_configV3IpcTestConnected) {
        ++m_configV3PersistRequestCountForTest;
        return m_configV3IpcTestPersistResponse;
    }
#endif
    return m_client.PersistConfigV3();
}

#if defined(EGOTOUCH_APP_SERVICE_PROXY_TEST)
void ServiceProxy::SetConfigV3IpcTestResponses(bool connected,
                                               const Ipc::IpcResponse& applyResponse,
                                               const Ipc::IpcResponse& persistResponse) {
    SetConfigV3IpcTestResponses(connected, std::vector<Ipc::IpcResponse>{applyResponse}, persistResponse);
}

void ServiceProxy::SetConfigV3IpcTestResponses(bool connected,
                                               std::vector<Ipc::IpcResponse> applyResponses,
                                               const Ipc::IpcResponse& persistResponse,
                                               std::vector<uint8_t> snapshotBytes) {
    m_configV3IpcTestConnected = connected;
    m_configV3IpcTestApplyResponses = std::move(applyResponses);
    m_configV3IpcTestApplyResponseIndex = 0;
    m_configV3IpcTestApplyResponse = m_configV3IpcTestApplyResponses.empty()
        ? Ipc::IpcResponse{}
        : m_configV3IpcTestApplyResponses.back();
    m_configV3IpcTestPersistResponse = persistResponse;
    m_configV3IpcTestSnapshotBytes = std::move(snapshotBytes);
    m_hasLastConfigV3ApplyRequestForTest = false;
    m_lastConfigV3ApplyRequestForTest = Ipc::ApplyConfigPatchV3RequestWire{};
    m_configV3ApplyRequestCountForTest = 0;
    m_configV3PersistRequestCountForTest = 0;
}
#endif

uint32_t HashBytes(std::span<const uint8_t> bytes) noexcept {
    uint32_t hash = 2166136261u;
    for (const uint8_t byte : bytes) {
        hash ^= byte;
        hash *= 16777619u;
    }
    return hash;
}

std::optional<std::vector<uint8_t>> ServiceProxy::FetchConfigV3Bytes(Ipc::ConfigV3PayloadKind payloadKind) {
#if defined(EGOTOUCH_APP_SERVICE_PROXY_TEST)
    if (m_configV3IpcTestConnected &&
        payloadKind == Ipc::ConfigV3PayloadKind::Snapshot &&
        !m_configV3IpcTestSnapshotBytes.empty()) {
        return m_configV3IpcTestSnapshotBytes;
    }
#endif
    constexpr uint32_t kMaxBytes = 1024u * 1024u;
    const uint8_t rawKind = static_cast<uint8_t>(payloadKind);
    std::vector<uint8_t> bytes;
    uint32_t schemaVersion = 0;
    uint32_t snapshotVersion = 0;
    uint32_t totalBytes = 0;
    uint32_t checksum = 0;
    uint32_t offset = 0;

    do {
        Ipc::ConfigV3PageRequestWire request{};
        request.payloadKind = rawKind;
        request.schemaVersion = schemaVersion;
        request.snapshotVersion = snapshotVersion;
        request.offset = offset;
        request.maxBytes = Ipc::ConfigV3PageCapacityBytes();

        const auto resp = payloadKind == Ipc::ConfigV3PayloadKind::Catalog
            ? m_client.GetConfigCatalogV3Page(request)
            : m_client.GetConfigSnapshotV3Page(request);
        if (!resp.success || resp.dataLen < sizeof(Ipc::ConfigV3PageResponseHeaderWire)) {
            LOG_WARN("App", __func__, "Config", "Config v3 page request failed kind={} offset={} status={}", rawKind, offset, static_cast<unsigned int>(resp.status));
            return std::nullopt;
        }

        Ipc::ConfigV3PageResponseHeaderWire header{};
        std::memcpy(&header, resp.data, sizeof(header));
        if (!Ipc::IsValidConfigV3PageResponse(header, resp.dataLen) || header.payloadKind != rawKind || header.offset != offset) {
            LOG_WARN("App", __func__, "Config", "Invalid config v3 page header kind={} offset={}", rawKind, offset);
            return std::nullopt;
        }
        if (offset == 0) {
            if (header.totalBytes > kMaxBytes) {
                LOG_WARN("App", __func__, "Config", "Config v3 payload too large kind={} bytes={}", rawKind, header.totalBytes);
                return std::nullopt;
            }
            schemaVersion = header.schemaVersion;
            snapshotVersion = header.snapshotVersion;
            totalBytes = header.totalBytes;
            checksum = header.checksum;
            bytes.reserve(totalBytes);
        } else if (header.schemaVersion != schemaVersion || header.snapshotVersion != snapshotVersion ||
                   header.totalBytes != totalBytes || header.checksum != checksum) {
            LOG_WARN("App", __func__, "Config", "Config v3 page metadata changed kind={} offset={}", rawKind, offset);
            return std::nullopt;
        }

        const auto* page = resp.data + header.headerBytes;
        bytes.insert(bytes.end(), page, page + header.pageBytes);
        offset += header.pageBytes;
        if (header.pageBytes == 0 && offset < totalBytes) {
            LOG_WARN("App", __func__, "Config", "Config v3 page made no progress kind={} offset={}", rawKind, offset);
            return std::nullopt;
        }
    } while (offset < totalBytes);

    if (bytes.size() != totalBytes || HashBytes(std::span<const uint8_t>(bytes.data(), bytes.size())) != checksum) {
        LOG_WARN("App", __func__, "Config", "Config v3 payload checksum mismatch kind={} bytes={}", rawKind, bytes.size());
        return std::nullopt;
    }
    return bytes;
}

bool ServiceProxy::ApplyConfigV3CatalogBytes(const uint8_t* data, size_t size) {
    try {
        const auto payload = Config::deserializeConfigV3Catalog(data, size);
        Config::ConfigCatalog catalog(payload.entries);
        m_configSchema = Config::BuildSchemaSnapshot(catalog);
        m_configDraft.catalogSchemaVersion = payload.schemaVersion;
        m_configDraft.catalogSnapshotVersion = payload.snapshotVersion;
        for (const auto& entry : payload.entries) {
            if (!entry.path.empty()) {
                m_configDraft.catalogDefaults.set<Config::ConfigValue>(entry.path, entry.defaultValue);
                if (!m_configDraft.editableDraft.has(entry.path)) {
                    m_configDraft.editableDraft.set<Config::ConfigValue>(entry.path, entry.defaultValue);
                }
            }
        }
        m_configV3CatalogReady = true;
        LOG_INFO("App", __func__, "Config", "Applied config v3 catalog entries={} schemaVersion={} snapshotVersion={}",
                 payload.entries.size(), payload.schemaVersion, payload.snapshotVersion);
        return true;
    } catch (const std::exception& ex) {
        m_configV3CatalogReady = false;
        LOG_WARN("App", __func__, "Config", "Failed to apply config v3 catalog: {}", ex.what());
        return false;
    }
}

bool ServiceProxy::ApplyConfigV3SnapshotBytes(const uint8_t* data, size_t size) {
    if (!m_configV3CatalogReady) {
        LOG_WARN("App", __func__, "Config", "Skipping config v3 snapshot because no ready catalog is available.");
        return false;
    }

    try {
        const auto payload = Config::deserializeConfigV3Snapshot(data, size);
        std::unordered_map<Config::ConfigKeyId, const Config::ConfigSchemaEntry*> entryByKey;
        for (const auto& entry : m_configSchema.entries) {
            if (entry.keyId != Config::ConfigKeyId::MaxKeyId && !entry.yamlPath.empty()) {
                entryByKey.emplace(entry.keyId, &entry);
            }
        }
        size_t applied = 0;
        size_t dirtySkipped = 0;
        size_t unknownSkipped = 0;
        for (const auto& entry : payload.entries) {
            const auto it = entryByKey.find(entry.keyId);
            if (it == entryByKey.end()) {
                ++unknownSkipped;
                LOG_WARN("App", __func__, "Config", "Skipping config v3 snapshot entry with unknown keyId={}", static_cast<unsigned int>(entry.keyId));
                continue;
            }
            const auto& schemaEntry = *it->second;
            const auto normalizedValue = NormalizeSnapshotValueForSchema(schemaEntry, entry.value);
            if (!normalizedValue.has_value()) {
                continue;
            }
            m_configDraft.serviceSnapshot.set<Config::ConfigValue>(schemaEntry.yamlPath, *normalizedValue);
            const auto stateIt = m_configDraft.pathStates.find(schemaEntry.yamlPath);
            if (stateIt != m_configDraft.pathStates.end()) {
                auto& state = stateIt->second;
                state.hasServiceSnapshot = true;
                state.hasDirtyBaseline = m_configDraft.dirtyBaseline.contains(schemaEntry.yamlPath);
                if (state.dirty) {
                    ++dirtySkipped;
                    continue;
                }
            }
            m_configDraft.editableDraft.set<Config::ConfigValue>(schemaEntry.yamlPath, *normalizedValue);
            ++applied;
        }
        m_configDraft.snapshotSchemaVersion = payload.schemaVersion;
        m_configDraft.snapshotVersion = payload.snapshotVersion;
        SyncServiceActiveMirrorFromSnapshot(
            m_configDraft.serviceSnapshot,
            m_srvActiveModeFull);
        SyncServiceDraftMirrorsFromStore(
            m_configDraft.editableDraft,
            m_srvDesiredModeFull,
            m_srvAutoMode,
            m_srvStylusVhfEnabled,
            m_srvPenButtonMode,
            m_srvPenButtonRoute);
        ApplyConfigStoreToLocalRuntime();
        LOG_INFO("App", __func__, "Config", "Applied config v3 snapshot entries={} applied={} dirtySkipped={} unknownSkipped={}",
                 payload.entries.size(), applied, dirtySkipped, unknownSkipped);
        return true;
    } catch (const std::exception& ex) {
        LOG_WARN("App", __func__, "Config", "Failed to apply config v3 snapshot: {}", ex.what());
        return false;
    }
}

bool ServiceProxy::ApplyConfigV3CatalogBytesForTest(const uint8_t* data, size_t size) {
    return ApplyConfigV3CatalogBytes(data, size);
}

bool ServiceProxy::ApplyConfigV3SnapshotBytesForTest(const uint8_t* data, size_t size) {
    return ApplyConfigV3SnapshotBytes(data, size);
}

ConfigV3BaselineVersions ServiceProxy::GetConfigV3BaselineVersionsForTest() const {
    return ConfigV3BaselineVersions{
        .catalogSchemaVersion = m_configDraft.catalogSchemaVersion,
        .catalogSnapshotVersion = m_configDraft.catalogSnapshotVersion,
        .snapshotSchemaVersion = m_configDraft.snapshotSchemaVersion,
        .snapshotVersion = m_configDraft.snapshotVersion,
    };
}

bool ServiceProxy::RefreshConfigCatalogV3() {
    if (!m_client.IsConnected()) {
        m_configV3CatalogReady = false;
        return false;
    }
    const auto bytes = FetchConfigV3Bytes(Ipc::ConfigV3PayloadKind::Catalog);
    if (!bytes.has_value()) {
        m_configV3CatalogReady = false;
        return false;
    }
    return ApplyConfigV3CatalogBytes(bytes->data(), bytes->size());
}

bool ServiceProxy::RefreshConfigSnapshotV3() {
    if (!IsConfigIpcConnectedForApply()) return false;
    if (!m_configV3CatalogReady) {
        LOG_WARN("App", __func__, "Config", "Skipping config v3 snapshot refresh because no ready catalog is available.");
        return false;
    }
    const auto bytes = FetchConfigV3Bytes(Ipc::ConfigV3PayloadKind::Snapshot);
    return bytes.has_value() && ApplyConfigV3SnapshotBytes(bytes->data(), bytes->size());
}

void ServiceProxy::RefreshConfigSnapshot() {
    if (!m_client.IsConnected()) return;
    if (!m_configV3CatalogReady) {
        LOG_WARN("App", __func__, "Config", "Config v3 snapshot skipped because no v3 catalog is available; preserving current app-local/offline config state; no fixed-ABI Service snapshot fallback is attempted.");
        return;
    }
    if (!RefreshConfigSnapshotV3()) {
        LOG_WARN("App", __func__, "Config", "Config v3 snapshot unavailable; preserving current app-local/offline config state; no fixed-ABI Service snapshot fallback is attempted.");
    }
}

void ServiceProxy::SetSrvModeFull(bool full) {
    if (!IsLiveControlAllowed()) return;
    m_srvDesiredModeFull.store(full, std::memory_order_relaxed);
    SetConfigDraftValue("service.mode", ToConfigValue(ToServiceModeConfig(full)));
}

void ServiceProxy::SetSrvStylusVhfEnabled(bool enabled) {
    if (!IsLiveControlAllowed()) return;
    m_srvStylusVhfEnabled.store(enabled, std::memory_order_relaxed);
    SetConfigDraftValue("service.stylus_vhf_enabled", ToConfigValue(enabled));
}

void ServiceProxy::SetSrvAutoMode(bool enabled) {
    if (!IsLiveControlAllowed()) return;
    m_srvAutoMode.store(enabled, std::memory_order_relaxed);
    SetConfigDraftValue("service.auto_mode", ToConfigValue(enabled));
}

void ServiceProxy::SetPenButtonMode(PenButtonMode m) {
    if (!IsLiveControlAllowed()) return;
    m_srvPenButtonMode.store(m, std::memory_order_relaxed);
    SetConfigDraftValue("service.pen_button_mode", ToConfigValue(m));
}

void ServiceProxy::SetPenButtonRoute(PenButtonRoute r) {
    if (!IsLiveControlAllowed()) return;
    m_srvPenButtonRoute.store(r, std::memory_order_relaxed);
    SetConfigDraftValue("service.pen_button_route", ToConfigValue(r));
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
