#pragma once
// ══════════════════════════════════════════════════════════════════════
// TouchPipeline Stage 8: PitchRemapAndDisplay —— TSAOut 输出层
//
// 对位 TSACore.dll（image base 0x6BA40000）W273AS2700（Gaokun Himax CSOT，
// 60 列 × 40 行，2560 × 1600）。权威规范：
//   docs/finger_algorithms/08_display_and_pitch_remap.md §1 – §11
//
// 原厂调用序（TSA_Processing @0x6BA9A443 尾段，实测反编译）：
//   TSA_MSProcess 返回
//     → TSAOut_PostProcess      @0x6BA6A77A   §1 §2  谁能进包 + 紧凑化
//     → TSA_SetDMDType          @0x6BA9D881   §6     设备状态字副作用
//     → DoubleReport_Process    @0x6BADED05   §7     本机恒关闭
//     → TSA_PitchSizeRemapProcess @0x6BB0611C §8     走线间距重映射
//     → TSAWrapper_UpdateReport @0x6BB2986B   §9     ← Q8 → 像素唯一发生点
//
// 贯穿全模块的一条不变量（§3 / §11-1）：**坐标全程在 Q8 网格域流转**，像素
// 只在 TSA_RptTouchXPos/YPos 取数的那一刻算一次。触点池里 outX_q8/outY_q8
// （原版 +0x170/+0x172）从头到尾是 Q8，所以 TSA_PitchSizeRemapProcess 虽然
// 排在 TSAOut 之后仍然完整生效，TSA_SetXPos 的反投影也才有意义。
// ══════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "SolverTypes.h"
#include "TouchCoreTypes.hpp"
#include "TouchFixed.hpp"
#include "TouchFrameTypes.h"
#include "TouchGates.hpp"

#include "config/ConfigBinder.h"
#include "config/ConfigStore.h"

namespace Solvers::Touch {

// ── 外部标准触控报文结构 g_report @0x6BBAA300（304 字节，§9.2）─────────
#pragma pack(push, 1)
struct TouchReportSlot {
    uint16_t xPos = 0;        // 显示像素 X (0..2559)
    uint16_t yPos = 0;        // 显示像素 Y (0..1599)
    int16_t  deltaX = 0;      // +0x04 与上一帧差分
    int16_t  deltaY = 0;      // +0x06 与上一帧差分
    uint8_t  touchId = 0;     // +0x08 **0 基** Tracking ID（TSA_RptTouchID = +0x154 − 1）
    uint8_t  reserved = 0;    // +0x09 对齐保留
};

struct TSAOutputReport {
    uint32_t touchIdMask = 0;                  // +0x000 活跃 ID 位掩码
    std::array<TouchReportSlot, 30> slots{};   // +0x004 槽数 30，只用 0..19
};
#pragma pack(pop)
static_assert(sizeof(TSAOutputReport) == 304, "TSAOutputReport must be exactly 304 bytes");

// ── W273AS2700 80 阶走线间距标定表（flash +0xA0 / +0x320，§8 实测）─────
inline constexpr std::array<double, 80> kW273Dim1PitchMap = {
    /*  0 */ 0.0,
    /*  1 */ 0.984375,
    /*  2 */ 1.96875,
    /*  3 */ 2.953125,
    /*  4 */ 3.9375,
    /*  5 */ 4.921875,
    /*  6 */ 5.90625,
    /*  7 */ 6.890625,
    /*  8 */ 7.875,
    /*  9 */ 8.859375,
    /* 10 */ 9.84375,
    /* 11 */ 10.8515625,
    /* 12 */ 11.859375,
    /* 13 */ 12.8671875,
    /* 14 */ 13.875,
    /* 15 */ 14.8828125,
    /* 16 */ 15.890625,
    /* 17 */ 16.8984375,
    /* 18 */ 17.90625,
    /* 19 */ 18.9140625,
    /* 20 */ 19.921875,
    /* 21 */ 20.9296875,
    /* 22 */ 21.9375,
    /* 23 */ 22.9453125,
    /* 24 */ 23.953125,
    /* 25 */ 24.9609375,
    /* 26 */ 25.96875,
    /* 27 */ 26.9765625,
    /* 28 */ 27.984375,
    /* 29 */ 28.9921875,
    /* 30 */ 30.0,
    /* 31 */ 31.0078125,
    /* 32 */ 32.015625,
    /* 33 */ 33.0234375,
    /* 34 */ 34.03125,
    /* 35 */ 35.0390625,
    /* 36 */ 36.046875,
    /* 37 */ 37.0546875,
    /* 38 */ 38.0625,
    /* 39 */ 39.0703125,
    /* 40 */ 40.078125,
    /* 41 */ 41.0859375,
    /* 42 */ 42.09375,
    /* 43 */ 43.1015625,
    /* 44 */ 44.109375,
    /* 45 */ 45.1171875,
    /* 46 */ 46.125,
    /* 47 */ 47.1328125,
    /* 48 */ 48.140625,
    /* 49 */ 49.1484375,
    /* 50 */ 50.15625,
    /* 51 */ 51.140625,
    /* 52 */ 52.125,
    /* 53 */ 53.109375,
    /* 54 */ 54.09375,
    /* 55 */ 55.078125,
    /* 56 */ 56.0625,
    /* 57 */ 57.046875,
    /* 58 */ 58.03125,
    /* 59 */ 59.015625,
    /* 60 */ 60.0,
    /* 61 */ 60.0,
    /* 62 */ 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    /* 72 */ 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0
};

// Dim2 表首项 100.0 是哨兵：SensorPitchSizeMap 在 UCOMISD 之后立刻 RET，
// Y 轴完全旁路（§8 实测，flash +0x320 = 0x4059000000000000）。
inline constexpr std::array<double, 80> kW273Dim2PitchMap = []() constexpr {
    std::array<double, 80> arr{};
    arr[0] = 100.0;
    return arr;
}();

// ══════════════════════════════════════════════════════════════════════
// Stage 8 主模块
// ══════════════════════════════════════════════════════════════════════
class PitchRemapAndDisplay {
public:
    static constexpr int kCols  = kSensorCols;     // 60（flash +0x1D）
    static constexpr int kRows  = kSensorRows;     // 40（flash +0x1E）
    static constexpr int kDispW = kDisplayWidth;   // 2560（flash +0x5A6）
    static constexpr int kDispH = kDisplayHeight;  // 1600（flash +0x5A8）
    static constexpr int kScale = kQ8Scale;        // 256

    // ── §3.1 比例常数（TSAConstPrmt_Init @0x6BB3BB00）──────────────────
    // K1 = 2560·65536 / (60<<8) = 10922（整数除法截断，精确值 10922.667）
    // K2 = 1600·65536 / (40<<8) = 10240（整除）
    // 复刻必须用同样的整数截断，不能用浮点比例（§11-2）。
    static constexpr int32_t kAffineScale1 = kAffineScaleDim1; // DAT_6bcab604
    static constexpr int32_t kAffineScale2 = kAffineScaleDim2; // DAT_6bcab608
    static constexpr int32_t kDim1Full     = kCols * 256;      // DAT_6bcab620 = 15360
    static constexpr int32_t kDim2Full     = kRows * 256;      // DAT_6bcab622 = 10240
    // DAT_6bcab61a = 4096 / flash[0x1B] + 1 = 4096/69 + 1 = 60 （Q8 per mm）
    static constexpr int32_t kQ8PerMm      = 4096 / 69 + 1;
    static_assert(kQ8PerMm == 60, "px/mm 必须按整数除法截断得 60（§11-2）");
    static_assert(kAffineScale1 == (kDispW * 65536) / kDim1Full, "K1 整数截断");
    static_assert(kAffineScale2 == (kDispH * 65536) / kDim2Full, "K2 整数截断");

