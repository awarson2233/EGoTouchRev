#pragma once

#include "SolverTypes.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace Solvers { namespace Touch {

struct PalmBoxRect {
    int minR = 39;
    int maxR = 0;
    int minC = 59;
    int maxC = 0;
};

struct TrackedPalmBox {
    int id = 0;
    PalmBoxRect bbox{};
    PalmBoxRect expandedBbox{};
    int age = 0;
    int missed = 0;
    int lastMatchedZoneIndex = -1;
    int anchorPeakCount = 0;
    int signalSum = 0;
    bool matchedPalmThisFrame = false;
};

class PalmBoxSuppressor {
public:
    static constexpr int kRows = 40;
    static constexpr int kCols = 60;
    static constexpr int kMaxPalmBoxes = 20;

    bool  m_enabled = true;
    int   m_expandRows = 9;
    int   m_expandCols = 10;
    float m_matchCenterDistance = 6.0f;
    float m_matchIoUThreshold = 0.10f;
    bool  m_palmLikelyOnly = true;
    bool  m_keepUntilNoPeakDomainInside = true;
    int   m_maxHoldFrames = 0;
    int   m_mergeGapRows = 0;
    int   m_mergeGapCols = 0;

    PalmBoxSuppressor() {
        m_tracks.reserve(kMaxPalmBoxes);
        m_observations.reserve(kMaxPalmBoxes);
        m_adjustedEvaluations.reserve(100);
    }

    // ── 统一签名入口：从 frame.touch.runtime 读取参数，结果写回 runtime ──
    inline void Process(HeatmapFrame& frame) {
        const auto& rt = frame.touch.runtime;
        if (rt.macroZones) {
            Process(*rt.macroZones, rt.zoneFeatures, rt.peaks, rt.peakEvaluations);
        }
        // adjusted evaluations 覆盖 runtime 中的原始 evaluations
        frame.touch.runtime.peakEvaluations = GetEvaluations();
    }

    inline void Process(const std::vector<MacroZone>& macroZones,
                        std::span<const MacroZoneFeature> zoneFeatures,
                        std::span<const Peak> peaks,
                        std::span<const PeakEvaluation> evaluations) {
        CopyEvaluations(evaluations);
        m_palmZoneMask.fill(0);

        if (!m_enabled || evaluations.size() != peaks.size()) {
            if (!m_enabled) ClearLiveState();
            return;
        }

        BuildObservations(macroZones, zoneFeatures, peaks);
        UpdateTracks(macroZones, peaks);
        SuppressTouchingZones(macroZones, peaks);
    }

    std::span<const PeakEvaluation> GetEvaluations() const {
        return std::span<const PeakEvaluation>(m_adjustedEvaluations.data(), m_adjustedEvaluations.size());
    }

    std::span<const TrackedPalmBox> GetPalmBoxes() const {
        return std::span<const TrackedPalmBox>(m_tracks.data(), m_tracks.size());
    }

    inline void ClearLiveState() {
        m_tracks.clear();
        m_nextIdSeed = 1;
    }

private:
    struct PalmBoxObservation {
        PalmBoxRect bbox{};
        int area = 0;
        int signalSum = 0;
        int peakCount = 0;
        int primaryZoneIndex = -1;
    };

    std::vector<TrackedPalmBox> m_tracks;
    int m_nextIdSeed = 1;
    std::vector<PalmBoxObservation> m_observations;
    std::array<uint8_t, kMaxPalmBoxes> m_palmZoneMask{};
    std::vector<PeakEvaluation> m_adjustedEvaluations;

    inline void CopyEvaluations(std::span<const PeakEvaluation> evaluations) {
        m_adjustedEvaluations.assign(evaluations.begin(), evaluations.end());
    }

    static inline bool IsValid(const PalmBoxRect& rect) {
        return rect.minR <= rect.maxR && rect.minC <= rect.maxC;
    }

    static inline PalmBoxRect FromZone(const MacroZone& zone) {
        return {zone.minR, zone.maxR, zone.minC, zone.maxC};
    }

    static inline PalmBoxRect Expand(const PalmBoxRect& rect, int rows, int cols) {
        if (!IsValid(rect)) return rect;
        return {
            std::max(0, rect.minR - std::max(0, rows)),
            std::min(kRows - 1, rect.maxR + std::max(0, rows)),
            std::max(0, rect.minC - std::max(0, cols)),
            std::min(kCols - 1, rect.maxC + std::max(0, cols))
        };
    }

    static inline bool IntersectsOrTouches(const PalmBoxRect& a, const PalmBoxRect& b) {
        return IsValid(a) && IsValid(b) &&
               a.minR <= b.maxR && a.maxR >= b.minR &&
               a.minC <= b.maxC && a.maxC >= b.minC;
    }

