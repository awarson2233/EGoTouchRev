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
    return ProcessTrackingAndGesture(frame);
}

void TouchPipeline::ReserveContactCapacity(HeatmapFrame& frame) const {
    const size_t desiredContactCapacity = static_cast<size_t>(
        std::max(m_contactExtractor.m_zoneExp.m_maxTouches, m_tracker.m_maxTouchCount));
    (void)frame;
    (void)desiredContactCapacity;
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
    m_edgeComp.Process(frame.touch.output.contacts.span(), edgeInfos, edgeBounds);
    m_edgeReject.Process(frame.touch.output.contacts.span(), edgeInfos, edgeBounds);
    m_stylusSuppress.Process(frame);
}

void TouchPipeline::UpdateContactCaches(HeatmapFrame& frame) {
    m_cachedPeakCount.store(static_cast<int>(m_peakDet.GetPeaks().size()), std::memory_order_relaxed);
    m_cachedZoneCount.store(m_contactExtractor.GetZoneCount(), std::memory_order_relaxed);
    m_cachedContactCount.store(static_cast<int>(frame.touch.output.contacts.size()), std::memory_order_relaxed);
}

bool TouchPipeline::ProcessTrackingAndGesture(HeatmapFrame& frame) {
    m_tracker.Process(frame);
    m_coordFilter.Process(frame);
    return ProcessGestureOutput(frame);
}

