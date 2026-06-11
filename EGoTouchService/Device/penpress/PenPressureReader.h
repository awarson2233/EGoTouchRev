#pragma once

#include "btmcu/BtHidChannel.h"
#include "btmcu/PenUsbTypes.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <span>

namespace Himax::Pen {

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
    ~PenPressureReader() override;

    /// 设置压感回调（每收到一个 'U' 报文触发，参数为完整四槽压力包）
    using PressureCallback = std::function<void(const PenPressureStats& stats)>;
    void SetPressureCallback(PressureCallback cb);
    /// 设置状态事件句柄（用于通知 App 侧刷新状态）
    void SetNotifyEvent(NativeEventHandle h) { m_notifyEvent = h; }

    /// 获取最新压力统计（原子读，线程安全）
    PenPressureStats GetPressureStats() const;
    void SetPressureRangeMode(PenPressureRangeMode mode);
    PenPressureRangeMode GetPressureRangeMode() const;

protected:
    std::optional<std::wstring> FindDevicePath() override;
    void OnPacketReceived(std::span<const uint8_t> packet) override;
    const char* ChannelName() const override { return "PenPressureReader"; }

private:
    static constexpr const wchar_t* kPressureHidMatch = L"vid_12d1&pid_10b8&mi_00&col01";

    mutable std::mutex m_cbMutex;
    std::shared_ptr<const PressureCallback> m_pressureCallback;

    void ApplyPressureModeLocked(PenPressureRangeMode mode);

    mutable std::mutex m_statsMutex;
    PenPressureStats m_stats;
    NativeEventHandle m_notifyEvent = nullptr;
};

} // namespace Himax::Pen
