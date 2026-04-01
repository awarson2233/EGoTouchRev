#pragma once

#include "AsaTypes.h"
#include "GridPeakDetector.h"
#include "CoordinateSolver.h"
#include "CoorPostProcessor.h"
#include "EngineTypes.h"

#include <array>
#include <cstdint>
#include <span>
#include <deque>
#include <mutex>
#include <string>
#include <iosfwd>

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

    /// 注入 BT MCU 压感值（由 PenBridge 线程实时更新，加入时间戳队列供 Process() 取用）
    void SetBtMcuPressure(uint16_t p);

    const StylusFrameData& GetLastResult() const {
        return m_lastResult;
    }

    void DrawConfigUI();
    void SaveConfig(std::ostream& out) const;
    void LoadConfig(const std::string& key,
                    const std::string& value);

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



    // ── VHF packet builder ──
    void BuildStylusPacket(StylusPacket& pkt) const;

    // ── Algorithm modules ──
    Asa::GridPeakDetector  m_peakDetector;
    Asa::CoordinateSolver  m_coordSolver;
    Asa::CoorPostProcessor m_postProcessor;

    // ── Pipeline state ──
    StylusFrameData  m_lastResult{};
    Asa::AsaGridData m_gridData{};
    bool m_prevValid = false;
    uint32_t m_prevStatus = 0;

    // ── Tilt state (full, migrated from StylusProcessor) ──
    bool  m_tiltEnabled = false;  // disabled for bringup (requires correct TX2 params)
    bool  m_tiltKeepLastOnInvalid = true;
    int   m_tiltDiffAverageWindow = 5;
    int   m_tiltDiffBufCount = 0;
    std::array<float, 10> m_tiltDiffBufX{};
    std::array<float, 10> m_tiltDiffBufY{};
    float m_tiltDegreePerCellX = 8.0f;
    float m_tiltDegreePerCellY = 8.0f;
    float m_tiltNormLenX = 7.16f;
    float m_tiltNormLenY = 7.16f;
    int   m_tiltMaxDegree = 60;
    int   m_tiltJitterThresholdDeg = 1;
    float m_tiltCoordIirOldWeight = 0.875f;
    int16_t m_prevTiltX = 0;
    int16_t m_prevTiltY = 0;
    float m_prevTiltDiffX = 0.0f;
    float m_prevTiltDiffY = 0.0f;
    bool  m_tiltHasHistory = false;

    void SolveTilt(const Asa::AsaCoorResult& tx1Coor,
                   const Asa::AsaCoorResult& tx2Coor);
    void ResetTilt();
    int ConvertCoordDiffToTilt(float diff, bool dimY) const;

    // ── Pressure state (full, migrated from StylusProcessor) ──
    int   m_pressureIirWeightQ7 = 64;
    uint16_t m_prevPressure = 0;
    bool  m_pressurePolyEnabled = true;
    std::array<double, 5> m_pressurePolySeg1{
        {0.0, 0.0, 0.0078740157480315, 0.0, 0.0}};
    std::array<double, 5> m_pressurePolySeg2{
        {-409.317785463, 4.39982201266, -0.00161165641489,
          2.623779267e-07, -1.60182e-11}};
    int   m_pressureMapSeg1Threshold = 11;
    int   m_pressureMapSeg2Threshold = 127;
    int   m_pressureMapGainPercent = 100;
    int   m_pressureTailFrames = 0;
    int   m_pressureTailMin = 10;
    int   m_pressureTailDecay = 48;
    int   m_pressureTailCounter = 0;

    void SolvePressure(uint16_t rawPressure, bool active);

    // ── Sensor dimensions (full sensor array, not just 9x9 grid) ──
    int m_sensorDimX = 37;  // rows (dim1) — adjust per device
    int m_sensorDimY = 23;  // cols (dim2) — adjust per device
    int m_anchorCenterOffset = 4; // grid center index to subtract (0=anchor is start, 4=anchor is center)

    // ── Edge coordinate post-process (from TSACore EdgeCoorPostProcess) ──
    bool m_edgeCoorPostEnabled = true;
    static constexpr int kEdgeDeadZone = 0x40;   // 64/1024 of a cell == 6.25%
    static constexpr int kCellUnit     = 0x400;  // 1024 units per cell
    static constexpr int kEdgeActiveZone = kCellUnit - kEdgeDeadZone; // 0x3C0
    void EdgeCoorPostProcess(float& dim1, float& dim2) const;

    // ── Slave header byte layout ──
    int m_slaveHdrBtnOffset = 6;    // byte offset for button (uint8)
    uint8_t m_rawSlaveHdr[7]{};     // cached for GUI display

    // ── Button state ──
    int m_buttonReleaseHoldFrames = 2;
    int m_buttonReleaseCounter = 0;
    uint32_t UpdateButtonState(uint32_t rawBits, bool active);

    // ── Recheck (migrated from StylusProcessor) ──
    bool m_recheckEnabled = true;
    int  m_recheckSignalThreshBase = 120;
    int  m_noiseLevel = 0;
    bool EvaluateRecheck() const;

    // ── HPP3 Noise Post Process (migrated) ──
    bool m_hpp3NoisePostEnabled = false;  // disabled for initial bringup
    int  m_hpp3SignalRatioFactor = 5;
    int  m_hpp3SignalDropFactor = 5;
    float m_hpp3CoorJumpThreshold = 20.0f;
    float m_prevValidX = 0.0f;
    float m_prevValidY = 0.0f;
    bool  m_prevValidPoint = false;
    bool ApplyHpp3NoisePost(const Asa::AsaCoorResult& coor);

    // ── Pen Lifecycle Tracker ──
    enum class PenLifecycle : uint8_t {
        Leave = 0,   // 笔不在感应范围内
        Hover,       // 悬浮：有位置信号，无压力
        Contact,     // 接触：有位置信号 + 压力
        Lifting,     // 抬笔过渡：防抖保留
    };
    PenLifecycle m_penLifecycle = PenLifecycle::Leave;
    int  m_liftingFrameCount = 0;
    int  m_liftingTimeout = 10;
    void UpdatePenLifecycle(bool penValid, bool penDown);

    // ── ASACalibration_Process (Phase 6 — rolling average) ──
    static constexpr int kCalibWindow = 5;
    int  m_calibCount = 0;
    std::array<int32_t, kCalibWindow> m_calibDim1{};
    std::array<int32_t, kCalibWindow> m_calibDim2{};
    bool m_calibEnabled = false;
    Asa::AsaCoorResult ApplyCalibration(const Asa::AsaCoorResult& c);
    void ResetCalibration();



    // ── Config ──
    bool m_enableSlaveChecksum = false;  // unverified; disable until checksum format is confirmed
    bool m_emitPacketWhenInvalid = true;

    // ── BT MCU 外部压感注入与防抖 ────────────────────────────────────────────────
    struct BtPressureSample {
        uint64_t timestamp_ms;
        uint16_t pressure;
    };
    mutable std::mutex m_btPressureMutex;
    std::deque<BtPressureSample> m_btPressureHistory;
};

} // namespace Engine
