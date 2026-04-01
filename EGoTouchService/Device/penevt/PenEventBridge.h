#pragma once

#include "btmcu/BtHidChannel.h"
#include "btmcu/PenUsbTypes.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mutex>

namespace Himax::Pen {

/// PenEventBridge — BT MCU 事件通道 (col00)
///
/// 负责：
///   - USB HID col00 设备发现（SetupDi GUID 枚举）
///   - 初始握手 (0x7101 + 0x7701)
///   - 事件帧读取 + 自动 ACK (0x8001)
///   - 0x7B InitParam → 0x7D01 回显
///   - 上层事件回调分发
class PenEventBridge : public BtHidChannel {
public:
    PenEventBridge() = default;

    /// 设置 MCU 事件回调（线程安全）。回调从事件读取线程发起，不得长时间阻塞。
    void SetEventCallback(PenEventCallback cb);
    /// 设置状态事件句柄（用于通知 App 侧刷新状态）
    void SetNotifyEvent(HANDLE h) { m_notifyEvent = h; }

    /// 手动触发握手（0x7101 + 0x7701），通常无需手动调用。
    void RunHandshake();

protected:
    std::optional<std::wstring> FindDevicePath() override;
    void OnConnected() override;
    void OnPacketReceived(const std::vector<uint8_t>& packet) override;
    const char* ChannelName() const override { return "PenEventBridge"; }

private:
    static const GUID kEventDeviceGuid;
    static int GetAckCode(uint8_t eventCode);

    void SendRawPacket(const std::vector<uint8_t>& pkt);
    void SendAck(uint8_t ackCode);
    void SendInitParamEcho(const uint8_t* data, size_t len);

    mutable std::mutex m_cbMutex;
    PenEventCallback m_eventCallback;
    HANDLE m_notifyEvent = nullptr;
};

} // namespace Himax::Pen
