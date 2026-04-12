#include "TouchSolver/TouchPipeline.h"
#include "StylusPipeline.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

constexpr int kRows = 40;
constexpr int kCols = 60;
constexpr int kDefaultBenchmarkFrames = 5000;
constexpr int kDefaultStylusPressure = 512;
constexpr double kTouchSampleRateHz = 120.0;

enum class BenchmarkMode {
    Linked,
    Independent,
    Both,
};

struct Options {
    std::filesystem::path dataRoot = std::filesystem::path("Tools/data");
    std::filesystem::path configPath;
    int frames = kDefaultBenchmarkFrames;
    int stylusPressure = kDefaultStylusPressure;
    BenchmarkMode mode = BenchmarkMode::Linked;
};

struct MasterDataset {
    std::vector<Solvers::HeatmapFrame> frames;
    std::filesystem::path rootDir;
};

struct SlaveDataset {
    std::vector<std::vector<uint8_t>> rawFrames;
    std::filesystem::path csvPath;
};

struct RunStats {
    std::string modeName;
    std::string runStart;
    std::string runEnd;
    int benchmarkFrames = 0;
    size_t sourceMasterFrames = 0;
    size_t sourceSlaveFrames = 0;
    int stylusPressure = 0;
    int masterProcessedFrames = 0;
    int slaveProcessedFrames = 0;
    int masterFailedFrames = 0;
    int slaveFailedFrames = 0;
    double wallTotalMs = 0.0;
    double masterTotalMs = 0.0;
    double slaveTotalMs = 0.0;
};

struct PriorityStatus {
    bool supported = false;
    bool processRealtime = false;
    bool threadTimeCritical = false;
    uint32_t processError = 0;
    uint32_t threadError = 0;
};

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

bool TryParseInt(const std::string& text, int& valueOut) {
    const std::string trimmed = Trim(text);
    if (trimmed.empty()) {
        return false;
    }

    size_t consumed = 0;
    try {
        const int value = std::stoi(trimmed, &consumed, 10);
        if (consumed != trimmed.size()) {
            return false;
        }
        valueOut = value;
        return true;
    } catch (...) {
        return false;
    }
}

std::vector<std::string> SplitCsvLine(const std::string& line) {
    std::vector<std::string> parts;
    std::stringstream ss(line);
    std::string part;
    while (std::getline(ss, part, ',')) {
        parts.push_back(Trim(part));
    }

    if (!line.empty() && line.back() == ',') {
        parts.emplace_back();
    }
    return parts;
}

bool IsLegacyTouchConfigSection(const std::string& section) {
    return section == "Master Frame Parser" ||
           section == "Baseline Subtraction" ||
           section == "CMF Processor" ||
           section == "Grid IIR Processor" ||
           section == "Feature Extractor (4.1/4.2)" ||
           section == "Touch Tracker (IDT)" ||
           section == "Coordinate Filter (1 Euro)" ||
           section == "TouchGestureStateMachine";
}

std::optional<std::string> MapLegacyTouchConfigKey(const std::string& section,
                                                   const std::string& key) {
    if (section == "Master Frame Parser") {
        if (key == "Enabled") return std::string("FrameParserEnabled");
        return std::nullopt;
    }
    if (section == "Baseline Subtraction") {
        if (key == "Enabled") return std::string("BaselineEnabled");
        return key;
    }
    if (section == "CMF Processor") {
        if (key == "Enabled") return std::string("CMFEnabled");
        if (key == "DimensionMode") return std::string("CMFDimensionMode");
        if (key == "ExclusionThreshold") return std::string("CMFExclusionThreshold");
        if (key == "MaxCorrection") return std::string("CMFMaxCorrection");
        return key;
    }
    if (section == "Grid IIR Processor") {
        if (key == "Enabled") return std::string("GridIIREnabled");
        return key;
    }
    if (section == "Feature Extractor (4.1/4.2)") {
        if (key == "Enabled") return std::nullopt;
        return key;
    }
    if (section == "Touch Tracker (IDT)") {
        if (key == "Enabled") return std::string("TrackerEnabled");
        return key;
    }
    if (section == "Coordinate Filter (1 Euro)") {
        if (key == "Enabled") return std::string("CoordFilterEnabled");
        return key;
    }
    if (section == "TouchGestureStateMachine") {
        if (key == "Enabled") return std::string("GestureEnabled");
        return key;
    }
    return std::nullopt;
}

