#pragma once
#include <algorithm>
#include <cstdint>

namespace Asa {

/// PenStateMachine — Pen lifecycle tracker + TSACore 3-bit ASA status word.
///
/// Combines two closely related state machines:
///   1. PenLifecycle: Leave → Hover → Contact → Lifting → Leave
///   2. ASA Status:   3-bit word {InRange, Ink, NoPressInk} with frame counters
///
/// The ASA status controls IIR skip logic and Still/Moving coefficient selection.
class PenStateMachine {
public:
    enum class Lifecycle : uint8_t {
        Leave = 0,    // Pen out of detection range
        Hover,        // Position valid, no pressure
        Contact,      // Position valid + pressure active
        Lifting,      // Transition: pressure just released (debounce hold)
    };

    // Status bit constants (mirrors TSACore DAT_18231950)
    static constexpr uint8_t kStatInRange    = 0x01;  // bit0: pen detected (hover)
    static constexpr uint8_t kStatInk        = 0x02;  // bit1: pressure active (writing)
    static constexpr uint8_t kStatNoPressInk = 0x04;  // bit2: coord valid, no pressure

    /// Update both state machines for the current frame.
    /// @param coordValid  Whether coordinate solve succeeded
    /// @param hasInk      Whether pressure > 0
    /// @param[out] animState  Output lifecycle byte for diagnostics
    inline void Update(bool coordValid, bool hasInk, uint8_t& animState) {
        UpdateLifecycle(coordValid, hasInk);
        UpdateAsaStatus(coordValid, hasInk);
        animState = static_cast<uint8_t>(m_lifecycle);
    }

    /// Check if pen just left range (for triggering resets in pipeline)
    inline bool JustLeftRange() const {
        return !(m_asaStatus & kStatInRange) && (m_prevAsaStatus & kStatInRange);
    }

    /// Check if IIR should be skipped this frame (first 2 frames of mode transition)
    inline bool ShouldSkipIIR() const {
        return ((m_prevAsaStatus & kStatInRange) == 0 || m_inRangeFrames < 2)
            && m_inkFrames < 2
            && m_noPressInkFrames < 2;
    }

    /// Check if currently inking (for IIR coefficient selection)
    inline bool IsInking() const {
        return (m_asaStatus & (kStatInk | kStatNoPressInk)) != 0;
    }

    // Diagnostic accessors
    Lifecycle GetLifecycle() const { return m_lifecycle; }
    uint8_t   GetAsaStatus() const { return m_asaStatus; }
    uint8_t   GetPrevAsaStatus() const { return m_prevAsaStatus; }
    int       GetInRangeFrames() const { return m_inRangeFrames; }
    int       GetInkFrames() const { return m_inkFrames; }
    int       GetNoPressInkFrames() const { return m_noPressInkFrames; }

    // ── Configuration ──
    int liftingTimeout = 10;

private:
    Lifecycle m_lifecycle = Lifecycle::Leave;
    int m_liftingFrameCount = 0;

    uint8_t m_asaStatus = 0;
    uint8_t m_prevAsaStatus = 0;
    int     m_inRangeFrames = 0;
    int     m_inkFrames = 0;
    int     m_noPressInkFrames = 0;

    inline void UpdateLifecycle(bool penValid, bool penDown) {
        switch (m_lifecycle) {
        case Lifecycle::Leave:
            if (penValid) m_lifecycle = Lifecycle::Hover;
            break;
        case Lifecycle::Hover:
            if (!penValid) {
                m_lifecycle = Lifecycle::Leave;
            } else if (penDown) {
                m_lifecycle = Lifecycle::Contact;
                m_liftingFrameCount = 0;
            }
            break;
        case Lifecycle::Contact:
            if (!penDown) {
                m_lifecycle = Lifecycle::Lifting;
                m_liftingFrameCount = 0;
            }
            break;
        case Lifecycle::Lifting:
            m_liftingFrameCount++;
            if (penDown) {
                m_lifecycle = Lifecycle::Contact;
                m_liftingFrameCount = 0;
            } else if (!penValid || m_liftingFrameCount > liftingTimeout) {
                m_lifecycle = Lifecycle::Leave;
            }
            break;
        }
    }

    inline void UpdateAsaStatus(bool coordValid, bool hasInk) {
        m_prevAsaStatus = m_asaStatus;
        m_asaStatus = 0;
        if (coordValid) m_asaStatus |= kStatInRange;
        if (hasInk)     m_asaStatus |= kStatInk;
        if (coordValid) m_asaStatus |= kStatNoPressInk;

        // NoPressInk mode
        if (m_asaStatus & kStatNoPressInk) {
            m_noPressInkFrames++;
            m_inRangeFrames = 0;
        } else {
            m_noPressInkFrames = 0;
        }

        // Ink mode
        if (m_asaStatus & kStatInk) {
            m_inkFrames++;
            m_inRangeFrames = 0;
        } else {
            m_inkFrames = 0;
        }

        // InRange mode
        if (m_asaStatus & kStatInRange) {
            m_inRangeFrames++;
        }

        // Exit range → full reset (TSACore: ExitInRangeMode → CoorInit)
        // Note: actual reset is triggered by caller via JustLeftRange()
    }
};

} // namespace Asa
