#include "ConfigRuntime.h"

#include "Logger.h"
#include "SolverTypes.h"
#include "TouchPipeline.h"
#include "StylusPipeline.h"
#include "config/ConfigBinder.h"
#include "config/ConfigCatalog.h"
#include "config/ConfigKeyMap.h"
#include "config/SchemaValidator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <exception>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace Service {

uint32_t HashBytes(std::span<const uint8_t> bytes) noexcept;

namespace {

const char* ToConfigValue(PenButtonMode mode) {
    switch (mode) {
    case PenButtonMode::OemCustom: return "oem_custom";
    case PenButtonMode::NativeBarrel: return "native_barrel";
    case PenButtonMode::NativeEraser: return "native_eraser";
    }
    return "oem_custom";
}

const char* ToConfigValue(PenButtonRoute route) {
    switch (route) {
    case PenButtonRoute::VhfOnly: return "vhf_only";
    case PenButtonRoute::Win32Only: return "win32_only";
    case PenButtonRoute::VhfAndWin32: return "vhf_and_win32";
    }
    return "vhf_only";
}

Config::ConfigValue ConfigValueFromTlvEntry(const Config::ConfigTlvEntry& entry, bool& ok) {
    ok = true;
    try {
        switch (entry.valueType) {
        case Config::ConfigValueType::Bool:
            if (entry.stringValue == "true" || entry.stringValue == "1") return Config::ConfigValue(true);
            if (entry.stringValue == "false" || entry.stringValue == "0") return Config::ConfigValue(false);
            ok = false; return Config::ConfigValue(false);
        case Config::ConfigValueType::Int32: {
            size_t pos = 0;
            const int parsed = std::stoi(entry.stringValue, &pos);
            if (pos != entry.stringValue.size()) { ok = false; return Config::ConfigValue(int32_t{0}); }
            return Config::ConfigValue(static_cast<int32_t>(parsed));
        }
        case Config::ConfigValueType::Float: {
            size_t pos = 0;
            const float parsed = std::stof(entry.stringValue, &pos);
            if (pos != entry.stringValue.size() || !std::isfinite(parsed)) { ok = false; return Config::ConfigValue(0.0f); }
            return Config::ConfigValue(parsed);
        }
        case Config::ConfigValueType::String:
            return Config::ConfigValue(entry.stringValue);
        case Config::ConfigValueType::Null:
        default:
            ok = false; return Config::ConfigValue(std::string{});
        }
    } catch (const std::exception&) {
        ok = false; return Config::ConfigValue(std::string{});
    }
}

constexpr std::array<std::pair<std::string_view, std::string_view>, 4> kStylusIirCoefficientPathPairs{
    std::pair{"stylus.sp.iir_coef_low_hover", "stylus.sp.iir_coef_low_in_band"},
    std::pair{"stylus.sp.iir_coef_high_hover", "stylus.sp.iir_coef_high_in_band"},
    std::pair{"stylus.sp.iir_coef_low_writing", "stylus.sp.iir_coef_low_edge"},
    std::pair{"stylus.sp.iir_coef_high_writing", "stylus.sp.iir_coef_high_edge"},
};

bool StylusIirCoefficientsWithinMax(const Config::ConfigStore& store) {
    const int32_t maxCoef = store.getOr<int32_t>("stylus.sp.iir_max_coef", 32);
    if (maxCoef < 1) return false;
    for (const auto [canonicalPath, legacyPath] : kStylusIirCoefficientPathPairs) {
        const auto path = store.has(canonicalPath) ? canonicalPath : legacyPath;
        const int32_t coef = store.getOr<int32_t>(path, 0);
        if (coef < 0 || coef > maxCoef) return false;
    }
    return true;
}

void ClampStylusIirCoefficients(Config::ConfigStore& store) {
    const int32_t maxCoef = std::clamp(store.getOr<int32_t>("stylus.sp.iir_max_coef", 32), int32_t{1}, int32_t{255});
    store.set<int32_t>("stylus.sp.iir_max_coef", maxCoef);
    for (const auto [canonicalPath, legacyPath] : kStylusIirCoefficientPathPairs) {
        const auto path = store.has(canonicalPath) ? canonicalPath : legacyPath;
        if (store.has(path)) store.set<int32_t>(path, std::clamp(store.get<int32_t>(path), int32_t{0}, maxCoef));
    }
}

template<typename Callback>
decltype(auto) WithRuntimeConfigDefaults(Callback&& callback) {
    Config::ConfigBinder binder;
    ServiceConfigState serviceDefaults;
    Solvers::TouchPipeline touchDefaults;
    Solvers::StylusPipeline stylusDefaults;
    RegisterServiceConfigBindings(binder, serviceDefaults);
    touchDefaults.registerBindings(binder);
    stylusDefaults.registerBindings(binder);
    Config::registerRuntimeKeyMappings(binder);

    Config::ConfigStore defaults;
    defaults.set<std::string>("service.mode", "full");
    defaults.set<bool>("service.auto_mode", true);
    defaults.set<bool>("service.stylus_vhf_enabled", true);
    defaults.set<std::string>("service.pen_button_mode", "oem_custom");
    defaults.set<std::string>("service.pen_button_route", "vhf_only");
    binder.writeDefaults(defaults);

    if constexpr (std::is_void_v<std::invoke_result_t<Callback, Config::ConfigBinder&, Config::ConfigStore&>>) {
        std::forward<Callback>(callback)(binder, defaults);
    } else {
        return std::forward<Callback>(callback)(binder, defaults);
    }
}

bool ConfigValueAllowedBySchema(std::string_view path,
                                const Config::ConfigValue& value,
                                const Config::ConfigSchemaSnapshot& schema,
                                bool requireLiveApply) {
    const auto it = std::find_if(schema.entries.begin(), schema.entries.end(),
        [path](const Config::ConfigSchemaEntry& entry) { return entry.yamlPath == path; });
    if (it == schema.entries.end()) return false;
    if (requireLiveApply) {
        if (!it->boundToRuntime ||
            (it->runtimeBinding != Config::ConfigRuntimeBinding::LiveSetter &&
             it->runtimeBinding != Config::ConfigRuntimeBinding::ManualLiveApply) ||
            !Config::isLiveApplyTiming(it->applyTiming)) return false;
    }
    if (path == "service.mode") {
        const auto str = Config::tryGetValue<std::string>(value);
        return str.has_value() && (*str == "full" || *str == "touch_only");
    }
    if (path == "service.pen_button_mode") return ParsePenButtonModeValue(value).has_value();
    if (path == "service.pen_button_route") return ParsePenButtonRouteValue(value).has_value();
    switch (it->uiType) {
    case Config::ConfigUiType::Bool: return Config::tryGetValue<bool>(value).has_value();
    case Config::ConfigUiType::Int32: {
        const auto v = Config::tryGetValue<int32_t>(value);
        return v.has_value() && (!it->range.has_value() || (*v >= it->range->min && *v <= it->range->max));
    }
    case Config::ConfigUiType::Float: {
        const auto v = Config::tryGetValue<float>(value);
        return v.has_value() && (!it->range.has_value() || (*v >= it->range->min && *v <= it->range->max));
    }
    case Config::ConfigUiType::Enum:
    case Config::ConfigUiType::String: return Config::tryGetValue<std::string>(value).has_value();
    }
    return false;
}

const Config::ConfigSchemaEntry* FindSchemaEntry(const Config::ConfigSchemaSnapshot& schema,
                                                 std::string_view path) {
    const auto it = std::find_if(schema.entries.begin(), schema.entries.end(),
        [path](const Config::ConfigSchemaEntry& entry) { return entry.yamlPath == path; });
    return it == schema.entries.end() ? nullptr : &*it;
}

bool IsPatchableV3Timing(Config::ConfigApplyTiming timing) {
    return Config::isLiveApplyTiming(timing) || timing == Config::ConfigApplyTiming::RestartRequired;
}

ConfigTargetResult MakeTargetResult(std::string_view targetName, ConfigApplyPhase phase, bool ok, std::string message = {}) {
    ConfigTargetResult result{};
    result.targetName = std::string(targetName);
    result.phase = phase;
    result.ok = ok;
    result.message = std::move(message);
    return result;
}

Config::ConfigV3CatalogPayload BuildCatalogPayloadFromSchema(const Config::ConfigSchemaSnapshot& schema) {
    Config::ConfigV3CatalogPayload payload{};
    payload.entries = Config::ConfigCatalogBuilder::fromSnapshot(schema).descriptors();
    payload.entries.erase(std::remove_if(payload.entries.begin(), payload.entries.end(),
        [](const Config::ConfigDescriptor& entry) { return entry.keyId == Config::ConfigKeyId::MaxKeyId; }),
        payload.entries.end());
    return payload;
}

Config::ConfigV3SnapshotPayload BuildSnapshotPayloadFromStore(const Config::ConfigStore& store,
                                                              const Config::ConfigSchemaSnapshot& schema) {
    Config::ConfigV3SnapshotPayload payload{};
    auto descriptors = Config::ConfigCatalogBuilder::fromSnapshot(schema).descriptors();
    for (const auto& descriptor : descriptors) {
        if (descriptor.keyId == Config::ConfigKeyId::MaxKeyId) continue;
        Config::ConfigV3SnapshotEntry entry{};
        entry.keyId = descriptor.keyId;
        entry.value = store.getOr<Config::ConfigValue>(descriptor.path, descriptor.defaultValue);
        payload.entries.push_back(std::move(entry));
    }
    return payload;
}

uint32_t SchemaVersionFor(const Config::ConfigSchemaSnapshot& schema) {
    auto payload = BuildCatalogPayloadFromSchema(schema);
    return HashBytes(Config::serializeConfigV3Catalog(payload));
}

uint32_t SnapshotVersionFor(const Config::ConfigStore& store,
                            const Config::ConfigSchemaSnapshot& schema,
                            uint32_t schemaVersion) {
    auto payload = BuildSnapshotPayloadFromStore(store, schema);
    payload.schemaVersion = schemaVersion;
    return HashBytes(Config::serializeConfigV3Snapshot(payload));
}

class ServicePolicyTarget final : public IConfigTarget {
public:
    std::string_view name() const noexcept override { return "ServicePolicyTarget"; }

