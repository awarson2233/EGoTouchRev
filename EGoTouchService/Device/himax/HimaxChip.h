#pragma once
#include "common/DeviceError.h"
#include "HimaxRegisters.h"
#include "himax/HimaxAfe.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

namespace Himax {

    enum class DeviceType;
    class HalDevice;

    enum class ConnectionState {
        Unconnected = 0,    // 未连接/句柄未打开
        Connected = 1,      // 通信就绪
        Error = 2           // 通信断开错误
    };

    enum class THP_INSPECTION_ENUM {
        HX_RAWDATA = 0,
        HX_ACT_IDLE_RAWDATA = 1,
        HX_LP_IDLE_RAWDATA = 2,
        HX_BACK_NORMAL = 3,
        EGO_RAWDATA = 4,
    };

    class Chip {
        private:
            std::unique_ptr<HalDevice> m_master;
            std::unique_ptr<HalDevice> m_slave;
            // 设备路径留着，句柄失效后可以重开
            std::wstring m_masterPath;
            std::wstring m_slavePath;
            // 保护两个设备指针本身：worker 重开句柄时，别的线程可能正在发取消请求
            mutable std::mutex m_devMu;
            const char* m_lastInitFailStep = "";
            bool m_autoIdle = true;                     // 连续无输入帧后由 Chip 自己进 IDLE

            // ── 读帧状态（对应 thp_afe_get_frame 的几个全局量）──────────────
            bool m_idleRetry = false;                   // g_FrameRetryEnableFlag：IC 在 IDLE，读不到帧先原地重读
            uint32_t m_readStalls = 0;                  // g_FrameRetryTimeoutCount：重开中断的累计次数，读到有效帧清零
            uint32_t m_handZeroFrames = 0;              // g_HandAllZeroCount
            uint32_t m_handChecksumErrors = 0;          // g_HandChecksumErrorCount
            uint32_t m_penZeroFrames = 0;               // g_PenAllZeroCount
            uint32_t m_penChecksumErrors = 0;           // g_PenChecksumErrorCount
            const char* m_sosReason = "";
            std::atomic<bool> m_frameReadActive{false};  // worker 正在 GetFrame 里（取消请求只在这时才发）

            uint8_t current_slot;
            uint32_t m_zeroFrameCount = 0;   // 连续无输入帧计数（idle 进入判断）
            uint32_t m_frameCount = 0;       // 帧计数器（2:1 交错用）
            bool     m_stylusActive = false; // 帧级 stylus 检测驱动 2:1 交错
            
            ic_operation        pic_op{};
            fw_operation        pfw_op{};
            flash_operation     pflash_op{};
            sram_operation      psram_op{};
            driver_operation    pdriver_op{};
            zf_operation        pzf_op{};
        
            
            ChipResult<HalDevice*> SelectDevice(DeviceType type);

            // ── Init/Deinit & recovery internals ─────────────────────────
            ChipResult<> init_buffers_and_register(void);
            ChipResult<> hx_hw_reset_ahb_intf(DeviceType type);
            ChipResult<> hx_sw_reset_ahb_intf(DeviceType type);
            ChipResult<> hx_sense_on(bool isHwReset);
            ChipResult<> hx_sense_off(bool check_en);
            ChipResult<> himax_mcu_power_on_init(void);
            ChipResult<> himax_mcu_interface_on(void);
            ChipResult<> hx_is_reload_done_ahb(void);
            ChipResult<> himax_mcu_reload_disable(uint8_t disable);
            ChipResult<> himax_mcu_read_FW_status(void);

            // ── Mode/config internals ────────────────────────────────────
            ChipResult<> himax_mcu_assign_sorting_mode(uint8_t* tmp_data);
            ChipResult<> himax_switch_data_type(DeviceType device, THP_INSPECTION_ENUM mode);
            ChipResult<> himax_switch_mode_inspection(THP_INSPECTION_ENUM mode);
            ChipResult<> hx_set_N_frame(uint8_t nFrame);
            ChipResult<> InitFailed(const char* step, const ChipResult<>& res);
            ChipResult<> ApplyFrameReadPolicy();
            void ResetFrameReadState();

        public:
            bool m_lastMasterWasRead = true; // 上一帧 master 是否实际被读取（供 DeviceRuntime 填写 masterWasRead）
            alignas(64) std::array<uint8_t, 6000> back_data{};
            THP_INSPECTION_ENUM m_inspection_mode;
            std::atomic<THP_AFE_MODE> afe_mode{THP_AFE_MODE::Normal};

