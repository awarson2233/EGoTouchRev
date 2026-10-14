#pragma once
// 参数模块（原 src/Prmt/TSA_Prmt.c 及各子表）。参数分几层：
//   - 机型参数 FlashPrmt：装入机型时整块复制，之后由下面两张表每帧写回其中可调的字段；
//   - 调参表 TtPrmt：机型参数里可调字段的当前值、上下限与说明，每帧写回机型参数；
//   - 运行时参数 RtPrmt：场景处理（TSA_ScenarioProcess）按键名改写的少数参数，每帧从机型参数重建再写回；
//   - 常量 ConstPrmt：由机型参数推导的换算常数，只在初始化（及分辨率变化）时计算；
//   - 动态门限 DynPrmt：每帧按触摸模式、噪声与扫描状态计算的门限。
// 各表的存储布局见 Prmt.h。

#include <cstdint>

namespace Solvers::Tsa::Core {

namespace Prmt {

/// 装入本机型（W273AS2700）的参数后重新初始化全部参数表。总是成功。
bool Init();
/// 宿主指定机型时调用；名字为空时失败。缺省机型 "DFLT000000" 的阵列尺寸取自 AFE。
bool LoadProject(const char* name);
/// 重新计算动态门限（TSA 复位时）。
void Reset();
/// 每帧开头：刷新 CMF 门限。
void PreProcess();
/// 每帧峰检测之前：调参表与运行时参数写回机型参数，再计算动态门限。
void Process();
/// 机型名，即机型参数表开头的字符串。
[[nodiscard]] const char* GetProjectName();

}  // namespace Prmt

namespace ConstPrmt {
void Init();
}  // namespace ConstPrmt

namespace DynPrmt {
void Init();
void PreProcess();
void Process();
}  // namespace DynPrmt

namespace RtPrmt {
void Init();
void Process();
/// 按键名改写运行时参数的当前值并钳在上下限内。键名不存在时返回 false。
bool SetKeyValue(const char* key, int32_t value);
}  // namespace RtPrmt

namespace TtPrmt {
void Init();
void Process();
}  // namespace TtPrmt

}  // namespace Solvers::Tsa::Core
