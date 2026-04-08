#include "Logger.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>

int main() {
    // We want to test the error path in Logger::Init
    // Specifically, if we provide a path that cannot be created,
    // it should output to std::cerr and not throw an unhandled exception or crash.

    // First, let's create a file so that create_directories on that path fails
    std::filesystem::path invalid_dir = "test_invalid_dir_file.txt";

    std::ofstream out(invalid_dir);
    out << "This is a file, not a dir";
    out.close();

    // We need to capture std::cerr to verify the error message
    std::stringstream buffer;
    std::streambuf* old_cerr = std::cerr.rdbuf(buffer.rdbuf());

    // This should fail to create the directory because a file already exists with that name.
    Common::Logger::Init("TestLogger", invalid_dir);

    // Restore std::cerr
    std::cerr.rdbuf(old_cerr);

    std::string err_output = buffer.str();

    // Check if the expected error message is in the captured output
    if (err_output.find("Failed to create log directory") == std::string::npos) {
        std::cerr << "[TEST] Failed: Expected error message not found in std::cerr. Output was: " << err_output << std::endl;
        std::filesystem::remove(invalid_dir);
        return 1;
    }

    // Verify that the logger was NOT initialized.
    if (Common::Logger::Get() != nullptr) {
        std::cerr << "[TEST] Failed: Logger should not be initialized on error." << std::endl;
        std::filesystem::remove(invalid_dir);
        Common::Logger::Shutdown();
        return 2;
    }

    // Cleanup
    std::filesystem::remove(invalid_dir);

    std::cout << "[TEST] Logger init error path test passed.\n";
    return 0;
}
