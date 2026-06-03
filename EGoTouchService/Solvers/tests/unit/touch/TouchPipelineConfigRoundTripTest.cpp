#include "TouchSolver/TouchPipeline.h"
#include "StylusPipeline.h"

#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void RequireNear(float actual, float expected, float epsilon, const char* message) {
    if (std::fabs(actual - expected) > epsilon) {
        throw std::runtime_error(message);
    }
}

void RequireMissingSubstring(const std::string& text, const char* substring) {
    if (text.find(substring) != std::string::npos) {
        throw std::runtime_error(std::string("unexpected saved config key: ") + substring);
    }
}

void RequirePresentSubstring(const std::string& text, const char* substring) {
    if (text.find(substring) == std::string::npos) {
        throw std::runtime_error(std::string("missing saved config key: ") + substring);
    }
}

template <typename Schema>
void RequireSchemaMissing(const Schema& schema, const char* key) {
    for (const auto& param : schema) {
        if (param.key == key) {
            throw std::runtime_error(std::string("unexpected schema key: ") + key);
        }
    }
}

template <typename Schema>
void RequireSchemaPresent(const Schema& schema, const char* key) {
    for (const auto& param : schema) {
        if (param.key == key) return;
    }
    throw std::runtime_error(std::string("missing schema key: ") + key);
}

void LoadFromSavedText(Solvers::TouchPipeline& pipeline, const std::string& saved) {
    std::istringstream in(saved);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        const size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        pipeline.LoadConfig(line.substr(0, eq), line.substr(eq + 1));
    }
}