bool ParseCsvRow60(const std::string& line, std::array<int16_t, kCols>& rowOut) {
    const auto cells = SplitCsvLine(line);
    if (static_cast<int>(cells.size()) != kCols) {
        return false;
    }

    for (int i = 0; i < kCols; ++i) {
        int value = 0;
        if (!TryParseInt(cells[static_cast<size_t>(i)], value)) {
            return false;
        }
        value = std::clamp(value, -32768, 32767);
        rowOut[static_cast<size_t>(i)] = static_cast<int16_t>(value);
    }
    return true;
}

bool LoadCsvHeatmapFrame(const std::filesystem::path& path, Solvers::HeatmapFrame& frameOut) {
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
            frameOut.heatmapMatrix[r][c] = rows[static_cast<size_t>(r)][static_cast<size_t>(c)];
        }
    }
    return true;
}

std::optional<size_t> ParseFrameIndex(const std::string& text) {
    int value = 0;
    if (!TryParseInt(text, value) || value < 0) {
        return std::nullopt;
    }
    return static_cast<size_t>(value);
}

std::optional<size_t> ParseFrameIndexFromPath(const std::filesystem::path& path) {
    const std::string stem = path.stem().string();
    constexpr std::string_view prefix = "frame_";
    if (!stem.starts_with(prefix)) {
        return std::nullopt;
    }
    return ParseFrameIndex(stem.substr(prefix.size()));
}

std::vector<size_t> ParseIndexedCsvFirstColumn(const std::filesystem::path& path,
                                               const std::string& datasetName) {
    std::ifstream in(path);
    if (!in.is_open()) {
        throw std::runtime_error("Failed to open CSV: " + path.string());
    }

    std::vector<size_t> indices;
    std::string line;
    bool sawHeader = false;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        const std::string trimmed = Trim(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }

        if (!sawHeader) {
            sawHeader = true;
            continue;
        }

        const auto cells = SplitCsvLine(trimmed);
        if (cells.empty()) {
            continue;
        }

        const auto index = ParseFrameIndex(cells.front());
        if (!index.has_value()) {
            throw std::runtime_error("Invalid frame index in " + datasetName + ": " + path.string());
        }
        indices.push_back(*index);
    }

    if (!sawHeader) {
        throw std::runtime_error("Missing CSV header in " + datasetName + ": " + path.string());
    }

    for (size_t i = 0; i < indices.size(); ++i) {
        if (indices[i] != i) {
            throw std::runtime_error("Frame column is not contiguous 0..N-1 in " + datasetName +
                                     ": " + path.string());
        }
    }

    return indices;
}

void LoadConfigFromFile(Solvers::TouchPipeline& touchPipeline,
                        Solvers::StylusPipeline& stylusPipeline,
                        const std::filesystem::path& configPath) {
    std::ifstream in(configPath);
    if (!in.is_open()) {
        throw std::runtime_error("Failed to open config: " + configPath.string());
    }

    std::string line;
    std::string section;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        const std::string trimmed = Trim(line);
        if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#') {
            continue;
        }

        if (trimmed.front() == '[' && trimmed.back() == ']') {
            section = trimmed.substr(1, trimmed.size() - 2);
            continue;
        }

        const size_t pos = trimmed.find('=');
        if (pos == std::string::npos) {
            continue;
        }

        const std::string key = Trim(trimmed.substr(0, pos));
        const std::string value = Trim(trimmed.substr(pos + 1));
        if (section == "TouchPipeline") {
            touchPipeline.LoadConfig(key, value);
        } else if (section == "StylusPipeline") {
            stylusPipeline.LoadConfig(key, value);
        } else if (IsLegacyTouchConfigSection(section)) {
            const auto mappedKey = MapLegacyTouchConfigKey(section, key);
            if (mappedKey.has_value()) {
                touchPipeline.LoadConfig(*mappedKey, value);
            }
        }
    }
}

