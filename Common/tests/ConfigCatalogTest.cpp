#include "config/ConfigBinder.h"
#include "config/ConfigCatalog.h"
#include "config/ConfigKeyMap.h"
#include "config/ConfigStore.h"

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
        TestBinderDefaultsCompatibilityAndUnknownRuntimePath();
        std::cout << "[TEST] CommonConfigCatalogTest passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] CommonConfigCatalogTest failed: " << ex.what() << '\n';
        return 1;
    }
}
