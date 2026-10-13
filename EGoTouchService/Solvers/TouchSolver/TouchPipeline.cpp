#include "TouchPipeline.h"
#include "config/ConfigBinder.h"
#include "config/ConfigStore.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>

namespace Solvers {

TouchPipeline::TouchPipeline()
    : m_coordFilter(m_reportFilter)
    , m_gesture(m_tracker)
    , m_palmBoxSuppressor(m_palmGrip)
    , m_contactExtractor(m_touchZone)
{
    // 模块 06 需要读模块 07 维护的三个位移锚点（+0x49A/+0x4A4/+0x4AC）来复刻
    // Touch_SetPersistFlag @0x6BA72776，并把 GripFilter 50 ms 放行 / GloveDetect
    // 两条路径的 Touch_CreatePostReport 请求转发给 07（文档 06 §2 / §4.6 / §5）。
    m_palmGrip.SetReportFilter(&m_reportFilter);

    // DAT_6BC4A910 在原版是**一个全局量**：TouchThold_Process 写、
    // HardwareAnalyzer_GetRaw2RawPrpt 与 CMF 读。Stage 0.5 排在 TSAPrmt_Process
    // 之前，读到的因此是上一帧的值（文档 01 §5 / 02 §14）。
    m_hwAnalyzer.m_sharedThold = &m_dynThold;
}

bool TouchPipeline::ProcessMasterParserOnly(HeatmapFrame& frame) {
    if (m_frameParser.m_enabled) {
        m_frameParser.Process(frame);
    }
    ResetIdleOutputs(frame);
    return true;
}

bool TouchPipeline::Process(HeatmapFrame& frame) {
    // 挂载共享大帧物理数据总线
    frame.touch.bigFrame = &m_bigFrame;
    m_bigFrame.ResetPerFrame();

    // ── 纯线性流水线调度编排（严格零算法分支逻辑，仅判断模块使能）──────────
    if (m_frameParser.m_enabled)  m_frameParser.Process(frame);

    // Stage 0.5：帧间硬件异常分析。判为异常帧时整帧保持上一帧输出并跳过其余各级，
    // 对位原版 HardwareAnalyzer_IsToBypassCurrentFrame 命中后直接走
    // Touch_KeepPrevTouchWithExceptionExcluded → TSA_MSProcessEnding 的短路分支。
    if (m_hwAnalyzer.Process(frame)) {
        frame.touch.output.contacts.clear();
        for (const auto& c : m_bigFrame.contacts) {
            frame.touch.output.contacts.push_back(c);
        }
        UpdateContactCaches(frame);
        return !frame.touch.output.contacts.empty() || m_bigFrame.hasLiveTouches;
    }

    // g_tsaPrpt._4_2_（raw2raw 超噪节点数）由 HardwareAnalyzer_GetRaw2RawPrpt
    // 产出，BLReset_IsBaselineNoisy @0x6BA4C0BA 读它判「噪声已散」（文档 01 §9）。
    m_baseline.m_inputs.raw2rawNumOverNoise =
        static_cast<uint16_t>(m_hwAnalyzer.Stats().numOverNoise);

    if (m_baseline.m_enabled)     m_baseline.Process(frame);

    // 对位原版 g_tsaStatic->bBaselineResetInDebounce：BLSM 阶段推进后锁存，
    // Stage 7 的 TouchReport_ToBeReported 通过 BLReset_InDebounce() 读它。
    m_bigFrame.baselineResetInDebounce = m_baseline.IsResetDebounceActive();

    // ⚠ 早先这里写 `m_dynThold.m_inputs.baselineResetInProgress = IsResetInProgress()`，
    // 把 `g_tsaStatic + 0x3F1` 当成「基线复位进行中」。反编译
    // `BLReset_Check @0x6BA48AF0` 推翻了该前提：`+0x3F1` 只在
    // `g_pPrevTouches->nTouchCount == 0x2000`（水下异常字）时才置位，其余帧被
    // 无条件清 0；本机不产生 0x2000 ⇒ **恒 0**（文档 02 §9 / §14）。
    // 接错会让任何一次普通基线复位都把整组门限压到 220。现按恒 0 处理。
    m_dynThold.m_inputs.underwaterResetSentinel = false;

    // Stage 2 需要 DAT_6BC4A910 的**上一帧**值（TSAPrmt_Process 排在 CMF 之后，
    // 文档 02 §14「关键时序」）：CMF_ProcessDimUnit 的共模超限标志与
    // TSAPrpt_GetDifPreCMFPrpt 的超噪计数都用它。
    m_cmf.m_noiseTholdPrevFrame = m_dynThold.NoiseTholdPrevFrame();

    // 对位 TSA_MSProcess：`if (!BLSM_IsReset()) { CMF_Process; GridIIR_Process; }`
    // BLSM_IsReset @0x6BA4D794 **只认 Stage 1** —— Stage 2（噪声重置）那一帧
    // Dif 已被 BLIIR_ResetDiff 清零，但 CMF 与 GridIIR 照跑。这是真实的非对称
    // 行为，必须照抄（文档 01 §7.5 / 复刻清单-6）。
    if (m_cmf.m_enabled && !m_baseline.BLSM_IsReset()) m_cmf.Process(frame);

    // Stage 2.5：对位原版 TSA_MSProcess 中 Peak_Process 前一行的 TSAPrmt_Process()，
    // 每帧用当前差分与跨帧状态把整组门限重算并挂到共享大帧上。
    if (m_dynThold.m_enabled) {
        m_dynThold.m_inputs.framePeakPositive = Touch::TouchThold::ScanFramePeakPositive(
            &m_bigFrame.diff[0][0], static_cast<size_t>(Touch::kTotalCells));
        // Stage 4 要的是同一个量，挂到大帧上共用，省掉它那趟全屏遍历。
        m_bigFrame.framePeakPositive = m_dynThold.m_inputs.framePeakPositive;
        m_bigFrame.framePeakValid = true;
        m_bigFrame.tholds = m_dynThold.Process();
    }

    if (m_peakDet.m_enabled)      m_peakDet.Process(frame);

    // Peak_Process 尾部写 g_tsaStatic->bFullIncellSignalDisparity，下一帧
    // TouchThold_GetPeakThold 读它决定峰值门限取 600 还是 300。这里把 Peak 级
    // 挂在大帧上的结论回灌给 Stage 2.5，闭合这条跨帧回路。
    m_dynThold.m_inputs.bFullIncellSignalDisparity = m_bigFrame.fullIncellSignalDisparity;

    if (m_touchZone.m_enabled)    m_touchZone.Process(frame);
    if (m_tracker.m_enabled)      m_tracker.Process(frame);
    if (m_palmGrip.m_enabled)     m_palmGrip.Process(frame);
    RunTouchModeFsm();
    if (m_reportFilter.m_enabled) m_reportFilter.Process(frame);

    // 帧尾回灌：对位原版 TSA_MSProcess 末尾的 PrevTouch_PostProcess @0x6BA6E09D，
    // 它把本帧 g_touchesPtr 的头部字段拷进 g_pPrevTouches，供**下一帧**的
    // TSAPrmt_Process / TZ_Process 读取。复刻里 Stage 2.5 与 Stage 4 都跑在
    // Stage 5 之前，所以在这里写就等于"下一帧读到的是上一帧的值"。
    LatchPrevTouchInputs();

    // 外层后处理：Thenar_ProcessInfoByDif 在 TSA_MSProcess 之外、上报之后执行，
    // 其判决供下一帧的 Stage 6 消费。
    if (m_palmGrip.m_enabled) m_palmGrip.ProcessThenarPostReport(m_bigFrame);

    // Stage 8 (TSAOut_PostProcess @0x6BA6A77A) 必须排在 PrevTouch_PostProcess 与
    // Thenar_ProcessInfoByDif **之后**：原版调用序是
    //   TSA_MSProcess{ ... TouchReport_Process, PrevTouch_PostProcess }
    //   → Thenar_ProcessInfoByDif → TSAOut_PostProcess → TSA_PitchSizeRemapProcess。
    // TSAOut 会用 Touch_UnitRemove 物理删除触点并紧凑化触点池，放在前面会让
    // 这两级读不到被删的单元。
    // B4：TSA_SetDMDType @0x6BA9D881 的输入。原版在 TSA_Processing 里由
    // TSA_SetDMDType 自己调 HardwareAnalyzer_GetDetectedException @0x6BAF0249；
    // 复刻里分析器是 Stage 0.5 的成员，这里取码后灌给 Stage 8。
    m_displayRemap.SetDetectedException(m_hwAnalyzer.GetDetectedException());
    if (m_displayRemap.m_enabled) m_displayRemap.Process(frame);

    // 锁存本帧最终输出，供下一个异常帧保持
    m_hwAnalyzer.LatchOutput(m_bigFrame);

    // 同步最终触点至 frame.touch.output.contacts
    frame.touch.output.contacts.clear();
    for (const auto& c : m_bigFrame.contacts) {
        frame.touch.output.contacts.push_back(c);
    }

    // ── 诊断缓存同步与触控状态呈报 ─────────────────────────────────
    UpdateContactCaches(frame);
    return !frame.touch.output.contacts.empty() || m_bigFrame.hasLiveTouches;
}

bool TouchPipeline::ProcessTrackingAndGesture(HeatmapFrame& frame) {
    frame.touch.bigFrame = &m_bigFrame;
    if (m_tracker.m_enabled)      m_tracker.Process(frame);
    if (m_palmGrip.m_enabled)     m_palmGrip.Process(frame);
    RunTouchModeFsm();
    if (m_reportFilter.m_enabled) m_reportFilter.Process(frame);
    LatchPrevTouchInputs();
    if (m_palmGrip.m_enabled) m_palmGrip.ProcessThenarPostReport(m_bigFrame);
    // B4：同上，见 ProcessFrame 里的说明。
    m_displayRemap.SetDetectedException(m_hwAnalyzer.GetDetectedException());
    if (m_displayRemap.m_enabled) m_displayRemap.Process(frame);

    frame.touch.output.contacts.clear();
    for (const auto& c : m_bigFrame.contacts) {
        frame.touch.output.contacts.push_back(c);
    }

    UpdateContactCaches(frame);
    return !frame.touch.output.contacts.empty() || m_bigFrame.hasLiveTouches;
}

// ── PrevTouch_PostProcess @0x6BA6E09D 的等价回灌 ─────────────────────
// 原版拷贝的是 g_touchesPtr 的头部计数区，这里只搬下游真正读的四项：
//   g_pPrevTouches->bPrevTouchCount ← 本帧触点数
//   g_pPrevTouches->field_0x5       ← g_touchesPtr[5]，TouchReport_ProcessUnit
//                                     每上报一个触点就自增一次，即**本帧实际
//                                     上报的触点数**（TouchThold 只判它是否为 1）
//   g_pPrevTouches[0xd].field_0x1c  ← 展开后即 prevTouch[0] + 0x1f0，
//                                     单指门限折扣要求那个触点**没有**被
//                                     Touch_SetVolatileFlagStableTouch 打上 0x1000
//   TZ_IsSD 的 g_pPrevTouches->nTouchCount ← 同一个触点数
void TouchPipeline::LatchPrevTouchInputs() {
    const uint8_t trackCount = static_cast<uint8_t>(
        std::min<size_t>(m_bigFrame.tracks.size(), 255));
    const size_t reportedCount = m_bigFrame.contacts.size();

    uint32_t firstTrackFlags = 0;
    if (!m_bigFrame.tracks.empty()) {
        firstTrackFlags = m_bigFrame.tracks[0].volatileFlags;
    }

    // TZ_CheckToSeparateZone 读的是 prevTouch + 0x218 bit0（上一帧该触点所属区域
    // 被握持分离过）。复刻的 Stage 4 在区域级判定时还没有触点绑定，这里把整帧的
    // 该位聚合后回灌，语义上放宽成"上一帧存在被分离的区域"。
    bool anySeparated = false;
    for (const auto& t : m_bigFrame.tracks) {
        if ((t.persistFlags & 1u) != 0) { anySeparated = true; break; }
    }
    m_touchZone.m_prevAnyZoneSeparated = anySeparated;

    m_dynThold.m_inputs.prevTouchCount = trackCount;
    m_dynThold.m_inputs.prevTouchStable = (reportedCount == 1);
    m_dynThold.m_inputs.prevSingleTouchExcluded =
        (firstTrackFlags & Touch::TOUCH_FLAG_STABLE_TOUCH) != 0;

    // TouchThold_GetNoisyTouchNum @0x6BA7C896：上一帧 touch[i]+0x1CC（M0）
    // 超过 flash[0x912] = 6000 的触点数，决定噪声基准取 100 / 150 / 200。
    {
        uint8_t noisy = 0;
        for (const auto& t : m_bigFrame.tracks) {
            if (static_cast<int32_t>(t.m0) > Touch::kNoisyTouchM0Thold && noisy < 255) ++noisy;
        }
        m_dynThold.m_inputs.noisyTouchCount = noisy;
    }

    // ── DAT_6BC4A928 / DAT_6BC4A924 的跨帧回灌（文档 02 §14）─────────
    // 原版 TouchMode_Process @0x6BA78DDE 排在 GripFilter 之后（帧尾），而
    // TSAPrmt_Process（本级的门限重算）排在 Peak 之前，所以 TouchThold 读到的
    // mode/stage 永远是**上一帧**的结论 —— 这是跨帧回路。
    //
    // 曾设想把 TouchModeFsm 拆成 UpdateMode(Stage 2.5 前) + ApplySuppress(6.5)
    // 以在帧内接通。反编译 `TouchMode_FingerDetect @0x6BA7930B` 否决了该方案：
    // 它读 `g_touchesPtr`（本帧触点列表与 +0x1F0 标志、+0x23C 峰索引）、
    // `g_pPeaks`（本帧峰值 z 与 +8 峰值和）以及 `DAT_6BCAB68C`（**本帧**刚算出的
    // sigThold）。这些量全部在 Stage 2.5 之后才存在，拆分会引入真正的循环依赖。
    // 因此保持 TouchModeFsm 在 Stage 6.5，改用跨帧回灌，与原版时序一致。
    m_dynThold.m_inputs.touchMode = m_touchMode.Mode();
    m_dynThold.m_inputs.touchStage = m_touchMode.ModeData();
    m_dynThold.m_inputs.gloveLatched = m_touchMode.GloveLatched();

    m_touchZone.m_prevTouchCount = trackCount;
    // g_pPrevTouches->bPrevTouchCount：PressureDrift_Detect @0x6BB441C9 的第一个
    // 分支判 `< 3`（文档 03 §3）。与 TZ 共用同一个"上一帧触点数"来源。
    m_peakDet.m_prevTouchCount = trackCount;
}

// ── TouchMode_Process @0x6BA78DDE 的调用落点 ─────────────────────────
// 原版位置是 GripFilter_Process 之后、AntiTouch_Process / TouchReport_Process
// 之前，对应复刻的 Stage 6 与 Stage 7 之间。它读本帧触点列表、写
// TrackingSlot::suppressFlags（touch + 0x208），供 Stage 7 的上报网闸
// 判据 6a / 6b 消费。
void TouchPipeline::RunTouchModeFsm() {
    if (!m_touchMode.m_enabled) return;

    Touch::TouchModeFsm::Inputs in{};
    in.touchCount = static_cast<uint8_t>(std::min<size_t>(m_bigFrame.tracks.size(), 255));
    // g_touchesPtr[0xd]：握持阶段 ∈ {1,2} 的触点数，由模块 06 的
    // GripFilter_BasicProcess @0x6BAE4C29 每帧累加后挂在共享大帧上。
    // 早先这里硬编码为 0（依据「dwGripFlags 整字为 0 ⇒ GripFilter 不执行」），
    // 反编译复核推翻了该前提：GripFilter_Process 的循环没有 grip-flag 前置。
    in.gripSuppressedCount = m_bigFrame.gripSuppressedCount;
    // TouchMode_GloveDetect @0x6BA793FE 读的同样是 `g_tsaStaticPtr[1].field_0x3`
    // （= TSAStatic+0x3F1，0x2000 水下异常哨兵），不是普通基线复位。
    // BLReset_Check @0x6BA48AF0 每帧在 nTouchCount != 0x2000 时把它清 0，
    // 本机不产生 0x2000 ⇒ 恒 0（文档 02 §9）。接成真实复位会误推手套闩锁。
    in.baselineResetInProgress = false;
    in.fullIncellSignalDisparity = m_bigFrame.fullIncellSignalDisparity;
    in.frameCounter = static_cast<uint32_t>(m_bigFrame.frameIndex);
    // g_pPeaks + 8：本帧峰值总量，FingerDetect 的 3 帧强峰确认读它。
    {
        int32_t sum = 0;
        for (const auto& pk : m_bigFrame.peaks) sum += pk.z;
        in.framePeakSum = sum;
    }
    // DAT_6bcab68c = TSADynPrmt_GetPkThold 每帧刷新的信号门限（稳态 600）。
    in.fingerPeakThold = m_bigFrame.tholds.sigThold;

    m_touchMode.Process(m_bigFrame, in);

    // TouchMode_GloveDetect 首次锁存时调 Touch_CreatePostReport(i, 5)；
    // 转发给模块 07，由本帧稍后的 TouchReport_ProcessUnit 消费。
    if (const uint32_t mask = m_touchMode.TakePostReportRequests(); mask != 0) {
        for (uint8_t id = 1; id < 32; ++id) {
            if ((mask & (1u << id)) != 0) m_reportFilter.RequestPostReport(id);
        }
    }
}

bool TouchPipeline::ProcessGestureOutput(HeatmapFrame& frame) {
    return m_legacyGesture.Process(frame);
}

// ⚠ 非诊断构建下这里**不再取 m_diagMu**。该锁的名义作用是保护 UI / IPC 线程
// 读 m_bigFrame，但 Process 的其余十几级本来就在锁外原位改写 m_bigFrame ——
// 只在帧尾这一处加锁并不能让那些读变安全，却让热路径每帧多一次加解锁（p99
// 尾部抖动的主要来源之一）。非诊断构建下本函数只写三个原子量，无需互斥。
// 诊断构建仍保留原有行为：debug 快照的填充与 GetPeaks/GetTouchZones 串行化。
void TouchPipeline::UpdateContactCaches(HeatmapFrame& frame) {
#if EGOTOUCH_DIAG
    std::lock_guard<std::mutex> lk(m_diagMu);
#endif
    m_cachedPeakCount.store(static_cast<int>(m_bigFrame.peaks.size()), std::memory_order_relaxed);
    m_cachedZoneCount.store(static_cast<int>(m_bigFrame.zones.size()), std::memory_order_relaxed);
    m_cachedContactCount.store(static_cast<int>(frame.touch.output.contacts.size()), std::memory_order_relaxed);

#if EGOTOUCH_DIAG
    // 1. 同步 Peaks 至 frame.touch.debug.peaks 供 IPC 共享缓冲区与 UI 显示
    frame.touch.debug.peaks.clear();
    frame.touch.debug.peaks.reserve(m_bigFrame.peaks.size());
    for (const auto& p : m_bigFrame.peaks) {
        TouchPeak tp{};
        tp.r = p.r;
        tp.c = p.c;
        tp.z = p.z;
        tp.id = p.id;
        frame.touch.debug.peaks.push_back(tp);
    }

    // 2. 同步 TouchZones (40x60) 矩阵至 debug.touchZones 展平数组
    std::memcpy(frame.touch.debug.touchZones.data(), m_bigFrame.touchZones, sizeof(frame.touch.debug.touchZones));

    // 3. 同步 ZoneBoxes 供 UI 框选高亮
    frame.touch.debug.zoneBoxes.clear();
    for (size_t i = 0; i < m_bigFrame.zones.size(); ++i) {
        const auto& z = m_bigFrame.zones[i];
        TouchZoneDebugBox box{};
        box.zoneId = z.id;
        box.zoneIndex = static_cast<uint8_t>(std::min<size_t>(i, 255));
        box.bbox = {z.minR, z.maxR, z.minC, z.maxC};
        box.area = z.pixelCount;
        box.signalSum = z.m0;
        frame.touch.debug.zoneBoxes.push_back(box);
    }
#endif
}

void TouchPipeline::ResetIdleOutputs(HeatmapFrame& frame) {
#if EGOTOUCH_DIAG
    std::lock_guard<std::mutex> lk(m_diagMu);
#endif
    frame.touch.output.contacts.clear();
    frame.touch.output.touchPackets = {};

    // 逐格握持驻留图是跨帧锁存的，只有在整条管线都不跑的空转路径上才清。
    // 对位原版 TSA_MSReset 里的缓冲区重置：没有帧进来时不该留着上一笔的黏滞。
    std::memset(m_bigFrame.gripTime, 0, sizeof(m_bigFrame.gripTime));
    std::memset(m_bigFrame.gripStage, 0, sizeof(m_bigFrame.gripStage));

    m_cachedPeakCount.store(0, std::memory_order_relaxed);
    m_cachedZoneCount.store(0, std::memory_order_relaxed);
    m_cachedContactCount.store(0, std::memory_order_relaxed);

#if EGOTOUCH_DIAG
    frame.touch.debug.peaks.clear();
    frame.touch.debug.touchZones.fill(0);
    frame.touch.debug.peakZones.fill(0);
    frame.touch.debug.zoneBoxes.clear();
    frame.touch.debug.palmBoxes.clear();
#endif
}

std::vector<Touch::Peak> TouchPipeline::GetPeaks() const {
    std::lock_guard<std::mutex> lk(m_diagMu);
    std::vector<Touch::Peak> result;
    result.reserve(m_bigFrame.peaks.size());
    for (size_t i = 0; i < m_bigFrame.peaks.size(); ++i) {
        const auto& p = m_bigFrame.peaks[i];
        Touch::Peak outP{};
        outP.r = p.r;
        outP.c = p.c;
        outP.z = p.z;
        outP.id = p.id;
        outP.tzAge = p.age;
        result.push_back(outP);
    }
    return result;
}

std::array<uint8_t, 2400> TouchPipeline::GetTouchZones() const {
    std::lock_guard<std::mutex> lk(m_diagMu);
    std::array<uint8_t, 2400> arr{};
    std::memcpy(arr.data(), m_bigFrame.touchZones, sizeof(arr));
    return arr;
}

std::array<uint8_t, 2400> TouchPipeline::GetZoneEdge() const {
    std::lock_guard<std::mutex> lk(m_diagMu);
    std::array<uint8_t, 2400> arr{};
    std::memcpy(arr.data(), m_bigFrame.zoneEdge, sizeof(arr));
    return arr;
}

// 手指管线不对外暴露任何运行时可调项：从线上格式解码到显示映射，每一个数值都取自
// TSACore 的 W273AS2700 Flash 整定表，改动其中任何一个都会偏离原版行为。这两个函数
// 因此是空实现，`ConfigKeyId` 的 Touch 分区亦已整体退役。
void TouchPipeline::registerBindings(Config::ConfigBinder& binder) {
    (void)binder;
}

void TouchPipeline::applyConfig(const Config::ConfigStore& store) {
    (void)store;
}

} // namespace Solvers
