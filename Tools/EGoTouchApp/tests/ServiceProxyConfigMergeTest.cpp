#include "ServiceProxyInternal.h"

#include <array>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

size_t CountOccurrences(const std::string& text, const std::string& needle) {
    size_t count = 0;
    size_t pos = 0;
    while ((pos = text.find(needle, pos)) != std::string::npos) {
        ++count;
        pos += needle.size();
    }
    return count;
}

void RequireMissingSubstring(const std::string& text, const char* substring) {
    if (text.find(substring) != std::string::npos) {
        throw std::runtime_error(std::string("unexpected config text: ") + substring);
    }
}

void RequirePresentSubstring(const std::string& text, const char* substring) {
    if (text.find(substring) == std::string::npos) {
        throw std::runtime_error(std::string("missing config text: ") + substring);
    }
}

void LoadTouchPipelineFromSectionText(const std::string& text,
                                  Solvers::TouchPipeline& pipeline) {
    std::istringstream in(text);
    std::string line;
    std::string section;
    while (std::getline(in, line)) {
        const std::string trimmed = App::TrimCopy(line);
        if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#') continue;
        if (trimmed.front() == '[' && trimmed.back() == ']') {
            section = App::TrimCopy(std::string_view(trimmed).substr(1, trimmed.size() - 2));
            continue;
        }

        std::string key;
        std::string value;
        if (!App::ParseIniKeyValue(trimmed, key, value)) continue;
        if (section == "TouchPipeline") {
            pipeline.LoadConfig(key, value);
        }
    }
}

void TestTrimParseAndLegacyMapping() {
    Require(App::TrimCopy("\xEF\xBB\xBF  key  \r\n") == "key", "TrimCopy should remove UTF-8 BOM and whitespace");

    std::string key;
    std::string value;
    Require(App::ParseIniKeyValue(" mode = full ", key, value), "ParseIniKeyValue should parse key/value lines");
    Require(key == "mode", "ParseIniKeyValue should trim keys");
    Require(value == "full", "ParseIniKeyValue should trim values");
    Require(!App::ParseIniKeyValue("[Service]", key, value), "ParseIniKeyValue should reject lines without '='");
    Require(App::ParseServiceBool("YES"), "ParseServiceBool should accept YES");
    Require(App::ParseServiceBool("on"), "ParseServiceBool should accept on");
    Require(App::ParseServiceBool("TRUE"), "ParseServiceBool should accept TRUE");
    Require(!App::ParseServiceBool("off"), "ParseServiceBool should reject off");

    constexpr std::array<const char*, 8> legacySections{
        "Master Frame Parser",
        "Baseline Subtraction",
        "CMF Processor",
        "Grid IIR Processor",
        "Feature Extractor (4.1/4.2)",
        "Touch Tracker (IDT)",
        "Coordinate Filter (1 Euro)",
        "TouchGestureStateMachine",
    };
    for (const char* section : legacySections) {
        Require(App::IsLegacyTouchSection(section), "expected legacy touch section to be recognized");
    }
    Require(App::MapLegacyTouchKey("Baseline Subtraction", "Enabled") == "BaselineEnabled",
            "legacy baseline Enabled should map to canonical key");
    Require(App::MapLegacyTouchKey("Grid IIR Processor", "Enabled") == "GridIIREnabled",
            "legacy Grid IIR Enabled should map to canonical key");
    Require(!App::MapLegacyTouchKey("Feature Extractor (4.1/4.2)", "Enabled").has_value(),
            "legacy feature extractor Enabled should be dropped");
}

void TestBuildServiceConfigSection() {
    const std::string section = App::BuildServiceConfigSection(false, true, false,
                                                               PenButtonMode::NativeBarrel,
                                                               PenButtonRoute::Win32Only);
    Require(section.find("[Service]\n") == 0, "service config should start with [Service]");
    RequirePresentSubstring(section, "mode=touch_only");
    RequirePresentSubstring(section, "auto_mode=1");
    RequirePresentSubstring(section, "stylus_vhf_enabled=0");
    RequirePresentSubstring(section, "pen_button_mode=");
    RequirePresentSubstring(section, "pen_button_route=");
}

