#pragma once
// ══════════════════════════════════════════════════════════════════════
// TouchPipeline Stage 4: TouchZoneAndCentroid (Header-only C++23)
// 
// Reverse engineered from TSACore.dll (0x6BA40000) for W273AS2700
// (Gaokun Himax CSOT, 60 columns x 40 rows = 2400 mutual capacitance nodes).
// Full documentation in docs/finger_algorithms/04_touch_zone_and_centroid.md.
//
// 职责（对位文档 04 §1..§7）：
//   1. 泛洪引擎（TZ_Process → TZ_PeakBasedTraversal @0x6BA89162 →
//      TZ_TraversalNeighbor @0x6BA88709）：
//      - 以 g_pPeaks 里未被认领的峰为种子，按索引升序遍历。
//      - 容器是 PosnSpecial：**两个共享同一缓冲区、相向生长的 LIFO 栈**
//        （正差分侧 / 负差分侧），容量 100，满即静默丢弃。**是 DFS 不是 BFS**，
//        先抽干正差分侧再整体切负差分侧（文档 §2.1、复刻要点 2）。
//      - 9 元步进表含中心格，**8 连通**；越界靠 uint8_t 回绕（§2.2、要点 3）。
//      - 停止判据只有两条：mag < thold 的终止层不入栈；栈满静默丢弃。
//        终止层**仍计入全部矩**（§2.4、要点 5）。
//   2. 质心数学（CTD_BasicCalculation @0x6BA551E0 / TZ_CentroidCore @0x6BA7F363）：
//      - 零阶/一阶矩只收正信号；二阶矩张量收 mag（屏内取绝对值，要点 4/6）。
//      - Q8：cog = ((M1 << 8) / M0) + 0x80，m0 == 0 → 0，只钳下界（要点 8）。
//      - 上钳位 bCols*256-1 / bRows*256-1 在 TZ_RegisterTouch @0x6BA8039B（要点 9）。
//   3. 边缘补偿（CTD_ECProcess @0x6BA55CA0）：
//      - 256 项 ln 表 g_ctd256Ln；查表索引 = 最外圈信号和 / 次外圈信号和 × 16。
//      - **只有线性混合，没有三次样条**：dist ≤ 256 全替换、256 < dist < 320
//        线性混合、dist ≥ 320 不作用；ratio = 16 是中性点，off **无下钳位**（要点 14）。
//   4. 共享大帧直出：touchZones / zoneEdge / zones / contacts。
//
// 已知**不复刻**的原厂成分（均已核实为纯遥测或本机不可达）：
//   · TZ_DilateAndErode @0x6BA8BCF4 + TZ_Transform：结果只挂进 g_ideGridData
//     调试网格，算法链路不读（文档 §4、要点 15）。
//   · TZ_ShapeDebounce @0x6BA8A3CF：全库无调用者（§6.8）。
//   · touch+0x194 的 EC 段选择：唯一写入者 TS_ProcessUnit 排在 EC 之后，
//     且每帧 Touch_Clean 清零 ⇒ EC 恒读到 0；四张表 N == 1，段搜索恒取段 0（§7.3）。
// ══════════════════════════════════════════════════════════════════════

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <array>
#include <algorithm>
#include <cmath>
#include <span>

#include "SolverTypes.h"
#include "TouchCoreTypes.hpp"
#include "TouchSharedTypes.h"
#include "TouchFixed.hpp"
#include "TouchGates.hpp"

namespace Config {
class ConfigBinder;
class ConfigStore;
}

namespace Solvers::Touch {

// ── 256 项固化自然对数表 (g_ctd256Ln: round(256 * ln(x))) ─────────────
// 对应 TSACore.dll 0x6BB51D80
inline constexpr uint16_t g_ctd256Ln[256] = {
    0x0000, 0x0000, 0x00B1, 0x0119, 0x0162, 0x019C, 0x01CA, 0x01F2,
    0x0214, 0x0232, 0x024D, 0x0265, 0x027C, 0x0290, 0x02A3, 0x02B5,
    0x02C5, 0x02D5, 0x02E3, 0x02F1, 0x02FE, 0x030B, 0x0317, 0x0322,
    0x032D, 0x0338, 0x0342, 0x034B, 0x0355, 0x035E, 0x0366, 0x036F,
    0x0377, 0x037F, 0x0386, 0x038E, 0x0395, 0x039C, 0x03A3, 0x03A9,
    0x03B0, 0x03B6, 0x03BC, 0x03C2, 0x03C8, 0x03CE, 0x03D4, 0x03D9,
    0x03DF, 0x03E4, 0x03E9, 0x03EE, 0x03F3, 0x03F8, 0x03FD, 0x0401,
    0x0406, 0x040B, 0x040F, 0x0413, 0x0418, 0x041C, 0x0420, 0x0424,
    0x0428, 0x042C, 0x0430, 0x0434, 0x0438, 0x043B, 0x043F, 0x0443,
    0x0446, 0x044A, 0x044D, 0x0451, 0x0454, 0x0458, 0x045B, 0x045E,
    0x0461, 0x0464, 0x0468, 0x046B, 0x046E, 0x0471, 0x0474, 0x0477,
    0x047A, 0x047D, 0x047F, 0x0482, 0x0485, 0x0488, 0x048B, 0x048D,
    0x0490, 0x0493, 0x0495, 0x0498, 0x049A, 0x049D, 0x049F, 0x04A2,
    0x04A4, 0x04A7, 0x04A9, 0x04AC, 0x04AE, 0x04B0, 0x04B3, 0x04B5,
    0x04B7, 0x04BA, 0x04BC, 0x04BE, 0x04C0, 0x04C3, 0x04C5, 0x04C7,
    0x04C9, 0x04CB, 0x04CD, 0x04CF, 0x04D1, 0x04D4, 0x04D6, 0x04D8,
    0x04DA, 0x04DC, 0x04DE, 0x04E0, 0x04E1, 0x04E3, 0x04E5, 0x04E7,
    0x04E9, 0x04EB, 0x04ED, 0x04EF, 0x04F1, 0x04F2, 0x04F4, 0x04F6,
    0x04F8, 0x04FA, 0x04FB, 0x04FD, 0x04FF, 0x0501, 0x0502, 0x0504,
    0x0506, 0x0507, 0x0509, 0x050B, 0x050C, 0x050E, 0x0510, 0x0511,
    0x0513, 0x0514, 0x0516, 0x0518, 0x0519, 0x051B, 0x051C, 0x051E,
    0x051F, 0x0521, 0x0522, 0x0524, 0x0525, 0x0527, 0x0528, 0x052A,
    0x052B, 0x052D, 0x052E, 0x052F, 0x0531, 0x0532, 0x0534, 0x0535,
    0x0537, 0x0538, 0x0539, 0x053B, 0x053C, 0x053D, 0x053F, 0x0540,
    0x0541, 0x0543, 0x0544, 0x0545, 0x0547, 0x0548, 0x0549, 0x054B,
    0x054C, 0x054D, 0x054E, 0x0550, 0x0551, 0x0552, 0x0553, 0x0555,
    0x0556, 0x0557, 0x0558, 0x055A, 0x055B, 0x055C, 0x055D, 0x055E,
    0x0560, 0x0561, 0x0562, 0x0563, 0x0564, 0x0565, 0x0567, 0x0568,
    0x0569, 0x056A, 0x056B, 0x056C, 0x056D, 0x056F, 0x0570, 0x0571,
    0x0572, 0x0573, 0x0574, 0x0575, 0x0576, 0x0577, 0x0578, 0x0579,
    0x057B, 0x057C, 0x057D, 0x057E, 0x057F, 0x0580, 0x0581, 0x0582,
    0x0583, 0x0584, 0x0585, 0x0586, 0x0587, 0x0588, 0x0589, 0x058A
};

// ── 遍历容器 PosnSpecial ───────────────────────────────────────────────
// 对位 PosnSpecial_Init @0x6BB422B6 / _Put @0x6BB42569 / _Get @0x6BB42624 /
// _UpdateSign @0x6BB42265（文档 04 §2.1、复刻要点 2）。
//
// 名为队列，实为**两个共享同一缓冲区、相向生长的 LIFO 栈**：
//   · 正区 [0, head)      —— g_tsaBufDif[pos] >= 0 的格子，Put 写 list[head++]
//   · 负区 [tail, cap)    —— g_tsaBufDif[pos] <  0 的格子，Put 写 list[--tail]
//   · Put 只在 head < tail 时生效，满即**静默丢弃**，无断言、无溢出标志。
//   · Get 先 UpdateSign()：当前极性一侧空了就翻转 mode 并推进一个 zone 槽；
//     mode == 0 弹 list[--head]，mode == 1 弹 list[tail++]。**两侧都是 LIFO**。
// 净效果：扩散是深度优先，且**先把正差分格全部吃干净、再整体切负差分格**。
// 早期复刻用的是 FIFO 环形队列（BFS），在两个 TZ / 两个子区竞争同一批格子时
// 认领顺序与原版不同 —— 而认领是「先到先得、不可抢占」的，顺序即结果。
class PosnSpecialStack {
public:
    static constexpr size_t kCapacity = 100;   // PosnSpecial_Init(buf, 100)

    constexpr void Init() noexcept {
        m_head = 0;
        m_tail = kCapacity;
        m_mode = 0;         // 0 = 正区，1 = 负区
        m_prevMode = 1;
        m_posMarker = 0;
        m_negMarker = 0;
    }

    [[nodiscard]] constexpr bool IsEmpty() const noexcept {
        return m_head == 0 && m_tail == kCapacity;
    }

    /// PosnSpecial_Put：按该格差分的符号决定压哪一侧；满则静默丢弃。
    constexpr void Put(uint16_t pos, bool negative) noexcept {
        if (m_head >= m_tail) return;              // 满 —— 原版同样什么都不做
        if (negative) {
            m_buffer[--m_tail] = pos;
        } else {
            m_buffer[m_head++] = pos;
        }
    }

    /// PosnSpecial_Get：UpdateSign 之后按当前极性弹栈顶。
    constexpr uint16_t Get() noexcept {
        UpdateSign();
        if (m_mode == 0) {
            const uint16_t v = m_buffer[--m_head];
            if (m_prevMode != 0) { ++m_posMarker; m_prevMode = m_mode; }
            return v;
        }
        const uint16_t v = m_buffer[m_tail++];
        if (m_mode != m_prevMode) { ++m_negMarker; m_prevMode = m_mode; }
        return v;
    }

    /// PosnSpecial_GetPosMarker @0x6BB423F7：「正侧被抽空并切换极性」的累计次数，
    /// 下限 1。TZ_GetType @0x6BA837F8 拿它当多指旁证。
    [[nodiscard]] constexpr uint8_t GetPosMarker() const noexcept {
        return m_posMarker == 0 ? uint8_t{1} : m_posMarker;
    }

private:
    /// PosnSpecial_UpdateSign：当前极性一侧空了就翻转。
    constexpr void UpdateSign() noexcept {
        if (m_mode == 1) {
            if (m_tail == kCapacity) m_mode = 0;   // 负区空
        } else {
            if (m_head == 0) m_mode = 1;           // 正区空
        }
    }

    std::array<uint16_t, kCapacity> m_buffer{};
    size_t  m_head = 0;
    size_t  m_tail = kCapacity;
    uint8_t m_mode = 0;
    uint8_t m_prevMode = 1;
    uint8_t m_posMarker = 0;
    uint8_t m_negMarker = 0;
};

// ── Stage 4 主管线模块 ─────────────────────────────────────────────
class TouchZoneAndCentroid {
public:
    static constexpr int kRows = kSensorRows;       // 40 行 (Rx / Y轴)
    static constexpr int kCols = kSensorCols;       // 60 列 (Tx / X轴)
    static constexpr int kTotalCellCount = kTotalCells; // 2400 互容节点
    static constexpr size_t kMaxZoneClusters = kMaxZones; // 最大支持 20 个触控区域

    // ── 模块使能开关 ──────────────────────────────────────────────────
    bool m_enabled = true;

    // ── 核心参数 (严格采用 W273AS2700 专机预置参数) ──────────────────
    int32_t  m_baseThreshold              = 220;    // Flash 0x081C (_2076_2_): dwGripFlags&4 分支的固定门限
    int32_t  m_thresholdRatio             = 64;     // Flash 0x081E (_2078_1_): 内沿比例 64/128 = 0.5
    int32_t  m_thresholdRatioEdge         = 64;     // Flash 0x081F (_2079_1_): 外沿比例 64/128 = 0.5
    int32_t  m_refThreshold               = 3000;   // Flash 0x05B4 (wPeakFilterReferenceThreshold): 3000
    int32_t  m_peakProcessingMode         = 3;      // Flash 0x0034 (dwPeakProcessingMode): 3
    int32_t  m_edgeWidthThreshold         = 300;    // Flash 0x0838 (_2104_2_): 300
    uint32_t m_dwGripFlags                = 0x00000000; // Flash 0x083C (dwGripFlags) 实测全 0

    // ── TZ_GetSigThold @0x6BA8A90B 的模式切换输入 ─────────────────────
    // 五路分发的判据来源。惰性项保留结构与依据，不得因“恒假”而删除。

    // dwFeatureFlags bit12。TouchThold_GetTouchDownThold 与 TZ_GetSigTholdCommon
    // 共用此位，本机未置位（见 TouchThold.hpp GetTouchDownThold 的同一结论）。
    bool     m_featForceEdgeRatio         = false;

    // dwProcessingState bit4 —— 侧边触控通道。MS 手指路径置的是
    // PROC_MS_ACTIVE = 0x4（汇编 6ba90e8f: OR EDX,0x4），bit4 只在
    // SideTouch_Process 的独立通道内置位 ⇒ MS 帧内恒假。
    bool     m_sideTouchProcessing        = false;

    // TSAStatic_IsAFTModeEnabled = dwAftModeFeatureMask & dwAftRuntimeModeMask。
    // 运行时掩码由上层 AFT 模式接口置位，复刻不提供该通道 ⇒ 恒假。
    bool     m_aftModeEnabled             = false;

    // Peak_needUDFPLargeFingerWorkaround：要求 TSA_GetPrmtName() 前四字节为
    // "P115"，本机是 W273AS2700 ⇒ 恒假。只在 AFT 分支内被读。
    bool     m_udfpLargeFingerWorkaround  = false;

    // TZ_GetSigThold 里与 TSAStatic_IsChargerPlugged 并列的另外两条充电器判据：
    // g_pPrevTouches->nTouchCount == 0x100 与 g_tsaStatic->bField_0x111 == 2。
    // 两者都由复刻尚未实现的 Exception_Process 维护，默认取假。
    bool     m_forceChargerSigThold       = false;

    // TZ_IsSD 的 g_pPrevTouches->nTouchCount，由 Tracking 级在帧尾回写。
    uint8_t  m_prevTouchCount             = 0;

    // ── 区域属性阶段 (TZ_GetProperty 尾段 + TZ_GetType) 的整定量 ────────
    // g_tsaPrmtFlash._2102_1_（flash +0x836 = 30）：TZ_GetShapeFlag / TZ_IsLO /
    // DB_LOSize 共用的"大物体特征尺寸"，单位 mm。
    uint8_t  m_largeObjectSizeMm          = 30;
    // g_tsaPrmtFlash._27_1_（flash +0x1B = 69）与 _28_1_（flash +0x1C = 66）：
    // Dim1 / Dim2 的 16 节点毫米跨距，mm → 节点 的换算分母。
    uint8_t  m_pitchFactorDim1            = 69;
    uint8_t  m_pitchFactorDim2            = 66;
    // g_tsaPrmtDynamic：TZ_GetType 在"上一帧是形态胖指"分支里拿它和重心信号比。
    int16_t  m_dynamicGain                = 1000;
    // DAT_6bca92b9：TZ_GetPeakGripFlag 的"离边框多远就一定不是握持"节点距离。
    // 该 DAT 在 .bss 内且 Ghidra 查不到写入者（由整定表整块拷贝填充），
    // 取 0 等价于"只要不在最外圈就判非握持"，与本机 dwGripFlags 全 0 的无握持
    // 形态一致。
    uint8_t  m_gripPeakEdgeDist           = 0;
    // CMF_GetCmfMethod()：本机走 2D 共模噪声估计（method ≠ 1），
    // 故 TZ_GetCMFFlag 的 ZONE_FLAG_LOW_NOISE_CMF 分支可达。
    int32_t  m_cmfMethod                  = 0;
    // g_tsaPrmtFlash.bCmfForceDim1Pass：为 0 时 TZ_GetCMFFlag 的相邻线噪声
    // 沿 Dim1（列）方向取样，为 1 时沿 Dim2（行）取样。
    bool     m_cmfForceDim1Pass           = false;
    // DAT_6bca933c：FIException_CheckWetFinger 的最小水膜格数。同样位于
    // Ghidra 看不到写入者的整定块内。
    uint16_t m_wetFingerMinWaterSize      = 0;
    // DAT_6bca933d / DAT_6bca933e：水膜信号窗口相对全屏正峰的百分比下/上界。
    uint8_t  m_wetFingerLowPct            = 0;
    uint8_t  m_wetFingerHighPct           = 0;
    // 上一帧是否有任何区域被握持分离（prevTouch + 0x218 bit0 的聚合）。
    // 由 TouchPipeline 在帧尾回灌，喂 TZ_CheckToSeparateZone 的强制合并分支。
    bool     m_prevAnyZoneSeparated       = false;

    // TZ_TraversalNeighbor 的握持扣减量 (DAT_6bca9284 / DAT_6bca9286)。
    // 原版只在 dwGripFlags & 0x400 成立时才把这两个 DAT 读进局部量，
    // 本机 dwGripFlags 实测全 0 ⇒ 扣减量恒 0，矩积分等价于不扣减。
    int32_t  m_gripDim1SigSub             = 0;
    int32_t  m_gripDim2SigSub             = 0;

    // TZ_Process @0x6BA8C9F0 的两处 age 处理（TZ_UpdatePeakTzAge / TZ_AgeProcess）
    // 以及 TZ_PeakBasedProcess 尾部的 TZ_UpdateGripFlag 都挂在 dwGripFlags & 0x200
    // 上，本机该位为 0 ⇒ 三者全部惰性，复刻不实现是正确的。
    // 同理 dwGripFlags & 0x100 / & 0x80 为 0，种子遍历走的是 TZ_NormalTraversal。

    // 握持阻断墙判决开关
    bool     m_gripWallEnabled            = true;   // 启用 gripRegion 0x0400 握持屏障墙阻断

    // SubTZ 二次拆分 (SubTz_GetSignalThresh / SubTZ_IsMF)
    bool     m_subTzEnabled               = true;
    int32_t  m_subThresholdRatio          = 64;     // Flash 0x081F: 子区门限 = 峰值 * 64 >> 7
    // Flash 0x0820：SubTz_GetSignalThresh 在 unit[0x350] == 8（SD / 信号摊平
    // 去抖出口）时改用的系数，实测同为 0x40 = 64 ⇒ 与 0x81F 数值等价，
    // 但分支本身可达（§6.2 可达性），不得并成一个常量。
    int32_t  m_subExcThresholdRatio       = 64;
    uint8_t  m_subMfRatioThold            = 48;     // Flash 0x0833: 鞍部/弱峰 < 48/64 才判多指
    // DAT_6bcab6ec：TZ_MFProcess 浅鞍部回退支把长短轴统一设成的固定 mm。
    uint8_t  m_mfFallbackAxisMm           = 10;
    // DB_LOSize @0x6BA55E20 读的是 g_pPrevTouches->field_0x3 ——「上一帧被握持
    // 通道**拒报**的触点数」，不是上一帧上报数。非 0 时 LO 面积门从 900 收紧到
    // 675 mm²。复刻侧握持拒报通道在模块 06，尚未回灌 ⇒ 取 0（不收紧）。
    uint8_t  m_prevGripRejectedCount      = 0;
    // g_tsaPrmtFlash.wPeakMode3ThresholdMax（flash +0x5B0）= 600。
    // TZ_DFSProcess @0x6BA832D9 拿它做「最强峰幅不足就一律判形态胖指」的门。
    int32_t  m_peakMode3ThresholdMax      = 600;

