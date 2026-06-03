#pragma once

#include "StylusSolver/AsaTypes.hpp"
#include "SolverTypes.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace Solvers::Stylus {

class LinearFilterProcess {
public:
    bool m_enabled = true;
    int m_sparseMoveThreshold = 64;
    int m_shortMoveThreshold = 16;
    int m_minFitPoints = 20;
    int m_anchorMoveThreshold = 32;
    int m_enterCountMax = 10;
    int m_exitCountMax = 10;
    int m_dragLimit = 32;
    int m_enterMaxDistSq = 900;
    int m_exitDistSq = 3600;
    int m_exitCos1000 = 700;
    int m_reverseCos1000 = -500;
    int m_sensorDim1Limit = 61440;
    int m_sensorDim2Limit = 40960;

    inline void Reset() {
        ResetStrokeState();
        m_rawX.fill(kInvalidCoord);
        m_rawY.fill(kInvalidCoord);
        m_filtX.fill(kInvalidCoord);
        m_filtY.fill(kInvalidCoord);
        m_rawHistoryHead = 0;
        m_filtHistoryHead = 0;
        m_anchorX = 0;
        m_anchorY = 0;
        m_prevPressureActive = false;
        m_historyCount = 0;
    }

    inline void ResetStrokeState() {
        m_state = 0;
        m_enterCnt = 0;
        m_exitCnt = 0;
        m_straightCount = 0;
        m_shortCount = 0;
        m_straightStart = 0;
        m_shortStart = 0;
        m_shortTotalCount = 0;
        m_hasStraightLast = false;
        m_hasShortLast = false;
        m_curLineFit = {};
        m_prevLineFit = {};
        m_anchor = {};
        m_segStart = {};
        m_segLast = {};
        m_current = {};
        m_output = {};
        m_active = false;
        m_lastDeltaDim1 = 0;
        m_lastDeltaDim2 = 0;
        m_frame = 0;
#if EGOTOUCH_DIAG
        m_lastCos1000 = 0;
        m_lastDragApplied = 0;
#endif
    }

