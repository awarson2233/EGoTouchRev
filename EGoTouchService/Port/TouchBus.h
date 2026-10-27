#pragma once
// TouchBus.h — 宿主访问触摸芯片的总线端口。
//
// 芯片的协议层（HimaxProtocol / HimaxChip）只通过 ITouchBus 收发；每个平台在 Port/<平台>/ 下给出
// CreateTouchBus 的实现：
//   win32/  SPBTESTTOOL 驱动的设备对象（DeviceIoControl）
//   linux/  himax-thp 内核驱动的字符设备（/dev/himax_thp）

#include "common/DeviceError.h"

#include <cstdint>
#include <filesystem>
#include <memory>

namespace Port {

/// 芯片的两路总线通道。Master 出手指侧的 heatmap 帧，Slave 出笔侧的帧；两颗芯片各在一路 SPI 上，
/// 功能码也不同（Master F2 写 / F3 读，Slave F4 写 / F5 读）。
enum class TouchChannel : uint8_t { Master, Slave };

class ITouchBus {
public:
    virtual ~ITouchBus() = default;

    virtual bool IsValid() const = 0;

    /// 总线读：发 [功能码, cmd, dummy]，收回 len 字节数据
    virtual ChipResult<> ReadBus(uint8_t cmd, uint8_t* data, uint32_t len) = 0;
    /// 总线写：发 [功能码, cmd, addr(4 字节，可为空), data(len 字节，可为空)]
    virtual ChipResult<> WriteBus(uint8_t cmd, const uint8_t* addr, const uint8_t* data, uint32_t len) = 0;

    /// 取一帧，只尝试一次：等多久由 SetTimeOut 决定，读不到之后怎么重试是调用方的事
    virtual ChipResult<> GetFrame(void* buffer, uint32_t outLen, uint32_t* retLen) = 0;
    virtual ChipResult<> SetTimeOut(uint32_t millisecond) = 0;
    /// true：GetFrame 阻塞等帧；false：没有帧立即返回
    virtual ChipResult<> SetBlock(bool status) = 0;
    /// 复位一次：复位脚拉低 holdMs 后放开（两颗芯片共用这根脚）。只有 Master 通道支持。
    /// Windows 按 SET_RESET 低 → 等 → 高 照做；Linux 交给驱动，脉宽不足 HIMAX_THP_RESET_MIN_MS 的按它算
    virtual ChipResult<> ResetPulse(uint32_t holdMs) = 0;
    /// 打开 / 关闭帧中断通知
    virtual ChipResult<> IntOpen() = 0;
    virtual ChipResult<> IntClose() = 0;
    /// 面板电源状态字，bit0 = 屏幕已上电（TDDI 的触摸部分跟着它上下电）
    virtual ChipResult<> ReadPanelStatus(uint8_t* data, uint32_t len) = 0;

    /// 最近一次失败的平台错误码（Win32 错误码 / errno），只用于日志
    virtual uint32_t GetError() const = 0;
    virtual bool IsTimeoutError() const = 0;
    /// 最近一次失败是不是被 RequestCancel 取消的
    virtual bool IsCancelledError() const = 0;
    /// 只发取消请求，不碰任何成员状态；可以从任意线程调用，用来把阻塞在读帧里的 worker 唤醒。
    virtual void RequestCancel() noexcept = 0;
};

/// 打开 path 指向的总线设备。打开失败也返回对象（IsValid() == false，GetError() 给出原因），
/// 方便调用方统一记录错误后重试。
std::unique_ptr<ITouchBus> CreateTouchBus(const std::filesystem::path& path, TouchChannel channel);

} // namespace Port