    bool isInterested(const ConfigChangeSet& changeSet) const override {
        return changeSet.containsPrefix("service.");
    }

    ConfigTargetResult validateConfig(const Config::ConfigStore& candidate,
                                      const ConfigChangeSet& changeSet) const override {
        if (changeSet.containsPath("service.mode")) {
            return MakeTargetResult(name(), ConfigApplyPhase::Live, false, "service.mode requires service restart");
        }
        if (candidate.has("service.mode")) {
            const auto mode = candidate.get<Config::ConfigValue>("service.mode");
            const auto str = Config::tryGetValue<std::string>(mode);
            if (!str.has_value() || (*str != "full" && *str != "touch_only")) {
                return MakeTargetResult(name(), ConfigApplyPhase::Live, false, "invalid service.mode");
            }
        }
        if (candidate.has("service.pen_button_mode") &&
            !ParsePenButtonModeValue(candidate.get<Config::ConfigValue>("service.pen_button_mode")).has_value()) {
            return MakeTargetResult(name(), ConfigApplyPhase::Live, false, "invalid service.pen_button_mode");
        }
        if (candidate.has("service.pen_button_route") &&
            !ParsePenButtonRouteValue(candidate.get<Config::ConfigValue>("service.pen_button_route")).has_value()) {
            return MakeTargetResult(name(), ConfigApplyPhase::Live, false, "invalid service.pen_button_route");
        }
        return MakeTargetResult(name(), ConfigApplyPhase::Live, true);
    }

