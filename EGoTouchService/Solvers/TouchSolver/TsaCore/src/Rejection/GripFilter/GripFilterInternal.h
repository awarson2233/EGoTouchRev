#pragma once
// GripFilter 各文件共用的上下文、属性位与内部函数。

#include <cstdint>

#include "Alg/Touch/TouchData.h"
#include "Prmt/Prmt.h"
#include "Rejection/GripFilter/GripFilter.h"

namespace Solvers::Tsa::Core::GripFilter {

// ── 属性位 ───────────────────────────────────────────────────────────────

/// TouchUnit::gripFlags
inline constexpr uint32_t kGripNewTouchNearby = 0x1;        // 附近有按下时间相近的新触点
inline constexpr uint32_t kGripMulti = 0x2;                 // 多指握持
inline constexpr uint32_t kGripInMultiRegion = 0x4;         // 落在多指握持区域
inline constexpr uint32_t kGripCloseInTime = 0x8;           // 紧挨着一个按下时间相近的触点
inline constexpr uint32_t kGripCloseInPalm = 0x10;          // 紧挨着一个触点（屏内的须在手掌区域里）
inline constexpr uint32_t kGripOutOfPalm = 0x20;            // 不在手掌区域
inline constexpr uint32_t kGripHasOther = 0x40;             // 本帧还有别的触点
inline constexpr uint32_t kGripNoQuality = 0x80;            // 本帧质量为“无”
inline constexpr uint32_t kGripOppositeSide = 0x100;        // 与按下时间相近的触点分处两侧（或贴着侧边触点）
inline constexpr uint32_t kGripIdMappingCancelled = 0x200;  // IDT：GripFilter 取消了配对
inline constexpr uint32_t kGripSafe = 0x800;                // 安全触点（绝对非握持）
inline constexpr uint32_t kGripFastMove = 0x1000;
inline constexpr uint32_t kGripPalmShape = 0x2000;          // 手掌区域里的长条形状
inline constexpr uint32_t kGripPalmShape2 = 0x4000;
inline constexpr uint32_t kGripFlickBlocked = 0x8000;       // 像轻扫但不允许按轻扫放行
inline constexpr uint32_t kGripCornerDebounced = 0x10000;   // 角落里按下消抖较久
inline constexpr uint32_t kGripHigherPriority = 0x20000;    // 比附近的触点优先
inline constexpr uint32_t kGripAbnormal = 0x40000;
inline constexpr uint32_t kGripWeakCenter = 0x80000;        // 在握持区域里或信号和偏低
inline constexpr uint32_t kGripMergedGrip = 0x100000;       // Touch：区域里并入了一个抬起的握持触点
inline constexpr uint32_t kGripPeakAbsNotGrip = 0x200000;   // 合并后的峰被 TZ 标为绝对非握持
inline constexpr uint32_t kGripElongated = 0x400000;        // 长条形状
inline constexpr uint32_t kGripInCorner = 0x800000;
inline constexpr uint32_t kGripInCornerFirstFrame = 0x1000000;
inline constexpr uint32_t kGripNearWithdrawable = 0x2000000;  // 优先且附近有可撤下的触点
inline constexpr uint32_t kGripGrab = 0x4000000;            // 多个触点几乎同时按下的抓握
inline constexpr uint32_t kGripHandRelated = 0x8000000;     // HandGesture：与另一个握持触点关联
inline constexpr uint32_t kGripRawJump = 0x10000000;        // Touch：新触点按下时 raw 跳变
inline constexpr uint32_t kGripAboveGrip = 0x20000000;      // 在另一个多指握持触点的上方
inline constexpr uint32_t kGripDelayTimeOut = 0x40000000;   // 按下已超过延时

/// TouchUnit::persistFlags2
inline constexpr uint32_t kPersist2IdleWeak = 0x2;           // Debounce：IDLE 扫描中按抬高的门限判按下
inline constexpr uint32_t kPersist2BothEdges = 0x8;          // Touch：两个方向的边缘比例都超过 7
inline constexpr uint32_t kPersist2MergedOrSplit = 0x3000000;  // Touch：因合并抬起（0x1000000）/ 因分离按下（0x2000000）
inline constexpr uint32_t kPersist2HeavyRegion = 0x4000000;
inline constexpr uint32_t kPersist2HeavyCorner = 0x8000000;
inline constexpr uint32_t kPersist2HeavyCornerFirstFrame = 0x10000000;
inline constexpr uint32_t kPersist2HeavyCornerOff = 0x20000000;
inline constexpr uint32_t kPersist2GripToPost = 0x40000000;  // 由握持转为报点，需补报

/// TouchUnit::persistFlags4
inline constexpr uint32_t kPersist4Merged = 0x1;             // TZ 把它并入了握持
inline constexpr uint32_t kPersist4HeavyCornerTouched = 0x2;
inline constexpr uint32_t kPersist4CloseGrip = 0x4;          // 曾在屏内紧挨着另一个触点
inline constexpr uint32_t kPersist4InnerEdgeNarrow = 0x8;    // 单列、跨多行且峰在内侧边缘
inline constexpr uint32_t kPersist4MergeCancelled = 0x10;    // 握持合并被取消

/// TouchUnit::persistFlags
inline constexpr uint32_t kPersistReported = 0x8;
inline constexpr uint32_t kPersistReportedOnce = 0x100000;
inline constexpr uint32_t kPersistMergeHandled = 0x200000;
inline constexpr uint32_t kPersistWithdrawable = 0x20000000;

/// TouchUnit::volatileFlags
inline constexpr uint32_t kVolatileSigSumSpread = 0x1;        // Touch：信号和占比超过 0x28
inline constexpr uint32_t kVolatileMultiGripRegion = 0x4000;  // TZ：落在多指握持区域

/// PeakEntry::flags（TZ 写入）
inline constexpr uint32_t kPeakAbsGrip = 0x400;
inline constexpr uint32_t kPeakAbsNotGrip = 0x800;

/// TouchUnit::touchSide（TSA side）
inline constexpr uint32_t kSideTop = 0x1;
inline constexpr uint32_t kSideBottom = 0x2;
inline constexpr uint32_t kSideLeft = 0x4;
inline constexpr uint32_t kSideRight = 0x8;

/// FlashPrmt::gripDevice
inline constexpr int32_t kDevicePad = 4;
inline constexpr int32_t kDeviceFold = 5;

// ── 上下文 ───────────────────────────────────────────────────────────────

/// 单个触点判定时的上下文（原 g_grip，0x6bc84e20）。
struct Context {
    uint8_t cur;
    uint8_t prev;
    uint8_t absGripEnterCount;  // 进入绝对握持的次数（打字场景开始或双手时清零，到 0xff 为止）
    uint32_t edge;              // 触点所在的边（0..3，见 GetEdge）
    uint32_t quality;           // kQuality*
    uint8_t prmtLevel;          // 握持区域等级对应的参数组
};

inline constexpr uint32_t kQualityAbsNotGrip = 0;
inline constexpr uint32_t kQualityNone = 1;
inline constexpr uint32_t kQualityTempGrip = 2;
inline constexpr uint32_t kQualityAbsGrip = 3;

/// 按握持区域等级取的一组参数（原 gripPrmt，只读）。
struct LevelPrmt {
    uint8_t absGripEnabled;
    uint16_t delayMs;
    uint16_t absTholdPercent;
};

Context& Ctx();
const LevelPrmt& CurPrmt();

inline TouchUnit& Cur(uint8_t idx) { return Touches().unit[idx]; }
// 原代码不检查历史下标：为 kNoTouch 时照样按下标读写，落在历史表之后的内存上。
inline PrevTouchUnit& Prev(uint8_t idx) { return PrevTouches().unit[idx]; }
inline TouchUnit& CurG() { return Cur(Ctx().cur); }
inline PrevTouchUnit& PrevG() { return Prev(Ctx().prev); }
inline const GripEdgePrmt& EdgePrmt() { return PrmtFlash().gripEdge[Ctx().edge]; }
inline bool IsPad() { return PrmtFlash().gripDevice == kDevicePad; }

// ── 主流程（GripFilter.cpp）────────────────────────────────────────────

uint32_t GetEdge();
uint8_t GetGripLevel(uint8_t regionBits);
uint8_t GetCenterLevel(uint8_t curIdx);
bool Is2HandGame();
bool IsVideoMode();
bool IsAbsGrip(uint8_t curIdx);
bool IsMerged(uint8_t curIdx, uint8_t prevIdx);

// ── 区域与死区（GripRegion.cpp）────────────────────────────────────────

void RegionReset();
void DeadzoneReset();
void UpdateDeadzone();
uint8_t GetEdgeWidthStart(uint16_t pos, uint8_t width);
uint8_t GetEdgeWidthEnd(uint16_t pos, uint8_t width, uint8_t len);
bool IsGripCriteriaDeadzone(uint8_t curIdx);
bool IsInDynamicDeadZone(uint16_t distThold);
bool IsTouchInHeavyRegion(uint8_t curIdx);
bool IsTouchInCornerRegion(uint8_t curIdx);
bool IsHeavyRegionTouched(uint8_t curIdx);
bool IsHeavyCornerTouched(uint8_t curIdx);
bool IsInExcludedRegion(uint8_t curIdx);

// ── 属性检测（GripDetect.cpp）──────────────────────────────────────────

void GetProperty();
void GetShapeFlag();
bool IsInMultiGripRegion(uint8_t curIdx);
bool IsInPalmRegion(uint8_t curIdx);
bool IsTouchIntervalInTime(uint8_t curIdx, uint8_t otherIdx, bool otherOnSide, int maxMs);
bool IsHigherPriorityTouch(uint8_t curIdx, uint8_t otherIdx);
int GetDim2Distance(uint8_t aStart, uint8_t aEnd, uint8_t bStart, uint8_t bEnd);
int GetCoreDim2Distance(uint8_t a, uint8_t b);
int GetSideDim2Distance(uint8_t coreIdx, uint8_t sideIdx);
void CloseGripOnCore();
void CloseGripOnSide();
void CheckMultiGrip();
void CheckMultiGripOnCore();
void CheckMultiGripOnSide();
void CheckCornerMultiGrip();
void CheckScreenRotatedMultiGrip();

// ── 判据（GripCriteria.cpp）────────────────────────────────────────────

bool BasicIsAbsNotGrip();
bool BasicIsAbsGrip();
bool BasicIsAbsGripPrmt();
bool BasicIsAbsGripAbnormal();
bool BasicIsTempGrip();
bool BasicIsTempGripAbnormal();
bool BasicIsFlick();
bool IsFlickAllowed();
bool IsFlickDetected();
bool IsRatioOK(uint32_t kind);
uint16_t GetEdgeClickRatioThold(uint8_t level);
bool IsOnDim1();
bool IsOnDim2();
bool IsNearbyCorner(uint8_t curIdx);
bool IsLongPress();
bool IsLongPressAllowed();
bool IsCornerLongPressAllowed();
bool IsVideoModeLongPressAllowed();
bool IsScreenRotatedStateLongPressAllowed();
bool IsSignalOK(uint8_t curIdx, uint32_t kind, bool checkState);
bool IsSuppressedGlobally();
bool IsSuppressedGlobally2();
bool IsSuppressedByHandGesture();
bool IsSuppressedByCoreTouch();
bool IsCornerGripClick();
bool IsNegSigTouchBypass();
bool IsAbnormalLiftOff();
bool IsToCheckAbnormal();
bool IsToCheckStuckSideTouch();
bool IsStuckSideTouch();
bool IsBackToGripAllowed();
bool IsBackToGripSatisfied();
bool IsSeparated(uint8_t curIdx, uint8_t prevIdx);
uint16_t GetAbsThold(uint16_t percent);
uint16_t GetDelayTime();
bool IsToApplyMaxDelay();
bool IsToApplyDelay();
bool IsToBypassDelay();

}  // namespace Solvers::Tsa::Core::GripFilter
