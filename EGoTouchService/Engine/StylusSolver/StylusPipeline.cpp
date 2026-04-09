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
inline void WriteU16Le(std::array<uint8_t, 17>& b,
                       size_t off, uint16_t v) {
    b[off]     = static_cast<uint8_t>(v & 0xFF);
    b[off + 1] = static_cast<uint8_t>((v >> 8) & 0xFF);
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
// Process — main pipeline
// ══════════════════════════════════════════════
bool StylusPipeline::Process(
        std::span<const uint8_t> rawData,
        StylusPacket& outPacket) {
    m_lastResult = StylusFrameData{};
    outPacket = StylusPacket{};

    // P2 #20: Frequency shift output freeze
    // TSACore: ReleaseASAReportInFreqShifting → memcpy(cur←prev)
    // During freq shifting, freeze output to prevent coordinate noise.
    if (m_freqShiftFreezing && m_hasLastGoodFrame) {
        m_lastResult = m_lastGoodFrame;
        m_lastResult.pipelineStage = 6; // FreqShift frozen
        BuildStylusPacket(outPacket);
        return outPacket.valid;
    }



    // 1. Parse slave words
    std::array<uint16_t, kSlaveWordCount> sw{};
    if (!ParseSlaveWords(rawData, sw)) {
        m_lastResult.slaveValid = false;
        m_lastResult.pipelineStage = 1; // SlaveParseFailure
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

    // 3. Slave header (7 bytes at frame start): status / button
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

    // 4. TX1 validity: anchor words must not both be 0x00FF
    if (!m_gridData.tx1.valid) {
        m_lastResult.point.valid = false;
        m_lastResult.pipelineStage = 2; // TX1 invalid (no pen)

        // P3 #21: Pen exit smoothing — if pen was inking, freeze 1 frame
        if (m_prevValid && HandlePenExitSmooth(outPacket)) {
            m_prevValid = false;
            m_wasInking = false;
            UpdatePenLifecycle(false, false);
            UpdateAsaStateMachine(false, false);
            m_prevStatus = m_lastResult.status;
            return outPacket.valid;
        }

        if (!m_prevValid) {
            m_postProcessor.Reset(); m_oneEuroFilter.Reset(); ResetTilt(); ResetCalibration();
            m_coorReviser.Reset(); m_linearFilter.Reset();
            m_signalSuppressActive = false;  // P1: reset hysteresis
            m_hasLastGoodFrame = false;       // P1: reset noise freeze
        }
        m_prevValid = false;
        m_wasInking = false;  // P3: clear inking state
        UpdatePenLifecycle(false, false);
        UpdateAsaStateMachine(false, false);
        if (m_emitPacketWhenInvalid) BuildStylusPacket(outPacket);
        m_prevStatus = m_lastResult.status;
        return false;
    }

    // 4b. P3 #22: Common-Mode Filtering (TSACore: HPP3_CMFProcess / GetCMN)
    // Morphological min-max filtering to remove common-mode baseline noise.
    if (m_cmfEnabled) {
        ApplyCommonModeFilter(m_gridData.tx1.grid);
        if (m_gridData.tx2.valid) {
            ApplyCommonModeFilter(m_gridData.tx2.grid);
        }
    }

    // 5. Peak detection
    auto peak = m_peakDetector.FindPeak(m_gridData.tx1.grid);
    if (!peak.valid) {
        m_lastResult.point.valid = false;
        m_lastResult.pipelineStage = 3; // No peak found
        m_prevValid = false;
        UpdatePenLifecycle(false, false);
        UpdateAsaStateMachine(false, false);
        if (m_emitPacketWhenInvalid) BuildStylusPacket(outPacket);
        m_prevStatus = m_lastResult.status;
        return false;
    }



    // 6. 1D projection
    auto proj = m_peakDetector.ProjectTo1D(m_gridData.tx1.grid, peak);

    // 7. Coordinate interpolation
    auto rawCoor = m_coordSolver.Solve(proj);
    if (!rawCoor.valid) {
        m_lastResult.point.valid = false;
        m_lastResult.pipelineStage = 4; // Coord solve failed
        m_prevValid = false;
        UpdatePenLifecycle(false, false);
        UpdateAsaStateMachine(false, false);
        if (m_emitPacketWhenInvalid) BuildStylusPacket(outPacket);
        m_prevStatus = m_lastResult.status;
        return false;
    }

    // ── 7b. LOCAL coordinate diagnostics ──
    // rawCoor.dim1/dim2 are local to the 9×9 window (range [0, 9*1024)).
    // TSACore: Entire post-processing chain works in LOCAL space.
    //          Anchor offset is added only at the final output stage.
    m_lastResult.point.tx1X = static_cast<float>(rawCoor.dim1) / Asa::kCoorUnit;
    m_lastResult.point.tx1Y = static_cast<float>(rawCoor.dim2) / Asa::kCoorUnit;

    // TSACore: SensorPitchSizeMapDim1/Dim2 operates on LOCAL coordinates.
    // It only uses pitchTable entries [0..gridDim-1], not the full sensor range.
    if (m_pitchMapEnabled) {
        rawCoor.dim1 = Asa::SensorPitchSizeMap(
            rawCoor.dim1, m_pitchTableDim1.data(), Asa::kCoorUnit);
        rawCoor.dim2 = Asa::SensorPitchSizeMap(
            rawCoor.dim2, m_pitchTableDim2.data(), Asa::kCoorUnit);
    }

    // 8. HPP3 Noise post-process
    //    P1: Freeze output on noise detection instead of skipping.
    //    TSACore: memcpy(curASOut <- prevASOut) + return 5.
    //    Result: pen continues to report last-known-good position.
    if (m_hpp3NoisePostEnabled && ApplyHpp3NoisePost(rawCoor)) {
        if (m_hasLastGoodFrame) {
            // Freeze: replay last known-good frame
            m_lastResult = m_lastGoodFrame;
            m_lastResult.pipelineStage = 5; // mark as noise-frozen
            BuildStylusPacket(outPacket);
            // Keep prevValid=true so IIR/jitter state stays valid
            return outPacket.valid;
        }
        // No good frame available yet — fall through as invalid
        m_lastResult.point.valid = false;
        m_lastResult.pipelineStage = 5;
        m_prevValid = false;
        UpdatePenLifecycle(false, false);
        UpdateAsaStateMachine(false, false);
        if (m_emitPacketWhenInvalid) BuildStylusPacket(outPacket);
        m_prevStatus = m_lastResult.status;
        return false;
    }

    // ═══════════════════════════════════════════════════════════════
    // Post-processing chain — TSACore order (ASA_CoorPostProcess)
    //   1. LinearFilter  → 2. PushHistory → 3. 3PointAvg
    //   4. CoorRevise    → 5. CalcSpeed   → 6. GetIIRCoef
    //   7. IIR filter    → 8. Jitter      → 9. Calibration
    //
    // filterMode: 0=IIR, 1=1-Euro, 2=Bypass (skip ALL smoothing)
    // ═══════════════════════════════════════════════════════════════

    // In LOCAL space, edge region is relative to 9×9 grid boundaries
    const bool isEdge  = m_edgeLiftCorrector.IsInEdgeRegion(
        static_cast<float>(rawCoor.dim1), static_cast<float>(rawCoor.dim2),
        Asa::kGridDim, Asa::kGridDim);

    // Step 9a. LinearFilter (7-state line detection)
    auto postCoor = m_linearFilter.enabled
        ? m_linearFilter.Process(rawCoor, m_lastResult.pressure)
        : rawCoor;

    // Step 9b. PushHistory (TSACore: GetRealTimeCoor2Buf)
    //          Always run — needed for speed diagnostics
    m_postProcessor.StepPushHistory(postCoor);

    // Step 9c. 3-point average (TSACore: Get3PointAvgFilter)
    //          Controlled by m_postProcessor.enable3PointAvg
    postCoor = m_postProcessor.Step3PointAvg(postCoor);

    // Step 9d. CoorReviser (TX2 dual-frequency revision)
    // TSACore: Both TX1 and TX2 are in LOCAL space during CoorReviseProcess
    if (m_coorReviser.enabled && m_gridData.tx2.valid) {
        auto tx2Peak = m_peakDetector.FindPeak(m_gridData.tx2.grid);
        if (tx2Peak.valid) {
            auto tx2Proj = m_peakDetector.ProjectTo1D(
                m_gridData.tx2.grid, tx2Peak);
            auto tx2Coor = m_coordSolver.Solve(tx2Proj);
            if (tx2Coor.valid) {
                m_lastResult.point.tx2X = static_cast<float>(tx2Coor.dim1) / Asa::kCoorUnit;
                m_lastResult.point.tx2Y = static_cast<float>(tx2Coor.dim2) / Asa::kCoorUnit;
                // TX2 stays in LOCAL space — no anchor offset added here
            }
            postCoor = m_coorReviser.Revise(postCoor, tx2Coor,
                                             m_lastResult.pressure);
        }
    }

    // Step 9e. Speed calculation (TSACore: GetCoorSpeed)
    //          Always run — needed for diagnostics
    m_postProcessor.StepCalcSpeed();

    // Step 9f. Dynamic IIR coefficient (TSACore: GetIIRCoef)
    //          TSACore switches Still/Moving based on (status & 6):
    //          Still (hover) = stronger smoothing, Moving (ink) = weaker smoothing
    const bool isInking = (m_asaStatus & (kStatInk | kStatNoPressInk)) != 0;
    const int iirCoefInt = m_postProcessor.StepCalcIIRCoef(isInking);

    // Step 9g. Coordinate smoothing filter (mode-switched)
    //   0 = IIR (TSACore Q8), 1 = 1-Euro adaptive, 2 = None (bypass)
    //
    //   TSACore CoorFilterProcess IIR skip condition:
    //     Skip when ((prevStatus & 1)==0 || inRangeFrames < 2)
    //               && inkFrames < 2 && noPressInkFrames < 2
    //   → IIR is skipped for the first 2 frames of any mode transition
    const bool shouldSkipIIR =
        ((m_prevAsaStatus & kStatInRange) == 0 || m_inRangeFrames < 2)
        && m_inkFrames < 2
        && m_noPressInkFrames < 2;

    if (m_filterMode == 0) {
        postCoor = m_postProcessor.StepIIR(postCoor, iirCoefInt, shouldSkipIIR);
    } else if (m_filterMode == 1) {
        postCoor = m_oneEuroFilter.Filter(postCoor);
    }

    // Step 9h. Jitter offset compensation (TSACore: AftCoorProcess)
    //          Controlled by m_postProcessor.enableJitter
    postCoor = m_postProcessor.StepJitter(postCoor, isEdge);

    // Step 9i. Update 3-point history for next frame
    m_postProcessor.StepUpdate3PtHistory(postCoor);

    // 10. Calibration (Phase 6)
    auto finalCoor = m_calibEnabled ? ApplyCalibration(postCoor) : postCoor;
    m_lastResult.pipelineStage = 0; // Success

    // ── 10a. LOCAL → GLOBAL coordinate conversion ──
    // TSACore: Anchor offset is added only here, AFTER all post-processing.
    // This ensures IIR/jitter/linear filters never see anchor transition jumps.
    const int32_t centerOff =
        m_anchorCenterOffset * Asa::kCoorUnit;
    finalCoor.dim1 += static_cast<int32_t>(m_gridData.tx1.anchorCol) *
                      Asa::kCoorUnit - centerOff;
    finalCoor.dim2 += static_cast<int32_t>(m_gridData.tx1.anchorRow) *
                      Asa::kCoorUnit - centerOff;

    m_lastResult.point.valid = finalCoor.valid;
    m_lastResult.point.x = static_cast<float>(finalCoor.dim1);
    m_lastResult.point.y = static_cast<float>(finalCoor.dim2);

    // 诊断：写入实时分解量（供 DrawConfigUI 展示）
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
    // ── P2: 扩展上报参数（上位机实时监控） ──
    {
        const auto& sp = m_postProcessor.GetSpeed();
        m_dbg.speedInstant  = sp.instant;
        m_dbg.speedShortAvg = sp.shortAvg;
        m_dbg.speedFullAvg  = sp.fullAvg;
    }
    m_dbg.iirCoef   = m_postProcessor.GetLastIIRCoef();
    m_dbg.isHover   = (m_lastResult.pressure == 0);
    m_dbg.isEdge    = isEdge;
    m_dbg.tiltDiffX = m_prevTiltDiffX;
    m_dbg.tiltDiffY = m_prevTiltDiffY;
    m_dbg.peakSignal = m_lastResult.signalX;

    // ── P3/P4: Extended pipeline diagnostics ──
    m_dbg.signalRatio       = m_signalRatio;
    m_dbg.freqShiftFreezing = false;  // TODO: implement freq-shift freezing
    m_dbg.exitSmoothed      = (m_lastResult.pipelineStage == 7);
    m_dbg.cmfEnabled        = m_cmfEnabled;
    m_dbg.coorReviserActive = m_coorReviser.enabled;
    m_dbg.coorRevDeltaX     = m_coorReviser.GetLastDeltaX();
    m_dbg.coorRevDeltaY     = m_coorReviser.GetLastDeltaY();
    m_dbg.tiltAnomalyDamped = m_tiltAnomalyDamped;
    m_dbg.sigSuppressActive = false;  // TODO: implement signal suppression
    m_dbg.penLifecycle      = static_cast<uint8_t>(m_penLifecycle);
    m_dbg.wasInking         = m_wasInking;
    m_dbg.avg3PtDim1        = postCoor.dim1;
    m_dbg.avg3PtDim2        = postCoor.dim2;

    // 10b. Edge coordinate compensation
    if (m_edgeCoorPostEnabled)
        EdgeCoorPostProcess(m_lastResult.point.x, m_lastResult.point.y);

    // 10c. P1: Edge-lift artifact correction
    //      If the pen just lifted at the edge with a coordinate snap,
    //      freeze to the previous frame's coordinate.
    if (m_elcEnabled && m_edgeLiftCorrector.IsEdgeLiftArtifact(
            m_lastResult.point.x, m_lastResult.point.y,
            m_prevPointX, m_prevPointY,
            m_lastResult.pressure, m_prevPressure,
            m_sensorRows, m_sensorCols)) {
        m_lastResult.point.x = m_prevPointX;
        m_lastResult.point.y = m_prevPointY;
    }

    // 11. TX2 for tilt + diagnostic coordinate output
    //     Always process TX2 when grid is valid, even if tilt is disabled,
    //     so that tx2X/tx2Y diagnostic fields are populated for the UI.
    if (m_gridData.tx2.valid) {
        auto tx2Peak = m_peakDetector.FindPeak(m_gridData.tx2.grid);
        if (tx2Peak.valid) {
            // P2 #15: Compute TX1/TX2 signal ratio (TSACore GetTX1TX2SignalRatio)
            {
                const int tx1Sig = static_cast<int>(m_lastResult.signalX);
                const int tx2Sig = static_cast<int>(std::clamp(
                    m_gridData.tx2.grid[tx2Peak.peakRow][tx2Peak.peakCol],
                    static_cast<int16_t>(0), static_cast<int16_t>(0x7FFF)));
                uint16_t ratio;
                if (tx1Sig > 0 && tx2Sig < tx1Sig * 5) {
                    ratio = static_cast<uint16_t>(tx2Sig * 100 / tx1Sig);
                } else {
                    ratio = 500;  // capped at 500%
                }
                // Push to ring buffer (TSACore: BufTX1TX2SignalRatio)
                m_signalRatioBufCount = std::min(
                    m_signalRatioBufCount + 1, kSignalRatioBufLen);
                for (int i = kSignalRatioBufLen - 1; i > 0; --i)
                    m_signalRatioBuf[static_cast<size_t>(i)] =
                        m_signalRatioBuf[static_cast<size_t>(i - 1)];
                m_signalRatioBuf[0] = ratio;
                // Average for diagnostic
                int sum = 0;
                for (int i = 0; i < m_signalRatioBufCount; ++i)
                    sum += m_signalRatioBuf[static_cast<size_t>(i)];
                m_signalRatio = static_cast<uint16_t>(
                    sum / std::max(1, m_signalRatioBufCount));
            }

            auto tx2Proj = m_peakDetector.ProjectTo1D(
                m_gridData.tx2.grid, tx2Peak);
            auto tx2Coor = m_coordSolver.Solve(tx2Proj);
            if (tx2Coor.valid) {
                // ★ Write TX2 LOCAL diagnostic coordinates (for heatmap cross display)
                m_lastResult.point.tx2X = static_cast<float>(tx2Coor.dim1) / Asa::kCoorUnit;
                m_lastResult.point.tx2Y = static_cast<float>(tx2Coor.dim2) / Asa::kCoorUnit;

                // Convert TX2 to global for tilt diff
                const int32_t tiltCenterOff =
                    m_anchorCenterOffset * Asa::kCoorUnit;
                tx2Coor.dim1 += static_cast<int32_t>(m_gridData.tx2.anchorCol) *
                                Asa::kCoorUnit - tiltCenterOff;
                tx2Coor.dim2 += static_cast<int32_t>(m_gridData.tx2.anchorRow) *
                                Asa::kCoorUnit - tiltCenterOff;
                if (m_tiltEnabled) {
                    SolveTilt(finalCoor, tx2Coor);
                }
            }
        }
    }

    // 12. Pressure — BT MCU injection only (Task 1)
    //     First, update signalX from peak data for signal-suppression gate.
    m_lastResult.signalX = static_cast<uint16_t>(
        std::clamp(m_gridData.tx1.grid[peak.peakRow][peak.peakCol],
                   static_cast<int16_t>(0), static_cast<int16_t>(0x7FFF)));
    {
        uint16_t btPress = 0;
        {
            auto nowObj = std::chrono::steady_clock::now();
            uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                  nowObj.time_since_epoch()).count();
            std::lock_guard<std::mutex> lock(m_btPressureMutex);
            while (!m_btPressureHistory.empty() &&
                   now_ms > m_btPressureHistory.front().timestamp_ms + 100) {
                m_btPressureHistory.pop_front();
            }
            for (auto it = m_btPressureHistory.rbegin();
                 it != m_btPressureHistory.rend(); ++it) {
                if (now_ms <= it->timestamp_ms + 50) {
                    if (it->pressure > btPress) btPress = it->pressure;
                }
            }
        }
        SolvePressure(btPress, finalCoor.valid,
                      static_cast<int>(m_lastResult.signalX), isEdge);
    }

    // 13. Button (from slave header)
    if (hdr.valid)
        m_lastResult.status = UpdateButtonState(
            hdr.button, finalCoor.valid);

    // 14. Pen lifecycle + TSACore state machine
    UpdatePenLifecycle(finalCoor.valid, m_lastResult.pressure > 0);
    // TSACore: DAT_18231b28 = DAT_18231950 (save status at end of ASA_CoorPostProcess)
    UpdateAsaStateMachine(finalCoor.valid, m_lastResult.pressure > 0);

    // 15. Build packet + save state for next frame
    m_prevPointX = m_lastResult.point.x;
    m_prevPointY = m_lastResult.point.y;
    m_prevValid = finalCoor.valid;
    m_prevStatus = m_lastResult.status;
    BuildStylusPacket(outPacket);
    // P1: Save last known-good frame for noise freeze (#19)
    m_lastGoodFrame = m_lastResult;
    m_hasLastGoodFrame = true;
    // P3 #21: Track inking state for pen exit smoothing
    if (m_lastResult.pressure > 0) m_wasInking = true;
    // ── 上报：最终压力和 VHF 状态 ──
    m_dbg.rawPressure = m_lastResult.point.rawPressure;
    m_dbg.mappedPressure = m_lastResult.pressure;
    m_dbg.vhfPenState = outPacket.valid ? outPacket.bytes[1] : 0;
    m_dbg.linearFilterState = static_cast<uint8_t>(m_linearFilter.GetState());
    return outPacket.valid;
}

// ══════════════════════════════════════════════
// Tilt (migrated from StylusProcessor)
// ══════════════════════════════════════════════
int StylusPipeline::ConvertCoordDiffToTilt(
        float coordDiff, bool dimY) const {
    const float normLen = std::max(0.1f,
        dimY ? m_tiltNormLenY : m_tiltNormLenX);
    const float legacyScale = std::max(0.1f,
        (dimY ? m_tiltDegreePerCellY : m_tiltDegreePerCellX) / 8.0f);
    const float scaled = coordDiff * legacyScale;
    float deg = 0.0f;
    if (std::abs(scaled) < normLen)
        deg = std::asin(scaled / normLen) * 57.2957795f;
    else
        deg = (scaled < 0.0f) ? -90.0f : 90.0f;
    const int maxT = std::clamp(m_tiltMaxDegree, 1, 89);
    return std::clamp(static_cast<int>(std::lround(deg)),
                      -maxT, maxT);
}

void StylusPipeline::ResetTilt() {
    m_tiltDiffBufCount = 0;
    m_tiltDiffBufX.fill(0.0f);
    m_tiltDiffBufY.fill(0.0f);
    m_prevTiltDiffX = 0.0f;
    m_prevTiltDiffY = 0.0f;
    m_prevTiltX = 0;
    m_prevTiltY = 0;
    m_tiltHasHistory = false;
}

void StylusPipeline::SolveTilt(
        const Asa::AsaCoorResult& c1,
        const Asa::AsaCoorResult& c2) {
    if (!c1.valid || !c2.valid) {
        if (m_tiltKeepLastOnInvalid && m_tiltHasHistory) {
            m_lastResult.point.tiltX = m_prevTiltX;
            m_lastResult.point.tiltY = m_prevTiltY;
        }
        return;
    }

    float diffX = static_cast<float>(c2.dim1 - c1.dim1) / 1024.0f;
    float diffY = static_cast<float>(c2.dim2 - c1.dim2) / 1024.0f;

    // P2 #15: Tilt coord diff anomaly detection
    // TSACore: when TX1/TX2 peak positions mismatch or diff jumps too far,
    // use buffered value (1/8 IIR blend) instead of raw diff.
    m_tiltAnomalyDamped = false;
    if (m_tiltHasHistory) {
        const float jumpX = std::abs(diffX - m_prevTiltDiffX);
        const float jumpY = std::abs(diffY - m_prevTiltDiffY);
        if (jumpX > m_tiltDiffAnomalyThreshold ||
            jumpY > m_tiltDiffAnomalyThreshold) {
            // Anomalous jump: blend 1/8 new + 7/8 old (heavy damping)
            diffX = m_prevTiltDiffX * 0.875f + diffX * 0.125f;
            diffY = m_prevTiltDiffY * 0.875f + diffY * 0.125f;
            m_tiltAnomalyDamped = true;
        }
    }

    // Shift into diff buffer
    m_tiltDiffBufCount = std::min(10, m_tiltDiffBufCount + 1);
    for (int i = 9; i > 0; --i) {
        m_tiltDiffBufX[static_cast<size_t>(i)] =
            m_tiltDiffBufX[static_cast<size_t>(i-1)];
        m_tiltDiffBufY[static_cast<size_t>(i)] =
            m_tiltDiffBufY[static_cast<size_t>(i-1)];
    }
    m_tiltDiffBufX[0] = diffX;
    m_tiltDiffBufY[0] = diffY;

    // Sliding average
    const int cnt = std::max(1, std::min(m_tiltDiffBufCount,
        std::clamp(m_tiltDiffAverageWindow, 1, 10)));
    float sX = 0, sY = 0;
    for (int i = 0; i < cnt; ++i) {
        sX += m_tiltDiffBufX[static_cast<size_t>(i)];
        sY += m_tiltDiffBufY[static_cast<size_t>(i)];
    }
    diffX = sX / cnt; diffY = sY / cnt;

    // IIR
    if (m_tiltHasHistory) {
        const float w = std::clamp(m_tiltCoordIirOldWeight, 0.f, 0.995f);
        diffX = m_prevTiltDiffX * w + diffX * (1.f - w);
        diffY = m_prevTiltDiffY * w + diffY * (1.f - w);
    }
    m_prevTiltDiffX = diffX;
    m_prevTiltDiffY = diffY;

    // P2: Vector length clamp (TSACore step 9)
    // If |diff_vector| > limit, scale to limit preserving direction
    {
        const float vecLen = std::sqrt(diffX * diffX + diffY * diffY);
        const float limit = std::max(0.1f, m_tiltVectorClampLimit);
        if (vecLen > limit && vecLen > 0.001f) {
            diffX = diffX * (limit / vecLen);
            diffY = diffY * (limit / vecLen);
        }
    }

    int outX = ConvertCoordDiffToTilt(diffX, false);
    int outY = ConvertCoordDiffToTilt(diffY, true);

    // Jitter lock
    if (m_tiltHasHistory) {
        const int jit = std::max(0, m_tiltJitterThresholdDeg);
        if (std::abs(outX - m_prevTiltX) <= jit) outX = m_prevTiltX;
        if (std::abs(outY - m_prevTiltY) <= jit) outY = m_prevTiltY;
    }

    m_lastResult.point.tiltX = static_cast<int16_t>(outX);
    m_lastResult.point.tiltY = static_cast<int16_t>(outY);
    m_prevTiltX = m_lastResult.point.tiltX;
    m_prevTiltY = m_lastResult.point.tiltY;
    m_tiltHasHistory = true;
}

// ══════════════════════════════════════════════
// Pressure (migrated from StylusProcessor)
// ══════════════════════════════════════════════
void StylusPipeline::SolvePressure(
        uint16_t rawPressure, bool active,
        int signalStrength, bool isEdge) {
    if (!active) {
        m_lastResult.pressure = 0;
        m_prevPressure = 0;
        m_pressureTailCounter = 0;
        return;
    }

    // P1: Signal strength suppression with hysteresis (TSACore HPP3_SuppressBtPressBySignal)
    // Uses enter/exit thresholds + state flag to prevent rapid toggling.
    // TSACore also skips suppression when coordinate is in edge region.
    if (m_pressureSignalSuppressEnabled && signalStrength > 0) {
        if (!m_signalSuppressActive) {
            // Not suppressing → enter suppression if signal drops below
            // enter threshold AND not in edge region
            if (signalStrength < m_pressureSignalSuppressEnter && !isEdge) {
                m_signalSuppressActive = true;
            }
        } else {
            // Currently suppressing → exit only when signal rises above
            // exit threshold (higher than enter to prevent oscillation)
            if (signalStrength > m_pressureSignalSuppressExit) {
                m_signalSuppressActive = false;
            }
        }
        if (m_signalSuppressActive) {
            m_lastResult.pressure = 0;
            m_prevPressure = 0;
            return;
        }
    }
    // Polynomial mapping
    const int x = static_cast<int>(rawPressure);
    const int th1 = m_pressureMapSeg1Threshold;
    const int th2 = m_pressureMapSeg2Threshold;
    int mapped = 0;
    if (x <= th1) {
        mapped = (x > 1) ? 1 : x;
    } else if (m_pressurePolyEnabled) {
        const auto eval = [x](const std::array<double,5>& c) {
            double d = static_cast<double>(x);
            return static_cast<int>(
                c[0] + c[1]*d + c[2]*d*d + c[3]*d*d*d + c[4]*d*d*d*d);
        };
        mapped = (x <= th2) ? eval(m_pressurePolySeg1)
                            : eval(m_pressurePolySeg2);
    } else {
        mapped = x;
    }
    mapped = mapped * std::clamp(m_pressureMapGainPercent, 1, 1000) / 100;
    mapped = std::clamp(mapped, 0, 0x0FFF);

    // IIR — P1: Q8 (÷256) to match TSACore (was Q7 ÷128)
    if (mapped > 0 && m_prevPressure > 0) {
        const int w = std::clamp(m_pressureIirWeightQ8, 1, 255);
        mapped = ((static_cast<int>(m_prevPressure) *
                   (256 - w)) + mapped * w + 128) >> 8;
        mapped = std::clamp(mapped, 0, 0x0FFF);
    }
    // Tail decay
    if (mapped == 0 && m_prevPressure > 0 &&
        m_pressureTailFrames > 0 &&
        m_pressureTailCounter < m_pressureTailFrames) {
        mapped = std::max(m_pressureTailMin,
            std::max(0, static_cast<int>(m_prevPressure) -
                        std::max(1, m_pressureTailDecay)));
        mapped = std::clamp(mapped, 0, 0x0FFF);
        m_pressureTailCounter++;
    } else if (mapped > 0) {
        m_pressureTailCounter = 0;
    }

    m_lastResult.pressure = static_cast<uint16_t>(mapped);
    m_prevPressure = m_lastResult.pressure;
}

// ══════════════════════════════════════════════
// Button
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
// EdgeCoorPostProcess (ported from TSACore)
// Compensates for edge signal attenuation on
// the first and last sensor cell in each axis.
// ══════════════════════════════════════════════
void StylusPipeline::EdgeCoorPostProcess(
        float& dim1, float& dim2) const {
    // Helper: process one dimension
    auto edgeClamp = [](float coord, int sensorDim) -> float {
        constexpr float deadZone   = static_cast<float>(kEdgeDeadZone);
        constexpr float cellUnit   = static_cast<float>(kCellUnit);
        constexpr float activeZone = static_cast<float>(kEdgeActiveZone);
        const float maxCoord = static_cast<float>(sensorDim) * cellUnit;

        // First cell: coord in [0, cellUnit)
        if (coord < cellUnit) {
            if (coord < deadZone)
                return 0.0f;               // dead zone → clamp to 0
            return (coord - deadZone) * cellUnit / activeZone;
        }
        // Last cell: coord in [(sensorDim-1)*cellUnit, sensorDim*cellUnit]
        const float lastCellStart = static_cast<float>(sensorDim - 1) * cellUnit;
        if (coord > lastCellStart) {
            float distFromEnd = maxCoord - coord;
            if (distFromEnd < deadZone)
                return maxCoord;            // dead zone → clamp to max
            return maxCoord - (distFromEnd - deadZone) * cellUnit / activeZone;
        }
        return coord;  // interior cells: no change
    };

    dim1 = edgeClamp(dim1, m_sensorCols);  // point.x → 水平/Col 方向 → 用列数
    dim2 = edgeClamp(dim2, m_sensorRows);  // point.y → 垂直/Row 方向 → 用行数
}

// ══════════════════════════════════════════════
// Recheck
// ══════════════════════════════════════════════
bool StylusPipeline::EvaluateRecheck() const {
    if (!m_recheckEnabled) return true;
    const int sig = static_cast<int>(m_lastResult.signalX);
    const int th = (m_noiseLevel > 2) ? m_recheckSignalThreshBase * 2
                                       : m_recheckSignalThreshBase;
    return sig >= th;
}

// ══════════════════════════════════════════════
// HPP3 Noise Post
// ══════════════════════════════════════════════
bool StylusPipeline::ApplyHpp3NoisePost(
        const Asa::AsaCoorResult& coor) {
    if (!coor.valid) return false;
    const float cx = static_cast<float>(coor.dim1);
    const float cy = static_cast<float>(coor.dim2);

    if (m_prevValidPoint) {
        const float dx = cx - m_prevValidX;
        const float dy = cy - m_prevValidY;
        if (dx * dx + dy * dy >
            m_hpp3CoorJumpThreshold * m_hpp3CoorJumpThreshold) {
            return true; // noise jump detected
        }
    }
    m_prevValidX = cx;
    m_prevValidY = cy;
    m_prevValidPoint = true;
    return false;
}

// ══════════════════════════════════════════════
// P3 #21: HandlePenExitSmooth
// TSACore: ReleaseASAReportExitStylus
// When pen was inking and signal is lost, freeze output for 1 frame
// using the last known-good frame data, with edge coordinate snapping
// if the pen exited at a panel edge.
// ══════════════════════════════════════════════
bool StylusPipeline::HandlePenExitSmooth(StylusPacket& outPacket) {
    if (!m_exitSmoothEnabled) return false;
    if (!m_wasInking) return false;  // not inking → no smoothing needed
    if (!m_hasLastGoodFrame) return false;  // no good frame to freeze

    // Freeze to last known-good frame
    m_lastResult = m_lastGoodFrame;

    // TSACore: EdgeCoorProcessExitStylusWithInk
    // If the last coordinate was in an edge region AND jumped significantly
    // from the frame before, snap coordinates to the pre-jump position.
    // This prevents the final frame from showing a wild coordinate excursion.
    const float lastX = m_lastGoodFrame.point.x;
    const float lastY = m_lastGoodFrame.point.y;
    const float prevX = m_prevPointX;
    const float prevY = m_prevPointY;

    // Edge region check (first/last pitch)
    const float dimXMax = static_cast<float>(m_sensorCols) * Asa::kCoorUnit;
    const float dimYMax = static_cast<float>(m_sensorRows) * Asa::kCoorUnit;
    const float edgeTh = static_cast<float>(Asa::kCoorUnit);  // 0x400

    bool atEdge = (lastX < edgeTh || lastX > dimXMax - edgeTh ||
                   lastY < edgeTh || lastY > dimYMax - edgeTh);

    if (atEdge) {
        // Distance check: if large jump → snap to previous position
        float dx = lastX - prevX;
        float dy = lastY - prevY;
        if (dx * dx + dy * dy > 0x200 * 0x200) {  // 512² units threshold
            m_lastResult.point.x = prevX;
            m_lastResult.point.y = prevY;
        }
    }

    m_lastResult.pipelineStage = 7;  // Exit smooth
    m_lastResult.point.valid = true;
    BuildStylusPacket(outPacket);

    // Reset inking flag — only smooth once
    m_wasInking = false;
    return true;
}

// ══════════════════════════════════════════════
// P3 #22: ApplyCommonModeFilter
// TSACore: HPP3_CMFProcess / GetCMN
// Morphological open (erosion then dilation) on each row and column
// of the 9×9 grid to estimate common-mode baseline, then subtract.
// ══════════════════════════════════════════════
void StylusPipeline::ApplyCommonModeFilter(
        int16_t grid[Asa::kGridDim][Asa::kGridDim]) {
    constexpr int N = Asa::kGridDim;
    const int w = std::clamp(m_cmfWindowSize, 1, N - 1);

    // Helper: 1D morphological open (erosion→dilation) on a vector
    // This estimates the common-mode baseline.
    auto morphOpen1D = [&](int16_t* arr, int len) {
        std::array<int16_t, Asa::kGridDim> eroded{};
        std::array<int16_t, Asa::kGridDim> dilated{};

        // Erosion: min over window [i-w, i+w]
        for (int i = 0; i < len; ++i) {
            int lo = std::max(0, i - w);
            int hi = std::min(len - 1, i + w);
            int16_t minVal = arr[lo];
            for (int j = lo + 1; j <= hi; ++j) {
                if (arr[j] < minVal) minVal = arr[j];
            }
            eroded[static_cast<size_t>(i)] = minVal;
        }

        // Dilation: max over erosion result with same window
        for (int i = 0; i < len; ++i) {
            int lo = std::max(0, i - w);
            int hi = std::min(len - 1, i + w);
            int16_t maxVal = eroded[static_cast<size_t>(lo)];
            for (int j = lo + 1; j <= hi; ++j) {
                if (eroded[static_cast<size_t>(j)] > maxVal)
                    maxVal = eroded[static_cast<size_t>(j)];
            }
            dilated[static_cast<size_t>(i)] = maxVal;
        }

        // Subtract baseline: arr[i] -= dilated[i]
        for (int i = 0; i < len; ++i) {
            arr[i] -= dilated[static_cast<size_t>(i)];
            // Clamp to non-negative (signal should be >= 0 after CMF)
            if (arr[i] < 0) arr[i] = 0;
        }
    };

    // Apply to each row (dim1 direction)
    for (int r = 0; r < N; ++r) {
        morphOpen1D(grid[r], N);
    }

    // Apply to each column (dim2 direction)
    for (int c = 0; c < N; ++c) {
        std::array<int16_t, Asa::kGridDim> col{};
        for (int r = 0; r < N; ++r) {
            col[static_cast<size_t>(r)] = grid[r][c];
        }
        morphOpen1D(col.data(), N);
        for (int r = 0; r < N; ++r) {
            grid[r][c] = col[static_cast<size_t>(r)];
        }
    }
}

// ══════════════════════════════════════════════
// UpdatePenLifecycle — Pen Lifecycle Tracker
// Leave → Hover → Contact → Lifting → Leave
// ══════════════════════════════════════════════
void StylusPipeline::UpdatePenLifecycle(
        bool penValid, bool penDown) {
    switch (m_penLifecycle) {
    case PenLifecycle::Leave:
        if (penValid)
            m_penLifecycle = PenLifecycle::Hover;
        break;
    case PenLifecycle::Hover:
        if (!penValid) {
            m_penLifecycle = PenLifecycle::Leave;
        } else if (penDown) {
            m_penLifecycle = PenLifecycle::Contact;
            m_liftingFrameCount = 0;
        }
        break;
    case PenLifecycle::Contact:
        if (!penDown) {
            m_penLifecycle = PenLifecycle::Lifting;
            m_liftingFrameCount = 0;
        }
        break;
    case PenLifecycle::Lifting:
        m_liftingFrameCount++;
        if (penDown) {
            m_penLifecycle = PenLifecycle::Contact;
            m_liftingFrameCount = 0;
        } else if (!penValid ||
                   m_liftingFrameCount > m_liftingTimeout) {
            m_penLifecycle = PenLifecycle::Leave;
        }
        break;
    }
    m_lastResult.animState = static_cast<uint8_t>(m_penLifecycle);
}

// ══════════════════════════════════════════════
// UpdateAsaStateMachine — TSACore HPP3_ASAStaticStatusPostProcess
//
// Maintains 3-bit status word with independent frame counters.
// Controls IIR skip (first 2 frames after mode transition) and
// Still/Moving IIR coefficient selection.
//
// State bits:
//   bit0 (0x01) InRange    — pen in detection range (hover)
//   bit1 (0x02) Ink        — pressure active (writing)
//   bit2 (0x04) NoPressInk — coord valid, no pressure (near-surface)
// ══════════════════════════════════════════════
void StylusPipeline::UpdateAsaStateMachine(
        bool coordValid, bool hasInk) {
    // Save previous status for IIR skip logic
    m_prevAsaStatus = m_asaStatus;

    // Build new status bits (TSACore: HPP3_ASAStaticStatusPostProcess)
    m_asaStatus = 0;
    if (coordValid)  m_asaStatus |= kStatInRange;    // bit0
    if (hasInk)      m_asaStatus |= kStatInk;        // bit1
    if (coordValid)  m_asaStatus |= kStatNoPressInk;  // bit2 (coord valid = "NoPressInk")

    // ── NoPressInk mode ──
    // TSACore: EnterNoPressInkMode clears InRange counter
    if (m_asaStatus & kStatNoPressInk) {
        m_noPressInkFrames++;
        m_inRangeFrames = 0;  // TSACore: EnterNoPressInkMode → DAT_1823194c = 0
    } else {
        m_noPressInkFrames = 0;  // TSACore: ExitNoPressInkMode
    }

    // ── Ink mode ──
    // TSACore: EnterInkMode clears InRange counter
    if (m_asaStatus & kStatInk) {
        m_inkFrames++;
        m_inRangeFrames = 0;  // TSACore: EnterInkMode → DAT_1823194c = 0
    } else {
        m_inkFrames = 0;  // TSACore: ExitInkMode
    }

    // ── InRange mode ──
    if (m_asaStatus & kStatInRange) {
        m_inRangeFrames++;
    }

    // ── Exit range → full reset ──
    // TSACore: ExitInRangeMode calls CoorInit() + ASAPropertyInit()
    if (!(m_asaStatus & kStatInRange) && (m_prevAsaStatus & kStatInRange)) {
        // Pen just left range — reset all coordinate history
        m_inRangeFrames = 0;
        m_inkFrames = 0;
        m_noPressInkFrames = 0;
        m_postProcessor.Reset();  // TSACore: CoorInit()
    }
}

// ══════════════════════════════════════════════
// ASACalibration_Process (Phase 6)
// Rolling kCalibWindow average on final coordinates
// ══════════════════════════════════════════════
Asa::AsaCoorResult StylusPipeline::ApplyCalibration(
        const Asa::AsaCoorResult& c) {
    if (!c.valid) { ResetCalibration(); return c; }
    int idx = m_calibCount % kCalibWindow;
    m_calibDim1[static_cast<size_t>(idx)] = c.dim1;
    m_calibDim2[static_cast<size_t>(idx)] = c.dim2;
    m_calibCount = std::min(m_calibCount + 1, kCalibWindow);

    int32_t s1 = 0, s2 = 0;
    for (int i = 0; i < m_calibCount; ++i) {
        s1 += m_calibDim1[static_cast<size_t>(i)];
        s2 += m_calibDim2[static_cast<size_t>(i)];
    }
    Asa::AsaCoorResult out = c;
    out.dim1 = s1 / m_calibCount;
    out.dim2 = s2 / m_calibCount;
    return out;
}

void StylusPipeline::ResetCalibration() {
    m_calibCount = 0;
    m_calibDim1.fill(0);
    m_calibDim2.fill(0);
}

// ══════════════════════════════════════════════
// BuildStylusPacket
// ══════════════════════════════════════════════
void StylusPipeline::BuildStylusPacket(StylusPacket& pkt) const {
    pkt = StylusPacket{};
    pkt.reportId = 0x08;
    // ── HID Pen Report layout (from hidinjector.sys descriptor) ──
    //   b[0]      : Report ID (0x08)
    //   b[1]      : Status bits (TipSwitch:0, Barrel:1, Invert:2, Eraser:3, pad:4, InRange:5)
    //   b[2]      : Contact Identifier
    //   b[3..4]   : X position (uint16 LE, 0..16000)
    //   b[5..6]   : Y position (uint16 LE, 0..25600)
    //   b[7..8]   : Tip Pressure (uint16 LE, 0..4095)
    //   b[9..10]  : X Tilt (int16 LE, -9000..+9000 centidegrees)
    //   b[11..12] : Y Tilt (int16 LE, -9000..+9000 centidegrees)
    // Total = 13 bytes
    pkt.length = 13;
    if (!m_lastResult.point.valid && !m_emitPacketWhenInvalid) {
        pkt.valid = false; return;
    }
    pkt.valid = true;
    auto& b = pkt.bytes;
    b.fill(0);
    b[0] = 0x08;

    // ── Status byte ──
    {
        uint8_t penState = 0;
        if (m_lastResult.point.valid)
            penState |= (1u << 5);   // bit5 = InRange
        if (m_lastResult.pressure > 0)
            penState |= (1u << 0);   // bit0 = TipSwitch
        // BarrelSwitch: driven by BLE button data via UpdateButtonFromBle()
        // m_bleButtonState bit0 = barrel button
        const uint8_t bleBtn = m_bleButtonState.load(std::memory_order_relaxed);
        if (bleBtn & 0x01)
            penState |= (1u << 1);   // bit1 = BarrelSwitch
        b[1] = penState;
    }

    // ── Contact ID ──
    b[2] = 0x00;

    // ── X/Y coordinates (16-bit each) ──
    // Axis mapping (matches Touch report in VhfReporter::BuildTouchReports):
    //   Sensor Row (point.y, 40 rows, short edge 166mm) → HID X (16000)
    //   Sensor Col (point.x, 60 cols, long edge 266mm)  → HID Y (25600)
    //
    // Physical orientation (user's screen):
    //   Right-bottom = (col=0, row=0)
    //   Left-top     = (col=60, row=40)
    //
    // HID orientation: (0,0) = top-left, X grows right, Y grows down
    //   Row→HID_X: row=0(bottom) → HID_X=max(bottom), row=40(top) → HID_X=0(top)  ← no invert
    //   Col→HID_Y: col=0(right) → HID_Y=max(right), col=60(left) → HID_Y=0(left)  ← invert
    if (m_lastResult.point.valid) {
        const float offsetRow = static_cast<float>(m_screenOffsetY);
        const float offsetCol = static_cast<float>(m_screenOffsetX);
        const float sensorRangeRow =
            static_cast<float>(m_sensorRows * Asa::kCoorUnit);
        const float sensorRangeCol =
            static_cast<float>(m_sensorCols * Asa::kCoorUnit);
        const float activeRow = sensorRangeRow - offsetRow -
            static_cast<float>(m_screenEndMarginY);
        const float activeCol = sensorRangeCol - offsetCol -
            static_cast<float>(m_screenEndMarginX);

        // point.y = row-direction value, point.x = col-direction value
        float gy = std::clamp(m_lastResult.point.y - offsetRow, 0.0f,
                              std::max(1.0f, activeRow));
        float gx = std::clamp(m_lastResult.point.x - offsetCol, 0.0f,
                              std::max(1.0f, activeCol));

        // Row → HID X (16000): row=0(bottom)→max, row=40(top)→0
        // No inversion needed: row increases upward, HID X=0 is top
        const float normHidX = activeRow > 0.0f
            ? (gy / activeRow) : 0.5f;
        // Col → HID Y (25600): col=0(right)→max, col=60(left)→0
        // Invert: col increases leftward, but HID Y=0 is left
        const float normHidY = activeCol > 0.0f
            ? (1.0f - gx / activeCol) : 0.5f;

        uint16_t vx = static_cast<uint16_t>(std::clamp(
            static_cast<int32_t>(std::lround(normHidX * kHidMaxX)),
            0, static_cast<int32_t>(kHidMaxX)));
        uint16_t vy = static_cast<uint16_t>(std::clamp(
            static_cast<int32_t>(std::lround(normHidY * kHidMaxY)),
            0, static_cast<int32_t>(kHidMaxY)));

        WriteU16Le(b, 3, vx);   // HID X: b[3..4] — from sensor Row
        WriteU16Le(b, 5, vy);   // HID Y: b[5..6] — from sensor Col
    }

    // ── Pressure (16-bit, 0..4095) ──
    uint16_t press = static_cast<uint16_t>(
        std::min(static_cast<uint32_t>(m_lastResult.pressure), 4095u));
    WriteU16Le(b, 7, press);    // Pressure: b[7..8]

    // ── Tilt X/Y (int16, centidegrees: value_deg * 100) ──
    // TSACore GetTiltByCoorDif outputs degrees in [-90, +90].
    // HID descriptor expects centidegrees [-9000, +9000].
    int16_t tiltXCdeg = static_cast<int16_t>(std::clamp(
        static_cast<int32_t>(m_lastResult.point.tiltX) * 100,
        static_cast<int32_t>(-kTiltMax),
        static_cast<int32_t>(kTiltMax)));
    int16_t tiltYCdeg = static_cast<int16_t>(std::clamp(
        static_cast<int32_t>(m_lastResult.point.tiltY) * 100,
        static_cast<int32_t>(-kTiltMax),
        static_cast<int32_t>(kTiltMax)));
    WriteU16Le(b, 9,  static_cast<uint16_t>(tiltXCdeg));  // X Tilt: b[9..10]
    WriteU16Le(b, 11, static_cast<uint16_t>(tiltYCdeg));  // Y Tilt: b[11..12]
}

// ══════════════════════════════════════════════
// GetConfigSchema — Configuration metadata
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
            ConfigParam::Int, const_cast<int*>(&m_liftingTimeout), 1, 30, Cat::General),
        ConfigParam("sp.calibEnabled", "Rolling Avg (5-frame)",
            ConfigParam::Bool, const_cast<bool*>(&m_calibEnabled), Cat::General),

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
            ConfigParam::Bool, const_cast<bool*>(&m_recheckEnabled), Cat::Solver),
        ConfigParam("sp.recheckThBase", "Signal Thresh Base",
            ConfigParam::Int, const_cast<int*>(&m_recheckSignalThreshBase), 10, 500, Cat::Solver),

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
            ConfigParam::Bool, const_cast<bool*>(&m_hpp3NoisePostEnabled), Cat::Filter),
        ConfigParam("sp.hpp3JumpTh", "Jump Threshold",
            ConfigParam::Float, const_cast<float*>(&m_hpp3CoorJumpThreshold), 1.0f, 100.0f, Cat::Filter),

        // === Behavior ===
        ConfigParam("sp.edgeCoorPostEnabled", "Enable Edge Coordinate Process",
            ConfigParam::Bool, const_cast<bool*>(&m_edgeCoorPostEnabled), Cat::Behavior),
        ConfigParam("sp.elcEnabled", "Enable Edge Lift Corrector",
            ConfigParam::Bool, const_cast<bool*>(&m_elcEnabled), Cat::Behavior),
        ConfigParam("sp.crEnabled", "Enable TX2 Coor Reviser",
            ConfigParam::Bool, const_cast<bool*>(&m_coorReviser.enabled), Cat::Behavior),
        ConfigParam("sp.tiltEnabled", "Enable Tilt",
            ConfigParam::Bool, const_cast<bool*>(&m_tiltEnabled), Cat::Behavior),
        ConfigParam("sp.tiltKeepLast", "Keep Last On Invalid",
            ConfigParam::Bool, const_cast<bool*>(&m_tiltKeepLastOnInvalid), Cat::Behavior),
        ConfigParam("sp.tiltDiffAvgWin", "Diff Average Window",
            ConfigParam::Int, const_cast<int*>(&m_tiltDiffAverageWindow), 1, 10, Cat::Behavior),
        ConfigParam("sp.tiltDegCellX", "Degree/Cell X",
            ConfigParam::Float, const_cast<float*>(&m_tiltDegreePerCellX), 1.0f, 30.0f, Cat::Behavior),
        ConfigParam("sp.tiltDegCellY", "Degree/Cell Y",
            ConfigParam::Float, const_cast<float*>(&m_tiltDegreePerCellY), 1.0f, 30.0f, Cat::Behavior),
        ConfigParam("sp.tiltNormLenX", "Norm Len X",
            ConfigParam::Float, const_cast<float*>(&m_tiltNormLenX), 0.5f, 20.0f, Cat::Behavior),
        ConfigParam("sp.tiltNormLenY", "Norm Len Y",
            ConfigParam::Float, const_cast<float*>(&m_tiltNormLenY), 0.5f, 20.0f, Cat::Behavior),
        ConfigParam("sp.tiltMaxDeg", "Max Degree",
            ConfigParam::Int, const_cast<int*>(&m_tiltMaxDegree), 10, 89, Cat::Behavior),
        ConfigParam("sp.tiltJitterDeg", "Jitter Threshold",
            ConfigParam::Int, const_cast<int*>(&m_tiltJitterThresholdDeg), 0, 10, Cat::Behavior),
        ConfigParam("sp.tiltIirOldW", "IIR Old Weight",
            ConfigParam::Float, const_cast<float*>(&m_tiltCoordIirOldWeight), 0.0f, 0.99f, Cat::Behavior),

        // === Output ===
        ConfigParam("sp.pressPolyEnabled", "Polynomial Mapping",
            ConfigParam::Bool, const_cast<bool*>(&m_pressurePolyEnabled), Cat::Output),
        ConfigParam("sp.pressIirQ8", "IIR Weight (Q8)",
            ConfigParam::Int, const_cast<int*>(&m_pressureIirWeightQ8), 16, 255, Cat::Output),
        ConfigParam("sp.pressSeg1Th", "Seg1 Threshold",
            ConfigParam::Int, const_cast<int*>(&m_pressureMapSeg1Threshold), 0, 50, Cat::Output),
        ConfigParam("sp.pressSeg2Th", "Seg2 Threshold",
            ConfigParam::Int, const_cast<int*>(&m_pressureMapSeg2Threshold), 50, 500, Cat::Output),
        ConfigParam("sp.pressGain", "Gain %",
            ConfigParam::Int, const_cast<int*>(&m_pressureMapGainPercent), 10, 500, Cat::Output),
        ConfigParam("sp.pressTailFrames", "Tail Frames",
            ConfigParam::Int, const_cast<int*>(&m_pressureTailFrames), 0, 20, Cat::Output),
        ConfigParam("sp.pressTailMin", "Tail Min",
            ConfigParam::Int, const_cast<int*>(&m_pressureTailMin), 0, 100, Cat::Output),
        ConfigParam("sp.pressTailDecay", "Tail Decay Rate",
            ConfigParam::Int, const_cast<int*>(&m_pressureTailDecay), 1, 200, Cat::Output),
        ConfigParam("sp.slaveHdrBtnOffset", "Button Byte Offset",
            ConfigParam::Int, const_cast<int*>(&m_slaveHdrBtnOffset), 0, 6, Cat::Output),
        // P1: Signal suppression hysteresis
        ConfigParam("sp.sigSuppressEnabled", "Signal Suppress Enabled",
            ConfigParam::Bool, const_cast<bool*>(&m_pressureSignalSuppressEnabled), Cat::Output),
        ConfigParam("sp.sigSuppressEnter", "Signal Suppress Enter Thr",
            ConfigParam::Int, const_cast<int*>(&m_pressureSignalSuppressEnter), 10, 2000, Cat::Output),
        ConfigParam("sp.sigSuppressExit", "Signal Suppress Exit Thr",
            ConfigParam::Int, const_cast<int*>(&m_pressureSignalSuppressExit), 10, 3000, Cat::Output),

        // P3: Pen exit smoothing, TP pattern, CMF
        ConfigParam("sp.exitSmoothEnabled", "Pen Exit Smooth",
            ConfigParam::Bool, const_cast<bool*>(&m_exitSmoothEnabled), Cat::Behavior),
        ConfigParam("sp.cmfEnabled", "CMF Enabled",
            ConfigParam::Bool, const_cast<bool*>(&m_cmfEnabled), Cat::Filter),
        ConfigParam("sp.cmfWindowSize", "CMF Window Size",
            ConfigParam::Int, const_cast<int*>(&m_cmfWindowSize), 1, 8, Cat::Filter),
        ConfigParam("sp.tpPatternEnabled", "TP Pattern Comp Enabled",
            ConfigParam::Bool, const_cast<bool*>(&m_tpPatternCompEnabled), Cat::Solver),

        // === IIR Q8 (TSACore: CoorIIRFilterType, GetIIRCoef) ===
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
// SaveConfig — INI-style serialization
// ══════════════════════════════════════════════
void StylusPipeline::SaveConfig(std::ostream& out) const {
    out << "sp.enableSlaveChecksum="
        << m_enableSlaveChecksum << "\n";
    out << "sp.emitPacketWhenInvalid="
        << m_emitPacketWhenInvalid << "\n";
    out << "sp.buttonReleaseHold="
        << m_buttonReleaseHoldFrames << "\n";
    out << "sp.coordUseTriangle="
        << m_coordSolver.useTriangle << "\n";
    out << "sp.coordEdgeCompBit3="
        << m_coordSolver.edgeCompBit3 << "\n";
    out << "sp.lfEnabled="
        << m_linearFilter.enabled << "\n";
    out << "sp.lfMinFitLen="
        << m_linearFilter.minFitLength << "\n";
    out << "sp.lfEnterResidual="
        << m_linearFilter.enterResidualThreshold << "\n";
    out << "sp.lfExitDeviation="
        << m_linearFilter.exitDeviation << "\n";
    out << "sp.lfPerpConstraint="
        << m_linearFilter.perpConstraint << "\n";
    out << "sp.3ptAvgEnabled="
        << m_postProcessor.enable3PointAvg << "\n";
    out << "sp.jitterEnabled="
        << m_postProcessor.enableJitter << "\n";
    out << "sp.jitterEdgeDim1="
        << m_postProcessor.jitterEdgeParamDim1 << "\n";
    out << "sp.jitterEdgeDim2="
        << m_postProcessor.jitterEdgeParamDim2 << "\n";
    out << "sp.jitterCenterDim1="
        << m_postProcessor.jitterCenterParamDim1 << "\n";
    out << "sp.jitterCenterDim2="
        << m_postProcessor.jitterCenterParamDim2 << "\n";
    out << "sp.crEnabled="
        << m_coorReviser.enabled << "\n";
    out << "sp.elcEnabled="
        << m_elcEnabled << "\n";
    // P0: Pitch Compensation
    out << "sp.pitchCompDim1Enabled="
        << m_coordSolver.pitchCompDim1.enabled << "\n";
    out << "sp.pitchCompDim2Enabled="
        << m_coordSolver.pitchCompDim2.enabled << "\n";
    // P0: Gravity
    out << "sp.gravityNoiseFloor="
        << m_coordSolver.gravityNoiseFloor << "\n";
    out << "sp.gravityFictEdge="
        << m_coordSolver.gravityFictitiousEdge << "\n";
    // Tilt
    out << "sp.tiltEnabled=" << m_tiltEnabled << "\n";
    out << "sp.tiltKeepLast="
        << m_tiltKeepLastOnInvalid << "\n";
    out << "sp.tiltDiffAvgWin="
        << m_tiltDiffAverageWindow << "\n";
    out << "sp.tiltDegCellX="
        << m_tiltDegreePerCellX << "\n";
    out << "sp.tiltDegCellY="
        << m_tiltDegreePerCellY << "\n";
    out << "sp.tiltNormLenX="
        << m_tiltNormLenX << "\n";
    out << "sp.tiltNormLenY="
        << m_tiltNormLenY << "\n";
    out << "sp.tiltMaxDeg="
        << m_tiltMaxDegree << "\n";
    out << "sp.tiltJitterDeg="
        << m_tiltJitterThresholdDeg << "\n";
    out << "sp.tiltIirOldW="
        << m_tiltCoordIirOldWeight << "\n";
    // Pressure
    out << "sp.pressPolyEnabled="
        << m_pressurePolyEnabled << "\n";
    out << "sp.pressIirQ8="
        << m_pressureIirWeightQ8 << "\n";
    out << "sp.pressSeg1Th="
        << m_pressureMapSeg1Threshold << "\n";
    out << "sp.pressSeg2Th="
        << m_pressureMapSeg2Threshold << "\n";
    out << "sp.pressGain="
        << m_pressureMapGainPercent << "\n";
    out << "sp.pressTailFrames="
        << m_pressureTailFrames << "\n";
    out << "sp.pressTailMin="
        << m_pressureTailMin << "\n";
    out << "sp.pressTailDecay="
        << m_pressureTailDecay << "\n";
    // P1: Signal suppression hysteresis
    out << "sp.sigSuppressEnabled="
        << m_pressureSignalSuppressEnabled << "\n";
    out << "sp.sigSuppressEnter="
        << m_pressureSignalSuppressEnter << "\n";
    out << "sp.sigSuppressExit="
        << m_pressureSignalSuppressExit << "\n";
    // HPP3 Noise
    out << "sp.hpp3NoiseEnabled="
        << m_hpp3NoisePostEnabled << "\n";
    out << "sp.hpp3JumpTh="
        << m_hpp3CoorJumpThreshold << "\n";
    // Recheck
    out << "sp.recheckEnabled="
        << m_recheckEnabled << "\n";
    out << "sp.recheckThBase="
        << m_recheckSignalThreshBase << "\n";
    // Pen Lifecycle
    out << "sp.liftingTimeout="
        << m_liftingTimeout << "\n";
    // Calibration
    out << "sp.calibEnabled="
        << m_calibEnabled << "\n";
    // IIR Q8 params
    out << "sp.iirStillLo=" << m_postProcessor.stillIirLow << "\n";
    out << "sp.iirStillHi=" << m_postProcessor.stillIirHigh << "\n";
    out << "sp.iirMoveLo=" << m_postProcessor.movingIirLow << "\n";
    out << "sp.iirMoveHi=" << m_postProcessor.movingIirHigh << "\n";
    out << "sp.iirDivisorN=" << m_postProcessor.iirDivisorN << "\n";
    out << "sp.iirHighSpdThr=" << m_postProcessor.highSpeedThr << "\n";
    out << "sp.iirDirHalve=" << m_postProcessor.enableDirectionalHalve << "\n";
    out << "sp.pitchMapEnabled=" << m_pitchMapEnabled << "\n";
    // Filter mode
    out << "sp.filterMode=" << m_filterMode << "\n";
    // IIR speed thresholds
    out << "sp.iirStillLowThr=" << m_postProcessor.stillLowSpeedThr << "\n";
    out << "sp.iirMoveLowThr=" << m_postProcessor.movingLowSpeedThr << "\n";
    out << "sp.iirMotionFrames=" << m_postProcessor.motionDetectFrames << "\n";
    out << "sp.iirSkipFrames=" << m_postProcessor.iirSkipFrames << "\n";
    // 1-Euro params
    out << "sp.1eur.minCutoff=" << m_oneEuroFilter.minCutoffF << "\n";
    out << "sp.1eur.beta=" << m_oneEuroFilter.betaF << "\n";
    out << "sp.1eur.dCutoff=" << m_oneEuroFilter.dCutoffF << "\n";
    out << "sp.1eur.sampleRate=" << m_oneEuroFilter.sampleRate << "\n";
}

