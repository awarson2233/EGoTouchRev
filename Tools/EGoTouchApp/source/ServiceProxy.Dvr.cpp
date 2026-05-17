#include "ServiceProxyInternal.h"
#include "Logger.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <type_traits>
#include <vector>

namespace App {

namespace {

constexpr const char* kExportRootDir = "C:/ProgramData/EGoTouchRev/exports";
constexpr int kCurrentDvrFormatVersion = 3;

enum DvrBinaryFlags : uint32_t {
    kDvrFlagHasStylusDiagnostics    = 1u << 0,
    kDvrFlagHasStructuredSuffix     = 1u << 1,
    kDvrFlagHasReceiveSystemEpochUs = 1u << 2,
    kDvrFlagHasDynamicDebug         = 1u << 3,
};

enum class Dvr2SectionType : uint32_t {
    Meta = 1,
    Index = 2,
    Frames = 3,
    DynamicDebugSchema = 4,
    DynamicDebugValues = 5,
};

struct Dvr2FileHeader {
    char magic[8];
    uint16_t formatVersion = static_cast<uint16_t>(kCurrentDvrFormatVersion);
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
    uint32_t frameRecordVersion = 1;
    uint32_t flags = 0;
    uint32_t reserved = 0;
};

struct Dvr2DynamicDebugSchemaHeaderV1 {
    uint16_t schemaVersion = 0;
    uint16_t fieldCount = 0;
    uint32_t schemaHash = 0;
    uint32_t recordSize = 0;
    uint32_t reserved = 0;
};

struct Dvr2DynamicDebugValuesHeaderV1 {
    uint32_t frameCount = 0;
    uint32_t reserved = 0;
};

struct Dvr2DynamicDebugFrameHeaderV1 {
    uint32_t sampleCount = 0;
    uint32_t reserved = 0;
};

struct Dvr2DynamicDebugSampleV1 {
    uint16_t fieldId = 0;
    uint8_t valueType = static_cast<uint8_t>(Ipc::DebugValueType::UInt32);
    uint8_t flags = 0;
    uint32_t reserved = 0;
    uint64_t rawValue = 0;
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
    int16_t heatmapMatrix[40][60]{};
    uint16_t masterSuffix[Frame::kMasterSuffixWords]{};
    uint16_t slaveSuffix[Frame::kSlaveSuffixWords]{};
    Dvr2StylusDataRecordV1 stylus{};
    Dvr2ContactRecordV1 contacts[Dvr::kMaxContacts]{};
    uint32_t contactCount = 0;
    Dvr2PeakRecordV1 peaks[Dvr::kMaxPeaks]{};
    uint32_t peakCount = 0;
};

static_assert(std::is_trivially_copyable_v<Dvr2FrameRecordV1>);
static_assert(std::is_standard_layout_v<Dvr2FrameRecordV1>);

bool WriteAll(std::ofstream& out, const void* data, size_t bytes) {
    out.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(bytes));
    return static_cast<bool>(out);
}

bool ReadAll(std::ifstream& in, void* data, size_t bytes) {
    in.read(reinterpret_cast<char*>(data), static_cast<std::streamsize>(bytes));
    return static_cast<bool>(in);
}

std::array<char, 8> MakeDvr2Magic() {
    return {'E', 'G', 'O', 'D', 'V', 'R', '2', '\0'};
}

std::array<char, 8> MakeLegacyDvrMagic() {
    return {'E', 'G', 'O', 'D', 'V', 'R', 'B', '1'};
}

std::string MakeDatasetTimestampString() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_s(&tm, &t);
    const auto us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000;
    std::ostringstream ts;
    ts << std::put_time(&tm, "%Y%m%d_%H%M%S")
       << '_' << std::setfill('0') << std::setw(6) << us.count();
    return ts.str();
}

std::filesystem::path MakeDvrExportRoot() {
    namespace fs = std::filesystem;
    fs::path dir(kExportRootDir);
    dir /= "dvr";
    return dir;
}

std::string MakeDvrDatasetName() {
    return "dvr" + MakeDatasetTimestampString();
}

uint32_t ComputeDvrBinaryFlags(const std::vector<Dvr::DvrFrameSlot>& frames) {
    uint32_t flags = kDvrFlagHasStylusDiagnostics | kDvrFlagHasStructuredSuffix;
    for (const auto& frame : frames) {
        if (frame.receiveSystemEpochUs != 0) {
            flags |= kDvrFlagHasReceiveSystemEpochUs;
        }
    }
    return flags;
}

bool HasUniqueDynamicFieldIds(const DvrDynamicDebugSchema& schema) {
    for (size_t i = 0; i < schema.fields.size(); ++i) {
        for (size_t j = i + 1; j < schema.fields.size(); ++j) {
            if (schema.fields[i].fieldId == schema.fields[j].fieldId) {
                return false;
            }
        }
    }
    return true;
}

bool DynamicDebugSamplesMatchSchema(const DvrDynamicDebugSchema& schema,
                                    const DvrDynamicDebugFrame& frame) {
    if (frame.samples.size() != schema.fields.size()) return false;
    for (size_t i = 0; i < schema.fields.size(); ++i) {
        if (frame.samples[i].fieldId != schema.fields[i].fieldId) return false;
        if (frame.samples[i].value.valueType != schema.fields[i].valueType) return false;
    }
    return true;
}

bool CanPersistDynamicDebug(const std::vector<Dvr::DvrFrameSlot>& frames,
                            const DvrDynamicDebugSchema* dynamicSchema) {
    if (!dynamicSchema || dynamicSchema->fields.empty()) return false;
    if (!HasUniqueDynamicFieldIds(*dynamicSchema)) return false;
    for (const auto& frame : frames) {
        if (!DynamicDebugSamplesMatchSchema(*dynamicSchema, frame.dynamicDebug)) {
            return false;
        }
    }
    return true;
}

bool ValidateDynamicDebugPayload(const DvrDynamicDebugSchema& schema,
                                 const std::vector<DvrDynamicDebugFrame>& frames,
                                 uint32_t expectedFrameCount,
                                 std::string* outError) {
    if (schema.fields.empty()) {
        if (outError) *outError = "DVR2 dynamic schema section contains no fields";
        return false;
    }
    if (!HasUniqueDynamicFieldIds(schema)) {
        if (outError) *outError = "DVR2 dynamic schema contains duplicate field IDs";
        return false;
    }
    if (frames.size() != expectedFrameCount) {
        if (outError) *outError = "DVR2 dynamic values frame count mismatch";
        return false;
    }
    for (size_t i = 0; i < frames.size(); ++i) {
        if (!DynamicDebugSamplesMatchSchema(schema, frames[i])) {
            if (outError) *outError = "DVR2 dynamic values do not match dynamic schema";
            return false;
        }
    }
    return true;
}

