#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

namespace Asa {

// ── Constants ──
static constexpr int kNpiMaxGridCols = 80;
static constexpr int kNpiMaxGridRows = 80;
static constexpr int kNpiMaxGridCells = kNpiMaxGridCols * kNpiMaxGridRows;
static constexpr int kNpiHistoryBufSize = 32;
static constexpr int kNpiTiltBins = 64;

/// Input data for NoPressInkGate::Process().
struct NoPressInkInput {
    bool     coordValid = false;
    bool     tx1BlockValid = false;
    bool     lowSignalSuppressed = false;
    bool     tx2Started = false;        // TSACore g_flagTX2Start equivalent for current frame
    uint16_t realPressure = 0;          // from PressureSolver (after suppression)
    uint16_t prevOutputPressure = 0;    // previous frame's final output pressure
    uint16_t rawPressure = 0;           // raw BT MCU value (for learning guard)
    // Dual-channel signals (§4: EnterToNoPressInk / ExitToNoPressInk)
    uint16_t tx1SignalDim1 = 0;         // dim1 projection peak signal
    uint16_t tx1SignalDim2 = 0;         // dim2 projection peak signal
    uint16_t tx1Composite = 0;          // combined TX1 signal
    uint16_t tx2Composite = 0;          // TX2 peak signal (tilt proxy)
    // Coordinate (for learned table lookup)
    int32_t  coorDim1 = 0;              // Q10 global coordinate
    int32_t  coorDim2 = 0;              // Q10 global coordinate
    // Cross-frame state
    uint8_t  prevStaticBits = 0;        // previous frame's static bits
};

/// Output from NoPressInkGate::Process().
struct NoPressInkResult {
    bool     active = false;            // no-press ink state is active
    uint16_t outputPressure = 0;        // = realPressure unless active补压
    uint16_t enterThreshold = 0;        // diagnostic: current enter threshold
    uint16_t exitThreshold = 0;         // diagnostic: current exit threshold
    uint16_t tiltCompensation = 0;      // diagnostic: tilt compensation value
    uint16_t learnedBase = 0;           // diagnostic: learned base threshold
    uint16_t tableMaxSignal = 0;        // diagnostic: current table max
    bool     learningReady = false;     // diagnostic: g_noPressPara != 0
};

/// NoPressInkGate v2 — Self-learning NoPressInk with tilt compensation.
///
/// 1:1 mirrors TSACore NoPressInkProcess (§4), including:
///   - Three-stage learning: prepare → short-term → long-term
///   - Tilt compensation with online scale fitting
///   - Signal abnormality detection (4-bit guard)
///   - Dual-channel enter/exit threshold with hysteresis debounce
///
/// The learning feedback loop:
///   learned_table → GetLearnedThreshold(x,y) → base + tiltComp
///   → enter/exit thresholds → faster pen-down / cleaner pen-up
class NoPressInkGate {
public:

    // ════════════════════════════════════════════════
    // Main entry
    // ════════════════════════════════════════════════

    /// Process one frame. Call after PressureSolver, before PostPressureProcess.
    inline NoPressInkResult Process(const NoPressInkInput& in) {
        NoPressInkResult out{};
        out.tableMaxSignal = m_tableMaxSignal;
        out.learningReady = (m_noPressPara != 0);

        if (!enabled || !in.coordValid || !in.tx1BlockValid) {
            ResetFrame();
            out.outputPressure = in.realPressure;
            return out;
        }

        // §4: NoPressInkProcess
        if (!tLearnedFeatureEnabled || m_noPressPara != 0) {
            HandleNoPressInk(in, out);
        } else {
            m_active = false;
        }

        // §4: Learning chain
        if (tLearnedFeatureEnabled) {
            if (m_noPressPara == 0) {
                LearnPrepare(in);
            } else {
                LearnOnline(in);
            }
        }

        out.active = m_active;
        out.outputPressure = in.realPressure;
        // Note: actual pressure补压 is done by PostPressureProcess,
        // not here. We only report whether NoPressInk is active.
        return out;
    }

    // ════════════════════════════════════════════════
    // Reset (two-level, solving conflict point 5)
    // ════════════════════════════════════════════════

