#pragma once
// 运行中由各模块按场景改写的两块参数：g_tsaPrmtRam 与 g_tsaPrmtRamConst。
// 原代码用 TSAPrmt(模块, 字段) / TSAConstPrmt(模块, 字段) 宏访问，字段名取自断言文本。

#include <cstddef>
#include <cstdint>

#include "Common/Image.h"

namespace Solvers::Tsa::Core {

/// 自适应 IIR 的系数表：位移量在 lightDelta 以下用 lightCoef，heavyDelta 以上用 heavyCoef，中间线性插值。
struct AiirPrmt {
    uint16_t lightDelta;
    uint16_t heavyDelta;
    uint8_t lightCoef;
    uint8_t heavyCoef;
};

/// g_tsaPrmtRam
struct RamPrmt {
    uint8_t ctdEdgeTable[0x16];  // CTD 的边缘补偿分段表（CTD 尚未转换，按字节访问）
    AiirPrmt aiir;               // Filter 按触点状态从 flash 参数里选一组写入
};
static_assert(offsetof(RamPrmt, aiir) == 0x16);
static_assert(sizeof(RamPrmt) == 0x1c);

/// g_tsaPrmtRamConst
struct RamConstPrmt {
    uint8_t reserved00[2];
    uint16_t aiirScaledCoefToDeltaRatio;  // 系数插值斜率，Q10：每单位位移增加的系数
};
static_assert(offsetof(RamConstPrmt, aiirScaledCoefToDeltaRatio) == 0x2);

[[nodiscard]] inline RamPrmt& PrmtRam() noexcept { return Image::At<RamPrmt>(0x6bcab720); }
[[nodiscard]] inline RamConstPrmt& PrmtRamConst() noexcept { return Image::At<RamConstPrmt>(0x6bcab664); }

}  // namespace Solvers::Tsa::Core
