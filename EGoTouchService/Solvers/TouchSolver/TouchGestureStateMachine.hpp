#pragma once
// ── TouchPipeline Module: TouchGestureStateMachine ──
// Header-only. Converted from Reporting/TouchGestureStateMachine.{h,cpp}.
// 5-Phase gesture lifecycle per slot.

#include "SolverTypes.h"
#include <array>
#include <cstdint>
#include <algorithm>
#include <cmath>

namespace Solvers { namespace Touch {

enum class GesturePhase : uint8_t {
    Idle = 0, PressCandidate, Dragging, LongPressHold, ReleasePending,
};

struct GestureSlot {
    GesturePhase phase = GesturePhase::Idle;
    GesturePhase prevPhase = GesturePhase::Idle;
    float anchorX = 0, anchorY = 0;
    float lastTrackedX = 0, lastTrackedY = 0;
    float lastOutputX = 0, lastOutputY = 0;
    uint16_t ageFrames = 0, missingFrames = 0, stableFrames = 0;
    float sizeMm = 0; int signalSum = 0, area = 0; bool isEdge = false;
    bool quickTapEligible = true, upEmitted = false;
    void Reset() {
        phase = prevPhase = GesturePhase::Idle;
        anchorX = anchorY = lastTrackedX = lastTrackedY = lastOutputX = lastOutputY = 0;
        ageFrames = missingFrames = stableFrames = 0;
        sizeMm = 0; signalSum = area = 0; isEdge = false;
        quickTapEligible = true; upEmitted = false;
    }
};

class TouchGestureStateMachine {
public:
    static constexpr int kMaxSlots = 20;
    bool  m_enabled = true;
    int   m_pressCandidateFrames = 1;
    int   m_pressCandidateMinSignal = 0;
    float m_pressCandidateMinSizeMm = 0.0f;
    float m_dragThreshold = 0.8f;
    int   m_longPressFrames = 46;
    float m_longPressMoveTolerance = 0.8f;
    int   m_releasePendingFrames = 0;
    bool  m_bypassStateMachine = false;

    TouchGestureStateMachine() { for (auto& s : m_slots) s.Reset(); }
    inline bool HasLiveState() const {
        for (const auto& slot : m_slots) {
            if (slot.phase != GesturePhase::Idle || slot.upEmitted) {
                return true;
            }
        }
        return false;
    }

