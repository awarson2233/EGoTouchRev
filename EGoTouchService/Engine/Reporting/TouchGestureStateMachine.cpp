#include "TouchGestureStateMachine.h"
#include <algorithm>
#include <cmath>

namespace Engine {

TouchGestureStateMachine::TouchGestureStateMachine() {
    for (auto& s : m_slots) s.Reset();
}

// =============================================================================
// Process
// =============================================================================
bool TouchGestureStateMachine::Process(HeatmapFrame& frame) {
    // Build slot→contact mapping.
    std::array<TouchContact*, kMaxSlots> contactForSlot{};
    contactForSlot.fill(nullptr);
    for (auto& c : frame.contacts) {
        const int idx = c.id - 1;
        if (idx >= 0 && idx < kMaxSlots)
            contactForSlot[idx] = &c;
    }

    // Filter: treat tracker's state=Up contacts as absent.
    // The state machine is the sole authority on Up lifecycle.
    // Without this, residual Up contacts trigger spurious PressCandidate.
    for (int i = 0; i < kMaxSlots; ++i) {
        if (contactForSlot[i] && contactForSlot[i]->state == TouchStateUp)
            contactForSlot[i] = nullptr;
    }

    // Phase 1: Update slots.
    for (int i = 0; i < kMaxSlots; ++i) {
        auto& slot = m_slots[i];
        TouchContact* contact = contactForSlot[i];

        // Idle + no contact: clear cooldown.
        if (slot.phase == GesturePhase::Idle && contact == nullptr) {
            if (slot.upEmitted) slot.upEmitted = false;
            continue;
        }
        // Up debounce: suppress residual tracker contact for 1 frame.
        if (slot.phase == GesturePhase::Idle && slot.upEmitted) {
            slot.upEmitted = false;
            if (contact != nullptr) {
                contact->isReported = false;
                contact->reportEvent = TouchReportIdle;
            }
            continue;
        }
        UpdateSlot(slot, contact, i);
    }

    // Phase 2: Rewrite output fields based on slot state.
    for (auto& c : frame.contacts) {
        const int idx = c.id - 1;
        if (idx < 0 || idx >= kMaxSlots) continue;
        const auto& slot = m_slots[idx];

        switch (slot.phase) {
        case GesturePhase::Idle:
            c.isReported = false;
            c.reportEvent = TouchReportIdle;
            break;

        case GesturePhase::PressCandidate:
            if (slot.stableFrames >= static_cast<uint16_t>(m_pressCandidateFrames)) {
                c.isReported = true;
                c.reportEvent = TouchReportDown;
                c.x = slot.lastOutputX;  // Anchor-locked.
                c.y = slot.lastOutputY;
            } else {
                c.isReported = false;
                c.reportEvent = TouchReportIdle;
            }
            break;

        case GesturePhase::Dragging:
            c.isReported = true;
            c.reportEvent = TouchReportMove;
            c.x = slot.lastOutputX;  // Live coords.
            c.y = slot.lastOutputY;
            break;

        case GesturePhase::LongPressHold:
            c.isReported = true;
            c.reportEvent = TouchReportMove;
            c.x = slot.lastOutputX;  // Anchor-locked.
            c.y = slot.lastOutputY;
            break;

        case GesturePhase::ReleasePending:
            c.isReported = true;
            c.reportEvent = TouchReportMove;
            c.x = slot.lastOutputX;  // Hold last output.
            c.y = slot.lastOutputY;
            break;
        }
    }

    // Phase 3: ReleasePending → Idle transitions (emit Up).
    for (int i = 0; i < kMaxSlots; ++i) {
        auto& slot = m_slots[i];
        if (slot.phase != GesturePhase::ReleasePending) continue;
        if (slot.missingFrames <= static_cast<uint16_t>(m_releasePendingFrames)) continue;

        TouchContact upEvent;
        upEvent.id = i + 1;
        upEvent.x = slot.lastOutputX;
        upEvent.y = slot.lastOutputY;
        upEvent.state = TouchStateUp;
        upEvent.area = slot.area;
        upEvent.signalSum = slot.signalSum;
        upEvent.sizeMm = slot.sizeMm;
        upEvent.isEdge = slot.isEdge;
        upEvent.isReported = true;
        upEvent.reportEvent = TouchReportUp;
        upEvent.lifeFlags = TouchLifeLiftOff;
        upEvent.reportFlags = 0;

        frame.contacts.push_back(upEvent);
        slot.Reset();
        slot.upEmitted = true;
    }

    return true;
}

// =============================================================================
// UpdateSlot — per-slot state transition (design doc §5-§6)
// =============================================================================
void TouchGestureStateMachine::UpdateSlot(
    GestureSlot& slot, const TouchContact* contact, int /*slotIndex*/)
{
    switch (slot.phase) {

    // ----- IDLE → PressCandidate (T1) -----
    case GesturePhase::Idle:
        if (contact != nullptr) {
            slot.phase = GesturePhase::PressCandidate;
            slot.anchorX = contact->x;
            slot.anchorY = contact->y;
            slot.lastTrackedX = contact->x;
            slot.lastTrackedY = contact->y;
            slot.lastOutputX = contact->x;
            slot.lastOutputY = contact->y;
            slot.ageFrames = 1;
            slot.missingFrames = 0;
            slot.stableFrames = 1;
            slot.sizeMm = contact->sizeMm;
            slot.signalSum = contact->signalSum;
            slot.area = contact->area;
            slot.isEdge = contact->isEdge;
            slot.quickTapEligible = true;
        }
        break;

    // ----- PRESS CANDIDATE -----
    // Evaluates: → Dragging (T2), → LongPressHold (T3), → ReleasePending (T4)
    case GesturePhase::PressCandidate:
        if (contact == nullptr) {
            // T4: Contact vanished → ReleasePending.
            slot.prevPhase = GesturePhase::PressCandidate;
            slot.phase = GesturePhase::ReleasePending;
            slot.missingFrames = 1;
            return;
        }

        slot.ageFrames += 1;
        slot.missingFrames = 0;
        slot.lastTrackedX = contact->x;
        slot.lastTrackedY = contact->y;
        slot.sizeMm = contact->sizeMm;
        slot.signalSum = contact->signalSum;
        slot.area = contact->area;
        slot.isEdge = contact->isEdge;

        // Stable check for Down emission.
        {
            bool stable = true;
            if (m_pressCandidateMinSignal > 0 &&
                contact->signalSum < m_pressCandidateMinSignal)
                stable = false;
            if (m_pressCandidateMinSizeMm > 0.0f &&
                contact->sizeMm < m_pressCandidateMinSizeMm)
                stable = false;
            slot.stableFrames = stable ? (slot.stableFrames + 1) : 0;
        }

        // Coordinate stays locked to anchor during PressCandidate.
        slot.lastOutputX = slot.anchorX;
        slot.lastOutputY = slot.anchorY;

        // T2: Drag check — distance from anchor exceeds threshold.
        {
            const float dx = contact->x - slot.anchorX;
            const float dy = contact->y - slot.anchorY;
            if (dx * dx + dy * dy > m_dragThreshold * m_dragThreshold) {
                slot.phase = GesturePhase::Dragging;
                slot.quickTapEligible = false;
                // First drag frame: start outputting live coords.
                slot.lastOutputX = contact->x;
                slot.lastOutputY = contact->y;
                return;
            }
        }

        // T3: LongPress check — enough time stationary near anchor.
        if (slot.ageFrames >= static_cast<uint16_t>(m_longPressFrames)) {
            const float dx = contact->x - slot.anchorX;
            const float dy = contact->y - slot.anchorY;
            if (dx * dx + dy * dy <= m_longPressMoveTolerance * m_longPressMoveTolerance) {
                slot.phase = GesturePhase::LongPressHold;
                slot.quickTapEligible = false;
                // Coords stay locked to anchor.
            }
        }
        break;

    // ----- DRAGGING (no further state transitions except release) -----
    case GesturePhase::Dragging:
        if (contact == nullptr) {
            // T5: → ReleasePending, grace = 0.
            slot.prevPhase = GesturePhase::Dragging;
            slot.phase = GesturePhase::ReleasePending;
            slot.missingFrames = static_cast<uint16_t>(m_releasePendingFrames + 1);
            return;
        }

        slot.ageFrames += 1;
        slot.missingFrames = 0;
        slot.lastTrackedX = contact->x;
        slot.lastTrackedY = contact->y;
        slot.lastOutputX = contact->x;   // Live passthrough.
        slot.lastOutputY = contact->y;
        slot.sizeMm = contact->sizeMm;
        slot.signalSum = contact->signalSum;
        slot.area = contact->area;
        slot.isEdge = contact->isEdge;
        break;

    // ----- LONG PRESS HOLD (anchor-locked, only → ReleasePending) -----
    case GesturePhase::LongPressHold:
        if (contact == nullptr) {
            // T6: → ReleasePending, grace = 0.
            slot.prevPhase = GesturePhase::LongPressHold;
            slot.phase = GesturePhase::ReleasePending;
            slot.missingFrames = static_cast<uint16_t>(m_releasePendingFrames + 1);
            return;
        }

        slot.ageFrames += 1;
        slot.missingFrames = 0;
        slot.lastTrackedX = contact->x;
        slot.lastTrackedY = contact->y;
        slot.sizeMm = contact->sizeMm;
        slot.signalSum = contact->signalSum;
        slot.area = contact->area;
        slot.isEdge = contact->isEdge;

        // LongPressHold → Dragging: if finger moves beyond threshold.
        {
            const float dx = contact->x - slot.anchorX;
            const float dy = contact->y - slot.anchorY;
            if (dx * dx + dy * dy > m_dragThreshold * m_dragThreshold) {
                slot.phase = GesturePhase::Dragging;
                slot.lastOutputX = contact->x;
                slot.lastOutputY = contact->y;
                return;
            }
        }
        // Otherwise output stays locked to anchor.
        slot.lastOutputX = slot.anchorX;
        slot.lastOutputY = slot.anchorY;
        break;

    // ----- RELEASE PENDING -----
    // T7: recover to prevPhase if contact returns within grace.
    // T8: timeout → emit Up (handled in Phase 3 of Process()).
    case GesturePhase::ReleasePending:
        if (contact != nullptr) {
            // T7: Contact recovered — restore to prevPhase.
            slot.phase = slot.prevPhase;
            slot.missingFrames = 0;
            slot.lastTrackedX = contact->x;
            slot.lastTrackedY = contact->y;
            slot.sizeMm = contact->sizeMm;
            slot.signalSum = contact->signalSum;
            slot.area = contact->area;
            slot.isEdge = contact->isEdge;
            // Update output based on restored phase.
            if (slot.phase == GesturePhase::Dragging) {
                slot.lastOutputX = contact->x;
                slot.lastOutputY = contact->y;
            }
            // PressCandidate / LongPressHold keep anchor-locked output.
            return;
        }
        slot.missingFrames += 1;
        break;
    }
}

// =============================================================================
// GetConfigSchema
// =============================================================================
std::vector<ConfigParam> TouchGestureStateMachine::GetConfigSchema() const {
    std::vector<ConfigParam> schema = IFrameProcessor::GetConfigSchema();
    schema.push_back(ConfigParam("PressCandidateFrames", "Press Candidate Frames",
        ConfigParam::Int, const_cast<int*>(&m_pressCandidateFrames), 1, 10));
    schema.push_back(ConfigParam("PressCandidateMinSignal", "Press Candidate Min Signal",
        ConfigParam::Int, const_cast<int*>(&m_pressCandidateMinSignal), 0, 500));
    schema.push_back(ConfigParam("PressCandidateMinSizeMm", "Press Min Size mm",
        ConfigParam::Float, const_cast<float*>(&m_pressCandidateMinSizeMm), 0.0f, 5.0f));
    schema.push_back(ConfigParam("DragThreshold", "Drag Threshold",
        ConfigParam::Float, const_cast<float*>(&m_dragThreshold), 0.1f, 5.0f));
    schema.push_back(ConfigParam("LongPressFrames", "LongPress Frames",
        ConfigParam::Int, const_cast<int*>(&m_longPressFrames), 10, 120));
    schema.push_back(ConfigParam("LongPressMoveTolerance", "LongPress Move Tolerance",
        ConfigParam::Float, const_cast<float*>(&m_longPressMoveTolerance), 0.1f, 3.0f));
    schema.push_back(ConfigParam("ReleasePendingFrames", "Release Pending Frames",
        ConfigParam::Int, const_cast<int*>(&m_releasePendingFrames), 0, 10));
    return schema;
}

// =============================================================================
// SaveConfig / LoadConfig
// =============================================================================
void TouchGestureStateMachine::SaveConfig(std::ostream& out) const {
    IFrameProcessor::SaveConfig(out);
    out << "PressCandidateFrames=" << m_pressCandidateFrames << "\n";
    out << "PressCandidateMinSignal=" << m_pressCandidateMinSignal << "\n";
    out << "PressCandidateMinSizeMm=" << m_pressCandidateMinSizeMm << "\n";
    out << "DragThreshold=" << m_dragThreshold << "\n";
    out << "LongPressFrames=" << m_longPressFrames << "\n";
    out << "LongPressMoveTolerance=" << m_longPressMoveTolerance << "\n";
    out << "ReleasePendingFrames=" << m_releasePendingFrames << "\n";
}

void TouchGestureStateMachine::LoadConfig(
    const std::string& key, const std::string& value)
{
    IFrameProcessor::LoadConfig(key, value);
    if (key == "PressCandidateFrames")
        m_pressCandidateFrames = std::clamp(std::stoi(value), 1, 30);
    else if (key == "PressCandidateMinSignal")
        m_pressCandidateMinSignal = std::max(0, std::stoi(value));
    else if (key == "PressCandidateMinSizeMm")
        m_pressCandidateMinSizeMm = std::max(0.0f, std::stof(value));
    else if (key == "DragThreshold")
        m_dragThreshold = std::max(0.01f, std::stof(value));
    else if (key == "LongPressFrames")
        m_longPressFrames = std::clamp(std::stoi(value), 1, 300);
    else if (key == "LongPressMoveTolerance")
        m_longPressMoveTolerance = std::max(0.01f, std::stof(value));
    else if (key == "ReleasePendingFrames")
        m_releasePendingFrames = std::clamp(std::stoi(value), 0, 30);
}

} // namespace Engine