    /// Frame-level reset: clears debounce/active state but preserves learned tables.
    /// Called by Pipeline on early-exit paths (no signal, parse fail, noise jump, etc.)
    inline void ResetFrame() {
        m_active = false;
        m_enterStreak = 0;
        m_exitStreak = 0;
        m_learningStableCounter = 0;
        m_prevRealPressFlag = false;
        m_tx2Started = false;
    }

    /// Full reset: clears everything including learned tables.
    /// Called only on config change or explicit re-initialization.
    inline void ResetAll() {
        ResetFrame();
        m_noPressPara = 0;
        m_tableMaxSignal = 0;
        m_shortTermMax = 0;
        m_abnormalFlags = 0;
        m_histCount = 0;
        m_prepareDownCount = 0;
        m_prepareMaxSignal = 0;
        m_prepareSampleCount = 0;

        std::memset(m_learnedLT, 0, sizeof(m_learnedLT));
        std::memset(m_learnedLT_tx2, 0, sizeof(m_learnedLT_tx2));
        std::memset(m_shortTerm, 0, sizeof(m_shortTerm));
        std::memset(m_shortTerm_tx2, 0, sizeof(m_shortTerm_tx2));
        std::memset(m_axisX, 0, sizeof(m_axisX));
        std::memset(m_axisY, 0, sizeof(m_axisY));
        std::memset(m_histTx1, 0, sizeof(m_histTx1));
        std::memset(m_histTx2, 0, sizeof(m_histTx2));

        ResetTiltLearning();
    }

    /// Backward-compatible alias (maps to ResetFrame for safety).
    inline void Reset() { ResetFrame(); }

    // ════════════════════════════════════════════════
    // Configuration
    // ════════════════════════════════════════════════

    bool enabled = false;               // ASA feature bit6 (NoPressInk enabled)
    bool tLearnedFeatureEnabled = false; // ASA feature bit7 (learned table mode)

    // Grid dimensions
    int  gridCols = 60;                 // DAT_1820d610
    int  gridRows = 40;                 // DAT_1820d611

    // Base threshold (used when learned table is empty)
    int  baseThreshold = 10000;         // g_asaPrmtStylus + 0x240

    // Enter/exit ratio (percent of base+tiltComp)
    int  enterRatioPercent = 100;       // g_asaPrmtStylus + 0x244
    int  exitRatioPercent = 30;         // g_asaPrmtStylus + 0x245

    // Tilt compensation
    int  tiltDeadzone = 1000;           // g_asaPrmtStylus + 0x246
    int  tiltCap = 10000;              // g_asaPrmtStylus + 0x248
    int  tiltScaleInit = 29;           // g_asaPrmtStylus + 0x24A
    bool tiltLearnStrict = true;        // g_asaPrmtStylus + 0x24B

    // Debounce
    int  enterDebounceFrames = 2;       // TSACore default
    int  exitDebounceFrames = 2;        // TSACore default

    // Dual-channel mode (gaokunhimaxcsot: +0xA50 = 1)
    bool dualChannelMode = true;

    // Synthetic min pressure when NoPressInk补压
    int  syntheticMinPressure = 10;

    // CoorRevise feature check
    bool coorReviseEnabled = true;

    /// If enabled, a current-frame TX2-start drop hard-clears active no-press ink.
    bool fastLiftHardClear = true;

private:

    // ════════════════════════════════════════════════
    // §4: NoPressInkHandle — core enter/exit logic
    // ════════════════════════════════════════════════

