#pragma once

#include "Hpp2CoordinateSolver.hpp"
#include "Hpp2Runtime.hpp"

#include <algorithm>
#include <array>
#include <cstdint>

namespace Solvers::Stylus::Hpp2 {

struct Hpp2PeakSearchUtils {
    static bool IsLocalPeak(const Settings& settings,
                            const std::array<uint16_t, kMaxSamples>& line,
                            int offset,
                            int length,
                            int index) {
        const uint16_t current = line[static_cast<std::size_t>(offset + index)];
        if (current <= settings.peakSignalFloor) {
            return false;
        }
        const int neighborDist = std::max(1, settings.peakSearchNeighborDist);
        for (int delta = 1; delta <= neighborDist; ++delta) {
            if (index >= delta &&
                line[static_cast<std::size_t>(offset + index - delta)] > current) {
                return false;
            }
            if (index + delta < length &&
                line[static_cast<std::size_t>(offset + index + delta)] >= current) {
                return false;
            }
        }
        return true;
    }

    static void SearchPeakBoundary(const std::array<uint16_t, kMaxSamples>& line,
                                   int offset,
                                   int length,
                                   int peakIndex,
                                   PeakUnit& unit) {
        static constexpr uint32_t kBoundarySlopeQ5 = 0x23; // TSACore SearchPeakBoundary for group 0/1.
        static constexpr int kContributionPermilleFloor = 50;
        static constexpr uint32_t kAccumSignalFloor = 200;

        int left = peakIndex;
        if (left != 0) {
            left = peakIndex - 1;
            int contributionPermille = 1000;
            uint32_t accumulated = line[static_cast<std::size_t>(offset + peakIndex)];
            while (left != 0 && contributionPermille > kContributionPermilleFloor) {
                const uint32_t prev = line[static_cast<std::size_t>(offset + left - 1)];
                const uint32_t current = line[static_cast<std::size_t>(offset + left)];
                if (prev >= ((kBoundarySlopeQ5 * current) >> 5)) {
                    break;
                }
                accumulated += current;
                accumulated = std::max(accumulated, kAccumSignalFloor);
                contributionPermille = static_cast<int>((current * 1000u) / accumulated);
                if (contributionPermille > kContributionPermilleFloor) {
                    --left;
                }
            }
        }

        int right = peakIndex;
        if (right < length - 1) {
            right = peakIndex + 1;
            int contributionPermille = 1000;
            uint32_t accumulated = line[static_cast<std::size_t>(offset + peakIndex)];
            while (right < length - 1 && contributionPermille > kContributionPermilleFloor) {
                const uint32_t next = line[static_cast<std::size_t>(offset + right + 1)];
                const uint32_t current = line[static_cast<std::size_t>(offset + right)];
                if (next >= ((kBoundarySlopeQ5 * current) >> 5)) {
                    break;
                }
                accumulated += current;
                accumulated = std::max(accumulated, kAccumSignalFloor);
                contributionPermille = static_cast<int>((current * 1000u) / accumulated);
                if (contributionPermille > kContributionPermilleFloor) {
                    ++right;
                }
            }
        }

        unit.leftBoundary = left;
        unit.rightBoundary = right;
        unit.width = right - left + 1;
    }

