#include "TouchSolver/TouchPipeline.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr int kRows = 40;
constexpr int kCols = 60;
constexpr int kBenchmarkFrames = 3000;
constexpr size_t kMasterFrameMinBytes = 5063;

std::string Trim(std::string s) {
    auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
    return s;
}

std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return s;
}

bool ParseCsvRow60(const std::string& line, std::array<int16_t, kCols>& rowOut) {
    std::stringstream ss(line);
    std::string cell;
    int index = 0;

    while (std::getline(ss, cell, ',')) {
        const std::string token = Trim(cell);
        if (token.empty() || index >= kCols) {
            return false;
        }

        size_t consumed = 0;
        int value = 0;
        try {
            value = std::stoi(token, &consumed, 10);
        } catch (...) {
            return false;
        }
        if (consumed != token.size()) {
            return false;
        }

        value = std::clamp(value, -32768, 32767);
        rowOut[index++] = static_cast<int16_t>(value);
    }

    return index == kCols;
}

bool LoadCsvHeatmapFrame(const std::filesystem::path& path, Engine::HeatmapFrame& frameOut) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }

    std::vector<std::array<int16_t, kCols>> rows;
    rows.reserve(kRows);

    std::string line;
    while (std::getline(in, line)) {
        std::array<int16_t, kCols> row{};
        if (!ParseCsvRow60(line, row)) {
            continue;
        }
        rows.push_back(row);
        if (static_cast<int>(rows.size()) == kRows) {
            break;
        }
    }

    if (static_cast<int>(rows.size()) != kRows) {
        return false;
    }

    for (int r = 0; r < kRows; ++r) {
        for (int c = 0; c < kCols; ++c) {
            frameOut.heatmapMatrix[r][c] = rows[r][c];
        }
    }
    return true;
}

bool LoadRawBinaryFrame(const std::filesystem::path& path, Engine::HeatmapFrame& frameOut) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        return false;
    }

    in.seekg(0, std::ios::end);
    const std::streamoff fileSize = in.tellg();
    if (fileSize <= 0 || static_cast<size_t>(fileSize) < kMasterFrameMinBytes) {
        return false;
    }
    in.seekg(0, std::ios::beg);

    frameOut.rawData.resize(static_cast<size_t>(fileSize));
    if (!in.read(reinterpret_cast<char*>(frameOut.rawData.data()), fileSize)) {
        return false;
    }
    return true;
}

std::vector<std::filesystem::path> CollectCandidateFiles(const std::filesystem::path& inputDir) {
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(inputDir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        files.push_back(entry.path());
    }

    std::sort(files.begin(), files.end(), [](const auto& a, const auto& b) {
        return a.generic_string() < b.generic_string();
    });
    return files;
}

bool LoadConfigFromFile(Engine::TouchPipeline& pipeline, const std::filesystem::path& configPath) {
    std::ifstream in(configPath);
    if (!in.is_open()) {
        return false;
    }

    std::string line;
    std::string section;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty() || line[0] == ';') {
            continue;
        }

        if (line[0] == '[') {
            const size_t endBracket = line.find(']');
            if (endBracket == std::string::npos) {
                continue;
            }
            section = line.substr(1, endBracket - 1);
            continue;
        }

        const size_t pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }

        const std::string key = line.substr(0, pos);
        const std::string val = line.substr(pos + 1);
        if (section == "TouchPipeline") {
            pipeline.LoadConfig(key, val);
        }
    }

    return true;
}

std::filesystem::path ResolveConfigPath(int argc, char** argv) {
    if (argc >= 3) {
        return std::filesystem::path(argv[2]);
    }

    const std::filesystem::path cwd = std::filesystem::current_path();
    const std::array<std::filesystem::path, 6> candidates = {
        cwd / "config.ini",
        cwd / "build" / "config.ini",
        cwd / "build" / "config" / "config.ini",
        cwd / ".." / "config.ini",
        cwd / ".." / ".." / "config.ini",
        std::filesystem::path("config.ini")
    };

    for (const auto& candidate : candidates) {
        std::error_code ec;
        if (std::filesystem::exists(candidate, ec) && !ec) {
            return candidate;
        }
    }

    return {};
}

} // namespace

