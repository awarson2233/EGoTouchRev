#pragma once

#include "StylusSolver/AsaTypes.hpp"

#include <algorithm>
#include <array>
#include <cstdint>

namespace Solvers {
struct HeatmapFrame;
}

namespace Solvers::Stylus::Hpp2 {

struct LineProfile {
    static constexpr int kMaxSamples = 100;
    static constexpr int kHistorySize = 10;

    std::array<uint16_t, kMaxSamples> raw{};
    std::array<uint16_t, kMaxSamples> cmnBaseline{};
    std::array<uint16_t, kMaxSamples> cmnSubtracted{};
    std::array<uint16_t, kMaxSamples> chargerNoiseRatio{};
    std::array<uint32_t, kHistorySize> lineSumHistory{};
};

static constexpr int kMaxSamples = LineProfile::kMaxSamples;
static constexpr int kHistorySize = LineProfile::kHistorySize;
static constexpr int kNumRawHistoryFrames = 10;
static constexpr uint16_t kFreqF1 = 0x00b0;
static constexpr uint16_t kFreqF2 = 0x00fc;
static constexpr int kInvalidPeak = 0xff;
static constexpr int kMaxPeaksPerDim = 4;

struct PeakUnit {
    int index = -1;
    int leftBoundary = -1;
    int rightBoundary = -1;
    uint16_t peakSignal = 0;
    uint16_t netSignal = 0;
    uint32_t signalRegionSum = 0;
    int threeNeighborSum = 0;
    uint16_t avgBaseline = 0;
    int width = 0;
    int candidateCoor = 0;
    int age = 0;
    int noiseProp = 0;
    uint16_t rankScore = 0;
    bool onEdge = false;
    bool valid = false;
};

struct Peak {
    int index = -1;
    uint16_t signal = 0;
    bool valid = false;
};

using PeakTable = std::array<PeakUnit, kMaxPeaksPerDim>;

struct Settings {
    bool enabled = true;
    int sensorTxCount = 60;
    int sensorRxCount = 40;
    int cmfWindowRadius = 6;
    uint32_t rawAbnormalLineSumThreshold = 30000;
    uint16_t rawAbnormalEnergyRatioThreshold = 200;
    uint32_t cmnAbnormalSumThreshold = 9000;
    uint16_t cmnAbnormalMinThreshold = 0x09c4;
    uint16_t chargerNoiseClearFloor = 20;
    uint16_t chargerNoiseRatioThreshold = 299;
    uint32_t chargerNoiseSumThreshold = 400;
    uint16_t chargerNoiseMaxSampleThreshold = 200;
    uint8_t chargerNoiseAbnormalChannelThreshold = 2;
    uint16_t chargerNoisePeakProtectRadius = 2;
    uint16_t chargerNoiseMinRawSample = 50;
    uint16_t peakSignalFloor = 250;
    int peakSearchNeighborDist = 2;
    int peakMinWidth = 2;
    int peakMaxWidth = 20;
    uint16_t pressureEdgeEnterThreshold = 1500;
    uint16_t pressureEdgeExitThreshold = 3000;
    uint16_t pressureDeltaNormal = 0x400;
    uint16_t pressureDeltaTight = 0x40;
    bool useTightPressureDelta = false;

    int SampleCount() const {
        return std::clamp(sensorTxCount + sensorRxCount, 0, kMaxSamples);
    }
};

struct Runtime : Asa::Runtime {
    LineProfile line{};
    uint32_t rawLineSum = 0;
    uint16_t mainFreq = 0;
    uint16_t auxFreq = 0;
    uint16_t rawPressure = 0;
    uint32_t buttonBits = 0;
    uint16_t energyRatioPrev = 100;
    uint16_t energyRatioPrev2 = 100;
    uint16_t energyRatioF1Prev2 = 100;
    uint16_t energyRatioF2Prev2 = 100;
    bool rawAbnormal = false;
    bool cmnAbnormal = false;
    bool bypassCurFrame = false;
    uint8_t selectedPeakDim1 = 0xff;
    uint8_t selectedPeakDim2 = 0xff;
    bool buttonPressed = false;
    uint8_t buttonReleaseFrames = 0;

