#pragma once

#include "btmcu/PenUsbTransport.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <thread>

namespace Himax::Pen {

/// BtHidChannel — Base class for BT MCU HID channels.
///
/// Encapsulates the shared pattern of both PenEventBridge (col00) and
/// PenPressureReader (col01):
///   - Transport lifecycle (create, open, close)
///   - Worker thread management (start, stop, join)
///   - Automatic device discovery with retry loop
///
/// Subclasses override:
///   - FindDevicePath()  — return the channel-specific HID device path
///   - OnConnected()     — called once after successful Open (e.g. handshake)
///   - OnPacketReceived()— called for each received packet in the read loop
///   - ChannelName()     — return a human-readable name for logging
class BtHidChannel {
public:
    /// ⚠️  Derived classes MUST call Stop() in their destructors before
    /// the base class destructor runs. Failure to do so will cause a
    /// use-after-destroy crash in the worker thread.
    virtual ~BtHidChannel();

    BtHidChannel(const BtHidChannel&) = delete;
    BtHidChannel& operator=(const BtHidChannel&) = delete;

    bool Start();
    /// MUST be called before destruction so the worker cannot dispatch into
    /// already-destroyed subclass state.
    void Stop();
    bool IsRunning() const { return m_running.load(); }

protected:
    BtHidChannel();

    /// Access to the underlying transport for subclass send operations.
    IPenUsbTransport* GetTransport() { return m_transport.get(); }
    bool IsTransportOpen() const {
        return m_transport && m_transport->IsOpen();
    }

    // ── Subclass hooks ────────────────────────────────────────────
    /// Return the device path for this channel, or nullopt if not found.
    virtual std::optional<std::wstring> FindDevicePath() = 0;

    /// Called once after the transport is successfully opened.
    /// Default: no-op.
    virtual void OnConnected() {}

    /// Called for each received packet.
    virtual void OnPacketReceived(std::span<const uint8_t> packet) = 0;

    /// Return a short name for log messages (e.g. "PenEventBridge").
    virtual const char* ChannelName() const = 0;

private:
    void WorkerFunc();

    std::atomic<bool> m_running{false};
    std::unique_ptr<IPenUsbTransport> m_transport;
    std::thread m_thread;
};

} // namespace Himax::Pen
