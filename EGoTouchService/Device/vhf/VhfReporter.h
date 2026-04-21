#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>

#include "SolverTypes.h"

#ifndef _WINDOWS_
#include <Windows.h>
#endif

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
    void Dispatch(Solvers::HeatmapFrame& frame);

    /// 独立手写笔写入
    void DispatchStylus(const Solvers::StylusPacket& packet);

    /// 独立手指写入 (含 BuildTouchReports)
    void DispatchTouch(Solvers::HeatmapFrame& frame);

    // 开关
    void SetEnabled(bool v) { m_enabled.store(v, std::memory_order_relaxed); }
    bool IsEnabled() const { return m_enabled.load(std::memory_order_relaxed); }

    void SetTransposeEnabled(bool v) {
        m_transpose.store(v, std::memory_order_relaxed);
    }
    bool IsTransposeEnabled() const {
        return m_transpose.load(std::memory_order_relaxed);
    }

    void SetEraserState(uint8_t v) {
        m_eraserState.store(v, std::memory_order_relaxed);
    }

    bool IsDeviceOpen() const;
    void Close();

private:
    bool UpdateTouchState(bool hasTouch);

    void WriteTouchPacketsLocked(
        const std::array<Solvers::TouchPacket, 2>& packets);
    void WriteTouchAllUpLocked();
    void WriteStylusPacketLocked(const uint8_t* data, size_t len);

    bool EnsureDeviceOpenLocked();
    void CloseDeviceLocked();
    void ScheduleReopenLocked();
    bool WritePacketLocked(const uint8_t* data, size_t len,
                           const char* tag);

    static constexpr auto kReopenBackoff = std::chrono::milliseconds(200);

    std::atomic<bool> m_enabled{true};
    std::atomic<bool> m_transpose{false};
    std::atomic<bool> m_hadTouchLastFrame{false};
    std::atomic<uint8_t> m_eraserState{0};

    mutable std::mutex m_mu;
    HANDLE m_handle = INVALID_HANDLE_VALUE;
    std::chrono::steady_clock::time_point m_nextOpenAttempt{};

    static const GUID kVhfGuid;
};