    ConfigTargetResult applyConfig(const Config::ConfigStore&,
                                   const ConfigChangeSet& changeSet,
                                   ConfigApplyPhase phase) const override {
        auto result = MakeTargetResult(name(), phase, true);
        if (!changeSet.empty()) {
            ConfigApplyAction action{};
            action.kind = ConfigApplyActionKind::ServicePolicy;
            action.targetName = std::string(name());
            result.actions.push_back(std::move(action));
        }
        return result;
    }
};

class PipelineConfigTarget final : public IConfigTarget {
public:
    std::string_view name() const noexcept override { return "PipelineConfigTarget"; }

    bool isInterested(const ConfigChangeSet& changeSet) const override {
        return changeSet.containsPrefix("touch.") || changeSet.containsPrefix("stylus.");
    }

    ConfigTargetResult validateConfig(const Config::ConfigStore& candidate,
                                      const ConfigChangeSet& changeSet) const override {
        if (changeSet.containsPrefix("stylus.") && !StylusIirCoefficientsWithinMax(candidate)) {
            return MakeTargetResult(name(), ConfigApplyPhase::Live, false, "invalid stylus IIR coefficient/max relationship");
        }
        return MakeTargetResult(name(), ConfigApplyPhase::Live, true);
    }

    ConfigTargetResult applyConfig(const Config::ConfigStore&,
                                   const ConfigChangeSet& changeSet,
                                   ConfigApplyPhase phase) const override {
        auto result = MakeTargetResult(name(), phase, true);
        if (!changeSet.empty()) {
            ConfigApplyAction action{};
            action.kind = ConfigApplyActionKind::PipelineRuntime;
            action.targetName = std::string(name());
            result.actions.push_back(std::move(action));
        }
        return result;
    }
};

} // namespace