std::filesystem::path ResolveConfigPath(const std::filesystem::path& explicitPath) {
    if (!explicitPath.empty()) {
        return explicitPath;
    }

    const std::filesystem::path cwd = std::filesystem::current_path();
    const std::array<std::filesystem::path, 7> candidates = {
        cwd / "config.ini",
        cwd / "build" / "config.ini",
        cwd / "build" / "config" / "config.ini",
        cwd / ".." / "config.ini",
        cwd / ".." / ".." / "config.ini",
        std::filesystem::path("config.ini"),
        std::filesystem::path("C:/ProgramData/EGoTouchRev/config.ini")
    };

    for (const auto& candidate : candidates) {
        std::error_code ec;
        if (std::filesystem::exists(candidate, ec) && !ec) {
            return candidate;
        }
    }

    return {};
}

std::filesystem::path ResolveDataRootPath(const std::filesystem::path& configuredPath,
                                          const std::filesystem::path& argv0Path) {
    auto isValidDataRoot = [](const std::filesystem::path& candidate) {
        std::error_code ec;
        return std::filesystem::exists(candidate / "master", ec) &&
               std::filesystem::is_directory(candidate / "master", ec) &&
               std::filesystem::exists(candidate / "slave", ec) &&
               std::filesystem::is_directory(candidate / "slave", ec);
    };

    if (configuredPath.is_absolute()) {
        return configuredPath;
    }

    const std::filesystem::path cwd = std::filesystem::current_path();
    const std::filesystem::path exeDir = std::filesystem::absolute(argv0Path).parent_path();
    const std::array<std::filesystem::path, 8> candidates = {
        cwd / configuredPath,
        cwd / ".." / configuredPath,
        cwd / ".." / ".." / configuredPath,
        exeDir / configuredPath,
        exeDir / ".." / configuredPath,
        exeDir / ".." / ".." / configuredPath,
        std::filesystem::path("Tools/data"),
        configuredPath
    };

    for (const auto& candidate : candidates) {
        if (isValidDataRoot(candidate)) {
            return candidate;
        }
    }

    return configuredPath;
}

BenchmarkMode ParseBenchmarkMode(const std::string& text) {
    const std::string normalized = ToLower(Trim(text));
    if (normalized == "linked") {
        return BenchmarkMode::Linked;
    }
    if (normalized == "independent") {
        return BenchmarkMode::Independent;
    }
    if (normalized == "both") {
        return BenchmarkMode::Both;
    }

    throw std::runtime_error("Unsupported mode: " + text);
}

std::string BenchmarkModeName(BenchmarkMode mode) {
    switch (mode) {
    case BenchmarkMode::Linked:
        return "linked";
    case BenchmarkMode::Independent:
        return "independent";
    case BenchmarkMode::Both:
        return "both";
    }
    return "unknown";
}

void PrintUsage() {
    std::cout
        << "Usage: SolversRawdataBenchmarkTest [options]\n"
        << "  --frames <N>             Total replay steps (default 5000)\n"
        << "  --mode <linked|independent|both>\n"
        << "                           Benchmark mode (default linked)\n"
        << "  --data-root <path>       Root containing master/ and slave/ (default Tools/data)\n"
        << "  --config <path>          Explicit config.ini path\n"
        << "  --stylus-pressure <N>    Fixed stylus pressure (default 512)\n"
        << "  --help                   Show this help\n";
}

Options ParseOptions(int argc, char** argv) {
    Options options;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto requireValue = [&](const char* name) -> std::string {
            if (i + 1 >= argc) {
                throw std::runtime_error(std::string("Missing value for ") + name);
            }
            return argv[++i];
        };

        if (arg == "--help" || arg == "-h") {
            PrintUsage();
            std::exit(0);
        } else if (arg == "--frames") {
            int value = 0;
            if (!TryParseInt(requireValue("--frames"), value) || value <= 0) {
                throw std::runtime_error("--frames must be a positive integer");
            }
            options.frames = value;
        } else if (arg == "--mode") {
            options.mode = ParseBenchmarkMode(requireValue("--mode"));
        } else if (arg == "--data-root") {
            options.dataRoot = requireValue("--data-root");
        } else if (arg == "--config") {
            options.configPath = requireValue("--config");
        } else if (arg == "--stylus-pressure") {
            int value = 0;
            if (!TryParseInt(requireValue("--stylus-pressure"), value) || value < 0) {
                throw std::runtime_error("--stylus-pressure must be a non-negative integer");
            }
            options.stylusPressure = value;
        } else {
            throw std::runtime_error("Unknown argument: " + arg);
        }
    }

    return options;
}