    inline void Process(HeatmapFrame& frame) {
        auto& runtime = frame.stylus.runtime;
        const auto& historyRaw = runtime.tx1.coordinate.reportGlobalCoor;
        const auto& filterInput = runtime.post.edgePostCoor.valid ? runtime.post.edgePostCoor : historyRaw;
        const bool pressureActive = runtime.pressure.outputPressure > 0;

        if (!m_enabled || !historyRaw.valid || !filterInput.valid) {
            Reset();
            runtime.post.postCoor = historyRaw;
            runtime.post.predictedCoor = historyRaw;
            runtime.post.finalCoor = historyRaw;
            runtime.post.finalValid = historyRaw.valid;
            runtime.post.linearFilterState = 0;
            runtime.post.linearFilterActive = false;
            runtime.post.linearFilterDeltaDim1 = 0;
            runtime.post.linearFilterDeltaDim2 = 0;
#if EGOTOUCH_DIAG
            runtime.post.lfLineFitSlopeA = 0.f;
            runtime.post.lfLineFitInterceptB = 0.f;
            runtime.post.lfLineFitValid = false;
            runtime.post.lfCos1000 = 0;
            runtime.post.lfStraightBufCount = 0;
            runtime.post.lfDragApplied = 0;
#endif
            return;
        }

        // ── GetRealTimeCoor2Buf ──
        // Keep coordinate history live for both hover and contact frames.
        const bool wasPressureActive = m_prevPressureActive;
        if (!wasPressureActive && pressureActive) {
            m_anchorX = historyRaw.dim1;
            m_anchorY = historyRaw.dim2;
        }

        PushRawHistory(historyRaw.dim1, historyRaw.dim2);

        // ── Get3PointAvgFilter ──
        // 3-point moving average with sentinel guard (INT32_MAX = invalid/missing data)
        Asa::AsaCoorResult avg3{};
        if (m_historyCount >= 3) {
            const bool x2Valid = RawX(2) != kInvalidCoord;
            const bool y2Valid = RawY(2) != kInvalidCoord;
            avg3.valid = true;

            if (x2Valid) {
                avg3.dim1 = (RawX(0) + RawX(1) + RawX(2)) / 3;
            } else {
                avg3.dim1 = RawX(0);
            }

            if (y2Valid) {
                avg3.dim2 = (RawY(0) + RawY(1) + RawY(2)) / 3;
            } else {
                avg3.dim2 = RawY(0);
            }
        } else {
            avg3 = historyRaw;
        }

        PushFilteredHistory(avg3.dim1, avg3.dim2);
        runtime.post.postCoor = avg3;

        // Quadratic extrapolation prediction: 3*x0 − 3*x1 + x2
        Asa::AsaCoorResult predicted;
        if (m_historyCount >= 3) {
            predicted.valid = true;
            predicted.dim1 = 3 * RawX(0) - 3 * RawX(1) + RawX(2);
            predicted.dim2 = 3 * RawY(0) - 3 * RawY(1) + RawY(2);
        } else {
            predicted = avg3;
        }
        runtime.post.predictedCoor = predicted;

        if (!pressureActive) {
            if (wasPressureActive) {
                ResetStrokeState();
            }
            m_prevPressureActive = false;
            runtime.post.finalCoor = historyRaw;
            runtime.post.finalValid = historyRaw.valid;
            runtime.post.linearFilterState = 0;
            runtime.post.linearFilterActive = false;
            runtime.post.linearFilterDeltaDim1 = 0;
            runtime.post.linearFilterDeltaDim2 = 0;
#if EGOTOUCH_DIAG
            runtime.post.lfLineFitSlopeA = 0.f;
            runtime.post.lfLineFitInterceptB = 0.f;
            runtime.post.lfLineFitValid = false;
            runtime.post.lfCos1000 = 0;
            runtime.post.lfStraightBufCount = 0;
            runtime.post.lfDragApplied = 0;
#endif
            return;
        }

        m_prevPressureActive = true;

        // ── Linear correction consumes the staged coordinate path. avg3/predicted stay as
        // diagnostics and side-channel trend data, mirroring the original split.
        const Asa::AsaCoorResult result = Process(
            filterInput, pressureActive, m_sensorDim1Limit, m_sensorDim2Limit);

        runtime.post.finalCoor = result;
        runtime.post.finalValid = result.valid;
        runtime.post.linearFilterState = m_state;
        runtime.post.linearFilterActive = m_active;
        runtime.post.linearFilterDeltaDim1 = m_lastDeltaDim1;
        runtime.post.linearFilterDeltaDim2 = m_lastDeltaDim2;
#if EGOTOUCH_DIAG
        runtime.post.lfLineFitSlopeA = static_cast<float>(m_curLineFit.slopeA);
        runtime.post.lfLineFitInterceptB = static_cast<float>(m_curLineFit.interceptB);
        runtime.post.lfLineFitValid = m_curLineFit.valid;
        runtime.post.lfCos1000 = m_lastCos1000;
        runtime.post.lfStraightBufCount = m_straightCount;
        runtime.post.lfDragApplied = m_lastDragApplied;
#endif
    }

