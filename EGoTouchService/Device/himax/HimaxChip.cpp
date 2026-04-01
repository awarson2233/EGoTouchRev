/*
 * @Author: Detach0-0 detach0-0@outlook.com
 * @Date: 2025-12-05 11:45:27
 * @LastEditors: Detach0-0 detach0-0@outlook.com
 * @LastEditTime: 2026-01-09 16:37:22
 * @FilePath: \EGoTouchRev-vsc\HimaxChipCore\source\HimaxChip.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "Device.h"
#include "himax/HimaxChip.h"
#include "himax/HimaxProtocol.h"
#include "Logger.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <string>
#include <synchapi.h>
#include <vector>
#include <windows.h>

namespace {

/**
 * @brief 将地址值按指定长度解析为小端序字节数组
 * @param addr 地址值
 * @param cmd 目标缓冲区
 * @param len 目标长度 (1, 2, 或 4)
 */
void himax_parse_assign_cmd(uint32_t addr, uint8_t *cmd, int len) {
    switch (len) {
    case 1:
        cmd[0] = static_cast<uint8_t>(addr & 0xFF);
        break;
    case 2:
        cmd[0] = static_cast<uint8_t>(addr & 0xFF);
        cmd[1] = static_cast<uint8_t>((addr >> 8) & 0xFF);
        break;
    case 4:
        cmd[0] = static_cast<uint8_t>(addr & 0xFF);
        cmd[1] = static_cast<uint8_t>((addr >> 8) & 0xFF);
        cmd[2] = static_cast<uint8_t>((addr >> 16) & 0xFF);
        cmd[3] = static_cast<uint8_t>((addr >> 24) & 0xFF);
        break;
    default:
        break;
    }
}

} // end anonymous namespace

