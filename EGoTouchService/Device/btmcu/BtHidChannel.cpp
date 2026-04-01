#include "btmcu/BtHidChannel.h"
#include "Logger.h"

#include <chrono>

namespace Himax::Pen {

BtHidChannel::BtHidChannel()
    : m_transport(CreatePenUsbTransportWin32())
{
}

BtHidChannel::~BtHidChannel() {
    Stop();
}

void BtHidChannel::Start() {
    if (m_running.exchange(true)) return;
    m_thread = std::thread(&BtHidChannel::WorkerFunc, this);
    LOG_INFO(ChannelName(), "Start", "MCU",
             "Channel thread launched.");
}

void BtHidChannel::Stop() {
    if (!m_running.exchange(false)) return;
    if (m_transport) m_transport->Close();
    if (m_thread.joinable()) m_thread.join();
    LOG_INFO(ChannelName(), "Stop", "MCU",
             "Channel stopped.");
}

void BtHidChannel::WorkerFunc() {
    LOG_INFO(ChannelName(), "WorkerFunc", "MCU", "[Thread] Started.");

    // ── 自动设备发现（重试直到成功或 Stop()）──
    while (m_running.load()) {
        auto path = FindDevicePath();
        if (path) {
            auto res = m_transport->Open(*path);
            if (res) {
                LOG_INFO(ChannelName(), "WorkerFunc", "MCU",
                         "[Thread] Channel opened.");
                break;
            }
        }
        LOG_WARN(ChannelName(), "WorkerFunc", "MCU",
                 "[Thread] Device not found, retry in 2s...");
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    // ── 连接后回调（握手等）──
    if (m_running.load()) {
        OnConnected();
    }

    // ── 循环读取 ──
    while (m_running.load()) {
        std::vector<uint8_t> rxBuf;
        auto res = m_transport->ReadPacket(rxBuf, 1000);
        if (!res || rxBuf.empty()) continue;

        OnPacketReceived(rxBuf);
    }

    LOG_INFO(ChannelName(), "WorkerFunc", "MCU", "[Thread] Exited.");
}

} // namespace Himax::Pen