    inline Asa::AsaCoorResult Process(const Asa::AsaCoorResult& raw,
                                      bool pressureActive,
                                      int sensorDim1Limit,
                                      int sensorDim2Limit) {
        Asa::AsaCoorResult result = raw;
        m_active = false;
        m_lastDeltaDim1 = 0;
        m_lastDeltaDim2 = 0;

        if (!m_enabled || !raw.valid || !pressureActive) {
            Reset();
            return result;
        }

        m_current = {raw.dim1, raw.dim2, m_frame++};
        m_output = m_current;

        if (m_straightCount > std::max(1, m_minFitPoints) - 1) {
            UpdateStraightLinePrmt(m_curLineFit, true);
            UpdateStraightLinePrmt(m_prevLineFit, false);
        }

        if (m_state > 6) {
            m_state = 3;
        }

        switch (m_state) {
        case 0:
            m_state = 1;
            break;
        case 1:
            m_state = 2;
            break;
        case 2:
            m_state = 3;
            break;
        case 3:
            CurveLineProcess();
            break;
        case 4:
            EnterStraightLineProcess();
            break;
        case 5:
            StraightLineProcess();
            break;
        case 6:
            ExitStraightLineProcess();
            break;
        default:
            m_state = 3;
            break;
        }

        BufferStraightPaintPoint(m_current);
        BufferShortDistancePoint(m_current);

        result.dim1 = std::clamp(m_output.x, 0, std::max(0, sensorDim1Limit));
        result.dim2 = std::clamp(m_output.y, 0, std::max(0, sensorDim2Limit));
        result.valid = true;
        m_lastDeltaDim1 = result.dim1 - raw.dim1;
        m_lastDeltaDim2 = result.dim2 - raw.dim2;
        m_active = m_active || m_lastDeltaDim1 != 0 || m_lastDeltaDim2 != 0;
        return result;
    }

    uint8_t State() const { return m_state; }
    bool Active() const { return m_active; }
    int LastDeltaDim1() const { return m_lastDeltaDim1; }
    int LastDeltaDim2() const { return m_lastDeltaDim2; }
#if EGOTOUCH_DIAG
    double LineFitSlopeA() const { return m_curLineFit.slopeA; }
    double LineFitInterceptB() const { return m_curLineFit.interceptB; }
    bool LineFitValid() const { return m_curLineFit.valid; }
    int32_t LastCos1000() const { return m_lastCos1000; }
    int32_t StraightBufCount() const { return m_straightCount; }
    int32_t LastDragApplied() const { return m_lastDragApplied; }
#endif

private:
    struct LinearPoint {
        int32_t x = 0;
        int32_t y = 0;
        uint16_t frame = 0;
    };

    struct LineFitParam {
        double slopeA = 0.0;
        double interceptB = 0.0;
        double norm = 1.0;
        bool swapXY = false;
        int32_t sumDistSq = 0;
        int32_t maxDistSq = 0;
        int32_t lastDistSq = 0;
        bool valid = false;
    };

    static constexpr int kStraightCapacity = 400;
    static constexpr int kShortCapacity = 20;

    std::array<LinearPoint, kStraightCapacity> m_straightBuf{};
    std::array<LinearPoint, kShortCapacity> m_shortBuf{};
    int m_straightStart = 0;
    int m_shortStart = 0;
    int m_straightCount = 0;
    int m_shortCount = 0;
    int m_shortTotalCount = 0;
    bool m_hasStraightLast = false;
    bool m_hasShortLast = false;
    LinearPoint m_prevStraight{};
    LinearPoint m_lastStraight{};
    LinearPoint m_prevShort{};
    LinearPoint m_lastShort{};
    LinearPoint m_current{};
    LinearPoint m_output{};
    LinearPoint m_anchor{};
    LinearPoint m_segStart{};
    LinearPoint m_segLast{};
    LineFitParam m_curLineFit{};
    LineFitParam m_prevLineFit{};
    uint8_t m_state = 0;
    int m_enterCnt = 0;
    int m_exitCnt = 0;
    uint16_t m_frame = 0;
    bool m_active = false;
    int m_lastDeltaDim1 = 0;
    int m_lastDeltaDim2 = 0;

    // ── TSACore GetRealTimeCoor2Buf / Get3PointAvgFilter state ──
    static constexpr int kRawHistorySize = 24;
    static constexpr int32_t kInvalidCoord = 0x7FFFFFFF;

    std::array<int32_t, kRawHistorySize> m_rawX{};
    std::array<int32_t, kRawHistorySize> m_rawY{};
    std::array<int32_t, kRawHistorySize> m_filtX{};
    std::array<int32_t, kRawHistorySize> m_filtY{};
    int m_rawHistoryHead = 0;
    int m_filtHistoryHead = 0;
    int32_t m_anchorX = 0;
    int32_t m_anchorY = 0;
    bool m_prevPressureActive = false;
    int m_historyCount = 0;

#if EGOTOUCH_DIAG
    int32_t m_lastCos1000 = 0;
    int32_t m_lastDragApplied = 0;
#endif