void TestCurrentTouchConfigKeysAreHardcoded() {
    Solvers::TouchPipeline pipeline;
    pipeline.m_baseline.m_enabled = false;
    pipeline.m_baseline.m_baseline = 123;
    pipeline.m_cmf.m_enabled = false;
    pipeline.m_tracker.m_maxTrackDistance = 7.5f;
    pipeline.m_tracker.m_stylusSuppressPenPeakThreshold = 2468;
    pipeline.m_gesture.m_pressCandidateMinSignal = 999;
    pipeline.m_gesture.m_bypassStateMachine = true;

    std::ostringstream out;
    pipeline.SaveConfig(out);
    const std::string saved = out.str();

    const char* frozenSerializedKeys[] = {
        "BaselineEnabled=",
        "BaselineValue=",
        "CMFEnabled=",
        "CMFDimensionMode=",
        "MaxTrackDistance=",
        "StylusSuppressPenPeakThreshold=",
        "PressCandidateMinSignal=",
        "BypassStateMachine=",
    };
    for (const char* key : frozenSerializedKeys) {
        RequireMissingSubstring(saved, key);
    }
    RequireMissingSubstring(saved, "BaselineSettleFrames=");

    const auto schema = pipeline.GetConfigSchema();
    const char* frozenSchemaKeys[] = {
        "BaselineEnabled",
        "BaselineValue",
        "CMFEnabled",
        "MaxTrackDistance",
        "StylusSuppressPenPeakThreshold",
        "TouchDownDebounceMaxExtra",
        "RxGhostWeakRatio",
        "PalmShadowEnabled",
        "BypassStateMachine",
    };
    for (const char* key : frozenSchemaKeys) {
        RequireSchemaMissing(schema, key);
    }
    RequireSchemaMissing(schema, "BaselineSettleFrames");

    Solvers::TouchPipeline loaded;
    const bool baselineEnabled = loaded.m_baseline.m_enabled;
    const int baselineValue = loaded.m_baseline.m_baseline;
    const bool cmfEnabled = loaded.m_cmf.m_enabled;
    const auto cmfMode = loaded.m_cmf.m_mode;
    const float maxTrackDistance = loaded.m_tracker.m_maxTrackDistance;
    const int penPeakThreshold = loaded.m_tracker.m_stylusSuppressPenPeakThreshold;
    const int suppressPenPeakThreshold = loaded.m_stylusSuppress.m_stylusSuppressPenPeakThreshold;
    const int pressCandidateMinSignal = loaded.m_gesture.m_pressCandidateMinSignal;
    const bool bypassStateMachine = loaded.m_gesture.m_bypassStateMachine;

    loaded.LoadConfig("BaselineEnabled", "0");
    loaded.LoadConfig("BaselineValue", "123");
    loaded.LoadConfig("CMFEnabled", "0");
    loaded.LoadConfig("CMFDimensionMode", "3");
    loaded.LoadConfig("MaxTrackDistance", "7.5");
    loaded.LoadConfig("StylusSuppressPenPeakThreshold", "2468");
    loaded.LoadConfig("PressCandidateMinSignal", "999");
    loaded.LoadConfig("BypassStateMachine", "1");

    Require(loaded.m_baseline.m_enabled == baselineEnabled,
            "frozen baseline enabled config should not load");
    Require(loaded.m_baseline.m_baseline == baselineValue,
            "frozen baseline value config should not load");
    Require(loaded.m_cmf.m_enabled == cmfEnabled,
            "frozen cmf enabled config should not load");
    Require(loaded.m_cmf.m_mode == cmfMode,
            "frozen cmf dimension mode config should not load");
    RequireNear(loaded.m_tracker.m_maxTrackDistance, maxTrackDistance, 0.0001f,
                "frozen max track distance config should not load");
    Require(loaded.m_tracker.m_stylusSuppressPenPeakThreshold == penPeakThreshold,
            "frozen stylus suppress tracker config should not load");
    Require(loaded.m_stylusSuppress.m_stylusSuppressPenPeakThreshold == suppressPenPeakThreshold,
            "frozen stylus suppress runtime config should not load");
    Require(loaded.m_gesture.m_pressCandidateMinSignal == pressCandidateMinSignal,
            "frozen press candidate config should not load");
    Require(loaded.m_gesture.m_bypassStateMachine == bypassStateMachine,
            "frozen gesture bypass config should not load");
}

    void TestBaselineFingerStateConfigRoundTrip() {
        Solvers::TouchPipeline pipeline;
        pipeline.m_baseline.m_backgroundAlphaShift = 4;
        pipeline.m_baseline.m_backgroundMaxStep = 384;
        pipeline.m_baseline.m_recoveryAlphaShift = 1;
        pipeline.m_baseline.m_recoveryMaxStep = 128;
        pipeline.m_baseline.m_recoveryMaxFrames = 15;

        std::ostringstream out;
        pipeline.SaveConfig(out);
        const std::string saved = out.str();

        RequirePresentSubstring(saved, "BaselineBackgroundAlphaShift=4");
        RequirePresentSubstring(saved, "BaselineBackgroundMaxStep=384");
        RequirePresentSubstring(saved, "BaselineRecoveryAlphaShift=1");
        RequirePresentSubstring(saved, "BaselineRecoveryMaxStep=128");
        RequirePresentSubstring(saved, "BaselineRecoveryMaxFrames=15");

        const auto schema = pipeline.GetConfigSchema();
        RequireSchemaPresent(schema, "BaselineBackgroundAlphaShift");
        RequireSchemaPresent(schema, "BaselineBackgroundMaxStep");
        RequireSchemaPresent(schema, "BaselineRecoveryAlphaShift");
        RequireSchemaPresent(schema, "BaselineRecoveryMaxStep");
        RequireSchemaPresent(schema, "BaselineRecoveryMaxFrames");

        Solvers::TouchPipeline loaded;
        LoadFromSavedText(loaded, saved);

        Require(loaded.m_baseline.m_backgroundAlphaShift == 4,
            "baseline background alpha shift should round-trip");
        Require(loaded.m_baseline.m_backgroundMaxStep == 384,
            "baseline background max step should round-trip");
        Require(loaded.m_baseline.m_recoveryAlphaShift == 1,
            "baseline recovery alpha shift should round-trip");
        Require(loaded.m_baseline.m_recoveryMaxStep == 128,
            "baseline recovery max step should round-trip");
        Require(loaded.m_baseline.m_recoveryMaxFrames == 15,
            "baseline recovery max frames should round-trip");
    }

