#pragma once
// TSACore 的入口与主流程（原 src/App/TSA_Main.c、TSA_MSProc.c、TSA_MS.c、TSA_Malloc.c）：机型与 AFE 的
// 初始化、每帧的处理（互容主流程、笔、输出），以及何时允许 AFE 进入 IDLE。

#include <cstdint>

#include "App/Afe.h"

namespace Solvers::Tsa::Core::Tsa {

/// 宿主初始化：先装入 AFE 能力，再按机型名初始化全部模块。参数为空或机型装入失败时返回 false。
[[nodiscard]] bool InitAfeProject(const char* project, const Afe::Caps& caps);
/// 宿主送来一帧：拷下 AFE 帧后处理。
void ProcessAfeFrame(const Afe::Frame& frame, uint32_t ctrl);
/// 互容主流程复位：重新初始化各模块，保留跨复位的统计（视系统复位位而定）。
void MsReset();

/// 是否允许 AFE 进 IDLE：笔模式下不允许；自检时听自检的；侧边触摸机型按侧边规则。
[[nodiscard]] bool IsOkToEnterIdle();
/// 主区域本帧可否进 IDLE（含 dbEnterIdle 防抖），供 Sys 更新 TsaStatic::idleReady。
[[nodiscard]] bool UpdateIdleMode();
/// 侧边区域：侧边触摸被裁掉，恒为不可进。
[[nodiscard]] bool UpdateSideIdleMode();
/// ASA 进入 HPP2 / HPP3 笔模式。
[[nodiscard]] bool InStylusMode();

/// 宿主查询要切到的 AFE 频点 / 扫描率：返回 false 时 *index 有效。
[[nodiscard]] bool GetAfeScanFreqIndex(uint8_t* index);
[[nodiscard]] bool GetAfeScanRateIndex(uint8_t* index);
/// 宿主是否需要重新校准 AFE。
[[nodiscard]] bool IsReCalibNeeded();

}  // namespace Solvers::Tsa::Core::Tsa
