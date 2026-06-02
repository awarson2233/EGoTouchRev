#include "himax/HimaxChip.h"
#include "himax/HimaxProtocol.h"
#include "himax/HimaxByteUtils.h"
#include "Logger.h"

#include <array>
#include <vector>
#include <windows.h>

namespace Himax {

ChipResult<> Chip::init_buffers_and_register(void) {
    std::vector<uint8_t> tmp_data(0x50, 0);
    uint8_t tmp_data1[4]{0};

    constexpr uint32_t kCmdSlotBaseAddr = 0x10007550;
    constexpr uint32_t kCmdWritePtrAddr = 0x1000753C;

    if (auto res = HimaxProtocol::register_write(m_master.get(), kCmdSlotBaseAddr, tmp_data.data(), 0x50); !res) return res;
    if (auto res = HimaxProtocol::register_write(m_master.get(), kCmdWritePtrAddr, tmp_data1, 4); !res) return res;

    current_slot = 0x00;
    return {};
}

ChipResult<> Chip::hx_hw_reset_ahb_intf(DeviceType type) {
    auto dev_res = SelectDevice(type);
    if (!dev_res) return std::unexpected(dev_res.error());
    HalDevice* dev = *dev_res;

    uint8_t tmp_data[4];

    LOG_INFO("HimaxChip", __func__, GetStateStr(), "Enter!");

    detail::ParseAddressLittleEndian(pfw_op.data_clear, tmp_data, 4);
    if (auto res = HimaxProtocol::register_write(dev, pdriver_op.addr_fw_define_2nd_flash_reload, tmp_data, 4); !res) {
        LOG_ERROR("HimaxChip", __func__, GetStateStr(), "clear reload failed");
        return res;
    }

    // 物理复位操作：经测试，Slave 句柄 (WinError 1168) 不支持复位控制，
    // 物理复位引脚仅绑定在 Master 句柄上，故统一由 m_master 执行。
    if (auto res = m_master->SetReset(0); !res) {
        LOG_ERROR("HimaxChip", __func__, GetStateStr(), "Physical SetReset(0) via Master failed, WinError: {}", static_cast<int>(m_master->GetError()));
        return res;
    }

    if (auto res = m_master->SetReset(1); !res) {
        LOG_ERROR("HimaxChip", __func__, GetStateStr(), "Physical SetReset(1) via Master failed, WinError: {}", static_cast<int>(m_master->GetError()));
        return res;
    }

    if (auto res = HimaxProtocol::burst_enable(dev, 1); !res) {
        LOG_ERROR("HimaxChip", __func__, GetStateStr(), "burst_enable set to 1 failed");
        return res;
    }

    return {};
}

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
        LOG_WARN("HimaxChip", __func__, GetStateStr(), "Failed to enter Safe Mode before reset, proceeding anyway...");
    }

    Sleep(10);
    detail::ParseAddressLittleEndian(pdriver_op.data_fw_define_flash_reload_en, tmp_data, 4);
    if (auto res = HimaxProtocol::register_write(dev, pdriver_op.addr_fw_define_2nd_flash_reload, tmp_data, 4); !res) {
        LOG_ERROR("HimaxChip", __func__, GetStateStr(), "clean reload done failed!");
        return res;
    }
    Sleep(10);

    detail::ParseAddressLittleEndian(pfw_op.data_system_reset, tmp_data, 4);
    if (auto res = HimaxProtocol::register_write(dev, pfw_op.addr_system_reset, tmp_data, 4); !res) {
        LOG_ERROR("HimaxChip", __func__, GetStateStr(), "Failed to write System Reset command");
        return res;
    }

    Sleep(100);
    if (auto res = HimaxProtocol::burst_enable(dev, 1); !res) return res;

    return {};
}