    static void UpdatePeakPrpt(const std::array<uint16_t, kMaxSamples>& line,
                               int offset,
                               int length,
                               PeakUnit& unit) {
        uint32_t regionSum = 0;
        uint16_t baselineMin = 0xffff;
        for (int i = unit.leftBoundary; i <= unit.rightBoundary; ++i) {
            const uint16_t sample = line[static_cast<std::size_t>(offset + i)];
            regionSum += sample;
            baselineMin = std::min(baselineMin, sample);
        }

        uint32_t threeNeighborSum = 0;
        const int neighborStart = std::max(0, unit.index - 1);
        const int neighborEnd = std::min(length - 1, unit.index + 1);
        for (int i = neighborStart; i <= neighborEnd; ++i) {
            threeNeighborSum += line[static_cast<std::size_t>(offset + i)];
        }

        // TSACore UpdatePeakPrpt carries pSignalProfile/pAverageProfile-derived
        // values into UpdatePeakNoisePrpt.  HPP2 line mode keeps the observable
        // equivalents here: bounded region sum, 3-neighbor sum, and the local
        // average-baseline approximation used by the current CMN-subtracted path.
        const uint16_t peakSample = line[static_cast<std::size_t>(offset + unit.index)];
        const uint16_t avgBaseline = baselineMin;
        const uint16_t peakSignal = peakSample > avgBaseline ? static_cast<uint16_t>(peakSample - avgBaseline) : 1;
        const uint32_t net = regionSum - static_cast<uint32_t>(unit.width) * avgBaseline;
        unit.signalRegionSum = regionSum;
        unit.threeNeighborSum = static_cast<int>(std::min<uint32_t>(threeNeighborSum, 0x7fffffffu));
        unit.avgBaseline = avgBaseline;
        unit.peakSignal = peakSignal;
        unit.netSignal = static_cast<uint16_t>(std::min<uint32_t>(net, 0xffffu));
    }

    static int GetPeakPos(int groupId,
                          const std::array<uint16_t, kMaxSamples>& line,
                          int offset,
                          int length,
                          const PeakUnit& unit) {
        const int edgeThresholdLast = groupId == 0 ? 5000 : 4500;
        const int edgeThresholdFirst = groupId == 0 ? 5000 : 3700;
        // TSACore GetPeakPos (0x6baad7cb) takes the gravity-data path:
        // GetFictiousEdge(peakGroupId, peakIndex) -> UpdateTX1GravityData or
        // UpdateTX2GravityData -> Gravity -> coarseIndex * 0x400 + gravityOffset.
        // This rebuild intentionally keeps the triangle coordinate path because,
        // in HPP2 line mode, SolveByTriangle produces the same coarse + sub-pitch
        // position class from peakPos and neighbor samples without materializing
        // TSACore's transient gravity buffers.
        return Hpp2CoordinateMath::SolveByTriangle(line, offset, length, unit.index, 50, edgeThresholdLast, edgeThresholdFirst);
    }

    static void UpdatePeakNoiseFlags(const Settings& settings, PeakUnit& unit) {
        // Not TSACore UpdatePeakNoisePrpt (0x6babddff).  The original computes
        // GetSignalUnstableLevel(threeNeighborSum, historyAvgSignal,
        // otherDimHistoryAvgSignal), last-output coordinate distance, and an SS
        // matrix recheck condition.  Those inputs are cross-frame/HPP3-grid
        // history artifacts; HPP2 line mode currently only needs the local
        // width-based noise flags below.
        unit.noiseProp = 0;
        if (unit.width < settings.peakMinWidth) {
            unit.noiseProp |= 0x01;
        }
        if (unit.width > settings.peakMaxWidth) {
            unit.noiseProp |= 0x02;
        }
        const uint32_t baselineAdjustedRegion =
            unit.signalRegionSum - static_cast<uint32_t>(unit.width) * unit.avgBaseline;
        if (unit.peakSignal > 1000 && baselineAdjustedRegion > static_cast<uint32_t>(unit.peakSignal) * 3u) {
            unit.noiseProp |= 0x04;
        }
    }

    static Peak FindPeak(const Settings& settings,
                         const std::array<uint16_t, kMaxSamples>& line,
                         int offset,
                         int length) {
        Peak peak{};
        for (int i = 0; i < length; ++i) {
            if (!IsLocalPeak(settings, line, offset, length, i)) {
                continue;
            }
            PeakUnit unit{};
            unit.valid = true;
            unit.index = i;
            SearchPeakBoundary(line, offset, length, i, unit);
            UpdatePeakPrpt(line, offset, length, unit);
            if (unit.netSignal < settings.peakSignalFloor || unit.width < settings.peakMinWidth || unit.width > settings.peakMaxWidth) {
                continue;
            }
            if (!peak.valid || unit.peakSignal > peak.signal) {
                peak.index = unit.index;
                peak.signal = unit.peakSignal;
                peak.valid = true;
            }
        }
        return peak;
    }
};

} // namespace Solvers::Stylus::Hpp2
