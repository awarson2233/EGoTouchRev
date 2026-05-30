#pragma once

#include <filesystem>
#include <string>

class TempConfigFile {
public:
    explicit TempConfigFile(std::string name);
    ~TempConfigFile();

    TempConfigFile(const TempConfigFile&) = delete;
    TempConfigFile& operator=(const TempConfigFile&) = delete;

    const std::string& Path() const { return m_pathString; }
    void Write(std::string content) const;

private:
    std::filesystem::path m_path;
    std::string m_pathString;
};
