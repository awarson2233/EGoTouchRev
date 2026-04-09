#pragma once

#include "AsaTypes.hpp"
#include "GridPeakDetector.hpp"
#include "CoordinateSolver.hpp"
#include "CoorPostProcessor.hpp"
#include "EdgeLiftCorrector.hpp"
#include "CoorReviser.hpp"
#include "LinearFilter.hpp"
#include "OneEuroFilter.hpp"
#include "TiltSolver.hpp"
#include "PressureSolver.hpp"
#include "CommonModeFilter.hpp"
#include "PenStateMachine.hpp"
#include "PacketBuilder.hpp"
#include "NoiseGate.hpp"
#include "PipelineUtils.hpp"
#include "EngineTypes.h"
#include "ConfigSchema.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <span>
#include <string>
#include <iosfwd>
#include <vector>

namespace Engine {

/// StylusPipeline — Independent stylus processing entry point.
/// Replaces the old StylusProcessor (IFrameProcessor).
/// Complete pipeline: rawData → Parse → 9×9 Grid → Peak → Coord →
///     PostProcess → Tilt/Pressure → Recheck → Animation →
///     Calibration → StylusPacket.
class StylusPipeline {
public:
    bool Process(std::span<const uint8_t> rawData,
                 StylusPacket& outPacket);

    /// 注入 BT MCU 压感值（由 PenBridge 线程实时更新）
    void SetBtMcuPressure(uint16_t p) {
        m_pressureSolver.SetBtMcuPressure(p);
    }

    /// 蓝牙按键数据注入接口
    void UpdateButtonFromBle(uint8_t raw_data) {
        m_bleButtonState.store(raw_data, std::memory_order_relaxed);
    }

    const StylusFrameData& GetLastResult() const {
        return m_lastResult;
    }

    /// P2 #20: Frequency shift output freeze control.
    void SetFreqShiftFreezing(bool freezing) {
        m_freqShiftFreezing = freezing;
    }

    std::vector<ConfigParam> GetConfigSchema() const;
    void SaveConfig(std::ostream& out) const;
    void LoadConfig(const std::string& key,
                    const std::string& value);

    /// Filter mode accessors (0=IIR, 1=1-Euro, 2=None)
    int  GetFilterMode() const { return m_filterMode; }
    void SetFilterMode(int mode) { m_filterMode = std::clamp(mode, 0, 2); }

    /// 实时坐标分解诊断
    using DbgCoordBreakdown = Engine::StylusFrameData::StylusDiagnostics;
    const DbgCoordBreakdown& GetDebugCoord() const { return m_dbg; }


private:
    // ── Frame Constants ──
    static constexpr size_t kSlaveFrameBytes  = 339;
    static constexpr size_t kSlaveHeaderBytes = 7;
    static constexpr size_t kSlaveWordCount   = 166;
    static constexpr size_t kSlaveWordOffset  = kSlaveHeaderBytes;
    static constexpr size_t kStylusBlockWords = 83;

    // ── Slave frame parsing ──
    bool ParseSlaveWords(
        std::span<const uint8_t> rawData,
        std::array<uint16_t, kSlaveWordCount>& outWords) const;
    bool ValidateChecksum16(
        const uint8_t* bytes, size_t wordCount,
        uint16_t& outChecksum) const;

    // ── Algorithm modules (header-only) ──
    Asa::GridPeakDetector    m_peakDetector;
    Asa::CoordinateSolver    m_coordSolver;
    Asa::CoorPostProcessor   m_postProcessor;
    Asa::EdgeLiftCorrector   m_edgeLiftCorrector;
    Asa::CoorReviser         m_coorReviser;
    Asa::LinearFilter        m_linearFilter;
    Asa::OneEuroFilter       m_oneEuroFilter;
    Asa::TiltSolver          m_tiltSolver;
    Asa::PressureSolver      m_pressureSolver;
    Asa::CommonModeFilter    m_cmfFilter;
    Asa::PenStateMachine     m_penStateMachine;
    Asa::NoiseGate           m_noiseGate;
    Asa::CalibrationAvg      m_calibration;
    Asa::EdgeCoorPost        m_edgeCoorPost;
    Asa::SignalRatioTracker  m_signalRatioTracker;
    PacketBuilder            m_packetBuilder;

    // ── Smoothing filter mode (0=IIR, 1=1-Euro, 2=None) ──
    int m_filterMode = 0;

    // ── Pipeline state ──
    StylusFrameData  m_lastResult{};
    Asa::AsaGridData m_gridData{};
    bool m_prevValid = false;
    uint32_t m_prevStatus = 0;
    float m_prevPointX = 0.0f;
    float m_prevPointY = 0.0f;
    uint16_t m_prevPressureVal = 0;  // previous frame pressure for edge-lift detection
    bool m_freqShiftFreezing = false;

    // ── BLE button state ──
    std::atomic<uint8_t> m_bleButtonState{0};

    // ── Sensor dimensions ──
    int m_sensorRows = 40;
    int m_sensorCols = 60;
    int m_anchorCenterOffset = 4;

    // ── SensorPitchSizeMap tables ──
    bool m_pitchMapEnabled = false;
    std::array<double, Asa::kMaxSensorDim + 1> m_pitchTableDim1 =
        []{ std::array<double, Asa::kMaxSensorDim + 1> a; a.fill(100.0); return a; }();
    std::array<double, Asa::kMaxSensorDim + 1> m_pitchTableDim2 =
        []{ std::array<double, Asa::kMaxSensorDim + 1> a; a.fill(100.0); return a; }();

    // ── Slave header layout ──
    int m_slaveHdrBtnOffset = 6;
    uint8_t m_rawSlaveHdr[7]{};

    // ── Button state ──
    int m_buttonReleaseHoldFrames = 2;
    int m_buttonReleaseCounter = 0;
    uint32_t UpdateButtonState(uint32_t rawBits, bool active);

    // ── Edge lift corrector switch ──
    bool m_elcEnabled = true;

    // ── Noise level (for recheck) ──
    int m_noiseLevel = 0;

    // ── Good frame history for freeze output ──
    StylusFrameData m_lastGoodFrame{};
    bool m_hasLastGoodFrame = false;

    // ── Pen exit smoothing ──
    bool m_wasInking = false;

    // ── Config ──
    bool m_enableSlaveChecksum = false;
    bool m_emitPacketWhenInvalid = true;

    // P3 #16: TP Pattern Compensation (placeholder)
    bool m_tpPatternCompEnabled = false;
    std::array<double, 4> m_tpPatternCoefDim1{{0.0, 0.0, 0.0, 0.0}};
    std::array<double, 4> m_tpPatternCoefDim2{{0.0, 0.0, 0.0, 0.0}};

    // ── 诊断 ──
    DbgCoordBreakdown m_dbg{};
};

} // namespace Engine
