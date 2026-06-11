#pragma once
// GuiLogSink: Thread-safe buffer for log entries.
// Shared by both EGoTouchApp and EGoTouchService.
// Decoupled from spdlog to minimize binary footprint.

#include <mutex>
#include <string>
#include <deque>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace Common {

class GuiLogSink {
public:
    static constexpr int kDebugMaxLines = 2000;
    static constexpr int kReleaseMaxLines = 500;
#if defined(NDEBUG)
    static constexpr int kMaxLines = kReleaseMaxLines;
#else
    static constexpr int kMaxLines = kDebugMaxLines;
#endif

    static std::shared_ptr<GuiLogSink> Instance() {
        static auto inst = std::make_shared<GuiLogSink>();
        return inst;
    }

    // Optional event to signal when new log lines arrive (Service-side)
    void SetNotifyEvent(HANDLE h) {
        std::lock_guard<std::mutex> lk(mutex_);
        m_notifyEvent = h;
    }

    // Read all buffered lines (thread-safe copy)
    std::deque<std::string> GetLines() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return lines_;
    }

    // Drain new lines since last drain (for IPC forwarding)
    std::deque<std::string> DrainNewLines() {
        std::lock_guard<std::mutex> lk(mutex_);
        std::deque<std::string> result;
        result.swap(pendingLines_);
        return result;
    }

    // Directly insert a pre-formatted string (e.g. forwarded Service logs)
    void PushRaw(const std::string& line) {
        std::lock_guard<std::mutex> lk(mutex_);
        lines_.push_back(line);
        pendingLines_.push_back(line);
        while (static_cast<int>(lines_.size()) > kMaxLines)
            lines_.pop_front();
        while (static_cast<int>(pendingLines_.size()) > kMaxLines)
            pendingLines_.pop_front();
        if (m_notifyEvent) SetEvent(m_notifyEvent);
    }

    void Clear() {
        std::lock_guard<std::mutex> lk(mutex_);
        lines_.clear();
        pendingLines_.clear();
    }

private:
    mutable std::mutex mutex_;
    std::deque<std::string> lines_;
    std::deque<std::string> pendingLines_;  // for DrainNewLines
    HANDLE m_notifyEvent = nullptr;
};

} // namespace Common
