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

Chip::Chip(const std::filesystem::path& master_path, const std::filesystem::path& slave_path)
    : pic_op(InitIcOperation()),
      pfw_op(InitFwOperation()),
      pflash_op(InitFlashOperation()),
      psram_op(InitSramOperation()),
      pdriver_op(InitDriverOperation()),
      pzf_op(InitZfOperation()),
      m_masterPath(master_path),
      m_slavePath(slave_path),
      m_afe(*this)
{
    m_master = Port::CreateTouchBus(m_masterPath, DeviceType::Master);
    m_slave = Port::CreateTouchBus(m_slavePath, DeviceType::Slave);

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
    return m_afe.GetStylusStateSnapshot().connected;
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

Chip::BusProbe Chip::ProbeBus() {
    BusProbe probe{};
    if (!m_master || !m_master->IsValid() || !m_slave || !m_slave->IsValid()) {
        return probe;
    }
    uint8_t slave[4]{};
    uint8_t master[4]{};
    probe.slaveRequired = m_slaveEnabled;
    if (m_slaveEnabled) {
        if (auto res = m_slave->ReadBus(0x13, slave, 1); !res) {
            probe.nativeError = m_slave->GetError();
            return probe;
        }
    }
    if (auto res = m_master->ReadBus(0x13, master, 1); !res) {
        probe.nativeError = m_master->GetError();
        return probe;
    }
    probe.ioOk = true;
    probe.slave = slave[0];
    probe.master = master[0];
    return probe;
}

ChipResult<> Chip::check_bus(void) {
    const BusProbe probe = ProbeBus();
    if (!probe.ioOk) {
        LOG_WARN("HimaxChip", __func__, GetStateStr(), "Bus read failed (err={}).", probe.nativeError);
        return std::unexpected(ChipError::CommunicationError);
    }
    if (!probe.Ready()) {
        LOG_WARN("HimaxChip", __func__, GetStateStr(), "Bus not ready: 0x13 master=0x{:02X} slave=0x{:02X}.",
                 probe.master, probe.slave);
        return std::unexpected(ChipError::InvalidOperation);
    }
    return {};
}

void Chip::CancelPendingFrameRead() {
    if (!m_frameReadActive.load(std::memory_order_acquire)) return;
    std::lock_guard<std::mutex> lock(m_devMu);
    if (m_master) m_master->RequestCancel();
    if (m_slave)  m_slave->RequestCancel();
}

bool Chip::ReopenDevices() {
    std::lock_guard<std::mutex> lock(m_devMu);
    // 先全部关掉再开：SPB 测试驱动的设备对象不保证能被同一进程重复打开
    m_master.reset();
    m_slave.reset();
    m_master = Port::CreateTouchBus(m_masterPath, DeviceType::Master);
    m_slave = Port::CreateTouchBus(m_slavePath, DeviceType::Slave);

    const bool ok = m_master->IsValid() && m_slave->IsValid();
    if (ok) {
        LOG_WARN("HimaxChip", __func__, GetStateStr(), "Device handles reopened.");
    } else {
        LOG_ERROR("HimaxChip", __func__, GetStateStr(), "Reopen failed: master err={}, slave err={}.",
                  m_master->GetError(), m_slave->GetError());
    }
    return ok;
}

std::optional<bool> Chip::ReadPanelPower() {
    if (!m_master || !m_master->IsValid()) return std::nullopt;
    uint8_t value[4]{};
    if (auto res = m_master->ReadPanelStatus(value, sizeof(value)); !res) return std::nullopt;
    return (value[0] & 0x01) != 0;
}

} // namespace Himax
