/**
 * @file Logger.h
 * @brief 通用日志模块 (Powered by spdlog)
 * @description 提供异步、多层级、双端 (Console + Daily File) 输出的日志功能。
 * 强制层次化前缀: [Layer][Class::Method][State] Message
 */
#pragma once

#include <string>
#include <memory>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/async.h>

namespace Common {

class Logger {
public:
    /**
     * @brief 初始化全局 Spdlog 实例
     * @param loggerName 日志记录器的名称
     * @param logDir 日志存放目录
     */
    static void Init(const std::string& loggerName = "EGoTouch", 
                     const std::filesystem::path& logDir = "C:/ProgramData/EGoTouchRev/logs/",
                     spdlog::sink_ptr extraSink = nullptr);

    /**
     * @brief 关闭并清理日志
     */
    static void Shutdown();

    /**
     * @brief 获取底层 spdlog logger 实例
     */
    static std::shared_ptr<spdlog::logger> Get() { return s_logger; }

private:
    static std::shared_ptr<spdlog::logger> s_logger;
};

// ---------------------------------------------------------
// 宏定义：带层次化结构的日志输出
// 格式要求: [Layer][Class::Method][State] Message
// ---------------------------------------------------------

// 底层辅助宏，负责将格式拼接并送入 spdlog
#define LOG_INTERNAL(level, layer, method, state, msg, ...) \
    if (Common::Logger::Get()) { \
        Common::Logger::Get()->level( \
            "[{}] [{}] [{}] {}", \
            (layer), (method), (state), std::format(msg __VA_OPT__(,) __VA_ARGS__) \
        ); \
    }

// 暴露给业务层使用的便捷宏
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
#define LOG_TRACE(layer, method, state, msg, ...) LOG_INTERNAL(trace, layer, method, state, msg __VA_OPT__(,) __VA_ARGS__)
#else
#define LOG_TRACE(layer, method, state, msg, ...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
#define LOG_DEBUG(layer, method, state, msg, ...) LOG_INTERNAL(debug, layer, method, state, msg __VA_OPT__(,) __VA_ARGS__)
#else
#define LOG_DEBUG(layer, method, state, msg, ...) (void)0
#endif

#define LOG_INFO(layer,  method, state, msg, ...) LOG_INTERNAL(info,  layer, method, state, msg __VA_OPT__(,) __VA_ARGS__)
#define LOG_WARN(layer,  method, state, msg, ...) LOG_INTERNAL(warn,  layer, method, state, msg __VA_OPT__(,) __VA_ARGS__)
#define LOG_ERROR(layer, method, state, msg, ...) LOG_INTERNAL(error, layer, method, state, msg __VA_OPT__(,) __VA_ARGS__)
#define LOG_CRIT(layer,  method, state, msg, ...) LOG_INTERNAL(critical, layer, method, state, msg __VA_OPT__(,) __VA_ARGS__)

} // namespace Common
