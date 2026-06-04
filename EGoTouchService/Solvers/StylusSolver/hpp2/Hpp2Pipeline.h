#pragma once

#include "SolverTypes.h"
#include "StylusSolver/AsaTypes.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace Solvers::Stylus::Hpp2 {

// HPP2 Pipeline — line-mode stylus data processing.
//
// Mirrors the HPP2_DispatchDataProcess common path observed in TSACore:
// staging -> CMF -> data quality/history -> peak/noise/status ->
// pressure/post-pressure/button/static-status/coordinate.  The four TSACore
// data-type wrappers are opcode-identical in the analyzed build, so this class
// keeps one common implementation and preserves data type as an extension hook.
class Pipeline {
public:
    enum DataType : uint8_t {
        Line = 0,
        IQLine = 1,
        Grid = 2,
        TiedGrid = 3,
    };

    bool m_enabled = true;
    DataType m_dataType = Line;

    int m_sensorTxCount = 60;
    int m_sensorRxCount = 40;
    int m_cmfWindowRadius = 6;
    uint32_t m_rawAbnormalLineSumThreshold = 30000;
    uint16_t m_rawAbnormalEnergyRatioThreshold = 200;
    uint32_t m_cmnAbnormalSumThreshold = 9000;        // TSAPrmt: HPP2 CMN abnormal sum gate.
    uint16_t m_cmnAbnormalMinThreshold = 0x09c4;      // TSAPrmt: HPP2 CMN minimum channel gate.
    uint16_t m_chargerNoiseClearFloor = 20;           // TSAPrmt: ChargerNoiseJudge clear-frame denominator floor.
    uint16_t m_chargerNoiseRatioThreshold = 299;      // TSAPrmt: ChargerNoiseJudge ratio must be > 299.
    uint32_t m_chargerNoiseSumThreshold = 400;        // TSAPrmt: ChargerNoiseJudge accumulated noise sum must be > 400.
    uint16_t m_chargerNoiseMaxSampleThreshold = 200;  // TSAPrmt: ChargerNoiseJudge max noise sample must be > 200.
    uint8_t m_chargerNoiseAbnormalChannelThreshold = 2; // TSAPrmt: ChargerNoiseJudge abnormal count must be > 2.
    uint16_t m_chargerNoisePeakProtectRadius = 2;     // TSAPrmt: IndexValidation peak-neighbor exclusion radius.
    uint16_t m_chargerNoiseMinRawSample = 50;         // TSAPrmt: IndexValidation ignores raw samples below 0x32.
    uint16_t m_peakSignalFloor = 250;                 // TSACore SearchPeak: g_asaStatic.field_0x4a local peak floor.
    int m_peakSearchNeighborDist = 2;                 // TSACore SearchPeak: +/-2 local-neighbor peak check.
    int m_peakMinWidth = 2;                           // TSAPrmt/SearchPeakBoundary: minimum accepted peak width.
    int m_peakMaxWidth = 20;                          // TSAPrmt/SearchPeakBoundary: maximum reasonable peak width.
    uint16_t m_pressureEdgeEnterThreshold = 1500;
    uint16_t m_pressureEdgeExitThreshold = 3000;
    uint16_t m_pressureDeltaNormal = 0x400;
    uint16_t m_pressureDeltaTight = 0x40;
    bool m_useTightPressureDelta = false;

    bool Process(HeatmapFrame& frame) {
        auto& runtime = frame.stylus.runtime;
        runtime.flow.pipelineStage = 2;
        runtime.flow.frameClass = Asa::StylusFrameClass::Valid;

        if (!m_enabled || !frame.stylus.input.hpp2LineValid) {
            runtime.flow.terminal = true;
            runtime.flow.frameClass = Asa::StylusFrameClass::NoSignal;
            m_wasInRange = false;
            return false;
        }

        StageInput(frame);
        DispatchDataProcess(frame);

        if (runtime.hpp2.bypassCurFrame || runtime.hpp2.rawAbnormal || runtime.hpp2.cmnAbnormal) {
            runtime.flow.terminal = true;
            runtime.post.freqBypassed = runtime.hpp2.bypassCurFrame;
            return false;
        }

        if (!UpdateInRangeStatus(frame)) {
            runtime.flow.terminal = true;
            return false;
        }

        FilterPressure(frame);
        ApplyEdgePressureGuard(frame);
        ProcessButton(frame);
        UpdateStaticStatusAfterPressure(frame);

        if (!UpdateCoordinates(frame)) {
            runtime.flow.terminal = true;
            runtime.flow.frameClass = Asa::StylusFrameClass::Tx1Missing;
            return false;
        }

        PublishPressure(frame);
        runtime.post.confidence = 1.0f;
        runtime.flow.terminal = false;
        return true;
    }

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
        m_cmnSum.fill(0);
        m_cmnMin.fill(0xffff);
    }

