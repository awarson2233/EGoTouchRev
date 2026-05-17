#include "../EGoTouchApp/source/ServiceProxyInternal.h"

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

void TestMasterParserOnlySnapshotRestore() {
    Solvers::TouchPipeline pipeline;
    pipeline.m_baseline.m_enabled = false;
    pipeline.m_cmf.m_enabled = true;
    pipeline.m_gridIIR.m_enabled = false;
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
    Require(!pipeline.m_gridIIR.m_enabled, "grid iir should be disabled in parser-only mode");
    Require(!pipeline.m_tracker.m_enabled, "tracker should be disabled in parser-only mode");
    Require(!pipeline.m_coordFilter.m_enabled, "coord filter should be disabled in parser-only mode");
    Require(!pipeline.m_gesture.m_enabled, "gesture should be disabled in parser-only mode");

    App::ApplyTouchPipelineModuleEnableState(pipeline, snapshot);
    Require(!pipeline.m_baseline.m_enabled, "baseline should restore exact original state");
    Require(pipeline.m_cmf.m_enabled, "cmf should restore exact original state");
    Require(!pipeline.m_gridIIR.m_enabled, "grid iir should restore exact original state");
    Require(pipeline.m_tracker.m_enabled, "tracker should restore exact original state");
    Require(!pipeline.m_coordFilter.m_enabled, "coord filter should restore exact original state");
    Require(pipeline.m_gesture.m_enabled, "gesture should restore exact original state");
}

void TestPersistedTouchConfigUsesSnapshotWhileOverlayActive() {
    Solvers::TouchPipeline pipeline;
    pipeline.m_baseline.m_enabled = true;
    pipeline.m_cmf.m_enabled = false;
    pipeline.m_gridIIR.m_enabled = true;
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
    Require(loaded.m_gridIIR.m_enabled, "persisted grid iir flag should come from snapshot");
    Require(!loaded.m_tracker.m_enabled, "persisted tracker flag should come from snapshot");
    Require(loaded.m_coordFilter.m_enabled, "persisted coord filter flag should come from snapshot");
    Require(!loaded.m_gesture.m_enabled, "persisted gesture flag should come from snapshot");
    Require(loaded.m_baseline.m_baseline == 123, "non-overlay touch config should still persist");
    Require(loaded.m_tracker.m_maxTrackDistance == 7.5f, "non-overlay tracker config should still persist");
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
        App::BuildServiceConfigSection(true, true, false),
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
        TestMasterParserOnlySnapshotRestore();
        TestPersistedTouchConfigUsesSnapshotWhileOverlayActive();
        TestMergePreservesUnrelatedSectionsAndReplacesTouchSections();
        std::cout << "[TEST] ServiceProxy config merge tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
