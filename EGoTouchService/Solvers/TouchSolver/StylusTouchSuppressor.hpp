#pragma once

#include "SolverTypes.h"
#include <algorithm>
#include <cmath>

namespace Solvers { namespace Touch {

class StylusTouchSuppressor {
public:
    bool  m_stylusSuppressGlobalEnabled = false;
    bool  m_stylusSuppressLocalEnabled = false;
    float m_stylusSuppressLocalDistance = 2.5f;
    int   m_stylusSuppressPenPeakThreshold = 1500;
    int   m_stylusSuppressTouchSignalKeep = 6000;
    int   m_stylusSuppressTouchAreaKeep = 12;
    bool  m_stylusAftEnabled = false;
    int   m_stylusAftDebounceFrames = 3;
    int   m_stylusAftWeakSignalThreshold = 240;
    float m_stylusAftWeakSizeThresholdMm = 1.2f;
    int   m_stylusAftSuppressFrames = 40;
    float m_fallbackSizeMm = 1.0f;
    float m_sizeAreaScale = 0.22f;
    float m_sizeSignalScale = 0.35f;

    inline bool Process(HeatmapFrame& frame);

private:
    static constexpr float kStylusTouchCoordScale = 1.0f / 1024.0f;

    struct StylusNoiseEvidence {
        bool pointValid = false;
        bool writingLike = false;
        bool stable = false;
        bool active = false;
        bool tx2Strong = false;
        bool tx2Dominant = false;
        float x = 0.0f;
        float y = 0.0f;
        int signalX = 0;
        int signalY = 0;
        int maxRawPeak = 0;
    };