    // TZ_CalcGripRatio 的方向掩码：1=上 2=下 4=左 8=右（转屏时置 0xF）
    uint32_t m_gripRatioDirMask           = 0x0C;

    // 边缘非线性对数补偿参数 (W273AS2700 Flash 0x0840..0x087F)。
    // 完全补偿区宽度 0x100 与样条过渡区宽度 0x40 是 CTD_ECGetFinalOffset 里的
    // 立即数，不是整定项，已内联进该函数；补偿强度系数原版不存在，已删除。

    // W273AS2700 四周边缘对数分段参数
    uint8_t  m_leftEdgeLower              = 16;     // Flash 0x0843
    uint8_t  m_leftEdgeUpper              = 224;    // Flash 0x0844
    uint8_t  m_rightEdgeLower             = 16;     // Flash 0x0854
    uint8_t  m_rightEdgeUpper             = 224;    // Flash 0x0855
    uint8_t  m_topEdgeLower               = 16;     // Flash 0x0865
    uint8_t  m_topEdgeUpper               = 96;     // Flash 0x0866
    uint8_t  m_bottomEdgeLower            = 16;     // Flash 0x0876
    uint8_t  m_bottomEdgeUpper            = 96;     // Flash 0x0877

    // 传感器物理电极几何偏置表 (1/128 电极步距，默认全 0)
    int8_t   m_sensorDim1Offset[kSensorRows][kSensorCols] = {}; // X 轴几何偏置
    int8_t   m_sensorDim2Offset[kSensorRows][kSensorCols] = {}; // Y 轴几何偏置

    TouchZoneAndCentroid() noexcept {
        Reset();
    }

    void Reset() noexcept {
        m_stack.Init();
        m_lastZoneCount = 0;
        m_lastFramePeakPositive = 0;
        std::memset(m_sensorDim1Offset, 0, sizeof(m_sensorDim1Offset));
        std::memset(m_sensorDim2Offset, 0, sizeof(m_sensorDim2Offset));
    }

    [[nodiscard]] int16_t GetLastFramePeakPositive() const noexcept { return m_lastFramePeakPositive; }

    void SetEnabled(bool enabled) noexcept { m_enabled = enabled; }
    [[nodiscard]] bool IsEnabled() const noexcept { return m_enabled; }
    [[nodiscard]] size_t GetZoneCount() const noexcept { return m_lastZoneCount; }

    // ── 核心接口: 针对共享大帧 TouchBigFrame 的零拷贝原位处理 ────────
    bool Process(TouchBigFrame& bigFrame) {
        if (!m_enabled) {
            bigFrame.zones.clear();
            m_lastZoneCount = 0;
            return true;
        }

        // 1. TZ_PeakBasedProcess @0x6BA8BFFD 的帧初始化序列（文档 §1，顺序不可换）：
        //    ① dwFeatureFlags & 0x100000 → TZ_SSLoProcess：实测该位为 0，
        //       g_isSSLo / g_isSSLoSD 停在初值 0（见 IsLargeObject 注释）；
        //    ② 局部 unit 数组 memset 0x370 字节为 0（TZ_UnitInit 的全零）；
        //    ③ memset(g_tsaBufTz, 0, 2400)；
        //    ④ 仅当 PROC_MS_ACTIVE（手指管线恒真）再清 g_tsaBufSubTZ 与
        //       g_tsaBufTzZone；
        //    ⑤ tagBase = 0x10、subTagBase = 0x10、三个 accessedFlag 清零。
        //    复刻侧 touchZones / zoneEdge 承担 g_tsaBufTz / class-2 标记的角色，
        //    m_subClaim / m_subHalo 承担 g_tsaBufSubTZ（按 TZ 包围盒逐区域清）。
        for (int r = 0; r < kRows; ++r) {
            for (int c = 0; c < kCols; ++c) {
                bigFrame.touchZones[r][c] = 0;
                bigFrame.zoneEdge[r][c] = 0;
                // g_tsaBufSubTZ / 其外壳标记同样是**帧级**缓冲，
                // 只在 TZ_PeakBasedProcess 帧初清一次、整帧跨 TZ 持续有效。
                m_subClaim[r][c] = 0;
                m_subHalo[r][c] = 0;
            }
        }
        bigFrame.zones.clear();

        // SubTZ 的跨 TZ 累积全局量在**帧级**复位（TSA_MSProcess 级别，
        // SubTZ_Process 不按 TZ 清零）——复刻要点 17。
        m_maxSubCoreCount = 0;
        m_maxSubMajorMm = 0;
        m_maxSubMinorMm = 0;
        m_subKeptNum = 0;
        m_framePeakCount = static_cast<uint8_t>(std::min<size_t>(bigFrame.peaks.size(), 255));

        // 2. 泛洪步进方向表 —— 对位 TZ_TraversalNeighbor @ 0x6BA88709：
        //    9 项 = 中心 + 8 邻域。索引 0 是中心自身，只做标记与积分，不再入队。
        static constexpr int kDc[9] = {0, 1, 1, 1, 0, -1, -1, -1, 0};
        static constexpr int kDr[9] = {0, -1, 0, 1, 1, 1, 0, -1, -1};

        // 全屏正峰值，供 TZ_GetEdgeSigThold 与分类门限使用 (g_tsaPrpt.nSPeakPositive)。
        // Stage 2.5 已经扫过同一块 diff 算过同一个量，中间没有任何人改写它 ——
        // 直接取用，只有独立入口（没走 Stage 2.5）时才自己扫一遍。
        int16_t framePeakPositive = 0;
        if (bigFrame.framePeakValid) {
            framePeakPositive = bigFrame.framePeakPositive;
        } else {
            for (int r = 0; r < kRows; ++r) {
                for (int c = 0; c < kCols; ++c) {
                    framePeakPositive = std::max(framePeakPositive, bigFrame.diff[r][c]);
                }
            }
        }
        m_lastFramePeakPositive = framePeakPositive;

        // TZ_CheckFatFingerSD @0x6BA8B4F9 在 TZ_PeakBasedProcess 里每帧算一次
        // 后写进本帧所有 TZ 单元的 unit[0x34B]。判据 1 是硬门：
        // `SS_IsDim1Valid() && SS_IsDim2Valid()` 读 flash+0x608（实测 0x00，
        // 自容通道整体关闭）⇒ **本机恒返回 0**（文档 §6.2）。
        // TZ_IsSD @0x6BA8A67E 同样只依赖帧级量，这里也提到循环外。
        const bool isSD = IsSD(framePeakPositive);

        // 3. 遍历候选种子峰：**按峰索引升序遍历全部峰**（要点 1）。
        //    跳过条件是 `g_tsaBufTz[pos] != 0 || g_tsaBufSubTZ[pos] != 0`
        //    —— 两张认领图都要看，且 g_tsaBufSubTZ 是跨 TZ 持续的帧级图。
        for (size_t pi = 0; pi < bigFrame.peaks.size(); ++pi) {
            if (bigFrame.zones.size() >= kMaxZoneClusters) break;

            const auto& seedPeak = bigFrame.peaks[pi];
            if (seedPeak.isSuppressed) continue;

            const uint8_t seedR = seedPeak.r;
            const uint8_t seedC = seedPeak.c;
            if (seedR >= kRows || seedC >= kCols) continue;

            if (bigFrame.touchZones[seedR][seedC] != 0) continue;
            if (m_subClaim[seedR][seedC] != 0) continue;

            // 分配当前区域 ID (1 ~ 20)
            const uint8_t zoneId = static_cast<uint8_t>(bigFrame.zones.size() + 1);

            const int16_t peakSig = bigFrame.diff[seedR][seedC];

            // 泛洪门限 (TZ_GetSigThold)：下界取 Stage 2.5 的 noiseThold
            // （DAT_6bc4a910，稳态 125 / 首帧 600，见 TouchThold.hpp），
            // 充电器判据取本帧 bigFrame.chargerPlugged。
            const int16_t baseThold = GetSigThold(peakSig, IsOuterEdgePeak(seedR, seedC),
                                                  isSD, bigFrame.tholds.noiseThold,
                                                  bigFrame.chargerPlugged);
            // 分类门限 (TZ_GetEdgeSigThold)，用于轮廓/边缘宽度判定
            const int16_t edgeSigThold = GetEdgeSigThold(peakSig, baseThold, framePeakPositive);

            // TZ_NormalTraversal @0x6BA8BAEC：峰值**贴边**（离边框不足 2 格）时临时
            // 清掉 dwGripFlags 的 0x400，也就是只有**内部**区域才对矩做握持扣减 ——
            // 贴边时能量本就残缺，再扣一刀会把重心进一步拽离边框。
            const bool nearBorderPeak = (seedC < 2 || seedC + 2 >= kCols ||
                                         seedR < 2 || seedR + 2 >= kRows);
            const bool applyGripSub = !nearBorderPeak;
            const int32_t gripSubDim1 = applyGripSub ? m_gripDim1SigSub : 0;
            const int32_t gripSubDim2 = applyGripSub ? m_gripDim2SigSub : 0;

            // 4. PosnSpecial_Init(buf, 100) + 种子入栈（不预先标记：中心会在首次
            //    出栈时由索引 0 处理）。对位 TZ_PeakBasedTraversal @0x6BA89162。
            m_stack.Init();
            m_stack.Put(static_cast<uint16_t>(seedR * kCols + seedC),
                        bigFrame.diff[seedR][seedC] < 0);

            int32_t  m0x = 0;          // Σ max(0, sig - gripSubDim1)   → 重心 X 分母 (+0x1c)
            int32_t  m0y = 0;          // Σ max(0, sig - gripSubDim2)   → 重心 Y 分母 (+0x20)
            int32_t  m1x = 0;          // Σ ((c*128 + off) * d1) >> 7   (+0x2c4)
            int32_t  m1y = 0;          // Σ ((r*128 + off) * d2) >> 7   (+0x2c8)
            int32_t  sigSum = 0;       // Σ sig（正向）                  (+0x18)
            // TZ_UpdateMValue 的信号加权几何矩 (+0x2f8..+0x318)，供 TZ_GetAxis 解长短轴
            int64_t  axN = 0, axSx = 0, axSy = 0, axSxy = 0, axSxx = 0, axSyy = 0;
            // TZ_UpdateEdgeInfo 的边沿能量统计 (+0x2cc..+0x2e0)，供 GripFilter 求比值
            int32_t  edgeSum1 = 0, innerSum1 = 0, edgeSum2 = 0, innerSum2 = 0;
            int16_t  edgeMax1 = 0, edgeMax2 = 0;
            uint16_t pixelCount = 0;   // 核心区正向节点数               (+0xc)
            uint16_t haloCount = 0;    // 亚门限外壳节点数
            uint8_t  minR = seedR, maxR = seedR;
            uint8_t  minC = seedC, maxC = seedC;
            bool     touchesGripWall = false;
            uint16_t gripRegionMax = 0;   // 区域属性 (+0x32a)，非阻断条件

            // 5. 9 偏移受控 DFS 泛洪（双向 LIFO 栈，文档 §2.1/§2.3）
            while (!m_stack.IsEmpty()) {
                const uint16_t currPos = m_stack.Get();
                const int currR = currPos / kCols;
                const int currC = currPos % kCols;

                for (int d = 0; d < 9; ++d) {
                    const int nr = currR + kDr[d];
                    const int nc = currC + kDc[d];

                    // 越界
                    if (nr < 0 || nr >= kRows || nc < 0 || nc >= kCols) continue;

                    // 已被本区域或其它区域占用
                    if (bigFrame.touchZones[nr][nc] != 0) continue;

                    const int16_t nSig = bigFrame.diff[nr][nc];

                    // 命中握持阻断墙：既不入队也不积分
                    if (IsGripWall(nr, nc, nSig)) {
                        touchesGripWall = true;
                        continue;
                    }

                    // gripRegion 只作为区域属性参与统计，不阻断泛洪
                    gripRegionMax = std::max(gripRegionMax, bigFrame.gripRegion[nr][nc]);

                    // 最外两列按有符号值判定，其余按绝对值判定
                    // (TZ_TraversalNeighbor 的 local_c 分支)
                    const int32_t cmpSig = IsEdgeColumn(nc) ? static_cast<int32_t>(nSig)
                                                            : std::abs(static_cast<int32_t>(nSig));
                    // TZ_TraversalNeighbor 的 local_19：上一帧被判为握持驻留的格子
                    // **无条件算核心格**，绕过泛洪门限。这条跨帧黏滞是贴边触点在
                    // 最外一列信号摆动时仍能守住包围盒的唯一机制。
                    const bool stickyCore = (bigFrame.gripStage[nr][nc] != 0);
                    const bool isCore = (cmpSig >= baseThold) || stickyCore;

                    // 无论核心还是外壳都要标记，外壳因此成为泛洪的自然停止边界
                    bigFrame.touchZones[nr][nc] = zoneId;

                    if (isCore) {
                        minR = std::min(minR, static_cast<uint8_t>(nr));
                        maxR = std::max(maxR, static_cast<uint8_t>(nr));
                        minC = std::min(minC, static_cast<uint8_t>(nc));
                        maxC = std::max(maxC, static_cast<uint8_t>(nc));
                        if (d != 0) {
                            m_stack.Put(static_cast<uint16_t>(nr * kCols + nc), nSig < 0);
                        }
                    } else {
                        bigFrame.zoneEdge[nr][nc] = zoneId;
                        ++haloCount;
                    }

                    // ── 共同尾部（文档 §2.4）：核心（class 1）与终止层（class 2）
                    //    走的是同一段累加块，**终止层同样计入全部矩**（要点 5）。
                    //    二阶矩张量 TZ_UpdateMValue @0x6BA87890 的进入判据是
                    //    `mag > 0`，**与 sig 的符号无关** —— 屏内负差分格取绝对值
                    //    后被当作正质量累加，会撑大椭圆轴。这是原厂行为，照抄
                    //    （要点 6）。零阶 / 一阶矩才是 `sig >= 1` 门。
                    if (cmpSig > 0) {
                        const int64_t w  = cmpSig;
                        const int64_t gx = nc;
                        const int64_t gy = nr;
                        axN   += w;
                        axSx  += w * gx;
                        axSy  += w * gy;
                        axSxy += w * gx * gy;
                        axSxx += w * gx * gx;
                        axSyy += w * gy * gy;
                    }

                    if (nSig >= 1) {
                        const int32_t d1 = std::max<int32_t>(0, static_cast<int32_t>(nSig) - gripSubDim1);
                        const int32_t d2 = std::max<int32_t>(0, static_cast<int32_t>(nSig) - gripSubDim2);
                        const int32_t xPhy = static_cast<int32_t>(nc) * 128 + m_sensorDim1Offset[nr][nc];
                        const int32_t yPhy = static_cast<int32_t>(nr) * 128 + m_sensorDim2Offset[nr][nc];

                        m0x += d1;
                        m0y += d2;
                        // trunc7：逐格向零截断后再求和，累积舍入误差属于行为的一部分
                        m1x += Fixed::ShrTowardZero(xPhy * d1, 7);
                        m1y += Fixed::ShrTowardZero(yPhy * d2, 7);
                        sigSum += nSig;
                        if (isCore) ++pixelCount;

                        // TZ_UpdateEdgeInfo：最外/次外行列的能量，核心节点另记最大值
                        AccumulateEdgeInfo(nr, nc, nSig, isCore,
                                           edgeSum1, innerSum1, edgeSum2, innerSum2,
                                           edgeMax1, edgeMax2);
                    }
                }
            }

            // 空区域（种子本身就被判为负或被墙挡住）直接丢弃
            if (pixelCount == 0) {
                continue;
            }

            // 5b. 椭圆率必须在 zoneEdge 被物理边框标记污染之前统计
            const uint16_t ellipseRate = ComputeEllipseRate(bigFrame, zoneId, minR, maxR, minC, maxC);

            // 6. 8-bit 定点小数重心计算 (CTD_BasicCalculation & TZ_CentroidCore)
            // X_cog = floor((M1x << 8) / M0x) + 0x80，X/Y 分母各自独立。
            // 上钳位 bCols*256-1 / bRows*256-1 在原版属于 TZ_RegisterTouch
            // @0x6BA8039B（对 zone+0x2ae/+0x2b0 就地改写，复刻要点 9）。
            const uint16_t cogX_q8 = ClampCogDim1(CTD_BasicCalculation(m1x, m0x));
            const uint16_t cogY_q8 = ClampCogDim2(CTD_BasicCalculation(m1y, m0y));
            const int32_t m0 = m0x;

            // 7. 边缘非线性对数拉伸补偿 (CTD_ECProcess)
            //    外推量的自变量是贴边能量比 (zone+0x334/0x335)，原版在 TZ_GetProperty
            //    阶段就算好了，EC 只是读取，因此这里必须先于补偿求值。
            //    下面的 FinalizeGripRatio 会把同一对比值原样写回 cluster。
            const uint8_t gripRatioDim1 = EdgeRatio(edgeSum1, innerSum1);
            const uint8_t gripRatioDim2 = EdgeRatio(edgeSum2, innerSum2);
            uint16_t ecX_q8 = cogX_q8;
            uint16_t ecY_q8 = cogY_q8;
            ApplyEdgeCompensation(minR, maxR, minC, maxC, cogX_q8, cogY_q8,
                                  gripRatioDim1, gripRatioDim2, ecX_q8, ecY_q8);

            // 8. 区域边界已在泛洪过程中由亚门限外壳节点标记（对应原版 base+2 标记），
            //    这里只补上贴屏物理边缘的核心节点。
            for (int r = minR; r <= maxR; ++r) {
                for (int c = minC; c <= maxC; ++c) {
                    if (bigFrame.touchZones[r][c] != zoneId) continue;
                    if (r == 0 || r == kRows - 1 || c == 0 || c == kCols - 1) {
                        bigFrame.zoneEdge[r][c] = zoneId;
                    }
                }
            }

            // 9. 组装并写入触控区域实体 ZoneCluster
            ZoneCluster cluster{};
            cluster.id = zoneId;
            cluster.peakIndex = static_cast<uint8_t>(pi);
            cluster.m0 = m0;
            cluster.m1x = m1x;
            cluster.m1y = m1y;
            cluster.cogX_q8 = cogX_q8;
            cluster.cogY_q8 = cogY_q8;
            cluster.ecX_q8 = ecX_q8;
            cluster.ecY_q8 = ecY_q8;
            cluster.edgeDistDim1_q8 = EdgeDistDim1(ecX_q8);
            cluster.edgeDistDim2_q8 = EdgeDistDim2(ecY_q8);
            cluster.minR = minR;
            cluster.maxR = maxR;
            cluster.minC = minC;
            cluster.maxC = maxC;
            cluster.pixelCount = pixelCount;
            cluster.haloCount = haloCount;
            cluster.sigSum = sigSum;
            cluster.edgeSigThold = edgeSigThold;
            cluster.baseSigThold = baseThold;
            cluster.isGripWall = touchesGripWall;
            cluster.gripRegionMax = gripRegionMax;
            CalcAxis(axN, axSx, axSy, axSxy, axSxx, axSyy,
                     cluster.axisMajorQ8, cluster.axisMinorQ8, cluster.axisAngleDeg);
            cluster.axisMajorMm = PixelToMM(cluster.axisMajorQ8);
            cluster.axisMinorMm = PixelToMM(cluster.axisMinorQ8);
            cluster.ellipseRate = ellipseRate;
            cluster.edgeSumDim1 = edgeSum1;
            cluster.innerSumDim1 = innerSum1;
            cluster.edgeSumDim2 = edgeSum2;
            cluster.innerSumDim2 = innerSum2;
            cluster.edgeMaxDim1 = edgeMax1;
            cluster.edgeMaxDim2 = edgeMax2;
            FinalizeGripRatio(bigFrame, cluster, peakSig);

            // 10. SubTZ_Process：对本 TZ 的**每一个峰**各跑一次二次扩散
            //     （不是「峰数 >= 2 才挂载」——单峰 TZ 也要跑，因为
            //     TZ_JudgeFFOrNF 的唯一输入 DAT_6bbc8684 就在这里产出）。
            //     返回 true 表示已按多指拆分并自行写入了多个区域。
            if (!TrySplitZone(bigFrame, cluster, peakSig, baseThold, edgeSigThold,
                              gripSubDim1, gripSubDim2)) {
                // 未拆分：在本 TZ 的 SubTZ_Process 跑完之后取快照，等价于原版
                // 「累积到本 TZ 为止」的 DAT_6bbc8684 / 8686 / 8681 值。
                SnapshotSubTzGlobals(cluster);
                bigFrame.zones.push_back(cluster);
            }
        }

        // 10.5 区域有效性闸：TZ_UnitProcess @0x6BA8B236 在 TZ_GetProperty 之前
        //      先跑 TZ_PeakInfoRetrieval → TZ_BeforeSDCompensationPeakInfoRetrieval
        //      → TZ_IsValid @0x6BA8AACA，不过闸的区域**根本不进入**属性阶段与
        //      触点注册（文档 04 §TZ_IsValid）。复刻此前缺 B1 通道，整条闸没接。
        FilterInvalidZones(bigFrame);

        // 11. 区域属性收尾：对位 TZ_GetProperty @0x6BA8B126 在 SubTZ_Process 之后
        //     的那一串标志产出（TZ_GetShapeFlag / TZ_GetSurroundingStatusFlag /
        //     TZ_GetCMFFlag / TZ_CheckToSeparateZone），随后是 Exception_ProcessTouchZone
        //     与 TZ_GetType。原版跑在 TZ 单元内、拆分之后，这里同样对拆分后的
        //     每个区域各跑一遍。
        for (auto& zc : bigFrame.zones) {
            ClassifyZone(bigFrame, zc);
        }

        // 12. 同步触点列表 bigFrame.contacts 以便下游阶段直接使用
        bigFrame.contacts.clear();
        for (const auto& zc : bigFrame.zones) {
            TouchContact tc{};
            tc.id = zc.id;
            tc.x = static_cast<float>(zc.ecX_q8) / 256.0f;
            tc.y = static_cast<float>(zc.ecY_q8) / 256.0f;
            tc.rawXBeforeEC = static_cast<float>(zc.cogX_q8) / 256.0f;
            tc.rawYBeforeEC = static_cast<float>(zc.cogY_q8) / 256.0f;
            tc.signalSum = zc.m0;
            tc.area = zc.pixelCount;
            tc.state = TouchStateDown;
            tc.isEdge = (zc.minC == 0 || zc.maxC == kCols - 1 || zc.minR == 0 || zc.maxR == kRows - 1);
            if (zc.ecX_q8 != zc.cogX_q8) tc.ecFlags |= 0x100;
            if (zc.ecY_q8 != zc.cogY_q8) tc.ecFlags |= 0x200;
            bigFrame.contacts.push_back(tc);
        }

        m_lastZoneCount = bigFrame.zones.size();
        m_prevTouchCount = static_cast<uint8_t>(std::min<size_t>(bigFrame.contacts.size(), 255));
        UpdatePrevShapeTable(bigFrame);
        return true;
    }

