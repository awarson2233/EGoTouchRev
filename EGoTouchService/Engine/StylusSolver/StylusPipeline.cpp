#include "StylusPipeline.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <ostream>

namespace Engine {

// ── Helpers ──
namespace {
inline uint16_t ReadU16Le(const uint8_t* p) {
    return static_cast<uint16_t>(p[0]) |
           (static_cast<uint16_t>(p[1]) << 8);
}
} // namespace

// ══════════════════════════════════════════════
// ParseSlaveWords
// ══════════════════════════════════════════════
bool StylusPipeline::ParseSlaveWords(
        std::span<const uint8_t> rawData,
        std::array<uint16_t, kSlaveWordCount>& out) const {
    const size_t required = kSlaveHeaderBytes + kSlaveWordCount * 2;
    if (rawData.size() < required) {
        LOG_DEBUG("Engine", __func__, "SlaveFrame", "rawData too small: {} < {}",  rawData.size(), required);
        return false;
    }
    if (m_enableSlaveChecksum) {
        uint16_t cs = 0;
        if (!ValidateChecksum16(rawData.data() + kSlaveHeaderBytes,
                                kSlaveWordCount, cs)) {
            LOG_DEBUG("Engine", __func__, "SlaveFrame", "Checksum failed: cs=0x{:04X}",  cs);
            return false;
        }
    }
    const uint8_t* payload = rawData.data() + kSlaveHeaderBytes;
    for (size_t i = 0; i < kSlaveWordCount; ++i)
        out[i] = ReadU16Le(payload + i * 2);
    return true;
}

bool StylusPipeline::ValidateChecksum16(
        const uint8_t* bytes, size_t wordCount,
        uint16_t& outChecksum) const {
    uint32_t sum = 0;
    for (size_t i = 0; i < wordCount; ++i)
        sum += ReadU16Le(bytes + i * 2);
    outChecksum = static_cast<uint16_t>(sum & 0xFFFF);
    return (outChecksum == 0) && (sum != 0);
}

// ══════════════════════════════════════════════
// UpdateButtonState
// ══════════════════════════════════════════════
uint32_t StylusPipeline::UpdateButtonState(
        uint32_t rawBits, bool active) {
    if (!active) { m_buttonReleaseCounter = 0; return 0; }
    const uint32_t pressed = (rawBits & 0x1u) ? 1u : 0u;
    if (pressed) {
        m_buttonReleaseCounter = m_buttonReleaseHoldFrames;
        m_lastResult.button = 1;
        return m_lastResult.status;
    }
    if (m_buttonReleaseCounter > 0) {
        m_buttonReleaseCounter--;
        m_lastResult.button = 1;
        return m_lastResult.status;
    }
    m_lastResult.button = 0;
    return m_lastResult.status;
}

// ══════════════════════════════════════════════
// Process — main pipeline (linear orchestrator)
// ══════════════════════════════════════════════
bool StylusPipeline::Process(
        std::span<const uint8_t> rawData,
        StylusPacket& outPacket) {
    m_lastResult = StylusFrameData{};
    outPacket = StylusPacket{};

    // P2 #20: Frequency shift output freeze
    if (m_freqShiftFreezing && m_hasLastGoodFrame) {
        m_lastResult = m_lastGoodFrame;
        m_lastResult.pipelineStage = 6;
        m_packetBuilder.Build(m_lastResult,
            m_bleButtonState.load(std::memory_order_relaxed),
            m_emitPacketWhenInvalid, outPacket);
        return outPacket.valid;
    }

    // ── Phase 1: Input Parsing ──

    // 1. Parse slave words
    std::array<uint16_t, kSlaveWordCount> sw{};
    if (!ParseSlaveWords(rawData, sw)) {
        m_lastResult.slaveValid = false;
        m_lastResult.pipelineStage = 1;
        if (m_emitPacketWhenInvalid) {
            outPacket.valid = true; outPacket.reportId = 0x08;
            outPacket.length = 17; outPacket.bytes.fill(0);
            outPacket.bytes[0] = 0x08;
        }
        m_prevValid = false;
        m_postProcessor.Reset();
        m_coorReviser.Reset();
        m_linearFilter.Reset();
        m_oneEuroFilter.Reset();
        return false;
    }
    m_lastResult.slaveValid = true;

    // 2. Extract dual 9x9 grids
    m_gridData = Asa::ExtractGridFromSlaveWords(
        sw.data(), static_cast<int>(sw.size()));

    // 3. Slave header (status / button)
    struct SlaveHdr {
        bool valid = false;
        uint16_t status = 0;
        uint32_t button = 0;
    } hdr;
    if (rawData.size() >= kSlaveHeaderBytes) {
        const uint8_t* p = rawData.data();
        std::memcpy(m_rawSlaveHdr, p, kSlaveHeaderBytes);
        hdr.valid  = true;
        hdr.status = ReadU16Le(p);
        hdr.button = (m_slaveHdrBtnOffset >= 0 &&
                      m_slaveHdrBtnOffset <= 6)
                     ? static_cast<uint32_t>(p[m_slaveHdrBtnOffset]) : 0u;
        m_lastResult.status = hdr.status;
    }

    // 4. TX1 validity check
    if (!m_gridData.tx1.valid) {
        m_lastResult.point.valid = false;
        m_lastResult.pipelineStage = 2;

        // Pen exit smoothing
        if (m_prevValid && m_noiseGate.ShouldExitSmooth(m_wasInking, m_hasLastGoodFrame)) {
            m_lastResult = m_lastGoodFrame;
            float outX, outY;
            m_noiseGate.ApplyExitEdgeSnap(
                m_lastGoodFrame.point.x, m_lastGoodFrame.point.y,
                m_prevPointX, m_prevPointY,
                m_sensorRows, m_sensorCols, outX, outY);
            m_lastResult.point.x = outX;
            m_lastResult.point.y = outY;
            m_lastResult.pipelineStage = 7;
            m_lastResult.point.valid = true;
            m_packetBuilder.Build(m_lastResult,
                m_bleButtonState.load(std::memory_order_relaxed),
                m_emitPacketWhenInvalid, outPacket);
            m_prevValid = false;
            m_wasInking = false;
            uint8_t animState = 0;
            m_penStateMachine.Update(false, false, animState);
            m_lastResult.animState = animState;
            m_prevStatus = m_lastResult.status;
            return outPacket.valid;
        }

        if (!m_prevValid) {
            m_postProcessor.Reset(); m_oneEuroFilter.Reset();
            m_tiltSolver.Reset(); m_calibration.Reset();
            m_coorReviser.Reset(); m_linearFilter.Reset();
            m_pressureSolver.ResetSuppression();
            m_noiseGate.Reset();
            m_hasLastGoodFrame = false;
        }
        m_prevValid = false;
        m_wasInking = false;
        uint8_t animState = 0;
        m_penStateMachine.Update(false, false, animState);
        m_lastResult.animState = animState;
        if (m_emitPacketWhenInvalid)
            m_packetBuilder.Build(m_lastResult,
                m_bleButtonState.load(std::memory_order_relaxed),
                m_emitPacketWhenInvalid, outPacket);
        m_prevStatus = m_lastResult.status;
        return false;
    }

    // 4b. Common-mode filtering
    m_cmfFilter.Apply(m_gridData.tx1.grid);
    if (m_gridData.tx2.valid)
        m_cmfFilter.Apply(m_gridData.tx2.grid);

    // ── Phase 2: Coordinate Solving ──

    // 5. Peak detection
    auto peak = m_peakDetector.FindPeak(m_gridData.tx1.grid);
    if (!peak.valid) {
        m_lastResult.point.valid = false;
        m_lastResult.pipelineStage = 3;
        m_prevValid = false;
        uint8_t animState = 0;
        m_penStateMachine.Update(false, false, animState);
        m_lastResult.animState = animState;
        if (m_emitPacketWhenInvalid)
            m_packetBuilder.Build(m_lastResult,
                m_bleButtonState.load(std::memory_order_relaxed),
                m_emitPacketWhenInvalid, outPacket);
        m_prevStatus = m_lastResult.status;
        return false;
    }

    // 6. 1D projection
    auto proj = m_peakDetector.ProjectTo1D(m_gridData.tx1.grid, peak);

    // 7. Coordinate interpolation
    auto rawCoor = m_coordSolver.Solve(proj);
    if (!rawCoor.valid) {
        m_lastResult.point.valid = false;
        m_lastResult.pipelineStage = 4;
        m_prevValid = false;
        uint8_t animState = 0;
        m_penStateMachine.Update(false, false, animState);
        m_lastResult.animState = animState;
        if (m_emitPacketWhenInvalid)
            m_packetBuilder.Build(m_lastResult,
                m_bleButtonState.load(std::memory_order_relaxed),
                m_emitPacketWhenInvalid, outPacket);
        m_prevStatus = m_lastResult.status;
        return false;
    }

    // 7b. LOCAL coordinate diagnostics
    m_lastResult.point.tx1X = static_cast<float>(rawCoor.dim1) / Asa::kCoorUnit;
    m_lastResult.point.tx1Y = static_cast<float>(rawCoor.dim2) / Asa::kCoorUnit;

    // Pitch map
    if (m_pitchMapEnabled) {
        rawCoor.dim1 = Asa::SensorPitchSizeMap(
            rawCoor.dim1, m_pitchTableDim1.data(), Asa::kCoorUnit);
        rawCoor.dim2 = Asa::SensorPitchSizeMap(
            rawCoor.dim2, m_pitchTableDim2.data(), Asa::kCoorUnit);
    }

    // 8. HPP3 Noise post-process
    if (m_noiseGate.DetectNoiseJump(rawCoor)) {
        if (m_hasLastGoodFrame) {
            m_lastResult = m_lastGoodFrame;
            m_lastResult.pipelineStage = 5;
            m_packetBuilder.Build(m_lastResult,
                m_bleButtonState.load(std::memory_order_relaxed),
                m_emitPacketWhenInvalid, outPacket);
            return outPacket.valid;
        }
        m_lastResult.point.valid = false;
        m_lastResult.pipelineStage = 5;
        m_prevValid = false;
        uint8_t animState = 0;
        m_penStateMachine.Update(false, false, animState);
        m_lastResult.animState = animState;
        if (m_emitPacketWhenInvalid)
            m_packetBuilder.Build(m_lastResult,
                m_bleButtonState.load(std::memory_order_relaxed),
                m_emitPacketWhenInvalid, outPacket);
        m_prevStatus = m_lastResult.status;
        return false;
    }

    // ── Phase 3: Post-Processing Chain ──

    const bool isEdge = m_edgeLiftCorrector.IsInEdgeRegion(
        static_cast<float>(rawCoor.dim1), static_cast<float>(rawCoor.dim2),
        Asa::kGridDim, Asa::kGridDim);

    // 9a. LinearFilter (7-state line detection)
    auto postCoor = m_linearFilter.enabled
        ? m_linearFilter.Process(rawCoor, m_lastResult.pressure)
        : rawCoor;

    // 9b. PushHistory
    m_postProcessor.StepPushHistory(postCoor);

    // 9c. 3-point average
    postCoor = m_postProcessor.Step3PointAvg(postCoor);

    // 9d. CoorReviser (TX2 dual-frequency revision)
    if (m_coorReviser.enabled && m_gridData.tx2.valid) {
        auto tx2Peak = m_peakDetector.FindPeak(m_gridData.tx2.grid);
        if (tx2Peak.valid) {
            auto tx2Proj = m_peakDetector.ProjectTo1D(
                m_gridData.tx2.grid, tx2Peak);
            auto tx2Coor = m_coordSolver.Solve(tx2Proj);
            if (tx2Coor.valid) {
                m_lastResult.point.tx2X = static_cast<float>(tx2Coor.dim1) / Asa::kCoorUnit;
                m_lastResult.point.tx2Y = static_cast<float>(tx2Coor.dim2) / Asa::kCoorUnit;
            }
            postCoor = m_coorReviser.Revise(postCoor, tx2Coor,
                                             m_lastResult.pressure);
        }
    }

    // 9e. Speed calculation
    m_postProcessor.StepCalcSpeed();

    // 9f. IIR coefficient
    const bool isInking = m_penStateMachine.IsInking();
    const int iirCoefInt = m_postProcessor.StepCalcIIRCoef(isInking);

    // 9g. Coordinate smoothing (mode-switched)
    const bool shouldSkipIIR = m_penStateMachine.ShouldSkipIIR();
    if (m_filterMode == 0) {
        postCoor = m_postProcessor.StepIIR(postCoor, iirCoefInt, shouldSkipIIR);
    } else if (m_filterMode == 1) {
        postCoor = m_oneEuroFilter.Filter(postCoor);
    }

    // 9h. Jitter offset compensation
    postCoor = m_postProcessor.StepJitter(postCoor, isEdge);

    // 9i. Update 3-point history
    m_postProcessor.StepUpdate3PtHistory(postCoor);

    // 10. Calibration (rolling average)
    auto finalCoor = m_calibration.Apply(postCoor);
    m_lastResult.pipelineStage = 0; // Success

    // ── Phase 4: LOCAL → GLOBAL conversion ──
    const int32_t centerOff = m_anchorCenterOffset * Asa::kCoorUnit;
    finalCoor.dim1 += static_cast<int32_t>(m_gridData.tx1.anchorCol) *
                      Asa::kCoorUnit - centerOff;
    finalCoor.dim2 += static_cast<int32_t>(m_gridData.tx1.anchorRow) *
                      Asa::kCoorUnit - centerOff;

    m_lastResult.point.valid = finalCoor.valid;
    m_lastResult.point.x = static_cast<float>(finalCoor.dim1);
    m_lastResult.point.y = static_cast<float>(finalCoor.dim2);

    // ── Diagnostics ──
    m_dbg.anchorRow = m_gridData.tx1.anchorRow;
    m_dbg.anchorCol = m_gridData.tx1.anchorCol;
    m_dbg.rawDim1   = rawCoor.dim1;
    m_dbg.rawDim2   = rawCoor.dim2;
    m_dbg.finalDim1 = finalCoor.dim1;
    m_dbg.finalDim2 = finalCoor.dim2;
    m_dbg.centerOff = centerOff;
    m_dbg.pointX    = m_lastResult.point.x;
    m_dbg.pointY    = m_lastResult.point.y;
    m_dbg.valid     = finalCoor.valid;
    {
        const auto& sp = m_postProcessor.GetSpeed();
        m_dbg.speedInstant  = sp.instant;
        m_dbg.speedShortAvg = sp.shortAvg;
        m_dbg.speedFullAvg  = sp.fullAvg;
    }
    m_dbg.iirCoef   = m_postProcessor.GetLastIIRCoef();
    m_dbg.isHover   = (m_lastResult.pressure == 0);
    m_dbg.isEdge    = isEdge;
    m_dbg.tiltDiffX = m_tiltSolver.GetPrevDiffX();
    m_dbg.tiltDiffY = m_tiltSolver.GetPrevDiffY();
    m_dbg.peakSignal = m_lastResult.signalX;

    m_dbg.signalRatio       = m_signalRatioTracker.GetAvgRatio();
    m_dbg.freqShiftFreezing = false;
    m_dbg.exitSmoothed      = (m_lastResult.pipelineStage == 7);
    m_dbg.cmfEnabled        = m_cmfFilter.enabled;
    m_dbg.coorReviserActive = m_coorReviser.enabled;
    m_dbg.coorRevDeltaX     = m_coorReviser.GetLastDeltaX();
    m_dbg.coorRevDeltaY     = m_coorReviser.GetLastDeltaY();
    m_dbg.tiltAnomalyDamped = m_tiltSolver.anomalyDamped;
    m_dbg.sigSuppressActive = false;
    m_dbg.penLifecycle      = static_cast<uint8_t>(m_penStateMachine.GetLifecycle());
    m_dbg.wasInking         = m_wasInking;
    m_dbg.avg3PtDim1        = postCoor.dim1;
    m_dbg.avg3PtDim2        = postCoor.dim2;

    // 10b. Edge coordinate compensation
    m_edgeCoorPost.Apply(m_lastResult.point.x, m_lastResult.point.y,
                         m_sensorCols, m_sensorRows);

    // 10c. Edge-lift artifact correction
    if (m_elcEnabled && m_edgeLiftCorrector.IsEdgeLiftArtifact(
            m_lastResult.point.x, m_lastResult.point.y,
            m_prevPointX, m_prevPointY,
            m_lastResult.pressure, m_prevPressureVal,
            m_sensorRows, m_sensorCols)) {
        m_lastResult.point.x = m_prevPointX;
        m_lastResult.point.y = m_prevPointY;
    }

    // ── Phase 5: Supplementary Features ──

    // 11. TX2 → Tilt + signal ratio
    if (m_gridData.tx2.valid) {
        auto tx2Peak = m_peakDetector.FindPeak(m_gridData.tx2.grid);
        if (tx2Peak.valid) {
            // Signal ratio tracking
            m_signalRatioTracker.Push(
                static_cast<int16_t>(std::clamp(
                    m_gridData.tx1.grid[peak.peakRow][peak.peakCol],
                    static_cast<int16_t>(0), static_cast<int16_t>(0x7FFF))),
                static_cast<int16_t>(std::clamp(
                    m_gridData.tx2.grid[tx2Peak.peakRow][tx2Peak.peakCol],
                    static_cast<int16_t>(0), static_cast<int16_t>(0x7FFF))));

            auto tx2Proj = m_peakDetector.ProjectTo1D(
                m_gridData.tx2.grid, tx2Peak);
            auto tx2Coor = m_coordSolver.Solve(tx2Proj);
            if (tx2Coor.valid) {
                m_lastResult.point.tx2X = static_cast<float>(tx2Coor.dim1) / Asa::kCoorUnit;
                m_lastResult.point.tx2Y = static_cast<float>(tx2Coor.dim2) / Asa::kCoorUnit;

                // Convert TX2 to global for tilt
                const int32_t tiltCenterOff = m_anchorCenterOffset * Asa::kCoorUnit;
                tx2Coor.dim1 += static_cast<int32_t>(m_gridData.tx2.anchorCol) *
                                Asa::kCoorUnit - tiltCenterOff;
                tx2Coor.dim2 += static_cast<int32_t>(m_gridData.tx2.anchorRow) *
                                Asa::kCoorUnit - tiltCenterOff;
                if (m_tiltSolver.enabled) {
                    m_tiltSolver.Solve(finalCoor, tx2Coor,
                        m_lastResult.point.tiltX, m_lastResult.point.tiltY);
                }
            }
        }
    }

    // 12. Pressure (BT MCU)
    m_lastResult.signalX = static_cast<uint16_t>(
        std::clamp(m_gridData.tx1.grid[peak.peakRow][peak.peakCol],
                   static_cast<int16_t>(0), static_cast<int16_t>(0x7FFF)));
    {
        uint16_t btPress = m_pressureSolver.GetLatestBtPressure();
        m_lastResult.point.rawPressure = btPress;
        m_lastResult.pressure = m_pressureSolver.Solve(
            btPress, finalCoor.valid,
            static_cast<int>(m_lastResult.signalX), isEdge);
    }

    // 13. Button (from slave header)
    if (hdr.valid)
        m_lastResult.status = UpdateButtonState(hdr.button, finalCoor.valid);

    // 14. Pen lifecycle + state machine
    uint8_t animState = 0;
    m_penStateMachine.Update(finalCoor.valid, m_lastResult.pressure > 0, animState);
    m_lastResult.animState = animState;

    // State machine: if pen just left range, reset coordinate history
    if (m_penStateMachine.JustLeftRange())
        m_postProcessor.Reset();

    // ── Phase 6: Output ──

    m_prevPointX = m_lastResult.point.x;
    m_prevPointY = m_lastResult.point.y;
    m_prevValid = finalCoor.valid;
    m_prevStatus = m_lastResult.status;
    m_prevPressureVal = m_lastResult.pressure;
    m_packetBuilder.Build(m_lastResult,
        m_bleButtonState.load(std::memory_order_relaxed),
        m_emitPacketWhenInvalid, outPacket);

    // Save last known-good frame
    m_lastGoodFrame = m_lastResult;
    m_hasLastGoodFrame = true;
    if (m_lastResult.pressure > 0) m_wasInking = true;

    // Final diagnostics
    m_dbg.rawPressure = m_lastResult.point.rawPressure;
    m_dbg.mappedPressure = m_lastResult.pressure;
    m_dbg.vhfPenState = outPacket.valid ? outPacket.bytes[1] : 0;
    m_dbg.linearFilterState = static_cast<uint8_t>(m_linearFilter.GetState());
    return outPacket.valid;
}

// ══════════════════════════════════════════════
// GetConfigSchema
// ══════════════════════════════════════════════
std::vector<ConfigParam> StylusPipeline::GetConfigSchema() const {
    using Cat = ConfigParam::Category;
    return {
        // General
        ConfigParam("sp.enableSlaveChecksum", "Enable Slave Checksum",
            ConfigParam::Bool, const_cast<bool*>(&m_enableSlaveChecksum), Cat::General),
        ConfigParam("sp.emitPacketWhenInvalid", "Emit Packet When Invalid",
            ConfigParam::Bool, const_cast<bool*>(&m_emitPacketWhenInvalid), Cat::General),
        ConfigParam("sp.buttonReleaseHold", "Button Release Hold",
            ConfigParam::Int, const_cast<int*>(&m_buttonReleaseHoldFrames), 0, 10, Cat::General),
        ConfigParam("sp.liftingTimeout", "Lifting Timeout",
            ConfigParam::Int, const_cast<int*>(&m_penStateMachine.liftingTimeout), 1, 30, Cat::General),
        ConfigParam("sp.calibEnabled", "Rolling Avg (5-frame)",
            ConfigParam::Bool, const_cast<bool*>(&m_calibration.enabled), Cat::General),

        // === Solver ===
        ConfigParam("sp.coordUseTriangle", "Use Triangle Mode",
            ConfigParam::Bool, const_cast<bool*>(&m_coordSolver.useTriangle), Cat::Solver),
        ConfigParam("sp.coordEdgeCompBit3", "Triangle Edge Compensation",
            ConfigParam::Bool, const_cast<bool*>(&m_coordSolver.edgeCompBit3), Cat::Solver),
        ConfigParam("sp.sensorRows", "Sensor Rows (Y)",
            ConfigParam::Int, const_cast<int*>(&m_sensorRows), 9, 80, Cat::Solver),
        ConfigParam("sp.sensorCols", "Sensor Cols (X)",
            ConfigParam::Int, const_cast<int*>(&m_sensorCols), 9, 80, Cat::Solver),
        ConfigParam("sp.anchorCenterOffset", "Anchor Center Offset",
            ConfigParam::Int, const_cast<int*>(&m_anchorCenterOffset), 0, 8, Cat::Solver),
        ConfigParam("sp.pitchCompDim1Enabled", "Pitch Comp Dim1 Enable",
            ConfigParam::Bool, const_cast<bool*>(&m_coordSolver.pitchCompDim1.enabled), Cat::Solver),
        ConfigParam("sp.pitchCompDim2Enabled", "Pitch Comp Dim2 Enable",
            ConfigParam::Bool, const_cast<bool*>(&m_coordSolver.pitchCompDim2.enabled), Cat::Solver),
        ConfigParam("sp.gravityNoiseFloor", "Gravity Noise Floor",
            ConfigParam::Int, const_cast<int32_t*>(&m_coordSolver.gravityNoiseFloor), 0, 500, Cat::Solver),
        ConfigParam("sp.gravityFictEdge", "Gravity Fictitious Edge",
            ConfigParam::Bool, const_cast<bool*>(&m_coordSolver.gravityFictitiousEdge), Cat::Solver),
        ConfigParam("sp.recheckEnabled", "Enable Recheck",
            ConfigParam::Bool, const_cast<bool*>(&m_noiseGate.recheckEnabled), Cat::Solver),
        ConfigParam("sp.recheckThBase", "Signal Thresh Base",
            ConfigParam::Int, const_cast<int*>(&m_noiseGate.recheckSignalThreshBase), 10, 500, Cat::Solver),

        // === Filter ===
        ConfigParam("sp.lfEnabled", "LinearFilter Enabled",
            ConfigParam::Bool, const_cast<bool*>(&m_linearFilter.enabled), Cat::Filter),
        ConfigParam("sp.lfMinFitLen", "LF Min Fit Length",
            ConfigParam::Int, const_cast<int*>(&m_linearFilter.minFitLength), 5, 100, Cat::Filter),
        ConfigParam("sp.lfEnterResidual", "LF Enter Residual Thr",
            ConfigParam::Float, const_cast<float*>(&m_linearFilter.enterResidualThreshold), 1.0f, 500.0f, Cat::Filter),
        ConfigParam("sp.lfExitDeviation", "LF Exit Deviation",
            ConfigParam::Float, const_cast<float*>(&m_linearFilter.exitDeviation), 10.0f, 1000.0f, Cat::Filter),
        ConfigParam("sp.lfPerpConstraint", "LF Perp Constraint (0-1)",
            ConfigParam::Float, const_cast<float*>(&m_linearFilter.perpConstraint), 0.0f, 1.0f, Cat::Filter),
        ConfigParam("sp.3ptAvgEnabled", "3-Point Average Enabled",
            ConfigParam::Bool, const_cast<bool*>(&m_postProcessor.enable3PointAvg), Cat::Filter),
        ConfigParam("sp.jitterEnabled", "Jitter Suppression Enabled",
            ConfigParam::Bool, const_cast<bool*>(&m_postProcessor.enableJitter), Cat::Filter),
        ConfigParam("sp.jitterEdgeDim1", "Jitter Edge Param Dim1",
            ConfigParam::Int, const_cast<int*>(&m_postProcessor.jitterEdgeParamDim1), 0, 20, Cat::Filter),
        ConfigParam("sp.jitterEdgeDim2", "Jitter Edge Param Dim2",
            ConfigParam::Int, const_cast<int*>(&m_postProcessor.jitterEdgeParamDim2), 0, 20, Cat::Filter),
        ConfigParam("sp.jitterCenterDim1", "Jitter Center Param Dim1",
            ConfigParam::Int, const_cast<int*>(&m_postProcessor.jitterCenterParamDim1), 0, 20, Cat::Filter),
        ConfigParam("sp.jitterCenterDim2", "Jitter Center Param Dim2",
            ConfigParam::Int, const_cast<int*>(&m_postProcessor.jitterCenterParamDim2), 0, 20, Cat::Filter),
        ConfigParam("sp.hpp3NoiseEnabled", "Enable HPP3 Noise",
            ConfigParam::Bool, const_cast<bool*>(&m_noiseGate.noisePostEnabled), Cat::Filter),
        ConfigParam("sp.hpp3JumpTh", "Jump Threshold",
            ConfigParam::Float, const_cast<float*>(&m_noiseGate.coorJumpThreshold), 1.0f, 100.0f, Cat::Filter),

        // === Behavior ===
        ConfigParam("sp.edgeCoorPostEnabled", "Enable Edge Coordinate Process",
            ConfigParam::Bool, const_cast<bool*>(&m_edgeCoorPost.enabled), Cat::Behavior),
        ConfigParam("sp.elcEnabled", "Enable Edge Lift Corrector",
            ConfigParam::Bool, const_cast<bool*>(&m_elcEnabled), Cat::Behavior),
        ConfigParam("sp.crEnabled", "Enable TX2 Coor Reviser",
            ConfigParam::Bool, const_cast<bool*>(&m_coorReviser.enabled), Cat::Behavior),
        ConfigParam("sp.tiltEnabled", "Enable Tilt",
            ConfigParam::Bool, const_cast<bool*>(&m_tiltSolver.enabled), Cat::Behavior),
        ConfigParam("sp.tiltKeepLast", "Keep Last On Invalid",
            ConfigParam::Bool, const_cast<bool*>(&m_tiltSolver.keepLastOnInvalid), Cat::Behavior),
        ConfigParam("sp.tiltDiffAvgWin", "Diff Average Window",
            ConfigParam::Int, const_cast<int*>(&m_tiltSolver.diffAverageWindow), 1, 10, Cat::Behavior),
        ConfigParam("sp.tiltDegCellX", "Degree/Cell X",
            ConfigParam::Float, const_cast<float*>(&m_tiltSolver.degreePerCellX), 1.0f, 30.0f, Cat::Behavior),
        ConfigParam("sp.tiltDegCellY", "Degree/Cell Y",
            ConfigParam::Float, const_cast<float*>(&m_tiltSolver.degreePerCellY), 1.0f, 30.0f, Cat::Behavior),
        ConfigParam("sp.tiltNormLenX", "Norm Len X",
            ConfigParam::Float, const_cast<float*>(&m_tiltSolver.normLenX), 0.5f, 20.0f, Cat::Behavior),
        ConfigParam("sp.tiltNormLenY", "Norm Len Y",
            ConfigParam::Float, const_cast<float*>(&m_tiltSolver.normLenY), 0.5f, 20.0f, Cat::Behavior),
        ConfigParam("sp.tiltMaxDeg", "Max Degree",
            ConfigParam::Int, const_cast<int*>(&m_tiltSolver.maxDegree), 10, 89, Cat::Behavior),
        ConfigParam("sp.tiltJitterDeg", "Jitter Threshold",
            ConfigParam::Int, const_cast<int*>(&m_tiltSolver.jitterThresholdDeg), 0, 10, Cat::Behavior),
        ConfigParam("sp.tiltIirOldW", "IIR Old Weight",
            ConfigParam::Float, const_cast<float*>(&m_tiltSolver.iirOldWeight), 0.0f, 0.99f, Cat::Behavior),

        // === Output ===
        ConfigParam("sp.pressPolyEnabled", "Polynomial Mapping",
            ConfigParam::Bool, const_cast<bool*>(&m_pressureSolver.polyEnabled), Cat::Output),
        ConfigParam("sp.pressIirQ8", "IIR Weight (Q8)",
            ConfigParam::Int, const_cast<int*>(&m_pressureSolver.iirWeightQ8), 16, 255, Cat::Output),
        ConfigParam("sp.pressSeg1Th", "Seg1 Threshold",
            ConfigParam::Int, const_cast<int*>(&m_pressureSolver.seg1Threshold), 0, 50, Cat::Output),
        ConfigParam("sp.pressSeg2Th", "Seg2 Threshold",
            ConfigParam::Int, const_cast<int*>(&m_pressureSolver.seg2Threshold), 50, 500, Cat::Output),
        ConfigParam("sp.pressGain", "Gain %",
            ConfigParam::Int, const_cast<int*>(&m_pressureSolver.gainPercent), 10, 500, Cat::Output),
        ConfigParam("sp.pressTailFrames", "Tail Frames",
            ConfigParam::Int, const_cast<int*>(&m_pressureSolver.tailFrames), 0, 20, Cat::Output),
        ConfigParam("sp.pressTailMin", "Tail Min",
            ConfigParam::Int, const_cast<int*>(&m_pressureSolver.tailMin), 0, 100, Cat::Output),
        ConfigParam("sp.pressTailDecay", "Tail Decay Rate",
            ConfigParam::Int, const_cast<int*>(&m_pressureSolver.tailDecay), 1, 200, Cat::Output),
        ConfigParam("sp.slaveHdrBtnOffset", "Button Byte Offset",
            ConfigParam::Int, const_cast<int*>(&m_slaveHdrBtnOffset), 0, 6, Cat::Output),
        ConfigParam("sp.sigSuppressEnabled", "Signal Suppress Enabled",
            ConfigParam::Bool, const_cast<bool*>(&m_pressureSolver.signalSuppressEnabled), Cat::Output),
        ConfigParam("sp.sigSuppressEnter", "Signal Suppress Enter Thr",
            ConfigParam::Int, const_cast<int*>(&m_pressureSolver.signalSuppressEnter), 10, 2000, Cat::Output),
        ConfigParam("sp.sigSuppressExit", "Signal Suppress Exit Thr",
            ConfigParam::Int, const_cast<int*>(&m_pressureSolver.signalSuppressExit), 10, 3000, Cat::Output),

        // P3: Pen exit smoothing, TP pattern, CMF
        ConfigParam("sp.exitSmoothEnabled", "Pen Exit Smooth",
            ConfigParam::Bool, const_cast<bool*>(&m_noiseGate.exitSmoothEnabled), Cat::Behavior),
        ConfigParam("sp.cmfEnabled", "CMF Enabled",
            ConfigParam::Bool, const_cast<bool*>(&m_cmfFilter.enabled), Cat::Filter),
        ConfigParam("sp.cmfWindowSize", "CMF Window Size",
            ConfigParam::Int, const_cast<int*>(&m_cmfFilter.windowSize), 1, 8, Cat::Filter),
        ConfigParam("sp.tpPatternEnabled", "TP Pattern Comp Enabled",
            ConfigParam::Bool, const_cast<bool*>(&m_tpPatternCompEnabled), Cat::Solver),

        // === IIR Q8 ===
        ConfigParam("sp.iirStillLo", "IIR Still Low Coef",
            ConfigParam::Int, const_cast<int*>(&m_postProcessor.stillIirLow), 0, 32, Cat::Filter),
        ConfigParam("sp.iirStillHi", "IIR Still High Coef",
            ConfigParam::Int, const_cast<int*>(&m_postProcessor.stillIirHigh), 0, 32, Cat::Filter),
        ConfigParam("sp.iirMoveLo", "IIR Moving Low Coef",
            ConfigParam::Int, const_cast<int*>(&m_postProcessor.movingIirLow), 0, 32, Cat::Filter),
        ConfigParam("sp.iirMoveHi", "IIR Moving High Coef",
            ConfigParam::Int, const_cast<int*>(&m_postProcessor.movingIirHigh), 0, 32, Cat::Filter),
        ConfigParam("sp.iirDivisorN", "IIR Divisor N",
            ConfigParam::Int, const_cast<int*>(&m_postProcessor.iirDivisorN), 1, 256, Cat::Filter),
        ConfigParam("sp.iirHighSpdThr", "IIR High Speed Threshold",
            ConfigParam::Int, const_cast<int*>(&m_postProcessor.highSpeedThr), 1, 1000, Cat::Filter),
        ConfigParam("sp.iirDirHalve", "IIR Directional Halve",
            ConfigParam::Bool, const_cast<bool*>(&m_postProcessor.enableDirectionalHalve), Cat::Filter),
        ConfigParam("sp.pitchMapEnabled", "Pitch Map Enabled",
            ConfigParam::Bool, const_cast<bool*>(&m_pitchMapEnabled), Cat::Solver),

        // === Filter Mode ===
        ConfigParam("sp.filterMode", "Filter Mode (0=IIR 1=1Euro 2=Off)",
            ConfigParam::Int, const_cast<int*>(&m_filterMode), 0, 2, Cat::Filter),

        // === IIR Speed Thresholds ===
        ConfigParam("sp.iirStillLowThr", "IIR Still Low Speed Thr",
            ConfigParam::Int, const_cast<int*>(&m_postProcessor.stillLowSpeedThr), 0, 200, Cat::Filter),
        ConfigParam("sp.iirMoveLowThr", "IIR Moving Low Speed Thr",
            ConfigParam::Int, const_cast<int*>(&m_postProcessor.movingLowSpeedThr), 0, 200, Cat::Filter),
        ConfigParam("sp.iirMotionFrames", "IIR Motion Detect Frames",
            ConfigParam::Int, const_cast<int*>(&m_postProcessor.motionDetectFrames), 1, 10, Cat::Filter),
        ConfigParam("sp.iirSkipFrames", "IIR Skip Frames",
            ConfigParam::Int, const_cast<int*>(&m_postProcessor.iirSkipFrames), 0, 10, Cat::Filter),

        // === 1-Euro Filter ===
        ConfigParam("sp.1eur.minCutoff", "1Euro MinCutoff",
            ConfigParam::Float, const_cast<float*>(&m_oneEuroFilter.minCutoffF),
            0.01f, 20.0f, Cat::Filter),
        ConfigParam("sp.1eur.beta", "1Euro Beta",
            ConfigParam::Float, const_cast<float*>(&m_oneEuroFilter.betaF),
            0.0001f, 2.0f, Cat::Filter),
        ConfigParam("sp.1eur.dCutoff", "1Euro DCutoff",
            ConfigParam::Float, const_cast<float*>(&m_oneEuroFilter.dCutoffF),
            0.1f, 10.0f, Cat::Filter),
        ConfigParam("sp.1eur.sampleRate", "1Euro SampleRate",
            ConfigParam::Int, const_cast<int*>(&m_oneEuroFilter.sampleRate), 60, 480, Cat::Filter),
    };
}

// ══════════════════════════════════════════════
// SaveConfig
// ══════════════════════════════════════════════
void StylusPipeline::SaveConfig(std::ostream& out) const {
    out << "sp.enableSlaveChecksum=" << m_enableSlaveChecksum << "\n";
    out << "sp.emitPacketWhenInvalid=" << m_emitPacketWhenInvalid << "\n";
    out << "sp.buttonReleaseHold=" << m_buttonReleaseHoldFrames << "\n";
    out << "sp.coordUseTriangle=" << m_coordSolver.useTriangle << "\n";
    out << "sp.coordEdgeCompBit3=" << m_coordSolver.edgeCompBit3 << "\n";
    out << "sp.lfEnabled=" << m_linearFilter.enabled << "\n";
    out << "sp.lfMinFitLen=" << m_linearFilter.minFitLength << "\n";
    out << "sp.lfEnterResidual=" << m_linearFilter.enterResidualThreshold << "\n";
    out << "sp.lfExitDeviation=" << m_linearFilter.exitDeviation << "\n";
    out << "sp.lfPerpConstraint=" << m_linearFilter.perpConstraint << "\n";
    out << "sp.3ptAvgEnabled=" << m_postProcessor.enable3PointAvg << "\n";
    out << "sp.jitterEnabled=" << m_postProcessor.enableJitter << "\n";
    out << "sp.jitterEdgeDim1=" << m_postProcessor.jitterEdgeParamDim1 << "\n";
    out << "sp.jitterEdgeDim2=" << m_postProcessor.jitterEdgeParamDim2 << "\n";
    out << "sp.jitterCenterDim1=" << m_postProcessor.jitterCenterParamDim1 << "\n";
    out << "sp.jitterCenterDim2=" << m_postProcessor.jitterCenterParamDim2 << "\n";
    out << "sp.crEnabled=" << m_coorReviser.enabled << "\n";
    out << "sp.elcEnabled=" << m_elcEnabled << "\n";
    out << "sp.pitchCompDim1Enabled=" << m_coordSolver.pitchCompDim1.enabled << "\n";
    out << "sp.pitchCompDim2Enabled=" << m_coordSolver.pitchCompDim2.enabled << "\n";
    out << "sp.gravityNoiseFloor=" << m_coordSolver.gravityNoiseFloor << "\n";
    out << "sp.gravityFictEdge=" << m_coordSolver.gravityFictitiousEdge << "\n";
    out << "sp.tiltEnabled=" << m_tiltSolver.enabled << "\n";
    out << "sp.tiltKeepLast=" << m_tiltSolver.keepLastOnInvalid << "\n";
    out << "sp.tiltDiffAvgWin=" << m_tiltSolver.diffAverageWindow << "\n";
    out << "sp.tiltDegCellX=" << m_tiltSolver.degreePerCellX << "\n";
    out << "sp.tiltDegCellY=" << m_tiltSolver.degreePerCellY << "\n";
    out << "sp.tiltNormLenX=" << m_tiltSolver.normLenX << "\n";
    out << "sp.tiltNormLenY=" << m_tiltSolver.normLenY << "\n";
    out << "sp.tiltMaxDeg=" << m_tiltSolver.maxDegree << "\n";
    out << "sp.tiltJitterDeg=" << m_tiltSolver.jitterThresholdDeg << "\n";
    out << "sp.tiltIirOldW=" << m_tiltSolver.iirOldWeight << "\n";
    out << "sp.pressPolyEnabled=" << m_pressureSolver.polyEnabled << "\n";
    out << "sp.pressIirQ8=" << m_pressureSolver.iirWeightQ8 << "\n";
    out << "sp.pressSeg1Th=" << m_pressureSolver.seg1Threshold << "\n";
    out << "sp.pressSeg2Th=" << m_pressureSolver.seg2Threshold << "\n";
    out << "sp.pressGain=" << m_pressureSolver.gainPercent << "\n";
    out << "sp.pressTailFrames=" << m_pressureSolver.tailFrames << "\n";
    out << "sp.pressTailMin=" << m_pressureSolver.tailMin << "\n";
    out << "sp.pressTailDecay=" << m_pressureSolver.tailDecay << "\n";
    out << "sp.sigSuppressEnabled=" << m_pressureSolver.signalSuppressEnabled << "\n";
    out << "sp.sigSuppressEnter=" << m_pressureSolver.signalSuppressEnter << "\n";
    out << "sp.sigSuppressExit=" << m_pressureSolver.signalSuppressExit << "\n";
    out << "sp.hpp3NoiseEnabled=" << m_noiseGate.noisePostEnabled << "\n";
    out << "sp.hpp3JumpTh=" << m_noiseGate.coorJumpThreshold << "\n";
    out << "sp.recheckEnabled=" << m_noiseGate.recheckEnabled << "\n";
    out << "sp.recheckThBase=" << m_noiseGate.recheckSignalThreshBase << "\n";
    out << "sp.liftingTimeout=" << m_penStateMachine.liftingTimeout << "\n";
    out << "sp.calibEnabled=" << m_calibration.enabled << "\n";
    out << "sp.exitSmoothEnabled=" << m_noiseGate.exitSmoothEnabled << "\n";
    out << "sp.cmfEnabled=" << m_cmfFilter.enabled << "\n";
    out << "sp.cmfWindowSize=" << m_cmfFilter.windowSize << "\n";
    out << "sp.tpPatternEnabled=" << m_tpPatternCompEnabled << "\n";
    out << "sp.iirStillLo=" << m_postProcessor.stillIirLow << "\n";
    out << "sp.iirStillHi=" << m_postProcessor.stillIirHigh << "\n";
    out << "sp.iirMoveLo=" << m_postProcessor.movingIirLow << "\n";
    out << "sp.iirMoveHi=" << m_postProcessor.movingIirHigh << "\n";
    out << "sp.iirDivisorN=" << m_postProcessor.iirDivisorN << "\n";
    out << "sp.iirHighSpdThr=" << m_postProcessor.highSpeedThr << "\n";
    out << "sp.iirDirHalve=" << m_postProcessor.enableDirectionalHalve << "\n";
    out << "sp.pitchMapEnabled=" << m_pitchMapEnabled << "\n";
    out << "sp.filterMode=" << m_filterMode << "\n";
    out << "sp.iirStillLowThr=" << m_postProcessor.stillLowSpeedThr << "\n";
    out << "sp.iirMoveLowThr=" << m_postProcessor.movingLowSpeedThr << "\n";
    out << "sp.iirMotionFrames=" << m_postProcessor.motionDetectFrames << "\n";
    out << "sp.iirSkipFrames=" << m_postProcessor.iirSkipFrames << "\n";
    out << "sp.1eur.minCutoff=" << m_oneEuroFilter.minCutoffF << "\n";
    out << "sp.1eur.beta=" << m_oneEuroFilter.betaF << "\n";
    out << "sp.1eur.dCutoff=" << m_oneEuroFilter.dCutoffF << "\n";
    out << "sp.1eur.sampleRate=" << m_oneEuroFilter.sampleRate << "\n";
}

// ══════════════════════════════════════════════
// LoadConfig
// ══════════════════════════════════════════════
void StylusPipeline::LoadConfig(
        const std::string& key,
        const std::string& value) {
    auto toBool = [](const std::string& v) { return v == "1"; };
    auto toInt = [](const std::string& v) {
        try { return std::stoi(v); } catch (...) { return 0; }
    };
    auto toFloat = [](const std::string& v) {
        try { return std::stof(v); } catch (...) { return 0.0f; }
    };

    if (key == "sp.enableSlaveChecksum") m_enableSlaveChecksum = toBool(value);
    else if (key == "sp.emitPacketWhenInvalid") m_emitPacketWhenInvalid = toBool(value);
    else if (key == "sp.buttonReleaseHold") m_buttonReleaseHoldFrames = toInt(value);
    else if (key == "sp.coordUseTriangle") m_coordSolver.useTriangle = toBool(value);
    else if (key == "sp.coordEdgeCompBit3") m_coordSolver.edgeCompBit3 = toBool(value);
    else if (key == "sp.lfEnabled") m_linearFilter.enabled = toBool(value);
    else if (key == "sp.lfMinFitLen") m_linearFilter.minFitLength = toInt(value);
    else if (key == "sp.lfEnterResidual") m_linearFilter.enterResidualThreshold = toFloat(value);
    else if (key == "sp.lfExitDeviation") m_linearFilter.exitDeviation = toFloat(value);
    else if (key == "sp.lfPerpConstraint") m_linearFilter.perpConstraint = toFloat(value);
    else if (key == "sp.3ptAvgEnabled") m_postProcessor.enable3PointAvg = toBool(value);
    else if (key == "sp.jitterEnabled") m_postProcessor.enableJitter = toBool(value);
    else if (key == "sp.jitterEdgeDim1") m_postProcessor.jitterEdgeParamDim1 = toInt(value);
    else if (key == "sp.jitterEdgeDim2") m_postProcessor.jitterEdgeParamDim2 = toInt(value);
    else if (key == "sp.jitterCenterDim1") m_postProcessor.jitterCenterParamDim1 = toInt(value);
    else if (key == "sp.jitterCenterDim2") m_postProcessor.jitterCenterParamDim2 = toInt(value);
    else if (key == "sp.crEnabled") m_coorReviser.enabled = toBool(value);
    else if (key == "sp.elcEnabled") m_elcEnabled = toBool(value);
    else if (key == "sp.pitchCompDim1Enabled") m_coordSolver.pitchCompDim1.enabled = toBool(value);
    else if (key == "sp.pitchCompDim2Enabled") m_coordSolver.pitchCompDim2.enabled = toBool(value);
    else if (key == "sp.gravityNoiseFloor") m_coordSolver.gravityNoiseFloor = toInt(value);
    else if (key == "sp.gravityFictEdge") m_coordSolver.gravityFictitiousEdge = toBool(value);
    else if (key == "sp.tiltEnabled") m_tiltSolver.enabled = toBool(value);
    else if (key == "sp.tiltKeepLast") m_tiltSolver.keepLastOnInvalid = toBool(value);
    else if (key == "sp.tiltDiffAvgWin") m_tiltSolver.diffAverageWindow = toInt(value);
    else if (key == "sp.tiltDegCellX") m_tiltSolver.degreePerCellX = toFloat(value);
    else if (key == "sp.tiltDegCellY") m_tiltSolver.degreePerCellY = toFloat(value);
    else if (key == "sp.tiltNormLenX") m_tiltSolver.normLenX = toFloat(value);
    else if (key == "sp.tiltNormLenY") m_tiltSolver.normLenY = toFloat(value);
    else if (key == "sp.tiltMaxDeg") m_tiltSolver.maxDegree = toInt(value);
    else if (key == "sp.tiltJitterDeg") m_tiltSolver.jitterThresholdDeg = toInt(value);
    else if (key == "sp.tiltIirOldW") m_tiltSolver.iirOldWeight = toFloat(value);
    else if (key == "sp.pressPolyEnabled") m_pressureSolver.polyEnabled = toBool(value);
    else if (key == "sp.pressIirQ8") m_pressureSolver.iirWeightQ8 = std::clamp(toInt(value), 16, 255);
    else if (key == "sp.pressSeg1Th") m_pressureSolver.seg1Threshold = toInt(value);
    else if (key == "sp.pressSeg2Th") m_pressureSolver.seg2Threshold = toInt(value);
    else if (key == "sp.pressGain") m_pressureSolver.gainPercent = toInt(value);
    else if (key == "sp.pressTailFrames") m_pressureSolver.tailFrames = toInt(value);
    else if (key == "sp.pressTailMin") m_pressureSolver.tailMin = toInt(value);
    else if (key == "sp.pressTailDecay") m_pressureSolver.tailDecay = toInt(value);
    else if (key == "sp.sigSuppressEnabled") m_pressureSolver.signalSuppressEnabled = toBool(value);
    else if (key == "sp.sigSuppressEnter") m_pressureSolver.signalSuppressEnter = toInt(value);
    else if (key == "sp.sigSuppressExit") m_pressureSolver.signalSuppressExit = toInt(value);
    else if (key == "sp.hpp3NoiseEnabled") m_noiseGate.noisePostEnabled = toBool(value);
    else if (key == "sp.hpp3JumpTh") m_noiseGate.coorJumpThreshold = toFloat(value);
    else if (key == "sp.recheckEnabled") m_noiseGate.recheckEnabled = toBool(value);
    else if (key == "sp.recheckThBase") m_noiseGate.recheckSignalThreshBase = toInt(value);
    else if (key == "sp.liftingTimeout") m_penStateMachine.liftingTimeout = toInt(value);
    else if (key == "sp.calibEnabled") m_calibration.enabled = toBool(value);
    else if (key == "sp.exitSmoothEnabled") m_noiseGate.exitSmoothEnabled = toBool(value);
    else if (key == "sp.cmfEnabled") m_cmfFilter.enabled = toBool(value);
    else if (key == "sp.cmfWindowSize") m_cmfFilter.windowSize = toInt(value);
    else if (key == "sp.tpPatternEnabled") m_tpPatternCompEnabled = toBool(value);
    else if (key == "sp.iirStillLo") m_postProcessor.stillIirLow = toInt(value);
    else if (key == "sp.iirStillHi") m_postProcessor.stillIirHigh = toInt(value);
    else if (key == "sp.iirMoveLo") m_postProcessor.movingIirLow = toInt(value);
    else if (key == "sp.iirMoveHi") m_postProcessor.movingIirHigh = toInt(value);
    else if (key == "sp.iirDivisorN") m_postProcessor.iirDivisorN = toInt(value);
    else if (key == "sp.iirHighSpdThr") m_postProcessor.highSpeedThr = toInt(value);
    else if (key == "sp.iirDirHalve") m_postProcessor.enableDirectionalHalve = toBool(value);
    else if (key == "sp.pitchMapEnabled") m_pitchMapEnabled = toBool(value);
    else if (key == "sp.filterMode") m_filterMode = toInt(value);
    else if (key == "sp.iirStillLowThr") m_postProcessor.stillLowSpeedThr = toInt(value);
    else if (key == "sp.iirMoveLowThr") m_postProcessor.movingLowSpeedThr = toInt(value);
    else if (key == "sp.iirMotionFrames") m_postProcessor.motionDetectFrames = toInt(value);
    else if (key == "sp.iirSkipFrames") m_postProcessor.iirSkipFrames = toInt(value);
    else if (key == "sp.1eur.minCutoff") m_oneEuroFilter.minCutoffF = toFloat(value);
    else if (key == "sp.1eur.beta") m_oneEuroFilter.betaF = toFloat(value);
    else if (key == "sp.1eur.dCutoff") m_oneEuroFilter.dCutoffF = toFloat(value);
    else if (key == "sp.1eur.sampleRate") m_oneEuroFilter.sampleRate = toInt(value);
}

} // namespace Engine