void TestEdgeCompensationIsHardcoded() {
    Solvers::TouchPipeline pipeline;

    std::ostringstream out;
    pipeline.SaveConfig(out);
    const std::string saved = out.str();

    RequireMissingSubstring(saved, "ECEnabled=");
    RequireMissingSubstring(saved, "ECStrength=");
    RequireMissingSubstring(saved, "ECBlendRange=");
    RequireMissingSubstring(saved, "ECDim1NearSegments=");
    RequireMissingSubstring(saved, "ECDim1NearS0Width=");
    RequireMissingSubstring(saved, "ECDim2FarS3LutHigh=");

    const auto schema = pipeline.GetConfigSchema();
    RequireSchemaMissing(schema, "ECStrength");
    RequireSchemaMissing(schema, "ECDim1NearSegments");
    RequireSchemaMissing(schema, "ECDim1NearS0Width");
    RequireSchemaMissing(schema, "ECDim1NearS0LutLow");
    RequireSchemaMissing(schema, "ECDim2FarS3LutHigh");

    pipeline.LoadConfig("ECEnabled", "0");
    pipeline.LoadConfig("ECStrength", "0.42");
    pipeline.LoadConfig("ECBlendRange", "0.25");
    pipeline.LoadConfig("ECDim1NearSegments", "1");
    pipeline.LoadConfig("ECDim1NearS0Width", "11");

    Require(pipeline.m_edgeComp.m_enabled, "EC enabled flag should stay hardcoded");
    RequireNear(pipeline.m_edgeComp.m_ecStrength, 1.0f, 0.0001f,
                "EC strength should stay hardcoded");
    RequireNear(pipeline.m_edgeComp.m_ecBlendRange, 0.505f, 0.0001f,
                "EC blend range should stay hardcoded");
    Require(pipeline.m_edgeComp.m_profiles[0].numSegments == 3,
            "EC segment count should stay hardcoded");
    Require(pipeline.m_edgeComp.m_profiles[0].segments[0].touchSizeThreshold == 64,
            "EC profile width should stay hardcoded");
}

void TestMasterParserOnlyKeepsRawHeatmapSemantics() {
    constexpr uint16_t kRawParserOnlyValue = 1234;
    std::vector<uint8_t> raw(static_cast<size_t>(Frame::kMasterFrameSize), 0);
    for (int i = 0; i < Frame::kMatrixCells; ++i) {
        const size_t offset = static_cast<size_t>(Frame::kMatrixOffset + i * 2);
        raw[offset] = static_cast<uint8_t>(kRawParserOnlyValue & 0xFF);
        raw[offset + 1] = static_cast<uint8_t>(kRawParserOnlyValue >> 8);
    }

    Solvers::HeatmapFrame frame;
    frame.rawPtr = raw.data();
    frame.rawLen = raw.size();
    frame.touch.output.contacts.push_back({});

    Solvers::TouchPipeline pipeline;
    pipeline.m_baseline.m_baseline = kRawParserOnlyValue;
    pipeline.ProcessMasterParserOnly(frame);

    Require(frame.heatmapMatrix[0][0] == static_cast<int16_t>(kRawParserOnlyValue),
            "master parser only should expose raw heatmap values without baseline subtraction");
    Require(frame.touch.output.contacts.empty(),
            "master parser only should still clear touch outputs");
}

void TestInvalidConfigValuesAreIgnored() {
    Solvers::TouchPipeline touch;
    touch.m_baseline.m_noFingerMaxStep = 12;

    touch.LoadConfig("BaselineNoFingerMaxStep", "abc");
    Require(touch.m_baseline.m_noFingerMaxStep == 12,
            "invalid touch integer config should preserve current value");

    Solvers::StylusPipeline stylus;
    stylus.m_hpp3.m_postPressure.m_btFreqShiftDebounceFrames = 12;
    stylus.m_hpp3.m_aftCoorProcess.m_sensorTxCount = 40;
    stylus.m_hpp3.m_aftCoorProcess.m_bypassLock = true;

    stylus.LoadConfig("sp.btFreqShiftDebounceFrames", "999999999999999999999");
    stylus.LoadConfig("sp.lockSensorTxCount", "abc");
    stylus.LoadConfig("sp.lockBypass", "maybe");

    Require(stylus.m_hpp3.m_postPressure.m_btFreqShiftDebounceFrames == 12,
            "out-of-range stylus integer config should preserve current value");
    Require(stylus.m_hpp3.m_aftCoorProcess.m_sensorTxCount == 40,
            "invalid stylus integer config should preserve current value");
    Require(stylus.m_hpp3.m_aftCoorProcess.m_bypassLock,
            "invalid stylus boolean config should preserve current value");
}

} // namespace

int main() {
    try {
        TestCurrentTouchConfigKeysAreHardcoded();
        TestBaselineFingerStateConfigRoundTrip();
        TestEdgeCompensationIsHardcoded();
        TestMasterParserOnlyKeepsRawHeatmapSemantics();
        TestInvalidConfigValuesAreIgnored();
        std::cout << "[TEST] TouchPipeline config round-trip tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