int main(int argc, char** argv) {
    const std::filesystem::path inputDir = (argc >= 2) ? std::filesystem::path(argv[1])
                                                        : std::filesystem::path("exports/rawdata");

    if (!std::filesystem::exists(inputDir) || !std::filesystem::is_directory(inputDir)) {
        std::cerr << "[RawdataBenchmarkTest] Input directory not found: " << inputDir.string() << "\n";
        std::cerr << "[RawdataBenchmarkTest] Expected raw frames under exports/rawdata\n";
        return 1;
    }

    const auto candidateFiles = CollectCandidateFiles(inputDir);
    if (candidateFiles.empty()) {
        std::cerr << "[RawdataBenchmarkTest] No files found under: " << inputDir.string() << "\n";
        return 2;
    }

    std::vector<Engine::HeatmapFrame> inputFrames;
    inputFrames.reserve(candidateFiles.size());

    size_t loadedBinary = 0;
    size_t loadedCsv = 0;
    for (const auto& file : candidateFiles) {
        const std::string ext = ToLower(file.extension().string());
        Engine::HeatmapFrame frame;
        bool loaded = false;

        if (ext == ".csv") {
            loaded = LoadCsvHeatmapFrame(file, frame);
            if (loaded) {
                ++loadedCsv;
            }
        } else {
            loaded = LoadRawBinaryFrame(file, frame);
            if (loaded) {
                ++loadedBinary;
            }
        }

        if (loaded) {
            inputFrames.push_back(std::move(frame));
        }
    }

    if (inputFrames.empty()) {
        std::cerr << "[RawdataBenchmarkTest] No valid frame loaded from: " << inputDir.string() << "\n";
        std::cerr << "[RawdataBenchmarkTest] Valid input: *.bin/*.raw/*.dat(>=5063 bytes) or 40x60 csv\n";
        return 3;
    }

    const std::filesystem::path configPath = ResolveConfigPath(argc, argv);
    if (configPath.empty()) {
        std::cerr << "[RawdataBenchmarkTest] config.ini not found.\n";
        std::cerr << "[RawdataBenchmarkTest] Usage: EngineRawdataBenchmarkTest [input_dir] [config_ini_path]\n";
        return 4;
    }

    Engine::TouchPipeline pipeline;
    if (!LoadConfigFromFile(pipeline, configPath)) {
        std::cerr << "[RawdataBenchmarkTest] Failed to open config: " << configPath.string() << "\n";
        return 5;
    }

    int droppedFrames = 0;
    const auto begin = std::chrono::steady_clock::now();
    for (int i = 0; i < kBenchmarkFrames; ++i) {
        Engine::HeatmapFrame frame = inputFrames[static_cast<size_t>(i) % inputFrames.size()];
        if (!pipeline.Process(frame)) {
            ++droppedFrames;
        }
    }
    const auto end = std::chrono::steady_clock::now();

    const double totalMs = std::chrono::duration<double, std::milli>(end - begin).count();
    const double avgMs = totalMs / static_cast<double>(kBenchmarkFrames);

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "[RawdataBenchmarkTest] input_dir=" << inputDir.string() << "\n";
    std::cout << "[RawdataBenchmarkTest] config_ini=" << configPath.string() << "\n";
    std::cout << "[RawdataBenchmarkTest] source_frames=" << inputFrames.size()
              << " (binary=" << loadedBinary << ", csv=" << loadedCsv << ")\n";
    std::cout << "[RawdataBenchmarkTest] benchmark_frames=" << kBenchmarkFrames << "\n";
    std::cout << "[RawdataBenchmarkTest] dropped_frames=" << droppedFrames << "\n";
    std::cout << "[RawdataBenchmarkTest] total_ms=" << totalMs << "\n";
    std::cout << "[RawdataBenchmarkTest] avg_ms_per_frame=" << avgMs << "\n";

    return 0;
}