    inline void HandleNoPressInk(const NoPressInkInput& in, NoPressInkResult& out) {
        BufferSignalHistory(in);
        CheckSignalAbnormal(in);

        // §4: IsTiltLearnedOK guard
        if (tLearnedFeatureEnabled && !IsTiltLearnedOK(in)) {
            m_active = false;
            return;
        }

        // §4.1: Tilt compensation
        uint16_t tiltComp = 0;
        if (in.tx2Composite > 0) {
            tiltComp = CalcTiltCompensation(in.tx2Composite);
        }
        out.tiltCompensation = tiltComp;

        // §4: UpdateNoPressInkThold
        uint16_t base = 0;
        if (m_noPressPara != 0) {
            base = GetLearnedThreshold(in.coorDim1, in.coorDim2);
        } else {
            base = static_cast<uint16_t>(std::clamp(baseThreshold, 0, 0xFFFF));
        }
        out.learnedBase = base;

        const int total = static_cast<int>(base) + static_cast<int>(tiltComp);
        const int enterTh = total * std::max(0, enterRatioPercent) / 100;
        const int exitTh  = total * std::max(0, exitRatioPercent) / 100;
        out.enterThreshold = static_cast<uint16_t>(std::clamp(enterTh, 0, 0xFFFF));
        out.exitThreshold  = static_cast<uint16_t>(std::clamp(exitTh, 0, 0xFFFF));

        // §4: Enter/exit debounce with dual-channel threshold
        if (in.realPressure == 0) {
            // Try to enter
            if (!m_active) {
                if (CheckEnter(in, out.enterThreshold)) {
                    m_enterStreak++;
                } else {
                    m_enterStreak = 0;
                }
                m_exitStreak = 0;
                if (m_enterStreak >= std::max(1, enterDebounceFrames)) {
                    m_active = true;
                    m_exitStreak = 0;
                }
            }
            // Try to exit (check independently, §4 line 1067-1079)
            if (m_active) {
                if (CheckExit(in, out.exitThreshold)) {
                    m_exitStreak++;
                } else {
                    m_exitStreak = 0;
                }
                if (m_exitStreak >= std::max(1, exitDebounceFrames)) {
                    m_active = false;
                    m_enterStreak = 0;
                }
            }
        } else {
            m_enterStreak = 0;
            m_exitStreak = 0;
        }

        // §4 line 1081-1083: CoorRevise guard
        if (coorReviseEnabled && !in.tx2Started) {
            m_active = false;
        }

        // Track TX2 start using current-frame pipeline signal.
        m_tx2Started = in.tx2Started;
    }

    // ════════════════════════════════════════════════
    // §4: EnterToNoPressInk / ExitToNoPressInk
    // ════════════════════════════════════════════════

    /// §4 line 1008-1019: dual-channel enter check
    inline bool CheckEnter(const NoPressInkInput& in, uint16_t enterTh) const {
        if (dualChannelMode) {
            // gaokunhimaxcsot: +0xA50 = 1 → both channels must exceed
            return (in.tx1SignalDim2 > enterTh) && (in.tx1SignalDim1 > enterTh);
        }
        // Fallback: combined
        const int half1 = enterTh / 2;
        const int half2 = enterTh - half1;
        return (half1 + half2) < static_cast<int>(in.tx1Composite);
    }

    /// §4 line 1021-1032: dual-channel exit check
    inline bool CheckExit(const NoPressInkInput& in, uint16_t exitTh) const {
        if (dualChannelMode) {
            return (in.tx1SignalDim2 < exitTh) && (in.tx1SignalDim1 < exitTh);
        }
        const int half1 = exitTh / 2;
        const int half2 = exitTh - half1;
        return static_cast<int>(in.tx1Composite) < (half1 + half2);
    }

    // ════════════════════════════════════════════════
    // §4.1: Tilt compensation
    // ════════════════════════════════════════════════

    /// §4.1 line 1124-1147: GetCompensationByTilt
    inline uint16_t CalcTiltCompensation(uint16_t tx2Signal) const {
        int s = static_cast<int>(tx2Signal);
        const int cap = std::max(0, tiltCap);
        const int deadzone = std::max(0, tiltDeadzone);

        if (s > cap) s = cap;
        if (s < deadzone) return 0;
        s -= deadzone;

        return static_cast<uint16_t>(
            std::clamp(static_cast<int>(m_tiltCompScaleActive) * s / 100, 0, 0xFFFF));
    }

    /// §4 line 969-988: IsTiltLearnedOK
    inline bool IsTiltLearnedOK(const NoPressInkInput& in) const {
        if (!tiltLearnStrict) return true;
        if (m_tiltLearnOK) return true;

        // Fallback: TX2 within learned range + margin
        if (in.tx2Composite < m_tiltLearnMaxTx2 + 200 &&
            m_tiltLearnMinTx2 < in.tx2Composite + 200) {
            return true;
        }
        return false;
    }

    // ════════════════════════════════════════════════
    // §4.1: Learned threshold lookup
    // ════════════════════════════════════════════════

