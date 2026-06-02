#include "TouchPipeline.h"
#include "ConfigParse.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

namespace {

enum class TouchConfigKey : uint16_t {
    AccBoostSizeMm,
    AccThresholdBoost,
    AlwaysMatchDistance,
    BaselineBackgroundAlphaShift,
    BaselineBackgroundMaxStep,
    BaselineEnabled,
    BaselineNegativeAlphaShift,
    BaselineNegativeDeadband,
    BaselineNegativeMaxStep,
    BaselineNoFingerAlphaShift,
    BaselineNoFingerMaxStep,
    BaselineNoiseAlphaShift,
    BaselineNoiseDeadband,
    BaselineNoiseTrackingEnabled,
    BaselinePeakThreshold,
    BaselinePositiveAlphaShift,
    BaselinePositiveDeadband,
    BaselinePositiveMaxStep,
    BaselineRecoveryAlphaShift,
    BaselineRecoveryMaxFrames,
    BaselineRecoveryMaxStep,
    BaselineReleaseHoldFrames,
    BaselineValue,
    BypassStateMachine,
    CMFDimensionMode,
    CMFEnabled,
    CMFExclusionThreshold,
    CMFMaxCorrection,
    CoordFilterEnabled,
    DilateErode,
    DragThreshold,
    DynamicDebounceEnabled,
    EdgePeakFilter,
    EdgeThreshold,
    EdgeThresholdEnabled,
    EdgeTrackBoost,
    FallbackSizeMm,
    FrameParserEnabled,
    GapRelinkEnabled,
    GapRelinkWindowFrames,
    GestureEnabled,
    LongPressFrames,
    LongPressMoveTolerance,
    MacroZoneMinArea,
    MaxPeaks,
    MaxTouches,
    MaxTrackDistance,
    OneEuroBeta,
    OneEuroDCutoff,
    OneEuroMinCutoff,
    PalmAnalyzerEnabled,
    PalmAreaMinForDensity,
    PalmAreaThreshold,
    PalmAwareExpansionEnabled,
    PalmCandidateAreaThreshold,
    PalmCandidateSignalThreshold,
    PalmDensityThresholdLow,
    PalmElongatedAspectRatio,
    PalmElongatedEnabled,
    PalmElongatedMinArea,
    PalmEnabled,
    PalmFillRatioThreshold,
    PalmFingerInPalmMaxRadius,
    PalmFingerInPalmThresholdRatio,
    PalmFlatSharpnessThreshold,
    PalmLikelyAllowContact,
    PalmLikelyAreaThreshold,
    PalmShadowEnabled,
    PalmShadowHoldFrames,
    PalmShadowRadius,
    PalmShadowSeedScore,
    PalmSignalSumThreshold,
    PalmStrongPeakProminence,
    PeakEvalAmbiguousMargin,
    PeakEvalEnabled,
    PeakEvalFingerProminence,
    PeakEvalFingerSharpness,
    PeakEvalPalmSharpnessMax,
    PeakThreshold,
    PredictionScale,
    PressCandidateFrames,
    PressCandidateMinSignal,
    PressCandidateMinSizeMm,
    PressureDriftDebounce,
    PressureDriftFilter,
    ReleasePendingFrames,
    RxGhostFilterEnabled,
    RxGhostLineDelta,
    RxGhostOnlyNew,
    RxGhostWeakRatio,
    SigTholdLimit,
    SizeAreaScale,
    SizeSignalScale,
    StylusAftDebounceFrames,
    StylusAftEnabled,
    StylusAftPalmAreaThreshold,
    StylusAftPalmSizeThresholdMm,
    StylusAftPalmSuppressFrames,
    StylusAftRadius,
    StylusAftRecentFrames,
    StylusAftSuppressFrames,
    StylusAftWeakSignalThreshold,
    StylusAftWeakSizeThresholdMm,
    StylusSuppressGlobalEnabled,
    StylusSuppressLocalDistance,
    StylusSuppressLocalEnabled,
    StylusSuppressPenPeakThreshold,
    StylusSuppressTouchAreaKeep,
    StylusSuppressTouchSignalKeep,
    TouchDownDebounceFrames,
    TouchDownDebounceMaxExtra,
    TouchDownEdgeRejectMinSignal,
    TouchDownRejectEnabled,
    TouchDownRejectMinSignal,
    TouchDownRejectMinSizeMm,
    TouchDownSmallSizeThresholdMm,
    TouchDownWeakSignalThreshold,
    TrackerEnabled,
    UseHungarian,
    Z1FilterEnabled,
    Z8FilterEnabled,
    Z8Radius,
    ZoneTholdScale,
    ZoneTholdShift,
    Unknown,
};

bool IsFrozenCurrentTouchConfigKey(std::string_view key) {
    constexpr std::array<std::string_view, 117> kFrozenKeys = {{
        "AccBoostSizeMm",
        "AccThresholdBoost",
        "AlwaysMatchDistance",
        "BaselineEnabled",
        "BaselineNegativeAlphaShift",
        "BaselineNegativeDeadband",
        "BaselineNegativeMaxStep",
        "BaselineNoiseAlphaShift",
        "BaselineNoiseDeadband",
        "BaselineNoiseTrackingEnabled",
        "BaselinePeakThreshold",
        "BaselinePositiveAlphaShift",
        "BaselinePositiveDeadband",
        "BaselinePositiveMaxStep",
        "BaselineReleaseHoldFrames",
        "BaselineValue",
        "BypassStateMachine",
        "CMFDimensionMode",
        "CMFEnabled",
        "CMFExclusionThreshold",
        "CMFMaxCorrection",
        "CoordFilterEnabled",
        "DilateErode",
        "DragThreshold",
        "DynamicDebounceEnabled",
        "EdgePeakFilter",
        "EdgeThreshold",
        "EdgeThresholdEnabled",
        "EdgeTrackBoost",
        "FallbackSizeMm",
        "FrameParserEnabled",
        "GapRelinkEnabled",
        "GapRelinkWindowFrames",
        "GestureEnabled",
        "LongPressFrames",
        "LongPressMoveTolerance",
        "MacroZoneMinArea",
        "MaxPeaks",
        "MaxTouches",
        "MaxTrackDistance",
        "OneEuroBeta",
        "OneEuroDCutoff",
        "OneEuroMinCutoff",
        "PalmAnalyzerEnabled",
        "PalmAreaMinForDensity",
        "PalmAreaThreshold",
        "PalmAwareExpansionEnabled",
        "PalmCandidateAreaThreshold",
        "PalmCandidateSignalThreshold",
        "PalmDensityThresholdLow",
        "PalmElongatedAspectRatio",
        "PalmElongatedEnabled",
        "PalmElongatedMinArea",
        "PalmEnabled",
        "PalmFillRatioThreshold",
        "PalmFingerInPalmMaxRadius",
        "PalmFingerInPalmThresholdRatio",
        "PalmFlatSharpnessThreshold",
        "PalmLikelyAllowContact",
        "PalmLikelyAreaThreshold",
        "PalmShadowEnabled",
        "PalmShadowHoldFrames",
        "PalmShadowRadius",
        "PalmShadowSeedScore",
        "PalmSignalSumThreshold",
        "PalmStrongPeakProminence",
        "PeakEvalAmbiguousMargin",
        "PeakEvalEnabled",
        "PeakEvalFingerProminence",
        "PeakEvalFingerSharpness",
        "PeakEvalPalmSharpnessMax",
        "PeakThreshold",
        "PredictionScale",
        "PressCandidateFrames",
        "PressCandidateMinSignal",
        "PressCandidateMinSizeMm",
        "PressureDriftDebounce",
        "PressureDriftFilter",
        "ReleasePendingFrames",
        "RxGhostFilterEnabled",
        "RxGhostLineDelta",
        "RxGhostOnlyNew",
        "RxGhostWeakRatio",
        "SigTholdLimit",
        "SizeAreaScale",
        "SizeSignalScale",
        "StylusAftDebounceFrames",
        "StylusAftEnabled",
        "StylusAftPalmAreaThreshold",
        "StylusAftPalmSizeThresholdMm",
        "StylusAftPalmSuppressFrames",
        "StylusAftRadius",
        "StylusAftRecentFrames",
        "StylusAftSuppressFrames",
        "StylusAftWeakSignalThreshold",
        "StylusAftWeakSizeThresholdMm",
        "StylusSuppressGlobalEnabled",
        "StylusSuppressLocalDistance",
        "StylusSuppressLocalEnabled",
        "StylusSuppressPenPeakThreshold",
        "StylusSuppressTouchAreaKeep",
        "StylusSuppressTouchSignalKeep",
        "TouchDownDebounceFrames",
        "TouchDownDebounceMaxExtra",
        "TouchDownEdgeRejectMinSignal",
        "TouchDownRejectEnabled",
        "TouchDownRejectMinSignal",
        "TouchDownRejectMinSizeMm",
        "TouchDownSmallSizeThresholdMm",
        "TouchDownWeakSignalThreshold",
        "TrackerEnabled",
        "UseHungarian",
        "Z1FilterEnabled",
        "Z8FilterEnabled",
        "Z8Radius",
        "ZoneTholdScale",
        "ZoneTholdShift",
    }};
    static_assert(std::is_sorted(kFrozenKeys.begin(), kFrozenKeys.end()), "Frozen keys must be sorted");

    return std::binary_search(kFrozenKeys.begin(), kFrozenKeys.end(), key);
}

TouchConfigKey FindTouchConfigKey(std::string_view key) {
    constexpr std::array<std::pair<std::string_view, TouchConfigKey>, 124> kTable = {{
        {"AccBoostSizeMm", TouchConfigKey::AccBoostSizeMm},
        {"AccThresholdBoost", TouchConfigKey::AccThresholdBoost},
        {"AlwaysMatchDistance", TouchConfigKey::AlwaysMatchDistance},
        {"BaselineBackgroundAlphaShift", TouchConfigKey::BaselineBackgroundAlphaShift},
        {"BaselineBackgroundMaxStep", TouchConfigKey::BaselineBackgroundMaxStep},
        {"BaselineEnabled", TouchConfigKey::BaselineEnabled},
        {"BaselineNegativeAlphaShift", TouchConfigKey::BaselineNegativeAlphaShift},
        {"BaselineNegativeDeadband", TouchConfigKey::BaselineNegativeDeadband},
        {"BaselineNegativeMaxStep", TouchConfigKey::BaselineNegativeMaxStep},
        {"BaselineNoFingerAlphaShift", TouchConfigKey::BaselineNoFingerAlphaShift},
        {"BaselineNoFingerMaxStep", TouchConfigKey::BaselineNoFingerMaxStep},
        {"BaselineNoiseAlphaShift", TouchConfigKey::BaselineNoiseAlphaShift},
        {"BaselineNoiseDeadband", TouchConfigKey::BaselineNoiseDeadband},
        {"BaselineNoiseTrackingEnabled", TouchConfigKey::BaselineNoiseTrackingEnabled},
        {"BaselinePeakThreshold", TouchConfigKey::BaselinePeakThreshold},
        {"BaselinePositiveAlphaShift", TouchConfigKey::BaselinePositiveAlphaShift},
        {"BaselinePositiveDeadband", TouchConfigKey::BaselinePositiveDeadband},
        {"BaselinePositiveMaxStep", TouchConfigKey::BaselinePositiveMaxStep},
        {"BaselineRecoveryAlphaShift", TouchConfigKey::BaselineRecoveryAlphaShift},
        {"BaselineRecoveryMaxFrames", TouchConfigKey::BaselineRecoveryMaxFrames},
        {"BaselineRecoveryMaxStep", TouchConfigKey::BaselineRecoveryMaxStep},
        {"BaselineReleaseHoldFrames", TouchConfigKey::BaselineReleaseHoldFrames},
        {"BaselineValue", TouchConfigKey::BaselineValue},
        {"BypassStateMachine", TouchConfigKey::BypassStateMachine},
        {"CMFDimensionMode", TouchConfigKey::CMFDimensionMode},
        {"CMFEnabled", TouchConfigKey::CMFEnabled},
        {"CMFExclusionThreshold", TouchConfigKey::CMFExclusionThreshold},
        {"CMFMaxCorrection", TouchConfigKey::CMFMaxCorrection},
        {"CoordFilterEnabled", TouchConfigKey::CoordFilterEnabled},
        {"DilateErode", TouchConfigKey::DilateErode},
        {"DragThreshold", TouchConfigKey::DragThreshold},
        {"DynamicDebounceEnabled", TouchConfigKey::DynamicDebounceEnabled},
        {"EdgePeakFilter", TouchConfigKey::EdgePeakFilter},
        {"EdgeThreshold", TouchConfigKey::EdgeThreshold},
        {"EdgeThresholdEnabled", TouchConfigKey::EdgeThresholdEnabled},
        {"EdgeTrackBoost", TouchConfigKey::EdgeTrackBoost},
        {"FallbackSizeMm", TouchConfigKey::FallbackSizeMm},
        {"FrameParserEnabled", TouchConfigKey::FrameParserEnabled},
        {"GapRelinkEnabled", TouchConfigKey::GapRelinkEnabled},
        {"GapRelinkWindowFrames", TouchConfigKey::GapRelinkWindowFrames},
        {"GestureEnabled", TouchConfigKey::GestureEnabled},
        {"LongPressFrames", TouchConfigKey::LongPressFrames},
        {"LongPressMoveTolerance", TouchConfigKey::LongPressMoveTolerance},
        {"MacroZoneMinArea", TouchConfigKey::MacroZoneMinArea},
        {"MaxPeaks", TouchConfigKey::MaxPeaks},
        {"MaxTouches", TouchConfigKey::MaxTouches},
        {"MaxTrackDistance", TouchConfigKey::MaxTrackDistance},
        {"OneEuroBeta", TouchConfigKey::OneEuroBeta},
        {"OneEuroDCutoff", TouchConfigKey::OneEuroDCutoff},
        {"OneEuroMinCutoff", TouchConfigKey::OneEuroMinCutoff},
        {"PalmAnalyzerEnabled", TouchConfigKey::PalmAnalyzerEnabled},
        {"PalmAreaMinForDensity", TouchConfigKey::PalmAreaMinForDensity},
        {"PalmAreaThreshold", TouchConfigKey::PalmAreaThreshold},
        {"PalmAwareExpansionEnabled", TouchConfigKey::PalmAwareExpansionEnabled},
        {"PalmCandidateAreaThreshold", TouchConfigKey::PalmCandidateAreaThreshold},
        {"PalmCandidateSignalThreshold", TouchConfigKey::PalmCandidateSignalThreshold},
        {"PalmDensityThresholdLow", TouchConfigKey::PalmDensityThresholdLow},
        {"PalmElongatedAspectRatio", TouchConfigKey::PalmElongatedAspectRatio},
        {"PalmElongatedEnabled", TouchConfigKey::PalmElongatedEnabled},
        {"PalmElongatedMinArea", TouchConfigKey::PalmElongatedMinArea},
        {"PalmEnabled", TouchConfigKey::PalmEnabled},
        {"PalmFillRatioThreshold", TouchConfigKey::PalmFillRatioThreshold},
        {"PalmFingerInPalmMaxRadius", TouchConfigKey::PalmFingerInPalmMaxRadius},
        {"PalmFingerInPalmThresholdRatio", TouchConfigKey::PalmFingerInPalmThresholdRatio},
        {"PalmFlatSharpnessThreshold", TouchConfigKey::PalmFlatSharpnessThreshold},
        {"PalmLikelyAllowContact", TouchConfigKey::PalmLikelyAllowContact},
        {"PalmLikelyAreaThreshold", TouchConfigKey::PalmLikelyAreaThreshold},
        {"PalmShadowEnabled", TouchConfigKey::PalmShadowEnabled},
        {"PalmShadowHoldFrames", TouchConfigKey::PalmShadowHoldFrames},
        {"PalmShadowRadius", TouchConfigKey::PalmShadowRadius},
        {"PalmShadowSeedScore", TouchConfigKey::PalmShadowSeedScore},
        {"PalmSignalSumThreshold", TouchConfigKey::PalmSignalSumThreshold},
        {"PalmStrongPeakProminence", TouchConfigKey::PalmStrongPeakProminence},
        {"PeakEvalAmbiguousMargin", TouchConfigKey::PeakEvalAmbiguousMargin},
        {"PeakEvalEnabled", TouchConfigKey::PeakEvalEnabled},
        {"PeakEvalFingerProminence", TouchConfigKey::PeakEvalFingerProminence},
        {"PeakEvalFingerSharpness", TouchConfigKey::PeakEvalFingerSharpness},
        {"PeakEvalPalmSharpnessMax", TouchConfigKey::PeakEvalPalmSharpnessMax},
        {"PeakThreshold", TouchConfigKey::PeakThreshold},
        {"PredictionScale", TouchConfigKey::PredictionScale},
        {"PressCandidateFrames", TouchConfigKey::PressCandidateFrames},
        {"PressCandidateMinSignal", TouchConfigKey::PressCandidateMinSignal},
        {"PressCandidateMinSizeMm", TouchConfigKey::PressCandidateMinSizeMm},
        {"PressureDriftDebounce", TouchConfigKey::PressureDriftDebounce},
        {"PressureDriftFilter", TouchConfigKey::PressureDriftFilter},
        {"ReleasePendingFrames", TouchConfigKey::ReleasePendingFrames},
        {"RxGhostFilterEnabled", TouchConfigKey::RxGhostFilterEnabled},
        {"RxGhostLineDelta", TouchConfigKey::RxGhostLineDelta},
        {"RxGhostOnlyNew", TouchConfigKey::RxGhostOnlyNew},
        {"RxGhostWeakRatio", TouchConfigKey::RxGhostWeakRatio},
        {"SigTholdLimit", TouchConfigKey::SigTholdLimit},
        {"SizeAreaScale", TouchConfigKey::SizeAreaScale},
        {"SizeSignalScale", TouchConfigKey::SizeSignalScale},
        {"StylusAftDebounceFrames", TouchConfigKey::StylusAftDebounceFrames},
        {"StylusAftEnabled", TouchConfigKey::StylusAftEnabled},
        {"StylusAftPalmAreaThreshold", TouchConfigKey::StylusAftPalmAreaThreshold},
        {"StylusAftPalmSizeThresholdMm", TouchConfigKey::StylusAftPalmSizeThresholdMm},
        {"StylusAftPalmSuppressFrames", TouchConfigKey::StylusAftPalmSuppressFrames},
        {"StylusAftRadius", TouchConfigKey::StylusAftRadius},
        {"StylusAftRecentFrames", TouchConfigKey::StylusAftRecentFrames},
        {"StylusAftSuppressFrames", TouchConfigKey::StylusAftSuppressFrames},
        {"StylusAftWeakSignalThreshold", TouchConfigKey::StylusAftWeakSignalThreshold},
        {"StylusAftWeakSizeThresholdMm", TouchConfigKey::StylusAftWeakSizeThresholdMm},
        {"StylusSuppressGlobalEnabled", TouchConfigKey::StylusSuppressGlobalEnabled},
        {"StylusSuppressLocalDistance", TouchConfigKey::StylusSuppressLocalDistance},
        {"StylusSuppressLocalEnabled", TouchConfigKey::StylusSuppressLocalEnabled},
        {"StylusSuppressPenPeakThreshold", TouchConfigKey::StylusSuppressPenPeakThreshold},
        {"StylusSuppressTouchAreaKeep", TouchConfigKey::StylusSuppressTouchAreaKeep},
        {"StylusSuppressTouchSignalKeep", TouchConfigKey::StylusSuppressTouchSignalKeep},
        {"TouchDownDebounceFrames", TouchConfigKey::TouchDownDebounceFrames},
        {"TouchDownDebounceMaxExtra", TouchConfigKey::TouchDownDebounceMaxExtra},
        {"TouchDownEdgeRejectMinSignal", TouchConfigKey::TouchDownEdgeRejectMinSignal},
        {"TouchDownRejectEnabled", TouchConfigKey::TouchDownRejectEnabled},
        {"TouchDownRejectMinSignal", TouchConfigKey::TouchDownRejectMinSignal},
        {"TouchDownRejectMinSizeMm", TouchConfigKey::TouchDownRejectMinSizeMm},
        {"TouchDownSmallSizeThresholdMm", TouchConfigKey::TouchDownSmallSizeThresholdMm},
        {"TouchDownWeakSignalThreshold", TouchConfigKey::TouchDownWeakSignalThreshold},
        {"TrackerEnabled", TouchConfigKey::TrackerEnabled},
        {"UseHungarian", TouchConfigKey::UseHungarian},
        {"Z1FilterEnabled", TouchConfigKey::Z1FilterEnabled},
        {"Z8FilterEnabled", TouchConfigKey::Z8FilterEnabled},
        {"Z8Radius", TouchConfigKey::Z8Radius},
        {"ZoneTholdScale", TouchConfigKey::ZoneTholdScale},
        {"ZoneTholdShift", TouchConfigKey::ZoneTholdShift},
    }};
    static_assert(std::is_sorted(kTable.begin(), kTable.end(),
                                 [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; }),
                  "Touch config keys must be sorted");

    const auto it = std::lower_bound(kTable.begin(), kTable.end(), key,
                                     [](const auto& entry, std::string_view value) {
                                         return entry.first < value;
                                     });
    if (it != kTable.end() && it->first == key) return it->second;
    return TouchConfigKey::Unknown;
}

template <typename TValue>
void WriteConfigLine(std::ostream& out, std::string_view key, const TValue& value) {
    if (IsFrozenCurrentTouchConfigKey(key)) return;
    out << key << '=' << value << '\n';
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

// ══════════════════════════════════════════════════════════════════════
// GetConfigSchema — unified from all sub-modules
// ══════════════════════════════════════════════════════════════════════
std::vector<ConfigParam> TouchPipeline::GetConfigSchema() const {
    std::vector<ConfigParam> s;
    s.reserve(120);

    // ── Frame Parser ──
    if (!IsFrozenCurrentTouchConfigKey("FrameParserEnabled")) s.emplace_back("FrameParserEnabled", "Frame Parser Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_frameParser.m_enabled)).Module("Frame Parser");

    // ── Signal Conditioning: Baseline ──
    if (!IsFrozenCurrentTouchConfigKey("BaselineEnabled")) s.emplace_back("BaselineEnabled", "Baseline Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_baseline.m_enabled)).Module("Signal Conditioning");
    if (!IsFrozenCurrentTouchConfigKey("BaselineValue")) s.emplace_back("BaselineValue", "Baseline Value",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_baseline), 0, 65535).Module("Signal Conditioning");
    if (!IsFrozenCurrentTouchConfigKey("BaselineNoiseDeadband")) s.emplace_back("BaselineNoiseDeadband", "Baseline Noise Deadband",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_noiseDeadband), 0, 100).Module("Signal Conditioning");
    if (!IsFrozenCurrentTouchConfigKey("BaselinePositiveDeadband")) s.emplace_back("BaselinePositiveDeadband", "Baseline Positive Deadband",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_positiveDeadband), 0, 200).Module("Signal Conditioning");
    if (!IsFrozenCurrentTouchConfigKey("BaselineNegativeDeadband")) s.emplace_back("BaselineNegativeDeadband", "Baseline Negative Deadband",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_negativeDeadband), 0, 200).Module("Signal Conditioning");
    if (!IsFrozenCurrentTouchConfigKey("BaselinePeakThreshold")) s.emplace_back("BaselinePeakThreshold", "Baseline Peak Threshold",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_peakThreshold), 1, 2000).Module("Signal Conditioning");
    if (!IsFrozenCurrentTouchConfigKey("BaselineReleaseHoldFrames")) s.emplace_back("BaselineReleaseHoldFrames", "Baseline Release Hold Frames",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_releaseHoldFrames), 0, 255).Module("Signal Conditioning");
    if (!IsFrozenCurrentTouchConfigKey("BaselinePositiveAlphaShift")) s.emplace_back("BaselinePositiveAlphaShift", "Baseline Positive Alpha Shift",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_positiveAlphaShift), 0, 15).Module("Signal Conditioning");
    if (!IsFrozenCurrentTouchConfigKey("BaselineNegativeAlphaShift")) s.emplace_back("BaselineNegativeAlphaShift", "Baseline Negative Alpha Shift",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_negativeAlphaShift), 0, 15).Module("Signal Conditioning");
    if (!IsFrozenCurrentTouchConfigKey("BaselineNoiseAlphaShift")) s.emplace_back("BaselineNoiseAlphaShift", "Baseline Noise Alpha Shift",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_noiseAlphaShift), 0, 15).Module("Signal Conditioning");
    if (!IsFrozenCurrentTouchConfigKey("BaselineBackgroundAlphaShift")) s.emplace_back("BaselineBackgroundAlphaShift", "Baseline Background Alpha Shift",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_backgroundAlphaShift), 0, 15).Module("Signal Conditioning");
    if (!IsFrozenCurrentTouchConfigKey("BaselineNoFingerAlphaShift")) s.emplace_back("BaselineNoFingerAlphaShift", "Baseline No-Finger Alpha Shift",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_noFingerAlphaShift), 0, 15).Module("Signal Conditioning");
    if (!IsFrozenCurrentTouchConfigKey("BaselinePositiveMaxStep")) s.emplace_back("BaselinePositiveMaxStep", "Baseline Positive Max Step",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_positiveMaxStep), 0, 200).Module("Signal Conditioning");
    if (!IsFrozenCurrentTouchConfigKey("BaselineNegativeMaxStep")) s.emplace_back("BaselineNegativeMaxStep", "Baseline Negative Max Step",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_negativeMaxStep), 0, 200).Module("Signal Conditioning");
    if (!IsFrozenCurrentTouchConfigKey("BaselineBackgroundMaxStep")) s.emplace_back("BaselineBackgroundMaxStep", "Baseline Background Max Step",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_backgroundMaxStep), 1, 2048).Module("Signal Conditioning");
    if (!IsFrozenCurrentTouchConfigKey("BaselineNoFingerMaxStep")) s.emplace_back("BaselineNoFingerMaxStep", "Baseline No-Finger Max Step",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_noFingerMaxStep), 1, 2048).Module("Signal Conditioning");
    if (!IsFrozenCurrentTouchConfigKey("BaselineRecoveryAlphaShift")) s.emplace_back("BaselineRecoveryAlphaShift", "Baseline Recovery Alpha Shift",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_recoveryAlphaShift), 0, 15).Module("Signal Conditioning");
    if (!IsFrozenCurrentTouchConfigKey("BaselineRecoveryMaxStep")) s.emplace_back("BaselineRecoveryMaxStep", "Baseline Recovery Max Step",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_recoveryMaxStep), 1, 2048).Module("Signal Conditioning");
    if (!IsFrozenCurrentTouchConfigKey("BaselineRecoveryMaxFrames")) s.emplace_back("BaselineRecoveryMaxFrames", "Baseline Recovery Max Frames",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_recoveryMaxFrames), 1, 120).Module("Signal Conditioning");
    if (!IsFrozenCurrentTouchConfigKey("BaselineNoiseTrackingEnabled")) s.emplace_back("BaselineNoiseTrackingEnabled", "Baseline Noise Tracking Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_baseline.m_noiseTrackingEnabled)).Module("Signal Conditioning");

    // ── Signal Conditioning: CMF ──
    if (!IsFrozenCurrentTouchConfigKey("CMFEnabled")) s.emplace_back("CMFEnabled", "CMF Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_cmf.m_enabled)).Module("Signal Conditioning");
    if (!IsFrozenCurrentTouchConfigKey("CMFExclusionThreshold")) s.emplace_back("CMFExclusionThreshold", "CMF Exclusion Threshold",
                   ConfigParam::Int, const_cast<int*>(&m_cmf.m_exclusionThreshold), 50, 2000).Module("Signal Conditioning");
    if (!IsFrozenCurrentTouchConfigKey("CMFMaxCorrection")) s.emplace_back("CMFMaxCorrection", "CMF Max Correction",
                   ConfigParam::Int, const_cast<int*>(&m_cmf.m_maxCorrection), 10, 2000).Module("Signal Conditioning");

    // ── Peak Detection ──
    if (!IsFrozenCurrentTouchConfigKey("PeakThreshold")) s.emplace_back("PeakThreshold", "Peak Threshold",
                   ConfigParam::Int, const_cast<int*>(&m_peakDet.m_threshold), 1, 1000).Module("Peak Detection");
    if (!IsFrozenCurrentTouchConfigKey("SigTholdLimit")) s.emplace_back("SigTholdLimit", "Sig Thold Limit",
                   ConfigParam::Int, const_cast<int*>(&m_peakDet.m_sigTholdLimit), 1, 1000).Module("Peak Detection");
    if (!IsFrozenCurrentTouchConfigKey("Z8FilterEnabled")) s.emplace_back("Z8FilterEnabled", "Z8 Filter Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_peakDet.m_z8Filter)).Module("Peak Detection");
    if (!IsFrozenCurrentTouchConfigKey("Z1FilterEnabled")) s.emplace_back("Z1FilterEnabled", "Z1 Filter Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_peakDet.m_z1Filter)).Module("Peak Detection");
    if (!IsFrozenCurrentTouchConfigKey("PressureDriftFilter")) s.emplace_back("PressureDriftFilter", "Pressure Drift Filter",
                   ConfigParam::Bool, const_cast<bool*>(&m_peakDet.m_pressureDriftFilter)).Module("Peak Detection");
    if (!IsFrozenCurrentTouchConfigKey("EdgePeakFilter")) s.emplace_back("EdgePeakFilter", "Edge Peak Filter",
                   ConfigParam::Bool, const_cast<bool*>(&m_peakDet.m_edgePeakFilter)).Module("Peak Detection");
    if (!IsFrozenCurrentTouchConfigKey("EdgeThresholdEnabled")) s.emplace_back("EdgeThresholdEnabled", "Edge Threshold Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_peakDet.m_edgeThresholdEnabled)).Module("Peak Detection");
    if (!IsFrozenCurrentTouchConfigKey("EdgeThreshold")) s.emplace_back("EdgeThreshold", "Edge Threshold",
                   ConfigParam::Int, const_cast<int*>(&m_peakDet.m_edgeThreshold), 1, 1000).Module("Peak Detection");
    if (!IsFrozenCurrentTouchConfigKey("Z8Radius")) s.emplace_back("Z8Radius", "Z8 Max Search Radius",
                   ConfigParam::Int, const_cast<int*>(&m_peakDet.m_z8Radius), 1, 5).Module("Peak Detection");
    if (!IsFrozenCurrentTouchConfigKey("MaxPeaks")) s.emplace_back("MaxPeaks", "Peak Limit Cap",
                   ConfigParam::Int, const_cast<int*>(&m_peakDet.m_maxPeaks), 5, 100).Module("Peak Detection");
    if (!IsFrozenCurrentTouchConfigKey("PressureDriftDebounce")) s.emplace_back("PressureDriftDebounce", "Pressure Debounce Limit",
                   ConfigParam::Int, const_cast<int*>(&m_peakDet.m_pressureDriftDebounceLimit), 0, 10).Module("Peak Detection");
    if (!IsFrozenCurrentTouchConfigKey("MacroZoneMinArea")) s.emplace_back("MacroZoneMinArea", "MacroZone Min Area",
                   ConfigParam::Int, const_cast<int*>(&m_peakDet.m_macroZoneMinArea), 1, 20).Module("Peak Detection");

    // ── Zone & Contact ──
    if (!IsFrozenCurrentTouchConfigKey("DilateErode")) s.emplace_back("DilateErode", "Dilate Erode Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_contactExtractor.m_zoneExp.m_dilateErode)).Module("Zone & Contact");
    if (!IsFrozenCurrentTouchConfigKey("ZoneTholdScale")) s.emplace_back("ZoneTholdScale", "Zone Thold Numer",
                   ConfigParam::Int, const_cast<int*>(&m_contactExtractor.m_zoneExp.m_tholdScaleNumer), 0, 255).Module("Zone & Contact");
    if (!IsFrozenCurrentTouchConfigKey("ZoneTholdShift")) s.emplace_back("ZoneTholdShift", "Zone Thold Shift",
                   ConfigParam::Int, const_cast<int*>(&m_contactExtractor.m_zoneExp.m_tholdScaleShift), 0, 15).Module("Zone & Contact");
    if (!IsFrozenCurrentTouchConfigKey("MaxTouches")) s.emplace_back("MaxTouches", "Max Contact Outputs",
                   ConfigParam::Int, const_cast<int*>(&m_contactExtractor.m_zoneExp.m_maxTouches), 1, 50).Module("Zone & Contact");
    // ── Touch Classification ──
    if (!IsFrozenCurrentTouchConfigKey("PalmEnabled")) s.emplace_back("PalmEnabled", "Touch Classification Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_touchClassifier.m_enabled)).Module("Touch Classification");
    if (!IsFrozenCurrentTouchConfigKey("PalmAreaThreshold")) s.emplace_back("PalmAreaThreshold", "Palm Area Threshold",
                   ConfigParam::Int, const_cast<int*>(&m_touchClassifier.m_areaThreshold), 5, 300).Module("Touch Classification");
    if (!IsFrozenCurrentTouchConfigKey("PalmSignalSumThreshold")) s.emplace_back("PalmSignalSumThreshold", "Palm SignalSum Threshold",
                   ConfigParam::Int, const_cast<int*>(&m_touchClassifier.m_signalSumThreshold), 1000, 500000).Module("Touch Classification");
    if (!IsFrozenCurrentTouchConfigKey("PalmDensityThresholdLow")) s.emplace_back("PalmDensityThresholdLow", "Palm Density Low Threshold",
                   ConfigParam::Float, const_cast<float*>(&m_touchClassifier.m_densityThresholdLow), 50.0f, 2000.0f).Module("Touch Classification");
    if (!IsFrozenCurrentTouchConfigKey("PalmAreaMinForDensity")) s.emplace_back("PalmAreaMinForDensity", "Palm Density Min Area",
                   ConfigParam::Int, const_cast<int*>(&m_touchClassifier.m_areaMinForDensity), 3, 100).Module("Touch Classification");
    if (!IsFrozenCurrentTouchConfigKey("PalmElongatedEnabled")) s.emplace_back("PalmElongatedEnabled", "Elongated Press Reject",
                   ConfigParam::Bool, const_cast<bool*>(&m_touchClassifier.m_elongatedEnabled)).Module("Touch Classification");
    if (!IsFrozenCurrentTouchConfigKey("PalmElongatedMinArea")) s.emplace_back("PalmElongatedMinArea", "Elongated Min Area",
                   ConfigParam::Int, const_cast<int*>(&m_touchClassifier.m_elongatedMinArea), 3, 100).Module("Touch Classification");
    if (!IsFrozenCurrentTouchConfigKey("PalmElongatedAspectRatio")) s.emplace_back("PalmElongatedAspectRatio", "Elongated Aspect Ratio",
                   ConfigParam::Float, const_cast<float*>(&m_touchClassifier.m_elongatedAspectRatio), 1.5f, 10.0f).Module("Touch Classification");
    if (!IsFrozenCurrentTouchConfigKey("PalmAnalyzerEnabled")) s.emplace_back("PalmAnalyzerEnabled", "Palm Analyzer Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_touchClassifier.m_analyzerEnabled)).Module("Touch Classification");
    if (!IsFrozenCurrentTouchConfigKey("PalmCandidateAreaThreshold")) s.emplace_back("PalmCandidateAreaThreshold", "Palm Candidate Area",
                   ConfigParam::Int, const_cast<int*>(&m_touchClassifier.m_candidateAreaThreshold), 5, 300).Module("Touch Classification");
    if (!IsFrozenCurrentTouchConfigKey("PalmCandidateSignalThreshold")) s.emplace_back("PalmCandidateSignalThreshold", "Palm Candidate Signal",
                   ConfigParam::Int, const_cast<int*>(&m_touchClassifier.m_candidateSignalThreshold), 1000, 500000).Module("Touch Classification");
    if (!IsFrozenCurrentTouchConfigKey("PalmLikelyAreaThreshold")) s.emplace_back("PalmLikelyAreaThreshold", "Palm Likely Area",
                   ConfigParam::Int, const_cast<int*>(&m_touchClassifier.m_likelyAreaThreshold), 5, 300).Module("Touch Classification");
    if (!IsFrozenCurrentTouchConfigKey("PalmFillRatioThreshold")) s.emplace_back("PalmFillRatioThreshold", "Palm Fill Ratio",
                   ConfigParam::Float, const_cast<float*>(&m_touchClassifier.m_fillRatioThreshold), 0.0f, 1.0f).Module("Touch Classification");
    if (!IsFrozenCurrentTouchConfigKey("PalmFlatSharpnessThreshold")) s.emplace_back("PalmFlatSharpnessThreshold", "Palm Flat Sharpness",
                   ConfigParam::Float, const_cast<float*>(&m_touchClassifier.m_flatSharpnessThreshold), 1.0f, 4.0f).Module("Touch Classification");
    if (!IsFrozenCurrentTouchConfigKey("PalmStrongPeakProminence")) s.emplace_back("PalmStrongPeakProminence", "Palm Strong Peak Prominence",
                   ConfigParam::Int, const_cast<int*>(&m_touchClassifier.m_strongPeakProminence), 0, 5000).Module("Touch Classification");
    if (!IsFrozenCurrentTouchConfigKey("PeakEvalEnabled")) s.emplace_back("PeakEvalEnabled", "Peak Eval Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_touchClassifier.m_peakEvalEnabled)).Module("Touch Classification");
    if (!IsFrozenCurrentTouchConfigKey("PeakEvalFingerProminence")) s.emplace_back("PeakEvalFingerProminence", "Peak Eval Finger Prominence",
                   ConfigParam::Int, const_cast<int*>(&m_touchClassifier.m_fingerProminence), 0, 2000).Module("Touch Classification");
    if (!IsFrozenCurrentTouchConfigKey("PeakEvalFingerSharpness")) s.emplace_back("PeakEvalFingerSharpness", "Peak Eval Finger Sharpness",
                   ConfigParam::Float, const_cast<float*>(&m_touchClassifier.m_fingerSharpness), 1.0f, 5.0f).Module("Touch Classification");
    if (!IsFrozenCurrentTouchConfigKey("PeakEvalPalmSharpnessMax")) s.emplace_back("PeakEvalPalmSharpnessMax", "Peak Eval Palm Sharpness Max",
                   ConfigParam::Float, const_cast<float*>(&m_touchClassifier.m_palmSharpnessMax), 1.0f, 5.0f).Module("Touch Classification");
    if (!IsFrozenCurrentTouchConfigKey("PeakEvalAmbiguousMargin")) s.emplace_back("PeakEvalAmbiguousMargin", "Peak Eval Ambiguous Margin",
                   ConfigParam::Float, const_cast<float*>(&m_touchClassifier.m_ambiguousMargin), 0.0f, 1.0f).Module("Touch Classification");
    if (!IsFrozenCurrentTouchConfigKey("PalmAwareExpansionEnabled")) s.emplace_back("PalmAwareExpansionEnabled", "Palm Aware Expansion Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_touchClassifier.m_palmAwareExpansionEnabled)).Module("Touch Classification");
    if (!IsFrozenCurrentTouchConfigKey("PalmFingerInPalmThresholdRatio")) s.emplace_back("PalmFingerInPalmThresholdRatio", "Finger In Palm Threshold Ratio",
                   ConfigParam::Float, const_cast<float*>(&m_touchClassifier.m_fingerInPalmThresholdRatio), 0.0f, 1.0f).Module("Touch Classification");
    if (!IsFrozenCurrentTouchConfigKey("PalmFingerInPalmMaxRadius")) s.emplace_back("PalmFingerInPalmMaxRadius", "Finger In Palm Max Radius",
                   ConfigParam::Int, const_cast<int*>(&m_touchClassifier.m_fingerInPalmMaxRadius), 0, 10).Module("Touch Classification");
    if (!IsFrozenCurrentTouchConfigKey("PalmLikelyAllowContact")) s.emplace_back("PalmLikelyAllowContact", "Palm Likely Allow Contact",
                   ConfigParam::Bool, const_cast<bool*>(&m_touchClassifier.m_palmLikelyAllowContact)).Module("Touch Classification");
    if (!IsFrozenCurrentTouchConfigKey("PalmShadowEnabled")) s.emplace_back("PalmShadowEnabled", "Palm Shadow Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_touchClassifier.m_palmShadowEnabled)).Module("Touch Classification");
    if (!IsFrozenCurrentTouchConfigKey("PalmShadowRadius")) s.emplace_back("PalmShadowRadius", "Palm Shadow Radius",
                   ConfigParam::Int, const_cast<int*>(&m_touchClassifier.m_palmShadowRadius), 0, 8).Module("Touch Classification");
    if (!IsFrozenCurrentTouchConfigKey("PalmShadowHoldFrames")) s.emplace_back("PalmShadowHoldFrames", "Palm Shadow Hold Frames",
                   ConfigParam::Int, const_cast<int*>(&m_touchClassifier.m_palmShadowHoldFrames), 0, 60).Module("Touch Classification");
    if (!IsFrozenCurrentTouchConfigKey("PalmShadowSeedScore")) s.emplace_back("PalmShadowSeedScore", "Palm Shadow Seed Score",
                   ConfigParam::Float, const_cast<float*>(&m_touchClassifier.m_palmShadowSeedScore), 0.0f, 1.0f).Module("Touch Classification");

    // ── Tracking ──
    if (!IsFrozenCurrentTouchConfigKey("TrackerEnabled")) s.emplace_back("TrackerEnabled", "Tracker Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_tracker.m_enabled)).Module("Tracking");
    if (!IsFrozenCurrentTouchConfigKey("UseHungarian")) s.emplace_back("UseHungarian", "Use Hungarian",
                   ConfigParam::Bool, const_cast<bool*>(&m_tracker.m_useHungarian)).Module("Tracking");
    if (!IsFrozenCurrentTouchConfigKey("MaxTrackDistance")) s.emplace_back("MaxTrackDistance", "Max Track Dist",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_maxTrackDistance), 1.0f, 20.0f).Module("Tracking");
    if (!IsFrozenCurrentTouchConfigKey("AlwaysMatchDistance")) s.emplace_back("AlwaysMatchDistance", "Always Match Dist",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_alwaysMatchDistance), 0.5f, 6.0f).Module("Tracking");
    if (!IsFrozenCurrentTouchConfigKey("EdgeTrackBoost")) s.emplace_back("EdgeTrackBoost", "Edge Track Boost",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_edgeTrackBoost), 1.0f, 5.0f).Module("Tracking");
    if (!IsFrozenCurrentTouchConfigKey("AccThresholdBoost")) s.emplace_back("AccThresholdBoost", "Accel Gate Boost",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_accThresholdBoost), 1.0f, 8.0f).Module("Tracking");
    if (!IsFrozenCurrentTouchConfigKey("AccBoostSizeMm")) s.emplace_back("AccBoostSizeMm", "Accel Boost Size",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_accBoostSizeMm), 0.1f, 5.0f).Module("Tracking");
    if (!IsFrozenCurrentTouchConfigKey("PredictionScale")) s.emplace_back("PredictionScale", "Prediction Scale",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_predictionScale), 0.0f, 2.0f).Module("Tracking");
    if (!IsFrozenCurrentTouchConfigKey("GapRelinkEnabled")) s.emplace_back("GapRelinkEnabled", "Gap Relink Enable",
                   ConfigParam::Bool, const_cast<bool*>(&m_tracker.m_gapRelinkEnabled)).Module("Tracking");
    if (!IsFrozenCurrentTouchConfigKey("GapRelinkWindowFrames")) s.emplace_back("GapRelinkWindowFrames", "Gap Relink Window",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_gapRelinkWindowFrames), 0, 5).Module("Tracking");
    if (!IsFrozenCurrentTouchConfigKey("TouchDownDebounceFrames")) s.emplace_back("TouchDownDebounceFrames", "Down Debounce",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_touchDownDebounceFrames), 0, 10).Module("Tracking");
    if (!IsFrozenCurrentTouchConfigKey("DynamicDebounceEnabled")) s.emplace_back("DynamicDebounceEnabled", "Dynamic Debounce",
                   ConfigParam::Bool, const_cast<bool*>(&m_tracker.m_dynamicDebounceEnabled)).Module("Tracking");
    if (!IsFrozenCurrentTouchConfigKey("TouchDownDebounceMaxExtra")) s.emplace_back("TouchDownDebounceMaxExtra", "Debounce Extra Cap",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_touchDownDebounceMaxExtra), 0, 10).Module("Tracking");
    if (!IsFrozenCurrentTouchConfigKey("TouchDownWeakSignalThreshold")) s.emplace_back("TouchDownWeakSignalThreshold", "Weak Signal Threshold",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_touchDownWeakSignalThreshold), 0, 2000).Module("Tracking");
    if (!IsFrozenCurrentTouchConfigKey("TouchDownSmallSizeThresholdMm")) s.emplace_back("TouchDownSmallSizeThresholdMm", "Small Size Threshold",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_touchDownSmallSizeThresholdMm), 0.1f, 5.0f).Module("Tracking");
    if (!IsFrozenCurrentTouchConfigKey("TouchDownRejectEnabled")) s.emplace_back("TouchDownRejectEnabled", "Enable Reject",
                   ConfigParam::Bool, const_cast<bool*>(&m_tracker.m_touchDownRejectEnabled)).Module("Tracking");
    if (!IsFrozenCurrentTouchConfigKey("TouchDownRejectMinSignal")) s.emplace_back("TouchDownRejectMinSignal", "Reject Signal Th",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_touchDownRejectMinSignal), 0, 500).Module("Tracking");
    if (!IsFrozenCurrentTouchConfigKey("TouchDownRejectMinSizeMm")) s.emplace_back("TouchDownRejectMinSizeMm", "Reject Size Th",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_touchDownRejectMinSizeMm), 0.1f, 5.0f).Module("Tracking");
    if (!IsFrozenCurrentTouchConfigKey("TouchDownEdgeRejectMinSignal")) s.emplace_back("TouchDownEdgeRejectMinSignal", "Edge Reject Signal",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_touchDownEdgeRejectMinSignal), 0, 2000).Module("Tracking");
    if (!IsFrozenCurrentTouchConfigKey("FallbackSizeMm")) s.emplace_back("FallbackSizeMm", "Fallback Size",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_fallbackSizeMm), 0.1f, 10.0f).Module("Tracking");
    if (!IsFrozenCurrentTouchConfigKey("SizeAreaScale")) s.emplace_back("SizeAreaScale", "Size Area Scale",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_sizeAreaScale), 0.0f, 5.0f).Module("Tracking");
    if (!IsFrozenCurrentTouchConfigKey("SizeSignalScale")) s.emplace_back("SizeSignalScale", "Size Signal Scale",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_sizeSignalScale), 0.0f, 5.0f).Module("Tracking");
    if (!IsFrozenCurrentTouchConfigKey("RxGhostFilterEnabled")) s.emplace_back("RxGhostFilterEnabled", "RX Ghost Filter",
                   ConfigParam::Bool, const_cast<bool*>(&m_tracker.m_rxGhostFilterEnabled)).Module("Tracking");
    if (!IsFrozenCurrentTouchConfigKey("RxGhostLineDelta")) s.emplace_back("RxGhostLineDelta", "RX Ghost Delta",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_rxGhostLineDelta), 0, 10).Module("Tracking");
    if (!IsFrozenCurrentTouchConfigKey("RxGhostWeakRatio")) s.emplace_back("RxGhostWeakRatio", "RX Ghost Weak Ratio",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_rxGhostWeakRatio), 0.0f, 1.0f).Module("Tracking");
    if (!IsFrozenCurrentTouchConfigKey("RxGhostOnlyNew")) s.emplace_back("RxGhostOnlyNew", "RX Ghost Only New",
                   ConfigParam::Bool, const_cast<bool*>(&m_tracker.m_rxGhostOnlyNew)).Module("Tracking");

    // ── Stylus Suppress ──
    if (!IsFrozenCurrentTouchConfigKey("StylusSuppressGlobalEnabled")) s.emplace_back("StylusSuppressGlobalEnabled", "Pen Global Suppress",
                   ConfigParam::Bool, const_cast<bool*>(&m_tracker.m_stylusSuppressGlobalEnabled)).Module("Stylus Suppress");
    if (!IsFrozenCurrentTouchConfigKey("StylusSuppressLocalEnabled")) s.emplace_back("StylusSuppressLocalEnabled", "Pen Local Suppress",
                   ConfigParam::Bool, const_cast<bool*>(&m_tracker.m_stylusSuppressLocalEnabled)).Module("Stylus Suppress");
    if (!IsFrozenCurrentTouchConfigKey("StylusSuppressLocalDistance")) s.emplace_back("StylusSuppressLocalDistance", "Suppress Radius",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_stylusSuppressLocalDistance), 0.5f, 10.0f).Module("Stylus Suppress");
    if (!IsFrozenCurrentTouchConfigKey("StylusSuppressPenPeakThreshold")) s.emplace_back("StylusSuppressPenPeakThreshold", "Pen Peak Threshold",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_stylusSuppressPenPeakThreshold), 0, 10000).Module("Stylus Suppress");
    if (!IsFrozenCurrentTouchConfigKey("StylusSuppressTouchSignalKeep")) s.emplace_back("StylusSuppressTouchSignalKeep", "Touch Signal Keep",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_stylusSuppressTouchSignalKeep), 0, 30000).Module("Stylus Suppress");
    if (!IsFrozenCurrentTouchConfigKey("StylusSuppressTouchAreaKeep")) s.emplace_back("StylusSuppressTouchAreaKeep", "Touch Area Keep",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_stylusSuppressTouchAreaKeep), 0, 100).Module("Stylus Suppress");
    if (!IsFrozenCurrentTouchConfigKey("StylusAftEnabled")) s.emplace_back("StylusAftEnabled", "Enable AFT (Anti-Falsing)",
                   ConfigParam::Bool, const_cast<bool*>(&m_tracker.m_stylusAftEnabled)).Module("Stylus Suppress");
    if (!IsFrozenCurrentTouchConfigKey("StylusAftRecentFrames")) s.emplace_back("StylusAftRecentFrames", "AFT Recent Frames",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_stylusAftRecentFrames), 0, 200).Module("Stylus Suppress");
    if (!IsFrozenCurrentTouchConfigKey("StylusAftRadius")) s.emplace_back("StylusAftRadius", "AFT Radius",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_stylusAftRadius), 0.5f, 10.0f).Module("Stylus Suppress");
    if (!IsFrozenCurrentTouchConfigKey("StylusAftDebounceFrames")) s.emplace_back("StylusAftDebounceFrames", "AFT Debounce Frames",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_stylusAftDebounceFrames), 0, 30).Module("Stylus Suppress");
    if (!IsFrozenCurrentTouchConfigKey("StylusAftWeakSignalThreshold")) s.emplace_back("StylusAftWeakSignalThreshold", "AFT Weak Signal",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_stylusAftWeakSignalThreshold), 0, 5000).Module("Stylus Suppress");
    if (!IsFrozenCurrentTouchConfigKey("StylusAftWeakSizeThresholdMm")) s.emplace_back("StylusAftWeakSizeThresholdMm", "AFT Weak Size",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_stylusAftWeakSizeThresholdMm), 0.1f, 10.0f).Module("Stylus Suppress");
    if (!IsFrozenCurrentTouchConfigKey("StylusAftSuppressFrames")) s.emplace_back("StylusAftSuppressFrames", "AFT Suppress Frames",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_stylusAftSuppressFrames), 0, 200).Module("Stylus Suppress");
    if (!IsFrozenCurrentTouchConfigKey("StylusAftPalmSuppressFrames")) s.emplace_back("StylusAftPalmSuppressFrames", "AFT Palm Suppress Frames",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_stylusAftPalmSuppressFrames), 0, 300).Module("Stylus Suppress");
    if (!IsFrozenCurrentTouchConfigKey("StylusAftPalmAreaThreshold")) s.emplace_back("StylusAftPalmAreaThreshold", "AFT Palm Area",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_stylusAftPalmAreaThreshold), 0, 500).Module("Stylus Suppress");
    if (!IsFrozenCurrentTouchConfigKey("StylusAftPalmSizeThresholdMm")) s.emplace_back("StylusAftPalmSizeThresholdMm", "AFT Palm Size",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_stylusAftPalmSizeThresholdMm), 0.1f, 20.0f).Module("Stylus Suppress");

    // ── Coordinate Filter ──
    if (!IsFrozenCurrentTouchConfigKey("CoordFilterEnabled")) s.emplace_back("CoordFilterEnabled", "Coord Filter Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_coordFilter.m_enabled)).Module("Coordinate Filter");
    if (!IsFrozenCurrentTouchConfigKey("OneEuroMinCutoff")) s.emplace_back("OneEuroMinCutoff", "1€ Min Cutoff",
                   ConfigParam::Float, const_cast<float*>(&m_coordFilter.m_minCutoff), 0.1f, 20.0f).Module("Coordinate Filter");
    if (!IsFrozenCurrentTouchConfigKey("OneEuroBeta")) s.emplace_back("OneEuroBeta", "1€ Beta",
                   ConfigParam::Float, const_cast<float*>(&m_coordFilter.m_beta), 0.0f, 0.5f).Module("Coordinate Filter");
    if (!IsFrozenCurrentTouchConfigKey("OneEuroDCutoff")) s.emplace_back("OneEuroDCutoff", "1€ Deriv Cutoff",
                   ConfigParam::Float, const_cast<float*>(&m_coordFilter.m_dCutoff), 0.1f, 10.0f).Module("Coordinate Filter");

    // ── Gesture ──
    if (!IsFrozenCurrentTouchConfigKey("GestureEnabled")) s.emplace_back("GestureEnabled", "Gesture SM Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_gesture.m_enabled)).Module("Gesture");
    if (!IsFrozenCurrentTouchConfigKey("PressCandidateFrames")) s.emplace_back("PressCandidateFrames", "Press Candidate Frames",
                   ConfigParam::Int, const_cast<int*>(&m_gesture.m_pressCandidateFrames), 1, 10).Module("Gesture");
    if (!IsFrozenCurrentTouchConfigKey("DragThreshold")) s.emplace_back("DragThreshold", "Drag Threshold",
                   ConfigParam::Float, const_cast<float*>(&m_gesture.m_dragThreshold), 0.1f, 5.0f).Module("Gesture");
    if (!IsFrozenCurrentTouchConfigKey("LongPressFrames")) s.emplace_back("LongPressFrames", "LongPress Frames",
                   ConfigParam::Int, const_cast<int*>(&m_gesture.m_longPressFrames), 10, 120).Module("Gesture");
    if (!IsFrozenCurrentTouchConfigKey("ReleasePendingFrames")) s.emplace_back("ReleasePendingFrames", "Release Pending Frames",
                   ConfigParam::Int, const_cast<int*>(&m_gesture.m_releasePendingFrames), 0, 10).Module("Gesture");
    if (!IsFrozenCurrentTouchConfigKey("BypassStateMachine")) s.emplace_back("BypassStateMachine", "Bypass State Machine",
                   ConfigParam::Bool, const_cast<bool*>(&m_gesture.m_bypassStateMachine)).Module("Gesture");

    return s;
}

// ══════════════════════════════════════════════════════════════════════
// SaveConfig
// ══════════════════════════════════════════════════════════════════════
void TouchPipeline::SaveConfig(std::ostream& out) const {
    // Phase 1
    WriteConfigLine(out, "FrameParserEnabled", (m_frameParser.m_enabled?"1":"0"));
    // Phase 2: Baseline
    WriteConfigLine(out, "BaselineEnabled", (m_baseline.m_enabled?"1":"0"));
    WriteConfigLine(out, "BaselineValue", m_baseline.m_baseline);
    WriteConfigLine(out, "BaselineNoiseDeadband", m_baseline.m_noiseDeadband);
    WriteConfigLine(out, "BaselinePositiveDeadband", m_baseline.m_positiveDeadband);
    WriteConfigLine(out, "BaselineNegativeDeadband", m_baseline.m_negativeDeadband);
    WriteConfigLine(out, "BaselinePeakThreshold", m_baseline.m_peakThreshold);
    WriteConfigLine(out, "BaselineReleaseHoldFrames", m_baseline.m_releaseHoldFrames);
    WriteConfigLine(out, "BaselinePositiveAlphaShift", m_baseline.m_positiveAlphaShift);
    WriteConfigLine(out, "BaselineNegativeAlphaShift", m_baseline.m_negativeAlphaShift);
    WriteConfigLine(out, "BaselineNoiseAlphaShift", m_baseline.m_noiseAlphaShift);
    WriteConfigLine(out, "BaselineBackgroundAlphaShift", m_baseline.m_backgroundAlphaShift);
    WriteConfigLine(out, "BaselineNoFingerAlphaShift", m_baseline.m_noFingerAlphaShift);
    WriteConfigLine(out, "BaselinePositiveMaxStep", m_baseline.m_positiveMaxStep);
    WriteConfigLine(out, "BaselineNegativeMaxStep", m_baseline.m_negativeMaxStep);
    WriteConfigLine(out, "BaselineBackgroundMaxStep", m_baseline.m_backgroundMaxStep);
    WriteConfigLine(out, "BaselineNoFingerMaxStep", m_baseline.m_noFingerMaxStep);
    WriteConfigLine(out, "BaselineRecoveryAlphaShift", m_baseline.m_recoveryAlphaShift);
    WriteConfigLine(out, "BaselineRecoveryMaxStep", m_baseline.m_recoveryMaxStep);
    WriteConfigLine(out, "BaselineRecoveryMaxFrames", m_baseline.m_recoveryMaxFrames);
    WriteConfigLine(out, "BaselineNoiseTrackingEnabled", (m_baseline.m_noiseTrackingEnabled?"1":"0"));
    // Phase 2: CMF
    WriteConfigLine(out, "CMFEnabled", (m_cmf.m_enabled?"1":"0"));
    WriteConfigLine(out, "CMFDimensionMode", static_cast<int>(m_cmf.m_mode));
    WriteConfigLine(out, "CMFExclusionThreshold", m_cmf.m_exclusionThreshold);
    WriteConfigLine(out, "CMFMaxCorrection", m_cmf.m_maxCorrection);
    // Phase 3: PeakDetector (same keys as old FeatureExtractor)
    WriteConfigLine(out, "PeakThreshold", m_peakDet.m_threshold);
    WriteConfigLine(out, "SigTholdLimit", m_peakDet.m_sigTholdLimit);
    WriteConfigLine(out, "Z8FilterEnabled", (m_peakDet.m_z8Filter?"1":"0"));
    WriteConfigLine(out, "Z1FilterEnabled", (m_peakDet.m_z1Filter?"1":"0"));
    WriteConfigLine(out, "PressureDriftFilter", (m_peakDet.m_pressureDriftFilter?"1":"0"));
    WriteConfigLine(out, "EdgePeakFilter", (m_peakDet.m_edgePeakFilter?"1":"0"));
    WriteConfigLine(out, "EdgeThresholdEnabled", (m_peakDet.m_edgeThresholdEnabled?"1":"0"));
    WriteConfigLine(out, "EdgeThreshold", m_peakDet.m_edgeThreshold);
    WriteConfigLine(out, "Z8Radius", m_peakDet.m_z8Radius);
    WriteConfigLine(out, "MaxPeaks", m_peakDet.m_maxPeaks);
    WriteConfigLine(out, "PressureDriftDebounce", m_peakDet.m_pressureDriftDebounceLimit);
    WriteConfigLine(out, "MacroZoneMinArea", m_peakDet.m_macroZoneMinArea);
    // Phase 4: ZoneExpander
    WriteConfigLine(out, "DilateErode", (m_contactExtractor.m_zoneExp.m_dilateErode?"1":"0"));
    WriteConfigLine(out, "ZoneTholdScale", m_contactExtractor.m_zoneExp.m_tholdScaleNumer);
    WriteConfigLine(out, "ZoneTholdShift", m_contactExtractor.m_zoneExp.m_tholdScaleShift);
    WriteConfigLine(out, "MaxTouches", m_contactExtractor.m_zoneExp.m_maxTouches);
    // Phase 3: TouchClassifier
    WriteConfigLine(out, "PalmEnabled", (m_touchClassifier.m_enabled?"1":"0"));
    WriteConfigLine(out, "PalmAreaThreshold", m_touchClassifier.m_areaThreshold);
    WriteConfigLine(out, "PalmSignalSumThreshold", m_touchClassifier.m_signalSumThreshold);
    WriteConfigLine(out, "PalmDensityThresholdLow", m_touchClassifier.m_densityThresholdLow);
    WriteConfigLine(out, "PalmAreaMinForDensity", m_touchClassifier.m_areaMinForDensity);
    WriteConfigLine(out, "PalmElongatedEnabled", (m_touchClassifier.m_elongatedEnabled?"1":"0"));
    WriteConfigLine(out, "PalmElongatedMinArea", m_touchClassifier.m_elongatedMinArea);
    WriteConfigLine(out, "PalmElongatedAspectRatio", m_touchClassifier.m_elongatedAspectRatio);
    WriteConfigLine(out, "PalmAnalyzerEnabled", (m_touchClassifier.m_analyzerEnabled?"1":"0"));
    WriteConfigLine(out, "PalmCandidateAreaThreshold", m_touchClassifier.m_candidateAreaThreshold);
    WriteConfigLine(out, "PalmCandidateSignalThreshold", m_touchClassifier.m_candidateSignalThreshold);
    WriteConfigLine(out, "PalmLikelyAreaThreshold", m_touchClassifier.m_likelyAreaThreshold);
    WriteConfigLine(out, "PalmFillRatioThreshold", m_touchClassifier.m_fillRatioThreshold);
    WriteConfigLine(out, "PalmFlatSharpnessThreshold", m_touchClassifier.m_flatSharpnessThreshold);
    WriteConfigLine(out, "PalmStrongPeakProminence", m_touchClassifier.m_strongPeakProminence);
    WriteConfigLine(out, "PeakEvalEnabled", (m_touchClassifier.m_peakEvalEnabled?"1":"0"));
    WriteConfigLine(out, "PeakEvalFingerProminence", m_touchClassifier.m_fingerProminence);
    WriteConfigLine(out, "PeakEvalFingerSharpness", m_touchClassifier.m_fingerSharpness);
    WriteConfigLine(out, "PeakEvalPalmSharpnessMax", m_touchClassifier.m_palmSharpnessMax);
    WriteConfigLine(out, "PeakEvalAmbiguousMargin", m_touchClassifier.m_ambiguousMargin);
    WriteConfigLine(out, "PalmAwareExpansionEnabled", (m_touchClassifier.m_palmAwareExpansionEnabled?"1":"0"));
    WriteConfigLine(out, "PalmFingerInPalmThresholdRatio", m_touchClassifier.m_fingerInPalmThresholdRatio);
    WriteConfigLine(out, "PalmFingerInPalmMaxRadius", m_touchClassifier.m_fingerInPalmMaxRadius);
    WriteConfigLine(out, "PalmLikelyAllowContact", (m_touchClassifier.m_palmLikelyAllowContact?"1":"0"));
    WriteConfigLine(out, "PalmShadowEnabled", (m_touchClassifier.m_palmShadowEnabled?"1":"0"));
    WriteConfigLine(out, "PalmShadowRadius", m_touchClassifier.m_palmShadowRadius);
    WriteConfigLine(out, "PalmShadowHoldFrames", m_touchClassifier.m_palmShadowHoldFrames);
    WriteConfigLine(out, "PalmShadowSeedScore", m_touchClassifier.m_palmShadowSeedScore);
    // Phase 5: TouchTracker (same keys as old TouchTracker)
    WriteConfigLine(out, "TrackerEnabled", (m_tracker.m_enabled?"1":"0"));
    WriteConfigLine(out, "UseHungarian", (m_tracker.m_useHungarian?"1":"0"));
    WriteConfigLine(out, "MaxTrackDistance", m_tracker.m_maxTrackDistance);
    WriteConfigLine(out, "AlwaysMatchDistance", m_tracker.m_alwaysMatchDistance);
    WriteConfigLine(out, "EdgeTrackBoost", m_tracker.m_edgeTrackBoost);
    WriteConfigLine(out, "AccThresholdBoost", m_tracker.m_accThresholdBoost);
    WriteConfigLine(out, "AccBoostSizeMm", m_tracker.m_accBoostSizeMm);
    WriteConfigLine(out, "PredictionScale", m_tracker.m_predictionScale);
    WriteConfigLine(out, "GapRelinkEnabled", (m_tracker.m_gapRelinkEnabled?"1":"0"));
    WriteConfigLine(out, "GapRelinkWindowFrames", m_tracker.m_gapRelinkWindowFrames);
    WriteConfigLine(out, "TouchDownDebounceFrames", m_tracker.m_touchDownDebounceFrames);
    WriteConfigLine(out, "DynamicDebounceEnabled", (m_tracker.m_dynamicDebounceEnabled?"1":"0"));
    WriteConfigLine(out, "TouchDownDebounceMaxExtra", m_tracker.m_touchDownDebounceMaxExtra);
    WriteConfigLine(out, "TouchDownWeakSignalThreshold", m_tracker.m_touchDownWeakSignalThreshold);
    WriteConfigLine(out, "TouchDownSmallSizeThresholdMm", m_tracker.m_touchDownSmallSizeThresholdMm);
    WriteConfigLine(out, "TouchDownRejectEnabled", (m_tracker.m_touchDownRejectEnabled?"1":"0"));
    WriteConfigLine(out, "TouchDownRejectMinSignal", m_tracker.m_touchDownRejectMinSignal);
    WriteConfigLine(out, "TouchDownRejectMinSizeMm", m_tracker.m_touchDownRejectMinSizeMm);
    WriteConfigLine(out, "TouchDownEdgeRejectMinSignal", m_tracker.m_touchDownEdgeRejectMinSignal);
    WriteConfigLine(out, "FallbackSizeMm", m_tracker.m_fallbackSizeMm);
    WriteConfigLine(out, "SizeAreaScale", m_tracker.m_sizeAreaScale);
    WriteConfigLine(out, "SizeSignalScale", m_tracker.m_sizeSignalScale);
    WriteConfigLine(out, "RxGhostFilterEnabled", (m_tracker.m_rxGhostFilterEnabled?"1":"0"));
    WriteConfigLine(out, "RxGhostLineDelta", m_tracker.m_rxGhostLineDelta);
    WriteConfigLine(out, "RxGhostWeakRatio", m_tracker.m_rxGhostWeakRatio);
    WriteConfigLine(out, "RxGhostOnlyNew", (m_tracker.m_rxGhostOnlyNew?"1":"0"));
    WriteConfigLine(out, "StylusSuppressGlobalEnabled", (m_tracker.m_stylusSuppressGlobalEnabled?"1":"0"));
    WriteConfigLine(out, "StylusSuppressLocalEnabled", (m_tracker.m_stylusSuppressLocalEnabled?"1":"0"));
    WriteConfigLine(out, "StylusSuppressLocalDistance", m_tracker.m_stylusSuppressLocalDistance);
    WriteConfigLine(out, "StylusSuppressPenPeakThreshold", m_tracker.m_stylusSuppressPenPeakThreshold);
    WriteConfigLine(out, "StylusSuppressTouchSignalKeep", m_tracker.m_stylusSuppressTouchSignalKeep);
    WriteConfigLine(out, "StylusSuppressTouchAreaKeep", m_tracker.m_stylusSuppressTouchAreaKeep);
    WriteConfigLine(out, "StylusAftEnabled", (m_tracker.m_stylusAftEnabled?"1":"0"));
    WriteConfigLine(out, "StylusAftRecentFrames", m_tracker.m_stylusAftRecentFrames);
    WriteConfigLine(out, "StylusAftRadius", m_tracker.m_stylusAftRadius);
    WriteConfigLine(out, "StylusAftDebounceFrames", m_tracker.m_stylusAftDebounceFrames);
    WriteConfigLine(out, "StylusAftWeakSignalThreshold", m_tracker.m_stylusAftWeakSignalThreshold);
    WriteConfigLine(out, "StylusAftWeakSizeThresholdMm", m_tracker.m_stylusAftWeakSizeThresholdMm);
    WriteConfigLine(out, "StylusAftSuppressFrames", m_tracker.m_stylusAftSuppressFrames);
    WriteConfigLine(out, "StylusAftPalmSuppressFrames", m_tracker.m_stylusAftPalmSuppressFrames);
    WriteConfigLine(out, "StylusAftPalmAreaThreshold", m_tracker.m_stylusAftPalmAreaThreshold);
    WriteConfigLine(out, "StylusAftPalmSizeThresholdMm", m_tracker.m_stylusAftPalmSizeThresholdMm);
    // Phase 5: CoordinateFilter
    WriteConfigLine(out, "CoordFilterEnabled", (m_coordFilter.m_enabled?"1":"0"));
    WriteConfigLine(out, "OneEuroMinCutoff", m_coordFilter.m_minCutoff);
    WriteConfigLine(out, "OneEuroBeta", m_coordFilter.m_beta);
    WriteConfigLine(out, "OneEuroDCutoff", m_coordFilter.m_dCutoff);
    // Phase 6: GestureStateMachine
    WriteConfigLine(out, "GestureEnabled", (m_gesture.m_enabled?"1":"0"));
    WriteConfigLine(out, "PressCandidateFrames", m_gesture.m_pressCandidateFrames);
    WriteConfigLine(out, "PressCandidateMinSignal", m_gesture.m_pressCandidateMinSignal);
    WriteConfigLine(out, "PressCandidateMinSizeMm", m_gesture.m_pressCandidateMinSizeMm);
    WriteConfigLine(out, "DragThreshold", m_gesture.m_dragThreshold);
    WriteConfigLine(out, "LongPressFrames", m_gesture.m_longPressFrames);
    WriteConfigLine(out, "LongPressMoveTolerance", m_gesture.m_longPressMoveTolerance);
    WriteConfigLine(out, "ReleasePendingFrames", m_gesture.m_releasePendingFrames);
    WriteConfigLine(out, "BypassStateMachine", (m_gesture.m_bypassStateMachine?"1":"0"));
}
// ══════════════════════════════════════════════════════════════════════
// LoadConfig — key/value dispatch (compatible with old config keys)
// ══════════════════════════════════════════════════════════════════════
void TouchPipeline::LoadConfig(const std::string& key,
                                const std::string& value) {
    if (IsFrozenCurrentTouchConfigKey(key)) return;

    const TouchConfigKey configKey = FindTouchConfigKey(key);
    bool needsStylusSync = false;
    auto toBool = [&](const std::string& v) { return ParseConfigBool(key, v); };
    try {
        switch (configKey) {
        case TouchConfigKey::AccBoostSizeMm:
            m_tracker.m_accBoostSizeMm = ParseConfigFloat(key, value);
            break;
        case TouchConfigKey::AccThresholdBoost:
            m_tracker.m_accThresholdBoost = ParseConfigFloat(key, value);
            break;
        case TouchConfigKey::AlwaysMatchDistance:
            m_tracker.m_alwaysMatchDistance = ParseConfigFloat(key, value);
            break;
        case TouchConfigKey::BaselineBackgroundAlphaShift:
            m_baseline.m_backgroundAlphaShift = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::BaselineBackgroundMaxStep:
            m_baseline.m_backgroundMaxStep = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::BaselineEnabled:
            m_baseline.m_enabled = toBool(value);
            break;
        case TouchConfigKey::BaselineNegativeAlphaShift:
            m_baseline.m_negativeAlphaShift = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::BaselineNegativeDeadband:
            m_baseline.m_negativeDeadband = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::BaselineNegativeMaxStep:
            m_baseline.m_negativeMaxStep = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::BaselineNoFingerAlphaShift:
            m_baseline.m_noFingerAlphaShift = std::clamp(ParseConfigInt(key, value), 0, 15);
            break;
        case TouchConfigKey::BaselineNoFingerMaxStep:
            m_baseline.m_noFingerMaxStep = std::clamp(ParseConfigInt(key, value), 1, 2048);
            break;
        case TouchConfigKey::BaselineNoiseAlphaShift:
            m_baseline.m_noiseAlphaShift = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::BaselineNoiseDeadband:
            m_baseline.m_noiseDeadband = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::BaselineNoiseTrackingEnabled:
            m_baseline.m_noiseTrackingEnabled = toBool(value);
            break;
        case TouchConfigKey::BaselinePeakThreshold:
            m_baseline.m_peakThreshold = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::BaselinePositiveAlphaShift:
            m_baseline.m_positiveAlphaShift = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::BaselinePositiveDeadband:
            m_baseline.m_positiveDeadband = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::BaselinePositiveMaxStep:
            m_baseline.m_positiveMaxStep = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::BaselineRecoveryAlphaShift:
            m_baseline.m_recoveryAlphaShift = std::clamp(ParseConfigInt(key, value), 0, 15);
            break;
        case TouchConfigKey::BaselineRecoveryMaxFrames:
            m_baseline.m_recoveryMaxFrames = std::clamp(ParseConfigInt(key, value), 1, 120);
            break;
        case TouchConfigKey::BaselineRecoveryMaxStep:
            m_baseline.m_recoveryMaxStep = std::clamp(ParseConfigInt(key, value), 1, 2048);
            break;
        case TouchConfigKey::BaselineReleaseHoldFrames:
            m_baseline.m_releaseHoldFrames = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::BaselineValue:
            m_baseline.m_baseline = ParseConfigInt(key, value);
            m_baseline.Reset();
            break;
        case TouchConfigKey::BypassStateMachine:
            m_gesture.m_bypassStateMachine = toBool(value);
            break;
        case TouchConfigKey::CMFDimensionMode:
            m_cmf.m_mode = static_cast<Touch::CMFProcessor::DimensionMode>(ParseConfigInt(key, value));
            break;
        case TouchConfigKey::CMFEnabled:
            m_cmf.m_enabled = toBool(value);
            break;
        case TouchConfigKey::CMFExclusionThreshold:
            m_cmf.m_exclusionThreshold = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::CMFMaxCorrection:
            m_cmf.m_maxCorrection = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::CoordFilterEnabled:
            m_coordFilter.m_enabled = toBool(value);
            break;
        case TouchConfigKey::DilateErode:
            m_contactExtractor.m_zoneExp.m_dilateErode = toBool(value);
            break;
        case TouchConfigKey::DragThreshold:
            m_gesture.m_dragThreshold = ParseConfigFloat(key, value);
            break;
        case TouchConfigKey::DynamicDebounceEnabled:
            m_tracker.m_dynamicDebounceEnabled = toBool(value);
            break;
        case TouchConfigKey::EdgePeakFilter:
            m_peakDet.m_edgePeakFilter = toBool(value);
            break;
        case TouchConfigKey::EdgeThreshold:
            m_peakDet.m_edgeThreshold = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::EdgeThresholdEnabled:
            m_peakDet.m_edgeThresholdEnabled = toBool(value);
            break;
        case TouchConfigKey::EdgeTrackBoost:
            m_tracker.m_edgeTrackBoost = ParseConfigFloat(key, value);
            break;
        case TouchConfigKey::FallbackSizeMm:
            m_tracker.m_fallbackSizeMm = ParseConfigFloat(key, value);
            needsStylusSync = true;
            break;
        case TouchConfigKey::FrameParserEnabled:
            m_frameParser.m_enabled = toBool(value);
            break;
        case TouchConfigKey::GapRelinkEnabled:
            m_tracker.m_gapRelinkEnabled = toBool(value);
            break;
        case TouchConfigKey::GapRelinkWindowFrames:
            m_tracker.m_gapRelinkWindowFrames = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::GestureEnabled:
            m_gesture.m_enabled = toBool(value);
            break;
        case TouchConfigKey::LongPressFrames:
            m_gesture.m_longPressFrames = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::LongPressMoveTolerance:
            m_gesture.m_longPressMoveTolerance = ParseConfigFloat(key, value);
            break;
        case TouchConfigKey::MacroZoneMinArea:
            m_peakDet.m_macroZoneMinArea = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::MaxPeaks:
            m_peakDet.m_maxPeaks = std::clamp(ParseConfigInt(key, value), 1, Touch::PeakDetector::kMaxStoredPeaks);
            break;
        case TouchConfigKey::MaxTouches:
            m_contactExtractor.m_zoneExp.m_maxTouches = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::MaxTrackDistance:
            m_tracker.m_maxTrackDistance = ParseConfigFloat(key, value);
            break;
        case TouchConfigKey::OneEuroBeta:
            m_coordFilter.m_beta = ParseConfigFloat(key, value);
            break;
        case TouchConfigKey::OneEuroDCutoff:
            m_coordFilter.m_dCutoff = ParseConfigFloat(key, value);
            break;
        case TouchConfigKey::OneEuroMinCutoff:
            m_coordFilter.m_minCutoff = ParseConfigFloat(key, value);
            break;
        case TouchConfigKey::PalmAnalyzerEnabled:
            m_touchClassifier.m_analyzerEnabled = toBool(value);
            break;
        case TouchConfigKey::PalmAreaMinForDensity:
            m_touchClassifier.m_areaMinForDensity = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::PalmAreaThreshold:
            m_touchClassifier.m_areaThreshold = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::PalmAwareExpansionEnabled:
            m_touchClassifier.m_palmAwareExpansionEnabled = toBool(value);
            break;
        case TouchConfigKey::PalmCandidateAreaThreshold:
            m_touchClassifier.m_candidateAreaThreshold = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::PalmCandidateSignalThreshold:
            m_touchClassifier.m_candidateSignalThreshold = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::PalmDensityThresholdLow:
            m_touchClassifier.m_densityThresholdLow = ParseConfigFloat(key, value);
            break;
        case TouchConfigKey::PalmElongatedAspectRatio:
            m_touchClassifier.m_elongatedAspectRatio = ParseConfigFloat(key, value);
            break;
        case TouchConfigKey::PalmElongatedEnabled:
            m_touchClassifier.m_elongatedEnabled = toBool(value);
            break;
        case TouchConfigKey::PalmElongatedMinArea:
            m_touchClassifier.m_elongatedMinArea = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::PalmEnabled:
            m_touchClassifier.m_enabled = toBool(value);
            break;
        case TouchConfigKey::PalmFillRatioThreshold:
            m_touchClassifier.m_fillRatioThreshold = ParseConfigFloat(key, value);
            break;
        case TouchConfigKey::PalmFingerInPalmMaxRadius:
            m_touchClassifier.m_fingerInPalmMaxRadius = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::PalmFingerInPalmThresholdRatio:
            m_touchClassifier.m_fingerInPalmThresholdRatio = ParseConfigFloat(key, value);
            break;
        case TouchConfigKey::PalmFlatSharpnessThreshold:
            m_touchClassifier.m_flatSharpnessThreshold = ParseConfigFloat(key, value);
            break;
        case TouchConfigKey::PalmLikelyAllowContact:
            m_touchClassifier.m_palmLikelyAllowContact = toBool(value);
            break;
        case TouchConfigKey::PalmLikelyAreaThreshold:
            m_touchClassifier.m_likelyAreaThreshold = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::PalmShadowEnabled:
            m_touchClassifier.m_palmShadowEnabled = toBool(value);
            break;
        case TouchConfigKey::PalmShadowHoldFrames:
            m_touchClassifier.m_palmShadowHoldFrames = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::PalmShadowRadius:
            m_touchClassifier.m_palmShadowRadius = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::PalmShadowSeedScore:
            m_touchClassifier.m_palmShadowSeedScore = ParseConfigFloat(key, value);
            break;
        case TouchConfigKey::PalmSignalSumThreshold:
            m_touchClassifier.m_signalSumThreshold = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::PalmStrongPeakProminence:
            m_touchClassifier.m_strongPeakProminence = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::PeakEvalAmbiguousMargin:
            m_touchClassifier.m_ambiguousMargin = ParseConfigFloat(key, value);
            break;
        case TouchConfigKey::PeakEvalEnabled:
            m_touchClassifier.m_peakEvalEnabled = toBool(value);
            break;
        case TouchConfigKey::PeakEvalFingerProminence:
            m_touchClassifier.m_fingerProminence = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::PeakEvalFingerSharpness:
            m_touchClassifier.m_fingerSharpness = ParseConfigFloat(key, value);
            break;
        case TouchConfigKey::PeakEvalPalmSharpnessMax:
            m_touchClassifier.m_palmSharpnessMax = ParseConfigFloat(key, value);
            break;
        case TouchConfigKey::PeakThreshold:
            m_peakDet.m_threshold = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::PredictionScale:
            m_tracker.m_predictionScale = ParseConfigFloat(key, value);
            break;
        case TouchConfigKey::PressCandidateFrames:
            m_gesture.m_pressCandidateFrames = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::PressCandidateMinSignal:
            m_gesture.m_pressCandidateMinSignal = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::PressCandidateMinSizeMm:
            m_gesture.m_pressCandidateMinSizeMm = ParseConfigFloat(key, value);
            break;
        case TouchConfigKey::PressureDriftDebounce:
            m_peakDet.m_pressureDriftDebounceLimit = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::PressureDriftFilter:
            m_peakDet.m_pressureDriftFilter = toBool(value);
            break;
        case TouchConfigKey::ReleasePendingFrames:
            m_gesture.m_releasePendingFrames = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::RxGhostFilterEnabled:
            m_tracker.m_rxGhostFilterEnabled = toBool(value);
            break;
        case TouchConfigKey::RxGhostLineDelta:
            m_tracker.m_rxGhostLineDelta = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::RxGhostOnlyNew:
            m_tracker.m_rxGhostOnlyNew = toBool(value);
            break;
        case TouchConfigKey::RxGhostWeakRatio:
            m_tracker.m_rxGhostWeakRatio = ParseConfigFloat(key, value);
            break;
        case TouchConfigKey::SigTholdLimit:
            m_peakDet.m_sigTholdLimit = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::SizeAreaScale:
            m_tracker.m_sizeAreaScale = ParseConfigFloat(key, value);
            needsStylusSync = true;
            break;
        case TouchConfigKey::SizeSignalScale:
            m_tracker.m_sizeSignalScale = ParseConfigFloat(key, value);
            needsStylusSync = true;
            break;
        case TouchConfigKey::StylusAftDebounceFrames:
            m_tracker.m_stylusAftDebounceFrames = ParseConfigInt(key, value);
            needsStylusSync = true;
            break;
        case TouchConfigKey::StylusAftEnabled:
            m_tracker.m_stylusAftEnabled = toBool(value);
            needsStylusSync = true;
            break;
        case TouchConfigKey::StylusAftPalmAreaThreshold:
            m_tracker.m_stylusAftPalmAreaThreshold = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::StylusAftPalmSizeThresholdMm:
            m_tracker.m_stylusAftPalmSizeThresholdMm = ParseConfigFloat(key, value);
            break;
        case TouchConfigKey::StylusAftPalmSuppressFrames:
            m_tracker.m_stylusAftPalmSuppressFrames = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::StylusAftRadius:
            m_tracker.m_stylusAftRadius = ParseConfigFloat(key, value);
            break;
        case TouchConfigKey::StylusAftRecentFrames:
            m_tracker.m_stylusAftRecentFrames = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::StylusAftSuppressFrames:
            m_tracker.m_stylusAftSuppressFrames = ParseConfigInt(key, value);
            needsStylusSync = true;
            break;
        case TouchConfigKey::StylusAftWeakSignalThreshold:
            m_tracker.m_stylusAftWeakSignalThreshold = ParseConfigInt(key, value);
            needsStylusSync = true;
            break;
        case TouchConfigKey::StylusAftWeakSizeThresholdMm:
            m_tracker.m_stylusAftWeakSizeThresholdMm = ParseConfigFloat(key, value);
            needsStylusSync = true;
            break;
        case TouchConfigKey::StylusSuppressGlobalEnabled:
            m_tracker.m_stylusSuppressGlobalEnabled = toBool(value);
            needsStylusSync = true;
            break;
        case TouchConfigKey::StylusSuppressLocalDistance:
            m_tracker.m_stylusSuppressLocalDistance = ParseConfigFloat(key, value);
            needsStylusSync = true;
            break;
        case TouchConfigKey::StylusSuppressLocalEnabled:
            m_tracker.m_stylusSuppressLocalEnabled = toBool(value);
            needsStylusSync = true;
            break;
        case TouchConfigKey::StylusSuppressPenPeakThreshold:
            m_tracker.m_stylusSuppressPenPeakThreshold = ParseConfigInt(key, value);
            needsStylusSync = true;
            break;
        case TouchConfigKey::StylusSuppressTouchAreaKeep:
            m_tracker.m_stylusSuppressTouchAreaKeep = ParseConfigInt(key, value);
            needsStylusSync = true;
            break;
        case TouchConfigKey::StylusSuppressTouchSignalKeep:
            m_tracker.m_stylusSuppressTouchSignalKeep = ParseConfigInt(key, value);
            needsStylusSync = true;
            break;
        case TouchConfigKey::TouchDownDebounceFrames:
            m_tracker.m_touchDownDebounceFrames = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::TouchDownDebounceMaxExtra:
            m_tracker.m_touchDownDebounceMaxExtra = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::TouchDownEdgeRejectMinSignal:
            m_tracker.m_touchDownEdgeRejectMinSignal = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::TouchDownRejectEnabled:
            m_tracker.m_touchDownRejectEnabled = toBool(value);
            break;
        case TouchConfigKey::TouchDownRejectMinSignal:
            m_tracker.m_touchDownRejectMinSignal = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::TouchDownRejectMinSizeMm:
            m_tracker.m_touchDownRejectMinSizeMm = ParseConfigFloat(key, value);
            break;
        case TouchConfigKey::TouchDownSmallSizeThresholdMm:
            m_tracker.m_touchDownSmallSizeThresholdMm = ParseConfigFloat(key, value);
            break;
        case TouchConfigKey::TouchDownWeakSignalThreshold:
            m_tracker.m_touchDownWeakSignalThreshold = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::TrackerEnabled:
            m_tracker.m_enabled = toBool(value);
            break;
        case TouchConfigKey::UseHungarian:
            m_tracker.m_useHungarian = toBool(value);
            break;
        case TouchConfigKey::Z1FilterEnabled:
            m_peakDet.m_z1Filter = toBool(value);
            break;
        case TouchConfigKey::Z8FilterEnabled:
            m_peakDet.m_z8Filter = toBool(value);
            break;
        case TouchConfigKey::Z8Radius:
            m_peakDet.m_z8Radius = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::ZoneTholdScale:
            m_contactExtractor.m_zoneExp.m_tholdScaleNumer = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::ZoneTholdShift:
            m_contactExtractor.m_zoneExp.m_tholdScaleShift = ParseConfigInt(key, value);
            break;
        case TouchConfigKey::Unknown:
            break;
        }

        if (needsStylusSync) SyncStylusSuppressConfigFromTracker();
    } catch (const ConfigParseError& error) {
        LogConfigParseWarning("TouchPipeline", __func__, key, value, error);
    }
}
} // namespace Solvers
