#pragma once
// ══════════════════════════════════════════════════════════════════════
// TouchPipeline — Linear orchestrator for the TSACore touch solver.
// Architecture:
//   - All 8 algorithm modules are modern C++23 header-only (.hpp)
//   - Process() executes all stages linearly, no virtual dispatch.
//   - Operates directly on the shared big frame (TouchBigFrame).
//   - TouchPipeline.cpp contains ONLY orchestration logic, no heuristic
//     calculations or complex branching.
// ══════════════════════════════════════════════════════════════════════

#include "SolverTypes.h"
#include "TouchCoreTypes.hpp"

// ── Stage 0: Frame Ingestion ──
#include "MasterFrameParser.hpp"

// ── Stage 1: Signal Conditioning & Baseline Management ──
#include "HardwareAnalyzerAndHold.hpp"

#include "BaselineAndDiff.hpp"

// ── Stage 2: Common Mode Noise Filtering & Grid IIR ──
#include "CmfAndGridFilter.hpp"

// ── Stage 2.5: Per-frame Dynamic Threshold Recomputation ──
#include "TouchThold.hpp"

// ── Stage 3: Peak Detection & Grip Region ──
#include "PeakAndGripRegion.hpp"

// ── Stage 4: Touch Zone BFS Flood & Weighted Centroid CoG ──
#include "TouchZoneAndCentroid.hpp"

// ── Stage 5: Multi-target Hungarian Tracking & Touch Event FSM ──
#include "TrackingAndEvents.hpp"

// ── Stage 6: Palm Rejection, Thenar & Grip Filtering ──
#include "PalmAndGripRejection.hpp"

// ── Stage 6.5: Four-state Touch Mode FSM ──
#include "TouchModeFsm.hpp"

// ── Stage 7: Touch Report Admission & Adaptive Speed IIR Filter ──
#include "TouchReportAndFilter.hpp"

// ── Stage 8: Display Mapping, Sensor Pitch Remap & Double Report ──
#include "PitchRemapAndDisplay.hpp"

// ── Backward Compatibility Headers ──
#include "TouchGestureStateMachine.hpp"

#include <atomic>
#include <string>
#include <vector>
#include <array>
#include <mutex>

namespace Config {
class ConfigBinder;
class ConfigStore;
}

namespace Solvers {

class TouchPipeline {
public:
    TouchPipeline();

    /// Main entry: processes one frame through all stages using the shared big frame.
    bool Process(HeatmapFrame& frame);
    bool ProcessMasterParserOnly(HeatmapFrame& frame);

    /// Runs tracking and downstream stages.
    bool ProcessTrackingAndGesture(HeatmapFrame& frame);
    bool ProcessGestureOutput(HeatmapFrame& frame);

    /// Pipeline name for config file section header.
    std::string GetName() const { return "TouchPipeline"; }

    // ── Configuration interface ──
    void registerBindings(Config::ConfigBinder& binder);
    void applyConfig(const Config::ConfigStore& store);

    // ── Thread-safe accessors for UI / IPC / Diagnostics ──
    std::vector<Touch::Peak> GetPeaks() const;
    std::array<uint8_t, 2400> GetTouchZones() const;
    std::array<uint8_t, 2400> GetZoneEdge() const;

    int GetCachedPeakCount() const { return m_cachedPeakCount.load(std::memory_order_relaxed); }
    int GetCachedZoneCount() const { return m_cachedZoneCount.load(std::memory_order_relaxed); }
    int GetCachedContactCount() const { return m_cachedContactCount.load(std::memory_order_relaxed); }

    // ── Authentic 8-Stage Algorithm Modules ──
    Touch::MasterFrameParser        m_frameParser;    // Stage 0: 裸帧解析
    Touch::HardwareAnalyzerAndHold  m_hwAnalyzer;     // Stage 0.5: 帧间硬件异常分析与异常帧保持
    Touch::BaselineAndDiff          m_baseline;       // Stage 1: 差分与基线 FSM
    Touch::CmfAndGridFilter         m_cmf;            // Stage 2: 1D Dim1 CMF 与网格滤波
    Touch::TouchThold               m_dynThold;       // Stage 2.5: 每帧动态门限重算 (TSAPrmt_Process)
    Touch::PeakAndGripRegion        m_peakDet;        // Stage 3: Mode 3 极值与 2D 握持墙
    Touch::TouchZoneAndCentroid     m_touchZone;      // Stage 4: 4-连通 BFS 与定点加权重心
    Touch::TrackingAndEvents        m_tracker;        // Stage 5: 匈牙利追踪与 PTP 状态机
    Touch::PalmAndGripRejection     m_palmGrip;       // Stage 6: 大鱼际识别与握持防误触
    Touch::TouchModeFsm             m_touchMode;      // Stage 6.5: 四态触控模式 FSM (TouchMode_Process)
    Touch::TouchReportAndFilter     m_reportFilter;   // Stage 7: 上报网闸、自适应 IIR 与迟滞防抖
    Touch::PitchRemapAndDisplay     m_displayRemap;   // Stage 8: 80 阶样条走线重映射与显示映射

    // ── Compatibility aliases for external callers / tests ──
    Touch::TouchGestureStateMachine m_legacyGesture;
    Touch::TouchReportAndFilter&    m_coordFilter;
    Touch::TrackingAndEvents&       m_gesture;
    Touch::PalmAndGripRejection&    m_palmBoxSuppressor;
    Touch::TouchZoneAndCentroid&    m_contactExtractor;

private:
    void UpdateContactCaches(HeatmapFrame& frame);
    void ResetIdleOutputs(HeatmapFrame& frame);
    /// 帧尾把本帧触点统计回灌给 Stage 2.5 / Stage 4 的跨帧输入
    /// （对位 PrevTouch_PostProcess）。
    void LatchPrevTouchInputs();
    /// Stage 6.5：对位 TSA_MSProcess 里 GripFilter_Process 之后那一次
    /// TouchMode_Process 调用（AntiTouch_Process / TouchReport_Process 之前）。
    void RunTouchModeFsm();

    // 共享大帧物理总线实例
    Touch::TouchBigFrame m_bigFrame;

    // 线程安全互斥锁与原子计数
    mutable std::mutex m_diagMu;
    std::atomic<int> m_cachedPeakCount{0};
    std::atomic<int> m_cachedZoneCount{0};
    std::atomic<int> m_cachedContactCount{0};
};

} // namespace Solvers
