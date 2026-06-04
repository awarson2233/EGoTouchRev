#include "StylusSolver/StylusPipeline.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

using Solvers::HeatmapFrame;

void Require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

HeatmapFrame MakeHpp2Frame(int dim1Peak,
                           int dim2Peak,
                           uint16_t dim1PeakValue,
                           uint16_t dim2PeakValue,
                           uint16_t pressure,
                           uint32_t buttonBits = 0) {
    HeatmapFrame frame{};
    auto& input = frame.stylus.input;
    input.auxStatusFlags = 0x1; // HPP2 protocol, not HPP3
    input.mainFreq = 0x00b0;
    input.auxFreq = 0x00fc;
    input.framePressure = pressure;
    input.buttonBits = buttonBits;
    input.hpp2LineValid = true;
    input.hpp2LineData.fill(10);
    input.hpp2LineData[static_cast<std::size_t>(dim1Peak)] = dim1PeakValue;
    input.hpp2LineData[static_cast<std::size_t>(60 + dim2Peak)] = dim2PeakValue;
    return frame;
}

void TestHpp2FrameProducesReport() {
    Solvers::StylusPipeline pipeline;
    HeatmapFrame frame = MakeHpp2Frame(12, 7, 2600, 2400, 512, 1);

    pipeline.Process(frame);

    Require(!frame.stylus.runtime.Active().flow.terminal, "HPP2 frame should run non-terminal");
    Require(frame.stylus.output.valid, "HPP2 output should be valid");
    Require(frame.stylus.output.inRange, "HPP2 output should be in range");
    Require(frame.stylus.output.tipDown, "HPP2 pressure should produce tip-down");
    Require(frame.stylus.output.pressure == 512, "HPP2 pressure should publish frame pressure");
    Require(frame.stylus.runtime.hpp2.buttonPressed, "HPP2 button bit should be debounced as pressed");
    Require(frame.stylus.output.point.x > 0.0f && frame.stylus.output.point.y > 0.0f,
            "HPP2 coordinate should be populated");
}

void TestHpp2EdgePressureGuardSuppressesPressure() {
    Solvers::StylusPipeline pipeline;
    HeatmapFrame frame = MakeHpp2Frame(0, 7, 1000, 2400, 512);

    pipeline.Process(frame);

    Require(frame.stylus.output.valid, "edge-guarded HPP2 output should still have coordinate");
    Require(frame.stylus.output.inRange, "edge-guarded HPP2 output should remain in range");
    Require(!frame.stylus.output.tipDown, "low edge signal should clear tip-down");
    Require(frame.stylus.output.pressure == 0, "low edge signal should clear pressure");
}

void TestHpp2AbnormalRawRejects() {
    Solvers::StylusPipeline pipeline;

    // Frame 1: warmup to populate line sum history.
    // Without history, energyRatioPrev returns 100 (div-by-zero guard),
    // which is below the 200 threshold required for rawAbnormal.
    HeatmapFrame warmup = MakeHpp2Frame(12, 7, 2600, 2400, 512);
    pipeline.Process(warmup);

    // Frame 2: fill all 100 samples with 1000.
    // rawLineSum = 100 * 1000 = 100000 > 30000 threshold.
    // energyRatioPrev = 100000 * 100 / warmupLineSum ≈ much > 200.
    HeatmapFrame frame = MakeHpp2Frame(12, 7, 2600, 2400, 512);
    frame.stylus.input.hpp2LineData.fill(1000);

    pipeline.Process(frame);

    Require(frame.stylus.runtime.Active().flow.terminal, "abnormal raw line sum should reject the frame");
    Require(frame.stylus.runtime.hpp2.rawAbnormal, "rawAbnormal flag should be set");
}

void TestHpp2NoPeakRejects() {
    Solvers::StylusPipeline pipeline;
    // Peak values of 10 are well below the 250 signal floor
    HeatmapFrame frame = MakeHpp2Frame(12, 7, 10, 10, 512);

    pipeline.Process(frame);

    Require(frame.stylus.runtime.Active().flow.terminal, "sub-floor peak signal should reject the frame");
}

void TestHpp2ButtonReleaseCounterDecrements() {
    Solvers::StylusPipeline pipeline;

    // Frame 1: button bit set
    HeatmapFrame f1 = MakeHpp2Frame(12, 7, 2600, 2400, 512, 1u);
    pipeline.Process(f1);
    Require(f1.stylus.runtime.hpp2.buttonPressed, "frame 1 button should be pressed");
    Require(f1.stylus.runtime.hpp2.buttonReleaseFrames == 2, "release counter should start at 2");

    // Frame 2: button bit clear, release counter still active
    HeatmapFrame f2 = MakeHpp2Frame(12, 7, 2600, 2400, 512, 0u);
    pipeline.Process(f2);
    Require(f2.stylus.runtime.hpp2.buttonPressed, "frame 2 button should still be held by release counter");
    Require(f2.stylus.runtime.hpp2.buttonReleaseFrames == 1, "release counter should decrement to 1");

    // Frame 3: button bit still clear, counter decrements to 0
    HeatmapFrame f3 = MakeHpp2Frame(12, 7, 2600, 2400, 512, 0u);
    pipeline.Process(f3);
    Require(f3.stylus.runtime.hpp2.buttonPressed, "frame 3 button should still be held");
    Require(f3.stylus.runtime.hpp2.buttonReleaseFrames == 0, "release counter should reach 0");

    // Frame 4: counter exhausted, button released
    HeatmapFrame f4 = MakeHpp2Frame(12, 7, 2600, 2400, 512, 0u);
    pipeline.Process(f4);
    Require(!f4.stylus.runtime.hpp2.buttonPressed, "frame 4 button should be released");
}

} // namespace

int main() {
    try {
        TestHpp2FrameProducesReport();
        TestHpp2EdgePressureGuardSuppressesPressure();
        TestHpp2AbnormalRawRejects();
        TestHpp2NoPeakRejects();
        TestHpp2ButtonReleaseCounterDecrements();
    } catch (const std::exception& ex) {
        std::cerr << "[FAIL] StylusHpp2PipelineTest: " << ex.what() << "\n";
        return 1;
    }
    std::cout << "[PASS] StylusHpp2PipelineTest\n";
    return 0;
}