namespace Himax {

Chip::Chip(const std::wstring& master_path, const std::wstring& slave_path, const std::wstring& interrupt_path)
    : pic_op(InitIcOperation()),
      pfw_op(InitFwOperation()),
      pflash_op(InitFlashOperation()),
      psram_op(InitSramOperation()),
      pdriver_op(InitDriverOperation()),
      pzf_op(InitZfOperation()),
      m_afe(*this)
{
    m_master = std::make_unique<HalDevice>(master_path.c_str(), DeviceType::Master);
    m_slave = std::make_unique<HalDevice>(slave_path.c_str(), DeviceType::Slave);
    m_interrupt = std::make_unique<HalDevice>(interrupt_path.c_str(), DeviceType::Interrupt);
    
    m_inspection_mode = THP_INSPECTION_ENUM::HX_RAWDATA;
    current_slot = 0;
}

ChipResult<> Chip::SetFrameReadPolicy(bool block, uint8_t timeoutMs) {
    auto apply_policy = [&](HalDevice* dev, const char* devName) -> ChipResult<> {
        if (!dev || !dev->IsValid()) {
            LOG_ERROR("Device", "Chip::SetFrameReadPolicy", GetStateStr(), "{} handle invalid", devName);
            return std::unexpected(ChipError::CommunicationError);
        }

        if (auto res = dev->SetBlock(block); !res) {
            LOG_ERROR("Device", "Chip::SetFrameReadPolicy", GetStateStr(),
                      "{} SetBlock({}) failed", devName, block ? 1 : 0);
            return res;
        }

        if (auto res = dev->SetTimeOut(timeoutMs); !res) {
            LOG_ERROR("Device", "Chip::SetFrameReadPolicy", GetStateStr(),
                      "{} SetTimeOut({}) failed", devName, timeoutMs);
            return res;
        }
        return {};
    };

    if (auto res = apply_policy(m_master.get(), "Master"); !res) return res;
    if (auto res = apply_policy(m_slave.get(), "Slave"); !res) return res;

    LOG_INFO("Device", "Chip::SetFrameReadPolicy", GetStateStr(),
             "Applied read policy: block={}, timeout={}ms", block ? 1 : 0, timeoutMs);
    return {};
}

ChipResult<> Chip::SetFrameReadNormalPolicy() {
    // Normal mode: blocking read with short timeout to absorb kernel frame-ready jitter.
    return SetFrameReadPolicy(true, 100);
}

ChipResult<> Chip::SetFrameReadIdlePolicy() {
    // Idle mode: keep blocking semantics and allow a short wait window.
    return SetFrameReadPolicy(true, 200);
}

ChipResult<> Chip::NotifyTouchWakeup() {
    if (m_connState.load() != ConnectionState::Connected) {
        return std::unexpected(ChipError::InvalidOperation);
    }

    if (afe_mode != THP_AFE_MODE::Idle) {
        return {};
    }

    if (auto res = SetFrameReadNormalPolicy(); !res) return res;
    afe_mode = THP_AFE_MODE::Normal;
    LOG_INFO("Device", "Chip::NotifyTouchWakeup", GetStateStr(),
             "===== IDLE EXIT ===== Touch wakeup → Normal mode.");
    return {};
}

/**
 * @brief 根据设备类型选择对应的 HalDevice 指针
 * @param type 设备类型 (Master/Slave/Interrupt)
 * @return HalDevice* 有效设备指针，若无效或类型错误则返回 nullptr
 */
ChipResult<HalDevice*> Chip::SelectDevice(DeviceType type) {
    HalDevice* dev = nullptr;
    switch (type) {
    case DeviceType::Master:
        dev = m_master.get();
        break;
    case DeviceType::Slave:
        dev = m_slave.get();
        break;
    case DeviceType::Interrupt:
        dev = m_interrupt.get();
        break;
    default:
        return std::unexpected(ChipError::InvalidOperation);
    }
    if (dev && dev->IsValid()) {
        return dev;
    }
    return std::unexpected(ChipError::CommunicationError);
}

/**
 * @brief 检查指定类型的设备是否已就绪且有效
 * @param type 设备类型
 * @return bool 是否就绪
 */
bool Chip::IsReady(DeviceType type) const {
    return const_cast<Chip*>(this)->SelectDevice(type).has_value();
}

/**
 * @brief 检查主从设备的总线连接状态
 * @return bool 主从设备总线均正常返回 true
 */
ChipResult<> Chip::check_bus(void) {
    uint8_t tmp_data[4]{};
    uint8_t tmp_data1[4]{};

    // Check Slave
    if (auto res = m_slave->ReadBus(0x13, tmp_data, 1); !res) {
        LOG_ERROR("Device", "Chip::check_bus", GetStateStr(), "Slave Bus Dead! Error: {:d}", (int)res.error());
        return std::unexpected(ChipError::CommunicationError);
    }
    LOG_INFO("Device", "Chip::check_bus", GetStateStr(), "Slave Bus Alive! (0x13: 0x{:02X})", tmp_data[0]);

    // Check Master
    if (auto res = m_master->ReadBus(0x13, tmp_data1, 1); !res) {
        LOG_ERROR("Device", "Chip::check_bus", GetStateStr(), "Master Bus Dead! Error: {:d}", (int)res.error());
        return std::unexpected(ChipError::CommunicationError);
    }
    LOG_INFO("Device", "Chip::check_bus", GetStateStr(), "Master Bus Alive! (0x13: 0x{:02X})", tmp_data1[0]);

    return {};
}

/**
 * @brief 初始化缓冲区和寄存器设置
 * @return bool 是否成功
 */
ChipResult<> Chip::init_buffers_and_register(void) {
    std::vector<uint8_t> tmp_data(0x50, 0);
    uint8_t tmp_data1[4]{0};

    uint32_t tmp_register = 0x10007550;
    uint32_t tmp_register2 = 0x1000753C;
    
    if (auto res = HimaxProtocol::register_write(m_master.get(), tmp_register, tmp_data.data(), 0x50); !res) return res;
    if (auto res = HimaxProtocol::register_write(m_master.get(), tmp_register2, tmp_data1, 4); !res) return res;
    
    current_slot = 0x00;
    return {};
}

ChipResult<> Chip::himax_mcu_assign_sorting_mode(uint8_t* tmp_data) {
    bool step_ok = false;
    for (int i = 0; i < 10; ++i) {
        if (HimaxProtocol::write_and_verify(m_master.get(), pfw_op.addr_sorting_mode_en, tmp_data, 4)) {
            step_ok = true;
            break;
        }
    }
    if (!step_ok) {
        LOG_ERROR("Device", "Chip::himax_mcu_assign_sorting_mode", GetStateStr(), "Failed to assign sorting mode!");
        return std::unexpected(ChipError::VerificationFailed);
    }
    return {};
}

ChipResult<> Chip::himax_switch_data_type(DeviceType device, THP_INSPECTION_ENUM mode) {
    std::array<uint8_t, 4> tmp_data{};
    std::string message;
    auto dev_res = SelectDevice(device);
    uint8_t cnt = 50;

    if (!dev_res) {
        LOG_ERROR("Device", "Chip::himax_switch_data_type", GetStateStr(), "get device failed");
        return std::unexpected(dev_res.error());
    }
    HalDevice* dev = *dev_res;

    switch (mode) {
    case THP_INSPECTION_ENUM::EGO_RAWDATA:
        tmp_data[0] = 0xF6;
        message = "EGO_RAWDATA";    
        break;
    
    case THP_INSPECTION_ENUM::HX_RAWDATA:
        tmp_data[0] = 0x0A;
        message = "HX_RAWDATA";
        break;

    case THP_INSPECTION_ENUM::HX_ACT_IDLE_RAWDATA:
        tmp_data[0] = 0x0A;
        message = "HX_ACT_IDLE_RAWDATA";
        break;
    
    case THP_INSPECTION_ENUM::HX_LP_IDLE_RAWDATA:
        tmp_data[0] = 0x0A;
        message = "HX_LP_IDLE_RAWDATA";
        break;
    
    case THP_INSPECTION_ENUM::HX_BACK_NORMAL:
        tmp_data[0] = 0x00;
        message = "HX_BACK_NORMAL";
        break;

    default:
        tmp_data[0] = 0x00;
        message = "HX_BACK_NORMAL_UNKNOW";
        break;
    }
    LOG_INFO("Device", "Chip::himax_switch_data_type", GetStateStr(), "switch to {}!", message);

    ChipResult<> step_ok = std::unexpected(ChipError::InternalError);
    do {
        step_ok = HimaxProtocol::write_and_verify(dev, pfw_op.addr_raw_out_sel, tmp_data.data(), 4);
    }while (!step_ok && --cnt > 0);

    if (step_ok) {
        LOG_INFO("Device", "Chip::himax_switch_data_type", GetStateStr(), "switch to {} success!", message);
        return {};
    } else {
        LOG_ERROR("Device", "Chip::himax_switch_data_type", GetStateStr(), "switch failed!");
        return step_ok;
    }
}

/**
 * @brief 执行硬件复位 AHB 接口
 * @param type 设备类型
 * @return bool 是否成功
 */
ChipResult<> Chip::hx_hw_reset_ahb_intf(DeviceType type) {
    auto dev_res = SelectDevice(type);
    if (!dev_res) return std::unexpected(dev_res.error());
    HalDevice* dev = *dev_res;

    uint8_t tmp_data[4];
    
    LOG_INFO("Device", "Chip::hx_hw_reset_ahb_intf", GetStateStr(), "Enter!");

    himax_parse_assign_cmd(pfw_op.data_clear, tmp_data, 4);
    if (auto res = HimaxProtocol::register_write(dev, pdriver_op.addr_fw_define_2nd_flash_reload, tmp_data, 4); !res) {
        LOG_ERROR("Device", "Chip::hx_hw_reset_ahb_intf", GetStateStr(), "clear reload failed");
        return res;
    }

    // 物理复位操作：经测试，Slave 句柄 (WinError 1168) 不支持复位控制，
    // 物理复位引脚仅绑定在 Master 句柄上，故统一由 m_master 执行。
    if (auto res = m_master->SetReset(0); !res) {
        LOG_ERROR("Device", "Chip::hx_hw_reset_ahb_intf", GetStateStr(), "Physical SetReset(0) via Master failed, WinError: {}", (int)m_master->GetError());
        return res;
    }

    if (auto res = m_master->SetReset(1); !res) {
        LOG_ERROR("Device", "Chip::hx_hw_reset_ahb_intf", GetStateStr(), "Physical SetReset(1) via Master failed, WinError: {}", (int)m_master->GetError());
        return res;
    }

    if (auto res = HimaxProtocol::burst_enable(dev, 1); !res) {
        LOG_ERROR("Device", "Chip::hx_hw_reset_ahb_intf", GetStateStr(), "burst_enable set to 1 failed");
        return res;
    }

    return {};
}

/**
 * @brief 执行软件复位 AHB 接口
 * @param type 设备类型
 * @return bool 是否成功
 */
ChipResult<> Chip::hx_sw_reset_ahb_intf(DeviceType type) {
    auto dev_res = SelectDevice(type);
    if (!dev_res) return std::unexpected(dev_res.error());
    HalDevice* dev = *dev_res;

    uint8_t tmp_data[4];

    // 尝试5次进入safe mode
    bool safe_mode_ok = false;
    for (int i = 0; i < 5; ++i) {
        if (HimaxProtocol::safeModeSetRaw(dev, true)) {
            safe_mode_ok = true;
            break;
        }
        Sleep(10);
    }

    if (!safe_mode_ok) {
        LOG_WARN("Device", "Chip::hx_sw_reset_ahb_intf", GetStateStr(), "Failed to enter Safe Mode before reset, proceeding anyway...");
    }

    Sleep(10);
    himax_parse_assign_cmd(pdriver_op.data_fw_define_flash_reload_en, tmp_data, 4);
    if (auto res = HimaxProtocol::register_write(dev, pdriver_op.addr_fw_define_2nd_flash_reload, tmp_data, 4); !res) {
        LOG_ERROR("Device", "Chip::hx_sw_reset_ahb_intf", GetStateStr(), "clean reload done failed!");
        return res;
    }
    Sleep(10);
    
    himax_parse_assign_cmd(pfw_op.data_system_reset, tmp_data, 4);
    if (auto res = HimaxProtocol::register_write(dev, pfw_op.addr_system_reset, tmp_data, 4); !res) {
        LOG_ERROR("Device", "Chip::hx_sw_reset_ahb_intf", GetStateStr(), "Failed to write System Reset command");
        return res;
    }

    Sleep(100);
    if (auto res = HimaxProtocol::burst_enable(dev, 1); !res) return res;

    return {};
}

/**
 * @brief 设置 Flash 重载状态
 * @param disable 状态 (0 为禁用, 非 0 为启用)
 * @return bool 是否成功
 */
ChipResult<> Chip::himax_mcu_reload_disable(uint8_t disable) {
    LOG_INFO("Device", "Chip::himax_mcu_reload_disable", GetStateStr(), "entering!");
    std::array<uint8_t, 4> tmp_data{};

    if (disable) {
        himax_parse_assign_cmd(pdriver_op.data_fw_define_flash_reload_dis, tmp_data.data(), 4);
    } else {
        himax_parse_assign_cmd(pdriver_op.data_fw_define_flash_reload_en, tmp_data.data(), 4);
    }
    
    if (auto res = HimaxProtocol::register_write(m_master.get(), pdriver_op.addr_fw_define_flash_reload, tmp_data.data(), 4); !res) {
        return res;
    }

    LOG_INFO("Device", "Chip::himax_mcu_reload_disable", GetStateStr(), "setting OK!");
    return {};
}

/**
 * @brief 设置 N 帧参数
 * @param nFrame 帧数
 * @return bool 是否成功
 */
ChipResult<> Chip::hx_set_N_frame(uint8_t nFrame) {
    if (!m_master || !m_master->IsValid()) return std::unexpected(ChipError::CommunicationError);
    LOG_INFO("Device", "Chip::hx_set_N_frame", GetStateStr(), "Enter!");

    std::array<uint8_t, 4> tmp_data{};

    const uint32_t target1 = static_cast<uint32_t>(nFrame);
    const uint32_t target2 = 0x7F0C0000u + static_cast<uint32_t>(nFrame);

    auto pack32 = [&](uint32_t v) {
        tmp_data[0] = static_cast<uint8_t>(v & 0xFFu);
        tmp_data[1] = static_cast<uint8_t>((v >> 8) & 0xFFu);
        tmp_data[2] = static_cast<uint8_t>((v >> 16) & 0xFFu);
        tmp_data[3] = static_cast<uint8_t>((v >> 24) & 0xFFu);
    };

    pack32(target1);
    if (auto res = HimaxProtocol::write_and_verify(m_master.get(), pfw_op.addr_set_frame_addr, tmp_data.data(), 4); !res) return res;

    pack32(target2);
    if (auto res = HimaxProtocol::write_and_verify(m_master.get(), pfw_op.addr_set_frame_addr, tmp_data.data(), 4); !res) return res;
    
    LOG_INFO("Device", "Chip::hx_set_N_frame", GetStateStr(), "Out!");
    return {};
}

ChipResult<> Chip::himax_switch_mode_inspection(THP_INSPECTION_ENUM mode) {
    std::string message;
    constexpr int kUnlockAttempts = 20;
    std::array<uint8_t, 4> tmp_data{};
    bool clear = false;
    LOG_INFO("Device", "Chip::himax_switch_mode_inspection", GetStateStr(), "Entering!");

    /*Stop Handshakng*/
    himax_parse_assign_cmd(psram_op.addr_rawdata_end, tmp_data.data(), 4);
    for (size_t i = 0; i < kUnlockAttempts; i++) {
        if (HimaxProtocol::write_and_verify(m_master.get(), psram_op.addr_rawdata_addr, tmp_data.data(), 4, 2)) {
            clear = true;
            break;
        }
    }
    if (!clear) {
        LOG_ERROR("Device", "Chip::himax_switch_mode_inspection", GetStateStr(), "switch mode unlock failed after {} attempts", kUnlockAttempts);
        return std::unexpected(ChipError::VerificationFailed);
    }

    /*Switch Mode*/
    tmp_data.fill(0);
    switch (mode) {
    case THP_INSPECTION_ENUM::HX_RAWDATA: // normal
        tmp_data[0] = 0x00;
        tmp_data[1] = 0x00;
        message = "HX_RAWDATA";
        break;
    case THP_INSPECTION_ENUM::HX_ACT_IDLE_RAWDATA: // open
        tmp_data[0] = 0x22;
        tmp_data[1] = 0x22;
        message = "HX_ACT_IDLE_RAWDATA";
        break;
    case THP_INSPECTION_ENUM::HX_LP_IDLE_RAWDATA: // short
        tmp_data[0] = 0x50;
        tmp_data[1] = 0x50;
        message = "HX_LP_IDLE_RAWDATA";
        break;
    case THP_INSPECTION_ENUM::HX_BACK_NORMAL:
        tmp_data[0] = 0x00;
        tmp_data[1] = 0x00;
        message = "HX_BACK_NORMAL";
        break;
    case THP_INSPECTION_ENUM::EGO_RAWDATA:
        tmp_data[0] = 0x00;
        tmp_data[1] = 0x00;
        message = "EGO_RAWDATA";
        break;
    }

    if (auto res = himax_mcu_assign_sorting_mode(tmp_data.data()); !res) {
        LOG_ERROR("Device", "Chip::himax_switch_mode_inspection", GetStateStr(), "Failed to switch to {}", message);
        return res;
    }
    
    LOG_INFO("Device", "Chip::himax_switch_mode_inspection", GetStateStr(), "Switching to {} Success", message);
    return {};
}

/**
 * @brief 检查 Flash 重载是否完成
 * @return bool 完成返回 true
 */
ChipResult<> Chip::hx_is_reload_done_ahb(void) {
    std::array<uint8_t, 4> tmp_data{};
    if (auto res = HimaxProtocol::register_read(m_master.get(), pdriver_op.addr_fw_define_2nd_flash_reload, tmp_data.data(), 4); !res) return res;
    
    if (tmp_data[0] == 0xC0 && tmp_data[1] == 0x72) {
        return {};
    }
    return std::unexpected(ChipError::InvalidOperation);
}

ChipResult<> Chip::himax_mcu_read_FW_status(void) {
    std::array<uint8_t, 4> tmp_data{};
    uint32_t dbg_reg_ary[4] = {pfw_op.addr_fw_dbg_msg_addr, pfw_op.addr_chk_fw_status,
	pfw_op.addr_chk_dd_status, pfw_op.addr_flag_reset_event};

    for (uint32_t addr : dbg_reg_ary) {
        if (auto res = HimaxProtocol::register_read(m_master.get(), addr, tmp_data.data(), 4); !res) return res;
        LOG_INFO("Device", "Chip::himax_mcu_read_FW_status", GetStateStr(), "{:x} = {::#x}",addr, tmp_data);
    }
    return {};
}

ChipResult<> Chip::himax_mcu_interface_on(void) {
    LOG_INFO("Device", "Chip::himax_mcu_interface_on", GetStateStr(), "Enter!");

    std::array<uint8_t, 4> tmp_data{};
    std::array<uint8_t, 4> tmp_data2{};
    int cnt = 0;
    
    if (auto res = m_master->ReadBus(pic_op.addr_ahb_rdata_byte_0, tmp_data.data(), 4); !res) {
        LOG_ERROR("Device", "Chip::himax_mcu_interface_on", GetStateStr(), "ReadBus failed");
        return res;
    }        
    
    do {
        tmp_data[0] = pic_op.data_conti;
        
        if (auto res = m_master->WriteBus(pic_op.addr_conti, NULL, tmp_data.data(), 1); !res) {
            LOG_ERROR("Device", "Chip::himax_mcu_interface_on", GetStateStr(), "bus access fail!");
            return res;
        }
        
        tmp_data[0] = pic_op.data_incr4;
        if (auto res = m_master->WriteBus(pic_op.addr_incr4, NULL, tmp_data.data(), 1); !res) {
            LOG_ERROR("Device", "Chip::himax_mcu_interface_on", GetStateStr(), "bus access fail!");
            return res;
        }

        if (auto res = m_master->ReadBus(pic_op.addr_conti, tmp_data.data(), 1); !res) return res;
        if (auto res = m_master->ReadBus(pic_op.addr_incr4, tmp_data2.data(), 1); !res) return res;

        if (tmp_data[0] == pic_op.data_conti && tmp_data2[0] == pic_op.data_incr4) {
            break;
        }

        Sleep(1);
    } while (++cnt < 10);

    if (cnt > 0) {
        LOG_INFO("Device", "Chip::himax_mcu_interface_on", GetStateStr(), "Polling burst mode: {:d} times", cnt);
    }
    return {};
}

/**
 * @brief 开启感应模式
 * @param FlashMode 是否执行硬件复位
 * @return bool 是否成功
 */
ChipResult<> Chip::hx_sense_on(bool FlashMode) {
    LOG_INFO("Device", "Chip::hx_sense_on", GetStateStr(), "Enter, isHwReset = {}", FlashMode);
    std::array<uint8_t, 4> tmp_data{};

    if (auto res = himax_mcu_interface_on(); !res) return res;
    himax_parse_assign_cmd(pfw_op.data_clear, tmp_data.data(), 4);
    if (auto res = HimaxProtocol::register_write(m_master.get(), pfw_op.addr_ctrl_fw_isr, tmp_data.data(), 4); !res) return res;
    
    Sleep(11);

    if (!FlashMode) {
        if (auto res = m_master->SetReset(false); !res) return res;
        if (auto res = m_master->SetReset(true); !res) return res;
    } else {
        tmp_data.fill(0);
        if (auto res = m_master->WriteBus(pic_op.adr_i2c_psw_lb, NULL, tmp_data.data(), 2); !res) return res;
    }
    return {};
}

/**
 * @brief 关闭感应模式
 * @param check_en 是否检查固件状态
 * @return bool 是否成功
 */
ChipResult<> Chip::hx_sense_off(bool check_en) {
    ChipResult<> step_ok = {};
    int cnt = 0;
    std::array<uint8_t, 4> send_data{};
    std::array<uint8_t, 4> back_data{};
    
    do {
        if (cnt == 0 || (back_data[0] != 0xA5 && back_data[0] != 0x00 && back_data[0] != 0x87)) {
            himax_parse_assign_cmd(pfw_op.data_fw_stop, send_data.data(), 4);
            step_ok = HimaxProtocol::register_write(m_master.get(), pfw_op.addr_ctrl_fw_isr, send_data.data(), 4);
            if (!step_ok) return step_ok;
        }
        Sleep(20);

        step_ok = HimaxProtocol::register_read(m_master.get(), pfw_op.addr_chk_fw_status, back_data.data(), 4);
        if (!step_ok) return step_ok;

        if ((back_data[0] != 0x05) || (check_en == false)) { 
            LOG_INFO("Device", "Chip::hx_sense_off", GetStateStr(), "Do not need wait FW, status = 0x{:X}", back_data[0]);
            break;
        }

        if (auto res = HimaxProtocol::register_read(m_master.get(), pfw_op.addr_ctrl_fw_isr, back_data.data(), 4); !res) return res;

    }while (send_data[0] != 0x87 && (++cnt < 20) && check_en);


    cnt = 0;
    back_data.fill(0);
    do {
        ChipResult<> safe_res = std::unexpected(ChipError::InternalError);
        for (int i = 0; i < 5; i++) {
            safe_res = HimaxProtocol::safeModeSetRaw(m_master.get(), true);
            if (safe_res) break;
        }

        if (auto res = HimaxProtocol::register_read(m_master.get(), pfw_op.addr_chk_fw_status, back_data.data(), 4); !res) return res;
        LOG_INFO("Device", "Chip::hx_sense_off", GetStateStr(), "Check enter_safe_mode data[0]={:x}", back_data[0]);

        if (back_data[0] == 0x0C) {
            //reset TCON
            himax_parse_assign_cmd(pic_op.addr_tcon_on_rst, send_data.data(), 4);
            if (auto res = HimaxProtocol::register_write(m_master.get(), pic_op.addr_tcon_on_rst, send_data.data(), 4); !res) return res;
            
            return {};
        }
        if (auto res = m_master->SetReset(0); !res) return res;
        Sleep(20);
        if (auto res = m_master->SetReset(1); !res) return res; // Fix: SetReset should be 1, original had 50? SetReset(bool)
        Sleep(50);
    }while (cnt++ < 15);

    LOG_INFO("Device", "Chip::hx_sense_off", GetStateStr(), "Out!");
    return std::unexpected(ChipError::VerificationFailed);
}

ChipResult<> Chip::himax_mcu_power_on_init(void) {
    LOG_INFO("Device", "Chip::himax_mcu_power_on_init", GetStateStr(), "entering!");
    std::array<uint8_t, 4> tmp_data{0x01, 0x00, 0x00, 0x00};
    std::array<uint8_t, 4> tmp_data2{};
    uint8_t retry = 0;

    if (auto res = HimaxProtocol::register_write(m_master.get(), pfw_op.addr_raw_out_sel, tmp_data2.data(), 4); !res) return res;
    if (auto res = HimaxProtocol::register_write(m_master.get(), pfw_op.addr_sorting_mode_en, tmp_data2.data(), 4); !res) return res;
    if (auto res = HimaxProtocol::register_write(m_master.get(), pfw_op.addr_set_frame_addr, tmp_data.data(), 4); !res) return res;
    if (auto res = HimaxProtocol::register_write(m_master.get(), pdriver_op.addr_fw_define_2nd_flash_reload, tmp_data2.data(), 4); !res) return res;

    if (auto res = hx_sense_on(false); !res) return res;

    LOG_INFO("Device", "Chip::himax_mcu_power_on_init", GetStateStr(), "waiting for FW reload data");

    while (retry++ < 30) {
        if (auto res = HimaxProtocol::register_read(m_master.get(), pdriver_op.addr_fw_define_2nd_flash_reload, tmp_data.data(), 4); !res) return res;
        if ((tmp_data[3] == 0x00 && tmp_data[2] == 0x00 &&
			tmp_data[1] == 0x72 && tmp_data[0] == 0xC0)) {
            LOG_INFO("Device", "Chip::himax_mcu_power_on_init", GetStateStr(), "FW reload done!");
            return {};
        }
        LOG_INFO("Device", "Chip::himax_mcu_power_on_init", GetStateStr(), "waiting for FW reload data %d", retry); 
        auto res = himax_mcu_read_FW_status(); // 打印 log
        Sleep(11);
    }
    LOG_ERROR("Device", "Chip::himax_mcu_power_on_init", GetStateStr(), "FW reload timeout!");
    return std::unexpected(ChipError::Timeout);
}

/**
 * @brief 进入低功耗模式
 * @param param 参数
 * @return bool 是否成功
 */


ChipResult<> Chip::Init(void) {
    if (auto res = hx_hw_reset_ahb_intf(DeviceType::Master); !res) return res;
    Sleep(10);
    LOG_INFO("Device", "Chip::Init", GetStateStr(), "Starting initialization sequence...");

    std::array<uint8_t, 4> tmp_data = {0xA5, 0x5A, 0x00, 0x00};

    if (auto res = himax_mcu_reload_disable(false); !res) return res;
    if (auto res = himax_switch_mode_inspection(THP_INSPECTION_ENUM::EGO_RAWDATA); !res) return res;
    if (auto res = hx_set_N_frame(1); !res) return res;

    if (auto res = himax_mcu_power_on_init(); !res) return res;
    
    if (auto res = himax_switch_data_type(DeviceType::Master, THP_INSPECTION_ENUM::EGO_RAWDATA); !res) return res;
    if (auto res = HimaxProtocol::register_write(m_master.get(), psram_op.addr_rawdata_addr, tmp_data.data(), 4); !res) return res;
    if (auto res = himax_switch_data_type(DeviceType::Slave, THP_INSPECTION_ENUM::EGO_RAWDATA); !res) return res;
    if (auto res = HimaxProtocol::register_write(m_slave.get(), psram_op.addr_rawdata_addr, tmp_data.data(), 4); !res) return res;
    
    // 打开中断/数据采集通道
    if (auto res = m_master->IntOpen(); !res) {
        LOG_ERROR("Device", "Chip::Init", GetStateStr(), "Master IntOpen failed!");
        return res;
    }
    if (auto res = m_slave->IntOpen(); !res) {
        LOG_ERROR("Device", "Chip::Init", GetStateStr(), "Slave IntOpen failed!");
        return res;
    }

    // Default to normal read policy after channel is opened.
    if (auto res = SetFrameReadNormalPolicy(); !res) {
        LOG_ERROR("Device", "Chip::Init", GetStateStr(), "Apply normal read policy failed");
        return res;
    }

    // 标记连接就绪（后续 AFE 命令检查此状态）
    m_connState.store(ConnectionState::Connected);

    // 重置手写笔软件状态（确保无残留 pending）
    m_afe.ResetStylusState();

    if (auto res = m_afe.StartCalibration(); !res) {
        LOG_WARN("Device", "Chip::Init", GetStateStr(),
                 "start_calibration failed (non-fatal), chip may use default rate");
    } else {
        LOG_INFO("Device", "Chip::Init", GetStateStr(), "start_calibration success.");
    }
    
    // 强制芯片切换到 120Hz 扫描率（cmd=0x0E, val=0x00）
    // 确保上电后不处于芯片默认的低帧率或未知模式
    if (auto res = m_afe.ForceToScanRate(0x00); !res) {
        LOG_WARN("Device", "Chip::Init", GetStateStr(),
                 "force_to_scan_rate(120Hz) failed (non-fatal), chip may use default rate");
    } else {
        LOG_INFO("Device", "Chip::Init", GetStateStr(), "Scan rate forced to 120Hz.");
    }

    LOG_INFO("Device", "Chip::Init", GetStateStr(), "Initialization and Sense ON successful.");
    return {};
}

ChipResult<> Chip::Deinit(bool check_en) {
    // CRITICAL: Mark disconnected FIRST so that other threads (acquisition, processing)
    // immediately stop accessing the device.  Their GetConnectionState() checks will
    // fail before we tear down the channels below.
    m_connState.store(ConnectionState::Unconnected);
    LOG_INFO("Device", "Chip::Deinit", GetStateStr(), "Starting Deinit sequence...");

    // Send sense off if bus is accessible
    auto resOff = hx_sense_off(check_en);
    if (!resOff) {
        LOG_WARN("Device", "Chip::Deinit", GetStateStr(), "hx_sense_off had issues during Deinit.");
    }
    
    auto m_res = m_master->IntClose();
    auto s_res = m_slave->IntClose();
    
    if (!m_res || !s_res) {
         LOG_WARN("Device", "Chip::Deinit", GetStateStr(), "IntClose had issues during Deinit.");
    }
    
    LOG_INFO("Device", "Chip::Deinit", GetStateStr(), "Deinit successful.");
    return {};
}

void Chip::HoldReset() {
    m_connState.store(ConnectionState::Unconnected);

    // 1) Cancel any in-flight GetFrame / bus I/O first
    CancelPendingFrameRead();

    // 2) Pull reset low — chip powered off, no bus traffic needed
    (void)m_master->SetReset(false);

    // 3) Close interrupt channels so next Init can IntOpen cleanly
    (void)m_master->IntClose();
    (void)m_slave->IntClose();

    LOG_INFO("Device", "Chip::HoldReset", "Unconnected",
             "Reset held low, interrupt channels closed (suspend).");
}

Chip::~Chip() {
    if (m_connState.load() != ConnectionState::Unconnected) {
        LOG_INFO("Device", "Chip::~Chip", GetStateStr(), "Implicitly calling Deinit().");
        (void)Deinit(false);
    }
}

/**
 * @brief 检测是否有手指触摸
 * 
 * 从 master status 表（back_data[4807..]）的 word[54] 和 word[55] 判断。
 * 固件在有触点时写入 TX/RX 坐标；无触点时 xy 均为 0xFF。
 */
bool Chip::isFingerDetected() const {
    constexpr size_t kStatusOffset = 7 + 4800;  // 4807: master status 表起始
    constexpr size_t kTouchXWord   = 54;        // 触点 X 坐标 (u16 word index)
    constexpr size_t kTouchYWord   = 55;        // 触点 Y 坐标 (u16 word index)

    const uint8_t* st = back_data.data() + kStatusOffset;

    auto readU16 = [&](size_t wordIdx) -> uint16_t {
        size_t byteIdx = wordIdx * 2;
        return static_cast<uint16_t>(st[byteIdx] | (st[byteIdx + 1] << 8));
    };

    uint16_t touch_x = readU16(kTouchXWord);
    uint16_t touch_y = readU16(kTouchYWord);

    bool detected = !((touch_x & 0xFF) == 0xFF && (touch_y & 0xFF) == 0xFF);

    return detected;
}

/**
 * @brief 检测是否有手写笔输入
 * 
 * Slave 帧起始于 back_data[5063]，共 339 字节。
 * 与 master 帧一样有 7 字节校验头，真正的状态从第 8 字节起。
 * 字段 1-2（byte[7] / byte[8]）用于手写笔存在判定，0xFF=无笔。
 */
bool Chip::isStylusDetected() const {
    constexpr size_t kSlaveOffset = 5063;
    constexpr size_t kHeaderSize  = 7;    // 与 master 一致的校验头

    const uint8_t* st = back_data.data() + kSlaveOffset + kHeaderSize;

    auto readU16 = [&](size_t wordIdx) -> uint16_t {
        size_t byteIdx = wordIdx * 2;
        return static_cast<uint16_t>(st[byteIdx] | (st[byteIdx + 1] << 8));
    };
    const uint16_t field1 = readU16(0);  // 字段 1（word index 0）
    const uint16_t field2 = readU16(1);  // 字段 2（word index 1）
    return !((field1 & 0xFF) == 0xFF && (field2 & 0xFF) == 0xFF);
}

ChipResult<> Chip::GetFrame(void) {

    // ── Idle 模式：500ms 睡眠后做一次帧获取检测唤醒 ──────────
    if (afe_mode.load() == THP_AFE_MODE::Idle) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // 尝试读一帧用于唤醒检测（master + slave）
        auto m_res = m_master->GetFrame(back_data.data(), 5063, nullptr);
        auto s_res = m_slave->GetFrame(back_data.data() + 5063, 339, nullptr);

        if (m_res && s_res) {
            if (isFingerDetected() || isStylusDetected()) {
                (void)NotifyTouchWakeup();
                LOG_INFO("Device", "Chip::GetFrame", GetStateStr(),
                         "Input detected in idle → wakeup to Normal");
            }
            // 无输入: 丢弃 idle 帧，返回超时让 caller 继续循环
            return std::unexpected(ChipError::Timeout);
        }
        // 读帧失败（超时）= 芯片仍在深度 idle
        return std::unexpected(ChipError::Timeout);
    }

