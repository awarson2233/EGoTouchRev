#pragma once
// TouchSink.h — 宿主把手指触点交给系统的输出端口。
//
// 报告就是 VhfTouchPacket::Build（Device/vhf/VhfReporterTouchPacketHelper.h）打出的 32 字节触摸报告，
// 各平台按同一个布局描述成一个 HID 触摸屏：
//   win32/  还在 VhfReporter 里（VHF 注入驱动），没有经这个接口
//   linux/  uhid（/dev/uhid），由内核的 hid-multitouch 生成触摸屏输入设备
//
// 报告布局（Report ID 1，混合模式：一帧超过 5 个触点时分两包，触点总数只在第一包里）：
//   [0]       Report ID
//   [1..30]   5 个触点，每个 6 字节：状态（bit0 按下、bit1 置信）、ID、X u16（0..16000）、Y u16（0..25600）
//   [31]      本帧触点总数（第二包为 0）

#include <cstddef>
#include <cstdint>
#include <memory>

namespace Port {

class ITouchSink {
public:
    virtual ~ITouchSink() = default;

    virtual bool IsValid() const = 0;

    /// 写一包 32 字节的触摸报告
    virtual bool WriteReport(const uint8_t* report, size_t len) = 0;
    /// 抬起所有还按着的触点（停止出帧、退出时调用）
    virtual void ReleaseAll() = 0;

    /// 最近一次失败的平台错误码，只用于日志
    virtual uint32_t GetError() const = 0;
};

/// 创建触摸屏输出设备。失败也返回对象（IsValid() == false，GetError() 给出原因）。
std::unique_ptr<ITouchSink> CreateTouchSink();

} // namespace Port
