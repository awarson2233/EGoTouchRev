#include "ServiceProxy.h"
#include "ServiceProxyInternal.h"

#include <array>
#include <bit>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

namespace App {
namespace {

using ConfigValueType = Dvr::Format::Dvr2ConfigValueType;

Dvr::Format::Dvr2RuntimeConfigFieldDef MakeRuntimeConfigFieldDef(
    const DvrRuntimeConfigField& field) {
    Dvr::Format::Dvr2RuntimeConfigFieldDef wire{};
    wire.fieldId = field.fieldId;
    wire.valueType = static_cast<uint8_t>(field.valueType);
    wire.category = field.category;
    wire.minValue = field.minValue;
    wire.maxValue = field.maxValue;
    Dvr::Format::CopyFixedString(wire.section, sizeof(wire.section), field.section);
    Dvr::Format::CopyFixedString(wire.key, sizeof(wire.key), field.key);
    Dvr::Format::CopyFixedString(wire.displayName, sizeof(wire.displayName), field.displayName);
    Dvr::Format::CopyFixedString(wire.moduleTag, sizeof(wire.moduleTag), field.moduleTag);
    Dvr::Format::CopyFixedString(wire.unit, sizeof(wire.unit), field.unit);
    return wire;
}

uint32_t RuntimeConfigSchemaHash(const std::vector<DvrRuntimeConfigField>& fields) {
    std::vector<Dvr::Format::Dvr2RuntimeConfigFieldDef> wireFields;
    wireFields.reserve(fields.size());
    for (const auto& field : fields) {
        wireFields.push_back(MakeRuntimeConfigFieldDef(field));
    }
    return Dvr::Format::ComputeRuntimeConfigSchemaHash(wireFields);
}

uint64_t FloatRaw(float value) noexcept {
    return static_cast<uint64_t>(std::bit_cast<uint32_t>(value));
}

const char* ServiceModeText(bool full) noexcept {
    return full ? "full" : "touch_only";
}

class RuntimeConfigSnapshotBuilder {
public:
    void AddString(std::string_view section,
                   std::string_view key,
                   std::string value,
                   std::string_view displayName,
                   std::string_view moduleTag) {
        auto field = MakeField(section, key, ConfigValueType::String, displayName, moduleTag);
        auto runtimeValue = MakeValue(field.fieldId, ConfigValueType::String);
        runtimeValue.stringValue = std::move(value);
        m_snapshot.fields.push_back(std::move(field));
        m_snapshot.values.push_back(std::move(runtimeValue));
    }

    void AddBool(std::string_view section,
                 std::string_view key,
                 bool value,
                 std::string_view displayName,
                 std::string_view moduleTag) {
        AddRaw(section, key, ConfigValueType::Bool, value ? 1u : 0u, displayName, moduleTag);
    }

    void AddUInt8(std::string_view section,
                  std::string_view key,
                  uint8_t value,
                  std::string_view displayName,
                  std::string_view moduleTag) {
        AddRaw(section, key, ConfigValueType::UInt8, value, displayName, moduleTag);
    }

    void AddInt32(std::string_view section,
                  std::string_view key,
                  int32_t value,
                  std::string_view displayName,
                  std::string_view moduleTag,
                  float minValue = 0.0f,
                  float maxValue = 0.0f,
                  std::string_view unit = {}) {
        AddRaw(section,
               key,
               ConfigValueType::Int32,
               static_cast<uint64_t>(static_cast<int64_t>(value)),
               displayName,
               moduleTag,
               minValue,
               maxValue,
               unit);
    }

    void AddFloat32(std::string_view section,
                    std::string_view key,
                    float value,
                    std::string_view displayName,
                    std::string_view moduleTag,
                    float minValue = 0.0f,
                    float maxValue = 0.0f,
                    std::string_view unit = {}) {
        AddRaw(section, key, ConfigValueType::Float32, FloatRaw(value), displayName, moduleTag, minValue, maxValue, unit);
    }

