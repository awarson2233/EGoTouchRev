#pragma once
// GuiLogSink: spdlog sink that buffers log entries for display.
// Shared by both EGoTouchApp and EGoTouchService.

#include <spdlog/sinks/base_sink.h>
#include <mutex>
#include <string>
#include <deque>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace Common {

class GuiLogSink : public spdlog::sinks::base_sink<std::mutex> {
public:
    static constexpr int kMaxLines = 2000;

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
    // without running it through the spdlog formatter.
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

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        spdlog::memory_buf_t formatted;
        formatter_->format(msg, formatted);
        std::lock_guard<std::mutex> lk(mutex_);
        std::string line = fmt::to_string(formatted);
        if (!line.empty() && line.back() == '\n') line.pop_back();
        lines_.push_back(line);
        pendingLines_.push_back(line);
        while (static_cast<int>(lines_.size()) > kMaxLines)
            lines_.pop_front();
        while (static_cast<int>(pendingLines_.size()) > kMaxLines)
            pendingLines_.pop_front();
        if (m_notifyEvent) SetEvent(m_notifyEvent);
    }

    void flush_() override {}

private:
    mutable std::mutex mutex_;
    std::deque<std::string> lines_;
    std::deque<std::string> pendingLines_;  // for DrainNewLines
    HANDLE m_notifyEvent = nullptr;
};

} // namespace Common