MasterDataset LoadMasterDataset(const std::filesystem::path& rootDir) {
    if (!std::filesystem::exists(rootDir) || !std::filesystem::is_directory(rootDir)) {
        throw std::runtime_error("Master input directory not found: " + rootDir.string());
    }

    const auto statusIndices = ParseIndexedCsvFirstColumn(rootDir / "master_status.csv", "master_status");

    std::vector<std::pair<size_t, Solvers::HeatmapFrame>> indexedFrames;
    for (const auto& entry : std::filesystem::directory_iterator(rootDir)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        const auto index = ParseFrameIndexFromPath(entry.path());
        if (!index.has_value()) {
            continue;
        }

        Solvers::HeatmapFrame frame;
        if (!LoadCsvHeatmapFrame(entry.path(), frame)) {
            throw std::runtime_error("Failed to load master heatmap frame: " + entry.path().string());
        }
        indexedFrames.emplace_back(*index, std::move(frame));
    }

    if (indexedFrames.empty()) {
        throw std::runtime_error("No master heatmap frames found under: " + rootDir.string());
    }

    std::sort(indexedFrames.begin(), indexedFrames.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });

    for (size_t i = 0; i < indexedFrames.size(); ++i) {
        if (indexedFrames[i].first != i) {
            throw std::runtime_error("Master frame files are not contiguous frame_0..frame_N under: " +
                                     rootDir.string());
        }
    }

    if (indexedFrames.size() != statusIndices.size()) {
        throw std::runtime_error("Master heatmap frame count does not match master_status.csv under: " +
                                 rootDir.string());
    }

    MasterDataset dataset;
    dataset.rootDir = rootDir;
    dataset.frames.reserve(indexedFrames.size());
    for (auto& entry : indexedFrames) {
        dataset.frames.push_back(std::move(entry.second));
    }
    return dataset;
}

SlaveDataset LoadSlaveDataset(const std::filesystem::path& rootDir) {
    if (!std::filesystem::exists(rootDir) || !std::filesystem::is_directory(rootDir)) {
        throw std::runtime_error("Slave input directory not found: " + rootDir.string());
    }

    const std::filesystem::path csvPath = rootDir / "slave_suffix.csv";
    std::ifstream in(csvPath);
    if (!in.is_open()) {
        throw std::runtime_error("Failed to open slave CSV: " + csvPath.string());
    }

    SlaveDataset dataset;
    dataset.csvPath = csvPath;

    std::string line;
    bool sawHeader = false;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        const std::string trimmed = Trim(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }

        if (!sawHeader) {
            const auto headerCells = SplitCsvLine(trimmed);
            if (headerCells.size() < static_cast<size_t>(2 + Frame::kSlaveSuffixWords) ||
                headerCells.front() != "Frame") {
                throw std::runtime_error("Unexpected slave CSV header: " + csvPath.string());
            }
            sawHeader = true;
            continue;
        }

        const auto cells = SplitCsvLine(trimmed);
        if (cells.size() < static_cast<size_t>(2 + Frame::kSlaveSuffixWords)) {
            throw std::runtime_error("Slave CSV row has insufficient columns: " + csvPath.string());
        }

        const auto frameIndex = ParseFrameIndex(cells[0]);
        if (!frameIndex.has_value() || *frameIndex != dataset.rawFrames.size()) {
            throw std::runtime_error("Slave CSV Frame column is not contiguous 0..N-1: " + csvPath.string());
        }

        std::vector<uint8_t> rawFrame(static_cast<size_t>(Frame::kSlaveFrameSize), 0);
        for (int word = 0; word < Frame::kSlaveSuffixWords; ++word) {
            int value = 0;
            if (!TryParseInt(cells[static_cast<size_t>(word + 2)], value)) {
                throw std::runtime_error("Invalid slave suffix value in " + csvPath.string());
            }

            const uint16_t wordValue = static_cast<uint16_t>(std::clamp(value, 0, 0xFFFF));
            const size_t byteOffset = static_cast<size_t>(Frame::kHeaderBytes + word * 2);
            rawFrame[byteOffset] = static_cast<uint8_t>(wordValue & 0xFFu);
            rawFrame[byteOffset + 1] = static_cast<uint8_t>((wordValue >> 8) & 0xFFu);
        }

        dataset.rawFrames.push_back(std::move(rawFrame));
    }

    if (!sawHeader) {
        throw std::runtime_error("Missing slave CSV header: " + csvPath.string());
    }
    if (dataset.rawFrames.empty()) {
        throw std::runtime_error("No slave frames found in: " + csvPath.string());
    }

    return dataset;
}

