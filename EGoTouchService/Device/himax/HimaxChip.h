#pragma once
#include "Device.h"
#include "HimaxProtocol.h"
#include "HimaxRegisters.h"
#include "himax/HimaxAfe.h"
#include <atomic>
#include <cstdint>
#include <memory>
#include <array>
#include <string>
#include <thread>
#include <winnt.h>

namespace Himax {

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
            std::unique_ptr<HalDevice> m_interrupt;
            
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
            ChipResult<> init_buffers_and_register(void);
            
            ChipResult<> hx_hw_reset_ahb_intf(DeviceType type);
            ChipResult<> hx_sw_reset_ahb_intf(DeviceType type); 
            ChipResult<> hx_is_reload_done_ahb(void);
            ChipResult<> himax_mcu_reload_disable(uint8_t disable);
            ChipResult<> himax_mcu_read_FW_status(void);

            ChipResult<> hx_sense_on(bool isHwReset);
            ChipResult<> hx_sense_off(bool check_en);
            ChipResult<> himax_mcu_power_on_init(void);
            
            ChipResult<> himax_mcu_assign_sorting_mode(uint8_t* tmp_data);
            ChipResult<> himax_switch_data_type(DeviceType device, THP_INSPECTION_ENUM mode);
            ChipResult<> himax_switch_mode_inspection(THP_INSPECTION_ENUM mode);
            ChipResult<> himax_mcu_interface_on(void);
            ChipResult<> hx_set_N_frame(uint8_t nFrame);
            
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
            const std::array<uint8_t, 6000>& GetFrameBuffer() const { return back_data; }
            void SetAfeMode(THP_AFE_MODE mode) { afe_mode.store(mode); }

            ChipResult<> SetFrameReadPolicy(bool block, uint8_t timeoutMs);
            ChipResult<> SetFrameReadNormalPolicy();
            ChipResult<> SetFrameReadIdlePolicy();
            ChipResult<> NotifyTouchWakeup();

            Chip(const std::wstring& master_path, const std::wstring& slave_path, const std::wstring& interrupt_path);
            ~Chip(); // Add destructor for explicit cleanup
            
            bool IsReady(DeviceType type) const;
            
            ChipResult<> Init(void);
            ChipResult<> Deinit(bool check_en = true);
            void HoldReset();   // Suspend: pull reset low + IntClose, no bus traffic
            ChipResult<> check_bus(void);
            ChipResult<> GetFrame(void);
            void CancelPendingFrameRead();
    };
}
