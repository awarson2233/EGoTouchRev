#pragma once

#include "AsaTypes.h"
#include "GridPeakDetector.h"
#include "CoordinateSolver.h"
#include "CoorPostProcessor.h"
#include "EdgeLiftCorrector.h"
#include "CoorReviser.h"
#include "LinearFilter.h"
#include "EngineTypes.h"
#include "ConfigSchema.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <span>
#include <deque>
#include <mutex>
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

    /// 注入 BT MCU 压感值（由 PenBridge 线程实时更新，加入时间戳队列供 Process() 取用）
    void SetBtMcuPressure(uint16_t p);

    /// 蓝牙按键数据注入接口（由 PenBridge 或 BLE 线程调用）
    /// raw_data bit0: barrel button, bit1: eraser toggle
    /// 在 BLE 数据未接入前，按键状态默认保持 IDLE (0)
    void UpdateButtonFromBle(uint8_t raw_data) {
        m_bleButtonState.store(raw_data, std::memory_order_relaxed);
    }

    const StylusFrameData& GetLastResult() const {
        return m_lastResult;
    }

    /// P2 #20: Frequency shift output freeze control.
    /// Call with true when frequency shift starts, false when done.
    /// During freeze, Process() will output the last known-good frame.
    void SetFreqShiftFreezing(bool freezing) {
        m_freqShiftFreezing = freezing;
    }

    std::vector<ConfigParam> GetConfigSchema() const;
    void SaveConfig(std::ostream& out) const;
    void LoadConfig(const std::string& key,
                    const std::string& value);

    /// 实时坐标分解诊断（每帧 Process() 后更新）
    /// Canonical definition is Engine::StylusFrameData::StylusDiagnostics (EngineTypes.h).
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



    // ── VHF packet builder ──
    void BuildStylusPacket(StylusPacket& pkt) const;

    // ── Algorithm modules ──
    Asa::GridPeakDetector    m_peakDetector;
    Asa::CoordinateSolver    m_coordSolver;
    Asa::CoorPostProcessor   m_postProcessor;
    Asa::EdgeLiftCorrector   m_edgeLiftCorrector;
    bool                     m_elcEnabled = true; // P1: Edge-lift corrector switch
    Asa::CoorReviser         m_coorReviser;       // P2: TX2 dual-freq revision
    Asa::LinearFilter        m_linearFilter;      // P2: 7-state line filter

    // ── Pipeline state ──
    StylusFrameData  m_lastResult{};
    Asa::AsaGridData m_gridData{};
    bool m_prevValid = false;
    uint32_t m_prevStatus = 0;
    float m_prevPointX = 0.0f;  // P1: previous frame X for edge-lift detection
    float m_prevPointY = 0.0f;  // P1: previous frame Y for edge-lift detection
    // P2 #20: Frequency shift freeze flag
    // When true, output is frozen to lastGoodFrame until shift completes.
    // Set by external frequency shift controller; cleared after shift done.
    bool m_freqShiftFreezing = false;

    // ── BLE button state (decoupled from hardcoded logic) ──
    std::atomic<uint8_t> m_bleButtonState{0};  // bit0: barrel, bit1: eraser

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
    // P2: Tilt vector length clamp limit (in coordinate diff units)
    // Mirrors TSACore step 9: if |v| > limit, scale v to limit preserving direction
    float m_tiltVectorClampLimit = 7.0f;
    int16_t m_prevTiltX = 0;
    int16_t m_prevTiltY = 0;
    float m_prevTiltDiffX = 0.0f;
    float m_prevTiltDiffY = 0.0f;
    bool  m_tiltHasHistory = false;
    // P2 #15: TX1/TX2 signal ratio for tilt anomaly detection
    // TSACore: GetTX1TX2SignalRatio + BufTX1TX2SignalRatio (3-frame avg)
    static constexpr int kSignalRatioBufLen = 3;
    std::array<uint16_t, kSignalRatioBufLen> m_signalRatioBuf{};
    int m_signalRatioBufCount = 0;
    uint16_t m_signalRatio = 100;  // percent: TX2_signal*100/TX1_signal
    // When signal ratio is anomalous (too large diff), use buffered tilt diff
    float m_tiltDiffAnomalyThreshold = 3.0f;  // max allowed coord diff jump
    bool  m_tiltAnomalyDamped = false;  // diagnostic: true when anomaly damping fired

    void SolveTilt(const Asa::AsaCoorResult& tx1Coor,
                   const Asa::AsaCoorResult& tx2Coor);
    void ResetTilt();
    int ConvertCoordDiffToTilt(float diff, bool dimY) const;

    // ── Pressure state (full, migrated from StylusProcessor) ──
    // P1: Changed from Q7 (÷128) to Q8 (÷256) to match TSACore.
    // Default 64/256 = 0.25 alpha (was 64/128 = 0.50, too responsive).
    int   m_pressureIirWeightQ8 = 64;
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

    void SolvePressure(uint16_t rawPressure, bool active,
                       int signalStrength = 0, bool isEdge = false);

    // P1: Signal strength pressure suppression with hysteresis.
    // Mirrors TSACore HPP3_SuppressBtPressBySignal:
    //   - Enter suppression when signal < enterThreshold && not in edge
    //   - Exit suppression  when signal > exitThreshold
    //   - Uses g_hpp3ExitFlag state to prevent oscillation.
    bool m_pressureSignalSuppressEnabled = false;
    int  m_pressureSignalSuppressEnter = 200;  // prmt[0x24C]: enter suppression below this
    int  m_pressureSignalSuppressExit  = 300;  // prmt[0x24E]: exit suppression above this
    bool m_signalSuppressActive = false;       // hysteresis state flag (g_hpp3ExitFlag)

    // ── Sensor dimensions (full sensor array, not just 9x9 grid) ──
    // 屏幕方向: 右下角(0,0), 左上角(39,59)
    int m_sensorRows = 40;  // 行数 (Row/垂直/Y方向): anchor 范围 [0, 39]
    int m_sensorCols = 60;  // 列数 (Col/水平/X方向): anchor 范围 [0, 59]
    int m_anchorCenterOffset = 4; // grid center index to subtract (0=anchor is start, 4=anchor is center)

    // ── HID report logical maximum (from hidinjector.sys descriptor) ──
    // Descriptor: X Logical Max = 0x3E80 = 16000, Y Logical Max = 0x6400 = 25600
    // Physical:   X = 1660 (0.01mm), Y = 2660 (0.01mm)
    static constexpr float kHidMaxX = 16000.0f;
    static constexpr float kHidMaxY = 25600.0f;
    static constexpr int16_t kTiltMax = 9000;  // ±90° in centidegrees

    // ── P2: Screen mapping offset/margin (mirrors TSACore GetClipReport) ──
    // These define the active area within the sensor grid.
    // Coordinates outside [offset, sensorDim - endMargin] are clamped.
    int m_screenOffsetX = 0;     // left dead zone (in kCoorUnit)
    int m_screenOffsetY = 0;     // top dead zone
    int m_screenEndMarginX = 0;  // right dead zone
    int m_screenEndMarginY = 0;  // bottom dead zone

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
    // P1: Changed from frame-skip to freeze-output.
    // When noise is detected, output is frozen to last known-good frame
    // (mirrors TSACore: memcpy(curASOut ← prevASOut) + return 5).
    bool m_hpp3NoisePostEnabled = false;  // disabled for initial bringup
    int  m_hpp3SignalRatioFactor = 5;
    int  m_hpp3SignalDropFactor = 5;
    float m_hpp3CoorJumpThreshold = 20.0f;
    float m_prevValidX = 0.0f;
    float m_prevValidY = 0.0f;
    bool  m_prevValidPoint = false;
    // P1: Last known-good frame data for freeze output
    StylusFrameData m_lastGoodFrame{};
    bool m_hasLastGoodFrame = false;
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

    // P3 #21: Pen exit smoothing (TSACore: ReleaseASAReportExitStylus)
    // When pen transitions from valid→invalid while inking, freeze output
    // for 1 frame with edge coordinate snapping if at panel edge.
    bool m_exitSmoothEnabled = true;
    bool m_wasInking = false;  // true if prev frame had pressure > 0
    bool HandlePenExitSmooth(StylusPacket& outPacket);

    // ── ASACalibration_Process (Phase 6 — rolling average) ──
    static constexpr int kCalibWindow = 5;
    int  m_calibCount = 0;
    std::array<int32_t, kCalibWindow> m_calibDim1{};
    std::array<int32_t, kCalibWindow> m_calibDim2{};
    bool m_calibEnabled = false;
    Asa::AsaCoorResult ApplyCalibration(const Asa::AsaCoorResult& c);
    void ResetCalibration();


    // ── 实时坐标分解诊断（内部存储）──
    DbgCoordBreakdown m_dbg{};


    // ── Config ──
    bool m_enableSlaveChecksum = false;  // unverified; disable until checksum format is confirmed
    bool m_emitPacketWhenInvalid = true;

    // P3 #16: TP Pattern Compensation (TSACore: CoorTpPatternCompensate)
    // Cubic polynomial on (TX1-TX2) diff, applied temporarily for RefreshTX1Pos.
    // Since we don't have RefreshTX1Pos, this is a no-op placeholder.
    // Enabled only if bit6 of feature flag is set.
    bool m_tpPatternCompEnabled = false;
    std::array<double, 4> m_tpPatternCoefDim1{{0.0, 0.0, 0.0, 0.0}};
    std::array<double, 4> m_tpPatternCoefDim2{{0.0, 0.0, 0.0, 0.0}};

    // P3 #22: Common-Mode Filtering (TSACore: HPP3_CMFProcess / GetCMN)
    // Morphological min-max filtering on raw 1D projections to estimate
    // common-mode noise, then subtract it before interpolation.
    bool m_cmfEnabled = false;
    int  m_cmfWindowSize = 6;  // erosion/dilation window half-width
    void ApplyCommonModeFilter(int16_t grid[Asa::kGridDim][Asa::kGridDim]);

    // ── BT MCU 外部压感注入与防抖 ────────────────────────────────────────────────
    struct BtPressureSample {
        uint64_t timestamp_ms;
        uint16_t pressure;
    };
    mutable std::mutex m_btPressureMutex;
    std::deque<BtPressureSample> m_btPressureHistory;
};

} // namespace Engine
