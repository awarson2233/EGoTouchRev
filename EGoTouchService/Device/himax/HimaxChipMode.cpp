#include "himax/HimaxChip.h"
#include "himax/HimaxProtocol.h"
#include "himax/HimaxByteUtils.h"
#include "Logger.h"

#include <array>
#include <string>

namespace Himax {

ChipResult<> Chip::himax_mcu_assign_sorting_mode(uint8_t* tmp_data) {
    bool step_ok = false;
    for (int i = 0; i < 10; ++i) {
        if (HimaxProtocol::write_and_verify(m_master.get(), pfw_op.addr_sorting_mode_en, tmp_data, 4)) {
            step_ok = true;
            break;
        }
    }
    if (!step_ok) {
        LOG_ERROR("HimaxChip", __func__, GetStateStr(), "Failed to assign sorting mode!");
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
        LOG_ERROR("HimaxChip", __func__, GetStateStr(), "get device failed");
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
    LOG_INFO("HimaxChip", __func__, GetStateStr(), "switch to {}!", message);

    ChipResult<> step_ok = std::unexpected(ChipError::InternalError);
    do {
        step_ok = HimaxProtocol::write_and_verify(dev, pfw_op.addr_raw_out_sel, tmp_data.data(), 4);
    } while (!step_ok && --cnt > 0);

    if (step_ok) {
        LOG_INFO("HimaxChip", __func__, GetStateStr(), "switch to {} success!", message);
        return {};
    }

    LOG_ERROR("HimaxChip", __func__, GetStateStr(), "switch failed!");
    return step_ok;
}

ChipResult<> Chip::hx_set_N_frame(uint8_t nFrame) {
    if (!m_master || !m_master->IsValid()) return std::unexpected(ChipError::CommunicationError);
    LOG_INFO("HimaxChip", __func__, GetStateStr(), "Enter!");

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

    LOG_INFO("HimaxChip", __func__, GetStateStr(), "Out!");
    return {};
}

ChipResult<> Chip::himax_switch_mode_inspection(THP_INSPECTION_ENUM mode) {
    std::string message;
    constexpr int kUnlockAttempts = 20;
    std::array<uint8_t, 4> tmp_data{};
    bool clear = false;
    LOG_INFO("HimaxChip", __func__, GetStateStr(), "Entering!");

    detail::ParseAddressLittleEndian(psram_op.addr_rawdata_end, tmp_data.data(), 4);
    for (size_t i = 0; i < kUnlockAttempts; i++) {
        if (HimaxProtocol::write_and_verify(m_master.get(), psram_op.addr_rawdata_addr, tmp_data.data(), 4, 2)) {
            clear = true;
            break;
        }
    }
    if (!clear) {
        LOG_ERROR("HimaxChip", __func__, GetStateStr(), "switch mode unlock failed after {} attempts", kUnlockAttempts);
        return std::unexpected(ChipError::VerificationFailed);
    }

    tmp_data.fill(0);
    switch (mode) {
    case THP_INSPECTION_ENUM::HX_RAWDATA:
        tmp_data[0] = 0x00;
        tmp_data[1] = 0x00;
        message = "HX_RAWDATA";
        break;
    case THP_INSPECTION_ENUM::HX_ACT_IDLE_RAWDATA:
        tmp_data[0] = 0x22;
        tmp_data[1] = 0x22;
        message = "HX_ACT_IDLE_RAWDATA";
        break;
    case THP_INSPECTION_ENUM::HX_LP_IDLE_RAWDATA:
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
        LOG_ERROR("HimaxChip", __func__, GetStateStr(), "Failed to switch to {}", message);
        return res;
    }

    LOG_INFO("HimaxChip", __func__, GetStateStr(), "Switching to {} Success", message);
    return {};
}

} // namespace Himax