    // ── 门控（TouchGates 单一事实源，§10）─────────────────────────────
    static constexpr bool kFeatSmartCover   = Gates::IsOn("FEAT_SMART_COVER");   // false
    static constexpr bool kFeatDoubleReport = Gates::IsOn("FEAT_DOUBLE_REPORT"); // false
    static constexpr bool kFoldSupported    = Gates::IsOn("TSA_FOLD");           // false
    static constexpr bool kUdfpEnabled      = Gates::IsOn("UDFP");               // false

    // ── 模块使能 ──────────────────────────────────────────────────────
    bool m_enabled           = true;
    bool m_pitchRemapEnabled = true;   // Dim1 表激活（pDim1PitchMap[0] != 100.0）

    // ── 显示分辨率与 margin（flash +0x5A6/+0x5A8/+0x5AE..+0x5B3）───────
    int32_t m_displayWidth  = kDispW;
    int32_t m_displayHeight = kDispH;
    int32_t m_marginDim1Lo  = 0;   // flash +0x5AE（int8）
    int32_t m_marginDim1Hi  = 0;   // flash +0x5AF（int8）
    int32_t m_marginDim2Lo  = 0;   // flash +0x5B0（int8）
    int32_t m_marginDim2Hi  = 0;   // flash +0x5B1（int8）
    int32_t m_edgeDim2Lo    = 0;   // flash +0x5B2（int8）
    int32_t m_edgeDim2Hi    = 0;   // flash +0x5B3（int8）
    // flash +0x5A0 = 1：对外 API 的上报倍率。内部裁剪判定固定用 1。
    int32_t m_reportScale   = 1;

    // ── 轴向变换与镜像控制位（§4）─────────────────────────────────────
    uint8_t m_flashCmfForceDim1Pass = 1;  // flash +0x18 = 1
    uint8_t m_flashAxisBind         = 1;  // flash +0x19 = 1
    uint8_t m_flashSwapSel          = 0;  // flash +0x5A4 = 0
    bool    m_flipDim1              = false; // flash +0x5A2 = 0
    bool    m_flipDim2              = false; // flash +0x5A3 = 0

    // ── X 核心区（TSAStatic_Init @0x6BAA1A42；仅 TSA_SetXCoreAreaCommand 能改）
    // 本机 wField_0x2e0 = 0、nXResMax = TSA_GetXRes() − 1 = 2559，而
    // TSAOut_DisplayXPos 返回值内部已钳在同区间 ⇒ 两条裁剪分支恒不触发（§2/§10）。
    int32_t m_xCoreAreaMin = 0;
    int32_t m_xResMax      = kDispW - 1;

    // ── 场景/通道开关 ─────────────────────────────────────────────────
    bool m_isGameScenario   = false;  // TSA_IsGameScenario()
    // SideTouch_Report @0x6BB1A7A5：侧边通道本机关闭（无侧边触点源）。
    // 打开后它会在 TSAOut **之后**反向追加触点并同时增加 *g_touchesPtr 与 [5]。
    bool m_sideTouchEnabled = false;

    // ── 80 阶走线插值表 ───────────────────────────────────────────────
    std::array<double, 80> m_dim1PitchMap = kW273Dim1PitchMap;
    std::array<double, 80> m_dim2PitchMap = kW273Dim2PitchMap;

    // ── 倍频（§7，本机 FEAT_DOUBLE_REPORT = 0 恒关闭）─────────────────
    bool     m_doubleReportEnabled = kFeatDoubleReport;
    uint32_t m_doubleReportDelayUs = 8333; // min(周期+1000μs, 实测间隔) / 2

    struct DoubleReportSlotHistory {
        int32_t prev1X = 0, prev1Y = 0;   // t−1
        int32_t prev2X = 0, prev2Y = 0;   // t−2
        bool    hasPrev1 = false;
        bool    hasPrev2 = false;
    };

    PitchRemapAndDisplay() noexcept { Reset(); }

    void Reset() noexcept {
        m_drHistory.fill(DoubleReportSlotHistory{});
        m_backupTracks.clear();
        m_backupContacts.clear();
        m_backupValid = false;
        m_report = TSAOutputReport{};
        m_prevReport = TSAOutputReport{};
        m_reportedNum = 0;
        m_contactCount = 0;
        m_dmdType = 0;
    }

    void ClearLiveState() noexcept { Reset(); }

    void SetEnabled(bool enabled) noexcept { m_enabled = enabled; }
    [[nodiscard]] bool IsEnabled() const noexcept { return m_enabled; }

    [[nodiscard]] const TSAOutputReport& GetOutputReport() const noexcept { return m_report; }
    [[nodiscard]] uint32_t GetDoubleReportDelayUs() const noexcept { return m_doubleReportDelayUs; }
    [[nodiscard]] bool HasPendingDoubleReport() const noexcept { return m_backupValid; }
    /// g_touchesPtr[5]：本帧真正进包的触点数。
    [[nodiscard]] uint8_t GetReportedNum() const noexcept { return m_reportedNum; }
    /// g_tsaStaticPtr->bField_0x00：+0x1F0 & 4 的触点数。
    [[nodiscard]] uint8_t GetContactCount() const noexcept { return m_contactCount; }
    /// g_tsaStaticPtr->nField_0x138：TSA_SetDMDType 写出的设备状态字（§6）。
    [[nodiscard]] int32_t GetDmdType() const noexcept { return m_dmdType; }
    /// B4：HardwareAnalyzer_GetDetectedException @0x6BAF0249 的返回值（0..0x1F）。
    /// TouchPipeline 在本级 Process 之前从 Stage 0.5 的分析器取来 —— 对位原版
    /// TSA_Processing 里 TSA_SetDMDType 自己调那一次，时序等价（本机异常码的
    /// 唯一产出方 HardwareAnalyzer_ProcessByRaw2Raw 在 Stage 0.5 就跑完了）。
    void SetDetectedException(int32_t exc) noexcept { m_detectedException = exc; }

    // ══════════════════════════════════════════════════════════════════
    // §3 数学核（全部照抄原厂舍入方向，§11-3）
    // ══════════════════════════════════════════════════════════════════