    /// 帧末刷新跨帧形态历史表，供下一帧的 TZ_GetDBShape / TZ_GetPrevShape 查。
    /// 年龄递推对位 prevTouch + 0x156（轨迹年龄，帧）。
    void UpdatePrevShapeTable(const TouchBigFrame& bigFrame) noexcept {
        std::array<PrevShapeEntry, kMaxZoneClusters> next{};
        size_t n = 0;
        for (const auto& zc : bigFrame.zones) {
            if (n >= next.size()) break;
            if (zc.peakIndex >= bigFrame.peaks.size()) continue;
            const uint8_t pid = bigFrame.peaks[zc.peakIndex].id;
            if (pid == 0) continue;
            uint16_t age = 0;
            for (size_t i = 0; i < m_prevShapeCount; ++i) {
                if (m_prevShapes[i].id == pid) {
                    age = static_cast<uint16_t>(std::min<int>(0xFFFF, m_prevShapes[i].age + 1));
                    break;
                }
            }
            next[n].id = pid;
            next[n].shape = zc.zoneType;
            next[n].age = age;
            ++n;
        }
        m_prevShapes = next;
        m_prevShapeCount = n;
    }

    /// TZ_GetPrevShape @0x6BA8A030：峰 → 上一帧轨迹 → 形态（+0x1E0）。
    /// 峰没有继承 ID（首帧 / 刚按下）或上一帧没有该轨迹 ⇒ 返回 0。
    [[nodiscard]] int32_t GetPrevShape(uint8_t peakId) const noexcept {
        if (peakId == 0) return 0;
        for (size_t i = 0; i < m_prevShapeCount; ++i) {
            if (m_prevShapes[i].id == peakId) return m_prevShapes[i].shape;
        }
        return 0;
    }

    /// TZ_GetPrevAge @0x6BA8A186：同路取历史 +0x156。
    [[nodiscard]] uint16_t GetPrevAge(uint8_t peakId) const noexcept {
        if (peakId == 0) return 0;
        for (size_t i = 0; i < m_prevShapeCount; ++i) {
            if (m_prevShapes[i].id == peakId) return m_prevShapes[i].age;
        }
        return 0;
    }

    // ── 兼容入口: 针对经典 HeatmapFrame ──────────────────────────────
    bool Process(HeatmapFrame& frame) {
        if (!m_enabled) return true;

        if (frame.touch.bigFrame != nullptr) {
            if (frame.touch.bigFrame->peaks.empty() && !frame.touch.runtime.peaks.empty()) {
                for (const auto& pk : frame.touch.runtime.peaks) {
                    PeakCandidate cand{};
                    cand.r = static_cast<uint8_t>(pk.r);
                    cand.c = static_cast<uint8_t>(pk.c);
                    cand.z = pk.z;
                    cand.id = pk.id;
                    frame.touch.bigFrame->peaks.push_back(cand);
                }
            }
            if (frame.touch.bigFrame->peaksBeforeSwitchDif.count == 0) {
                SynthesizeBeforeSdSnapshot(*frame.touch.bigFrame);
            }
            const bool ok = Process(*frame.touch.bigFrame);
            frame.touch.output.contacts.clear();
            for (const auto& tc : frame.touch.bigFrame->contacts) {
                frame.touch.output.contacts.push_back(tc);
            }
            return ok;
        }

        TouchBigFrame bigFrame{};
        bigFrame.ResetPerFrame();
        for (int r = 0; r < kRows; ++r) {
            for (int c = 0; c < kCols; ++c) {
                bigFrame.diff[r][c] = frame.heatmapMatrix[r][c];
            }
        }

        for (size_t i = 0; i < frame.touch.runtime.peaks.size(); ++i) {
            const auto& pk = frame.touch.runtime.peaks[i];
            PeakCandidate cand{};
            cand.r = static_cast<uint8_t>(pk.r);
            cand.c = static_cast<uint8_t>(pk.c);
            cand.z = pk.z;
            cand.id = pk.id;
            bigFrame.peaks.push_back(cand);
        }
        // B1：这条兼容入口不经 PeakAndGripRegion，g_peaksBeforeSwitchDifBuf
        // 通道没人填。本机 SS_ChargerNoiseFilterSwitchBuffer /
        // SignalDisparity_SwitchBuffer 恒 0 ⇒ Peak_Process 永不重跑，快照内容
        // 与本帧峰表一致（文档 03 §11），这里照此就地补齐，
        // 否则 TZ_IsValid 的 +0x2B4 闸会把全部区域误杀。
        SynthesizeBeforeSdSnapshot(bigFrame);

        bool ok = Process(bigFrame);
        if (!ok) return false;

        frame.touch.output.contacts.clear();
        for (const auto& tc : bigFrame.contacts) {
            frame.touch.output.contacts.push_back(tc);
        }
        return true;
    }

    // ══════════════════════════════════════════════════════════════════
    // 区域属性阶段：TZ_GetProperty @0x6BA8B126 的尾段 + TZ_GetType @0x6BA837F8
    // ══════════════════════════════════════════════════════════════════
    // 原版在 MS 路径（dwProcessingState & PROC_MS_ACTIVE）下按这个顺序跑：
    //   Exception_ProcessTouchZone → TZ_CheckToSeparateZone → SubTZ_Process
    //   → TZ_GetShapeFlag → TZ_GetSurroundingStatusFlag → TZ_GetCMFFlag
    //   → TZ_VPProcess → TZ_GetType
    // 复刻把 SubTZ 拆分（TrySplitZone）提前到了区域装配里，其余原序保留。
    // 产出的 zoneFlags / zoneType / exceptionState 三件套是 TZ_UpdateTouchFlag
    // 与 TZ_RegisterTouch 的唯一输入。
    void ClassifyZone(const TouchBigFrame& bigFrame, ZoneCluster& zc) const noexcept {
        const uint8_t zonePeakCount = CountZonePeaks(bigFrame, zc);

        // 这三项在原版由 TZ_GetProperty 前段（TZ_CentroidCore / TZ_GetAxis /
        // TZ_GetShapeRatio / TZ_GetAveCentSig）产出，复刻在这里一次补齐。
        zc.centSig = ComputeAveCentSig(bigFrame, zc);
        zc.shapeRatio = ComputeShapeRatio(zc);

        uint32_t flags = zc.zoneFlags;

        // ── TZ_GetEdgeTouchedFlag @0x6BA81544 ─────────────────────────
        if (zc.minC == 0 || zc.maxC == kCols - 1 || zc.minR == 0 || zc.maxR == kRows - 1) {
            flags |= ZONE_FLAG_EDGE_TOUCHED;
        }
        if (zc.minC < 2 || zc.maxC >= kCols - 2 || zc.minR < 2 || zc.maxR >= kRows - 2) {
            flags |= ZONE_FLAG_NEAR_EDGE;
        }

        // ── TZ_GetHTIValidFlag @0x6BA81626 ────────────────────────────
        if (static_cast<uint32_t>(zonePeakCount) * 4u < zc.pixelCount) {
            flags |= ZONE_FLAG_HTI_VALID;
        }

        // ── 异常通道：Exception_ProcessTouchZone @0x6BA591D1 ───────────
        // 前置门 GRIP_FLAG_0x20（dwGripFlags bit5）是 Kind::EitherOr：置位才
        // **跳过**异常处理；实测为 0 ⇒ 照常执行，unit[0x350] 每帧被写。
        // 把它当死枝删掉会漏写 9 个下游字段。dwPeakProcessingMode == 3 ⇒ FI 分支。
        if (!Gates::IsOn("GRIP_FLAG_0x20")) {
            zc.exceptionState = FIExceptionProcessTouchZone(bigFrame, zc, zonePeakCount);
        }

        // ── TZ_CheckToSeparateZone @0x6BA855B7 ────────────────────────
        // 由 dwFeatureFlags & 0x20 放行（本机 0x01090121，该位为 1）。
        flags |= CheckToSeparateZone(flags);

        // ── TZ_GetShapeFlag @0x6BA81FDB ───────────────────────────────
        flags |= GetShapeFlag(zc);

        // ── TZ_GetSurroundingStatusFlag @0x6BA81EFD ───────────────────
        // TZ_GetSurroundingStatus 的四条判据每一条都以
        // `6 < g_tsaBufSelf[...]` 开头 —— 那是**自容通道**的行/列积分。本机
        // flash[0x608] = 0，自容整体关闭，该缓冲区恒零 ⇒ 函数恒返回 0。
        // 结构保留、实现短路，据此 ZONE_FLAG_SURROUNDED 在 W273 上永不置位，
        // 连带 touch + 0x1f0 bit25 与 g_touchesPtr[0x24] bit4 同样惰性。
        if (SurroundingStatus()) {
            flags |= ZONE_FLAG_SURROUNDED;
        }

        // ── TZ_GetCMFFlag @0x6BA82694 ─────────────────────────────────
        flags |= GetCmfFlag(bigFrame, zc, flags);

        // ── TZ_GetPeakGripFlag @0x6BA84485 ────────────────────────────
        // 原版只在 TZ_CheckToSeparateZone 内部调用两次；由于它是纯区域属性
        // 且 CheckToSeparateZone 要读它的结果，这里提到分离判定之后重跑一次，
        // 语义等价于原版第二次调用。
        flags |= GetPeakGripFlag(bigFrame, zc);

        // ── TZ_VPProcess @0x6BA8D348 ──────────────────────────────────
        // 虚拟峰插入只在 TZ_IsVirtualPeakAllowed 放行时发生，后者要求
        // dwGripFlags 的握持通道开启（本机整字为 0）⇒ 整条链惰性，
        // ZONE_FLAG_VIRTUAL_PEAK 永不置位。

        zc.zoneFlags = flags;

        // ── TZ_GetType @0x6BA837F8 ────────────────────────────────────
        zc.zoneType = GetZoneType(bigFrame, zc, zonePeakCount);

        // ── TZ_MultiFatFingerCheck @0x6BA836A1（TZ_GetType 尾部）────────
        if (zc.zoneType == ZoneTypeMultiFinger) {
            zc.zoneFlags |= MultiFatFingerCheck(zc);
        }
    }

    // ══════════════════════════════════════════════════════════════════
    // B1：g_peaksBeforeSwitchDifBuf @0x6BBC0B40 → TZ_IsValid
    // ══════════════════════════════════════════════════════════════════
    /// 兜底：不经 Stage 3 的兼容入口没人填补偿前峰快照，按「快照 == 本帧峰表」
    /// 就地生成（文档 03 §11 的可达性结论）。正常管线不会走到这里。
    static void SynthesizeBeforeSdSnapshot(TouchBigFrame& bigFrame) noexcept {
        PeakTable& snap = bigFrame.peaksBeforeSwitchDif;
        snap = PeakTable{};
        const size_t n = std::min<size_t>(bigFrame.peaks.size(),
                                          static_cast<size_t>(kPeakMaxNum));
        for (size_t i = 0; i < n; ++i) {
            const auto& pk = bigFrame.peaks[i];
            snap.e[i].id  = pk.id;
            snap.e[i].c   = pk.c;
            snap.e[i].r   = pk.r;
            snap.e[i].sig = pk.z;
            snap.e[i].z8  = pk.nZ8;
            snap.e[i].zn  = pk.nZ4;
        }
        snap.count = static_cast<uint8_t>(n);
    }

    /// TZ_BeforeSDCompensationPeakInfoRetrieval @0x6BA8ADEC（文档 04 §TZ_IsValid）。
    /// 遍历**补偿前峰表**，凡是峰位落在 TZ 标签图非零格、且尚未被别的区域认领的，
    /// 就记到当前区域的 +0x2B4 计数上并置认领位 g_pkBeforeSDCAccessedFlag。
    ///
    /// 两处对位细节：
    ///   · 原版判据是 `g_tzAccess[pos] != 0`（标签图**任意非零**），不是
    ///     `== 本区域标签`。由于 TZ_NormalTraversal 逐峰泛洪、跑完一个单元
    ///     立刻 UnitProcess，标签图此刻只含已处理区域 + 当前区域，而已处理
    ///     区域的峰早被认领位挡掉 ⇒ 等价于「落在本区域标签内」。复刻的区域
    ///     标签互不重复（拆分子区已重贴标签），按标签比较即可。
    ///   · 原版这里还会把 +0x2B5 **覆写**成补偿前峰表里最强峰的下标，冲掉
    ///     TZ_GetMaxMinPkIdx @0x6BA82207 刚写进去的当前峰表 maxSig 下标 ——
    ///     两张表的下标空间不同，这是原厂缺陷。它的唯一去向是
    ///     TZ_RegisterTouch @0x6BA80831 写 touch + 0x23D，复刻侧无消费者，
    ///     故只记录不落字段（照抄不修正的前提是有人读，这里没有）。
    [[nodiscard]] static uint8_t CountBeforeSdPeaks(const TouchBigFrame& bigFrame,
                                                    const ZoneCluster& zc,
                                                    uint32_t& claimMask) noexcept {
        uint8_t n = 0;
        const auto& snap = bigFrame.peaksBeforeSwitchDif;
        const uint8_t cnt = static_cast<uint8_t>(
            std::min<size_t>(snap.count, static_cast<size_t>(kPeakMaxNum)));
        for (uint8_t i = 0; i < cnt; ++i) {
            const uint32_t bit = 1u << (i & 0x1Fu);
            if ((claimMask & bit) != 0) continue;
            const uint8_t r = snap.e[i].r;
            const uint8_t c = snap.e[i].c;
            if (r >= kRows || c >= kCols) continue;
            if (bigFrame.touchZones[r][c] != zc.id) continue;
            claimMask |= bit;
            ++n;
        }
        return n;
    }

    /// TZ_IsValid @0x6BA8AACA：
    ///   +0x2B2 == 0                       ⇒ 无效（区域内一个当前峰都没有）
    ///   !(dwProcessingState & PROC_MS_ACTIVE) ⇒ 有效（手指管线恒真，此支不可达）
    ///   +0x2B4 == 0                       ⇒ 无效（区域只由 SD 补偿**之后**新增的
    ///                                       峰构成 —— 整区丢弃）
    /// 无效区域不跑 TZ_GetPrevTouch / TZ_GetProperty / TZ_UnitToTouch，
    /// 因此在复刻里直接从 bigFrame.zones 里剔除。
    void FilterInvalidZones(TouchBigFrame& bigFrame) const noexcept {
        uint32_t beforeSdClaim = 0;   // g_pkBeforeSDCAccessedFlag，帧级
        size_t kept = 0;
        for (size_t i = 0; i < bigFrame.zones.size(); ++i) {
            const ZoneCluster& zc = bigFrame.zones[i];
            const uint8_t curPeaks = CountZonePeaks(bigFrame, zc);        // +0x2B2
            const uint8_t sdPeaks  = CountBeforeSdPeaks(bigFrame, zc, beforeSdClaim);  // +0x2B4
            if (curPeaks == 0) continue;
            if (sdPeaks == 0) continue;   // PROC_MS_ACTIVE 在手指管线恒真
            if (kept != i) bigFrame.zones[kept] = bigFrame.zones[i];
            ++kept;
        }
        bigFrame.zones.resize(kept);
    }

    /// 落在本区域标签内的候选峰个数 (zone + 0x2b2)
    [[nodiscard]] static uint8_t CountZonePeaks(const TouchBigFrame& bigFrame,
                                                const ZoneCluster& zc) noexcept {
        uint32_t n = 0;
        for (size_t i = 0; i < bigFrame.peaks.size(); ++i) {
            const auto& pk = bigFrame.peaks[i];
            if (pk.r >= kRows || pk.c >= kCols) continue;
            if (bigFrame.touchZones[pk.r][pk.c] == zc.id) ++n;
        }
        return static_cast<uint8_t>(std::min<uint32_t>(n, 255));
    }

