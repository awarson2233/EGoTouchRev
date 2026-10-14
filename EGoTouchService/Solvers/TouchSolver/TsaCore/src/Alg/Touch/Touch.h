#pragma once
// 当前帧触点表的维护（原 src/Alg/Touch/TouchData.c）：
//   - 表的清空、单元拷贝 / 移除、与历史表的下标对应；
//   - UnitUpdate：TE 跟踪匹配之后，按历史计算位移、边缘、单帧 / 跨帧标志与极值；
//   - ProcessAftTouchEvent：TouchEvent 之后统计异常形状、报点数，判定可信度与合并 / 分离；
//   - 各模块用到的单个触点查询。
// 距离与时间换算在 TouchGeometry.h。

#include <cstdint>

#include "Alg/Touch/TouchData.h"
#include "Alg/Touch/TouchGeometry.h"

namespace Solvers::Tsa::Core::Touch {

void Init();
void Clean();

/// 按 ID 给每个当前触点找到历史表下标（找不到为 kNoTouch）。
void AssignPreIdxBasedOnId();
[[nodiscard]] uint8_t GetIdxBasedOnPreIdx(uint8_t prevIdx);
/// 本帧作废：当前表恢复成历史表（表头里异常相关的部分除外）。
void KeepPrevTouchWithExceptionExcluded();

/// 历史单元的公共前缀拷回当前单元，保留当前帧的 sigSum 极值、三个比值与 persistFlags3。
void UnitCopy(uint8_t curIdx, uint8_t prevIdx);
void UnitRemove(uint8_t curIdx);
/// TE 匹配后对仍在场的触点逐个调用。
void UnitUpdate(uint8_t curIdx, uint8_t prevIdx);
void SetEdgeDist(uint8_t curIdx);

void ProcessAftTouchEvent();
/// 握持在屏幕左右边缘的 β 手势：连续两帧满足时打标记并计数。
void ProcessBetaGrip();
/// 表头 extFlags 记下水下 / 出水 / 拿起 / 放下等外部状态，然后 AntiTouch 上报。
void ProcessExtInfo();
void ThenarFeatureReset();
/// 首次对外报点时，在起点与当前点之间插值补进历史轨迹。
void CreatePostReport(uint8_t curIdx);

void GetRptSqDistMmFromStartPos(uint8_t curIdx, uint8_t prevIdx);
void GetRptSqDistMmFromPrevPos(uint8_t curIdx, uint8_t prevIdx);
void GetRptSqDistDisplayPixelFromPrevPos(uint8_t curIdx, uint8_t prevIdx);
void GetFltSqDistPixelFromPrevPos(uint8_t curIdx, uint8_t prevIdx);
[[nodiscard]] uint16_t GetSqDistMmInBetween(uint8_t idxA, uint8_t idxB);

[[nodiscard]] bool IsOffEvent(uint32_t event);
[[nodiscard]] bool IsOff(uint8_t curIdx);
/// 当前 / 历史单元或任一表头带异常。下标为 kNoTouch 时不看对应单元。
[[nodiscard]] bool IsException(uint8_t curIdx, uint8_t prevIdx);
[[nodiscard]] bool IsGrip(uint8_t curIdx);
[[nodiscard]] bool IsGhostByMinSigOff(uint8_t curIdx);
[[nodiscard]] bool IsGhostByMinSigOff2(uint8_t curIdx);
[[nodiscard]] bool IsFullyLiftOff(uint8_t curIdx);
/// 主屏触点 coreIdx 与侧边触点 sideIdx 是否落在彼此的区域里。
[[nodiscard]] bool IsSideCoreInSameZone(uint8_t coreIdx, uint8_t sideIdx);
[[nodiscard]] bool NeedXyFilter(uint8_t curIdx, uint8_t prevIdx);
[[nodiscard]] bool ValidComparison(uint8_t curIdx, uint8_t prevIdx);
/// 触点所在格自按下 / 自开始报点以来的时间。
[[nodiscard]] int GetTimeOn(uint8_t curIdx);
[[nodiscard]] int GetTimeRpt(uint8_t curIdx);

}  // namespace Solvers::Tsa::Core::Touch
