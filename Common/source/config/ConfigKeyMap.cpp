#include "config/ConfigKeyMap.h"

#include "config/ConfigBinder.h"
#include "config/ConfigCatalog.h"
#include "config/ConfigStore.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Config {
namespace {

const std::pair<ConfigKeyId, std::string_view> kStaticKeyMap[] = {
    {ConfigKeyId::SvcMode, "service.mode"},
    {ConfigKeyId::SvcAutoMode, "service.auto_mode"},
    {ConfigKeyId::SvcStylusVhfEnabled, "service.stylus_vhf_enabled"},
    {ConfigKeyId::SvcPenButtonMode, "service.pen_button_mode"},
    {ConfigKeyId::SvcPenButtonRoute, "service.pen_button_route"},

    {ConfigKeyId::TouchBaselineBgAlphaShift, "touch.signal_cond.baseline_bg_alpha_shift"},
    {ConfigKeyId::TouchBaselineBgMaxStep, "touch.signal_cond.baseline_bg_max_step"},
    {ConfigKeyId::TouchBaselineNoFingerAlphaShift, "touch.signal_cond.baseline_no_finger_alpha_shift"},
    {ConfigKeyId::TouchBaselineNoFingerMaxStep, "touch.signal_cond.baseline_no_finger_max_step"},
    {ConfigKeyId::TouchBaselineRecoveryAlphaShift, "touch.signal_cond.baseline_recovery_alpha_shift"},
    {ConfigKeyId::TouchBaselineRecoveryMaxFrames, "touch.signal_cond.baseline_recovery_max_frames"},
    {ConfigKeyId::TouchBaselineRecoveryMaxStep, "touch.signal_cond.baseline_recovery_max_step"},
    {ConfigKeyId::TouchFrameParserEnabled, "touch.frame_parser.enabled"},
    {ConfigKeyId::TouchBaselineEnabled, "touch.signal_cond.baseline_enabled"},
    {ConfigKeyId::TouchBaselineValue, "touch.signal_cond.baseline_value"},
    {ConfigKeyId::TouchBaselineNoiseDeadband, "touch.signal_cond.baseline_noise_deadband"},
    {ConfigKeyId::TouchBaselinePositiveDeadband, "touch.signal_cond.baseline_positive_deadband"},
    {ConfigKeyId::TouchBaselineNegativeDeadband, "touch.signal_cond.baseline_negative_deadband"},
    {ConfigKeyId::TouchBaselinePeakThreshold, "touch.signal_cond.baseline_peak_threshold"},
    {ConfigKeyId::TouchBaselineReleaseHoldFrames, "touch.signal_cond.baseline_release_hold_frames"},
    {ConfigKeyId::TouchBaselinePositiveAlphaShift, "touch.signal_cond.baseline_positive_alpha_shift"},
    {ConfigKeyId::TouchBaselineNegativeAlphaShift, "touch.signal_cond.baseline_negative_alpha_shift"},
    {ConfigKeyId::TouchBaselineNoiseAlphaShift, "touch.signal_cond.baseline_noise_alpha_shift"},
    {ConfigKeyId::TouchBaselinePositiveMaxStep, "touch.signal_cond.baseline_positive_max_step"},
    {ConfigKeyId::TouchBaselineNegativeMaxStep, "touch.signal_cond.baseline_negative_max_step"},
    {ConfigKeyId::TouchBaselineNoiseTrackingEnabled, "touch.signal_cond.baseline_noise_tracking_enabled"},
    {ConfigKeyId::TouchCmfEnabled, "touch.signal_cond.cmf_enabled"},
    {ConfigKeyId::TouchCmfExclusionThreshold, "touch.signal_cond.cmf_exclusion_threshold"},
    {ConfigKeyId::TouchCmfMaxCorrection, "touch.signal_cond.cmf_max_correction"},
    {ConfigKeyId::TouchPeakDetectionThreshold, "touch.peak_detection.threshold"},
    {ConfigKeyId::TouchPeakDetectionMaxPeaks, "touch.peak_detection.max_peaks"},
    {ConfigKeyId::TouchPeakDetectionLocalMaxRadius, "touch.peak_detection.local_max_radius"},
    {ConfigKeyId::TouchPeakDetectionEdgeThresholdEnabled, "touch.peak_detection.edge_threshold_enabled"},
    {ConfigKeyId::TouchPeakDetectionEdgeThreshold, "touch.peak_detection.edge_threshold"},
    {ConfigKeyId::TouchPeakDetectionZ8FilterEnabled, "touch.peak_detection.z8_filter_enabled"},
    {ConfigKeyId::TouchPeakDetectionZ1FilterEnabled, "touch.peak_detection.z1_filter_enabled"},
    {ConfigKeyId::TouchPeakDetectionClosePeakSaddleFilterEnabled, "touch.peak_detection.close_peak_saddle_filter_enabled"},
    {ConfigKeyId::TouchPeakDetectionClosePeakRadius, "touch.peak_detection.close_peak_radius"},
    {ConfigKeyId::TouchPeakDetectionMacroZoneMinArea, "touch.peak_detection.macro_zone_min_area"},
    {ConfigKeyId::TouchClassifierEnabled, "touch.classifier.enabled"},
    {ConfigKeyId::TouchClassifierAnalyzerEnabled, "touch.classifier.analyzer_enabled"},
    {ConfigKeyId::TouchClassifierPeakEvalEnabled, "touch.classifier.peak_eval_enabled"},
    {ConfigKeyId::TouchClassifierAreaThreshold, "touch.classifier.area_threshold"},
    {ConfigKeyId::TouchClassifierSignalSumThreshold, "touch.classifier.signal_sum_threshold"},
    {ConfigKeyId::TouchClassifierFingerProminence, "touch.classifier.finger_prominence"},
    {ConfigKeyId::TouchClassifierFingerSharpness, "touch.classifier.finger_sharpness"},
    {ConfigKeyId::TouchClassifierPalmSharpnessMax, "touch.classifier.palm_sharpness_max"},
    {ConfigKeyId::TouchClassifierPalmAwareExpansionEnabled, "touch.classifier.palm_aware_expansion_enabled"},
    {ConfigKeyId::TouchClassifierFingerInPalmThresholdRatio, "touch.classifier.finger_in_palm_threshold_ratio"},
    {ConfigKeyId::TouchClassifierFingerInPalmMaxRadius, "touch.classifier.finger_in_palm_max_radius"},
    {ConfigKeyId::TouchZoneContactThresholdScaleNumer, "touch.zone_contact.threshold_scale_numer"},
    {ConfigKeyId::TouchZoneContactThresholdScaleShift, "touch.zone_contact.threshold_scale_shift"},
    {ConfigKeyId::TouchZoneContactDilateErodeEnabled, "touch.zone_contact.dilate_erode_enabled"},
    {ConfigKeyId::TouchZoneContactMaxTouches, "touch.zone_contact.max_touches"},
    {ConfigKeyId::TouchZoneContactEdgeWidthThreshold, "touch.zone_contact.edge_width_threshold"},
    {ConfigKeyId::TouchZoneContactTouchSizePixelPitchMm, "touch.zone_contact.touch_size_pixel_pitch_mm"},
    {ConfigKeyId::TouchZoneContactTouchSizeUnitPerSigMm2, "touch.zone_contact.touch_size_unit_per_sig_mm2"},
    {ConfigKeyId::TouchEdgeEnabled, "touch.edge.enabled"},
    {ConfigKeyId::TouchEdgeCompStrength, "touch.edge.comp_strength"},
    {ConfigKeyId::TouchEdgeFullCompRange, "touch.edge.full_comp_range"},
    {ConfigKeyId::TouchEdgeBlendRange, "touch.edge.blend_range"},
    {ConfigKeyId::TouchEdgeRejectEnabled, "touch.edge.reject_enabled"},
    {ConfigKeyId::TouchEdgeRejectMargin, "touch.edge.reject_margin"},
    {ConfigKeyId::TouchTrackingEnabled, "touch.tracking.enabled"},
    {ConfigKeyId::TouchTrackingMaxTouchCount, "touch.tracking.max_touch_count"},
    {ConfigKeyId::TouchTrackingMaxTrackDistance, "touch.tracking.max_track_distance"},
    {ConfigKeyId::TouchTrackingAlwaysMatchDistance, "touch.tracking.always_match_distance"},
    {ConfigKeyId::TouchTrackingGapRelinkEnabled, "touch.tracking.gap_relink_enabled"},
    {ConfigKeyId::TouchTrackingGapRelinkWindowFrames, "touch.tracking.gap_relink_window_frames"},
    {ConfigKeyId::TouchTrackingTouchDownDebounceFrames, "touch.tracking.touch_down_debounce_frames"},
    {ConfigKeyId::TouchTrackingDynamicDebounceEnabled, "touch.tracking.dynamic_debounce_enabled"},
    {ConfigKeyId::TouchTrackingUseHungarian, "touch.tracking.use_hungarian"},
    {ConfigKeyId::TouchStylusSuppressGlobalEnabled, "touch.stylus_suppress.global_enabled"},
    {ConfigKeyId::TouchStylusSuppressLocalEnabled, "touch.stylus_suppress.local_enabled"},
    {ConfigKeyId::TouchStylusSuppressLocalDistance, "touch.stylus_suppress.local_distance"},
    {ConfigKeyId::TouchStylusSuppressPenPeakThreshold, "touch.stylus_suppress.pen_peak_threshold"},
    {ConfigKeyId::TouchStylusSuppressAftEnabled, "touch.stylus_suppress.aft_enabled"},
    {ConfigKeyId::TouchStylusSuppressAftRecentFrames, "touch.stylus_suppress.aft_recent_frames"},
    {ConfigKeyId::TouchStylusSuppressAftRadius, "touch.stylus_suppress.aft_radius"},
    {ConfigKeyId::TouchCoordFilterEnabled, "touch.coord_filter.enabled"},
    {ConfigKeyId::TouchCoordFilterMinCutoff, "touch.coord_filter.min_cutoff"},
    {ConfigKeyId::TouchCoordFilterBeta, "touch.coord_filter.beta"},
    {ConfigKeyId::TouchCoordFilterDCutoff, "touch.coord_filter.d_cutoff"},
    {ConfigKeyId::TouchGestureEnabled, "touch.gesture.enabled"},
    {ConfigKeyId::TouchGesturePressCandidateFrames, "touch.gesture.press_candidate_frames"},
    {ConfigKeyId::TouchGestureDragThreshold, "touch.gesture.drag_threshold"},
    {ConfigKeyId::TouchGestureLongPressFrames, "touch.gesture.long_press_frames"},
    {ConfigKeyId::TouchGestureReleasePendingFrames, "touch.gesture.release_pending_frames"},
    {ConfigKeyId::TouchGestureBypassStateMachine, "touch.gesture.bypass_state_machine"},

    {ConfigKeyId::StylusSpFrameParserEnabled, "stylus.sp.frame_parser_enabled"},
    {ConfigKeyId::StylusSpPeakDetectorEnabled, "stylus.sp.peak_detector_enabled"},
    {ConfigKeyId::StylusSpCoordinateSolverEnabled, "stylus.sp.coordinate_solver_enabled"},
    {ConfigKeyId::StylusSpTiltProcessEnabled, "stylus.sp.tilt_process_enabled"},
    {ConfigKeyId::StylusSpPressureSolverEnabled, "stylus.sp.pressure_solver_enabled"},
    {ConfigKeyId::StylusSpPostPressureEnabled, "stylus.sp.post_pressure_enabled"},
    {ConfigKeyId::StylusSpFakePressureDecreaseEnabled, "stylus.sp.fake_pressure_decrease_enabled"},
    {ConfigKeyId::StylusSpBtFreqShiftDebounceFrames, "stylus.sp.bt_freq_shift_debounce_frames"},
    {ConfigKeyId::StylusSpPressureEdgeEnterThreshold, "stylus.sp.pressure_edge_enter_threshold"},
    {ConfigKeyId::StylusSpPressureEdgeExitThreshold, "stylus.sp.pressure_edge_exit_threshold"},
    {ConfigKeyId::StylusSpTipDownPressureThreshold, "stylus.sp.tip_down_pressure_threshold"},
    {ConfigKeyId::StylusSpBtPressSuppressEnterThreshold, "stylus.sp.bt_press_signal_suppress_enter_threshold"},
    {ConfigKeyId::StylusSpBtPressSuppressExitThreshold, "stylus.sp.bt_press_signal_suppress_exit_threshold"},
    {ConfigKeyId::StylusSpSignalFloor, "stylus.sp.signal_floor"},
    {ConfigKeyId::StylusSpEdgeCoorEnabled, "stylus.sp.edge_coor_enabled"},
    {ConfigKeyId::StylusSpEdgeCoorPostEnabled, "stylus.sp.edge_coor_post_enabled"},
    {ConfigKeyId::StylusSpNoisePostEnabled, "stylus.sp.noise_post_enabled"},
    {ConfigKeyId::StylusSpNoiseSignalRatioThold, "stylus.sp.noise_signal_ratio_thold"},
    {ConfigKeyId::StylusSpNoiseSignalDropRatio, "stylus.sp.noise_signal_drop_ratio"},
    {ConfigKeyId::StylusSpLinearFilterEnabled, "stylus.sp.linear_filter_enabled"},
    {ConfigKeyId::StylusSpCoorReviseEnabled, "stylus.sp.coor_revise_enabled"},
    {ConfigKeyId::StylusSpCoorReviseFactorDim1, "stylus.sp.coor_revise_factor_dim1"},
    {ConfigKeyId::StylusSpCoorReviseFactorDim2, "stylus.sp.coor_revise_factor_dim2"},
    {ConfigKeyId::StylusSpCoorSpeedEnabled, "stylus.sp.coor_speed_enabled"},
    {ConfigKeyId::StylusSpIirFilterEnabled, "stylus.sp.iir_filter_enabled"},
    {ConfigKeyId::StylusSpIirCoefLowHover, "stylus.sp.iir_coef_low_hover"},
    {ConfigKeyId::StylusSpIirCoefHighHover, "stylus.sp.iir_coef_high_hover"},
    {ConfigKeyId::StylusSpIirSpeedTholdHover, "stylus.sp.iir_speed_thold_hover"},
    {ConfigKeyId::StylusSpIirCoefLowWriting, "stylus.sp.iir_coef_low_writing"},
    {ConfigKeyId::StylusSpIirCoefHighWriting, "stylus.sp.iir_coef_high_writing"},
    {ConfigKeyId::StylusSpIirSpeedTholdWriting, "stylus.sp.iir_speed_thold_writing"},
    {ConfigKeyId::StylusSpIirSpeedMax, "stylus.sp.iir_speed_max"},
    {ConfigKeyId::StylusSpIirMaxCoef, "stylus.sp.iir_max_coef"},
    {ConfigKeyId::StylusSpAftCoorEnabled, "stylus.sp.aft_coor_enabled"},
    {ConfigKeyId::StylusSpLockFlashInBandX, "stylus.sp.lock_flash_in_band_x"},
    {ConfigKeyId::StylusSpLockFlashInBandY, "stylus.sp.lock_flash_in_band_y"},
    {ConfigKeyId::StylusSpLockFlashEdgeX, "stylus.sp.lock_flash_edge_x"},
    {ConfigKeyId::StylusSpLockFlashEdgeY, "stylus.sp.lock_flash_edge_y"},
    {ConfigKeyId::StylusSpLockSensorTxCount, "stylus.sp.lock_sensor_tx_count"},
    {ConfigKeyId::StylusSpLockSensorRxCount, "stylus.sp.lock_sensor_rx_count"},
    {ConfigKeyId::StylusSpLockBypass, "stylus.sp.lock_bypass"},
};



std::unordered_map<ConfigKeyId, std::string>& mutableKeyIdToPath()
{
    static std::unordered_map<ConfigKeyId, std::string> map;
    return map;
}

std::unordered_map<std::string, ConfigKeyId>& mutablePathToKeyId()
{
    static std::unordered_map<std::string, ConfigKeyId> map;
    return map;
}

void ensureStaticMapsInitialized()
{
    static const bool initialized = [] {
        auto& idToPath = mutableKeyIdToPath();
        auto& pathToId = mutablePathToKeyId();
        for (const auto& [id, pathView] : kStaticKeyMap) {
            std::string path{pathView};
            idToPath.try_emplace(id, path);
            pathToId.try_emplace(std::move(path), id);
        }

        return true;
    }();
    (void)initialized;
}

} // namespace

