#include "TouchSolver/TouchPipeline.h"
#include "StylusPipeline.h"
#include "FrameLayout.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
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
constexpr int kDefaultBenchmarkFrames = 5000000;
constexpr int kDefaultStylusPressure = 512;
constexpr double kTouchSampleRateHz = 120.0;
constexpr int kDvrMaxContacts = 10;
constexpr int kDvrMaxPeaks = 30;

const std::filesystem::path kDefaultDatasetPath = std::filesystem::path("../Tools/tests/dataset.dvrbin");

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4324)
#endif

enum class BenchmarkMode {
    Linked,
    Independent,
    Both,
};

enum class Dvr2SectionType : uint32_t {
    Meta = 1,
    Index = 2,
    Frames = 3,
    DynamicDebugSchema = 4,
    DynamicDebugValues = 5,
};

struct Options {
    std::filesystem::path datasetPath = kDefaultDatasetPath;
    std::filesystem::path configPath;
    int frames = kDefaultBenchmarkFrames;
    int stylusPressure = kDefaultStylusPressure;
    BenchmarkMode mode = BenchmarkMode::Linked;
};

struct Dvr2FileHeader {
    char magic[8];
    uint16_t formatVersion = 0;
    uint16_t headerSize = sizeof(Dvr2FileHeader);
    uint32_t sectionCount = 0;
    uint64_t tocOffset = 0;
    uint32_t flags = 0;
    uint32_t reserved = 0;
};

struct Dvr2SectionEntry {
    uint32_t type = 0;
    uint32_t version = 0;
    uint64_t offset = 0;
    uint64_t size = 0;
};

struct Dvr2MetaSectionV1 {
    uint32_t frameCount = 0;
    uint32_t frameRecordVersion = 0;
    uint32_t flags = 0;
    uint32_t reserved = 0;
};

struct Dvr2IndexEntryV1 {
    uint64_t timestamp = 0;
    uint64_t receiveSystemEpochUs = 0;
    uint64_t frameOffset = 0;
    uint32_t frameSize = 0;
    uint32_t reserved = 0;
};

struct Dvr2ContactRecordV1 {
    int32_t id = 0;
    float x = 0.0f;
    float y = 0.0f;
    int32_t state = 0;
    int32_t area = 0;
    int32_t signalSum = 0;
};

struct Dvr2PeakRecordV1 {
    int32_t r = 0;
    int32_t c = 0;
    int16_t z = 0;
    uint8_t id = 0;
    uint8_t reserved = 0;
};

struct Dvr2StylusPointRecordV1 {
    uint8_t valid = 0;
    uint8_t reserved0[3]{};
    float x = 0.0f;
    float y = 0.0f;
    uint16_t reportX = 0;
    uint16_t reportY = 0;
    uint16_t pressure = 0;
    uint16_t rawPressure = 0;
    uint16_t mappedPressure = 0;
    uint16_t peakTx1 = 0;
    uint16_t peakTx2 = 0;
    uint16_t reserved1 = 0;
    float tx1X = 0.0f;
    float tx1Y = 0.0f;
    float tx2X = 0.0f;
    float tx2Y = 0.0f;
    float confidence = 0.0f;
};

struct Dvr2StylusDataRecordV1 {
    uint8_t slaveValid = 0;
    uint8_t checksumOk = 0;
    uint8_t tx1BlockValid = 0;
    uint8_t tx2BlockValid = 0;
    uint32_t status = 0;
    uint16_t pressure = 0;
    uint16_t signalX = 0;
    uint16_t signalY = 0;
    uint16_t maxRawPeak = 0;
    uint8_t pipelineStage = 0;
    uint8_t reserved[5]{};
    Dvr2StylusPointRecordV1 point{};
};

