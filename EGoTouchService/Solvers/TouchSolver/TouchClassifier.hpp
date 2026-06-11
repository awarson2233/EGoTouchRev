#pragma once

#include "SolverTypes.h"
#include "MSType.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace Solvers { namespace Touch {

class TouchClassifier {
public:
    TouchClassifier() {
        m_zoneFeatures.reserve(20);
        m_peakEvaluations.reserve(100);
    }

    bool  m_enabled = true;
    int   m_areaThreshold = 50;
    int   m_signalSumThreshold = 80000;
    float m_densityThresholdLow = 400.0f;
    int   m_areaMinForDensity = 20;
    bool  m_elongatedEnabled = true;
    int   m_elongatedMinArea = 10;
    float m_elongatedAspectRatio = 4.0f;
    int   m_lastRejectedCount = 0;

    bool  m_analyzerEnabled = true;
    int   m_candidateAreaThreshold = 35;
    int   m_candidateSignalThreshold = 80000;
    int   m_likelyAreaThreshold = 55;
    float m_fillRatioThreshold = 0.40f;
    float m_flatSharpnessThreshold = 2.20f;
    int   m_strongPeakProminence = 2000;

    bool  m_peakEvalEnabled = true;
    int   m_fingerProminence = 100;
    float m_fingerSharpness = 3.35f;
    float m_palmSharpnessMax = 3.30f;
    float m_ambiguousMargin = 0.15f;
    bool  m_palmAwareExpansionEnabled = true;
    float m_fingerInPalmThresholdRatio = 0.70f;
    int   m_fingerInPalmMaxRadius = 3;
    bool  m_palmLikelyAllowContact = false;
    bool  m_palmShadowEnabled = true;
    int   m_palmShadowRadius = 2;
    int   m_palmShadowHoldFrames = 12;
    float m_palmShadowSeedScore = 0.55f;

    inline void Process(const HeatmapFrame& frame,
                        const std::vector<MacroZone>& macroZones,
                        std::span<const Peak> peaks) {
        AnalyzeZones(frame, macroZones);
        UpdatePalmShadow(macroZones);
        EvaluatePeaks(frame, peaks, m_zoneFeatures);
    }

    inline int AnalyzeZones(const HeatmapFrame& frame,
                            const std::vector<MacroZone>& macroZones) {
        m_zoneFeatures.clear();
        m_lastRejectedCount = 0;
        if (!m_enabled || !m_analyzerEnabled) return 0;

        for (int zi = 0; zi < static_cast<int>(macroZones.size()); ++zi) {
            m_zoneFeatures.push_back(BuildFeature(frame, macroZones[static_cast<size_t>(zi)], zi));
        }
        return 0;
    }

    inline void EvaluatePeaks(const HeatmapFrame& frame,
                              std::span<const Peak> peaks,
                              const std::vector<MacroZoneFeature>& zoneFeatures) {
        m_peakEvaluations.clear();
        if (!m_peakEvalEnabled) return;
        m_peakEvaluations.resize(peaks.size());

        for (size_t i = 0; i < peaks.size(); ++i) {
            m_peakEvaluations[i] = EvaluatePeak(frame, peaks[i], zoneFeatures);
        }
    }

    const std::vector<MacroZoneFeature>& GetZoneFeatures() const { return m_zoneFeatures; }

    std::span<const PeakEvaluation> GetPeakEvaluations() const {
        return std::span<const PeakEvaluation>(m_peakEvaluations.data(), m_peakEvaluations.size());
    }

    std::span<const PeakEvaluation> GetEvaluations() const { return GetPeakEvaluations(); }

private:
    static constexpr int kRows = 40;
    static constexpr int kCols = 60;
    static constexpr int kGridSize = kRows * kCols;

    std::vector<MacroZoneFeature> m_zoneFeatures;
    std::vector<PeakEvaluation> m_peakEvaluations;
    std::array<uint8_t, kGridSize> m_palmShadowAge{};