void TestMasterParserOnlySnapshotRestore() {
    Solvers::TouchPipeline pipeline;
    pipeline.m_baseline.m_enabled = false;
    pipeline.m_cmf.m_enabled = true;
    pipeline.m_tracker.m_enabled = true;
    pipeline.m_coordFilter.m_enabled = false;
    pipeline.m_gesture.m_enabled = true;

    const auto snapshot = App::CaptureTouchPipelineModuleEnableState(pipeline);

    App::TouchPipelineModuleEnableState parserOnlyState = snapshot;
    parserOnlyState.baselineEnabled = false;
    parserOnlyState.cmfEnabled = false;
    parserOnlyState.gridIIREnabled = false;
    parserOnlyState.trackerEnabled = false;
    parserOnlyState.coordFilterEnabled = false;
    parserOnlyState.gestureEnabled = false;
    App::ApplyTouchPipelineModuleEnableState(pipeline, parserOnlyState);

    Require(!pipeline.m_baseline.m_enabled, "baseline should be disabled in parser-only mode");
    Require(!pipeline.m_cmf.m_enabled, "cmf should be disabled in parser-only mode");
    Require(!pipeline.m_tracker.m_enabled, "tracker should be disabled in parser-only mode");
    Require(!pipeline.m_coordFilter.m_enabled, "coord filter should be disabled in parser-only mode");
    Require(!pipeline.m_gesture.m_enabled, "gesture should be disabled in parser-only mode");

    App::ApplyTouchPipelineModuleEnableState(pipeline, snapshot);
    Require(!pipeline.m_baseline.m_enabled, "baseline should restore exact original state");
    Require(pipeline.m_cmf.m_enabled, "cmf should restore exact original state");
    Require(pipeline.m_tracker.m_enabled, "tracker should restore exact original state");
    Require(!pipeline.m_coordFilter.m_enabled, "coord filter should restore exact original state");
    Require(pipeline.m_gesture.m_enabled, "gesture should restore exact original state");
}

void TestPersistedTouchConfigSkipsFrozenKeysWhileOverlayActive() {
    Solvers::TouchPipeline pipeline;
    pipeline.m_baseline.m_enabled = true;
    pipeline.m_cmf.m_enabled = false;
    pipeline.m_tracker.m_enabled = false;
    pipeline.m_coordFilter.m_enabled = true;
    pipeline.m_gesture.m_enabled = false;
    pipeline.m_baseline.m_baseline = 123;
    pipeline.m_baseline.m_noFingerMaxStep = 900;
    pipeline.m_tracker.m_maxTrackDistance = 7.5f;
    pipeline.m_tracker.m_stylusSuppressPenPeakThreshold = 2468;
    pipeline.m_gesture.m_bypassStateMachine = true;

    const auto snapshot = App::CaptureTouchPipelineModuleEnableState(pipeline);

    App::TouchPipelineModuleEnableState parserOnlyState = snapshot;
    parserOnlyState.baselineEnabled = false;
    parserOnlyState.cmfEnabled = false;
    parserOnlyState.gridIIREnabled = false;
    parserOnlyState.trackerEnabled = false;
    parserOnlyState.coordFilterEnabled = false;
    parserOnlyState.gestureEnabled = false;
    App::ApplyTouchPipelineModuleEnableState(pipeline, parserOnlyState);

    const std::string persistedText = App::BuildTouchPipelineConfigSection(pipeline, &snapshot);
    RequirePresentSubstring(persistedText, "[TouchPipeline]\n");
    RequirePresentSubstring(persistedText, "BaselineNoFingerMaxStep=900");
    RequireMissingSubstring(persistedText, "BaselineEnabled=");
    RequireMissingSubstring(persistedText, "BaselineValue=");
    RequireMissingSubstring(persistedText, "CMFEnabled=");
    RequireMissingSubstring(persistedText, "TrackerEnabled=");
    RequireMissingSubstring(persistedText, "MaxTrackDistance=");
    RequireMissingSubstring(persistedText, "StylusSuppressPenPeakThreshold=");
    RequireMissingSubstring(persistedText, "BypassStateMachine=");
    RequireMissingSubstring(persistedText, "GridIIREnabled=");

    Solvers::TouchPipeline loaded;
    LoadTouchPipelineFromSectionText(persistedText, loaded);
    Require(loaded.m_baseline.m_noFingerMaxStep == 900,
            "non-frozen baseline no-finger max step should still persist");
}