            // ── AfeController (owns stylus state and AFE command dispatch) ──
            AfeController m_afe;

            // 触点/手写笔检测：从帧数据判断是否有输入
            bool isFingerDetected() const;
            bool isStylusDetected() const;

            
            // 主机的连接状态机
            std::atomic<ConnectionState> m_connState{ConnectionState::Unconnected};

            // 用于日志的前缀转换辅助函数
            const char* GetStateStr() const {
                switch(m_connState.load()) {
                    case ConnectionState::Unconnected: return "Unconnected";
                    case ConnectionState::Connected: return "Connected";
                    case ConnectionState::Error: return "Error";
                    default: return "Unknown";
                }
            }

            // ── AfeController 访问接口 ──────────────────────────────────
            // 这些方法供 AfeController 使用，避免将 Chip 内部全部暴露
            HalDevice* GetMasterDevice() { return m_master.get(); }
            uint8_t& GetCurrentSlot() { return current_slot; }
            ConnectionState GetConnectionState() const { return m_connState.load(); }
            /// thp_afe_enter_idle 的后半段：命令发出去之后，读不到帧先按 IDLE 的方式原地重读
            void ArmIdleRetry() noexcept { m_idleRetry = true; }

            // ── Runtime Facade (stable boundary for DeviceRuntime) ───────
            ChipResult<> SendAfeCommand(command cmd);
            bool IsStylusConnected() const;
            const std::array<uint8_t, 6000>& GetFrameBuffer() const { return back_data; }
            bool GetLastMasterWasRead() const { return m_lastMasterWasRead; }
            uint16_t GetLastFrameTimestamp() const;

            Chip(const std::wstring& master_path, const std::wstring& slave_path);
            ~Chip(); // Add destructor for explicit cleanup
            
            bool IsReady(DeviceType type) const;
            
            ChipResult<> Init(void);
            ChipResult<> Deinit(bool check_en = true);
            /// 静默：关闭两路中断通道，不向 IC 发任何总线命令，也不动 reset。
            /// 灭屏待机与重新建链之前都从这里经过，保证 Init 总是从同一个起点开始。
            void Quiesce();
            /// 关闭并重新打开两个设备句柄（对应原厂的整只守护重启）。必须先 Quiesce。
            /// 只能在 worker 线程调用。
            bool ReopenDevices();
            /// 面板电源真值：SPB 驱动的 ACPI 读取，bit0 = 屏幕已上电。读取失败返回空。
            std::optional<bool> ReadPanelPower();
            /// true：连续无输入帧够多时 Chip 自己进 IDLE（TouchPipeline 用）。TSACore 解算时关掉，
            /// 改由 TSACore 的 IDLE 请求下发 EnterIdle。
            void SetAutoIdleEnabled(bool enabled) noexcept { m_autoIdle = enabled; }
            /// IC 在最近一帧的帧尾里自报的模式（word0 == 1 即 IDLE）。宿主不维护自己的 IDLE 状态。
            THP_AFE_MODE GetAfeMode() const noexcept { return afe_mode.load(); }
            /// 总线就绪探测（对应 himax_thp_drv!checkBusReady）：两路都读寄存器 0x13，
            /// IOCTL 成功且读到非零才算就绪。面板 / 触摸部分没上电时 IOCTL 照样成功，但读到 0。
            struct BusProbe {
                bool ioOk = false;
                uint8_t master = 0;
                uint8_t slave = 0;
                uint32_t win32Error = 0;
                bool Ready() const noexcept { return ioOk && master != 0 && slave != 0; }
            };
            BusProbe ProbeBus();
            ChipResult<> check_bus(void);
            /// 最近一次 Init 失败在哪一步（成功后为空串）
            const char* LastInitFailStep() const noexcept { return m_lastInitFailStep; }

            /// 取一帧的结果，对应 thp_afe_get_frame 的三种出口。
            enum class FrameRead : uint8_t {
                Frame,    // back_data 里是新的一帧（master 是否有效看 GetLastMasterWasRead）
                NoData,   // 这一轮没有可用的数据（两路都无效，或读帧被取消），直接再取
                Sos,      // 原厂的 status 0x40：链路要整体重启，原因见 LastSosReason
            };
            FrameRead GetFrame(void);
            const char* LastSosReason() const noexcept { return m_sosReason; }
            /// 从任意线程调用：worker 正阻塞在 GetFrame 里时让它以 NoData 返回，其余时候什么都不做
            /// （不会打断建链或命令的寄存器读写）。
            void CancelPendingFrameRead();
    };
}
