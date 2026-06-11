/**
 * @file Logger.cpp
 * @brief 自研轻量级 MiniLogger 的实现 (零 C++ Streams 依赖优化版)
 */
#include "Logger.h"
#include "GuiLogSink.h"
#include <mutex>
#include <chrono>
#include <filesystem>
#include <ctime>
#include <cctype>
#include <cstdio>

namespace MiniFmt {

static void format_single_arg(std::string& out, const std::string& spec, const LogArg& arg) {
    char buf[256];
    buf[0] = '\0';

    if (spec.empty()) {
        switch (arg.type) {
            case LogArg::Type::Int:
                snprintf(buf, sizeof(buf), "%d", static_cast<int>(arg.i_val));
                break;
            case LogArg::Type::UInt:
                snprintf(buf, sizeof(buf), "%u", static_cast<unsigned int>(arg.u_val));
                break;
            case LogArg::Type::LongLong:
                snprintf(buf, sizeof(buf), "%lld", arg.i_val);
                break;
            case LogArg::Type::ULongLong:
                snprintf(buf, sizeof(buf), "%llu", arg.u_val);
                break;
            case LogArg::Type::Double:
                snprintf(buf, sizeof(buf), "%f", arg.d_val);
                break;
            case LogArg::Type::String:
                out += arg.s_val;
                return;
            case LogArg::Type::Pointer:
                snprintf(buf, sizeof(buf), "%p", arg.p_val);
                break;
        }
        out += buf;
        return;
    }

    if (spec[0] == '<') {
        int width = 0;
        try {
            width = std::stoi(spec.substr(1));
        } catch (...) {}

        std::string base_str;
        switch (arg.type) {
            case LogArg::Type::Int:
                snprintf(buf, sizeof(buf), "%d", static_cast<int>(arg.i_val));
                base_str = buf;
                break;
            case LogArg::Type::UInt:
                snprintf(buf, sizeof(buf), "%u", static_cast<unsigned int>(arg.u_val));
                base_str = buf;
                break;
            case LogArg::Type::LongLong:
                snprintf(buf, sizeof(buf), "%lld", arg.i_val);
                base_str = buf;
                break;
            case LogArg::Type::ULongLong:
                snprintf(buf, sizeof(buf), "%llu", arg.u_val);
                base_str = buf;
                break;
            case LogArg::Type::Double:
                snprintf(buf, sizeof(buf), "%f", arg.d_val);
                base_str = buf;
                break;
            case LogArg::Type::String:
                base_str = arg.s_val;
                break;
            case LogArg::Type::Pointer:
                snprintf(buf, sizeof(buf), "%p", arg.p_val);
                base_str = buf;
                break;
        }
        
        if (width > 0) {
            std::string fmt_align = "%-" + std::to_string(width) + "s";
            char align_buf[512];
            snprintf(align_buf, sizeof(align_buf), fmt_align.c_str(), base_str.c_str());
            out += align_buf;
        } else {
            out += base_str;
        }
        return;
    }

    bool uppercase = false;
    bool hex_mode = false;
    bool fill_zero = false;
    int width = 0;

    size_t idx = 0;
    if (idx < spec.size() && spec[idx] == '0') {
        fill_zero = true;
        idx++;
    }
    size_t num_start = idx;
    while (idx < spec.size() && std::isdigit(static_cast<unsigned char>(spec[idx]))) {
        idx++;
    }
    if (idx > num_start) {
        try {
            width = std::stoi(spec.substr(num_start, idx - num_start));
        } catch (...) {}
    }
    if (idx < spec.size()) {
        char ch = spec[idx];
        if (ch == 'x' || ch == 'X') {
            hex_mode = true;
            if (ch == 'X') uppercase = true;
        }
    }

    std::string fmt_str = "%";
    if (fill_zero) fmt_str += "0";
    if (width > 0) fmt_str += std::to_string(width);
    
    if (hex_mode) {
        if (uppercase) fmt_str += "llX";
        else fmt_str += "llx";
    } else {
        if (arg.type == LogArg::Type::UInt || arg.type == LogArg::Type::ULongLong) {
            fmt_str += "llu";
        } else {
            fmt_str += "lld";
        }
    }

    if (arg.type == LogArg::Type::UInt || arg.type == LogArg::Type::ULongLong) {
        snprintf(buf, sizeof(buf), fmt_str.c_str(), arg.u_val);
    } else if (arg.type == LogArg::Type::Double) {
        snprintf(buf, sizeof(buf), fmt_str.c_str(), static_cast<long long>(arg.d_val));
    } else if (arg.type == LogArg::Type::Pointer) {
        snprintf(buf, sizeof(buf), fmt_str.c_str(), reinterpret_cast<uintptr_t>(arg.p_val));
    } else if (arg.type == LogArg::Type::String) {
        try {
            long long val = std::stoll(arg.s_val);
            snprintf(buf, sizeof(buf), fmt_str.c_str(), val);
        } catch (...) {
            snprintf(buf, sizeof(buf), "%s", arg.s_val.c_str());
        }
    } else {
        snprintf(buf, sizeof(buf), fmt_str.c_str(), arg.i_val);
    }
    out += buf;
}

std::string format_core(const char* fmt_str, const LogArg* args, size_t count) {
    std::string out;
    out.reserve(256);
    size_t arg_idx = 0;
    
    while (*fmt_str) {
        if (*fmt_str == '{') {
            const char* end = fmt_str + 1;
            while (*end && *end != '}') {
                end++;
            }
            if (*end == '}') {
                std::string spec(fmt_str + 1, end);
                if (arg_idx < count) {
                    format_single_arg(out, spec, args[arg_idx++]);
                } else {
                    out += "{";
                    out += spec;
                    out += "}";
                }
                fmt_str = end + 1;
                continue;
            }
        }
        out += *fmt_str;
        fmt_str++;
    }
    return out;
}

} // namespace MiniFmt