    /// TZ_GetAveCentSig @0x6BA82B60：取重心所在整数格及其（按 Q8 小数部分决定的）
    /// 右/下邻格，对有效格的 Dif 求算术平均。结果写 zone + 0x2e，
    /// TZ_RegisterTouch 转存 touch + 0x29e，Touch_UnitUpdateMaxMin 再锁存成 +0x1ba。
    [[nodiscard]] static int16_t ComputeAveCentSig(const TouchBigFrame& bigFrame,
                                                   const ZoneCluster& zc) noexcept {
        const int cx = static_cast<int>(zc.cogX_q8) >> 8;
        const int cy = static_cast<int>(zc.cogY_q8) >> 8;
        const bool takeRight = (zc.cogX_q8 & 0xFFu) != 0;
        const bool takeDown  = (zc.cogY_q8 & 0xFFu) != 0;

        int32_t sum = 0;
        uint8_t cnt = 0;
        auto accumulate = [&](int r, int c) {
            if (r < 0 || r >= kRows || c < 0 || c >= kCols) return;
            sum += bigFrame.diff[r][c];
            ++cnt;
        };
        accumulate(cy, cx);
        if (takeRight) accumulate(cy, cx + 1);
        if (takeDown)  accumulate(cy + 1, cx);
        if (takeRight && takeDown) accumulate(cy + 1, cx + 1);
        if (cnt == 0) cnt = 1;   // 原版 local_b[0] == 0 时强制成 1
        return static_cast<int16_t>(sum / cnt);
    }

    /// TZ_GetShapeRatio @0x6BA833CB：实际核心面积 ÷ 长短轴围出的理论面积，×16。
    /// 理论面积为 0 时钳成 0xff。写 zone + 0x2f5，TZ_IsLO 读。
    [[nodiscard]] uint8_t ComputeShapeRatio(const ZoneCluster& zc) const noexcept {
        const uint32_t denom = (static_cast<uint32_t>(zc.axisMinorMm) *
                                static_cast<uint32_t>(zc.axisMajorMm) * 256u) /
                               std::max<uint32_t>(1u, m_pitchFactorDim1) /
                               std::max<uint32_t>(1u, m_pitchFactorDim2);
        if (denom == 0) return 0xFFu;
        return static_cast<uint8_t>(std::min<uint64_t>(
            255u, (static_cast<uint64_t>(zc.pixelCount) << 4) / denom));
    }

    /// TZ_GetShapeFlag @0x6BA81FDB：五条独立判据，任一命中即置 ZONE_FLAG_BIG_SHAPE。
    /// 原版比较的是**最大子区**的长短轴，复刻里拆分后的每个区域就是它自己。
    [[nodiscard]] uint32_t GetShapeFlag(const ZoneCluster& zc) const noexcept {
        const int major = zc.axisMajorMm;
        const int minor = zc.axisMinorMm;
        // (1) 长轴超过 1.5 倍大物体特征尺寸：(30 × 16 − 30) / 10 = 45 mm
        if ((m_largeObjectSizeMm * 16 - m_largeObjectSizeMm) / 10 < major) {
            return ZONE_FLAG_BIG_SHAPE;
        }
        // (2) 长轴是短轴 3 倍以上且本身超过 15 mm ⇒ 细长条
        if (minor * 3 < major && major > 15) {
            return ZONE_FLAG_BIG_SHAPE;
        }
        // (3)(4) 包围盒跨度达到"大物体特征尺寸折算出的节点数"
        const int spanLimit = (m_largeObjectSizeMm * 16) /
                              std::max<int>(1, m_pitchFactorDim1) - 1;
        const int spanDim1 = static_cast<int>(zc.maxC) - static_cast<int>(zc.minC) + 1;
        const int spanDim2 = static_cast<int>(zc.maxR) - static_cast<int>(zc.minR) + 1;
        if (spanLimit <= spanDim1 || spanLimit <= spanDim2) {
            return ZONE_FLAG_BIG_SHAPE;
        }
        // (5) 任一方向的边沿能量比过高 ⇒ 能量摊在边框上的大接触
        if (zc.gripRatioDim1 > 0x13 || zc.gripRatioDim2 > 0x13) {
            return ZONE_FLAG_BIG_SHAPE;
        }
        return 0;
    }

    /// TZ_GetSurroundingStatus @0x6BA819D9。自容通道关闭 ⇒ 恒假，见 ClassifyZone 注释。
    [[nodiscard]] static constexpr bool SurroundingStatus() noexcept { return false; }

    /// TZ_GetCMFFlag @0x6BA82694
    [[nodiscard]] uint32_t GetCmfFlag(const TouchBigFrame& bigFrame, const ZoneCluster& zc,
                                      uint32_t flagsSoFar) const noexcept {
        if (zc.peakIndex >= bigFrame.peaks.size()) return 0;
        const auto& pk = bigFrame.peaks[zc.peakIndex];
        if (pk.r >= kRows || pk.c >= kCols) return 0;

        uint32_t out = 0;
        const int32_t noise = bigFrame.cmNoise[pk.r][pk.c];     // CMF_GetNoiseValue
        const int32_t sigPlusNoise = noise + pk.z;

        // TouchThold_ToeUseSpecialThold：峰落在特定的首列 / 末二列 / 末行上时改用
        // 低灵敏门限副本。原版的三条坐标判据在这里逐条照搬。
        int32_t limit = bigFrame.tholds.sigThold;
        if (pk.c == 1 || pk.c == kCols - 2 || pk.r == kRows - 1) {
            limit = bigFrame.tholds.sigTholdLow;
        }
        if (sigPlusNoise < limit && m_cmfMethod != 1) {
            out |= ZONE_FLAG_LOW_NOISE_CMF;
        }

        // 第二块只在"大形态或被包围"的区域上跑。
        if ((flagsSoFar & (ZONE_FLAG_BIG_SHAPE | ZONE_FLAG_SURROUNDED)) != 0) {
            const int32_t ref = bigFrame.tholds.peakFilterRef;
            auto sample = [&](int r, int c) -> int32_t {
                if (r < 0 || r >= kRows || c < 0 || c >= kCols) return 0;
                return bigFrame.cmNoise[r][c];
            };
            int32_t n1, n2;
            if (!m_cmfForceDim1Pass) {
                n1 = sample(zc.minR, static_cast<int>(zc.minC) - 1);
                n2 = sample(zc.maxR, static_cast<int>(zc.maxC) + 1);
            } else {
                n1 = sample(static_cast<int>(zc.minR) - 1, zc.minC);
                n2 = sample(static_cast<int>(zc.maxR) + 1, zc.maxC);
            }
            for (const int32_t n : {n1, n2}) {
                if (ref < n * 2 && noise * 2 <= n && sigPlusNoise < n * 2) {
                    out |= ZONE_FLAG_NEIGHBOR_NOISE;
                }
            }
        }
        return out;
    }

    /// TZ_GetPeakGripFlag @0x6BA84485：逐峰分类成"握持峰 / 非握持峰"，
    /// 并单独记一位"存在既非握持、又不贴外沿的峰"。
    [[nodiscard]] uint32_t GetPeakGripFlag(const TouchBigFrame& bigFrame,
                                           const ZoneCluster& zc) const noexcept {
        uint32_t out = 0;
        for (size_t i = 0; i < bigFrame.peaks.size(); ++i) {
            const auto& pk = bigFrame.peaks[i];
            if (pk.r >= kRows || pk.c >= kCols) continue;
            if (bigFrame.touchZones[pk.r][pk.c] != zc.id) continue;

            const int distToEdge = std::min({static_cast<int>(pk.c),
                                             kCols - 1 - static_cast<int>(pk.c),
                                             static_cast<int>(pk.r),
                                             kRows - 1 - static_cast<int>(pk.r)});
            const bool inGrip = (bigFrame.gripRegion[pk.r][pk.c] & 0x0400u) != 0;

            if (distToEdge > static_cast<int>(m_gripPeakEdgeDist)) {
                out |= ZONE_FLAG_PEAK_NOT_IN_GRIP;
            } else if (inGrip) {
                out |= ZONE_FLAG_PEAK_IN_GRIP;
            } else {
                // TZ_IsPeakNotInGrip：峰不在握持掩码内即判非握持。
                out |= ZONE_FLAG_PEAK_NOT_IN_GRIP;
            }

            // Peak_IsOnOuterEdge：最外一圈。
            const bool onOuterEdge = (pk.c == 0 || pk.c == kCols - 1 ||
                                      pk.r == 0 || pk.r == kRows - 1);
            if (!inGrip && !onOuterEdge) {
                out |= ZONE_FLAG_INNER_NON_GRIP_PK;
            }
        }
        return out;
    }

    /// TZ_CheckToSeparateZone @0x6BA855B7
    /// 两条结论：上一帧已被分离且本帧仍有非握持峰 ⇒ 强制合并（0x10000）；
    /// 同时存在握持峰与非握持峰且强制合并或 TZ_IsToSeparateZone 成立 ⇒ 分离（0x1000000）。
    /// TZ_IsGripMergeAllowed / TZ_IsToSeparateZone 都建立在 GripFilter 的区域级
    /// 握持结论上，而本机 dwGripFlags 整字为 0、握持通道整体关闭 ⇒ 两者恒假。
    /// 因此本机只留下"上一帧分离 → 本帧继续分离"这一条自持路径，而它没有起点，
    /// 整条链在 W273 上惰性。结构保留，删掉会让人误以为原版没有这条。
    [[nodiscard]] uint32_t CheckToSeparateZone(uint32_t flagsSoFar) const noexcept {
        constexpr bool kGripMergeAllowed = false;   // TZ_IsGripMergeAllowed
        constexpr bool kIsToSeparateZone = false;   // TZ_IsToSeparateZone
        if (!kGripMergeAllowed) return 0;

        uint32_t out = 0;
        bool forced = false;
        if (m_prevAnyZoneSeparated && (flagsSoFar & ZONE_FLAG_PEAK_NOT_IN_GRIP) != 0) {
            out |= ZONE_FLAG_GRIP_MERGE_FORCED;
            forced = true;
        }
        if ((flagsSoFar & ZONE_FLAG_PEAK_IN_GRIP) != 0 &&
            (flagsSoFar & ZONE_FLAG_PEAK_NOT_IN_GRIP) != 0 &&
            (forced || kIsToSeparateZone)) {
            out |= ZONE_FLAG_ZONE_SEPARATED;
        }
        return out;
    }

    /// TZ_MultiFatFingerCheck @0x6BA836A1（只在 type == 3 时跑）
    [[nodiscard]] uint32_t MultiFatFingerCheck(const ZoneCluster& zc) const noexcept {
        // (1) 重心落在握持区且上一帧对应触点不是握持点。ZONE_FLAG_CENTROID_GRIP
        //     的产出方 TZ_CentroidGripRatio 挂在 dwGripFlags 上 ⇒ 本机恒 0。
        if ((zc.zoneFlags & ZONE_FLAG_CENTROID_GRIP) != 0) {
            return ZONE_FLAG_MULTI_FAT_FINGER;
        }
        // (2) 原版：`DAT_6bbc8684 > 10 && SubTZ_GetNumAftFilter() > 1`，
        //     即「**跨 TZ 累积的最大子区 coreCount** > 10 且过滤后存活子区数 > 1」。
        //     不是本区域的 pixelCount，也不是「是否由拆分产生」（差异 04-B）。
        if (zc.maxSubCoreCount > 10 && zc.subKeptNum > 1) {
            return ZONE_FLAG_MULTI_FAT_FINGER;
        }
        // (3) 原版第三块由区域标志的**符号位**放行，该位无任何写入方 ⇒ 惰性。
        return 0;
    }

    /// TZ_IsLO @0x6BA83474：大物体判定。
    ///
    /// 关键：它**不看 TZ 自身的轴**，而是看 `g_subTZ[DAT_6bbc8686]`（面积最大
    /// 的那个子区，跨 TZ 累积）的长短轴 —— dwGripFlags & 0x10 实测为 0，
    /// **清零分支才是生效分支**（文档 §9 的 GRIP_FLAG_0x10 属 Kind::EitherOr，
    /// 两支都不是死枝，必须实现清零支）。置位支用 unit[0x2EC]·unit[0x2EE]。
    ///
    /// 本机可达性（§6.8）：dwFeatureFlags bit20 = 0 ⇒ TZ_SSLoProcess 整帧不执行，
    /// g_isSSLo / g_isSSLoSD 恒 0 ⇒ 门 G0 退化为 `unit[0x350] < 9`、ssBig 快速
    /// 判定（701 / 501 mm²）恒假，只剩「面积 > DB_LOSize」这一条通路。
    [[nodiscard]] bool IsLargeObject(const ZoneCluster& zc) const noexcept {
        constexpr bool kIsSSLo   = false;   // g_isSSLo   —— dwFeatureFlags bit20 = 0
        constexpr bool kIsSSLoSD = false;   // g_isSSLoSD —— 同上

        // GRIP_FLAG_0x10 是 Kind::EitherOr：**关闭支才是完整分支**，不可当死枝删。
        const uint32_t area =
            Gates::IsOn("GRIP_FLAG_0x10")
                ? static_cast<uint32_t>(zc.axisMinorMm) * static_cast<uint32_t>(zc.axisMajorMm)
                : static_cast<uint32_t>(zc.maxSubMinorMm) * static_cast<uint32_t>(zc.maxSubMajorMm);

        // 门 G0：!(exc < 9 || g_isSSLoSD || g_isSSLo) ⇒ 直接否决
        if (!(zc.exceptionState < 9u || kIsSSLoSD || kIsSSLo)) return false;

        // ssBig 快速判定（本机恒假，结构保留）
        const bool ssBig = (kIsSSLoSD && zc.exceptionState >= 8u) ||
                           (kIsSSLo && zc.exceptionState <= 7u);
        if (ssBig && (area >= 701u || (m_framePeakCount >= 6 && area >= 501u))) return true;

        // DB_LOSize @0x6BA55E20：flash[0x836]² = 900 mm²；
        // 上一帧有被握持通道**拒报**的触点（prevHdr+0x03 != 0）时收紧到 675。
        uint32_t loSize = static_cast<uint32_t>(m_largeObjectSizeMm) *
                          static_cast<uint32_t>(m_largeObjectSizeMm);
        if (m_prevGripRejectedCount != 0) loSize = static_cast<uint32_t>(loSize * 3u) >> 2;

        if (loSize < area) {
            // 「紧凑」判据读的同样是最大子区的长轴，不是 TZ 的长轴
            if (zc.maxSubMajorMm <= m_largeObjectSizeMm) return true;
            // TZ_GetShapeRatio 产出的 unit[0x2F5]：手指 / 掌根形态下量级都在 10
            // 以上，`> 5` 在本机近似恒真 ⇒ 回退判定实际退化为「面积 > 900」。
            if (zc.shapeRatio > 5) return true;
        }
        return false;
    }

    /// TZ_GetType @0x6BA837F8：区域类型判决树，逐分支照搬。
    [[nodiscard]] int32_t GetZoneType(const TouchBigFrame& bigFrame, const ZoneCluster& zc,
                                      uint8_t zonePeakCount) const noexcept {
        if (IsLargeObject(zc)) return ZoneTypeLargeObject;

        // dwGripFlags & 1 本机为 0；ZONE_FLAG_GRIP_ZONE 的产出方 GripFilter_RegionProcess
        // 同样挂在 dwGripFlags 上 ⇒ 这里恒走 else 分支，不会退化成 type 1。
        if ((zc.zoneFlags & ZONE_FLAG_GRIP_ZONE) != 0 &&
            (zc.zoneFlags & ZONE_FLAG_ABS_GRIP) == 0) {
            return ZoneTypeGrip;
        }

        if (zonePeakCount < 2) {
            // TZ_GetPrevShape(unit[0x2b9])：本区域第一个峰的上一帧形态。
            const int32_t prevShape = GetPrevShape(FirstZonePeakId(bigFrame, zc));
            if (prevShape == 2 || (zc.zoneFlags & ZONE_FLAG_SHAPE_HINT) != 0) {
                return ZoneTypeShapeFatFinger;
            }
            if (prevShape == 3) return ZoneTypeMultiFinger;
            return JudgeFatFingerOrNormal(zc);
        }

        if ((zc.zoneFlags & ZONE_FLAG_GRIP_MERGE_FORCED) != 0) return ZoneTypeShapeFatFinger;
        if ((zc.zoneFlags & ZONE_FLAG_ZONE_SEPARATED) != 0) return ZoneTypeMultiFinger;
        // 原版还有一个 0x10000000 分支 ⇒ type 2。该位在整个 DLL 里只有
        // TZ_CheckToSeparateZone 一个写入点（与 0x10000 同一处），而那条路径
        // 本机不可达，保留判据本身。
        if ((zc.zoneFlags & 0x10000000u) != 0) return ZoneTypeShapeFatFinger;

        // 绝对握持 / 形态提示旁路
        if ((zc.zoneFlags & ZONE_FLAG_ABS_GRIP) != 0 &&
            (zc.zoneFlags & ZONE_FLAG_SHAPE_HINT) == 0 &&
            !IsAbsFatFingerMinorAxis(zc.axisMinorMm)) {
            return ZoneTypeMultiFinger;
        }

        // 上一帧就是形态胖指（prev[+0x1E0] == 2）、且本帧重心信号没有明显长起来
        // ⇒ 继续判形态胖指。unit[0x324] == 0x14 时没有前帧触点，整条判据落空。
        const int32_t prevZoneType = GetPrevShape(FirstZonePeakId(bigFrame, zc));
        const bool keepShapeFF = (zc.axisMajorMm >= 0x14) && (prevZoneType == 2) &&
                                 (zc.zoneFlags & ZONE_FLAG_ABS_GRIP) == 0 &&
                                 (static_cast<int32_t>(m_dynamicGain) * 2 >
                                  static_cast<int32_t>(zc.centSig) * 3);
        if (keepShapeFF) return ZoneTypeShapeFatFinger;

        if ((zc.zoneFlags & ZONE_FLAG_VIRTUAL_PEAK) != 0) return ZoneTypeMultiFinger;

        // 原版这里还有一条：PosnSpecial_GetPosMarker() >= 2 且
        // peakSig[max]/2 < DAT_6bcac4f8 ⇒ type 3。PosMarker 统计的是「正差分侧
        // 被抽空并切换到负差分侧」的次数，MS 手指路径上一个连通域只会切一次
        // ⇒ 恒为 1 < 2，该支不可达，故恒落到 SubTZ_IsMF 分支。

        // SubTZ_IsMF()：真值在 TrySplitZone 里算并落在 fromMultiFinger 上。
        if (zc.fromMultiFinger) return ZoneTypeMultiFinger;

        // TZ_DFSProcess @0x6BA832D9
        return DFSProcess(bigFrame, zc, zonePeakCount);
    }

    /// 本区域第一个峰（unit[0x2b9]）继承的 Tracking ID。
    [[nodiscard]] static uint8_t FirstZonePeakId(const TouchBigFrame& bigFrame,
                                                 const ZoneCluster& zc) noexcept {
        for (size_t i = 0; i < bigFrame.peaks.size(); ++i) {
            const auto& pk = bigFrame.peaks[i];
            if (pk.r >= kRows || pk.c >= kCols) continue;
            if (bigFrame.touchZones[pk.r][pk.c] == zc.id) return pk.id;
        }
        return 0;
    }

    /// TZ_DFSProcess @0x6BA832D9 的完整出口：
    ///   TSAStatic_IsAllFreqNoisy() → 2；
    ///   最强峰幅 < wPeakMode3ThresholdMax（= 600）→ 2；
    ///   TZ_GetDBShape 非 0 → 除非 TZ_IsAbsFatFinger 另判 2，否则直接采纳；
    ///   为 0 → TZ_JudgeFFOrMF @0x6BA8D12D。
    /// （其中 `前帧轨迹数 < 本帧峰数` 时先跑一次 TZ_PeakFilter，那是峰表整形，
    ///   复刻侧峰表由模块 03 定稿，此处不重入。）
    [[nodiscard]] int32_t DFSProcess(const TouchBigFrame& bigFrame, const ZoneCluster& zc,
                                     uint8_t zonePeakCount) const noexcept {
        constexpr bool kAllFreqNoisy = false;   // TSAStatic_IsAllFreqNoisy：频点管理未复刻
        if (kAllFreqNoisy) return ZoneTypeShapeFatFinger;

        const int32_t strongestPeakSig =
            (zc.peakIndex < bigFrame.peaks.size())
                ? static_cast<int32_t>(bigFrame.peaks[zc.peakIndex].z)
                : 0;
        if (strongestPeakSig < m_peakMode3ThresholdMax) {
            return ZoneTypeShapeFatFinger;
        }

        const int32_t db = GetDBShape(bigFrame, zc, zonePeakCount);
        if (db != 0) {
            if (IsAbsFatFinger(zc)) return ZoneTypeShapeFatFinger;
            return db;
        }
        return JudgeFatFingerOrMulti(zc);
    }

