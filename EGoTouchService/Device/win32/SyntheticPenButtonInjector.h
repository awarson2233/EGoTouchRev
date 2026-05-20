#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

/// Win32 虚拟笔按键注入器 — 使用 CreateSyntheticPointerDevice +
/// InjectSyntheticPointerInput 注入笔杆按键或橡皮擦脉冲。
///
/// 所有注入均为边沿触发（瞬时），不锁存。适合笔硬件只发单次手势事件的场景。
class SyntheticPenButtonInjector {
public:
    SyntheticPenButtonInjector() = default;
    ~SyntheticPenButtonInjector();

    SyntheticPenButtonInjector(const SyntheticPenButtonInjector&) = delete;
    SyntheticPenButtonInjector& operator=(const SyntheticPenButtonInjector&) = delete;

    /// 确保虚拟笔设备已创建。首次调用时初始化，后续调用立即返回。
    bool EnsureDevice();

    /// 注入一次笔杆按键脉冲
    bool InjectBarrelPulse(POINT screenPt);

    /// 注入一次橡皮擦脉冲
    bool InjectEraserPulse(POINT screenPt);

    /// 注入 Win+F22 快捷键
    bool InjectWinF22Shortcut();

    /// 设备是否已就绪
    bool IsReady() const { return m_device != nullptr; }

private:
    bool InjectPenPulse(POINT screenPt, UINT32 penFlags, UINT32 extraPointerFlags);

    HSYNTHETICPOINTERDEVICE m_device = nullptr;
};
