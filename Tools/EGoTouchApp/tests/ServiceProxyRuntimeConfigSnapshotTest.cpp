#include "ServiceProxyInternal.h"
#include "DvrFormat.h"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

const App::DvrRuntimeConfigField* FindField(const App::DvrRuntimeConfigSnapshot& snapshot,
                                            const std::string& section,
                                            const std::string& key) {
    for (const auto& field : snapshot.fields) {
        if (field.section == section && field.key == key) return &field;
    }
    return nullptr;
}

const App::DvrRuntimeConfigValue* FindValue(const App::DvrRuntimeConfigSnapshot& snapshot,
                                            const App::DvrRuntimeConfigField& field) {
    for (const auto& value : snapshot.values) {
        if (value.fieldId == field.fieldId) return &value;
    }
    return nullptr;
}

void RequireMissingField(const App::DvrRuntimeConfigSnapshot& snapshot,
                         const std::string& section,
                         const std::string& key) {
    if (FindField(snapshot, section, key) != nullptr) {
        throw std::runtime_error("unexpected runtime config field: " + section + "." + key);
    }
}

void RequireStringValue(const App::DvrRuntimeConfigSnapshot& snapshot,
                        const std::string& section,
                        const std::string& key,
                        const std::string& expected) {
    const auto* field = FindField(snapshot, section, key);
    Require(field != nullptr, "expected runtime config string field");
    Require(field->valueType == Dvr::Format::Dvr2ConfigValueType::String, "expected string field type");
    const auto* value = FindValue(snapshot, *field);
    Require(value != nullptr, "expected runtime config string value");
    Require(value->stringValue == expected, "runtime config string value mismatch");
}

void RequireRawValue(const App::DvrRuntimeConfigSnapshot& snapshot,
                     const std::string& section,
                     const std::string& key,
                     Dvr::Format::Dvr2ConfigValueType expectedType,
                     uint64_t expectedRaw) {
    const auto* field = FindField(snapshot, section, key);
    Require(field != nullptr, "expected runtime config raw field");
    Require(field->valueType == expectedType, "runtime config raw field type mismatch");
    const auto* value = FindValue(snapshot, *field);
    Require(value != nullptr, "expected runtime config raw value");
    Require(value->valid, "runtime config value should be valid");
    Require(value->valueType == expectedType, "runtime config raw value type mismatch");
    Require(value->rawValue == expectedRaw, "runtime config raw value mismatch");
}

void TestServiceAndAppRuntimeFields() {
    Solvers::TouchPipeline touchPipeline;
    Solvers::StylusPipeline stylusPipeline;
    App::ServiceRuntimeConfigState serviceState{};
    serviceState.desiredModeFull = false;
    serviceState.activeModeFull = true;
    serviceState.autoMode = false;
    serviceState.stylusVhfEnabled = true;
    serviceState.penButtonMode = PenButtonMode::NativeEraser;
    serviceState.penButtonRoute = PenButtonRoute::Win32Only;

    App::AppRuntimeConfigState appState{};
    appState.vhfEnabled = false;
    appState.vhfTranspose = true;
    appState.masterParserOnly = true;

    const auto snapshot = App::BuildRuntimeConfigSnapshotFromState(serviceState, appState, touchPipeline, stylusPipeline);

    RequireStringValue(snapshot, "Service", "desired_mode", "touch_only");
    RequireStringValue(snapshot, "Service", "active_mode", "full");
    RequireRawValue(snapshot, "Service", "auto_mode", Dvr::Format::Dvr2ConfigValueType::Bool, 0);
    RequireRawValue(snapshot, "Service", "stylus_vhf_enabled", Dvr::Format::Dvr2ConfigValueType::Bool, 1);
    RequireRawValue(snapshot, "Service", "pen_button_mode", Dvr::Format::Dvr2ConfigValueType::UInt8, static_cast<uint8_t>(PenButtonMode::NativeEraser));
    RequireRawValue(snapshot, "Service", "pen_button_route", Dvr::Format::Dvr2ConfigValueType::UInt8, static_cast<uint8_t>(PenButtonRoute::Win32Only));
    RequireRawValue(snapshot, "AppRuntime", "vhf_enabled", Dvr::Format::Dvr2ConfigValueType::Bool, 0);
    RequireRawValue(snapshot, "AppRuntime", "vhf_transpose", Dvr::Format::Dvr2ConfigValueType::Bool, 1);
    RequireRawValue(snapshot, "AppRuntime", "master_parser_only", Dvr::Format::Dvr2ConfigValueType::Bool, 1);
}

void TestPipelineFieldsAndTypes() {
    Solvers::TouchPipeline touchPipeline;
    Solvers::StylusPipeline stylusPipeline;
    touchPipeline.m_baseline.m_enabled = false;
    touchPipeline.m_baseline.m_baseline = 123;
    touchPipeline.m_baseline.m_noFingerMaxStep = 777;
    touchPipeline.m_tracker.m_maxTrackDistance = 7.5f;
    touchPipeline.m_tracker.m_stylusSuppressPenPeakThreshold = 2468;
    touchPipeline.m_gesture.m_bypassStateMachine = true;
    stylusPipeline.m_hpp3.m_postPressure.m_btFreqShiftDebounceFrames = 2;

    const auto snapshot = App::BuildRuntimeConfigSnapshotFromState({}, {}, touchPipeline, stylusPipeline);

    RequireMissingField(snapshot, "TouchPipeline", "BaselineEnabled");
    RequireMissingField(snapshot, "TouchPipeline", "BaselineValue");
    RequireMissingField(snapshot, "TouchPipeline", "MaxTrackDistance");
    RequireMissingField(snapshot, "TouchPipeline", "StylusSuppressPenPeakThreshold");
    RequireMissingField(snapshot, "TouchPipeline", "BypassStateMachine");
    RequireRawValue(snapshot, "TouchPipeline", "BaselineNoFingerMaxStep", Dvr::Format::Dvr2ConfigValueType::Int32, 777);
    RequireRawValue(snapshot, "StylusPipeline", "sp.btFreqShiftDebounceFrames", Dvr::Format::Dvr2ConfigValueType::Int32, 2);
}

void TestFieldIdsAreContiguousAndValuesAligned() {
    Solvers::TouchPipeline touchPipeline;
    Solvers::StylusPipeline stylusPipeline;
    const auto snapshot = App::BuildRuntimeConfigSnapshotFromState({}, {}, touchPipeline, stylusPipeline);

    Require(!snapshot.fields.empty(), "runtime config snapshot should include fields");
    Require(snapshot.fields.size() == snapshot.values.size(), "runtime config fields and values should align");
    for (size_t i = 0; i < snapshot.fields.size(); ++i) {
        const uint32_t expectedId = static_cast<uint32_t>(i + 1);
        Require(snapshot.fields[i].fieldId == expectedId, "runtime config field ids should be contiguous");
        Require(snapshot.values[i].fieldId == expectedId, "runtime config value ids should align with fields");
    }
}

} // namespace

int main() {
    try {
        TestServiceAndAppRuntimeFields();
        TestPipelineFieldsAndTypes();
        TestFieldIdsAreContiguousAndValuesAligned();
        std::cout << "[TEST] ServiceProxy runtime config snapshot tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
