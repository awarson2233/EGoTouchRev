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
#include <exception>
#include <memory>
#include <optional>
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

bool ValueInRange(double value, const std::optional<Config::ConfigRange>& range) {
    return !range.has_value() || (value >= range->min && value <= range->max);
}

std::optional<Config::ConfigValue> NormalizeConfigValueForSchema(std::string_view path,
                                                                 const Config::ConfigValue& value,
                                                                 const Config::ConfigSchemaSnapshot& schema,
                                                                 bool requireLiveApply) {
    const auto it = std::find_if(schema.entries.begin(), schema.entries.end(),
        [path](const Config::ConfigSchemaEntry& entry) { return entry.yamlPath == path; });
    if (it == schema.entries.end()) return std::nullopt;
    if (requireLiveApply) {
        if (!it->boundToRuntime ||
            (it->runtimeBinding != Config::ConfigRuntimeBinding::LiveSetter &&
             it->runtimeBinding != Config::ConfigRuntimeBinding::ManualLiveApply) ||
            !Config::isLiveApplyTiming(it->applyTiming)) return std::nullopt;
    }
    if (path == "service.mode") {
        const auto str = Config::tryGetValue<std::string>(value);
        if (str.has_value() && (*str == "full" || *str == "touch_only")) return value;
        return std::nullopt;
    }
    if (path == "service.pen_button_mode") {
        return ParsePenButtonModeValue(value).has_value() ? std::optional<Config::ConfigValue>{value} : std::nullopt;
    }
    if (path == "service.pen_button_route") {
        return ParsePenButtonRouteValue(value).has_value() ? std::optional<Config::ConfigValue>{value} : std::nullopt;
    }
    switch (it->uiType) {
    case Config::ConfigUiType::Bool:
        if (Config::tryGetValue<bool>(value).has_value()) return value;
        break;
    case Config::ConfigUiType::Int32: {
        const auto v = Config::tryGetValue<int32_t>(value);
        if (v.has_value() && ValueInRange(*v, it->range)) return value;
        break;
    }
    case Config::ConfigUiType::Float: {
        if (const auto v = Config::tryGetValue<float>(value)) {
            if (std::isfinite(*v) && ValueInRange(*v, it->range)) return value;
        }
        if (const auto v = Config::tryGetValue<int32_t>(value)) {
            const float normalized = static_cast<float>(*v);
            if (std::isfinite(normalized) && ValueInRange(normalized, it->range)) {
                return Config::ConfigValue(normalized);
            }
        }
        break;
    }
    case Config::ConfigUiType::Enum:
    case Config::ConfigUiType::String:
        if (Config::tryGetValue<std::string>(value).has_value()) return value;
        break;
    }
    return std::nullopt;
}

