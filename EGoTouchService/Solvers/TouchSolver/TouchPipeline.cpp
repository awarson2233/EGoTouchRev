#include "TouchPipeline.h"
#include "ConfigParse.h"

#include <algorithm>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>

namespace {

bool IsFrozenCurrentTouchConfigKey(std::string_view key) {
    constexpr std::string_view kFrozenKeys[] = {
        "FrameParserEnabled",
        "BaselineEnabled",
        "BaselineValue",
        "BaselineNoiseDeadband",
        "BaselinePositiveDeadband",
        "BaselineNegativeDeadband",
        "BaselinePeakThreshold",
        "BaselineReleaseHoldFrames",
        "BaselinePositiveAlphaShift",
        "BaselineNegativeAlphaShift",
        "BaselineNoiseAlphaShift",
        "BaselinePositiveMaxStep",
        "BaselineNegativeMaxStep",
        "BaselineNoiseTrackingEnabled",
        "CMFEnabled",
        "CMFDimensionMode",
        "CMFExclusionThreshold",
        "CMFMaxCorrection",
        "PeakThreshold",
        "SigTholdLimit",
        "Z8FilterEnabled",
        "Z1FilterEnabled",
        "PressureDriftFilter",
        "EdgePeakFilter",
        "EdgeThresholdEnabled",
        "EdgeThreshold",
        "Z8Radius",
        "MaxPeaks",
        "PressureDriftDebounce",
        "MacroZoneMinArea",
        "DilateErode",
        "ZoneTholdScale",
        "ZoneTholdShift",
        "MaxTouches",
        "PalmEnabled",
        "PalmAreaThreshold",
        "PalmSignalSumThreshold",
        "PalmDensityThresholdLow",
        "PalmAreaMinForDensity",
        "PalmElongatedEnabled",
        "PalmElongatedMinArea",
        "PalmElongatedAspectRatio",
        "PalmAnalyzerEnabled",
        "PalmCandidateAreaThreshold",
        "PalmCandidateSignalThreshold",
        "PalmLikelyAreaThreshold",
        "PalmFillRatioThreshold",
        "PalmFlatSharpnessThreshold",
        "PalmStrongPeakProminence",
        "PeakEvalEnabled",
        "PeakEvalFingerProminence",
        "PeakEvalFingerSharpness",
        "PeakEvalPalmSharpnessMax",
        "PeakEvalAmbiguousMargin",
        "PalmAwareExpansionEnabled",
        "PalmFingerInPalmThresholdRatio",
        "PalmFingerInPalmMaxRadius",
        "PalmLikelyAllowContact",
        "PalmShadowEnabled",
        "PalmShadowRadius",
        "PalmShadowHoldFrames",
        "PalmShadowSeedScore",
        "TrackerEnabled",
        "UseHungarian",
        "MaxTrackDistance",
        "AlwaysMatchDistance",
        "EdgeTrackBoost",
        "AccThresholdBoost",
        "AccBoostSizeMm",
        "PredictionScale",
        "GapRelinkEnabled",
        "GapRelinkWindowFrames",
        "TouchDownDebounceFrames",
        "DynamicDebounceEnabled",
        "TouchDownDebounceMaxExtra",
        "TouchDownWeakSignalThreshold",
        "TouchDownSmallSizeThresholdMm",
        "TouchDownRejectEnabled",
        "TouchDownRejectMinSignal",
        "TouchDownRejectMinSizeMm",
        "TouchDownEdgeRejectMinSignal",
        "FallbackSizeMm",
        "SizeAreaScale",
        "SizeSignalScale",
        "RxGhostFilterEnabled",
        "RxGhostLineDelta",
        "RxGhostWeakRatio",
        "RxGhostOnlyNew",
        "StylusSuppressGlobalEnabled",
        "StylusSuppressLocalEnabled",
        "StylusSuppressLocalDistance",
        "StylusSuppressPenPeakThreshold",
        "StylusSuppressTouchSignalKeep",
        "StylusSuppressTouchAreaKeep",
        "StylusAftEnabled",
        "StylusAftRecentFrames",
        "StylusAftRadius",
        "StylusAftDebounceFrames",
        "StylusAftWeakSignalThreshold",
        "StylusAftWeakSizeThresholdMm",
        "StylusAftSuppressFrames",
        "StylusAftPalmSuppressFrames",
        "StylusAftPalmAreaThreshold",
        "StylusAftPalmSizeThresholdMm",
        "CoordFilterEnabled",
        "OneEuroMinCutoff",
        "OneEuroBeta",
        "OneEuroDCutoff",
        "GestureEnabled",
        "PressCandidateFrames",
        "PressCandidateMinSignal",
        "PressCandidateMinSizeMm",
        "DragThreshold",
        "LongPressFrames",
        "LongPressMoveTolerance",
        "ReleasePendingFrames",
        "BypassStateMachine",
    };

    for (std::string_view frozenKey : kFrozenKeys) {
        if (frozenKey == key) return true;
    }
    return false;
}

} // namespace

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
    const size_t desiredContactCapacity = static_cast<size_t>(
        std::max(m_contactExtractor.m_zoneExp.m_maxTouches, m_tracker.m_maxTouchCount));
    if (frame.touch.output.contacts.capacity() < desiredContactCapacity) {
        frame.touch.output.contacts.reserve(desiredContactCapacity);
    }

    // ── Phase 1: Frame Parsing ──────────────────────────────────────
    m_frameParser.Process(frame);

    const bool masterValid = frame.masterWasRead && frame.masterSuffixValid;
    const bool hasCurrentFinger = masterValid && frame.masterSuffix.hasFinger();
    const bool hasLiveTouchState = m_tracker.HasLiveTracks() || m_gesture.HasLiveState();
    // ── Phase 2: Signal Conditioning ────────────────────────────────
    m_baseline.Process(frame, hasCurrentFinger);

    if (!hasCurrentFinger && !hasLiveTouchState) {
        ResetIdleOutputs(frame);
        return true;
    }

    m_cmf.Process(frame);

    // ── Phase 3: Candidate Generation ───────────────────────────────
    frame.touch.output.contacts.clear();
    m_macroZoneDet.Process(frame, m_peakDet.m_threshold);
    m_peakDet.Detect(frame, m_macroZoneDet.GetMacroZones());

    const auto peaks = m_peakDet.GetPeaks();

    // ── Phase 4: Candidate Classification ───────────────────────────
    m_touchClassifier.Process(frame, m_macroZoneDet.GetMacroZones(), peaks);
    const auto peakEvaluations = m_touchClassifier.GetPeakEvaluations();

    // Diagnostic segmentation remains separate from contact generation.
    m_contactExtractor.ProcessDiagnostics(frame, m_macroZoneDet.GetMacroZones(), peaks);

    // ── Phase 5: Contact Extraction ─────────────────────────────────
    m_contactExtractor.m_zoneExp.m_palmAwareExpansionEnabled = m_touchClassifier.m_palmAwareExpansionEnabled;
    m_contactExtractor.m_zoneExp.m_fingerInPalmThresholdRatio = m_touchClassifier.m_fingerInPalmThresholdRatio;
    m_contactExtractor.m_zoneExp.m_fingerInPalmMaxRadius = m_touchClassifier.m_fingerInPalmMaxRadius;
    m_contactExtractor.Process(frame, peaks, m_peakDet.m_threshold, peakEvaluations);

    // ── Phase 6: Contact Post-Processing ─────────────────────────────
    const auto& edgeInfos = m_contactExtractor.GetEdgeInfos();
    const auto& edgeBounds = m_contactExtractor.GetEdgeBounds();
    m_edgeComp.Process(frame.touch.output.contacts, edgeInfos, edgeBounds);
    m_edgeReject.Process(frame.touch.output.contacts, edgeInfos, edgeBounds);
    m_stylusSuppress.Process(frame);

    m_cachedPeakCount.store(static_cast<int>(peaks.size()), std::memory_order_relaxed);
    m_cachedZoneCount.store(m_contactExtractor.GetZoneCount(), std::memory_order_relaxed);
    m_cachedContactCount.store(static_cast<int>(frame.touch.output.contacts.size()), std::memory_order_relaxed);

    // ── Update diagnostic caches ────────────────────────────────