    /// §4.1 line 1155-1200: GetNopressInkTholdFromLearnedTable
    /// 3×3 neighborhood average from learned long-term table.
    inline uint16_t GetLearnedThreshold(int32_t coorDim1, int32_t coorDim2) const {
        const int cols = std::clamp(gridCols, 1, kNpiMaxGridCols);
        const int rows = std::clamp(gridRows, 1, kNpiMaxGridRows);

        int cx = static_cast<int>(coorDim1 >> 10);
        int cy = static_cast<int>(coorDim2 >> 10);
        cx = std::clamp(cx, 0, cols - 1);
        cy = std::clamp(cy, 0, rows - 1);

        const int x0 = std::max(0, cx - 1);
        const int x1 = std::min(cols - 1, cx + 1);
        const int y0 = std::max(0, cy - 1);
        const int y1 = std::min(rows - 1, cy + 1);

        uint32_t sum = 0;
        int cnt = 0;
        for (int yy = y0; yy <= y1; ++yy) {
            for (int xx = x0; xx <= x1; ++xx) {
                const int idx = yy * cols + xx;
                if (idx < kNpiMaxGridCells && m_learnedLT[idx] != 0) {
                    sum += m_learnedLT[idx];
                    cnt++;
                }
            }
        }

        if (cnt > 0) {
            return static_cast<uint16_t>(sum / static_cast<uint32_t>(cnt));
        }
        if (m_tableMaxSignal > 0) {
            return m_tableMaxSignal;
        }
        return static_cast<uint16_t>(std::clamp(baseThreshold, 0, 0xFFFF));
    }

    // ════════════════════════════════════════════════
    // §4.2: Signal abnormality detection
    // ════════════════════════════════════════════════

    inline void BufferSignalHistory(const NoPressInkInput& in) {
        if (m_histCount < kNpiHistoryBufSize) {
            // Shift all entries right
            for (int i = m_histCount; i > 0; --i) {
                m_histTx1[i] = m_histTx1[i - 1];
                m_histTx2[i] = m_histTx2[i - 1];
            }
            m_histCount++;
        } else {
            for (int i = kNpiHistoryBufSize - 1; i > 0; --i) {
                m_histTx1[i] = m_histTx1[i - 1];
                m_histTx2[i] = m_histTx2[i - 1];
            }
        }
        m_histTx1[0] = in.tx1Composite;
        m_histTx2[0] = in.tx2Composite;
    }

    /// §4.2 line 1490-1583: CheckSignalAbnormalStatus
    inline void CheckSignalAbnormal(const NoPressInkInput& in) {
        if (m_histCount < 10) return;

        // bit1/bit2: single-frame jump / short-window accumulated jump
        m_abnormalFlags &= ~(0x02u | 0x04u);

        const int sigNow  = static_cast<int>(m_histTx1[0]);
        const int sigPrev = static_cast<int>(m_histTx1[1]);
        const int tx2Now  = static_cast<int>(m_histTx2[0]);
        const int tx2Prev = static_cast<int>(m_histTx2[1]);

        const int instantJump = std::abs((sigNow + tx2Now) - (sigPrev + tx2Prev));
        if (sigNow > 0 && instantJump > sigNow / 15) {
            m_abnormalFlags |= 0x02;
        }

        int sumJump = 0;
        for (int i = 0; i < 5 && (5 + i) < m_histCount; ++i) {
            sumJump += (static_cast<int>(m_histTx2[5 + i]) -
                        static_cast<int>(m_histTx1[5 + i])) +
                       (static_cast<int>(m_histTx1[i]) -
                        static_cast<int>(m_histTx2[i]));
        }
        if (sigNow > 0 && std::abs(sumJump) > sigNow / 3) {
            m_abnormalFlags |= 0x04;
        }

        // bit3: palm coupling noise — skipped (requires raw grid diff pointer)
        // bit4: tilt offset mismatch
        m_abnormalFlags &= ~0x10u;
        if (m_tiltCompScaleCandidate != 0 && m_histCount >= 10) {
            // Simplified: check if current signal deviates >1/6 from expected
            // based on tilt compensation model
            const uint16_t tiltIdx = GetTiltBinIndex(m_histTx2[0]);
            if (m_tiltHistCount[tiltIdx] > 10) {
                const uint16_t expected = static_cast<uint16_t>(
                    m_tiltHistSum[tiltIdx] / m_tiltHistCount[tiltIdx]);
                const int diff = std::abs(sigNow - static_cast<int>(expected));
                if (diff > sigNow / 6) {
                    m_abnormalFlags |= 0x10;
                }
            }
        }
    }

