#include "TouchSolver/TouchPipeline.h"

#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void RequireNear(float actual, float expected, float epsilon, const char* message) {
    if (std::fabs(actual - expected) > epsilon) {
        throw std::runtime_error(message);
    }
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

void TestStylusSuppressRoundTrip() {
    Solvers::TouchPipeline pipeline;
    pipeline.m_tracker.m_stylusSuppressGlobalEnabled = false;
    pipeline.m_tracker.m_stylusSuppressLocalEnabled = true;
    pipeline.m_tracker.m_stylusSuppressLocalDistance = 3.75f;
    pipeline.m_tracker.m_stylusSuppressPenPeakThreshold = 2468;
    pipeline.m_tracker.m_stylusSuppressTouchSignalKeep = 6789;
    pipeline.m_tracker.m_stylusSuppressTouchAreaKeep = 17;
    pipeline.m_tracker.m_stylusAftEnabled = true;
    pipeline.m_tracker.m_stylusAftRecentFrames = 33;
    pipeline.m_tracker.m_stylusAftRadius = 4.25f;
    pipeline.m_tracker.m_stylusAftDebounceFrames = 6;
    pipeline.m_tracker.m_stylusAftWeakSignalThreshold = 321;
    pipeline.m_tracker.m_stylusAftWeakSizeThresholdMm = 1.75f;
    pipeline.m_tracker.m_stylusAftSuppressFrames = 44;
    pipeline.m_tracker.m_stylusAftPalmSuppressFrames = 88;
    pipeline.m_tracker.m_stylusAftPalmAreaThreshold = 27;
    pipeline.m_tracker.m_stylusAftPalmSizeThresholdMm = 4.5f;
    pipeline.m_tracker.m_touchDownDebounceMaxExtra = 5;
    pipeline.m_tracker.m_touchDownWeakSignalThreshold = 222;
    pipeline.m_tracker.m_touchDownSmallSizeThresholdMm = 1.55f;
    pipeline.m_tracker.m_touchDownRejectMinSizeMm = 1.05f;
    pipeline.m_tracker.m_touchDownEdgeRejectMinSignal = 111;
    pipeline.m_tracker.m_fallbackSizeMm = 1.25f;
    pipeline.m_tracker.m_sizeAreaScale = 0.31f;
    pipeline.m_tracker.m_sizeSignalScale = 0.47f;
    pipeline.m_tracker.m_rxGhostFilterEnabled = true;
    pipeline.m_tracker.m_rxGhostLineDelta = 2;
    pipeline.m_tracker.m_rxGhostWeakRatio = 0.42f;
    pipeline.m_tracker.m_rxGhostOnlyNew = false;

    std::ostringstream out;
    pipeline.SaveConfig(out);
    const std::string saved = out.str();

    Require(saved.find("StylusSuppressPenPeakThreshold=2468") != std::string::npos,
            "saved config should include stylus peak threshold");
    Require(saved.find("StylusAftWeakSignalThreshold=321") != std::string::npos,
            "saved config should include AFT weak signal threshold");
    Require(saved.find("StylusAftPalmSizeThresholdMm=4.5") != std::string::npos,
            "saved config should include AFT palm size threshold");
    Require(saved.find("TouchDownDebounceMaxExtra=5") != std::string::npos,
            "saved config should include debounce extra cap");
    Require(saved.find("RxGhostWeakRatio=0.42") != std::string::npos,
            "saved config should include rx ghost ratio");

    Solvers::TouchPipeline loaded;
    LoadFromSavedText(loaded, saved);

    Require(!loaded.m_tracker.m_stylusSuppressGlobalEnabled, "global suppress flag should round-trip");
    Require(loaded.m_tracker.m_stylusSuppressLocalEnabled, "local suppress flag should round-trip");
    RequireNear(loaded.m_tracker.m_stylusSuppressLocalDistance, 3.75f, 0.0001f,
                "local suppress distance should round-trip");
    Require(loaded.m_tracker.m_stylusSuppressPenPeakThreshold == 2468,
            "pen peak threshold should round-trip");
    Require(loaded.m_tracker.m_stylusSuppressTouchSignalKeep == 6789,
            "touch signal keep should round-trip");
    Require(loaded.m_tracker.m_stylusSuppressTouchAreaKeep == 17,
            "touch area keep should round-trip");
    Require(loaded.m_tracker.m_stylusAftEnabled, "AFT enabled should round-trip");
    Require(loaded.m_tracker.m_stylusAftRecentFrames == 33,
            "AFT recent frames should round-trip");
    RequireNear(loaded.m_tracker.m_stylusAftRadius, 4.25f, 0.0001f,
                "AFT radius should round-trip");
    Require(loaded.m_tracker.m_stylusAftDebounceFrames == 6,
            "AFT debounce frames should round-trip");
    Require(loaded.m_tracker.m_stylusAftWeakSignalThreshold == 321,
            "AFT weak signal threshold should round-trip");
    RequireNear(loaded.m_tracker.m_stylusAftWeakSizeThresholdMm, 1.75f, 0.0001f,
                "AFT weak size threshold should round-trip");
    Require(loaded.m_tracker.m_stylusAftSuppressFrames == 44,
            "AFT suppress frames should round-trip");
    Require(loaded.m_tracker.m_stylusAftPalmSuppressFrames == 88,
            "AFT palm suppress frames should round-trip");
    Require(loaded.m_tracker.m_stylusAftPalmAreaThreshold == 27,
            "AFT palm area threshold should round-trip");
    RequireNear(loaded.m_tracker.m_stylusAftPalmSizeThresholdMm, 4.5f, 0.0001f,
                "AFT palm size threshold should round-trip");
    Require(loaded.m_tracker.m_touchDownDebounceMaxExtra == 5,
            "debounce extra cap should round-trip");
    Require(loaded.m_tracker.m_touchDownWeakSignalThreshold == 222,
            "weak signal threshold should round-trip");
    RequireNear(loaded.m_tracker.m_touchDownSmallSizeThresholdMm, 1.55f, 0.0001f,
                "small size threshold should round-trip");
    RequireNear(loaded.m_tracker.m_touchDownRejectMinSizeMm, 1.05f, 0.0001f,
                "reject min size should round-trip");
    Require(loaded.m_tracker.m_touchDownEdgeRejectMinSignal == 111,
            "edge reject signal should round-trip");
    RequireNear(loaded.m_tracker.m_fallbackSizeMm, 1.25f, 0.0001f,
                "fallback size should round-trip");
    RequireNear(loaded.m_tracker.m_sizeAreaScale, 0.31f, 0.0001f,
                "size area scale should round-trip");
    RequireNear(loaded.m_tracker.m_sizeSignalScale, 0.47f, 0.0001f,
                "size signal scale should round-trip");
    Require(loaded.m_tracker.m_rxGhostFilterEnabled,
            "rx ghost filter flag should round-trip");
    Require(loaded.m_tracker.m_rxGhostLineDelta == 2,
            "rx ghost line delta should round-trip");
    RequireNear(loaded.m_tracker.m_rxGhostWeakRatio, 0.42f, 0.0001f,
                "rx ghost weak ratio should round-trip");
    Require(!loaded.m_tracker.m_rxGhostOnlyNew,
            "rx ghost only-new flag should round-trip");
}

void TestStylusSuppressSchemaContainsNewKeys() {
    Solvers::TouchPipeline pipeline;
    const auto schema = pipeline.GetConfigSchema();

    auto hasKey = [&](const char* key) {
        for (const auto& param : schema) {
            if (param.key == key) return true;
        }
        return false;
    };

    Require(hasKey("StylusSuppressPenPeakThreshold"),
            "schema should expose stylus peak threshold");
    Require(hasKey("StylusAftDebounceFrames"),
            "schema should expose AFT debounce frames");
    Require(hasKey("StylusAftWeakSignalThreshold"),
            "schema should expose AFT weak signal threshold");
    Require(hasKey("StylusAftPalmAreaThreshold"),
            "schema should expose AFT palm area threshold");
    Require(hasKey("TouchDownDebounceMaxExtra"),
            "schema should expose debounce extra cap");
    Require(hasKey("RxGhostWeakRatio"),
            "schema should expose rx ghost weak ratio");
}

} // namespace

int main() {
    try {
        TestStylusSuppressRoundTrip();
        TestStylusSuppressSchemaContainsNewKeys();
        std::cout << "[TEST] TouchPipeline config round-trip tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
