#pragma once

#include "FrameLayout.h"
#include "IpcProtocol.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>
#include <vector>

namespace Dvr::Format {

constexpr int kCurrentDvrFormatVersion = 6;
constexpr int kMaxContacts = 10;
constexpr int kMaxPeaks = 30;

constexpr std::array<char, 8> kDvr2Magic{'E', 'G', 'O', 'D', 'V', 'R', '2', '\0'};
constexpr std::array<char, 8> kLegacyDvrMagic{'E', 'G', 'O', 'D', 'V', 'R', 'B', '1'};

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
    FrameSchema = 6,
};

enum class Dvr2ValueType : uint8_t {
    UInt8 = 0,
    UInt16 = 1,
    UInt32 = 2,
    UInt64 = 3,
    Int16 = 4,
    Int32 = 5,
    Float32 = 6,
    Bool = 7,
    Bytes = 8,
};

enum class Dvr2FieldRank : uint8_t {
    Scalar = 0,
    Array = 1,
    Matrix = 2,
    StructArray = 3,
};

enum class Dvr2FieldGroup : uint8_t {
    Frame = 0,
    Heatmap = 1,
    MasterSuffix = 2,
    SlaveSuffix = 3,
    Stylus = 4,
    Contacts = 5,
    Peaks = 6,
    Raw = 7,
};