    // ════════════════════════════════════════════════
    // §4.3: Three-stage learning
    // ════════════════════════════════════════════════

    /// §4.3 line 1236-1288: NoPressInkLearningPrepareProcess
    /// Cold-start: accumulate signal statistics during real-pressure contact.
    inline void LearnPrepare(const NoPressInkInput& in) {
        const bool curRealPress = (in.realPressure > 0);

        // Count pen-down transitions
        if ((in.prevStaticBits & 0x02) == 0 && curRealPress) {
            m_prepareDownCount++;
        }

        if (curRealPress) {
            const uint16_t comp = CalcTiltCompensation(in.tx2Composite);
            const int usable = static_cast<int>(in.tx1Composite) - static_cast<int>(comp);
            if (usable > 0 && static_cast<uint16_t>(usable) > m_prepareMaxSignal) {
                m_prepareMaxSignal = static_cast<uint16_t>(usable);
            }
            m_prepareSampleCount++;
        }

        // Graduation check
        if (m_prepareDownCount > 1 &&
            m_prepareSampleCount > 100 &&
            m_abnormalFlags == 0) {
            m_noPressPara = 1;

            // Scale normalized seed table → initial long-term table
            if (m_tableMaxSignal > 0) {
                const int cells = std::min(gridCols * gridRows, kNpiMaxGridCells);
                for (int i = 0; i < cells; ++i) {
                    if (m_learnedLT[i] != 0) {
                        m_learnedLT[i] = static_cast<uint16_t>(
                            static_cast<uint32_t>(m_prepareMaxSignal) *
                            m_learnedLT[i] / m_tableMaxSignal);
                    }
                }
            }
            m_tableMaxSignal = m_prepareMaxSignal;
            m_prepareDownCount = 0;
            m_prepareMaxSignal = 0;
            m_prepareSampleCount = 0;
        }

        if (m_abnormalFlags != 0) {
            m_prepareDownCount = 0;
            m_prepareMaxSignal = 0;
            m_prepareSampleCount = 0;
        }
    }

    /// §4.3 line 1412-1439: NoPressInkLearningProcess
    /// Online learning during real-pressure contact.
    inline void LearnOnline(const NoPressInkInput& in) {
        const bool curRealPress = (in.realPressure > 0);

        if (!curRealPress) {
            // Pen-up transition: commit short-term → long-term
            if (m_prevRealPressFlag) {
                ValidateShortTerm();
                CommitToLongTerm();
                CommitTiltScale();
            }
        } else if (!m_prevRealPressFlag) {
            // Pen-down transition: start new short-term learning
            m_learningStableCounter = 0;
            ClearShortTermTable();
        } else {
            // Continuous contact: accumulate
            if (!IsLearningStable(in)) {
                UpdateShortTermMax(in);
            } else {
                CalcAndWriteThreshold(in);
            }
        }

        m_prevRealPressFlag = curRealPress;
    }

    /// §4.3 line 1290-1315: IsMeetNoPressInkLearningCondition
    inline bool IsLearningStable(const NoPressInkInput& in) {
        m_learningStableCounter++;

        if (!IsTiltMeetLearnedCondition(in)) {
            m_learningStableCounter = 0;
        }
        if (in.rawPressure > 4000) {
            m_learningStableCounter = 0;
        }
        if (m_abnormalFlags != 0) {
            m_learningStableCounter = 0;
        }

        if (m_learningStableCounter > 0x1D) {
            m_learningStableCounter = 0x1E;
            return true;
        }
        return false;
    }

    /// §4.3 line 1202-1217: Position must be away from edges for learning
    inline bool IsTiltMeetLearnedCondition(const NoPressInkInput& in) const {
        const int cols = std::clamp(gridCols, 1, kNpiMaxGridCols);
        const int rows = std::clamp(gridRows, 1, kNpiMaxGridRows);
        const int32_t margin = 0x200;
        const int32_t maxDim1 = cols * 0x400;
        const int32_t maxDim2 = rows * 0x400;

        if (in.coorDim1 < margin || in.coorDim1 > maxDim1 - margin) return false;
        if (in.coorDim2 < margin || in.coorDim2 > maxDim2 - margin) return false;
        return true;
    }

