#pragma once
// 网格缓冲的分配与逐帧维护（原 src/App/TSA_Buffer.c 与 TSA_Mem.c）：每格的按下 / 抬起 / 报点 / 握持
// 计时、区域编码的上一帧副本、每格所属触点 ID。缓冲指针在 TsaBufferTable。

namespace Solvers::Tsa::Core::Buffer {

/// 按项目参数定下网格尺寸，再分配全部模块的缓冲（先释放旧的）。
[[nodiscard]] bool InitProject();
/// 释放全部模块的缓冲。
void FreeAll();

[[nodiscard]] bool Malloc();
void Free();
void Init();
/// 区域编码出来之后：按每格是否在区域里推进按下 / 抬起计时，存下本帧的 tzZone，求每格所属触点 ID。
void PreProcess();
/// 本帧结束：存下区域图。
void PostProcess();
/// 上一帧的触点处理完之后：推进每格的报点计时。
void PrevFrameProcess();
/// 握持计时与每格的握持阶段。
void GripStageProcess();

}  // namespace Solvers::Tsa::Core::Buffer
