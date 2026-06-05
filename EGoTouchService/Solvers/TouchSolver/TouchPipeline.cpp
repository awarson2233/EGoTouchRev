#include "TouchPipeline.h"
#include "config/ConfigBinder.h"
#include "config/ConfigStore.h"

#include <algorithm>
#include <array>
#include <cstdint>

namespace Solvers {

// ══════════════════════════════════════════════════════════════════════
// Process — linear orchestration of all 6 phases
// ══════════════════════════════════════════════════════════════════════
bool TouchPipeline::ProcessMasterParserOnly(HeatmapFrame& frame) {
    m_frameParser.Process(frame);
    ResetIdleOutputs(frame);
    return true;
}

bool TouchPipeline::Process(HeatmapFrame& frame) {
    ReserveContactCapacity(frame);

    if (!ProcessFrameParser(frame)) return true;
    if (!ProcessSignalConditioning(frame)) return true;

    GenerateContacts(frame);
    PostProcessContacts(frame);
    UpdateContactCaches(frame);
#if EGOTOUCH_DIAG
    UpdateDiagnosticCaches(frame);
#endif
    ProcessTrackingAndGesture(frame);
    return true;
}

void TouchPipeline::ReserveContactCapacity(HeatmapFrame& frame) const {
    const size_t desiredContactCapacity = static_cast<size_t>(
        std::max(m_contactExtractor.m_zoneExp.m_maxTouches, m_tracker.m_maxTouchCount));
    if (frame.touch.output.contacts.capacity() < desiredContactCapacity) {
        frame.touch.output.contacts.reserve(desiredContactCapacity);
    }
}

bool TouchPipeline::ProcessFrameParser(HeatmapFrame& frame) {
    // ── Phase 1: Frame Parsing ──────────────────────────────────────
    m_frameParser.Process(frame);
    if (m_frameParser.m_enabled) return true;

    ResetIdleOutputs(frame);
    return false;
}

bool TouchPipeline::ProcessSignalConditioning(HeatmapFrame& frame) {
    const bool hasCurrentFinger = frame.masterWasRead &&
                                  frame.masterSuffixValid &&
                                  frame.masterSuffix.hasFinger();
    const bool hasLiveTouchState = m_tracker.HasLiveTracks() || m_gesture.HasLiveState();

    // ── Phase 2: Signal Conditioning ────────────────────────────────
    m_baseline.Process(frame, hasCurrentFinger);
    if (hasCurrentFinger || hasLiveTouchState) {
        m_cmf.Process(frame);
        return true;
    }

    ResetIdleOutputs(frame);
    return false;
}

void TouchPipeline::GenerateContacts(HeatmapFrame& frame) {
    // ── Phase 3: Candidate Generation ───────────────────────────────
    frame.touch.output.contacts.clear();
    m_macroZoneDet.Process(frame, m_peakDet.m_threshold);
    const auto& macroZones = m_macroZoneDet.GetMacroZones();
    m_peakDet.Detect(frame, macroZones);
    const auto peaks = m_peakDet.GetPeaks();

    // ── Phase 4: Candidate Classification ───────────────────────────
    m_touchClassifier.Process(frame, macroZones, peaks);
    const auto peakEvaluations = m_touchClassifier.GetPeakEvaluations();

#if EGOTOUCH_DIAG
    // Diagnostic segmentation remains separate from contact generation.
    m_contactExtractor.ProcessDiagnostics(frame, macroZones, peaks);
#endif

    // ── Phase 5: Contact Extraction ─────────────────────────────────
    m_contactExtractor.m_zoneExp.m_palmAwareExpansionEnabled = m_touchClassifier.m_palmAwareExpansionEnabled;
    m_contactExtractor.m_zoneExp.m_fingerInPalmThresholdRatio = m_touchClassifier.m_fingerInPalmThresholdRatio;
    m_contactExtractor.m_zoneExp.m_fingerInPalmMaxRadius = m_touchClassifier.m_fingerInPalmMaxRadius;
    m_contactExtractor.Process(frame, peaks, m_peakDet.m_threshold, peakEvaluations);
}

void TouchPipeline::PostProcessContacts(HeatmapFrame& frame) {
    const auto& edgeInfos = m_contactExtractor.GetEdgeInfos();
    const auto& edgeBounds = m_contactExtractor.GetEdgeBounds();
    m_edgeComp.Process(frame.touch.output.contacts, edgeInfos, edgeBounds);
    m_edgeReject.Process(frame.touch.output.contacts, edgeInfos, edgeBounds);
    m_stylusSuppress.Process(frame);
}

void TouchPipeline::UpdateContactCaches(HeatmapFrame& frame) {
    m_cachedPeakCount.store(static_cast<int>(m_peakDet.GetPeaks().size()), std::memory_order_relaxed);
    m_cachedZoneCount.store(m_contactExtractor.GetZoneCount(), std::memory_order_relaxed);
    m_cachedContactCount.store(static_cast<int>(frame.touch.output.contacts.size()), std::memory_order_relaxed);
}

void TouchPipeline::ProcessTrackingAndGesture(HeatmapFrame& frame) {
    m_tracker.Process(frame);
    m_coordFilter.Process(frame);
    m_gesture.Process(frame);
}

#if EGOTOUCH_DIAG
void TouchPipeline::UpdateDiagnosticCaches(HeatmapFrame& frame) {
    const auto& macroZones = m_macroZoneDet.GetMacroZones();
    const auto peaks = m_peakDet.GetPeaks();

    // MacroZone → touchZones colormap for IPC visualization
    frame.touch.debug.touchZones.fill(0);
    for (size_t i = 0; i < macroZones.size(); ++i) {
        const uint8_t colorId = static_cast<uint8_t>((i % 10) + 1);
        for (int idx : macroZones[i].pixels) {
            if (idx >= 0 && idx < 2400) {
                frame.touch.debug.touchZones[idx] = colorId;
            }
        }
    }

    frame.touch.debug.peakZones = m_contactExtractor.GetPeakZones();

    if (frame.touch.debug.peaks.capacity() < peaks.size()) {
        frame.touch.debug.peaks.reserve(peaks.size());
    }
    frame.touch.debug.peaks.clear();
    for (const auto& pk : peaks) {
        frame.touch.debug.peaks.push_back({pk.r, pk.c, pk.z, pk.id});
    }

    const auto& zoneEdge = m_contactExtractor.GetZoneEdge();
    const bool touchZonesChanged = frame.touch.debug.touchZones != m_diagTouchZonesPrev;
    const bool zoneEdgeChanged = zoneEdge != m_diagZoneEdgePrev;

    {
        std::lock_guard<std::mutex> lk(m_diagMtx);
        if (m_diagPeaks.capacity() < peaks.size()) {
            m_diagPeaks.reserve(peaks.size());
        }
        m_diagPeaks.assign(peaks.begin(), peaks.end());
        if (touchZonesChanged) {
            m_diagTouchZones = frame.touch.debug.touchZones;
            m_diagTouchZonesPrev = frame.touch.debug.touchZones;
        }
        if (zoneEdgeChanged) {
            m_diagZoneEdge = zoneEdge;
            m_diagZoneEdgePrev = zoneEdge;
        }
    }
}
#endif

void TouchPipeline::ResetIdleOutputs(HeatmapFrame& frame) {
    frame.touch.output.contacts.clear();
    frame.touch.output.touchPackets = {};

    m_cachedPeakCount.store(0, std::memory_order_relaxed);
    m_cachedZoneCount.store(0, std::memory_order_relaxed);
    m_cachedContactCount.store(0, std::memory_order_relaxed);

#if EGOTOUCH_DIAG
    frame.touch.debug.touchZones.fill(0);
    frame.touch.debug.peakZones.fill(0);
    frame.touch.debug.peaks.clear();

    {
        std::lock_guard<std::mutex> lk(m_diagMtx);
        m_diagPeaks.clear();
        m_diagTouchZones.fill(0);
        m_diagZoneEdge.fill(0);
        m_diagTouchZonesPrev.fill(0);
        m_diagZoneEdgePrev.fill(0);
    }
#endif
}

void TouchPipeline::SyncStylusSuppressConfigFromTracker() {
    m_stylusSuppress.m_stylusSuppressGlobalEnabled = m_tracker.m_stylusSuppressGlobalEnabled;
    m_stylusSuppress.m_stylusSuppressLocalEnabled = m_tracker.m_stylusSuppressLocalEnabled;
    m_stylusSuppress.m_stylusSuppressLocalDistance = m_tracker.m_stylusSuppressLocalDistance;
    m_stylusSuppress.m_stylusSuppressPenPeakThreshold = m_tracker.m_stylusSuppressPenPeakThreshold;
    m_stylusSuppress.m_stylusSuppressTouchSignalKeep = m_tracker.m_stylusSuppressTouchSignalKeep;
    m_stylusSuppress.m_stylusSuppressTouchAreaKeep = m_tracker.m_stylusSuppressTouchAreaKeep;
    m_stylusSuppress.m_stylusAftEnabled = m_tracker.m_stylusAftEnabled;
    m_stylusSuppress.m_stylusAftDebounceFrames = m_tracker.m_stylusAftDebounceFrames;
    m_stylusSuppress.m_stylusAftWeakSignalThreshold = m_tracker.m_stylusAftWeakSignalThreshold;
    m_stylusSuppress.m_stylusAftWeakSizeThresholdMm = m_tracker.m_stylusAftWeakSizeThresholdMm;
    m_stylusSuppress.m_stylusAftSuppressFrames = m_tracker.m_stylusAftSuppressFrames;
    m_stylusSuppress.m_fallbackSizeMm = m_tracker.m_fallbackSizeMm;
    m_stylusSuppress.m_sizeAreaScale = m_tracker.m_sizeAreaScale;
    m_stylusSuppress.m_sizeSignalScale = m_tracker.m_sizeSignalScale;
}

// ══════════════════════════════════════════════════════════════════════
// Thread-safe accessors
// ══════════════════════════════════════════════════════════════════════
std::vector<Touch::Peak> TouchPipeline::GetPeaks() const {
#if EGOTOUCH_DIAG
    std::lock_guard<std::mutex> lk(m_diagMtx);
    return m_diagPeaks;
#else
    return {};
#endif
}

std::array<uint8_t, 2400> TouchPipeline::GetTouchZones() const {
#if EGOTOUCH_DIAG
    std::lock_guard<std::mutex> lk(m_diagMtx);
    return m_diagTouchZones;
#else
    return {};
#endif
}

std::array<uint8_t, 2400> TouchPipeline::GetZoneEdge() const {
#if EGOTOUCH_DIAG
    std::lock_guard<std::mutex> lk(m_diagMtx);
    return m_diagZoneEdge;
#else
    return {};
#endif
}

void TouchPipeline::registerBindings(Config::ConfigBinder& binder) {
    using Config::ConfigRange;

    binder.bind("touch.signal_cond.baseline_bg_alpha_shift",
                &Touch::BaselineTracker::m_backgroundAlphaShift, m_baseline,
                static_cast<int32_t>(3), ConfigRange{0, 15},
                "Background alpha shift for baseline tracking");
    binder.bind("touch.signal_cond.baseline_bg_max_step",
                &Touch::BaselineTracker::m_backgroundMaxStep, m_baseline,
                static_cast<int32_t>(512), ConfigRange{1, 2048},
                "Background max step for baseline tracking");
    binder.bind("touch.signal_cond.baseline_no_finger_alpha_shift",
                &Touch::BaselineTracker::m_noFingerAlphaShift, m_baseline,
                static_cast<int32_t>(3), ConfigRange{0, 15},
                "No-finger alpha shift for baseline tracking");
    binder.bind("touch.signal_cond.baseline_no_finger_max_step",
                &Touch::BaselineTracker::m_noFingerMaxStep, m_baseline,
                static_cast<int32_t>(512), ConfigRange{1, 2048},
                "No-finger max step for baseline tracking");
    binder.bind("touch.signal_cond.baseline_recovery_alpha_shift",
                &Touch::BaselineTracker::m_recoveryAlphaShift, m_baseline,
                static_cast<int32_t>(2), ConfigRange{0, 15},
                "Recovery alpha shift for baseline tracking");
    binder.bind("touch.signal_cond.baseline_recovery_max_frames",
                &Touch::BaselineTracker::m_recoveryMaxFrames, m_baseline,
                static_cast<int32_t>(30), ConfigRange{1, 120},
                "Max frames for baseline recovery");
    binder.bind("touch.signal_cond.baseline_recovery_max_step",
                &Touch::BaselineTracker::m_recoveryMaxStep, m_baseline,
                static_cast<int32_t>(256), ConfigRange{1, 2048},
                "Recovery max step for baseline tracking");

    binder.bind("touch.frame_parser.enabled",
                &Touch::MasterFrameParser::m_enabled, m_frameParser,
                true, {}, "Frame Parser enable switch");
}

void TouchPipeline::applyConfig(const Config::ConfigStore& store) {
    m_baseline.m_backgroundAlphaShift = store.getOr<int32_t>("touch.signal_cond.baseline_bg_alpha_shift", 3);
    m_baseline.m_backgroundMaxStep = store.getOr<int32_t>("touch.signal_cond.baseline_bg_max_step", 512);
    m_baseline.m_noFingerAlphaShift = store.getOr<int32_t>("touch.signal_cond.baseline_no_finger_alpha_shift", 3);
    m_baseline.m_noFingerMaxStep = store.getOr<int32_t>("touch.signal_cond.baseline_no_finger_max_step", 512);
    m_baseline.m_recoveryAlphaShift = store.getOr<int32_t>("touch.signal_cond.baseline_recovery_alpha_shift", 2);
    m_baseline.m_recoveryMaxFrames = store.getOr<int32_t>("touch.signal_cond.baseline_recovery_max_frames", 30);
    m_baseline.m_recoveryMaxStep = store.getOr<int32_t>("touch.signal_cond.baseline_recovery_max_step", 256);

    m_frameParser.m_enabled = store.getOr<bool>("touch.frame_parser.enabled", true);
}

} // namespace Solvers