    static inline int AbsDiff(int32_t a, int32_t b) {
        return static_cast<int>(std::abs(static_cast<int64_t>(a) - static_cast<int64_t>(b)));
    }

    inline bool MovedFrom(const LinearPoint& point, int threshold) const {
        return AbsDiff(m_current.x, point.x) > threshold ||
               AbsDiff(m_current.y, point.y) > threshold;
    }

    static inline int32_t DistToInt(double value) {
        if (value <= 0.0) return 0;
        if (value >= static_cast<double>(std::numeric_limits<int32_t>::max())) {
            return std::numeric_limits<int32_t>::max();
        }
        return static_cast<int32_t>(static_cast<int64_t>(value));
    }

    inline void PushRawHistory(int32_t x, int32_t y) {
        if (m_historyCount != 0) {
            m_rawHistoryHead = (m_rawHistoryHead + kRawHistorySize - 1) % kRawHistorySize;
        }
        m_rawX[static_cast<std::size_t>(m_rawHistoryHead)] = x;
        m_rawY[static_cast<std::size_t>(m_rawHistoryHead)] = y;
        if (m_historyCount < kRawHistorySize) {
            ++m_historyCount;
        }
    }

    inline void PushFilteredHistory(int32_t x, int32_t y) {
        if (m_historyCount > 1) {
            m_filtHistoryHead = (m_filtHistoryHead + kRawHistorySize - 1) % kRawHistorySize;
        }
        m_filtX[static_cast<std::size_t>(m_filtHistoryHead)] = x;
        m_filtY[static_cast<std::size_t>(m_filtHistoryHead)] = y;
    }

    inline std::size_t RawHistoryIndex(int logicalIndex) const {
        return static_cast<std::size_t>((m_rawHistoryHead + logicalIndex) % kRawHistorySize);
    }

    inline int32_t RawX(int logicalIndex) const {
        return m_rawX[RawHistoryIndex(logicalIndex)];
    }

    inline int32_t RawY(int logicalIndex) const {
        return m_rawY[RawHistoryIndex(logicalIndex)];
    }

    inline std::size_t StraightIndex(int logicalIndex) const {
        return static_cast<std::size_t>((m_straightStart + logicalIndex) % kStraightCapacity);
    }

    inline const LinearPoint& StraightAt(int logicalIndex) const {
        return m_straightBuf[StraightIndex(logicalIndex)];
    }

    inline std::size_t ShortIndex(int logicalIndex) const {
        return static_cast<std::size_t>((m_shortStart + logicalIndex) % kShortCapacity);
    }

    inline void CurveLineProcess() {
        const int minPoints = std::max(2, m_minFitPoints);
        if (m_straightCount > minPoints) {
            TrimStraightTo(minPoints);
        }
        if (m_straightCount >= minPoints && m_curLineFit.valid &&
            m_curLineFit.maxDistSq < m_enterMaxDistSq) {
            m_anchor = m_current;
            m_state = 4;
            m_enterCnt = std::max(0, m_enterCountMax);
        }
    }

    inline void EnterStraightLineProcess() {
        const int enterMax = std::max(1, m_enterCountMax);
        if (m_enterCnt <= 0) {
            m_state = 5;
            m_enterCnt = 0;
        }
        if (m_enterCnt != 0 && MovedFrom(m_anchor, m_anchorMoveThreshold)) {
            --m_enterCnt;
            m_anchor = m_current;
        }
        if (m_curLineFit.valid && GetPoint2LineDisSquare(m_curLineFit, m_current) > m_exitDistSq) {
            m_state = 6;
            m_exitCnt = std::clamp(m_exitCountMax - m_enterCnt, 0, std::max(0, m_exitCountMax));
        }
        const int drag = m_dragLimit - (m_dragLimit * std::clamp(m_enterCnt, 0, enterMax)) / enterMax;
        DragPoint2Line(m_curLineFit, drag);
    }