void TestPersistedGridIIRStateIsNotInjectedWhenPipelineOmitsGridIIR() {
    Solvers::TouchPipeline pipeline;
    App::TouchPipelineModuleEnableState snapshot = App::CaptureTouchPipelineModuleEnableState(pipeline);
    snapshot.gridIIREnabled = false;

    const std::string persistedText = App::BuildTouchPipelineConfigSection(pipeline, &snapshot);
    RequireMissingSubstring(persistedText, "GridIIREnabled=");
}

void TestStylusPipelineConfigRoundTripCoversIirAndLockKeys() {
    Solvers::StylusPipeline saved;
    saved.m_hpp3.m_coorIIRProcess.m_coefLowInBand = 11;
    saved.m_hpp3.m_coorIIRProcess.m_coefHighInBand = 12;
    saved.m_hpp3.m_coorIIRProcess.m_speedTholdInBand = 13;
    saved.m_hpp3.m_coorIIRProcess.m_coefLowEdge = 14;
    saved.m_hpp3.m_coorIIRProcess.m_coefHighEdge = 15;
    saved.m_hpp3.m_coorIIRProcess.m_speedTholdEdge = 16;
    saved.m_hpp3.m_coorIIRProcess.m_speedMax = 321;
    saved.m_hpp3.m_coorIIRProcess.m_maxCoef = 17;
    saved.m_hpp3.m_aftCoorProcess.m_lockFlashInBandX = 21;
    saved.m_hpp3.m_aftCoorProcess.m_lockFlashInBandY = 22;
    saved.m_hpp3.m_aftCoorProcess.m_lockFlashEdgeX = 23;
    saved.m_hpp3.m_aftCoorProcess.m_lockFlashEdgeY = 24;

    std::ostringstream out;
    saved.SaveConfig(out);

    Solvers::StylusPipeline loaded;
    std::istringstream in(out.str());
    std::string line;
    while (std::getline(in, line)) {
        std::string key;
        std::string value;
        if (App::ParseIniKeyValue(App::TrimCopy(line), key, value)) {
            loaded.LoadConfig(key, value);
        }
    }

    Require(loaded.m_hpp3.m_coorIIRProcess.m_coefLowInBand == 11, "IIR low in-band coefficient should round-trip");
    Require(loaded.m_hpp3.m_coorIIRProcess.m_coefHighInBand == 12, "IIR high in-band coefficient should round-trip");
    Require(loaded.m_hpp3.m_coorIIRProcess.m_speedTholdInBand == 13, "IIR in-band speed threshold should round-trip");
    Require(loaded.m_hpp3.m_coorIIRProcess.m_coefLowEdge == 14, "IIR low edge coefficient should round-trip");
    Require(loaded.m_hpp3.m_coorIIRProcess.m_coefHighEdge == 15, "IIR high edge coefficient should round-trip");
    Require(loaded.m_hpp3.m_coorIIRProcess.m_speedTholdEdge == 16, "IIR edge speed threshold should round-trip");
    Require(loaded.m_hpp3.m_coorIIRProcess.m_speedMax == 321, "IIR speed max should round-trip");
    Require(loaded.m_hpp3.m_coorIIRProcess.m_maxCoef == 17, "IIR max coefficient should round-trip");
    Require(loaded.m_hpp3.m_aftCoorProcess.m_lockFlashInBandX == 21, "AFT in-band X lock flash should round-trip");
    Require(loaded.m_hpp3.m_aftCoorProcess.m_lockFlashInBandY == 22, "AFT in-band Y lock flash should round-trip");
    Require(loaded.m_hpp3.m_aftCoorProcess.m_lockFlashEdgeX == 23, "AFT edge X lock flash should round-trip");
    Require(loaded.m_hpp3.m_aftCoorProcess.m_lockFlashEdgeY == 24, "AFT edge Y lock flash should round-trip");
}

void TestMergeWithEmptyServiceSectionRemovesExistingService() {
    Solvers::TouchPipeline touchPipeline;
    Solvers::StylusPipeline stylusPipeline;
    const std::string existing =
        "[Service]\n"
        "mode=full\n"
        "\n"
        "[Unrelated]\n"
        "alpha=1\n";

    const std::string merged = App::MergeServiceProxyConfigSections(
        existing,
        {},
        App::BuildTouchPipelineConfigSection(touchPipeline),
        App::BuildStylusPipelineConfigSection(stylusPipeline));

    Require(merged.find("[Service]") == std::string::npos,
            "empty service section should remove existing client-side [Service] when preserve is not requested");
    RequirePresentSubstring(merged, "[Unrelated]\nalpha=1");
    Require(CountOccurrences(merged, "[TouchPipeline]") == 1,
            "empty service merge should still write touch pipeline section");
    Require(CountOccurrences(merged, "[StylusPipeline]") == 1,
            "empty service merge should still write stylus pipeline section");
}

