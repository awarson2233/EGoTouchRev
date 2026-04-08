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
        if (!m_prevValid) { m_postProcessor.Reset(); ResetTilt(); ResetCalibration(); m_coorReviser.Reset(); m_linearFilter.Reset(); }
        m_prevValid = false;
        UpdatePenLifecycle(false, false);
        if (m_emitPacketWhenInvalid) BuildStylusPacket(outPacket);
        m_prevStatus = m_lastResult.status;
        return false;
    }

    // Log anchor + computed full coordinate for debug (throttled)
    {
        static int sAnchorLogCount = 0;
        if ((sAnchorLogCount++ % 60) == 0) {
            LOG_TRACE("Engine", __func__, "Anchor", "anchor=({},{}) grid_center={} sensor=({} rows, {} cols)",  m_gridData.tx1.anchorRow, m_gridData.tx1.anchorCol, m_gridData.tx1.grid[4][4], m_sensorRows, m_sensorCols);
        }
    }

    // 5. Peak detection
    auto peak = m_peakDetector.FindPeak(m_gridData.tx1.grid);
    if (!peak.valid) {
        m_lastResult.point.valid = false;
        m_lastResult.pipelineStage = 3; // No peak found
        m_prevValid = false;
        UpdatePenLifecycle(false, false);
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
        if (m_emitPacketWhenInvalid) BuildStylusPacket(outPacket);
        m_prevStatus = m_lastResult.status;
        return false;
    }

    // ── 7b. LOCAL → GLOBAL coordinate conversion ──
    // rawCoor.dim1/dim2 are local to the 9×9 window (range [0, 9*1024)).
    m_lastResult.point.tx1X = static_cast<float>(rawCoor.dim1) / Asa::kCoorUnit;
    m_lastResult.point.tx1Y = static_cast<float>(rawCoor.dim2) / Asa::kCoorUnit;

    const int32_t centerOff =
        m_anchorCenterOffset * Asa::kCoorUnit;
    rawCoor.dim1 += static_cast<int32_t>(m_gridData.tx1.anchorCol) *
                    Asa::kCoorUnit - centerOff;
    rawCoor.dim2 += static_cast<int32_t>(m_gridData.tx1.anchorRow) *
                    Asa::kCoorUnit - centerOff;

    // 8. HPP3 Noise post-process
    if (m_hpp3NoisePostEnabled && ApplyHpp3NoisePost(rawCoor)) {
        m_lastResult.point.valid = false;
        m_lastResult.pipelineStage = 5; // Noise rejected
        m_prevValid = false;
        UpdatePenLifecycle(false, false);
        if (m_emitPacketWhenInvalid) BuildStylusPacket(outPacket);
        m_prevStatus = m_lastResult.status;
        return false;
    }

    // 9. Post-processing chain (IIR, jitter) — now on GLOBAL coordinates
    //    P1: pass hover/edge flags for 3-mode IIR coefficient selection
    const bool isHover = (m_lastResult.pressure == 0);
    const bool isEdge  = m_edgeLiftCorrector.IsInEdgeRegion(
        static_cast<float>(rawCoor.dim1), static_cast<float>(rawCoor.dim2),
        m_sensorRows, m_sensorCols);
    auto postCoor = m_postProcessor.Process(rawCoor, isHover, isEdge);

    // 9b. P2: LinearFilter (7-state line detection)
    postCoor = m_linearFilter.Process(postCoor, m_lastResult.pressure);

    // 9c. P2: CoorReviser (TX2 dual-frequency revision)
    if (m_coorReviser.enabled && m_gridData.tx2.valid) {
        auto tx2Peak = m_peakDetector.FindPeak(m_gridData.tx2.grid);
        if (tx2Peak.valid) {
            auto tx2Proj = m_peakDetector.ProjectTo1D(
                m_gridData.tx2.grid, tx2Peak);
            auto tx2Coor = m_coordSolver.Solve(tx2Proj);
            if (tx2Coor.valid) {
                m_lastResult.point.tx2X = static_cast<float>(tx2Coor.dim1) / Asa::kCoorUnit;
                m_lastResult.point.tx2Y = static_cast<float>(tx2Coor.dim2) / Asa::kCoorUnit;

                // Convert TX2 to global too
                tx2Coor.dim1 += static_cast<int32_t>(m_gridData.tx2.anchorCol) *
                                Asa::kCoorUnit - centerOff;
                tx2Coor.dim2 += static_cast<int32_t>(m_gridData.tx2.anchorRow) *
                                Asa::kCoorUnit - centerOff;
            }
            postCoor = m_coorReviser.Revise(postCoor, tx2Coor);
        }
    }

    // 10. Calibration (Phase 6)
    auto finalCoor = m_calibEnabled ? ApplyCalibration(postCoor) : postCoor;
    m_lastResult.pipelineStage = 0; // Success

    m_lastResult.point.valid = finalCoor.valid;
    // Coordinates are now GLOBAL — use directly for point.x/y
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
    m_dbg.isHover   = isHover;
    m_dbg.isEdge    = isEdge;
    m_dbg.tiltDiffX = m_prevTiltDiffX;
    m_dbg.tiltDiffY = m_prevTiltDiffY;
    m_dbg.peakSignal = m_lastResult.signalX;

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

    // 11. TX2 for tilt
    if (m_tiltEnabled && m_gridData.tx2.valid) {
        auto tx2Peak = m_peakDetector.FindPeak(m_gridData.tx2.grid);
        if (tx2Peak.valid) {
            auto tx2Proj = m_peakDetector.ProjectTo1D(
                m_gridData.tx2.grid, tx2Peak);
            auto tx2Coor = m_coordSolver.Solve(tx2Proj);
            if (tx2Coor.valid) {
                // Convert TX2 to global for consistent tilt diff
                tx2Coor.dim1 += static_cast<int32_t>(m_gridData.tx2.anchorCol) *
                                Asa::kCoorUnit - centerOff;
                tx2Coor.dim2 += static_cast<int32_t>(m_gridData.tx2.anchorRow) *
                                Asa::kCoorUnit - centerOff;
                SolveTilt(finalCoor, tx2Coor);
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
        static int sPressLogCount = 0;
        if ((sPressLogCount++ % 120) == 0) {
            LOG_DEBUG("Engine", __func__, "Pressure", "btMcu={} active={}",  btPress, finalCoor.valid);
        }
        SolvePressure(btPress, finalCoor.valid,
                      static_cast<int>(m_lastResult.signalX));
    }

    // 13. Button (from slave header)
    if (hdr.valid)
        m_lastResult.status = UpdateButtonState(
            hdr.button, finalCoor.valid);

    // 14. Pen lifecycle
    UpdatePenLifecycle(finalCoor.valid, m_lastResult.pressure > 0);

    // 15. Build packet + save state for next frame
    m_prevPointX = m_lastResult.point.x;
    m_prevPointY = m_lastResult.point.y;
    m_prevValid = finalCoor.valid;
    m_prevStatus = m_lastResult.status;
    BuildStylusPacket(outPacket);
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
        int signalStrength) {
    if (!active) {
        m_lastResult.pressure = 0;
        m_prevPressure = 0;
        m_pressureTailCounter = 0;
        return;
    }

    // P2: Signal strength suppression
    // When TX1 signal is too weak, force pressure to 0 to prevent ghost traces
    if (m_pressureSignalSuppressEnabled &&
        signalStrength < m_pressureSignalSuppressThreshold &&
        signalStrength > 0) {
        m_lastResult.pressure = 0;
        m_prevPressure = 0;
        return;
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
        ConfigParam("sp.calibEnabled", "Enable Calibration",
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
            ConfigParam::Int, const_cast<int*>(&m_pressureIirWeightQ8), 1, 255, Cat::Output),
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
        m_pressureIirWeightQ8 = toInt(value);
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