ChipResult<> Chip::himax_mcu_reload_disable(uint8_t disable) {
    LOG_INFO("HimaxChip", __func__, GetStateStr(), "entering!");
    std::array<uint8_t, 4> tmp_data{};

    if (disable) {
        detail::ParseAddressLittleEndian(pdriver_op.data_fw_define_flash_reload_dis, tmp_data.data(), 4);
    } else {
        detail::ParseAddressLittleEndian(pdriver_op.data_fw_define_flash_reload_en, tmp_data.data(), 4);
    }

    if (auto res = HimaxProtocol::register_write(m_master.get(), pdriver_op.addr_fw_define_flash_reload, tmp_data.data(), 4); !res) {
        return res;
    }

    LOG_INFO("HimaxChip", __func__, GetStateStr(), "setting OK!");
    return {};
}

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
    uint32_t dbg_reg_ary[4] = {
        pfw_op.addr_fw_dbg_msg_addr,
        pfw_op.addr_chk_fw_status,
        pfw_op.addr_chk_dd_status,
        pfw_op.addr_flag_reset_event,
    };

    for (uint32_t addr : dbg_reg_ary) {
        if (auto res = HimaxProtocol::register_read(m_master.get(), addr, tmp_data.data(), 4); !res) return res;
    }
    return {};
}

