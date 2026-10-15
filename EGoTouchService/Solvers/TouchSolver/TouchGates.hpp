#pragma once

// ══════════════════════════════════════════════════════════════════════
// TouchGates —— W273AS2700 (Gaokun Himax CSOT) 的门控单一事实源。
//
// 原厂用配置位/特性位决定每段代码是否执行。这些取值此前散落在各模块的成员
// 初值与注释断言里，没有单一来源，也没有任何机制保证与文档同步 —— 结果是有
// 四条判断反了（见 docs/artifacts/touchsolver-vs-tsacore-gap.md 的 A4）。
//
// 本文件把它们收敛成一张带证据的表。每一项都必须写明**实测出处**。
//
// 最重要的是 Kind 这个分类：门控为 0 **不等于**那段逻辑可以删。
// ══════════════════════════════════════════════════════════════════════

#include <cstdint>

namespace Solvers::Touch::Gates {

/// 门控的处置类型 —— 决定「本机关闭」时复刻该怎么做。
enum class Kind {
    /// 关闭即整段不执行，且不产出任何下游要读的字段。复刻可整段省略。
    DeadBranch,
    /// 二选一：门控为 0 时被选中的恰好是**完整分支**。
    /// 省略会漏写下游字段，是最容易踩的坑。
    EitherOr,
    /// 关闭的只是一条「简化路径」，关闭时走的是完整路径。可省略置位分支。
    Simplification,
    /// 本机开启，必须实现。
    Active,
};

struct Gate {
    const char* name;
    bool        on;       ///< W273AS2700 实测取值
    Kind        kind;
    const char* evidence; ///< 实测出处：表名 + 偏移 + 值，或文档小节
};

// ── 特性位 dwFeatureFlags = 0x01090121 ───────────────────────────────
// = flash+0x50 (0x00000101) | flash+0x54 (0x01090020)，详见 01 §14.2。
inline constexpr uint32_t kFeatureFlags  = 0x01090121u;
inline constexpr uint32_t kFeatureFlags2 = 0x00000120u; // flash+0x58 | +0x5C，01 §14.2
inline constexpr uint32_t kGripFlags     = 0x00000000u; // 主管线全字 0，03 §14

inline constexpr Gate kGateTable[] = {
    // ── 本机开启 ──────────────────────────────────────────────────────
    {"FEAT_GLOVE",           true,  Kind::Active,
     "dwFeatureFlags bit0；门控 flash+0x918 = 0x0E bit2 恒真；06 §5.1"},
    {"FEAT_GRIP",            true,  Kind::Active,
     "dwFeatureFlags bit5"},
    {"FEAT_COORD_FREEZE",    true,  Kind::Active,
     "dwFeatureFlags bit8；全库仅 3 个读点，都在 Filter_ModeTouchReportedProcessUnit；07 §8"},
    {"FEAT_WINDOWS_PAD",     true,  Kind::Active,
     "dwFeatureFlags bit24；GripFilter 的 50 ms 延迟来自它；06 §4"},

    // ── 关闭且可整段省略 ──────────────────────────────────────────────
    {"FEAT_SMART_COVER",     false, Kind::DeadBranch, "dwFeatureFlags bit1"},
    {"FEAT_HIGH_SENSE",      false, Kind::DeadBranch, "dwFeatureFlags bit4"},
    {"FEAT_UNDERWATER",      false, Kind::DeadBranch, "dwFeatureFlags bit11"},
    {"FEAT_SAFE_BASELINE",   false, Kind::DeadBranch, "dwFeatureFlags bit17"},
    {"FEAT_AFT",             false, Kind::DeadBranch, "dwFeatureFlags bit18"},
    {"FEAT_DOUBLE_REPORT",   false, Kind::DeadBranch,
     "dwFeatureFlags bit21；DoubleReport_Scenario 首条判据恒假，整链死；08 §7"},
    {"FEAT_HOTZONE",         false, Kind::DeadBranch,
     "dwFeatureFlags bit22；HotZoneAdjust 全链不执行；07 §12"},
    {"EDGE_PEAK_WORKAROUND", false, Kind::DeadBranch,
     "dwFeatureFlags2 bit0；TSA_SetWorkarounds 不在宿主解析清单，永远 0x120；03 §10"},
    {"SS_SELF_CAPACITANCE",  false, Kind::DeadBranch,
     "flash+0x608 = 0x00；SS_Process 空操作、SignalDisparity 与 ChargerNoiseFilter 全链死；02 §12/§13"},
    {"LIFTOFF_Z_DECAY",      false, Kind::DeadBranch,
     "flash+0x8A2 = 0；Filter_IsTouchLiftOff 与二次 JIT 全关；07 §16"},
    {"POSTURE_MULTISTEP_BACK", false, Kind::DeadBranch,
     "DAT_6bcaa14c 无写入者；姿态回退与多步回退整链关闭；07 §5"},
    {"TSA_FOLD",             false, Kind::DeadBranch,
     "TSAFold_IsSupported() = (flash+0x17CC > 1)，实测 = 1；08 §1"},
    {"UDFP",                 false, Kind::DeadBranch,
     "flash+0x1740 = 0（但 BefUDFPUnit 预筛仍真实删触点）；08 §2"},
    {"TOE_SYNA_BL",          false, Kind::DeadBranch, "flash+0x1750 = 0；01 §14"},
    {"NOTCH_PROTECT",        false, Kind::DeadBranch, "flash+0x1758 = 0；01 §14"},

    // ── 模块 06 的门控总表（文档 06 §1，tsaprmt-table-reader 实测）─────
    {"TM_CFG_WORD_NONZERO",  true,  Kind::Active,
     "DAT_6bcab700 = flash+0x918 = 0x0E；bit1 Finger / bit2 Glove / bit3 厚手套；"
     "早先误判为恒 0，导致 TouchMode 三态被写死成不可达；06 §1/§5.1"},
    {"TM_FINGER_DETECT",     true,  Kind::Active,
     "cfg & 2 ∧ !(feat & 0x1000)；06 §1"},
    {"TM_GLOVE_DETECT",      true,  Kind::Active,
     "cfg & 4 ∧ feat bit0；闩锁是 DAT_6BC4A924 而非 TSAStatic+0x3F1；06 §5.1"},
    {"TM_PRIORITY_SWITCH",   false, Kind::DeadBranch,
     "cfg & 8 ∧ feat bit4 = 0；优先级表固定，且改写值与静态表逐格相同；06 §1"},
    {"TM_STYLUS_DETECT",     false, Kind::DeadBranch,
     "TouchMode_StylusDetect @0x6BA798B8 无判据，恒返回 result = 1；06 §1"},
    // GripFilter_IsMergeHandlerEnabled @0x6BAE18C4 = 四条边的
    //   GripFilter_IsMergeHandlerEdgeEnabled(e) @0x6BAE1763 = flash[0x8D0 + e·0xC] & 2
    // 之或。实测 flash 0x8D0/0x8DC/0x8E8/0x8F4 = 00 00 00 00（bit1 全 0）⇒ 返回 0。
    // 它门控的**只有一处**：GripFilter_Process @0x6BAE1DD6 对
    // TSABuffer_GripStageProcess @0x6BA8EF57 的调用（反汇编
    // `CALL 0x6bae18c4 / TEST AL,AL / JZ 0x6bae1ddb / CALL 0x6ba8ef57`）。
    // 因该函数开头无条件调 TSABuffer_GripTimeProcess、结尾写 GripTimeOff，
    // 三张表 g_tsaBufGripStage / GripTime / GripTimeOff 在本机确实恒 0；
    // 但这是「调用点被跳过」的结果，**不是**这三张表各自另有门控。
    {"GRIP_MERGE_HANDLER",   false, Kind::DeadBranch,
     "GripFilter_IsMergeHandlerEnabled = ⋁ flash[0x8D0+e·0xC] & 2，四边实测全 0；"
     "仅门控 GripFilter_Process → TSABuffer_GripStageProcess 一处调用；06 §1/§4.7"},
    {"GRIP_DYN_DEADZONE",    false, Kind::DeadBranch,
     "flash+0x8C0 = 0 ⇒ GripFilter_IsInDynamicDeadZone 首行返回 0；06 §1"},
    {"GRIP_IDLE",            false, Kind::DeadBranch,
     "flash+0x8C2 = 0 ⇒ GripFilter_IDLEProcess 的 g_isOKToIDLE 恒 false；06 §1"},
    {"GRIP_PAD_MODE",        false, Kind::DeadBranch,
     "flash+0x8C4 = 2（非 Pad）；06 §1"},
    {"GRIP_NEG_SIG",         false, Kind::DeadBranch,
     "flash+0x94A = 0 ⇒ NegSig 旁路整族关闭；06 §1/§4.3"},
    {"UDFP_RELAX",           false, Kind::DeadBranch,
     "flash+0x1741 = 0 ⇒ AntiTouch_IsTouchCloseToUdfpArea 恒假，长轴门限不放宽到 40；06 §1"},
    {"FEAT_PROXIMITY",       false, Kind::DeadBranch,
     "dwFeatureFlags bit14；Proximity_Process 只 memset，TouchPostProcess 不可达；06 §8"},
    {"FEAT_SIDE_TOUCH",      false, Kind::DeadBranch,
     "dwFeatureFlags bit15；06 §8"},
    {"THENAR_BY_TOUCH",      false, Kind::DeadBranch,
     "Thenar_ProcessInfoByTouch / EventUpdateNoDebounce 无调用者，hdr+0x50 恒 0；06 §1"},

    // ── 模块 01 的门控总表（文档 01 §13，tsaprmt-table-reader 实测）─────
    {"BL_TRACK_FROM_PRECMF", true, Kind::EitherOr,
     "flash+0x69 = 0x01 ⇒ BLIIR_DoUpdate @0x6BA47141 的源缓冲取 g_tsaBufPreCMFRaw；"
     "为 0 才取 g_tsaBufRaw。两支都是完整分支；01 §6/§13"},
    {"BL_OK_TO_IDLE_WIDE",   false, Kind::EitherOr,
     "flash+0x44 = 0x00 ⇒ BLSM_IsOKToIDLE @0x6BA4D7FD 只认 Stage 0；"
     "置位时才放宽到 stage ∈ {0,4,6,7}；01 §7.5/§13"},
    {"BL_DIRTY_BASELINE",    true,  Kind::Active,
     "flash+0x6A = 0x01 ⇒ BLReset_IsCurBaselineDirty @0x6BA4AAEE 生效（原因码 0x13）；01 §9/§13"},
    {"BL_ABNORMAL_BASELINE", true,  Kind::Active,
     "flash+0x6B = 0x01 ⇒ BLReset_IsBaselineAbnormal @0x6BA4B506 生效（原因码 0xF）；"
     "**副作用：BLReset_Check @0x6BA48AF0 变死代码**；01 §9/§13"},
    {"RAWDATA_FORCE_NORM",   false, Kind::EitherOr,
     "bRawdataForceNormalize = flash+0x604 = 0x00 ⇒ Rawdata_NormalizeGridUnit "
     "@0x6BA69428 把线上字按 **unsigned 零扩展** 取用（MasterFrameParser 的 "
     "`wire ^ 0x8000` 对位的就是这一支）；置位才按 (short) 解读；01 §4/§13"},
    {"RAWDATA_PROCESS",      false, Kind::DeadBranch,
     "Rawdata_Process @0x6BA69581 在本预置下两条分支都无副作用：bit16=0 支写的 "
     "g_tsaBufRawGrid 全库无读取者（xref 已核实），bit16=1 支的缩放系数 "
     "|gain|/0x3FFF = 16383/16383 恒等。可整段省略；01 §4/复刻清单-10"},
    {"USE_DIF_BUF_PATH",     false, Kind::EitherOr,
     "dwSystemFlags bit16（宿主每帧灌入，非 Flash）。置位时整个基线子系统被旁路、"
     "Dif 由宿主直供；本机清 0 ⇒ 走**完整**基线管线。属不还原的宿主契约；01 §2/§14.1"},
    {"AFE_RAW_UNSTABLE",     false, Kind::Active,
     "static+0xD8 & 0x40000（AFE 帧「raw 失稳」标志）⇒ BLSM_UpdateForRawUnstable "
     "@0x6BA4C27B 的唯一门控。复刻侧无此 AFE 通道 ⇒ 输入缺失而非逻辑缺失："
     "函数体按原版实现（死区 [-16,16] 闭区间、步长 17），门控由 "
     "BaselineInputs::afeRawUnstable 驱动，默认关；01 §9.3"},
    {"BL_STAGE_8_9",         false, Kind::DeadBranch,
     "属性位 0x80 的唯一写入者 BLSM_GetProperty @0x6BA4D523 产出集合是 "
     "{0,1,2,0x10,0x40,0x100,0x200} ⇒ Stage 8 / Stage 9 **无入口**。"
     "不得为了补全而伪造跳转；01 §7.2/复刻清单-9"},

    // ── 模块 02 的门控总表（文档 02 §16，tsaprmt-table-reader 实测）─────
    {"CMF_RAW_DOMAIN",       false, Kind::EitherOr,
     "bCmfEnabled = flash+0x68 = 0x00：置位时走 Raw 域前置 Rawdata_CMF "
     "@0x6BA69381，清零时走 Diff 域后置 CMF_Process @0x6BA4F2C8。"
     "本机取后者，**后置那支才是完整分支**；02 §1"},
    {"CMF_DIFF_DOMAIN",      true,  Kind::Active,
     "bCmfEnabled == 0 的另一侧；CMF_Process 每帧执行；02 §1"},
    {"CMF_BLOCKED",          false, Kind::DeadBranch,
     "TSAStatic+0x11C，唯一 setter 是导出 TSA_SetDisableCMFFlag，"
     "不在 ApDaemon.dll 的解析清单里 ⇒ 恒 0；02 §1"},
    {"CMF_2D_PCA",           false, Kind::DeadBranch,
     "CMF_GetCmfMethod @0x6BA4D903 恒返回 0（flash+0x810 = 0、+0x814 = 2）；"
     "CmfPca 表 4908 B 全 0，强开会触发 GetNeighborCoef 的断言；02 §2"},
    {"CMF_FORCE_DIM1",       true,  Kind::Active,
     "bCmfForceDim1Pass = flash+0x19 = 0x01 ⇒ 锁死 40 个 Tx 行切片；02 §2"},
    {"CMF_COEF_ALT",         false, Kind::Simplification,
     "dwSystemFlags bit18 (0x40000) 由宿主 dwCtrlFlags 每帧灌入，常态未置位 ⇒ "
     "TSADynPrmt_GetCMFCoef @0x6BB3D0BD 取正常组 (600,150)；备用组 "
     "flash+0x626/0x628 = (15000,200)。属不还原的宿主契约；02 §4/§16"},
    {"CMF_WEIGHTED",         false, Kind::DeadBranch,
     "CMF_ProcessCore @0x6BA4E799 硬编码 `CMF_ProcessCoreWeight(buf, 0, ...)`，"
     "第 6 参是 pOutDraft 不是权重 ⇒ 1D 路径的加权聚类与加权仲裁全是死代码；02 §5/§8"},
    {"CMF_SS_ASSIST",        false, Kind::DeadBranch,
     "SS_IsValidForCMF 函数体 `return 0` ⇒ SS_DiffRelatCal / "
     "SS_DiffCMFProcessDimUnit 分支永不执行；02 §4"},
    {"CMF_SMARTCOVER_2ND",   false, Kind::DeadBranch,
     "皮套自适应二次收敛需 FEAT_SMART_COVER ∧ flash+0x6D = 0x00，两者皆 0；02 §8"},
    {"CMF_TOE_EDGE_THOLD",   false, Kind::DeadBranch,
     "TouchThold_ToeUseSpecialThold @0x6BA7CD8F 按工程名匹配 P116/P150/P186，"
     "W273AS2700 不匹配 ⇒ 边缘切片不降到 200；02 §4"},
    {"CMF_GAME_EXTRA",       false, Kind::DeadBranch,
     "flash+0x624 = 0x0000（备用组 +0x62A 同为 0）⇒ "
     "CMF_ProcessDimCommonNoise(1,1200,200,60,40) 永不执行；02 §15"},
    {"CMF_REMAP",            false, Kind::DeadBranch,
     "CMFRemap_IsNeedRemap 要求 numRxCombine≠1 ∨ flash+0x6CD ∨ flash+0x80E；"
     "实测 flash+0x6CC = 1、+0x6CD = 0、+0x80E = 0 ⇒ Enter/Exit 立即返回；02 §10"},
    {"BL_COMP_DISABLED",     false, Kind::DeadBranch,
     "TSAStatic+0x3F0 恒 0（宿主不调 TSA_SetDisableBaselineUpdateFlag）⇒ "
     "全屏基线补偿这道门**永远是开的**，不是「补偿关闭」；02 §9"},
    {"BL_COMP_INVERT",       false, Kind::DeadBranch,
     "TSAStatic+0x11A 镜像 +0x3F1，后者是 0x2000 水下异常哨兵（BLReset_Check "
     "@0x6BA48AF0 每帧在 nTouchCount≠0x2000 时清 0）⇒ 恒 0，补偿方向固定 -step；02 §9"},
    {"UNDERWATER_SENTINEL",  false, Kind::DeadBranch,
     "TSAStatic+0x3F1 恒 0。它**不是**「基线复位进行中」—— 早先把 TouchThold "
     "的门限旁路接到普通基线复位上，会让任何一次复位都把整组门限压到 220；02 §9/§14"},
    {"THOLD_MODE_HYSTERESIS", true, Kind::Active,
     "cfg(flash+0x918)&2 = 1 ∧ mode==Finger ∧ stage==2 ⇒ 峰值门限 600，"
     "否则 220。GetPeakThold 的第一个 if 因手套许可恒真而是死代码；02 §14/§17-9"},
    {"THOLD_GLOVE_LATCH",    true,  Kind::Active,
     "DAT_6BC4A924 运行期闩锁（机制活着，模块 06 已让 Glove 可达）；"
     "普通手指场景它恒 0，*outNoise 基准因此恒 600；02 §14"},
    {"THOLD_HIGH_SENSE_360", false, Kind::DeadBranch,
     "TouchThold_UseHighTholdAtSensitivityMode 按工程名匹配 "
     "P116/P150/P172/P173/P186，本机不匹配 ⇒ Glove 稳态的 360 不可达；02 §14"},
    {"FI_EXCEPTION_STUBS",   false, Kind::DeadBranch,
     "dwPeakProcessingMode = flash+0x34 = 3 ⇒ "
     "FIException_CheckChargerNoiseInRxLines @0x6BA59429 只有一条 ret、"
     "FIException_IsChargerNoiseDetected @0x6BA5942F 恒返回 0、"
     "FIException_CheckPanelSD @0x6BA593BC 空桩；02 §12"},
    {"SS_CHARGER_NOISE_FILTER", false, Kind::DeadBranch,
     "SS_IsChargerNoiseFilterEnabled 要 SS 双维有效 ∧ DAT_6BCAB6B8；"
     "flash+0x608 = 0x00、flash+0x61C = 0x0000，双重否决；02 §12"},
    {"SIGNAL_DISPARITY",     false, Kind::DeadBranch,
     "SignalDisparity_Process @0x6BB35D84 首行要 SS 双维有效 ⇒ 整个函数体不执行；"
     "两个 SwitchBuffer 恒 0 ⇒ **本机永不重跑 Peak**；02 §13"},
    {"GRID_IIR",             true,  Kind::Active,
     "TSAStatic_IsAllFreqNoisy 动态判据，α = flash+0x8F8 低字节 = 224；"
     "充电器插入时 field_0xD 只置不清（粘滞）；02 §11/§17-6"},

    // ── 模块 03 的门控总表（文档 03 §15，tsaprmt-table-reader 实测）─────
    {"PEAK_MODE_3",          true,  Kind::Active,
     "dwPeakProcessingMode = flash+0x34 = 03 00 00 00 ⇒ FI 模式：启用 600/2 = 300 "
     "腰斩、Peak_Z8Filter、SD 判决与 Z1 回退；03 §1/§15"},
    {"PEAK_MODE_2",          false, Kind::DeadBranch,
     "同一字段不可能同时为 2 ⇒ Peak_Process 里 mode==2 的 "
     "SignalDisparity_IsOKToProcess + Peak_Z1Filter(maxPeak/3) 分支永不进入；03 §1"},
    {"PEAK_EXCLUDE_SIDE_REGION", false, Kind::DeadBranch,
     "Peak_IsToExcludeSideRegion @0x6BA685AC 函数体就是 `return 0` ⇒ 扫描范围恒为"
     "整屏 row∈[0,39]、col∈[0,59]；03 §1/§2"},
    {"PEAK_SIDE_SPECIAL_THOLD", false, Kind::DeadBranch,
     "需 Peak_IsToExcludeSideRegion()（恒 0）∧ TouchThold_ToeUseSpecialThold()"
     "（P116/P150/P186，本机不匹配）同时成立 ⇒ 中心区与边列/边行共用同一门限，"
     "DAT_6bcab68a 在 Peak 级永不生效；03 §2"},
    {"PEAK_UDFP_DESC_SORT",  false, Kind::DeadBranch,
     "Peak_needUDFPLargeFingerWorkaround @0x6BA68BC6 首条判据 "
     "strncmp(TSA_GetPrmtName(), \"P115\", 4) == 0，本机预置名 W273AS2700 ⇒ "
     "永远走 Peak_SortInSigAscending（弱峰在前）；03 §1/§8"},
    {"PRESSURE_DRIFT",       true,  Kind::Active,
     "dwFeatureFlags2 bit5（0x120）⇒ 每个通过邻域判决的候选峰都要过 "
     "PressureDrift_Detect @0x6BB441C9：450/225 幅值窗、行梯度 ≤200、"
     "行正和 ≥4.5·v、梯度和 ≤6·v、25 帧/50 次去抖；03 §3/§15"},
    {"PEAK_RERUN_ON_SWITCHBUF", false, Kind::DeadBranch,
     "SS_ChargerNoiseFilterSwitchBuffer() 与 SignalDisparity_SwitchBuffer() 恒 0 "
     "（flash+0x608 = 0）⇒ **本机永不重跑 Peak_Process**。但 "
     "g_peaksBeforeSwitchDifBuf 快照仍被 TZ_BeforeSDCompensationPeakInfoRetrieval "
     "@0x6BA8ADEC **无条件**消费，结构必须保留；03 §11"},
    {"GRIP_REGION_FOLD",     false, Kind::DeadBranch,
     "flash+0x8C4 = 2（直板）≠ 5 ⇒ GripFilter_RegionSetByScreenRotationFold 不执行，"
     "边距不互换；03 §13"},
    {"GRIP_REGION_PALM_EXPAND", false, Kind::DeadBranch,
     "dwFeatureFlags bit15 = 0（0x01090121）⇒ GripFilter_RegionSetByScreenRotation "
     "里的 TypingScenario 掌压扩展区（RegionSet(0x400/0x800, 3|2, 1)）不涂；03 §13"},
    {"GRIP_REGION_HAND_LAYER", true, Kind::Active,
     "GripFilter_RegionSetByHandGesture @0x6BAEC611 无条件执行，对 "
     "g_tsaBufTzPrevHand 非零格 |= 0x08。它是握持掩码**唯一**的跨帧来源；"
     "复刻侧 HandGesture 子管线（DataSwitch_ToHand 通道）未实现 ⇒ 该层恒 0，"
     "与原版「无手掌握持时全 0」一致，属输入缺失而非逻辑缺失；03 §13/§16-10"},

    // ── 关闭 = 简化开关，关闭时走完整路径（可省略置位分支）────────────
    {"GRIP_FLAG_BIT0",       false, Kind::Simplification, "dwGripFlags bit0；04 §9"},
    {"GRIP_FLAG_0x08",       false, Kind::Simplification, "dwGripFlags bit3；04 §9"},

    // ── ⚠ 二选一：关闭时被选中的正是完整分支，**不可当死枝删除** ──────
    {"GRIP_FLAG_0x10",       false, Kind::EitherOr,
     "dwGripFlags bit4：决定 LO 面积来源。关闭 = 走完整来源；03 §14、04 §9"},
    {"GRIP_FLAG_0x20",       false, Kind::EitherOr,
     "dwGripFlags bit5：关闭时才产出 unit[0x350] 等 9 个字段，删掉会漏写；03 §14、04 §9"},

    // ── 关闭但**有真实副作用**，不可简单省略 ──────────────────────────
    {"GRIP_FLAG_0x400",      false, Kind::Simplification,
     "TZ 期间临时置位；扣除量 flash+0x884/0x886 实测均为 0，是恒等变换；03 §14"},
};

/// 编译期查表。名字打错会直接编译失败，而不是静默返回 false。
[[nodiscard]] constexpr const Gate& Find(const char* name) {
    for (const auto& g : kGateTable) {
        const char* a = g.name;
        const char* b = name;
        while (*a && *a == *b) { ++a; ++b; }
        if (*a == '\0' && *b == '\0') return g;
    }
    throw "未知门控名";  // constexpr 上下文中即编译错误
}

[[nodiscard]] constexpr bool IsOn(const char* name) { return Find(name).on; }
[[nodiscard]] constexpr Kind KindOf(const char* name) { return Find(name).kind; }

// ── 自检：把文档里最容易判反的四条钉死 ────────────────────────────────
static_assert(IsOn("FEAT_GLOVE"),
              "Glove 门控恒真（flash+0x918 = 0x0E），此前被误判为不可达");
static_assert(IsOn("FEAT_COORD_FREEZE"),
              "bit8 是坐标冻结使能，不是 XY 滤波位");
static_assert(!IsOn("FEAT_DOUBLE_REPORT") && !IsOn("FEAT_HOTZONE"),
              "倍频与热区本机关闭");
static_assert(IsOn("TM_FINGER_DETECT") && IsOn("TM_GLOVE_DETECT"),
              "flash+0x918 = 0x0E ⇒ TouchMode 的 Finger / Glove 检测都开着；"
              "早先把它们判成不可达是错的（06 §5.1）");
static_assert(!IsOn("GRIP_MERGE_HANDLER") && !IsOn("GRIP_DYN_DEADZONE") && !IsOn("GRIP_IDLE"),
              "GripFilter 的三条旁支关闭，但主体（BasicProcess/Report）照常执行");
static_assert(KindOf("GRIP_FLAG_0x20") == Kind::EitherOr,
              "0x20 关闭时走的是完整分支，删除会漏写 unit[0x350]");
static_assert((kFeatureFlags & (1u << 8)) != 0, "FEAT_COORD_FREEZE 与位图一致");
static_assert((kFeatureFlags & (1u << 21)) == 0, "FEAT_DOUBLE_REPORT 与位图一致");
static_assert((kFeatureFlags & (1u << 22)) == 0, "FEAT_HOTZONE 与位图一致");
static_assert(kFeatureFlags2 == 0x120u && (kFeatureFlags2 & 1u) == 0,
              "EdgePeakFilter workaround 未置位");

// ── 模块 01 的易判反项 ────────────────────────────────────────────────
static_assert(!IsOn("BL_STAGE_8_9") && KindOf("BL_STAGE_8_9") == Kind::DeadBranch,
              "Stage 8/9 无入口：属性位 0x80 全库无产出方（01 §7.2）");
static_assert(!IsOn("USE_DIF_BUF_PATH") && KindOf("USE_DIF_BUF_PATH") == Kind::EitherOr,
              "bit16 清 0 时走的才是完整基线管线，不可当死枝删（01 §2）");
static_assert(IsOn("BL_TRACK_FROM_PRECMF"),
              "flash+0x69 = 1 ⇒ BLIIR_DoUpdate 跟的是 PreCMF Raw（01 §6）");
static_assert(!IsOn("RAWDATA_FORCE_NORM"),
              "flash+0x604 = 0 ⇒ 线上字按 unsigned 取用，对位 wire ^ 0x8000（01 §4）");

// ── 模块 02 的四条易判反项 ────────────────────────────────────────────
static_assert(!IsOn("CMF_RAW_DOMAIN") && IsOn("CMF_DIFF_DOMAIN"),
              "bCmfEnabled = 0 ⇒ 走后置 CMF_Process，Rawdata_CMF 不执行（02 §1）");
static_assert(IsOn("CMF_FORCE_DIM1") && !IsOn("CMF_2D_PCA"),
              "flash+0x19 = 1 锁死 1D Dim1，2D PCA 关闭（02 §2）");
static_assert(!IsOn("BL_COMP_DISABLED") && KindOf("BL_COMP_DISABLED") == Kind::DeadBranch,
              "TSAStatic+0x3F0 恒 0 ⇒ 基线补偿这道门永远是开的，不可当作『补偿关闭』");
static_assert(!IsOn("UNDERWATER_SENTINEL"),
              "TSAStatic+0x3F1 是 0x2000 水下异常哨兵，**不是**基线复位进行中（02 §9/§14）");
static_assert(IsOn("THOLD_MODE_HYSTERESIS"),
              "峰值门限在手指确认前为 220、确认后为 600；把 600 当稳态恒值是错的（02 §17-9）");
static_assert(!IsOn("CMF_WEIGHTED") && !IsOn("CMF_SS_ASSIST"),
              "1D 路径无权重、SS 辅助为空桩（02 §5/§8）");

// ── 模块 03 的易判反项 ────────────────────────────────────────────────
static_assert(IsOn("PEAK_MODE_3") && !IsOn("PEAK_MODE_2"),
              "dwPeakProcessingMode = 3：腰斩 + Z8 + SD 判决全开，mode 2 分支死（03 §1）");
static_assert(!IsOn("PEAK_EXCLUDE_SIDE_REGION") && !IsOn("PEAK_SIDE_SPECIAL_THOLD"),
              "扫描范围恒为整屏，且不存在边列专用门限 —— 两者是同一条链（03 §2）");
static_assert(IsOn("PRESSURE_DRIFT") && (kFeatureFlags2 & 0x20u) != 0,
              "dwFeatureFlags2 bit5 = 1 ⇒ PressureDrift_Detect 必须实现，不是死枝（03 §3）");
static_assert(!IsOn("PEAK_RERUN_ON_SWITCHBUF"),
              "本机永不重跑 Peak_Process，但 g_peaksBeforeSwitchDifBuf 快照仍被无条件消费（03 §11）");
static_assert(!IsOn("GRIP_REGION_PALM_EXPAND") && (kFeatureFlags & 0x8000u) == 0,
              "bit15 = 0 ⇒ 掌压扩展区不涂（03 §13）");

} // namespace Solvers::Touch::Gates