bool TouchPipeline::ProcessGestureOutput(HeatmapFrame& frame) {
    return m_gesture.Process(frame);
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

    binder.bind("touch.frame_parser.enabled",
                &Touch::MasterFrameParser::m_enabled, m_frameParser,
                true, {}, "Frame Parser enable switch");

    binder.bind("touch.signal_cond.baseline_enabled",
                &Touch::BaselineTracker::m_enabled, m_baseline,
                true, {}, "Baseline tracker enable switch");
    binder.bind("touch.signal_cond.baseline_value",
                &Touch::BaselineTracker::m_baseline, m_baseline,
                static_cast<int32_t>(0x7FEE), ConfigRange{0, 65535},
                "Initial baseline value in ADC units");
    binder.bind("touch.signal_cond.baseline_noise_deadband",
                &Touch::BaselineTracker::m_noiseDeadband, m_baseline,
                static_cast<int32_t>(90), ConfigRange{0, 200},
                "Baseline noise deadband");
    binder.bind("touch.signal_cond.baseline_positive_deadband",
                &Touch::BaselineTracker::m_positiveDeadband, m_baseline,
                static_cast<int32_t>(14), ConfigRange{0, 200},
                "Baseline positive deadband");
    binder.bind("touch.signal_cond.baseline_negative_deadband",
                &Touch::BaselineTracker::m_negativeDeadband, m_baseline,
                static_cast<int32_t>(13), ConfigRange{0, 200},
                "Baseline negative deadband");
    binder.bind("touch.signal_cond.baseline_peak_threshold",
                &Touch::BaselineTracker::m_peakThreshold, m_baseline,
                static_cast<int32_t>(305), ConfigRange{1, 2000},
                "Baseline freeze threshold");
    binder.bind("touch.signal_cond.baseline_release_hold_frames",
                &Touch::BaselineTracker::m_releaseHoldFrames, m_baseline,
                static_cast<int32_t>(60), ConfigRange{0, 255},
                "Baseline release hold frames");
    binder.bind("touch.signal_cond.baseline_positive_alpha_shift",
                &Touch::BaselineTracker::m_positiveAlphaShift, m_baseline,
                static_cast<int32_t>(7), ConfigRange{0, 15},
                "Positive baseline alpha shift");
    binder.bind("touch.signal_cond.baseline_negative_alpha_shift",
                &Touch::BaselineTracker::m_negativeAlphaShift, m_baseline,
                static_cast<int32_t>(5), ConfigRange{0, 15},
                "Negative baseline alpha shift");
    binder.bind("touch.signal_cond.baseline_noise_alpha_shift",
                &Touch::BaselineTracker::m_noiseAlphaShift, m_baseline,
                static_cast<int32_t>(6), ConfigRange{0, 15},
                "Noise baseline alpha shift");
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
    binder.bind("touch.signal_cond.baseline_positive_max_step",
                &Touch::BaselineTracker::m_positiveMaxStep, m_baseline,
                static_cast<int32_t>(20), ConfigRange{0, 200},
                "Positive baseline max step");
    binder.bind("touch.signal_cond.baseline_negative_max_step",
                &Touch::BaselineTracker::m_negativeMaxStep, m_baseline,
                static_cast<int32_t>(20), ConfigRange{0, 200},
                "Negative baseline max step");
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
    binder.bind("touch.signal_cond.baseline_noise_tracking_enabled",
                &Touch::BaselineTracker::m_noiseTrackingEnabled, m_baseline,
                true, {}, "Enable baseline tracking inside noise deadband");

    binder.bind("touch.signal_cond.cmf_enabled",
                &Touch::CMFProcessor::m_enabled, m_cmf,
                true, {}, "Common mode filter enable switch");
    binder.bind("touch.signal_cond.cmf_exclusion_threshold",
                &Touch::CMFProcessor::m_exclusionThreshold, m_cmf,
                static_cast<int32_t>(2000), ConfigRange{0, 32767},
                "CMF exclusion threshold");
    binder.bind("touch.signal_cond.cmf_max_correction",
                &Touch::CMFProcessor::m_maxCorrection, m_cmf,
                static_cast<int32_t>(2000), ConfigRange{0, 32767},
                "CMF max correction");

    binder.bind("touch.peak_detection.threshold",
                &Touch::PeakDetector::m_threshold, m_peakDet,
                static_cast<int32_t>(350), ConfigRange{0, 4095},
                "Peak detection threshold");
    binder.bind("touch.peak_detection.max_peaks",
                &Touch::PeakDetector::m_maxPeaks, m_peakDet,
                static_cast<int32_t>(20), ConfigRange{1, 100},
                "Maximum detected peaks");
    binder.bind("touch.peak_detection.local_max_radius",
                &Touch::PeakDetector::m_localMaxRadius, m_peakDet,
                static_cast<int32_t>(1), ConfigRange{1, 5},
                "Local maximum search radius");
    binder.bind("touch.peak_detection.edge_threshold_enabled",
                &Touch::PeakDetector::m_edgeThresholdEnabled, m_peakDet,
                true, {}, "Enable edge-specific peak threshold");
    binder.bind("touch.peak_detection.edge_threshold",
                &Touch::PeakDetector::m_edgeThreshold, m_peakDet,
                static_cast<int32_t>(300), ConfigRange{0, 4095},
                "Edge-specific peak threshold");
    binder.bind("touch.peak_detection.z8_filter_enabled",
                &Touch::PeakDetector::m_z8Filter, m_peakDet,
                true, {}, "Enable isolated spike Z8 filter");
    binder.bind("touch.peak_detection.z1_filter_enabled",
                &Touch::PeakDetector::m_z1Filter, m_peakDet,
                true, {}, "Enable low-signal Z1 filter");
    binder.bind("touch.peak_detection.close_peak_saddle_filter_enabled",
                &Touch::PeakDetector::m_closePeakSaddleFilter, m_peakDet,
                true, {}, "Enable close-peak saddle suppression");
    binder.bind("touch.peak_detection.close_peak_radius",
                &Touch::PeakDetector::m_closePeakRadius, m_peakDet,
                static_cast<int32_t>(2), ConfigRange{1, 8},
                "Close-peak saddle radius");
    binder.bind("touch.peak_detection.macro_zone_min_area",
                &Touch::PeakDetector::m_macroZoneMinArea, m_peakDet,
                static_cast<int32_t>(4), ConfigRange{1, 64},
                "Minimum macro-zone area for peaks");

    binder.bind("touch.classifier.enabled",
                &Touch::TouchClassifier::m_enabled, m_touchClassifier,
                true, {}, "Touch classifier enable switch");
    binder.bind("touch.classifier.analyzer_enabled",
                &Touch::TouchClassifier::m_analyzerEnabled, m_touchClassifier,
                true, {}, "Zone analyzer enable switch");
    binder.bind("touch.classifier.peak_eval_enabled",
                &Touch::TouchClassifier::m_peakEvalEnabled, m_touchClassifier,
                true, {}, "Peak evaluation enable switch");
    binder.bind("touch.classifier.area_threshold",
                &Touch::TouchClassifier::m_areaThreshold, m_touchClassifier,
                static_cast<int32_t>(50), ConfigRange{0, 500},
                "Touch area threshold");
    binder.bind("touch.classifier.signal_sum_threshold",
                &Touch::TouchClassifier::m_signalSumThreshold, m_touchClassifier,
                static_cast<int32_t>(80000), ConfigRange{0, 1000000},
                "Touch signal sum threshold");
    binder.bind("touch.classifier.finger_prominence",
                &Touch::TouchClassifier::m_fingerProminence, m_touchClassifier,
                static_cast<int32_t>(100), ConfigRange{0, 5000},
                "Finger peak prominence threshold");
    binder.bind("touch.classifier.finger_sharpness",
                &Touch::TouchClassifier::m_fingerSharpness, m_touchClassifier,
                3.35f, ConfigRange{0.0, 10.0},
                "Finger sharpness threshold");
    binder.bind("touch.classifier.palm_sharpness_max",
                &Touch::TouchClassifier::m_palmSharpnessMax, m_touchClassifier,
                3.30f, ConfigRange{0.0, 10.0},
                "Palm sharpness maximum");
    binder.bind("touch.classifier.palm_aware_expansion_enabled",
                &Touch::TouchClassifier::m_palmAwareExpansionEnabled, m_touchClassifier,
                true, {}, "Enable palm-aware zone expansion");
    binder.bind("touch.classifier.finger_in_palm_threshold_ratio",
                &Touch::TouchClassifier::m_fingerInPalmThresholdRatio, m_touchClassifier,
                0.70f, ConfigRange{0.0, 1.0},
                "Finger-in-palm threshold ratio");
    binder.bind("touch.classifier.finger_in_palm_max_radius",
                &Touch::TouchClassifier::m_fingerInPalmMaxRadius, m_touchClassifier,
                static_cast<int32_t>(3), ConfigRange{0, 16},
                "Finger-in-palm max expansion radius");

    binder.bind("touch.zone_contact.threshold_scale_numer",
                &Touch::ZoneExpander::m_tholdScaleNumer, m_contactExtractor.m_zoneExp,
                static_cast<int32_t>(0x40), ConfigRange{0, 255},
                "Zone threshold scale numerator");
    binder.bind("touch.zone_contact.threshold_scale_shift",
                &Touch::ZoneExpander::m_tholdScaleShift, m_contactExtractor.m_zoneExp,
                static_cast<int32_t>(7), ConfigRange{0, 15},
                "Zone threshold scale shift");
    binder.bind("touch.zone_contact.dilate_erode_enabled",
                &Touch::ZoneExpander::m_dilateErode, m_contactExtractor.m_zoneExp,
                true, {}, "Enable zone dilate/erode cleanup");
    binder.bind("touch.zone_contact.max_touches",
                &Touch::ZoneExpander::m_maxTouches, m_contactExtractor.m_zoneExp,
                static_cast<int32_t>(10), ConfigRange{1, 20},
                "Maximum contacts after zone extraction");
    binder.bind("touch.zone_contact.edge_width_threshold",
                &Touch::ZoneExpander::m_edgeWidthThreshold, m_contactExtractor.m_zoneExp,
                static_cast<int32_t>(300), ConfigRange{0, 4095},
                "Zone edge width threshold");
    binder.bind("touch.zone_contact.touch_size_pixel_pitch_mm",
                &Touch::ContactExtractor::TouchSizeCalculator::m_pixelPitchMm, m_contactExtractor.m_touchSize,
                4.5f, ConfigRange{0.1, 20.0},
                "Touch size pixel pitch in millimeters");
    binder.bind("touch.zone_contact.touch_size_unit_per_sig_mm2",
                &Touch::ContactExtractor::TouchSizeCalculator::m_unitPerSigMm2, m_contactExtractor.m_touchSize,
                static_cast<int32_t>(128), ConfigRange{1, 4096},
                "Touch size signal scale");

    binder.bind("touch.edge.enabled",
                &Touch::EdgeCompensator::m_enabled, m_edgeComp,
                true, {}, "Edge compensation enable switch");
    binder.bind("touch.edge.comp_strength",
                &Touch::EdgeCompensator::m_ecStrength, m_edgeComp,
                1.0f, ConfigRange{0.0, 1.0},
                "Edge compensation strength");
    binder.bind("touch.edge.full_comp_range",
                &Touch::EdgeCompensator::m_ecFullCompRange, m_edgeComp,
                0.5f, ConfigRange{0.0, 5.0},
                "Edge full compensation range");
    binder.bind("touch.edge.blend_range",
                &Touch::EdgeCompensator::m_ecBlendRange, m_edgeComp,
                0.505f, ConfigRange{0.0, 5.0},
                "Edge compensation blend range");
    binder.bind("touch.edge.reject_enabled",
                &Touch::EdgeRejector::m_enabled, m_edgeReject,
                true, {}, "Edge rejection enable switch");
    binder.bind("touch.edge.reject_margin",
                &Touch::EdgeRejector::m_edgeMargin, m_edgeReject,
                static_cast<int32_t>(2), ConfigRange{0, 10},
                "Edge rejection margin");

    binder.bind("touch.tracking.enabled",
                &Touch::TouchTracker::m_enabled, m_tracker,
                true, {}, "Touch tracker enable switch");
    binder.bind("touch.tracking.max_touch_count",
                &Touch::TouchTracker::m_maxTouchCount, m_tracker,
                static_cast<int32_t>(20), ConfigRange{1, 20},
                "Maximum tracked touch count");
    binder.bind("touch.tracking.max_track_distance",
                &Touch::TouchTracker::m_maxTrackDistance, m_tracker,
                4.985f, ConfigRange{0.0, 30.0},
                "Maximum track matching distance");
    binder.bind("touch.tracking.always_match_distance",
                &Touch::TouchTracker::m_alwaysMatchDistance, m_tracker,
                2.0f, ConfigRange{0.0, 30.0},
                "Always-match distance");
    binder.bind("touch.tracking.gap_relink_enabled",
                &Touch::TouchTracker::m_gapRelinkEnabled, m_tracker,
                true, {}, "Enable gap relink");
    binder.bind("touch.tracking.gap_relink_window_frames",
                &Touch::TouchTracker::m_gapRelinkWindowFrames, m_tracker,
                static_cast<int32_t>(4), ConfigRange{0, 30},
                "Gap relink window in frames");
    binder.bind("touch.tracking.touch_down_debounce_frames",
                &Touch::TouchTracker::m_touchDownDebounceFrames, m_tracker,
                static_cast<int32_t>(1), ConfigRange{0, 30},
                "Touch-down debounce frames");
    binder.bind("touch.tracking.dynamic_debounce_enabled",
                &Touch::TouchTracker::m_dynamicDebounceEnabled, m_tracker,
                true, {}, "Enable dynamic touch-down debounce");
    binder.bind("touch.tracking.use_hungarian",
                &Touch::TouchTracker::m_useHungarian, m_tracker,
                true, {}, "Use Hungarian assignment for tracking");

    binder.bind("touch.stylus_suppress.global_enabled",
                &Touch::TouchTracker::m_stylusSuppressGlobalEnabled, m_tracker,
                true, {}, "Enable stylus touch suppression globally");
    binder.bind("touch.stylus_suppress.local_enabled",
                &Touch::TouchTracker::m_stylusSuppressLocalEnabled, m_tracker,
                true, {}, "Enable local stylus touch suppression");
    binder.bind("touch.stylus_suppress.local_distance",
                &Touch::TouchTracker::m_stylusSuppressLocalDistance, m_tracker,
                2.5f, ConfigRange{0.0, 30.0},
                "Local stylus suppression distance");
    binder.bind("touch.stylus_suppress.pen_peak_threshold",
                &Touch::TouchTracker::m_stylusSuppressPenPeakThreshold, m_tracker,
                static_cast<int32_t>(1500), ConfigRange{0, 20000},
                "Stylus suppression pen peak threshold");
    binder.bind("touch.stylus_suppress.aft_enabled",
                &Touch::TouchTracker::m_stylusAftEnabled, m_tracker,
                true, {}, "Enable after-stylus touch suppression");
    binder.bind("touch.stylus_suppress.aft_recent_frames",
                &Touch::TouchTracker::m_stylusAftRecentFrames, m_tracker,
                static_cast<int32_t>(24), ConfigRange{0, 240},
                "After-stylus recent-frame window");
    binder.bind("touch.stylus_suppress.aft_radius",
                &Touch::TouchTracker::m_stylusAftRadius, m_tracker,
                2.8f, ConfigRange{0.0, 30.0},
                "After-stylus suppression radius");

    binder.bind("touch.coord_filter.enabled",
                &Touch::CoordinateFilter::m_enabled, m_coordFilter,
                true, {}, "Coordinate filter enable switch");
    binder.bind("touch.coord_filter.min_cutoff",
                &Touch::CoordinateFilter::m_minCutoff, m_coordFilter,
                4.404f, ConfigRange{0.0, 50.0},
                "Coordinate filter minimum cutoff");
    binder.bind("touch.coord_filter.beta",
                &Touch::CoordinateFilter::m_beta, m_coordFilter,
                0.5f, ConfigRange{0.0, 10.0},
                "Coordinate filter beta");
    binder.bind("touch.coord_filter.d_cutoff",
                &Touch::CoordinateFilter::m_dCutoff, m_coordFilter,
                1.0f, ConfigRange{0.0, 50.0},
                "Coordinate filter derivative cutoff");

    binder.bind("touch.gesture.enabled",
                &Touch::TouchGestureStateMachine::m_enabled, m_gesture,
                true, {}, "Gesture state machine enable switch");
    binder.bind("touch.gesture.press_candidate_frames",
                &Touch::TouchGestureStateMachine::m_pressCandidateFrames, m_gesture,
                static_cast<int32_t>(1), ConfigRange{0, 30},
                "Press-candidate frames");
    binder.bind("touch.gesture.drag_threshold",
                &Touch::TouchGestureStateMachine::m_dragThreshold, m_gesture,
                0.8f, ConfigRange{0.0, 20.0},
                "Drag threshold");
    binder.bind("touch.gesture.long_press_frames",
                &Touch::TouchGestureStateMachine::m_longPressFrames, m_gesture,
                static_cast<int32_t>(46), ConfigRange{0, 300},
                "Long-press frames");
    binder.bind("touch.gesture.release_pending_frames",
                &Touch::TouchGestureStateMachine::m_releasePendingFrames, m_gesture,
                static_cast<int32_t>(0), ConfigRange{0, 60},
                "Release-pending frames");
    binder.bind("touch.gesture.bypass_state_machine",
                &Touch::TouchGestureStateMachine::m_bypassStateMachine, m_gesture,
                false, {}, "Bypass gesture state machine");}