Dvr2FrameRecordV1 MakeFrameRecord(const Dvr::DvrFrameSlot& src) {
    Dvr2FrameRecordV1 dst{};
    dst.timestamp = src.timestamp;
    dst.receiveSystemEpochUs = src.receiveSystemEpochUs;
    dst.dvrSeq = src.dvrSeq;
    dst.masterWasRead = src.masterWasRead ? 1 : 0;
    dst.masterSuffixValid = src.masterSuffixValid ? 1 : 0;
    dst.slaveSuffixValid = src.slaveSuffixValid ? 1 : 0;
    std::memcpy(dst.heatmapMatrix, src.heatmapMatrix, sizeof(dst.heatmapMatrix));
    std::memcpy(dst.masterSuffix, src.masterSuffix.words, sizeof(dst.masterSuffix));
    std::memcpy(dst.slaveSuffix, src.slaveSuffix.words, sizeof(dst.slaveSuffix));

    dst.stylus.slaveValid = src.stylus.slaveValid ? 1 : 0;
    dst.stylus.checksumOk = src.stylus.checksumOk ? 1 : 0;
    dst.stylus.tx1BlockValid = src.stylus.tx1BlockValid ? 1 : 0;
    dst.stylus.tx2BlockValid = src.stylus.tx2BlockValid ? 1 : 0;
    dst.stylus.status = src.stylus.status;
    dst.stylus.pressure = src.stylus.pressure;
    dst.stylus.signalX = src.stylus.signalX;
    dst.stylus.signalY = src.stylus.signalY;
    dst.stylus.maxRawPeak = src.stylus.maxRawPeak;
    dst.stylus.pipelineStage = src.stylus.pipelineStage;
    dst.stylus.point.valid = src.stylus.point.valid ? 1 : 0;
    dst.stylus.point.x = src.stylus.point.x;
    dst.stylus.point.y = src.stylus.point.y;
    dst.stylus.point.reportX = src.stylus.point.reportX;
    dst.stylus.point.reportY = src.stylus.point.reportY;
    dst.stylus.point.pressure = src.stylus.point.pressure;
    dst.stylus.point.rawPressure = src.stylus.point.rawPressure;
    dst.stylus.point.mappedPressure = src.stylus.point.mappedPressure;
    dst.stylus.point.peakTx1 = src.stylus.point.peakTx1;
    dst.stylus.point.peakTx2 = src.stylus.point.peakTx2;
    dst.stylus.point.tx1X = src.stylus.point.tx1X;
    dst.stylus.point.tx1Y = src.stylus.point.tx1Y;
    dst.stylus.point.tx2X = src.stylus.point.tx2X;
    dst.stylus.point.tx2Y = src.stylus.point.tx2Y;
    dst.stylus.point.confidence = src.stylus.point.confidence;

    dst.contactCount = std::min<uint32_t>(src.contactCount, Dvr::kMaxContacts);
    for (uint32_t i = 0; i < dst.contactCount; ++i) {
        dst.contacts[i].id = src.contacts[i].id;
        dst.contacts[i].x = src.contacts[i].x;
        dst.contacts[i].y = src.contacts[i].y;
        dst.contacts[i].state = src.contacts[i].state;
        dst.contacts[i].area = src.contacts[i].area;
        dst.contacts[i].signalSum = src.contacts[i].signalSum;
    }

    dst.peakCount = std::min<uint32_t>(src.peakCount, Dvr::kMaxPeaks);
    for (uint32_t i = 0; i < dst.peakCount; ++i) {
        dst.peaks[i].r = src.peaks[i].r;
        dst.peaks[i].c = src.peaks[i].c;
        dst.peaks[i].z = src.peaks[i].z;
        dst.peaks[i].id = src.peaks[i].id;
    }
    return dst;
}

void PopulateHeatmapFrameFromRecord(const Dvr2FrameRecordV1& src, Solvers::HeatmapFrame& dst) {
    dst = {};
    dst.timestamp = src.timestamp;
    dst.receiveSystemEpochUs = src.receiveSystemEpochUs;
    dst.masterWasRead = src.masterWasRead != 0;
    std::memcpy(dst.heatmapMatrix, src.heatmapMatrix, sizeof(dst.heatmapMatrix));
    std::memcpy(dst.masterSuffix.words, src.masterSuffix, sizeof(src.masterSuffix));
    std::memcpy(dst.slaveSuffix.words, src.slaveSuffix, sizeof(src.slaveSuffix));
    dst.masterSuffixValid = src.masterSuffixValid != 0;
    dst.slaveSuffixValid = src.slaveSuffixValid != 0;

    dst.stylus.input.slaveValid = src.stylus.slaveValid != 0;
    dst.stylus.input.checksumOk = src.stylus.checksumOk != 0;
    dst.stylus.input.tx1BlockValid = src.stylus.tx1BlockValid != 0;
    dst.stylus.input.tx2BlockValid = src.stylus.tx2BlockValid != 0;
    dst.stylus.input.status = src.stylus.status;
    dst.stylus.output.pressure = src.stylus.pressure;
    dst.stylus.interop.signalX = src.stylus.signalX;
    dst.stylus.interop.signalY = src.stylus.signalY;
    dst.stylus.interop.maxRawPeak = src.stylus.maxRawPeak;
    dst.stylus.output.pipelineStage = src.stylus.pipelineStage;
    dst.stylus.output.point.valid = src.stylus.point.valid != 0;
    dst.stylus.output.point.x = src.stylus.point.x;
    dst.stylus.output.point.y = src.stylus.point.y;
    dst.stylus.output.point.reportX = src.stylus.point.reportX;
    dst.stylus.output.point.reportY = src.stylus.point.reportY;
    dst.stylus.output.point.pressure = src.stylus.point.pressure;
    dst.stylus.output.point.rawPressure = src.stylus.point.rawPressure;
    dst.stylus.output.point.mappedPressure = src.stylus.point.mappedPressure;
    dst.stylus.output.point.peakTx1 = src.stylus.point.peakTx1;
    dst.stylus.output.point.peakTx2 = src.stylus.point.peakTx2;
    dst.stylus.output.point.tx1X = src.stylus.point.tx1X;
    dst.stylus.output.point.tx1Y = src.stylus.point.tx1Y;
    dst.stylus.output.point.tx2X = src.stylus.point.tx2X;
    dst.stylus.output.point.tx2Y = src.stylus.point.tx2Y;
    dst.stylus.output.point.confidence = src.stylus.point.confidence;

    dst.contacts.clear();
    dst.contacts.reserve(std::min<uint32_t>(src.contactCount, Dvr::kMaxContacts));
    for (uint32_t i = 0; i < src.contactCount && i < static_cast<uint32_t>(Dvr::kMaxContacts); ++i) {
        Solvers::TouchContact tc{};
        tc.id = src.contacts[i].id;
        tc.x = src.contacts[i].x;
        tc.y = src.contacts[i].y;
        tc.state = src.contacts[i].state;
        tc.area = src.contacts[i].area;
        tc.signalSum = src.contacts[i].signalSum;
        dst.contacts.push_back(tc);
    }

    dst.peaks.clear();
    dst.peaks.reserve(std::min<uint32_t>(src.peakCount, Dvr::kMaxPeaks));
    for (uint32_t i = 0; i < src.peakCount && i < static_cast<uint32_t>(Dvr::kMaxPeaks); ++i) {
        Solvers::TouchPeak tp{};
        tp.r = src.peaks[i].r;
        tp.c = src.peaks[i].c;
        tp.z = src.peaks[i].z;
        tp.id = src.peaks[i].id;
        dst.peaks.push_back(tp);
    }
}

