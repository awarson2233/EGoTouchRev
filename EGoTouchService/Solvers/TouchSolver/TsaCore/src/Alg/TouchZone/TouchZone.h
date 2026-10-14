#pragma once
// 触摸区域（原 Detection/TZ.c，原厂 src/Alg/TouchZone/TZ_*.c）：每帧以每个尚未被覆盖的峰为起点做区域生长，
// 得到一个个触摸区域（TouchZoneUnit），统计信号矩、外接框、贴边信息与轮廓，判定区域形状（手指、大手指、多指、
// 大物体），处理握持合并与虚拟峰，最后把区域登记成当前帧的触点。
//
//   StateMachine.cpp  每帧流程、区域图的膨胀 / 腐蚀、握持年龄、区域生长门限
//   Traversal.cpp     区域生长（逐格累计）与峰过滤
//   Common.cpp        质心 / 椭圆轴、区域属性、登记成触点
//   Shape.cpp         形状判定（DFS、形状消抖、大物体）
//   GripMerge.cpp     握持区域的合并与拆分
//   VirtualPeak.cpp   虚拟峰
//   Contour.cpp       轮廓与圆度 / 椭圆度

#include <cstdint>

namespace Solvers::Tsa::Core {
struct TouchZoneUnit;
}
namespace Solvers::Tsa::Core::SubTouchZone {
struct Unit;
}

namespace Solvers::Tsa::Core::TouchZone {

[[nodiscard]] bool Malloc();
void Free();
void Init();
/// 每帧：由峰表生成触摸区域并登记成触点（触点表从 0 开始重建）。
void Process();

/// 二阶矩 → 椭圆长短轴（Q8.8，√(6λ)）与长轴方向（度）。m0 为 0 时按 1 算；轴长不到 0.5 时取 0x80。
void AxisFromMoments(double m0, double m1Dim1, double m1Dim2, double m2Dim1Dim2, double m2Dim1, double m2Dim2,
                     uint16_t* majorAxis, uint16_t* minorAxis, int8_t* angle);
/// 区域异常检测用的六个比值（TouchZoneUnit::exceptionRatio）。
void GetExceptionRatio(TouchZoneUnit& zone);
/// 两峰中点的信号相对较弱峰的比值（×64）；中点不比较弱峰低时为 64。
[[nodiscard]] uint8_t CentToMinPkRatio(TouchZoneUnit& zone, uint8_t peakA, uint8_t peakB);
/// SubTZ：握持拆分出的子区域形状异常或含多个峰时撤回拆分。
void SeparateZoneRegret(TouchZoneUnit& zone, const SubTouchZone::Unit& sub);
/// SubTZ：遇到握持格时把它作为虚拟峰并入区域（每个区域一次）。
void TryExtraMergePeak(TouchZoneUnit& zone, uint8_t dim1, uint8_t dim2);

}  // namespace Solvers::Tsa::Core::TouchZone
