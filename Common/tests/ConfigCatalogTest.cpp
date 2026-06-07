#include "config/ConfigBinder.h"
#include "config/ConfigCatalog.h"
#include "config/ConfigKeyMap.h"
#include "config/ConfigStore.h"
#include "config/ConfigTlv.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void Require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct RuntimeConfig {
    bool serviceMode = false;
    int32_t baselineStep = 0;
    int32_t unknownRuntime = 0;
};

Config::ConfigBinder MakeBinder(RuntimeConfig& runtime)
{
    Config::ConfigBinder binder;
    binder.bind("service.mode", &RuntimeConfig::serviceMode, runtime, false, {}, "service mode");
    binder.bind("touch.signal_cond.baseline_bg_max_step", &RuntimeConfig::baselineStep, runtime, int32_t{7}, {0.0, 99.0}, "baseline step");
    binder.bind("touch.unmapped.runtime_only", &RuntimeConfig::unknownRuntime, runtime, int32_t{11}, {0.0, 20.0}, "unknown runtime");
    return binder;
}

void TestUniquePathAndKeyIdValidation()
{
    Config::ConfigDescriptor first;
    first.path = "service.mode";
    first.keyId = Config::ConfigKeyId::SvcMode;
    first.defaultValue = false;

    Config::ConfigDescriptor duplicatePath = first;
    duplicatePath.keyId = Config::ConfigKeyId::SvcAutoMode;

    bool duplicatePathRejected = false;
    try {
        Config::ConfigCatalog({first, duplicatePath});
    } catch (const std::invalid_argument&) {
        duplicatePathRejected = true;
    }
    Require(duplicatePathRejected, "ConfigCatalog should reject duplicate paths");

    Config::ConfigDescriptor duplicateKey = first;
    duplicateKey.path = "service.auto_mode";

    bool duplicateKeyRejected = false;
    try {
        Config::ConfigCatalog({first, duplicateKey});
    } catch (const std::invalid_argument&) {
        duplicateKeyRejected = true;
    }
    Require(duplicateKeyRejected, "ConfigCatalog should reject duplicate keyIds");
}

void TestRoundtripAndStableSort()
{
    Config::ConfigDescriptor touch;
    touch.path = "touch.signal_cond.baseline_bg_max_step";
    touch.keyId = Config::ConfigKeyId::TouchBaselineBgMaxStep;
    touch.defaultValue = int32_t{7};
    touch.uiType = Config::ConfigUiType::Int32;
    touch.description = "touch descriptor";

    Config::ConfigDescriptor service;
    service.path = "service.mode";
    service.keyId = Config::ConfigKeyId::SvcMode;
    service.defaultValue = false;
    service.uiType = Config::ConfigUiType::Bool;
    service.description = "service descriptor";

    Config::ConfigCatalog catalog({touch, service});
    Require(catalog.descriptors().front().keyId == Config::ConfigKeyId::SvcMode,
            "ConfigCatalog should sort descriptors by keyId");
    Require(catalog.findByPath("service.mode").has_value(), "ConfigCatalog should find by path");
    Require(catalog.findByKeyId(Config::ConfigKeyId::TouchBaselineBgMaxStep).has_value(),
            "ConfigCatalog should find by keyId");

    const auto snapshot = Config::BuildSchemaSnapshot(catalog);
    Require(snapshot.entries.size() == 2, "BuildSchemaSnapshot should preserve descriptor count");
    Require(snapshot.entries[0].keyId == Config::ConfigKeyId::SvcMode,
            "schema snapshot should remain sorted by keyId");

    const auto roundtrip = Config::ConfigCatalogBuilder::fromSnapshot(snapshot);
    Require(roundtrip.descriptors().size() == 2, "catalog snapshot roundtrip should preserve descriptor count");
    Require(roundtrip.descriptors()[1].description == "touch descriptor",
            "catalog snapshot roundtrip should preserve metadata");
}

