#pragma once

#include "AsaTypes.hpp"
#include "GridPeakDetector.hpp"
#include "CoordinateSolver.hpp"
#include "CoorPostProcessor.hpp"
#include "CoorReviser.hpp"
#include "LinearFilter.hpp"
#include "TiltSolver.hpp"
#include "BtPressBuffer.hpp"
#include "PressureSolver.hpp"
#include "NoPressInkGate.hpp"
#include "CommonModeFilter.hpp"
#include "PenStateMachine.hpp"
#include "PacketBuilder.hpp"
#include "NoiseGate.hpp"
#include "PipelineUtils.hpp"
#include "SolverTypes.h"
#include "ConfigSchema.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <iosfwd>
#include <vector>

namespace Solvers {

/// StylusPipeline — Linear orchestrator for stylus processing.
///
/// v2 architecture:
///   Phase 1: Input parsing
///   Phase 2: TX1/TX2 coordinate solve → GLOBAL
///   Phase 2.5: Pressure + State Machine → MotionProfile
///   Phase 3: Post-processing (LinearFilter → CoorReviser → IIR → Jitter)
///   Phase 4: Edge compensation + output
class StylusPipeline {
public:
    /// Frame-level entry: reads raw data from frame, solves stylus,
    /// writes results back into frame.stylus (mirrors TouchPipeline::Process).
    bool Process(HeatmapFrame& frame);

    /// 注入 BT MCU 压感值（由 PenBridge 线程实时更新）
    void SetBtMcuPressure(uint16_t p) {
        m_btPressBuf.Push(p);
    }

    const StylusFrameData& GetLastResult() const {
        return m_lastResult;
    }

    std::vector<ConfigParam> GetConfigSchema() const;
    void SaveConfig(std::ostream& out) const;
    void LoadConfig(const std::string& key,
                    const std::string& value);

    /// Filter mode accessors (0=IIR, 1=1-Euro, 2=None)
    int  GetFilterMode() const { return m_filterMode; }
    void SetFilterMode(int mode) { m_filterMode = std::clamp(mode, 0, 2); }

    /// 实时坐标分解诊断
    using DbgCoordBreakdown = Solvers::StylusFrameData::StylusDiagnostics;
    const DbgCoordBreakdown& GetDebugCoord() const { return m_dbg; }


private:
    /// Raw-bytes entry kept as private implementation detail.
    bool ProcessRaw(std::span<const uint8_t> rawData,
                    StylusPacket& outPacket);

    // ── Frame Constants ──
    static constexpr size_t kMasterBytes      = 5063;
    static constexpr size_t kSlaveFrameBytes  = 339;
    static constexpr size_t kSlaveHeaderBytes = 7;
    static constexpr size_t kSlaveWordCount   = 166;
    static constexpr size_t kSlaveWordOffset  = kSlaveHeaderBytes;
    static constexpr size_t kStylusBlockWords = 83;

    // ── Slave frame parsing ──
    bool ParseSlaveWords(
        std::span<const uint8_t> rawData,
        std::array<uint16_t, kSlaveWordCount>& outWords) const;
    bool ValidateChecksum16(
        const uint8_t* bytes, size_t wordCount,
        uint16_t& outChecksum) const;
    bool HasCurrentStylusSignal(std::span<const uint8_t> rawData) const;
    bool ProcessNoStylusFrame(std::span<const uint8_t> rawData,
                              StylusPacket& outPacket);
    void ClosePressureGate(uint32_t latestBtSeq);
    void TrackBlockedBtSeq(uint32_t latestBtSeq);

    // ── Algorithm modules (header-only) ──
    Asa::GridPeakDetector    m_peakDetector;
    Asa::CoordinateSolver    m_coordSolver;
    Asa::CoorPostProcessor   m_postProcessor;
    Asa::CoorReviser         m_coorReviser;
    Asa::LinearFilter        m_linearFilter;
    Asa::TiltSolver          m_tiltSolver;
    Asa::PressureSolver      m_pressureSolver;
    Asa::BtPressBuffer       m_btPressBuf;
    Asa::NoPressInkGate      m_noPressInkGate;
    Asa::CommonModeFilter    m_cmfFilter;
    Asa::PenStateMachine     m_penStateMachine;
    Asa::NoiseGate           m_noiseGate;
    Asa::EdgeCoorPost        m_edgeCoorPost;
    Asa::SignalRatioTracker  m_signalRatioTracker;
    PacketBuilder            m_packetBuilder;

    // ── Smoothing filter mode (0=IIR, 1=1-Euro, 2=None) ──
    int m_filterMode = 0;

    // ── Pipeline state ──
    StylusFrameData  m_lastResult{};
    Asa::AsaGridData m_gridData{};
    bool m_prevValid = false;
    uint32_t m_prevStatus = 0;
    float m_prevPointX = 0.0f;
    float m_prevPointY = 0.0f;
    bool m_pressureGateOpen = false;
    bool m_waitingFreshBtAfterRelease = false;
    uint32_t m_blockedBtSeqFloor = 0;

    // ── Sensor dimensions ──
    int m_sensorRows = 40;
    int m_sensorCols = 60;
    int m_anchorCenterOffset = 4;

    // ── SensorPitchSizeMap tables ──
    bool m_pitchMapEnabled = false;
    std::array<double, Asa::kMaxSensorDim + 1> m_pitchTableDim1 =
        []{ std::array<double, Asa::kMaxSensorDim + 1> a; a.fill(100.0); return a; }();
    std::array<double, Asa::kMaxSensorDim + 1> m_pitchTableDim2 =
        []{ std::array<double, Asa::kMaxSensorDim + 1> a; a.fill(100.0); return a; }();

    // ── Slave header layout ──
    uint8_t m_rawSlaveHdr[7]{};

    // ── Noise level (for recheck) ──
    int m_recheckThBase = 800;
    int m_recheckThMulti = 1200;

    // ── Good frame history for freeze output ──
    StylusFrameData m_lastGoodFrame{};
    bool m_hasLastGoodFrame = false;

    // ── Config ──
    bool m_enableSlaveChecksum = false;
    bool m_emitPacketWhenInvalid = true;
    int  m_btMapMode = 0;
    int  m_tx1InkEnterThreshold = 9000;
    int  m_tx1LiftSuspiciousThreshold = 7000;
    int  m_tx1LiftAbsoluteThreshold = 4500;

    // P3 #16: TP Pattern Compensation (placeholder)
    bool m_tpPatternCompEnabled = false;
    std::array<double, 4> m_tpPatternCoefDim1{{0.0, 0.0, 0.0, 0.0}};
    std::array<double, 4> m_tpPatternCoefDim2{{0.0, 0.0, 0.0, 0.0}};

    // ── 诊断 ──
    DbgCoordBreakdown m_dbg{};
};

} // namespace Solvers
