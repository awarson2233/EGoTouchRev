#include "TouchSolver/TouchPipeline.h"
#include "StylusPipeline.h"
#include "FrameLayout.h"
#include "DvrFormat.h"

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
constexpr int kDefaultBenchmarkFrames = 50000;
constexpr int kDefaultStylusPressure = 512;
constexpr double kTouchSampleRateHz = 120.0;
constexpr int kDvrMaxContacts = 10;
constexpr int kDvrMaxPeaks = 30;

const std::filesystem::path kDefaultDatasetPath = std::filesystem::path("Common/DVRCore/tests/fixtures/dvrbin/dataset.dvrbin");
const std::filesystem::path kDefaultTouchDatasetPath = kDefaultDatasetPath;
const std::filesystem::path kDefaultStylusDatasetPath = kDefaultDatasetPath;

enum class BenchmarkMode {
    Linked,
    Independent,
    Both,
};

namespace DvrFmt = Dvr::Format;

using Dvr2FileHeader = DvrFmt::Dvr2FileHeader;
using Dvr2SectionEntry = DvrFmt::Dvr2SectionEntry;
using Dvr2SectionType = DvrFmt::Dvr2SectionType;
using Dvr2MetaSection = DvrFmt::Dvr2MetaSection;
using Dvr2FrameSchemaHeader = DvrFmt::Dvr2FrameSchemaHeader;
using Dvr2FieldDef = DvrFmt::Dvr2FieldDef;
using Dvr2IndexEntry = DvrFmt::Dvr2IndexEntry;

struct Options {
    std::filesystem::path touchDatasetPath = kDefaultTouchDatasetPath;
    std::filesystem::path stylusDatasetPath = kDefaultStylusDatasetPath;
    std::filesystem::path configPath;
    int frames = kDefaultBenchmarkFrames;
    int stylusPressure = kDefaultStylusPressure;
    BenchmarkMode mode = BenchmarkMode::Linked;
};

static_assert(sizeof(Dvr2FileHeader) == 32);
static_assert(sizeof(Dvr2SectionEntry) == 24);
static_assert(sizeof(Dvr2MetaSection) == 64);
static_assert(sizeof(Dvr2FrameSchemaHeader) == 32);
static_assert(sizeof(Dvr2FieldDef) == 164);
static_assert(sizeof(Dvr2IndexEntry) == 32);

struct DvrDatasetFrame {
    Solvers::HeatmapFrame touchFrame;
    std::vector<uint8_t> rawFrame;
};

struct DvrDataset {
    std::vector<DvrDatasetFrame> frames;
    std::filesystem::path filePath;
    int formatVersion = 0;
    uint32_t flags = 0;
    int rawStylusSignalFrames = 0;
    int suffixStylusSignalFrames = 0;
    int recordedStylusSlaveValidFrames = 0;
    int recordedStylusTx1ValidFrames = 0;
    int recordedStylusOutputValidFrames = 0;
    int recordedStylusPointValidFrames = 0;
};