    inline MacroZoneFeature BuildFeature(const HeatmapFrame& frame,
                                         const MacroZone& zone,
                                         int zoneIndex) const {
        MacroZoneFeature feature;
        feature.zoneIndex = zoneIndex;
        feature.area = zone.area;
        feature.signalSum = zone.signalSum;
        feature.bboxW = std::max(0, zone.maxC - zone.minC + 1);
        feature.bboxH = std::max(0, zone.maxR - zone.minR + 1);
        feature.bboxArea = feature.bboxW * feature.bboxH;
        feature.fillRatio = feature.bboxArea > 0
            ? static_cast<float>(zone.area) / static_cast<float>(feature.bboxArea)
            : 0.0f;
        feature.meanSignal = zone.area > 0
            ? static_cast<float>(zone.signalSum) / static_cast<float>(zone.area)
            : 0.0f;
        feature.density = feature.meanSignal;

        const int longSide = std::max(feature.bboxW, feature.bboxH);
        const int shortSide = std::min(feature.bboxW, feature.bboxH);
        feature.aspectRatio = shortSide > 0
            ? static_cast<float>(longSide) / static_cast<float>(shortSide)
            : 1.0f;

        double varianceSum = 0.0;
        for (int idx : zone.pixels) {
            const int r = idx / kCols;
            const int c = idx % kCols;
            const int sig = frame.heatmapMatrix[r][c];
            feature.maxSignal = std::max(feature.maxSignal, sig);
            const double delta = static_cast<double>(sig) - feature.meanSignal;
            varianceSum += delta * delta;
            if (r == 0) feature.edgeTouchMask |= 0x01;
            if (r == kRows - 1) feature.edgeTouchMask |= 0x02;
            if (c == 0) feature.edgeTouchMask |= 0x04;
            if (c == kCols - 1) feature.edgeTouchMask |= 0x08;
        }
        feature.signalVariance = zone.area > 0
            ? static_cast<float>(varianceSum / static_cast<double>(zone.area))
            : 0.0f;
        const float macroProminence = static_cast<float>(feature.maxSignal) - feature.meanSignal;
        const float macroSharpness = static_cast<float>(feature.maxSignal) / std::max(1.0f, feature.meanSignal);
        if (macroSharpness <= m_flatSharpnessThreshold) feature.reasonFlags |= PalmReasonFlatSignalShape;
        if (macroProminence >= static_cast<float>(m_strongPeakProminence)) feature.reasonFlags |= PalmReasonStrongSharpPeakPresent;

        if (zone.area >= m_candidateAreaThreshold) feature.reasonFlags |= PalmReasonLargeArea;
        if (zone.signalSum >= m_candidateSignalThreshold) feature.reasonFlags |= PalmReasonLargeSignalSum;
        if (zone.area >= m_areaMinForDensity && feature.density < m_densityThresholdLow) feature.reasonFlags |= PalmReasonLowDensity;
        if (m_elongatedEnabled && zone.area >= m_elongatedMinArea && feature.aspectRatio >= m_elongatedAspectRatio) feature.reasonFlags |= PalmReasonElongated;
        if (feature.fillRatio >= m_fillRatioThreshold && zone.area >= m_candidateAreaThreshold) feature.reasonFlags |= PalmReasonHighFillRatio;
        if (feature.edgeTouchMask != 0 && longSide >= 5) feature.reasonFlags |= PalmReasonEdgeWideContact;

        float palmScore = 0.0f;
        if (zone.area >= m_areaThreshold) palmScore += 0.35f;
        else if (zone.area >= m_candidateAreaThreshold) palmScore += 0.20f;
        if (zone.signalSum >= m_signalSumThreshold) palmScore += 0.25f;
        if (feature.reasonFlags & PalmReasonLowDensity) palmScore += 0.15f;
        if (feature.reasonFlags & PalmReasonElongated) palmScore += 0.15f;
        if (feature.reasonFlags & PalmReasonHighFillRatio) palmScore += 0.15f;
        if (feature.reasonFlags & PalmReasonEdgeWideContact) palmScore += 0.10f;
        if (feature.reasonFlags & PalmReasonFlatSignalShape) palmScore += 0.10f;
        if (feature.reasonFlags & PalmReasonStrongSharpPeakPresent) palmScore -= 0.20f;
        feature.palmScore = std::clamp(palmScore, 0.0f, 1.0f);

        float fingerScore = 0.0f;
        if (zone.area < m_candidateAreaThreshold) fingerScore += 0.45f;
        if (longSide <= 5) fingerScore += 0.25f;
        if (feature.aspectRatio < 2.5f) fingerScore += 0.15f;
        if (feature.density >= m_densityThresholdLow) fingerScore += 0.15f;
        if (feature.reasonFlags & PalmReasonStrongSharpPeakPresent) fingerScore += 0.20f;
        feature.fingerScore = std::clamp(fingerScore, 0.0f, 1.0f);

        if (zone.area >= m_likelyAreaThreshold && feature.palmScore >= 0.55f) {
            feature.palmClass = PalmClass::PalmLikely;
        } else if (feature.palmScore >= 0.35f) {
            feature.palmClass = PalmClass::PalmCandidate;
        } else if (feature.fingerScore >= 0.55f) {
            feature.palmClass = PalmClass::FingerLikely;
        } else {
            feature.palmClass = PalmClass::Ambiguous;
        }

        return feature;
    }