    DvrRuntimeConfigSnapshot Finish() && {
        m_snapshot.schemaHash = RuntimeConfigSchemaHash(m_snapshot.fields);
        return std::move(m_snapshot);
    }

private:
    DvrRuntimeConfigField MakeField(std::string_view section,
                                    std::string_view key,
                                    ConfigValueType valueType,
                                    std::string_view displayName,
                                    std::string_view moduleTag,
                                    float minValue = 0.0f,
                                    float maxValue = 0.0f,
                                    std::string_view unit = {}) {
        DvrRuntimeConfigField field{};
        field.fieldId = ++m_nextFieldId;
        field.valueType = valueType;
        field.minValue = minValue;
        field.maxValue = maxValue;
        field.section = std::string(section);
        field.key = std::string(key);
        field.displayName = std::string(displayName);
        field.moduleTag = std::string(moduleTag);
        field.unit = std::string(unit);
        return field;
    }

    static DvrRuntimeConfigValue MakeValue(uint32_t fieldId, ConfigValueType valueType) {
        DvrRuntimeConfigValue value{};
        value.fieldId = fieldId;
        value.valueType = valueType;
        value.valid = true;
        return value;
    }

    void AddRaw(std::string_view section,
                std::string_view key,
                ConfigValueType valueType,
                uint64_t rawValue,
                std::string_view displayName,
                std::string_view moduleTag,
                float minValue = 0.0f,
                float maxValue = 0.0f,
                std::string_view unit = {}) {
        auto field = MakeField(section, key, valueType, displayName, moduleTag, minValue, maxValue, unit);
        auto value = MakeValue(field.fieldId, valueType);
        value.rawValue = rawValue;
        m_snapshot.fields.push_back(std::move(field));
        m_snapshot.values.push_back(std::move(value));
    }