    /// §4.3 line 1317-1322: UpdateMaxSignalInTableShortTerm
    inline void UpdateShortTermMax(const NoPressInkInput& in) {
        const uint16_t comp = CalcTiltCompensation(in.tx2Composite);
        const int usable = static_cast<int>(in.tx1Composite) - static_cast<int>(comp);
        if (usable > 0 && static_cast<uint16_t>(usable) > m_shortTermMax) {
            m_shortTermMax = static_cast<uint16_t>(usable);
        }
    }

    /// §4.3 line 1324-1368: CalcNoPressInkThd
    inline void CalcAndWriteThreshold(const NoPressInkInput& in) {
        const uint16_t comp = CalcTiltCompensation(in.tx2Composite);
        if (in.tx1Composite < comp) {
            ResetTiltLearning();
            return;
        }

        const uint16_t usableSig = static_cast<uint16_t>(
            static_cast<int>(in.tx1Composite) - static_cast<int>(comp));

        const int cols = std::clamp(gridCols, 1, kNpiMaxGridCols);
        const int rows = std::clamp(gridRows, 1, kNpiMaxGridRows);
        int cx = static_cast<int>(in.coorDim1 >> 10);
        int cy = static_cast<int>(in.coorDim2 >> 10);
        cx = std::clamp(cx, 0, cols - 1);
        cy = std::clamp(cy, 0, rows - 1);
        const int cell = cx + cols * cy;
        if (cell >= kNpiMaxGridCells) return;

        // Two-level guard: must reach 80% of short-term max and 70% of table max
        if (m_shortTermMax > 0 && usableSig < m_shortTermMax * 4 / 5) {
            m_learningStableCounter = 0;
            return;
        }
        if (m_tableMaxSignal > 0 && usableSig < m_tableMaxSignal * 7 / 10) {
            m_learningStableCounter = 0;
            return;
        }

        // Write to short-term table if cell qualifies
        const bool cellOk =
            (m_learnedLT[cell] == 0 || (m_learnedLT[cell] * 3 / 5) < usableSig);
        const bool axisXOk =
            (m_axisX[cx] == 0 || (m_axisX[cx] * 3 / 5) < usableSig);
        const bool axisYOk =
            (m_axisY[cy] == 0 || (m_axisY[cy] * 3 / 5) < usableSig);

        if (cellOk && axisXOk && axisYOk) {
            if (m_shortTerm[cell] == 0 || m_shortTerm[cell] < usableSig) {
                m_shortTerm[cell] = usableSig;
                m_shortTerm_tx2[cell] = in.tx2Composite;
            }
            UpdateTiltStats(in.tx1Composite, in.tx2Composite);
        }
    }

    // ════════════════════════════════════════════════
    // §4.3: Table management
    // ════════════════════════════════════════════════

    /// §4.3 line 1370-1378: CheckShortTermTable
    inline void ValidateShortTerm() {
        if (m_tiltCompScaleCandidate != 0 &&
            std::abs(static_cast<int>(m_tiltCompScaleActive) -
                     static_cast<int>(m_tiltCompScaleCandidate)) > 5) {
            ClearShortTermTable();
        }
    }

    /// §4.3 line 1380-1403: UpdateLongTermTable
    inline void CommitToLongTerm() {
        const int cells = std::min(gridCols * gridRows, kNpiMaxGridCells);
        const int deadzone = std::max(0, tiltDeadzone);

        for (int i = 0; i < cells; ++i) {
            if (m_shortTerm[i] != 0) {
                m_learnedLT[i] = m_shortTerm[i];
                m_learnedLT_tx2[i] = m_shortTerm_tx2[i];
            }

            // Tilt re-calibration
            if (m_tiltCompScaleCandidate != 0 && m_learnedLT[i] != 0) {
                const int delta =
                    static_cast<int>(m_tiltCompScaleActive) -
                    static_cast<int>(m_tiltCompScaleCandidate);
                const int tx2Offset =
                    static_cast<int>(m_learnedLT_tx2[i]) - deadzone;
                if (tx2Offset > 0) {
                    const int adj = (delta * tx2Offset) / 100;
                    const int newVal = static_cast<int>(m_learnedLT[i]) + adj;
                    m_learnedLT[i] = static_cast<uint16_t>(std::clamp(newVal, 0, 0xFFFF));
                }
            }

            if (m_learnedLT[i] > m_tableMaxSignal) {
                m_tableMaxSignal = m_learnedLT[i];
            }
        }
    }