void TestMergeWithEmptyServiceSectionCanPreserveExistingService() {
    Solvers::TouchPipeline touchPipeline;
    Solvers::StylusPipeline stylusPipeline;
    const std::string existing =
        "; keep header comment\n"
        "[Service]\n"
        "mode=touch_only\n"
        "auto_mode=0\n"
        "stylus_vhf_enabled=0\n"
        "pen_button_mode=2\n"
        "pen_button_route=1\n"
        "\n"
        "[Unrelated]\n"
        "alpha=1\n";

    const std::string merged = App::MergeServiceProxyConfigSections(
        existing,
        {},
        App::BuildTouchPipelineConfigSection(touchPipeline),
        App::BuildStylusPipelineConfigSection(stylusPipeline),
        true);

    Require(CountOccurrences(merged, "[Service]") == 1,
            "preserve merge should keep existing [Service] section");
    RequirePresentSubstring(merged, "mode=touch_only");
    RequirePresentSubstring(merged, "auto_mode=0");
    RequirePresentSubstring(merged, "stylus_vhf_enabled=0");
    RequirePresentSubstring(merged, "pen_button_mode=2");
    RequirePresentSubstring(merged, "pen_button_route=1");
    RequirePresentSubstring(merged, "[Unrelated]\nalpha=1");
    Require(CountOccurrences(merged, "[TouchPipeline]") == 1,
            "preserve merge should still write touch pipeline section");
    Require(CountOccurrences(merged, "[StylusPipeline]") == 1,
            "preserve merge should still write stylus pipeline section");

    const std::string withoutService = App::MergeServiceProxyConfigSections(
        "[Unrelated]\nalpha=1\n",
        {},
        App::BuildTouchPipelineConfigSection(touchPipeline),
        App::BuildStylusPipelineConfigSection(stylusPipeline),
        true);
    Require(withoutService.find("[Service]") == std::string::npos,
            "preserve merge should not invent [Service] when no existing section exists");
}

void TestMergeReplacesServiceWithCanonicalSection() {
    Solvers::TouchPipeline touchPipeline;
    Solvers::StylusPipeline stylusPipeline;
    const std::string existing =
        "[Service]\n"
        "mode=touch_only\n"
        "auto_mode=0\n"
        "stylus_vhf_enabled=0\n"
        "pen_button_mode=2\n"
        "pen_button_route=1\n"
        "\n"
        "[Unrelated]\n"
        "alpha=1\n";

    const std::string merged = App::MergeServiceProxyConfigSections(
        existing,
        App::BuildServiceConfigSection(true, true, true,
                                       PenButtonMode::OemCustom,
                                       PenButtonRoute::VhfOnly),
        App::BuildTouchPipelineConfigSection(touchPipeline),
        App::BuildStylusPipelineConfigSection(stylusPipeline),
        true);

    Require(CountOccurrences(merged, "[Service]") == 1,
            "canonical merge should keep exactly one [Service] section");
    RequirePresentSubstring(merged, "mode=full");
    RequirePresentSubstring(merged, "auto_mode=1");
    RequirePresentSubstring(merged, "stylus_vhf_enabled=1");
    RequireMissingSubstring(merged, "mode=touch_only");
    RequirePresentSubstring(merged, "[Unrelated]\nalpha=1");
}

void TestMergeDeduplicatesOwnedSections() {
    Solvers::TouchPipeline touchPipeline;
    Solvers::StylusPipeline stylusPipeline;
    const std::string existing =
        "[Service]\nmode=full\n\n"
        "[Baseline Subtraction]\nEnabled=1\n\n"
        "[Service]\nmode=touch_only\n\n"
        "[TouchPipeline]\nBaselineEnabled=0\n\n"
        "[StylusPipeline]\nsp.filterMode=1\n\n"
        "[StylusPipeline]\nsp.filterMode=2\n";

    const std::string merged = App::MergeServiceProxyConfigSections(
        existing,
        App::BuildServiceConfigSection(true, false, true),
        App::BuildTouchPipelineConfigSection(touchPipeline),
        App::BuildStylusPipelineConfigSection(stylusPipeline));

    Require(CountOccurrences(merged, "[Service]") == 1,
            "duplicate [Service] sections should collapse to one canonical section");
    Require(CountOccurrences(merged, "[TouchPipeline]") == 1,
            "legacy and canonical touch sections should collapse to one canonical section");
    Require(CountOccurrences(merged, "[StylusPipeline]") == 1,
            "duplicate [StylusPipeline] sections should collapse to one canonical section");
    RequireMissingSubstring(merged, "[Baseline Subtraction]");
}

