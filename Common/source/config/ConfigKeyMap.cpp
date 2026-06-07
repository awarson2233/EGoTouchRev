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
    {ConfigKeyId::StylusSpIirCoefLowInBand, "stylus.sp.iir_coef_low_in_band"},
    {ConfigKeyId::StylusSpIirCoefHighInBand, "stylus.sp.iir_coef_high_in_band"},
    {ConfigKeyId::StylusSpIirSpeedTholdInBand, "stylus.sp.iir_speed_thold_in_band"},
    {ConfigKeyId::StylusSpIirCoefLowEdge, "stylus.sp.iir_coef_low_edge"},
    {ConfigKeyId::StylusSpIirCoefHighEdge, "stylus.sp.iir_coef_high_edge"},
    {ConfigKeyId::StylusSpIirSpeedTholdEdge, "stylus.sp.iir_speed_thold_edge"},
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