bool ReadSectionEntries(std::ifstream& in,
                        const Dvr2FileHeader& header,
                        std::vector<Dvr2SectionEntry>& outSections,
                        std::string* outError) {
    in.seekg(static_cast<std::streamoff>(header.tocOffset), std::ios::beg);
    if (!in.good()) {
        if (outError) *outError = "invalid DVR2 TOC offset";
        return false;
    }
    outSections.assign(header.sectionCount, {});
    if (!ReadAll(in, outSections.data(), outSections.size() * sizeof(Dvr2SectionEntry))) {
        if (outError) *outError = "failed to read DVR2 TOC";
        return false;
    }
    return true;
}

const Dvr2SectionEntry* FindSection(const std::vector<Dvr2SectionEntry>& sections, Dvr2SectionType type) {
    for (const auto& section : sections) {
        if (section.type == static_cast<uint32_t>(type)) return &section;
    }
    return nullptr;
}

template <size_t N>
std::string FormatCsvPacketBytes(const std::array<uint8_t, N>& bytes) {
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (size_t i = 0; i < bytes.size(); ++i) {
        out << std::setw(2) << static_cast<unsigned int>(bytes[i]);
        if (i + 1 < bytes.size()) out << ' ';
    }
    out << std::dec;
    return out.str();
}

template <size_t N>
bool WriteCsvPacketLine(std::ostream& out, const std::array<uint8_t, N>& bytes) {
    out << FormatCsvPacketBytes(bytes);
    return static_cast<bool>(out);
}

struct CsvKeyValueRow {
    std::string key;
    std::string value;
};

const DvrDynamicDebugSample* FindDynamicSample(const DvrDynamicDebugFrame* frame, uint16_t fieldId) {
    if (!frame) return nullptr;
    for (const auto& sample : frame->samples) {
        if (sample.fieldId == fieldId) return &sample;
    }
    return nullptr;
}

std::string ResolveDynamicColumnName(const DynamicDebugField& field) {
    if (!field.dvrColumnName.empty()) return field.dvrColumnName;
    if (!field.displayName.empty()) return field.displayName;
    return field.key;
}

std::string FormatDynamicDebugValue(const DvrDynamicDebugSample* sample) {
    if (!sample || !sample->value.valid) {
        return "N/A";
    }
    std::ostringstream out;
    switch (sample->value.valueType) {
    case Ipc::DebugValueType::UInt32:
        out << static_cast<unsigned int>(sample->value.rawValue & 0xFFFFFFFFu);
        break;
    case Ipc::DebugValueType::Int32:
        out << static_cast<int32_t>(sample->value.rawValue & 0xFFFFFFFFu);
        break;
    case Ipc::DebugValueType::Float32: {
        uint32_t bits = static_cast<uint32_t>(sample->value.rawValue & 0xFFFFFFFFu);
        float fv = 0.0f;
        std::memcpy(&fv, &bits, sizeof(fv));
        out << fv;
        break;
    }
    case Ipc::DebugValueType::Bool:
        out << (((sample->value.rawValue & 0x1ull) != 0) ? 1 : 0);
        break;
    default:
        out << "<unknown>";
        break;
    }
    return out.str();
}

void InsertDynamicRow(std::vector<CsvKeyValueRow>& rows,
                      const DynamicDebugField& field,
                      const DvrDynamicDebugFrame* dynamicFrame) {
    CsvKeyValueRow row{ResolveDynamicColumnName(field),
                       FormatDynamicDebugValue(FindDynamicSample(dynamicFrame, field.fieldId))};

    switch (field.dvrPositionMode) {
    case Ipc::DebugDvrPositionMode::AfterAnchor:
        if (!field.dvrAnchor.empty()) {
            for (size_t i = 0; i < rows.size(); ++i) {
                if (rows[i].key == field.dvrAnchor) {
                    rows.insert(rows.begin() + static_cast<std::ptrdiff_t>(i + 1), std::move(row));
                    return;
                }
            }
        }
        rows.push_back(std::move(row));
        return;
    case Ipc::DebugDvrPositionMode::Index:
        if (field.dvrIndex >= 0) {
            const size_t clamped = std::min<size_t>(static_cast<size_t>(field.dvrIndex), rows.size());
            rows.insert(rows.begin() + static_cast<std::ptrdiff_t>(clamped), std::move(row));
            return;
        }
        rows.push_back(std::move(row));
        return;
    case Ipc::DebugDvrPositionMode::Append:
    default:
        rows.push_back(std::move(row));
        return;
    }
}

void ApplyDynamicRows(std::vector<CsvKeyValueRow>& rows,
                      Ipc::DebugDvrTarget target,
                      const DvrDynamicDebugSchema* dynamicSchema,
                      const DvrDynamicDebugFrame* dynamicFrame) {
    if (!dynamicSchema) return;

    std::vector<const DynamicDebugField*> ordered;
    ordered.reserve(dynamicSchema->fields.size());
    for (const auto& field : dynamicSchema->fields) {
        if (field.dvrTarget == target) {
            ordered.push_back(&field);
        }
    }
    std::stable_sort(ordered.begin(), ordered.end(), [](const DynamicDebugField* a, const DynamicDebugField* b) {
        return a->uiOrder < b->uiOrder;
    });
    for (const auto* field : ordered) {
        InsertDynamicRow(rows, *field, dynamicFrame);
    }
}

void WriteCsvKeyValueSection(std::ostream& out,
                             std::string_view title,
                             const std::vector<CsvKeyValueRow>& rows) {
    out << "--- " << title << " ---\n";
    for (const auto& row : rows) {
        out << row.key << ',' << row.value << "\n";
    }
    out << "\n";
}

bool ReadDynamicDebugSchemaSection(std::ifstream& in,
                                   const Dvr2SectionEntry& section,
                                   DvrDynamicDebugSchema& outSchema,
                                   std::string* outError) {
    outSchema = {};
    if (section.size < sizeof(Dvr2DynamicDebugSchemaHeaderV1)) {
        if (outError) *outError = "DVR2 dynamic schema section is truncated";
        return false;
    }

    in.seekg(static_cast<std::streamoff>(section.offset), std::ios::beg);
    if (!in.good()) {
        if (outError) *outError = "invalid DVR2 dynamic schema offset";
        return false;
    }

    Dvr2DynamicDebugSchemaHeaderV1 header{};
    if (!ReadAll(in, &header, sizeof(header))) {
        if (outError) *outError = "failed to read DVR2 dynamic schema header";
        return false;
    }
    if (header.recordSize != sizeof(Ipc::DebugFieldSchemaWire)) {
        if (outError) *outError = "unsupported DVR2 dynamic schema record size";
        return false;
    }

    const uint64_t requiredSize = sizeof(Dvr2DynamicDebugSchemaHeaderV1) +
        static_cast<uint64_t>(header.fieldCount) * sizeof(Ipc::DebugFieldSchemaWire);
    if (section.size != requiredSize) {
        if (outError) *outError = "DVR2 dynamic schema section size mismatch";
        return false;
    }

    outSchema.schemaVersion = header.schemaVersion;
    outSchema.schemaHash = header.schemaHash;
    outSchema.fields.reserve(header.fieldCount);
    for (uint16_t i = 0; i < header.fieldCount; ++i) {
        Ipc::DebugFieldSchemaWire wire{};
        if (!ReadAll(in, &wire, sizeof(wire))) {
            if (outError) *outError = "failed to read DVR2 dynamic schema record";
            return false;
        }

        DynamicDebugField field{};
        field.fieldId = wire.fieldId;
        field.valueType = static_cast<Ipc::DebugValueType>(wire.valueType);
        field.sourceKind = static_cast<Ipc::DebugSourceKind>(wire.sourceKind);
        field.sourceIndex = wire.sourceIndex;
        field.uiOrder = wire.uiOrder;
        field.dvrTarget = static_cast<Ipc::DebugDvrTarget>(wire.dvrTarget);
        field.dvrPositionMode = static_cast<Ipc::DebugDvrPositionMode>(wire.dvrPositionMode);
        field.dvrIndex = wire.dvrIndex;
        field.key = wire.key;
        field.displayName = wire.displayName;
        field.unit = wire.unit;
        field.uiGroup = wire.uiGroup;
        field.dvrColumnName = wire.dvrColumnName;
        field.dvrAnchor = wire.dvrAnchor;
        outSchema.fields.push_back(std::move(field));
    }
    return true;
}