namespace Common {

namespace {
FILE* g_fileStream = nullptr;
std::mutex g_logMutex;
std::filesystem::path g_logPath;
bool g_initialized = false;
} // namespace

void Logger::Init(const std::string& loggerName, const std::filesystem::path& logDir, std::shared_ptr<GuiLogSink> extraSink) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (g_initialized) {
        return;
    }

    try {
        namespace fs = std::filesystem;
        std::error_code ec;
        fs::create_directories(logDir, ec);
        if (ec) {
            fprintf(stderr, "Failed to create log directory: %s\n", ec.message().c_str());
            return;
        }

        g_logPath = logDir / (loggerName + ".txt");

        bool truncate = false;
        if (fs::exists(g_logPath, ec) && !ec) {
            if (fs::file_size(g_logPath, ec) > 5 * 1024 * 1024 && !ec) {
                truncate = true;
            }
        }

#if defined(_WIN32)
        std::wstring wpath = g_logPath.wstring();
        errno_t err = _wfopen_s(&g_fileStream, wpath.c_str(), truncate ? L"wb" : L"ab");
        g_initialized = (err == 0 && g_fileStream != nullptr);
#else
        g_fileStream = fopen(g_logPath.string().c_str(), truncate ? "wb" : "ab");
        g_initialized = (g_fileStream != nullptr);
#endif

        if (g_initialized) {
            const char* separator = "========================================\n";
            fwrite(separator, 1, strlen(separator), g_fileStream);
            fflush(g_fileStream);
        } else {
            fprintf(stderr, "Failed to open log file: %s\n", g_logPath.string().c_str());
        }

    } catch (const std::exception& ex) {
        fprintf(stderr, "Log initialization failed: %s\n", ex.what());
    }
}

void Logger::Shutdown() {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (!g_initialized) {
        return;
    }

    if (g_fileStream) {
        fclose(g_fileStream);
        g_fileStream = nullptr;
    }
    g_initialized = false;
}

void Logger::Log(const char* level, const char* layer, const char* method, const char* state, const std::string& message) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (!g_initialized) {
        return;
    }

    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::tm tm_now{};
    localtime_s(&tm_now, &time_t_now);

    std::string timeStr = MiniFmt::format(
        "{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}.{:03d}",
        tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday,
        tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec,
        static_cast<int>(ms.count())
    );

    std::string formatted_line = MiniFmt::format(
        "[{}] [{:<7}] [{}] [{}] [{}] {}",
        timeStr, level, layer, method, state, message
    );

    if (g_fileStream) {
        fwrite(formatted_line.c_str(), 1, formatted_line.size(), g_fileStream);
        fputc('\n', g_fileStream);
        fflush(g_fileStream);
    }

#if !defined(NDEBUG)
    fprintf(stderr, "%s\n", formatted_line.c_str());
#endif

    GuiLogSink::Instance()->PushRaw(formatted_line);
}

} // namespace Common