// ══════════════════════════════════════════════
// LoadConfig — INI-style deserialization
// ══════════════════════════════════════════════
void StylusPipeline::LoadConfig(
        const std::string& key,
        const std::string& value) {
    auto toBool = [](const std::string& v) { return v == "1"; };
    auto toInt = [](const std::string& v) {
        try { return std::stoi(v); }
        catch (...) { return 0; }
    };
    auto toFloat = [](const std::string& v) {
        try { return std::stof(v); }
        catch (...) { return 0.0f; }
    };

    if (key == "sp.enableSlaveChecksum")
        m_enableSlaveChecksum = toBool(value);
    else if (key == "sp.emitPacketWhenInvalid")
        m_emitPacketWhenInvalid = toBool(value);
    else if (key == "sp.buttonReleaseHold")
        m_buttonReleaseHoldFrames = toInt(value);
    else if (key == "sp.coordUseTriangle")
        m_coordSolver.useTriangle = toBool(value);
    else if (key == "sp.coordEdgeCompBit3")
        m_coordSolver.edgeCompBit3 = toBool(value);
    else if (key == "sp.lfEnabled")
        m_linearFilter.enabled = toBool(value);
    else if (key == "sp.lfMinFitLen")
        m_linearFilter.minFitLength = toInt(value);
    else if (key == "sp.lfEnterResidual")
        m_linearFilter.enterResidualThreshold = toFloat(value);
    else if (key == "sp.lfExitDeviation")
        m_linearFilter.exitDeviation = toFloat(value);
    else if (key == "sp.lfPerpConstraint")
        m_linearFilter.perpConstraint = toFloat(value);
    else if (key == "sp.3ptAvgEnabled")
        m_postProcessor.enable3PointAvg = toBool(value);
    else if (key == "sp.jitterEnabled")
        m_postProcessor.enableJitter = toBool(value);
    else if (key == "sp.jitterEdgeDim1")
        m_postProcessor.jitterEdgeParamDim1 = toInt(value);
    else if (key == "sp.jitterEdgeDim2")
        m_postProcessor.jitterEdgeParamDim2 = toInt(value);
    else if (key == "sp.jitterCenterDim1")
        m_postProcessor.jitterCenterParamDim1 = toInt(value);
    else if (key == "sp.jitterCenterDim2")
        m_postProcessor.jitterCenterParamDim2 = toInt(value);
    else if (key == "sp.crEnabled")
        m_coorReviser.enabled = toBool(value);
    else if (key == "sp.elcEnabled")
        m_elcEnabled = toBool(value);
    // P0: Pitch Compensation
    else if (key == "sp.pitchCompDim1Enabled")
        m_coordSolver.pitchCompDim1.enabled = toBool(value);
    else if (key == "sp.pitchCompDim2Enabled")
        m_coordSolver.pitchCompDim2.enabled = toBool(value);
    // P0: Gravity
    else if (key == "sp.gravityNoiseFloor")
        m_coordSolver.gravityNoiseFloor = toInt(value);
    else if (key == "sp.gravityFictEdge")
        m_coordSolver.gravityFictitiousEdge = toBool(value);
    // Tilt
    else if (key == "sp.tiltEnabled")
        m_tiltEnabled = toBool(value);
    else if (key == "sp.tiltKeepLast")
        m_tiltKeepLastOnInvalid = toBool(value);
    else if (key == "sp.tiltDiffAvgWin")
        m_tiltDiffAverageWindow = toInt(value);
    else if (key == "sp.tiltDegCellX")
        m_tiltDegreePerCellX = toFloat(value);
    else if (key == "sp.tiltDegCellY")
        m_tiltDegreePerCellY = toFloat(value);
    else if (key == "sp.tiltNormLenX")
        m_tiltNormLenX = toFloat(value);
    else if (key == "sp.tiltNormLenY")
        m_tiltNormLenY = toFloat(value);
    else if (key == "sp.tiltMaxDeg")
        m_tiltMaxDegree = toInt(value);
    else if (key == "sp.tiltJitterDeg")
        m_tiltJitterThresholdDeg = toInt(value);
    else if (key == "sp.tiltIirOldW")
        m_tiltCoordIirOldWeight = toFloat(value);
    // Pressure
    else if (key == "sp.pressPolyEnabled")
        m_pressurePolyEnabled = toBool(value);
    else if (key == "sp.pressIirQ8")
        m_pressureIirWeightQ8 = std::clamp(toInt(value), 16, 255);
    else if (key == "sp.pressSeg1Th")
        m_pressureMapSeg1Threshold = toInt(value);
    else if (key == "sp.pressSeg2Th")
        m_pressureMapSeg2Threshold = toInt(value);
    else if (key == "sp.pressGain")
        m_pressureMapGainPercent = toInt(value);
    else if (key == "sp.pressTailFrames")
        m_pressureTailFrames = toInt(value);
    else if (key == "sp.pressTailMin")
        m_pressureTailMin = toInt(value);
    else if (key == "sp.pressTailDecay")
        m_pressureTailDecay = toInt(value);
    // P1: Signal suppression hysteresis
    else if (key == "sp.sigSuppressEnabled")
        m_pressureSignalSuppressEnabled = toBool(value);
    else if (key == "sp.sigSuppressEnter")
        m_pressureSignalSuppressEnter = toInt(value);
    else if (key == "sp.sigSuppressExit")
        m_pressureSignalSuppressExit = toInt(value);
    // HPP3 Noise
    else if (key == "sp.hpp3NoiseEnabled")
        m_hpp3NoisePostEnabled = toBool(value);
    else if (key == "sp.hpp3JumpTh")
        m_hpp3CoorJumpThreshold = toFloat(value);
    // Recheck
    else if (key == "sp.recheckEnabled")
        m_recheckEnabled = toBool(value);
    else if (key == "sp.recheckThBase")
        m_recheckSignalThreshBase = toInt(value);
    // Pen Lifecycle
    else if (key == "sp.liftingTimeout")
        m_liftingTimeout = toInt(value);
    // Calibration
    else if (key == "sp.calibEnabled")
        m_calibEnabled = toBool(value);
    // P3: Exit smooth, CMF, TP pattern
    else if (key == "sp.exitSmoothEnabled")
        m_exitSmoothEnabled = toBool(value);
    else if (key == "sp.cmfEnabled")
        m_cmfEnabled = toBool(value);
    else if (key == "sp.cmfWindowSize")
        m_cmfWindowSize = toInt(value);
    else if (key == "sp.tpPatternEnabled")
        m_tpPatternCompEnabled = toBool(value);
    // IIR Q8 params
    else if (key == "sp.iirStillLo")
        m_postProcessor.stillIirLow = toInt(value);
    else if (key == "sp.iirStillHi")
        m_postProcessor.stillIirHigh = toInt(value);
    else if (key == "sp.iirMoveLo")
        m_postProcessor.movingIirLow = toInt(value);
    else if (key == "sp.iirMoveHi")
        m_postProcessor.movingIirHigh = toInt(value);
    else if (key == "sp.iirDivisorN")
        m_postProcessor.iirDivisorN = toInt(value);
    else if (key == "sp.iirHighSpdThr")
        m_postProcessor.highSpeedThr = toInt(value);
    else if (key == "sp.iirDirHalve")
        m_postProcessor.enableDirectionalHalve = toBool(value);
    else if (key == "sp.pitchMapEnabled")
        m_pitchMapEnabled = toBool(value);
    // Filter mode
    else if (key == "sp.filterMode")
        m_filterMode = toInt(value);
    // IIR speed thresholds
    else if (key == "sp.iirStillLowThr")
        m_postProcessor.stillLowSpeedThr = toInt(value);
    else if (key == "sp.iirMoveLowThr")
        m_postProcessor.movingLowSpeedThr = toInt(value);
    else if (key == "sp.iirMotionFrames")
        m_postProcessor.motionDetectFrames = toInt(value);
    else if (key == "sp.iirSkipFrames")
        m_postProcessor.iirSkipFrames = toInt(value);
    // 1-Euro params
    else if (key == "sp.1eur.minCutoff")
        m_oneEuroFilter.minCutoffF = toFloat(value);
    else if (key == "sp.1eur.beta")
        m_oneEuroFilter.betaF = toFloat(value);
    else if (key == "sp.1eur.dCutoff")
        m_oneEuroFilter.dCutoffF = toFloat(value);
    else if (key == "sp.1eur.sampleRate")
        m_oneEuroFilter.sampleRate = toInt(value);
}

void StylusPipeline::SetBtMcuPressure(uint16_t p) {
    auto nowObj = std::chrono::steady_clock::now();
    uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          nowObj.time_since_epoch()).count();

    std::lock_guard<std::mutex> lock(m_btPressureMutex);
    m_btPressureHistory.push_back({now_ms, p});
    if (m_btPressureHistory.size() > 20) {
        m_btPressureHistory.pop_front();
    }
}

} // namespace Engine