    // ── SensorPitchSizeMap @0x6BB05FC0（§8）────────────────────────────
    // C_out = trunc( (S − r)·P[k] + r·P[k+1] )，CVTTSD2SI **向零截断**。
    // 边界：k ≤ 78 才插值（访问 P[79] 正好是末项，不越界）；
    //       **k ≥ 79 返回 0**（打到原点，不是钳到端点）—— 原厂行为，照抄。
    static inline int32_t SensorPitchSizeMap(int32_t coordIn, const double* pTable,
                                             uint16_t scale) noexcept {
        if (pTable == nullptr) return coordIn;
        if (pTable[0] == 100.0) [[likely]] {
            return coordIn;  // 哨兵：UCOMISD 严格相等，命中即原值直通
        }
        if (scale == 0) [[unlikely]] {
            return 0;
        }
        const int32_t s = static_cast<int32_t>(scale);
        const uint16_t k = static_cast<uint16_t>(coordIn / s);
        if (k >= 0x4F) {
            return 0;
        }
        const int32_t r = (coordIn % s) & 0xFFFF;
        const double out = pTable[static_cast<size_t>(k) + 1] * static_cast<double>(r) +
                           pTable[k] * static_cast<double>(s - r);
        return static_cast<int32_t>(out);  // 向零截断
    }

    // ── Bezier @0x6BA6595D（§7.2）──────────────────────────────────────
    // P(1/2) = (3·P_t + 6·P_{t−1} − P_{t−2}) / 8；负值 +7 后算术右移 3。
    static inline int32_t Bezier(int32_t pPrev2, int32_t pPrev1, int32_t pCur) noexcept {
        int32_t v = Fixed::WrapSub32(Fixed::WrapAdd32(pCur * 3, pPrev1 * 6), pPrev2);
        return Fixed::ShrTowardZero(v, 3);
    }

    // ── TSAOut_EdgeScale @0x6BA69C9E（§3.3）───────────────────────────
    // a + (((256 − a)·v) >> 8)，负中间值先 +0xFF（向零取整）。
    static inline int32_t EdgeScale(int32_t a, int32_t v) noexcept {
        return a + Fixed::ShrTowardZero((0x100 - a) * v, 8);
    }

    // ── TSAOut_Display2Pixel @0x6BA69C56 ──────────────────────────────
    static inline int32_t Display2Pixel(int32_t px) noexcept {
        return (px * 0x10000 + kAffineScale2 / 2) / kAffineScale2;
    }

    // ── TSAFold_Dim1/Dim2PosOutRemap @0x6BB28420 / 0x6BB284E2 ─────────
    // 门控 TSAFold_IsSupported() = (flash +0x17CC > 1)，实测 = 1 → 关闭，原值透传。
    static inline int32_t FoldDim1PosOutRemap(int32_t v, int32_t /*scale*/) noexcept {
        if constexpr (kFoldSupported) { /* 折叠屏机型才有重映射，本机不可达 */ }
        return v;
    }
    static inline int32_t FoldDim2PosOutRemap(int32_t v, int32_t /*scale*/) noexcept {
        if constexpr (kFoldSupported) { }
        return v;
    }

    // ── TSAOut_Dim1PosInternal @0x6BA69A50（§3.2）─────────────────────
    // v = ((q8 · K1 · scale + 0x8000) >> 16) − scale·M_lo   ← **无符号**右移
    // 钳到 [0, 2560·scale − 1]。q8 与 K1 都非负，不会出现负中间值。
    [[nodiscard]] int32_t Dim1PosInternal(uint16_t q8, int32_t scale) const noexcept {
        const uint32_t prod = static_cast<uint32_t>(q8) *
                              static_cast<uint32_t>(kAffineScale1) *
                              static_cast<uint32_t>(scale);
        int32_t v = static_cast<int32_t>((prod + 0x8000u) >> 16) - scale * m_marginDim1Lo;
        if (v < 0) v = 0;
        const int32_t hi = scale * m_displayWidth - 1;
        if (hi < v) v = hi;
        return FoldDim1PosOutRemap(v & 0xFFFF, scale);
    }

    // ── TSAOut_Dim2PosInternal @0x6BA69CCB（§3.3）─────────────────────
    // 与 Dim1 的两处**实现不对称**（原厂行为，照抄）：
    //   1. 首/末一格内的边缘外推，只有 Dim2 有（触发条件为对应 margin < 0，
    //      本机 flash +0x5B0 = +0x5B1 = 0 ⇒ 两段都关闭）；
    //   2. 舍入：负中间值先 +0x17FFF 再**算术**右移，而不是 +0x8000 无符号右移；
    //   3. 上界判据是 `scale·1600 <= v`（Dim1 是 `scale·2560 − 1 < v`）。
    [[nodiscard]] int32_t Dim2PosInternal(uint16_t q8, int32_t scale) const noexcept {
        int32_t p = static_cast<int32_t>(q8);
        if (q8 < 0x100) {
            if (m_marginDim2Lo < 0) {
                p = EdgeScale(Display2Pixel(m_marginDim2Lo - m_edgeDim2Lo), p);
            }
        } else if (p > (kRows - 1) * 0x100 && m_marginDim2Hi < 0) {
            const int32_t e = EdgeScale(Display2Pixel(m_marginDim2Hi - m_edgeDim2Hi),
                                        kRows * 0x100 - p);
            p = kRows * 0x100 - e;
        }
        const int32_t t = Fixed::WrapMul32(Fixed::WrapMul32(scale, kAffineScale2), p);
        int32_t r = Fixed::WrapAdd32(t, 0x8000);
        if (r < 0) r = Fixed::WrapAdd32(t, 0x17FFF);
        int32_t v = (r >> 16) - scale * m_marginDim2Lo;   // 算术右移
        if (v < 0) v = 0;
        if (scale * m_displayHeight <= v) v = scale * m_displayHeight - 1;
        return FoldDim2PosOutRemap(v & 0xFFFF, scale);
    }

    // ── TSAOut_RevertDim1Pos @0x6BA69B2F / RevertDim2Pos @0x6BA69EB9（§3.4）
    // q8 = ((px + M_lo)·65536 + K/2) / K，上限 DAT_6bcab620/622 − 1。
    [[nodiscard]] uint16_t RevertDim1Pos(int32_t pixelVal) const noexcept {
        int32_t c = m_marginDim1Lo + pixelVal;
        if (c < 0) c = 0;
        uint32_t q8 = (static_cast<uint32_t>(c) * 0x10000u + (kAffineScale1 >> 1)) / kAffineScale1;
        if (q8 >= static_cast<uint32_t>(kDim1Full)) q8 = static_cast<uint32_t>(kDim1Full) - 1;
        return static_cast<uint16_t>(q8);
    }

    [[nodiscard]] uint16_t RevertDim2Pos(int32_t pixelVal) const noexcept {
        int32_t c = m_marginDim2Lo + pixelVal;
        if (c < 0) c = 0;
        uint32_t q8 = (static_cast<uint32_t>(c) * 0x10000u + (kAffineScale2 >> 1)) / kAffineScale2;
        if (q8 >= static_cast<uint32_t>(kDim2Full)) q8 = static_cast<uint32_t>(kDim2Full) - 1;
        return static_cast<uint16_t>(q8);
    }

    // ── TSAOut_ExtToIntDist @0x6BA69BC3（§3.4）────────────────────────
    // (px << 16) / K1，**不加半个 LSB**，向下取整。
    static inline uint32_t ExtToIntDist(uint16_t pixelDist) noexcept {
        return static_cast<uint32_t>((static_cast<uint64_t>(pixelDist) << 16) /
                                     static_cast<uint64_t>(kAffineScale1));
    }