    inline void StraightLineProcess() {
        if (m_straightCount > 0) {
            m_segStart = StraightAt(0);
            m_segLast = StraightAt(m_straightCount - 1);
        }

        const double distSq = m_curLineFit.valid ? GetPoint2LineDisSquare(m_curLineFit, m_current) : 0.0;
        const int cos1000 = GetCurrentLineAngleCos1000();
#if EGOTOUCH_DIAG
        m_lastCos1000 = static_cast<int32_t>(cos1000);
#endif
        if (cos1000 < m_exitCos1000 || distSq > static_cast<double>(m_exitDistSq)) {
            m_anchor = m_segLast;
            m_state = 6;
            m_exitCnt = std::max(0, m_exitCountMax);
        }

        int drag = m_dragLimit;
        if (cos1000 < m_reverseCos1000) {
            if (m_exitCnt > 0) --m_exitCnt;
            const int exitMax = std::max(1, m_exitCountMax);
            drag = (std::clamp(m_exitCnt, 0, exitMax) * m_dragLimit) / exitMax;
        }
        DragPoint2Line(m_prevLineFit.valid ? m_prevLineFit : m_curLineFit, drag);
    }

    inline void ExitStraightLineProcess() {
        const int minPoints = std::max(2, m_minFitPoints);
        if (m_exitCnt <= 0) {
            m_state = 3;
            TrimStraightTo(minPoints);
            m_exitCnt = 0;
        }
        if (m_exitCnt != 0 && MovedFrom(m_anchor, m_anchorMoveThreshold)) {
            const int cos1000 = GetCurrentLineAngleCos1000();
#if EGOTOUCH_DIAG
            m_lastCos1000 = static_cast<int32_t>(cos1000);
#endif
            --m_exitCnt;
            if (m_exitCnt != 0 && cos1000 < m_reverseCos1000) {
                --m_exitCnt;
            }
            m_exitCnt = std::max(0, m_exitCnt);
            m_anchor = m_current;
        }
        if (m_straightCount >= minPoints && m_curLineFit.valid &&
            m_curLineFit.maxDistSq < m_enterMaxDistSq) {
            const int enterMax = std::max(0, m_enterCountMax);
            m_state = 4;
            m_enterCnt = std::clamp(enterMax - m_exitCnt, 0, enterMax);
        }
        const int exitMax = std::max(1, m_exitCountMax);
        const int drag = (std::clamp(m_exitCnt, 0, exitMax) * m_dragLimit) / exitMax;
        DragPoint2Line(m_curLineFit, drag);
    }

    inline void BufferStraightPaintPoint(const LinearPoint& point) {
        if (m_hasStraightLast &&
            AbsDiff(point.x, m_lastStraight.x) <= m_sparseMoveThreshold &&
            AbsDiff(point.y, m_lastStraight.y) <= m_sparseMoveThreshold) {
            return;
        }
        const bool incrementCount = m_state != 4 && m_state != 6;
        AppendStraight(point, incrementCount);
    }

    inline void BufferShortDistancePoint(const LinearPoint& point) {
        if (m_hasShortLast &&
            AbsDiff(point.x, m_lastShort.x) <= m_shortMoveThreshold &&
            AbsDiff(point.y, m_lastShort.y) <= m_shortMoveThreshold) {
            return;
        }
        if (m_shortCount < kShortCapacity) {
            m_shortBuf[ShortIndex(m_shortCount)] = point;
            ++m_shortCount;
        } else {
            m_shortStart = (m_shortStart + 1) % kShortCapacity;
            m_shortBuf[ShortIndex(m_shortCount - 1)] = point;
        }
        m_shortTotalCount = std::min(m_shortTotalCount + 1, 100);
        m_prevShort = m_hasShortLast ? m_lastShort : point;
        m_lastShort = point;
        m_hasShortLast = true;
    }