ConfigRuntime::ConfigRuntime() {
    RegisterDefaultConfigTargets();
}

void ConfigRuntime::RegisterConfigTarget(std::unique_ptr<IConfigTarget> target) {
    if (!target) return;
    std::lock_guard<std::mutex> lk(m_mutex);
    m_targets.push_back(std::move(target));
}

void ConfigRuntime::RegisterDefaultConfigTargets() {
    m_targets.push_back(std::make_unique<ServicePolicyTarget>());
    m_targets.push_back(std::make_unique<PipelineConfigTarget>());
}

Config::ConfigStore ConfigRuntime::BuildFactoryDefaultStore() {
    return WithRuntimeConfigDefaults([](Config::ConfigBinder&, Config::ConfigStore& defaults) {
        return defaults;
    });
}

Config::ConfigSchemaSnapshot ConfigRuntime::BuildFactoryDefaultSchema() {
    return WithRuntimeConfigDefaults([](Config::ConfigBinder& binder, Config::ConfigStore& defaults) {
        return Config::BuildMergedSchema(defaults, binder);
    });
}

bool ConfigRuntime::Initialize(const std::string& configPath, const StartupValidator& validateStartupConfig) {
    Config::ConfigStore defaults;
    Config::ConfigSchemaSnapshot schema;
    std::optional<std::string> cliConfigPath;
    if (!configPath.empty()) cliConfigPath = configPath;
    auto paths = Config::resolve(cliConfigPath);
    if (!paths.has_value()) {
        LOG_ERROR("Service", __func__, "Config", "config/default.yaml was not resolved; startup blocked.");
        return false;
    }
    if (paths.has_value()) {
        try {
            Config::ConfigStore yamlDefaults;
            yamlDefaults.loadFromYaml(paths->defaultConfig);
            defaults.mergeFrom(yamlDefaults);
            ApplyLegacyServiceModeMigration(defaults, yamlDefaults);
        } catch (const std::exception& ex) {
            LOG_WARN("Service", __func__, "Config", "Failed to load default config '{}': {}", paths->defaultConfig, ex.what());
        }
    }

    WithRuntimeConfigDefaults([&defaults, &schema](Config::ConfigBinder& binder, Config::ConfigStore& factoryDefaults) {
        factoryDefaults.mergeFrom(defaults);
        defaults = factoryDefaults;
        schema = Config::BuildMergedSchema(defaults, binder);
    });

    Config::ConfigStore current;
    current.mergeFrom(defaults);
    bool penButtonRouteExplicit = false;
    if (paths.has_value() && paths->overrideExists) {
        try {
            Config::ConfigStore overrides;
            overrides.loadFromYaml(paths->overrideConfig);
            penButtonRouteExplicit = overrides.has("service.pen_button_route");
            current.mergeFrom(overrides);
            ApplyLegacyServiceModeMigration(current, overrides);
        } catch (const std::exception& ex) {
            LOG_WARN("Service", __func__, "Config", "Failed to load override config '{}': {}", paths->overrideConfig, ex.what());
        }
    }

    if (!ValidateStartupConfig(current, validateStartupConfig)) {
        LOG_ERROR("Service", __func__, "Config", "Startup config schema validation failed; startup blocked.");
        return false;
    }

    for (const auto& path : current.allPaths()) {
        const auto value = current.get<Config::ConfigValue>(path);
        if (!ConfigValueAllowedBySchema(path, value, schema, false) && defaults.has(path)) {
            current.set<Config::ConfigValue>(path, defaults.get<Config::ConfigValue>(path));
            LOG_WARN("Service", __func__, "Config", "Invalid config value at '{}'; restored default.", path);
        }
    }
    if (!StylusIirCoefficientsWithinMax(current)) {
        ClampStylusIirCoefficients(current);
        LOG_WARN("Service", __func__, "Config", "Invalid stylus IIR coefficient/max relationship; clamped coefficients to max.");
    }

    std::lock_guard<std::mutex> lk(m_mutex);
    m_defaults = std::move(defaults);
    m_store = std::move(current);
    m_schema = std::move(schema);
    m_paths = std::move(paths);
    m_penButtonRouteExplicit = penButtonRouteExplicit;
    auto state = ReadServiceConfigStateFromStoreLocked();
    WriteServiceConfigStateToStoreLocked(state);
    return true;
}