ChipResult<> Chip::himax_mcu_interface_on(void) {
    LOG_INFO("HimaxChip", __func__, GetStateStr(), "Enter!");

    std::array<uint8_t, 4> tmp_data{};
    std::array<uint8_t, 4> tmp_data2{};
    int cnt = 0;

    if (auto res = m_master->ReadBus(pic_op.addr_ahb_rdata_byte_0, tmp_data.data(), 4); !res) {
        LOG_ERROR("HimaxChip", __func__, GetStateStr(), "ReadBus failed");
        return res;
    }

    do {
        tmp_data[0] = pic_op.data_conti;

        if (auto res = m_master->WriteBus(pic_op.addr_conti, nullptr, tmp_data.data(), 1); !res) {
            LOG_ERROR("HimaxChip", __func__, GetStateStr(), "bus access fail!");
            return res;
        }

        tmp_data[0] = pic_op.data_incr4;
        if (auto res = m_master->WriteBus(pic_op.addr_incr4, nullptr, tmp_data.data(), 1); !res) {
            LOG_ERROR("HimaxChip", __func__, GetStateStr(), "bus access fail!");
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
        LOG_INFO("HimaxChip", __func__, GetStateStr(), "Polling burst mode: {:d} times", cnt);
    }
    return {};
}

ChipResult<> Chip::hx_sense_on(bool FlashMode) {
    LOG_INFO("HimaxChip", __func__, GetStateStr(), "Enter, isHwReset = {}", FlashMode);
    std::array<uint8_t, 4> tmp_data{};

    if (auto res = himax_mcu_interface_on(); !res) return res;
    detail::ParseAddressLittleEndian(pfw_op.data_clear, tmp_data.data(), 4);
    if (auto res = HimaxProtocol::register_write(m_master.get(), pfw_op.addr_ctrl_fw_isr, tmp_data.data(), 4); !res) return res;

    Sleep(11);

    if (!FlashMode) {
        if (auto res = m_master->SetReset(false); !res) return res;
        if (auto res = m_master->SetReset(true); !res) return res;
    } else {
        tmp_data.fill(0);
        if (auto res = m_master->WriteBus(pic_op.adr_i2c_psw_lb, nullptr, tmp_data.data(), 2); !res) return res;
    }
    return {};
}

ChipResult<> Chip::hx_sense_off(bool check_en) {
    ChipResult<> step_ok = {};
    int cnt = 0;
    std::array<uint8_t, 4> send_data{};
    std::array<uint8_t, 4> back_data{};

    do {
        if (cnt == 0 || (back_data[0] != 0xA5 && back_data[0] != 0x00 && back_data[0] != 0x87)) {
            detail::ParseAddressLittleEndian(pfw_op.data_fw_stop, send_data.data(), 4);
            step_ok = HimaxProtocol::register_write(m_master.get(), pfw_op.addr_ctrl_fw_isr, send_data.data(), 4);
            if (!step_ok) return step_ok;
        }
        Sleep(20);

        step_ok = HimaxProtocol::register_read(m_master.get(), pfw_op.addr_chk_fw_status, back_data.data(), 4);
        if (!step_ok) return step_ok;

        if ((back_data[0] != 0x05) || (check_en == false)) {
            LOG_INFO("HimaxChip", __func__, GetStateStr(), "Do not need wait FW, status = 0x{:X}", back_data[0]);
            break;
        }

        if (auto res = HimaxProtocol::register_read(m_master.get(), pfw_op.addr_ctrl_fw_isr, back_data.data(), 4); !res) return res;

    } while (send_data[0] != 0x87 && (++cnt < 20) && check_en);

    cnt = 0;
    back_data.fill(0);
    do {
        ChipResult<> safe_res = std::unexpected(ChipError::InternalError);
        for (int i = 0; i < 5; i++) {
            safe_res = HimaxProtocol::safeModeSetRaw(m_master.get(), true);
            if (safe_res) break;
        }

        if (auto res = HimaxProtocol::register_read(m_master.get(), pfw_op.addr_chk_fw_status, back_data.data(), 4); !res) return res;
        LOG_INFO("HimaxChip", __func__, GetStateStr(), "Check enter_safe_mode data[0]={:x}", back_data[0]);

        if (back_data[0] == 0x0C) {
            // reset TCON
            detail::ParseAddressLittleEndian(pic_op.addr_tcon_on_rst, send_data.data(), 4);
            if (auto res = HimaxProtocol::register_write(m_master.get(), pic_op.addr_tcon_on_rst, send_data.data(), 4); !res) return res;
            return {};
        }

        if (auto res = m_master->SetReset(0); !res) return res;
        Sleep(20);
        if (auto res = m_master->SetReset(1); !res) return res;
        Sleep(50);
    } while (cnt++ < 15);

    LOG_INFO("HimaxChip", __func__, GetStateStr(), "Out!");
    return std::unexpected(ChipError::VerificationFailed);
}

ChipResult<> Chip::himax_mcu_power_on_init(void) {
    LOG_INFO("HimaxChip", __func__, GetStateStr(), "entering!");
    std::array<uint8_t, 4> tmp_data{0x01, 0x00, 0x00, 0x00};
    std::array<uint8_t, 4> tmp_data2{};
    uint8_t retry = 0;

    if (auto res = HimaxProtocol::register_write(m_master.get(), pfw_op.addr_raw_out_sel, tmp_data2.data(), 4); !res) return res;
    if (auto res = HimaxProtocol::register_write(m_master.get(), pfw_op.addr_sorting_mode_en, tmp_data2.data(), 4); !res) return res;
    if (auto res = HimaxProtocol::register_write(m_master.get(), pfw_op.addr_set_frame_addr, tmp_data.data(), 4); !res) return res;
    if (auto res = HimaxProtocol::register_write(m_master.get(), pdriver_op.addr_fw_define_2nd_flash_reload, tmp_data2.data(), 4); !res) return res;

    if (auto res = hx_sense_on(false); !res) return res;

    while (retry++ < 30) {
        if (auto res = HimaxProtocol::register_read(m_master.get(), pdriver_op.addr_fw_define_2nd_flash_reload, tmp_data.data(), 4); !res) return res;
        if ((tmp_data[3] == 0x00 && tmp_data[2] == 0x00 && tmp_data[1] == 0x72 && tmp_data[0] == 0xC0)) {
            LOG_INFO("HimaxChip", __func__, GetStateStr(), "FW reload done!");
            return {};
        }
        (void)himax_mcu_read_FW_status();
        Sleep(11);
    }

    LOG_ERROR("HimaxChip", __func__, GetStateStr(), "FW reload timeout!");
    return std::unexpected(ChipError::Timeout);
}

ChipResult<> Chip::Init(void) {
    if (auto res = hx_hw_reset_ahb_intf(DeviceType::Master); !res) return res;
    Sleep(10);
    LOG_INFO("HimaxChip", __func__, GetStateStr(), "Starting initialization sequence...");

    std::array<uint8_t, 4> tmp_data = {0xA5, 0x5A, 0x00, 0x00};

    if (auto res = himax_mcu_reload_disable(false); !res) return res;
    if (auto res = himax_switch_mode_inspection(THP_INSPECTION_ENUM::EGO_RAWDATA); !res) return res;
    if (auto res = hx_set_N_frame(1); !res) return res;

    if (auto res = himax_mcu_power_on_init(); !res) return res;

    if (auto res = himax_switch_data_type(DeviceType::Master, THP_INSPECTION_ENUM::EGO_RAWDATA); !res) return res;
    if (auto res = HimaxProtocol::register_write(m_master.get(), psram_op.addr_rawdata_addr, tmp_data.data(), 4); !res) return res;
    if (auto res = himax_switch_data_type(DeviceType::Slave, THP_INSPECTION_ENUM::EGO_RAWDATA); !res) return res;
    if (auto res = HimaxProtocol::register_write(m_slave.get(), psram_op.addr_rawdata_addr, tmp_data.data(), 4); !res) return res;

    if (auto res = m_master->IntOpen(); !res) {
        LOG_ERROR("HimaxChip", __func__, GetStateStr(), "Master IntOpen failed!");
        return res;
    }
    if (auto res = m_slave->IntOpen(); !res) {
        LOG_ERROR("HimaxChip", __func__, GetStateStr(), "Slave IntOpen failed!");
        (void)m_slave->IntClose();
        (void)m_master->IntClose();
        return res;
    }

    if (auto res = SetFrameReadNormalPolicy(); !res) {
        LOG_ERROR("HimaxChip", __func__, GetStateStr(), "Apply normal read policy failed");
        (void)m_slave->IntClose();
        (void)m_master->IntClose();
        return res;
    }

    current_slot = 0;
    m_zeroFrameCount = 0;
    m_frameCount = 0;
    m_stylusActive = false;
    m_lastMasterWasRead = true;
    afe_mode.store(THP_AFE_MODE::Normal);
    m_connState.store(ConnectionState::Connected);

    m_afe.ResetStylusState();

    if (auto res = m_afe.StartCalibration(); !res) {
        LOG_WARN("HimaxChip", __func__, GetStateStr(), "start_calibration failed (non-fatal), chip may use default rate");
    } else {
        LOG_INFO("HimaxChip", __func__, GetStateStr(), "start_calibration success.");
    }

    if (auto res = m_afe.ForceToScanRate(0x00); !res) {
        LOG_WARN("HimaxChip", __func__, GetStateStr(), "force_to_scan_rate(120Hz) failed (non-fatal), chip may use default rate");
    } else {
        LOG_INFO("HimaxChip", __func__, GetStateStr(), "Scan rate forced to 120Hz.");
    }

    LOG_INFO("HimaxChip", __func__, GetStateStr(), "Initialization and Sense ON successful.");
    return {};
}

ChipResult<> Chip::Deinit(bool check_en) {
    LOG_INFO("HimaxChip", __func__, GetStateStr(), "Starting Deinit sequence...");
    m_connState.store(ConnectionState::Unconnected);
    afe_mode.store(THP_AFE_MODE::Normal);
    m_zeroFrameCount = 0;
    m_frameCount = 0;
    m_stylusActive = false;
    m_lastMasterWasRead = true;
    current_slot = 0;
    m_afe.ResetStylusState();

    auto resOff = hx_sense_off(check_en);
    if (!resOff) {
        LOG_WARN("HimaxChip", __func__, GetStateStr(), "hx_sense_off had issues during Deinit.");
    }

    auto m_res = m_master->IntClose();
    auto s_res = m_slave->IntClose();

    if (!m_res || !s_res) {
         LOG_WARN("HimaxChip", __func__, GetStateStr(), "IntClose had issues during Deinit.");
    }

    LOG_INFO("HimaxChip", __func__, GetStateStr(), "Deinit successful.");
    return {};
}

void Chip::HoldReset() {
    m_connState.store(ConnectionState::Unconnected);
    afe_mode.store(THP_AFE_MODE::Normal);
    m_zeroFrameCount = 0;
    m_frameCount = 0;
    m_stylusActive = false;
    m_lastMasterWasRead = true;
    current_slot = 0;
    m_afe.ResetStylusState();

    CancelPendingFrameRead();

    (void)m_master->SetReset(false);

    (void)m_master->IntClose();
    (void)m_slave->IntClose();

    LOG_INFO("HimaxChip", __func__, "Unconnected", "Reset held low, interrupt channels closed (suspend).");
}

} // namespace Himax