    static inline bool ContainsPoint(const PalmBoxRect& rect, int row, int col) {
        return IsValid(rect) && row >= rect.minR && row <= rect.maxR &&
               col >= rect.minC && col <= rect.maxC;
    }

    static inline int Area(const PalmBoxRect& rect) {
        if (!IsValid(rect)) return 0;
        return (rect.maxR - rect.minR + 1) * (rect.maxC - rect.minC + 1);
    }

    static inline PalmBoxRect UnionRect(const PalmBoxRect& a, const PalmBoxRect& b) {
        if (!IsValid(a)) return b;
        if (!IsValid(b)) return a;
        return {
            std::min(a.minR, b.minR),
            std::max(a.maxR, b.maxR),
            std::min(a.minC, b.minC),
            std::max(a.maxC, b.maxC)
        };
    }

    static inline float CenterDistanceSq(const PalmBoxRect& a, const PalmBoxRect& b) {
        const float ar = (static_cast<float>(a.minR + a.maxR) * 0.5f);
        const float ac = (static_cast<float>(a.minC + a.maxC) * 0.5f);
        const float br = (static_cast<float>(b.minR + b.maxR) * 0.5f);
        const float bc = (static_cast<float>(b.minC + b.maxC) * 0.5f);
        const float dr = ar - br;
        const float dc = ac - bc;
        return dr * dr + dc * dc;
    }

    static inline float IoU(const PalmBoxRect& a, const PalmBoxRect& b) {
        if (!IntersectsOrTouches(a, b)) return 0.0f;
        const int minR = std::max(a.minR, b.minR);
        const int maxR = std::min(a.maxR, b.maxR);
        const int minC = std::max(a.minC, b.minC);
        const int maxC = std::min(a.maxC, b.maxC);
        const int inter = Area({minR, maxR, minC, maxC});
        const int uni = Area(a) + Area(b) - inter;
        return uni > 0 ? static_cast<float>(inter) / static_cast<float>(uni) : 0.0f;
    }

    inline bool IsPalmObservation(const MacroZoneFeature& feature) const {
        if (m_palmLikelyOnly) return feature.palmClass == PalmClass::PalmLikely;
        return feature.palmClass == PalmClass::PalmLikely ||
               feature.palmClass == PalmClass::PalmCandidate;
    }

    inline int CountPeaksInZone(std::span<const Peak> peaks, int zoneIndex) const {
        int count = 0;
        for (const auto& peak : peaks) {
            if (peak.macroZoneIndex == zoneIndex) ++count;
        }
        return count;
    }

    inline void AddOrMergeObservation(const PalmBoxObservation& obs) {
        const PalmBoxRect mergeRect = Expand(obs.bbox, m_mergeGapRows, m_mergeGapCols);
        for (auto& existing : m_observations) {
            if (!IntersectsOrTouches(mergeRect, existing.bbox) &&
                !IntersectsOrTouches(Expand(existing.bbox, m_mergeGapRows, m_mergeGapCols), obs.bbox)) {
                continue;
            }
            existing.bbox = UnionRect(existing.bbox, obs.bbox);
            existing.area += obs.area;
            existing.signalSum += obs.signalSum;
            existing.peakCount += obs.peakCount;
            if (existing.primaryZoneIndex < 0) existing.primaryZoneIndex = obs.primaryZoneIndex;
            return;
        }

        if (m_observations.size() < kMaxPalmBoxes) {
            m_observations.push_back(obs);
        }
    }

    inline void BuildObservations(const std::vector<MacroZone>& macroZones,
                                  std::span<const MacroZoneFeature> zoneFeatures,
                                  std::span<const Peak> peaks) {
        m_observations.clear();
        const int featureCount = static_cast<int>(zoneFeatures.size());
        for (int fi = 0; fi < featureCount; ++fi) {
            const auto& feature = zoneFeatures[static_cast<size_t>(fi)];
            if (!IsPalmObservation(feature)) continue;
            const int zoneIndex = feature.zoneIndex;
            if (zoneIndex < 0 || zoneIndex >= static_cast<int>(macroZones.size()) || zoneIndex >= kMaxPalmBoxes) continue;
            m_palmZoneMask[static_cast<size_t>(zoneIndex)] = 1;
            const auto& zone = macroZones[static_cast<size_t>(zoneIndex)];
            PalmBoxObservation obs;
            obs.bbox = FromZone(zone);
            obs.area = zone.area;
            obs.signalSum = zone.signalSum;
            obs.peakCount = CountPeaksInZone(peaks, zoneIndex);
            obs.primaryZoneIndex = zoneIndex;
            AddOrMergeObservation(obs);
        }
    }