    // ── Normal 模式：2:1 交错帧获取 ──────────────────────────
    // 原厂顺序: Master → Slave（确保 master scan cycle 完成后再读 slave 数据）

    // 1. 条件读 Master (touch, 有效 120Hz)
    //    奇数帧 且 手写笔已连接 → 跳过 Master，复用上一帧数据
    bool skipMaster = (m_frameCount & 1) != 0 && m_afe.GetStylusState().connected;
    if (!skipMaster) {
        if (auto res = m_master->GetFrame(back_data.data(), 5063, nullptr); !res) {
            if (m_master->IsTimeoutError())
                return std::unexpected(ChipError::Timeout);
            LOG_ERROR("Device", "Chip::GetFrame", GetStateStr(), "Master GetFrame failed!");
            return res;
        }
    }

    // 2. 总是读 Slave (stylus, 有效 240Hz)
    if (auto res = m_slave->GetFrame(back_data.data() + 5063, 339, nullptr); !res) {
        if (m_slave->IsTimeoutError())
            return std::unexpected(ChipError::Timeout);
        LOG_ERROR("Device", "Chip::GetFrame", GetStateStr(), "Slave GetFrame failed!");
        return res;
    }
    m_frameCount++;

    // ── 零帧计数 & idle 自动进入 ─────────────────────────────
    constexpr uint32_t kIdleEntryThreshold = 600;  // ~5 秒 @120Hz

    if (isFingerDetected() || isStylusDetected()) {
        m_zeroFrameCount = 0;
    } else {
        m_zeroFrameCount++;
        if (m_zeroFrameCount >= kIdleEntryThreshold) {
            LOG_INFO("Device", "Chip::GetFrame", GetStateStr(),
                     "No input for {} frames → EnterIdle",
                     m_zeroFrameCount);
            (void)m_afe.EnterIdle();
            m_zeroFrameCount = 0;
        }
    }

    // 调试：每 600 帧打计数日志
    if ((m_frameCount % 600) == 0) {
        LOG_DEBUG("Device", "Chip::GetFrame", GetStateStr(),
                 "[IDLE-DIAG] m_frameCount={} m_zeroFrameCount={} afe_mode={}",
                 m_frameCount, m_zeroFrameCount,
                 afe_mode.load() == THP_AFE_MODE::Idle ? "Idle" : "Normal");
    }

    return {};
}

void Chip::CancelPendingFrameRead() {
    if (m_master) m_master->CancelPendingIo();
    if (m_slave)  m_slave->CancelPendingIo();
}

} // namespace Himax