private:
    static constexpr int kMaxSamples = StylusRuntimeHpp2LineProfile::kMaxSamples;
    static constexpr int kHistorySize = StylusRuntimeHpp2LineProfile::kHistorySize;
    static constexpr int kNumRawHistoryFrames = 10;
    static constexpr uint16_t kFreqF1 = 0x00b0;
    static constexpr uint16_t kFreqF2 = 0x00fc;
    static constexpr int kInvalidPeak = 0xff;

    struct Hpp2PeakUnit {
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
        bool onEdge = false;
        bool valid = false;
    };
    static constexpr int kMaxPeaksPerDim = 4;

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
    std::array<Hpp2PeakUnit, kMaxPeaksPerDim> m_peakTableDim1{};
    std::array<Hpp2PeakUnit, kMaxPeaksPerDim> m_peakTableDim2{};
    int m_peakCountDim1 = 0;
    int m_peakCountDim2 = 0;

    static constexpr std::array<double, 4> kPitchCompDim1{{
        0.0, -1.7109151490662926, 0.005959771652221362, -5.113555667385272e-06}};
    static constexpr std::array<double, 4> kPitchCompDim2{{
        0.0, -1.4495726495726495, 0.004745726495726496, -3.7393162393162394e-06}};
    static constexpr std::array<double, Asa::kMaxSensorDim + 1> kDim1PitchTable{{
        0, 0.984375, 1.96875, 2.953125, 3.9375, 4.921875, 5.90625, 6.890625, 7.875,
        8.859375, 9.84375, 10.8515625, 11.859375, 12.8671875, 13.875, 14.8828125,
        15.890625, 16.8984375, 17.90625, 18.9140625, 19.921875, 20.9296875, 21.9375,
        22.9453125, 23.953125, 24.9609375, 25.96875, 26.9765625, 27.984375, 28.9921875,
        30, 31.0078125, 32.015625, 33.0234375, 34.03125, 35.0390625, 36.046875,
        37.0546875, 38.0625, 39.0703125, 40.078125, 41.0859375, 42.09375, 43.1015625,
        44.109375, 45.1171875, 46.125, 47.1328125, 48.140625, 49.1484375, 50.15625,
        51.140625, 52.125, 53.109375, 54.09375, 55.078125, 56.0625, 57.046875,
        58.03125, 59.015625, 60, 60, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 100}};
    static constexpr std::array<double, Asa::kMaxSensorDim + 1> kDim2PitchTable{{100.0}};

    void StageInput(HeatmapFrame& frame) {
        auto& input = frame.stylus.input;
        auto& hpp2 = frame.stylus.runtime.hpp2;
        hpp2 = {};
        hpp2.mainFreq = input.mainFreq;
        hpp2.auxFreq = input.auxFreq;
        hpp2.rawPressure = input.framePressure;
        hpp2.buttonBits = input.buttonBits;
        m_curFreqIdx = (hpp2.mainFreq == kFreqF1) ? 0 : 1;

        const int count = SampleCount();
        uint32_t sum = 0;
        for (int i = 0; i < count; ++i) {
            const uint16_t sample = input.hpp2LineData[static_cast<std::size_t>(i)];
            hpp2.line.raw[static_cast<std::size_t>(i)] = sample;
            hpp2.line.cmnSubtracted[static_cast<std::size_t>(i)] = sample;
            sum += sample;
        }
        hpp2.rawLineSum = sum;

        for (int i = kHistorySize - 1; i > 0; --i) {
            m_lineSumHistory[static_cast<std::size_t>(i)] = m_lineSumHistory[static_cast<std::size_t>(i - 1)];
        }
        m_lineSumHistory[0] = sum;
        hpp2.line.lineSumHistory = m_lineSumHistory;
        hpp2.energyRatioPrev = RatioToHistory(sum, 1);  // Mirrors pPeakFlagMap[0xdf0].
        hpp2.energyRatioPrev2 = RatioToHistory(sum, 2);
        if (m_curFreqIdx == 0) {
            m_energyRatioF1 = hpp2.energyRatioPrev2;     // Mirrors pPeakFlagMap[0xdf2].
        } else {
            m_energyRatioF2 = hpp2.energyRatioPrev2;     // Mirrors pPeakFlagMap[0xdf4].
        }
        hpp2.energyRatioF1Prev2 = m_energyRatioF1;
        hpp2.energyRatioF2Prev2 = m_energyRatioF2;
    }

    void DispatchDataProcess(HeatmapFrame& frame) {
        (void)m_dataType; // TSACore wrappers are identical in the analyzed build.
        CmfProcess(frame);
        DataQualityProcess(frame);
        LinePeaksProcess(frame);
        NoiseProcess(frame);
    }

    void CmfProcess(HeatmapFrame& frame) {
        m_cmnSum = {};
        m_cmnMin = {{0xffff, 0xffff}};
        ProcessCmfGroup(frame, 0, 0, m_sensorTxCount);
        ProcessCmfGroup(frame, 1, m_sensorTxCount, m_sensorRxCount);
    }

    void DataQualityProcess(HeatmapFrame& frame) {
        auto& hpp2 = frame.stylus.runtime.hpp2;
        hpp2.rawAbnormal =
            hpp2.rawLineSum > m_rawAbnormalLineSumThreshold &&
            hpp2.energyRatioPrev > m_rawAbnormalEnergyRatioThreshold;

        hpp2.cmnAbnormal =
            (m_cmnSum[0] + m_cmnSum[1]) > m_cmnAbnormalSumThreshold &&
            (m_cmnMin[0] < m_cmnAbnormalMinThreshold || m_cmnMin[1] < m_cmnAbnormalMinThreshold);

        const std::size_t freqIdx = static_cast<std::size_t>(m_curFreqIdx);
        m_noiseSum[freqIdx] = 0;
        m_noiseFlag[freqIdx] = 0;
        ChargerNoiseJudge(frame);
        RotateRawHistory(hpp2.line.raw, SampleCount());

        const bool noisy = hpp2.rawAbnormal || hpp2.cmnAbnormal || m_noiseFlag[freqIdx] != 0;
        if (hpp2.mainFreq == kFreqF1) {
            m_freqNoiseLatchF1 = noisy;
        } else {
            m_freqNoiseLatchF2 = noisy;
        }
    }

    void LinePeaksProcess(HeatmapFrame& frame) {
        auto& runtime = frame.stylus.runtime;
        auto& hpp2 = runtime.hpp2;
        SearchPeak(0, hpp2.line.cmnSubtracted, 0, m_sensorTxCount, m_peakTableDim1, m_peakCountDim1);
        SearchPeak(1, hpp2.line.cmnSubtracted, m_sensorTxCount, m_sensorRxCount, m_peakTableDim2, m_peakCountDim2);

        const Hpp2PeakUnit* dim1 = SelectStrongestPeak(m_peakTableDim1, m_peakCountDim1);
        const Hpp2PeakUnit* dim2 = SelectStrongestPeak(m_peakTableDim2, m_peakCountDim2);

        hpp2.selectedPeakDim1 = dim1 != nullptr ? static_cast<uint8_t>(dim1->index) : kInvalidPeak;
        hpp2.selectedPeakDim2 = dim2 != nullptr ? static_cast<uint8_t>(dim2->index) : kInvalidPeak;
        const std::size_t freqIdx = static_cast<std::size_t>(m_curFreqIdx);
        m_prevPeakDim1ByFreq[freqIdx] = hpp2.selectedPeakDim1;
        m_prevPeakDim2ByFreq[freqIdx] = hpp2.selectedPeakDim2;

        runtime.signal.signalX = dim1 != nullptr ? dim1->peakSignal : 0;
        runtime.signal.signalY = dim2 != nullptr ? dim2->peakSignal : 0;
        runtime.signal.maxRawPeak = std::max(runtime.signal.signalX, runtime.signal.signalY);
        runtime.signal.recheckEnabled = true;
        runtime.signal.recheckThreshold = m_peakSignalFloor;
        runtime.signal.recheckThresholdMulti = static_cast<uint16_t>(std::max<int>(m_peakSignalFloor, 256));
        runtime.signal.recheckPassed = dim1 != nullptr && dim2 != nullptr;
        runtime.signal.dim1EdgeActive = dim1 != nullptr && dim1->onEdge;
        runtime.signal.dim2EdgeActive = dim2 != nullptr && dim2->onEdge;
        runtime.signal.dim1EdgeSignal = runtime.signal.dim1EdgeActive ? dim1->netSignal : 0;
        runtime.signal.dim2EdgeSignal = runtime.signal.dim2EdgeActive ? dim2->netSignal : 0;

        runtime.tx1.feature.peak.valid = dim1 != nullptr && dim2 != nullptr;
        runtime.tx1.feature.peak.peakValue = runtime.signal.maxRawPeak;
        runtime.tx1.feature.peak.peakCol = dim1 != nullptr ? dim1->index : -1;
        runtime.tx1.feature.peak.peakRow = dim2 != nullptr ? dim2->index : -1;
        // SelectedPeakNetSignal exports the chosen slot's integrated net signal;
        // the slot itself is still chosen by peakSignal to match TSACore's table
        // strongest comparison on pPeakTable[slot+0x3e].
        runtime.tx1.feature.dim1SelectedPeakNetSignal = dim1 != nullptr ? dim1->netSignal : 0;
        runtime.tx1.feature.dim2SelectedPeakNetSignal = dim2 != nullptr ? dim2->netSignal : 0;
        runtime.tx1.feature.dim1SelectedPeakOnEdge = runtime.signal.dim1EdgeActive;
        runtime.tx1.feature.dim2SelectedPeakOnEdge = runtime.signal.dim2EdgeActive;
    }

    void NoiseProcess(HeatmapFrame& frame) {
        auto& hpp2 = frame.stylus.runtime.hpp2;
        const std::size_t freqIdx = static_cast<std::size_t>(m_curFreqIdx);
        const bool chargerNoise = m_noiseFlag[freqIdx] != 0;
        hpp2.bypassCurFrame = chargerNoise ||
            (hpp2.mainFreq == kFreqF1 ? m_freqNoiseLatchF1 : m_freqNoiseLatchF2);
    }

    bool UpdateInRangeStatus(HeatmapFrame& frame) {
        auto& runtime = frame.stylus.runtime;
        const auto& hpp2 = runtime.hpp2;
        const bool inRange = hpp2.selectedPeakDim1 != kInvalidPeak && hpp2.selectedPeakDim2 != kInvalidPeak;
        runtime.decision.inRangeCandidate = inRange;
        runtime.post.finalValid = inRange;
        if (!inRange) {
            m_wasInRange = false;
            return false;
        }
        m_wasInRange = true;
        return true;
    }

    void FilterPressure(HeatmapFrame& frame) {
        auto& runtime = frame.stylus.runtime;
        auto& pressure = runtime.pressure;
        const uint16_t raw = static_cast<uint16_t>(std::min<uint32_t>(runtime.hpp2.rawPressure, 0x0fffu));
        uint16_t output = raw;
        if (output != 0 && m_prevPressure != 0) {
            output = LimitPressureDelta(m_prevPressure, output,
                                        m_useTightPressureDelta ? m_pressureDeltaTight : m_pressureDeltaNormal);
            output = PressureIir(m_prevPressure, output, 0x40);
        }

        pressure.pressureIsReal = true;
        pressure.rawPressure = raw;
        pressure.mappedPressure = raw;
        pressure.outputPressure = output;
        pressure.lookaheadHoverGate = false;
        pressure.predictedAgeFrames = 0;
    }

    void ApplyEdgePressureGuard(HeatmapFrame& frame) {
        auto& runtime = frame.stylus.runtime;
        auto& pressure = runtime.pressure;
        if (pressure.outputPressure == 0) {
            m_edgeSignalTooLowLatched = false;
            return;
        }

        const bool dim1Enter = runtime.signal.dim1EdgeActive &&
            runtime.signal.dim1EdgeSignal < m_pressureEdgeEnterThreshold;
        const bool dim2Enter = runtime.signal.dim2EdgeActive &&
            runtime.signal.dim2EdgeSignal < m_pressureEdgeEnterThreshold;
        if (!m_edgeSignalTooLowLatched && (dim1Enter || dim2Enter)) {
            m_edgeSignalTooLowLatched = true;
        }

        const bool dim1Clear = !runtime.signal.dim1EdgeActive ||
            runtime.signal.dim1EdgeSignal > m_pressureEdgeExitThreshold;
        const bool dim2Clear = !runtime.signal.dim2EdgeActive ||
            runtime.signal.dim2EdgeSignal > m_pressureEdgeExitThreshold;
        if (m_edgeSignalTooLowLatched && dim1Clear && dim2Clear) {
            m_edgeSignalTooLowLatched = false;
        }

        if (m_edgeSignalTooLowLatched) {
            pressure.outputPressure = 0;
        }
#if EGOTOUCH_DIAG
        pressure.edgeSignalTooLowLatched = m_edgeSignalTooLowLatched;
#endif
    }

    void ProcessButton(HeatmapFrame& frame) {
        auto& hpp2 = frame.stylus.runtime.hpp2;
        if ((hpp2.buttonBits & 1u) != 0) {
            hpp2.buttonPressed = true;
            m_buttonReleaseCnt = 2;
        } else if (m_buttonReleaseCnt != 0) {
            hpp2.buttonPressed = true;
            --m_buttonReleaseCnt;
        } else {
            hpp2.buttonPressed = false;
        }
        hpp2.buttonReleaseFrames = m_buttonReleaseCnt;
    }

    void UpdateStaticStatusAfterPressure(HeatmapFrame& frame) {
        auto& runtime = frame.stylus.runtime;
        runtime.decision.tipDownCandidate =
            runtime.decision.inRangeCandidate && runtime.pressure.outputPressure != 0;
        runtime.decision.authoritativeDown = runtime.decision.tipDownCandidate;
        PublishPressure(frame);
        m_prevPressure = runtime.pressure.outputPressure;
    }

    bool UpdateCoordinates(HeatmapFrame& frame) const {
        auto& runtime = frame.stylus.runtime;
        const auto& hpp2 = runtime.hpp2;
        if (hpp2.selectedPeakDim1 == kInvalidPeak || hpp2.selectedPeakDim2 == kInvalidPeak) {
            return false;
        }

        int32_t dim1 = SolveByTriangle(hpp2.line.cmnSubtracted, 0, m_sensorTxCount,
                                       hpp2.selectedPeakDim1, 50, 5000, 5000);
        int32_t dim2 = SolveByTriangle(hpp2.line.cmnSubtracted, m_sensorTxCount, m_sensorRxCount,
                                       hpp2.selectedPeakDim2, 50, 4500, 3700);
        if (dim1 < 0 || dim2 < 0) {
            return false;
        }

        dim1 = ApplyPitchCompensation(dim1, kPitchCompDim1);
        dim2 = ApplyPitchCompensation(dim2, kPitchCompDim2);
        dim1 = Asa::SensorPitchSizeMap(dim1, kDim1PitchTable.data(), Asa::kCoorUnit);
        dim2 = Asa::SensorPitchSizeMap(dim2, kDim2PitchTable.data(), Asa::kCoorUnit);

        Asa::AsaCoorResult coor{};
        coor.valid = true;
        coor.dim1 = std::clamp(dim1, 0, m_sensorTxCount * Asa::kCoorUnit - 1);
        coor.dim2 = std::clamp(dim2, 0, m_sensorRxCount * Asa::kCoorUnit - 1);
        runtime.tx1.coordinate.localGridCoor = coor;
        runtime.tx1.coordinate.reportGlobalCoor = coor;
        runtime.post.finalCoor = coor;
        runtime.post.finalValid = true;
        return true;
    }

    void PublishPressure(HeatmapFrame& frame) const {
        auto& runtime = frame.stylus.runtime;
        runtime.post.finalPressure = runtime.pressure.outputPressure;
        runtime.post.point.rawPressure = runtime.pressure.rawPressure;
        runtime.post.point.mappedPressure = runtime.pressure.mappedPressure;
        runtime.post.point.pressure = runtime.pressure.outputPressure;
    }

    int SampleCount() const {
        return std::clamp(m_sensorTxCount + m_sensorRxCount, 0, kMaxSamples);
    }

    struct Peak {
        int index = -1;
        uint16_t signal = 0;
        bool valid = false;
    };

    void SearchPeak(int groupId,
                    const std::array<uint16_t, kMaxSamples>& line,
                    int offset,
                    int length,
                    std::array<Hpp2PeakUnit, kMaxPeaksPerDim>& table,
                    int& count) {
        const auto previousTable = table;
        const int previousCount = count;
        table = {};
        count = 0;
        if (length <= 0) {
            return;
        }

        for (int i = 0; i < length; ++i) {
            if (!IsLocalPeak(line, offset, length, i)) {
                continue;
            }

            Hpp2PeakUnit unit{};
            unit.valid = true;
            unit.index = i;
            SearchPeakBoundary(line, offset, length, i, unit);
            UpdatePeakPrpt(line, offset, length, unit);
            unit.candidateCoor = GetPeakPos(groupId, line, offset, length, unit);
            UpdatePeakNoiseFlags(unit);
            unit.onEdge = unit.candidateCoor < Asa::kCoorUnit ||
                unit.candidateCoor > (length - 1) * Asa::kCoorUnit;

            if (unit.netSignal < m_peakSignalFloor || unit.width < m_peakMinWidth || unit.width > m_peakMaxWidth) {
                continue;
            }
            InsertPeakUnit(unit, table, count);
        }

        UpdatePeaksAge(table, count, previousTable, previousCount);
    }

    bool IsLocalPeak(const std::array<uint16_t, kMaxSamples>& line, int offset, int length, int index) const {
        const uint16_t current = line[static_cast<std::size_t>(offset + index)];
        if (current <= m_peakSignalFloor) {
            return false;
        }
        const int neighborDist = std::max(1, m_peakSearchNeighborDist);
        for (int delta = 1; delta <= neighborDist; ++delta) {
            if (index >= delta &&
                line[static_cast<std::size_t>(offset + index - delta)] > current) {
                return false;
            }
            if (index + delta < length &&
                line[static_cast<std::size_t>(offset + index + delta)] >= current) {
                return false;
            }
        }
        return true;
    }

    static void SearchPeakBoundary(const std::array<uint16_t, kMaxSamples>& line,
                                   int offset,
                                   int length,
                                   int peakIndex,
                                   Hpp2PeakUnit& unit) {
        static constexpr uint32_t kBoundarySlopeQ5 = 0x23; // TSACore SearchPeakBoundary for group 0/1.
        static constexpr int kContributionPermilleFloor = 50;
        static constexpr uint32_t kAccumSignalFloor = 200;

        int left = peakIndex;
        if (left != 0) {
            left = peakIndex - 1;
            int contributionPermille = 1000;
            uint32_t accumulated = line[static_cast<std::size_t>(offset + peakIndex)];
            while (left != 0 && contributionPermille > kContributionPermilleFloor) {
                const uint32_t prev = line[static_cast<std::size_t>(offset + left - 1)];
                const uint32_t current = line[static_cast<std::size_t>(offset + left)];
                if (prev >= ((kBoundarySlopeQ5 * current) >> 5)) {
                    break;
                }
                accumulated += current;
                accumulated = std::max(accumulated, kAccumSignalFloor);
                contributionPermille = static_cast<int>((current * 1000u) / accumulated);
                if (contributionPermille > kContributionPermilleFloor) {
                    --left;
                }
            }
        }

        int right = peakIndex;
        if (right < length - 1) {
            right = peakIndex + 1;
            int contributionPermille = 1000;
            uint32_t accumulated = line[static_cast<std::size_t>(offset + peakIndex)];
            while (right < length - 1 && contributionPermille > kContributionPermilleFloor) {
                const uint32_t next = line[static_cast<std::size_t>(offset + right + 1)];
                const uint32_t current = line[static_cast<std::size_t>(offset + right)];
                if (next >= ((kBoundarySlopeQ5 * current) >> 5)) {
                    break;
                }
                accumulated += current;
                accumulated = std::max(accumulated, kAccumSignalFloor);
                contributionPermille = static_cast<int>((current * 1000u) / accumulated);
                if (contributionPermille > kContributionPermilleFloor) {
                    ++right;
                }
            }
        }

        unit.leftBoundary = left;
        unit.rightBoundary = right;
        unit.width = right - left + 1;
    }

    static void UpdatePeakPrpt(const std::array<uint16_t, kMaxSamples>& line,
                               int offset,
                               int length,
                               Hpp2PeakUnit& unit) {
        uint32_t regionSum = 0;
        uint16_t baselineMin = 0xffff;
        for (int i = unit.leftBoundary; i <= unit.rightBoundary; ++i) {
            const uint16_t sample = line[static_cast<std::size_t>(offset + i)];
            regionSum += sample;
            baselineMin = std::min(baselineMin, sample);
        }

        uint32_t threeNeighborSum = 0;
        const int neighborStart = std::max(0, unit.index - 1);
        const int neighborEnd = std::min(length - 1, unit.index + 1);
        for (int i = neighborStart; i <= neighborEnd; ++i) {
            threeNeighborSum += line[static_cast<std::size_t>(offset + i)];
        }

        // TSACore UpdatePeakPrpt carries pSignalProfile/pAverageProfile-derived
        // values into UpdatePeakNoisePrpt.  HPP2 line mode keeps the observable
        // equivalents here: bounded region sum, 3-neighbor sum, and the local
        // average-baseline approximation used by the current CMN-subtracted path.
        const uint16_t peakSample = line[static_cast<std::size_t>(offset + unit.index)];
        const uint16_t avgBaseline = baselineMin;
        const uint16_t peakSignal = peakSample > avgBaseline ? static_cast<uint16_t>(peakSample - avgBaseline) : 1;
        const uint32_t net = regionSum - static_cast<uint32_t>(unit.width) * avgBaseline;
        unit.signalRegionSum = regionSum;
        unit.threeNeighborSum = static_cast<int>(std::min<uint32_t>(threeNeighborSum, 0x7fffffffu));
        unit.avgBaseline = avgBaseline;
        unit.peakSignal = peakSignal;
        unit.netSignal = static_cast<uint16_t>(std::min<uint32_t>(net, 0xffffu));
    }

    static int GetPeakPos(int groupId,
                          const std::array<uint16_t, kMaxSamples>& line,
                          int offset,
                          int length,
                          const Hpp2PeakUnit& unit) {
        const int edgeThresholdLast = groupId == 0 ? 5000 : 4500;
        const int edgeThresholdFirst = groupId == 0 ? 5000 : 3700;
        // TSACore GetPeakPos (0x6baad7cb) takes the gravity-data path:
        // GetFictiousEdge(peakGroupId, peakIndex) -> UpdateTX1GravityData or
        // UpdateTX2GravityData -> Gravity -> coarseIndex * 0x400 + gravityOffset.
        // This rebuild intentionally keeps the triangle coordinate path because,
        // in HPP2 line mode, SolveByTriangle produces the same coarse + sub-pitch
        // position class from peakPos and neighbor samples without materializing
        // TSACore's transient gravity buffers.
        return SolveByTriangle(line, offset, length, unit.index, 50, edgeThresholdLast, edgeThresholdFirst);
    }

    void UpdatePeakNoiseFlags(Hpp2PeakUnit& unit) const {
        // Not TSACore UpdatePeakNoisePrpt (0x6babddff).  The original computes
        // GetSignalUnstableLevel(threeNeighborSum, historyAvgSignal,
        // otherDimHistoryAvgSignal), last-output coordinate distance, and an SS
        // matrix recheck condition.  Those inputs are cross-frame/HPP3-grid
        // history artifacts; HPP2 line mode currently only needs the local
        // width-based noise flags below.
        unit.noiseProp = 0;
        if (unit.width < m_peakMinWidth) {
            unit.noiseProp |= 0x01;
        }
        if (unit.width > m_peakMaxWidth) {
            unit.noiseProp |= 0x02;
        }
        const uint32_t baselineAdjustedRegion =
            unit.signalRegionSum - static_cast<uint32_t>(unit.width) * unit.avgBaseline;
        if (unit.peakSignal > 1000 && baselineAdjustedRegion > static_cast<uint32_t>(unit.peakSignal) * 3u) {
            unit.noiseProp |= 0x04;
        }
    }

    static void InsertPeakUnit(const Hpp2PeakUnit& unit,
                               std::array<Hpp2PeakUnit, kMaxPeaksPerDim>& table,
                               int& count) {
        int slot = count < kMaxPeaksPerDim ? count : -1;
        if (slot < 0) {
            uint16_t weakest = table[0].peakSignal;
            slot = 0;
            for (int i = 1; i < kMaxPeaksPerDim; ++i) {
                if (table[static_cast<std::size_t>(i)].peakSignal < weakest) {
                    weakest = table[static_cast<std::size_t>(i)].peakSignal;
                    slot = i;
                }
            }
            if (unit.peakSignal <= weakest) {
                return;
            }
        } else {
            ++count;
        }
        table[static_cast<std::size_t>(slot)] = unit;
    }

    static void UpdatePeaksAge(std::array<Hpp2PeakUnit, kMaxPeaksPerDim>& table,
                               int count,
                               const std::array<Hpp2PeakUnit, kMaxPeaksPerDim>& previousTable,
                               int previousCount) {
        // Intentional simplification for HPP2 line mode: inherit only peak age.
        // TSACore also carries long-scale IIR/average/min/max history fields that
        // feed HPP3 grid feature smoothing, which is not required on this path.
        for (int i = 0; i < count; ++i) {
            auto& unit = table[static_cast<std::size_t>(i)];
            for (int j = 0; j < previousCount; ++j) {
                const auto& previous = previousTable[static_cast<std::size_t>(j)];
                if (!previous.valid) {
                    continue;
                }
                const int delta = unit.index - previous.index;
                if (delta >= -2 && delta <= 2) {
                    unit.age = std::min(previous.age + 1, 0xfff5);
                    break;
                }
            }
        }
    }

    static const Hpp2PeakUnit* SelectStrongestPeak(const std::array<Hpp2PeakUnit, kMaxPeaksPerDim>& table,
                                                   int count) {
        // TSACore compares the strongest slot using pPeakTable[slot+0x3e], a
        // copied peakSignal field.  Keep peakSignal as the primary ordering key;
        // netSignal is only a deterministic tie-breaker and exported separately.
        const Hpp2PeakUnit* strongest = nullptr;
        for (int i = 0; i < count; ++i) {
            const auto& unit = table[static_cast<std::size_t>(i)];
            if (!unit.valid) {
                continue;
            }
            if (strongest == nullptr || unit.peakSignal > strongest->peakSignal ||
                (unit.peakSignal == strongest->peakSignal && unit.netSignal > strongest->netSignal)) {
                strongest = &unit;
            }
        }
        return strongest;
    }

    uint16_t RatioToHistory(uint32_t current, int historyIndex) const {
        if (historyIndex < 0 || historyIndex >= kHistorySize) {
            return 100;
        }
        const uint32_t denom = m_lineSumHistory[static_cast<std::size_t>(historyIndex)];
        if (denom == 0) {
            return 100;
        }
        return static_cast<uint16_t>(std::min<uint32_t>((current * 100u) / denom, 0xffffu));
    }

    void ChargerNoiseJudge(HeatmapFrame& frame) {
        auto& hpp2 = frame.stylus.runtime.hpp2;
        const std::size_t freqIdx = static_cast<std::size_t>(m_curFreqIdx);
        const int frameCount = CurrentFreqFrameCount();
        uint16_t maxNoiseSample = 0;
        uint8_t abnormalChannelCount = 0;
        hpp2.line.chargerNoiseRatio.fill(0);

        if (frameCount > 1) {
            const int clearFrame = GetClearFrameIndex(frameCount);
            const Peak currentDim1 = FindPeak(hpp2.line.cmnSubtracted, 0, m_sensorTxCount);
            const Peak currentDim2 = FindPeak(hpp2.line.cmnSubtracted, m_sensorTxCount, m_sensorRxCount);
            const int count = SampleCount();
            for (int i = 0; i < count; ++i) {
                const uint16_t clearSample = m_rawHistory[freqIdx][static_cast<std::size_t>(clearFrame)][static_cast<std::size_t>(i)];
                const uint16_t floor = std::max<uint16_t>(m_chargerNoiseClearFloor, 1);
                const uint16_t denom = std::max<uint16_t>(clearSample, floor);
                const uint16_t currentSample = hpp2.line.raw[static_cast<std::size_t>(i)];
                const uint32_t ratio = (static_cast<uint32_t>(currentSample) * 100u) / denom;
                hpp2.line.chargerNoiseRatio[static_cast<std::size_t>(i)] =
                    static_cast<uint16_t>(std::min<uint32_t>(ratio, 0xffffu));

                if (IndexValidation(i, currentSample, currentDim1, currentDim2)) {
                    continue;
                }

                if (ratio > m_chargerNoiseRatioThreshold) {
                    ++abnormalChannelCount;
                    maxNoiseSample = std::max(maxNoiseSample, currentSample);
                    m_noiseSum[freqIdx] += currentSample;
                }
            }
        }

        if (m_noiseSum[freqIdx] > m_chargerNoiseSumThreshold &&
            maxNoiseSample > m_chargerNoiseMaxSampleThreshold &&
            abnormalChannelCount > m_chargerNoiseAbnormalChannelThreshold) {
            m_noiseFlag[freqIdx] = 1;
        }
    }

    int GetClearFrameIndex(int frameCount) const {
        const std::size_t freqIdx = static_cast<std::size_t>(m_curFreqIdx);
        int fallbackNoiseIndex = -1;
        uint16_t minNoiseSum = 0xffffu;
        const int historyLimit = std::min(frameCount, kNumRawHistoryFrames);
        for (int i = 0; i < historyLimit; ++i) {
            const std::size_t idx = static_cast<std::size_t>(i);
            if (m_noiseFlagHistory[freqIdx][idx] == 0) {
                return i;
            }
            const uint16_t truncatedNoiseSum = static_cast<uint16_t>(m_noiseSumHistory[freqIdx][idx]);
            if (truncatedNoiseSum < minNoiseSum) {
                minNoiseSum = truncatedNoiseSum;
                fallbackNoiseIndex = i;
            }
        }
        if (fallbackNoiseIndex >= 0) {
            return fallbackNoiseIndex;
        }
        return 0;
    }

    bool IndexValidation(int index, uint16_t rawSample, const Peak& currentDim1, const Peak& currentDim2) const {
        if (rawSample < m_chargerNoiseMinRawSample) {
            return true;
        }
        if (IsProtectedPeakIndex(index, 0, currentDim1.valid ? currentDim1.index : kInvalidPeak)) {
            return true;
        }
        if (IsProtectedPeakIndex(index, m_sensorTxCount, currentDim2.valid ? currentDim2.index : kInvalidPeak)) {
            return true;
        }
        // TSACore anchors this range against asaPrePrpt. The rebuild keeps the
        // closest observable equivalent: previous selected HPP2 line peaks for
        // the same frequency, avoiding cross-frequency F1/F2 contamination.
        const std::size_t freqIdx = static_cast<std::size_t>(m_curFreqIdx);
        if (IsProtectedPeakIndex(index, 0, m_prevPeakDim1ByFreq[freqIdx])) {
            return true;
        }
        if (IsProtectedPeakIndex(index, m_sensorTxCount, m_prevPeakDim2ByFreq[freqIdx])) {
            return true;
        }
        return false;
    }

    bool IsProtectedPeakIndex(int globalIndex, int offset, int localPeak) const {
        if (localPeak == kInvalidPeak) {
            return false;
        }
        const int localIndex = globalIndex - offset;
        const int length = offset == 0 ? m_sensorTxCount : m_sensorRxCount;
        if (localIndex < 0 || localIndex >= length) {
            return false;
        }
        const int delta = localIndex - localPeak;
        const int radius = static_cast<int>(m_chargerNoisePeakProtectRadius);
        return delta >= -radius && delta <= radius;
    }

    void RotateRawHistory(const std::array<uint16_t, kMaxSamples>& currentRaw, int count) {
        const std::size_t freqIdx = static_cast<std::size_t>(m_curFreqIdx);
        for (int i = kNumRawHistoryFrames - 1; i > 0; --i) {
            m_rawHistory[freqIdx][static_cast<std::size_t>(i)] = m_rawHistory[freqIdx][static_cast<std::size_t>(i - 1)];
            m_noiseFlagHistory[freqIdx][static_cast<std::size_t>(i)] = m_noiseFlagHistory[freqIdx][static_cast<std::size_t>(i - 1)];
            m_noiseSumHistory[freqIdx][static_cast<std::size_t>(i)] = m_noiseSumHistory[freqIdx][static_cast<std::size_t>(i - 1)];
        }
        auto& head = m_rawHistory[freqIdx][0];
        head.fill(0);
        for (int i = 0; i < count; ++i) {
            head[static_cast<std::size_t>(i)] = currentRaw[static_cast<std::size_t>(i)];
        }
        m_noiseFlagHistory[freqIdx][0] = m_noiseFlag[freqIdx];
        m_noiseSumHistory[freqIdx][0] = m_noiseSum[freqIdx];

        int& frameCount = CurrentFreqFrameCountRef();
        frameCount = std::min(frameCount + 1, kNumRawHistoryFrames);
    }

    int CurrentFreqFrameCount() const {
        return m_curFreqIdx == 0 ? m_f1FrameCnt : m_f2FrameCnt;
    }

    int& CurrentFreqFrameCountRef() {
        return m_curFreqIdx == 0 ? m_f1FrameCnt : m_f2FrameCnt;
    }

    void ProcessCmfGroup(HeatmapFrame& frame, int group, int offset, int length) {
        auto& hpp2 = frame.stylus.runtime.hpp2;
        std::array<uint16_t, kMaxSamples> slidingMin{};
        for (int i = 0; i < length; ++i) {
            uint16_t minValue = 0xffff;
            const int start = std::max(0, i - m_cmfWindowRadius);
            const int end = std::min(length - 1, i + m_cmfWindowRadius);
            for (int j = start; j <= end; ++j) {
                minValue = std::min(minValue, hpp2.line.raw[static_cast<std::size_t>(offset + j)]);
            }
            slidingMin[static_cast<std::size_t>(i)] = minValue;
        }

        uint32_t cmnSum = 0;
        uint16_t cmnMin = 0xffff;
        for (int i = 0; i < length; ++i) {
            uint16_t maxValue = 0;
            const int start = std::max(0, i - m_cmfWindowRadius);
            const int end = std::min(length - 1, i + m_cmfWindowRadius);
            for (int j = start; j <= end; ++j) {
                maxValue = std::max(maxValue, slidingMin[static_cast<std::size_t>(j)]);
            }
            const int idx = offset + i;
            const uint16_t raw = hpp2.line.raw[static_cast<std::size_t>(idx)];
            hpp2.line.cmnBaseline[static_cast<std::size_t>(idx)] = maxValue;
            hpp2.line.cmnSubtracted[static_cast<std::size_t>(idx)] = raw > maxValue ? static_cast<uint16_t>(raw - maxValue) : 0;
            cmnSum += maxValue;
            cmnMin = std::min(cmnMin, maxValue);
        }
        m_cmnSum[static_cast<std::size_t>(group)] = cmnSum;
        m_cmnMin[static_cast<std::size_t>(group)] = cmnMin;
    }

    Peak FindPeak(const std::array<uint16_t, kMaxSamples>& line, int offset, int length) const {
        Peak peak{};
        for (int i = 0; i < length; ++i) {
            if (!IsLocalPeak(line, offset, length, i)) {
                continue;
            }
            Hpp2PeakUnit unit{};
            unit.valid = true;
            unit.index = i;
            SearchPeakBoundary(line, offset, length, i, unit);
            UpdatePeakPrpt(line, offset, length, unit);
            if (unit.netSignal < m_peakSignalFloor || unit.width < m_peakMinWidth || unit.width > m_peakMaxWidth) {
                continue;
            }
            if (!peak.valid || unit.peakSignal > peak.signal) {
                peak.index = unit.index;
                peak.signal = unit.peakSignal;
                peak.valid = true;
            }
        }
        return peak;
    }

    static uint16_t LimitPressureDelta(uint16_t previous, uint16_t current, uint16_t maxDelta) {
        if (current > previous) {
            return static_cast<uint16_t>(std::min<uint32_t>(current, static_cast<uint32_t>(previous) + maxDelta));
        }
        return static_cast<uint16_t>(current + maxDelta < previous ? previous - maxDelta : current);
    }

    static uint16_t PressureIir(uint16_t previous, uint16_t current, uint8_t alpha) {
        const uint32_t mixed = static_cast<uint32_t>(previous) * (0x80u - alpha) +
                               static_cast<uint32_t>(current) * alpha;
        return static_cast<uint16_t>(std::min<uint32_t>(mixed >> 7, 0x0fffu));
    }

    static int32_t ApplyPitchCompensation(int32_t coor, const std::array<double, 4>& comp) {
        const int remainder = ((coor % Asa::kCoorUnit) + Asa::kCoorUnit) % Asa::kCoorUnit;
        const int x = (remainder < 0x201) ? (0x200 - remainder) : (remainder - 0x200);
        const double dx = static_cast<double>(x);
        int correction = static_cast<int>(comp[0] + comp[1] * dx + comp[2] * dx * dx + comp[3] * dx * dx * dx);
        if (remainder >= 0x201) {
            correction = -correction;
        }
        return coor + correction;
    }

    static int32_t SolveByTriangle(const std::array<uint16_t, kMaxSamples>& line,
                                   int offset,
                                   int length,
                                   int peakIdx,
                                   int edgeRatio,
                                   int edgeThresholdLast,
                                   int edgeThresholdFirst) {
        if (peakIdx < 0 || peakIdx >= length) {
            return -1;
        }
        const auto s = [&](int localIndex) -> int {
            if (localIndex < 0 || localIndex >= length) {
                return 0;
            }
            return line[static_cast<std::size_t>(offset + localIndex)];
        };

        if (peakIdx == 0 && length >= 3) {
            return TriangleAlgEdge(s(0), s(1), s(2), edgeRatio, edgeThresholdFirst);
        }
        if (peakIdx == length - 1 && length >= 3) {
            const int edge = TriangleAlgEdge(s(peakIdx), s(peakIdx - 1), s(peakIdx - 2), edgeRatio, edgeThresholdLast);
            return length * Asa::kCoorUnit - edge;
        }
        const int offsetInCell = TriangleAlgUsing3Point(s(peakIdx - 1), s(peakIdx), s(peakIdx + 1));
        return peakIdx * Asa::kCoorUnit + offsetInCell;
    }

    static int TriangleAlgUsing3Point(int left, int center, int right) {
        if (center <= 0) {
            return Asa::kCoorUnit / 2;
        }
        if (right < left) {
            int minVal = right;
            if (center <= right) {
                minVal = center - 1;
            }
            const int den = std::max(1, center - minVal);
            const int offset = (((left - minVal) * Asa::kCoorUnit) / den) / 2;
            return (Asa::kCoorUnit / 2) - offset;
        }
        int minVal = left;
        if (center <= left) {
            minVal = center - 1;
        }
        const int den = std::max(1, center - minVal);
        const int offset = (((right - minVal) * Asa::kCoorUnit) / den) / 2;
        return offset + (Asa::kCoorUnit / 2);
    }

    static int TriangleAlgEdge(int peak, int n1, int n2, int ratio, int threshold) {
        const int safeRatio = ratio == 0 ? 1 : ratio;
        int virtualNeighbor = ((peak - n1) * 10) / safeRatio;
        const int comp2 = peak - ((n1 - n2) * safeRatio) / 10;
        if (virtualNeighbor < comp2) {
            virtualNeighbor = comp2;
            const int sum = peak + n1 + comp2;
            int gate = comp2;
            if (sum < threshold) {
                gate = threshold - peak - n1;
            }
            if (comp2 < gate) {
                virtualNeighbor = (comp2 + gate) / 2;
            }
        }
        if (peak <= virtualNeighbor) {
            virtualNeighbor = peak - 1;
        }
        int result = TriangleAlgUsing3Point(virtualNeighbor, peak, n1);
        if (peak + n1 + n2 < (threshold * 2) / 5) {
            result = 0;
        }
        return result;
    }
};

} // namespace Solvers::Stylus::Hpp2