    // ── TSAOut_IntToExtDist @0x6BA69BFB（§3.4）────────────────────────
    // 取绝对值 → (|d|·K1 + 0x8000) >> 16 → 恢复符号。
    static inline int32_t IntToExtDist(int16_t d) noexcept {
        const int16_t a = (d < 0) ? static_cast<int16_t>(-d) : d;
        uint32_t v = (static_cast<uint32_t>(static_cast<int32_t>(a) * kAffineScale1) + 0x8000u) >> 16;
        return (d < 0) ? -static_cast<int32_t>(v) : static_cast<int32_t>(v);
    }

    // ── TS_MMToPixel @0x6BA7C416：返回的其实是 Q8，函数名有误导（§3.4）──
    static inline int32_t MMToPixel(uint8_t mm) noexcept { return kQ8PerMm * mm; }

    // ── TSAOut_Axis @0x6BA6A2E5 = IntToExtDist 的薄包装 ───────────────
    static inline int32_t AxisToPixel(uint8_t mm) noexcept {
        return IntToExtDist(Fixed::Wrap16(MMToPixel(mm)));
    }

    // ── TSAOut_Angle @0x6BA6A305（§4）─────────────────────────────────
    // 判据是 flash[0x18] == flash[0x19]（**相等**，不是异或），本机成立。
    [[nodiscard]] int8_t RemapAngle(int8_t rawAngle) const noexcept {
        if (m_flashCmfForceDim1Pass == m_flashAxisBind) {
            return static_cast<int8_t>(rawAngle < 1 ? rawAngle + 90 : rawAngle - 90);
        }
        return static_cast<int8_t>(-rawAngle);
    }

    // ── §4 轴向转置判据 ───────────────────────────────────────────────
    // 原厂：`(flash[0x18] ^ flash[0x19]) == flash[0x5A4]` 成立 ⇒ **不转置**。
    // 本机 1 ^ 1 = 0 == 0 ⇒ Dim1 → X、Dim2 → Y。
    [[nodiscard]] bool IsTransposed() const noexcept {
        return (m_flashCmfForceDim1Pass ^ m_flashAxisBind) != m_flashSwapSel;
    }

    // ── TSAOut_TouchXPosInternal @0x6BA69F4D（§4）─────────────────────
    [[nodiscard]] int32_t TouchXPosInternal(const TrackingSlot& t, int32_t scale) const noexcept {
        if (!IsTransposed()) {
            const int32_t v = Dim1PosInternal(t.outX_q8, scale);
            return m_flipDim1 ? (m_displayWidth * scale - v - 1) : v;
        }
        const int32_t v = Dim2PosInternal(t.outY_q8, scale);
        return m_flipDim2 ? (m_displayHeight * scale - v - 1) : v;
    }

    // ── TSAOut_TouchYPosInternal @0x6BA6A0C7（§4）─────────────────────
    [[nodiscard]] int32_t TouchYPosInternal(const TrackingSlot& t, int32_t scale) const noexcept {
        if (!IsTransposed()) {
            const int32_t v = Dim2PosInternal(t.outY_q8, scale);
            return m_flipDim2 ? (m_displayHeight * scale - v - 1) : v;
        }
        const int32_t v = Dim1PosInternal(t.outX_q8, scale);
        return m_flipDim1 ? (m_displayWidth * scale - v - 1) : v;
    }

    // TSAOut_DisplayXPos @0x6BA6A241 = TouchXPosInternal(i, 1)，返回 u16。
    [[nodiscard]] uint16_t DisplayXPos(const TrackingSlot& t) const noexcept {
        return Fixed::WrapU16(TouchXPosInternal(t, 1));
    }
    [[nodiscard]] uint16_t DisplayYPos(const TrackingSlot& t) const noexcept {
        return Fixed::WrapU16(TouchYPosInternal(t, 1));
    }

    // ── TSA_SetXPos @0x6BA6A355（§2-1）────────────────────────────────
    // 反向投影：按 `flash[0x18] == flash[0x19]` 选维，把像素换回 Q8 写 +0x170/+0x172。
    void SetXPos(TrackingSlot& t, int32_t pixelVal) const noexcept {
        if (m_flashCmfForceDim1Pass == m_flashAxisBind) {
            t.outX_q8 = RevertDim1Pos(pixelVal);
        } else {
            t.outY_q8 = RevertDim2Pos(pixelVal);
        }
    }

    // ══════════════════════════════════════════════════════════════════
    // 管线主流程 —— 严格照 TSA_Processing @0x6BA9A443 尾段的调用序
    // ══════════════════════════════════════════════════════════════════
    bool Process(TouchBigFrame& bigFrame) {
        if (!m_enabled) return true;

        // 07 的 Filter_ProcessUnit 尾部无条件 `+0x170/+0x172 ← +0x16C/+0x16E`
        // （文档 07 §「0x170/0x172 无条件从 0x16C/0x16E 复制」）。复刻里 07 的
        // JIT 结果落在 fltX_q8/fltY_q8，所以在本级入口播种输出槽。
        // 注意：原厂调用序是 ER_Process → TouchReport_Process，这条无条件复制
        // 会覆盖 ER_MoveOutCorrection @0x6BA57E7D 写进 +0x170 的钳位值 —— 这是
        // 原厂行为，不做"修正"。
        SeedOutputSlots(bigFrame);

        // §1 §2：TSAOut_PostProcess @0x6BA6A77A
        TSAOut_PostProcess(bigFrame);

        // §6：TSA_SetDMDType @0x6BA9D881（写设备状态字 + 推进 BigData 打点）
        m_dmdType = SetDMDType();

        // §7：DoubleReport_Process @0x6BADED05。本机 FEAT_DOUBLE_REPORT = 0，
        // Scenario 首条判据恒假 ⇒ 每帧只做 TryToEnable + CaculateInterval 两件
        // 无副作用的事，Calculate / Trigger 整链不可达。
        if (m_doubleReportEnabled) {
            DoubleReport_Calculate(bigFrame);
        }

        // §8：TSA_PitchSizeRemapProcess @0x6BB0611C（仍在 Q8 域就地重映射）
        if (m_pitchRemapEnabled) {
            ApplyPitchRemap(bigFrame);
        }

        // §9：TSAWrapper_UpdateReport @0x6BB2986B —— Q8 → 像素唯一发生点
        UpdateOutputReport(bigFrame);

        // 把像素坐标与 Tip 结论同步回 contacts（复刻侧的对外交付面）
        SynchronizeContacts(bigFrame);

        return true;
    }