uint32_t HashBytes(std::span<const uint8_t> bytes) noexcept {
    uint32_t hash = 2166136261u;
    for (const uint8_t byte : bytes) {
        hash ^= byte;
        hash *= 16777619u;
    }
    return hash;
}

ConfigRuntime::ConfigV3Blob ConfigRuntime::BuildCatalogV3Blob() const {
    Config::ConfigSchemaSnapshot schema;
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        schema = m_schema;
    }

    auto payload = BuildCatalogPayloadFromSchema(schema);
    payload.schemaVersion = SchemaVersionFor(schema);
    payload.snapshotVersion = 0;
    auto bytes = Config::serializeConfigV3Catalog(payload);
    const uint32_t checksum = HashBytes(bytes);
    return ConfigV3Blob{std::move(bytes), payload.schemaVersion, payload.snapshotVersion, checksum};
}

ConfigRuntime::ConfigV3Blob ConfigRuntime::BuildSnapshotV3Blob() const {
    Config::ConfigStore store;
    Config::ConfigSchemaSnapshot schema;
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        store = m_store;
        schema = m_schema;
    }

    auto payload = BuildSnapshotPayloadFromStore(store, schema);
    payload.schemaVersion = SchemaVersionFor(schema);
    payload.snapshotVersion = SnapshotVersionFor(store, schema, payload.schemaVersion);
    auto bytes = Config::serializeConfigV3Snapshot(payload);
    const uint32_t checksum = HashBytes(bytes);
    return ConfigV3Blob{std::move(bytes), payload.schemaVersion, payload.snapshotVersion, checksum};
}

bool ConfigRuntime::ValidateStartupConfig(const Config::ConfigStore& store, const StartupValidator& validateStartupConfig) const {
    // Phase 1 runs before DeviceRuntime exists: validate service-owned config keys here,
    // then delegate pipeline validation to the startup callback when one is supplied.
    ServiceConfigState schemaState{};
    Config::ConfigBinder serviceBinder;
    RegisterServiceConfigBindings(serviceBinder, schemaState);
    auto serviceValidation = Config::SchemaValidator::validate(store, serviceBinder);
    serviceValidation.logAll();
    if (!serviceValidation.ok()) return false;
    return !validateStartupConfig || validateStartupConfig(store);
}

Config::ConfigStore ConfigRuntime::SnapshotStore() const {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_store;
}

ServiceConfigState ConfigRuntime::ServiceState() const {
    std::lock_guard<std::mutex> lk(m_mutex);
    return ReadServiceConfigStateFromStoreLocked();
}

void ConfigRuntime::WriteServiceState(const ServiceConfigState& config) {
    std::lock_guard<std::mutex> lk(m_mutex);
    WriteServiceConfigStateToStoreLocked(config);
}

ServiceConfigState ConfigRuntime::ReadServiceConfigStateFromStoreLocked() const {
    ServiceConfigState state{};
    state.penButtonRouteExplicit = m_penButtonRouteExplicit;
    ApplyConfig(state, m_store);
    state.penButtonRouteExplicit = m_penButtonRouteExplicit;
    return state;
}

void ConfigRuntime::WriteServiceConfigStateToStoreLocked(const ServiceConfigState& config) {
    m_store.set<std::string>("service.mode", ServiceModeToConfig(config.mode));
    m_store.set<bool>("service.auto_mode", config.autoMode);
    m_store.set<bool>("service.stylus_vhf_enabled", config.stylusVhfEnabled);
    m_store.set<std::string>("service.pen_button_mode", ToConfigValue(config.penButtonMode));
    m_store.set<std::string>("service.pen_button_route", ToConfigValue(config.penButtonRoute));
    m_penButtonRouteExplicit = config.penButtonRouteExplicit;
}

bool ConfigRuntime::PersistServicePolicyConfig(const ServiceConfigState& config) {
    Config::ConfigStore storeToPersist;
    Config::ConfigStore defaultsToPersist;
    Config::ConfigPaths pathsToPersist;
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        if (!m_paths.has_value()) {
            LOG_ERROR("Service", __func__, "Config", "Cannot persist config before startup paths are resolved.");
            return false;
        }
        WriteServiceConfigStateToStoreLocked(config);
        storeToPersist = m_store;
        defaultsToPersist = m_defaults;
        pathsToPersist = *m_paths;
    }
    try {
        storeToPersist.saveOverrides(pathsToPersist.overrideConfig, defaultsToPersist);
        return true;
    } catch (const std::exception& ex) {
        LOG_ERROR("Service", __func__, "Config", "PersistConfig failed: {}", ex.what());
        return false;
    }
}