struct Dvr2FrameRecordV1 {
    uint64_t timestamp = 0;
    uint64_t receiveSystemEpochUs = 0;
    uint64_t dvrSeq = 0;
    uint8_t masterWasRead = 1;
    uint8_t masterSuffixValid = 0;
    uint8_t slaveSuffixValid = 0;
    uint8_t reserved0 = 0;
    int16_t heatmapMatrix[kRows][kCols]{};
    uint16_t masterSuffix[Frame::kMasterSuffixWords]{};
    uint16_t slaveSuffix[Frame::kSlaveSuffixWords]{};
    Dvr2StylusDataRecordV1 stylus{};
    Dvr2ContactRecordV1 contacts[kDvrMaxContacts]{};
    uint32_t contactCount = 0;
    Dvr2PeakRecordV1 peaks[kDvrMaxPeaks]{};
    uint32_t peakCount = 0;
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

static_assert(sizeof(Dvr2FileHeader) == 32);
static_assert(sizeof(Dvr2SectionEntry) == 24);
static_assert(sizeof(Dvr2IndexEntryV1) == 32);
static_assert(sizeof(Dvr2FrameRecordV1) == 6096);
static_assert(std::is_trivially_copyable_v<Dvr2FrameRecordV1>);
static_assert(std::is_standard_layout_v<Dvr2FrameRecordV1>);

struct DvrDatasetFrame {
    Solvers::HeatmapFrame touchFrame;
    std::vector<uint8_t> rawFrame;
};

struct DvrDataset {
    std::vector<DvrDatasetFrame> frames;
    std::filesystem::path filePath;
    int formatVersion = 0;
    uint32_t flags = 0;
};

struct RunStats {
    std::string modeName;
    std::string runStart;
    std::string runEnd;
    int benchmarkFrames = 0;
    size_t sourceFrames = 0;
    int dvrFormatVersion = 0;
    uint32_t dvrFlags = 0;
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

bool ReadAll(std::ifstream& in, void* data, size_t bytes) {
    in.read(reinterpret_cast<char*>(data), static_cast<std::streamsize>(bytes));
    return static_cast<bool>(in);
}

std::array<char, 8> MakeDvr2Magic() {
    return {'E', 'G', 'O', 'D', 'V', 'R', '2', '\0'};
}

bool RangeWithinFile(uint64_t offset, uint64_t size, uint64_t fileSize) {
    return offset <= fileSize && size <= fileSize - offset;
}

const Dvr2SectionEntry* FindSection(const std::vector<Dvr2SectionEntry>& sections,
                                    Dvr2SectionType type) {
    for (const auto& section : sections) {
        if (section.type == static_cast<uint32_t>(type)) {
            return &section;
        }
    }
    return nullptr;
}

void WriteLe16(std::vector<uint8_t>& raw, size_t offset, uint16_t value) {
    raw[offset] = static_cast<uint8_t>(value & 0xFFu);
    raw[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
}

std::vector<uint8_t> BuildRawFrameBytes(const Solvers::HeatmapFrame& frame) {
    std::vector<uint8_t> raw(static_cast<size_t>(Frame::kTotalFrameSize), 0);

    for (int r = 0; r < kRows; ++r) {
        for (int c = 0; c < kCols; ++c) {
            const size_t offset = static_cast<size_t>(Frame::kMatrixOffset + (r * kCols + c) * 2);
            WriteLe16(raw, offset, static_cast<uint16_t>(frame.heatmapMatrix[r][c]));
        }
    }

    for (int word = 0; word < Frame::kMasterSuffixWords; ++word) {
        const size_t offset = static_cast<size_t>(Frame::kMasterSuffixOffset + word * 2);
        WriteLe16(raw, offset, frame.masterSuffix.words[word]);
    }

    for (int word = 0; word < Frame::kSlaveSuffixWords; ++word) {
        const size_t offset = static_cast<size_t>(Frame::kSlaveSuffixOffset + word * 2);
        WriteLe16(raw, offset, frame.slaveSuffix.words[word]);
    }

    return raw;
}

Solvers::HeatmapFrame PopulateHeatmapFrameFromRecord(const Dvr2FrameRecordV1& record) {
    Solvers::HeatmapFrame frame;
    frame.timestamp = record.timestamp;
    frame.receiveSystemEpochUs = record.receiveSystemEpochUs;
    frame.masterWasRead = record.masterWasRead != 0;
    frame.masterSuffixValid = record.masterSuffixValid != 0;
    frame.slaveSuffixValid = record.slaveSuffixValid != 0;
    std::memcpy(frame.heatmapMatrix, record.heatmapMatrix, sizeof(frame.heatmapMatrix));
    std::memcpy(frame.masterSuffix.words, record.masterSuffix, sizeof(record.masterSuffix));
    std::memcpy(frame.slaveSuffix.words, record.slaveSuffix, sizeof(record.slaveSuffix));
    return frame;
}

DvrDataset LoadDvrDataset(const std::filesystem::path& filePath) {
    std::error_code ec;
    if (!std::filesystem::exists(filePath, ec) || !std::filesystem::is_regular_file(filePath, ec)) {
        throw std::runtime_error("DVR input file not found: " + filePath.string());
    }

    const auto fileSizeValue = std::filesystem::file_size(filePath, ec);
    if (ec || fileSizeValue > std::numeric_limits<uint64_t>::max()) {
        throw std::runtime_error("Failed to inspect DVR file size: " + filePath.string());
    }
    const uint64_t fileSize = static_cast<uint64_t>(fileSizeValue);

    std::ifstream in(filePath, std::ios::binary);
    if (!in.is_open()) {
        throw std::runtime_error("Failed to open DVR file: " + filePath.string());
    }

    Dvr2FileHeader header{};
    if (!ReadAll(in, &header, sizeof(header))) {
        throw std::runtime_error("Failed to read DVR2 header: " + filePath.string());
    }

    const auto dvr2Magic = MakeDvr2Magic();
    if (!std::equal(dvr2Magic.begin(), dvr2Magic.end(), header.magic)) {
        throw std::runtime_error("Invalid DVR2 magic: " + filePath.string());
    }
    if (header.headerSize != sizeof(Dvr2FileHeader)) {
        throw std::runtime_error("Unsupported DVR2 header size: " + filePath.string());
    }
    if (header.sectionCount == 0 || header.sectionCount > 16) {
        throw std::runtime_error("Unsupported DVR2 section count: " + filePath.string());
    }
    const uint64_t tocBytes = static_cast<uint64_t>(header.sectionCount) * sizeof(Dvr2SectionEntry);
    if (!RangeWithinFile(header.tocOffset, tocBytes, fileSize)) {
        throw std::runtime_error("Invalid DVR2 TOC range: " + filePath.string());
    }

    std::vector<Dvr2SectionEntry> sections(header.sectionCount);
    in.seekg(static_cast<std::streamoff>(header.tocOffset), std::ios::beg);
    if (!ReadAll(in, sections.data(), sections.size() * sizeof(Dvr2SectionEntry))) {
        throw std::runtime_error("Failed to read DVR2 TOC: " + filePath.string());
    }

    const auto* metaSection = FindSection(sections, Dvr2SectionType::Meta);
    const auto* indexSection = FindSection(sections, Dvr2SectionType::Index);
    const auto* framesSection = FindSection(sections, Dvr2SectionType::Frames);
    if (!metaSection || !indexSection || !framesSection) {
        throw std::runtime_error("DVR2 file is missing required sections: " + filePath.string());
    }
    if (metaSection->version != 1 || indexSection->version != 1 || framesSection->version != 1) {
        throw std::runtime_error("Unsupported DVR2 section version: " + filePath.string());
    }
    if (metaSection->size != sizeof(Dvr2MetaSectionV1) ||
        !RangeWithinFile(metaSection->offset, metaSection->size, fileSize)) {
        throw std::runtime_error("Invalid DVR2 meta section: " + filePath.string());
    }

    Dvr2MetaSectionV1 meta{};
    in.seekg(static_cast<std::streamoff>(metaSection->offset), std::ios::beg);
    if (!ReadAll(in, &meta, sizeof(meta))) {
        throw std::runtime_error("Failed to read DVR2 meta section: " + filePath.string());
    }
    if (meta.frameRecordVersion != 1) {
        throw std::runtime_error("Unsupported DVR2 frame record version: " + filePath.string());
    }
    if (meta.flags != header.flags) {
        throw std::runtime_error("DVR2 header/meta flags mismatch: " + filePath.string());
    }
    if (meta.frameCount == 0) {
        throw std::runtime_error("DVR2 dataset contains no frames: " + filePath.string());
    }

    const uint64_t expectedIndexSize = static_cast<uint64_t>(meta.frameCount) * sizeof(Dvr2IndexEntryV1);
    const uint64_t expectedFramesSize = static_cast<uint64_t>(meta.frameCount) * sizeof(Dvr2FrameRecordV1);
    if (indexSection->size != expectedIndexSize || !RangeWithinFile(indexSection->offset, indexSection->size, fileSize)) {
        throw std::runtime_error("Invalid DVR2 index section: " + filePath.string());
    }
    if (framesSection->size != expectedFramesSize || !RangeWithinFile(framesSection->offset, framesSection->size, fileSize)) {
        throw std::runtime_error("Invalid DVR2 frame section: " + filePath.string());
    }

    std::vector<Dvr2IndexEntryV1> index(meta.frameCount);
    in.seekg(static_cast<std::streamoff>(indexSection->offset), std::ios::beg);
    if (!ReadAll(in, index.data(), index.size() * sizeof(Dvr2IndexEntryV1))) {
        throw std::runtime_error("Failed to read DVR2 index section: " + filePath.string());
    }

    DvrDataset dataset;
    dataset.filePath = filePath;
    dataset.formatVersion = static_cast<int>(header.formatVersion);
    dataset.flags = header.flags;
    dataset.frames.reserve(meta.frameCount);

    for (uint32_t i = 0; i < meta.frameCount; ++i) {
        const uint64_t expectedFrameOffset = framesSection->offset +
            static_cast<uint64_t>(i) * sizeof(Dvr2FrameRecordV1);
        if (index[i].frameOffset != expectedFrameOffset || index[i].frameSize != sizeof(Dvr2FrameRecordV1)) {
            throw std::runtime_error("Invalid DVR2 frame index entry: " + filePath.string());
        }

        Dvr2FrameRecordV1 record{};
        in.seekg(static_cast<std::streamoff>(index[i].frameOffset), std::ios::beg);
        if (!ReadAll(in, &record, sizeof(record))) {
            throw std::runtime_error("Failed to read DVR2 frame record: " + filePath.string());
        }

        DvrDatasetFrame sample;
        sample.touchFrame = PopulateHeatmapFrameFromRecord(record);
        sample.rawFrame = BuildRawFrameBytes(sample.touchFrame);
        dataset.frames.push_back(std::move(sample));
    }

    return dataset;
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

std::filesystem::path ResolveDatasetPath(const std::filesystem::path& configuredPath,
                                         const std::filesystem::path& argv0Path) {
    if (configuredPath.is_absolute()) {
        return configuredPath;
    }

    auto isValidDataset = [](const std::filesystem::path& candidate) {
        std::error_code ec;
        return std::filesystem::exists(candidate, ec) &&
               std::filesystem::is_regular_file(candidate, ec);
    };

    const std::filesystem::path cwd = std::filesystem::current_path();
    const std::filesystem::path exeDir = std::filesystem::absolute(argv0Path).parent_path();
    const std::array<std::filesystem::path, 8> candidates = {
        cwd / configuredPath,
        cwd / ".." / configuredPath,
        cwd / ".." / ".." / configuredPath,
        exeDir / configuredPath,
        exeDir / ".." / configuredPath,
        exeDir / ".." / ".." / configuredPath,
        kDefaultDatasetPath,
        configuredPath
    };

    for (const auto& candidate : candidates) {
        if (isValidDataset(candidate)) {
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
        << "  --dataset <path>         DVR2 .dvrbin input (default Tools/tests/dataset.dvrbin)\n"
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
        } else if (arg == "--dataset") {
            options.datasetPath = requireValue("--dataset");
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
    frame.rawPtr = nullptr;
    frame.rawLen = 0;
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
                      const DvrDataset& dataset) {
    if (mode == BenchmarkMode::Both) {
        throw std::runtime_error("RunBenchmark does not accept mode=both");
    }

    Solvers::TouchPipeline touchPipeline;
    Solvers::StylusPipeline stylusPipeline;
    LoadConfigFromFile(touchPipeline, stylusPipeline, configPath);

    const std::vector<size_t> sequence = BuildReplaySequence(dataset.frames.size());

    RunStats stats;
    stats.modeName = BenchmarkModeName(mode);
    stats.benchmarkFrames = options.frames;
    stats.sourceFrames = dataset.frames.size();
    stats.dvrFormatVersion = dataset.formatVersion;
    stats.dvrFlags = dataset.flags;
    stats.stylusPressure = options.stylusPressure;

    const auto wallStart = std::chrono::steady_clock::now();
    const auto runStart = std::chrono::system_clock::now();
    stats.runStart = FormatWallClock(runStart);

    for (int step = 0; step < options.frames; ++step) {
        const size_t replayIndex = sequence[static_cast<size_t>(step) % sequence.size()];
        const auto& sample = dataset.frames[replayIndex];

        Solvers::HeatmapFrame stylusFrame;
        stylusFrame.rawPtr = sample.rawFrame.data();
        stylusFrame.rawLen = sample.rawFrame.size();

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

        Solvers::HeatmapFrame touchFrame = PrepareTouchFrame(sample.touchFrame, step);
        if (mode == BenchmarkMode::Linked) {
            touchFrame.stylus = stylusFrame.stylus;
        }

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
    std::cout << "[RawdataBenchmarkTest] dataset=" << options.datasetPath.string() << "\n";
    std::cout << "[RawdataBenchmarkTest] config_ini=" << configPath.string() << "\n";
    std::cout << "[RawdataBenchmarkTest] dvr_format_version=" << stats.dvrFormatVersion << "\n";
    std::cout << "[RawdataBenchmarkTest] dvr_flags=" << stats.dvrFlags << "\n";
    std::cout << "[RawdataBenchmarkTest] stylus_pressure=" << stats.stylusPressure << "\n";
    std::cout << "[RawdataBenchmarkTest] run_start=" << stats.runStart << "\n";
    std::cout << "[RawdataBenchmarkTest] run_end=" << stats.runEnd << "\n";
    std::cout << "[RawdataBenchmarkTest] benchmark_frames=" << stats.benchmarkFrames << "\n";
    std::cout << "[RawdataBenchmarkTest] source_frames=" << stats.sourceFrames << "\n";
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
        const std::filesystem::path datasetPath = ResolveDatasetPath(options.datasetPath, argv[0]);
        if (configPath.empty()) {
            std::cerr << "[RawdataBenchmarkTest] config.ini not found.\n";
            PrintUsage();
            return 1;
        }

        Options resolvedOptions = options;
        resolvedOptions.datasetPath = datasetPath;

        const DvrDataset dataset = LoadDvrDataset(datasetPath);

        if (dataset.frames.size() == 480) {
            const auto replay480 = BuildReplaySequence(480);
            if (replay480.size() != 960 || replay480[479] != 479 || replay480[480] != 479 ||
                replay480[481] != 478) {
                std::cerr << "[RawdataBenchmarkTest] Internal replay-sequence validation failed.\n";
                return 2;
            }
        }

        if (options.mode == BenchmarkMode::Both) {
            const RunStats linkedStats = RunBenchmark(
                BenchmarkMode::Linked, resolvedOptions, configPath, dataset);
            PrintStats(linkedStats, resolvedOptions, configPath);
            std::cout << "\n";

            const RunStats independentStats = RunBenchmark(
                BenchmarkMode::Independent, resolvedOptions, configPath, dataset);
            PrintStats(independentStats, resolvedOptions, configPath);
        } else {
            const RunStats stats = RunBenchmark(
                options.mode, resolvedOptions, configPath, dataset);
            PrintStats(stats, resolvedOptions, configPath);
        }

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[RawdataBenchmarkTest] " << ex.what() << "\n";
        return 10;
    }
}
