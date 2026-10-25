#include "himax/HimaxChip.h"
#include "himax/HimaxProtocol.h"
#include "himax/HimaxByteUtils.h"
#include "Logger.h"
#include "FrameLayout.h"
#include "Port/Time.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <initializer_list>
#include <vector>

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
        LOG_ERROR("HimaxChip", __func__, GetStateStr(), "Physical SetReset(0) via Master failed, err: {}", static_cast<int>(m_master->GetError()));
        return res;
    }
    // 复位要保持一段时间（himax-spi 的 himax_pin_reset 拉低 5 ms）。SpbThpTool 的复位 IOCTL 自带时序，
    // Linux 的 himax-thp 只改电平：不等的话脉冲太短，IC 没有真正复位，后面的 FW reload 等不到
    Port::SleepMs(5);

    if (auto res = m_master->SetReset(1); !res) {
        LOG_ERROR("HimaxChip", __func__, GetStateStr(), "Physical SetReset(1) via Master failed, err: {}", static_cast<int>(m_master->GetError()));
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
        Port::SleepMs(10);
    }

    if (!safe_mode_ok) {
        LOG_WARN("HimaxChip", __func__, GetStateStr(), "Failed to enter Safe Mode before reset, proceeding anyway...");
    }

    Port::SleepMs(10);
    detail::ParseAddressLittleEndian(pdriver_op.data_fw_define_flash_reload_en, tmp_data, 4);
    if (auto res = HimaxProtocol::register_write(dev, pdriver_op.addr_fw_define_2nd_flash_reload, tmp_data, 4); !res) {
        LOG_ERROR("HimaxChip", __func__, GetStateStr(), "clean reload done failed!");
        return res;
    }
    Port::SleepMs(10);

    detail::ParseAddressLittleEndian(pfw_op.data_system_reset, tmp_data, 4);
    if (auto res = HimaxProtocol::register_write(dev, pfw_op.addr_system_reset, tmp_data, 4); !res) {
        LOG_ERROR("HimaxChip", __func__, GetStateStr(), "Failed to write System Reset command");
        return res;
    }

    Port::SleepMs(100);
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

        Port::SleepMs(1);
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

    Port::SleepMs(11);

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
        Port::SleepMs(20);

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
        Port::SleepMs(20);
        if (auto res = m_master->SetReset(1); !res) return res;
        Port::SleepMs(50);
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
        Port::SleepMs(11);
    }

    LOG_ERROR("HimaxChip", __func__, GetStateStr(), "FW reload timeout!");
    return std::unexpected(ChipError::Timeout);
}