    inline int AllocateId(const TrackedPalmBox* reservedTracks, int reservedCount) const {
        for (int i = 0; i < kMaxPalmBoxes; ++i) {
            const int candidate = ((m_nextIdSeed - 1 + i) % kMaxPalmBoxes) + 1;
            bool used = false;
            for (int j = 0; j < reservedCount; ++j) {
                if (reservedTracks[j].id == candidate) { used = true; break; }
            }
            if (used) continue;
            for (const auto& track : m_tracks) {
                if (track.id == candidate) { used = true; break; }
            }
            if (!used) return candidate;
        }
        return 0;
    }

    inline bool PassMatch(const TrackedPalmBox& track, const PalmBoxObservation& obs) const {
        const float distanceSq = CenterDistanceSq(track.bbox, obs.bbox);
        const float maxDist = std::max(0.0f, m_matchCenterDistance);
        return distanceSq <= maxDist * maxDist || IoU(track.bbox, obs.bbox) >= m_matchIoUThreshold;
    }

    inline TrackedPalmBox MakeTrackFromObservation(const PalmBoxObservation& obs, int id, int previousAge = 0) const {
        TrackedPalmBox track;
        track.id = id;
        track.bbox = obs.bbox;
        track.expandedBbox = Expand(obs.bbox, m_expandRows, m_expandCols);
        track.age = previousAge + 1;
        track.missed = 0;
        track.lastMatchedZoneIndex = obs.primaryZoneIndex;
        track.anchorPeakCount = obs.peakCount;
        track.signalSum = obs.signalSum;
        track.matchedPalmThisFrame = true;
        return track;
    }

    inline int CountPeakDomainsInside(const PalmBoxRect& rect,
                                      const std::vector<MacroZone>& macroZones,
                                      std::span<const Peak> peaks) const {
        int count = 0;
        for (const auto& peak : peaks) {
            if (ContainsPoint(rect, peak.r, peak.c)) {
                ++count;
                continue;
            }
            const int zoneIndex = peak.macroZoneIndex;
            if (zoneIndex < 0 || zoneIndex >= static_cast<int>(macroZones.size())) continue;
            if (IntersectsOrTouches(rect, FromZone(macroZones[static_cast<size_t>(zoneIndex)]))) {
                ++count;
            }
        }
        return count;
    }

    inline bool ShouldKeepUnmatchedTrack(const TrackedPalmBox& track,
                                         const std::vector<MacroZone>& macroZones,
                                         std::span<const Peak> peaks,
                                         int& outAnchorPeakCount) const {
        outAnchorPeakCount = CountPeakDomainsInside(track.bbox, macroZones, peaks);
        if (!m_keepUntilNoPeakDomainInside || outAnchorPeakCount <= 0) return false;
        if (m_maxHoldFrames > 0 && track.missed + 1 > m_maxHoldFrames) return false;
        return true;
    }

    inline void AddDedupedTrack(TrackedPalmBox* tracks, int& count, const TrackedPalmBox& incoming) const {
        for (int i = 0; i < count; ++i) {
            auto& existing = tracks[i];
            const bool duplicate = IntersectsOrTouches(existing.bbox, incoming.bbox) ||
                                   IoU(existing.bbox, incoming.bbox) >= 0.25f;
            if (!duplicate) continue;
            const bool preferIncoming = incoming.age > existing.age ||
                (incoming.age == existing.age && incoming.signalSum > existing.signalSum);
            const int keepId = preferIncoming ? incoming.id : existing.id;
            const int keepAge = std::max(existing.age, incoming.age);
            const int keepMissed = std::min(existing.missed, incoming.missed);
            existing.bbox = UnionRect(existing.bbox, incoming.bbox);
            existing.expandedBbox = Expand(existing.bbox, m_expandRows, m_expandCols);
            existing.id = keepId;
            existing.age = keepAge;
            existing.missed = keepMissed;
            existing.anchorPeakCount += incoming.anchorPeakCount;
            existing.signalSum = std::max(existing.signalSum, incoming.signalSum);
            existing.matchedPalmThisFrame = existing.matchedPalmThisFrame || incoming.matchedPalmThisFrame;
            if (existing.lastMatchedZoneIndex < 0) existing.lastMatchedZoneIndex = incoming.lastMatchedZoneIndex;
            return;
        }

        if (count < kMaxPalmBoxes) {
            tracks[count++] = incoming;
        }
    }