    bool Process(HeatmapFrame& frame) {
        if (!m_enabled) return true;

        if (frame.touch.bigFrame != nullptr) {
            const bool ok = Process(*frame.touch.bigFrame);
            frame.touch.output.contacts.clear();
            for (const auto& c : frame.touch.bigFrame->contacts) {
                frame.touch.output.contacts.push_back(c);
            }
            return ok;
        }

        // 无 BigFrame 时的轻量模式（适配独立单元测试与旧代码）
        for (auto& contact : frame.touch.output.contacts) {
            if (contact.id <= 0 || contact.id > 20) continue;
            if (!contact.isReported) continue;

            TrackingSlot tmp{};
            tmp.outX_q8 = Fixed::WrapU16(static_cast<int32_t>(contact.x * 256.0f));
            tmp.outY_q8 = Fixed::WrapU16(static_cast<int32_t>(contact.y * 256.0f));
            if (m_pitchRemapEnabled) {
                tmp.outX_q8 = Fixed::WrapU16(
                    SensorPitchSizeMap(tmp.outX_q8, m_dim1PitchMap.data(), 256));
                tmp.outY_q8 = Fixed::WrapU16(
                    SensorPitchSizeMap(tmp.outY_q8, m_dim2PitchMap.data(), 256));
            }
            contact.x = static_cast<float>(TouchXPosInternal(tmp, m_reportScale));
            contact.y = static_cast<float>(TouchYPosInternal(tmp, m_reportScale));
        }
        return true;
    }

    // ── §7.3 倍频第二阶段：DoubleReport_Trigger @0x6BADEE02 ───────────
    // 宿主延迟 DoubleReport_DelayUs() 后回调。IsEnabled 为 0 直接放弃。
    // 原厂时序漏洞（§7.3）：备份发生在 PitchSizeRemap **之前**，恢复出来的是
    // 未重映射坐标，所以这里补跑一次重映射 —— 仅在倍频真的开启时才有意义。
    bool TriggerDoubleReport(TouchBigFrame& bigFrame) {
        if (!m_enabled || !m_doubleReportEnabled || !m_backupValid) {
            return false;
        }
        bigFrame.tracks = m_backupTracks;
        bigFrame.contacts = m_backupContacts;

        // 全池 TE == 2 → 4，防止主机在同一次按下里收到两次 PointerDown。
        for (auto& track : bigFrame.tracks) {
            if (track.state == EventDown) track.state = EventMove;
        }
        for (auto& contact : bigFrame.contacts) {
            if (contact.state == TouchStateDown) contact.state = TouchStateMove;
            if (contact.reportEvent == TouchReportDown) contact.reportEvent = TouchReportMove;
        }

        if (m_pitchRemapEnabled) ApplyPitchRemap(bigFrame);
        UpdateOutputReport(bigFrame);
        SynchronizeContacts(bigFrame);

        m_backupValid = false;
        return true;
    }

    // ══════════════════════════════════════════════════════════════════
    // §1 TSAOut_PostProcess @0x6BA6A77A —— 谁能进包
    // ══════════════════════════════════════════════════════════════════
    void TSAOut_PostProcess(TouchBigFrame& bigFrame) noexcept {
        uint8_t contactCnt = 0;      // local_a
        m_reportedNum = 0;           // g_touchesPtr[5] / [6]

        // **整帧封杀**：header[3] 是 07 中「区域类型 5（大物体）被拒报」的计数。
        // 只要本帧有任一触点因大物体被拒，而 FEAT_SMART_COVER 关闭且不在游戏
        // 场景，就 `*g_touchesPtr = 0` —— 整帧全丢，且跳过 BefUDFP 预筛。
        // 本机 dwFeatureFlags = 0x01090121，& 0x2 == 0 ⇒ 这条路**武装**。
        bool killFrame = false;
        if (bigFrame.cancelledTouchCount != 0 && !kFeatSmartCover) {
            if (!m_isGameScenario) {
                bigFrame.tracks.clear();
                killFrame = true;
            }
        }

        if (!killFrame) {
            // 预筛：TSAOut_PostProcessBefUDFPUnit @0x6BA6A6F3。UDFP 本机关闭，
            // 这轮在功能上冗余，但它**真的会删触点**，不抄会让计数中间值对不上。
            for (int i = static_cast<int>(bigFrame.tracks.size()) - 1; i >= 0; --i) {
                PostProcessBefUDFPUnit(bigFrame, i);
            }
        }

        // UDFPEvent_Process @0x6BB3392C：整体被 `if (DAT_6bcaa140 != 0)` 包住，
        // flash +0x1740 = 0 ⇒ 空操作。
        if constexpr (kUdfpEnabled) { /* 本机不可达 */ }

        // 倒序遍历是**必须的**：剔除走 Touch_UnitRemove，它把后面的单元整体前移；
        // 倒序时尚未访问的下标都在当前下标之前，不会被移动，既不漏也不重。
        for (int i = static_cast<int>(bigFrame.tracks.size()) - 1; i >= 0; --i) {
            PostProcessUnit(bigFrame, i, contactCnt);
        }

        m_contactCount = contactCnt;   // → g_tsaStaticPtr->bField_0x00

        // SideTouch_Report @0x6BB1A7A5：在 TSAOut **之后**反向追加侧边触点，
        // 每追加一个都再查一次 `g_touchesPtr[5] < 20`，并同时 [5]++ 与 *ptr++。
        // 本机无侧边通道数据源 ⇒ 关闭。`(DAT_6bcaa0a4 & 3)` 回填段
        //（flash +0x16A4 = 0）在原厂也已关闭。
        if (m_sideTouchEnabled) {
            SideTouch_Report(bigFrame);
        }
    }

    // ── TSAOut_PostProcessBefUDFPUnit @0x6BA6A6F3 —— 只查两条 ──────────
    void PostProcessBefUDFPUnit(TouchBigFrame& bigFrame, int idx) const noexcept {
        const TrackingSlot& t = bigFrame.tracks[static_cast<size_t>(idx)];
        if ((t.lifeFlags & LifeReportAdmit) == 0) {            // +0x1F8 & 8 == 0
            Touch_UnitRemove(bigFrame, idx);
        } else if (IsGripHardSuppressed(t)) {                  // +0x1F0 & 0x8000000
            Touch_UnitRemove(bigFrame, idx);
        }
    }