ConfigRuntime::TlvApplyResult ConfigRuntime::ApplyTlvChunk(const Ipc::ConfigTlvChunkRequestWire& chunk) {
    TlvApplyResult result{};
    if (chunk.wireVersion != Ipc::kIpcProtocolVersion ||
        chunk.totalLen == 0 || chunk.totalLen > Ipc::kConfigTlvMaxPayloadBytes ||
        chunk.chunkLen > Ipc::kConfigTlvChunkPayloadBytes ||
        static_cast<uint32_t>(chunk.offset) + chunk.chunkLen > chunk.totalLen) {
        result.status = Ipc::IpcStatusCode::InvalidRequest;
        return result;
    }

    std::lock_guard<std::mutex> lk(m_mutex);
    const bool first = (chunk.flags & Ipc::kConfigTlvChunkFirst) != 0;
    if (first) {
        m_pendingTlvSessionId = chunk.sessionId;
        m_pendingTlvTotalLen = chunk.totalLen;
        m_pendingTlvReceived = 0;
        m_pendingTlvPayload.assign(chunk.totalLen, uint8_t{0});
    }
    if (m_pendingTlvSessionId != chunk.sessionId ||
        m_pendingTlvTotalLen != chunk.totalLen ||
        m_pendingTlvPayload.size() != chunk.totalLen ||
        chunk.offset != m_pendingTlvReceived) {
        result.status = Ipc::IpcStatusCode::InvalidRequest;
        return result;
    }
    std::memcpy(m_pendingTlvPayload.data() + chunk.offset, chunk.bytes, chunk.chunkLen);
    m_pendingTlvReceived = static_cast<uint16_t>(m_pendingTlvReceived + chunk.chunkLen);
    if ((chunk.flags & Ipc::kConfigTlvChunkLast) == 0) return result;
    if (m_pendingTlvReceived != m_pendingTlvTotalLen) {
        result.status = Ipc::IpcStatusCode::InvalidRequest;
        return result;
    }
    const auto payload = m_pendingTlvPayload;
    m_pendingTlvPayload.clear();
    m_pendingTlvSessionId = 0;
    m_pendingTlvTotalLen = 0;
    m_pendingTlvReceived = 0;
    result.completed = true;
    ApplyCompletedTlvPayloadLocked(payload, result);
    return result;
}

bool ConfigRuntime::ApplyCompletedTlvPayloadLocked(const std::vector<uint8_t>& payload, TlvApplyResult& result) {
    V3ApplyResult v3Result{};
    if (!ApplyPatchPayloadLocked(payload.data(), payload.size(), true, v3Result)) {
        result.status = v3Result.ipcStatus == Ipc::IpcStatusCode::Ok
            ? Ipc::IpcStatusCode::InvalidRequest
            : v3Result.ipcStatus;
        result.entryCount = v3Result.entryCount;
        result.changedCount = v3Result.changedCount;
        result.targetResults = std::move(v3Result.targetResults);
        return false;
    }

    result.status = v3Result.ipcStatus;
    result.entryCount = v3Result.entryCount;
    result.changedCount = v3Result.changedCount;
    result.desiredServiceConfig = v3Result.desiredServiceConfig;
    result.pipelineConfig = std::move(v3Result.pipelineConfig);
    result.targetResults = std::move(v3Result.targetResults);
    result.applyActions = std::move(v3Result.applyActions);
    return true;
}

