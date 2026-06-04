#pragma once

#include "SolverTypes.h"
#include "GhostSuppressor.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <vector>

namespace Solvers { namespace Touch {

class TouchTracker {
public:
    bool m_enabled = true;
    int   m_maxTouchCount = 20;
    float m_maxTrackDistance = 4.985f;
    float m_alwaysMatchDistance = 2.0f;
    float m_edgeTrackBoost = 1.5f;
    float m_accThresholdBoost = 4.0f;
    float m_accBoostSizeMm = 1.6f;
    float m_predictionScale = 1.0f;
    bool  m_gapRelinkEnabled = true;
    int   m_gapRelinkWindowFrames = 4;
    int   m_touchDownDebounceFrames = 1;
    bool  m_dynamicDebounceEnabled = true;
    int   m_touchDownDebounceMaxExtra = 4;
    int   m_touchDownWeakSignalThreshold = 180;
    float m_touchDownSmallSizeThresholdMm = 1.3f;
    bool  m_touchDownRejectEnabled = true;
    int   m_touchDownRejectMinSignal = 55;
    float m_touchDownRejectMinSizeMm = 0.95f;
    int   m_touchDownEdgeRejectMinSignal = 90;
    float m_fallbackSizeMm = 1.0f;
    float m_sizeAreaScale = 0.22f;
    float m_sizeSignalScale = 0.35f;
    bool  m_rxGhostFilterEnabled = true;
    int   m_rxGhostLineDelta = 0;
    float m_rxGhostWeakRatio = 0.5f;
    bool  m_rxGhostOnlyNew = true;
    bool  m_useHungarian = true;

    bool  m_stylusSuppressGlobalEnabled = true;
    bool  m_stylusSuppressLocalEnabled = true;
    float m_stylusSuppressLocalDistance = 2.5f;
    int   m_stylusSuppressPenPeakThreshold = 1500;
    int   m_stylusSuppressTouchSignalKeep = 6000;
    int   m_stylusSuppressTouchAreaKeep = 12;
    bool  m_stylusAftEnabled = true;
    int   m_stylusAftRecentFrames = 24;
    float m_stylusAftRadius = 2.8f;
    int   m_stylusAftDebounceFrames = 3;
    int   m_stylusAftWeakSignalThreshold = 240;
    float m_stylusAftWeakSizeThresholdMm = 1.2f;
    int   m_stylusAftSuppressFrames = 40;
    int   m_stylusAftPalmSuppressFrames = 100;
    int   m_stylusAftPalmAreaThreshold = 20;
    float m_stylusAftPalmSizeThresholdMm = 2.5f;

    inline bool Process(HeatmapFrame& frame);
    inline bool HasLiveTracks() const { return m_trackCount > 0; }

private:
    static constexpr int kMaxTracks = 20;
    static constexpr float kGapRelinkSecondBestMarginSq = 4.0f;
    static constexpr float kGapRelinkSecondBestRatio = 1.75f;
    static constexpr float kGapRelinkSpeedWindowScale = 0.75f;
    static constexpr float kGapRelinkHardCapScale = 2.0f;
    static constexpr float kActiveBootstrapGateScale = 1.25f;
    static constexpr float kStylusTouchCoordScale = 1.0f / 1024.0f;

    enum class TrackPhase : uint8_t { Active = 0, SilentGap = 1 };

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

    struct TrackState {
        int id = 0;
        float x = 0, y = 0, vx = 0, vy = 0;
        int area = 0, signalSum = 0;
        float sizeMm = 0;
        int age = 0, missed = 0, downDebounceFrames = 0;
        bool upEventEmitted = false;
        bool isEdge = false;
        uint32_t edgeFlags = 0;
        uint8_t centroidEdgeFlags = 0;
        uint32_t ecFlags = 0;
        float edgeDistX = 0.0f;
        float edgeDistY = 0.0f;
        float rawXBeforeEC = 0.0f;
        float rawYBeforeEC = 0.0f;
        uint8_t ecWidthX = 0;
        uint8_t ecWidthY = 0;
        int stylusSuppressFrames = 0;
        TrackPhase phase = TrackPhase::Active;
        int gapFrames = 0;
    };

    TrackState m_tracks[kMaxTracks];
    int m_trackCount = 0;
    int m_nextIdSeed = 1;
    int m_stylusFramesSinceActive = 1000000;
    float m_lastStylusX = 0, m_lastStylusY = 0;

