#include "TouchSolver/GridIIRProcessor.hpp"

#include <iostream>
#include <stdexcept>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void PrimeHistory(Solvers::Touch::GridIIRProcessor& processor) {
    Solvers::HeatmapFrame frame;
    processor.Process(frame);
}

void TestPeakCandidateBypassesLowSignalDecay() {
    Solvers::Touch::GridIIRProcessor processor;
    processor.m_gateStaticFloor = 200;
    processor.m_decayWeight = 200;
    processor.m_decayStep = 80;
    PrimeHistory(processor);

    Solvers::HeatmapFrame frame;
    frame.heatmapMatrix[20][20] = 180;
    processor.Process(frame, 130);

    Require(frame.heatmapMatrix[20][20] == 180,
            "peak candidate below GridIIR gate should be preserved");
}

void TestBelowPeakCandidateStillDecays() {
    Solvers::Touch::GridIIRProcessor processor;
    processor.m_gateStaticFloor = 200;
    processor.m_decayWeight = 200;
    processor.m_decayStep = 80;
    PrimeHistory(processor);

    Solvers::HeatmapFrame frame;
    frame.heatmapMatrix[20][20] = 120;
    processor.Process(frame, 130);

    Require(frame.heatmapMatrix[20][20] < 120,
            "signal below peak candidate threshold should still decay");
}

void TestDefaultProcessKeepsLegacyDecay() {
    Solvers::Touch::GridIIRProcessor processor;
    processor.m_gateStaticFloor = 200;
    processor.m_decayWeight = 200;
    processor.m_decayStep = 80;
    PrimeHistory(processor);

    Solvers::HeatmapFrame frame;
    frame.heatmapMatrix[20][20] = 180;
    processor.Process(frame);

    Require(frame.heatmapMatrix[20][20] < 180,
            "default GridIIR call should keep legacy low-signal decay");
}

} // namespace

int main() {
    try {
        TestPeakCandidateBypassesLowSignalDecay();
        TestBelowPeakCandidateStillDecays();
        TestDefaultProcessKeepsLegacyDecay();
        std::cout << "[TEST] Touch GridIIR processor tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