bool ReadDynamicDebugValuesSection(std::ifstream& in,
                                   const Dvr2SectionEntry& section,
                                   uint32_t expectedFrameCount,
                                   std::vector<DvrDynamicDebugFrame>& outFrames,
                                   std::string* outError) {
    outFrames.clear();
    if (section.size < sizeof(Dvr2DynamicDebugValuesHeaderV1)) {
        if (outError) *outError = "DVR2 dynamic values section is truncated";
        return false;
    }

    in.seekg(static_cast<std::streamoff>(section.offset), std::ios::beg);
    if (!in.good()) {
        if (outError) *outError = "invalid DVR2 dynamic values offset";
        return false;
    }

    Dvr2DynamicDebugValuesHeaderV1 header{};
    if (!ReadAll(in, &header, sizeof(header))) {
        if (outError) *outError = "failed to read DVR2 dynamic values header";
        return false;
    }
    if (header.frameCount != expectedFrameCount) {
        if (outError) *outError = "DVR2 dynamic values frame count mismatch";
        return false;
    }

    uint64_t bytesConsumed = sizeof(Dvr2DynamicDebugValuesHeaderV1);
    outFrames.resize(header.frameCount);
    for (uint32_t frameIndex = 0; frameIndex < header.frameCount; ++frameIndex) {
        if (bytesConsumed + sizeof(Dvr2DynamicDebugFrameHeaderV1) > section.size) {
            if (outError) *outError = "DVR2 dynamic values frame header exceeds section bounds";
            return false;
        }

        Dvr2DynamicDebugFrameHeaderV1 frameHeader{};
        if (!ReadAll(in, &frameHeader, sizeof(frameHeader))) {
            if (outError) *outError = "failed to read DVR2 dynamic frame header";
            return false;
        }
        bytesConsumed += sizeof(Dvr2DynamicDebugFrameHeaderV1);

        const uint64_t frameBytes = static_cast<uint64_t>(frameHeader.sampleCount) * sizeof(Dvr2DynamicDebugSampleV1);
        if (bytesConsumed + frameBytes > section.size) {
            if (outError) *outError = "DVR2 dynamic frame exceeds section bounds";
            return false;
        }

        auto& frame = outFrames[frameIndex];
        frame.samples.reserve(frameHeader.sampleCount);
        for (uint32_t sampleIndex = 0; sampleIndex < frameHeader.sampleCount; ++sampleIndex) {
            Dvr2DynamicDebugSampleV1 wire{};
            if (!ReadAll(in, &wire, sizeof(wire))) {
                if (outError) *outError = "failed to read DVR2 dynamic sample";
                return false;
            }

            DvrDynamicDebugSample sample{};
            sample.fieldId = wire.fieldId;
            sample.value.valueType = static_cast<Ipc::DebugValueType>(wire.valueType);
            sample.value.valid = (wire.flags & 0x1u) != 0;
            sample.value.rawValue = wire.rawValue;
            frame.samples.push_back(std::move(sample));
        }
        bytesConsumed += frameBytes;
    }
    if (bytesConsumed != section.size) {
        if (outError) *outError = "DVR2 dynamic values section size mismatch";
        return false;
    }
    return true;
}

} // namespace

std::filesystem::path ResolveReplayBinaryPath(const std::filesystem::path& input) {
    return input;
}