    inline bool Process(HeatmapFrame& frame) {
        if (!m_enabled) return true;

        // Bypass mode
        if (m_bypassStateMachine) {
            for (auto& c : frame.contacts) {
                if (c.id <= 0 || !c.isReported) continue;
                switch (c.state) {
                case TouchStateDown: c.reportEvent = TouchReportDown; break;
                case TouchStateMove: c.reportEvent = TouchReportMove; break;
                case TouchStateUp:   c.reportEvent = TouchReportUp;   break;
                default: break;
                }
            }
            return true;
        }

        // Build slot→contact mapping
        std::array<TouchContact*, kMaxSlots> contactForSlot{};
        contactForSlot.fill(nullptr);
        for (auto& c : frame.contacts) {
            const int idx = c.id - 1;
            if (idx >= 0 && idx < kMaxSlots) contactForSlot[idx] = &c;
        }
        for (int i = 0; i < kMaxSlots; ++i) {
            if (contactForSlot[i] && contactForSlot[i]->state == TouchStateUp)
                contactForSlot[i] = nullptr;
        }

        // Phase 1: Update slots
        for (int i = 0; i < kMaxSlots; ++i) {
            auto& slot = m_slots[i];
            TouchContact* contact = contactForSlot[i];
            if (slot.phase == GesturePhase::Idle && contact == nullptr) {
                if (slot.upEmitted) slot.upEmitted = false;
                continue;
            }
            if (slot.phase == GesturePhase::Idle && slot.upEmitted) {
                slot.upEmitted = false;
                if (contact) { contact->isReported = false; contact->reportEvent = TouchReportIdle; }
                continue;
            }
            UpdateSlot(slot, contact, i);
        }

        // Phase 2: Rewrite output fields
        for (auto& c : frame.contacts) {
            const int idx = c.id - 1;
            if (idx < 0 || idx >= kMaxSlots) continue;
            if (c.state == TouchStateUp) { c.isReported = false; continue; }
            if ((c.lifeFlags & TouchLifeSilentGap) != 0) {
                c.isReported = false;
                c.reportEvent = TouchReportIdle;
                continue;
            }
            const auto& slot = m_slots[idx];
            switch (slot.phase) {
            case GesturePhase::Idle:
                c.isReported = false; c.reportEvent = TouchReportIdle; break;
            case GesturePhase::PressCandidate:
                if (slot.stableFrames >= static_cast<uint16_t>(m_pressCandidateFrames)) {
                    c.isReported = true; c.reportEvent = TouchReportDown;
                    c.x = slot.lastOutputX; c.y = slot.lastOutputY;
                } else { c.isReported = false; c.reportEvent = TouchReportIdle; }
                break;
            case GesturePhase::Dragging:
                c.isReported = true; c.reportEvent = TouchReportMove;
                c.x = slot.lastOutputX; c.y = slot.lastOutputY; break;
            case GesturePhase::LongPressHold:
                c.isReported = true; c.reportEvent = TouchReportMove;
                c.x = slot.lastOutputX; c.y = slot.lastOutputY; break;
            case GesturePhase::ReleasePending:
                c.isReported = true; c.reportEvent = TouchReportMove;
                c.x = slot.lastOutputX; c.y = slot.lastOutputY; break;
            }
        }

        // Phase 3: ReleasePending → Idle (emit Up)
        for (int i = 0; i < kMaxSlots; ++i) {
            auto& slot = m_slots[i];
            if (slot.phase != GesturePhase::ReleasePending) continue;
            if (slot.missingFrames <= static_cast<uint16_t>(m_releasePendingFrames)) continue;
            TouchContact upEvent;
            upEvent.id = i + 1; upEvent.x = slot.lastOutputX; upEvent.y = slot.lastOutputY;
            upEvent.state = TouchStateUp; upEvent.area = slot.area;
            upEvent.signalSum = slot.signalSum; upEvent.sizeMm = slot.sizeMm;
            upEvent.isEdge = slot.isEdge; upEvent.isReported = true;
            upEvent.reportEvent = TouchReportUp;
            upEvent.lifeFlags = TouchLifeLiftOff; upEvent.reportFlags = 0;
            frame.contacts.push_back(upEvent);
            slot.Reset(); slot.upEmitted = true;
        }
        return true;
    }

private:
    std::array<GestureSlot, kMaxSlots> m_slots{};