    /// TZ_GetDBShape @0x6BA8A24F：按区域内每个峰回查上一帧轨迹的形态做投票。
    ///   ① cnt[NF] == peakCnt 且 peakCnt == 上一帧上报数            → 3
    ///   ② cnt[MF] != 0 且 cnt[NF] + cnt[MF] == peakCnt 且 maxAge >= 5 → 3
    ///   ③ cnt[未知] + 1 == peakCnt 且 cnt[FF] + cnt[NF] == 1 且 maxAge > 4 → 2
    ///   其余 → 0（无结论）
    ///
    /// 规则 ① 看起来反直觉——「上一帧全是普通手指」却判 MF——实际含义是
    /// 「这些峰各自都有独立轨迹，且轨迹总数与本区峰数相等」，即本 TZ 是多根
    /// **已分开的手指被重新粘到一起，必须拆**。这正是「两指并拢合并成一个
    /// 触点」症状的另一条跨帧防线（差异 04-13）。
    ///
    /// 首帧特例（复刻要点 16）：所有峰的前世形态都是 0 ⇒ 三条规则全部落空。
    [[nodiscard]] int32_t GetDBShape(const TouchBigFrame& bigFrame, const ZoneCluster& zc,
                                     uint8_t zonePeakCount) const noexcept {
        uint32_t cntNf = 0, cntFf = 0, cntMf = 0, cntUnknown = 0;
        uint16_t maxAge = 0;
        uint32_t seen = 0;
        for (size_t i = 0; i < bigFrame.peaks.size(); ++i) {
            const auto& pk = bigFrame.peaks[i];
            if (pk.r >= kRows || pk.c >= kCols) continue;
            if (bigFrame.touchZones[pk.r][pk.c] != zc.id) continue;
            ++seen;
            const int32_t  s = GetPrevShape(pk.id);
            const uint16_t a = GetPrevAge(pk.id);
            if (maxAge < a) maxAge = a;
            if (s == 1)      ++cntNf;
            else if (s == 2) ++cntFf;
            else if (s == 3) ++cntMf;
            else             ++cntUnknown;
        }
        (void)seen;
        const uint32_t peakCnt = zonePeakCount;

        if (cntNf == peakCnt && peakCnt == m_prevTouchCount) return 3;
        if (cntMf != 0 && cntNf + cntMf == peakCnt && maxAge >= 5) return 3;
        if (cntUnknown + 1 == peakCnt && cntFf + cntNf == 1 && maxAge > 4) return 2;
        return 0;
    }

    /// TZ_IsAbsFatFinger @0x6BA8CExx：绝对胖指旁路，判据同 TZ_IsAbsFFMinorAxis
    /// 的门限块（Ghidra 查不到写入方的整定块，见下）。
    [[nodiscard]] static bool IsAbsFatFinger(const ZoneCluster& zc) noexcept {
        return IsAbsFatFingerMinorAxis(zc.axisMinorMm);
    }

    /// TZ_JudgeFFOrMF @0x6BA8D12D：依次尝试五个判据定 2 或 3。
    ///   ① unit[0x350] > 8 → 2（异常态一律当成一整块胖指，不拆）
    ///   ②③④ 短轴 / 中心传感器 / 长轴
    ///   ⑤ 鞍部比
    [[nodiscard]] int32_t JudgeFatFingerOrMulti(const ZoneCluster& zc) const noexcept {
        if (zc.exceptionState > ZONE_EXC_DEBOUNCED) return ZoneTypeShapeFatFinger;
        if (IsAbsFatFingerMinorAxis(zc.axisMinorMm)) return ZoneTypeShapeFatFinger;
        if (zc.axisMajorMm > m_largeObjectSizeMm) return ZoneTypeMultiFinger;
        return ZoneTypeShapeFatFinger;
    }

    /// TZ_JudgeFFOrNF @0x6BA8D1ED（全函数就一句）：
    ///     `return (DAT_6bbc8684 > 9) ? 2 /* FF */ : 1 /* NF */;`
    /// 输入是**跨 TZ 累积的最大子区 coreCount**，不是本区域的 pixelCount。
    /// 同一帧里前一个 TZ 的大子区会把后一个单峰 TZ 也推成 FF —— 原厂行为，
    /// 复刻照抄（要点 17）。返回值 1 / 2 与 ZoneTypeGrip / ZoneTypeShapeFatFinger
    /// 的枚举数值一致（TouchZoneType：Grip = 1、ShapeFatFinger = 2），
    /// 枚举名 Grip 在这里的语义其实是 NF「普通手指」——原版 type 1 同时承担
    /// 「普通手指」与 dwGripFlags&1 短路出口两种来源，值相同（差异 04-05，
    /// 已核实数值无差异，仅命名易误读）。
    [[nodiscard]] static int32_t JudgeFatFingerOrNormal(const ZoneCluster& zc) noexcept {
        return (zc.maxSubCoreCount > 9) ? ZoneTypeShapeFatFinger : ZoneTypeGrip;
    }

    /// TZ_IsAbsFFMinorAxis @0x6BA8CE81：短轴达到绝对胖指门限。该门限取自
    /// 同一批 Ghidra 查不到写入方的整定块，按 0 处理即"短轴非零就算"，
    /// 这里保守地要求短轴至少 1 个节点宽。
    [[nodiscard]] static constexpr bool IsAbsFatFingerMinorAxis(uint8_t minorMm) noexcept {
        return minorMm >= 20;
    }

    // ── 异常通道：FIException_ProcessTouchZone @0x6BA59BDB ─────────────
    // 原版在 dwPeakProcessingMode == 3（本机）时走 FI 分支。要点：
    //   · FIException_CheckPanelSD @0x6BA593BC 与
    //     FIException_CheckChargerNoiseInRxLines @0x6BA59429 在 TSACore 里都是
    //     **空函数体**，FI 模式下没有任何东西写全局原始异常码；
    //     Exception_UpdatePrevException 在 mode 3 下只剩 BLReset_Status() == 1 时
    //     置 0x200 这一条真路径。也就是说全局异常状态机在本机几乎是空转的。
    //   · 因此 zone + 0x350 在 W273 上的可达取值只有
    //     0（正常）/ 0x80（湿手指）/ 0x200（区域基线异常）/ 0x8（去抖收敛值）。
    //     上报网闸判据 4 要的 0x40（手掌）只由 HI 分支产出 ⇒ 本机不可达。
    [[nodiscard]] uint32_t FIExceptionProcessTouchZone(const TouchBigFrame& bigFrame,
                                                       const ZoneCluster& zc,
                                                       uint8_t zonePeakCount) const noexcept {
        // 全局原始异常码为 0x100（充电器噪声）时整区直接标记。FI 分支的检测器是
        // 空壳，该码在本机起不来，判据保留。
        constexpr uint32_t kGlobalRawException = ZONE_EXC_NONE;
        if (kGlobalRawException == ZONE_EXC_CHARGER_NOISE) {
            return ZONE_EXC_CHARGER_NOISE;
        }

        uint32_t exc = ZONE_EXC_NONE;
        if (CheckAbnormalTouchZoneBaseline(bigFrame, zc, zonePeakCount)) {
            exc |= ZONE_EXC_ABNORMAL_BASELINE;
        } else if (CheckWetFinger(zc)) {
            exc |= ZONE_EXC_WET_FINGER;
        }
        return DebounceTouchZone(exc);
    }

    /// FIException_CheckAbnormalTouchZoneBaseline @0x6BA595EE
    /// 触发条件：本区域吃下了**全帧所有的峰**，并且异常比 > 200。
    /// 异常比 = TZ_GetAbsRatio(zone + 0x28, zone + 0x18)，即 |(x << 8) / 正信号和|
    /// 钳在 2000 以内。zone + 0x28 的写入方在 Ghidra 里没有定位到，按语义
    /// （"基线异常"= 区域里出现了与正信号同量级的负信号）取区域内负 Dif 的绝对和。
    [[nodiscard]] bool CheckAbnormalTouchZoneBaseline(const TouchBigFrame& bigFrame,
                                                       const ZoneCluster& zc,
                                                       uint8_t zonePeakCount) const noexcept {
        if (zonePeakCount != static_cast<uint8_t>(bigFrame.peaks.size())) return false;
        int32_t negSum = 0;
        for (int r = zc.minR; r <= zc.maxR; ++r) {
            for (int c = zc.minC; c <= zc.maxC; ++c) {
                if (bigFrame.touchZones[r][c] != zc.id) continue;
                const int16_t v = bigFrame.diff[r][c];
                if (v < 0) negSum -= v;
            }
        }
        return AbsRatio(negSum, zc.sigSum) > 200;
    }

    /// TZ_GetAbsRatio @0x6BA823AD
    [[nodiscard]] static int32_t AbsRatio(int32_t a, int32_t b) noexcept {
        if (b == 0) return 2000;
        const int32_t v = std::abs((a << 8) / b);
        return std::min(v, 2000);
    }

    /// FIException_CheckWetFinger @0x6BA59919
    /// 原版沿**上一帧同 ID 触点的 16 深位置历史队列**（prevTouch + 0x474..0x486）
    /// 逐点做水膜泛洪（FIException_GetWaterSize → FIException_IsAroundSmall），
    /// 统计落在 [峰值×low%, 峰值×high%] 窗口内、且周围没有更强信号的格子数，
    /// 再与 DAT_6bca933c 比较。复刻侧没有维护该位置历史队列 ⇒ 队列为空、
    /// 水膜格数恒 0；而 DAT_6bca933c 同样取不到（位于 Ghidra 查不到写入方的
    /// 整定块）。两端都缺 ⇒ 结构保留、判决短路。
    [[nodiscard]] bool CheckWetFinger(const ZoneCluster& zc) const noexcept {
        constexpr uint16_t kWaterSize = 0;   // 无位置历史队列 ⇒ 泛洪不出格子
        if (kWaterSize < m_wetFingerMinWaterSize) return false;
        // 长短轴比 < 2 的区域不算湿手指；贴到物理边框的也不算。
        const uint32_t aspect = (zc.axisMinorMm == 0)
                                    ? 0u
                                    : (static_cast<uint32_t>(zc.axisMajorMm) * 10u) /
                                          static_cast<uint32_t>(zc.axisMinorMm);
        if (aspect < 0x14u) return false;
        if ((zc.zoneFlags & ZONE_FLAG_EDGE_TOUCHED) != 0) return false;
        return true;
    }

    /// FIException_DebounceTouchZone @0x6BA59471
    /// 原版拿"上一帧同 ID 触点的 +0x1ec"与全局前帧异常码一起做三路收敛。
    /// 全局异常码在本机只可能是 0 或 0x200（基线复位），两者都让原版走
    /// `(prevGlobal & 0xc0) == 0` ∧ `prevGlobal != 8` 那一支，该支只在
    /// 区域异常码恰为 2 时才改写 —— 而 FI 分支从不产出 2。剩下唯一生效的是
    /// 尾部那条：连续上报帧数 > 4 且异常非零时收敛成 0x8。
    [[nodiscard]] uint32_t DebounceTouchZone(uint32_t exc) const noexcept {
        if (m_prevTouchCount > 4 && exc != ZONE_EXC_NONE) {
            return ZONE_EXC_DEBOUNCED;
        }
        return exc;
    }

    // ── 泛洪门限 (TZ_GetSigThold @ 0x6BA8A90B) ────────────────────────
    /// 原版是五路分发再做统一钳位：
    ///   thold = min( peakSig, max( <分支值>, DAT_6bc4a910 ) )
    /// 下界 DAT_6bc4a910 来自 TouchThold_GetNoiseThold @0x6BA7CA48。
    /// ⚠ 早先注释断言"本机恒 0"（理由是那几个 DAT 位于未初始化 .bss）——
    /// tsaprmt-table-reader 实测推翻了该前提：flash+0x90C/0x90E/0x910/0x914
    /// = 100/150/200/20，于是下界 = 上一帧强触点数决定的 125 / 187 / 250，
    /// TouchThold_Init 的首帧值是 600。TSAStatic_IsSafeBaselineApplied() 为假
    /// （SafeBaseline 特征位关闭）这一条仍成立，那条 max 分支确实不走。
    /// 这里统一取 Stage 2.5 的 tholds.noiseThold，不再使用任何静态保底常数。
    ///
    /// 五路分支（判据见成员变量注释）：
    ///   1. dwGripFlags & 4      → Flash 0x081C 固定 220        （本机 dwGripFlags = 0，惰性）
    ///   2. dwProcessingState&0x10 → TZ_GetSigTholdForSideTouch  （MS 帧内恒假，惰性）
    ///   3. AFT 模式             → TZ_GetSigTholdForAFT          （运行时掩码恒 0，惰性）
    ///   4. 充电器 / 异常态      → TZ_GetSigTholdForCharger      （真实可达）
    ///   5. 其余                 → TZ_GetSigTholdCommon
    [[nodiscard]] int16_t GetSigThold(int16_t peakSig, bool onOuterEdge, bool isSD,
                                      int16_t noiseThold, bool chargerPlugged) const noexcept {
        int32_t thold;
        if ((m_dwGripFlags & 0x4u) != 0) {
            thold = m_baseThreshold;
        } else if (m_sideTouchProcessing) {
            thold = SigTholdForSideTouch(peakSig);
        } else if (m_aftModeEnabled) {
            thold = SigTholdForAFT(peakSig);
        } else if (chargerPlugged || m_forceChargerSigThold) {
            thold = SigTholdForCharger(peakSig);
        } else {
            thold = SigTholdCommon(peakSig, onOuterEdge, isSD);
        }
        thold = std::max<int32_t>(thold, noiseThold);
        thold = std::min<int32_t>(thold, peakSig);
        return static_cast<int16_t>(thold);
    }

    /// TZ_GetSigTholdCommon：ref = min(peakSig, 3000)，thold = (ratio * ref) >> 7。
    /// 比例选择是「外沿且非 SD 才用 0x81F，否则用 0x81E」——与直觉相反，
    /// 且 dwFeatureFlags bit12 置位时强制走 0x81F。本机两个比例同为 64，
    /// 因此分支只影响结构不影响数值。
    [[nodiscard]] constexpr int32_t SigTholdCommon(int16_t peakSig, bool onOuterEdge,
                                                   bool isSD) const noexcept {
        const int32_t ref = std::min<int32_t>(peakSig, m_refThreshold);
        const bool useEdgeRatio = m_featForceEdgeRatio || (onOuterEdge && !isSD);
        const int32_t ratio = useEdgeRatio ? m_thresholdRatioEdge : m_thresholdRatio;
        // 原版这一处是 `(uint32)(k * ref) >> 7` —— **无符号移位、向下取整**，
        // 是全模块唯一不走向零截断的定点除（复刻要点 7）。
        return Fixed::ShrUnsigned(ratio * ref, 7);
    }

    /// TZ_GetSigTholdForCharger @0x6BA8A8xx：充电器插入时整体切换门限模型。
    /// mode == 3 时 thold = (Flash 0x081F * peakSig) >> 7 —— 注意**不做 3000
    /// 上限钳位**，强信号下门限随峰值线性抬升，比 Common 更严；
    /// 其余 mode 直接取 peakSig / 2。
    [[nodiscard]] constexpr int32_t SigTholdForCharger(int16_t peakSig) const noexcept {
        if (m_peakProcessingMode == 3) {
            return Fixed::ShrTowardZero(m_thresholdRatioEdge * static_cast<int32_t>(peakSig), 7);
        }
        return static_cast<int32_t>(peakSig) / 2;
    }

    /// TZ_GetSigTholdForAFT：ref = min(peakSig, 3000)，按 UDFP 大手指补偿
    /// 取 ref>>1 或 ref>>2。本机 AFT 通道不可达（m_aftModeEnabled 恒假），
    /// 保留实现以便换项目时直接启用。
    [[nodiscard]] constexpr int32_t SigTholdForAFT(int16_t peakSig) const noexcept {
        const int32_t ref = std::min<int32_t>(peakSig, m_refThreshold);
        return m_udfpLargeFingerWorkaround ? (ref >> 1) : (ref >> 2);
    }

    /// TZ_GetSigTholdForSideTouch：ref = min(peakSig, 3000)，thold = ref >> 1。
    /// 本机侧边触控通道不可达（m_sideTouchProcessing 恒假）。
    [[nodiscard]] constexpr int32_t SigTholdForSideTouch(int16_t peakSig) const noexcept {
        return std::min<int32_t>(peakSig, m_refThreshold) >> 1;
    }

    /// TZ_IsSD @0x6BA8A67E：仅用于 TZ_GetSigTholdCommon 的比例选择。
    ///   nSPeakPositive * 3 > wPeakFilterReferenceThreshold * 2 ⇒ 非 SD；
    ///   上一帧触点数 == 8                                      ⇒ SD；
    ///   其余需读上一帧触点的 +0x228 / +0x1e0 / +0x1f4 / +0x1ba 字段，
    ///   复刻 Tracking 未导出这些量，按原版的默认出口取假。
    [[nodiscard]] bool IsSD(int16_t framePeakPositive) const noexcept {
        if (static_cast<int32_t>(framePeakPositive) * 3 > m_refThreshold * 2) return false;
        return m_prevTouchCount == 8;
    }

    // ── 分类门限 (TZ_GetEdgeSigThold @ 0x6BA8xxxx) ────────────────────
    /// thold = max(baseThold, min(全屏正峰 / 4, peakSig / 2))
    /// 注意参考量是**当前帧的全屏最大正信号**，不是常量。
    /// 原版：`e = nSPeakPositive; if (e < 0) e += 3; e >>= 2;` —— 向零截断的 /4。
    /// 正常工况下 thold = peakSig/2，min(...) <= peakSig/2 = thold ⇒
    /// **edgeThold 恒等于 thold**，class 4 与 class 1 判据重合；只有
    /// peakSig > 3000（thold 被封顶 1500）或走 Charger/AFT/SideTouch 分支才分离。
    [[nodiscard]] static constexpr int16_t GetEdgeSigThold(int16_t peakSig, int16_t baseThold,
                                                           int16_t framePeakPositive) noexcept {
        int32_t thold = Fixed::ShrTowardZero(framePeakPositive, 2);
        const int32_t halfPeak = static_cast<int32_t>(peakSig) / 2;
        if (halfPeak < thold) thold = halfPeak;
        if (thold < baseThold) thold = baseThold;
        return static_cast<int16_t>(thold);
    }

    /// Peak_IsOnOuterEdge：峰值落在最外一圈
    [[nodiscard]] static constexpr bool IsOuterEdgePeak(int r, int c) noexcept {
        return (r == 0 || r == kRows - 1 || c == 0 || c == kCols - 1);
    }

    /// TZ_TraversalNeighbor 的 local_c 分支：最外两列不取绝对值
    [[nodiscard]] static constexpr bool IsEdgeColumn(int c) noexcept {
        return !(c > 1 && c < kCols - 2);
    }

    // trunc7 已统一走 TouchFixed 的 Fixed::ShrTowardZero(v, 7)，
    // 本模块不再手写移位取整（公共约束）。

