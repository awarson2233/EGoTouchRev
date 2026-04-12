#pragma once
// ══════════════════════════════════════════════════════════════════════
// TouchPipeline — Single-CPP linear orchestrator for the Touch solver.
// Architecture: mirrors StylusPipeline (v2.4).
//   - All algorithm modules are header-only (.hpp) and held as members.
//   - Process() executes all phases linearly, no virtual dispatch.
//   - Config schema/save/load unified in this class.
// ══════════════════════════════════════════════════════════════════════

#include "EngineTypes.h"
#include "ConfigSchema.h"

// ── Phase 1: Frame Parsing ──
#include "MasterFrameParser.hpp"

// ── Phase 2: Signal Conditioning ──
#include "BaselineSubtraction.hpp"
#include "CMFProcessor.hpp"
#include "GridIIRProcessor.hpp"

// ── Phase 3: Feature Extraction ──
#include "MacroZoneDetector.hpp"
#include "PalmRejector.hpp"
#include "PeakDetector.hpp"
#include "MicroZoneSegmenter.hpp"

// ── Phase 4: Zone & Contact ──
#include "ZoneExpander.hpp"
#include "EdgeCompensation.hpp"
#include "TouchSize.hpp"
#include "EdgeRejection.hpp"

// ── Phase 5: Tracking & Coordinate Filtering ──
#include "TouchTracker.hpp"
#include "CoordinateFilter.hpp"

// ── Phase 6: Gesture & Output ──
#include "TouchGestureStateMachine.hpp"

#include <mutex>
#include <string>
#include <vector>
#include <iostream>

namespace Engine {

class TouchPipeline {
public:
    TouchPipeline() = default;

    /// Main entry: processes one frame through all 6 phases.
    bool Process(HeatmapFrame& frame);

    /// Pipeline name for config file section header.
    std::string GetName() const { return "TouchPipeline"; }

    // ── Configuration interface (replaces per-module IFrameProcessor config) ──
    std::vector<ConfigParam> GetConfigSchema() const;
    void SaveConfig(std::ostream& out) const;
    void LoadConfig(const std::string& key, const std::string& value);

    // ── Thread-safe accessors for UI/IPC (guarded by mutex) ──
    std::vector<Touch::Peak> GetPeaks() const;
    std::array<uint8_t, 2400> GetTouchZones() const;
    std::array<uint8_t, 2400> GetZoneEdge() const;

    int GetCachedPeakCount() const { return m_cachedPeakCount; }
    int GetCachedZoneCount() const { return m_cachedZoneCount; }
    int GetCachedContactCount() const { return m_cachedContactCount; }

    // ── Public module access (for direct parameter tuning from UI thread) ──
    // NOTE: Write access must be done when pipeline is idle or via config reload.
    Touch::MasterFrameParser         m_frameParser;
    Touch::BaselineSubtraction       m_baseline;
    Touch::CMFProcessor              m_cmf;
    Touch::GridIIRProcessor          m_gridIIR;
    Touch::MacroZoneDetector         m_macroZoneDet;
    Touch::PalmRejector              m_palmReject;
    Touch::PeakDetector              m_peakDet;
    Touch::MicroZoneSegmenter        m_microZoneSeg;
    Touch::ZoneExpander              m_zoneExp;
    Touch::EdgeCompensator           m_edgeComp;
    Touch::TouchSizeCalculator       m_touchSize;
    Touch::EdgeRejector              m_edgeReject;
    Touch::TouchTracker              m_tracker;
    Touch::CoordinateFilter          m_coordFilter;
    Touch::TouchGestureStateMachine  m_gesture;

private:
    mutable std::mutex m_mtx;

    // UI-thread-safe cache
    int m_cachedPeakCount = 0;
    int m_cachedZoneCount = 0;
    int m_cachedContactCount = 0;
};

} // namespace Engine
