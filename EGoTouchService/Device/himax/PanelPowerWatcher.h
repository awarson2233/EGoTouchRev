#pragma once
#include <atomic>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <thread>

namespace Himax {

    /// 面板电源通道，对应 THP_Service!SpiInterruptNotificationThread。
    ///
    /// 在 master 设备上单独开一个句柄，循环挂起 SPB 驱动的中断通知（IOCTL 0x4001C20）。
    /// 这个通知与帧中断无关：正常出帧时它一直挂起，只在面板电源变化时完成。每次完成后读
    /// ACPI（IOCTL 0x4001C38）并把 bit0（面板已上电）交给回调。
    ///
    /// 句柄归监听线程独占，与取帧用的 master / slave 句柄互不影响。
    class PanelPowerWatcher {
    public:
        /// panelOn：ACPI bit0；raw：ACPI 读回的首字节，只用于记日志
        using Callback = std::function<void(bool panelOn, uint8_t raw)>;

        explicit PanelPowerWatcher(std::wstring path);
        ~PanelPowerWatcher();
        PanelPowerWatcher(const PanelPowerWatcher&) = delete;
        PanelPowerWatcher& operator=(const PanelPowerWatcher&) = delete;

        /// 启动监听线程。回调在监听线程上执行：先上报一次当前电平，之后电平每变一次上报一次。
        void Start(Callback cb);
        /// 取消挂起的通知并等线程退出。
        void Stop();

        /// 累计收到的中断通知次数
        uint32_t Notifications() const noexcept { return m_notifications.load(std::memory_order_relaxed); }

    private:
        void Run();

        std::wstring m_path;
        Callback m_cb;
        std::thread m_thread;
        void* m_stopEvent = nullptr;   // HANDLE，手动复位
        std::atomic<uint32_t> m_notifications{0};
    };

} // namespace Himax