    inline void AppendStraight(const LinearPoint& point, bool incrementCount) {
        if (m_straightCount == 0) {
            m_straightStart = 0;
            m_straightBuf[0] = point;
            m_straightCount = 1;
        } else if (incrementCount && m_straightCount < kStraightCapacity) {
            m_straightBuf[StraightIndex(m_straightCount)] = point;
            ++m_straightCount;
        } else {
            m_straightStart = (m_straightStart + 1) % kStraightCapacity;
            m_straightBuf[StraightIndex(m_straightCount - 1)] = point;
        }
        RefreshStraightEndpoints();
    }

    inline void TrimStraightTo(int count) {
        count = std::clamp(count, 0, kStraightCapacity);
        if (m_straightCount <= count) return;
        if (count == 0) {
            m_straightStart = 0;
            m_straightCount = 0;
            RefreshStraightEndpoints();
            return;
        }
        m_straightStart = static_cast<int>(StraightIndex(m_straightCount - count));
        m_straightCount = count;
        RefreshStraightEndpoints();
    }

    inline void RefreshStraightEndpoints() {
        if (m_straightCount <= 0) {
            m_hasStraightLast = false;
            return;
        }
        m_prevStraight = m_straightCount > 1 ? StraightAt(m_straightCount - 2)
                                             : StraightAt(m_straightCount - 1);
        m_lastStraight = StraightAt(m_straightCount - 1);
        m_hasStraightLast = true;
    }

    inline void UpdateStraightLinePrmt(LineFitParam& fit, bool includeCurrentPoint) const {
        fit = {};
        if (m_straightCount <= 1) return;

        const LinearPoint origin = StraightAt(m_straightCount - 1);
        const int sampleCount = m_straightCount + (includeCurrentPoint ? 1 : 0);
        if (sampleCount <= 1) return;

        double sumX = 0.0;
        double sumY = 0.0;
        double sumXX = 0.0;
        double sumXY = 0.0;
        double sumYY = 0.0;

        auto addFitSample = [&](const LinearPoint& point) {
            const double dx = static_cast<double>(point.x - origin.x);
            const double dy = static_cast<double>(point.y - origin.y);
            sumX += dx;
            sumY += dy;
            sumXX += dx * dx;
            sumXY += dx * dy;
            sumYY += dy * dy;
        };

        for (int i = 0; i < m_straightCount; ++i) {
            addFitSample(StraightAt(i));
        }
        if (includeCurrentPoint) {
            addFitSample(m_current);
        }

        const double invCount = 1.0 / static_cast<double>(sampleCount);
        const double covXY = sumXY - (sumX * sumY) * invCount;
        const double covXX = sumXX - (sumX * sumX) * invCount;
        const double covYY = sumYY - (sumY * sumY) * invCount;
        constexpr double kEps = 1e-9;

        if (std::abs(covXX) <= kEps && std::abs(covYY) <= kEps) {
            return;
        }

        if (covXX <= covYY) {
            fit.swapXY = true;
            fit.slopeA = std::abs(covYY) <= kEps ? 0.0 : covXY / covYY;
            fit.interceptB = (sumX * invCount + static_cast<double>(origin.x)) -
                             fit.slopeA * (static_cast<double>(origin.y) + sumY * invCount);
        } else {
            fit.swapXY = false;
            fit.slopeA = std::abs(covXX) <= kEps ? 0.0 : covXY / covXX;
            fit.interceptB = (sumY * invCount + static_cast<double>(origin.y)) -
                             fit.slopeA * (static_cast<double>(origin.x) + sumX * invCount);
        }

        fit.norm = std::sqrt(fit.slopeA * fit.slopeA + 1.0);
        fit.valid = fit.norm > kEps;
        if (!fit.valid) return;

        auto accumulateDistance = [&](const LinearPoint& point) {
            const double dist = SignedDistance(fit, point);
            const double distSq = dist * dist;
            const int32_t distInt = DistToInt(distSq);
            fit.sumDistSq = static_cast<int32_t>(std::clamp<int64_t>(
                static_cast<int64_t>(fit.sumDistSq) + distInt,
                0,
                std::numeric_limits<int32_t>::max()));
            fit.maxDistSq = std::max(fit.maxDistSq, distInt);
            fit.lastDistSq = distInt;
        };

        for (int i = 0; i < m_straightCount; ++i) {
            accumulateDistance(StraightAt(i));
        }
        if (includeCurrentPoint) {
            accumulateDistance(m_current);
        }
    }

