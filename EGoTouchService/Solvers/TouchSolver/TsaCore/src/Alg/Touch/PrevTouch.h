#pragma once
// 历史触点表的维护（原 src/Alg/Touch/PrevTouchData.c）：当前帧与历史的下标对应、帧末把当前帧并入历史、
// 每个历史触点的轨迹缓冲（30 个带时间的点）与网格轨迹队列（16 格）。

#include <cstdint>

#include "Alg/Touch/TouchData.h"

namespace Solvers::Tsa::Core::PrevTouch {

void Init();
/// 清空整张历史表；手掌在左 / 右侧时在表头 extFlags 记 2 / 4。
void Release();

/// 按 ID 给每个历史触点找到当前帧的下标（找不到为 kNoTouch）。
void AssignCurIdxBasedOnId();
[[nodiscard]] uint8_t GetIdxBasedOnId(uint8_t id);
[[nodiscard]] uint8_t GetIdxBasedOnCurIdx(uint8_t curIdx);
/// id 既不在历史表里、也不在最近回收的 ID 里时才能分配给新触点。
[[nodiscard]] bool IsValidId(uint8_t id);
[[nodiscard]] bool IsAnyExceptionDetected();

/// 帧末：表头从当前帧并入历史，然后 RemoveOff。
void PostProcess();
/// 仍在场的触点拷进历史，已抬起的从历史里移除，其余历史槽位清零。
void RemoveOff();

void UnitCopy(uint8_t prevIdx, uint8_t curIdx);
void UnitRelease(uint8_t prevIdx);
void UnitRemove(uint8_t prevIdx);
/// 触点仍在场时更新历史里的运动量（位移、速度、加速度）与极值。
void UnitUpdate(uint8_t prevIdx, uint8_t curIdx);
/// 刚按下的触点记下所在格的 minSigOff / timeOff / timeRpt。
void UnitUpdateCommon(uint8_t prevIdx, uint8_t curIdx);

[[nodiscard]] uint8_t HistoryBufferGetNum(uint8_t prevIdx);
[[nodiscard]] bool IsHistoryBufferFull(uint8_t prevIdx);
void HistoryBufferClean(uint8_t prevIdx);
/// 追加到末尾；满了先丢掉最早的一个。
bool HistoryBufferPush(uint8_t prevIdx, const HistoryEntry& entry);
/// 取出最早的一个。out 先清零。
bool HistoryBufferPop(uint8_t prevIdx, HistoryEntry& out);
bool HistoryBufferGet(uint8_t prevIdx, uint8_t pos, HistoryEntry& out);
void HistoryBufferInsert(uint8_t prevIdx, uint8_t pos, const HistoryEntry& entry);
void HistoryBufferRemove(uint8_t prevIdx, uint8_t pos);

/// 把位置所在的格子记进网格轨迹队列；与上一格不相邻时先补一个中点格。返回是否有新格子入队。
bool HistoryQueuePush(uint8_t prevIdx, uint16_t dim1, uint16_t dim2);

}  // namespace Solvers::Tsa::Core::PrevTouch