    static inline float DistanceSq(float x1, float y1, float x2, float y2);
    static inline bool IsEdgeTouch(float x, float y, int cols, int rows, float em);
    static inline bool IsEdgeContact(const TouchContact& touch, int cols, int rows, float em);
    static inline void StoreEdgeMetadata(TrackState& track, const TouchContact& touch);
    static inline void RestoreEdgeMetadata(TouchContact& touch, const TrackState& track);
    inline void GetMatchReference(const TrackState& track, float& outX, float& outY) const;
    inline float EstimateSizeMm(int area, int signalSum) const;
    inline int ComputeTouchDownDebounceFrames(const TouchContact& touch) const;
    inline float ComputeTrackGateSq(const TrackState& track, const TouchContact& contact, int cols, int rows, float edgeMargin) const;
    inline bool PassActiveBootstrapGate(const TrackState& track, const TouchContact& contact, int cols, int rows, float edgeMargin) const;
    inline void MatchAgainstSubset(const std::vector<TouchContact>& contacts, int curCount, const int* prevSubset, int subsetCount, int* curToPre) const;
    static inline void UpdateBestCandidate(float distance, int candidateId, float& best, float& second, int& bestId);
    inline bool PassGapRelinkAmbiguity(float best, float second) const;
    inline TouchContact BuildSilentGapContact(const TrackState& track, int prevIndex, int cols, int rows, float edgeMargin) const;
    inline TouchContact BuildLiftOffContact(const TrackState& track, int prevIndex, int cols, int rows, float edgeMargin) const;
    inline int AllocateId(const TrackState* reservedNextTracks, int reservedCount) const;
    inline StylusNoiseEvidence BuildStylusNoiseEvidence(const HeatmapFrame& frame,
                                                        int recheckThreshold) const;
    inline bool IsStrongTouchCandidate(const TouchContact& touch) const;
    inline bool ResolveStylusAftContext(const HeatmapFrame& frame, float& outX, float& outY);
    inline bool ShouldStylusAftSuppress(const TouchContact& touch, int touchAge, float stylusX, float stylusY, int& outHoldFrames) const;
    inline void SolveAssignment(const float* cost, int n, int m, int* rowToCol) const;
};

inline float TouchTracker::DistanceSq(float x1, float y1, float x2, float y2) {
    const float dx = x1 - x2;
    const float dy = y1 - y2;
    return dx * dx + dy * dy;
}

inline bool TouchTracker::IsEdgeTouch(float x, float y, int cols, int rows, float em) {
    return (x <= em) || (y <= em) || (x >= float(cols) - em) || (y >= float(rows) - em);
}

inline bool TouchTracker::IsEdgeContact(const TouchContact& touch, int cols, int rows, float em) {
    return touch.isEdge ||
           IsEdgeTouch(touch.x, touch.y, cols, rows, em) ||
           (touch.edgeFlags & (0x20 | 0x80000)) != 0 ||
           touch.centroidEdgeFlags != 0;
}

inline void TouchTracker::StoreEdgeMetadata(TrackState& track, const TouchContact& touch) {
    track.isEdge = touch.isEdge ||
                   (touch.edgeFlags & (0x20 | 0x80000)) != 0 ||
                   touch.centroidEdgeFlags != 0;
    track.edgeFlags = touch.edgeFlags;
    track.centroidEdgeFlags = touch.centroidEdgeFlags;
    track.ecFlags = touch.ecFlags;
    track.edgeDistX = touch.edgeDistX;
    track.edgeDistY = touch.edgeDistY;
    track.rawXBeforeEC = touch.rawXBeforeEC;
    track.rawYBeforeEC = touch.rawYBeforeEC;
    track.ecWidthX = touch.ecWidthX;
    track.ecWidthY = touch.ecWidthY;
}

inline void TouchTracker::RestoreEdgeMetadata(TouchContact& touch, const TrackState& track) {
    touch.isEdge = touch.isEdge || track.isEdge;
    touch.edgeFlags = track.edgeFlags;
    touch.centroidEdgeFlags = track.centroidEdgeFlags;
    touch.ecFlags = track.ecFlags;
    touch.edgeDistX = track.edgeDistX;
    touch.edgeDistY = track.edgeDistY;
    touch.rawXBeforeEC = track.rawXBeforeEC;
    touch.rawYBeforeEC = track.rawYBeforeEC;
    touch.ecWidthX = track.ecWidthX;
    touch.ecWidthY = track.ecWidthY;
}

inline void TouchTracker::GetMatchReference(const TrackState& track, float& outX, float& outY) const {
    float framesAhead = m_predictionScale;
    if (track.phase == TrackPhase::SilentGap) {
        framesAhead = m_predictionScale * static_cast<float>(std::max(1, track.gapFrames + 1));
    }
    outX = track.x + track.vx * framesAhead;
    outY = track.y + track.vy * framesAhead;
}

inline float TouchTracker::EstimateSizeMm(int area, int signalSum) const {
    if (signalSum > 0)
        return std::max(m_fallbackSizeMm, std::cbrt(static_cast<float>(signalSum)) * m_sizeSignalScale);
    if (area > 0)
        return std::max(m_fallbackSizeMm, std::sqrt(static_cast<float>(area)) * m_sizeAreaScale);
    return m_fallbackSizeMm;
}

inline int TouchTracker::ComputeTouchDownDebounceFrames(const TouchContact& touch) const {
    int frames = m_touchDownDebounceFrames;
    if (!m_dynamicDebounceEnabled) return frames;
    int extra = 0;
    if (touch.signalSum > 0 && touch.signalSum < m_touchDownWeakSignalThreshold) extra += 1;
    if (touch.sizeMm > 0.0f && touch.sizeMm < m_touchDownSmallSizeThresholdMm) extra += 1;
    if (touch.isEdge) extra += 1;
    return frames + std::clamp(extra, 0, m_touchDownDebounceMaxExtra);
}

inline float TouchTracker::ComputeTrackGateSq(const TrackState& track,
                                              const TouchContact& contact,
                                              int cols,
                                              int rows,
                                              float edgeMargin) const {
    float gateDist = m_maxTrackDistance;
    const bool edge = track.isEdge ||
                      IsEdgeTouch(track.x, track.y, cols, rows, edgeMargin) ||
                      IsEdgeContact(contact, cols, rows, edgeMargin);
    if (edge) gateDist *= m_edgeTrackBoost;
    const float sizeMm = std::max(track.sizeMm, EstimateSizeMm(contact.area, contact.signalSum));
    if (edge || sizeMm <= m_accBoostSizeMm) gateDist *= m_accThresholdBoost;
    if (track.phase == TrackPhase::SilentGap) {
        const float speed = std::sqrt(track.vx * track.vx + track.vy * track.vy);
        const float extra = std::min(m_maxTrackDistance,
                                     speed * kGapRelinkSpeedWindowScale *
                                     static_cast<float>(std::max(1, track.gapFrames)));
        gateDist = std::min(gateDist + extra, m_maxTrackDistance * kGapRelinkHardCapScale);
    }
    return gateDist * gateDist;
}

inline bool TouchTracker::PassActiveBootstrapGate(const TrackState& track,
                                                  const TouchContact& contact,
                                                  int cols,
                                                  int rows,
                                                  float edgeMargin) const {
    if (track.phase != TrackPhase::Active || track.age > 1 || track.missed != 0) return false;
    if (track.isEdge || IsEdgeTouch(track.x, track.y, cols, rows, edgeMargin)) return false;
    if (IsEdgeContact(contact, cols, rows, edgeMargin)) return false;
    if (contact.signalSum < m_touchDownWeakSignalThreshold * 2) return false;
    if (EstimateSizeMm(contact.area, contact.signalSum) <= m_touchDownSmallSizeThresholdMm) return false;

    float refX = 0.0f, refY = 0.0f;
    GetMatchReference(track, refX, refY);
    const float bootstrapGate = m_maxTrackDistance * kActiveBootstrapGateScale;
    return DistanceSq(contact.x, contact.y, refX, refY) <= bootstrapGate * bootstrapGate;
}

inline void TouchTracker::MatchAgainstSubset(const std::vector<TouchContact>& contacts,
                                             int curCount,
                                             const int* prevSubset,
                                             int subsetCount,
                                             int* curToPre) const {
    if (curCount <= 0 || subsetCount <= 0) return;
    constexpr int kRows = 40, kCols = 60;
    constexpr float kEdgeMargin = 2.0f;
    constexpr float kInvalidMatchCost = 1.0e12f;
    float cost[kMaxTracks * kMaxTracks];
    for (int c = 0; c < curCount; ++c) {
        for (int i = 0; i < subsetCount; ++i) {
            const int pre = prevSubset[i];
            float refX = 0.0f, refY = 0.0f;
            GetMatchReference(m_tracks[pre], refX, refY);
            const float d = DistanceSq(contacts[c].x, contacts[c].y, refX, refY);
            const float gateSq = ComputeTrackGateSq(m_tracks[pre], contacts[c], kCols, kRows, kEdgeMargin);
            cost[c * kMaxTracks + i] = (d <= gateSq) ? d : kInvalidMatchCost;
        }
    }

    int rowToSubset[kMaxTracks];
    for (int i = 0; i < curCount; ++i) rowToSubset[i] = -1;
    if (m_useHungarian) {
        SolveAssignment(cost, curCount, subsetCount, rowToSubset);
    } else {
        bool prevUsed[kMaxTracks] = {};
        for (int c = 0; c < curCount; ++c) {
            float best = std::numeric_limits<float>::max();
            int bestIdx = -1;
            for (int i = 0; i < subsetCount; ++i) {
                if (prevUsed[i]) continue;
                if (cost[c * kMaxTracks + i] < best) {
                    best = cost[c * kMaxTracks + i];
                    bestIdx = i;
                }
            }
            if (bestIdx >= 0 && best < kInvalidMatchCost) {
                prevUsed[bestIdx] = true;
                rowToSubset[c] = bestIdx;
            }
        }
    }

    for (int c = 0; c < curCount; ++c) {
        const int subset = rowToSubset[c];
        if (subset >= 0 && cost[c * kMaxTracks + subset] < kInvalidMatchCost) {
            curToPre[c] = prevSubset[subset];
        }
    }
}

inline void TouchTracker::UpdateBestCandidate(float distance,
                                              int candidateId,
                                              float& best,
                                              float& second,
                                              int& bestId) {
    if (distance < best) {
        second = best;
        best = distance;
        bestId = candidateId;
    } else if (distance < second) {
        second = distance;
    }
}

inline bool TouchTracker::PassGapRelinkAmbiguity(float best, float second) const {
    if (!std::isfinite(best)) return false;
    if (!std::isfinite(second)) return true;
    return second >= best + kGapRelinkSecondBestMarginSq ||
           second >= best * kGapRelinkSecondBestRatio;
}

inline TouchContact TouchTracker::BuildSilentGapContact(const TrackState& track,
                                                        int prevIndex,
                                                        int cols,
                                                        int rows,
                                                        float edgeMargin) const {
    float hiddenX = track.x;
    float hiddenY = track.y;
    const bool predicted = m_predictionScale > 0.0f;
    if (predicted) {
        const float framesAhead = m_predictionScale * static_cast<float>(std::max(1, track.gapFrames));
        hiddenX = std::clamp(track.x + track.vx * framesAhead, 0.0f, static_cast<float>(cols));
        hiddenY = std::clamp(track.y + track.vy * framesAhead, 0.0f, static_cast<float>(rows));
    }

    TouchContact hidden;
    hidden.id = track.id;
    hidden.x = hiddenX;
    hidden.y = hiddenY;
    hidden.state = TouchStateMove;
    hidden.area = track.area;
    hidden.signalSum = track.signalSum;
    hidden.sizeMm = track.sizeMm;
    RestoreEdgeMetadata(hidden, track);
    hidden.isEdge = hidden.isEdge || IsEdgeTouch(hidden.x, hidden.y, cols, rows, edgeMargin);
    hidden.isReported = false;
    hidden.prevIndex = prevIndex;
    hidden.debugFlags = predicted ? 0x28 : 0x20;
    hidden.lifeFlags = TouchLifeMapped | TouchLifeSilentGap;
    if (hidden.isEdge) hidden.lifeFlags |= TouchLifeEdge;
    hidden.reportFlags = 0;
    hidden.reportEvent = TouchReportIdle;
    return hidden;
}

inline TouchContact TouchTracker::BuildLiftOffContact(const TrackState& track,
                                                      int prevIndex,
                                                      int cols,
                                                      int rows,
                                                      float edgeMargin) const {
    TouchContact up;
    up.id = track.id;
    up.x = track.x;
    up.y = track.y;
    up.state = TouchStateUp;
    up.area = track.area;
    up.signalSum = track.signalSum;
    up.sizeMm = track.sizeMm;
    RestoreEdgeMetadata(up, track);
    up.isEdge = up.isEdge || IsEdgeTouch(up.x, up.y, cols, rows, edgeMargin);
    up.isReported = true;
    up.prevIndex = prevIndex;
    up.debugFlags = 0x04;
    up.lifeFlags = TouchLifeLiftOff;
    if (up.isEdge) up.lifeFlags |= TouchLifeEdge;
    up.reportFlags = 0;
    up.reportEvent = TouchReportUp;
    return up;
}

inline int TouchTracker::AllocateId(const TrackState* reservedNextTracks, int reservedCount) const {
    for (int i = 0; i < kMaxTracks; ++i) {
        const int candidate = ((m_nextIdSeed - 1 + i) % kMaxTracks) + 1;
        bool used = false;
        for (int j = 0; j < reservedCount; ++j) {
            if (reservedNextTracks[j].id == candidate) {
                used = true;
                break;
            }
        }
        if (used) continue;
        for (int j = 0; j < m_trackCount; ++j) {
            if (m_tracks[j].id == candidate) {
                used = true;
                break;
            }
        }
        if (!used) return candidate;
    }
    return 0;
}

inline TouchTracker::StylusNoiseEvidence TouchTracker::BuildStylusNoiseEvidence(
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

inline bool TouchTracker::IsStrongTouchCandidate(const TouchContact& touch) const {
    return (touch.signalSum >= m_stylusSuppressTouchSignalKeep) &&
           (touch.area >= m_stylusSuppressTouchAreaKeep);
}

inline bool TouchTracker::ResolveStylusAftContext(const HeatmapFrame& frame, float& outX, float& outY) {
    if (!m_stylusAftEnabled) return false;
    const auto& interop = frame.stylus.interop;
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
    const StylusNoiseEvidence evidence =
        BuildStylusNoiseEvidence(frame, finalThreshold);
    if (evidence.pointValid && evidence.stable) {
        m_lastStylusX = evidence.x;
        m_lastStylusY = evidence.y;
        m_stylusFramesSinceActive = 0;
    } else if (m_stylusFramesSinceActive < 1000000) {
        m_stylusFramesSinceActive += 1;
    }
    if (m_stylusFramesSinceActive > m_stylusAftRecentFrames) return false;
    outX = m_lastStylusX;
    outY = m_lastStylusY;
    return true;
}

inline bool TouchTracker::ShouldStylusAftSuppress(const TouchContact& touch,
                                                  int touchAge,
                                                  float stylusX,
                                                  float stylusY,
                                                  int& outHoldFrames) const {
    outHoldFrames = 0;
    if (!m_stylusAftEnabled) return false;
    if (DistanceSq(touch.x, touch.y, stylusX, stylusY) > m_stylusAftRadius * m_stylusAftRadius) return false;
    if (IsStrongTouchCandidate(touch)) return false;
    const bool palm = (touch.area >= m_stylusAftPalmAreaThreshold) || (touch.sizeMm >= m_stylusAftPalmSizeThresholdMm);
    const bool weak = (touch.signalSum < m_stylusAftWeakSignalThreshold) && (touch.sizeMm < m_stylusAftWeakSizeThresholdMm);
    const bool young = (touchAge <= m_stylusAftDebounceFrames);
    if (palm) {
        outHoldFrames = m_stylusAftPalmSuppressFrames;
        return true;
    }
    if (weak || young) {
        outHoldFrames = m_stylusAftSuppressFrames;
        return true;
    }
    return false;
}

inline void TouchTracker::SolveAssignment(const float* cost, int n, int m, int* rowToCol) const {
    if (n == 0 || m == 0) {
        for (int i = 0; i < n; ++i) rowToCol[i] = -1;
        return;
    }

    bool transposed = false;
    float matrix[kMaxTracks * kMaxTracks];
    int origN = n, origM = m;
    if (n > m) {
        transposed = true;
        for (int r = 0; r < n; ++r)
            for (int c2 = 0; c2 < m; ++c2)
                matrix[c2 * kMaxTracks + r] = cost[r * kMaxTracks + c2];
        std::swap(n, m);
    } else {
        for (int r = 0; r < n; ++r)
            for (int c2 = 0; c2 < m; ++c2)
                matrix[r * kMaxTracks + c2] = cost[r * kMaxTracks + c2];
    }

    const float kInf = std::numeric_limits<float>::max() / 8.0f;
    float u[kMaxTracks + 1] = {}, v[kMaxTracks + 1] = {};
    int p[kMaxTracks + 1] = {}, way[kMaxTracks + 1] = {};
    for (int i = 1; i <= n; ++i) {
        p[0] = i;
        int j0 = 0;
        float minv[kMaxTracks + 1];
        bool used[kMaxTracks + 1] = {};
        for (int j = 0; j <= m; ++j) minv[j] = kInf;
        do {
            used[j0] = true;
            const int i0 = p[j0];
            float delta = kInf;
            int j1 = 0;
            for (int j = 1; j <= m; ++j) {
                if (used[j]) continue;
                const float cur = matrix[(i0 - 1) * kMaxTracks + (j - 1)] - u[i0] - v[j];
                if (cur < minv[j]) {
                    minv[j] = cur;
                    way[j] = j0;
                }
                if (minv[j] < delta) {
                    delta = minv[j];
                    j1 = j;
                }
            }
            for (int j = 0; j <= m; ++j) {
                if (used[j]) {
                    u[p[j]] += delta;
                    v[j] -= delta;
                } else {
                    minv[j] -= delta;
                }
            }
            j0 = j1;
        } while (p[j0] != 0);
        do {
            const int j1 = way[j0];
            p[j0] = p[j1];
            j0 = j1;
        } while (j0 != 0);
    }

    if (!transposed) {
        for (int i = 0; i < n; ++i) rowToCol[i] = -1;
        for (int j = 1; j <= m; ++j)
            if (p[j] != 0) rowToCol[p[j] - 1] = j - 1;
    } else {
        for (int i = 0; i < origN; ++i) rowToCol[i] = -1;
        for (int prev = 0; prev < origM; ++prev) {
            int idx = -1;
            for (int j = 1; j <= m; ++j) {
                if (p[j] - 1 == prev) {
                    idx = j - 1;
                    break;
                }
            }
            if (idx >= 0 && idx < origN) rowToCol[idx] = prev;
        }
    }
}

inline bool TouchTracker::Process(HeatmapFrame& frame) {
    if (!m_enabled) {
        int nextId = 1;
        for (auto& c : frame.touch.output.contacts) {
            c.id = nextId++;
            c.state = TouchStateDown;
            c.isEdge = IsEdgeContact(c, 60, 40, 2.0f);
            c.isReported = true;
            c.reportEvent = TouchReportIdle;
        }
        return true;
    }
    // Safety clamp: prevent out-of-bounds write if m_maxTouchCount exceeds fixed arrays
    const int effectiveMaxTouches = (std::min)(m_maxTouchCount, kMaxTracks);
    if (frame.touch.output.contacts.size() > static_cast<size_t>(effectiveMaxTouches))
        frame.touch.output.contacts.resize(static_cast<size_t>(effectiveMaxTouches));

    constexpr int kRows = 40, kCols = 60;
    constexpr float kEdgeMargin = 2.0f;
    float stylusAftX = 0, stylusAftY = 0;
    const bool stylusAftActive = ResolveStylusAftContext(frame, stylusAftX, stylusAftY);
    const bool gapRelinkActive = m_gapRelinkEnabled && (m_gapRelinkWindowFrames > 0);

    const int curCount = static_cast<int>(std::min(frame.touch.output.contacts.size(), static_cast<size_t>(kMaxTracks)));
    const int preCount = m_trackCount;
    int curToPre[kMaxTracks];
    bool alwaysMatched[kMaxTracks] = {};
    bool gapRelinked[kMaxTracks] = {};
    for (int i = 0; i < curCount; ++i) curToPre[i] = -1;

    int activePrev[kMaxTracks], silentPrev[kMaxTracks];
    int activeCount = 0, silentCount = 0;
    for (int p = 0; p < preCount; ++p) {
        const auto& t = m_tracks[p];
        if (gapRelinkActive && t.phase == TrackPhase::SilentGap && t.gapFrames > 0 && t.gapFrames <= m_gapRelinkWindowFrames)
            silentPrev[silentCount++] = p;
        else
            activePrev[activeCount++] = p;
    }

    if (curCount > 0 && activeCount > 0) {
        MatchAgainstSubset(frame.touch.output.contacts, curCount, activePrev, activeCount, curToPre);
        for (int c = 0; c < curCount; ++c) {
            const int pre = curToPre[c];
            if (pre < 0) continue;
            float refX = 0.0f, refY = 0.0f;
            GetMatchReference(m_tracks[pre], refX, refY);
            const float gateSq = ComputeTrackGateSq(m_tracks[pre], frame.touch.output.contacts[c], kCols, kRows, kEdgeMargin);
            if (DistanceSq(frame.touch.output.contacts[c].x, frame.touch.output.contacts[c].y, refX, refY) > gateSq)
                curToPre[c] = -1;
        }

        if (gapRelinkActive && curCount == 1 && activeCount == 1 && silentCount == 0 && curToPre[0] < 0) {
            const int pre = activePrev[0];
            if (PassActiveBootstrapGate(m_tracks[pre], frame.touch.output.contacts[0], kCols, kRows, kEdgeMargin)) {
                curToPre[0] = pre;
            }
        }

        const float alwaysMatchSq = m_alwaysMatchDistance * m_alwaysMatchDistance;
        bool cUsed[kMaxTracks] = {}, pUsed[kMaxTracks] = {};
        for (int c = 0; c < curCount; ++c) {
            if (curToPre[c] >= 0) {
                cUsed[c] = true;
                pUsed[curToPre[c]] = true;
            }
        }
        for (int c = 0; c < curCount; ++c) {
            if (cUsed[c]) continue;
            float best = std::numeric_limits<float>::max();
            int bestPre = -1;
            for (int i = 0; i < activeCount; ++i) {
                const int pre = activePrev[i];
                if (pUsed[pre]) continue;
                float refX = 0.0f, refY = 0.0f;
                GetMatchReference(m_tracks[pre], refX, refY);
                const float d = DistanceSq(frame.touch.output.contacts[c].x, frame.touch.output.contacts[c].y, refX, refY);
                if (d < best) { best = d; bestPre = pre; }
            }
            if (bestPre >= 0 && best <= alwaysMatchSq &&
                !IsEdgeContact(frame.touch.output.contacts[c], kCols, kRows, kEdgeMargin)) {
                curToPre[c] = bestPre;
                alwaysMatched[c] = true;
                cUsed[c] = true;
                pUsed[bestPre] = true;
            }
        }
    }

    if (gapRelinkActive && curCount > 0 && silentCount > 0) {
        float bestForCur[kMaxTracks], secondForCur[kMaxTracks];
        int bestTrackForCur[kMaxTracks];
        float bestForTrack[kMaxTracks], secondForTrack[kMaxTracks];
        int bestCurForTrack[kMaxTracks];
        for (int i = 0; i < kMaxTracks; ++i) {
            bestForCur[i] = secondForCur[i] = std::numeric_limits<float>::infinity();
            bestTrackForCur[i] = -1;
            bestForTrack[i] = secondForTrack[i] = std::numeric_limits<float>::infinity();
            bestCurForTrack[i] = -1;
        }
        for (int c = 0; c < curCount; ++c) {
            if (curToPre[c] >= 0) continue;
            for (int i = 0; i < silentCount; ++i) {
                const int pre = silentPrev[i];
                float refX = 0.0f, refY = 0.0f;
                GetMatchReference(m_tracks[pre], refX, refY);
                const float d = DistanceSq(frame.touch.output.contacts[c].x, frame.touch.output.contacts[c].y, refX, refY);
                const float gateSq = ComputeTrackGateSq(m_tracks[pre], frame.touch.output.contacts[c], kCols, kRows, kEdgeMargin);
                if (d > gateSq) continue;
                UpdateBestCandidate(d, pre, bestForCur[c], secondForCur[c], bestTrackForCur[c]);
                UpdateBestCandidate(d, c, bestForTrack[pre], secondForTrack[pre], bestCurForTrack[pre]);
            }
        }
        for (int c = 0; c < curCount; ++c) {
            if (curToPre[c] >= 0) continue;
            const int pre = bestTrackForCur[c];
            if (pre < 0 || bestCurForTrack[pre] != c) continue;
            if (!PassGapRelinkAmbiguity(bestForCur[c], secondForCur[c])) continue;
            if (!PassGapRelinkAmbiguity(bestForTrack[pre], secondForTrack[pre])) continue;
            curToPre[c] = pre;
            gapRelinked[c] = true;
        }
    }

    bool preMatched[kMaxTracks] = {};
    TrackState nextTracks[kMaxTracks];
    int nextTrackCount = 0;
    TouchContact out[kMaxTracks * 2];
    int outCount = 0;

    for (int c = 0; c < curCount; ++c) {
        TouchContact o = frame.touch.output.contacts[c];
        const int pre = curToPre[c];
        if (pre < 0) continue;
        preMatched[pre] = true;
        TrackState t = m_tracks[pre];
        const bool wasSilentGap = (t.phase == TrackPhase::SilentGap);
        const int frameSpan = wasSilentGap ? std::max(1, t.gapFrames + 1) : 1;
        t.phase = TrackPhase::Active;
        t.gapFrames = 0;
        o.id = t.id;
        o.prevIndex = pre;
        o.isEdge = IsEdgeContact(o, kCols, kRows, kEdgeMargin);
        o.lifeFlags = TouchLifeMapped;
        if (o.isEdge) o.lifeFlags |= TouchLifeEdge;
        if (alwaysMatched[c]) o.lifeFlags |= TouchLifeAlwaysMatch;
        const float curSize = EstimateSizeMm(o.area, o.signalSum);
        t.sizeMm = std::max({curSize, t.sizeMm, m_fallbackSizeMm});
        o.sizeMm = t.sizeMm;
        const bool emitDown = !wasSilentGap && (t.age <= 1 || t.downDebounceFrames > 0);
        o.state = emitDown ? TouchStateDown : TouchStateMove;
        if (emitDown && t.downDebounceFrames > 0) {
            o.lifeFlags |= TouchLifeDebounced;
            t.downDebounceFrames -= 1;
        }
        t.vx = (o.x - t.x) / static_cast<float>(frameSpan);
        t.vy = (o.y - t.y) / static_cast<float>(frameSpan);
        t.x = o.x;
        t.y = o.y;
        t.area = o.area;
        t.signalSum = o.signalSum;
        StoreEdgeMetadata(t, o);
        t.missed = 0;
        t.age += 1;
        t.upEventEmitted = false;
        if (!stylusAftActive && t.stylusSuppressFrames > 0) t.stylusSuppressFrames -= 1;
        bool aftSuppressed = false;
        if (stylusAftActive) {
            if (t.stylusSuppressFrames > 0) {
                aftSuppressed = true;
                t.stylusSuppressFrames -= 1;
            } else {
                int hold = 0;
                if (ShouldStylusAftSuppress(o, t.age, stylusAftX, stylusAftY, hold)) {
                    aftSuppressed = true;
                    t.stylusSuppressFrames = std::max(0, hold - 1);
                }
            }
        }
        o.isReported = !aftSuppressed;
        o.reportEvent = TouchReportIdle;
        o.reportFlags = 0;
        if (aftSuppressed) o.debugFlags = 0x101;
        else if (gapRelinked[c]) o.debugFlags = 0x21;
        else o.debugFlags = 0x01;
        if (outCount < effectiveMaxTouches) out[outCount++] = o;
        if (nextTrackCount < effectiveMaxTouches) nextTracks[nextTrackCount++] = t;
    }

    for (int c = 0; c < curCount; ++c) {
        if (curToPre[c] >= 0) continue;
        TouchContact o = frame.touch.output.contacts[c];
        TrackState t;
        t.id = AllocateId(nextTracks, nextTrackCount);
        if (t.id == 0) continue;
        t.x = o.x; t.y = o.y; t.area = o.area; t.signalSum = o.signalSum;
        t.sizeMm = EstimateSizeMm(o.area, o.signalSum);
        t.age = 1; t.missed = 0; t.phase = TrackPhase::Active; t.gapFrames = 0;
        o.isEdge = IsEdgeContact(o, kCols, kRows, kEdgeMargin);
        if (m_touchDownRejectEnabled) {
            const bool weak = (t.signalSum < m_touchDownRejectMinSignal);
            const bool tiny = (t.sizeMm < m_touchDownRejectMinSizeMm);
            const bool weakEdge = o.isEdge && (t.signalSum < m_touchDownEdgeRejectMinSignal);
            if ((weak && tiny) || weakEdge) continue;
        }
        t.downDebounceFrames = ComputeTouchDownDebounceFrames(o);
        if (stylusAftActive) {
            int hold = 0;
            if (ShouldStylusAftSuppress(o, t.age, stylusAftX, stylusAftY, hold))
                t.stylusSuppressFrames = std::max(0, hold - 1);
        }
        m_nextIdSeed = (t.id % effectiveMaxTouches) + 1;
        o.id = t.id;
        o.state = TouchStateDown;
        o.sizeMm = t.sizeMm;
        o.isReported = (t.stylusSuppressFrames <= 0);
        o.prevIndex = -1;
        o.debugFlags = 0x02;
        o.lifeFlags = TouchLifeNew;
        if (o.isEdge) o.lifeFlags |= TouchLifeEdge;
        if (t.downDebounceFrames > 0) {
            o.lifeFlags |= TouchLifeDebounced;
            t.downDebounceFrames -= 1;
        }
        StoreEdgeMetadata(t, o);
        o.reportEvent = TouchReportIdle;
        o.reportFlags = 0;
        if (outCount < effectiveMaxTouches) out[outCount++] = o;
        if (nextTrackCount < effectiveMaxTouches) nextTracks[nextTrackCount++] = t;
    }

    for (int p = 0; p < preCount; ++p) {
        if (preMatched[p]) continue;
        TrackState t = m_tracks[p];
        if (gapRelinkActive) {
            if (t.phase != TrackPhase::SilentGap) {
                t.phase = TrackPhase::SilentGap;
                t.gapFrames = 1;
            } else {
                t.gapFrames += 1;
            }
            t.missed = t.gapFrames;
            if (t.stylusSuppressFrames > 0) t.stylusSuppressFrames -= 1;
            if (t.gapFrames <= m_gapRelinkWindowFrames) {
                TouchContact hidden = BuildSilentGapContact(t, p, kCols, kRows, kEdgeMargin);
                if (outCount < effectiveMaxTouches) out[outCount++] = hidden;
                if (nextTrackCount < effectiveMaxTouches) nextTracks[nextTrackCount++] = t;
            } else {
                TouchContact up = BuildLiftOffContact(t, p, kCols, kRows, kEdgeMargin);
                if (outCount < effectiveMaxTouches) out[outCount++] = up;
            }
            continue;
        }

        t.missed += 1;
        if (t.stylusSuppressFrames > 0) t.stylusSuppressFrames -= 1;
        TouchContact up = BuildLiftOffContact(t, p, kCols, kRows, kEdgeMargin);
        if (outCount < effectiveMaxTouches) out[outCount++] = up;
    }

    GhostSuppressor ghostSuppressor;
    ghostSuppressor.m_rxGhostFilterEnabled = m_rxGhostFilterEnabled;
    ghostSuppressor.m_rxGhostLineDelta = m_rxGhostLineDelta;
    ghostSuppressor.m_rxGhostWeakRatio = m_rxGhostWeakRatio;
    ghostSuppressor.m_rxGhostOnlyNew = m_rxGhostOnlyNew;
    ghostSuppressor.ProcessTracked(out, outCount, effectiveMaxTouches, nextTracks, nextTrackCount,
                                   TouchStateUp, TouchStateDown, TouchLifeSilentGap);

    frame.touch.output.contacts.assign(out, out + outCount);
    std::memcpy(m_tracks, nextTracks, sizeof(TrackState) * nextTrackCount);
    m_trackCount = nextTrackCount;

    int activeSuppressFrames = 0;
    for (int i = 0; i < m_trackCount; ++i) {
        activeSuppressFrames = std::max(activeSuppressFrames,
                                        m_tracks[i].stylusSuppressFrames);
    }
    frame.stylus.interop.touchSuppressFrames = static_cast<uint8_t>(
        std::clamp(std::max(activeSuppressFrames,
                            static_cast<int>(frame.stylus.interop.touchSuppressFrames)),
                   0, 255));
    if (activeSuppressFrames > 0) {
        frame.stylus.interop.touchSuppressActive = true;
    }
    return true;
}

}} // namespace Solvers::Touch