void TestConfigV3PayloadSerialization()
{
    Config::ConfigDescriptor second;
    second.path = "touch.signal_cond.baseline_bg_max_step";
    second.keyId = Config::ConfigKeyId::TouchBaselineBgMaxStep;
    second.defaultValue = int32_t{7};
    second.uiType = Config::ConfigUiType::Int32;
    second.range = Config::ConfigRange{0.0, 99.0};
    second.displayName = "Baseline Step";
    second.description = "baseline description";
    second.moduleTag = "Touch";
    second.enumMapping = {{1, "one"}, {2, "two"}};
    second.runtimeBinding = Config::ConfigRuntimeBinding::LiveSetter;
    second.boundToRuntime = true;

    Config::ConfigDescriptor first;
    first.path = "service.mode";
    first.keyId = Config::ConfigKeyId::SvcMode;
    first.defaultValue = false;
    first.uiType = Config::ConfigUiType::Bool;
    first.displayName = "Service Mode";
    first.description = "service description";
    first.moduleTag = "Service";

    Config::ConfigV3CatalogPayload catalog;
    catalog.schemaVersion = 11;
    catalog.snapshotVersion = 22;
    catalog.entries = {second, first};
    const auto catalogBytes = Config::serializeConfigV3Catalog(catalog);
    const auto catalogRoundtrip = Config::deserializeConfigV3Catalog(catalogBytes.data(), catalogBytes.size());
    Require(catalogRoundtrip.schemaVersion == 11 && catalogRoundtrip.snapshotVersion == 22, "Config v3 catalog versions should roundtrip");
    Require(catalogRoundtrip.entries.size() == 2, "Config v3 catalog entries should roundtrip");
    Require(catalogRoundtrip.entries[0].keyId == Config::ConfigKeyId::SvcMode, "Config v3 catalog should sort by keyId");
    Require(catalogRoundtrip.entries[1].path == second.path, "Config v3 catalog path should roundtrip");
    Require(Config::tryGetValue<int32_t>(catalogRoundtrip.entries[1].defaultValue).value_or(0) == 7, "Config v3 catalog default value should roundtrip");
    Require(catalogRoundtrip.entries[1].range.has_value() && catalogRoundtrip.entries[1].range->max == 99.0, "Config v3 catalog range should roundtrip");
    Require(catalogRoundtrip.entries[1].displayName == second.displayName, "Config v3 catalog displayName should roundtrip");
    Require(catalogRoundtrip.entries[1].description == second.description, "Config v3 catalog description should roundtrip");
    Require(catalogRoundtrip.entries[1].moduleTag == second.moduleTag, "Config v3 catalog moduleTag should roundtrip");
    Require(catalogRoundtrip.entries[1].enumMapping.size() == 2 && catalogRoundtrip.entries[1].enumMapping[1].second == "two", "Config v3 catalog enumMapping should roundtrip");
    Require(catalogRoundtrip.entries[1].runtimeBinding == Config::ConfigRuntimeBinding::LiveSetter, "Config v3 catalog runtimeBinding should roundtrip");
    Require(catalogRoundtrip.entries[1].boundToRuntime, "Config v3 catalog boundToRuntime should roundtrip");

    Config::ConfigV3SnapshotPayload snapshot;
    snapshot.schemaVersion = 11;
    snapshot.snapshotVersion = 23;
    snapshot.entries = {{Config::ConfigKeyId::SvcAutoMode, true}, {Config::ConfigKeyId::SvcPenButtonMode, std::string{"native_barrel"}}, {Config::ConfigKeyId::TouchBaselineBgMaxStep, int32_t{42}}};
    const auto snapshotBytes = Config::serializeConfigV3Snapshot(snapshot);
    const auto snapshotRoundtrip = Config::deserializeConfigV3Snapshot(snapshotBytes.data(), snapshotBytes.size());
    Require(snapshotRoundtrip.entries.size() == 3, "Config v3 snapshot entries should roundtrip");
    Require(Config::tryGetValue<bool>(snapshotRoundtrip.entries[0].value).value_or(false), "Config v3 snapshot bool should roundtrip");
    Require(Config::tryGetValue<std::string>(snapshotRoundtrip.entries[1].value).value_or("") == "native_barrel", "Config v3 snapshot string should roundtrip");
    Require(Config::tryGetValue<int32_t>(snapshotRoundtrip.entries[2].value).value_or(0) == 42, "Config v3 snapshot int32 should roundtrip");

    bool truncatedRejected = false;
    try { (void)Config::deserializeConfigV3Catalog(catalogBytes.data(), catalogBytes.size() - 1); } catch (const std::runtime_error&) { truncatedRejected = true; }
    Require(truncatedRejected, "Config v3 catalog should reject truncated payload");
    auto trailing = snapshotBytes;
    trailing.push_back(0xEE);
    bool trailingRejected = false;
    try { (void)Config::deserializeConfigV3Snapshot(trailing.data(), trailing.size()); } catch (const std::runtime_error&) { trailingRejected = true; }
    Require(trailingRejected, "Config v3 snapshot should reject trailing bytes");
    auto unsupported = catalogBytes;
    unsupported[4] = 2;
    bool versionRejected = false;
    try { (void)Config::deserializeConfigV3Catalog(unsupported.data(), unsupported.size()); } catch (const std::runtime_error&) { versionRejected = true; }
    Require(versionRejected, "Config v3 catalog should reject unsupported version");
}

void TestBinderDefaultsCompatibilityAndUnknownRuntimePath()
{
    RuntimeConfig runtime;
    runtime.serviceMode = true;
    runtime.baselineStep = 42;
    auto binder = MakeBinder(runtime);

    Config::ConfigStore defaults;
    defaults.set("service.mode", false);
    defaults.set("touch.signal_cond.baseline_bg_max_step", int32_t{7});
    defaults.set("stylus.sp.signal_floor", int32_t{5});

    Config::registerRuntimeKeyMappings(binder);
    Require(!Config::tryKeyIdForPath("touch.unmapped.runtime_only").has_value(),
            "registerRuntimeKeyMappings must not silently allocate IPC keyIds");

    const auto catalog = Config::BuildConfigCatalog(defaults, binder);
    const auto unknown = catalog.findByPath("touch.unmapped.runtime_only");
    Require(unknown.has_value(), "catalog should retain unknown runtime path as metadata");
    Require(unknown->get().keyId == Config::ConfigKeyId::MaxKeyId,
            "unknown runtime path should use MaxKeyId sentinel");

    const auto schema = Config::BuildMergedSchema(defaults, binder);
    Require(schema.entries.size() == 4, "BuildMergedSchema should merge defaults and binder paths");
    Require(schema.entries[0].keyId == Config::ConfigKeyId::SvcMode,
            "BuildMergedSchema should sort by stable keyId");

    bool sawCurrentRuntimeValue = false;
    for (const auto& entry : schema.entries) {
        if (entry.yamlPath == "touch.signal_cond.baseline_bg_max_step") {
            sawCurrentRuntimeValue = Config::tryGetValue<int32_t>(entry.currentValue).value_or(0) == 42;
        }
    }
    Require(sawCurrentRuntimeValue, "BuildMergedSchema should preserve binder current value compatibility");
}

} // namespace

int main()
{
    try {
        TestUniquePathAndKeyIdValidation();
        TestRoundtripAndStableSort();
        TestConfigV3PayloadSerialization();
        TestBinderDefaultsCompatibilityAndUnknownRuntimePath();
        std::cout << "[TEST] CommonConfigCatalogTest passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] CommonConfigCatalogTest failed: " << ex.what() << '\n';
        return 1;
    }
}