void TestMergePreservesUnrelatedSectionsAndReplacesTouchSections() {
    Solvers::TouchPipeline touchPipeline;
    touchPipeline.m_baseline.m_enabled = false;
    touchPipeline.m_baseline.m_noFingerMaxStep = 600;
    touchPipeline.m_tracker.m_enabled = true;
    Solvers::StylusPipeline stylusPipeline;
    stylusPipeline.m_hpp3.m_postPressure.m_btFreqShiftDebounceFrames = 2;

    const std::string existing =
        "; keep header comment\n"
        "[Unrelated]\n"
        "alpha=1\n"
        "\n"
        "[Service]\n"
        "mode=touch_only\n"
        "auto_mode=0\n"
        "stylus_vhf_enabled=0\n"
        "\n"
        "[Baseline Subtraction]\n"
        "Enabled=1\n"
        "BaselineValue=99\n"
        "\n"
        "[Touch Tracker (IDT)]\n"
        "Enabled=0\n"
        "MaxTrackDistance=99\n"
        "\n"
        "[StylusPipeline]\n"
        "sp.filterMode=1\n"
        "\n"
        "[Tail]\n"
        "omega=2\n";

    const std::string merged = App::MergeServiceProxyConfigSections(
        existing,
        App::BuildServiceConfigSection(true, true, false,
                                       PenButtonMode::OemCustom,
                                       PenButtonRoute::VhfOnly),
        App::BuildTouchPipelineConfigSection(touchPipeline),
        App::BuildStylusPipelineConfigSection(stylusPipeline));

    RequirePresentSubstring(merged, "; keep header comment");
    RequirePresentSubstring(merged, "[Unrelated]\nalpha=1");
    RequirePresentSubstring(merged, "[Tail]\nomega=2");
    RequireMissingSubstring(merged, "[Baseline Subtraction]");
    RequireMissingSubstring(merged, "[Touch Tracker (IDT)]");
    Require(CountOccurrences(merged, "[Service]") == 1,
            "merge should keep exactly one service section");
    Require(CountOccurrences(merged, "[TouchPipeline]") == 1,
            "merge should keep exactly one touch pipeline section");
    Require(CountOccurrences(merged, "[StylusPipeline]") == 1,
            "merge should keep exactly one stylus pipeline section");
    RequirePresentSubstring(merged, "mode=full");
    RequirePresentSubstring(merged, "auto_mode=1");
    RequirePresentSubstring(merged, "stylus_vhf_enabled=0");
    RequireMissingSubstring(merged, "BaselineEnabled=");
    RequireMissingSubstring(merged, "BaselineValue=");
    RequireMissingSubstring(merged, "MaxTrackDistance=");
    RequirePresentSubstring(merged, "BaselineNoFingerMaxStep=600");
    RequirePresentSubstring(merged, "sp.btFreqShiftDebounceFrames=2");
}

} // namespace

int main() {
    try {
        TestTrimParseAndLegacyMapping();
        TestBuildServiceConfigSection();
        TestMasterParserOnlySnapshotRestore();
        TestPersistedTouchConfigSkipsFrozenKeysWhileOverlayActive();
        TestPersistedGridIIRStateIsNotInjectedWhenPipelineOmitsGridIIR();
        TestStylusPipelineConfigRoundTripCoversIirAndLockKeys();
        TestMergeWithEmptyServiceSectionRemovesExistingService();
        TestMergeWithEmptyServiceSectionCanPreserveExistingService();
        TestMergeReplacesServiceWithCanonicalSection();
        TestMergeDeduplicatesOwnedSections();
        TestMergePreservesUnrelatedSectionsAndReplacesTouchSections();
        std::cout << "[TEST] ServiceProxy config merge tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