bool ConfigValueAllowedBySchema(std::string_view path,
                                const Config::ConfigValue& value,
                                const Config::ConfigSchemaSnapshot& schema,
                                bool requireLiveApply) {
    return NormalizeConfigValueForSchema(path, value, schema, requireLiveApply).has_value();
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
    if (!configPath.empty()) {
        LOG_WARN("Service", __func__, "Config", "Ignoring external config path '{}'; YAML config files are no longer supported.", configPath);
    }

    Config::ConfigStore defaults;
    Config::ConfigSchemaSnapshot schema;
    WithRuntimeConfigDefaults([&defaults, &schema](Config::ConfigBinder& binder, Config::ConfigStore& factoryDefaults) {
        defaults = factoryDefaults;
        schema = Config::BuildMergedSchema(defaults, binder);
    });

    Config::ConfigStore current;
    current.mergeFrom(defaults);
    bool penButtonRouteExplicit = false;

    if (!ValidateStartupConfig(current, validateStartupConfig)) {
        LOG_ERROR("Service", __func__, "Config", "Startup config schema validation failed; startup blocked.");
        return false;
    }

    for (const auto& path : current.allPaths()) {
        const auto value = current.get<Config::ConfigValue>(path);
        if (const auto normalizedValue = NormalizeConfigValueForSchema(path, value, schema, false)) {
            if (*normalizedValue != value) {
                current.set<Config::ConfigValue>(path, *normalizedValue);
            }
        } else if (defaults.has(path)) {
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
    m_activeStore = m_store;
    m_schema = std::move(schema);
    m_penButtonRouteExplicit = penButtonRouteExplicit;
    m_activePenButtonRouteExplicit = penButtonRouteExplicit;
    auto state = ReadActiveServiceConfigStateLocked();
    WriteServiceConfigStateToStoreLocked(m_store, m_penButtonRouteExplicit, state);
    WriteServiceConfigStateToStoreLocked(m_activeStore, m_activePenButtonRouteExplicit, state);
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
    return ReadActiveServiceConfigStateLocked();
}

void ConfigRuntime::WriteServiceState(const ServiceConfigState& config) {
    std::lock_guard<std::mutex> lk(m_mutex);
    WriteServiceConfigStateToStoreLocked(m_store, m_penButtonRouteExplicit, config);
    WriteServiceConfigStateToStoreLocked(m_activeStore, m_activePenButtonRouteExplicit, config);
}

ServiceConfigState ConfigRuntime::ReadServiceConfigStateFromStoreLocked(const Config::ConfigStore& store,
                                                                        bool penButtonRouteExplicit) const {
    ServiceConfigState state{};
    state.penButtonRouteExplicit = penButtonRouteExplicit;
    ApplyConfig(state, store);
    state.penButtonRouteExplicit = penButtonRouteExplicit;
    return state;
}

ServiceConfigState ConfigRuntime::ReadActiveServiceConfigStateLocked() const {
    return ReadServiceConfigStateFromStoreLocked(m_activeStore, m_activePenButtonRouteExplicit);
}

void ConfigRuntime::WriteServiceConfigStateToStoreLocked(Config::ConfigStore& store,
                                                         bool& penButtonRouteExplicit,
                                                         const ServiceConfigState& config) {
    store.set<std::string>("service.mode", ServiceModeToConfig(config.mode));
    store.set<bool>("service.auto_mode", config.autoMode);
    store.set<bool>("service.stylus_vhf_enabled", config.stylusVhfEnabled);
    store.set<std::string>("service.pen_button_mode", ToConfigValue(config.penButtonMode));
    store.set<std::string>("service.pen_button_route", ToConfigValue(config.penButtonRoute));
    penButtonRouteExplicit = config.penButtonRouteExplicit;
}

bool ConfigRuntime::PersistServicePolicyConfig(const ServiceConfigState& config) {
    std::lock_guard<std::mutex> lk(m_mutex);
    WriteServiceConfigStateToStoreLocked(m_store, m_penButtonRouteExplicit, config);
    WriteServiceConfigStateToStoreLocked(m_activeStore, m_activePenButtonRouteExplicit, config);
    LOG_WARN("Service", __func__, "Config", "Service policy updated for current session only; persistent config files are no longer supported.");
    return false;
}

bool ConfigRuntime::ApplyPatchPayloadLocked(const uint8_t* data,
                                            size_t size,
                                            V3ApplyResult& result) {
    const auto parseResult = Config::deserializePatchDetailed(data, size);
    if (!parseResult.ok()) {
        result.runtimeStatus = parseResult.status == Config::ConfigTlvParseStatus::InvalidArgument
            ? ServiceRuntimeStatusCode::InternalError
            : ServiceRuntimeStatusCode::InvalidRequest;
        result.status = ConfigV3MutationStatus::Rejected;
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
    Config::ConfigStore candidateDesired = m_store;
    Config::ConfigStore candidateActive = m_activeStore;
    const bool currentDesiredPenButtonRouteExplicit = m_penButtonRouteExplicit;
    const bool currentActivePenButtonRouteExplicit = m_activePenButtonRouteExplicit;
    ConfigChangeSet liveChangeSet{};
    ConfigChangeSet restartChangeSet{};
    liveChangeSet.entryCount = patch.entries.size();
    restartChangeSet.entryCount = patch.entries.size();

    auto rejectEntry = [&](const Config::ConfigTlvEntry& entry) {
        result.runtimeStatus = ServiceRuntimeStatusCode::Ok;
        result.status = ConfigV3MutationStatus::Rejected;
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
        const Config::ConfigValue parsedValue = ConfigValueFromTlvEntry(entry, valueOk);
        if (!valueOk) {
            rejectEntry(entry);
            return false;
        }

        const std::string pathString(*path);
        const auto* schemaEntry = FindSchemaEntry(m_schema, pathString);
        const auto normalizedValue = NormalizeConfigValueForSchema(pathString, parsedValue, m_schema, false);
        if (schemaEntry == nullptr ||
            !schemaEntry->boundToRuntime ||
            (schemaEntry->runtimeBinding != Config::ConfigRuntimeBinding::LiveSetter &&
             schemaEntry->runtimeBinding != Config::ConfigRuntimeBinding::ManualLiveApply) ||
            !IsPatchableV3Timing(schemaEntry->applyTiming) ||
            !normalizedValue.has_value()) {
            rejectEntry(entry);
            return false;
        }

        const Config::ConfigValue& value = *normalizedValue;
        const bool touchesPenButtonRoute = pathString == "service.pen_button_route";
        if (touchesPenButtonRoute) penButtonRouteTouched = true;
        const bool hadPreviousValue = candidateDesired.has(pathString);
        const Config::ConfigValue previousValue = hadPreviousValue
            ? candidateDesired.get<Config::ConfigValue>(pathString)
            : Config::ConfigValue(std::string{});
        const bool valueChanged = !hadPreviousValue || previousValue != value;
        const bool explicitnessChanged = touchesPenButtonRoute && !currentDesiredPenButtonRouteExplicit;
        const bool changed = valueChanged || explicitnessChanged;
        candidateDesired.set<Config::ConfigValue>(pathString, value);
        if (schemaEntry->applyTiming != Config::ConfigApplyTiming::RestartRequired) {
            candidateActive.set<Config::ConfigValue>(pathString, value);
        }
        if (!changed) continue;

        ConfigChange change{};
        change.path = pathString;
        change.keyId = entry.keyId;
        change.previousValue = previousValue;
        change.newValue = value;
        change.hadPreviousValue = hadPreviousValue;

        if (schemaEntry->applyTiming == Config::ConfigApplyTiming::RestartRequired) {
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
        auto validation = target->validateConfig(candidateActive, liveChangeSet);
        result.targetResults.push_back(validation);
        if (!validation.ok) {
            result.runtimeStatus = ServiceRuntimeStatusCode::Ok;
            result.status = ConfigV3MutationStatus::Rejected;
            result.rejectedCount = liveChangeSet.changedCount();
            LOG_WARN("Service", __func__, "Config", "Rejected config v3 patch by {}: {}",
                     validation.targetName, validation.message);
            return false;
        }
    }

    const auto currentServiceConfig = ReadActiveServiceConfigStateLocked();
    result.desiredServiceConfig = currentServiceConfig;
    ApplyConfig(result.desiredServiceConfig, candidateActive);
    result.desiredServiceConfig.penButtonRouteExplicit = currentActivePenButtonRouteExplicit || penButtonRouteTouched;

    m_store = candidateDesired;
    m_activeStore = candidateActive;
    m_penButtonRouteExplicit = currentDesiredPenButtonRouteExplicit || penButtonRouteTouched;
    m_activePenButtonRouteExplicit = currentActivePenButtonRouteExplicit || penButtonRouteTouched;
    result.pipelineConfig = m_activeStore;

    for (const auto& target : m_targets) {
        if (!target->isInterested(liveChangeSet)) continue;
        auto applyResult = target->applyConfig(m_activeStore, liveChangeSet, ConfigApplyPhase::Live);
        for (auto& action : applyResult.actions) {
            if (action.kind == ConfigApplyActionKind::ServicePolicy) {
                action.serviceConfig = result.desiredServiceConfig;
            } else if (action.kind == ConfigApplyActionKind::PipelineRuntime) {
                action.configStore = m_activeStore;
            }
            result.applyActions.push_back(action);
        }
        result.targetResults.push_back(std::move(applyResult));
    }

    result.runtimeStatus = ServiceRuntimeStatusCode::Ok;
    result.status = result.changedCount == 0 ? ConfigV3MutationStatus::NoChanges : ConfigV3MutationStatus::Ok;
    return true;
}

ConfigRuntime::V3ApplyResult ConfigRuntime::ApplyConfigPatchV3(uint32_t baseSchemaVersion,
                                                               uint32_t baseSnapshotVersion,
                                                               const uint8_t* data,
                                                               size_t size) {
    V3ApplyResult result{};
    if (data == nullptr || size == 0 || size > kConfigPatchV3PayloadBytes) {
        result.runtimeStatus = ServiceRuntimeStatusCode::InvalidRequest;
        result.status = ConfigV3MutationStatus::Rejected;
        result.rejectedCount = 1;
        return result;
    }

    std::lock_guard<std::mutex> lk(m_mutex);
    const uint32_t currentSchemaVersion = SchemaVersionFor(m_schema);
    const uint32_t currentSnapshotVersion = SnapshotVersionFor(m_store, m_schema, currentSchemaVersion);
    if (baseSchemaVersion != currentSchemaVersion || baseSnapshotVersion != currentSnapshotVersion) {
        result.runtimeStatus = ServiceRuntimeStatusCode::Ok;
        result.status = ConfigV3MutationStatus::VersionMismatch;
        return result;
    }

    (void)ApplyPatchPayloadLocked(data, size, result);
    return result;
}

ConfigRuntime::V3PersistResult ConfigRuntime::PersistConfigV3() {
    V3PersistResult result{};
    result.runtimeStatus = ServiceRuntimeStatusCode::UnsupportedCommand;
    result.status = ConfigV3MutationStatus::PersistFailed;
    result.failedCount = 1;
    LOG_WARN("Service", __func__, "Config", "PersistConfigV3 rejected: persistent config files are no longer supported.");
    return result;
}

} // namespace Service
