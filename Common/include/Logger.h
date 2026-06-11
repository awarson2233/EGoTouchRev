/**
 * @file Logger.h
 * @brief 通用日志模块 (自研轻量级 MiniLogger)
 * @description 零 spdlog 依赖，支持多线程安全、文件持久化与 GuiLogSink 转发。
 * 强制层次化前缀: [Layer][Class::Method][State] Message
 */
#pragma once

#include <string>
#include <memory>
#include <filesystem>
#include <type_traits>

namespace MiniFmt {

struct LogArg {
    enum class Type {
        Int,
        UInt,
        LongLong,
        ULongLong,
        Double,
        String,
        Pointer
    };

    Type type;
    union {
        long long i_val;
        unsigned long long u_val;
        double d_val;
        const void* p_val;
    };
    std::string s_val;

    LogArg() : type(Type::Int), i_val(0) {}

    template <typename T, typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value && !std::is_same<T, bool>::value, int>::type = 0>
    LogArg(T v) : type(Type::LongLong), i_val(static_cast<long long>(v)) {}

    template <typename T, typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value && !std::is_same<T, bool>::value, int>::type = 0>
    LogArg(T v) : type(Type::ULongLong), u_val(static_cast<unsigned long long>(v)) {}

    LogArg(bool v) : type(Type::Int), i_val(v ? 1 : 0) {}
    LogArg(double v) : type(Type::Double), d_val(v) {}
    LogArg(float v) : type(Type::Double), d_val(static_cast<double>(v)) {}
    LogArg(const char* v) : type(Type::String), s_val(v ? v : "") {}
    LogArg(char* v) : type(Type::String), s_val(v ? v : "") {}
    LogArg(const std::string& v) : type(Type::String), s_val(v) {}
    LogArg(std::string_view v) : type(Type::String), s_val(v) {}

    template <typename T, typename std::enable_if<std::is_pointer<T>::value && !std::is_same<T, const char*>::value && !std::is_same<T, char*>::value, int>::type = 0>
    LogArg(T v) : type(Type::Pointer), p_val(static_cast<const void*>(v)) {}
};

std::string format_core(const char* fmt_str, const LogArg* args, size_t count);

template <typename... Args>
std::string format(const char* fmt_str, const Args&... args) {
    if constexpr (sizeof...(args) == 0) {
        return fmt_str;
    } else {
        LogArg arg_array[sizeof...(args)] = { LogArg(args)... };
        return format_core(fmt_str, arg_array, sizeof...(args));
    }
}

} // namespace MiniFmt



namespace Common {

class GuiLogSink;

class Logger {
public:
    /**
     * @brief 初始化全局日志实例
     * @param loggerName 日志记录器的名称
     * @param logDir 日志存放目录
     */
    static void Init(const std::string& loggerName = "EGoTouch", 
                     const std::filesystem::path& logDir = "C:/ProgramData/EGoTouchRev/logs/",
                     std::shared_ptr<GuiLogSink> extraSink = nullptr);

    /**
     * @brief 关闭并清理日志
     */
    static void Shutdown();

    /**
     * @brief 核心日志输出静态方法
     */
    static void Log(const char* level, const char* layer, const char* method, const char* state, const std::string& message);

    /**
     * @brief 获取底层 logger 桩函数
     */
    static std::shared_ptr<GuiLogSink> Get() { return nullptr; }
};

// ---------------------------------------------------------
// 宏定义：带层次化结构的日志输出
// 格式要求: [Layer][Class::Method][State] Message
// ---------------------------------------------------------

// 底层辅助宏，负责将格式拼接并送入自研 Log 引擎
#define LOG_INTERNAL(level, layer, method, state, msg, ...) \
    Common::Logger::Log(#level, (layer), (method), (state), MiniFmt::format(msg __VA_OPT__(,) __VA_ARGS__))

// 暴露给业务层使用的便捷宏
#if defined(NDEBUG)
#define LOG_TRACE(...) ((void)0)
#define LOG_DEBUG(...) ((void)0)
#define LOG_INFO(...)  ((void)0)
#else
#define LOG_TRACE(layer, method, state, msg, ...) LOG_INTERNAL(trace, layer, method, state, msg __VA_OPT__(,) __VA_ARGS__)
#define LOG_DEBUG(layer, method, state, msg, ...) LOG_INTERNAL(debug, layer, method, state, msg __VA_OPT__(,) __VA_ARGS__)
#define LOG_INFO(layer,  method, state, msg, ...) LOG_INTERNAL(info,  layer, method, state, msg __VA_OPT__(,) __VA_ARGS__)
#endif

#define LOG_WARN(layer,  method, state, msg, ...) LOG_INTERNAL(warn,  layer, method, state, msg __VA_OPT__(,) __VA_ARGS__)
#define LOG_ERROR(layer, method, state, msg, ...) LOG_INTERNAL(error, layer, method, state, msg __VA_OPT__(,) __VA_ARGS__)
#define LOG_CRIT(layer,  method, state, msg, ...) LOG_INTERNAL(critical, layer, method, state, msg __VA_OPT__(,) __VA_ARGS__)

} // namespace Common