    // ══════════════════════════════════════════════════════════════════
    // §2 TSAOut_PostProcessUnit @0x6BA6A3EF —— 逐条判据与紧凑化
    // ══════════════════════════════════════════════════════════════════
    void PostProcessUnit(TouchBigFrame& bigFrame, int idx, uint8_t& contactCnt) noexcept {
        TrackingSlot& t = bigFrame.tracks[static_cast<size_t>(idx)];

        // TE 状态的唯一解释点（TouchCoreTypes::MapToPtp）。原厂 TSAOut 不直接读
        // +0x21C —— TE 的全部影响已由 07 折叠进 +0x1F8 bit3（§5 的对照表）。
        // 复刻把这条对照表钉死在这里：0x01 不发包、0x20 物理删除、0x40 是唯一
        // Tip = 0 帧、0x08/0x10 以 Tip = 1 正常进包。
        const PtpDisposition ptp = MapToPtp(t.state);

        const bool admitBit3   = (t.lifeFlags & LifeReportAdmit) != 0;  // 判据 1
        const bool gripKill    = IsGripHardSuppressed(t);               // 判据 2
        // 判据 3：触点单元 +0x1F4 & 0x4000（反编译写作 `+ 500`）。全库没有任何
        // 写入者置过 0x4000 ⇒ 本机恒不成立，保留结构不实现来源。
        const bool abnormal    = (t.volatileFlags2 & 0x4000u) != 0;
        // 判据 4：+0x1F8 & 0x10000 姿态取消（TouchReport_PostureAntiTouch），
        // 07 已证不可达。
        const bool postureKill = (t.persistFlags1F8 & 0x10000u) != 0;

        // 四条否决是**或**关系而非先后关系；再叠上 TE 的进包结论。
        if (!admitBit3 || gripKill || abnormal || postureKill ||
            ptp.removeNow || !ptp.inPacket) {
            Touch_UnitRemove(bigFrame, idx);
            return;
        }

        // 1. X 核心区裁剪（**网格域**，只裁 X，Y 方向没有对应代码）。
        //    TSAOut_DisplayXPos 返回 u16 且内部已钳在 [0, 2559]，
        //    而 wField_0x2e0 = 0、nXResMax = 2559 ⇒ 本机两条分支恒不触发。
        if (DisplayXPos(t) < m_xCoreAreaMin) {
            SetXPos(t, m_xCoreAreaMin);
        } else if (m_xResMax < DisplayXPos(t)) {
            SetXPos(t, m_xResMax);
        }

        // 2. g_touchesPtr[5]++ / [6]++
        ++m_reportedNum;

        // 3. (+0x1F0 & 4) 成立则累加 → static->bField_0x00
        if ((t.volatileFlags & 0x4u) != 0) {
            ++contactCnt;
        }

        // 4. 与历史池握手（§2-4）：PrevTouch_GetIdxBasedOnID(+0x154) 找到历史槽，
        //    若槽号 < 20 且历史 +0x1F8 & 0x20000000 == 0，则写 历史+0x598 =
        //    pFrameBufferPtr 并置 +0x1F8 |= 0x20000000。这是「该轨迹已首次上报」
        //    的一次性锁存，05 的 TSABuffer_TimeRptProcess 用它区分在场轨迹。
        //    复刻的历史池即轨迹自身的 persistFlags1F8。
        if ((t.persistFlags1F8 & 0x20000000u) == 0) {
            t.persistFlags1F8 |= 0x20000000u;
        }

        // 5. Tip 位（§5）：g_report 里没有事件字段，宿主靠 TSA_RptTouchEvent
        //    判 `!= 0x40` 合成。复刻把结论落在轨迹上。
        t.tipDown = ptp.tip;
    }

    // ── Touch_UnitRemove @0x6BA763F4 —— 整段 0x168 前移紧凑化 ──────────
    // ID（+0x154）随单元一起搬移，**不重排、不重编号**；所以池下标与 ID 在
    // 本模块之后完全解耦：下标连续 0..n−1，ID 是 1..20 的稀疏集合。
    static void Touch_UnitRemove(TouchBigFrame& bigFrame, int idx) noexcept {
        // 入口把 > 20 的计数强行截到 20 并报断言。FixedVector 容量即 20，
        // 结构上不可达，保留判据。
        if (bigFrame.tracks.size() > kMaxTracks) {
            bigFrame.tracks.erase(bigFrame.tracks.cbegin() +
                                      static_cast<std::ptrdiff_t>(kMaxTracks),
                                  bigFrame.tracks.cend());
        }
        if (idx >= 0 && static_cast<size_t>(idx) < bigFrame.tracks.size()) {
            bigFrame.tracks.erase(bigFrame.tracks.cbegin() + idx,
                                  bigFrame.tracks.cbegin() + idx + 1);
        }
    }

    // ── SideTouch_Report @0x6BB1A7A5（本机关闭）────────────────────────
    void SideTouch_Report(TouchBigFrame& /*bigFrame*/) noexcept {
        // 追加逻辑需要侧边通道触点池（g_touchesSide / DAT_6bc8d3b4 族），复刻侧
        // 无该数据源。打开时的语义：逐个查 `+0x1F8 & 8`，每次追加前再查一次
        // `g_touchesPtr[5] < 20`，通过则 SideTouch_UnitCopyToOutReport 追加到主池
        // 尾部并同时 `[5]++` 与 `*g_touchesPtr++`，超限只报断言不追加。
    }

    // ══════════════════════════════════════════════════════════════════
    // §6 TSA_SetDMDType @0x6BA9D881 —— 不只是日志
    // ══════════════════════════════════════════════════════════════════
    // HardwareAnalyzer_GetDetectedException() 的 0..0x1F 异常码查表映射成位掩码；
    // 掩码为 0 时推进 BigData 打点，BigData_IsReportReady() 为真则掩码取 0x10。
    // 返回值只给日志用，但 `static->nField_0x138 = 掩码` 与 BigData 推进是实打实
    // 的副作用（影响宿主读到的设备状态字），不能整体省掉。
    //
    // B4 已接通：HardwareAnalyzerAndHold::GetDetectedException() 产出原厂的
    // 0..0x1F 异常码（它的 m_exceptions 本来就是原厂 `1 << code` 掩码，缺的
    // 只是取码 + 重复去抖那一层），TouchPipeline 在本级之前灌进来。
    // 本机只有 code 2 / 0x1D 有产出方，其余码位仍扫不到。
    int32_t m_detectedException = 0;   // HardwareAnalyzer_GetDetectedException()
    bool    m_bigDataReportReady = false; // BigData_IsReportReady()
    uint32_t m_bigDataFrameCount = 0;  // BigData_ReportProcess() 的打点推进

    [[nodiscard]] static constexpr int32_t MapExceptionToDmdMask(int32_t exc) noexcept {
        switch (exc) {
        case 1:                return 0x1;
        case 2:                return 0x2;
        case 3:                return 0x8;
        case 4:                return 0x20;
        case 5: case 0x1F:     return 0x8000;
        case 6: case 7:        return 0x4;
        case 8:                return 0x40;
        case 9:                return 0x80;
        case 10:               return 0x100;
        case 0xB:              return 0x200;
        case 0xC:              return 0x400;
        case 0xD:              return 0x800;
        case 0xE:              return 0x1000;
        case 0xF:              return 0x2000;
        case 0x10:             return 0x4000;
        case 0x11: case 0x12:  return 0x10000;
        case 0x13:             return 0x20000;
        case 0x14: case 0x1E:  return 0x1;
        case 0x15:             return 0x40000;
        case 0x16: case 0x17:  return 0x80000;
        case 0x18:             return 0x800000;
        case 0x19:             return 0x1000000;
        case 0x1A:             return 0x2000000;
        case 0x1B:             return 0x4000000;
        case 0x1C:             return 0x8000000;
        default:               return 0;
        }
    }

    int32_t SetDMDType() noexcept {
        int32_t mask = MapExceptionToDmdMask(m_detectedException);
        // 原厂在 exc != 0 时还会调 HardwareAnalyzer_GenerateExceptionDescription()
        // （只生成日志串，无坐标副作用）。
        if (mask == 0) {
            ++m_bigDataFrameCount;            // BigData_ReportProcess()
            if (m_bigDataReportReady) {
                mask = 0x10;
            }
        }
        return mask;                          // → g_tsaStaticPtr->nField_0x138
    }

