#include "StylusSolver/StylusPipeline.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void LoadFromSavedText(Solvers::StylusPipeline& pipeline, const std::string& saved) {
    std::istringstream in(saved);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        const auto eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        pipeline.LoadConfig(line.substr(0, eq), line.substr(eq + 1));
    }
}

void RequireSavedContains(const std::string& saved, const char* entry, const char* message) {
    Require(saved.find(entry) != std::string::npos, message);
}

void TestStylusPipelineRoundTripIncludesCoordinateAndPostPressure() {
    Solvers::StylusPipeline pipeline;
    pipeline.m_tiltProcess.m_enabled = false;
    pipeline.m_coordinateSolver.m_signalFloor = 77;
    pipeline.m_postPressure.m_enabled = false;
    pipeline.m_postPressure.m_fakePressureDecreaseEnabled = false;
    pipeline.m_postPressure.m_btFreqShiftDebounceFrames = 7;
    pipeline.m_postPressure.m_pressureEdgeEnterThreshold = 910;
    pipeline.m_postPressure.m_pressureEdgeExitThreshold = 456;
    pipeline.m_pressureSolver.m_btPressSignalSuppressEnterThreshold = 321;
    pipeline.m_pressureSolver.m_btPressSignalSuppressExitThreshold = 123;

    std::ostringstream out;
    pipeline.SaveConfig(out);
    const std::string saved = out.str();

    RequireSavedContains(saved,
                         "sp.tiltProcessEnabled=0",
                         "saved config should include tilt process enabled flag");
    RequireSavedContains(saved,
                         "sp.signalFloor=77",
                         "saved config should include coordinate signal floor");
    RequireSavedContains(saved,
                         "sp.postPressureEnabled=0",
                         "saved config should contain post-pressure enable flag");
    RequireSavedContains(saved,
                         "sp.fakePressureDecreaseEnabled=0",
                         "saved config should contain fake pressure flag");
    RequireSavedContains(saved,
                         "sp.btFreqShiftDebounceFrames=7",
                         "saved config should contain BT freq debounce frames");
    RequireSavedContains(saved,
                         "sp.pressureEdgeEnterThreshold=910",
                         "saved config should preserve enter threshold key");
    RequireSavedContains(saved,
                         "sp.pressureEdgeExitThreshold=456",
                         "saved config should preserve exit threshold key");
    RequireSavedContains(saved,
                         "sp.btPressSignalSuppressEnterThreshold=321",
                         "saved config should contain BT pressure suppress enter threshold");
    RequireSavedContains(saved,
                         "sp.btPressSignalSuppressExitThreshold=123",
                         "saved config should contain BT pressure suppress exit threshold");

    Solvers::StylusPipeline restored;
    LoadFromSavedText(restored, saved);

    Require(!restored.m_tiltProcess.m_enabled,
            "loaded config should restore tilt process enabled flag");
    Require(restored.m_coordinateSolver.m_signalFloor == 77,
            "loaded config should restore coordinate signal floor");
    Require(!restored.m_postPressure.m_enabled,
            "loaded config should restore post-pressure enable flag");
    Require(!restored.m_postPressure.m_fakePressureDecreaseEnabled,
            "loaded config should restore fake pressure flag");
    Require(restored.m_postPressure.m_btFreqShiftDebounceFrames == 7,
            "loaded config should restore BT freq debounce frames");
    Require(restored.m_postPressure.m_pressureEdgeEnterThreshold == 910,
            "loaded config should restore enter threshold");
    Require(restored.m_postPressure.m_pressureEdgeExitThreshold == 456,
            "loaded config should restore exit threshold");
    Require(restored.m_pressureSolver.m_btPressSignalSuppressEnterThreshold == 321,
            "loaded config should restore BT pressure suppress enter threshold");
    Require(restored.m_pressureSolver.m_btPressSignalSuppressExitThreshold == 123,
            "loaded config should restore BT pressure suppress exit threshold");
}

} // namespace

int main() {
    try {
        TestStylusPipelineRoundTripIncludesCoordinateAndPostPressure();
        std::cout << "[TEST] Stylus pipeline config round-trip tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