    inline void UpdateSlot(GestureSlot& slot, const TouchContact* contact, int) {
        switch (slot.phase) {
        case GesturePhase::Idle:
            if (contact) {
                slot.phase = GesturePhase::PressCandidate;
                slot.anchorX = contact->x; slot.anchorY = contact->y;
                slot.lastTrackedX = contact->x; slot.lastTrackedY = contact->y;
                slot.lastOutputX = contact->x; slot.lastOutputY = contact->y;
                slot.ageFrames = 1; slot.missingFrames = 0; slot.stableFrames = 1;
                slot.sizeMm = contact->sizeMm; slot.signalSum = contact->signalSum;
                slot.area = contact->area; slot.isEdge = contact->isEdge;
                slot.quickTapEligible = true;
            }
            break;
        case GesturePhase::PressCandidate:
            if (!contact) {
                slot.prevPhase = GesturePhase::PressCandidate;
                slot.phase = GesturePhase::ReleasePending; slot.missingFrames = 1; return;
            }
            slot.ageFrames += 1; slot.missingFrames = 0;
            slot.lastTrackedX = contact->x; slot.lastTrackedY = contact->y;
            slot.sizeMm = contact->sizeMm; slot.signalSum = contact->signalSum;
            slot.area = contact->area; slot.isEdge = contact->isEdge;
            { bool stable = true;
              if (m_pressCandidateMinSignal > 0 && contact->signalSum < m_pressCandidateMinSignal) stable = false;
              if (m_pressCandidateMinSizeMm > 0.0f && contact->sizeMm < m_pressCandidateMinSizeMm) stable = false;
              slot.stableFrames = stable ? (slot.stableFrames + 1) : 0;
            }
            slot.lastOutputX = slot.anchorX; slot.lastOutputY = slot.anchorY;
            { float dx = contact->x - slot.anchorX, dy = contact->y - slot.anchorY;
              if (dx*dx+dy*dy > m_dragThreshold*m_dragThreshold) {
                  slot.phase = GesturePhase::Dragging; slot.quickTapEligible = false;
                  slot.lastOutputX = contact->x; slot.lastOutputY = contact->y; return;
              }
            }
            if (slot.ageFrames >= static_cast<uint16_t>(m_longPressFrames)) {
                float dx = contact->x - slot.anchorX, dy = contact->y - slot.anchorY;
                if (dx*dx+dy*dy <= m_longPressMoveTolerance*m_longPressMoveTolerance) {
                    slot.phase = GesturePhase::LongPressHold; slot.quickTapEligible = false;
                }
            }
            break;
        case GesturePhase::Dragging:
            if (!contact) {
                slot.prevPhase = GesturePhase::Dragging;
                slot.phase = GesturePhase::ReleasePending;
                slot.missingFrames = static_cast<uint16_t>(m_releasePendingFrames + 1); return;
            }
            slot.ageFrames += 1; slot.missingFrames = 0;
            slot.lastTrackedX = contact->x; slot.lastTrackedY = contact->y;
            slot.lastOutputX = contact->x; slot.lastOutputY = contact->y;
            slot.sizeMm = contact->sizeMm; slot.signalSum = contact->signalSum;
            slot.area = contact->area; slot.isEdge = contact->isEdge;
            break;
        case GesturePhase::LongPressHold:
            if (!contact) {
                slot.prevPhase = GesturePhase::LongPressHold;
                slot.phase = GesturePhase::ReleasePending;
                slot.missingFrames = static_cast<uint16_t>(m_releasePendingFrames + 1); return;
            }
            slot.ageFrames += 1; slot.missingFrames = 0;
            slot.lastTrackedX = contact->x; slot.lastTrackedY = contact->y;
            slot.sizeMm = contact->sizeMm; slot.signalSum = contact->signalSum;
            slot.area = contact->area; slot.isEdge = contact->isEdge;
            { float dx = contact->x - slot.anchorX, dy = contact->y - slot.anchorY;
              if (dx*dx+dy*dy > m_dragThreshold*m_dragThreshold) {
                  slot.phase = GesturePhase::Dragging;
                  slot.lastOutputX = contact->x; slot.lastOutputY = contact->y; return;
              }
            }
            slot.lastOutputX = slot.anchorX; slot.lastOutputY = slot.anchorY;
            break;
        case GesturePhase::ReleasePending:
            if (contact) {
                slot.phase = slot.prevPhase; slot.missingFrames = 0;
                slot.lastTrackedX = contact->x; slot.lastTrackedY = contact->y;
                slot.sizeMm = contact->sizeMm; slot.signalSum = contact->signalSum;
                slot.area = contact->area; slot.isEdge = contact->isEdge;
                if (slot.phase == GesturePhase::Dragging) { slot.lastOutputX = contact->x; slot.lastOutputY = contact->y; }
                return;
            }
            slot.missingFrames += 1;
            break;
        }
    }
};

}} // namespace Solvers::Touch
