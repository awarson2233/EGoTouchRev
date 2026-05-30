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
    Require(section.find("mode=touch_only") != std::string::npos, "service mode should serialize touch_only");
    Require(section.find("auto_mode=1") != std::string::npos, "service auto_mode should serialize true");
    Require(section.find("stylus_vhf_enabled=0") != std::string::npos, "service stylus setting should serialize false");
    Require(section.find("pen_button_mode=") != std::string::npos, "service pen_button_mode should serialize");
    Require(section.find("pen_button_route=") != std::string::npos, "service pen_button_route should serialize");
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

void TestPersistedTouchConfigUsesSnapshotWhileOverlayActive() {
    Solvers::TouchPipeline pipeline;
    pipeline.m_baseline.m_enabled = true;
    pipeline.m_cmf.m_enabled = false;
    pipeline.m_tracker.m_enabled = false;
    pipeline.m_coordFilter.m_enabled = true;
    pipeline.m_gesture.m_enabled = false;
    pipeline.m_baseline.m_baseline = 123;
    pipeline.m_tracker.m_maxTrackDistance = 7.5f;

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
    Solvers::TouchPipeline loaded;
    LoadTouchPipelineFromSectionText(persistedText, loaded);

    Require(loaded.m_baseline.m_enabled, "persisted baseline flag should come from snapshot");
    Require(!loaded.m_cmf.m_enabled, "persisted cmf flag should come from snapshot");
    Require(!loaded.m_tracker.m_enabled, "persisted tracker flag should come from snapshot");
    Require(loaded.m_coordFilter.m_enabled, "persisted coord filter flag should come from snapshot");
    Require(!loaded.m_gesture.m_enabled, "persisted gesture flag should come from snapshot");
    Require(loaded.m_baseline.m_baseline == 123, "non-overlay touch config should still persist");
    Require(loaded.m_tracker.m_maxTrackDistance == 7.5f, "non-overlay tracker config should still persist");
}

void TestPersistedGridIIRStateIsNotInjectedWhenPipelineOmitsGridIIR() {
    Solvers::TouchPipeline pipeline;
    App::TouchPipelineModuleEnableState snapshot = App::CaptureTouchPipelineModuleEnableState(pipeline);
    snapshot.gridIIREnabled = false;

    const std::string persistedText = App::BuildTouchPipelineConfigSection(pipeline, &snapshot);
    Require(persistedText.find("GridIIREnabled=") == std::string::npos,
            "disabled GridIIR should not be injected into current TouchPipeline config while GridIIR is not an active pipeline module");
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
            "empty service section should remove existing client-side [Service]");
    Require(merged.find("[Unrelated]\nalpha=1") != std::string::npos,
            "empty service merge should preserve unrelated sections");
    Require(CountOccurrences(merged, "[TouchPipeline]") == 1,
            "empty service merge should still write touch pipeline section");
    Require(CountOccurrences(merged, "[StylusPipeline]") == 1,
            "empty service merge should still write stylus pipeline section");
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
    Require(merged.find("[Baseline Subtraction]") == std::string::npos,
            "legacy touch section should not remain after dedupe");
}

void TestMergePreservesUnrelatedSectionsAndReplacesTouchSections() {
    Solvers::TouchPipeline touchPipeline;
    touchPipeline.m_baseline.m_enabled = false;
    touchPipeline.m_tracker.m_enabled = true;
    Solvers::StylusPipeline stylusPipeline;
    stylusPipeline.m_postPressure.m_btFreqShiftDebounceFrames = 2;

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

    Require(merged.find("; keep header comment") != std::string::npos,
            "merge should preserve top-level comments");
    Require(merged.find("[Unrelated]\nalpha=1") != std::string::npos,
            "merge should preserve unrelated sections before owned sections");
    Require(merged.find("[Tail]\nomega=2") != std::string::npos,
            "merge should preserve unrelated sections after owned sections");
    Require(merged.find("[Baseline Subtraction]") == std::string::npos,
            "merge should remove legacy touch sections");
    Require(merged.find("[Touch Tracker (IDT)]") == std::string::npos,
            "merge should remove legacy tracker sections");
    Require(CountOccurrences(merged, "[Service]") == 1,
            "merge should keep exactly one service section");
    Require(CountOccurrences(merged, "[TouchPipeline]") == 1,
            "merge should keep exactly one touch pipeline section");
    Require(CountOccurrences(merged, "[StylusPipeline]") == 1,
            "merge should keep exactly one stylus pipeline section");
    Require(merged.find("mode=full") != std::string::npos,
            "merge should replace service mode with current value");
    Require(merged.find("auto_mode=1") != std::string::npos,
            "merge should replace service auto_mode with current value");
    Require(merged.find("stylus_vhf_enabled=0") != std::string::npos,
            "merge should write service stylus setting");
    Require(merged.find("BaselineEnabled=0") != std::string::npos,
            "merge should write canonical touch pipeline content");
    Require(merged.find("sp.btFreqShiftDebounceFrames=2") != std::string::npos,
            "merge should write canonical stylus pipeline content");
}

} // namespace

int main() {
    try {
        TestTrimParseAndLegacyMapping();
        TestBuildServiceConfigSection();
        TestMasterParserOnlySnapshotRestore();
        TestPersistedTouchConfigUsesSnapshotWhileOverlayActive();
        TestPersistedGridIIRStateIsNotInjectedWhenPipelineOmitsGridIIR();
        TestMergeWithEmptyServiceSectionRemovesExistingService();
        TestMergeDeduplicatesOwnedSections();
        TestMergePreservesUnrelatedSectionsAndReplacesTouchSections();
        std::cout << "[TEST] ServiceProxy config merge tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