    /// CTD_BasicCalculation @ 0x6BA551E0：`m0 == 0 ⇒ 0`，否则 `(m1 << 8) / m0 + 0x80`
    /// 并只钳下界 0。原版**没有上界**，也不回退到峰值坐标 —— m1x = Σ c·d 决定了
    /// 商的上限是 `256·(Cols−1) + 128`，钳位够不着，多加一层只会掩盖上游异常。
    [[nodiscard]] static uint16_t CTD_BasicCalculation(int32_t m1, int32_t m0) noexcept {
        if (m0 == 0) return 0;
        int64_t v = ((static_cast<int64_t>(m1) << 8) / m0) + 0x80;
        if (v < 0) v = 0;
        return static_cast<uint16_t>(v);
    }

    /// TZ_RegisterTouch @0x6BA8039B 的两句上钳位：
    ///   `if (DAT_6bcab620 - 1 < zone[0x2ae]) zone[0x2ae] = DAT_6bcab620 - 1;`
    /// DAT_6bcab620 / 0x622 = bCols·256 / bRows·256（TSAConstPrmt_Init）。
    /// 只在登记触点时生效，且是**就地改写 zone**，故重心一经算出即钳。
    [[nodiscard]] static constexpr uint16_t ClampCogDim1(uint16_t v) noexcept {
        constexpr int kMax = kCols * 256 - 1;
        return (static_cast<int>(v) > kMax) ? static_cast<uint16_t>(kMax) : v;
    }

    [[nodiscard]] static constexpr uint16_t ClampCogDim2(uint16_t v) noexcept {
        constexpr int kMax = kRows * 256 - 1;
        return (static_cast<int>(v) > kMax) ? static_cast<uint16_t>(kMax) : v;
    }

    // ── 形态长短轴解算 (TZ_GetAxis / TZ_GetAxisInternal @ 0x6BA7EC90) ──
    /// 输入是核心节点的**无权**几何矩 (TZ_CalcAxisMass)，输出 Q8 节点长度的长短轴与
    /// 长轴倾角。轴长取协方差矩阵特征值的 sqrt(6λ)，与原版逐字对应。
    static void CalcAxis(int64_t n, int64_t sx, int64_t sy, int64_t sxy,
                         int64_t sxx, int64_t syy,
                         uint16_t& majorQ8, uint16_t& minorQ8, int8_t& angleDeg) noexcept {
        const double m0 = (n == 0) ? 1.0 : static_cast<double>(n);
        const double mx = static_cast<double>(sx) / m0;
        const double my = static_cast<double>(sy) / m0;
        const double cxx = static_cast<double>(sxx) / m0 - mx * mx;
        const double cyy = static_cast<double>(syy) / m0 - my * my;
        const double cxy = static_cast<double>(sxy) / m0 - mx * my;

        const double disc = std::sqrt(4.0 * cxy * cxy + (cxx - cyy) * (cxx - cyy));
        const double lMaj = std::max(0.0, (cxx + cyy + disc) * 6.0);
        const double lMin = std::max(0.0, (cxx + cyy - disc) * 6.0);
        const double aMaj = std::sqrt(lMaj);
        const double aMin = std::sqrt(lMin);

        majorQ8 = (aMaj >= 0.5) ? static_cast<uint16_t>(aMaj * 256.0) : uint16_t{0x80};
        minorQ8 = (aMin >= 0.5) ? static_cast<uint16_t>(aMin * 256.0) : uint16_t{0x80};

        const double num = static_cast<double>(sxy) * m0 - static_cast<double>(sx) * static_cast<double>(sy);
        const double den = -((static_cast<double>(syy) - static_cast<double>(sxx)) * m0
                             - static_cast<double>(sy) * static_cast<double>(sy)
                             + static_cast<double>(sx) * static_cast<double>(sx));
        angleDeg = static_cast<int8_t>(static_cast<int>((std::atan2(num + num, den) * 57.29577957855229) / 2.0));
    }

    // ── 边沿能量比 (TZ_UpdateEdgeInfo / TZ_GetGripRatio / TZ_CalcGripRatio) ──
    /// 最外一圈 (Flash 0x90..0x93 = 列 0/59、行 0/39) 与次外一圈的能量分开累加。
    /// 握持时接触主体压在边框外侧，最外圈能量反而高于次外圈，比值因此显著 > 16。
    static void AccumulateEdgeInfo(int r, int c, int16_t sig, bool isCore,
                                   int32_t& edgeSum1, int32_t& innerSum1,
                                   int32_t& edgeSum2, int32_t& innerSum2,
                                   int16_t& edgeMax1, int16_t& edgeMax2) noexcept {
        if (c == 0 || c == kCols - 1) {
            edgeSum1 += sig;
            if (isCore && sig > edgeMax1) edgeMax1 = sig;
        } else if (c == 1 || c == kCols - 2) {
            innerSum1 += sig;
        }
        if (r == 0 || r == kRows - 1) {
            edgeSum2 += sig;
            if (isCore && sig > edgeMax2) edgeMax2 = sig;
        } else if (r == 1 || r == kRows - 2) {
            innerSum2 += sig;
        }
    }

    /// TZ_CentroidGripRatio @0x6BA7F2F0：ratio = (edge << 4) / inner，饱和到 255；
    /// inner 为 0 而 edge 非 0 时直接给满值，edge 为 0（含两者皆 0）给 0。
    /// 除法在原版里走**无符号**，edge 为负时商溢出成满值 —— 最外一列整体为负
    /// 意味着那里全是握持负信号，判成"贴死边框"符合原意，照抄不要改成有符号。
    [[nodiscard]] static uint8_t EdgeRatio(int32_t edge, int32_t inner) noexcept {
        if (edge == 0) return 0;
        if (inner == 0) return 255;
        const uint32_t v = static_cast<uint32_t>(edge * 16) / static_cast<uint32_t>(inner);
        return static_cast<uint8_t>(std::min<uint32_t>(v, 255));
    }

    /// TZ_UpdateGripRatio + TZ_CalcGripRatioDim1OnShortAxis：
    /// 先按包围盒贴哪条边选出有效比值，再用最外圈核心最大信号做一次有效性过滤
    /// （边沿最大值不足峰值一半说明主体并不在边框上，该方向的比值作废）。
    void FinalizeGripRatio(const TouchBigFrame& bigFrame, ZoneCluster& zc,
                           int16_t peakSig) const noexcept {
        zc.gripRatioDim1 = EdgeRatio(zc.edgeSumDim1, zc.innerSumDim1);
        zc.gripRatioDim2 = EdgeRatio(zc.edgeSumDim2, zc.innerSumDim2);

        uint8_t r1 = zc.gripRatioDim1;
        uint8_t r2 = zc.gripRatioDim2;
        if (zc.edgeMaxDim1 <= peakSig / 2) r1 = 0;
        if (zc.edgeMaxDim2 <= peakSig / 2) r2 = 0;

        // 包围盒离两条边都不足 1 格 → 角落，四个方向一律参与
        const bool corner = (zc.minR + zc.minC < 2) ||
                            ((kRows - 1 - zc.maxR) + zc.minC < 2) ||
                            (zc.minR + (kCols - 1 - zc.maxC) < 2) ||
                            ((kRows - zc.maxR) + (kCols - 2 - zc.maxC) < 2);

        // mask 决定哪些方向参与判定：默认 0xC 只看左右两条长边，
        // 转屏时 (dwSystemFlags & 0x80) 放开成 0xF；角落区域四个方向一律参与。
        const uint32_t mask = m_gripRatioDirMask;
        uint8_t flags = 0;
        uint8_t best = 0;
        if (((mask & 0x4) || corner) && zc.minC == 0 && r1 != 0) { flags |= 0x04; best = std::max(best, r1); }
        if (((mask & 0x8) || corner) && zc.maxC == kCols - 1)     { if (r1 != 0) flags |= 0x08; best = std::max(best, r1); }
        if (((mask & 0x1) || corner) && zc.minR == 0)             { if (r2 != 0) flags |= 0x01; best = std::max(best, r2); }
        if (((mask & 0x2) || corner) && zc.maxR == kRows - 1)     { if (r2 != 0) flags |= 0x02; best = std::max(best, r2); }

        zc.gripRatio = best;
        zc.gripEdgeFlags = flags;

        // GripFilter_GetCenterLevel：重心格子上 gripRegion 的最高置位序号 (0..3)
        const int cx = std::clamp<int>(zc.cogX_q8 >> 8, 0, kCols - 1);
        const int cy = std::clamp<int>(zc.cogY_q8 >> 8, 0, kRows - 1);
        zc.gripLevel = GripLevelOf(bigFrame.gripRegion[cy][cx]);
    }

    /// GripFilter_GetGripLevel：取 0..3 中满足 (1 << level) <= value 的最高位序
    [[nodiscard]] static uint8_t GripLevelOf(uint16_t regionValue) noexcept {
        int level = 3;
        while (level >= 0 && regionValue < (1u << level)) --level;
        return static_cast<uint8_t>(level < 0 ? 0 : level);
    }

    // ── SubTZ 二次拆分 (SubTZ_Process @ 0x6BA7E646) ─────────────────────
    static constexpr size_t kMaxSubZones = 8;

    /// 单个子区域的累加器，对位 g_subTZ 的 0x48 字节记录（文档 04 §6.1 表）。
    /// 注意矩累加用的是**原始行列号、不含电极几何偏置、不做 >>7**
    /// （SubTZ_UpdateUnit @0x6BA7D3CD），与主 TZ 质心的定点基准不同（§6.4）。
    struct SubAccum {
        int16_t  maxSig = INT16_MIN;   // +0x00，初值 0x8000
        uint8_t  peakIndex = 0x14;     // +0x02，代表峰索引，0x14 = 无效
        uint16_t coreCount = 0;        // +0x04，超门限格数
        int32_t  m0 = 0;               // +0x0c
        int32_t  m1x = 0, m1y = 0;     // +0x10 / +0x14
        int64_t  m11 = 0, m20 = 0, m02 = 0;   // +0x18 / +0x20 / +0x28
        uint16_t majorMm = 0, minorMm = 0;    // +0x30 / +0x32（SubTZ_GetAxis → TS_PixelToMM）
        uint16_t majorQ8 = 0, minorQ8 = 0;
        int8_t   angleDeg = 0;
        uint8_t  minC = 0xFF, maxC = 0, minR = 0xFF, maxR = 0;  // +0x39..+0x3c
        uint16_t gripRegionMax = 0;    // +0x3d
        uint32_t rejectFlags = 0;      // +0x40，bit0 幅值不足 / bit1 质量不足

        // 复刻侧另记的量：EC / 椭圆率 / 上报所需
        int32_t  sigSum = 0;
        int32_t  edgeSum1 = 0, innerSum1 = 0, edgeSum2 = 0, innerSum2 = 0;
        int16_t  edgeMax1 = 0, edgeMax2 = 0;
        uint16_t haloCount = 0;
        uint8_t  peakR = 0, peakC = 0;
        int16_t  peakSig = 0;
        int32_t  peakZ9 = INT32_MIN;
        uint16_t cogX_q8 = 0, cogY_q8 = 0;
        bool     kept = false;
    };

    /// CTD_General @0x6BA5523C：以 (centerC, centerR) 为中心、宽 width、高 height 窗口求局部重心 (Q8)
    static void CTD_General(const int16_t diff[kSensorRows][kSensorCols],
                            int centerC, int centerR, int width, int height,
                            uint16_t& outX_q8, uint16_t& outY_q8) noexcept {
        const int halfW = width >> 1;
        const int halfH = height >> 1;
        const int minC = std::max(0, centerC - halfW);
        const int maxC = std::min(kCols - 1, centerC + halfW);
        const int minR = std::max(0, centerR - halfH);
        const int maxR = std::min(kRows - 1, centerR + halfH);

        int32_t m1x = 0;
        int32_t m1y = 0;
        int32_t m0  = 0;

        // 行主序遍历。原版这里写成 for c { for r } 的列主序，对行主序存放的
        // 网格是每格一次 cache line miss；求和顺序对结果没有影响（整型加法
        // 可交换，且不涉及回绕语义），换成行主序是纯粹的等价改写。
        for (int r = minR; r <= maxR; ++r) {
            for (int c = minC; c <= maxC; ++c) {
                const int16_t sig = diff[r][c];
                if (sig > 0) {
                    m1x += static_cast<int32_t>(sig) * c;
                    m1y += static_cast<int32_t>(sig) * r;
                    m0  += sig;
                }
            }
        }
        outX_q8 = ClampCogDim1(CTD_BasicCalculation(m1x, m0));
        outY_q8 = ClampCogDim2(CTD_BasicCalculation(m1y, m0));
    }

    /// Peak_GetZ9：Z9 = Z1 + Z8（峰中心 Dif + 8 邻域代数和，不含中心）。
    /// SubTZ_Process @0x6BA7E646 用它选 DAT_6bbc8682「最强子区」——
    /// **不是按 maxSig、也不是按 M0**（文档 §6.5，差异 04-A）。
    /// strongest 同时决定 massGate 与鞍部配对基准，选错会让 CentToMinPeakRatio 整体失真。
    [[nodiscard]] static int32_t PeakZ9(const PeakCandidate& pk) noexcept {
        return static_cast<int32_t>(pk.z) + pk.nZ8;
    }

    /// SubTZ_IsGrip @0x6BA7D9B0：子区触及最外列（minCol == 0 || maxCol == bCols-1）
    /// 且长轴 > 0x13 mm ⇒ **无条件保留**，绕过幅值 / 质量两道门。
    /// 缺了它贴边子区会被误淘汰，keptNum 掉到 1 ⇒ 双指不拆（差异 04-C）。
    [[nodiscard]] static bool SubTzIsGrip(const SubAccum& a) noexcept {
        return ((a.minC == 0) || (static_cast<int>(a.maxC) == kCols - 1)) && (a.majorMm > 0x13);
    }

    /// SubTz_GetSignalThresh @0x6BA7DE91：**全区域共享一个门限**，所有子区在
    /// 同一水平面切割，分水岭不会偏向强峰。
    ///   flags & 2                      → mul7(flash[0x81F], peakSig[minPeakIdx])
    ///   unit[0x350] == 8               → mul7(flash[0x820], peakSig[minPeakIdx])
    ///   最强峰贴最外一圈               → mul7(flash[0x81F], peakSig[minPeakIdx])
    ///   其余                           → mul7(flash[0x81F], peakSig[maxPeakIdx])
    /// 最后 max(th, 父 TZ 门限)。边缘峰改用**最弱峰**作参考，因为边缘峰幅值被
    /// 电容截断失真、不可信。
    /// SD 图切换分支（unit[0x34B] != 0 || unit[0x350] & 0xC8）还要求
    /// SignalDisparity_IsSignalCompensated() ∧ FEAT_SIGNAL_DISPARITY_POST，
    /// 复刻侧没有 g_tsaBufSd 这条通道 ⇒ 结构保留、落到后续分支（§6.2 可达性）。
    [[nodiscard]] int32_t SubTzGetSignalThresh(const ZoneCluster& whole, int16_t baseThold,
                                               int16_t minPkSig, int16_t maxPkSig,
                                               bool maxPkOnOuterEdge) const noexcept {
        int32_t ratio = m_subThresholdRatio;       // flash[0x81F]
        int16_t refSig;
        if ((whole.zoneFlags & ZONE_FLAG_SHAPE_HINT) != 0) {
            refSig = minPkSig;
        } else if (whole.exceptionState == ZONE_EXC_DEBOUNCED) {
            ratio = m_subExcThresholdRatio;        // flash[0x820]，实测同为 64
            refSig = minPkSig;
        } else if (maxPkOnOuterEdge) {
            refSig = minPkSig;
        } else {
            refSig = maxPkSig;
        }
        // mul7(k, s) = (k*s < 0 ? k*s + 0x7F : k*s) >> 7 —— 向零截断（要点 7）
        int32_t th = Fixed::ShrTowardZero(ratio * static_cast<int32_t>(refSig), 7);
        if (th < baseThold) th = baseThold;        // 父 TZ 门限托底
        return th;
    }

