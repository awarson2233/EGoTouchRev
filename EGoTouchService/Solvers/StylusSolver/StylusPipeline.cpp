#include "StylusPipeline.h"
#include "config/ConfigBinder.h"
#include "config/ConfigStore.h"

#include <algorithm>
#include <ostream>

namespace Solvers {

void StylusPipeline::registerBindings(Config::ConfigBinder& binder) {
    using Config::ConfigRange;

    // ── HPP2 ──
    binder.bindSchema("stylus.hpp2.enabled", Config::ConfigValue(true), "bool", {}, "HPP2 pipeline enable");
    binder.bindSchema("stylus.hpp2.sensor_tx_count", Config::ConfigValue(static_cast<int32_t>(60)), "int", ConfigRange{1.0, 100.0}, "HPP2 sensor TX count");
    binder.bindSchema("stylus.hpp2.sensor_rx_count", Config::ConfigValue(static_cast<int32_t>(40)), "int", ConfigRange{1.0, 100.0}, "HPP2 sensor RX count");
    binder.bindSchema("stylus.hpp2.cmf_window_radius", Config::ConfigValue(static_cast<int32_t>(6)), "int", ConfigRange{0.0, 32.0}, "HPP2 CMF window radius");
    // HPP2 runtime configuration is intentionally disabled; keep keys as schema-only compatibility metadata.
    binder.bindSchema("stylus.hpp2.raw_abnormal_line_sum_threshold", Config::ConfigValue(static_cast<int32_t>(30000)), "int", ConfigRange{0.0, 2147483647.0}, "HPP2 raw abnormal line sum threshold");
    binder.bindSchema("stylus.hpp2.raw_abnormal_energy_ratio_threshold", Config::ConfigValue(static_cast<int32_t>(200)), "int", ConfigRange{0.0, 65535.0}, "HPP2 raw abnormal energy ratio threshold");
    binder.bindSchema("stylus.hpp2.cmn_abnormal_sum_threshold", Config::ConfigValue(static_cast<int32_t>(9000)), "int", ConfigRange{0.0, 2147483647.0}, "HPP2 CMN abnormal sum threshold");
    binder.bindSchema("stylus.hpp2.cmn_abnormal_min_threshold", Config::ConfigValue(static_cast<int32_t>(2500)), "int", ConfigRange{0.0, 65535.0}, "HPP2 CMN abnormal min threshold");
    binder.bindSchema("stylus.hpp2.charger_noise_clear_floor", Config::ConfigValue(static_cast<int32_t>(20)), "int", ConfigRange{1.0, 65535.0}, "HPP2 charger noise clear floor");
    binder.bindSchema("stylus.hpp2.charger_noise_ratio_threshold", Config::ConfigValue(static_cast<int32_t>(299)), "int", ConfigRange{0.0, 65535.0}, "HPP2 charger noise ratio threshold");
    binder.bindSchema("stylus.hpp2.charger_noise_sum_threshold", Config::ConfigValue(static_cast<int32_t>(400)), "int", ConfigRange{0.0, 2147483647.0}, "HPP2 charger noise sum threshold");
    binder.bindSchema("stylus.hpp2.charger_noise_max_sample_threshold", Config::ConfigValue(static_cast<int32_t>(200)), "int", ConfigRange{0.0, 65535.0}, "HPP2 charger noise max sample threshold");
    binder.bindSchema("stylus.hpp2.charger_noise_abnormal_channel_threshold", Config::ConfigValue(static_cast<int32_t>(2)), "int", ConfigRange{0.0, 255.0}, "HPP2 charger noise abnormal channel threshold");
    binder.bindSchema("stylus.hpp2.charger_noise_peak_protect_radius", Config::ConfigValue(static_cast<int32_t>(2)), "int", ConfigRange{0.0, 65535.0}, "HPP2 charger noise peak protect radius");
    binder.bindSchema("stylus.hpp2.charger_noise_min_raw_sample", Config::ConfigValue(static_cast<int32_t>(50)), "int", ConfigRange{0.0, 65535.0}, "HPP2 charger noise min raw sample");
    binder.bindSchema("stylus.hpp2.peak_signal_floor", Config::ConfigValue(static_cast<int32_t>(250)), "int", ConfigRange{0.0, 65535.0}, "HPP2 peak signal floor");
    binder.bindSchema("stylus.hpp2.peak_search_neighbor_dist", Config::ConfigValue(static_cast<int32_t>(2)), "int", ConfigRange{1.0, 16.0}, "HPP2 peak search neighbor distance");
    binder.bindSchema("stylus.hpp2.peak_min_width", Config::ConfigValue(static_cast<int32_t>(2)), "int", ConfigRange{1.0, 100.0}, "HPP2 peak min width");
    binder.bindSchema("stylus.hpp2.peak_max_width", Config::ConfigValue(static_cast<int32_t>(20)), "int", ConfigRange{1.0, 100.0}, "HPP2 peak max width");
    binder.bindSchema("stylus.hpp2.pressure_edge_enter_threshold", Config::ConfigValue(static_cast<int32_t>(1500)), "int", ConfigRange{0.0, 65535.0}, "HPP2 pressure edge enter threshold");
    binder.bindSchema("stylus.hpp2.pressure_edge_exit_threshold", Config::ConfigValue(static_cast<int32_t>(3000)), "int", ConfigRange{0.0, 65535.0}, "HPP2 pressure edge exit threshold");
    binder.bindSchema("stylus.hpp2.pressure_delta_normal", Config::ConfigValue(static_cast<int32_t>(1024)), "int", ConfigRange{0.0, 65535.0}, "HPP2 pressure delta normal");
    binder.bindSchema("stylus.hpp2.pressure_delta_tight", Config::ConfigValue(static_cast<int32_t>(64)), "int", ConfigRange{0.0, 65535.0}, "HPP2 pressure delta tight");
    binder.bindSchema("stylus.hpp2.use_tight_pressure_delta", Config::ConfigValue(false), "bool", {}, "HPP2 use tight pressure delta");
    // ── Frame Parser ──
    binder.bind("stylus.sp.frame_parser_enabled", &Stylus::StylusFrameParser::m_enabled, m_frameParser,
                true, {}, "Frame parser enable");

    // ── Data Solve / HPP3 stages ──
    binder.bind("stylus.sp.peak_detector_enabled", &Stylus::Hpp3::GridFeatureExtractor::m_enabled,
                m_hpp3.m_featureExtractor, true, {}, "HPP3 peak detector enable");
    binder.bind("stylus.sp.coordinate_solver_enabled", &Stylus::Hpp3::CoordinateSolver::m_enabled,
                m_hpp3.m_coordinateSolver, true, {}, "HPP3 coordinate solver enable");
    binder.bind("stylus.sp.tilt_process_enabled", &Stylus::Hpp3::TiltProcess::m_enabled,
                m_hpp3.m_tiltProcess, true, {}, "HPP3 tilt process enable");
    binder.bind("stylus.sp.pressure_solver_enabled", &Stylus::Hpp3::PressureSolver::m_enabled,
                m_hpp3.m_pressureSolver, true, {}, "HPP3 pressure solver enable");

    // ── Pressure / HPP3 post ──
    binder.bind("stylus.sp.post_pressure_enabled", &Stylus::Hpp3::Hpp3PostPressureProcess::m_enabled,
                m_hpp3.m_postPressure, true, {}, "HPP3 post pressure enable");
    binder.bind("stylus.sp.fake_pressure_decrease_enabled", &Stylus::Hpp3::Hpp3PostPressureProcess::m_fakePressureDecreaseEnabled,
                m_hpp3.m_postPressure, false, {}, "HPP3 fake pressure decrease enable");
    binder.bind("stylus.sp.bt_freq_shift_debounce_frames", &Stylus::Hpp3::Hpp3PostPressureProcess::m_btFreqShiftDebounceFrames,
                m_hpp3.m_postPressure, static_cast<int32_t>(2), ConfigRange{0.0, 255.0}, "BT frequency shift debounce frames");
    binder.bindSchema("stylus.sp.pressure_edge_enter_threshold", Config::ConfigValue(static_cast<int32_t>(1500)), "int", ConfigRange{0.0, 65535.0}, "HPP3 pressure edge enter threshold", Config::ConfigRuntimeBinding::ManualLiveApply);
    binder.bindSchema("stylus.sp.pressure_edge_exit_threshold", Config::ConfigValue(static_cast<int32_t>(3000)), "int", ConfigRange{0.0, 65535.0}, "HPP3 pressure edge exit threshold", Config::ConfigRuntimeBinding::ManualLiveApply);
    binder.bindSchema("stylus.sp.tip_down_pressure_threshold", Config::ConfigValue(static_cast<int32_t>(1)), "int", ConfigRange{0.0, 4095.0}, "HPP3 tip-down pressure threshold", Config::ConfigRuntimeBinding::ManualLiveApply);
    binder.bindSchema("stylus.sp.bt_press_signal_suppress_enter_threshold", Config::ConfigValue(static_cast<int32_t>(2200)), "int", ConfigRange{0.0, 65535.0}, "HPP3 BT pressure signal suppress enter threshold", Config::ConfigRuntimeBinding::ManualLiveApply);
    binder.bindSchema("stylus.sp.bt_press_signal_suppress_exit_threshold", Config::ConfigValue(static_cast<int32_t>(3200)), "int", ConfigRange{0.0, 65535.0}, "HPP3 BT pressure signal suppress exit threshold", Config::ConfigRuntimeBinding::ManualLiveApply);
    binder.bindSchema("stylus.sp.signal_floor", Config::ConfigValue(static_cast<int32_t>(64)), "int", ConfigRange{0.0, 65535.0}, "HPP3 coordinate signal floor", Config::ConfigRuntimeBinding::ManualLiveApply);

    // ── Coordinate ──
    binder.bind("stylus.sp.edge_coor_enabled", &Stylus::EdgeCoorProcess::m_enabled, m_edgeCoorProcess,
                true, {}, "Edge coordinate process enable");
    binder.bind("stylus.sp.edge_coor_post_enabled", &Stylus::EdgeCoorPostProcess::m_enabled, m_edgeCoorPostProcess,
                true, {}, "Edge coordinate post process enable");

    // ── Noise Post ──
    binder.bind("stylus.sp.noise_post_enabled", &Stylus::Hpp3::Hpp3NoisePostProcess::m_enabled,
                m_hpp3.m_noisePostProcess, true, {}, "HPP3 noise post process enable");
    binder.bindSchema("stylus.sp.noise_signal_ratio_thold", Config::ConfigValue(static_cast<int32_t>(5)), "int", ConfigRange{1.0, 16.0}, "HPP3 noise signal ratio threshold", Config::ConfigRuntimeBinding::ManualLiveApply);
    binder.bindSchema("stylus.sp.noise_signal_drop_ratio", Config::ConfigValue(static_cast<int32_t>(5)), "int", ConfigRange{1.0, 16.0}, "HPP3 noise signal drop ratio", Config::ConfigRuntimeBinding::ManualLiveApply);

    // ── Common Post ──
    binder.bind("stylus.sp.linear_filter_enabled", &Stylus::LinearFilterProcess::m_enabled,
                m_commonPost.m_linearFilterProcess, true, {}, "Linear filter enable");
    binder.bind("stylus.sp.coor_revise_enabled", &Stylus::CoorReviseProcess::m_enabled,
                m_commonPost.m_coorReviseProcess, true, {}, "Coordinate revise enable");
    binder.bind("stylus.sp.coor_revise_factor_dim1", &Stylus::CoorReviseProcess::m_factorDim1,
                m_commonPost.m_coorReviseProcess, static_cast<int32_t>(5), ConfigRange{0.0, 255.0}, "Coordinate revise factor dim1");
    binder.bind("stylus.sp.coor_revise_factor_dim2", &Stylus::CoorReviseProcess::m_factorDim2,
                m_commonPost.m_coorReviseProcess, static_cast<int32_t>(5), ConfigRange{0.0, 255.0}, "Coordinate revise factor dim2");
    binder.bind("stylus.sp.coor_speed_enabled", &Stylus::CoorSpeedProcess::m_enabled,
                m_commonPost.m_coorSpeedProcess, true, {}, "Coordinate speed process enable");
    binder.bind("stylus.sp.iir_filter_enabled", &Stylus::CoorIIRProcess::m_enabled,
                m_commonPost.m_coorIIRProcess, true, {}, "IIR coordinate filter enable");
    binder.bindSchema("stylus.sp.iir_coef_low_in_band", Config::ConfigValue(static_cast<int32_t>(2)), "int", ConfigRange{0.0, 255.0}, "IIR low coefficient in band", Config::ConfigRuntimeBinding::ManualLiveApply);
    binder.bindSchema("stylus.sp.iir_coef_high_in_band", Config::ConfigValue(static_cast<int32_t>(16)), "int", ConfigRange{0.0, 255.0}, "IIR high coefficient in band", Config::ConfigRuntimeBinding::ManualLiveApply);
    binder.bindSchema("stylus.sp.iir_speed_thold_in_band", Config::ConfigValue(static_cast<int32_t>(20)), "int", ConfigRange{0.0, 255.0}, "IIR speed threshold in band", Config::ConfigRuntimeBinding::ManualLiveApply);
    binder.bindSchema("stylus.sp.iir_coef_low_edge", Config::ConfigValue(static_cast<int32_t>(6)), "int", ConfigRange{0.0, 255.0}, "IIR low coefficient at edge", Config::ConfigRuntimeBinding::ManualLiveApply);
    binder.bindSchema("stylus.sp.iir_coef_high_edge", Config::ConfigValue(static_cast<int32_t>(18)), "int", ConfigRange{0.0, 255.0}, "IIR high coefficient at edge", Config::ConfigRuntimeBinding::ManualLiveApply);
    binder.bindSchema("stylus.sp.iir_speed_thold_edge", Config::ConfigValue(static_cast<int32_t>(10)), "int", ConfigRange{0.0, 255.0}, "IIR speed threshold at edge", Config::ConfigRuntimeBinding::ManualLiveApply);
    binder.bind("stylus.sp.iir_speed_max", &Stylus::CoorIIRProcess::m_speedMax,
                m_commonPost.m_coorIIRProcess, static_cast<int32_t>(205), ConfigRange{0.0, 1000.0}, "IIR speed max");
    binder.bindSchema("stylus.sp.iir_max_coef", Config::ConfigValue(static_cast<int32_t>(32)), "int", ConfigRange{1.0, 255.0}, "IIR maximum coefficient denominator", Config::ConfigRuntimeBinding::ManualLiveApply);

    // ── AFT Coor ──
    binder.bind("stylus.sp.aft_coor_enabled", &Stylus::AftCoorProcess::m_enabled,
                m_commonPost.m_aftCoorProcess, true, {}, "AFT coordinate process enable");
    binder.bindSchema("stylus.sp.lock_flash_in_band_x", Config::ConfigValue(static_cast<int32_t>(0)), "int", ConfigRange{0.0, 255.0}, "AFT lock flash in-band X", Config::ConfigRuntimeBinding::ManualLiveApply);
    binder.bindSchema("stylus.sp.lock_flash_in_band_y", Config::ConfigValue(static_cast<int32_t>(0)), "int", ConfigRange{0.0, 255.0}, "AFT lock flash in-band Y", Config::ConfigRuntimeBinding::ManualLiveApply);
    binder.bindSchema("stylus.sp.lock_flash_edge_x", Config::ConfigValue(static_cast<int32_t>(1)), "int", ConfigRange{0.0, 255.0}, "AFT lock flash edge X", Config::ConfigRuntimeBinding::ManualLiveApply);
    binder.bindSchema("stylus.sp.lock_flash_edge_y", Config::ConfigValue(static_cast<int32_t>(2)), "int", ConfigRange{0.0, 255.0}, "AFT lock flash edge Y", Config::ConfigRuntimeBinding::ManualLiveApply);
    binder.bind("stylus.sp.lock_sensor_tx_count", &Stylus::AftCoorProcess::m_sensorTxCount,
                m_commonPost.m_aftCoorProcess, static_cast<int32_t>(60), ConfigRange{1.0, 200.0}, "AFT lock sensor TX count");
    binder.bind("stylus.sp.lock_sensor_rx_count", &Stylus::AftCoorProcess::m_sensorRxCount,
                m_commonPost.m_aftCoorProcess, static_cast<int32_t>(40), ConfigRange{1.0, 200.0}, "AFT lock sensor RX count");
    binder.bind("stylus.sp.lock_bypass", &Stylus::AftCoorProcess::m_bypassLock,
                m_commonPost.m_aftCoorProcess, false, {}, "AFT lock bypass");
}

void StylusPipeline::applyConfig(const Config::ConfigStore& store) {
    m_frameParser.m_enabled = store.getOr<bool>("stylus.sp.frame_parser_enabled", true);
    m_hpp3.m_featureExtractor.m_enabled = store.getOr<bool>("stylus.sp.peak_detector_enabled", true);
    m_hpp3.m_coordinateSolver.m_enabled = store.getOr<bool>("stylus.sp.coordinate_solver_enabled", true);

    const bool tiltEnabled = store.getOr<bool>("stylus.sp.tilt_process_enabled", true);
    m_hpp3.m_tiltProcess.m_enabled = tiltEnabled;
    if (!tiltEnabled) { m_hpp3.m_tiltProcess.Reset(); }

    m_hpp3.m_pressureSolver.m_enabled = store.getOr<bool>("stylus.sp.pressure_solver_enabled", true);

    const bool postPressureEnabled = store.getOr<bool>("stylus.sp.post_pressure_enabled", true);
    m_hpp3.m_postPressure.m_enabled = postPressureEnabled;
    m_hpp3.m_postPressure.m_fakePressureDecreaseEnabled = store.getOr<bool>("stylus.sp.fake_pressure_decrease_enabled", false);
    m_hpp3.m_postPressure.m_btFreqShiftDebounceFrames = store.getOr<int32_t>("stylus.sp.bt_freq_shift_debounce_frames", 2);
    m_hpp3.m_postPressure.m_pressureEdgeEnterThreshold = static_cast<uint16_t>(store.getOr<int32_t>("stylus.sp.pressure_edge_enter_threshold", 1500));
    m_hpp3.m_postPressure.m_pressureEdgeExitThreshold = static_cast<uint16_t>(store.getOr<int32_t>("stylus.sp.pressure_edge_exit_threshold", 3000));
    if (!postPressureEnabled) { m_hpp3.m_postPressure.Reset(); }

    m_hpp3.m_pressureSolver.m_tipDownPressureThreshold = static_cast<uint16_t>(store.getOr<int32_t>("stylus.sp.tip_down_pressure_threshold", 1));
    m_hpp3.m_pressureSolver.m_btPressSignalSuppressEnterThreshold = static_cast<uint16_t>(store.getOr<int32_t>("stylus.sp.bt_press_signal_suppress_enter_threshold", 2200));
    m_hpp3.m_pressureSolver.m_btPressSignalSuppressExitThreshold = static_cast<uint16_t>(store.getOr<int32_t>("stylus.sp.bt_press_signal_suppress_exit_threshold", 3200));
    m_hpp3.m_coordinateSolver.m_signalFloor = static_cast<uint16_t>(store.getOr<int32_t>("stylus.sp.signal_floor", 64));

    const bool edgeCoorEnabled = store.getOr<bool>("stylus.sp.edge_coor_enabled", true);
    m_edgeCoorProcess.m_enabled = edgeCoorEnabled;
    if (!edgeCoorEnabled) { m_edgeCoorProcess.Reset(); }
    m_edgeCoorPostProcess.m_enabled = store.getOr<bool>("stylus.sp.edge_coor_post_enabled", true);

    const bool noisePostEnabled = store.getOr<bool>("stylus.sp.noise_post_enabled", true);
    m_hpp3.m_noisePostProcess.m_enabled = noisePostEnabled;
    m_hpp3.m_noisePostProcess.m_signalRatioThreshold = static_cast<uint8_t>(store.getOr<int32_t>("stylus.sp.noise_signal_ratio_thold", 5));
    m_hpp3.m_noisePostProcess.m_signalMagnitudeDropRatio = static_cast<uint8_t>(store.getOr<int32_t>("stylus.sp.noise_signal_drop_ratio", 5));
    if (!noisePostEnabled) { m_hpp3.m_noisePostProcess.Reset(); }

    const bool linearFilterEnabled = store.getOr<bool>("stylus.sp.linear_filter_enabled", true);
    m_commonPost.m_linearFilterProcess.m_enabled = linearFilterEnabled;
    if (!linearFilterEnabled) { m_commonPost.m_linearFilterProcess.Reset(); }

    const bool coorReviseEnabled = store.getOr<bool>("stylus.sp.coor_revise_enabled", true);
    m_commonPost.m_coorReviseProcess.m_enabled = coorReviseEnabled;
    m_commonPost.m_coorReviseProcess.m_factorDim1 = store.getOr<int32_t>("stylus.sp.coor_revise_factor_dim1", 5);
    m_commonPost.m_coorReviseProcess.m_factorDim2 = store.getOr<int32_t>("stylus.sp.coor_revise_factor_dim2", 5);
    if (!coorReviseEnabled) { m_commonPost.m_coorReviseProcess.Reset(); }

    const bool coorSpeedEnabled = store.getOr<bool>("stylus.sp.coor_speed_enabled", true);
    m_commonPost.m_coorSpeedProcess.m_enabled = coorSpeedEnabled;
    if (!coorSpeedEnabled) { m_commonPost.m_coorSpeedProcess.Reset(); }

    const bool iirFilterEnabled = store.getOr<bool>("stylus.sp.iir_filter_enabled", true);
    m_commonPost.m_coorIIRProcess.m_enabled = iirFilterEnabled;
    m_commonPost.m_coorIIRProcess.m_coefLowInBand = store.getOr<int32_t>("stylus.sp.iir_coef_low_in_band", 2);
    m_commonPost.m_coorIIRProcess.m_coefHighInBand = store.getOr<int32_t>("stylus.sp.iir_coef_high_in_band", 16);
    m_commonPost.m_coorIIRProcess.m_speedTholdInBand = store.getOr<int32_t>("stylus.sp.iir_speed_thold_in_band", 20);
    m_commonPost.m_coorIIRProcess.m_coefLowEdge = store.getOr<int32_t>("stylus.sp.iir_coef_low_edge", 6);
    m_commonPost.m_coorIIRProcess.m_coefHighEdge = store.getOr<int32_t>("stylus.sp.iir_coef_high_edge", 18);
    m_commonPost.m_coorIIRProcess.m_speedTholdEdge = store.getOr<int32_t>("stylus.sp.iir_speed_thold_edge", 10);
    m_commonPost.m_coorIIRProcess.m_speedMax = store.getOr<int32_t>("stylus.sp.iir_speed_max", 205);
    m_commonPost.m_coorIIRProcess.m_maxCoef = store.getOr<int32_t>("stylus.sp.iir_max_coef", 32);
    if (!iirFilterEnabled) { m_commonPost.m_coorIIRProcess.Reset(); }

    const bool aftCoorEnabled = store.getOr<bool>("stylus.sp.aft_coor_enabled", true);
    m_commonPost.m_aftCoorProcess.m_enabled = aftCoorEnabled;
    m_commonPost.m_aftCoorProcess.m_lockFlashInBandX = static_cast<uint8_t>(store.getOr<int32_t>("stylus.sp.lock_flash_in_band_x", 0));
    m_commonPost.m_aftCoorProcess.m_lockFlashInBandY = static_cast<uint8_t>(store.getOr<int32_t>("stylus.sp.lock_flash_in_band_y", 0));
    m_commonPost.m_aftCoorProcess.m_lockFlashEdgeX = static_cast<uint8_t>(store.getOr<int32_t>("stylus.sp.lock_flash_edge_x", 1));
    m_commonPost.m_aftCoorProcess.m_lockFlashEdgeY = static_cast<uint8_t>(store.getOr<int32_t>("stylus.sp.lock_flash_edge_y", 2));
    m_commonPost.m_aftCoorProcess.m_sensorTxCount = store.getOr<int32_t>("stylus.sp.lock_sensor_tx_count", 60);
    m_commonPost.m_aftCoorProcess.m_sensorRxCount = store.getOr<int32_t>("stylus.sp.lock_sensor_rx_count", 40);
    m_commonPost.m_aftCoorProcess.m_bypassLock = store.getOr<bool>("stylus.sp.lock_bypass", false);
    if (!aftCoorEnabled) { m_commonPost.m_aftCoorProcess.Reset(); }
}

bool StylusPipeline::Process(HeatmapFrame& frame) {
    frame.stylus.ResetPerFrameState();
    ReadLatestBtSample(frame.stylus.input.btSample);

    const auto selectTerminalProtocol = [&]() {
        if (m_lastActiveProtocol == StylusRuntime::Protocol::Hpp2) {
            frame.stylus.runtime.SelectHpp2().flow.terminal = true;
        } else if (m_lastActiveProtocol == StylusRuntime::Protocol::Hpp3) {
            frame.stylus.runtime.SelectHpp3().flow.terminal = true;
        } else if (m_penSession.protocolHint == StylusProtocolHint::Hpp2) {
            frame.stylus.runtime.SelectHpp2().flow.terminal = true;
        } else if (m_penSession.protocolHint == StylusProtocolHint::Hpp3) {
            frame.stylus.runtime.SelectHpp3().flow.terminal = true;
        } else {
            // Protocol-neutral terminal: keep activeProtocol as None so a fresh
            // disconnected session does not get misclassified as HPP3. Mark both
            // runtimes terminal because Active() maps None to HPP3 for legacy reads.
            frame.stylus.runtime.hpp2.flow.terminal = true;
            frame.stylus.runtime.hpp3.flow.terminal = true;
        }
    };

    if (m_penSession.hasConnectionState && !m_penSession.connected) {
        selectTerminalProtocol();
        FinalizeTerminalFrame(frame);
        return true;
    }

    const bool hasHpp3Evidence = frame.rawPtr != nullptr || frame.slaveSuffixValid;
    const StylusInputSnapshot inputBeforeParse = frame.stylus.input;
    m_frameParser.Process(frame);

    const bool parsedTerminal = frame.stylus.runtime.Active().flow.terminal;
    const bool parsedHpp2 = frame.stylus.runtime.activeProtocol == StylusRuntime::Protocol::Hpp2;

    if (parsedTerminal && !hasHpp3Evidence &&
        m_penSession.protocolHint == StylusProtocolHint::Hpp2 && !parsedHpp2) {
        frame.stylus.input = inputBeforeParse;
        m_frameParser.ProcessHpp2Line(frame);
    }

    if (frame.stylus.runtime.Active().flow.terminal) {
        FinalizeTerminalFrame(frame);
        return true;
    }

    bool completed = false;
    if (frame.stylus.runtime.activeProtocol == StylusRuntime::Protocol::Hpp2) {
        completed = m_hpp2.Process(frame);
    } else if (frame.stylus.runtime.activeProtocol == StylusRuntime::Protocol::Hpp3) {
        completed = m_hpp3.Process(frame);
    }

    if (!completed) {
        FinalizeTerminalFrame(frame);
        return true;
    }

    m_lastFrameWasTerminal = false;

    // ── Shared / common post-processing tail ───────────────────────
    m_edgeCoorProcess.Process(frame);
    m_edgeCoorPostProcess.Process(frame);
    m_commonPost.Process(frame);
    m_edgeCoorProcess.CaptureFinal(frame.stylus.runtime.Active());
    m_commit.Commit(frame);
    if (frame.stylus.runtime.activeProtocol != StylusRuntime::Protocol::None) {
        m_lastActiveProtocol = frame.stylus.runtime.activeProtocol;
    }
    return true;
}

void StylusPipeline::ApplyPenSession(const StylusPenSession& session) {
    const bool changed =
        m_penSession.hasConnectionState != session.hasConnectionState ||
        m_penSession.connected != session.connected ||
        m_penSession.hasStylusId != session.hasStylusId ||
        m_penSession.stylusId != session.stylusId ||
        m_penSession.protocolHint != session.protocolHint ||
        m_penSession.revision != session.revision;

    m_penSession = session;
    if (!changed) {
        return;
    }

    ResetStatefulStages();
    ClearBtSample();
    if (m_penSession.connected) {
        if (m_penSession.protocolHint == StylusProtocolHint::Hpp2) {
            m_lastActiveProtocol = StylusRuntime::Protocol::Hpp2;
        } else if (m_penSession.protocolHint == StylusProtocolHint::Hpp3) {
            m_lastActiveProtocol = StylusRuntime::Protocol::Hpp3;
        } else {
            m_lastActiveProtocol = StylusRuntime::Protocol::None;
        }
    }
    m_lastFrameWasTerminal = true;
}

void StylusPipeline::ResetStatefulStages() {
    m_hpp2.ResetOnTerminal();
    m_hpp3.ResetOnTerminal();
    m_edgeCoorProcess.Reset();
    m_edgeCoorPostProcess.Reset();
    m_commonPost.ResetOnTerminal();
}

void StylusPipeline::ClearBtSample() {
    std::lock_guard<std::mutex> lk(m_btMutex);
    m_btSample = {};
}

void StylusPipeline::FinalizeTerminalFrame(HeatmapFrame& frame) {
    if (frame.stylus.runtime.activeProtocol != StylusRuntime::Protocol::None) {
        m_lastActiveProtocol = frame.stylus.runtime.activeProtocol;
    }
    if (!m_lastFrameWasTerminal) {
        ResetStatefulStages();
    }
    m_lastFrameWasTerminal = true;
#if EGOTOUCH_DIAG
    frame.stylus.runtime.ResetDiagnosticFields();
#endif
    m_edgeCoorProcess.CaptureFinal(frame.stylus.runtime.Active());
    m_commit.Commit(frame);
}

void StylusPipeline::SetBtMcuPressure(uint16_t pressure) {
    Asa::BtInputSnapshot next{};
    next.pressure[3] = pressure;
    next.hasSample = true;

    std::lock_guard<std::mutex> lk(m_btMutex);
    next.seq = m_btSample.seq + 1;
    m_btSample = next;
}

void StylusPipeline::SetBtMcuPressurePacket(const std::array<uint16_t, 4>& pressure,
                                            const std::array<uint16_t, 4>& rawPressure,
                                            uint8_t freq1,
                                            uint8_t freq2) {
    Asa::BtInputSnapshot next{};
    next.pressure = pressure;
    next.rawPressure = rawPressure;
    next.freq1 = freq1;
    next.freq2 = freq2;
    next.hasSample = true;
    next.hasFreq = true;

    std::lock_guard<std::mutex> lk(m_btMutex);
    next.seq = m_btSample.seq + 1;
    m_btSample = next;
}

void StylusPipeline::ReadLatestBtSample(Asa::BtInputSnapshot& out) const {
    std::lock_guard<std::mutex> lk(m_btMutex);
    out = m_btSample;
}

} // namespace Solvers
