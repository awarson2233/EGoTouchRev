#pragma once

#include "Hpp2ButtonProcess.hpp"
#include "Hpp2ChargerNoiseProcess.hpp"
#include "Hpp2CmfProcess.hpp"
#include "Hpp2Runtime.hpp"
#include "Hpp2CoordinateSolver.hpp"
#include "Hpp2DataQualityProcess.hpp"
#include "Hpp2LinePeakExtractor.hpp"
#include "Hpp2PeakSelector.hpp"
#include "Hpp2PressureProcess.hpp"
#include "Hpp2RangeStatusProcess.hpp"
#include "Hpp2StageInputProcess.hpp"
#include "Hpp2StaticStatusProcess.hpp"
#include "SolverTypes.h"
#include "StylusSolver/AsaTypes.hpp"

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
        auto& runtime = frame.stylus.runtime.SelectHpp2();
        runtime.flow.pipelineStage = 2;
        runtime.flow.frameClass = Asa::FrameClass::Valid;

        const Settings settings = MakeSettings();
        Context ctx{frame, runtime, settings, m_state};

        // TSACore boundary: txCount + rxCount must not exceed line profile capacity.
        if (settings.sensorTxCount <= 0 || settings.sensorRxCount <= 0 ||
            settings.sensorTxCount + settings.sensorRxCount > kMaxSamples) {
            runtime.flow.terminal = true;
            runtime.flow.frameClass = Asa::FrameClass::ParseFail;
            return false;
        }

        if (!settings.enabled || !frame.stylus.input.hpp2LineValid) {
            runtime.flow.terminal = true;
            runtime.flow.frameClass = Asa::FrameClass::NoSignal;
            m_state.m_wasInRange = false;
            return false;
        }

        m_stageInput.Process(ctx);
        DispatchDataProcess(ctx);

        if (runtime.bypassCurFrame || runtime.rawAbnormal || runtime.cmnAbnormal) {
            runtime.flow.terminal = true;
            runtime.post.freqBypassed = runtime.bypassCurFrame;
            return false;
        }

        if (!m_rangeStatusProcess.Process(ctx)) {
            runtime.flow.terminal = true;
            return false;
        }

        m_pressureProcess.Process(ctx);
        m_buttonProcess.Process(ctx);
        m_staticStatusProcess.Process(ctx);

        if (!m_coordinateSolver.Process(ctx)) {
            runtime.flow.terminal = true;
            runtime.flow.frameClass = Asa::FrameClass::Tx1Missing;
            return false;
        }

        Hpp2PressureProcess::PublishPressure(frame);
        runtime.post.confidence = 1.0f;
        runtime.flow.terminal = false;
        return true;
    }

    void ResetOnTerminal() {
        m_state.ResetOnTerminal();
    }

private:
    State m_state;

    Hpp2StageInputProcess    m_stageInput;
    Hpp2CmfProcess           m_cmfProcess;
    Hpp2DataQualityProcess   m_dataQualityProcess;
    Hpp2ChargerNoiseProcess  m_chargerNoiseProcess;
    Hpp2LinePeakExtractor    m_linePeakExtractor;
    Hpp2PeakSelector         m_peakSelector;
    Hpp2RangeStatusProcess   m_rangeStatusProcess;
    Hpp2PressureProcess      m_pressureProcess;
    Hpp2ButtonProcess        m_buttonProcess;
    Hpp2StaticStatusProcess  m_staticStatusProcess;
    Hpp2CoordinateSolver     m_coordinateSolver;

    void DispatchDataProcess(Context& ctx) {
        (void)m_dataType; // TSACore wrappers are identical in the analyzed build.
        m_cmfProcess.Process(ctx);
        m_dataQualityProcess.Process(ctx);
        m_chargerNoiseProcess.Process(ctx);
        m_chargerNoiseProcess.RotateRawHistory(ctx);
        m_dataQualityProcess.UpdateFrequencyLatch(ctx);
        m_linePeakExtractor.Process(ctx);
        m_peakSelector.Process(ctx);
    }

    Settings MakeSettings() const {
        Settings settings{};
        settings.enabled = m_enabled;
        settings.sensorTxCount = m_sensorTxCount;
        settings.sensorRxCount = m_sensorRxCount;
        settings.cmfWindowRadius = m_cmfWindowRadius;
        settings.rawAbnormalLineSumThreshold = m_rawAbnormalLineSumThreshold;
        settings.rawAbnormalEnergyRatioThreshold = m_rawAbnormalEnergyRatioThreshold;
        settings.cmnAbnormalSumThreshold = m_cmnAbnormalSumThreshold;
        settings.cmnAbnormalMinThreshold = m_cmnAbnormalMinThreshold;
        settings.chargerNoiseClearFloor = m_chargerNoiseClearFloor;
        settings.chargerNoiseRatioThreshold = m_chargerNoiseRatioThreshold;
        settings.chargerNoiseSumThreshold = m_chargerNoiseSumThreshold;
        settings.chargerNoiseMaxSampleThreshold = m_chargerNoiseMaxSampleThreshold;
        settings.chargerNoiseAbnormalChannelThreshold = m_chargerNoiseAbnormalChannelThreshold;
        settings.chargerNoisePeakProtectRadius = m_chargerNoisePeakProtectRadius;
        settings.chargerNoiseMinRawSample = m_chargerNoiseMinRawSample;
        settings.peakSignalFloor = m_peakSignalFloor;
        settings.peakSearchNeighborDist = m_peakSearchNeighborDist;
        settings.peakMinWidth = m_peakMinWidth;
        settings.peakMaxWidth = m_peakMaxWidth;
        settings.pressureEdgeEnterThreshold = m_pressureEdgeEnterThreshold;
        settings.pressureEdgeExitThreshold = m_pressureEdgeExitThreshold;
        settings.pressureDeltaNormal = m_pressureDeltaNormal;
        settings.pressureDeltaTight = m_pressureDeltaTight;
        settings.useTightPressureDelta = m_useTightPressureDelta;
        return settings;
    }
};

} // namespace Solvers::Stylus::Hpp2