    // ══════════════════════════════════════════════════════════════════
    // §8 TSA_PitchSizeRemapProcess @0x6BB0611C
    // ══════════════════════════════════════════════════════════════════
    // 原厂对 `i < (u8)*g_touchesPtr` 全池无条件重映射（TSAOut 已把不合格的点
    // 删掉了，剩下的都进包），scale 固定 0x100（Q8），结果 `(s16)` 截断回写。
    // 值域自洽：输入 [0, 15359] ⇒ k ≤ 59，输出上界 1·59.015625 + 255·60.0
    // = 15359.015… 截断为 15359，不会推出量程，后续像素换算无需额外防护。
    void ApplyPitchRemap(TouchBigFrame& bigFrame) noexcept {
        for (auto& track : bigFrame.tracks) {
            const int32_t d1 = SensorPitchSizeMap(track.outX_q8, m_dim1PitchMap.data(), 0x100);
            const int32_t d2 = SensorPitchSizeMap(track.outY_q8, m_dim2PitchMap.data(), 0x100);
            track.outX_q8 = Fixed::WrapU16(d1);   // (s16) 回写，回绕不饱和
            track.outY_q8 = Fixed::WrapU16(d2);
        }
    }

    // ══════════════════════════════════════════════════════════════════
    // §9 对外交付：TSA_Rpt* 导出族 + TSAWrapper_UpdateReport
    // ══════════════════════════════════════════════════════════════════
    // FEAT_AFT_ENABLED = 0 ⇒ 全部 TSA_Rpt* 走触点池分支（§10）。

    /// TSA_RptTouchNum @0x6BA9B09C = *g_touchesPtr
    [[nodiscard]] static uint8_t RptTouchNum(const TouchBigFrame& bf) noexcept {
        return static_cast<uint8_t>(bf.tracks.size());
    }
    /// TSA_RptTouchID @0x6BA9B256 = +0x154 − 1，**0 基**，取值 0..19
    [[nodiscard]] static int32_t RptTouchID(const TouchBigFrame& bf, uint8_t idx) noexcept {
        return static_cast<int32_t>(bf.tracks[idx].id) - 1;
    }
    /// TSA_RptTouchXPos @0x6BA9B428 → TSAOut_RptXPos → TouchXPosInternal(idx, flash[0x5A0])
    [[nodiscard]] uint16_t RptTouchXPos(const TouchBigFrame& bf, uint8_t idx) const noexcept {
        return Fixed::WrapU16(TouchXPosInternal(bf.tracks[idx], m_reportScale));
    }
    [[nodiscard]] uint16_t RptTouchYPos(const TouchBigFrame& bf, uint8_t idx) const noexcept {
        return Fixed::WrapU16(TouchYPosInternal(bf.tracks[idx], m_reportScale));
    }
    /// TSA_RptTouchEvent @0x6BA9B3CC = +0x21C 原样透传；**Tip 位的唯一来源**
    [[nodiscard]] static int32_t RptTouchEvent(const TouchBigFrame& bf, uint8_t idx) noexcept {
        return bf.tracks[idx].state;
    }
    /// HID Tip Switch ← `TSA_RptTouchEvent != 0x40`
    [[nodiscard]] static bool RptTouchTip(const TouchBigFrame& bf, uint8_t idx) noexcept {
        return RptTouchEvent(bf, idx) != EventUpReport;
    }
    /// TSA_RptTouchAxisMajor/Minor @0x6BA9BCA2/0x6BA9BE32 = TSAOut_Axis(TS_MMToPixel(mm))
    [[nodiscard]] static int32_t RptTouchAxisMajor(const TouchBigFrame& bf, uint8_t idx) noexcept {
        return AxisToPixel(bf.tracks[idx].axisMajorMm);
    }
    [[nodiscard]] static int32_t RptTouchAxisMinor(const TouchBigFrame& bf, uint8_t idx) noexcept {
        return AxisToPixel(bf.tracks[idx].axisMinorMm);
    }
    /// TSA_RptTouchAxisAngle @0x6BA9BF37 = TSAOut_Angle(+0x192)
    [[nodiscard]] int32_t RptTouchAxisAngle(const TouchBigFrame& bf, uint8_t idx) const noexcept {
        return RemapAngle(bf.tracks[idx].iirAxisAngleDeg);
    }
    /// TSA_RptTouchXWidth @0x6BA9B4DC / YWidth：按转置位选维，**钳到 ≤ 15**
    [[nodiscard]] uint8_t RptTouchXWidth(const TouchBigFrame& bf, uint8_t idx) const noexcept {
        const auto& t = bf.tracks[idx];
        const uint8_t w = (m_flashCmfForceDim1Pass == m_flashAxisBind) ? t.extentX : t.extentY;
        return w > 0xF ? static_cast<uint8_t>(0xF) : w;
    }
    [[nodiscard]] uint8_t RptTouchYWidth(const TouchBigFrame& bf, uint8_t idx) const noexcept {
        const auto& t = bf.tracks[idx];
        const uint8_t w = (m_flashCmfForceDim1Pass == m_flashAxisBind) ? t.extentY : t.extentX;
        return w > 0xF ? static_cast<uint8_t>(0xF) : w;
    }
    /// TSA_RptTouchSuppressFlag @0x6BA9C4DD = +0x208
    [[nodiscard]] static uint32_t RptTouchSuppressFlag(const TouchBigFrame& bf, uint8_t idx) noexcept {
        return bf.tracks[idx].suppressFlags;
    }
    /// TSA_RptTouchException = +0x1EC
    [[nodiscard]] static uint32_t RptTouchException(const TouchBigFrame& bf, uint8_t idx) noexcept {
        return bf.tracks[idx].exceptionType;
    }

    /// TSAWrapper_GetTouchId @0x6BB29819：`1 << (id & 31)` 拼掩码
    [[nodiscard]] static uint32_t WrapperGetTouchId(const TouchBigFrame& bf) noexcept {
        uint32_t mask = 0;
        const uint8_t n = RptTouchNum(bf);
        for (uint8_t i = 0; i < n; ++i) {
            mask |= 1u << (static_cast<uint32_t>(RptTouchID(bf, i)) & 31u);
        }
        return mask;
    }

    // ── TSAWrapper_UpdateReport @0x6BB2986B（§9.1）────────────────────
    void UpdateOutputReport(const TouchBigFrame& bigFrame) noexcept {
        m_report.touchIdMask = WrapperGetTouchId(bigFrame);
        const uint32_t prevMask = m_prevReport.touchIdMask;

        const uint8_t n = RptTouchNum(bigFrame);
        for (uint8_t idx = 0; idx < n; ++idx) {
            const int32_t rawId = RptTouchID(bigFrame, idx);
            if (rawId < 0 || rawId >= static_cast<int32_t>(m_report.slots.size())) continue;
            const uint8_t id = static_cast<uint8_t>(rawId);

            TouchReportSlot& s = m_report.slots[id];
            s.touchId = id;
            s.xPos = RptTouchXPos(bigFrame, idx);
            s.yPos = RptTouchYPos(bigFrame, idx);

            if ((prevMask & (1u << (id & 31u))) == 0) {
                s.deltaX = 0;
                // **原厂缺陷照抄**（汇编 0x6BB29964 / 0x6BB29989 已确认）：
                // 新触点首帧连写两次 +0x08（dx），+0x0A（dy）从未清零，保留着
                // 上一个占用该槽的触点留下的旧值。不得"修正"成写 dy。
                s.deltaX = 0;
            } else {
                const TouchReportSlot& p = m_prevReport.slots[id];
                s.deltaX = Fixed::Wrap16(static_cast<int32_t>(s.xPos) - static_cast<int32_t>(p.xPos));
                s.deltaY = Fixed::Wrap16(static_cast<int32_t>(s.yPos) - static_cast<int32_t>(p.yPos));
            }
        }

        // 抬起的触点**不清槽**，只是掩码里那一位变 0，消费方必须先读掩码。
        m_prevReport = m_report;   // memcpy(&g_preReport, &g_report, 0x130)
    }