    inline void UpdatePalmShadow(const std::vector<MacroZone>& macroZones) {
        if (!m_palmShadowEnabled) {
            m_palmShadowAge.fill(0);
            return;
        }

        DecayPalmShadow();
        if (!m_enabled || !m_analyzerEnabled || m_zoneFeatures.empty()) return;

        SeedPalmShadow(macroZones);
        ApplyPalmShadowToZones(macroZones);
    }

    inline void DecayPalmShadow() {
        for (auto& age : m_palmShadowAge) {
            if (age > 0) --age;
        }
    }

    inline void SeedPalmShadow(const std::vector<MacroZone>& macroZones) {
        const int holdFrames = std::clamp(m_palmShadowHoldFrames, 0, 255);
        if (holdFrames <= 0) return;

        const int radius = std::max(0, m_palmShadowRadius);
        const size_t count = std::min(macroZones.size(), m_zoneFeatures.size());
        for (size_t i = 0; i < count; ++i) {
            const auto& feature = m_zoneFeatures[i];
            const bool seed = feature.palmClass == PalmClass::PalmLikely ||
                              (feature.palmClass == PalmClass::PalmCandidate &&
                               feature.palmScore >= m_palmShadowSeedScore);
            if (!seed) continue;
            for (int idx : macroZones[i].pixels) {
                DilatePalmShadowCell(idx, radius, static_cast<uint8_t>(holdFrames));
            }
        }
    }

    inline void DilatePalmShadowCell(int idx, int radius, uint8_t holdFrames) {
        if (idx < 0 || idx >= kGridSize) return;
        const int row = idx / kCols;
        const int col = idx % kCols;
        const int rowMin = std::max(0, row - radius);
        const int rowMax = std::min(kRows - 1, row + radius);
        const int colMin = std::max(0, col - radius);
        const int colMax = std::min(kCols - 1, col + radius);

        for (int r = rowMin; r <= rowMax; ++r) {
            for (int c = colMin; c <= colMax; ++c) {
                auto& age = m_palmShadowAge[static_cast<size_t>(r * kCols + c)];
                age = std::max(age, holdFrames);
            }
        }
    }

    inline void ApplyPalmShadowToZones(const std::vector<MacroZone>& macroZones) {
        const size_t count = std::min(macroZones.size(), m_zoneFeatures.size());
        for (size_t i = 0; i < count; ++i) {
            bool shadowTouch = false;
            for (int idx : macroZones[i].pixels) {
                if (idx < 0 || idx >= kGridSize) continue;
                if (m_palmShadowAge[static_cast<size_t>(idx)] > 0) {
                    shadowTouch = true;
                    break;
                }
            }
            if (!shadowTouch) continue;

            auto& feature = m_zoneFeatures[i];
            feature.reasonFlags |= PalmReasonShadowTouch;
            feature.palmClass = PalmClass::PalmLikely;
            feature.palmScore = std::max(feature.palmScore, 0.80f);
        }
    }