namespace {

// ── himax_thp_drv 读 OTP 用到的几个寄存器原语 ─────────────────────────────

ChipResult<> WriteU32(HalDevice* dev, uint32_t addr, uint32_t value) {
    uint8_t b[4];
    detail::ParseAddressLittleEndian(value, b, 4);
    return HimaxProtocol::register_write(dev, addr, b, 4);
}

/// register_write_vertify_1 / _2：写 4 字节后读回，只比较 mask 覆盖的低位
bool WriteVerify(HalDevice* dev, uint32_t addr, uint32_t value, uint32_t mask) {
    if (!WriteU32(dev, addr, value)) return false;
    uint8_t b[4]{};
    if (!HimaxProtocol::register_read(dev, addr, b, 4)) return false;
    const uint32_t got = b[0] | (b[1] << 8) | (b[2] << 16) | (static_cast<uint32_t>(b[3]) << 24);
    return (got & mask) == (value & mask);
}

/// FUN_18001b8a0 / FUN_18001b910：显示驱动寄存器写，地址 (reg | 0x30000) << 12，数据首字节是 reg 本身。
/// 长度不是 4 的倍数时 writeRegisterRaw_intf 再补一笔 0x0C = 0x01。
bool DdRegWrite(HalDevice* dev, std::initializer_list<uint8_t> data) {
    const uint8_t reg = *data.begin();
    const uint32_t len = static_cast<uint32_t>(data.size());
    if (!HimaxProtocol::register_write(dev, (reg | 0x30000u) << 12, data.begin(), len)) return false;
    if (len % 4 != 0) {
        const uint8_t one = 0x01;
        if (!dev->WriteBus(0x0C, nullptr, &one, 1)) return false;
    }
    return true;
}

/// hx_ddreg_read_byte：关 burst，读 ((reg | 0x30000) << 2 | bank) << 10 | offset 的首字节，再开 burst
bool DdRegReadByte(HalDevice* dev, uint8_t reg, uint8_t bank, uint8_t offset, uint8_t& out) {
    uint8_t b[4]{};
    const uint32_t addr = (((reg | 0x30000u) << 2 | (bank & 3u)) << 10) | offset;
    const bool ok = static_cast<bool>(HimaxProtocol::register_read(dev, addr, b, 4));
    if (ok) out = b[0];
    const uint8_t burstOn = 0x13;
    (void)dev->WriteBus(0x0D, nullptr, &burstOn, 1);
    return ok;
}

/// FUN_18001b560（hx_ddreg_set）：开 = 0x9000009C←0xDD、0x90000280←0xA5；关反序清零。每步后等 10 ms。
bool DdRegAccess(HalDevice* dev, bool enable) {
    if (enable) {
        (void)WriteVerify(dev, 0x9000009C, 0xDD, 0xFF);
        Port::SleepMs(10);
        const bool ok = WriteVerify(dev, 0x90000280, 0xA5, 0xFF);
        Port::SleepMs(10);
        return ok;
    }
    (void)WriteVerify(dev, 0x90000280, 0x00, 0xFF);
    Port::SleepMs(10);
    const bool ok = WriteVerify(dev, 0x9000009C, 0x00, 0xFF);
    Port::SleepMs(10);
    return ok;
}

template <typename F>
bool RetryEvery10ms(int extraTries, F&& step) {
    for (int i = 0;; ++i) {
        if (step()) return true;
        if (i >= extraTries) return false;
        Port::SleepMs(10);
    }
}

bool SafeMode(HalDevice* dev, bool on) {
    for (int i = 0; i < 6; ++i) {
        if (HimaxProtocol::safeModeSetRaw(dev, on)) return true;
    }
    return false;
}

// hx_validate_ProjectID 的已知型号表
constexpr std::array<const char*, 4> kKnownProjectIds = {"W273AS2700", "W273AS2710", "W273AS1310", "W273AS1300"};  // 同 Solvers::Tsa::kPanelProjectIds
// getProjectID_OTP 依次试的三个 OTP 槽
constexpr std::array<uint16_t, 3> kOtpProjectSlots = {0x0F18, 0x0F3A, 0x0F5C};

} // namespace