    inline void UpdateTracks(const std::vector<MacroZone>& macroZones,
                             std::span<const Peak> peaks) {
        std::array<uint8_t, kMaxPalmBoxes> trackUsed{};
        std::array<uint8_t, kMaxPalmBoxes> obsUsed{};
        std::array<TrackedPalmBox, kMaxPalmBoxes> nextTracks{};
        int nextCount = 0;

        const int observationCount = static_cast<int>(m_observations.size());
        const int trackCount = static_cast<int>(m_tracks.size());
        for (int oi = 0; oi < observationCount; ++oi) {
            const auto& obs = m_observations[static_cast<size_t>(oi)];
            int bestTrack = -1;
            float bestCost = 1.0e9f;
            for (int ti = 0; ti < trackCount; ++ti) {
                if (trackUsed[static_cast<size_t>(ti)] != 0) continue;
                const auto& track = m_tracks[static_cast<size_t>(ti)];
                if (!PassMatch(track, obs)) continue;
                const float cost = CenterDistanceSq(track.bbox, obs.bbox) - IoU(track.bbox, obs.bbox) * 100.0f;
                if (cost < bestCost) {
                    bestCost = cost;
                    bestTrack = ti;
                }
            }
            if (bestTrack < 0) continue;
            trackUsed[static_cast<size_t>(bestTrack)] = 1;
            obsUsed[static_cast<size_t>(oi)] = 1;
            AddDedupedTrack(nextTracks.data(), nextCount,
                            MakeTrackFromObservation(obs, m_tracks[static_cast<size_t>(bestTrack)].id,
                                                     m_tracks[static_cast<size_t>(bestTrack)].age));
        }

        for (int ti = 0; ti < trackCount; ++ti) {
            if (trackUsed[static_cast<size_t>(ti)] != 0) continue;
            const auto& oldTrack = m_tracks[static_cast<size_t>(ti)];
            int anchorPeakCount = 0;
            if (!ShouldKeepUnmatchedTrack(oldTrack, macroZones, peaks, anchorPeakCount)) continue;
            TrackedPalmBox kept = oldTrack;
            kept.expandedBbox = Expand(kept.bbox, m_expandRows, m_expandCols);
            kept.age += 1;
            kept.missed += 1;
            kept.anchorPeakCount = anchorPeakCount;
            kept.lastMatchedZoneIndex = -1;
            kept.matchedPalmThisFrame = false;
            AddDedupedTrack(nextTracks.data(), nextCount, kept);
        }

        for (int oi = 0; oi < observationCount; ++oi) {
            if (obsUsed[static_cast<size_t>(oi)] != 0) continue;
            const auto& obs = m_observations[static_cast<size_t>(oi)];
            const int id = AllocateId(nextTracks.data(), nextCount);
            if (id == 0) continue;
            AddDedupedTrack(nextTracks.data(), nextCount, MakeTrackFromObservation(obs, id));
            m_nextIdSeed = (id % kMaxPalmBoxes) + 1;
        }

        m_tracks.assign(nextTracks.begin(), nextTracks.begin() + nextCount);
    }

    inline bool ZoneTouchesLivePalmBox(const PalmBoxRect& zoneRect) const {
        for (const auto& track : m_tracks) {
            if (IntersectsOrTouches(zoneRect, track.expandedBbox)) {
                return true;
            }
        }
        return false;
    }

    inline bool PeakTouchesLivePalmBox(const Peak& peak) const {
        for (const auto& track : m_tracks) {
            if (ContainsPoint(track.expandedBbox, peak.r, peak.c)) {
                return true;
            }
        }
        return false;
    }

    static inline void SuppressEvaluation(PeakEvaluation& eval) {
        eval.allowContact = false;
        eval.palmEvidenceOnly = true;
        eval.evalFlags |= PalmReasonPalmBoxSuppressed;
    }

    inline void SuppressTouchingZones(const std::vector<MacroZone>& macroZones,
                                      std::span<const Peak> peaks) {
        const int zoneCount = std::min(static_cast<int>(macroZones.size()), kMaxPalmBoxes);
        std::array<uint8_t, kMaxPalmBoxes> suppressedZones{};
        for (int zi = 0; zi < zoneCount; ++zi) {
            if (m_palmZoneMask[static_cast<size_t>(zi)] != 0) continue;
            if (ZoneTouchesLivePalmBox(FromZone(macroZones[static_cast<size_t>(zi)]))) {
                suppressedZones[static_cast<size_t>(zi)] = 1;
            }
        }

        const int peakCount = std::min(static_cast<int>(peaks.size()), static_cast<int>(m_adjustedEvaluations.size()));
        for (int pi = 0; pi < peakCount; ++pi) {
            const auto& peak = peaks[static_cast<size_t>(pi)];
            bool suppressed = PeakTouchesLivePalmBox(peak);
            const int zoneIndex = peak.macroZoneIndex;
            if (!suppressed && zoneIndex >= 0 && zoneIndex < zoneCount) {
                suppressed = suppressedZones[static_cast<size_t>(zoneIndex)] != 0;
            }
            if (!suppressed) continue;
            SuppressEvaluation(m_adjustedEvaluations[static_cast<size_t>(pi)]);
        }
    }
};

}} // namespace Solvers::Touch
