#pragma once
// Full Incell 的区域异常检测（FIException）：逐区域判基线异常与湿手指，再参照全局异常消抖，
// 本帧检出的最大异常记到 TsaStatic::fiException.detected。

#include <cstdint>

namespace Solvers::Tsa::Core {
struct TouchZoneUnit;
}

namespace Solvers::Tsa::Core::FiException {

[[nodiscard]] bool Malloc();
void Free();
void Init();

void ProcessTouchZone(TouchZoneUnit& zone);

}  // namespace Solvers::Tsa::Core::FiException