std::vector<size_t> BuildReplaySequence(size_t frameCount) {
    if (frameCount == 0) {
        throw std::runtime_error("Cannot build replay sequence for zero frames");
    }

    std::vector<size_t> sequence;
    sequence.reserve(frameCount * 2);
    for (size_t i = 0; i < frameCount; ++i) {
        sequence.push_back(i);
    }
    for (size_t i = frameCount; i > 0; --i) {
        sequence.push_back(i - 1);
    }
    return sequence;
}

uint64_t SyntheticTimestampMs(int frameStep) {
    return static_cast<uint64_t>(std::llround(static_cast<double>(frameStep) * 1000.0 / kTouchSampleRateHz));
}

Solvers::HeatmapFrame PrepareTouchFrame(const Solvers::HeatmapFrame& sourceFrame, int frameStep) {
    Solvers::HeatmapFrame frame = sourceFrame;
    frame.timestamp = SyntheticTimestampMs(frameStep);
    frame.contacts.clear();
    frame.touchPackets = {};
    frame.stylus = Solvers::StylusFrameData{};
    frame.masterWasRead = true;
    return frame;
}

std::string FormatWallClock(std::chrono::system_clock::time_point timePoint) {
    const std::time_t timeValue = std::chrono::system_clock::to_time_t(timePoint);
    std::tm localTime{};
    localtime_s(&localTime, &timeValue);

    std::ostringstream out;
    out << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return out.str();
}

PriorityStatus ElevateBenchmarkPriority() {
    PriorityStatus status;
#if defined(_WIN32)
    status.supported = true;
    if (SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS)) {
        status.processRealtime = true;
    } else {
        status.processError = static_cast<uint32_t>(GetLastError());
    }

    if (SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL)) {
        status.threadTimeCritical = true;
    } else {
        status.threadError = static_cast<uint32_t>(GetLastError());
    }
#endif
    return status;
}

void PrintPriorityStatus(const PriorityStatus& status) {
    if (!status.supported) {
        std::cout << "[RawdataBenchmarkTest] priority_control=unsupported\n";
        return;
    }

    std::cout << "[RawdataBenchmarkTest] process_priority="
              << (status.processRealtime ? "realtime" : "unchanged") << "\n";
    if (!status.processRealtime) {
        std::cout << "[RawdataBenchmarkTest] process_priority_error="
                  << status.processError << "\n";
    }

    std::cout << "[RawdataBenchmarkTest] thread_priority="
              << (status.threadTimeCritical ? "time_critical" : "unchanged") << "\n";
    if (!status.threadTimeCritical) {
        std::cout << "[RawdataBenchmarkTest] thread_priority_error="
                  << status.threadError << "\n";
    }
}

