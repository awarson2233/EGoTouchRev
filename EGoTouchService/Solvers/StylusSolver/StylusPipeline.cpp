#include "StylusPipeline.h"
#include "Logger.h"
#include <algorithm>
#include <cstring>
#include <ostream>

namespace Solvers {

// ── Helpers ──
namespace {
inline uint16_t ReadU16Le(const uint8_t* p) {
    return static_cast<uint16_t>(p[0]) |
           (static_cast<uint16_t>(p[1]) << 8);
}

struct StylusSignalMetrics {
    uint16_t signalX = 0;
    uint16_t signalY = 0;
    uint16_t maxRawPeak = 0;
    uint16_t tx1Composite = 0;
    uint16_t tx2Composite = 0;
    bool dim1EdgeActive = false;
    bool dim2EdgeActive = false;
    uint16_t dim1EdgeSignal = 0;
    uint16_t dim2EdgeSignal = 0;
};

struct StylusRecheckContext {
    bool stable = false;
    bool active = false;
    bool overlapLike = false;
    uint16_t finalThreshold = 0;
    uint16_t sustainThreshold = 0;
};

inline uint16_t ClampU16(int value) {
    return static_cast<uint16_t>(std::clamp(value, 0, 0xFFFF));
}

inline uint16_t NormalizeProjectionPeak(const int32_t* signal, int peakIdx, int span) {
    if (peakIdx < 0 || span <= 0) {
        return 0;
    }
    const int32_t peak = signal[peakIdx];
    return ClampU16(static_cast<int>(peak / std::max(1, span)));
}

inline bool IsAxisEdgeActive(int32_t coor, int sensorDim) {
    const int32_t margin = Asa::kCoorUnit;
    const int32_t maxCoor = sensorDim * Asa::kCoorUnit;
    return (coor < margin) || (coor > maxCoor - margin);
}

inline uint16_t BuildProjectionComposite(const Asa::AsaProjection& proj) {
    const uint16_t dim1ProjectionPeak =
        NormalizeProjectionPeak(proj.dim1, proj.peakIdxDim1, proj.spanDim1);
    const uint16_t dim2ProjectionPeak =
        NormalizeProjectionPeak(proj.dim2, proj.peakIdxDim2, proj.spanDim2);
    return ClampU16(
        (static_cast<int>(dim1ProjectionPeak) + static_cast<int>(dim2ProjectionPeak)) / 2);
}

inline StylusSignalMetrics BuildSignalMetrics(const Asa::AsaProjection& proj,
                                             const Asa::AsaCoorResult& rawCoor,
                                             uint16_t tx1PeakSignal,
                                             uint16_t tx2PeakSignal,
                                             int sensorCols,
                                             int sensorRows) {
    StylusSignalMetrics metrics{};
    metrics.signalX = tx1PeakSignal;
    metrics.signalY = tx2PeakSignal;
    metrics.maxRawPeak = std::max(metrics.signalX, metrics.signalY);

    const uint16_t dim1ProjectionPeak =
        NormalizeProjectionPeak(proj.dim1, proj.peakIdxDim1, proj.spanDim1);
    const uint16_t dim2ProjectionPeak =
        NormalizeProjectionPeak(proj.dim2, proj.peakIdxDim2, proj.spanDim2);
    metrics.tx1Composite = BuildProjectionComposite(proj);
    metrics.tx2Composite = tx2PeakSignal;
    metrics.dim1EdgeActive = IsAxisEdgeActive(rawCoor.dim1, sensorCols);
    metrics.dim2EdgeActive = IsAxisEdgeActive(rawCoor.dim2, sensorRows);
    metrics.dim1EdgeSignal = dim1ProjectionPeak;
    metrics.dim2EdgeSignal = dim2ProjectionPeak;
    return metrics;
}

inline StylusRecheckContext BuildRecheckContext(const StylusSignalMetrics& metrics,
                                                bool coordValid,
                                                bool recentlyWriting,
                                                int baseThreshold,
                                                int multiThreshold) {
    StylusRecheckContext ctx{};
    ctx.finalThreshold = ClampU16(baseThreshold);
    ctx.sustainThreshold = ClampU16(multiThreshold);
    ctx.overlapLike = coordValid && metrics.signalY > 0 &&
                      metrics.tx2Composite > metrics.tx1Composite &&
                      metrics.signalX < ctx.sustainThreshold;
    ctx.stable = coordValid && metrics.tx1Composite >= ctx.finalThreshold;
    ctx.active = coordValid && (recentlyWriting || metrics.tx1Composite >= ctx.sustainThreshold);
    return ctx;
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

bool StylusPipeline::HasCurrentStylusSignal(
        std::span<const uint8_t> rawData) const {
    if (rawData.size() < (kSlaveWordOffset + 4)) {
        return true;
    }

    const uint8_t* anchor = rawData.data() + kSlaveWordOffset;
    const uint16_t anchorRow = ReadU16Le(anchor);
    const uint16_t anchorCol = ReadU16Le(anchor + 2);
    return !((anchorRow & 0xFFu) == 0x00FFu &&
             (anchorCol & 0xFFu) == 0x00FFu);
}

void StylusPipeline::ClosePressureGate(uint32_t latestBtSeq) {
    m_pressureSolver.Reset();
    m_pressureGateOpen = false;
    m_waitingFreshBtAfterRelease = true;
    m_blockedBtSeqFloor = std::max(m_blockedBtSeqFloor, latestBtSeq);
}

void StylusPipeline::TrackBlockedBtSeq(uint32_t latestBtSeq) {
    if (!m_waitingFreshBtAfterRelease) {
        return;
    }
    m_blockedBtSeqFloor = std::max(m_blockedBtSeqFloor, latestBtSeq);
}

bool StylusPipeline::ProcessNoStylusFrame(
        std::span<const uint8_t> rawData,
        StylusPacket& outPacket) {
    m_lastResult = StylusFrameData{};
    outPacket = StylusPacket{};
    m_lastResult.slaveValid = (rawData.size() >= (kSlaveHeaderBytes + 4));
    m_lastResult.pipelineStage = 2;

    if (rawData.size() >= kSlaveHeaderBytes) {
        const uint8_t* p = rawData.data();
        std::memcpy(m_rawSlaveHdr, p, kSlaveHeaderBytes);
        m_lastResult.status = ReadU16Le(p);
    }

    m_postProcessor.Reset();
    m_tiltSolver.Reset();
    m_coorReviser.Reset();
    m_linearFilter.Reset();
    const auto latestBt = m_btPressBuf.ReadLatest();
    const bool hadWriting = (m_penStateMachine.GetState() == Asa::PenStateMachine::State::Writing);
    if (hadWriting) {
        ClosePressureGate(latestBt.seq);
    } else {
        TrackBlockedBtSeq(latestBt.seq);
        m_pressureSolver.Reset();
        m_pressureGateOpen = false;
    }
    m_noPressInkGate.Reset();
    m_noiseGate.Reset();
    m_hasLastGoodFrame = false;
    m_prevValid = false;

    Asa::PenFrameEvidence evidence{};
    evidence.noSignal = true;
    (void)m_penStateMachine.Update(evidence);
    m_lastResult.animState = m_penStateMachine.GetAnimState();
    if (m_emitPacketWhenInvalid) {
        m_packetBuilder.Build(m_lastResult,
            m_emitPacketWhenInvalid, outPacket);
    }
    m_prevStatus = m_lastResult.status;
    return false;
}

// ══════════════════════════════════════════════
// Process(HeatmapFrame&) — unified entry (mirrors TouchPipeline)
// ══════════════════════════════════════════════
bool StylusPipeline::Process(HeatmapFrame& frame) {
    StylusPacket pkt{};
    if (frame.rawLen >= kMasterBytes + kSlaveFrameBytes) {
        ProcessRaw(
            std::span<const uint8_t>(
                frame.rawPtr + kMasterBytes, kSlaveFrameBytes),
            pkt);
    }
    // Write results directly into the frame (no manual injection needed)
    frame.stylus = m_lastResult;
    frame.stylus.packet = pkt;
#ifdef _DEBUG
    frame.stylus.diag = m_dbg;
#endif
    return pkt.valid;
}

// ══════════════════════════════════════════════
// ProcessRaw — raw-bytes pipeline v2 (linear orchestrator)
//
// Phase 1:   Input parsing
// Phase 2:   TX1/TX2 coordinate solve → GLOBAL
// Phase 2.5: Pressure + StateMachine → MotionProfile
// Phase 3:   Post-processing (LinearFilter → CoorReviser → IIR → Jitter)
// Phase 4:   Edge compensation + output
// ══════════════════════════════════════════════
bool StylusPipeline::ProcessRaw(
        std::span<const uint8_t> rawData,
        StylusPacket& outPacket) {
    m_lastResult = StylusFrameData{};
    outPacket = StylusPacket{};
#ifdef _DEBUG
    m_dbg.sigSuppressActive = false;
#endif

    if (!HasCurrentStylusSignal(rawData)) {
        const size_t required = kSlaveHeaderBytes + kSlaveWordCount * 2;
        if (rawData.size() >= required) {
            bool checksumOk = true;
            if (m_enableSlaveChecksum) {
                uint16_t cs = 0;
                checksumOk = ValidateChecksum16(
                    rawData.data() + kSlaveHeaderBytes,
                    kSlaveWordCount, cs);
            }
            if (checksumOk) {
                return ProcessNoStylusFrame(rawData, outPacket);
            }
        }
    }

    // ── Phase 1: Input Parsing ──

    // 1. Parse slave words
    std::array<uint16_t, kSlaveWordCount> sw{};
    if (!ParseSlaveWords(rawData, sw)) {
        m_lastResult.slaveValid = false;
        m_lastResult.pipelineStage = 1;
        const auto latestBt = m_btPressBuf.ReadLatest();
        if (m_penStateMachine.GetState() == Asa::PenStateMachine::State::Writing) {
            ClosePressureGate(latestBt.seq);
        } else {
            TrackBlockedBtSeq(latestBt.seq);
            m_pressureSolver.Reset();
            m_pressureGateOpen = false;
        }
        m_noPressInkGate.Reset();
        m_postProcessor.Reset();
        m_coorReviser.Reset();
        m_linearFilter.Reset();
        m_prevValid = false;
        Asa::PenFrameEvidence evidence{};
        evidence.noSignal = true;
        auto profile = m_penStateMachine.Update(evidence);
        (void)profile;
        m_lastResult.animState = m_penStateMachine.GetAnimState();
        if (m_emitPacketWhenInvalid) {
            outPacket.valid = true; outPacket.reportId = 0x08;
            outPacket.length = 17; outPacket.bytes.fill(0);
            outPacket.bytes[0] = 0x08;
        }
        return false;
    }
    m_lastResult.slaveValid = true;

    // 2. Extract dual 9x9 grids
    m_gridData = Asa::ExtractGridFromSlaveWords(
        sw.data(), static_cast<int>(sw.size()));

    // 3. Slave header status
    if (rawData.size() >= kSlaveHeaderBytes) {
        const uint8_t* p = rawData.data();
        std::memcpy(m_rawSlaveHdr, p, kSlaveHeaderBytes);
        m_lastResult.status = ReadU16Le(p);
    }

    // 4. TX1 validity check
    if (!m_gridData.tx1.valid) {
        m_lastResult.point.valid = false;
        m_lastResult.pipelineStage = 2;
        const auto latestBt = m_btPressBuf.ReadLatest();
        m_postProcessor.Reset();
        m_tiltSolver.Reset();
        m_coorReviser.Reset(); m_linearFilter.Reset();
        if (m_penStateMachine.GetState() == Asa::PenStateMachine::State::Writing) {
            ClosePressureGate(latestBt.seq);
        } else {
            TrackBlockedBtSeq(latestBt.seq);
            m_pressureSolver.Reset();
            m_pressureGateOpen = false;
        }
        m_noiseGate.Reset();
        m_hasLastGoodFrame = false;
        m_prevValid = false;
                Asa::PenFrameEvidence evidence{};
        evidence.noSignal = true;
        auto profile = m_penStateMachine.Update(evidence);
        (void)profile;
        m_lastResult.animState = m_penStateMachine.GetAnimState();
        if (m_emitPacketWhenInvalid)
            m_packetBuilder.Build(m_lastResult,
                m_emitPacketWhenInvalid, outPacket);
        m_prevStatus = m_lastResult.status;
        return false;
    }

    int16_t preCmfTx1Grid[Asa::kGridDim][Asa::kGridDim]{};
    std::memcpy(preCmfTx1Grid, m_gridData.tx1.grid, sizeof(preCmfTx1Grid));

    // 4b. Common-mode filtering
    m_cmfFilter.Apply(m_gridData.tx1.grid);
    if (m_gridData.tx2.valid)
        m_cmfFilter.Apply(m_gridData.tx2.grid);

    // ── Phase 2: Coordinate Solving ──

    // 5. Peak detection
    const auto tx1Analysis = m_peakDetector.AnalyzePeakAndProjection(m_gridData.tx1.grid);
    const auto& peak = tx1Analysis.peak;
    if (!peak.valid) {
        m_lastResult.point.valid = false;
        m_lastResult.pipelineStage = 3;
        if (m_penStateMachine.GetState() == Asa::PenStateMachine::State::Writing) {
            ClosePressureGate(m_btPressBuf.ReadLatest().seq);
        } else {
            m_pressureSolver.Reset();
            m_pressureGateOpen = false;
        }
        m_noPressInkGate.Reset();
        m_prevValid = false;
                Asa::PenFrameEvidence evidence{};
        evidence.noSignal = true;
        auto profile = m_penStateMachine.Update(evidence);
        (void)profile;
        m_lastResult.animState = m_penStateMachine.GetAnimState();
        if (m_emitPacketWhenInvalid)
            m_packetBuilder.Build(m_lastResult,
                m_emitPacketWhenInvalid, outPacket);
        m_prevStatus = m_lastResult.status;
        return false;
    }

    // 6. 1D projection
    const auto& proj = tx1Analysis.projection;

    // 7. Coordinate interpolation
    auto rawCoor = m_coordSolver.Solve(
        proj,
        static_cast<int>(m_gridData.tx1.anchorRow),
        static_cast<int>(m_gridData.tx1.anchorCol),
        m_sensorRows, m_sensorCols,
        m_anchorCenterOffset);
    if (!rawCoor.valid) {
        m_lastResult.point.valid = false;
        m_lastResult.pipelineStage = 4;
        if (m_penStateMachine.GetState() == Asa::PenStateMachine::State::Writing) {
            ClosePressureGate(m_btPressBuf.ReadLatest().seq);
        } else {
            m_pressureSolver.Reset();
            m_pressureGateOpen = false;
        }
        m_noPressInkGate.Reset();
        m_prevValid = false;
                Asa::PenFrameEvidence evidence{};
        evidence.noSignal = true;
        auto profile = m_penStateMachine.Update(evidence);
        (void)profile;
        m_lastResult.animState = m_penStateMachine.GetAnimState();
        if (m_emitPacketWhenInvalid)
            m_packetBuilder.Build(m_lastResult,
                m_emitPacketWhenInvalid, outPacket);
        m_prevStatus = m_lastResult.status;
        return false;
    }

    // 7b. LOCAL coordinate diagnostics
    m_lastResult.point.tx1X = static_cast<float>(rawCoor.dim1) / Asa::kCoorUnit;
    m_lastResult.point.tx1Y = static_cast<float>(rawCoor.dim2) / Asa::kCoorUnit;

    // Pitch map (periodic, works in local space)
    if (m_pitchMapEnabled) {
        rawCoor.dim1 = Asa::SensorPitchSizeMap(
            rawCoor.dim1, m_pitchTableDim1.data(), Asa::kCoorUnit);
        rawCoor.dim2 = Asa::SensorPitchSizeMap(
            rawCoor.dim2, m_pitchTableDim2.data(), Asa::kCoorUnit);
    }

    // ── LOCAL → GLOBAL conversion ──
    const int32_t centerOff = m_anchorCenterOffset * Asa::kCoorUnit;
    rawCoor.dim1 += static_cast<int32_t>(m_gridData.tx1.anchorCol) *
                    Asa::kCoorUnit - centerOff;
    rawCoor.dim2 += static_cast<int32_t>(m_gridData.tx1.anchorRow) *
                    Asa::kCoorUnit - centerOff;

    // 8. Noise gate (jump detection)
    if (m_noiseGate.DetectNoiseJump(rawCoor)) {
        m_lastResult.point.valid = false;
        m_lastResult.pipelineStage = 5;
        if (m_penStateMachine.GetState() == Asa::PenStateMachine::State::Writing) {
            ClosePressureGate(m_btPressBuf.ReadLatest().seq);
        } else {
            m_pressureSolver.Reset();
            m_pressureGateOpen = false;
        }
        m_noPressInkGate.Reset();
        m_prevValid = false;
        m_postProcessor.Reset();
        m_linearFilter.Reset();
        m_coorReviser.Reset();
        Asa::PenFrameEvidence evidence{};
        evidence.noSignal = true;
        auto profile = m_penStateMachine.Update(evidence);
        (void)profile;
        m_lastResult.animState = m_penStateMachine.GetAnimState();
        if (m_emitPacketWhenInvalid)
            m_packetBuilder.Build(m_lastResult,
                m_emitPacketWhenInvalid, outPacket);
        m_prevStatus = m_lastResult.status;
        return false;
    }

    // ── Phase 2.5: Pressure + State Machine → MotionProfile ──

    const uint16_t tx1PeakSignal = static_cast<uint16_t>(
        std::clamp(m_gridData.tx1.grid[peak.peakRow][peak.peakCol],
                   static_cast<int16_t>(0), static_cast<int16_t>(0x7FFF)));
    uint16_t tx2PeakSignal = 0;
    Asa::GridPeakDetector::PeakProjectionAnalysis tx2Analysis{};
    Asa::GridPeakUnit tx2Peak{};
    const bool tx2PeakValid = m_gridData.tx2.valid;
    if (tx2PeakValid) {
        tx2Analysis = m_peakDetector.AnalyzePeakAndProjection(m_gridData.tx2.grid);
        tx2Peak = tx2Analysis.peak;
        if (tx2Peak.valid) {
            tx2PeakSignal = static_cast<uint16_t>(
                std::clamp(m_gridData.tx2.grid[tx2Peak.peakRow][tx2Peak.peakCol],
                           static_cast<int16_t>(0), static_cast<int16_t>(0x7FFF)));
        }
    }

    const auto preCmfTx1Projection = m_peakDetector.ProjectTo1D(preCmfTx1Grid, peak);

    // Pressure (BT MCU) — solve BEFORE state machine
    auto metrics = BuildSignalMetrics(
        proj,
        rawCoor,
        tx1PeakSignal,
        tx2PeakSignal,
        m_sensorCols,
        m_sensorRows);
    metrics.tx1Composite = BuildProjectionComposite(preCmfTx1Projection);
    const bool isEdge = metrics.dim1EdgeActive || metrics.dim2EdgeActive;
    m_lastResult.tx1BlockValid = m_gridData.tx1.valid;
    m_lastResult.tx2BlockValid = m_gridData.tx2.valid;
    m_lastResult.signalX = metrics.signalX;
    m_lastResult.signalY = metrics.signalY;
    m_lastResult.maxRawPeak = metrics.maxRawPeak;
    m_lastResult.point.peakTx1 = metrics.tx1Composite;
    m_lastResult.point.peakTx2 = metrics.tx2Composite;
    m_lastResult.hpp3Dim1SignalValid = (m_lastResult.signalX > 0);
    m_lastResult.hpp3Dim2SignalValid = (m_lastResult.signalY > 0);
    const auto btSample = m_btPressBuf.ReadLatest();
    m_lastResult.point.rawPressure = btSample.pressure;

    const bool previouslyWriting =
        (m_penStateMachine.GetState() == Asa::PenStateMachine::State::Writing);
    const auto recheck = BuildRecheckContext(
        metrics, rawCoor.valid, previouslyWriting, m_recheckThBase, m_recheckThMulti);
    const bool recheckPassed = m_noiseGate.EvaluateRecheck(
        metrics.signalX, metrics.signalY, metrics.maxRawPeak,
        recheck.finalThreshold, recheck.sustainThreshold, recheck.overlapLike);
    const bool baseValid = rawCoor.valid && m_lastResult.tx1BlockValid && recheckPassed;
    const bool hoverPresent = baseValid &&
                              (metrics.tx1Composite >= m_tx1LiftAbsoluteThreshold);
    const bool authoritativeDown = baseValid &&
                                   (metrics.tx1Composite >= m_tx1InkEnterThreshold);
    const bool keepInkAlive = baseValid &&
                              (metrics.tx1Composite >= m_tx1LiftSuspiciousThreshold);
    const bool suspiciousDrop = previouslyWriting && baseValid &&
                                (metrics.tx1Composite >= m_tx1LiftAbsoluteThreshold) &&
                                (metrics.tx1Composite < m_tx1LiftSuspiciousThreshold);
    const bool immediateRelease = previouslyWriting && !suspiciousDrop &&
                                  (!baseValid ||
                                   (metrics.tx1Composite < m_tx1LiftAbsoluteThreshold));

    if (suspiciousDrop && m_hasLastGoodFrame) {
        TrackBlockedBtSeq(btSample.seq);
        m_lastResult = m_lastGoodFrame;
        outPacket = m_lastGoodFrame.packet;
        m_prevStatus = m_lastResult.status;
        return outPacket.valid;
    }

    if (!authoritativeDown) {
        TrackBlockedBtSeq(btSample.seq);
    }
    if (immediateRelease) {
        ClosePressureGate(btSample.seq);
    }

    if (authoritativeDown && !m_pressureGateOpen) {
        if (m_waitingFreshBtAfterRelease) {
            if (btSample.hasSample && btSample.seq > m_blockedBtSeqFloor) {
                m_pressureGateOpen = true;
                m_waitingFreshBtAfterRelease = false;
                m_blockedBtSeqFloor = btSample.seq;
            }
        } else {
            m_pressureGateOpen = btSample.hasSample;
        }
    }

    const Asa::EdgeSignalInputs edgeSignals{
        metrics.dim1EdgeActive,
        metrics.dim2EdgeActive,
        static_cast<int>(metrics.dim1EdgeSignal),
        static_cast<int>(metrics.dim2EdgeSignal)
    };
    const auto pressureStage = m_pressureSolver.SolveStage(
        btSample, m_pressureGateOpen,
        static_cast<int>(metrics.signalX),
        isEdge, edgeSignals);

    Asa::PenFrameEvidence evidence{};
    evidence.coordValid = rawCoor.valid;
    evidence.noSignal = false;
    evidence.tx1BlockValid = m_lastResult.tx1BlockValid;
    evidence.sustainActive = false;
    evidence.activeStylusPresent = hoverPresent;
    evidence.hoverSignalPresent = hoverPresent;
    evidence.authoritativeDown = authoritativeDown;
    evidence.keepInkAlive = keepInkAlive;
    evidence.hoverPresent = hoverPresent && !keepInkAlive;
    evidence.immediateRelease = immediateRelease;
    evidence.recheckPassed = recheckPassed;
    evidence.overlapLike = recheck.overlapLike;
    evidence.edgeLike = isEdge;
    evidence.exitSmoothCandidate = false;
    evidence.suppressPressureButKeepContact = false;
    evidence.btPressureResidual = false;
    evidence.edgeSignalLow = false;
    evidence.pressureIsReal = pressureStage.isRealMeasurement;
    evidence.mappedPressure = pressureStage.mappedPressure;
    evidence.realPressure = pressureStage.realPressure;
    evidence.realMeasuredPressure = pressureStage.isRealMeasurement ? pressureStage.mappedPressure : 0;
    evidence.pressureForContact = pressureStage.realPressure;
    evidence.tx1Composite = metrics.tx1Composite;
    evidence.tx2Composite = metrics.tx2Composite;
    evidence.curDim1 = rawCoor.dim1;
    evidence.curDim2 = rawCoor.dim2;
    const auto update = m_penStateMachine.Update(evidence);
    const auto& motion = update.motion;
    const auto& output = update.output;

    if (m_penStateMachine.JustLeftRange()) {
        ClosePressureGate(btSample.seq);
        m_postProcessor.Reset();
        m_linearFilter.Reset();
        m_coorReviser.Reset();
        m_noiseGate.Reset();
    }

    m_lastResult.point.mappedPressure = pressureStage.mappedPressure;
    m_lastResult.pressure = output.outputPressure;
    m_lastResult.point.pressure = m_lastResult.pressure;
    m_lastResult.noPressInkActive = false;
    m_lastResult.tipSwitchActive = output.tipSwitchActive;
    m_lastResult.sustainOutput = false;
    m_lastResult.fastLiftOutput = false;
    m_lastResult.recheckEnabled = m_noiseGate.recheckEnabled;
    m_lastResult.recheckThreshold = recheck.finalThreshold;
    m_lastResult.recheckThresholdMulti = recheck.sustainThreshold;
    m_lastResult.recheckOverlap = recheck.overlapLike;
    m_lastResult.recheckPassed = recheckPassed;
#ifdef _DEBUG
    m_dbg.sigSuppressActive = false;
#endif

    m_lastResult.animState = m_penStateMachine.GetAnimState();

    // ── Phase 3: Post-Processing Chain (all in GLOBAL space, Profile-driven) ──

    // 9a. LinearFilter (shared history from PenStateMachine)
    auto postCoor = m_linearFilter.Process(
        rawCoor, motion.enableLinearFilter, m_penStateMachine);

    // 9b. CoorReviser (TX2 solve + tilt + coordinate revision)
    if (motion.enableCoorReviser && m_coorReviser.enabled && m_gridData.tx2.valid) {
        if (tx2Peak.valid) {
            // Signal ratio tracking
            m_signalRatioTracker.Push(
                static_cast<int16_t>(tx1PeakSignal),
                static_cast<int16_t>(tx2PeakSignal));

            const auto& tx2Proj = tx2Analysis.projection;
            auto tx2Coor = m_coordSolver.Solve(
                tx2Proj,
                static_cast<int>(m_gridData.tx2.anchorRow),
                static_cast<int>(m_gridData.tx2.anchorCol),
                m_sensorRows, m_sensorCols,
                m_anchorCenterOffset);
            if (tx2Coor.valid) {
                m_lastResult.point.tx2X = static_cast<float>(tx2Coor.dim1) / Asa::kCoorUnit;
                m_lastResult.point.tx2Y = static_cast<float>(tx2Coor.dim2) / Asa::kCoorUnit;

                // TX2: PitchMap (local, periodic)
                if (m_pitchMapEnabled) {
                    tx2Coor.dim1 = Asa::SensorPitchSizeMap(
                        tx2Coor.dim1, m_pitchTableDim1.data(), Asa::kCoorUnit);
                    tx2Coor.dim2 = Asa::SensorPitchSizeMap(
                        tx2Coor.dim2, m_pitchTableDim2.data(), Asa::kCoorUnit);
                }

                // TX2: LOCAL → GLOBAL
                tx2Coor.dim1 += static_cast<int32_t>(m_gridData.tx2.anchorCol) *
                                Asa::kCoorUnit - centerOff;
                tx2Coor.dim2 += static_cast<int32_t>(m_gridData.tx2.anchorRow) *
                                Asa::kCoorUnit - centerOff;

                // CoorReviser: tilt + coordinate revision
                int16_t tiltX = 0, tiltY = 0;
                postCoor = m_coorReviser.Revise(postCoor, tx2Coor,
                    m_lastResult.pressure, tiltX, tiltY);
                m_lastResult.point.tiltX = tiltX;
                m_lastResult.point.tiltY = tiltY;
            }
        }
    }

    // 9c. Coordinate smoothing (Profile-driven IIR or 1-Euro)
    if (m_filterMode == 0) {
        postCoor = m_postProcessor.StepIIR(
            postCoor, motion.iirCoef, motion.iirDivisorN, motion.skipIIR);
    } else if (m_filterMode == 1) {
        postCoor = m_postProcessor.StepIIR(
            postCoor, motion.iirCoef, motion.iirDivisorN, motion.skipIIR);
    }

    // 9d. Jitter suppression (Profile-driven strength)
    postCoor = m_postProcessor.StepJitter(postCoor, motion.jitterStrength, isEdge);

    // Store final coordinate
    auto finalCoor = postCoor;
    if (output.keepPreviousCoordinate && m_hasLastGoodFrame) {
        finalCoor.valid = output.keepInRangeOnReleaseFrame || m_lastGoodFrame.point.valid;
        finalCoor.dim1 = static_cast<int32_t>(m_lastGoodFrame.point.x);
        finalCoor.dim2 = static_cast<int32_t>(m_lastGoodFrame.point.y);
        if (output.applyExitEdgeSnap) {
            float snappedX = static_cast<float>(finalCoor.dim1);
            float snappedY = static_cast<float>(finalCoor.dim2);
            m_noiseGate.ApplyExitEdgeSnap(
                m_lastGoodFrame.point.x,
                m_lastGoodFrame.point.y,
                m_prevPointX,
                m_prevPointY,
                m_sensorRows,
                m_sensorCols,
                snappedX,
                snappedY);
            finalCoor.dim1 = static_cast<int32_t>(snappedX);
            finalCoor.dim2 = static_cast<int32_t>(snappedY);
        }
    }
    m_lastResult.pipelineStage = 0; // Success

    m_lastResult.point.valid = finalCoor.valid;
    m_lastResult.point.x = static_cast<float>(finalCoor.dim1);
    m_lastResult.point.y = static_cast<float>(finalCoor.dim2);

#ifdef _DEBUG
    // ── Diagnostics (Debug only) ──
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
    m_dbg.speedInstant  = m_penStateMachine.GetInstantSpeed();
    m_dbg.speedShortAvg = m_penStateMachine.GetSmoothedSpeed();
    m_dbg.speedFullAvg  = 0.f;
    m_dbg.iirCoef   = static_cast<float>(motion.iirCoef);
    m_dbg.isHover   = (m_lastResult.pressure == 0);
    m_dbg.isEdge    = isEdge;
    m_dbg.tiltDiffX = static_cast<float>(m_coorReviser.GetLastTiltX());
    m_dbg.tiltDiffY = static_cast<float>(m_coorReviser.GetLastTiltY());
    m_dbg.peakSignal = m_lastResult.maxRawPeak;
    m_dbg.signalRatio       = m_signalRatioTracker.GetAvgRatio();
    m_dbg.exitSmoothed      = false;
    m_dbg.cmfEnabled        = m_cmfFilter.enabled;
    m_dbg.coorReviserActive = m_coorReviser.enabled;
    m_dbg.coorRevDeltaX     = static_cast<float>(m_coorReviser.GetLastReviseX());
    m_dbg.coorRevDeltaY     = static_cast<float>(m_coorReviser.GetLastReviseY());
    m_dbg.tiltAnomalyDamped = false;
    m_dbg.penLifecycle      = static_cast<uint8_t>(m_penStateMachine.GetState());
    m_dbg.wasInking         = m_lastResult.tipSwitchActive;
    m_dbg.avg3PtDim1        = postCoor.dim1;
    m_dbg.avg3PtDim2        = postCoor.dim2;
#endif

    // ── Phase 4: Edge Compensation + Output ──

    // 10a. Keep only upstream coordinate compensation in this refactor.
    m_edgeCoorPost.Apply(m_lastResult.point.x, m_lastResult.point.y,
                         m_sensorCols, m_sensorRows);

    // ── Output ──

    m_prevPointX = m_lastResult.point.x;
    m_prevPointY = m_lastResult.point.y;
    m_prevValid = finalCoor.valid;
    m_prevStatus = m_lastResult.status;
    m_packetBuilder.Build(m_lastResult,
        m_emitPacketWhenInvalid, outPacket);

    // Save last known-good frame
    m_lastGoodFrame = m_lastResult;
    m_hasLastGoodFrame = true;

#ifdef _DEBUG
    // Final diagnostics
    m_dbg.rawPressure = m_lastResult.point.rawPressure;
    m_dbg.mappedPressure = m_lastResult.point.mappedPressure;
    m_dbg.btSeq = pressureStage.btSeq;
    m_dbg.predictedAgeFrames = pressureStage.predictedAgeFrames;
    m_dbg.pressureIsReal = pressureStage.isRealMeasurement;
    m_dbg.vhfPenState = outPacket.valid ? outPacket.bytes[1] : 0;
    m_dbg.linearFilterState = static_cast<uint8_t>(m_linearFilter.GetMode());
#endif
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

        // === Solver ===
        ConfigParam("sp.coordUseTriangle", "Use Triangle Mode",
            ConfigParam::Bool, const_cast<bool*>(&m_coordSolver.useTriangle), Cat::Solver),
        ConfigParam("sp.triEdgeSecondaryBlend", "Tri Edge Secondary Blend",
            ConfigParam::Bool, const_cast<bool*>(&m_coordSolver.triEdgeSecondaryBlend), Cat::Solver),
        ConfigParam("sp.triEdgeDim1Ratio", "Tri Edge Dim1 Ratio",
            ConfigParam::Int, const_cast<int*>(&m_coordSolver.triEdgeDim1.ratio), 0, 1000, Cat::Solver),
        ConfigParam("sp.triEdgeDim1ThLast", "Tri Edge Dim1 ThLast",
            ConfigParam::Int, const_cast<int*>(&m_coordSolver.triEdgeDim1.sumThresholdIdxLast), 0, 20000, Cat::Solver),
        ConfigParam("sp.triEdgeDim1Th0", "Tri Edge Dim1 Th0",
            ConfigParam::Int, const_cast<int*>(&m_coordSolver.triEdgeDim1.sumThresholdIdx0), 0, 20000, Cat::Solver),
        ConfigParam("sp.triEdgeDim2Ratio", "Tri Edge Dim2 Ratio",
            ConfigParam::Int, const_cast<int*>(&m_coordSolver.triEdgeDim2.ratio), 0, 1000, Cat::Solver),
        ConfigParam("sp.triEdgeDim2ThLast", "Tri Edge Dim2 ThLast",
            ConfigParam::Int, const_cast<int*>(&m_coordSolver.triEdgeDim2.sumThresholdIdxLast), 0, 20000, Cat::Solver),
        ConfigParam("sp.triEdgeDim2Th0", "Tri Edge Dim2 Th0",
            ConfigParam::Int, const_cast<int*>(&m_coordSolver.triEdgeDim2.sumThresholdIdx0), 0, 20000, Cat::Solver),
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
            ConfigParam::Int, const_cast<int*>(&m_recheckThBase), 10, 5000, Cat::Solver),
        ConfigParam("sp.recheckThMulti", "Signal Thresh Multi",
            ConfigParam::Int, const_cast<int*>(&m_recheckThMulti), 10, 5000, Cat::Solver),
        ConfigParam("sp.tx1InkEnterTh", "TX1 Ink Enter Threshold",
            ConfigParam::Int, const_cast<int*>(&m_tx1InkEnterThreshold), 10, 30000, Cat::Solver),
        ConfigParam("sp.tx1LiftSuspiciousTh", "TX1 Lift Suspicious Threshold",
            ConfigParam::Int, const_cast<int*>(&m_tx1LiftSuspiciousThreshold), 10, 30000, Cat::Solver),
        ConfigParam("sp.tx1LiftAbsoluteTh", "TX1 Lift Absolute Threshold",
            ConfigParam::Int, const_cast<int*>(&m_tx1LiftAbsoluteThreshold), 10, 30000, Cat::Solver),
        ConfigParam("sp.pitchMapEnabled", "Pitch Map Enabled",
            ConfigParam::Bool, const_cast<bool*>(&m_pitchMapEnabled), Cat::Solver),
        ConfigParam("sp.tpPatternEnabled", "TP Pattern Comp Enabled",
            ConfigParam::Bool, const_cast<bool*>(&m_tpPatternCompEnabled), Cat::Solver),

        // === State Machine ===
        ConfigParam("sp.smSpeedLow", "SM Speed Low Ref",
            ConfigParam::Float, const_cast<float*>(&m_penStateMachine.speedLow), 1.0f, 100.0f, Cat::Filter),
        ConfigParam("sp.smSpeedHigh", "SM Speed High Ref",
            ConfigParam::Float, const_cast<float*>(&m_penStateMachine.speedHigh), 50.0f, 1000.0f, Cat::Filter),
        ConfigParam("sp.smStillThr", "SM Still Speed Threshold",
            ConfigParam::Float, const_cast<float*>(&m_penStateMachine.stillSpeedThreshold), 0.5f, 50.0f, Cat::Filter),
        ConfigParam("sp.smIirLow", "SM IIR Low-Speed Coef",
            ConfigParam::Int, const_cast<int*>(&m_penStateMachine.movingIirLow), 1, 16, Cat::Filter),
        ConfigParam("sp.smIirHigh", "SM IIR High-Speed Coef",
            ConfigParam::Int, const_cast<int*>(&m_penStateMachine.movingIirHigh), 1, 16, Cat::Filter),
        ConfigParam("sp.smIirDivisor", "SM IIR Divisor N",
            ConfigParam::Int, const_cast<int*>(&m_penStateMachine.iirDivisorN), 1, 256, Cat::Filter),
        ConfigParam("sp.smHoverIir", "SM Hover IIR Coef",
            ConfigParam::Int, const_cast<int*>(&m_penStateMachine.hoverIirCoef), 1, 16, Cat::Filter),
        ConfigParam("sp.smJitterMax", "SM Jitter Max Strength",
            ConfigParam::Int, const_cast<int*>(&m_penStateMachine.jitterMax), 0, 5, Cat::Filter),
        ConfigParam("sp.smSpdSmooth", "SM Speed Smooth Window",
            ConfigParam::Int, const_cast<int*>(&m_penStateMachine.speedSmoothWindow), 1, 20, Cat::Filter),
        ConfigParam("sp.smDirHalve", "SM Dir Halve Enable",
            ConfigParam::Bool, const_cast<bool*>(&m_penStateMachine.enableDirectionalHalve), Cat::Filter),
        ConfigParam("sp.smDirVelThr", "SM Dir Velocity Thr",
            ConfigParam::Float, const_cast<float*>(&m_penStateMachine.directionalVelThreshold), 0.1f, 50.0f, Cat::Filter),
        ConfigParam("sp.smLiftTimeout", "SM Lift Timeout",
            ConfigParam::Int, const_cast<int*>(&m_penStateMachine.liftTimeout), 1, 30, Cat::General),
        ConfigParam("sp.smLongPress", "SM Long Press Frames",
            ConfigParam::Int, const_cast<int*>(&m_penStateMachine.longPressFrames), 10, 600, Cat::General),

        // === Jitter (4-param TSACore) ===
        ConfigParam("sp.jitEdgeDim1", "Jitter Edge Param Dim1",
            ConfigParam::Int, const_cast<int*>(&m_postProcessor.jitterEdgeParamDim1), 0, 20, Cat::Filter),
        ConfigParam("sp.jitEdgeDim2", "Jitter Edge Param Dim2",
            ConfigParam::Int, const_cast<int*>(&m_postProcessor.jitterEdgeParamDim2), 0, 20, Cat::Filter),
        ConfigParam("sp.jitCntrDim1", "Jitter Center Param Dim1",
            ConfigParam::Int, const_cast<int*>(&m_postProcessor.jitterCenterParamDim1), 0, 20, Cat::Filter),
        ConfigParam("sp.jitCntrDim2", "Jitter Center Param Dim2",
            ConfigParam::Int, const_cast<int*>(&m_postProcessor.jitterCenterParamDim2), 0, 20, Cat::Filter),

        // === LinearFilter ===
        ConfigParam("sp.lfMinFitLen", "LF Min Fit Length",
            ConfigParam::Int, const_cast<int*>(&m_linearFilter.minFitLength), 5, 100, Cat::Filter),
        ConfigParam("sp.lfEnterResidual", "LF Enter Residual Thr",
            ConfigParam::Float, const_cast<float*>(&m_linearFilter.enterResidualThreshold), 1.0f, 500.0f, Cat::Filter),
        ConfigParam("sp.lfExitDeviation", "LF Exit Deviation",
            ConfigParam::Float, const_cast<float*>(&m_linearFilter.exitDeviation), 10.0f, 1000.0f, Cat::Filter),
        ConfigParam("sp.lfPerpConstraint", "LF Perp Constraint (0-1)",
            ConfigParam::Float, const_cast<float*>(&m_linearFilter.perpConstraint), 0.0f, 1.0f, Cat::Filter),
        ConfigParam("sp.lfTransRate", "LF Transition Rate",
            ConfigParam::Float, const_cast<float*>(&m_linearFilter.transitionRate), 0.05f, 1.0f, Cat::Filter),

        // === Noise Gate ===
        ConfigParam("sp.hpp3NoiseEnabled", "Enable HPP3 Noise",
            ConfigParam::Bool, const_cast<bool*>(&m_noiseGate.noisePostEnabled), Cat::Filter),
        ConfigParam("sp.hpp3JumpTh", "Jump Threshold",
            ConfigParam::Float, const_cast<float*>(&m_noiseGate.coorJumpThreshold), 1.0f, 100.0f, Cat::Filter),
        ConfigParam("sp.cmfEnabled", "CMF Enabled",
            ConfigParam::Bool, const_cast<bool*>(&m_cmfFilter.enabled), Cat::Filter),
        ConfigParam("sp.cmfWindowSize", "CMF Window Size",
            ConfigParam::Int, const_cast<int*>(&m_cmfFilter.windowSize), 1, 8, Cat::Filter),

        // === Behavior ===
        ConfigParam("sp.edgeCoorPostEnabled", "Enable Edge Coordinate Process",
            ConfigParam::Bool, const_cast<bool*>(&m_edgeCoorPost.enabled), Cat::Behavior),
        ConfigParam("sp.crEnabled", "Enable TX2 Coor Reviser",
            ConfigParam::Bool, const_cast<bool*>(&m_coorReviser.enabled), Cat::Behavior),
        ConfigParam("sp.crTiltMultX", "CoorRevise Tilt Mult X",
            ConfigParam::Int, const_cast<int*>(&m_coorReviser.tiltMultiplierX), 0, 20, Cat::Behavior),
        ConfigParam("sp.crTiltMultY", "CoorRevise Tilt Mult Y",
            ConfigParam::Int, const_cast<int*>(&m_coorReviser.tiltMultiplierY), 0, 20, Cat::Behavior),
        ConfigParam("sp.crDiffAvgWin", "CoorRevise Diff Avg Window",
            ConfigParam::Int, const_cast<int*>(&m_coorReviser.diffAverageWindow), 1, 10, Cat::Behavior),
        ConfigParam("sp.crTiltAvgWin", "CoorRevise Tilt Avg Window",
            ConfigParam::Int, const_cast<int*>(&m_coorReviser.tiltAverageWindow), 1, 10, Cat::Behavior),
        ConfigParam("sp.crRevAvgWin", "CoorRevise Revise Avg Window",
            ConfigParam::Int, const_cast<int*>(&m_coorReviser.reviseAverageWindow), 1, 10, Cat::Behavior),
        ConfigParam("sp.crLimitStep", "CoorRevise Limit Step",
            ConfigParam::Int, const_cast<int*>(&m_coorReviser.reviseLimitStep), 1, 100, Cat::Behavior),
        ConfigParam("sp.crNormLenDim1", "CoorRevise NormLen Dim1",
            ConfigParam::Int, const_cast<int*>(&m_coorReviser.normLenDim1), 100, 10000, Cat::Behavior),
        ConfigParam("sp.crNormLenDim2", "CoorRevise NormLen Dim2",
            ConfigParam::Int, const_cast<int*>(&m_coorReviser.normLenDim2), 100, 10000, Cat::Behavior),
        ConfigParam("sp.crMaxTiltDeg", "CoorRevise Max Tilt Deg",
            ConfigParam::Int, const_cast<int*>(&m_coorReviser.maxTiltDeg), 10, 89, Cat::Behavior),
        ConfigParam("sp.crTiltJitterDeg", "CoorRevise Tilt Jitter Deg",
            ConfigParam::Int, const_cast<int*>(&m_coorReviser.tiltJitterDeg), 0, 10, Cat::Behavior),
        ConfigParam("sp.crKeepLast", "CoorRevise Keep Last On Invalid",
            ConfigParam::Bool, const_cast<bool*>(&m_coorReviser.keepLastOnInvalid), Cat::Behavior),

        // === Output ===
        ConfigParam("sp.pressPolyEnabled", "Polynomial Mapping",
            ConfigParam::Bool, const_cast<bool*>(&m_pressureSolver.polyEnabled), Cat::Output),
        ConfigParam("sp.pressSeg1Th", "Seg1 Threshold",
            ConfigParam::Int, const_cast<int*>(&m_pressureSolver.seg1Threshold), 0, 50, Cat::Output),
        ConfigParam("sp.pressSeg2Th", "Seg2 Threshold",
            ConfigParam::Int, const_cast<int*>(&m_pressureSolver.seg2Threshold), 50, 500, Cat::Output),
        ConfigParam("sp.pressGain", "Gain %",
            ConfigParam::Int, const_cast<int*>(&m_pressureSolver.gainPercent), 10, 500, Cat::Output),

        // === Filter Mode ===
        ConfigParam("sp.filterMode", "Filter Mode (0=IIR 1=1Euro 2=Off)",
            ConfigParam::Int, const_cast<int*>(&m_filterMode), 0, 2, Cat::Filter),
    };
}

// ══════════════════════════════════════════════
// SaveConfig
// ══════════════════════════════════════════════
void StylusPipeline::SaveConfig(std::ostream& out) const {
    out << "sp.enableSlaveChecksum=" << m_enableSlaveChecksum << "\n";
    out << "sp.emitPacketWhenInvalid=" << m_emitPacketWhenInvalid << "\n";
    out << "sp.coordUseTriangle=" << m_coordSolver.useTriangle << "\n";
    out << "sp.triEdgeSecondaryBlend=" << m_coordSolver.triEdgeSecondaryBlend << "\n";
    out << "sp.triEdgeDim1Ratio=" << m_coordSolver.triEdgeDim1.ratio << "\n";
    out << "sp.triEdgeDim1ThLast=" << m_coordSolver.triEdgeDim1.sumThresholdIdxLast << "\n";
    out << "sp.triEdgeDim1Th0=" << m_coordSolver.triEdgeDim1.sumThresholdIdx0 << "\n";
    out << "sp.triEdgeDim2Ratio=" << m_coordSolver.triEdgeDim2.ratio << "\n";
    out << "sp.triEdgeDim2ThLast=" << m_coordSolver.triEdgeDim2.sumThresholdIdxLast << "\n";
    out << "sp.triEdgeDim2Th0=" << m_coordSolver.triEdgeDim2.sumThresholdIdx0 << "\n";
    out << "sp.sensorRows=" << m_sensorRows << "\n";
    out << "sp.sensorCols=" << m_sensorCols << "\n";
    out << "sp.anchorCenterOffset=" << m_anchorCenterOffset << "\n";
    out << "sp.pitchCompDim1Enabled=" << m_coordSolver.pitchCompDim1.enabled << "\n";
    out << "sp.pitchCompDim2Enabled=" << m_coordSolver.pitchCompDim2.enabled << "\n";
    out << "sp.gravityNoiseFloor=" << m_coordSolver.gravityNoiseFloor << "\n";
    out << "sp.gravityFictEdge=" << m_coordSolver.gravityFictitiousEdge << "\n";
    out << "sp.pitchMapEnabled=" << m_pitchMapEnabled << "\n";
    // State machine
    out << "sp.smSpeedLow=" << m_penStateMachine.speedLow << "\n";
    out << "sp.smSpeedHigh=" << m_penStateMachine.speedHigh << "\n";
    out << "sp.smStillThr=" << m_penStateMachine.stillSpeedThreshold << "\n";
    out << "sp.smIirLow=" << m_penStateMachine.movingIirLow << "\n";
    out << "sp.smIirHigh=" << m_penStateMachine.movingIirHigh << "\n";
    out << "sp.smIirDivisor=" << m_penStateMachine.iirDivisorN << "\n";
    out << "sp.smHoverIir=" << m_penStateMachine.hoverIirCoef << "\n";
    out << "sp.smJitterMax=" << m_penStateMachine.jitterMax << "\n";
    out << "sp.smSpdSmooth=" << m_penStateMachine.speedSmoothWindow << "\n";
    out << "sp.smDirHalve=" << m_penStateMachine.enableDirectionalHalve << "\n";
    out << "sp.smDirVelThr=" << m_penStateMachine.directionalVelThreshold << "\n";
    out << "sp.smLiftTimeout=" << m_penStateMachine.liftTimeout << "\n";
    out << "sp.smLongPress=" << m_penStateMachine.longPressFrames << "\n";
    // Jitter 4-param
    out << "sp.jitEdgeDim1=" << m_postProcessor.jitterEdgeParamDim1 << "\n";
    out << "sp.jitEdgeDim2=" << m_postProcessor.jitterEdgeParamDim2 << "\n";
    out << "sp.jitCntrDim1=" << m_postProcessor.jitterCenterParamDim1 << "\n";
    out << "sp.jitCntrDim2=" << m_postProcessor.jitterCenterParamDim2 << "\n";
    // LinearFilter
    out << "sp.lfMinFitLen=" << m_linearFilter.minFitLength << "\n";
    out << "sp.lfEnterResidual=" << m_linearFilter.enterResidualThreshold << "\n";
    out << "sp.lfExitDeviation=" << m_linearFilter.exitDeviation << "\n";
    out << "sp.lfPerpConstraint=" << m_linearFilter.perpConstraint << "\n";
    out << "sp.lfTransRate=" << m_linearFilter.transitionRate << "\n";
    // CoorReviser
    out << "sp.crEnabled=" << m_coorReviser.enabled << "\n";
    out << "sp.crTiltMultX=" << m_coorReviser.tiltMultiplierX << "\n";
    out << "sp.crTiltMultY=" << m_coorReviser.tiltMultiplierY << "\n";
    out << "sp.crDiffAvgWin=" << m_coorReviser.diffAverageWindow << "\n";
    out << "sp.crTiltAvgWin=" << m_coorReviser.tiltAverageWindow << "\n";
    out << "sp.crRevAvgWin=" << m_coorReviser.reviseAverageWindow << "\n";
    out << "sp.crLimitStep=" << m_coorReviser.reviseLimitStep << "\n";
    out << "sp.crNormLenDim1=" << m_coorReviser.normLenDim1 << "\n";
    out << "sp.crNormLenDim2=" << m_coorReviser.normLenDim2 << "\n";
    out << "sp.crMaxTiltDeg=" << m_coorReviser.maxTiltDeg << "\n";
    out << "sp.crTiltJitterDeg=" << m_coorReviser.tiltJitterDeg << "\n";
    out << "sp.crKeepLast=" << m_coorReviser.keepLastOnInvalid << "\n";
    // Edge
    out << "sp.edgeCoorPostEnabled=" << m_edgeCoorPost.enabled << "\n";
    // Noise
    out << "sp.hpp3NoiseEnabled=" << m_noiseGate.noisePostEnabled << "\n";
    out << "sp.hpp3JumpTh=" << m_noiseGate.coorJumpThreshold << "\n";
    out << "sp.recheckEnabled=" << m_noiseGate.recheckEnabled << "\n";
    out << "sp.recheckThBase=" << m_recheckThBase << "\n";
    out << "sp.recheckThMulti=" << m_recheckThMulti << "\n";
    out << "sp.tx1InkEnterTh=" << m_tx1InkEnterThreshold << "\n";
    out << "sp.tx1LiftSuspiciousTh=" << m_tx1LiftSuspiciousThreshold << "\n";
    out << "sp.tx1LiftAbsoluteTh=" << m_tx1LiftAbsoluteThreshold << "\n";
    out << "sp.cmfEnabled=" << m_cmfFilter.enabled << "\n";
    out << "sp.cmfWindowSize=" << m_cmfFilter.windowSize << "\n";
    out << "sp.tpPatternEnabled=" << m_tpPatternCompEnabled << "\n";
    // Pressure
    out << "sp.pressPolyEnabled=" << m_pressureSolver.polyEnabled << "\n";
    out << "sp.pressSeg1Th=" << m_pressureSolver.seg1Threshold << "\n";
    out << "sp.pressSeg2Th=" << m_pressureSolver.seg2Threshold << "\n";
    out << "sp.pressGain=" << m_pressureSolver.gainPercent << "\n";
    // Filter mode
    out << "sp.filterMode=" << m_filterMode << "\n";
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
    else if (key == "sp.coordUseTriangle") m_coordSolver.useTriangle = toBool(value);
    else if (key == "sp.triEdgeSecondaryBlend" || key == "sp.coordEdgeCompBit3") m_coordSolver.triEdgeSecondaryBlend = toBool(value);
    else if (key == "sp.triEdgeDim1Ratio") m_coordSolver.triEdgeDim1.ratio = std::clamp(toInt(value), 0, 1000);
    else if (key == "sp.triEdgeDim1ThLast") m_coordSolver.triEdgeDim1.sumThresholdIdxLast = std::clamp(toInt(value), 0, 20000);
    else if (key == "sp.triEdgeDim1Th0") m_coordSolver.triEdgeDim1.sumThresholdIdx0 = std::clamp(toInt(value), 0, 20000);
    else if (key == "sp.triEdgeDim2Ratio") m_coordSolver.triEdgeDim2.ratio = std::clamp(toInt(value), 0, 1000);
    else if (key == "sp.triEdgeDim2ThLast") m_coordSolver.triEdgeDim2.sumThresholdIdxLast = std::clamp(toInt(value), 0, 20000);
    else if (key == "sp.triEdgeDim2Th0") m_coordSolver.triEdgeDim2.sumThresholdIdx0 = std::clamp(toInt(value), 0, 20000);
    else if (key == "sp.sensorRows") m_sensorRows = toInt(value);
    else if (key == "sp.sensorCols") m_sensorCols = toInt(value);
    else if (key == "sp.anchorCenterOffset") m_anchorCenterOffset = toInt(value);
    else if (key == "sp.pitchCompDim1Enabled") m_coordSolver.pitchCompDim1.enabled = toBool(value);
    else if (key == "sp.pitchCompDim2Enabled") m_coordSolver.pitchCompDim2.enabled = toBool(value);
    else if (key == "sp.gravityNoiseFloor") m_coordSolver.gravityNoiseFloor = toInt(value);
    else if (key == "sp.gravityFictEdge") m_coordSolver.gravityFictitiousEdge = toBool(value);
    else if (key == "sp.pitchMapEnabled") m_pitchMapEnabled = toBool(value);
    // State machine
    else if (key == "sp.smSpeedLow") m_penStateMachine.speedLow = toFloat(value);
    else if (key == "sp.smSpeedHigh") m_penStateMachine.speedHigh = toFloat(value);
    else if (key == "sp.smStillThr") m_penStateMachine.stillSpeedThreshold = toFloat(value);
    else if (key == "sp.smIirLow") m_penStateMachine.movingIirLow = toInt(value);
    else if (key == "sp.smIirHigh") m_penStateMachine.movingIirHigh = toInt(value);
    else if (key == "sp.smIirDivisor") m_penStateMachine.iirDivisorN = toInt(value);
    else if (key == "sp.smHoverIir") m_penStateMachine.hoverIirCoef = toInt(value);
    else if (key == "sp.smJitterMax") m_penStateMachine.jitterMax = toInt(value);
    else if (key == "sp.smSpdSmooth") m_penStateMachine.speedSmoothWindow = toInt(value);
    else if (key == "sp.smDirHalve") m_penStateMachine.enableDirectionalHalve = toBool(value);
    else if (key == "sp.smDirVelThr") m_penStateMachine.directionalVelThreshold = toFloat(value);
    else if (key == "sp.smLiftTimeout") m_penStateMachine.liftTimeout = toInt(value);
    else if (key == "sp.smLongPress") m_penStateMachine.longPressFrames = toInt(value);
    // Jitter 4-param
    else if (key == "sp.jitEdgeDim1") m_postProcessor.jitterEdgeParamDim1 = toInt(value);
    else if (key == "sp.jitEdgeDim2") m_postProcessor.jitterEdgeParamDim2 = toInt(value);
    else if (key == "sp.jitCntrDim1") m_postProcessor.jitterCenterParamDim1 = toInt(value);
    else if (key == "sp.jitCntrDim2") m_postProcessor.jitterCenterParamDim2 = toInt(value);
    // LinearFilter
    else if (key == "sp.lfMinFitLen") m_linearFilter.minFitLength = toInt(value);
    else if (key == "sp.lfEnterResidual") m_linearFilter.enterResidualThreshold = toFloat(value);
    else if (key == "sp.lfExitDeviation") m_linearFilter.exitDeviation = toFloat(value);
    else if (key == "sp.lfPerpConstraint") m_linearFilter.perpConstraint = toFloat(value);
    else if (key == "sp.lfTransRate") m_linearFilter.transitionRate = toFloat(value);
    // CoorReviser
    else if (key == "sp.crEnabled") m_coorReviser.enabled = toBool(value);
    else if (key == "sp.crTiltMultX") m_coorReviser.tiltMultiplierX = toInt(value);
    else if (key == "sp.crTiltMultY") m_coorReviser.tiltMultiplierY = toInt(value);
    else if (key == "sp.crDiffAvgWin") m_coorReviser.diffAverageWindow = toInt(value);
    else if (key == "sp.crTiltAvgWin") m_coorReviser.tiltAverageWindow = toInt(value);
    else if (key == "sp.crRevAvgWin") m_coorReviser.reviseAverageWindow = toInt(value);
    else if (key == "sp.crLimitStep") m_coorReviser.reviseLimitStep = toInt(value);
    else if (key == "sp.crNormLenDim1") m_coorReviser.normLenDim1 = toInt(value);
    else if (key == "sp.crNormLenDim2") m_coorReviser.normLenDim2 = toInt(value);
    else if (key == "sp.crMaxTiltDeg") m_coorReviser.maxTiltDeg = toInt(value);
    else if (key == "sp.crTiltJitterDeg") m_coorReviser.tiltJitterDeg = toInt(value);
    else if (key == "sp.crKeepLast") m_coorReviser.keepLastOnInvalid = toBool(value);
    // Edge
    else if (key == "sp.edgeCoorPostEnabled") m_edgeCoorPost.enabled = toBool(value);
    // Noise
    else if (key == "sp.hpp3NoiseEnabled") m_noiseGate.noisePostEnabled = toBool(value);
    else if (key == "sp.hpp3JumpTh") m_noiseGate.coorJumpThreshold = toFloat(value);
    else if (key == "sp.recheckEnabled") m_noiseGate.recheckEnabled = toBool(value);
    else if (key == "sp.recheckThBase") m_recheckThBase = toInt(value);
    else if (key == "sp.recheckThMulti") m_recheckThMulti = toInt(value);
    else if (key == "sp.tx1InkEnterTh") m_tx1InkEnterThreshold = toInt(value);
    else if (key == "sp.tx1LiftSuspiciousTh") m_tx1LiftSuspiciousThreshold = toInt(value);
    else if (key == "sp.tx1LiftAbsoluteTh") m_tx1LiftAbsoluteThreshold = toInt(value);
    else if (key == "sp.cmfEnabled") m_cmfFilter.enabled = toBool(value);
    else if (key == "sp.cmfWindowSize") m_cmfFilter.windowSize = toInt(value);
    else if (key == "sp.tpPatternEnabled") m_tpPatternCompEnabled = toBool(value);
    // Pressure
    else if (key == "sp.pressPolyEnabled") m_pressureSolver.polyEnabled = toBool(value);
    else if (key == "sp.pressSeg1Th") m_pressureSolver.seg1Threshold = toInt(value);
    else if (key == "sp.pressSeg2Th") m_pressureSolver.seg2Threshold = toInt(value);
    else if (key == "sp.pressGain") m_pressureSolver.gainPercent = toInt(value);
    // Filter mode
    else if (key == "sp.filterMode") m_filterMode = toInt(value);
}

} // namespace Solvers