#if EGOTOUCH_DIAG
    // MacroZone → touchZones colormap for IPC visualization
    frame.touch.debug.touchZones.fill(0);
    const auto& mZones = m_macroZoneDet.GetMacroZones();
    for (size_t i = 0; i < mZones.size(); ++i) {
        const uint8_t colorId = static_cast<uint8_t>((i % 10) + 1);
        for (int idx : mZones[i].pixels) {
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

    {
        std::lock_guard<std::mutex> lk(m_diagMtx);
        if (m_diagPeaks.capacity() < peaks.size()) {
            m_diagPeaks.reserve(peaks.size());
        }
        m_diagPeaks.assign(peaks.begin(), peaks.end());
        m_diagTouchZones = frame.touch.debug.touchZones;
        m_diagZoneEdge = m_contactExtractor.GetZoneEdge();
    }
#endif

    m_tracker.Process(frame);
    m_coordFilter.Process(frame);
    // ── Phase 6: Gesture Recognition & Output ───────────────────────
    m_gesture.Process(frame);

    return true;
}

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

// ══════════════════════════════════════════════════════════════════════
// GetConfigSchema — unified from all sub-modules
// ══════════════════════════════════════════════════════════════════════
std::vector<ConfigParam> TouchPipeline::GetConfigSchema() const {
    std::vector<ConfigParam> s;

    // ── Frame Parser ──
    s.emplace_back("FrameParserEnabled", "Frame Parser Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_frameParser.m_enabled)).Module("Frame Parser");

    // ── Signal Conditioning: Baseline ──
    s.emplace_back("BaselineEnabled", "Baseline Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_baseline.m_enabled)).Module("Signal Conditioning");
    s.emplace_back("BaselineValue", "Baseline Value",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_baseline), 0, 65535).Module("Signal Conditioning");
    s.emplace_back("BaselineNoiseDeadband", "Baseline Noise Deadband",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_noiseDeadband), 0, 100).Module("Signal Conditioning");
    s.emplace_back("BaselinePositiveDeadband", "Baseline Positive Deadband",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_positiveDeadband), 0, 200).Module("Signal Conditioning");
    s.emplace_back("BaselineNegativeDeadband", "Baseline Negative Deadband",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_negativeDeadband), 0, 200).Module("Signal Conditioning");
    s.emplace_back("BaselinePeakThreshold", "Baseline Peak Threshold",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_peakThreshold), 1, 2000).Module("Signal Conditioning");
    s.emplace_back("BaselineReleaseHoldFrames", "Baseline Release Hold Frames",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_releaseHoldFrames), 0, 255).Module("Signal Conditioning");
    s.emplace_back("BaselinePositiveAlphaShift", "Baseline Positive Alpha Shift",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_positiveAlphaShift), 0, 15).Module("Signal Conditioning");
    s.emplace_back("BaselineNegativeAlphaShift", "Baseline Negative Alpha Shift",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_negativeAlphaShift), 0, 15).Module("Signal Conditioning");
    s.emplace_back("BaselineNoiseAlphaShift", "Baseline Noise Alpha Shift",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_noiseAlphaShift), 0, 15).Module("Signal Conditioning");
    s.emplace_back("BaselineBackgroundAlphaShift", "Baseline Background Alpha Shift",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_backgroundAlphaShift), 0, 15).Module("Signal Conditioning");
    s.emplace_back("BaselineNoFingerAlphaShift", "Baseline No-Finger Alpha Shift",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_noFingerAlphaShift), 0, 15).Module("Signal Conditioning");
    s.emplace_back("BaselinePositiveMaxStep", "Baseline Positive Max Step",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_positiveMaxStep), 0, 200).Module("Signal Conditioning");
    s.emplace_back("BaselineNegativeMaxStep", "Baseline Negative Max Step",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_negativeMaxStep), 0, 200).Module("Signal Conditioning");
    s.emplace_back("BaselineBackgroundMaxStep", "Baseline Background Max Step",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_backgroundMaxStep), 1, 2048).Module("Signal Conditioning");
    s.emplace_back("BaselineNoFingerMaxStep", "Baseline No-Finger Max Step",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_noFingerMaxStep), 1, 2048).Module("Signal Conditioning");
    s.emplace_back("BaselineRecoveryAlphaShift", "Baseline Recovery Alpha Shift",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_recoveryAlphaShift), 0, 15).Module("Signal Conditioning");
    s.emplace_back("BaselineRecoveryMaxStep", "Baseline Recovery Max Step",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_recoveryMaxStep), 1, 2048).Module("Signal Conditioning");
    s.emplace_back("BaselineRecoveryMaxFrames", "Baseline Recovery Max Frames",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_recoveryMaxFrames), 1, 120).Module("Signal Conditioning");
    s.emplace_back("BaselineNoiseTrackingEnabled", "Baseline Noise Tracking Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_baseline.m_noiseTrackingEnabled)).Module("Signal Conditioning");

    // ── Signal Conditioning: CMF ──
    s.emplace_back("CMFEnabled", "CMF Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_cmf.m_enabled)).Module("Signal Conditioning");
    s.emplace_back("CMFExclusionThreshold", "CMF Exclusion Threshold",
                   ConfigParam::Int, const_cast<int*>(&m_cmf.m_exclusionThreshold), 50, 2000).Module("Signal Conditioning");
    s.emplace_back("CMFMaxCorrection", "CMF Max Correction",
                   ConfigParam::Int, const_cast<int*>(&m_cmf.m_maxCorrection), 10, 2000).Module("Signal Conditioning");

    // ── Peak Detection ──
    s.emplace_back("PeakThreshold", "Peak Threshold",
                   ConfigParam::Int, const_cast<int*>(&m_peakDet.m_threshold), 1, 1000).Module("Peak Detection");
    s.emplace_back("SigTholdLimit", "Sig Thold Limit",
                   ConfigParam::Int, const_cast<int*>(&m_peakDet.m_sigTholdLimit), 1, 1000).Module("Peak Detection");
    s.emplace_back("Z8FilterEnabled", "Z8 Filter Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_peakDet.m_z8Filter)).Module("Peak Detection");
    s.emplace_back("Z1FilterEnabled", "Z1 Filter Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_peakDet.m_z1Filter)).Module("Peak Detection");
    s.emplace_back("PressureDriftFilter", "Pressure Drift Filter",
                   ConfigParam::Bool, const_cast<bool*>(&m_peakDet.m_pressureDriftFilter)).Module("Peak Detection");
    s.emplace_back("EdgePeakFilter", "Edge Peak Filter",
                   ConfigParam::Bool, const_cast<bool*>(&m_peakDet.m_edgePeakFilter)).Module("Peak Detection");
    s.emplace_back("EdgeThresholdEnabled", "Edge Threshold Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_peakDet.m_edgeThresholdEnabled)).Module("Peak Detection");
    s.emplace_back("EdgeThreshold", "Edge Threshold",
                   ConfigParam::Int, const_cast<int*>(&m_peakDet.m_edgeThreshold), 1, 1000).Module("Peak Detection");
    s.emplace_back("Z8Radius", "Z8 Max Search Radius",
                   ConfigParam::Int, const_cast<int*>(&m_peakDet.m_z8Radius), 1, 5).Module("Peak Detection");
    s.emplace_back("MaxPeaks", "Peak Limit Cap",
                   ConfigParam::Int, const_cast<int*>(&m_peakDet.m_maxPeaks), 5, 100).Module("Peak Detection");
    s.emplace_back("PressureDriftDebounce", "Pressure Debounce Limit",
                   ConfigParam::Int, const_cast<int*>(&m_peakDet.m_pressureDriftDebounceLimit), 0, 10).Module("Peak Detection");
    s.emplace_back("MacroZoneMinArea", "MacroZone Min Area",
                   ConfigParam::Int, const_cast<int*>(&m_peakDet.m_macroZoneMinArea), 1, 20).Module("Peak Detection");

    // ── Zone & Contact ──
    s.emplace_back("DilateErode", "Dilate Erode Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_contactExtractor.m_zoneExp.m_dilateErode)).Module("Zone & Contact");
    s.emplace_back("ZoneTholdScale", "Zone Thold Numer",
                   ConfigParam::Int, const_cast<int*>(&m_contactExtractor.m_zoneExp.m_tholdScaleNumer), 0, 255).Module("Zone & Contact");
    s.emplace_back("ZoneTholdShift", "Zone Thold Shift",
                   ConfigParam::Int, const_cast<int*>(&m_contactExtractor.m_zoneExp.m_tholdScaleShift), 0, 15).Module("Zone & Contact");
    s.emplace_back("MaxTouches", "Max Contact Outputs",
                   ConfigParam::Int, const_cast<int*>(&m_contactExtractor.m_zoneExp.m_maxTouches), 1, 50).Module("Zone & Contact");
    // ── Touch Classification ──
    s.emplace_back("PalmEnabled", "Touch Classification Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_touchClassifier.m_enabled)).Module("Touch Classification");
    s.emplace_back("PalmAreaThreshold", "Palm Area Threshold",
                   ConfigParam::Int, const_cast<int*>(&m_touchClassifier.m_areaThreshold), 5, 300).Module("Touch Classification");
    s.emplace_back("PalmSignalSumThreshold", "Palm SignalSum Threshold",
                   ConfigParam::Int, const_cast<int*>(&m_touchClassifier.m_signalSumThreshold), 1000, 500000).Module("Touch Classification");
    s.emplace_back("PalmDensityThresholdLow", "Palm Density Low Threshold",
                   ConfigParam::Float, const_cast<float*>(&m_touchClassifier.m_densityThresholdLow), 50.0f, 2000.0f).Module("Touch Classification");
    s.emplace_back("PalmAreaMinForDensity", "Palm Density Min Area",
                   ConfigParam::Int, const_cast<int*>(&m_touchClassifier.m_areaMinForDensity), 3, 100).Module("Touch Classification");
    s.emplace_back("PalmElongatedEnabled", "Elongated Press Reject",
                   ConfigParam::Bool, const_cast<bool*>(&m_touchClassifier.m_elongatedEnabled)).Module("Touch Classification");
    s.emplace_back("PalmElongatedMinArea", "Elongated Min Area",
                   ConfigParam::Int, const_cast<int*>(&m_touchClassifier.m_elongatedMinArea), 3, 100).Module("Touch Classification");
    s.emplace_back("PalmElongatedAspectRatio", "Elongated Aspect Ratio",
                   ConfigParam::Float, const_cast<float*>(&m_touchClassifier.m_elongatedAspectRatio), 1.5f, 10.0f).Module("Touch Classification");
    s.emplace_back("PalmAnalyzerEnabled", "Palm Analyzer Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_touchClassifier.m_analyzerEnabled)).Module("Touch Classification");
    s.emplace_back("PalmCandidateAreaThreshold", "Palm Candidate Area",
                   ConfigParam::Int, const_cast<int*>(&m_touchClassifier.m_candidateAreaThreshold), 5, 300).Module("Touch Classification");
    s.emplace_back("PalmCandidateSignalThreshold", "Palm Candidate Signal",
                   ConfigParam::Int, const_cast<int*>(&m_touchClassifier.m_candidateSignalThreshold), 1000, 500000).Module("Touch Classification");
    s.emplace_back("PalmLikelyAreaThreshold", "Palm Likely Area",
                   ConfigParam::Int, const_cast<int*>(&m_touchClassifier.m_likelyAreaThreshold), 5, 300).Module("Touch Classification");
    s.emplace_back("PalmFillRatioThreshold", "Palm Fill Ratio",
                   ConfigParam::Float, const_cast<float*>(&m_touchClassifier.m_fillRatioThreshold), 0.0f, 1.0f).Module("Touch Classification");
    s.emplace_back("PalmFlatSharpnessThreshold", "Palm Flat Sharpness",
                   ConfigParam::Float, const_cast<float*>(&m_touchClassifier.m_flatSharpnessThreshold), 1.0f, 4.0f).Module("Touch Classification");
    s.emplace_back("PalmStrongPeakProminence", "Palm Strong Peak Prominence",
                   ConfigParam::Int, const_cast<int*>(&m_touchClassifier.m_strongPeakProminence), 0, 5000).Module("Touch Classification");
    s.emplace_back("PeakEvalEnabled", "Peak Eval Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_touchClassifier.m_peakEvalEnabled)).Module("Touch Classification");
    s.emplace_back("PeakEvalFingerProminence", "Peak Eval Finger Prominence",
                   ConfigParam::Int, const_cast<int*>(&m_touchClassifier.m_fingerProminence), 0, 2000).Module("Touch Classification");
    s.emplace_back("PeakEvalFingerSharpness", "Peak Eval Finger Sharpness",
                   ConfigParam::Float, const_cast<float*>(&m_touchClassifier.m_fingerSharpness), 1.0f, 5.0f).Module("Touch Classification");
    s.emplace_back("PeakEvalPalmSharpnessMax", "Peak Eval Palm Sharpness Max",
                   ConfigParam::Float, const_cast<float*>(&m_touchClassifier.m_palmSharpnessMax), 1.0f, 5.0f).Module("Touch Classification");
    s.emplace_back("PeakEvalAmbiguousMargin", "Peak Eval Ambiguous Margin",
                   ConfigParam::Float, const_cast<float*>(&m_touchClassifier.m_ambiguousMargin), 0.0f, 1.0f).Module("Touch Classification");
    s.emplace_back("PalmAwareExpansionEnabled", "Palm Aware Expansion Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_touchClassifier.m_palmAwareExpansionEnabled)).Module("Touch Classification");
    s.emplace_back("PalmFingerInPalmThresholdRatio", "Finger In Palm Threshold Ratio",
                   ConfigParam::Float, const_cast<float*>(&m_touchClassifier.m_fingerInPalmThresholdRatio), 0.0f, 1.0f).Module("Touch Classification");
    s.emplace_back("PalmFingerInPalmMaxRadius", "Finger In Palm Max Radius",
                   ConfigParam::Int, const_cast<int*>(&m_touchClassifier.m_fingerInPalmMaxRadius), 0, 10).Module("Touch Classification");
    s.emplace_back("PalmLikelyAllowContact", "Palm Likely Allow Contact",
                   ConfigParam::Bool, const_cast<bool*>(&m_touchClassifier.m_palmLikelyAllowContact)).Module("Touch Classification");
    s.emplace_back("PalmShadowEnabled", "Palm Shadow Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_touchClassifier.m_palmShadowEnabled)).Module("Touch Classification");
    s.emplace_back("PalmShadowRadius", "Palm Shadow Radius",
                   ConfigParam::Int, const_cast<int*>(&m_touchClassifier.m_palmShadowRadius), 0, 8).Module("Touch Classification");
    s.emplace_back("PalmShadowHoldFrames", "Palm Shadow Hold Frames",
                   ConfigParam::Int, const_cast<int*>(&m_touchClassifier.m_palmShadowHoldFrames), 0, 60).Module("Touch Classification");
    s.emplace_back("PalmShadowSeedScore", "Palm Shadow Seed Score",
                   ConfigParam::Float, const_cast<float*>(&m_touchClassifier.m_palmShadowSeedScore), 0.0f, 1.0f).Module("Touch Classification");

    // ── Tracking ──
    s.emplace_back("TrackerEnabled", "Tracker Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_tracker.m_enabled)).Module("Tracking");
    s.emplace_back("UseHungarian", "Use Hungarian",
                   ConfigParam::Bool, const_cast<bool*>(&m_tracker.m_useHungarian)).Module("Tracking");
    s.emplace_back("MaxTrackDistance", "Max Track Dist",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_maxTrackDistance), 1.0f, 20.0f).Module("Tracking");
    s.emplace_back("AlwaysMatchDistance", "Always Match Dist",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_alwaysMatchDistance), 0.5f, 6.0f).Module("Tracking");
    s.emplace_back("EdgeTrackBoost", "Edge Track Boost",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_edgeTrackBoost), 1.0f, 5.0f).Module("Tracking");
    s.emplace_back("AccThresholdBoost", "Accel Gate Boost",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_accThresholdBoost), 1.0f, 8.0f).Module("Tracking");
    s.emplace_back("AccBoostSizeMm", "Accel Boost Size",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_accBoostSizeMm), 0.1f, 5.0f).Module("Tracking");
    s.emplace_back("PredictionScale", "Prediction Scale",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_predictionScale), 0.0f, 2.0f).Module("Tracking");
    s.emplace_back("GapRelinkEnabled", "Gap Relink Enable",
                   ConfigParam::Bool, const_cast<bool*>(&m_tracker.m_gapRelinkEnabled)).Module("Tracking");
    s.emplace_back("GapRelinkWindowFrames", "Gap Relink Window",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_gapRelinkWindowFrames), 0, 5).Module("Tracking");
    s.emplace_back("TouchDownDebounceFrames", "Down Debounce",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_touchDownDebounceFrames), 0, 10).Module("Tracking");
    s.emplace_back("DynamicDebounceEnabled", "Dynamic Debounce",
                   ConfigParam::Bool, const_cast<bool*>(&m_tracker.m_dynamicDebounceEnabled)).Module("Tracking");
    s.emplace_back("TouchDownDebounceMaxExtra", "Debounce Extra Cap",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_touchDownDebounceMaxExtra), 0, 10).Module("Tracking");
    s.emplace_back("TouchDownWeakSignalThreshold", "Weak Signal Threshold",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_touchDownWeakSignalThreshold), 0, 2000).Module("Tracking");
    s.emplace_back("TouchDownSmallSizeThresholdMm", "Small Size Threshold",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_touchDownSmallSizeThresholdMm), 0.1f, 5.0f).Module("Tracking");
    s.emplace_back("TouchDownRejectEnabled", "Enable Reject",
                   ConfigParam::Bool, const_cast<bool*>(&m_tracker.m_touchDownRejectEnabled)).Module("Tracking");
    s.emplace_back("TouchDownRejectMinSignal", "Reject Signal Th",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_touchDownRejectMinSignal), 0, 500).Module("Tracking");
    s.emplace_back("TouchDownRejectMinSizeMm", "Reject Size Th",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_touchDownRejectMinSizeMm), 0.1f, 5.0f).Module("Tracking");
    s.emplace_back("TouchDownEdgeRejectMinSignal", "Edge Reject Signal",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_touchDownEdgeRejectMinSignal), 0, 2000).Module("Tracking");
    s.emplace_back("FallbackSizeMm", "Fallback Size",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_fallbackSizeMm), 0.1f, 10.0f).Module("Tracking");
    s.emplace_back("SizeAreaScale", "Size Area Scale",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_sizeAreaScale), 0.0f, 5.0f).Module("Tracking");
    s.emplace_back("SizeSignalScale", "Size Signal Scale",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_sizeSignalScale), 0.0f, 5.0f).Module("Tracking");
    s.emplace_back("RxGhostFilterEnabled", "RX Ghost Filter",
                   ConfigParam::Bool, const_cast<bool*>(&m_tracker.m_rxGhostFilterEnabled)).Module("Tracking");
    s.emplace_back("RxGhostLineDelta", "RX Ghost Delta",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_rxGhostLineDelta), 0, 10).Module("Tracking");
    s.emplace_back("RxGhostWeakRatio", "RX Ghost Weak Ratio",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_rxGhostWeakRatio), 0.0f, 1.0f).Module("Tracking");
    s.emplace_back("RxGhostOnlyNew", "RX Ghost Only New",
                   ConfigParam::Bool, const_cast<bool*>(&m_tracker.m_rxGhostOnlyNew)).Module("Tracking");

    // ── Stylus Suppress ──
    s.emplace_back("StylusSuppressGlobalEnabled", "Pen Global Suppress",
                   ConfigParam::Bool, const_cast<bool*>(&m_tracker.m_stylusSuppressGlobalEnabled)).Module("Stylus Suppress");
    s.emplace_back("StylusSuppressLocalEnabled", "Pen Local Suppress",
                   ConfigParam::Bool, const_cast<bool*>(&m_tracker.m_stylusSuppressLocalEnabled)).Module("Stylus Suppress");
    s.emplace_back("StylusSuppressLocalDistance", "Suppress Radius",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_stylusSuppressLocalDistance), 0.5f, 10.0f).Module("Stylus Suppress");
    s.emplace_back("StylusSuppressPenPeakThreshold", "Pen Peak Threshold",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_stylusSuppressPenPeakThreshold), 0, 10000).Module("Stylus Suppress");
    s.emplace_back("StylusSuppressTouchSignalKeep", "Touch Signal Keep",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_stylusSuppressTouchSignalKeep), 0, 30000).Module("Stylus Suppress");
    s.emplace_back("StylusSuppressTouchAreaKeep", "Touch Area Keep",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_stylusSuppressTouchAreaKeep), 0, 100).Module("Stylus Suppress");
    s.emplace_back("StylusAftEnabled", "Enable AFT (Anti-Falsing)",
                   ConfigParam::Bool, const_cast<bool*>(&m_tracker.m_stylusAftEnabled)).Module("Stylus Suppress");
    s.emplace_back("StylusAftRecentFrames", "AFT Recent Frames",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_stylusAftRecentFrames), 0, 200).Module("Stylus Suppress");
    s.emplace_back("StylusAftRadius", "AFT Radius",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_stylusAftRadius), 0.5f, 10.0f).Module("Stylus Suppress");
    s.emplace_back("StylusAftDebounceFrames", "AFT Debounce Frames",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_stylusAftDebounceFrames), 0, 30).Module("Stylus Suppress");
    s.emplace_back("StylusAftWeakSignalThreshold", "AFT Weak Signal",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_stylusAftWeakSignalThreshold), 0, 5000).Module("Stylus Suppress");
    s.emplace_back("StylusAftWeakSizeThresholdMm", "AFT Weak Size",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_stylusAftWeakSizeThresholdMm), 0.1f, 10.0f).Module("Stylus Suppress");
    s.emplace_back("StylusAftSuppressFrames", "AFT Suppress Frames",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_stylusAftSuppressFrames), 0, 200).Module("Stylus Suppress");
    s.emplace_back("StylusAftPalmSuppressFrames", "AFT Palm Suppress Frames",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_stylusAftPalmSuppressFrames), 0, 300).Module("Stylus Suppress");
    s.emplace_back("StylusAftPalmAreaThreshold", "AFT Palm Area",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_stylusAftPalmAreaThreshold), 0, 500).Module("Stylus Suppress");
    s.emplace_back("StylusAftPalmSizeThresholdMm", "AFT Palm Size",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_stylusAftPalmSizeThresholdMm), 0.1f, 20.0f).Module("Stylus Suppress");

    // ── Coordinate Filter ──
    s.emplace_back("CoordFilterEnabled", "Coord Filter Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_coordFilter.m_enabled)).Module("Coordinate Filter");
    s.emplace_back("OneEuroMinCutoff", "1€ Min Cutoff",
                   ConfigParam::Float, const_cast<float*>(&m_coordFilter.m_minCutoff), 0.1f, 20.0f).Module("Coordinate Filter");
    s.emplace_back("OneEuroBeta", "1€ Beta",
                   ConfigParam::Float, const_cast<float*>(&m_coordFilter.m_beta), 0.0f, 0.5f).Module("Coordinate Filter");
    s.emplace_back("OneEuroDCutoff", "1€ Deriv Cutoff",
                   ConfigParam::Float, const_cast<float*>(&m_coordFilter.m_dCutoff), 0.1f, 10.0f).Module("Coordinate Filter");

    // ── Gesture ──
    s.emplace_back("GestureEnabled", "Gesture SM Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_gesture.m_enabled)).Module("Gesture");
    s.emplace_back("PressCandidateFrames", "Press Candidate Frames",
                   ConfigParam::Int, const_cast<int*>(&m_gesture.m_pressCandidateFrames), 1, 10).Module("Gesture");
    s.emplace_back("DragThreshold", "Drag Threshold",
                   ConfigParam::Float, const_cast<float*>(&m_gesture.m_dragThreshold), 0.1f, 5.0f).Module("Gesture");
    s.emplace_back("LongPressFrames", "LongPress Frames",
                   ConfigParam::Int, const_cast<int*>(&m_gesture.m_longPressFrames), 10, 120).Module("Gesture");
    s.emplace_back("ReleasePendingFrames", "Release Pending Frames",
                   ConfigParam::Int, const_cast<int*>(&m_gesture.m_releasePendingFrames), 0, 10).Module("Gesture");
    s.emplace_back("BypassStateMachine", "Bypass State Machine",
                   ConfigParam::Bool, const_cast<bool*>(&m_gesture.m_bypassStateMachine)).Module("Gesture");

    s.erase(std::remove_if(s.begin(), s.end(), [](const ConfigParam& param) {
        return IsFrozenCurrentTouchConfigKey(param.key);
    }), s.end());

    return s;
}

// ══════════════════════════════════════════════════════════════════════
// SaveConfig
// ══════════════════════════════════════════════════════════════════════
void TouchPipeline::SaveConfig(std::ostream& out) const {
    std::ostringstream serialized;
    auto& configOut = serialized;

    // Phase 1
    configOut << "FrameParserEnabled=" << (m_frameParser.m_enabled?"1":"0") << "\n";
    // Phase 2: Baseline
    configOut << "BaselineEnabled=" << (m_baseline.m_enabled?"1":"0") << "\n";
    configOut << "BaselineValue=" << m_baseline.m_baseline << "\n";
    configOut << "BaselineNoiseDeadband=" << m_baseline.m_noiseDeadband << "\n";
    configOut << "BaselinePositiveDeadband=" << m_baseline.m_positiveDeadband << "\n";
    configOut << "BaselineNegativeDeadband=" << m_baseline.m_negativeDeadband << "\n";
    configOut << "BaselinePeakThreshold=" << m_baseline.m_peakThreshold << "\n";
    configOut << "BaselineReleaseHoldFrames=" << m_baseline.m_releaseHoldFrames << "\n";
    configOut << "BaselinePositiveAlphaShift=" << m_baseline.m_positiveAlphaShift << "\n";
    configOut << "BaselineNegativeAlphaShift=" << m_baseline.m_negativeAlphaShift << "\n";
    configOut << "BaselineNoiseAlphaShift=" << m_baseline.m_noiseAlphaShift << "\n";
    configOut << "BaselineBackgroundAlphaShift=" << m_baseline.m_backgroundAlphaShift << "\n";
    configOut << "BaselineNoFingerAlphaShift=" << m_baseline.m_noFingerAlphaShift << "\n";
    configOut << "BaselinePositiveMaxStep=" << m_baseline.m_positiveMaxStep << "\n";
    configOut << "BaselineNegativeMaxStep=" << m_baseline.m_negativeMaxStep << "\n";
    configOut << "BaselineBackgroundMaxStep=" << m_baseline.m_backgroundMaxStep << "\n";
    configOut << "BaselineNoFingerMaxStep=" << m_baseline.m_noFingerMaxStep << "\n";
    configOut << "BaselineRecoveryAlphaShift=" << m_baseline.m_recoveryAlphaShift << "\n";
    configOut << "BaselineRecoveryMaxStep=" << m_baseline.m_recoveryMaxStep << "\n";
    configOut << "BaselineRecoveryMaxFrames=" << m_baseline.m_recoveryMaxFrames << "\n";
    configOut << "BaselineNoiseTrackingEnabled=" << (m_baseline.m_noiseTrackingEnabled?"1":"0") << "\n";
    // Phase 2: CMF
    configOut << "CMFEnabled=" << (m_cmf.m_enabled?"1":"0") << "\n";
    configOut << "CMFDimensionMode=" << static_cast<int>(m_cmf.m_mode) << "\n";
    configOut << "CMFExclusionThreshold=" << m_cmf.m_exclusionThreshold << "\n";
    configOut << "CMFMaxCorrection=" << m_cmf.m_maxCorrection << "\n";
    // Phase 3: PeakDetector (same keys as old FeatureExtractor)
    configOut << "PeakThreshold=" << m_peakDet.m_threshold << "\n";
    configOut << "SigTholdLimit=" << m_peakDet.m_sigTholdLimit << "\n";
    configOut << "Z8FilterEnabled=" << (m_peakDet.m_z8Filter?"1":"0") << "\n";
    configOut << "Z1FilterEnabled=" << (m_peakDet.m_z1Filter?"1":"0") << "\n";
    configOut << "PressureDriftFilter=" << (m_peakDet.m_pressureDriftFilter?"1":"0") << "\n";
    configOut << "EdgePeakFilter=" << (m_peakDet.m_edgePeakFilter?"1":"0") << "\n";
    configOut << "EdgeThresholdEnabled=" << (m_peakDet.m_edgeThresholdEnabled?"1":"0") << "\n";
    configOut << "EdgeThreshold=" << m_peakDet.m_edgeThreshold << "\n";
    configOut << "Z8Radius=" << m_peakDet.m_z8Radius << "\n";
    configOut << "MaxPeaks=" << m_peakDet.m_maxPeaks << "\n";
    configOut << "PressureDriftDebounce=" << m_peakDet.m_pressureDriftDebounceLimit << "\n";
    configOut << "MacroZoneMinArea=" << m_peakDet.m_macroZoneMinArea << "\n";
    // Phase 4: ZoneExpander
    configOut << "DilateErode=" << (m_contactExtractor.m_zoneExp.m_dilateErode?"1":"0") << "\n";
    configOut << "ZoneTholdScale=" << m_contactExtractor.m_zoneExp.m_tholdScaleNumer << "\n";
    configOut << "ZoneTholdShift=" << m_contactExtractor.m_zoneExp.m_tholdScaleShift << "\n";
    configOut << "MaxTouches=" << m_contactExtractor.m_zoneExp.m_maxTouches << "\n";
    // Phase 3: TouchClassifier
    configOut << "PalmEnabled=" << (m_touchClassifier.m_enabled?"1":"0") << "\n";
    configOut << "PalmAreaThreshold=" << m_touchClassifier.m_areaThreshold << "\n";
    configOut << "PalmSignalSumThreshold=" << m_touchClassifier.m_signalSumThreshold << "\n";
    configOut << "PalmDensityThresholdLow=" << m_touchClassifier.m_densityThresholdLow << "\n";
    configOut << "PalmAreaMinForDensity=" << m_touchClassifier.m_areaMinForDensity << "\n";
    configOut << "PalmElongatedEnabled=" << (m_touchClassifier.m_elongatedEnabled?"1":"0") << "\n";
    configOut << "PalmElongatedMinArea=" << m_touchClassifier.m_elongatedMinArea << "\n";
    configOut << "PalmElongatedAspectRatio=" << m_touchClassifier.m_elongatedAspectRatio << "\n";
    configOut << "PalmAnalyzerEnabled=" << (m_touchClassifier.m_analyzerEnabled?"1":"0") << "\n";
    configOut << "PalmCandidateAreaThreshold=" << m_touchClassifier.m_candidateAreaThreshold << "\n";
    configOut << "PalmCandidateSignalThreshold=" << m_touchClassifier.m_candidateSignalThreshold << "\n";
    configOut << "PalmLikelyAreaThreshold=" << m_touchClassifier.m_likelyAreaThreshold << "\n";
    configOut << "PalmFillRatioThreshold=" << m_touchClassifier.m_fillRatioThreshold << "\n";
    configOut << "PalmFlatSharpnessThreshold=" << m_touchClassifier.m_flatSharpnessThreshold << "\n";
    configOut << "PalmStrongPeakProminence=" << m_touchClassifier.m_strongPeakProminence << "\n";
    configOut << "PeakEvalEnabled=" << (m_touchClassifier.m_peakEvalEnabled?"1":"0") << "\n";
    configOut << "PeakEvalFingerProminence=" << m_touchClassifier.m_fingerProminence << "\n";
    configOut << "PeakEvalFingerSharpness=" << m_touchClassifier.m_fingerSharpness << "\n";
    configOut << "PeakEvalPalmSharpnessMax=" << m_touchClassifier.m_palmSharpnessMax << "\n";
    configOut << "PeakEvalAmbiguousMargin=" << m_touchClassifier.m_ambiguousMargin << "\n";
    configOut << "PalmAwareExpansionEnabled=" << (m_touchClassifier.m_palmAwareExpansionEnabled?"1":"0") << "\n";
    configOut << "PalmFingerInPalmThresholdRatio=" << m_touchClassifier.m_fingerInPalmThresholdRatio << "\n";
    configOut << "PalmFingerInPalmMaxRadius=" << m_touchClassifier.m_fingerInPalmMaxRadius << "\n";
    configOut << "PalmLikelyAllowContact=" << (m_touchClassifier.m_palmLikelyAllowContact?"1":"0") << "\n";
    configOut << "PalmShadowEnabled=" << (m_touchClassifier.m_palmShadowEnabled?"1":"0") << "\n";
    configOut << "PalmShadowRadius=" << m_touchClassifier.m_palmShadowRadius << "\n";
    configOut << "PalmShadowHoldFrames=" << m_touchClassifier.m_palmShadowHoldFrames << "\n";
    configOut << "PalmShadowSeedScore=" << m_touchClassifier.m_palmShadowSeedScore << "\n";
    // Phase 5: TouchTracker (same keys as old TouchTracker)
    configOut << "TrackerEnabled=" << (m_tracker.m_enabled?"1":"0") << "\n";
    configOut << "UseHungarian=" << (m_tracker.m_useHungarian?"1":"0") << "\n";
    configOut << "MaxTrackDistance=" << m_tracker.m_maxTrackDistance << "\n";
    configOut << "AlwaysMatchDistance=" << m_tracker.m_alwaysMatchDistance << "\n";
    configOut << "EdgeTrackBoost=" << m_tracker.m_edgeTrackBoost << "\n";
    configOut << "AccThresholdBoost=" << m_tracker.m_accThresholdBoost << "\n";
    configOut << "AccBoostSizeMm=" << m_tracker.m_accBoostSizeMm << "\n";
    configOut << "PredictionScale=" << m_tracker.m_predictionScale << "\n";
    configOut << "GapRelinkEnabled=" << (m_tracker.m_gapRelinkEnabled?"1":"0") << "\n";
    configOut << "GapRelinkWindowFrames=" << m_tracker.m_gapRelinkWindowFrames << "\n";
    configOut << "TouchDownDebounceFrames=" << m_tracker.m_touchDownDebounceFrames << "\n";
    configOut << "DynamicDebounceEnabled=" << (m_tracker.m_dynamicDebounceEnabled?"1":"0") << "\n";
    configOut << "TouchDownDebounceMaxExtra=" << m_tracker.m_touchDownDebounceMaxExtra << "\n";
    configOut << "TouchDownWeakSignalThreshold=" << m_tracker.m_touchDownWeakSignalThreshold << "\n";
    configOut << "TouchDownSmallSizeThresholdMm=" << m_tracker.m_touchDownSmallSizeThresholdMm << "\n";
    configOut << "TouchDownRejectEnabled=" << (m_tracker.m_touchDownRejectEnabled?"1":"0") << "\n";
    configOut << "TouchDownRejectMinSignal=" << m_tracker.m_touchDownRejectMinSignal << "\n";
    configOut << "TouchDownRejectMinSizeMm=" << m_tracker.m_touchDownRejectMinSizeMm << "\n";
    configOut << "TouchDownEdgeRejectMinSignal=" << m_tracker.m_touchDownEdgeRejectMinSignal << "\n";
    configOut << "FallbackSizeMm=" << m_tracker.m_fallbackSizeMm << "\n";
    configOut << "SizeAreaScale=" << m_tracker.m_sizeAreaScale << "\n";
    configOut << "SizeSignalScale=" << m_tracker.m_sizeSignalScale << "\n";
    configOut << "RxGhostFilterEnabled=" << (m_tracker.m_rxGhostFilterEnabled?"1":"0") << "\n";
    configOut << "RxGhostLineDelta=" << m_tracker.m_rxGhostLineDelta << "\n";
    configOut << "RxGhostWeakRatio=" << m_tracker.m_rxGhostWeakRatio << "\n";
    configOut << "RxGhostOnlyNew=" << (m_tracker.m_rxGhostOnlyNew?"1":"0") << "\n";
    configOut << "StylusSuppressGlobalEnabled=" << (m_tracker.m_stylusSuppressGlobalEnabled?"1":"0") << "\n";
    configOut << "StylusSuppressLocalEnabled=" << (m_tracker.m_stylusSuppressLocalEnabled?"1":"0") << "\n";
    configOut << "StylusSuppressLocalDistance=" << m_tracker.m_stylusSuppressLocalDistance << "\n";
    configOut << "StylusSuppressPenPeakThreshold=" << m_tracker.m_stylusSuppressPenPeakThreshold << "\n";
    configOut << "StylusSuppressTouchSignalKeep=" << m_tracker.m_stylusSuppressTouchSignalKeep << "\n";
    configOut << "StylusSuppressTouchAreaKeep=" << m_tracker.m_stylusSuppressTouchAreaKeep << "\n";
    configOut << "StylusAftEnabled=" << (m_tracker.m_stylusAftEnabled?"1":"0") << "\n";
    configOut << "StylusAftRecentFrames=" << m_tracker.m_stylusAftRecentFrames << "\n";
    configOut << "StylusAftRadius=" << m_tracker.m_stylusAftRadius << "\n";
    configOut << "StylusAftDebounceFrames=" << m_tracker.m_stylusAftDebounceFrames << "\n";
    configOut << "StylusAftWeakSignalThreshold=" << m_tracker.m_stylusAftWeakSignalThreshold << "\n";
    configOut << "StylusAftWeakSizeThresholdMm=" << m_tracker.m_stylusAftWeakSizeThresholdMm << "\n";
    configOut << "StylusAftSuppressFrames=" << m_tracker.m_stylusAftSuppressFrames << "\n";
    configOut << "StylusAftPalmSuppressFrames=" << m_tracker.m_stylusAftPalmSuppressFrames << "\n";
    configOut << "StylusAftPalmAreaThreshold=" << m_tracker.m_stylusAftPalmAreaThreshold << "\n";
    configOut << "StylusAftPalmSizeThresholdMm=" << m_tracker.m_stylusAftPalmSizeThresholdMm << "\n";
    // Phase 5: CoordinateFilter
    configOut << "CoordFilterEnabled=" << (m_coordFilter.m_enabled?"1":"0") << "\n";
    configOut << "OneEuroMinCutoff=" << m_coordFilter.m_minCutoff << "\n";
    configOut << "OneEuroBeta=" << m_coordFilter.m_beta << "\n";
    configOut << "OneEuroDCutoff=" << m_coordFilter.m_dCutoff << "\n";
    // Phase 6: GestureStateMachine
    configOut << "GestureEnabled=" << (m_gesture.m_enabled?"1":"0") << "\n";
    configOut << "PressCandidateFrames=" << m_gesture.m_pressCandidateFrames << "\n";
    configOut << "PressCandidateMinSignal=" << m_gesture.m_pressCandidateMinSignal << "\n";
    configOut << "PressCandidateMinSizeMm=" << m_gesture.m_pressCandidateMinSizeMm << "\n";
    configOut << "DragThreshold=" << m_gesture.m_dragThreshold << "\n";
    configOut << "LongPressFrames=" << m_gesture.m_longPressFrames << "\n";
    configOut << "LongPressMoveTolerance=" << m_gesture.m_longPressMoveTolerance << "\n";
    configOut << "ReleasePendingFrames=" << m_gesture.m_releasePendingFrames << "\n";
    configOut << "BypassStateMachine=" << (m_gesture.m_bypassStateMachine?"1":"0") << "\n";

    std::istringstream lines(serialized.str());
    std::string line;
    while (std::getline(lines, line)) {
        const size_t separator = line.find('=');
        if (separator != std::string::npos &&
            IsFrozenCurrentTouchConfigKey(std::string_view(line).substr(0, separator))) {
            continue;
        }
        out << line << '\n';
    }
}

// ══════════════════════════════════════════════════════════════════════
// LoadConfig — key/value dispatch (compatible with old config keys)
// ══════════════════════════════════════════════════════════════════════
void TouchPipeline::LoadConfig(const std::string& key,
                                const std::string& value) {
    if (IsFrozenCurrentTouchConfigKey(key)) return;

    auto toBool = [&](const std::string& v) { return ParseConfigBool(key, v); };
    try {
    // Phase 1
    if      (key=="FrameParserEnabled")      m_frameParser.m_enabled = toBool(value);
    // Phase 2: Baseline
    else if (key=="BaselineEnabled")         m_baseline.m_enabled = toBool(value);
    else if (key=="BaselineValue")           { m_baseline.m_baseline = ParseConfigInt(key, value); m_baseline.Reset(); }
    else if (key=="BaselineNoiseDeadband")   m_baseline.m_noiseDeadband = ParseConfigInt(key, value);
    else if (key=="BaselinePositiveDeadband") m_baseline.m_positiveDeadband = ParseConfigInt(key, value);
    else if (key=="BaselineNegativeDeadband") m_baseline.m_negativeDeadband = ParseConfigInt(key, value);
    else if (key=="BaselinePeakThreshold") m_baseline.m_peakThreshold = ParseConfigInt(key, value);
    else if (key=="BaselineReleaseHoldFrames") m_baseline.m_releaseHoldFrames = ParseConfigInt(key, value);
    else if (key=="BaselinePositiveAlphaShift") m_baseline.m_positiveAlphaShift = ParseConfigInt(key, value);
    else if (key=="BaselineNegativeAlphaShift") m_baseline.m_negativeAlphaShift = ParseConfigInt(key, value);
    else if (key=="BaselineNoiseAlphaShift") m_baseline.m_noiseAlphaShift = ParseConfigInt(key, value);
    else if (key=="BaselineBackgroundAlphaShift") m_baseline.m_backgroundAlphaShift = ParseConfigInt(key, value);
    else if (key=="BaselineNoFingerAlphaShift") m_baseline.m_noFingerAlphaShift = std::clamp(ParseConfigInt(key, value), 0, 15);
    else if (key=="BaselinePositiveMaxStep") m_baseline.m_positiveMaxStep = ParseConfigInt(key, value);
    else if (key=="BaselineNegativeMaxStep") m_baseline.m_negativeMaxStep = ParseConfigInt(key, value);
    else if (key=="BaselineBackgroundMaxStep") m_baseline.m_backgroundMaxStep = ParseConfigInt(key, value);
    else if (key=="BaselineNoFingerMaxStep") m_baseline.m_noFingerMaxStep = std::clamp(ParseConfigInt(key, value), 1, 2048);
    else if (key=="BaselineRecoveryAlphaShift") m_baseline.m_recoveryAlphaShift = std::clamp(ParseConfigInt(key, value), 0, 15);
    else if (key=="BaselineRecoveryMaxStep") m_baseline.m_recoveryMaxStep = std::clamp(ParseConfigInt(key, value), 1, 2048);
    else if (key=="BaselineRecoveryMaxFrames") m_baseline.m_recoveryMaxFrames = std::clamp(ParseConfigInt(key, value), 1, 120);
    else if (key=="BaselineNoiseTrackingEnabled") m_baseline.m_noiseTrackingEnabled = toBool(value);
    // Phase 2: CMF
    else if (key=="CMFEnabled")              m_cmf.m_enabled = toBool(value);
    else if (key=="CMFDimensionMode")        m_cmf.m_mode = static_cast<Touch::CMFProcessor::DimensionMode>(ParseConfigInt(key, value));
    else if (key=="CMFExclusionThreshold")   m_cmf.m_exclusionThreshold = ParseConfigInt(key, value);
    else if (key=="CMFMaxCorrection")        m_cmf.m_maxCorrection = ParseConfigInt(key, value);
    // Phase 3: PeakDetector
    else if (key=="PeakThreshold")           m_peakDet.m_threshold = ParseConfigInt(key, value);
    else if (key=="SigTholdLimit")           m_peakDet.m_sigTholdLimit = ParseConfigInt(key, value);
    else if (key=="Z8FilterEnabled")         m_peakDet.m_z8Filter = toBool(value);
    else if (key=="Z1FilterEnabled")         m_peakDet.m_z1Filter = toBool(value);
    else if (key=="PressureDriftFilter")     m_peakDet.m_pressureDriftFilter = toBool(value);
    else if (key=="EdgePeakFilter")          m_peakDet.m_edgePeakFilter = toBool(value);
    else if (key=="EdgeThresholdEnabled")    m_peakDet.m_edgeThresholdEnabled = toBool(value);
    else if (key=="EdgeThreshold")           m_peakDet.m_edgeThreshold = ParseConfigInt(key, value);
    else if (key=="Z8Radius")                m_peakDet.m_z8Radius = ParseConfigInt(key, value);
    else if (key=="MaxPeaks")                m_peakDet.m_maxPeaks = std::clamp(ParseConfigInt(key, value), 1, Touch::PeakDetector::kMaxStoredPeaks);
    else if (key=="PressureDriftDebounce")   m_peakDet.m_pressureDriftDebounceLimit = ParseConfigInt(key, value);
    else if (key=="MacroZoneMinArea")        m_peakDet.m_macroZoneMinArea = ParseConfigInt(key, value);
    // Phase 4: ZoneExpander
    else if (key=="DilateErode")             m_contactExtractor.m_zoneExp.m_dilateErode = toBool(value);
    else if (key=="ZoneTholdScale")          m_contactExtractor.m_zoneExp.m_tholdScaleNumer = ParseConfigInt(key, value);
    else if (key=="ZoneTholdShift")          m_contactExtractor.m_zoneExp.m_tholdScaleShift = ParseConfigInt(key, value);
    else if (key=="MaxTouches")              m_contactExtractor.m_zoneExp.m_maxTouches = ParseConfigInt(key, value);
    // Phase 3: TouchClassifier
    else if (key=="PalmEnabled")             m_touchClassifier.m_enabled = toBool(value);
    else if (key=="PalmAreaThreshold")       m_touchClassifier.m_areaThreshold = ParseConfigInt(key, value);
    else if (key=="PalmSignalSumThreshold")  m_touchClassifier.m_signalSumThreshold = ParseConfigInt(key, value);
    else if (key=="PalmDensityThresholdLow") m_touchClassifier.m_densityThresholdLow = ParseConfigFloat(key, value);
    else if (key=="PalmAreaMinForDensity")   m_touchClassifier.m_areaMinForDensity = ParseConfigInt(key, value);
    else if (key=="PalmElongatedEnabled")    m_touchClassifier.m_elongatedEnabled = toBool(value);
    else if (key=="PalmElongatedMinArea")    m_touchClassifier.m_elongatedMinArea = ParseConfigInt(key, value);
    else if (key=="PalmElongatedAspectRatio")m_touchClassifier.m_elongatedAspectRatio = ParseConfigFloat(key, value);
    else if (key=="PalmAnalyzerEnabled")      m_touchClassifier.m_analyzerEnabled = toBool(value);
    else if (key=="PalmCandidateAreaThreshold") m_touchClassifier.m_candidateAreaThreshold = ParseConfigInt(key, value);
    else if (key=="PalmCandidateSignalThreshold") m_touchClassifier.m_candidateSignalThreshold = ParseConfigInt(key, value);
    else if (key=="PalmLikelyAreaThreshold")  m_touchClassifier.m_likelyAreaThreshold = ParseConfigInt(key, value);
    else if (key=="PalmFillRatioThreshold")   m_touchClassifier.m_fillRatioThreshold = ParseConfigFloat(key, value);
    else if (key=="PalmFlatSharpnessThreshold") m_touchClassifier.m_flatSharpnessThreshold = ParseConfigFloat(key, value);
    else if (key=="PalmStrongPeakProminence") m_touchClassifier.m_strongPeakProminence = ParseConfigInt(key, value);
    else if (key=="PeakEvalEnabled")          m_touchClassifier.m_peakEvalEnabled = toBool(value);
    else if (key=="PeakEvalFingerProminence") m_touchClassifier.m_fingerProminence = ParseConfigInt(key, value);
    else if (key=="PeakEvalFingerSharpness")  m_touchClassifier.m_fingerSharpness = ParseConfigFloat(key, value);
    else if (key=="PeakEvalPalmSharpnessMax") m_touchClassifier.m_palmSharpnessMax = ParseConfigFloat(key, value);
    else if (key=="PeakEvalAmbiguousMargin")  m_touchClassifier.m_ambiguousMargin = ParseConfigFloat(key, value);
    else if (key=="PalmAwareExpansionEnabled") m_touchClassifier.m_palmAwareExpansionEnabled = toBool(value);
    else if (key=="PalmFingerInPalmThresholdRatio") m_touchClassifier.m_fingerInPalmThresholdRatio = ParseConfigFloat(key, value);
    else if (key=="PalmFingerInPalmMaxRadius") m_touchClassifier.m_fingerInPalmMaxRadius = ParseConfigInt(key, value);
    else if (key=="PalmLikelyAllowContact")   m_touchClassifier.m_palmLikelyAllowContact = toBool(value);
    else if (key=="PalmShadowEnabled")        m_touchClassifier.m_palmShadowEnabled = toBool(value);
    else if (key=="PalmShadowRadius")         m_touchClassifier.m_palmShadowRadius = ParseConfigInt(key, value);
    else if (key=="PalmShadowHoldFrames")     m_touchClassifier.m_palmShadowHoldFrames = ParseConfigInt(key, value);
    else if (key=="PalmShadowSeedScore")      m_touchClassifier.m_palmShadowSeedScore = ParseConfigFloat(key, value);
    // Phase 5: TouchTracker
    else if (key=="TrackerEnabled")          m_tracker.m_enabled = toBool(value);
    else if (key=="UseHungarian")            m_tracker.m_useHungarian = toBool(value);
    else if (key=="MaxTrackDistance")        m_tracker.m_maxTrackDistance = ParseConfigFloat(key, value);
    else if (key=="AlwaysMatchDistance")     m_tracker.m_alwaysMatchDistance = ParseConfigFloat(key, value);
    else if (key=="EdgeTrackBoost")          m_tracker.m_edgeTrackBoost = ParseConfigFloat(key, value);
    else if (key=="AccThresholdBoost")       m_tracker.m_accThresholdBoost = ParseConfigFloat(key, value);
    else if (key=="AccBoostSizeMm")          m_tracker.m_accBoostSizeMm = ParseConfigFloat(key, value);
    else if (key=="PredictionScale")         m_tracker.m_predictionScale = ParseConfigFloat(key, value);
    else if (key=="GapRelinkEnabled")        m_tracker.m_gapRelinkEnabled = toBool(value);
    else if (key=="GapRelinkWindowFrames")   m_tracker.m_gapRelinkWindowFrames = ParseConfigInt(key, value);
    else if (key=="TouchDownDebounceFrames") m_tracker.m_touchDownDebounceFrames = ParseConfigInt(key, value);
    else if (key=="DynamicDebounceEnabled")  m_tracker.m_dynamicDebounceEnabled = toBool(value);
    else if (key=="TouchDownDebounceMaxExtra") m_tracker.m_touchDownDebounceMaxExtra = ParseConfigInt(key, value);
    else if (key=="TouchDownWeakSignalThreshold") m_tracker.m_touchDownWeakSignalThreshold = ParseConfigInt(key, value);
    else if (key=="TouchDownSmallSizeThresholdMm") m_tracker.m_touchDownSmallSizeThresholdMm = ParseConfigFloat(key, value);
    else if (key=="TouchDownRejectEnabled")  m_tracker.m_touchDownRejectEnabled = toBool(value);
    else if (key=="TouchDownRejectMinSignal")m_tracker.m_touchDownRejectMinSignal = ParseConfigInt(key, value);
    else if (key=="TouchDownRejectMinSizeMm") m_tracker.m_touchDownRejectMinSizeMm = ParseConfigFloat(key, value);
    else if (key=="TouchDownEdgeRejectMinSignal") m_tracker.m_touchDownEdgeRejectMinSignal = ParseConfigInt(key, value);
    else if (key=="FallbackSizeMm")          m_tracker.m_fallbackSizeMm = ParseConfigFloat(key, value);
    else if (key=="SizeAreaScale")           m_tracker.m_sizeAreaScale = ParseConfigFloat(key, value);
    else if (key=="SizeSignalScale")         m_tracker.m_sizeSignalScale = ParseConfigFloat(key, value);
    else if (key=="RxGhostFilterEnabled")    m_tracker.m_rxGhostFilterEnabled = toBool(value);
    else if (key=="RxGhostLineDelta")        m_tracker.m_rxGhostLineDelta = ParseConfigInt(key, value);
    else if (key=="RxGhostWeakRatio")        m_tracker.m_rxGhostWeakRatio = ParseConfigFloat(key, value);
    else if (key=="RxGhostOnlyNew")          m_tracker.m_rxGhostOnlyNew = toBool(value);
    else if (key=="StylusSuppressGlobalEnabled") {
        m_tracker.m_stylusSuppressGlobalEnabled = toBool(value);
        m_stylusSuppress.m_stylusSuppressGlobalEnabled = m_tracker.m_stylusSuppressGlobalEnabled;
    }
    else if (key=="StylusSuppressLocalEnabled") {
        m_tracker.m_stylusSuppressLocalEnabled = toBool(value);
        m_stylusSuppress.m_stylusSuppressLocalEnabled = m_tracker.m_stylusSuppressLocalEnabled;
    }
    else if (key=="StylusSuppressLocalDistance") {
        m_tracker.m_stylusSuppressLocalDistance = ParseConfigFloat(key, value);
        m_stylusSuppress.m_stylusSuppressLocalDistance = m_tracker.m_stylusSuppressLocalDistance;
    }
    else if (key=="StylusSuppressPenPeakThreshold") {
        m_tracker.m_stylusSuppressPenPeakThreshold = ParseConfigInt(key, value);
        m_stylusSuppress.m_stylusSuppressPenPeakThreshold = m_tracker.m_stylusSuppressPenPeakThreshold;
    }
    else if (key=="StylusSuppressTouchSignalKeep") {
        m_tracker.m_stylusSuppressTouchSignalKeep = ParseConfigInt(key, value);
        m_stylusSuppress.m_stylusSuppressTouchSignalKeep = m_tracker.m_stylusSuppressTouchSignalKeep;
    }
    else if (key=="StylusSuppressTouchAreaKeep") {
        m_tracker.m_stylusSuppressTouchAreaKeep = ParseConfigInt(key, value);
        m_stylusSuppress.m_stylusSuppressTouchAreaKeep = m_tracker.m_stylusSuppressTouchAreaKeep;
    }
    else if (key=="StylusAftEnabled") {
        m_tracker.m_stylusAftEnabled = toBool(value);
        m_stylusSuppress.m_stylusAftEnabled = m_tracker.m_stylusAftEnabled;
    }
    else if (key=="StylusAftRecentFrames")   m_tracker.m_stylusAftRecentFrames = ParseConfigInt(key, value);
    else if (key=="StylusAftRadius")         m_tracker.m_stylusAftRadius = ParseConfigFloat(key, value);
    else if (key=="StylusAftDebounceFrames") {
        m_tracker.m_stylusAftDebounceFrames = ParseConfigInt(key, value);
        m_stylusSuppress.m_stylusAftDebounceFrames = m_tracker.m_stylusAftDebounceFrames;
    }
    else if (key=="StylusAftWeakSignalThreshold") {
        m_tracker.m_stylusAftWeakSignalThreshold = ParseConfigInt(key, value);
        m_stylusSuppress.m_stylusAftWeakSignalThreshold = m_tracker.m_stylusAftWeakSignalThreshold;
    }
    else if (key=="StylusAftWeakSizeThresholdMm") {
        m_tracker.m_stylusAftWeakSizeThresholdMm = ParseConfigFloat(key, value);
        m_stylusSuppress.m_stylusAftWeakSizeThresholdMm = m_tracker.m_stylusAftWeakSizeThresholdMm;
    }
    else if (key=="StylusAftSuppressFrames") {
        m_tracker.m_stylusAftSuppressFrames = ParseConfigInt(key, value);
        m_stylusSuppress.m_stylusAftSuppressFrames = m_tracker.m_stylusAftSuppressFrames;
    }
    else if (key=="StylusAftPalmSuppressFrames") m_tracker.m_stylusAftPalmSuppressFrames = ParseConfigInt(key, value);
    else if (key=="StylusAftPalmAreaThreshold") m_tracker.m_stylusAftPalmAreaThreshold = ParseConfigInt(key, value);
    else if (key=="StylusAftPalmSizeThresholdMm") m_tracker.m_stylusAftPalmSizeThresholdMm = ParseConfigFloat(key, value);
    // Phase 5: CoordinateFilter
    else if (key=="CoordFilterEnabled")      m_coordFilter.m_enabled = toBool(value);
    else if (key=="OneEuroMinCutoff")        m_coordFilter.m_minCutoff = ParseConfigFloat(key, value);
    else if (key=="OneEuroBeta")             m_coordFilter.m_beta = ParseConfigFloat(key, value);
    else if (key=="OneEuroDCutoff")          m_coordFilter.m_dCutoff = ParseConfigFloat(key, value);
    // Phase 6: GestureStateMachine
    else if (key=="GestureEnabled")          m_gesture.m_enabled = toBool(value);
    else if (key=="PressCandidateFrames")    m_gesture.m_pressCandidateFrames = ParseConfigInt(key, value);
    else if (key=="PressCandidateMinSignal") m_gesture.m_pressCandidateMinSignal = ParseConfigInt(key, value);
    else if (key=="PressCandidateMinSizeMm") m_gesture.m_pressCandidateMinSizeMm = ParseConfigFloat(key, value);
    else if (key=="DragThreshold")           m_gesture.m_dragThreshold = ParseConfigFloat(key, value);
    else if (key=="LongPressFrames")         m_gesture.m_longPressFrames = ParseConfigInt(key, value);
    else if (key=="LongPressMoveTolerance")  m_gesture.m_longPressMoveTolerance = ParseConfigFloat(key, value);
    else if (key=="ReleasePendingFrames")    m_gesture.m_releasePendingFrames = ParseConfigInt(key, value);
    else if (key=="BypassStateMachine")      m_gesture.m_bypassStateMachine = toBool(value);

    SyncStylusSuppressConfigFromTracker();
    } catch (const ConfigParseError& error) {
        LogConfigParseWarning("TouchPipeline", __func__, key, value, error);
    }
}

} // namespace Solvers