bool Chip::ReadOtpProjectSlot(uint16_t slot, std::string& id, PanelIdentity& extra) {
    HalDevice* dev = m_master.get();
    if (!SafeMode(dev, true)) {
        LOG_WARN("HimaxChip", __func__, GetStateStr(), "safeModeSetRaw true failed");
        return false;
    }
    if (!RetryEvery10ms(20, [&] { return WriteVerify(dev, 0x9000800C, 0xAC53, 0xFFFF); })) {
        LOG_WARN("HimaxChip", __func__, GetStateStr(), "Failed to LPWUtmrIntAddr");
        return false;
    }
    if (!RetryEvery10ms(20, [&] { return static_cast<bool>(WriteU32(dev, 0x90008010, 0x35CA)); })) {
        LOG_WARN("HimaxChip", __func__, GetStateStr(), "Failed to LPWUtmrCtrlAddr");
        return false;
    }
    if (!RetryEvery10ms(20, [&] { return DdRegAccess(dev, true); })) {
        LOG_WARN("HimaxChip", __func__, GetStateStr(), "Failed to hx_ddreg_set(true)");
        return false;
    }

    // EB / B9 两道口令，BB 选 OTP 地址后触发读出，等 50 ms 再从 B1 bank 3 取 10 个字符
    bool ok = DdRegWrite(dev, {0xEB, 0x55, 0x66, 0xCC});
    Port::SleepMs(10);
    ok &= DdRegWrite(dev, {0xB9, 0x83, 0x12, 0x1A});
    Port::SleepMs(10);
    ok &= DdRegWrite(dev, {0xBB, 0x02, static_cast<uint8_t>(slot >> 8), static_cast<uint8_t>(slot)});
    Port::SleepMs(10);
    ok &= DdRegWrite(dev, {0xBB, 0x0A});
    Port::SleepMs(50);

    char chars[10]{};
    for (uint8_t i = 0; i < 10; ++i) {
        uint8_t c = 0;
        ok &= DdRegReadByte(dev, 0xB1, 3, static_cast<uint8_t>(1 + i), c);
        chars[i] = static_cast<char>(c);
        Port::SleepMs(1);
    }
    id.assign(chars, strnlen(chars, sizeof(chars)));

    // BB 写回原值
    static constexpr std::array<uint8_t, 12> kBbRestore = {0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0x0B};
    ok &= static_cast<bool>(HimaxProtocol::register_write(dev, 0x300BB000, kBbRestore.data(), static_cast<uint32_t>(kBbRestore.size())));
    Port::SleepMs(10);
    (void)DdRegReadByte(dev, 0xB4, 0, 8, extra.revisionCode);
    (void)DdRegReadByte(dev, 0xE2, 0, 1, extra.e2);

    if (!RetryEvery10ms(20, [&] { return DdRegAccess(dev, false); })) {
        LOG_WARN("HimaxChip", __func__, GetStateStr(), "Failed to hx_ddreg_set(false)");
        return false;
    }
    if (!SafeMode(dev, false)) {
        LOG_WARN("HimaxChip", __func__, GetStateStr(), "safeModeSetRaw false failed");
        return false;
    }
    Port::SleepMs(10);
    return ok;
}

void Chip::ReadPanelIdentity() {
    PanelIdentity panel;
    std::string id;
    for (std::size_t i = 0; i < kOtpProjectSlots.size(); ++i) {
        if (!ReadOtpProjectSlot(kOtpProjectSlots[i], id, panel)) {
            LOG_WARN("HimaxChip", __func__, GetStateStr(), "OTP read failed at slot 0x{:03X}", kOtpProjectSlots[i]);
            return;
        }
        panel.known = std::find_if(kKnownProjectIds.begin(), kKnownProjectIds.end(),
                                   [&](const char* k) { return id == k; }) != kKnownProjectIds.end();
        if (panel.known) break;
        LOG_WARN("HimaxChip", __func__, GetStateStr(), "Got wrong id = {}, re-try!", id);
    }
    panel.projectId = id;
    LOG_INFO("HimaxChip", __func__, GetStateStr(), "Project ID = {} (known={}, revision=0x{:02X}, E2h=0x{:02X})",
             panel.projectId, panel.known, panel.revisionCode, panel.e2);
    std::lock_guard lock(m_panelMu);
    m_panel = std::move(panel);
}

std::optional<PanelIdentity> Chip::GetPanelIdentity() const {
    std::lock_guard lock(m_panelMu);
    return m_panel;
}

ChipResult<> Chip::InitFailed(const char* step, const ChipResult<>& res) {
    m_lastInitFailStep = step;
    LOG_WARN("HimaxChip", "Init", GetStateStr(), "Init failed at step '{}': {} (master err={}, slave err={}).",
             step, ToString(res.error()), m_master ? m_master->GetError() : 0u,
             m_slave ? m_slave->GetError() : 0u);
    return res;
}