    static inline double SignedDistance(const LineFitParam& fit, const LinearPoint& point) {
        if (!fit.valid || fit.norm == 0.0) return 0.0;
        if (!fit.swapXY) {
            return (fit.slopeA * static_cast<double>(point.x) + fit.interceptB -
                    static_cast<double>(point.y)) /
                   fit.norm;
        }
        return (fit.slopeA * static_cast<double>(point.y) + fit.interceptB -
                static_cast<double>(point.x)) /
               fit.norm;
    }

    static inline double GetPoint2LineDisSquare(const LineFitParam& fit, const LinearPoint& point) {
        const double dist = SignedDistance(fit, point);
        return dist * dist;
    }

    inline void DragPoint2Line(const LineFitParam& fit, int maxDrag) {
        if (!fit.valid || maxDrag <= 0) return;

        double distance = 0.0;
        double deltaXScale = 0.0;
        double deltaYScale = 0.0;

        // TSACore branches on previous-fit axis even when dragging current fit.
        if (!m_prevLineFit.swapXY) {
            distance = (fit.interceptB + fit.slopeA * static_cast<double>(m_output.x) -
                        static_cast<double>(m_output.y)) /
                       fit.norm;
            deltaXScale = -fit.slopeA / fit.norm;
            deltaYScale = 1.0 / fit.norm;
        } else {
            distance = (fit.interceptB + fit.slopeA * static_cast<double>(m_output.y) -
                        static_cast<double>(m_output.x)) /
                       fit.norm;
            deltaYScale = -fit.slopeA / fit.norm;
            deltaXScale = 1.0 / fit.norm;
        }

        distance = std::clamp(distance, -static_cast<double>(maxDrag), static_cast<double>(maxDrag));
#if EGOTOUCH_DIAG
        m_lastDragApplied = static_cast<int32_t>(std::abs(distance));
#endif
        const int32_t nextX = static_cast<int32_t>(deltaXScale * distance + static_cast<double>(m_output.x));
        const int32_t nextY = static_cast<int32_t>(deltaYScale * distance + static_cast<double>(m_output.y));
        m_active = m_active || nextX != m_output.x || nextY != m_output.y;
        m_output.x = nextX;
        m_output.y = nextY;
    }

    inline int GetCurrentLineAngleCos1000() const {
        if (m_straightCount < 2) {
            return std::numeric_limits<int32_t>::max();
        }
        const LinearPoint start = StraightAt(0);
        const LinearPoint last = StraightAt(m_straightCount - 1);
        LinearPoint second = last;
        if (AbsDiff(last.x, m_current.x) < 33 && AbsDiff(last.y, m_current.y) < 33 &&
            m_straightCount > 1) {
            second = StraightAt(m_straightCount - 2);
        }
        return GetTwoLineAngle(start, last, second, m_current);
    }

    static inline int GetTwoLineAngle(const LinearPoint& a0,
                                      const LinearPoint& a1,
                                      const LinearPoint& b0,
                                      const LinearPoint& b1) {
        const int64_t v1x = static_cast<int64_t>(a0.x) - a1.x;
        const int64_t v1y = static_cast<int64_t>(a0.y) - a1.y;
        const int64_t v2x = static_cast<int64_t>(b0.x) - b1.x;
        const int64_t v2y = static_cast<int64_t>(b0.y) - b1.y;
        const double len1 = std::sqrt(static_cast<double>(v1x * v1x + v1y * v1y));
        const double len2 = std::sqrt(static_cast<double>(v2x * v2x + v2y * v2y));
        const int64_t denom = static_cast<int64_t>(len1 * len2);
        if (denom == 0) {
            return std::numeric_limits<int32_t>::max();
        }
        const int64_t dot = v1x * v2x + v1y * v2y;
        return static_cast<int>((dot * 1000) / denom);
    }
};

} // namespace Solvers::Stylus