bool WriteDvrBinaryFile(const std::filesystem::path& filePath,
                        const std::vector<Dvr::DvrFrameSlot>& frames,
                        const DvrDynamicDebugSchema* dynamicSchema,
                        uint32_t* outFlags) {
    if (frames.empty()) return false;

    uint32_t flags = ComputeDvrBinaryFlags(frames);

    std::vector<Dvr2FrameRecordV1> records;
    records.reserve(frames.size());
    for (const auto& frame : frames) {
        records.push_back(MakeFrameRecord(frame));
    }

    DvrDynamicDebugSchema effectiveDynamicSchema{};
    if (dynamicSchema) {
        effectiveDynamicSchema = *dynamicSchema;
    }

    const bool hasDynamicDebug = CanPersistDynamicDebug(frames, dynamicSchema);
    if (hasDynamicDebug) {
        flags |= kDvrFlagHasDynamicDebug;
    }
    if (outFlags) *outFlags = flags;

    std::vector<Ipc::DebugFieldSchemaWire> dynamicSchemaRecords;
    if (hasDynamicDebug) {
        dynamicSchemaRecords.reserve(effectiveDynamicSchema.fields.size());
        for (const auto& field : effectiveDynamicSchema.fields) {
            Ipc::DebugFieldSchemaWire wire{};
            wire.fieldId = field.fieldId;
            wire.valueType = static_cast<uint8_t>(field.valueType);
            wire.sourceKind = static_cast<uint8_t>(field.sourceKind);
            wire.sourceIndex = field.sourceIndex;
            wire.uiOrder = field.uiOrder;
            wire.dvrTarget = static_cast<uint8_t>(field.dvrTarget);
            wire.dvrPositionMode = static_cast<uint8_t>(field.dvrPositionMode);
            wire.dvrIndex = field.dvrIndex;
            std::snprintf(wire.key, sizeof(wire.key), "%s", field.key.c_str());
            std::snprintf(wire.displayName, sizeof(wire.displayName), "%s", field.displayName.c_str());
            std::snprintf(wire.unit, sizeof(wire.unit), "%s", field.unit.c_str());
            std::snprintf(wire.uiGroup, sizeof(wire.uiGroup), "%s", field.uiGroup.c_str());
            std::snprintf(wire.dvrColumnName, sizeof(wire.dvrColumnName), "%s", field.dvrColumnName.c_str());
            std::snprintf(wire.dvrAnchor, sizeof(wire.dvrAnchor), "%s", field.dvrAnchor.c_str());
            dynamicSchemaRecords.push_back(wire);
        }
    }

    Dvr2DynamicDebugSchemaHeaderV1 dynamicSchemaHeader{};
    std::vector<uint8_t> dynamicValuesBlob;
    if (hasDynamicDebug) {
        Dvr2DynamicDebugValuesHeaderV1 valuesHeader{};
        valuesHeader.frameCount = static_cast<uint32_t>(frames.size());
        dynamicValuesBlob.insert(dynamicValuesBlob.end(),
                                 reinterpret_cast<const uint8_t*>(&valuesHeader),
                                 reinterpret_cast<const uint8_t*>(&valuesHeader) + sizeof(valuesHeader));
        for (const auto& frame : frames) {
            Dvr2DynamicDebugFrameHeaderV1 frameHeader{};
            frameHeader.sampleCount = static_cast<uint32_t>(frame.dynamicDebug.samples.size());
            dynamicValuesBlob.insert(dynamicValuesBlob.end(),
                                     reinterpret_cast<const uint8_t*>(&frameHeader),
                                     reinterpret_cast<const uint8_t*>(&frameHeader) + sizeof(frameHeader));
            for (const auto& sample : frame.dynamicDebug.samples) {
                Dvr2DynamicDebugSampleV1 wire{};
                wire.fieldId = sample.fieldId;
                wire.valueType = static_cast<uint8_t>(sample.value.valueType);
                wire.flags = sample.value.valid ? 0x1u : 0x0u;
                wire.rawValue = sample.value.rawValue;
                dynamicValuesBlob.insert(dynamicValuesBlob.end(),
                                         reinterpret_cast<const uint8_t*>(&wire),
                                         reinterpret_cast<const uint8_t*>(&wire) + sizeof(wire));
            }
        }
    }

    Dvr2FileHeader header{};
    const auto magic = MakeDvr2Magic();
    std::copy(magic.begin(), magic.end(), header.magic);
    header.formatVersion = static_cast<uint16_t>(kCurrentDvrFormatVersion);
    header.sectionCount = hasDynamicDebug ? 5u : 3u;
    header.tocOffset = sizeof(Dvr2FileHeader);
    header.flags = flags;

    const uint64_t tocSize = static_cast<uint64_t>(header.sectionCount) * sizeof(Dvr2SectionEntry);
    const uint64_t metaOffset = header.tocOffset + tocSize;
    const uint64_t metaSize = sizeof(Dvr2MetaSectionV1);
    const uint64_t indexOffset = metaOffset + metaSize;
    const uint64_t indexSize = static_cast<uint64_t>(records.size()) * sizeof(Dvr2IndexEntryV1);
    const uint64_t framesOffset = indexOffset + indexSize;
    const uint64_t framesSize = static_cast<uint64_t>(records.size()) * sizeof(Dvr2FrameRecordV1);
    uint64_t nextOffset = framesOffset + framesSize;

    std::vector<Dvr2SectionEntry> sections;
    sections.reserve(header.sectionCount);
    sections.push_back({static_cast<uint32_t>(Dvr2SectionType::Meta), 1, metaOffset, metaSize});
    sections.push_back({static_cast<uint32_t>(Dvr2SectionType::Index), 1, indexOffset, indexSize});
    sections.push_back({static_cast<uint32_t>(Dvr2SectionType::Frames), 1, framesOffset, framesSize});

    if (hasDynamicDebug) {
        const uint64_t dynamicSchemaOffset = nextOffset;
        const uint64_t dynamicSchemaSize = sizeof(Dvr2DynamicDebugSchemaHeaderV1) +
            static_cast<uint64_t>(dynamicSchemaRecords.size()) * sizeof(Ipc::DebugFieldSchemaWire);
        nextOffset += dynamicSchemaSize;
        const uint64_t dynamicValuesOffset = nextOffset;
        const uint64_t dynamicValuesSize = static_cast<uint64_t>(dynamicValuesBlob.size());
        nextOffset += dynamicValuesSize;
        sections.push_back({static_cast<uint32_t>(Dvr2SectionType::DynamicDebugSchema), 1, dynamicSchemaOffset, dynamicSchemaSize});
        sections.push_back({static_cast<uint32_t>(Dvr2SectionType::DynamicDebugValues), 1, dynamicValuesOffset, dynamicValuesSize});
        dynamicSchemaHeader.schemaVersion = effectiveDynamicSchema.schemaVersion;
        dynamicSchemaHeader.fieldCount = static_cast<uint16_t>(dynamicSchemaRecords.size());
        dynamicSchemaHeader.schemaHash = effectiveDynamicSchema.schemaHash;
        dynamicSchemaHeader.recordSize = sizeof(Ipc::DebugFieldSchemaWire);
    }

    Dvr2MetaSectionV1 meta{};
    meta.frameCount = static_cast<uint32_t>(records.size());
    meta.frameRecordVersion = 1;
    meta.flags = flags;

    std::vector<Dvr2IndexEntryV1> index(records.size());
    for (size_t i = 0; i < records.size(); ++i) {
        index[i].timestamp = records[i].timestamp;
        index[i].receiveSystemEpochUs = records[i].receiveSystemEpochUs;
        index[i].frameOffset = framesOffset + static_cast<uint64_t>(i) * sizeof(Dvr2FrameRecordV1);
        index[i].frameSize = static_cast<uint32_t>(sizeof(Dvr2FrameRecordV1));
    }

    std::ofstream out(filePath, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return false;
    if (!WriteAll(out, &header, sizeof(header))) return false;
    if (!WriteAll(out, sections.data(), sections.size() * sizeof(Dvr2SectionEntry))) return false;
    if (!WriteAll(out, &meta, sizeof(meta))) return false;
    if (!WriteAll(out, index.data(), index.size() * sizeof(Dvr2IndexEntryV1))) return false;
    if (!WriteAll(out, records.data(), records.size() * sizeof(Dvr2FrameRecordV1))) return false;
    if (hasDynamicDebug) {
        if (!WriteAll(out, &dynamicSchemaHeader, sizeof(dynamicSchemaHeader))) return false;
        if (!dynamicSchemaRecords.empty() && !WriteAll(out, dynamicSchemaRecords.data(), dynamicSchemaRecords.size() * sizeof(Ipc::DebugFieldSchemaWire))) return false;
        if (!dynamicValuesBlob.empty() && !WriteAll(out, dynamicValuesBlob.data(), dynamicValuesBlob.size())) return false;
    }
    return true;
}

bool ReadDvrBinaryFile(const std::filesystem::path& filePath,
                       std::vector<Solvers::HeatmapFrame>& outFrames,
                       int& outVersion,
                       uint32_t* outFlags,
                       std::string* outError,
                       DvrDynamicDebugSchema* outDynamicSchema,
                       std::vector<DvrDynamicDebugFrame>* outDynamicFrames) {
    outFrames.clear();
    outVersion = 0;
    if (outFlags) *outFlags = 0;
    if (outError) outError->clear();
    if (outDynamicSchema) *outDynamicSchema = {};
    if (outDynamicFrames) outDynamicFrames->clear();

    std::ifstream in(filePath, std::ios::binary);
    if (!in.is_open()) {
        if (outError) *outError = "failed to open file";
        return false;
    }

    Dvr2FileHeader header{};
    if (!ReadAll(in, &header, sizeof(header))) {
        if (outError) *outError = "failed to read DVR2 header";
        return false;
    }

    const auto dvr2Magic = MakeDvr2Magic();
    const auto legacyMagic = MakeLegacyDvrMagic();
    if (std::equal(legacyMagic.begin(), legacyMagic.end(), header.magic)) {
        if (outError) *outError = "unsupported legacy DVR format; only DVR2 .dvrbin files are accepted";
        return false;
    }
    if (!std::equal(dvr2Magic.begin(), dvr2Magic.end(), header.magic)) {
        if (outError) *outError = "invalid DVR2 magic";
        return false;
    }
    if (header.sectionCount == 0) {
        if (outError) *outError = "DVR2 file contains no sections";
        return false;
    }

    std::vector<Dvr2SectionEntry> sections;
    if (!ReadSectionEntries(in, header, sections, outError)) {
        return false;
    }

    const auto* metaSection = FindSection(sections, Dvr2SectionType::Meta);
    const auto* indexSection = FindSection(sections, Dvr2SectionType::Index);
    const auto* framesSection = FindSection(sections, Dvr2SectionType::Frames);
    const auto* dynamicSchemaSection = FindSection(sections, Dvr2SectionType::DynamicDebugSchema);
    const auto* dynamicValuesSection = FindSection(sections, Dvr2SectionType::DynamicDebugValues);
    if (!metaSection || !indexSection || !framesSection) {
        if (outError) *outError = "DVR2 file is missing required sections";
        return false;
    }
    if (metaSection->version != 1 || indexSection->version != 1 || framesSection->version != 1) {
        if (outError) *outError = "unsupported DVR2 section version";
        return false;
    }

    in.seekg(static_cast<std::streamoff>(metaSection->offset), std::ios::beg);
    Dvr2MetaSectionV1 meta{};
    if (!ReadAll(in, &meta, sizeof(meta))) {
        if (outError) *outError = "failed to read DVR2 meta section";
        return false;
    }
    if (meta.frameRecordVersion != 1) {
        if (outError) *outError = "unsupported DVR2 frame record version";
        return false;
    }
    if (meta.flags != header.flags) {
        if (outError) *outError = "DVR2 header/meta flags mismatch";
        return false;
    }
    if (meta.frameCount == 0) {
        if (outError) *outError = "DVR2 dataset contains no frames";
        return false;
    }

    if (indexSection->size != static_cast<uint64_t>(meta.frameCount) * sizeof(Dvr2IndexEntryV1)) {
        if (outError) *outError = "DVR2 index section size mismatch";
        return false;
    }
    if (framesSection->size != static_cast<uint64_t>(meta.frameCount) * sizeof(Dvr2FrameRecordV1)) {
        if (outError) *outError = "DVR2 frame section size mismatch";
        return false;
    }

    std::vector<Dvr2IndexEntryV1> index(meta.frameCount);
    in.seekg(static_cast<std::streamoff>(indexSection->offset), std::ios::beg);
    if (!ReadAll(in, index.data(), index.size() * sizeof(Dvr2IndexEntryV1))) {
        if (outError) *outError = "failed to read DVR2 index section";
        return false;
    }

    outFrames.reserve(meta.frameCount);
    for (uint32_t i = 0; i < meta.frameCount; ++i) {
        if (index[i].frameSize != sizeof(Dvr2FrameRecordV1)) {
            if (outError) *outError = "unsupported DVR2 frame size";
            return false;
        }
        in.seekg(static_cast<std::streamoff>(index[i].frameOffset), std::ios::beg);
        if (!in.good()) {
            if (outError) *outError = "invalid DVR2 frame offset";
            return false;
        }
        Dvr2FrameRecordV1 record{};
        if (!ReadAll(in, &record, sizeof(record))) {
            if (outError) *outError = "failed to read DVR2 frame record";
            return false;
        }
        Solvers::HeatmapFrame frame{};
        PopulateHeatmapFrameFromRecord(record, frame);
        outFrames.push_back(std::move(frame));
    }

    if ((header.flags & kDvrFlagHasDynamicDebug) != 0) {
        if (!dynamicSchemaSection || !dynamicValuesSection) {
            if (outError) *outError = "DVR2 file is missing dynamic debug sections";
            return false;
        }
        if (dynamicSchemaSection->version != 1 || dynamicValuesSection->version != 1) {
            if (outError) *outError = "unsupported DVR2 dynamic debug section version";
            return false;
        }

        DvrDynamicDebugSchema parsedSchema{};
        if (!ReadDynamicDebugSchemaSection(in, *dynamicSchemaSection, parsedSchema, outError)) {
            return false;
        }

        std::vector<DvrDynamicDebugFrame> parsedFrames;
        if (!ReadDynamicDebugValuesSection(in, *dynamicValuesSection, meta.frameCount, parsedFrames, outError)) {
            return false;
        }
        if (!ValidateDynamicDebugPayload(parsedSchema, parsedFrames, meta.frameCount, outError)) {
            return false;
        }

        if (outDynamicSchema) {
            *outDynamicSchema = std::move(parsedSchema);
        }
        if (outDynamicFrames) {
            *outDynamicFrames = std::move(parsedFrames);
        }
    } else if (dynamicSchemaSection || dynamicValuesSection) {
        if (outError) *outError = "DVR2 file contains unexpected dynamic debug sections without HasDynamicDebug flag";
        return false;
    }

    outVersion = static_cast<int>(header.formatVersion);
    if (outFlags) *outFlags = header.flags;
    return true;
}

bool WriteFrameCsvFile(const std::filesystem::path& filePath,
                       const Solvers::HeatmapFrame& frame,
                       const Solvers::TouchPipeline* pipeline,
                       bool includeHeatmap,
                       bool includeMasterStatus,
                       bool includeSlaveStatus,
                       std::string_view captureMode,
                       bool includeMetadataHeader,
                       int formatVersion,
                       std::string_view sourceName,
                       const DvrDynamicDebugSchema* dynamicSchema,
                       const DvrDynamicDebugFrame* dynamicFrame) {
    std::ofstream out(filePath.string());
    if (!out.is_open()) {
        return false;
    }

    if (includeMetadataHeader) {
        out << "--- EGoTouch Frame Export ---\n";
        out << "ServiceTimestampRaw," << frame.timestamp << "\n";
        out << "HostReceiveEpochUs," << frame.receiveSystemEpochUs << "\n";
        out << "MasterSuffixTimestamp," << (frame.masterSuffixValid ? frame.masterSuffix.timestamp() : 0) << "\n";
        if (!sourceName.empty()) {
            out << "Source," << sourceName << "\n";
        }
        if (formatVersion > 0) {
            out << "DvrFormatVersion," << formatVersion << "\n";
        }
        out << "DatasetKind,DVR2\n";
        out << "CaptureMode," << captureMode << "\n\n";
    }

    if (pipeline) {
        out << "--- Config Parameters ---\n";
        out << "[TouchPipeline]\n";
        std::ostringstream cfgStream;
        pipeline->SaveConfig(cfgStream);
        out << cfgStream.str();
        out << "\n";
    }

    out << "--- Peaks ---\n";
    out << "Count," << frame.peaks.size() << "\n";
    out << "R,C,Z,ID\n";
    for (const auto& pk : frame.peaks) {
        out << pk.r << ',' << pk.c << ',' << pk.z << ',' << static_cast<unsigned int>(pk.id) << "\n";
    }
    out << "\n";

    out << "--- Contacts ---\n";
    out << "Count," << frame.contacts.size() << "\n";
    out << "ID,X,Y,State,Area,SignalSum,SizeMm,Reported,ReportEvent,LifeFlags,ReportFlags,DebugFlags\n";
    for (const auto& c : frame.contacts) {
        out << c.id << ','
            << c.x << ','
            << c.y << ','
            << c.state << ','
            << c.area << ','
            << c.signalSum << ','
            << c.sizeMm << ','
            << (c.isReported ? 1 : 0) << ','
            << c.reportEvent << ','
            << c.lifeFlags << ','
            << c.reportFlags << ','
            << c.debugFlags << "\n";
    }
    out << "\n";

    out << "--- Touch Packets (VHF 0x20) ---\n";
    out << "Packet0Valid," << (frame.touchPackets[0].valid ? 1 : 0) << "\n";
    out << "Packet0Hex,";
    WriteCsvPacketLine(out, frame.touchPackets[0].bytes);
    out << "\n";
    out << "Packet1Valid," << (frame.touchPackets[1].valid ? 1 : 0) << "\n";
    out << "Packet1Hex,";
    WriteCsvPacketLine(out, frame.touchPackets[1].bytes);
    out << "\n\n";


    if (includeHeatmap) {
        out << "--- Heatmap (40 rows x 60 cols) ---\n";
        for (int y = 0; y < 40; ++y) {
            for (int x = 0; x < 60; ++x) {
                out << frame.heatmapMatrix[y][x];
                if (x < 59) out << ',';
            }
            out << "\n";
        }
    }

    if (includeMasterStatus) {
        std::vector<CsvKeyValueRow> masterRows;
        masterRows.push_back({"MasterSuffixValid", frame.masterSuffixValid ? "1" : "0"});
        masterRows.push_back({"ContactCount", std::to_string(frame.contacts.size())});
        masterRows.push_back({"PeakCount", std::to_string(frame.peaks.size())});
        masterRows.push_back({"MasterWasRead", frame.masterWasRead ? "1" : "0"});
        if (frame.masterSuffixValid) {
            masterRows.push_back({"MasterSuffix_F0Noise", std::to_string(frame.masterSuffix.penF0NoiseCount())});
            masterRows.push_back({"MasterSuffix_F1Noise", std::to_string(frame.masterSuffix.penF1NoiseCount())});
            masterRows.push_back({"MasterSuffix_TpFreq1", std::to_string(frame.masterSuffix.tpFreq1())});
            masterRows.push_back({"MasterSuffix_TpFreq2", std::to_string(frame.masterSuffix.tpFreq2())});
            masterRows.push_back({"MasterSuffix_Timestamp", std::to_string(frame.masterSuffix.timestamp())});
        }
        ApplyDynamicRows(masterRows, Ipc::DebugDvrTarget::MasterStatus, dynamicSchema, dynamicFrame);
        WriteCsvKeyValueSection(out, "Master Status", masterRows);
    }

    if (includeSlaveStatus) {
        std::vector<CsvKeyValueRow> slaveRows;
        const auto& stylusInput = frame.stylus.input;
        const auto& stylusOutput = frame.stylus.output;
        const auto& stylusInterop = frame.stylus.interop;
        const auto& stylusPoint = stylusOutput.point;
#if EGOTOUCH_DIAG
        const auto& stylusDiag = frame.stylus.debug.coord;
#endif
        slaveRows.push_back({"SlaveValid", stylusInput.slaveValid ? "1" : "0"});
        slaveRows.push_back({"SlaveWordOffset", std::to_string(static_cast<unsigned int>(stylusInput.slaveWordOffset))});
        slaveRows.push_back({"Checksum16", std::to_string(stylusInput.checksum16)});
        slaveRows.push_back({"ChecksumOK", stylusInput.checksumOk ? "1" : "0"});
        slaveRows.push_back({"Tx1BlockValid", stylusInput.tx1BlockValid ? "1" : "0"});
        slaveRows.push_back({"Tx2BlockValid", stylusInput.tx2BlockValid ? "1" : "0"});
        slaveRows.push_back({"Status", std::to_string(stylusInput.status)});
        slaveRows.push_back({"RecheckEnabled", stylusInterop.recheckEnabled ? "1" : "0"});
        slaveRows.push_back({"RecheckPassed", stylusInterop.recheckPassed ? "1" : "0"});
        slaveRows.push_back({"RecheckOverlap", stylusInterop.recheckOverlap ? "1" : "0"});
        slaveRows.push_back({"RecheckThreshold", std::to_string(stylusInterop.recheckThreshold)});
        slaveRows.push_back({"TouchNullLike", stylusInterop.touchNullLike ? "1" : "0"});
        slaveRows.push_back({"TouchSuppressActive", stylusInterop.touchSuppressActive ? "1" : "0"});
        slaveRows.push_back({"TouchSuppressFrames", std::to_string(static_cast<unsigned int>(stylusInterop.touchSuppressFrames))});
        slaveRows.push_back({"PeakRawTx1", std::to_string(stylusInterop.signalX)});
        slaveRows.push_back({"PeakRawTx2", std::to_string(stylusInterop.signalY)});
        slaveRows.push_back({"MaxRawPeak", std::to_string(stylusInterop.maxRawPeak)});
#if EGOTOUCH_DIAG
        slaveRows.push_back({"PressureIsReal", stylusDiag.pressureIsReal ? "1" : "0"});
        slaveRows.push_back({"PredictedAgeFrames", std::to_string(static_cast<unsigned int>(stylusDiag.predictedAgeFrames))});
#endif
        slaveRows.push_back({"Pressure", std::to_string(stylusOutput.pressure)});
        slaveRows.push_back({"PointValid", stylusPoint.valid ? "1" : "0"});
        slaveRows.push_back({"PointX", std::to_string(stylusPoint.x)});
        slaveRows.push_back({"PointY", std::to_string(stylusPoint.y)});
        slaveRows.push_back({"ReportX", std::to_string(stylusPoint.reportX)});
        slaveRows.push_back({"ReportY", std::to_string(stylusPoint.reportY)});
        slaveRows.push_back({"PointConfidence", std::to_string(stylusPoint.confidence)});
        slaveRows.push_back({"RawPressure", std::to_string(stylusPoint.rawPressure)});
        slaveRows.push_back({"MappedPressure", std::to_string(stylusPoint.mappedPressure)});
        slaveRows.push_back({"SignalCompositeTx1", std::to_string(stylusPoint.peakTx1)});
        slaveRows.push_back({"SignalCompositeTx2", std::to_string(stylusPoint.peakTx2)});
        slaveRows.push_back({"Tx1X", std::to_string(stylusPoint.tx1X)});
        slaveRows.push_back({"Tx1Y", std::to_string(stylusPoint.tx1Y)});
        slaveRows.push_back({"Tx2X", std::to_string(stylusPoint.tx2X)});
        slaveRows.push_back({"Tx2Y", std::to_string(stylusPoint.tx2Y)});
        slaveRows.push_back({"TiltValid", stylusPoint.tiltValid ? "1" : "0"});
        slaveRows.push_back({"PreTiltX", std::to_string(stylusPoint.preTiltX)});
        slaveRows.push_back({"PreTiltY", std::to_string(stylusPoint.preTiltY)});
        slaveRows.push_back({"TiltX", std::to_string(stylusPoint.tiltX)});
        slaveRows.push_back({"TiltY", std::to_string(stylusPoint.tiltY)});
        slaveRows.push_back({"TiltMagnitude", std::to_string(stylusPoint.tiltMagnitude)});
        slaveRows.push_back({"TiltAzimuthDeg", std::to_string(stylusPoint.tiltAzimuthDeg)});
#if EGOTOUCH_DIAG
        slaveRows.push_back({"LegacyPacketValid", stylusOutput.packet.valid ? "1" : "0"});
        slaveRows.push_back({"LegacyPacketHex", stylusOutput.packet.valid ? FormatCsvPacketBytes(stylusOutput.packet.bytes) : "N/A"});
#endif
        ApplyDynamicRows(slaveRows, Ipc::DebugDvrTarget::SlaveSuffix, dynamicSchema, dynamicFrame);
        WriteCsvKeyValueSection(out, "Slave Status", slaveRows);
    }

    if (includeMasterStatus) {
        out << "--- Master Frame Suffix (128 words) ---\n";
        if (frame.masterSuffixValid) {
            for (int i = 0; i < Frame::kMasterSuffixWords; ++i) {
                out << frame.masterSuffix.words[i];
                if (i < Frame::kMasterSuffixWords - 1) {
                    out << (((i + 1) % 16 == 0) ? "\n" : ",");
                }
            }
            out << "\n";
        } else {
            out << "Data unavailable\n";
        }
    }

    if (includeSlaveStatus) {
        out << "\n--- Slave Frame Suffix (166 words) ---\n";
        if (frame.slaveSuffixValid) {
            for (int i = 0; i < Frame::kSlaveSuffixWords; ++i) {
                out << frame.slaveSuffix.words[i];
                if (i < Frame::kSlaveSuffixWords - 1) {
                    out << (((i + 1) % 16 == 0) ? "\n" : ",");
                }
            }
            out << "\n";
        } else {
            out << "Data unavailable\n";
        }
    }

    std::vector<CsvKeyValueRow> dynamicRows;
    ApplyDynamicRows(dynamicRows, Ipc::DebugDvrTarget::DynamicDebug, dynamicSchema, dynamicFrame);
    if (!dynamicRows.empty()) {
        out << "\n";
        WriteCsvKeyValueSection(out, "Dynamic Debug", dynamicRows);
    }

    return static_cast<bool>(out);
}

bool ServiceProxy::ExportLoadedDvrDatasetToCsv(const std::filesystem::path& outputDirectory,
                                               std::string* outError) const {
    DvrPlaybackDataset dataset;
    {
        std::lock_guard<std::mutex> lk(m_playbackMutex);
        if (m_playbackDataset.Empty()) {
            if (outError) *outError = "No playback dataset loaded.";
            return false;
        }
        dataset = m_playbackDataset;
    }

    std::error_code ec;
    std::filesystem::create_directories(outputDirectory, ec);
    if (ec) {
        if (outError) *outError = "Failed to create CSV directory.";
        return false;
    }

    for (size_t i = 0; i < dataset.frames.size(); ++i) {
        std::ostringstream name;
        name << "frame_" << std::setfill('0') << std::setw(4) << i << ".csv";
        const std::filesystem::path path = outputDirectory / name.str();
        const bool ok = WriteFrameCsvFile(path,
                                          dataset.frames[i].frame,
                                          &m_pipeline,
                                          true,
                                          true,
                                          true,
                                          "PlaybackDataset",
                                          true,
                                          dataset.formatVersion,
                                          outputDirectory.filename().string(),
                                          &dataset.dynamicDebugSchema,
                                          &dataset.frames[i].dynamicDebug);
        if (!ok) {
            if (outError) *outError = "Failed to write CSV frame file.";
            return false;
        }
    }

    std::ofstream manifest((outputDirectory / "dataset_manifest.csv").string());
    if (!manifest.is_open()) {
        if (outError) *outError = "Failed to write dataset manifest.";
        return false;
    }
    const char* timingModeText = "SyntheticFrameIndex";
    switch (dataset.timingMode) {
    case PlaybackTimingMode::HostReceiveEpochUs:
        timingModeText = "HostReceiveEpochUs";
        break;
    case PlaybackTimingMode::LegacyServiceTimestamp:
        timingModeText = "LegacyServiceTimestamp";
        break;
    case PlaybackTimingMode::SyntheticFrameIndex:
    default:
        timingModeText = "SyntheticFrameIndex";
        break;
    }
    manifest << "FrameCount," << dataset.frames.size() << "\n";
    manifest << "DvrFormatVersion," << dataset.formatVersion << "\n";
    manifest << "DvrFlags," << dataset.flags << "\n";
    manifest << "DatasetKind,DVR2\n";
    manifest << "PlaybackTimingMode," << timingModeText << "\n";
    manifest << "PlaybackFirstTimeUs," << dataset.frames.front().recordingTimeUs << "\n";
    manifest << "PlaybackLastTimeUs," << dataset.frames.back().recordingTimeUs << "\n";
    manifest << "ServiceFirstTimestampRaw," << dataset.frames.front().sourceTimeUs << "\n";
    manifest << "ServiceLastTimestampRaw," << dataset.frames.back().sourceTimeUs << "\n";
    manifest << "HostReceiveFirstEpochUs," << dataset.frames.front().hostReceiveUnixTimeUs << "\n";
    manifest << "HostReceiveLastEpochUs," << dataset.frames.back().hostReceiveUnixTimeUs << "\n";
    return true;
}

void ServiceProxy::TriggerDvrBinaryExport() {
    if (!m_dvrBuffer) return;
    if (m_dvrExporting.load()) return;

    if (m_dvrThread.joinable()) m_dvrThread.join();

    const uint64_t triggerSeq = m_dvrSeqCounter.load(std::memory_order_relaxed);
    m_dvrExporting.store(true);
    m_dvrThread = std::thread([this, triggerSeq]() {
        auto frames = m_dvrBuffer->GetSnapshot();
        if (frames.empty()) {
            m_dvrExporting.store(false);
            return;
        }

        namespace fs = std::filesystem;
        const fs::path dir = MakeDvrExportRoot();
        const std::string datasetName = MakeDvrDatasetName();
        std::error_code ec;
        fs::create_directories(dir, ec);
        if (ec) {
            LOG_ERROR("App", "TriggerDvrBinaryExport", "IPC", "Failed to create export directory: {}", dir.string());
            m_dvrExporting.store(false);
            return;
        }

        std::vector<Dvr::DvrFrameSlot> preTriggerFrames;
        preTriggerFrames.reserve(frames.size());
        for (const auto& fr : frames) {
            if (fr.dvrSeq <= triggerSeq) {
                preTriggerFrames.push_back(fr);
            }
        }
        if (preTriggerFrames.empty()) {
            LOG_WARN("App", "TriggerDvrBinaryExport", "IPC", "No pre-trigger frames available for export (triggerSeq={}).", triggerSeq);
            m_dvrExporting.store(false);
            return;
        }
        if (preTriggerFrames.size() > kDvrPreTriggerFrames) {
            const size_t dropCount = preTriggerFrames.size() - kDvrPreTriggerFrames;
            preTriggerFrames.erase(preTriggerFrames.begin(), preTriggerFrames.begin() + static_cast<long long>(dropCount));
        }

        const fs::path replayBinPath = dir / (datasetName + ".dvrbin");
        const auto dynamicSchema = GetCurrentDvrDynamicDebugSchema();
        if (!WriteDvrBinaryFile(replayBinPath, preTriggerFrames, &dynamicSchema, nullptr)) {
            LOG_ERROR("App", "TriggerDvrBinaryExport", "IPC", "Failed to write DVR2 dataset: {}", replayBinPath.string());
            m_dvrExporting.store(false);
            return;
        }

        LOG_INFO("App", "TriggerDvrBinaryExport", "IPC",
                 "Exported {} pre-trigger frames to {} (triggerSeq={})",
                 preTriggerFrames.size(), replayBinPath.string(), triggerSeq);
        m_dvrExporting.store(false);
    });
}

} // namespace App
