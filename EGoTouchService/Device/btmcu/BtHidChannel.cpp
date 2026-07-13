#include "btmcu/BtHidChannel.h"
#include "Logger.h"

#include <chrono>
#include <span>

namespace Himax::Pen {

BtHidChannel::BtHidChannel()
    : m_transport(CreatePenUsbTransportWin32())
{
}

BtHidChannel::~BtHidChannel() {
    if (IsRunning()) {
        LOG_ERROR("BtHidChannel", "~BtHidChannel", "FATAL",
                  "Destroyed while still running! Call Stop() before destruction.");
    }
}

bool BtHidChannel::Start() {
    if (m_running.exchange(true)) return true;
    try {
        m_thread = std::thread(&BtHidChannel::WorkerFunc, this);
    } catch (...) {
        m_running.store(false);
        LOG_ERROR(ChannelName(), "Start", "MCU",
                  "Failed to create channel worker thread.");
        return false;
    }
    LOG_INFO(ChannelName(), "Start", "MCU",
             "Channel thread launched.");
    return true;
}

void BtHidChannel::Stop() {
    if (!m_running.exchange(false)) return;
    if (m_transport) m_transport->CancelIo();
    if (m_thread.joinable()) m_thread.join();
    if (m_transport) m_transport->Close();
    LOG_INFO(ChannelName(), "Stop", "MCU",
             "Channel stopped.");
}

void BtHidChannel::WorkerFunc() {
    LOG_INFO(ChannelName(), "WorkerFunc", "MCU", "[Thread] Started.");

    while (m_running.load()) {
        bool opened = false;
        while (m_running.load() && !opened) {
            auto path = FindDevicePath();
            if (path) {
                std::string pathA(path->begin(), path->end());
                LOG_INFO(ChannelName(), "WorkerFunc", "MCU",
                         "[Thread] Found device: {}", pathA);
                auto res = m_transport->Open(*path);
                if (res) {
                    LOG_INFO(ChannelName(), "WorkerFunc", "MCU",
                             "[Thread] Channel opened successfully.");
                    opened = true;
                    break;
                }
                LOG_WARN(ChannelName(), "WorkerFunc", "MCU",
                         "[Thread] Open failed for device, retry...");
            } else {
                LOG_WARN(ChannelName(), "WorkerFunc", "MCU",
                         "[Thread] Device not found, retry in 2s...");
            }
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }

        if (!m_running.load() || !opened) {
            break;
        }

        OnConnected();

        while (m_running.load() && m_transport && m_transport->IsOpen()) {
            PenUsbPacketBuffer rxBuf;
            auto res = m_transport->ReadPacket(rxBuf, 1000);
            if (res && !rxBuf.empty()) {
                OnPacketReceived(rxBuf.view());
                continue;
            }
            if (!res && res.error() == ChipError::Timeout) {
                continue;
            }

            LOG_WARN(ChannelName(), "WorkerFunc", "MCU",
                     "[Thread] Read failed or returned empty packet; reopening channel.");
            if (m_transport) {
                m_transport->Close();
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
            break;
        }
    }

    LOG_INFO(ChannelName(), "WorkerFunc", "MCU", "[Thread] Exited.");
}

} // namespace Himax::Pen
