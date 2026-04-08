#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

#include "EngineTypes.h"

#ifndef _WINDOWS_
#include <Windows.h>
#endif
#include <SetupAPI.h>

/// VhfReporter — 负责将 Pipeline 输出的 TouchPacket /
/// StylusPacket 通过 VHF HID 注入器驱动写入系统。
/// 入口：Dispatch(HeatmapFrame&)  —— Worker 一行调用。
class VhfReporter {
public:
    VhfReporter();
    ~VhfReporter();
    VhfReporter(const VhfReporter&) = delete;
    VhfReporter& operator=(const VhfReporter&) = delete;

    /// 主入口 (legacy, 后向兼容)
    void Dispatch(Engine::HeatmapFrame& frame);

    /// 独立手写笔写入
    void DispatchStylus(const Engine::StylusPacket& packet);

    /// 独立手指写入 (含 BuildTouchReports)
    void DispatchTouch(Engine::HeatmapFrame& frame);

    // 开关
    void SetEnabled(bool v) { m_enabled.store(v); }
    bool IsEnabled() const { return m_enabled.load(); }

    void SetTransposeEnabled(bool v) { m_transpose.store(v); }
    bool IsTransposeEnabled() const { return m_transpose.load(); }

    void SetEraserState(uint8_t v) { m_eraserState.store(v); }

    bool IsDeviceOpen() const;
    void Close();

private:
    void BuildTouchReports(Engine::HeatmapFrame& frame);
    void ApplyStylusPostTransform(std::array<uint8_t, 17>& bytes);
    bool EnsureDeviceOpen();
    void CloseDevice();
    void ReopenDevice();
    bool WritePacket(const uint8_t* data, size_t len,
                     const char* tag);

    std::atomic<bool> m_enabled{true};
    std::atomic<bool> m_transpose{false};
    std::atomic<bool> m_hadTouchLastFrame{false};
    std::atomic<uint8_t> m_eraserState{0};

    std::mutex m_mu;
    HANDLE m_handle = INVALID_HANDLE_VALUE;

    static const GUID kVhfGuid;
};