    /// §4.3 line 1405-1410: UpdateTiltCompScal
    inline void CommitTiltScale() {
        if (m_tiltCompScaleCandidate != 0) {
            m_tiltCompScaleActive = m_tiltCompScaleCandidate;
        }
    }

    inline void ClearShortTermTable() {
        std::memset(m_shortTerm, 0, sizeof(m_shortTerm));
        std::memset(m_shortTerm_tx2, 0, sizeof(m_shortTerm_tx2));
        m_shortTermMax = 0;
    }

    // ════════════════════════════════════════════════
    // §4.3: Tilt learning statistics
    // ════════════════════════════════════════════════

    /// §4.3 line 1676-1718: UpdateTiltLearningStatistics
    inline void UpdateTiltStats(uint16_t tx1Sig, uint16_t tx2Sig) {
        const uint16_t tiltIdx = GetTiltBinIndex(tx2Sig);
        if (tiltIdx >= kNpiTiltBins) return;

        if (tiltIdx < m_tiltIdxMin) m_tiltIdxMin = static_cast<uint8_t>(tiltIdx);
        if (tiltIdx > m_tiltIdxMax) m_tiltIdxMax = static_cast<uint8_t>(tiltIdx);

        m_tiltHistSum[tiltIdx] += tx1Sig;
        m_tiltHistCount[tiltIdx]++;

        // Cap: subtract one average sample to keep sliding estimate
        if (m_tiltHistCount[tiltIdx] >= m_tiltHistDepthLimit) {
            m_tiltHistSum[tiltIdx] -=
                m_tiltHistSum[tiltIdx] / m_tiltHistCount[tiltIdx];
            m_tiltHistCount[tiltIdx]--;
        }

        if (tx2Sig < m_tiltLearnMinTx2) m_tiltLearnMinTx2 = tx2Sig;
        if (tx2Sig > m_tiltLearnMaxTx2) m_tiltLearnMaxTx2 = tx2Sig;

        // Fit tilt compensation scale when range is wide enough
        const int range = static_cast<int>(m_tiltIdxMax) - static_cast<int>(m_tiltIdxMin);
        if (range > 10) {
            if (range > 20) {
                m_tiltLearnOK = true;
            }
            // Simple linear fit for compensation coefficient
            uint16_t coef = FitTiltCoef();
            if (coef == 0) {
                coef = static_cast<uint16_t>(std::max(1, tiltScaleInit));
            }
            m_tiltCompScaleCandidate = coef;
        }
    }

    /// Simple linear regression to estimate tilt compensation coefficient.
    /// Mirrors TSACore GetTx2CompCoef.
    inline uint16_t FitTiltCoef() const {
        if (m_tiltIdxMax <= m_tiltIdxMin) return 0;

        // Find average TX1 signal at min and max tilt bins
        uint32_t sumLow = 0, cntLow = 0;
        uint32_t sumHigh = 0, cntHigh = 0;
        const int mid = (static_cast<int>(m_tiltIdxMin) + static_cast<int>(m_tiltIdxMax)) / 2;

        for (int i = m_tiltIdxMin; i <= m_tiltIdxMax && i < kNpiTiltBins; ++i) {
            if (m_tiltHistCount[i] > 0) {
                const uint32_t avg = m_tiltHistSum[i] / m_tiltHistCount[i];
                if (i <= mid) { sumLow += avg; cntLow++; }
                else          { sumHigh += avg; cntHigh++; }
            }
        }

        if (cntLow == 0 || cntHigh == 0) return 0;
        const int avgLow  = static_cast<int>(sumLow / cntLow);
        const int avgHigh = static_cast<int>(sumHigh / cntHigh);
        const int sigDelta = avgHigh - avgLow;
        if (sigDelta <= 0) return 0;

        // coef ≈ sigDelta / tiltRange (scaled to percentage)
        const int tiltRange = static_cast<int>(m_tiltIdxMax - m_tiltIdxMin);
        if (tiltRange == 0) return 0;

        const int coef = (sigDelta * 100) / (tiltRange * std::max(1, tiltCap / kNpiTiltBins));
        return static_cast<uint16_t>(std::clamp(coef, 1, 200));
    }

