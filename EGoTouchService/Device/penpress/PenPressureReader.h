#pragma once

#include "btmcu/BtHidChannel.h"
#include <cstdint>
#include <functional>
#include <mutex>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace Himax::Pen {

/// 压力统计数据（从 'U' 报文解析）
struct PenPressureStats {
    uint16_t press[4] = {0, 0, 0, 0};  ///< 四通道压力值
    uint8_t  freq1    = 0;              ///< BT 跳频频率1
    uint8_t  freq2    = 0;              ///< BT 跳频频率2
    uint8_t  reportType = 0;            ///< 报文类型（通常 'U' = 0x55）
};

/// PenPressureReader — BT MCU 压力通道 (col01)
///
/// 负责：
///   - USB HID col01 设备发现（VID/PID/MI/col 枚举）
///   - 连续读取 'U' (0x55) 报文
///   - 压力和 BT 频率数据解析
///   - 上层压感回调分发
class PenPressureReader : public BtHidChannel {
public:
    PenPressureReader() = default;

    /// 设置压感回调（每收到一个 'U' 报文触发）
    using PressureCallback = std::function<void(uint16_t press)>;
    void SetPressureCallback(PressureCallback cb);
    /// 设置状态事件句柄（用于通知 App 侧刷新状态）
    void SetNotifyEvent(HANDLE h) { m_notifyEvent = h; }

    /// 获取最新压力统计（原子读，线程安全）
    PenPressureStats GetPressureStats() const;

protected:
    std::optional<std::wstring> FindDevicePath() override;
    void OnPacketReceived(const std::vector<uint8_t>& packet) override;
    const char* ChannelName() const override { return "PenPressureReader"; }

private:
    static constexpr const wchar_t* kPressureHidMatch = L"vid_12d1&pid_10b8&mi_00&col01";

    mutable std::mutex m_cbMutex;
    PressureCallback m_pressureCallback;

    mutable std::mutex m_statsMutex;
    PenPressureStats m_stats;
    HANDLE m_notifyEvent = nullptr;
};

} // namespace Himax::Pen