bool ConfigRuntime::ApplyPatchPayloadLocked(const uint8_t* data,
                                            size_t size,
                                            bool requireLiveApply,
                                            V3ApplyResult& result) {
    const auto parseResult = Config::deserializePatchDetailed(data, size);
    if (!parseResult.ok()) {
        result.ipcStatus = parseResult.status == Config::ConfigTlvParseStatus::InvalidArgument
            ? Ipc::IpcStatusCode::InternalError
            : Ipc::IpcStatusCode::InvalidRequest;
        result.status = Ipc::ConfigV3MutationStatus::Rejected;
        result.rejectedCount = 1;
        result.failedKeyId = static_cast<Config::ConfigKeyId>(parseResult.issue.rawKeyId);
        result.failedValueType = static_cast<Config::ConfigValueType>(parseResult.issue.rawValueType);
        LOG_WARN("Service", __func__, "Config", "Rejected config v3 patch: status={} offset={} entryIndex={} keyId=0x{:04X} valueType=0x{:02X}",
                 Config::toString(parseResult.status), parseResult.issue.offset, parseResult.issue.entryIndex,
                 parseResult.issue.rawKeyId, parseResult.issue.rawValueType);
        return false;
    }

    const auto& patch = parseResult.patch;
    bool penButtonRouteTouched = false;
    Config::ConfigStore candidate = m_store;
    ConfigChangeSet liveChangeSet{};
    ConfigChangeSet restartChangeSet{};
    liveChangeSet.entryCount = patch.entries.size();
    restartChangeSet.entryCount = patch.entries.size();

    auto rejectEntry = [&](const Config::ConfigTlvEntry& entry) {
        result.ipcStatus = Ipc::IpcStatusCode::Ok;
        result.status = Ipc::ConfigV3MutationStatus::Rejected;
        result.rejectedCount = 1;
        result.failedKeyId = entry.keyId;
        result.failedValueType = entry.valueType;
    };

    for (const auto& entry : patch.entries) {
        const auto path = Config::tryPathForKeyId(entry.keyId);
        if (!path.has_value()) {
            rejectEntry(entry);
            return false;
        }

        bool valueOk = false;
        const Config::ConfigValue value = ConfigValueFromTlvEntry(entry, valueOk);
        if (!valueOk) {
            rejectEntry(entry);
            return false;
        }

        const std::string pathString(*path);
        const auto* schemaEntry = FindSchemaEntry(m_schema, pathString);
        if (schemaEntry == nullptr ||
            !schemaEntry->boundToRuntime ||
            (schemaEntry->runtimeBinding != Config::ConfigRuntimeBinding::LiveSetter &&
             schemaEntry->runtimeBinding != Config::ConfigRuntimeBinding::ManualLiveApply) ||
            (requireLiveApply
                ? !Config::isLiveApplyTiming(schemaEntry->applyTiming)
                : !IsPatchableV3Timing(schemaEntry->applyTiming)) ||
            !ConfigValueAllowedBySchema(pathString, value, m_schema, false)) {
            rejectEntry(entry);
            return false;
        }

        if (pathString == "service.pen_button_route") penButtonRouteTouched = true;
        const bool hadPreviousValue = candidate.has(pathString);
        const Config::ConfigValue previousValue = hadPreviousValue
            ? candidate.get<Config::ConfigValue>(pathString)
            : Config::ConfigValue(std::string{});
        const bool changed = !hadPreviousValue || previousValue != value;
        candidate.set<Config::ConfigValue>(pathString, value);
        if (!changed) continue;

        ConfigChange change{};
        change.path = pathString;
        change.keyId = entry.keyId;
        change.previousValue = previousValue;
        change.newValue = value;
        change.hadPreviousValue = hadPreviousValue;

        if (!requireLiveApply && schemaEntry->applyTiming == Config::ConfigApplyTiming::RestartRequired) {
            restartChangeSet.changes.push_back(std::move(change));
        } else {
            liveChangeSet.changes.push_back(std::move(change));
        }
    }

    result.entryCount = patch.entries.size();
    result.appliedCount = liveChangeSet.changedCount();
    result.restartRequiredCount = restartChangeSet.changedCount();
    result.changedCount = result.appliedCount + result.restartRequiredCount;

    for (const auto& target : m_targets) {
        if (!target->isInterested(liveChangeSet)) continue;
        auto validation = target->validateConfig(candidate, liveChangeSet);
        result.targetResults.push_back(validation);
        if (!validation.ok) {
            result.ipcStatus = Ipc::IpcStatusCode::Ok;
            result.status = Ipc::ConfigV3MutationStatus::Rejected;
            result.rejectedCount = liveChangeSet.changedCount();
            LOG_WARN("Service", __func__, "Config", "Rejected config v3 patch by {}: {}",
                     validation.targetName, validation.message);
            return false;
        }
    }

    const auto currentServiceConfig = ReadServiceConfigStateFromStoreLocked();
    result.desiredServiceConfig = currentServiceConfig;
    ApplyConfig(result.desiredServiceConfig, candidate);
    result.desiredServiceConfig.penButtonRouteExplicit = m_penButtonRouteExplicit || penButtonRouteTouched;
    if (restartChangeSet.containsPath("service.mode")) {
        result.desiredServiceConfig.mode = currentServiceConfig.mode;
    }

    m_store = candidate;
    m_penButtonRouteExplicit = m_penButtonRouteExplicit || penButtonRouteTouched;
    result.pipelineConfig = m_store;

    for (const auto& target : m_targets) {
        if (!target->isInterested(liveChangeSet)) continue;
        auto applyResult = target->applyConfig(m_store, liveChangeSet, ConfigApplyPhase::Live);
        for (auto& action : applyResult.actions) {
            if (action.kind == ConfigApplyActionKind::ServicePolicy) {
                action.serviceConfig = result.desiredServiceConfig;
            } else if (action.kind == ConfigApplyActionKind::PipelineRuntime) {
                action.configStore = m_store;
            }
            result.applyActions.push_back(action);
        }
        result.targetResults.push_back(std::move(applyResult));
    }

    result.ipcStatus = Ipc::IpcStatusCode::Ok;
    result.status = result.changedCount == 0 ? Ipc::ConfigV3MutationStatus::NoChanges : Ipc::ConfigV3MutationStatus::Ok;
    return true;
}

