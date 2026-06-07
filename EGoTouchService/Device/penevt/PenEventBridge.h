#pragma once

#include "btmcu/BtHidChannel.h"
#include "btmcu/PenUsbInitSession.h"
#include "btmcu/PenUsbTypes.h"

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace Himax::Pen {

/// PenEventBridge — BT MCU 事件通道 (col00)
///
/// 负责：
///   - USB HID col00 设备发现（SetupDi GUID 枚举）
///   - 初始握手 (0x7101 + 0x7701 + 0x7701)
///   - 事件帧读取 + 自动 ACK (0x8001)
///   - 0x7B InitParam → 0x7D01 回显
///   - 上层事件回调分发
class PenEventBridge : public BtHidChannel {
public:
    PenEventBridge() = default;
    ~PenEventBridge() override;

    /// 向 BT MCU 发送 SetScanMode 命令，通知笔切换扫描频率。
    /// 对应原厂 THP_Service::BtPen_SendPacket + ApDaemon::SetScanMode 的联合逻辑。
    /// @param freq1  新的 TX1 频率码
    /// @param freq2  新的 TX2 频率码
    /// @param mode   0=正常扫描, 3=检测模式（默认 0）
    /// @return true if packet was sent successfully
    bool SendScanMode(uint8_t freq1, uint8_t freq2, uint8_t mode = 0);

    /// 设置 MCU 事件回调（线程安全）。回调从事件读取线程发起，不得长时间阻塞。
    void SetEventCallback(PenEventCallback cb);
    /// 设置状态事件句柄（用于通知 App 侧刷新状态）
    void SetNotifyEvent(NativeEventHandle h) { m_notifyEvent = h; }

    /// 手动触发握手（0x7101 + 0x7701 + 0x7701），通常无需手动调用。
    void RunHandshake();

protected:
    std::optional<std::wstring> FindDevicePath() override;
    void OnConnected() override;
    void OnPacketReceived(const std::vector<uint8_t>& packet) override;
    const char* ChannelName() const override { return "PenEventBridge"; }

private:
    static int GetAckCode(uint8_t eventCode);

    bool SendRawPacket(const std::vector<uint8_t>& pkt);
    void SendAck(uint8_t eventCode, uint8_t ackCode);
    void ExecuteInitAction(PenUsbInitAction action);
    void AdvanceSessionFromEvent(uint8_t eventCode);

    bool SendQueryHardwareVersion();
    bool SendQueryPenStatus();
    bool SendFirstMcuStatusQuery();
    bool SendSecondMcuStatusQuery();
    bool SendInitProtocolParams();

    mutable std::mutex m_cbMutex;
    mutable std::mutex m_sessionMutex;
    mutable std::mutex m_txMutex;
    PenEventCallback m_eventCallback;
    NativeEventHandle m_notifyEvent = nullptr;
    PenUsbInitSession m_initSession;
};

} // namespace Himax::Pen