    static inline float DistanceSq(float x1, float y1, float x2, float y2);
    inline float EstimateSizeMm(const TouchContact& touch) const;
    inline StylusNoiseEvidence BuildStylusNoiseEvidence(const HeatmapFrame& frame,
                                                        int recheckThreshold) const;
    inline bool IsStrongTouchCandidate(const TouchContact& touch) const;
};

inline float StylusTouchSuppressor::DistanceSq(float x1, float y1, float x2, float y2) {
    const float dx = x1 - x2;
    const float dy = y1 - y2;
    return dx * dx + dy * dy;
}

inline float StylusTouchSuppressor::EstimateSizeMm(const TouchContact& touch) const {
    if (touch.sizeMm > 0.0f) return touch.sizeMm;
    if (touch.signalSum > 0)
        return std::max(m_fallbackSizeMm, std::cbrt(static_cast<float>(touch.signalSum)) * m_sizeSignalScale);
    if (touch.area > 0)
        return std::max(m_fallbackSizeMm, std::sqrt(static_cast<float>(touch.area)) * m_sizeAreaScale);
    return m_fallbackSizeMm;
}

inline StylusTouchSuppressor::StylusNoiseEvidence StylusTouchSuppressor::BuildStylusNoiseEvidence(
    const HeatmapFrame& frame,
    int recheckThreshold) const {
    StylusNoiseEvidence evidence;
    const auto& output = frame.stylus.output;
    const auto& interop = frame.stylus.interop;
    evidence.signalX = static_cast<int>(interop.signalX);
    evidence.signalY = static_cast<int>(interop.signalY);
    evidence.maxRawPeak = std::max({
        evidence.signalX,
        evidence.signalY,
        static_cast<int>(interop.maxRawPeak)
    });

    if (!(output.inRange && output.point.valid)) {
        return evidence;
    }

    evidence.pointValid = true;
    evidence.x = output.point.x * kStylusTouchCoordScale;
    evidence.y = output.point.y * kStylusTouchCoordScale;

    evidence.writingLike =
        output.tipDown ||
        (output.pressure > 0);
    evidence.tx2Strong = evidence.signalY >= recheckThreshold;
    evidence.tx2Dominant =
        (evidence.signalY > 0) &&
        (evidence.signalY * 4 >= std::max(1, evidence.signalX) * 3);
    const int sustainThreshold = std::max(64, recheckThreshold / 2);
    evidence.stable =
        evidence.pointValid &&
        interop.recheckPassed &&
        (evidence.maxRawPeak >= sustainThreshold) &&
        (evidence.writingLike || evidence.tx2Strong || evidence.tx2Dominant);
    evidence.active =
        evidence.stable &&
        (evidence.tx2Strong ||
         ((evidence.signalY >= sustainThreshold) && evidence.tx2Dominant));
    return evidence;
}

inline bool StylusTouchSuppressor::IsStrongTouchCandidate(const TouchContact& touch) const {
    return (touch.signalSum >= m_stylusSuppressTouchSignalKeep) &&
           (touch.area >= m_stylusSuppressTouchAreaKeep);
}

inline bool StylusTouchSuppressor::Process(HeatmapFrame& frame) {
    auto& interop = frame.stylus.interop;
    const bool localEnabled = m_stylusSuppressGlobalEnabled && m_stylusSuppressLocalEnabled;
    const bool aftEnabled = m_stylusSuppressGlobalEnabled && m_stylusAftEnabled;
    if (!localEnabled && !aftEnabled) {
        interop.recheckOverlap = false;
        interop.touchNullLike = false;
        interop.touchSuppressActive = false;
        interop.touchSuppressFrames = 0;
        return false;
    }

    interop.recheckEnabled = interop.recheckEnabled || localEnabled || aftEnabled;
    interop.recheckOverlap = false;
    const int baseThreshold =
        (interop.recheckThreshold > 0)
            ? static_cast<int>(interop.recheckThreshold)
            : m_stylusSuppressPenPeakThreshold;
    const int multiThreshold =
        (interop.recheckThresholdMulti > 0)
            ? static_cast<int>(interop.recheckThresholdMulti)
            : std::max(baseThreshold, 1200);
    const int finalThreshold =
        (frame.touch.output.contacts.size() > 2) ? multiThreshold : baseThreshold;
    interop.recheckThreshold =
        static_cast<uint16_t>(std::clamp(finalThreshold, 0, 0xFFFF));
    interop.touchNullLike = false;
    interop.touchSuppressActive = false;
    interop.touchSuppressFrames = 0;

    const StylusNoiseEvidence evidence =
        BuildStylusNoiseEvidence(frame, finalThreshold);
    interop.recheckPassed = interop.recheckPassed && evidence.stable;
    if (!localEnabled || !evidence.pointValid) return false;

    const float radiusSq = m_stylusSuppressLocalDistance * m_stylusSuppressLocalDistance;
    const float overlapRadius = std::min(m_stylusSuppressLocalDistance, 1.25f);
    const float overlapRadiusSq = overlapRadius * overlapRadius;
    int suppressedCount = 0;
    int holdFrames = 0;

    frame.touch.output.contacts.erase(std::remove_if(frame.touch.output.contacts.begin(), frame.touch.output.contacts.end(),
        [&](const TouchContact& c) {
            const float distSq = DistanceSq(c.x, c.y, evidence.x, evidence.y);
            if (distSq > radiusSq) return false;

            const bool overlap = distSq <= overlapRadiusSq;
            interop.recheckOverlap = interop.recheckOverlap || overlap;

            if (!evidence.active) return false;

            const float sizeMm = EstimateSizeMm(c);
            const bool strongTouch = IsStrongTouchCandidate(c);
            const bool weakTouch =
                (c.signalSum < m_stylusAftWeakSignalThreshold) ||
                (sizeMm < m_stylusAftWeakSizeThresholdMm) ||
                (c.area < std::max(1, m_stylusSuppressTouchAreaKeep / 2));
            const bool suppressNow =
                overlap &&
                !strongTouch &&
                (weakTouch || evidence.tx2Strong);

            if (!suppressNow) return false;

            ++suppressedCount;
            holdFrames = std::max(holdFrames,
                                  overlap ? m_stylusAftSuppressFrames
                                          : std::max(1, m_stylusAftDebounceFrames));
            return true;
        }), frame.touch.output.contacts.end());

    interop.touchNullLike =
        evidence.active && interop.recheckOverlap;
    interop.touchSuppressActive =
        evidence.active || suppressedCount > 0;
    if (interop.recheckOverlap && evidence.active) {
        interop.recheckPassed = false;
    }
    interop.touchSuppressFrames = static_cast<uint8_t>(
        std::clamp(holdFrames, 0, 255));
    return false;
}

}} // namespace Solvers::Touch