    DvrRuntimeConfigSnapshot m_snapshot;
    uint32_t m_nextFieldId = 0;
};

} // namespace

TouchPipelineModuleEnableState CaptureTouchPipelineModuleEnableState(
    const Solvers::TouchPipeline& pipeline) {
    TouchPipelineModuleEnableState state{};
    state.baselineEnabled = pipeline.m_baseline.m_enabled;
    state.cmfEnabled = pipeline.m_cmf.m_enabled;
    state.gridIIREnabled = true;  // TODO: gridIIR removed; keep default
    state.trackerEnabled = pipeline.m_tracker.m_enabled;
    state.coordFilterEnabled = pipeline.m_coordFilter.m_enabled;
    state.gestureEnabled = pipeline.m_gesture.m_enabled;
    return state;
}

void ApplyTouchPipelineModuleEnableState(
    Solvers::TouchPipeline& pipeline,
    const TouchPipelineModuleEnableState& state) {
    const bool oldTrackerEnabled = pipeline.m_tracker.m_enabled;
    const bool oldGestureEnabled = pipeline.m_gesture.m_enabled;

    pipeline.m_baseline.m_enabled = state.baselineEnabled;
    pipeline.m_cmf.m_enabled = state.cmfEnabled;
    // gridIIREnabled not applicable (GridIIR disabled per project memory)
    pipeline.m_tracker.m_enabled = state.trackerEnabled;
    pipeline.m_coordFilter.m_enabled = state.coordFilterEnabled;
    pipeline.m_gesture.m_enabled = state.gestureEnabled;

    if (pipeline.m_tracker.m_enabled != oldTrackerEnabled) {
        pipeline.m_tracker.ClearLiveState();
    }
    if (pipeline.m_gesture.m_enabled != oldGestureEnabled) {
        pipeline.m_gesture.ClearLiveState();
    }
}

DvrRuntimeConfigSnapshot BuildRuntimeConfigSnapshotFromState(
    const ServiceRuntimeConfigState& serviceState,
    const AppRuntimeConfigState& appRuntimeState,
    const Solvers::TouchPipeline& touchPipeline,
    const Solvers::StylusPipeline& stylusPipeline) {
    RuntimeConfigSnapshotBuilder builder;

    builder.AddString("Service", "desired_mode", ServiceModeText(serviceState.desiredModeFull), "Desired Service Mode", "Service");
    builder.AddString("Service", "active_mode", ServiceModeText(serviceState.activeModeFull), "Active Service Mode", "Service");
    builder.AddBool("Service", "auto_mode", serviceState.autoMode, "Auto Mode", "Service");
    builder.AddBool("Service", "stylus_vhf_enabled", serviceState.stylusVhfEnabled, "Stylus VHF Enabled", "Service");
    builder.AddUInt8("Service",
                     "pen_button_mode",
                     static_cast<uint8_t>(serviceState.penButtonMode),
                     "Pen Button Mode",
                     "Service");
    builder.AddUInt8("Service",
                     "pen_button_route",
                     static_cast<uint8_t>(serviceState.penButtonRoute),
                     "Pen Button Route",
                     "Service");

    builder.AddBool("AppRuntime", "vhf_enabled", appRuntimeState.vhfEnabled, "VHF Enabled", "App Runtime");
    builder.AddBool("AppRuntime", "vhf_transpose", appRuntimeState.vhfTranspose, "VHF Transpose", "App Runtime");
    builder.AddBool("AppRuntime", "master_parser_only", appRuntimeState.masterParserOnly, "Master Parser Only", "App Runtime");

    // 原 BaselineNoFingerMaxStep 是复刻自创的「单帧步长硬上限」，原厂没有对应量：
    // TSACore 的 Stage 0 步长完全由 TSADynPrmt_UpdateBfrTch @0x6BB3C960 算出
    // （clamp(noiseThold >> 8, 1, 255)，W273AS2700 实测 = 2）。改为直接快照
    // 该派生量，取值范围与原版 clamp 一致（文档 01 §8）。
    builder.AddInt32("TouchPipeline",
                     "BaselineNoTouchStep",
                     static_cast<int32_t>(touchPipeline.m_baseline.GetDynParams().stepNoTouch),
                     "Baseline No Touch Step",
                     "Touch / Signal Conditioning",
                     1.0f,
                     255.0f);
    builder.AddInt32("StylusPipeline",
                     "sp.btFreqShiftDebounceFrames",
                     stylusPipeline.m_hpp3.m_postPressure.m_btFreqShiftDebounceFrames,
                     "BT Freq Shift Debounce Frames",
                     "Stylus / SP",
                     0.0f,
                     255.0f,
                     "frames");

    return std::move(builder).Finish();
}

DvrRuntimeConfigSnapshot ServiceProxy::CaptureRuntimeConfigSnapshot() const {
    const ServiceRuntimeConfigState serviceState{
        .desiredModeFull = m_srvDesiredModeFull.load(std::memory_order_relaxed),
        .activeModeFull = m_srvActiveModeFull.load(std::memory_order_relaxed),
        .autoMode = m_srvAutoMode.load(std::memory_order_relaxed),
        .stylusVhfEnabled = m_srvStylusVhfEnabled.load(std::memory_order_relaxed),
        .penButtonMode = m_srvPenButtonMode.load(std::memory_order_relaxed),
        .penButtonRoute = m_srvPenButtonRoute.load(std::memory_order_relaxed),
    };
    const AppRuntimeConfigState appRuntimeState{
        .vhfEnabled = m_vhfEnabled.load(std::memory_order_relaxed),
        .vhfTranspose = m_vhfTranspose.load(std::memory_order_relaxed),
        .masterParserOnly = m_masterParserOnly.load(std::memory_order_relaxed),
    };
    return BuildRuntimeConfigSnapshotFromState(serviceState, appRuntimeState, m_pipeline, m_stylusPipeline);
}

} // namespace App
