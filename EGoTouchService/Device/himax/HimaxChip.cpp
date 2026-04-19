/*
 * @Author: Detach0-0 detach0-0@outlook.com
 * @Date: 2025-12-05 11:45:27
 * @LastEditors: Detach0-0 detach0-0@outlook.com
 * @LastEditTime: 2026-04-19
 * @FilePath: \EGoTouchRev-vsc\HimaxChipCore\source\HimaxChip.cpp
 */
#include "himax/HimaxChip.h"
#include "himax/HimaxProtocol.h"
#include "Logger.h"
#include "FrameLayout.h"

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

Chip::~Chip() {
    if (m_connState.load() != ConnectionState::Unconnected) {
        LOG_INFO("HimaxChip", __func__, GetStateStr(), "Implicitly calling Deinit().");
        (void)Deinit(false);
    }
}

ChipResult<> Chip::SendAfeCommand(command cmd) {
    return m_afe.SendCommand(cmd);
}

bool Chip::IsStylusConnected() const {
    return m_afe.GetStylusState().connected;
}

uint16_t Chip::GetLastFrameTimestamp() const {
    if (back_data.size() < Frame::kMasterFrameSize) return 0;
    Frame::MasterSuffixView masterSuffix;
    masterSuffix.LoadFromBytes(back_data.data() + Frame::kMasterSuffixOffset);
    return masterSuffix.timestamp();
}

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

bool Chip::IsReady(DeviceType type) const {
    return const_cast<Chip*>(this)->SelectDevice(type).has_value();
}

ChipResult<> Chip::check_bus(void) {
    uint8_t tmp_data[4]{};
    uint8_t tmp_data1[4]{};

    // Check Slave
    if (auto res = m_slave->ReadBus(0x13, tmp_data, 1); !res) {
        LOG_ERROR("HimaxChip", __func__, GetStateStr(), "Slave Bus Dead! Error: {:d}",  static_cast<int>(res.error()));
        return std::unexpected(ChipError::CommunicationError);
    }
    LOG_INFO("HimaxChip", __func__, GetStateStr(), "Slave Bus Alive! (0x13: 0x{:02X})",  tmp_data[0]);

    // Check Master
    if (auto res = m_master->ReadBus(0x13, tmp_data1, 1); !res) {
        LOG_ERROR("HimaxChip", __func__, GetStateStr(), "Master Bus Dead! Error: {:d}",  static_cast<int>(res.error()));
        return std::unexpected(ChipError::CommunicationError);
    }
    LOG_INFO("HimaxChip", __func__, GetStateStr(), "Master Bus Alive! (0x13: 0x{:02X})",  tmp_data1[0]);

    return {};
}

void Chip::CancelPendingFrameRead() {
    if (m_master) m_master->CancelPendingIo();
    if (m_slave)  m_slave->CancelPendingIo();
}

} // namespace Himax
