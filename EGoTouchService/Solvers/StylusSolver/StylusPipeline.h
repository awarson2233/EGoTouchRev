#pragma once

#include "AsaTypes.hpp"
#include "BtPressBuffer.hpp"
#include "CommonModeFilter.hpp"
#include "ConfigSchema.h"
#include "CoordinateSolver.hpp"
#include "CoorPostProcessor.hpp"
#include "CoorReviser.hpp"
#include "GridPeakDetector.hpp"
#include "LinearFilter.hpp"
#include "NoiseGate.hpp"
#include "PenStateMachine.hpp"
#include "PipelineUtils.hpp"
#include "PressureSolver.hpp"
#include "SolverTypes.h"
#include "StylusDiagnosticsWriter.hpp"
#include "StylusInputParser.hpp"
#include "StylusSignalAnalyzer.hpp"
#include "StylusStateController.hpp"

#include <algorithm>
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

namespace Solvers {

class StylusPipeline {
public:
    StylusPipeline();
    ~StylusPipeline();

    bool Process(HeatmapFrame& frame);

    void SetBtMcuPressure(uint16_t pressure) {
        m_btPressBuf.Push(pressure);
    }

    const StylusFrameData& GetLastResult() const;

    std::vector<ConfigParam> GetConfigSchema() const;
    void SaveConfig(std::ostream& out) const;
    void LoadConfig(const std::string& key, const std::string& value);

    int GetFilterMode() const {
        return m_postProcessor.filterMode;
    }

    void SetFilterMode(int mode) {
        m_postProcessor.filterMode = std::clamp(mode, 0, 2);
    }

    using DbgCoordBreakdown = StylusFrameData::StylusDiagnostics;
    const DbgCoordBreakdown& GetDebugCoord() const;

    int GetPacketSensorRows() const {
        return m_sensorRows;
    }

    int GetPacketSensorCols() const {
        return m_sensorCols;
    }

    bool GetEmitPacketWhenInvalid() const {
        return m_emitPacketWhenInvalid;
    }

    // Parse
    Asa::StylusInputParser m_inputParser;

    // Conditioning
    Asa::CommonModeFilter m_cmfFilter;

    // Extraction
    Asa::GridPeakDetector m_peakDetector;

    // Solve
    Asa::CoordinateSolver m_coordSolver;
    Asa::StylusSignalAnalyzer m_signalAnalyzer;
    Asa::PressureSolver m_pressureSolver;
    Asa::PenStateMachine m_penStateMachine;
    Asa::StylusStateController m_penState;
    Asa::BtPressBuffer m_btPressBuf;
    Asa::NoiseGate m_noiseGate;
    Asa::SignalRatioTracker m_signalRatioTracker;

    // Post
    Asa::LinearFilter m_linearFilter;
    Asa::CoorReviser m_coorReviser;
    Asa::CoorPostProcessor m_postProcessor;
    Asa::EdgeCoorPost m_edgeCoorPost;

private:
    class OutputState;

    static constexpr size_t kMasterBytes = 5063;
    static constexpr size_t kSlaveFrameBytes = 339;

    int m_sensorRows = 40;
    int m_sensorCols = 60;
    int m_anchorCenterOffset = 4;
    bool m_emitPacketWhenInvalid = true;
    std::unique_ptr<OutputState> m_output;
    StylusDiagnosticsWriter m_diagnostics;
};

} // namespace Solvers