RunStats RunBenchmark(BenchmarkMode mode,
                      const Options& options,
                      const std::filesystem::path& configPath,
                      const MasterDataset& masterDataset,
                      const SlaveDataset& slaveDataset) {
    if (mode == BenchmarkMode::Both) {
        throw std::runtime_error("RunBenchmark does not accept mode=both");
    }

    if (mode == BenchmarkMode::Linked &&
        masterDataset.frames.size() != slaveDataset.rawFrames.size()) {
        throw std::runtime_error("linked mode requires master/slave frame counts to match");
    }

    Solvers::TouchPipeline touchPipeline;
    Solvers::StylusPipeline stylusPipeline;
    LoadConfigFromFile(touchPipeline, stylusPipeline, configPath);

    const std::vector<size_t> masterSequence = BuildReplaySequence(masterDataset.frames.size());
    const std::vector<size_t> slaveSequence = BuildReplaySequence(slaveDataset.rawFrames.size());

    RunStats stats;
    stats.modeName = BenchmarkModeName(mode);
    stats.benchmarkFrames = options.frames;
    stats.sourceMasterFrames = masterDataset.frames.size();
    stats.sourceSlaveFrames = slaveDataset.rawFrames.size();
    stats.stylusPressure = options.stylusPressure;

    const auto wallStart = std::chrono::steady_clock::now();
    const auto runStart = std::chrono::system_clock::now();
    stats.runStart = FormatWallClock(runStart);

    for (int step = 0; step < options.frames; ++step) {
        if (mode == BenchmarkMode::Linked) {
            const size_t replayIndex = masterSequence[static_cast<size_t>(step) % masterSequence.size()];
            const auto& slaveRaw = slaveDataset.rawFrames[replayIndex];

            // Build a synthetic combined frame: kMasterBytes zeros + slave raw data
            static constexpr size_t kMasterBytes = 5063;
            std::vector<uint8_t> combined(kMasterBytes + slaveRaw.size(), 0);
            std::memcpy(combined.data() + kMasterBytes, slaveRaw.data(), slaveRaw.size());

            Solvers::HeatmapFrame stylusFrame;
            stylusFrame.rawPtr = combined.data();
            stylusFrame.rawLen = combined.size();

            const auto slaveBegin = std::chrono::steady_clock::now();
            stylusPipeline.SetBtMcuPressure(static_cast<uint16_t>(options.stylusPressure));
            const bool slaveOk = stylusPipeline.Process(stylusFrame);
            const auto slaveEnd = std::chrono::steady_clock::now();

            stats.slaveProcessedFrames++;
            if (!slaveOk) {
                stats.slaveFailedFrames++;
            }
            stats.slaveTotalMs +=
                std::chrono::duration<double, std::milli>(slaveEnd - slaveBegin).count();

            Solvers::HeatmapFrame touchFrame = PrepareTouchFrame(masterDataset.frames[replayIndex], step);
            touchFrame.stylus = stylusFrame.stylus;

            const auto masterBegin = std::chrono::steady_clock::now();
            const bool masterOk = touchPipeline.Process(touchFrame);
            const auto masterEnd = std::chrono::steady_clock::now();

            stats.masterProcessedFrames++;
            if (!masterOk) {
                stats.masterFailedFrames++;
            }
            stats.masterTotalMs +=
                std::chrono::duration<double, std::milli>(masterEnd - masterBegin).count();
        } else {
            const size_t masterIndex = masterSequence[static_cast<size_t>(step) % masterSequence.size()];
            const size_t slaveIndex = slaveSequence[static_cast<size_t>(step) % slaveSequence.size()];

            const auto& slaveRaw = slaveDataset.rawFrames[slaveIndex];
            static constexpr size_t kMasterBytes = 5063;
            std::vector<uint8_t> combined(kMasterBytes + slaveRaw.size(), 0);
            std::memcpy(combined.data() + kMasterBytes, slaveRaw.data(), slaveRaw.size());

            Solvers::HeatmapFrame stylusFrame;
            stylusFrame.rawPtr = combined.data();
            stylusFrame.rawLen = combined.size();

            const auto slaveBegin = std::chrono::steady_clock::now();
            stylusPipeline.SetBtMcuPressure(static_cast<uint16_t>(options.stylusPressure));
            const bool slaveOk = stylusPipeline.Process(stylusFrame);
            const auto slaveEnd = std::chrono::steady_clock::now();

            stats.slaveProcessedFrames++;
            if (!slaveOk) {
                stats.slaveFailedFrames++;
            }
            stats.slaveTotalMs +=
                std::chrono::duration<double, std::milli>(slaveEnd - slaveBegin).count();

            Solvers::HeatmapFrame touchFrame = PrepareTouchFrame(masterDataset.frames[masterIndex], step);
            const auto masterBegin = std::chrono::steady_clock::now();
            const bool masterOk = touchPipeline.Process(touchFrame);
            const auto masterEnd = std::chrono::steady_clock::now();

            stats.masterProcessedFrames++;
            if (!masterOk) {
                stats.masterFailedFrames++;
            }
            stats.masterTotalMs +=
                std::chrono::duration<double, std::milli>(masterEnd - masterBegin).count();
        }
    }

    const auto runEnd = std::chrono::system_clock::now();
    const auto wallEnd = std::chrono::steady_clock::now();
    stats.runEnd = FormatWallClock(runEnd);
    stats.wallTotalMs = std::chrono::duration<double, std::milli>(wallEnd - wallStart).count();

    return stats;
}