ConfigRuntime::V3ApplyResult ConfigRuntime::ApplyConfigPatchV3(uint32_t baseSchemaVersion,
                                                               uint32_t baseSnapshotVersion,
                                                               const uint8_t* data,
                                                               size_t size) {
    V3ApplyResult result{};
    if (data == nullptr || size == 0 || size > Ipc::kConfigPatchV3PayloadBytes) {
        result.ipcStatus = Ipc::IpcStatusCode::InvalidRequest;
        result.status = Ipc::ConfigV3MutationStatus::Rejected;
        result.rejectedCount = 1;
        return result;
    }

    std::lock_guard<std::mutex> lk(m_mutex);
    const uint32_t currentSchemaVersion = SchemaVersionFor(m_schema);
    const uint32_t currentSnapshotVersion = SnapshotVersionFor(m_store, m_schema, currentSchemaVersion);
    if (baseSchemaVersion != currentSchemaVersion || baseSnapshotVersion != currentSnapshotVersion) {
        result.ipcStatus = Ipc::IpcStatusCode::Ok;
        result.status = Ipc::ConfigV3MutationStatus::VersionMismatch;
        return result;
    }

    (void)ApplyPatchPayloadLocked(data, size, false, result);
    return result;
}

ConfigRuntime::V3PersistResult ConfigRuntime::PersistConfigV3() {
    V3PersistResult result{};
    Config::ConfigStore storeToPersist;
    Config::ConfigStore defaultsToPersist;
    Config::ConfigPaths pathsToPersist;
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        if (!m_paths.has_value()) {
            result.ipcStatus = Ipc::IpcStatusCode::InvalidState;
            result.status = Ipc::ConfigV3MutationStatus::PersistFailed;
            result.failedCount = 1;
            LOG_ERROR("Service", __func__, "Config", "Cannot persist config before startup paths are resolved.");
            return result;
        }

        for (const auto& entry : m_schema.entries) {
            if (entry.keyId == Config::ConfigKeyId::MaxKeyId || entry.yamlPath.empty()) {
                ++result.skippedCount;
                continue;
            }
            if (entry.persistPolicy != Config::ConfigPersistPolicy::UserOverride) {
                ++result.skippedCount;
                continue;
            }
            if (!m_store.has(entry.yamlPath)) {
                ++result.skippedCount;
                continue;
            }

            const auto currentValue = m_store.get<Config::ConfigValue>(entry.yamlPath);
            storeToPersist.set<Config::ConfigValue>(entry.yamlPath, currentValue);
            const Config::ConfigValue defaultValue = m_defaults.has(entry.yamlPath)
                ? m_defaults.get<Config::ConfigValue>(entry.yamlPath)
                : entry.defaultValue;
            defaultsToPersist.set<Config::ConfigValue>(entry.yamlPath, defaultValue);
            if (currentValue != defaultValue) {
                ++result.persistedCount;
            }
        }
        pathsToPersist = *m_paths;
    }

    try {
        storeToPersist.saveOverrides(pathsToPersist.overrideConfig, defaultsToPersist);
        result.ipcStatus = Ipc::IpcStatusCode::Ok;
        result.status = Ipc::ConfigV3MutationStatus::Ok;
    } catch (const std::exception& ex) {
        result.ipcStatus = Ipc::IpcStatusCode::InternalError;
        result.status = Ipc::ConfigV3MutationStatus::PersistFailed;
        result.failedCount = result.persistedCount == 0 ? 1 : result.persistedCount;
        LOG_ERROR("Service", __func__, "Config", "PersistConfigV3 failed: {}", ex.what());
    }
    return result;
}

} // namespace Service