const std::unordered_map<ConfigKeyId, std::string>& keyIdToPath()
{
    ensureStaticMapsInitialized();
    return mutableKeyIdToPath();
}

const std::unordered_map<std::string, ConfigKeyId>& pathToKeyId()
{
    ensureStaticMapsInitialized();
    return mutablePathToKeyId();
}

void registerKeyMapping(ConfigKeyId id, std::string_view yamlPath)
{
    ensureStaticMapsInitialized();

    auto& idToPath = mutableKeyIdToPath();
    auto& pathToId = mutablePathToKeyId();

    if (const auto existing = idToPath.find(id); existing != idToPath.end()) {
        pathToId.erase(existing->second);
    }

    std::string path{yamlPath};
    idToPath[id] = path;
    pathToId[std::move(path)] = id;
}

void registerRuntimeKeyMappings(const ConfigBinder& binder)
{
    ensureStaticMapsInitialized();

    // Compatibility entry point only: keyId is now a fixed IPC contract and must
    // not be allocated from runtime paths. Binding snapshots/catalogs surface
    // unknown paths as ConfigKeyId::MaxKeyId for validation or logging.
    (void)binder;
}

std::optional<std::string_view> tryPathForKeyId(ConfigKeyId id)
{
    ensureStaticMapsInitialized();
    const auto& idToPath = mutableKeyIdToPath();
    if (const auto it = idToPath.find(id); it != idToPath.end()) {
        return std::string_view{it->second};
    }
    return std::nullopt;
}

std::optional<ConfigKeyId> tryKeyIdForPath(std::string_view yamlPath)
{
    ensureStaticMapsInitialized();
    const auto& pathToId = mutablePathToKeyId();
    if (const auto it = pathToId.find(std::string{yamlPath}); it != pathToId.end()) {
        return it->second;
    }
    return std::nullopt;
}

ConfigSchemaSnapshot BuildMergedSchema(const ConfigStore& defaults, const ConfigBinder& binder)
{
    auto result = BuildSchemaSnapshot(BuildConfigCatalog(defaults, binder));
    const auto binderSnapshot = binder.snapshot();
    std::unordered_map<std::string, ConfigValue> currentByPath;
    currentByPath.reserve(binderSnapshot.entries.size());
    for (const auto& entry : binderSnapshot.entries) {
        currentByPath.emplace(entry.yamlPath, entry.currentValue);
    }
    for (auto& entry : result.entries) {
        if (const auto it = currentByPath.find(entry.yamlPath); it != currentByPath.end()) {
            entry.currentValue = it->second;
        }
    }
    return result;
}

} // namespace Config
