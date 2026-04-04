#pragma once
#include "../IFrameProcessor.h"
#include <array>
#include <cstdint>
#include <string>

namespace Engine {

// =============================================================================
// TouchGestureStateMachine — 5-Phase Design
// =============================================================================
// Pipeline position: AFTER CoordinateFilter, BEFORE VHF.
//
// 5-phase lifecycle per slot:
//   Idle → PressCandidate → Dragging      → ReleasePending → Idle
//                         → LongPressHold → ReleasePending → Idle
//
// PressCandidate : anchor-locked coords, evaluates drag vs long-press
// Dragging       : live coordinate passthrough
// LongPressHold  : anchor-locked coords (stationary hold)
// ReleasePending : grace window, recovers to prevPhase if contact returns

// ---- Phase enum ----
enum class GesturePhase : uint8_t {
    Idle = 0,
    PressCandidate,   // New contact, waiting for drag/longpress decision
    Dragging,         // Moving contact, live coordinate passthrough
    LongPressHold,    // Stationary hold, anchor-locked coords
    ReleasePending,   // Contact gone, grace window before Up
};

// ---- Per-slot state ----
struct GestureSlot {
    GesturePhase phase = GesturePhase::Idle;
    GesturePhase prevPhase = GesturePhase::Idle; // For ReleasePending recovery

    // Anchor: position at first Down emission (for tap/longpress stability).
    float anchorX = 0.0f;
    float anchorY = 0.0f;

    // Last tracked raw position from tracker output.
    float lastTrackedX = 0.0f;
    float lastTrackedY = 0.0f;

    // Last emitted output position (for coordinate hold).
    float lastOutputX = 0.0f;
    float lastOutputY = 0.0f;

    // Timing counters (frame-based).
    uint16_t ageFrames = 0;          // Frames since PressCandidate entry.
    uint16_t missingFrames = 0;      // Consecutive frames contact is absent.
    uint16_t stableFrames = 0;       // Consecutive frames that passed stable checks.

    // Feature snapshot from tracker.
    float sizeMm = 0.0f;
    int signalSum = 0;
    int area = 0;
    bool isEdge = false;

    // Flags.
    bool quickTapEligible = true;    // False once entered Dragging or LongPressHold.
    bool upEmitted = false;          // Up debounce: suppress contacts for 1 frame.

    void Reset() {
        phase = GesturePhase::Idle;
        prevPhase = GesturePhase::Idle;
        anchorX = anchorY = 0.0f;
        lastTrackedX = lastTrackedY = 0.0f;
        lastOutputX = lastOutputY = 0.0f;
        ageFrames = missingFrames = stableFrames = 0;
        sizeMm = 0.0f;
        signalSum = area = 0;
        isEdge = false;
        quickTapEligible = true;
        upEmitted = false;
    }
};

// ---- State machine class ----
class TouchGestureStateMachine : public IFrameProcessor {
public:
    static constexpr int kMaxSlots = 20;

    TouchGestureStateMachine();

    bool Process(HeatmapFrame& frame) override;
    std::string GetName() const override { return "TouchGestureStateMachine"; }
    std::vector<ConfigParam> GetConfigSchema() const override;
    void SaveConfig(std::ostream& out) const override;
    void LoadConfig(const std::string& key, const std::string& value) override;

private:
    std::array<GestureSlot, kMaxSlots> m_slots{};

    // ---- Tuning parameters (per design doc §11) ----
    int m_pressCandidateFrames = 1;       // Stable frames before emitting Down.
    int m_pressCandidateMinSignal = 0;    // Min signalSum for stable check.
    float m_pressCandidateMinSizeMm = 0.0f;

    float m_dragThreshold = 0.8f;         // DragStartDistance (sensor units).
    int m_longPressFrames = 46;           // ~380ms at 120Hz.
    float m_longPressMoveTolerance = 0.8f;// Max drift from anchor for longpress.

    int m_releasePendingFrames = 0;       // Grace frames (0 = instant Up).

    void UpdateSlot(GestureSlot& slot, const TouchContact* contact, int slotIndex);
};

} // namespace Engine