    // ══════════════════════════════════════════════════════════════════
    // 内部子阶段
    // ══════════════════════════════════════════════════════════════════

    static void SeedOutputSlots(TouchBigFrame& bigFrame) noexcept {
        for (auto& track : bigFrame.tracks) {
            track.outX_q8 = track.fltX_q8;
            track.outY_q8 = track.fltY_q8;
        }
    }

    /// +0x1F0 & 0x8000000：06 的 GripFilter 置的握持完全抑制位。复刻的等价产出
    /// 是 Stage 6 的 LifeGripSuppressed，原位一并读。
    [[nodiscard]] static bool IsGripHardSuppressed(const TrackingSlot& t) noexcept {
        return (t.volatileFlags & 0x08000000u) != 0 ||
               (t.lifeFlags & LifeGripSuppressed) != 0;
    }

    // ── §7.2 DoubleReport_Calculate @0x6BADE84B（本机不可达）──────────
    void DoubleReport_Calculate(TouchBigFrame& bigFrame) {
        // memcpy_s(&g_doubleReportTouchUnits, 0x1C20, g_touchesPtr + 0x140, 0x1C20)
        m_backupTracks = bigFrame.tracks;
        m_backupContacts = bigFrame.contacts;
        m_backupValid = true;

        std::array<bool, 21> activeInCurFrame{};

        for (auto& track : bigFrame.tracks) {
            if (track.id <= 0 || track.id > 20) continue;
            activeInCurFrame[track.id] = true;
            auto& h = m_drHistory[track.id];

            const int32_t curX = track.outX_q8;
            const int32_t curY = track.outY_q8;

            if (h.hasPrev1 && h.hasPrev2) {
                const int32_t bx = Bezier(h.prev2X, h.prev1X, curX);
                const int32_t by = Bezier(h.prev2Y, h.prev1Y, curY);
                // 单调性护栏用**闭区间** [min(P_{t−1}, P_t), max(...)]，X 与 Y 必须
                // 同时落在各自区间内；任一越界则两个轴一起降级为线性中点。
                if (bx >= std::min(h.prev1X, curX) && bx <= std::max(h.prev1X, curX) &&
                    by >= std::min(h.prev1Y, curY) && by <= std::max(h.prev1Y, curY)) {
                    track.outX_q8 = Fixed::WrapU16(bx);
                    track.outY_q8 = Fixed::WrapU16(by);
                } else {
                    track.outX_q8 = Fixed::WrapU16((curX + h.prev1X) >> 1);
                    track.outY_q8 = Fixed::WrapU16((curY + h.prev1Y) >> 1);
                }
            } else if (h.hasPrev1) {
                // 只有一帧历史 → 线性中点
                track.outX_q8 = Fixed::WrapU16((curX + h.prev1X) >> 1);
                track.outY_q8 = Fixed::WrapU16((curY + h.prev1Y) >> 1);
            }
            // 两帧历史都缺（按下首帧）→ 完全不动，直接发真实点
        }

        // DoubleReport_UpdatePrevTouch @0x6BADE6DE：历史链存的始终是**真实采样点**
        //（从备份取，不是已被覆写的触点池），插值点不会污染下一次插值。
        for (const auto& backupTrack : m_backupTracks) {
            if (backupTrack.id <= 0 || backupTrack.id > 20) continue;
            auto& h = m_drHistory[backupTrack.id];
            h.prev2X = h.prev1X;
            h.prev2Y = h.prev1Y;
            h.hasPrev2 = h.hasPrev1;
            h.prev1X = backupTrack.outX_q8;
            h.prev1Y = backupTrack.outY_q8;
            h.hasPrev1 = true;
        }

        // 抬起后槽位靠 memset 自动过期
        for (size_t id = 1; id <= 20; ++id) {
            if (!activeInCurFrame[id]) m_drHistory[id] = DoubleReportSlotHistory{};
        }
    }

    // ── 把 TSAOut 的结论同步回 contacts（复刻侧的对外交付面）──────────
    // 原版里 contacts 就是触点池本身，TSAOut 删单元即删触点；复刻的 contacts 是
    // 并行表，所以这里按存活轨迹重建，并在**取数时**做 Q8 → 像素。
    void SynchronizeContacts(TouchBigFrame& bigFrame) const noexcept {
        FixedVector<TouchContact, kMaxContacts> kept;
        const uint8_t n = RptTouchNum(bigFrame);
        for (uint8_t idx = 0; idx < n; ++idx) {
            const TrackingSlot& t = bigFrame.tracks[idx];
            TouchContact c{};
            bool found = false;
            for (const auto& old : bigFrame.contacts) {
                if (old.id == t.id) { c = old; found = true; break; }
            }
            if (!found) c.id = t.id;

            c.isReported = true;
            c.lifeFlags = t.lifeFlags;
            c.reportEvent = t.state;
            c.x = static_cast<float>(RptTouchXPos(bigFrame, idx));
            c.y = static_cast<float>(RptTouchYPos(bigFrame, idx));
            // Tip = 0 只可能来自 TE 0x40（§5）。
            if (!t.tipDown) c.state = TouchStateUp;
            kept.push_back(c);
        }
        bigFrame.contacts = kept;
        bigFrame.activeTouchMask = m_report.touchIdMask;
        bigFrame.hasLiveTouches = n != 0;
    }

    // ── ConfigBinder / ConfigStore ────────────────────────────────────
    // 屏幕尺寸、轴向翻转与边缘裁切同样由 Flash 表描述面板本身，不是上位机偏好，
    // 因此不注册任何运行时键。
    void registerBindings(Config::ConfigBinder& binder) { (void)binder; }
    void applyConfig(const Config::ConfigStore& store) { (void)store; }

private:
    std::array<DoubleReportSlotHistory, 21> m_drHistory{};

    FixedVector<TrackingSlot, kMaxTracks>   m_backupTracks;
    FixedVector<TouchContact, kMaxContacts> m_backupContacts;
    bool m_backupValid = false;

    TSAOutputReport m_report{};
    TSAOutputReport m_prevReport{};

    uint8_t m_reportedNum = 0;    // g_touchesPtr[5] / [6]
    uint8_t m_contactCount = 0;   // g_tsaStaticPtr->bField_0x00
    int32_t m_dmdType = 0;        // g_tsaStaticPtr->nField_0x138
};

} // namespace Solvers::Touch
