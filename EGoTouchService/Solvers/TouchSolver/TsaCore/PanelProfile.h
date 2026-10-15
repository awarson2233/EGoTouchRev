#pragma once
// 面板机型：显示驱动 OTP 里烧的 project id（Himax::Chip 第一次建链时读出）。原厂按它选两样东西：
//   himax_thp_drv!thp_afe_open_project   PixelGain 表：W273AS2700 / 2710 用 CSOT 的，1300 / 1310 用 BOE 的
//   ApDaemon → TSA_InitAFEProject        机型参数表：TSAPrmt 按名字找，2710 没有专表、落到缺省的 DFLT000000

#include <cstdint>
#include <string_view>

namespace Solvers::Tsa {

enum class PanelGain : uint8_t { Csot, Boe };

/// OTP 没读出来时用的机型（原厂此时打不开 AFE，这里按最常见的 CSOT 面板继续）
inline constexpr std::string_view kDefaultPanelProjectId = "W273AS2700";

/// getProjectID_OTP 认的机型；别的 id 原厂读到也当失败。
inline constexpr std::string_view kPanelProjectIds[] = {"W273AS2700", "W273AS2710", "W273AS1310", "W273AS1300"};

/// 读出的 id 不认识（或为空）时换成缺省机型。
constexpr std::string_view ResolvePanelProjectId(std::string_view projectId) noexcept {
    for (const std::string_view id : kPanelProjectIds) {
        if (id == projectId) return id;
    }
    return kDefaultPanelProjectId;
}

/// thp_afe_open_project 的面板分支；不认识的 id 原厂直接失败，这里按 CSOT
constexpr PanelGain GainForProject(std::string_view projectId) noexcept {
    return projectId == "W273AS1300" || projectId == "W273AS1310" ? PanelGain::Boe : PanelGain::Csot;
}

}  // namespace Solvers::Tsa