    inline PeakEvaluation EvaluatePeak(const HeatmapFrame& frame,
                                       const Peak& peak,
                                       const std::vector<MacroZoneFeature>& zoneFeatures) const {
        PeakEvaluation eval;
        const MacroZoneFeature* zone = nullptr;
        if (peak.macroZoneIndex >= 0 && peak.macroZoneIndex < static_cast<int>(zoneFeatures.size())) {
            zone = &zoneFeatures[static_cast<size_t>(peak.macroZoneIndex)];
            eval.zonePalmClass = zone->palmClass;
        }

        eval.localMean3x3 = LocalMean(frame, peak.r, peak.c, 1);
        eval.localMean5x5 = LocalMean(frame, peak.r, peak.c, 2);
        eval.prominence = static_cast<float>(peak.z) - eval.localMean5x5;
        eval.sharpness = static_cast<float>(peak.z) / std::max(1.0f, eval.localMean5x5);

        const bool inPalmZone = eval.zonePalmClass == PalmClass::PalmCandidate ||
                                eval.zonePalmClass == PalmClass::PalmLikely;
        const bool shadowTouch = zone && ((zone->reasonFlags & PalmReasonShadowTouch) != 0);
        const bool strongFingerShape = peak.z >= 1 &&
                                       eval.prominence >= static_cast<float>(m_fingerProminence) &&
                                       eval.sharpness >= m_fingerSharpness;
        const bool flatPalmShape = inPalmZone && eval.sharpness <= m_palmSharpnessMax;

        float fingerScore = 0.0f;
        if (eval.prominence >= static_cast<float>(m_fingerProminence)) fingerScore += 0.45f;
        if (eval.sharpness >= m_fingerSharpness) fingerScore += 0.35f;
        if (zone && zone->palmClass == PalmClass::FingerLikely) fingerScore += 0.20f;
        if (inPalmZone && strongFingerShape) fingerScore += 0.15f;
        eval.fingerScore = std::clamp(fingerScore, 0.0f, 1.0f);

        float palmScore = 0.0f;
        if (zone) palmScore += zone->palmScore * 0.45f;
        if (flatPalmShape) palmScore += 0.45f;
        if (inPalmZone && !strongFingerShape) palmScore += 0.15f;
        eval.palmScore = std::clamp(palmScore, 0.0f, 1.0f);

        if (shadowTouch) {
            eval.palmClass = PalmClass::PalmLikely;
            eval.palmScore = std::max(eval.palmScore, 0.80f);
            eval.allowContact = false;
            eval.palmEvidenceOnly = true;
            eval.evalFlags |= PalmReasonShadowTouch;
            if (strongFingerShape) eval.evalFlags |= PalmReasonStrongSharpPeakPresent;
            if (flatPalmShape) eval.evalFlags |= PalmReasonFlatSignalShape;
            return eval;
        }

        if (strongFingerShape && eval.fingerScore + m_ambiguousMargin >= eval.palmScore) {
            eval.palmClass = PalmClass::FingerLikely;
        } else if (flatPalmShape && eval.palmScore > eval.fingerScore) {
            eval.palmClass = PalmClass::PalmLikely;
        } else if (inPalmZone) {
            eval.palmClass = PalmClass::Ambiguous;
        } else if (eval.fingerScore >= 0.45f) {
            eval.palmClass = PalmClass::FingerLikely;
        } else {
            eval.palmClass = PalmClass::Unknown;
        }

        eval.allowContact = eval.palmClass != PalmClass::PalmLikely || m_palmLikelyAllowContact;
        eval.palmEvidenceOnly = eval.palmClass == PalmClass::PalmLikely && !eval.allowContact;
        if (strongFingerShape) eval.evalFlags |= PalmReasonStrongSharpPeakPresent;
        if (flatPalmShape) eval.evalFlags |= PalmReasonFlatSignalShape;
        return eval;
    }

    inline float LocalMean(const HeatmapFrame& frame, int row, int col, int radius) const {
        int sum = 0;
        int count = 0;
        for (int dr = -radius; dr <= radius; ++dr) {
            for (int dc = -radius; dc <= radius; ++dc) {
                const int r = row + dr;
                const int c = col + dc;
                if (r < 0 || r >= kRows || c < 0 || c >= kCols) continue;
                sum += frame.heatmapMatrix[r][c];
                ++count;
            }
        }
        return count > 0 ? static_cast<float>(sum) / static_cast<float>(count) : 0.0f;
    }
};

}} // namespace Solvers::Touch