    inline uint16_t GetTiltBinIndex(uint16_t tx2Signal) const {
        // Map TX2 signal to tilt bin: bin = tx2 * kNpiTiltBins / tiltCap
        const int cap = std::max(1, tiltCap);
        const int idx = static_cast<int>(tx2Signal) * kNpiTiltBins / cap;
        return static_cast<uint16_t>(std::clamp(idx, 0, kNpiTiltBins - 1));
    }

    inline void ResetTiltLearning() {
        std::memset(m_tiltHistSum, 0, sizeof(m_tiltHistSum));
        std::memset(m_tiltHistCount, 0, sizeof(m_tiltHistCount));
        m_tiltIdxMin = 0xFF;
        m_tiltIdxMax = 0;
        m_tiltCompScaleActive = static_cast<uint16_t>(std::max(1, tiltScaleInit));
        m_tiltCompScaleCandidate = 0;
        m_tiltLearnOK = false;
        m_tiltLearnMinTx2 = 0xFFFF;
        m_tiltLearnMaxTx2 = 0;
        m_tiltHistDepthLimit = static_cast<uint16_t>(std::max(1, tiltCap));

        m_noPressPara = 0;
        m_tableMaxSignal = 0;
        m_abnormalFlags &= ~0x1Fu;

        std::memset(m_learnedLT, 0, sizeof(m_learnedLT));
        std::memset(m_learnedLT_tx2, 0, sizeof(m_learnedLT_tx2));
    }

    // ════════════════════════════════════════════════
    // State
    // ════════════════════════════════════════════════

    // Core state
    bool     m_active = false;
    int      m_enterStreak = 0;
    int      m_exitStreak = 0;
    bool     m_tx2Started = false;
    bool     m_prevRealPressFlag = false;

    // Learning state
    uint8_t  m_noPressPara = 0;             // 0 = prepare phase, 1 = ready
    uint16_t m_tableMaxSignal = 0;          // max across entire long-term table
    uint16_t m_shortTermMax = 0;            // short-term session max
    uint32_t m_abnormalFlags = 0;           // 4-bit signal abnormality
    int      m_learningStableCounter = 0;

    // Prepare stage
    int      m_prepareDownCount = 0;
    uint16_t m_prepareMaxSignal = 0;
    int      m_prepareSampleCount = 0;

    // Signal history buffer
    uint16_t m_histTx1[kNpiHistoryBufSize]{};
    uint16_t m_histTx2[kNpiHistoryBufSize]{};
    int      m_histCount = 0;

    // Learned tables
    uint16_t m_learnedLT[kNpiMaxGridCells]{};       // long-term threshold
    uint16_t m_learnedLT_tx2[kNpiMaxGridCells]{};   // long-term TX2 values
    uint16_t m_shortTerm[kNpiMaxGridCells]{};        // short-term table
    uint16_t m_shortTerm_tx2[kNpiMaxGridCells]{};    // short-term TX2
    uint16_t m_axisX[kNpiMaxGridCols]{};             // per-column threshold
    uint16_t m_axisY[kNpiMaxGridRows]{};             // per-row threshold

    // Tilt learning
    uint32_t m_tiltHistSum[kNpiTiltBins]{};
    uint16_t m_tiltHistCount[kNpiTiltBins]{};
    uint8_t  m_tiltIdxMin = 0xFF;
    uint8_t  m_tiltIdxMax = 0;
    uint16_t m_tiltCompScaleActive = 29;
    uint16_t m_tiltCompScaleCandidate = 0;
    bool     m_tiltLearnOK = false;
    uint16_t m_tiltLearnMinTx2 = 0xFFFF;
    uint16_t m_tiltLearnMaxTx2 = 0;
    uint16_t m_tiltHistDepthLimit = 10000;
};

} // namespace Asa