void PrintStats(const RunStats& stats,
                const Options& options,
                const std::filesystem::path& configPath) {
    const double masterAvgMs =
        stats.masterProcessedFrames > 0
            ? stats.masterTotalMs / static_cast<double>(stats.masterProcessedFrames)
            : 0.0;
    const double slaveAvgMs =
        stats.slaveProcessedFrames > 0
            ? stats.slaveTotalMs / static_cast<double>(stats.slaveProcessedFrames)
            : 0.0;

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "[RawdataBenchmarkTest] mode=" << stats.modeName << "\n";
    std::cout << "[RawdataBenchmarkTest] data_root=" << options.dataRoot.string() << "\n";
    std::cout << "[RawdataBenchmarkTest] config_ini=" << configPath.string() << "\n";
    std::cout << "[RawdataBenchmarkTest] stylus_pressure=" << stats.stylusPressure << "\n";
    std::cout << "[RawdataBenchmarkTest] run_start=" << stats.runStart << "\n";
    std::cout << "[RawdataBenchmarkTest] run_end=" << stats.runEnd << "\n";
    std::cout << "[RawdataBenchmarkTest] benchmark_frames=" << stats.benchmarkFrames << "\n";
    std::cout << "[RawdataBenchmarkTest] source_master_frames=" << stats.sourceMasterFrames << "\n";
    std::cout << "[RawdataBenchmarkTest] source_slave_frames=" << stats.sourceSlaveFrames << "\n";
    std::cout << "[RawdataBenchmarkTest] wall_total_ms=" << stats.wallTotalMs << "\n";
    std::cout << "[RawdataBenchmarkTest] master_total_ms=" << stats.masterTotalMs << "\n";
    std::cout << "[RawdataBenchmarkTest] master_avg_ms_per_frame=" << masterAvgMs << "\n";
    std::cout << "[RawdataBenchmarkTest] master_failed_frames=" << stats.masterFailedFrames << "\n";
    std::cout << "[RawdataBenchmarkTest] slave_total_ms=" << stats.slaveTotalMs << "\n";
    std::cout << "[RawdataBenchmarkTest] slave_avg_ms_per_frame=" << slaveAvgMs << "\n";
    std::cout << "[RawdataBenchmarkTest] slave_failed_frames=" << stats.slaveFailedFrames << "\n";
}

} // namespace

int main(int argc, char** argv) {
    try {
        const PriorityStatus priorityStatus = ElevateBenchmarkPriority();
        PrintPriorityStatus(priorityStatus);

        const Options options = ParseOptions(argc, argv);
        const std::filesystem::path configPath = ResolveConfigPath(options.configPath);
        const std::filesystem::path dataRoot = ResolveDataRootPath(options.dataRoot, argv[0]);
        if (configPath.empty()) {
            std::cerr << "[RawdataBenchmarkTest] config.ini not found.\n";
            PrintUsage();
            return 1;
        }

        Options resolvedOptions = options;
        resolvedOptions.dataRoot = dataRoot;

        const MasterDataset masterDataset = LoadMasterDataset(dataRoot / "master");
        const SlaveDataset slaveDataset = LoadSlaveDataset(dataRoot / "slave");

        if (masterDataset.frames.size() == 480 && slaveDataset.rawFrames.size() == 480) {
            const auto replay480 = BuildReplaySequence(480);
            if (replay480.size() != 960 || replay480[479] != 479 || replay480[480] != 479 ||
                replay480[481] != 478) {
                std::cerr << "[RawdataBenchmarkTest] Internal replay-sequence validation failed.\n";
                return 2;
            }
        }

        if (options.mode == BenchmarkMode::Both) {
            const RunStats linkedStats = RunBenchmark(
                BenchmarkMode::Linked, resolvedOptions, configPath, masterDataset, slaveDataset);
            PrintStats(linkedStats, resolvedOptions, configPath);
            std::cout << "\n";

            const RunStats independentStats = RunBenchmark(
                BenchmarkMode::Independent, resolvedOptions, configPath, masterDataset, slaveDataset);
            PrintStats(independentStats, resolvedOptions, configPath);
        } else {
            const RunStats stats = RunBenchmark(
                options.mode, resolvedOptions, configPath, masterDataset, slaveDataset);
            PrintStats(stats, resolvedOptions, configPath);
        }

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[RawdataBenchmarkTest] " << ex.what() << "\n";
        return 10;
    }
}