void TouchPipeline::applyConfig(const Config::ConfigStore& store) {
    m_frameParser.m_enabled = store.getOr<bool>("touch.frame_parser.enabled", true);

    m_baseline.m_enabled = store.getOr<bool>("touch.signal_cond.baseline_enabled", true);
    m_baseline.m_baseline = store.getOr<int32_t>("touch.signal_cond.baseline_value", 0x7FEE);
    m_baseline.m_noiseDeadband = store.getOr<int32_t>("touch.signal_cond.baseline_noise_deadband", 90);
    m_baseline.m_positiveDeadband = store.getOr<int32_t>("touch.signal_cond.baseline_positive_deadband", 14);
    m_baseline.m_negativeDeadband = store.getOr<int32_t>("touch.signal_cond.baseline_negative_deadband", 13);
    m_baseline.m_peakThreshold = store.getOr<int32_t>("touch.signal_cond.baseline_peak_threshold", 305);
    m_baseline.m_releaseHoldFrames = store.getOr<int32_t>("touch.signal_cond.baseline_release_hold_frames", 60);
    m_baseline.m_positiveAlphaShift = store.getOr<int32_t>("touch.signal_cond.baseline_positive_alpha_shift", 7);
    m_baseline.m_negativeAlphaShift = store.getOr<int32_t>("touch.signal_cond.baseline_negative_alpha_shift", 5);
    m_baseline.m_noiseAlphaShift = store.getOr<int32_t>("touch.signal_cond.baseline_noise_alpha_shift", 6);
    m_baseline.m_backgroundAlphaShift = store.getOr<int32_t>("touch.signal_cond.baseline_bg_alpha_shift", 3);
    m_baseline.m_backgroundMaxStep = store.getOr<int32_t>("touch.signal_cond.baseline_bg_max_step", 512);
    m_baseline.m_noFingerAlphaShift = store.getOr<int32_t>("touch.signal_cond.baseline_no_finger_alpha_shift", 3);
    m_baseline.m_noFingerMaxStep = store.getOr<int32_t>("touch.signal_cond.baseline_no_finger_max_step", 512);
    m_baseline.m_positiveMaxStep = store.getOr<int32_t>("touch.signal_cond.baseline_positive_max_step", 20);
    m_baseline.m_negativeMaxStep = store.getOr<int32_t>("touch.signal_cond.baseline_negative_max_step", 20);
    m_baseline.m_recoveryAlphaShift = store.getOr<int32_t>("touch.signal_cond.baseline_recovery_alpha_shift", 2);
    m_baseline.m_recoveryMaxFrames = store.getOr<int32_t>("touch.signal_cond.baseline_recovery_max_frames", 30);
    m_baseline.m_recoveryMaxStep = store.getOr<int32_t>("touch.signal_cond.baseline_recovery_max_step", 256);
    m_baseline.m_noiseTrackingEnabled = store.getOr<bool>("touch.signal_cond.baseline_noise_tracking_enabled", true);

    m_cmf.m_enabled = store.getOr<bool>("touch.signal_cond.cmf_enabled", true);
    m_cmf.m_exclusionThreshold = store.getOr<int32_t>("touch.signal_cond.cmf_exclusion_threshold", 2000);
    m_cmf.m_maxCorrection = store.getOr<int32_t>("touch.signal_cond.cmf_max_correction", 2000);

    m_peakDet.m_threshold = store.getOr<int32_t>("touch.peak_detection.threshold", 350);
    m_peakDet.m_maxPeaks = store.getOr<int32_t>("touch.peak_detection.max_peaks", 20);
    m_peakDet.m_localMaxRadius = store.getOr<int32_t>("touch.peak_detection.local_max_radius", 1);
    m_peakDet.m_edgeThresholdEnabled = store.getOr<bool>("touch.peak_detection.edge_threshold_enabled", true);
    m_peakDet.m_edgeThreshold = store.getOr<int32_t>("touch.peak_detection.edge_threshold", 300);
    m_peakDet.m_z8Filter = store.getOr<bool>("touch.peak_detection.z8_filter_enabled", true);
    m_peakDet.m_z1Filter = store.getOr<bool>("touch.peak_detection.z1_filter_enabled", true);
    m_peakDet.m_closePeakSaddleFilter = store.getOr<bool>("touch.peak_detection.close_peak_saddle_filter_enabled", true);
    m_peakDet.m_closePeakRadius = store.getOr<int32_t>("touch.peak_detection.close_peak_radius", 2);
    m_peakDet.m_macroZoneMinArea = store.getOr<int32_t>("touch.peak_detection.macro_zone_min_area", 4);

    m_touchClassifier.m_enabled = store.getOr<bool>("touch.classifier.enabled", true);
    m_touchClassifier.m_analyzerEnabled = store.getOr<bool>("touch.classifier.analyzer_enabled", true);
    m_touchClassifier.m_peakEvalEnabled = store.getOr<bool>("touch.classifier.peak_eval_enabled", true);
    m_touchClassifier.m_areaThreshold = store.getOr<int32_t>("touch.classifier.area_threshold", 50);
    m_touchClassifier.m_signalSumThreshold = store.getOr<int32_t>("touch.classifier.signal_sum_threshold", 80000);
    m_touchClassifier.m_fingerProminence = store.getOr<int32_t>("touch.classifier.finger_prominence", 100);
    m_touchClassifier.m_fingerSharpness = store.getOr<float>("touch.classifier.finger_sharpness", 3.35f);
    m_touchClassifier.m_palmSharpnessMax = store.getOr<float>("touch.classifier.palm_sharpness_max", 3.30f);
    m_touchClassifier.m_palmAwareExpansionEnabled = store.getOr<bool>("touch.classifier.palm_aware_expansion_enabled", true);
    m_touchClassifier.m_fingerInPalmThresholdRatio = store.getOr<float>("touch.classifier.finger_in_palm_threshold_ratio", 0.70f);
    m_touchClassifier.m_fingerInPalmMaxRadius = store.getOr<int32_t>("touch.classifier.finger_in_palm_max_radius", 3);

    m_contactExtractor.m_zoneExp.m_tholdScaleNumer = store.getOr<int32_t>("touch.zone_contact.threshold_scale_numer", 0x40);
    m_contactExtractor.m_zoneExp.m_tholdScaleShift = store.getOr<int32_t>("touch.zone_contact.threshold_scale_shift", 7);
    m_contactExtractor.m_zoneExp.m_dilateErode = store.getOr<bool>("touch.zone_contact.dilate_erode_enabled", true);
    m_contactExtractor.m_zoneExp.m_maxTouches = store.getOr<int32_t>("touch.zone_contact.max_touches", 10);
    m_contactExtractor.m_zoneExp.m_edgeWidthThreshold = store.getOr<int32_t>("touch.zone_contact.edge_width_threshold", 300);
    m_contactExtractor.m_touchSize.m_pixelPitchMm = store.getOr<float>("touch.zone_contact.touch_size_pixel_pitch_mm", 4.5f);
    m_contactExtractor.m_touchSize.m_unitPerSigMm2 = store.getOr<int32_t>("touch.zone_contact.touch_size_unit_per_sig_mm2", 128);

    m_edgeComp.m_enabled = store.getOr<bool>("touch.edge.enabled", true);
    m_edgeComp.m_ecStrength = store.getOr<float>("touch.edge.comp_strength", 1.0f);
    m_edgeComp.m_ecFullCompRange = store.getOr<float>("touch.edge.full_comp_range", 0.5f);
    m_edgeComp.m_ecBlendRange = store.getOr<float>("touch.edge.blend_range", 0.505f);
    m_edgeReject.m_enabled = store.getOr<bool>("touch.edge.reject_enabled", true);
    m_edgeReject.m_edgeMargin = store.getOr<int32_t>("touch.edge.reject_margin", 2);

    const bool oldTrackerEnabled = m_tracker.m_enabled;
    m_tracker.m_enabled = store.getOr<bool>("touch.tracking.enabled", true);
    if (m_tracker.m_enabled != oldTrackerEnabled) {
        m_tracker.ClearLiveState();
    }
    m_tracker.m_maxTouchCount = store.getOr<int32_t>("touch.tracking.max_touch_count", 20);
    m_tracker.m_maxTrackDistance = store.getOr<float>("touch.tracking.max_track_distance", 4.985f);
    m_tracker.m_alwaysMatchDistance = store.getOr<float>("touch.tracking.always_match_distance", 2.0f);
    m_tracker.m_gapRelinkEnabled = store.getOr<bool>("touch.tracking.gap_relink_enabled", true);
    m_tracker.m_gapRelinkWindowFrames = store.getOr<int32_t>("touch.tracking.gap_relink_window_frames", 4);
    m_tracker.m_touchDownDebounceFrames = store.getOr<int32_t>("touch.tracking.touch_down_debounce_frames", 1);
    m_tracker.m_dynamicDebounceEnabled = store.getOr<bool>("touch.tracking.dynamic_debounce_enabled", true);
    m_tracker.m_useHungarian = store.getOr<bool>("touch.tracking.use_hungarian", true);

    m_tracker.m_stylusSuppressGlobalEnabled = store.getOr<bool>("touch.stylus_suppress.global_enabled", true);
    m_tracker.m_stylusSuppressLocalEnabled = store.getOr<bool>("touch.stylus_suppress.local_enabled", true);
    m_tracker.m_stylusSuppressLocalDistance = store.getOr<float>("touch.stylus_suppress.local_distance", 2.5f);
    m_tracker.m_stylusSuppressPenPeakThreshold = store.getOr<int32_t>("touch.stylus_suppress.pen_peak_threshold", 1500);
    m_tracker.m_stylusAftEnabled = store.getOr<bool>("touch.stylus_suppress.aft_enabled", true);
    m_tracker.m_stylusAftRecentFrames = store.getOr<int32_t>("touch.stylus_suppress.aft_recent_frames", 24);
    m_tracker.m_stylusAftRadius = store.getOr<float>("touch.stylus_suppress.aft_radius", 2.8f);
    SyncStylusSuppressConfigFromTracker();

    m_coordFilter.m_enabled = store.getOr<bool>("touch.coord_filter.enabled", true);
    m_coordFilter.m_minCutoff = store.getOr<float>("touch.coord_filter.min_cutoff", 4.404f);
    m_coordFilter.m_beta = store.getOr<float>("touch.coord_filter.beta", 0.5f);
    m_coordFilter.m_dCutoff = store.getOr<float>("touch.coord_filter.d_cutoff", 1.0f);

    const bool oldGestureEnabled = m_gesture.m_enabled;
    m_gesture.m_enabled = store.getOr<bool>("touch.gesture.enabled", true);
    if (m_gesture.m_enabled != oldGestureEnabled) {
        m_gesture.ClearLiveState();
    }
    m_gesture.m_pressCandidateFrames = store.getOr<int32_t>("touch.gesture.press_candidate_frames", 1);
    m_gesture.m_dragThreshold = store.getOr<float>("touch.gesture.drag_threshold", 0.8f);
    m_gesture.m_longPressFrames = store.getOr<int32_t>("touch.gesture.long_press_frames", 46);
    m_gesture.m_releasePendingFrames = store.getOr<int32_t>("touch.gesture.release_pending_frames", 0);
    m_gesture.m_bypassStateMachine = store.getOr<bool>("touch.gesture.bypass_state_machine", false);}

} // namespace Solvers
