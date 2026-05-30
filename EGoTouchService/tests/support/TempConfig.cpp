#include "TempConfig.h"

#include <chrono>
#include <fstream>
#include <stdexcept>
#include <string>

TempConfigFile::TempConfigFile(std::string name) {
    const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
    m_path = std::filesystem::temp_directory_path() /
             (std::move(name) + "-" + std::to_string(unique) + ".ini");
    m_pathString = m_path.string();
}

TempConfigFile::~TempConfigFile() {
    std::error_code ec;
    std::filesystem::remove(m_path, ec);
}

void TempConfigFile::Write(std::string content) const {
    std::ofstream out(m_path, std::ios::trunc);
    if (!out.is_open()) {
        throw std::runtime_error("failed to create temp config: " + m_pathString);
    }
    out << content;
}
