#pragma once

#include "FrameLayout.h"
#include "Ipc/IpcProtocol.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>
#include <vector>

namespace Dvr::Format {

constexpr int kCurrentDvrFormatVersion = 7;
constexpr int kMaxContacts = 10;
constexpr int kMaxPeaks = 30;
constexpr int kTouchPacketCount = 2;
constexpr int kStylusRawGridDim = 9;
constexpr int kStylusRawGridCells = kStylusRawGridDim * kStylusRawGridDim;

constexpr std::array<char, 8> kDvr2Magic{'E', 'G', 'O', 'D', 'V', 'R', '2', '\0'};
constexpr std::array<char, 8> kLegacyDvrMagic{'E', 'G', 'O', 'D', 'V', 'R', 'B', '1'};

enum DvrBinaryFlags : uint32_t {
    kDvrFlagHasStylusDiagnostics    = 1u << 0,
    kDvrFlagHasStructuredSuffix     = 1u << 1,
    kDvrFlagHasReceiveSystemEpochUs = 1u << 2,
    kDvrFlagHasDynamicDebug         = 1u << 3,
    kDvrFlagHasRuntimeConfig        = 1u << 4,
};

enum class Dvr2SectionType : uint32_t {
    Meta = 1,
    Index = 2,
    Frames = 3,
    DynamicDebugSchema = 4,
    DynamicDebugValues = 5,
    FrameSchema = 6,
    RuntimeConfigSchema = 7,
    RuntimeConfigValues = 8,
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

enum class Dvr2ConfigValueType : uint8_t {
    Bool = 0,
    Int32 = 1,
    UInt8 = 2,
    UInt16 = 3,
    Float32 = 4,
    Float64 = 5,
    String = 6,
    UInt32 = 7,
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
    TouchPackets = 8,
    Diagnostics = 9,
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

struct Dvr2RuntimeConfigSchemaHeader {
    uint16_t schemaVersion = 1;
    uint16_t fieldCount = 0;
    uint32_t schemaHash = 0;
    uint32_t recordSize = 0;
    uint32_t reserved = 0;
};

struct Dvr2RuntimeConfigFieldDef {
    uint32_t fieldId = 0;
    uint8_t valueType = static_cast<uint8_t>(Dvr2ConfigValueType::String);
    uint8_t category = 0;
    uint16_t flags = 0;
    float minValue = 0.0f;
    float maxValue = 0.0f;
    char section[32]{};
    char key[64]{};
    char displayName[64]{};
    char moduleTag[32]{};
    char unit[16]{};
};

struct Dvr2RuntimeConfigValuesHeader {
    uint16_t valueCount = 0;
    uint16_t recordSize = 0;
    uint32_t schemaHash = 0;
    uint32_t reserved[2]{};
};

struct Dvr2RuntimeConfigValueRecord {
    uint32_t fieldId = 0;
    uint8_t valueType = static_cast<uint8_t>(Dvr2ConfigValueType::String);
    uint8_t flags = 0;
    uint16_t stringLength = 0;
    uint64_t rawValue = 0;
    char stringValue[128]{};
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
    float sizeMm = 0.0f;
    float edgeDistX = 0.0f;
    float edgeDistY = 0.0f;
    float rawXBeforeEC = 0.0f;
    float rawYBeforeEC = 0.0f;
    int32_t prevIndex = -1;
    int32_t debugFlags = 0;
    uint32_t edgeFlags = 0;
    uint32_t ecFlags = 0;
    uint32_t lifeFlags = 0;
    uint32_t reportFlags = 0;
    int32_t reportEvent = 0;
    uint8_t isEdge = 0;
    uint8_t isReported = 1;
    uint8_t centroidEdgeFlags = 0;
    uint8_t ecWidthX = 0;
    uint8_t ecWidthY = 0;
    uint8_t reserved[3]{};
};

struct Dvr2PeakRecord {
    int32_t r = 0;
    int32_t c = 0;
    int16_t z = 0;
    uint8_t id = 0;
    uint8_t reserved = 0;
};

struct Dvr2TouchPacketRecord {
    uint8_t valid = 0;
    uint8_t reportId = 0x01;
    uint8_t length = 0x20;
    uint8_t reserved = 0;
    uint8_t bytes[32]{};
};

struct Dvr2StylusPacketRecord {
    uint8_t valid = 0;
    uint8_t reportId = 0x08;
    uint8_t length = 17;
    uint8_t reserved = 0;
    uint8_t bytes[17]{};
    uint8_t reservedTail[3]{};
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
    uint8_t tiltValid = 0;
    uint8_t reserved1 = 0;
    int16_t preTiltX = 0;
    int16_t preTiltY = 0;
    int16_t tiltX = 0;
    int16_t tiltY = 0;
    float tiltMagnitude = 0.0f;
    float tiltAzimuthDeg = 0.0f;
    float tx1X = 0.0f;
    float tx1Y = 0.0f;
    float tx2X = 0.0f;
    float tx2Y = 0.0f;
    float confidence = 0.0f;
};

struct Dvr2StylusRawGridBlockRecord {
    uint16_t anchorRow = 0x00FF;
    uint16_t anchorCol = 0x00FF;
    int16_t grid[kStylusRawGridDim][kStylusRawGridDim]{};
    uint8_t valid = 0;
    uint8_t reserved = 0;
};

struct Dvr2StylusRawGridRecord {
    Dvr2StylusRawGridBlockRecord tx1{};
    Dvr2StylusRawGridBlockRecord tx2{};
};

struct Dvr2StylusDataRecord {
    uint8_t slaveValid = 0;
    uint8_t checksumOk = 0;
    uint8_t slaveWordOffset = 0;
    uint8_t tx1BlockValid = 0;
    uint8_t tx2BlockValid = 0;
    uint8_t outputValid = 0;
    uint8_t inRange = 0;
    uint8_t tipDown = 0;
    uint32_t status = 0;
    uint16_t checksum16 = 0;
    uint16_t pressure = 0;
    uint16_t btPressure[4]{};
    uint16_t btRawPressure[4]{};
    uint32_t btSeq = 0;
    uint8_t btFreq1 = 0;
    uint8_t btFreq2 = 0;
    uint8_t btHasSample = 0;
    uint8_t btHasFreq = 0;
    uint16_t signalX = 0;
    uint16_t signalY = 0;
    uint16_t maxRawPeak = 0;
    uint8_t pipelineStage = 0;
    uint8_t recheckEnabled = 0;
    uint8_t recheckPassed = 1;
    uint8_t recheckOverlap = 0;
    uint16_t recheckThreshold = 0;
    uint16_t recheckThresholdMulti = 0;
    uint8_t touchNullLike = 0;
    uint8_t touchSuppressActive = 0;
    uint8_t touchSuppressFrames = 0;
    uint8_t pressureIsReal = 0;
    uint8_t predictedAgeFrames = 0;
    uint8_t reserved0 = 0;
    float outputConfidence = 0.0f;
    Dvr2StylusPacketRecord packet{};
    Dvr2StylusPointRecord point{};
    Dvr2StylusRawGridRecord rawGrid{};
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
    Dvr2TouchPacketRecord touchPackets[kTouchPacketCount]{};
    uint8_t touchZones[Frame::kMatrixCells]{};
    uint8_t peakZones[Frame::kMatrixCells]{};
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
static_assert(sizeof(Dvr2ContactRecord) == 80);
static_assert(sizeof(Dvr2PeakRecord) == 12);
static_assert(sizeof(Dvr2TouchPacketRecord) == 36);
static_assert(sizeof(Dvr2StylusPacketRecord) == 24);
static_assert(sizeof(Dvr2StylusPointRecord) == 64);
static_assert(sizeof(Dvr2StylusRawGridBlockRecord) == 168);
static_assert(sizeof(Dvr2StylusRawGridRecord) == 336);
static_assert(sizeof(Dvr2StylusDataRecord) == 488);
static_assert(sizeof(Dvr2DynamicDebugSchemaHeader) == 16);
static_assert(sizeof(Dvr2DynamicDebugValuesHeader) == 8);
static_assert(sizeof(Dvr2DynamicDebugFrameHeader) == 8);
static_assert(sizeof(Dvr2DynamicDebugSample) == 16);
static_assert(sizeof(Dvr2RuntimeConfigSchemaHeader) == 16);
static_assert(sizeof(Dvr2RuntimeConfigFieldDef) == 224);
static_assert(sizeof(Dvr2RuntimeConfigValuesHeader) == 16);
static_assert(sizeof(Dvr2RuntimeConfigValueRecord) == 144);
static_assert(std::is_trivially_copyable_v<Dvr2FramePayload>);
static_assert(std::is_standard_layout_v<Dvr2FramePayload>);
static_assert(offsetof(Dvr2FrameCore, heatmapMatrix) == 28);
static_assert(offsetof(Dvr2FrameCore, masterSuffix) == 4828);
static_assert(offsetof(Dvr2FrameCore, slaveSuffix) == 5084);
static_assert(offsetof(Dvr2FrameCore, touchPackets) == 5416);
static_assert(offsetof(Dvr2FrameCore, touchZones) == 5488);
static_assert(offsetof(Dvr2FrameCore, peakZones) == 7888);
static_assert(offsetof(Dvr2FrameCore, stylus) == 10288);
static_assert(offsetof(Dvr2FrameCore, contacts) == 10776);
static_assert(offsetof(Dvr2FrameCore, peaks) == 11580);
static_assert(offsetof(Dvr2FramePayload, rawDataLength) == 11944);
static_assert(offsetof(Dvr2FramePayload, rawData) == 11946);
static_assert(sizeof(Dvr2FramePayload) == 17352);

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

inline uint32_t ComputeRuntimeConfigSchemaHash(const std::vector<Dvr2RuntimeConfigFieldDef>& fields) {
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
    constexpr uint32_t stylusPacket = stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, packet));
    constexpr uint32_t stylusPoint = stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, point));
    constexpr uint32_t stylusRawGrid = stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, rawGrid));
    constexpr uint32_t stylusRawGridTx1 = stylusRawGrid + static_cast<uint32_t>(offsetof(Dvr2StylusRawGridRecord, tx1));
    constexpr uint32_t stylusRawGridTx2 = stylusRawGrid + static_cast<uint32_t>(offsetof(Dvr2StylusRawGridRecord, tx2));
    constexpr uint32_t contacts = core + static_cast<uint32_t>(offsetof(Dvr2FrameCore, contacts));
    constexpr uint32_t touchPackets = core + static_cast<uint32_t>(offsetof(Dvr2FrameCore, touchPackets));
    constexpr uint32_t peaks = core + static_cast<uint32_t>(offsetof(Dvr2FrameCore, peaks));

    std::vector<Dvr2FieldDef> fields;
    fields.reserve(118);
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

    const uint32_t touchPacketsParentId = id;
    fields.push_back(MakeField(id++, touchPackets, sizeof(Dvr2FrameCore::touchPackets), Dvr2ValueType::Bytes, Dvr2FieldRank::StructArray, Dvr2FieldGroup::TouchPackets, "touchPackets[]", "Touch Packets", sizeof(Dvr2TouchPacketRecord), kTouchPacketCount, sizeof(Dvr2TouchPacketRecord)));
    fields.push_back(MakeField(id++, touchPackets + static_cast<uint32_t>(offsetof(Dvr2TouchPacketRecord, valid)), sizeof(uint8_t), Dvr2ValueType::Bool, Dvr2FieldRank::Array, Dvr2FieldGroup::TouchPackets, "touchPackets[].valid", "Touch Packet Valid", sizeof(uint8_t), kTouchPacketCount, sizeof(Dvr2TouchPacketRecord), 0, 0, {}, touchPacketsParentId));
    fields.push_back(MakeField(id++, touchPackets + static_cast<uint32_t>(offsetof(Dvr2TouchPacketRecord, reportId)), sizeof(uint8_t), Dvr2ValueType::UInt8, Dvr2FieldRank::Array, Dvr2FieldGroup::TouchPackets, "touchPackets[].reportId", "Touch Packet Report ID", sizeof(uint8_t), kTouchPacketCount, sizeof(Dvr2TouchPacketRecord), 0, 0, {}, touchPacketsParentId));
    fields.push_back(MakeField(id++, touchPackets + static_cast<uint32_t>(offsetof(Dvr2TouchPacketRecord, length)), sizeof(uint8_t), Dvr2ValueType::UInt8, Dvr2FieldRank::Array, Dvr2FieldGroup::TouchPackets, "touchPackets[].length", "Touch Packet Length", sizeof(uint8_t), kTouchPacketCount, sizeof(Dvr2TouchPacketRecord), 0, 0, {}, touchPacketsParentId));
    fields.push_back(MakeField(id++, touchPackets + static_cast<uint32_t>(offsetof(Dvr2TouchPacketRecord, bytes)), sizeof(Dvr2TouchPacketRecord::bytes), Dvr2ValueType::UInt8, Dvr2FieldRank::Array, Dvr2FieldGroup::TouchPackets, "touchPackets[].bytes", "Touch Packet Bytes", sizeof(Dvr2TouchPacketRecord::bytes), kTouchPacketCount, sizeof(Dvr2TouchPacketRecord), 0, 0, {}, touchPacketsParentId));

    fields.push_back(MakeField(id++, core + static_cast<uint32_t>(offsetof(Dvr2FrameCore, touchZones)), sizeof(Dvr2FrameCore::touchZones), Dvr2ValueType::UInt8, Dvr2FieldRank::Array, Dvr2FieldGroup::Diagnostics, "touchZones", "Touch Zones", sizeof(uint8_t), Frame::kMatrixCells));
    fields.push_back(MakeField(id++, core + static_cast<uint32_t>(offsetof(Dvr2FrameCore, peakZones)), sizeof(Dvr2FrameCore::peakZones), Dvr2ValueType::UInt8, Dvr2FieldRank::Array, Dvr2FieldGroup::Diagnostics, "peakZones", "Peak Zones", sizeof(uint8_t), Frame::kMatrixCells));

    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, slaveValid)), sizeof(uint8_t), Dvr2ValueType::Bool, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.slaveValid", "Stylus Slave Valid"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, checksumOk)), sizeof(uint8_t), Dvr2ValueType::Bool, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.checksumOk", "Stylus Checksum OK"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, slaveWordOffset)), sizeof(uint8_t), Dvr2ValueType::UInt8, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.slaveWordOffset", "Stylus Slave Word Offset"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, checksum16)), sizeof(uint16_t), Dvr2ValueType::UInt16, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.checksum16", "Stylus Checksum16"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, tx1BlockValid)), sizeof(uint8_t), Dvr2ValueType::Bool, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.tx1BlockValid", "Stylus TX1 Block Valid"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, tx2BlockValid)), sizeof(uint8_t), Dvr2ValueType::Bool, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.tx2BlockValid", "Stylus TX2 Block Valid"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, status)), sizeof(uint32_t), Dvr2ValueType::UInt32, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.status", "Stylus Status"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, pressure)), sizeof(uint16_t), Dvr2ValueType::UInt16, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.pressure", "Stylus Pressure"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, btPressure)), sizeof(Dvr2StylusDataRecord::btPressure), Dvr2ValueType::UInt16, Dvr2FieldRank::Array, Dvr2FieldGroup::Stylus, "stylus.btPressure", "BT Pressure", sizeof(uint16_t), 4));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, btRawPressure)), sizeof(Dvr2StylusDataRecord::btRawPressure), Dvr2ValueType::UInt16, Dvr2FieldRank::Array, Dvr2FieldGroup::Stylus, "stylus.btRawPressure", "BT Raw Pressure", sizeof(uint16_t), 4));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, btSeq)), sizeof(uint32_t), Dvr2ValueType::UInt32, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.btSeq", "BT Seq"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, btFreq1)), sizeof(uint8_t), Dvr2ValueType::UInt8, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.btFreq1", "BT Freq1"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, btFreq2)), sizeof(uint8_t), Dvr2ValueType::UInt8, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.btFreq2", "BT Freq2"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, btHasSample)), sizeof(uint8_t), Dvr2ValueType::Bool, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.btHasSample", "BT Has Sample"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, btHasFreq)), sizeof(uint8_t), Dvr2ValueType::Bool, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.btHasFreq", "BT Has Freq"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, signalX)), sizeof(uint16_t), Dvr2ValueType::UInt16, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.signalX", "Stylus Signal X"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, signalY)), sizeof(uint16_t), Dvr2ValueType::UInt16, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.signalY", "Stylus Signal Y"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, maxRawPeak)), sizeof(uint16_t), Dvr2ValueType::UInt16, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.maxRawPeak", "Stylus Max Raw Peak"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, pipelineStage)), sizeof(uint8_t), Dvr2ValueType::UInt8, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.pipelineStage", "Stylus Pipeline Stage"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, outputValid)), sizeof(uint8_t), Dvr2ValueType::Bool, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.output.valid", "Stylus Output Valid"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, inRange)), sizeof(uint8_t), Dvr2ValueType::Bool, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.output.inRange", "Stylus In Range"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, tipDown)), sizeof(uint8_t), Dvr2ValueType::Bool, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.output.tipDown", "Stylus Tip Down"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, outputConfidence)), sizeof(float), Dvr2ValueType::Float32, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.output.confidence", "Stylus Output Confidence"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, recheckEnabled)), sizeof(uint8_t), Dvr2ValueType::Bool, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.interop.recheckEnabled", "Stylus Recheck Enabled"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, recheckPassed)), sizeof(uint8_t), Dvr2ValueType::Bool, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.interop.recheckPassed", "Stylus Recheck Passed"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, recheckOverlap)), sizeof(uint8_t), Dvr2ValueType::Bool, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.interop.recheckOverlap", "Stylus Recheck Overlap"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, recheckThreshold)), sizeof(uint16_t), Dvr2ValueType::UInt16, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.interop.recheckThreshold", "Stylus Recheck Threshold"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, recheckThresholdMulti)), sizeof(uint16_t), Dvr2ValueType::UInt16, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.interop.recheckThresholdMulti", "Stylus Recheck Threshold Multi"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, touchNullLike)), sizeof(uint8_t), Dvr2ValueType::Bool, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.interop.touchNullLike", "Stylus Touch Null Like"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, touchSuppressActive)), sizeof(uint8_t), Dvr2ValueType::Bool, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.interop.touchSuppressActive", "Stylus Touch Suppress Active"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, touchSuppressFrames)), sizeof(uint8_t), Dvr2ValueType::UInt8, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.interop.touchSuppressFrames", "Stylus Touch Suppress Frames"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, pressureIsReal)), sizeof(uint8_t), Dvr2ValueType::Bool, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.pressureIsReal", "Stylus Pressure Is Real"));
    fields.push_back(MakeField(id++, stylus + static_cast<uint32_t>(offsetof(Dvr2StylusDataRecord, predictedAgeFrames)), sizeof(uint8_t), Dvr2ValueType::UInt8, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.predictedAgeFrames", "Stylus Predicted Age Frames"));

    fields.push_back(MakeField(id++, stylusRawGridTx1 + static_cast<uint32_t>(offsetof(Dvr2StylusRawGridBlockRecord, valid)), sizeof(uint8_t), Dvr2ValueType::Bool, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.runtime.hpp3.rawGrid.grid.tx1.valid", "Stylus Raw TX1 Valid"));
    fields.push_back(MakeField(id++, stylusRawGridTx1 + static_cast<uint32_t>(offsetof(Dvr2StylusRawGridBlockRecord, anchorRow)), sizeof(uint16_t), Dvr2ValueType::UInt16, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.runtime.hpp3.rawGrid.grid.tx1.anchorRow", "Stylus Raw TX1 Anchor Row"));
    fields.push_back(MakeField(id++, stylusRawGridTx1 + static_cast<uint32_t>(offsetof(Dvr2StylusRawGridBlockRecord, anchorCol)), sizeof(uint16_t), Dvr2ValueType::UInt16, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.runtime.hpp3.rawGrid.grid.tx1.anchorCol", "Stylus Raw TX1 Anchor Col"));
    fields.push_back(MakeField(id++, stylusRawGridTx1 + static_cast<uint32_t>(offsetof(Dvr2StylusRawGridBlockRecord, grid)), sizeof(Dvr2StylusRawGridBlockRecord::grid), Dvr2ValueType::Int16, Dvr2FieldRank::Matrix, Dvr2FieldGroup::Stylus, "stylus.runtime.hpp3.rawGrid.grid.tx1.grid", "Stylus Raw TX1 Grid", sizeof(int16_t), kStylusRawGridCells, sizeof(int16_t), kStylusRawGridDim, kStylusRawGridDim));
    fields.push_back(MakeField(id++, stylusRawGridTx2 + static_cast<uint32_t>(offsetof(Dvr2StylusRawGridBlockRecord, valid)), sizeof(uint8_t), Dvr2ValueType::Bool, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.runtime.hpp3.rawGrid.grid.tx2.valid", "Stylus Raw TX2 Valid"));
    fields.push_back(MakeField(id++, stylusRawGridTx2 + static_cast<uint32_t>(offsetof(Dvr2StylusRawGridBlockRecord, anchorRow)), sizeof(uint16_t), Dvr2ValueType::UInt16, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.runtime.hpp3.rawGrid.grid.tx2.anchorRow", "Stylus Raw TX2 Anchor Row"));
    fields.push_back(MakeField(id++, stylusRawGridTx2 + static_cast<uint32_t>(offsetof(Dvr2StylusRawGridBlockRecord, anchorCol)), sizeof(uint16_t), Dvr2ValueType::UInt16, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.runtime.hpp3.rawGrid.grid.tx2.anchorCol", "Stylus Raw TX2 Anchor Col"));
    fields.push_back(MakeField(id++, stylusRawGridTx2 + static_cast<uint32_t>(offsetof(Dvr2StylusRawGridBlockRecord, grid)), sizeof(Dvr2StylusRawGridBlockRecord::grid), Dvr2ValueType::Int16, Dvr2FieldRank::Matrix, Dvr2FieldGroup::Stylus, "stylus.runtime.hpp3.rawGrid.grid.tx2.grid", "Stylus Raw TX2 Grid", sizeof(int16_t), kStylusRawGridCells, sizeof(int16_t), kStylusRawGridDim, kStylusRawGridDim));

    fields.push_back(MakeField(id++, stylusPacket + static_cast<uint32_t>(offsetof(Dvr2StylusPacketRecord, valid)), sizeof(uint8_t), Dvr2ValueType::Bool, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.output.packet.valid", "Stylus Packet Valid"));
    fields.push_back(MakeField(id++, stylusPacket + static_cast<uint32_t>(offsetof(Dvr2StylusPacketRecord, reportId)), sizeof(uint8_t), Dvr2ValueType::UInt8, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.output.packet.reportId", "Stylus Packet Report ID"));
    fields.push_back(MakeField(id++, stylusPacket + static_cast<uint32_t>(offsetof(Dvr2StylusPacketRecord, length)), sizeof(uint8_t), Dvr2ValueType::UInt8, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.output.packet.length", "Stylus Packet Length"));
    fields.push_back(MakeField(id++, stylusPacket + static_cast<uint32_t>(offsetof(Dvr2StylusPacketRecord, bytes)), sizeof(Dvr2StylusPacketRecord::bytes), Dvr2ValueType::UInt8, Dvr2FieldRank::Array, Dvr2FieldGroup::Stylus, "stylus.output.packet.bytes", "Stylus Packet Bytes", sizeof(uint8_t), sizeof(Dvr2StylusPacketRecord::bytes)));

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
    fields.push_back(MakeField(id++, stylusPoint + static_cast<uint32_t>(offsetof(Dvr2StylusPointRecord, tiltValid)), sizeof(uint8_t), Dvr2ValueType::Bool, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.point.tiltValid", "Stylus Tilt Valid"));
    fields.push_back(MakeField(id++, stylusPoint + static_cast<uint32_t>(offsetof(Dvr2StylusPointRecord, preTiltX)), sizeof(int16_t), Dvr2ValueType::Int16, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.point.preTiltX", "Stylus Pre Tilt X"));
    fields.push_back(MakeField(id++, stylusPoint + static_cast<uint32_t>(offsetof(Dvr2StylusPointRecord, preTiltY)), sizeof(int16_t), Dvr2ValueType::Int16, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.point.preTiltY", "Stylus Pre Tilt Y"));
    fields.push_back(MakeField(id++, stylusPoint + static_cast<uint32_t>(offsetof(Dvr2StylusPointRecord, tiltX)), sizeof(int16_t), Dvr2ValueType::Int16, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.point.tiltX", "Stylus Tilt X"));
    fields.push_back(MakeField(id++, stylusPoint + static_cast<uint32_t>(offsetof(Dvr2StylusPointRecord, tiltY)), sizeof(int16_t), Dvr2ValueType::Int16, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.point.tiltY", "Stylus Tilt Y"));
    fields.push_back(MakeField(id++, stylusPoint + static_cast<uint32_t>(offsetof(Dvr2StylusPointRecord, tiltMagnitude)), sizeof(float), Dvr2ValueType::Float32, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.point.tiltMagnitude", "Stylus Tilt Magnitude"));
    fields.push_back(MakeField(id++, stylusPoint + static_cast<uint32_t>(offsetof(Dvr2StylusPointRecord, tiltAzimuthDeg)), sizeof(float), Dvr2ValueType::Float32, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.point.tiltAzimuthDeg", "Stylus Tilt Azimuth Deg"));
    fields.push_back(MakeField(id++, stylusPoint + static_cast<uint32_t>(offsetof(Dvr2StylusPointRecord, tx1X)), sizeof(float), Dvr2ValueType::Float32, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.point.tx1X", "Stylus TX1 X"));
    fields.push_back(MakeField(id++, stylusPoint + static_cast<uint32_t>(offsetof(Dvr2StylusPointRecord, tx1Y)), sizeof(float), Dvr2ValueType::Float32, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.point.tx1Y", "Stylus TX1 Y"));
    fields.push_back(MakeField(id++, stylusPoint + static_cast<uint32_t>(offsetof(Dvr2StylusPointRecord, tx2X)), sizeof(float), Dvr2ValueType::Float32, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.point.tx2X", "Stylus TX2 X"));
    fields.push_back(MakeField(id++, stylusPoint + static_cast<uint32_t>(offsetof(Dvr2StylusPointRecord, tx2Y)), sizeof(float), Dvr2ValueType::Float32, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.point.tx2Y", "Stylus TX2 Y"));
    fields.push_back(MakeField(id++, stylusPoint + static_cast<uint32_t>(offsetof(Dvr2StylusPointRecord, confidence)), sizeof(float), Dvr2ValueType::Float32, Dvr2FieldRank::Scalar, Dvr2FieldGroup::Stylus, "stylus.point.confidence", "Stylus Confidence"));

    const uint32_t contactsParentId = id;
    fields.push_back(MakeField(id++, contacts, sizeof(Dvr2FrameCore::contacts), Dvr2ValueType::Bytes, Dvr2FieldRank::StructArray, Dvr2FieldGroup::Contacts, "contacts[]", "Contacts", sizeof(Dvr2ContactRecord), kMaxContacts, sizeof(Dvr2ContactRecord)));
#define DVR_CONTACT_FIELD(member, type, valueType, path, display) \
    fields.push_back(MakeField(id++, contacts + static_cast<uint32_t>(offsetof(Dvr2ContactRecord, member)), sizeof(type), valueType, Dvr2FieldRank::Array, Dvr2FieldGroup::Contacts, path, display, sizeof(type), kMaxContacts, sizeof(Dvr2ContactRecord), 0, 0, {}, contactsParentId))
    DVR_CONTACT_FIELD(id, int32_t, Dvr2ValueType::Int32, "contacts[].id", "Contact ID");
    DVR_CONTACT_FIELD(x, float, Dvr2ValueType::Float32, "contacts[].x", "Contact X");
    DVR_CONTACT_FIELD(y, float, Dvr2ValueType::Float32, "contacts[].y", "Contact Y");
    DVR_CONTACT_FIELD(state, int32_t, Dvr2ValueType::Int32, "contacts[].state", "Contact State");
    DVR_CONTACT_FIELD(area, int32_t, Dvr2ValueType::Int32, "contacts[].area", "Contact Area");
    DVR_CONTACT_FIELD(signalSum, int32_t, Dvr2ValueType::Int32, "contacts[].signalSum", "Contact Signal Sum");
    DVR_CONTACT_FIELD(sizeMm, float, Dvr2ValueType::Float32, "contacts[].sizeMm", "Contact Size Mm");
    DVR_CONTACT_FIELD(edgeDistX, float, Dvr2ValueType::Float32, "contacts[].edgeDistX", "Contact Edge Dist X");
    DVR_CONTACT_FIELD(edgeDistY, float, Dvr2ValueType::Float32, "contacts[].edgeDistY", "Contact Edge Dist Y");
    DVR_CONTACT_FIELD(rawXBeforeEC, float, Dvr2ValueType::Float32, "contacts[].rawXBeforeEC", "Contact Raw X Before EC");
    DVR_CONTACT_FIELD(rawYBeforeEC, float, Dvr2ValueType::Float32, "contacts[].rawYBeforeEC", "Contact Raw Y Before EC");
    DVR_CONTACT_FIELD(prevIndex, int32_t, Dvr2ValueType::Int32, "contacts[].prevIndex", "Contact Previous Index");
    DVR_CONTACT_FIELD(debugFlags, int32_t, Dvr2ValueType::Int32, "contacts[].debugFlags", "Contact Debug Flags");
    DVR_CONTACT_FIELD(edgeFlags, uint32_t, Dvr2ValueType::UInt32, "contacts[].edgeFlags", "Contact Edge Flags");
    DVR_CONTACT_FIELD(centroidEdgeFlags, uint8_t, Dvr2ValueType::UInt8, "contacts[].centroidEdgeFlags", "Contact Centroid Edge Flags");
    DVR_CONTACT_FIELD(ecFlags, uint32_t, Dvr2ValueType::UInt32, "contacts[].ecFlags", "Contact EC Flags");
    DVR_CONTACT_FIELD(ecWidthX, uint8_t, Dvr2ValueType::UInt8, "contacts[].ecWidthX", "Contact EC Width X");
    DVR_CONTACT_FIELD(ecWidthY, uint8_t, Dvr2ValueType::UInt8, "contacts[].ecWidthY", "Contact EC Width Y");
    DVR_CONTACT_FIELD(lifeFlags, uint32_t, Dvr2ValueType::UInt32, "contacts[].lifeFlags", "Contact Life Flags");
    DVR_CONTACT_FIELD(reportFlags, uint32_t, Dvr2ValueType::UInt32, "contacts[].reportFlags", "Contact Report Flags");
    DVR_CONTACT_FIELD(reportEvent, int32_t, Dvr2ValueType::Int32, "contacts[].reportEvent", "Contact Report Event");
    DVR_CONTACT_FIELD(isEdge, uint8_t, Dvr2ValueType::Bool, "contacts[].isEdge", "Contact Is Edge");
    DVR_CONTACT_FIELD(isReported, uint8_t, Dvr2ValueType::Bool, "contacts[].isReported", "Contact Is Reported");
#undef DVR_CONTACT_FIELD
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
