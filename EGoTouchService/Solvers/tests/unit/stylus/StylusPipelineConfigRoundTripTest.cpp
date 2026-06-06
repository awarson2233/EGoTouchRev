#include "StylusSolver/StylusPipeline.h"
#include "config/ConfigBinder.h"
#include "config/ConfigSchemaSnapshot.h"
#include "config/ConfigStore.h"
#include "config/ConfigValue.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

const Config::ConfigSchemaEntry* FindSchemaEntry(const Config::ConfigSchemaSnapshot& schema,
                                                 std::string_view yamlPath) {
    for (const auto& entry : schema.entries) {
        if (entry.yamlPath == yamlPath) {
            return &entry;
        }
    }
    return nullptr;
}

void RequireInt32Range(const Config::ConfigSchemaEntry& entry,
                       int32_t expectedDefault,
                       double expectedMin,
                       double expectedMax) {
    Require(entry.uiType == Config::ConfigUiType::Int32, "schema entry should be Int32");
    Require(entry.defaultValue == Config::ConfigValue(expectedDefault), "schema entry default should match");
    Require(entry.currentValue == Config::ConfigValue(expectedDefault), "schema entry current value should match default");
    Require(entry.range.has_value(), "schema entry should expose a range");
    Require(entry.range->min == expectedMin, "schema entry range min should match");
    Require(entry.range->max == expectedMax, "schema entry range max should match");
}

void RequireBoolDefault(const Config::ConfigSchemaEntry& entry, bool expectedDefault) {
    Require(entry.uiType == Config::ConfigUiType::Bool, "schema entry should be Bool");
    Require(entry.defaultValue == Config::ConfigValue(expectedDefault), "schema entry default should match");
    Require(entry.currentValue == Config::ConfigValue(expectedDefault), "schema entry current value should match default");
    Require(!entry.range.has_value(), "bool schema entry should not expose a range");
}

void RequireIirEntry(const Config::ConfigSchemaSnapshot& schema,
                     std::string_view yamlPath,
                     int32_t expectedDefault,
                     double expectedMin,
                     double expectedMax) {
    const auto* entry = FindSchemaEntry(schema, yamlPath);
    Require(entry != nullptr, "schema should contain expected IIR entry");
    Require(entry->moduleTag == "Stylus / Coordinate", "IIR entry should be grouped in Stylus / Coordinate");
    Require(entry->boundToRuntime, "IIR entry should be bound to runtime");
    Require(!entry->description.empty(), "IIR entry should expose a description");
    RequireInt32Range(*entry, expectedDefault, expectedMin, expectedMax);
}

Config::ConfigSchemaSnapshot BuildStylusSchema() {
    Solvers::StylusPipeline pipeline;
    Config::ConfigBinder binder;
    pipeline.registerBindings(binder);
    Config::ConfigStore defaults;
    binder.writeDefaults(defaults);
    binder.apply(defaults);
    return binder.snapshot();
}

void TestStylusIirBindingsExposeCompleteUiSchema() {
    const auto schema = BuildStylusSchema();

    const auto* enabled = FindSchemaEntry(schema, "stylus.sp.iir_filter_enabled");
    Require(enabled != nullptr, "schema should contain IIR enable entry");
    Require(enabled->moduleTag == "Stylus / Coordinate", "IIR enable entry should be grouped in Stylus / Coordinate");
    Require(enabled->boundToRuntime, "IIR enable entry should be bound to runtime");
    RequireBoolDefault(*enabled, true);

    RequireIirEntry(schema, "stylus.sp.iir_coef_low_in_band", 2, 0.0, 255.0);
    RequireIirEntry(schema, "stylus.sp.iir_coef_high_in_band", 16, 0.0, 255.0);
    RequireIirEntry(schema, "stylus.sp.iir_speed_thold_in_band", 20, 0.0, 255.0);
    RequireIirEntry(schema, "stylus.sp.iir_coef_low_edge", 6, 0.0, 255.0);
    RequireIirEntry(schema, "stylus.sp.iir_coef_high_edge", 18, 0.0, 255.0);
    RequireIirEntry(schema, "stylus.sp.iir_speed_thold_edge", 10, 0.0, 255.0);
    RequireIirEntry(schema, "stylus.sp.iir_speed_max", 205, 0.0, 1000.0);
    RequireIirEntry(schema, "stylus.sp.iir_max_coef", 32, 1.0, 255.0);
}

void TestHpp2BindingsAreSchemaOnly() {
    const auto schema = BuildStylusSchema();
    const auto* enabled = FindSchemaEntry(schema, "stylus.hpp2.enabled");
    Require(enabled != nullptr, "schema should contain HPP2 compatibility entry");
    Require(!enabled->boundToRuntime, "HPP2 entry should not be live runtime-bound");
    Require(enabled->runtimeBinding == Config::ConfigRuntimeBinding::SchemaOnly,
            "HPP2 entry should be schema-only");

    const auto* peakWidth = FindSchemaEntry(schema, "stylus.hpp2.peak_min_width");
    Require(peakWidth != nullptr, "schema should contain HPP2 peak width compatibility entry");
    Require(!peakWidth->boundToRuntime, "HPP2 peak width should not be live runtime-bound");
}

} // namespace

int main() {
    try {
        TestStylusIirBindingsExposeCompleteUiSchema();
        TestHpp2BindingsAreSchemaOnly();
        std::cout << "[TEST] Stylus pipeline config schema tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