struct RunStats {
    std::string modeName;
    std::string runStart;
    std::string runEnd;
    int benchmarkFrames = 0;
    size_t touchSourceFrames = 0;
    size_t stylusSourceFrames = 0;
    int dvrFormatVersion = 0;
    uint32_t dvrFlags = 0;
    int stylusPressure = 0;
    int stylusDatasetRawSignalFrames = 0;
    int stylusDatasetSuffixSignalFrames = 0;
    int stylusDatasetRecordedSlaveValidFrames = 0;
    int stylusDatasetRecordedTx1ValidFrames = 0;
    int stylusDatasetRecordedOutputValidFrames = 0;
    int stylusDatasetRecordedPointValidFrames = 0;
    int masterProcessedFrames = 0;
    int slaveProcessedFrames = 0;
    int masterFailedFrames = 0;
    int slaveFailedFrames = 0;
    int stylusValidFrames = 0;
    int stylusSignalFrames = 0;
    int stylusNoSignalFrames = 0;
    int stylusShortFrames = 0;
    int stylusParseFailFrames = 0;
    int stylusTx1MissingFrames = 0;
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
    return DvrFmt::kDvr2Magic;
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

uint16_t ReadLe16(const uint8_t* data) {
    return static_cast<uint16_t>(
        static_cast<uint16_t>(data[0]) |
        (static_cast<uint16_t>(data[1]) << 8));
}

bool HasStylusSignal(uint16_t anchorRow, uint16_t anchorCol) {
    return !(((anchorRow & 0xFFu) == Frame::kAnchorInvalid) &&
             ((anchorCol & 0xFFu) == Frame::kAnchorInvalid));
}

bool HasStylusSignal(const Frame::SlaveSuffixView& suffix) {
    return HasStylusSignal(suffix.tx1AnchorRow(), suffix.tx1AnchorCol());
}

bool RawFrameHasStylusSignal(const std::vector<uint8_t>& raw) {
    if (raw.size() < static_cast<size_t>(Frame::kTotalFrameSize)) {
        return false;
    }

    const size_t slaveOffset = raw.size() - static_cast<size_t>(Frame::kSlaveFrameSize);
    const uint8_t* slaveWords = raw.data() + slaveOffset + Frame::kHeaderBytes;
    return HasStylusSignal(ReadLe16(slaveWords), ReadLe16(slaveWords + sizeof(uint16_t)));
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

const Dvr2FieldDef& RequireFrameField(const std::vector<Dvr2FieldDef>& fields,
                                      std::string_view path) {
    const auto* field = DvrFmt::FindField(fields, path);
    if (!field) {
        throw std::runtime_error("DVR2 frame schema missing required field: " + std::string(path));
    }
    return *field;
}

uint64_t FrameFieldExtent(const Dvr2FieldDef& field) {
    uint64_t extent = static_cast<uint64_t>(field.offset) + field.size;
    if (field.elementCount > 1) {
        extent = std::max(extent,
            static_cast<uint64_t>(field.offset) +
            static_cast<uint64_t>(field.elementCount - 1) * field.stride + field.elementSize);
    }
    return extent;
}

template <typename T>
T ReadFrameFieldScalar(const std::vector<uint8_t>& record,
                       const std::vector<Dvr2FieldDef>& fields,
                       std::string_view path,
                       uint32_t index = 0) {
    const auto& field = RequireFrameField(fields, path);
    const uint64_t offset = static_cast<uint64_t>(field.offset) + static_cast<uint64_t>(index) * field.stride;
    if (offset + sizeof(T) > record.size()) {
        throw std::runtime_error("DVR2 frame field exceeds record: " + std::string(path));
    }
    T value{};
    std::memcpy(&value, record.data() + static_cast<size_t>(offset), sizeof(T));
    return value;
}

template <typename T>
T ReadOptionalFrameFieldScalar(const std::vector<uint8_t>& record,
                               const std::vector<Dvr2FieldDef>& fields,
                               std::string_view path,
                               T defaultValue = {}) {
    const auto* field = DvrFmt::FindField(fields, path);
    if (!field) {
        return defaultValue;
    }
    const uint64_t offset = field->offset;
    if (offset + sizeof(T) > record.size()) {
        throw std::runtime_error("DVR2 optional frame field exceeds record: " + std::string(path));
    }
    T value{};
    std::memcpy(&value, record.data() + static_cast<size_t>(offset), sizeof(T));
    return value;
}

void CopyFrameFieldBytes(const std::vector<uint8_t>& record,
                         const std::vector<Dvr2FieldDef>& fields,
                         std::string_view path,
                         void* destination,
                         size_t destinationBytes) {
    const auto& field = RequireFrameField(fields, path);
    if (field.offset + destinationBytes > record.size() || destinationBytes > field.size) {
        throw std::runtime_error("DVR2 frame field size mismatch: " + std::string(path));
    }
    std::memcpy(destination, record.data() + field.offset, destinationBytes);
}

Solvers::HeatmapFrame PopulateHeatmapFrameFromRecordBytes(const std::vector<uint8_t>& record,
                                                          const std::vector<Dvr2FieldDef>& fields) {
    Solvers::HeatmapFrame frame;
    frame.timestamp = ReadFrameFieldScalar<uint64_t>(record, fields, "timestamp");
    frame.receiveSystemEpochUs = ReadFrameFieldScalar<uint64_t>(record, fields, "receiveSystemEpochUs");
    frame.masterWasRead = ReadFrameFieldScalar<uint8_t>(record, fields, "masterWasRead") != 0;
    frame.masterSuffixValid = ReadFrameFieldScalar<uint8_t>(record, fields, "masterSuffixValid") != 0;
    frame.slaveSuffixValid = ReadFrameFieldScalar<uint8_t>(record, fields, "slaveSuffixValid") != 0;
    CopyFrameFieldBytes(record, fields, "heatmapMatrix", frame.heatmapMatrix, sizeof(frame.heatmapMatrix));
    CopyFrameFieldBytes(record, fields, "masterSuffix.words", frame.masterSuffix.words, sizeof(frame.masterSuffix.words));
    CopyFrameFieldBytes(record, fields, "slaveSuffix.words", frame.slaveSuffix.words, sizeof(frame.slaveSuffix.words));
    return frame;
}

std::vector<uint8_t> RawFrameBytesFromRecordBytes(const std::vector<uint8_t>& record,
                                                  const std::vector<Dvr2FieldDef>& fields,
                                                  const Solvers::HeatmapFrame& frame) {
    const auto& rawField = RequireFrameField(fields, "rawData");
    const size_t rawLen = std::min<size_t>(ReadFrameFieldScalar<uint16_t>(record, fields, "rawDataLength"), Frame::kTotalFrameSize);
    if (rawLen == 0) {
        return BuildRawFrameBytes(frame);
    }
    if (rawField.offset + rawLen > record.size()) {
        throw std::runtime_error("DVR2 rawData field exceeds record");
    }

    std::vector<uint8_t> raw(record.data() + rawField.offset, record.data() + rawField.offset + rawLen);
    if (frame.slaveSuffixValid && HasStylusSignal(frame.slaveSuffix) && !RawFrameHasStylusSignal(raw)) {
        raw = BuildRawFrameBytes(frame);
    }
    return raw;
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
    if (header.headerSize != sizeof(Dvr2FileHeader) || header.tocOffset != sizeof(Dvr2FileHeader)) {
        throw std::runtime_error("Unsupported DVR2 header layout: " + filePath.string());
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
    const auto* frameSchemaSection = FindSection(sections, Dvr2SectionType::FrameSchema);
    const auto* indexSection = FindSection(sections, Dvr2SectionType::Index);
    const auto* framesSection = FindSection(sections, Dvr2SectionType::Frames);
    if (!metaSection || !frameSchemaSection || !indexSection || !framesSection) {
        throw std::runtime_error("DVR2 file is missing required schema sections: " + filePath.string());
    }
    if (metaSection->version != 1 || frameSchemaSection->version != 1 || indexSection->version != 1 || framesSection->version != 1) {
        throw std::runtime_error("Unsupported DVR2 section version: " + filePath.string());
    }
    if (metaSection->size != sizeof(Dvr2MetaSection) ||
        !RangeWithinFile(metaSection->offset, metaSection->size, fileSize)) {
        throw std::runtime_error("Invalid DVR2 meta section: " + filePath.string());
    }

    Dvr2MetaSection meta{};
    in.seekg(static_cast<std::streamoff>(metaSection->offset), std::ios::beg);
    if (!ReadAll(in, &meta, sizeof(meta))) {
        throw std::runtime_error("Failed to read DVR2 meta section: " + filePath.string());
    }
    if (meta.flags != header.flags) {
        throw std::runtime_error("DVR2 header/meta flags mismatch: " + filePath.string());
    }
    if (meta.frameCount == 0) {
        throw std::runtime_error("DVR2 dataset contains no frames: " + filePath.string());
    }
    if (meta.txCount != Frame::kTxCount || meta.rxCount != Frame::kRxCount ||
        meta.masterSuffixWords != Frame::kMasterSuffixWords || meta.slaveSuffixWords != Frame::kSlaveSuffixWords ||
        meta.rawFrameSize != Frame::kTotalFrameSize) {
        throw std::runtime_error("Unsupported DVR2 meta dimensions: " + filePath.string());
    }

    if (frameSchemaSection->size < sizeof(Dvr2FrameSchemaHeader) ||
        !RangeWithinFile(frameSchemaSection->offset, frameSchemaSection->size, fileSize)) {
        throw std::runtime_error("Invalid DVR2 frame schema section: " + filePath.string());
    }
    Dvr2FrameSchemaHeader schemaHeader{};
    in.seekg(static_cast<std::streamoff>(frameSchemaSection->offset), std::ios::beg);
    if (!ReadAll(in, &schemaHeader, sizeof(schemaHeader))) {
        throw std::runtime_error("Failed to read DVR2 frame schema header: " + filePath.string());
    }
    if (schemaHeader.fieldRecordSize != sizeof(Dvr2FieldDef) || schemaHeader.frameRecordSize != meta.frameRecordSize) {
        throw std::runtime_error("Unsupported DVR2 frame schema layout: " + filePath.string());
    }
    const uint64_t expectedSchemaSize = sizeof(Dvr2FrameSchemaHeader) +
        static_cast<uint64_t>(schemaHeader.fieldCount) * sizeof(Dvr2FieldDef);
    if (frameSchemaSection->size != expectedSchemaSize) {
        throw std::runtime_error("DVR2 frame schema size mismatch: " + filePath.string());
    }
    std::vector<Dvr2FieldDef> frameFields(schemaHeader.fieldCount);
    if (!frameFields.empty() && !ReadAll(in, frameFields.data(), frameFields.size() * sizeof(Dvr2FieldDef))) {
        throw std::runtime_error("Failed to read DVR2 frame schema fields: " + filePath.string());
    }
    if (DvrFmt::ComputeFieldSchemaHash(frameFields) != schemaHeader.schemaHash || meta.frameSchemaHash != schemaHeader.schemaHash) {
        throw std::runtime_error("DVR2 frame schema hash mismatch: " + filePath.string());
    }
    for (const auto& path : {"timestamp", "receiveSystemEpochUs", "masterWasRead", "masterSuffixValid", "slaveSuffixValid",
                            "heatmapMatrix", "masterSuffix.words", "slaveSuffix.words", "rawDataLength", "rawData"}) {
        const auto& field = RequireFrameField(frameFields, path);
        if (FrameFieldExtent(field) > meta.frameRecordSize) {
            throw std::runtime_error("DVR2 frame schema field exceeds frame size: " + std::string(path));
        }
    }

    const uint64_t expectedIndexSize = static_cast<uint64_t>(meta.frameCount) * sizeof(Dvr2IndexEntry);
    const uint64_t expectedFramesSize = static_cast<uint64_t>(meta.frameCount) * meta.frameRecordSize;
    if (indexSection->size != expectedIndexSize || !RangeWithinFile(indexSection->offset, indexSection->size, fileSize)) {
        throw std::runtime_error("Invalid DVR2 index section: " + filePath.string());
    }
    if (framesSection->size != expectedFramesSize || !RangeWithinFile(framesSection->offset, framesSection->size, fileSize)) {
        throw std::runtime_error("Invalid DVR2 frame section: " + filePath.string());
    }

    std::vector<Dvr2IndexEntry> index(meta.frameCount);
    in.seekg(static_cast<std::streamoff>(indexSection->offset), std::ios::beg);
    if (!ReadAll(in, index.data(), index.size() * sizeof(Dvr2IndexEntry))) {
        throw std::runtime_error("Failed to read DVR2 index section: " + filePath.string());
    }

    DvrDataset dataset;
    dataset.filePath = filePath;
    dataset.formatVersion = static_cast<int>(header.formatVersion);
    dataset.flags = header.flags;
    dataset.frames.reserve(meta.frameCount);

    for (uint32_t i = 0; i < meta.frameCount; ++i) {
        const uint64_t expectedFrameOffset = framesSection->offset +
            static_cast<uint64_t>(i) * meta.frameRecordSize;
        if (index[i].frameOffset != expectedFrameOffset || index[i].frameSize != meta.frameRecordSize) {
            throw std::runtime_error("Invalid DVR2 frame index entry: " + filePath.string());
        }

        std::vector<uint8_t> record(meta.frameRecordSize);
        in.seekg(static_cast<std::streamoff>(index[i].frameOffset), std::ios::beg);
        if (!ReadAll(in, record.data(), record.size())) {
            throw std::runtime_error("Failed to read DVR2 frame payload: " + filePath.string());
        }

        DvrDatasetFrame sample;
        sample.touchFrame = PopulateHeatmapFrameFromRecordBytes(record, frameFields);
        sample.rawFrame = RawFrameBytesFromRecordBytes(record, frameFields, sample.touchFrame);
        if (RawFrameHasStylusSignal(sample.rawFrame)) {
            dataset.rawStylusSignalFrames++;
        }
        if (sample.touchFrame.slaveSuffixValid && HasStylusSignal(sample.touchFrame.slaveSuffix)) {
            dataset.suffixStylusSignalFrames++;
        }
        if (ReadOptionalFrameFieldScalar<uint8_t>(record, frameFields, "stylus.slaveValid") != 0) {
            dataset.recordedStylusSlaveValidFrames++;
        }
        if (ReadOptionalFrameFieldScalar<uint8_t>(record, frameFields, "stylus.tx1BlockValid") != 0) {
            dataset.recordedStylusTx1ValidFrames++;
        }
        if (ReadOptionalFrameFieldScalar<uint8_t>(record, frameFields, "stylus.output.valid") != 0) {
            dataset.recordedStylusOutputValidFrames++;
        }
        if (ReadOptionalFrameFieldScalar<uint8_t>(record, frameFields, "stylus.point.valid") != 0) {
            dataset.recordedStylusPointValidFrames++;
        }
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
        << "Usage: SolversRawdataBenchmark [options]\n"
        << "  --frames <N>             Total replay steps (default " << kDefaultBenchmarkFrames << ")\n"
        << "  --mode <linked|independent|both>\n"
        << "                           Benchmark mode (default linked)\n"
        << "  --touch-dataset <path>   Touch DVR2 .dvrbin input (default " << kDefaultTouchDatasetPath.string() << ")\n"
        << "  --stylus-dataset <path>  Stylus DVR2 .dvrbin input (default " << kDefaultStylusDatasetPath.string() << ")\n"
        << "  --dataset <path>         Use the same DVR2 .dvrbin input for touch and stylus\n"
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
        } else if (arg == "--touch-dataset") {
            options.touchDatasetPath = requireValue("--touch-dataset");
        } else if (arg == "--stylus-dataset") {
            options.stylusDatasetPath = requireValue("--stylus-dataset");
        } else if (arg == "--dataset") {
            const std::filesystem::path datasetPath = requireValue("--dataset");
            options.touchDatasetPath = datasetPath;
            options.stylusDatasetPath = datasetPath;
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
    frame.touch.output.contacts.clear();
    frame.touch.output.touchPackets = {};
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
        std::cout << "[RawdataBenchmark] priority_control=unsupported\n";
        return;
    }

    std::cout << "[RawdataBenchmark] process_priority="
              << (status.processRealtime ? "realtime" : "unchanged") << "\n";
    if (!status.processRealtime) {
        std::cout << "[RawdataBenchmark] process_priority_error="
                  << status.processError << "\n";
    }

    std::cout << "[RawdataBenchmark] thread_priority="
              << (status.threadTimeCritical ? "time_critical" : "unchanged") << "\n";
    if (!status.threadTimeCritical) {
        std::cout << "[RawdataBenchmark] thread_priority_error="
                  << status.threadError << "\n";
    }
}

RunStats RunBenchmark(BenchmarkMode mode,
                      const Options& options,
                      const std::filesystem::path& configPath,
                      const DvrDataset& touchDataset,
                      const DvrDataset& stylusDataset) {
    if (mode == BenchmarkMode::Both) {
        throw std::runtime_error("RunBenchmark does not accept mode=both");
    }

    Solvers::TouchPipeline touchPipeline;
    Solvers::StylusPipeline stylusPipeline;
    if (!configPath.empty()) {
        LoadConfigFromFile(touchPipeline, stylusPipeline, configPath);
    }

    const std::vector<size_t> touchSequence = BuildReplaySequence(touchDataset.frames.size());
    const std::vector<size_t> stylusSequence = BuildReplaySequence(stylusDataset.frames.size());

    RunStats stats;
    stats.modeName = BenchmarkModeName(mode);
    stats.benchmarkFrames = options.frames;
    stats.touchSourceFrames = touchDataset.frames.size();
    stats.stylusSourceFrames = stylusDataset.frames.size();
    stats.dvrFormatVersion = touchDataset.formatVersion;
    stats.dvrFlags = touchDataset.flags;
    stats.stylusPressure = options.stylusPressure;
    stats.stylusDatasetRawSignalFrames = stylusDataset.rawStylusSignalFrames;
    stats.stylusDatasetSuffixSignalFrames = stylusDataset.suffixStylusSignalFrames;
    stats.stylusDatasetRecordedSlaveValidFrames = stylusDataset.recordedStylusSlaveValidFrames;
    stats.stylusDatasetRecordedTx1ValidFrames = stylusDataset.recordedStylusTx1ValidFrames;
    stats.stylusDatasetRecordedOutputValidFrames = stylusDataset.recordedStylusOutputValidFrames;
    stats.stylusDatasetRecordedPointValidFrames = stylusDataset.recordedStylusPointValidFrames;

    const auto wallStart = std::chrono::steady_clock::now();
    const auto runStart = std::chrono::system_clock::now();
    stats.runStart = FormatWallClock(runStart);

    for (int step = 0; step < options.frames; ++step) {
        const size_t touchReplayIndex = touchSequence[static_cast<size_t>(step) % touchSequence.size()];
        const size_t stylusReplayIndex = stylusSequence[static_cast<size_t>(step) % stylusSequence.size()];
        const auto& touchSample = touchDataset.frames[touchReplayIndex];
        const auto& stylusSample = stylusDataset.frames[stylusReplayIndex];

        Solvers::HeatmapFrame stylusFrame;
        stylusFrame.rawPtr = stylusSample.rawFrame.data();
        stylusFrame.rawLen = stylusSample.rawFrame.size();

        const auto slaveBegin = std::chrono::steady_clock::now();
        stylusPipeline.SetBtMcuPressure(static_cast<uint16_t>(options.stylusPressure));
        const bool slaveOk = stylusPipeline.Process(stylusFrame);
        const auto slaveEnd = std::chrono::steady_clock::now();

        stats.slaveProcessedFrames++;
        if (!slaveOk) {
            stats.slaveFailedFrames++;
        }
        if (stylusFrame.stylus.runtime.Active().parse.valid) {
            stats.stylusValidFrames++;
        }
        if (stylusFrame.stylus.runtime.Active().parse.hasCurrentStylusSignal) {
            stats.stylusSignalFrames++;
        }
        switch (stylusFrame.stylus.runtime.Active().flow.frameClass) {
        case Asa::FrameClass::Valid:
            break;
        case Asa::FrameClass::ShortFrame:
            stats.stylusShortFrames++;
            break;
        case Asa::FrameClass::NoSignal:
            stats.stylusNoSignalFrames++;
            break;
        case Asa::FrameClass::ParseFail:
            stats.stylusParseFailFrames++;
            break;
        case Asa::FrameClass::Tx1Missing:
            stats.stylusTx1MissingFrames++;
            break;
        }
        stats.slaveTotalMs +=
            std::chrono::duration<double, std::milli>(slaveEnd - slaveBegin).count();

        Solvers::HeatmapFrame touchFrame = PrepareTouchFrame(touchSample.touchFrame, step);
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
    std::cout << "[RawdataBenchmark] mode=" << stats.modeName << "\n";
    std::cout << "[RawdataBenchmark] touch_dataset=" << options.touchDatasetPath.string() << "\n";
    std::cout << "[RawdataBenchmark] stylus_dataset=" << options.stylusDatasetPath.string() << "\n";
    std::cout << "[RawdataBenchmark] config_ini=" << configPath.string() << "\n";
    std::cout << "[RawdataBenchmark] dvr_format_version=" << stats.dvrFormatVersion << "\n";
    std::cout << "[RawdataBenchmark] dvr_flags=" << stats.dvrFlags << "\n";
    std::cout << "[RawdataBenchmark] stylus_pressure=" << stats.stylusPressure << "\n";
    std::cout << "[RawdataBenchmark] run_start=" << stats.runStart << "\n";
    std::cout << "[RawdataBenchmark] run_end=" << stats.runEnd << "\n";
    std::cout << "[RawdataBenchmark] benchmark_frames=" << stats.benchmarkFrames << "\n";
    std::cout << "[RawdataBenchmark] touch_source_frames=" << stats.touchSourceFrames << "\n";
    std::cout << "[RawdataBenchmark] stylus_source_frames=" << stats.stylusSourceFrames << "\n";
    std::cout << "[RawdataBenchmark] stylus_dataset_raw_signal_frames=" << stats.stylusDatasetRawSignalFrames << "\n";
    std::cout << "[RawdataBenchmark] stylus_dataset_suffix_signal_frames=" << stats.stylusDatasetSuffixSignalFrames << "\n";
    std::cout << "[RawdataBenchmark] stylus_dataset_recorded_slave_valid_frames=" << stats.stylusDatasetRecordedSlaveValidFrames << "\n";
    std::cout << "[RawdataBenchmark] stylus_dataset_recorded_tx1_valid_frames=" << stats.stylusDatasetRecordedTx1ValidFrames << "\n";
    std::cout << "[RawdataBenchmark] stylus_dataset_recorded_output_valid_frames=" << stats.stylusDatasetRecordedOutputValidFrames << "\n";
    std::cout << "[RawdataBenchmark] stylus_dataset_recorded_point_valid_frames=" << stats.stylusDatasetRecordedPointValidFrames << "\n";
    std::cout << "[RawdataBenchmark] wall_total_ms=" << stats.wallTotalMs << "\n";
    std::cout << "[RawdataBenchmark] master_total_ms=" << stats.masterTotalMs << "\n";
    std::cout << "[RawdataBenchmark] master_avg_ms_per_frame=" << masterAvgMs << "\n";
    std::cout << "[RawdataBenchmark] master_failed_frames=" << stats.masterFailedFrames << "\n";
    std::cout << "[RawdataBenchmark] slave_total_ms=" << stats.slaveTotalMs << "\n";
    std::cout << "[RawdataBenchmark] slave_avg_ms_per_frame=" << slaveAvgMs << "\n";
    std::cout << "[RawdataBenchmark] slave_failed_frames=" << stats.slaveFailedFrames << "\n";
    std::cout << "[RawdataBenchmark] stylus_valid_frames=" << stats.stylusValidFrames << "\n";
    std::cout << "[RawdataBenchmark] stylus_signal_frames=" << stats.stylusSignalFrames << "\n";
    std::cout << "[RawdataBenchmark] stylus_short_frames=" << stats.stylusShortFrames << "\n";
    std::cout << "[RawdataBenchmark] stylus_no_signal_frames=" << stats.stylusNoSignalFrames << "\n";
    std::cout << "[RawdataBenchmark] stylus_parse_fail_frames=" << stats.stylusParseFailFrames << "\n";
    std::cout << "[RawdataBenchmark] stylus_tx1_missing_frames=" << stats.stylusTx1MissingFrames << "\n";
}

bool DatasetNeedsStylusReadback(const DvrDataset& dataset) {
    return dataset.rawStylusSignalFrames > 0 ||
           dataset.suffixStylusSignalFrames > 0 ||
           dataset.recordedStylusSlaveValidFrames > 0 ||
           dataset.recordedStylusTx1ValidFrames > 0 ||
           dataset.recordedStylusOutputValidFrames > 0 ||
           dataset.recordedStylusPointValidFrames > 0;
}

void RequireStylusReadback(const RunStats& stats, const DvrDataset& stylusDataset) {
    if (!DatasetNeedsStylusReadback(stylusDataset)) {
        return;
    }
    if (stats.stylusValidFrames == 0 || stats.stylusSignalFrames == 0) {
        throw std::runtime_error(
            "Stylus dataset produced no valid stylus frames; check stylus rawData/slave payload");
    }
}

void PrintCtestMeasurement(std::string_view name, double value) {
    std::cout << "<DartMeasurement name=\"" << name << "\" type=\"numeric/double\">"
              << value << "</DartMeasurement>\n";
}

void PrintCtestMeasurement(std::string_view name, int value) {
    std::cout << "<DartMeasurement name=\"" << name << "\" type=\"numeric/integer\">"
              << value << "</DartMeasurement>\n";
}

void PrintCtestMeasurements(const RunStats& stats) {
    const std::string prefix = "RawdataBenchmark." + stats.modeName + ".";
    const double masterAvgMs =
        stats.masterProcessedFrames > 0
            ? stats.masterTotalMs / static_cast<double>(stats.masterProcessedFrames)
            : 0.0;
    const double slaveAvgMs =
        stats.slaveProcessedFrames > 0
            ? stats.slaveTotalMs / static_cast<double>(stats.slaveProcessedFrames)
            : 0.0;

    PrintCtestMeasurement(prefix + "wall_total_ms", stats.wallTotalMs);
    PrintCtestMeasurement(prefix + "master_total_ms", stats.masterTotalMs);
    PrintCtestMeasurement(prefix + "master_avg_ms_per_frame", masterAvgMs);
    PrintCtestMeasurement(prefix + "slave_total_ms", stats.slaveTotalMs);
    PrintCtestMeasurement(prefix + "slave_avg_ms_per_frame", slaveAvgMs);
    PrintCtestMeasurement(prefix + "stylus_valid_frames", stats.stylusValidFrames);
    PrintCtestMeasurement(prefix + "stylus_signal_frames", stats.stylusSignalFrames);
    PrintCtestMeasurement(prefix + "stylus_dataset_raw_signal_frames", stats.stylusDatasetRawSignalFrames);
    PrintCtestMeasurement(prefix + "stylus_dataset_suffix_signal_frames", stats.stylusDatasetSuffixSignalFrames);
    PrintCtestMeasurement(prefix + "stylus_dataset_recorded_tx1_valid_frames", stats.stylusDatasetRecordedTx1ValidFrames);
    PrintCtestMeasurement(prefix + "stylus_dataset_recorded_output_valid_frames", stats.stylusDatasetRecordedOutputValidFrames);
}

} // namespace

int main(int argc, char** argv) {
    try {
        const PriorityStatus priorityStatus = ElevateBenchmarkPriority();
        PrintPriorityStatus(priorityStatus);

        const Options options = ParseOptions(argc, argv);
        const std::filesystem::path configPath = ResolveConfigPath(options.configPath);
        if (configPath.empty()) {
            std::cout << "[RawdataBenchmark] config_ini=<defaults>\n";
        }

        Options resolvedOptions = options;
        const std::filesystem::path touchDatasetPath = ResolveDatasetPath(options.touchDatasetPath, argv[0]);
        const std::filesystem::path stylusDatasetPath = ResolveDatasetPath(options.stylusDatasetPath, argv[0]);
        resolvedOptions.touchDatasetPath = touchDatasetPath;
        resolvedOptions.stylusDatasetPath = stylusDatasetPath;

        const DvrDataset touchDataset = LoadDvrDataset(touchDatasetPath);
        const DvrDataset stylusDataset = LoadDvrDataset(stylusDatasetPath);

        if (touchDataset.frames.size() == 480) {
            const auto replay480 = BuildReplaySequence(480);
            if (replay480.size() != 960 || replay480[479] != 479 || replay480[480] != 479 ||
                replay480[481] != 478) {
                std::cerr << "[RawdataBenchmark] Internal replay-sequence validation failed.\n";
                return 2;
            }
        }

        if (options.mode == BenchmarkMode::Both) {
            const RunStats linkedStats = RunBenchmark(
                BenchmarkMode::Linked, resolvedOptions, configPath, touchDataset, stylusDataset);
            PrintStats(linkedStats, resolvedOptions, configPath);
            PrintCtestMeasurements(linkedStats);
            RequireStylusReadback(linkedStats, stylusDataset);
            std::cout << "\n";

            const RunStats independentStats = RunBenchmark(
                BenchmarkMode::Independent, resolvedOptions, configPath, touchDataset, stylusDataset);
            PrintStats(independentStats, resolvedOptions, configPath);
            PrintCtestMeasurements(independentStats);
            RequireStylusReadback(independentStats, stylusDataset);
        } else {
            const RunStats stats = RunBenchmark(
                options.mode, resolvedOptions, configPath, touchDataset, stylusDataset);
            PrintStats(stats, resolvedOptions, configPath);
            PrintCtestMeasurements(stats);
            RequireStylusReadback(stats, stylusDataset);
        }

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[RawdataBenchmark] " << ex.what() << "\n";
        return 10;
    }
}
