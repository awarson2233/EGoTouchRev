#pragma once
// 按信号符号分流的格子列表（原 src/Tool/PosnList/PosnSpecial/PosnSpecial.c）：TZ / SubTZ 做区域遍历时用。
// 列表放在调用方给的缓冲区里：Dif 非负的格子从前端压入，负的从后端压入。取出时先取完一种符号再换另一种，
// 每换一次符号开一个新的统计区间（最多 10 个），记下区间内 Dif 的最大、最小值、总和与格子数。
// 另带两组由 TZ 直接记录的计数（Record / Record2）。状态是一块全局（g_list），TZ 也按偏移直接读区间统计。

#include <cstddef>
#include <cstdint>

#include "Common/Image.h"

namespace Solvers::Tsa::Core::PosnSpecial {

inline constexpr uint8_t kZoneNum = 10;
inline constexpr uint8_t kNoSign = 0xff;

/// 一个同号区间的 Dif 统计。
struct Zone {
    int16_t maxSig;
    int16_t minSig;
    int32_t sigSum;
    uint8_t cellNum;
    uint8_t reserved009[0x3];
};
static_assert(sizeof(Zone) == 0xC);

/// g_list
struct List {
    uint16_t* cells;          // 调用方的缓冲区
    uint16_t capacity;        // 缓冲区格数，也是负端的起点
    uint16_t posCount;        // 前端（非负）已压入的个数
    uint16_t negHead;         // 后端（负）的栈顶
    uint8_t sign;             // 当前取哪一端：0 非负，1 负
    uint8_t prevSign;         // 上一次取出的符号
    uint8_t field10;          // Init 置 1，无人读
    uint8_t posMarker;        // 从负切到非负的次数
    uint8_t negMarker;        // 从非负切到负的次数
    uint8_t numSignChange;    // Record：Dif 符号变化的次数
    uint8_t numValidSensor;   // Record：超门限的有效格子数
    uint8_t numEdgeSensor;    // Record2：超门限的边缘格子数
    uint8_t reserved016[0x2];
    int32_t sumEdgeSensor;    // Record2：边缘格子的 Dif 总和
    uint8_t lastSign;         // Record：上一个格子的符号，kNoSign 为还没有
    uint8_t zoneIdx;          // 当前统计区间
    uint8_t reserved01E[0x2];
    Zone zone[kZoneNum];
};
static_assert(sizeof(List) == 0x98);
static_assert(offsetof(List, capacity) == 0x08);
static_assert(offsetof(List, sign) == 0x0E);
static_assert(offsetof(List, sumEdgeSensor) == 0x18);
static_assert(offsetof(List, lastSign) == 0x1C);
static_assert(offsetof(List, zone) == 0x20);

[[nodiscard]] inline List& Global() noexcept { return Image::At<List>(0x6bcac4c0); }

void Init(uint16_t* cells, uint16_t capacity);
/// 清零 Record / Record2 的计数。
void ReBase();
/// 压入一个格子（按 Dif 符号分端）；缓冲区满时丢弃。
void Put(uint16_t cell);
/// 取出一个格子并计入当前区间；当前一端取空时先换端、开新区间。调用前确认不空。
uint16_t Get();
[[nodiscard]] bool IsEmpty();
/// TZ 遍历时记一个格子：Dif 超过 thold 且 valid 时计为有效格子；Dif 符号与上一个不同时计一次符号变化。
void Record(int16_t sig, int16_t thold, bool valid);
/// TZ 遍历边缘时记一个格子：Dif 超过 thold 时计为边缘格子，Dif 计入总和。
void Record2(int16_t sig, int16_t thold);
[[nodiscard]] uint8_t GetNumSignChange();
[[nodiscard]] uint8_t GetNumValidSensor();
[[nodiscard]] uint8_t GetNumEdgeSensor();
[[nodiscard]] int32_t GetSumEdgeSensor();
/// posMarker，至少为 1。
[[nodiscard]] uint8_t GetPosMarker();

}  // namespace Solvers::Tsa::Core::PosnSpecial