enum Dvr2FieldFlags : uint8_t {
    kDvrFieldRequired = 1u << 0,
    kDvrFieldCsvExport = 1u << 1,
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

struct Dvr2MetaSection {
    uint32_t frameCount = 0;
    uint32_t flags = 0;
    uint32_t frameRecordSize = 0;
    uint32_t frameSchemaHash = 0;
    uint16_t txCount = Frame::kTxCount;
    uint16_t rxCount = Frame::kRxCount;
    uint16_t masterSuffixWords = Frame::kMasterSuffixWords;
    uint16_t slaveSuffixWords = Frame::kSlaveSuffixWords;
    uint16_t maxContacts = kMaxContacts;
    uint16_t maxPeaks = kMaxPeaks;
    uint32_t rawFrameSize = Frame::kTotalFrameSize;
    uint32_t reserved[8]{};
};

struct Dvr2FrameSchemaHeader {
    uint32_t schemaHash = 0;
    uint32_t fieldCount = 0;
    uint32_t fieldRecordSize = 0;
    uint32_t frameRecordSize = 0;
    uint32_t reserved[4]{};
};

struct Dvr2FieldDef {
    uint32_t fieldId = 0;
    uint32_t parentFieldId = 0;
    uint32_t offset = 0;
    uint32_t size = 0;
    uint32_t elementSize = 0;
    uint32_t elementCount = 0;
    uint32_t stride = 0;
    uint16_t rows = 0;
    uint16_t cols = 0;
    uint8_t valueType = static_cast<uint8_t>(Dvr2ValueType::Bytes);
    uint8_t rank = static_cast<uint8_t>(Dvr2FieldRank::Scalar);
    uint8_t group = static_cast<uint8_t>(Dvr2FieldGroup::Frame);
    uint8_t flags = 0;
    char path[64]{};
    char displayName[48]{};
    char unit[16]{};
};

struct Dvr2DynamicDebugSchemaHeader {
    uint16_t schemaVersion = 0;
    uint16_t fieldCount = 0;
    uint32_t schemaHash = 0;
    uint32_t recordSize = 0;
    uint32_t reserved = 0;
};

struct Dvr2DynamicDebugValuesHeader {
    uint32_t frameCount = 0;
    uint32_t reserved = 0;
};

struct Dvr2DynamicDebugFrameHeader {
    uint32_t sampleCount = 0;
    uint32_t reserved = 0;
};

struct Dvr2DynamicDebugSample {
    uint16_t fieldId = 0;
    uint8_t valueType = static_cast<uint8_t>(Ipc::DebugValueType::UInt32);
    uint8_t flags = 0;
    uint32_t reserved = 0;
    uint64_t rawValue = 0;
};

struct Dvr2IndexEntry {
    uint64_t timestamp = 0;
    uint64_t receiveSystemEpochUs = 0;
    uint64_t frameOffset = 0;
    uint32_t frameSize = 0;
    uint32_t reserved = 0;
};

struct Dvr2ContactRecord {
    int32_t id = 0;
    float x = 0.0f;
    float y = 0.0f;
    int32_t state = 0;
    int32_t area = 0;
    int32_t signalSum = 0;
};

struct Dvr2PeakRecord {
    int32_t r = 0;
    int32_t c = 0;
    int16_t z = 0;
    uint8_t id = 0;
    uint8_t reserved = 0;
};

struct Dvr2StylusPointRecord {
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

struct Dvr2StylusDataRecord {
    uint8_t slaveValid = 0;
    uint8_t checksumOk = 0;
    uint8_t tx1BlockValid = 0;
    uint8_t tx2BlockValid = 0;
    uint32_t status = 0;
    uint16_t pressure = 0;
    uint16_t btRawPressure[4]{};
    uint16_t signalX = 0;
    uint16_t signalY = 0;
    uint16_t maxRawPeak = 0;
    uint8_t pipelineStage = 0;
    uint8_t reserved[5]{};
    Dvr2StylusPointRecord point{};
};

struct Dvr2FrameCore {
    uint64_t timestamp = 0;
    uint64_t receiveSystemEpochUs = 0;
    uint64_t dvrSeq = 0;
    uint8_t masterWasRead = 1;
    uint8_t masterSuffixValid = 0;
    uint8_t slaveSuffixValid = 0;
    uint8_t reserved0 = 0;
    int16_t heatmapMatrix[Frame::kTxCount][Frame::kRxCount]{};
    uint16_t masterSuffix[Frame::kMasterSuffixWords]{};
    uint16_t slaveSuffix[Frame::kSlaveSuffixWords]{};
    Dvr2StylusDataRecord stylus{};
    Dvr2ContactRecord contacts[kMaxContacts]{};
    uint32_t contactCount = 0;
    Dvr2PeakRecord peaks[kMaxPeaks]{};
    uint32_t peakCount = 0;
};

struct Dvr2FramePayload {
    Dvr2FrameCore frame{};
    uint16_t rawDataLength = 0;
    uint8_t rawData[Frame::kTotalFrameSize]{};
};

static_assert(sizeof(Dvr2FileHeader) == 32);
static_assert(sizeof(Dvr2SectionEntry) == 24);
static_assert(sizeof(Dvr2MetaSection) == 64);
static_assert(sizeof(Dvr2FrameSchemaHeader) == 32);
static_assert(sizeof(Dvr2FieldDef) == 164);
static_assert(sizeof(Dvr2IndexEntry) == 32);
static_assert(sizeof(Dvr2ContactRecord) == 24);
static_assert(sizeof(Dvr2PeakRecord) == 12);
static_assert(sizeof(Dvr2DynamicDebugSchemaHeader) == 16);
static_assert(sizeof(Dvr2DynamicDebugValuesHeader) == 8);
static_assert(sizeof(Dvr2DynamicDebugFrameHeader) == 8);
static_assert(sizeof(Dvr2DynamicDebugSample) == 16);
static_assert(std::is_trivially_copyable_v<Dvr2FramePayload>);
static_assert(std::is_standard_layout_v<Dvr2FramePayload>);
static_assert(offsetof(Dvr2FrameCore, heatmapMatrix) == 28);
static_assert(offsetof(Dvr2FrameCore, masterSuffix) == 4828);
static_assert(offsetof(Dvr2FrameCore, slaveSuffix) == 5084);
static_assert(offsetof(Dvr2FrameCore, stylus) == 5416);
static_assert(offsetof(Dvr2FrameCore, contacts) == 5496);
static_assert(offsetof(Dvr2FrameCore, peaks) == 5740);
static_assert(offsetof(Dvr2FramePayload, rawDataLength) == 6104);
static_assert(offsetof(Dvr2FramePayload, rawData) == 6106);
static_assert(sizeof(Dvr2FramePayload) == 11512);

inline void CopyFixedString(char* dst, size_t dstSize, std::string_view src) {
    if (dstSize == 0) return;
    const size_t n = std::min(dstSize - 1, src.size());
    if (n != 0) {
        std::memcpy(dst, src.data(), n);
    }
    dst[n] = '\0';
}

inline uint32_t HashBytes(uint32_t h, const void* data, size_t bytes) {
    const auto* p = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < bytes; ++i) {
        h ^= p[i];
        h *= 16777619u;
    }
    return h;
}

inline uint32_t ComputeFieldSchemaHash(const std::vector<Dvr2FieldDef>& fields) {
    uint32_t h = 2166136261u;
    for (const auto& field : fields) {
        h = HashBytes(h, &field, sizeof(field));
    }
    return h;
}

inline Dvr2FieldDef MakeField(uint32_t fieldId,
                              uint32_t offset,
                              uint32_t size,
                              Dvr2ValueType valueType,
                              Dvr2FieldRank rank,
                              Dvr2FieldGroup group,
                              std::string_view path,
                              std::string_view displayName,
                              uint32_t elementSize = 0,
                              uint32_t elementCount = 1,
                              uint32_t stride = 0,
                              uint16_t rows = 0,
                              uint16_t cols = 0,
                              std::string_view unit = {},
                              uint32_t parentFieldId = 0,
                              uint8_t flags = static_cast<uint8_t>(kDvrFieldRequired | kDvrFieldCsvExport)) {
    Dvr2FieldDef field{};
    field.fieldId = fieldId;
    field.parentFieldId = parentFieldId;
    field.offset = offset;
    field.size = size;
    field.elementSize = elementSize == 0 ? size : elementSize;
    field.elementCount = elementCount;
    field.stride = stride == 0 ? field.elementSize : stride;
    field.rows = rows;
    field.cols = cols;
    field.valueType = static_cast<uint8_t>(valueType);
    field.rank = static_cast<uint8_t>(rank);
    field.group = static_cast<uint8_t>(group);
    field.flags = flags;
    CopyFixedString(field.path, sizeof(field.path), path);
    CopyFixedString(field.displayName, sizeof(field.displayName), displayName);
    CopyFixedString(field.unit, sizeof(field.unit), unit);
    return field;
}

inline std::vector<Dvr2FieldDef> BuildFrameSchema() {
    constexpr uint32_t core = static_cast<uint32_t>(offsetof(Dvr2FramePayload, frame));
    constexpr uint32_t stylus = core + static_cast<uint32_t>(offsetof(Dvr2FrameCore, stylus));
    constexpr uint32_t stylusPoint = stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, point));
    constexpr uint32_t contacts = core + static_cast<uint32_t>(offsetof(Dvr2FrameCore, contacts));
    constexpr uint32_t peaks = core + static_cast<uint32_t>(offsetof(Dvr2FrameCore, peaks));

    std::vector<Dvr2FieldDef> fields;
    fields.reserve(54);
    uint32_t id = 1;

    fields.push_back(MakeField(id++, core + static_cast<uint32_t>(offsetof(Dvr2FrameCore, timestamp)), sizeof(uint64_t), Dvr2ValueType::UInt64, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Frame, "timestamp", "Timestamp"));
    fields.push_back(MakeField(id++, core + static_cast<uint32_t>(offsetof(Dvr2FrameCore, receiveSystemEpochUs)), sizeof(uint64_t), Dvr2ValueType::UInt64, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Frame, "receiveSystemEpochUs", "Host Receive Epoch Us", 0, 1, 0, 0, 0, "us"));
    fields.push_back(MakeField(id++, core + static_cast<uint32_t>(offsetof(Dvr2FrameCore, dvrSeq)), sizeof(uint64_t), Dvr2ValueType::UInt64, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Frame, "dvrSeq", "DVR Sequence"));
    fields.push_back(MakeField(id++, core + static_cast<uint32_t>(offsetof(Dvr2FrameCore, masterWasRead)), sizeof(uint8_t), Dvr2ValueType::Bool, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Frame, "masterWasRead", "Master Was Read"));
    fields.push_back(MakeField(id++, core + static_cast<uint32_t>(offsetof(Dvr2FrameCore, masterSuffixValid)), sizeof(uint8_t), Dvr2ValueType::Bool, Dvr2FieldRank::Scalar, Dvr2FieldGroup::MasterSuffix, "masterSuffixValid", "Master Suffix Valid"));
    fields.push_back(MakeField(id++, core + static_cast<uint32_t>(offsetof(Dvr2FrameCore, slaveSuffixValid)), sizeof(uint8_t), Dvr2ValueType::Bool, Dvr2FieldRank::Scalar, Dvr2FieldGroup::SlaveSuffix, "slaveSuffixValid", "Slave Suffix Valid"));
    fields.push_back(MakeField(id++, core + static_cast<uint32_t>(offsetof(Dvr2FrameCore, heatmapMatrix)), sizeof(Dvr2FrameCore::heatmapMatrix), Dvr2ValueType::Int16, Dvr2FieldRank::Matrix, Dvr2FieldGroup::Heatmap, "heatmapMatrix", "Heatmap Matrix", sizeof(int16_t), Frame::kMatrixCells, sizeof(int16_t), Frame::kTxCount, Frame::kRxCount));
    fields.push_back(MakeField(id++, core + static_cast<uint32_t>(offsetof(Dvr2FrameCore, masterSuffix)), sizeof(Dvr2FrameCore::masterSuffix), Dvr2ValueType::UInt16, Dvr2FieldRank::Array, Dvr2FieldGroup::MasterSuffix, "masterSuffix.words", "Master Suffix Words", sizeof(uint16_t), Frame::kMasterSuffixWords));
    fields.push_back(MakeField(id++, core + static_cast<uint32_t>(offsetof(Dvr2FrameCore, slaveSuffix)), sizeof(Dvr2FrameCore::slaveSuffix), Dvr2ValueType::UInt16, Dvr2FieldRank::Array, Dvr2FieldGroup::SlaveSuffix, "slaveSuffix.words", "Slave Suffix Words", sizeof(uint16_t), Frame::kSlaveSuffixWords));

    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, slaveValid)), sizeof(uint8_t), Dvr2ValueType::Bool, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.slaveValid", "Stylus Slave Valid"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, checksumOk)), sizeof(uint8_t), Dvr2ValueType::Bool, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.checksumOk", "Stylus Checksum OK"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, tx1BlockValid)), sizeof(uint8_t), Dvr2ValueType::Bool, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.tx1BlockValid", "Stylus TX1 Block Valid"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, tx2BlockValid)), sizeof(uint8_t), Dvr2ValueType::Bool, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.tx2BlockValid", "Stylus TX2 Block Valid"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, status)), sizeof(uint32_t), Dvr2ValueType::UInt32, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.status", "Stylus Status"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, pressure)), sizeof(uint16_t), Dvr2ValueType::UInt16, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.pressure", "Stylus Pressure"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, btRawPressure)), sizeof(Dvr2StylusDataRecord::btRawPressure), Dvr2ValueType::UInt16, Dvr2FieldRank::Array, Dvr2FieldGroup::Stylus, "stylus.btRawPressure", "BT Raw Pressure", sizeof(uint16_t), 4));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, signalX)), sizeof(uint16_t), Dvr2ValueType::UInt16, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.signalX", "Stylus Signal X"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, signalY)), sizeof(uint16_t), Dvr2ValueType::UInt16, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.signalY", "Stylus Signal Y"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, maxRawPeak)), sizeof(uint16_t), Dvr2ValueType::UInt16, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.maxRawPeak", "Stylus Max Raw Peak"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, pipelineStage)), sizeof(uint8_t), Dvr2ValueType::UInt8, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.pipelineStage", "Stylus Pipeline Stage"));

    fields.push_back(MakeField(id++, stylusPoint + static_cast<uint32_t>(offsetof(Dvr2StylusPointRecord, valid)), sizeof(uint8_t), Dvr2ValueType::Bool, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.point.valid", "Stylus Point Valid"));
    fields.push_back(MakeField(id++, stylusPoint + static_cast<uint32_t>(offsetof(Dvr2StylusPointRecord, x)), sizeof(float), Dvr2ValueType::Float32, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.point.x", "Stylus Point X"));
    fields.push_back(MakeField(id++, stylusPoint + static_cast<uint32_t>(offsetof(Dvr2StylusPointRecord, y)), sizeof(float), Dvr2ValueType::Float32, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.point.y", "Stylus Point Y"));
    fields.push_back(MakeField(id++, stylusPoint + static_cast<uint32_t>(offsetof(Dvr2StylusPointRecord, reportX)), sizeof(uint16_t), Dvr2ValueType::UInt16, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.point.reportX", "Stylus Report X"));
    fields.push_back(MakeField(id++, stylusPoint + static_cast<uint32_t>(offsetof(Dvr2StylusPointRecord, reportY)), sizeof(uint16_t), Dvr2ValueType::UInt16, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.point.reportY", "Stylus Report Y"));
    fields.push_back(MakeField(id++, stylusPoint + static_cast<uint32_t>(offsetof(Dvr2StylusPointRecord, pressure)), sizeof(uint16_t), Dvr2ValueType::UInt16, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.point.pressure", "Stylus Point Pressure"));
    fields.push_back(MakeField(id++, stylusPoint + static_cast<uint32_t>(offsetof(Dvr2StylusPointRecord, rawPressure)), sizeof(uint16_t), Dvr2ValueType::UInt16, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.point.rawPressure", "Stylus Raw Pressure"));
    fields.push_back(MakeField(id++, stylusPoint + static_cast<uint32_t>(offsetof(Dvr2StylusPointRecord, mappedPressure)), sizeof(uint16_t), Dvr2ValueType::UInt16, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.point.mappedPressure", "Stylus Mapped Pressure"));
    fields.push_back(MakeField(id++, stylusPoint + static_cast<uint32_t>(offsetof(Dvr2StylusPointRecord, peakTx1)), sizeof(uint16_t), Dvr2ValueType::UInt16, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.point.peakTx1", "Stylus Peak TX1"));
    fields.push_back(MakeField(id++, stylusPoint + static_cast<uint32_t>(offsetof(Dvr2StylusPointRecord, peakTx2)), sizeof(uint16_t), Dvr2ValueType::UInt16, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.point.peakTx2", "Stylus Peak TX2"));
    fields.push_back(MakeField(id++, stylusPoint + static_cast<uint32_t>(offsetof(Dvr2StylusPointRecord, tx1X)), sizeof(float), Dvr2ValueType::Float32, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.point.tx1X", "Stylus TX1 X"));
    fields.push_back(MakeField(id++, stylusPoint + static_cast<uint32_t>(offsetof(Dvr2StylusPointRecord, tx1Y)), sizeof(float), Dvr2ValueType::Float32, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.point.tx1Y", "Stylus TX1 Y"));
    fields.push_back(MakeField(id++, stylusPoint + static_cast<uint32_t>(offsetof(Dvr2StylusPointRecord, tx2X)), sizeof(float), Dvr2ValueType::Float32, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.point.tx2X", "Stylus TX2 X"));
    fields.push_back(MakeField(id++, stylusPoint + static_cast<uint32_t>(offsetof(Dvr2StylusPointRecord, tx2Y)), sizeof(float), Dvr2ValueType::Float32, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.point.tx2Y", "Stylus TX2 Y"));
    fields.push_back(MakeField(id++, stylusPoint + static_cast<uint32_t>(offsetof(Dvr2StylusPointRecord, confidence)), sizeof(float), Dvr2ValueType::Float32, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.point.confidence", "Stylus Confidence"));

    const uint32_t contactsParentId = id;
    fields.push_back(MakeField(id++, contacts, sizeof(Dvr2FrameCore::contacts), Dvr2ValueType::Bytes, Dvr2FieldRank::StructArray, Dvr2FieldGroup::Contacts, "contacts[]", "Contacts", sizeof(Dvr2ContactRecord), kMaxContacts, sizeof(Dvr2ContactRecord)));
    fields.push_back(MakeField(id++, contacts + static_cast<uint32_t>(offsetof(Dvr2ContactRecord, id)), sizeof(int32_t), Dvr2ValueType::Int32, Dvr2FieldRank::Array, Dvr2FieldGroup::Contacts, "contacts[].id", "Contact ID", sizeof(int32_t), kMaxContacts, sizeof(Dvr2ContactRecord), 0, 0, {}, contactsParentId));
    fields.push_back(MakeField(id++, contacts + static_cast<uint32_t>(offsetof(Dvr2ContactRecord, x)), sizeof(float), Dvr2ValueType::Float32, Dvr2FieldRank::Array, Dvr2FieldGroup::Contacts, "contacts[].x", "Contact X", sizeof(float), kMaxContacts, sizeof(Dvr2ContactRecord), 0, 0, {}, contactsParentId));
    fields.push_back(MakeField(id++, contacts + static_cast<uint32_t>(offsetof(Dvr2ContactRecord, y)), sizeof(float), Dvr2ValueType::Float32, Dvr2FieldRank::Array, Dvr2FieldGroup::Contacts, "contacts[].y", "Contact Y", sizeof(float), kMaxContacts, sizeof(Dvr2ContactRecord), 0, 0, {}, contactsParentId));
    fields.push_back(MakeField(id++, contacts + static_cast<uint32_t>(offsetof(Dvr2ContactRecord, state)), sizeof(int32_t), Dvr2ValueType::Int32, Dvr2FieldRank::Array, Dvr2FieldGroup::Contacts, "contacts[].state", "Contact State", sizeof(int32_t), kMaxContacts, sizeof(Dvr2ContactRecord), 0, 0, {}, contactsParentId));
    fields.push_back(MakeField(id++, contacts + static_cast<uint32_t>(offsetof(Dvr2ContactRecord, area)), sizeof(int32_t), Dvr2ValueType::Int32, Dvr2FieldRank::Array, Dvr2FieldGroup::Contacts, "contacts[].area", "Contact Area", sizeof(int32_t), kMaxContacts, sizeof(Dvr2ContactRecord), 0, 0, {}, contactsParentId));
    fields.push_back(MakeField(id++, contacts + static_cast<uint32_t>(offsetof(Dvr2ContactRecord, signalSum)), sizeof(int32_t), Dvr2ValueType::Int32, Dvr2FieldRank::Array, Dvr2FieldGroup::Contacts, "contacts[].signalSum", "Contact Signal Sum", sizeof(int32_t), kMaxContacts, sizeof(Dvr2ContactRecord), 0, 0, {}, contactsParentId));
    fields.push_back(MakeField(id++, core + static_cast<uint32_t>(offsetof(Dvr2FrameCore, contactCount)), sizeof(uint32_t), Dvr2ValueType::UInt32, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Contacts, "contactCount", "Contact Count"));

    const uint32_t peaksParentId = id;
    fields.push_back(MakeField(id++, peaks, sizeof(Dvr2FrameCore::peaks), Dvr2ValueType::Bytes, Dvr2FieldRank::StructArray, Dvr2FieldGroup::Peaks, "peaks[]", "Peaks", sizeof(Dvr2PeakRecord), kMaxPeaks, sizeof(Dvr2PeakRecord)));
    fields.push_back(MakeField(id++, peaks + static_cast<uint32_t>(offsetof(Dvr2PeakRecord, r)), sizeof(int32_t), Dvr2ValueType::Int32, Dvr2FieldRank::Array, Dvr2FieldGroup::Peaks, "peaks[].r", "Peak Row", sizeof(int32_t), kMaxPeaks, sizeof(Dvr2PeakRecord), 0, 0, {}, peaksParentId));
    fields.push_back(MakeField(id++, peaks + static_cast<uint32_t>(offsetof(Dvr2PeakRecord, c)), sizeof(int32_t), Dvr2ValueType::Int32, Dvr2FieldRank::Array, Dvr2FieldGroup::Peaks, "peaks[].c", "Peak Column", sizeof(int32_t), kMaxPeaks, sizeof(Dvr2PeakRecord), 0, 0, {}, peaksParentId));
    fields.push_back(MakeField(id++, peaks + static_cast<uint32_t>(offsetof(Dvr2PeakRecord, z)), sizeof(int16_t), Dvr2ValueType::Int16, Dvr2FieldRank::Array, Dvr2FieldGroup::Peaks, "peaks[].z", "Peak Z", sizeof(int16_t), kMaxPeaks, sizeof(Dvr2PeakRecord), 0, 0, {}, peaksParentId));
    fields.push_back(MakeField(id++, peaks + static_cast<uint32_t>(offsetof(Dvr2PeakRecord, id)), sizeof(uint8_t), Dvr2ValueType::UInt8, Dvr2FieldRank::Array, Dvr2FieldGroup::Peaks, "peaks[].id", "Peak ID", sizeof(uint8_t), kMaxPeaks, sizeof(Dvr2PeakRecord), 0, 0, {}, peaksParentId));
    fields.push_back(MakeField(id++, core + static_cast<uint32_t>(offsetof(Dvr2FrameCore, peakCount)), sizeof(uint32_t), Dvr2ValueType::UInt32, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Peaks, "peakCount", "Peak Count"));
    fields.push_back(MakeField(id++, static_cast<uint32_t>(offsetof(Dvr2FramePayload, rawDataLength)), sizeof(uint16_t), Dvr2ValueType::UInt16, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Raw, "rawDataLength", "Raw Data Length"));
    fields.push_back(MakeField(id++, static_cast<uint32_t>(offsetof(Dvr2FramePayload, rawData)), sizeof(Dvr2FramePayload::rawData), Dvr2ValueType::UInt8, Dvr2FieldRank::Array, Dvr2FieldGroup::Raw, "rawData", "Raw SPI Data", sizeof(uint8_t), Frame::kTotalFrameSize));

    return fields;
}

inline const Dvr2FieldDef* FindField(const std::vector<Dvr2FieldDef>& fields, std::string_view path) {
    for (const auto& field : fields) {
        const auto* end = std::find(field.path, field.path + sizeof(field.path), '\0');
        if (std::string_view(field.path, static_cast<size_t>(end - field.path)) == path) {
            return &field;
        }
    }
    return nullptr;
}

} // namespace Dvr::Format