    void ResetFrameFlags() {
        Asa::Runtime::ResetFrameFlags();
        line = {};
        rawLineSum = 0;
        mainFreq = 0;
        auxFreq = 0;
        rawPressure = 0;
        buttonBits = 0;
        energyRatioPrev = 100;
        energyRatioPrev2 = 100;
        energyRatioF1Prev2 = 100;
        energyRatioF2Prev2 = 100;
        rawAbnormal = false;
        cmnAbnormal = false;
        bypassCurFrame = false;
        selectedPeakDim1 = 0xff;
        selectedPeakDim2 = 0xff;
        buttonPressed = false;
        buttonReleaseFrames = 0;
    }
};

struct State {
    std::array<uint32_t, kHistorySize> m_lineSumHistory{};
    std::array<std::array<std::array<uint16_t, kMaxSamples>, kNumRawHistoryFrames>, 2> m_rawHistory{}; // [freqIdx][frame][sample]
    std::array<uint32_t, 2> m_noiseSum{};
    std::array<uint8_t, 2> m_noiseFlag{};
    std::array<std::array<uint32_t, kNumRawHistoryFrames>, 2> m_noiseSumHistory{};
    std::array<std::array<uint8_t, kNumRawHistoryFrames>, 2> m_noiseFlagHistory{};
    uint8_t m_curFreqIdx = 0;
    int m_f1FrameCnt = 0;
    int m_f2FrameCnt = 0;
    uint16_t m_energyRatioF1 = 100;
    uint16_t m_energyRatioF2 = 100;
    std::array<uint32_t, 2> m_cmnSum{};
    std::array<uint16_t, 2> m_cmnMin{{0xffff, 0xffff}};
    uint16_t m_prevPressure = 0;
    bool m_edgeSignalTooLowLatched = false;
    uint8_t m_buttonReleaseCnt = 0;
    bool m_wasInRange = false;
    bool m_freqNoiseLatchF1 = false;
    bool m_freqNoiseLatchF2 = false;
    std::array<uint8_t, 2> m_prevPeakDim1ByFreq{{kInvalidPeak, kInvalidPeak}};
    std::array<uint8_t, 2> m_prevPeakDim2ByFreq{{kInvalidPeak, kInvalidPeak}};
    PeakTable m_peakTableDim1{};
    PeakTable m_peakTableDim2{};
    int m_peakCountDim1 = 0;
    int m_peakCountDim2 = 0;
    int m_bypassCounter = 0;
    bool m_prevBypassed = false;

    void ResetOnTerminal() {
        m_lineSumHistory.fill(0);
        m_prevPressure = 0;
        m_edgeSignalTooLowLatched = false;
        m_buttonReleaseCnt = 0;
        m_wasInRange = false;
        m_freqNoiseLatchF1 = false;
        m_freqNoiseLatchF2 = false;
        m_rawHistory = {};
        m_noiseSum.fill(0);
        m_noiseFlag.fill(0);
        m_noiseSumHistory = {};
        m_noiseFlagHistory = {};
        m_curFreqIdx = 0;
        m_f1FrameCnt = 0;
        m_f2FrameCnt = 0;
        m_energyRatioF1 = 100;
        m_energyRatioF2 = 100;
        m_prevPeakDim1ByFreq.fill(kInvalidPeak);
        m_prevPeakDim2ByFreq.fill(kInvalidPeak);
        m_peakTableDim1 = {};
        m_peakTableDim2 = {};
        m_peakCountDim1 = 0;
        m_peakCountDim2 = 0;
        m_bypassCounter = 0;
        m_prevBypassed = false;
        m_cmnSum.fill(0);
        m_cmnMin.fill(0xffff);
    }
};

struct Context {
    HeatmapFrame& frame;
    Runtime& runtime;
    const Settings& settings;
    State& state;
};

} // namespace Solvers::Stylus::Hpp2