    /// SubTZ_Process @0x6BA7E646 + SubTZ_Filter @0x6BA7DA00 + SubTZ_IsMF @0x6BA7EC1D
    /// + TZ_MFProcess @0x6BA80FBD 的复刻合并体。
    ///
    /// 原版对本 TZ 的**每一个峰**各跑一次扩散（不是「峰数 >= 2 才挂载」），
    /// 因为 DAT_6bbc8684 / DAT_6bbc8686 是单峰路径 TZ_JudgeFFOrNF 的唯一输入。
    /// 返回 true 表示已按多指拆分并把多个 ZoneCluster 写入 bigFrame.zones。
    bool TrySplitZone(TouchBigFrame& bigFrame, const ZoneCluster& whole,
                      int16_t wholePeakSig, int16_t baseThold, int16_t edgeSigThold,
                      int32_t gripSubDim1, int32_t gripSubDim2) {
        (void)gripSubDim1;
        (void)gripSubDim2;   // SubTZ_UpdateUnit 不做握持扣减，与主 TZ 不同
        if (!m_subTzEnabled) return false;

        const uint8_t label = whole.id;

        // 1. TZ_PeakInfoRetrieval：落在本 TZ 标签内的峰，按峰索引升序（zone+0x2b9[]）
        uint8_t peakIdx[kMaxSubZones];
        size_t  peakNum = 0;
        for (size_t i = 0; i < bigFrame.peaks.size() && peakNum < kMaxSubZones; ++i) {
            const auto& pk = bigFrame.peaks[i];
            if (pk.isSuppressed) continue;
            if (pk.r >= kRows || pk.c >= kCols) continue;
            if (bigFrame.touchZones[pk.r][pk.c] != label) continue;
            peakIdx[peakNum++] = static_cast<uint8_t>(i);
        }
        if (peakNum == 0) return false;

        // 2. 统一门限
        int16_t maxPkSig = INT16_MIN;
        int16_t minPkSig = INT16_MAX;
        size_t  maxPkSlot = 0;
        for (size_t k = 0; k < peakNum; ++k) {
            const auto& pk = bigFrame.peaks[peakIdx[k]];
            const int16_t sig = bigFrame.diff[pk.r][pk.c];
            if (sig > maxPkSig) { maxPkSig = sig; maxPkSlot = k; }
            if (sig < minPkSig) { minPkSig = sig; }
        }
        const auto& mainPk = bigFrame.peaks[peakIdx[maxPkSlot]];
        const bool mainOnEdge = IsOuterEdgePeak(mainPk.r, mainPk.c);
        const int32_t zoneSubThold =
            SubTzGetSignalThresh(whole, baseThold, minPkSig, maxPkSig, mainOnEdge);

        // unit[0x34B] == 0 恒成立（TZ_CheckFatFingerSD 因 flash[0x608] = 0 恒返回 0，
        // §6.2 判据 1 是硬门）；unit[0x350] & 8 / & 0x80 才是本机真实可达的通路。
        const bool sdLikeMoments = (whole.exceptionState == ZONE_EXC_DEBOUNCED);
        const bool allowOutsideTz = (whole.exceptionState == ZONE_EXC_DEBOUNCED) ||
                                    (whole.exceptionState == ZONE_EXC_WET_FINGER);

        // ★ 不在这里清 m_subClaim / m_subHalo：g_tsaBufSubTZ 是**帧级**认领图，
        //   只在 TZ_PeakBasedProcess 帧初清一次。前一个 TZ 的子区认领会一直
        //   留到帧末，并参与 TZ_PeakBasedProcess 的种子跳过判据
        //   `tz[pos] != 0 || subTz[pos] != 0`（要点 1、12）。

        SubAccum sub[kMaxSubZones];
        size_t   subNum = 0;
        size_t   strongest = 0;                 // DAT_6bbc8682
        int32_t  bestZ9 = INT32_MIN;

        for (size_t k = 0; k < peakNum; ++k) {
            const auto& pk = bigFrame.peaks[peakIdx[k]];
            // SubTZ_Process 的 g_tsaBufSubTZ[peakPos] == 0 判据。若该峰的种子格
            // 已被先前子区的**亚门限扩散**吃掉，就直接跳过、该峰不产生子区。
            // 这是原厂缺陷，**不要「修正」**（§6.4、差异 04-D）。
            if (m_subClaim[pk.r][pk.c] != 0) continue;

            SubAccum& acc = sub[subNum];
            acc = SubAccum{};
            acc.peakR = pk.r;
            acc.peakC = pk.c;
            acc.peakSig = bigFrame.diff[pk.r][pk.c];
            acc.peakIndex = peakIdx[k];
            acc.peakZ9 = PeakZ9(pk);

            // claimId = subNum + 1：DAT_6bbc8979（子标签基）**只在子区域非空时
            // 才 +8**，空扩散会让下一个峰复用同一标签 —— 这里 subNum 不自增，
            // 等价（§6.1）。
            FloodSubZone(bigFrame, label, whole, static_cast<uint8_t>(subNum + 1),
                         static_cast<int16_t>(zoneSubThold), allowOutsideTz, sdLikeMoments, acc);

            if (acc.coreCount == 0) continue;   // 空扩散

            // SubTZ_Centroid / SubTZ_GetAxis
            acc.cogX_q8 = ClampCogDim1(CTD_BasicCalculation(acc.m1x, acc.m0));
            acc.cogY_q8 = ClampCogDim2(CTD_BasicCalculation(acc.m1y, acc.m0));
            CalcAxis(acc.m0, acc.m1x, acc.m1y, acc.m11, acc.m20, acc.m02,
                     acc.majorQ8, acc.minorQ8, acc.angleDeg);
            acc.majorMm = PixelToMM(acc.majorQ8);
            acc.minorMm = PixelToMM(acc.minorQ8);

            // DAT_6bbc8682：按 **Peak_GetZ9 最大** 选最强子区（差异 04-A）
            if (acc.peakZ9 > bestZ9) { bestZ9 = acc.peakZ9; strongest = subNum; }

            // DAT_6bbc8684 / DAT_6bbc8686：最大子区 coreCount 与其下标。
            // **跨 TZ 累积、整帧只在帧首复位**，SubTZ_Process 不按 TZ 清零 ——
            // 同一帧里前一个 TZ 的大子区会把后一个单峰 TZ 也推成 FF。
            // 这是原厂行为，复刻照抄（要点 17、差异 04-B）。
            if (m_maxSubCoreCount < acc.coreCount) {
                m_maxSubCoreCount = acc.coreCount;
                m_maxSubMajorMm = static_cast<uint8_t>(std::min<uint16_t>(255, acc.majorMm));
                m_maxSubMinorMm = static_cast<uint8_t>(std::min<uint16_t>(255, acc.minorMm));
            }

            ++subNum;
            if (subNum >= kMaxSubZones) break;
        }

        // 3. SubTZ_Filter @0x6BA7DA00
        //    keep = (maxSig > wholePeakSig/2 && M0 > massGate) || SubTZ_IsGrip(sub)
        //    massGate = trunc3(g_subTZ[DAT_6bbc8682].M0)
        size_t  keptNum = 0;
        uint8_t saddleRatio = 0;                // DAT_6bbc8683：所有配对中最浅的鞍部
        if (subNum > 0) {
            const int32_t massGate = Fixed::ShrTowardZero(sub[strongest].m0, 3);
            const int32_t sigGate  = wholePeakSig / 2;
            for (size_t i = 0; i < subNum; ++i) {
                const bool keep = ((sub[i].maxSig > sigGate) && (sub[i].m0 > massGate)) ||
                                  SubTzIsGrip(sub[i]);
                sub[i].kept = keep;
                if (keep) {
                    ++keptNum;
                    if (i != strongest) {
                        const uint8_t rr = CentToMinPeakRatio(bigFrame, sub[i], sub[strongest]);
                        if (saddleRatio < rr) saddleRatio = rr;
                    }
                } else {
                    if (sub[i].maxSig <= sigGate)  sub[i].rejectFlags |= 1;
                    if (sub[i].m0     <= massGate) sub[i].rejectFlags |= 2;
                }
            }
        }
        m_subKeptNum = static_cast<uint8_t>(keptNum);   // DAT_6bbc8681

        // 4. SubTZ_IsMF @0x6BA7EC1D
        bool isMf = false;
        if (keptNum > 1) {
            isMf = (saddleRatio < m_subMfRatioThold);          // flash[0x833] = 48 → 75%
            // 跨帧滞回：判据是「上一帧上报数 == 过滤后子区数」且
            // **DAT_6bbc8684（跨 TZ 累积的最大子区 coreCount）> 10**，
            // 不是最强子区的面积（差异 04-B）。双指捏合时鞍部被肉垫填平
            // （ratio >= 48）也不会突然坍缩成单点。
            if (m_prevTouchCount == keptNum && m_maxSubCoreCount > 10) {
                isMf = true;
            }
        }

        if (!isMf) return false;

        // 5. TZ_MFProcess @0x6BA80FBD 的两条出口。
        //    原版判据：!(flags & 0x800) && !(flags & 0x1000000) && unit[0x34B] == 0
        //    → 浅鞍部回退（每峰 3×3 局部质心硬分）；否则 TZ_RegisterBasedOnSubTZ。
        //    flags & 0x800 由 TZ_MultiFatFingerCheck @0x6BA836A1 在
        //    DAT_6bbc8684 > 10 且 SubTZ_GetNumAftFilter() > 1 时置位。
        const bool multiFatFinger = (m_maxSubCoreCount > 10) && (keptNum > 1);
        if (multiFatFinger) {
            if (RegisterBasedOnSubTZ(bigFrame, whole, sub, subNum, strongest,
                                     baseThold, edgeSigThold)) {
                return true;
            }
        }
        return MFProcessFallback(bigFrame, whole, peakIdx, peakNum, baseThold, edgeSigThold);
    }

    /// TZ_RegisterBasedOnSubTZ(unit, 1) @0x6BA8096A：遍历所有 rejectFlags & 3 == 0
    /// 的子区，把子区的 M0 / coreCount / 轴 / Q8 质心 / gripRegion / 代表峰逐个
    /// 覆盖进同一个 unit 后调用 TZ_RegisterTouch。
    bool RegisterBasedOnSubTZ(TouchBigFrame& bigFrame, const ZoneCluster& whole,
                              SubAccum* sub, size_t subNum, size_t strongest,
                              int16_t baseThold, int16_t edgeSigThold) {
        const uint8_t label = whole.id;
        uint8_t newId[kMaxSubZones] = {};
        uint8_t nextId = label;
        for (size_t i = 0; i < subNum; ++i) {
            if (sub[i].kept) newId[i] = nextId++;
        }
        if (nextId == label) return false;      // 一个都没存活 —— 交给回退支

        for (int r = whole.minR; r <= whole.maxR; ++r) {
            for (int c = whole.minC; c <= whole.maxC; ++c) {
                if (bigFrame.touchZones[r][c] != label) continue;
                const uint8_t claim = m_subClaim[r][c];
                size_t si = (claim == 0 || claim > subNum) ? strongest
                                                           : static_cast<size_t>(claim - 1);
                if (!sub[si].kept) si = strongest;
                if (!sub[si].kept) { bigFrame.touchZones[r][c] = 0; continue; }
                bigFrame.touchZones[r][c] = newId[si];
                bigFrame.zoneEdge[r][c] = (m_subHalo[r][c] != 0) ? newId[si] : 0;
            }
        }

        for (size_t i = 0; i < subNum; ++i) {
            if (!sub[i].kept) continue;
            if (bigFrame.zones.size() >= kMaxZoneClusters) break;
            const SubAccum& a = sub[i];

            ZoneCluster zc{};
            zc.id = newId[i];
            zc.peakIndex = a.peakIndex;
            zc.m0 = a.m0;
            zc.m1x = a.m1x;
            zc.m1y = a.m1y;
            zc.cogX_q8 = a.cogX_q8;
            zc.cogY_q8 = a.cogY_q8;
            zc.ecX_q8 = zc.cogX_q8;
            zc.ecY_q8 = zc.cogY_q8;
            zc.minR = a.minR; zc.maxR = a.maxR;
            zc.minC = a.minC; zc.maxC = a.maxC;
            zc.pixelCount = a.coreCount;
            zc.haloCount = a.haloCount;
            zc.sigSum = a.sigSum;
            zc.baseSigThold = baseThold;
            zc.edgeSigThold = edgeSigThold;
            zc.gripRegionMax = a.gripRegionMax;
            zc.isGripWall = whole.isGripWall;
            zc.fromMultiFinger = true;
            zc.gripRatio = whole.gripRatio;
            zc.gripEdgeFlags = whole.gripEdgeFlags;
            zc.edgeSumDim1 = a.edgeSum1;
            zc.innerSumDim1 = a.innerSum1;
            zc.edgeSumDim2 = a.edgeSum2;
            zc.innerSumDim2 = a.innerSum2;
            zc.edgeMaxDim1 = a.edgeMax1;
            zc.edgeMaxDim2 = a.edgeMax2;
            zc.gripRatioDim1 = EdgeRatio(a.edgeSum1, a.innerSum1);
            zc.gripRatioDim2 = EdgeRatio(a.edgeSum2, a.innerSum2);
            zc.ellipseRate = ComputeEllipseRate(bigFrame, zc.id, a.minR, a.maxR, a.minC, a.maxC);
            zc.axisMajorQ8 = a.majorQ8;
            zc.axisMinorQ8 = a.minorQ8;
            zc.axisAngleDeg = a.angleDeg;
            zc.axisMajorMm = static_cast<uint8_t>(std::min<uint16_t>(255, a.majorMm));
            zc.axisMinorMm = static_cast<uint8_t>(std::min<uint16_t>(255, a.minorMm));
            zc.gripLevel = GripLevelOf(
                bigFrame.gripRegion[std::clamp<int>(zc.cogY_q8 >> 8, 0, kRows - 1)]
                                   [std::clamp<int>(zc.cogX_q8 >> 8, 0, kCols - 1)]);
            SnapshotSubTzGlobals(zc);
            ApplyEdgeCompensation(zc.minR, zc.maxR, zc.minC, zc.maxC,
                                  zc.cogX_q8, zc.cogY_q8,
                                  zc.gripRatioDim1, zc.gripRatioDim2,
                                  zc.ecX_q8, zc.ecY_q8);
            zc.edgeDistDim1_q8 = EdgeDistDim1(zc.ecX_q8);
            zc.edgeDistDim2_q8 = EdgeDistDim2(zc.ecY_q8);

            for (int r = zc.minR; r <= zc.maxR; ++r) {
                for (int c = zc.minC; c <= zc.maxC; ++c) {
                    if (bigFrame.touchZones[r][c] != zc.id) continue;
                    if (r == 0 || r == kRows - 1 || c == 0 || c == kCols - 1) {
                        bigFrame.zoneEdge[r][c] = zc.id;
                    }
                }
            }
            bigFrame.zones.push_back(zc);
        }
        return true;
    }

    /// TZ_MFProcess @0x6BA80FBD 的浅鞍部回退支：形态学没能切开，用**每个峰**的
    /// 3×3 局部质心硬分。原版对 zonePeakCount 个峰**全部**执行，没有幅值筛选、
    /// 也没有间距筛选（早期复刻自行加的那两道筛是发明，已删）；正格数与正信号和
    /// 按峰数均分（无符号除），长短轴统一设成 DAT_6bcab6ec、角度清零。
    bool MFProcessFallback(TouchBigFrame& bigFrame, const ZoneCluster& whole,
                           const uint8_t* peakIdx, size_t peakNum,
                           int16_t baseThold, int16_t edgeSigThold) {
        if (peakNum < 2) return false;

        const int32_t  n  = static_cast<int32_t>(peakNum);
        const uint16_t px = static_cast<uint16_t>(whole.pixelCount / peakNum);
        uint8_t nextId = whole.id;

        for (size_t i = 0; i < peakNum; ++i) {
            if (bigFrame.zones.size() >= kMaxZoneClusters) break;
            const auto& pk = bigFrame.peaks[peakIdx[i]];
            uint16_t cogX_q8 = 0, cogY_q8 = 0;
            CTD_General(bigFrame.diff, pk.c, pk.r, 3, 3, cogX_q8, cogY_q8);

            ZoneCluster zc{};
            zc.id = nextId++;
            zc.peakIndex = peakIdx[i];
            zc.m0 = whole.m0 / n;
            zc.pixelCount = px;
            zc.sigSum = whole.sigSum / n;
            zc.cogX_q8 = cogX_q8;
            zc.cogY_q8 = cogY_q8;
            zc.ecX_q8 = cogX_q8;
            zc.ecY_q8 = cogY_q8;
            zc.minR = static_cast<uint8_t>(std::max(0, static_cast<int>(pk.r) - 1));
            zc.maxR = static_cast<uint8_t>(std::min(kRows - 1, static_cast<int>(pk.r) + 1));
            zc.minC = static_cast<uint8_t>(std::max(0, static_cast<int>(pk.c) - 1));
            zc.maxC = static_cast<uint8_t>(std::min(kCols - 1, static_cast<int>(pk.c) + 1));
            zc.baseSigThold = baseThold;
            zc.edgeSigThold = edgeSigThold;
            zc.fromMultiFinger = true;
            zc.gripRegionMax = whole.gripRegionMax;
            zc.isGripWall = whole.isGripWall;
            zc.axisMajorMm = m_mfFallbackAxisMm;
            zc.axisMinorMm = m_mfFallbackAxisMm;
            zc.axisAngleDeg = 0;
            zc.gripRatio = whole.gripRatio;
            zc.gripEdgeFlags = whole.gripEdgeFlags;
            zc.gripRatioDim1 = whole.gripRatioDim1;
            zc.gripRatioDim2 = whole.gripRatioDim2;
            SnapshotSubTzGlobals(zc);
            ApplyEdgeCompensation(zc.minR, zc.maxR, zc.minC, zc.maxC,
                                  zc.cogX_q8, zc.cogY_q8,
                                  whole.gripRatioDim1, whole.gripRatioDim2,
                                  zc.ecX_q8, zc.ecY_q8);
            zc.edgeDistDim1_q8 = EdgeDistDim1(zc.ecX_q8);
            zc.edgeDistDim2_q8 = EdgeDistDim2(zc.ecY_q8);

            for (int r = zc.minR; r <= zc.maxR; ++r) {
                for (int c = zc.minC; c <= zc.maxC; ++c) {
                    if (bigFrame.touchZones[r][c] == whole.id) {
                        bigFrame.touchZones[r][c] = zc.id;
                    }
                }
            }
            bigFrame.zones.push_back(zc);
        }
        return true;
    }

    /// 把 SubTZ 的跨 TZ 累积全局量快照进区域（见 ZoneCluster 的字段注释）。
    void SnapshotSubTzGlobals(ZoneCluster& zc) const noexcept {
        zc.maxSubCoreCount = m_maxSubCoreCount;
        zc.maxSubMajorMm = m_maxSubMajorMm;
        zc.maxSubMinorMm = m_maxSubMinorMm;
        zc.subKeptNum = m_subKeptNum;
    }

    /// SubTZ_TraversalNeighbors @0x6BA7D5A5：与 TZ 相同的 9 偏移 8 连通 DFS，
    /// 容器同样是 PosnSpecial 双向 LIFO 栈。单邻居处理的三个要点：
    ///   1. g_tsaBufSubTZ[nb] != 0 → continue：**只有一张认领图**，
    ///      没有独立的「光晕图」。
    ///   2. 认领写回 g_tsaBufSubTZ[nb] = (subTagBase+1) * sgn(g_tsaBufDif[nb])
    ///      是**无条件执行**的 —— 核心格与亚门限格一律认领，只是亚门限格不入栈、
    ///      （正常工况下）不计矩。两峰之间的鞍部格因此被先到的子区锁死成水坝
    ///      （差异 04-D）。
    ///   3. SubTZ_IsInCurrTZ @0x6BA7D35D：g_tsaBufTz[nb] 必须落在
    ///      [tagBase, tagBase+8) 内，**有符号比较** ⇒ 负差分格的标签是负数，
    ///      一律出局。复刻里 touchZones 不带符号，用 diff < 0 等价替代。
    void FloodSubZone(TouchBigFrame& bigFrame, uint8_t label, const ZoneCluster& whole,
                      uint8_t claimId, int16_t thold, bool allowOutsideTz,
                      bool sdLikeMoments, SubAccum& acc) {
        static constexpr int kDc[9] = {0, 1, 1, 1, 0, -1, -1, -1, 0};
        static constexpr int kDr[9] = {0, -1, 0, 1, 1, 1, 0, -1, -1};

        m_stack.Init();
        m_stack.Put(static_cast<uint16_t>(acc.peakR * kCols + acc.peakC),
                    bigFrame.diff[acc.peakR][acc.peakC] < 0);

        while (!m_stack.IsEmpty()) {
            const uint16_t pos = m_stack.Get();
            const int currR = pos / kCols;
            const int currC = pos % kCols;

            for (int d = 0; d < 9; ++d) {
                const int nr = currR + kDr[d];
                const int nc = currC + kDc[d];
                if (nr < 0 || nr >= kRows || nc < 0 || nc >= kCols) continue;
                if (m_subClaim[nr][nc] != 0) continue;              // 已被认领

                // SubTZ_IsInCurrTZ：不得越出父 TZ 的标签区间（有符号比较）
                if (!allowOutsideTz) {
                    if (bigFrame.touchZones[nr][nc] != label) continue;
                    if (bigFrame.diff[nr][nc] < 0) continue;        // 负标签出局
                    if (nr < whole.minR || nr > whole.maxR ||
                        nc < whole.minC || nc > whole.maxC) continue;
                }

                const int16_t nSig = bigFrame.diff[nr][nc];

                // SubTZ_SetExtraFlag：记 g_tsaBufTzZone、取最大 gripRegion
                acc.gripRegionMax = std::max(acc.gripRegionMax, bigFrame.gripRegion[nr][nc]);

                if (nSig < thold) {
                    // 亚门限格：不入栈；只有 FatFingerSD / 异常态下才计矩
                    if (sdLikeMoments && nSig > 0) {
                        SubTzUpdateUnit(acc, nSig, nc, nr);
                    }
                    m_subHalo[nr][nc] = claimId;
                    ++acc.haloCount;
                } else {
                    ++acc.coreCount;
                    if (acc.maxSig < nSig) acc.maxSig = nSig;
                    SubTzUpdateUnit(acc, nSig, nc, nr);
                    acc.sigSum += nSig;
                    AccumulateEdgeInfo(nr, nc, nSig, true,
                                       acc.edgeSum1, acc.innerSum1,
                                       acc.edgeSum2, acc.innerSum2,
                                       acc.edgeMax1, acc.edgeMax2);
                    if (d != 0) {
                        m_stack.Put(static_cast<uint16_t>(nr * kCols + nc), nSig < 0);
                    }
                }

                // ★ 认领无条件执行（核心与亚门限一律认领）
                m_subClaim[nr][nc] = claimId;
            }
        }
    }

    /// SubTZ_UpdateUnit @0x6BA7D3CD：矩累加与包围盒。用**原始行列号**、
    /// 不含电极几何偏置、不做 >>7 —— 与主 TZ 的 Q7 物理坐标基准不同（§6.4）。
    ///   M0  += s;            M1x += s*x;          M1y += s*y;
    ///   M11 += y*(s*x);      M20 += x*(s*x);      M02 += y*s*y;
    /// 三个 64 位累加器的加数先做 **32 位乘法**再符号扩展，溢出按 32 位截断，
    /// 不得提前提升到 64 位。
    static void SubTzUpdateUnit(SubAccum& a, int16_t s, int c, int r) noexcept {
        const int32_t sx = Fixed::WrapMul32(s, c);
        a.m0  = Fixed::WrapAdd32(a.m0, s);
        a.m1x = Fixed::WrapAdd32(a.m1x, sx);
        a.m1y = Fixed::WrapAdd32(a.m1y, Fixed::WrapMul32(s, r));
        a.m11 += static_cast<int64_t>(Fixed::WrapMul32(r, sx));
        a.m20 += static_cast<int64_t>(Fixed::WrapMul32(c, sx));
        a.m02 += static_cast<int64_t>(Fixed::WrapMul32(Fixed::WrapMul32(r, s), r));
        if (c < a.minC) a.minC = static_cast<uint8_t>(c);
        if (a.maxC < c) a.maxC = static_cast<uint8_t>(c);
        if (r < a.minR) a.minR = static_cast<uint8_t>(r);
        if (a.maxR < r) a.maxR = static_cast<uint8_t>(r);
    }

