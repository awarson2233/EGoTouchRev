#pragma once

#include "btmcu/BtHidChannel.h"
#include "btmcu/PenUsbInitSession.h"
#include "btmcu/PenUsbTypes.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>

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

    /// 发送原厂固定的 0x7D01 初始化参数，不使用动态扫描模式参数。
    bool SendFactoryInitProtocolParams();

    /// 设置 MCU 事件回调（线程安全）。回调从事件读取线程发起，不得长时间阻塞。
    void SetEventCallback(PenEventCallback cb);
    /// 设置状态事件句柄（用于通知 App 侧刷新状态）
    void SetNotifyEvent(NativeEventHandle h) {
        m_notifyEvent.store(h, std::memory_order_release);
    }

    /// 手动触发握手（0x7101 + 0x7701 + 0x7701），通常无需手动调用。
    void RunHandshake();

    /// 标记 MCU 链路已因主机电源转换（Suspend / DisplayOff / LidOff）失效。
    /// 休眠不会让 col00 HID 句柄失效，读循环因此不会重连，握手会话会停在
    /// Phase::Running，0x7D01 永不重发。该标志把"需要重握手"记到下一次唤醒。
    void MarkLinkStale();
    bool IsLinkStale() const {
        return m_linkStale.load(std::memory_order_acquire);
    }

    /// 唤醒时调用：若链路被标记为 stale，则重置握手会话并清除标志。
    /// 不发送任何数据包——后续由 MCU 自身的事件链驱动回 0x7D01。
    /// @return true 表示本次确实重新武装了会话。
    bool RearmHandshakeIfStale();

    bool SendQueryPenModule();
    bool SendQuerySerialNumber();
    bool SendQueryHardwareVersion();
    bool SendQueryFirmwareVersion();
    bool SendQueryPenStatus();
    bool SendFirstMcuStatusQuery();
    bool SendSecondMcuStatusQuery();
    bool SendPairInfoSet(uint8_t value);

    /// 被动重握手的升级窗口：唤醒后若 MCU 在此时限内没有把会话推进到
    /// 0x7D01，则强制重连并改走主动握手。
    static constexpr std::chrono::milliseconds kHandshakeRecoveryTimeout{5000};

protected:
    std::optional<std::wstring> FindDevicePath() override;
    void OnConnected() override;
    void OnPacketReceived(std::span<const uint8_t> packet) override;
    void OnIdleTick() override;
    const char* ChannelName() const override { return "PenEventBridge"; }

private:
    static int GetAckCode(uint8_t eventCode);

    bool SendRawPacket(std::span<const uint8_t> pkt);
    void SendAck(uint8_t eventCode, uint8_t ackCode);
    void ExecuteInitAction(PenUsbInitAction action);
    void AdvanceSessionFromEvent(uint8_t eventCode);

    mutable std::mutex m_cbMutex;
    mutable std::mutex m_sessionMutex;
    mutable std::mutex m_txMutex;
    std::shared_ptr<const PenEventCallback> m_eventCallback;
    std::atomic<NativeEventHandle> m_notifyEvent{nullptr};
    std::atomic<bool> m_linkStale{false};
    PenUsbInitSession m_initSession;
    /// 非零表示"已被动重武装、等待 MCU 推进握手"的截止时刻。
    /// 与 m_initSession 同受 m_sessionMutex 保护。
    std::chrono::steady_clock::time_point m_handshakeDeadline{};
};

} // namespace Himax::Pen