ChipResult<> Chip::Init(void) {
    if (auto res = hx_hw_reset_ahb_intf(DeviceType::Master); !res) return InitFailed("hw_reset", res);
    Port::SleepMs(10);
    // 原厂每次 open_project 都先读 OTP 再复位开扫；面板不会变，这里只在第一次建链时读，读完再复位一次，
    // 留下的安全模式 / LPWU 定时器改动都随复位清掉
    if (!m_panelTried) {
        m_panelTried = true;
        ReadPanelIdentity();
        if (auto res = hx_hw_reset_ahb_intf(DeviceType::Master); !res) return InitFailed("hw_reset", res);
        Port::SleepMs(10);
    }
    LOG_INFO("HimaxChip", __func__, GetStateStr(), "Starting initialization sequence...");

    std::array<uint8_t, 4> tmp_data = {0xA5, 0x5A, 0x00, 0x00};

    if (auto res = himax_mcu_reload_disable(false); !res) return InitFailed("reload_disable", res);
    if (auto res = himax_switch_mode_inspection(THP_INSPECTION_ENUM::EGO_RAWDATA); !res) return InitFailed("switch_mode_inspection", res);
    if (auto res = hx_set_N_frame(1); !res) return InitFailed("set_N_frame", res);

    if (auto res = himax_mcu_power_on_init(); !res) return InitFailed("power_on_init", res);

    if (auto res = himax_switch_data_type(DeviceType::Master, THP_INSPECTION_ENUM::EGO_RAWDATA); !res) return InitFailed("master_data_type", res);
    if (auto res = HimaxProtocol::register_write(m_master.get(), psram_op.addr_rawdata_addr, tmp_data.data(), 4); !res) return InitFailed("master_rawdata_addr", res);
    if (m_slaveEnabled) {
        if (auto res = himax_switch_data_type(DeviceType::Slave, THP_INSPECTION_ENUM::EGO_RAWDATA); !res) return InitFailed("slave_data_type", res);
        if (auto res = HimaxProtocol::register_write(m_slave.get(), psram_op.addr_rawdata_addr, tmp_data.data(), 4); !res) return InitFailed("slave_rawdata_addr", res);
    }

    if (auto res = m_master->IntOpen(); !res) {
        return InitFailed("master_int_open", res);
    }
    if (m_slaveEnabled) {
        if (auto res = m_slave->IntOpen(); !res) {
            (void)m_slave->IntClose();
            (void)m_master->IntClose();
            return InitFailed("slave_int_open", res);
        }
    }

    if (auto res = ApplyFrameReadPolicy(); !res) {
        (void)m_slave->IntClose();
        (void)m_master->IntClose();
        return InitFailed("read_policy", res);
    }

    // thp_afe_start 的「start cleaning buffer」：复位前攒在通道里的旧帧读掉丢弃，先 slave 后 master，
    // 读不到就重开那一路的中断通道。不清的话第一帧会是上一轮留下的（常见是一帧 IDLE 帧）。
    int staleFrames = 0;
    for (int i = 0; i < 3; ++i) {
        if (!m_slaveEnabled) {
            // 只用 master：slave 没开中断，不读
        } else if (m_slave->GetFrame(back_data.data() + Frame::kSlaveHeaderOffset, static_cast<uint32_t>(Frame::kSlaveFrameSize), nullptr)) {
            ++staleFrames;
        } else {
            (void)m_slave->IntOpen();
        }
        if (m_master->GetFrame(back_data.data(), static_cast<uint32_t>(Frame::kMasterFrameSize), nullptr)) {
            ++staleFrames;
        } else {
            (void)m_master->IntOpen();
        }
    }
    LOG_INFO("HimaxChip", __func__, GetStateStr(), "Discarded {} buffered frame(s) after reset.", staleFrames);

    m_lastInitFailStep = "";
    current_slot = 0;
    ResetFrameReadState();
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
    ResetFrameReadState();
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

void Chip::Quiesce() {
    m_connState.store(ConnectionState::Unconnected);
    ResetFrameReadState();
    current_slot = 0;
    m_afe.ResetStylusState();

    // 对齐 thp_afe_screen_off：不拉 reset，不发总线命令。TDDI 的触摸部分随显示链路
    // 上下电，reset 一直压低会让它在亮屏时没法正常完成初始化。
    if (m_master) (void)m_master->IntClose();
    if (m_slave)  (void)m_slave->IntClose();

    LOG_INFO("HimaxChip", __func__, "Unconnected", "Interrupt channels closed.");
}

} // namespace Himax