    /// TZ_GetAutoCentToMinPkRatio @0x6BA82E3C：鞍部信号相对弱峰的比值。
    /// 两峰同行 / 同列时取连线上严格中间格的最小差分，否则取中点 1/2/4 格的均值。
    /// 相邻双峰（间距 1）时 while 体一次都不执行，返回哨兵 0x7FFF ⇒ s >= minPk
    /// ⇒ ratio = 64，即「永远判不出鞍部」，与「网格距离 >= 2 才可分」自洽。
    ///
    /// **原版没有 [0, 64] 钳位**：local_b = (byte)((s << 6) / minPk)。s 为负时
    /// 商为负，截成 byte 后是个大数（如 -10 → 246），DAT_6bbc8683 取最大值后
    /// 远大于 48 ⇒ 判**不是**多指。早期复刻钳到 0 会反过来强行判多指。
    /// 这是原厂行为，照抄不得修正。
    [[nodiscard]] static uint8_t CentToMinPeakRatio(const TouchBigFrame& bigFrame,
                                                    const SubAccum& a, const SubAccum& b) noexcept {
        // pk0Idx / pk1Idx 任一为 0x14（无效）或两者相同 ⇒ 返回 0（视为深鞍部）
        if (a.peakIndex == 0x14 || b.peakIndex == 0x14) return 0;
        if (a.peakIndex == b.peakIndex) return 0;

        const int sumC = static_cast<int>(a.peakC) + static_cast<int>(b.peakC);
        const int sumR = static_cast<int>(a.peakR) + static_cast<int>(b.peakR);
        const int midC = sumC >> 1;
        const int midR = sumR >> 1;

        int32_t sum = 0;
        int     cnt = 0;
        auto sample = [&](int r, int c) {
            if (r < 0 || r >= kRows || c < 0 || c >= kCols) return;
            sum += bigFrame.diff[r][c];
            ++cnt;
        };
        sample(midR, midC);
        if (sumC & 1) sample(midR, midC + 1);
        if (sumR & 1) sample(midR + 1, midC);
        if ((sumC & 1) && (sumR & 1)) sample(midR + 1, midC + 1);
        int32_t cent = (cnt == 0) ? 0 : sum / cnt;

        // TZ_UpdateAveCentSig：共线时改用连线上严格中间格的最小值（含 0x7FFF 哨兵）
        if (a.peakC == b.peakC) {
            const int lo = std::min(a.peakR, b.peakR), hi = std::max(a.peakR, b.peakR);
            cent = 0x7fff;
            for (int rr = lo + 1; rr < hi; ++rr) {
                cent = std::min<int32_t>(cent, bigFrame.diff[rr][a.peakC]);
            }
        } else if (a.peakR == b.peakR) {
            const int lo = std::min(a.peakC, b.peakC), hi = std::max(a.peakC, b.peakC);
            cent = 0x7fff;
            for (int cc = lo + 1; cc < hi; ++cc) {
                cent = std::min<int32_t>(cent, bigFrame.diff[a.peakR][cc]);
            }
        }

        const int32_t minPk = std::min(a.peakSig, b.peakSig);
        if (minPk != 0 && cent < minPk) {
            return static_cast<uint8_t>((cent << 6) / minPk);   // 无钳位，照抄
        }
        return 0x40;
    }

    /// TZ_GetERInernal @ 0x6BA86B64：椭圆率 ×100。
    /// 先由几何矩解出长短轴与倾角，把两轴同时除以 sqrt(6) 还原成 sqrt(λ)，取长短比
    /// 作为各向异性归一化因子；再把每个核心节点旋转到主轴系、按该因子压扁短轴方向，
    /// 得到归一化半径 d。最终 `ER = sqrt(n·Σd² / (Σd)² − 1) × 100`：理想椭圆时
    /// Σd² 与 (Σd)² 成比例、结果趋近 0，形状越不规则（粘连、掌缘）值越大。
    [[nodiscard]] uint16_t ComputeEllipseRate(const TouchBigFrame& bigFrame, uint8_t label,
                                             int minR, int maxR, int minC, int maxC) const noexcept {
        // 轮廓点列表 (TZ_GetContour → TZ_PushEdgeDot，容量 0x96)
        uint8_t dotR[kMaxContourDots];
        uint8_t dotC[kMaxContourDots];
        int     n = 0;
        for (int r = minR; r <= maxR && n < kMaxContourDots; ++r) {
            for (int c = minC; c <= maxC && n < kMaxContourDots; ++c) {
                if (!IsCoreCell(bigFrame, label, r, c)) continue;
                bool onContour = false;
                for (int dr = -1; dr <= 1 && !onContour; ++dr) {
                    for (int dc = -1; dc <= 1; ++dc) {
                        if (dr == 0 && dc == 0) continue;
                        if (!IsCoreCell(bigFrame, label, r + dr, c + dc)) { onContour = true; break; }
                    }
                }
                if (onContour) { dotR[n] = static_cast<uint8_t>(r); dotC[n] = static_cast<uint8_t>(c); ++n; }
            }
        }
        if (n <= 0) return 0;

        // TZ_CalcAxisMass：轮廓点的无权几何矩
        int64_t sx = 0, sy = 0, sxy = 0, sxx = 0, syy = 0;
        for (int i = 0; i < n; ++i) {
            const int64_t gx = dotC[i];
            const int64_t gy = dotR[i];
            sx += gx; sy += gy; sxy += gx * gy; sxx += gx * gx; syy += gy * gy;
        }

        uint16_t majQ8 = 0, minQ8 = 0;
        int8_t   ang = 0;
        CalcAxis(n, sx, sy, sxy, sxx, syy, majQ8, minQ8, ang);

        const int maj = static_cast<int>(static_cast<double>(majQ8) / kSqrt6);
        int       mn  = static_cast<int>(static_cast<double>(minQ8) / kSqrt6);
        if (mn == 0) mn = 1;
        const double ratio = static_cast<double>(maj) / static_cast<double>(mn);

        const double rad = (static_cast<double>(ang) * 3.14159265) / 180.0;
        const double cs = std::cos(rad);
        const double sn = std::sin(rad);
        const double mx = static_cast<double>(sx) / static_cast<double>(n);
        const double my = static_cast<double>(sy) / static_cast<double>(n);

        double sumSq = 0.0;
        double sumLen = 0.0;
        for (int i = 0; i < n; ++i) {
            const double dx = static_cast<double>(dotC[i]) - mx;
            const double dy = static_cast<double>(dotR[i]) - my;
            const double u = dx * cs + dy * sn;
            const double v = (-dx * sn + dy * cs) * ratio;
            const double d2 = u * u + v * v;
            sumSq += d2;
            sumLen += std::sqrt(d2);
        }
        if (sumLen == 0.0) return 0;

        double t = (static_cast<double>(n) * sumSq) / (sumLen * sumLen) - 1.0;
        if (t < 0.0) t = 0.0;
        return static_cast<uint16_t>(std::min(65535.0, std::sqrt(t) * 100.0));
    }

    [[nodiscard]] static bool IsCoreCell(const TouchBigFrame& bf, uint8_t label,
                                         int r, int c) noexcept {
        if (r < 0 || r >= kRows || c < 0 || c >= kCols) return false;
        return bf.touchZones[r][c] == label && bf.zoneEdge[r][c] != label;
    }

    static constexpr int kMaxContourDots = 150;   // TZ_PushEdgeDot 容量 0x96
    static constexpr double kSqrt6 = 2.449489742783178;

    /// TS_PixelToMM @ 0x6BA7C561：mm = round(K * q8 / 65536)，K = 65536 / 60
    [[nodiscard]] static constexpr uint8_t PixelToMM(uint16_t q8) noexcept {
        const uint32_t p = static_cast<uint32_t>(kPixelToMmK) * static_cast<uint32_t>(q8);
        return static_cast<uint8_t>(std::min<uint32_t>(255, (p >> 16) + ((p >> 15) & 1u)));
    }

    static constexpr uint32_t kPixelToMmK = 1092;   // DAT_6BCAB61C: 1 节点 ≈ 4.27 mm

    // ── 握持屏障墙检测 (TZ_IsGripWall @ 0x6BA8xxxx) ────────────────────
    /// 原版语义远比"边框掩码"窄：
    ///   * 仅在 dwGripFlags & 0x200 开启时生效；
    ///   * 仅作用于**最外两列** (c == 0 或 c == cols-1)；
    ///   * 负信号无条件成墙；
    ///   * 否则要求该格与种子的 tzAge 都非零且相差 >= 0xC9 才成墙。
    /// 我们没有 g_tsaBufTzAge 这条年龄图，所以只实现可复现的负信号分支。
    ///
    /// 注意：g_tsaBufGripRegion 在原版 TZ_TraversalNeighbor 里只被取最大值存进
    /// 区域属性 (+0x32a)，**从来不是泛洪的停止条件**。早期把它当阻断墙会让
    /// 贴边触点整片区域泛不开。
    [[nodiscard]] bool IsGripWall(int r, int c, int16_t sig) const noexcept {
        (void)r;
        if (!m_gripWallEnabled) return false;
        if (c != 0 && c != kCols - 1) return false;
        return sig < 0;
    }

    // ── 对数插值查找偏移量 (CTD_ECGetOffset @0x6BA553A6) ──────────────
    /// 自变量是该维的**贴边能量比**（最外一列/行能量和 ÷ 次外一列/行能量和 ×16），
    /// 不是坐标、也不是触点尺寸：手指悬出面板越多，能量越塌向最外一列，比值越大，
    /// 外推量也越大 —— 补偿强度由触发补偿的那个物理原因自己决定。
    ///
    /// 原版**只钳上界**。比值低于 lower 时结果为负，调用方的 compOff = 256 − offset
    /// 因而大于 256，坐标被推回边内，这是"否决本次补偿"的那一档，不能钳成 0。
    ///
    /// 第二个入参（原版 touch + 0x194）用于在整定表里选段，全 DLL 无写入者恒为 0，
    /// 且 W273 四张表的 numSegments 均为 1，分段机制整体退化，故不复刻。
    [[nodiscard]] static constexpr int ECGetOffset(uint8_t ratio, uint8_t lower, uint8_t upper) noexcept {
        const int hi = g_ctd256Ln[upper];
        const int lo = g_ctd256Ln[lower];
        if (hi == lo) return 0;
        const int val = g_ctd256Ln[ratio];
        return std::min((val - lo) * 256 / (hi - lo), 255);
    }

    // ── 平滑过渡样条混合 (CTD_ECGetFinalOffset) ───────────────────────
    // 依据 04_touch_zone_and_centroid.md 4.5.1:
    // d <= 256: d_final = compOff (256 - offset)
    // 256 < d < 320: 线性样条平滑过渡
    // d >= 320: d_final = d
    /// 原版写法是 `i = d - 0x100; if (i < 1) return compOff;` —— **负距离也走
    /// 这一支**，不能像早期复刻那样额外给一个 `d < 0 ⇒ 0` 的出口。
    [[nodiscard]] static constexpr int ECGetFinalOffset(int rawDistQ8, int compOffQ8) noexcept {
        constexpr int kBlendStartQ8 = 256;   // 原版立即数 0x100
        constexpr int kBlendWidthQ8 = 64;    // 原版立即数 0x40
        const int i = rawDistQ8 - kBlendStartQ8;
        if (i < 1) return compOffQ8;
        if (i >= kBlendWidthQ8) return rawDistQ8;
        const int t = i * (256 / kBlendWidthQ8); // 原版就是 i * 4
        int blended = rawDistQ8 * t + compOffQ8 * (256 - t);
        if (blended < 0) blended += 0xff;        // 原版的向零取整补正
        return blended >> 8;
    }

    // ── 单边补偿 (CTD_ECProcessDim1 / Dim2 的一条分支) ─────────────────
    /// farSide = false 取近端原点（左 / 上），距离量为 cog − 0；
    /// farSide = true  取远端原点（右 / 下），距离量为 bound − cog。
    /// 原版直接写 `touch[0x162] = 原点 ± d_final`：既没有强度系数，也**不钳位**。
    [[nodiscard]] static uint16_t CompensateEdge(uint16_t cog_q8, int bound_q8, bool farSide,
                                                 uint8_t ratio, uint8_t lower, uint8_t upper) noexcept {
        const int offset = ECGetOffset(ratio, lower, upper);
        const int compOff = 256 - offset;
        const int rawDist = farSide ? (bound_q8 - static_cast<int>(cog_q8)) : static_cast<int>(cog_q8);
        const int finalDist = ECGetFinalOffset(rawDist, compOff);
        const int coord = farSide ? (bound_q8 - finalDist) : finalDist;
        return static_cast<uint16_t>(coord);
    }

    // ── 边缘补偿总流程 (CTD_ECProcess @0x6BA55CA0) ────────────────────
    /// 进入判据对位 `TZ_CentroidEdgeFlag` @0x6BA7F3CB 置的 +0x204 四个标志位：
    /// 包围盒贴到最外一列/行，**且**重心距该边不足 2 格（Q8: 512）。两维彼此独立，
    /// 每维内部先判远端（右 / 下）再判近端（左 / 上），与原版的 if/else 次序一致。
    ///
    /// `gripRatioDim1 / Dim2` 就是原版 `touch + 0x196 / 0x197`：`TZ_GetGripRatio`
    /// @0x6BA81F82 在区域属性阶段算好落进 `zone + 0x334 / 0x335`，`TZ_RegisterTouch`
    /// 原样拷进触点，`CTD_ECProcess` 稍后读它。复刻据此把比值算在本函数之前。
    void ApplyEdgeCompensation(uint8_t minR, uint8_t maxR, uint8_t minC, uint8_t maxC,
                               uint16_t cogX_q8, uint16_t cogY_q8,
                               uint8_t gripRatioDim1, uint8_t gripRatioDim2,
                               uint16_t& ecX_q8, uint16_t& ecY_q8) const noexcept {
        ecX_q8 = cogX_q8;
        ecY_q8 = cogY_q8;

        constexpr int kDim1Bound = kCols * 256;
        constexpr int kDim2Bound = kRows * 256;
        constexpr int kEdgeGate  = 2 * 256;

        if (maxC == kCols - 1 && (kDim1Bound - cogX_q8) < kEdgeGate) {
            ecX_q8 = CompensateEdge(cogX_q8, kDim1Bound, true,
                                    gripRatioDim1, m_rightEdgeLower, m_rightEdgeUpper);
        } else if (minC == 0 && cogX_q8 < kEdgeGate) {
            ecX_q8 = CompensateEdge(cogX_q8, kDim1Bound, false,
                                    gripRatioDim1, m_leftEdgeLower, m_leftEdgeUpper);
        }

        if (maxR == kRows - 1 && (kDim2Bound - cogY_q8) < kEdgeGate) {
            ecY_q8 = CompensateEdge(cogY_q8, kDim2Bound, true,
                                    gripRatioDim2, m_bottomEdgeLower, m_bottomEdgeUpper);
        } else if (minR == 0 && cogY_q8 < kEdgeGate) {
            ecY_q8 = CompensateEdge(cogY_q8, kDim2Bound, false,
                                    gripRatioDim2, m_topEdgeLower, m_topEdgeUpper);
        }
    }

    // ── 触点距边距离 (Touch_SetEdgeDist @0x6BA72B19) ──────────────────
    /// Touch_GetDim1EdgeDist / Touch_GetDim2EdgeDist：取补偿后重心到该维两条
    /// 边界中较近那条的距离。边界常量是 DAT_6bcab62a..630，即 0 与 Cols·256 /
    /// Rows·256 —— 用"最外一列中心"会把右半边整体少算半格。
    [[nodiscard]] static constexpr uint16_t EdgeDistDim1(uint16_t x_q8) noexcept {
        const int hi = (kCols * 256) - static_cast<int>(x_q8);
        return static_cast<uint16_t>(std::max(0, std::min<int>(x_q8, hi)));
    }

    [[nodiscard]] static constexpr uint16_t EdgeDistDim2(uint16_t y_q8) noexcept {
        const int hi = (kRows * 256) - static_cast<int>(y_q8);
        return static_cast<uint16_t>(std::max(0, std::min<int>(y_q8, hi)));
    }

    // ── ConfigBinder 动态参数绑定接口 ────────────────────────────────
    void registerBindings(Config::ConfigBinder& binder) {
        (void)binder;
    }

    void applyConfig(const Config::ConfigStore& store) {
        (void)store;
    }

private:
    PosnSpecialStack m_stack;
    size_t m_lastZoneCount = 0;

    // ── SubTZ 的跨 TZ 累积全局量（文档 04 复刻要点 17）────────────────────
    // DAT_6bbc8684 / DAT_6bbc8686 / DAT_6bbc8681。**整帧只在帧首复位一次**，
    // SubTZ_Process 不按 TZ 清零 —— 前一个 TZ 的大子区会污染后一个 TZ 的判决。
    // 这是原厂行为，必须保留。
    uint8_t  m_framePeakCount = 0;   // g_pPeaks->bPeakCount，TZ_IsLO 的 ssBig 支要读
    uint16_t m_maxSubCoreCount = 0;
    uint8_t  m_maxSubMajorMm = 0;
    uint8_t  m_maxSubMinorMm = 0;
    uint8_t  m_subKeptNum = 0;

    // ── 跨帧形态继承的历史表（TZ_GetPrevShape @0x6BA8A030 /
    //    TZ_GetPrevAge @0x6BA8A186 的复刻侧替身）──────────────────────────
    // 原版按 peak[+0x04]（该峰继承的 Tracking ID）→ PrevTouch_GetIdxBasedOnID
    // → 历史 +0x1E0（形态）/ +0x156（年龄），且要求历史 +0x1F8 & 8（上一帧真的
    // 上报过）。复刻侧模块 05 的 TrackingSlot 没有导出区域形态字段，跨模块回灌
    // 会把改动扩散到 Tracking，因此本模块自持一张按峰 ID 索引的小表：
    // 每帧末把本帧产出的区域形态写进去，下一帧供 TZ_GetDBShape 查。
    // 语义差异：原版的「上一帧真的上报过」用的是上报网闸结论，这里用
    // 「本帧产出了区域」近似 —— 被下游（模块 06/07）拒报的触点在原版里
    // 会返回形态 0，复刻会返回其形态。已知偏差，记在此处。
    struct PrevShapeEntry {
        uint8_t  id = 0;        // 峰继承的 Tracking ID，0 = 空槽
        int32_t  shape = 0;     // 区域形态 1 NF / 2 FF / 3 MF / 5 LO
        uint16_t age = 0;       // 轨迹年龄（帧）
    };
    std::array<PrevShapeEntry, kMaxZoneClusters> m_prevShapes{};
    size_t m_prevShapeCount = 0;
    // SubTZ 的共享认领图 (g_tsaBufSubTZ) 与外壳标记，按 TZ 包围盒逐区域复用
    uint8_t m_subClaim[kSensorRows][kSensorCols] = {};
    uint8_t m_subHalo[kSensorRows][kSensorCols] = {};
    int16_t m_lastFramePeakPositive = 0; // g_tsaPrpt.nSPeakPositive
};

// 兼容别名
using ZoneExpanderModern = TouchZoneAndCentroid;

} // namespace Solvers::Touch
