#include "Logger.h"
#include "GuiLogSink.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void ShutdownLogger() {
    Common::Logger::Shutdown();
}

std::filesystem::path TestPath(const std::string& name) {
    return std::filesystem::current_path() / name;
}

bool ContainsLog(std::string_view text) {
    auto lines = Common::GuiLogSink::Instance()->GetLines();
    return std::any_of(lines.begin(), lines.end(), [text](const std::string& line) {
        return line.find(text) != std::string::npos;
    });
}

bool WaitForContains(std::string_view text) {
    for (int attempt = 0; attempt < 50; ++attempt) {
        if (ContainsLog(text)) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return ContainsLog(text);
}

void TestInvalidDirectoryDoesNotInitialize() {
    ShutdownLogger();
    const auto invalidDir = TestPath("CommonLoggerTest_invalid_dir_marker");
    std::filesystem::remove_all(invalidDir);

    {
        std::ofstream out(invalidDir);
        out << "This is a file, not a directory";
    }

    Common::GuiLogSink::Instance()->Clear();
    Common::Logger::Init("CommonLoggerTestInvalid", invalidDir);

    // Logging should be ignored safely because initialization failed
    LOG_WARN("Common", "LoggerTest", "InvalidDir", "invalid-dir marker");

    std::filesystem::remove(invalidDir);

    Require(!ContainsLog("invalid-dir marker"),
            "Logger should not initialize or log when directory is invalid");
}

void TestInitializeRepeatAndShutdown() {
    ShutdownLogger();
    const auto logDir = TestPath("CommonLoggerTest_repeat_logs");
    std::filesystem::remove_all(logDir);

    Common::GuiLogSink::Instance()->Clear();

    Common::Logger::Init("CommonLoggerTestRepeatA", logDir);
    Common::Logger::Init("CommonLoggerTestRepeatB", logDir); // Should be ignored safely

    LOG_WARN("Common", "LoggerTest", "RepeatInit", "repeat-init marker {}", 17);
    Require(WaitForContains("repeat-init marker 17"),
            "GuiLogSink should receive macro output after init");

    ShutdownLogger();
    std::filesystem::remove_all(logDir);
}

void TestExtraSinkReceivesFormattedMacroOutput() {
    ShutdownLogger();
    const auto logDir = TestPath("CommonLoggerTest_extra_sink_logs");
    std::filesystem::remove_all(logDir);

    Common::GuiLogSink::Instance()->Clear();
    Common::Logger::Init("CommonLoggerTestExtraSink", logDir);

    LOG_WARN("LayerA", "MethodB", "StateC", "value {}", 42);
    Require(WaitForContains("[LayerA] [MethodB] [StateC] value 42"),
            "LOG_WARN should emit formatted layer/method/state message to GuiLogSink");

    ShutdownLogger();
    std::filesystem::remove_all(logDir);
}

} // namespace

int main() {
    try {
        TestInvalidDirectoryDoesNotInitialize();
        TestInitializeRepeatAndShutdown();
        TestExtraSinkReceivesFormattedMacroOutput();
        ShutdownLogger();
        std::cout << "[TEST] CommonLoggerTest passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] CommonLoggerTest failed: " << ex.what() << '\n';
        ShutdownLogger();
        return 1;
    }
}
