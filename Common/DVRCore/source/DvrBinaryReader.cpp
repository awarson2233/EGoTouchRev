#include "DvrBinaryIO.h"
#include "DvrFormat.h"
#include "StylusSolver/AsaTypes.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <string_view>
#include <vector>

namespace Dvr {

namespace {

namespace DvrFmt = Dvr::Format;

using Dvr2FileHeader = DvrFmt::Dvr2FileHeader;
using Dvr2SectionEntry = DvrFmt::Dvr2SectionEntry;
using Dvr2SectionType = DvrFmt::Dvr2SectionType;
using Dvr2MetaSection = DvrFmt::Dvr2MetaSection;
using Dvr2FrameSchemaHeader = DvrFmt::Dvr2FrameSchemaHeader;
using Dvr2FieldDef = DvrFmt::Dvr2FieldDef;
using Dvr2IndexEntry = DvrFmt::Dvr2IndexEntry;
using Dvr2FramePayload = DvrFmt::Dvr2FramePayload;
using Dvr2DynamicDebugSchemaHeader = DvrFmt::Dvr2DynamicDebugSchemaHeader;
using Dvr2DynamicDebugValuesHeader = DvrFmt::Dvr2DynamicDebugValuesHeader;
using Dvr2DynamicDebugFrameHeader = DvrFmt::Dvr2DynamicDebugFrameHeader;
using Dvr2DynamicDebugSample = DvrFmt::Dvr2DynamicDebugSample;
using Dvr2RuntimeConfigSchemaHeader = DvrFmt::Dvr2RuntimeConfigSchemaHeader;
using Dvr2RuntimeConfigFieldDef = DvrFmt::Dvr2RuntimeConfigFieldDef;
using Dvr2RuntimeConfigValuesHeader = DvrFmt::Dvr2RuntimeConfigValuesHeader;
using Dvr2RuntimeConfigValueRecord = DvrFmt::Dvr2RuntimeConfigValueRecord;

bool ReadAll(std::ifstream& in, void* data, size_t bytes) {
    in.read(reinterpret_cast<char*>(data), static_cast<std::streamsize>(bytes));
    return static_cast<bool>(in);
}

std::array<char, 8> MakeDvr2Magic() {
    return DvrFmt::kDvr2Magic;
}

std::array<char, 8> MakeLegacyDvrMagic() {
    return DvrFmt::kLegacyDvrMagic;
}

bool HasUniqueDynamicFieldIds(const DynamicDebugSchema& schema) {
    for (size_t i = 0; i < schema.fields.size(); ++i) {
        for (size_t j = i + 1; j < schema.fields.size(); ++j) {
            if (schema.fields[i].fieldId == schema.fields[j].fieldId) {
                return false;
            }
        }
    }
    return true;
}

bool DynamicDebugSamplesMatchSchema(const DynamicDebugSchema& schema,
                                    const DynamicDebugFrame& frame) {
    if (frame.samples.size() != schema.fields.size()) return false;
    for (size_t i = 0; i < schema.fields.size(); ++i) {
        if (frame.samples[i].fieldId != schema.fields[i].fieldId) return false;
        if (frame.samples[i].value.valueType != schema.fields[i].valueType) return false;
    }
    return true;
}

bool ValidateDynamicDebugPayload(const DynamicDebugSchema& schema,
                                 const std::vector<DynamicDebugFrame>& frames,
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

std::string FixedStringToString(const char* data, size_t size) {
    const auto* end = std::find(data, data + size, '\0');
    return std::string(data, static_cast<size_t>(end - data));
}

Dvr2RuntimeConfigFieldDef MakeRuntimeConfigFieldDef(const RuntimeConfigField& field) {
    Dvr2RuntimeConfigFieldDef wire{};
    wire.fieldId = field.fieldId;
    wire.valueType = static_cast<uint8_t>(field.valueType);
    wire.category = field.category;
    wire.minValue = field.minValue;
    wire.maxValue = field.maxValue;
    DvrFmt::CopyFixedString(wire.section, sizeof(wire.section), field.section);
    DvrFmt::CopyFixedString(wire.key, sizeof(wire.key), field.key);
    DvrFmt::CopyFixedString(wire.displayName, sizeof(wire.displayName), field.displayName);
    DvrFmt::CopyFixedString(wire.moduleTag, sizeof(wire.moduleTag), field.moduleTag);
    DvrFmt::CopyFixedString(wire.unit, sizeof(wire.unit), field.unit);
    return wire;
}

Dvr2RuntimeConfigValueRecord MakeRuntimeConfigValueRecord(const RuntimeConfigValue& value) {
    Dvr2RuntimeConfigValueRecord wire{};
    wire.fieldId = value.fieldId;
    wire.valueType = static_cast<uint8_t>(value.valueType);
    wire.flags = value.valid ? 0x1u : 0x0u;
    wire.rawValue = value.rawValue;
    DvrFmt::CopyFixedString(wire.stringValue, sizeof(wire.stringValue), value.stringValue);
    wire.stringLength = static_cast<uint16_t>(std::min<size_t>(value.stringValue.size(), sizeof(wire.stringValue) - 1));
    return wire;
}

bool RuntimeConfigValuesMatchSchema(const RuntimeConfigSnapshot& snapshot) {
    if (snapshot.fields.size() != snapshot.values.size()) return false;
    for (size_t i = 0; i < snapshot.fields.size(); ++i) {
        if (snapshot.fields[i].fieldId != snapshot.values[i].fieldId) return false;
        if (snapshot.fields[i].valueType != snapshot.values[i].valueType) return false;
        for (size_t j = i + 1; j < snapshot.fields.size(); ++j) {
            if (snapshot.fields[i].fieldId == snapshot.fields[j].fieldId) return false;
            if (snapshot.fields[i].section == snapshot.fields[j].section &&
                snapshot.fields[i].key == snapshot.fields[j].key) return false;
        }
    }
    return true;
}

bool CanPersistRuntimeConfig(const RuntimeConfigSnapshot* snapshot) {
    if (!snapshot || snapshot->Empty()) return false;
    if (snapshot->fields.size() > 0xFFFFu || snapshot->values.size() > 0xFFFFu) return false;
    return RuntimeConfigValuesMatchSchema(*snapshot);
}

bool ValidateRuntimeConfigPayload(const RuntimeConfigSnapshot& snapshot,
                                  std::string* outError) {
    if (snapshot.Empty()) {
        if (outError) *outError = "DVR2 runtime config snapshot is empty";
        return false;
    }
    if (!RuntimeConfigValuesMatchSchema(snapshot)) {
        if (outError) *outError = "DVR2 runtime config values do not match config schema";
        return false;
    }
    return true;
}

bool RuntimeConfigSchemaHasUniqueFields(const std::vector<RuntimeConfigField>& fields) {
    for (size_t i = 0; i < fields.size(); ++i) {
        if (fields[i].fieldId == 0 || fields[i].section.empty() || fields[i].key.empty()) return false;
        for (size_t j = i + 1; j < fields.size(); ++j) {
            if (fields[i].fieldId == fields[j].fieldId) return false;
            if (fields[i].section == fields[j].section && fields[i].key == fields[j].key) return false;
        }
    }
    return true;
}

bool IsValidRuntimeConfigValueType(uint8_t valueType) {
    return valueType <= static_cast<uint8_t>(DvrFmt::Dvr2ConfigValueType::UInt32);
}

uint64_t FieldExtent(const Dvr2FieldDef& field) {
    const uint64_t elementCount = field.elementCount == 0 ? 1 : field.elementCount;
    const uint64_t elementSize = field.elementSize == 0 ? field.size : field.elementSize;
    const uint64_t stride = field.stride == 0 ? elementSize : field.stride;
    return static_cast<uint64_t>(field.offset) + (elementCount - 1) * stride + elementSize;
}

bool RangeWithinRecord(uint32_t offset, size_t length, size_t recordSize) noexcept {
    return offset <= recordSize && length <= recordSize - offset;
}

bool ValidateContiguousFieldBounds(const Dvr2FieldDef& field,
                                   size_t recordSize,
                                   size_t length,
                                   std::string_view path,
                                   std::string* outError) {
    if (!RangeWithinRecord(field.offset, length, recordSize)) {
        if (outError) {
            *outError = "DVR2 frame schema contiguous field exceeds record bounds: ";
            outError->append(path);
        }
        return false;
    }
    return true;
}

const Dvr2FieldDef* RequireField(const std::vector<Dvr2FieldDef>& fields,
                                 std::string_view path,
                                 std::string* outError) {
    const auto* field = DvrFmt::FindField(fields, path);
    if (!field && outError) {
        *outError = "DVR2 frame schema is missing required field: ";
        outError->append(path);
    }
    return field;
}

bool ValidateField(const Dvr2FieldDef& field,
                   size_t recordSize,
                   std::string_view path,
                   DvrFmt::Dvr2ValueType valueType,
                   DvrFmt::Dvr2FieldRank rank,
                   std::string* outError) {
    if (field.valueType != static_cast<uint8_t>(valueType) ||
        field.rank != static_cast<uint8_t>(rank)) {
        if (outError) {
            *outError = "DVR2 frame schema field has unexpected type/rank: ";
            outError->append(path);
        }
        return false;
    }
    if (FieldExtent(field) > recordSize) {
        if (outError) {
            *outError = "DVR2 frame schema field exceeds record bounds: ";
            outError->append(path);
        }
        return false;
    }
    return true;
}

template <typename T>
bool ReadScalarField(const std::vector<uint8_t>& record,
                     const std::vector<Dvr2FieldDef>& fields,
                     std::string_view path,
                     DvrFmt::Dvr2ValueType valueType,
                     T& out,
                     std::string* outError) {
    const auto* field = RequireField(fields, path, outError);
    if (!field) return false;
    if (!ValidateField(*field, record.size(), path, valueType, DvrFmt::Dvr2FieldRank::Scalar, outError)) return false;
    if (field->size != sizeof(T) || field->elementSize != sizeof(T)) {
        if (outError) {
            *outError = "DVR2 frame schema scalar size mismatch: ";
            outError->append(path);
        }
        return false;
    }
    std::memcpy(&out, record.data() + field->offset, sizeof(T));
    return true;
}

bool CopyContiguousField(const std::vector<uint8_t>& record,
                         const std::vector<Dvr2FieldDef>& fields,
                         std::string_view path,
                         DvrFmt::Dvr2ValueType valueType,
                         DvrFmt::Dvr2FieldRank rank,
                         uint32_t expectedSize,
                         void* dst,
                         std::string* outError) {
    const auto* field = RequireField(fields, path, outError);
    if (!field) return false;
    if (!ValidateField(*field, record.size(), path, valueType, rank, outError)) return false;
    if (field->size != expectedSize) {
        if (outError) {
            *outError = "DVR2 frame schema field size mismatch: ";
            outError->append(path);
        }
        return false;
    }
    if (!ValidateContiguousFieldBounds(*field, record.size(), expectedSize, path, outError)) return false;
    std::memcpy(dst, record.data() + field->offset, expectedSize);
    return true;
}

template <typename T>
bool ReadStridedField(const std::vector<uint8_t>& record,
                      const std::vector<Dvr2FieldDef>& fields,
                      std::string_view path,
                      DvrFmt::Dvr2ValueType valueType,
                      uint32_t index,
                      T& out,
                      std::string* outError) {
    const auto* field = RequireField(fields, path, outError);
    if (!field) return false;
    if (!ValidateField(*field, record.size(), path, valueType, DvrFmt::Dvr2FieldRank::Array, outError)) return false;
    if (field->elementSize != sizeof(T) || index >= field->elementCount) {
        if (outError) {
            *outError = "DVR2 frame schema strided field mismatch: ";
            outError->append(path);
        }
        return false;
    }
    const uint64_t offset = static_cast<uint64_t>(field->offset) + static_cast<uint64_t>(index) * field->stride;
    if (offset + sizeof(T) > record.size()) {
        if (outError) {
            *outError = "DVR2 frame schema strided field exceeds record bounds: ";
            outError->append(path);
        }
        return false;
    }
    std::memcpy(&out, record.data() + offset, sizeof(T));
    return true;
}

template <typename T>
bool TryReadScalarField(const std::vector<uint8_t>& record,
                        const std::vector<Dvr2FieldDef>& fields,
                        std::string_view path,
                        DvrFmt::Dvr2ValueType valueType,
                        T& out,
                        std::string* outError,
                        bool* outPresent = nullptr) {
    if (outPresent) *outPresent = false;
    const auto* field = DvrFmt::FindField(fields, path);
    if (!field) return true;
    if (!ValidateField(*field, record.size(), path, valueType, DvrFmt::Dvr2FieldRank::Scalar, outError)) return false;
    if (field->size != sizeof(T) || field->elementSize != sizeof(T)) {
        if (outError) {
            *outError = "DVR2 frame schema scalar size mismatch: ";
            outError->append(path);
        }
        return false;
    }
    std::memcpy(&out, record.data() + field->offset, sizeof(T));
    if (outPresent) *outPresent = true;
    return true;
}

bool TryCopyContiguousField(const std::vector<uint8_t>& record,
                            const std::vector<Dvr2FieldDef>& fields,
                            std::string_view path,
                            DvrFmt::Dvr2ValueType valueType,
                            DvrFmt::Dvr2FieldRank rank,
                            uint32_t expectedSize,
                            void* dst,
                            std::string* outError,
                            bool* outPresent = nullptr) {
    if (outPresent) *outPresent = false;
    const auto* field = DvrFmt::FindField(fields, path);
    if (!field) return true;
    if (!ValidateField(*field, record.size(), path, valueType, rank, outError)) return false;
    if (field->size != expectedSize) {
        if (outError) {
            *outError = "DVR2 frame schema field size mismatch: ";
            outError->append(path);
        }
        return false;
    }
    if (!ValidateContiguousFieldBounds(*field, record.size(), expectedSize, path, outError)) return false;
    std::memcpy(dst, record.data() + field->offset, expectedSize);
    if (outPresent) *outPresent = true;
    return true;
}

template <typename T>
bool TryReadStridedField(const std::vector<uint8_t>& record,
                         const std::vector<Dvr2FieldDef>& fields,
                         std::string_view path,
                         DvrFmt::Dvr2ValueType valueType,
                         uint32_t index,
                         T& out,
                         std::string* outError,
                         bool* outPresent = nullptr) {
    if (outPresent) *outPresent = false;
    const auto* field = DvrFmt::FindField(fields, path);
    if (!field) return true;
    if (!ValidateField(*field, record.size(), path, valueType, DvrFmt::Dvr2FieldRank::Array, outError)) return false;
    if (field->elementSize != sizeof(T) || index >= field->elementCount) {
        if (outError) {
            *outError = "DVR2 frame schema strided field mismatch: ";
            outError->append(path);
        }
        return false;
    }
    const uint64_t offset = static_cast<uint64_t>(field->offset) + static_cast<uint64_t>(index) * field->stride;
    if (offset + sizeof(T) > record.size()) {
        if (outError) {
            *outError = "DVR2 frame schema strided field exceeds record bounds: ";
            outError->append(path);
        }
        return false;
    }
    std::memcpy(&out, record.data() + offset, sizeof(T));
    if (outPresent) *outPresent = true;
    return true;
}

bool TryReadBoolScalarField(const std::vector<uint8_t>& record,
                            const std::vector<Dvr2FieldDef>& fields,
                            std::string_view path,
                            bool& out,
                            std::string* outError) {
    uint8_t value = out ? 1 : 0;
    bool present = false;
    if (!TryReadScalarField(record, fields, path, DvrFmt::Dvr2ValueType::Bool, value, outError, &present)) return false;
    if (present) out = value != 0;
    return true;
}

bool TryReadBoolStridedField(const std::vector<uint8_t>& record,
                             const std::vector<Dvr2FieldDef>& fields,
                             std::string_view path,
                             uint32_t index,
                             bool& out,
                             std::string* outError) {
    uint8_t value = out ? 1 : 0;
    bool present = false;
    if (!TryReadStridedField(record, fields, path, DvrFmt::Dvr2ValueType::Bool, index, value, outError, &present)) return false;
    if (present) out = value != 0;
    return true;
}

bool ValidateCriticalRequiredFieldsPresent(const std::vector<Dvr2FieldDef>& fields,
                                           std::string* outError) {
    // DVR2 readers are schema-driven: a legacy file should only be rejected when
    // fields that cannot be synthesized by PopulateHeatmapFrameFromRecordBytes()
    // are absent. Do not validate against the current writer schema here; newly
    // added fields are handled by TryRead* defaults below.
    constexpr std::array<std::string_view, 11> kCriticalRequiredFields{
        "timestamp",
        "receiveSystemEpochUs",
        "masterWasRead",
        "masterSuffixValid",
        "slaveSuffixValid",
        "heatmapMatrix",
        "masterSuffix.words",
        "slaveSuffix.words",
        "contactCount",
        "rawDataLength",
        "rawData",
    };

    for (const auto path : kCriticalRequiredFields) {
        if (!DvrFmt::FindField(fields, path)) {
            if (outError) {
                *outError = "DVR2 frame missing required field: ";
                outError->append(path);
            }
            return false;
        }
    }
    return true;
}

bool ValidateRequiredContactFieldsPresent(const std::vector<Dvr2FieldDef>& fields,
                                          uint32_t contactCount,
                                          std::string* outError) {
    if (contactCount == 0) return true;

    constexpr std::array<std::string_view, 5> kRequiredContactFields{
        "contacts[]",
        "contacts[].id",
        "contacts[].x",
        "contacts[].y",
        "contacts[].state",
    };

    for (const auto path : kRequiredContactFields) {
        if (!DvrFmt::FindField(fields, path)) {
            if (outError) {
                *outError = "DVR2 frame missing required contact field: ";
                outError->append(path);
            }
            return false;
        }
    }
    return true;
}

void SynthesizeRawGridFromLegacyFields(const std::vector<uint8_t>& record,
                                       const std::vector<Dvr2FieldDef>& fields,
                                       Solvers::HeatmapFrame& dst) {
    auto& grid = dst.stylus.runtime.hpp3.rawGrid.grid;
    if (grid.tx1.valid || grid.tx2.valid) return;

    if (const auto* rawLenField = DvrFmt::FindField(fields, "rawDataLength")) {
        const auto* rawField = DvrFmt::FindField(fields, "rawData");
        if (rawField && RangeWithinRecord(rawLenField->offset, sizeof(uint16_t), record.size())) {
            uint16_t rawLen = 0;
            std::memcpy(&rawLen, record.data() + rawLenField->offset, sizeof(rawLen));
            const size_t len = std::min<size_t>({rawLen, Frame::kTotalFrameSize, rawField->size});
            if (RangeWithinRecord(rawField->offset, len, record.size()) && len >= static_cast<size_t>(Frame::kTotalFrameSize)) {
                const size_t slaveOffset = rawField->offset + len - static_cast<size_t>(Frame::kSlaveFrameSize);
                const uint8_t* payload = record.data() + slaveOffset + Solvers::Stylus::Hpp3::kSlaveHeaderBytes;
                grid = Solvers::Stylus::Hpp3::ExtractGridFromSlavePayloadBytes(payload, static_cast<size_t>(Solvers::Stylus::Hpp3::kBlockWords * 2 * sizeof(uint16_t)));
                dst.stylus.input.tx1BlockValid = grid.tx1.valid;
                dst.stylus.input.tx2BlockValid = grid.tx2.valid;
                if (grid.tx1.valid || grid.tx2.valid) return;
            }
        }
    }

    if (dst.slaveSuffixValid) {
        grid = Solvers::Stylus::Hpp3::ExtractGridFromSlaveWords(dst.slaveSuffix.words, Frame::kSlaveSuffixWords);
        dst.stylus.input.tx1BlockValid = grid.tx1.valid;
        dst.stylus.input.tx2BlockValid = grid.tx2.valid;
    }
}

bool TryReadRawGridBlock(const std::vector<uint8_t>& record,
                         const std::vector<Dvr2FieldDef>& fields,
                         std::string_view validPath,
                         std::string_view anchorRowPath,
                         std::string_view anchorColPath,
                         std::string_view gridPath,
                         Solvers::Stylus::Hpp3::FreqBlock& out,
                         std::string* outError) {
    uint8_t valid = out.valid ? 1 : 0;
    bool present = false;
    if (!TryReadScalarField(record, fields, validPath, DvrFmt::Dvr2ValueType::Bool, valid, outError, &present)) return false;
    if (!present) return true;
    out.valid = valid != 0;
    if (!ReadScalarField(record, fields, anchorRowPath, DvrFmt::Dvr2ValueType::UInt16, out.anchorRow, outError)) return false;
    if (!ReadScalarField(record, fields, anchorColPath, DvrFmt::Dvr2ValueType::UInt16, out.anchorCol, outError)) return false;
    if (!CopyContiguousField(record, fields, gridPath, DvrFmt::Dvr2ValueType::Int16, DvrFmt::Dvr2FieldRank::Matrix, sizeof(out.grid), out.grid, outError)) return false;
    return true;
}

bool PopulateHeatmapFrameFromRecordBytes(const std::vector<uint8_t>& record,
                                         const std::vector<Dvr2FieldDef>& fields,
                                         Solvers::HeatmapFrame& dst,
                                         std::string* outError) {
    dst = {};
    if (!ValidateCriticalRequiredFieldsPresent(fields, outError)) return false;

    uint16_t u16 = 0;
    uint32_t u32 = 0;
    if (!TryReadScalarField(record, fields, "timestamp", DvrFmt::Dvr2ValueType::UInt64, dst.timestamp, outError)) return false;
    if (!TryReadScalarField(record, fields, "receiveSystemEpochUs", DvrFmt::Dvr2ValueType::UInt64, dst.receiveSystemEpochUs, outError)) return false;
    if (!TryReadBoolScalarField(record, fields, "masterWasRead", dst.masterWasRead, outError)) return false;
    if (!TryReadBoolScalarField(record, fields, "masterSuffixValid", dst.masterSuffixValid, outError)) return false;
    if (!TryReadBoolScalarField(record, fields, "slaveSuffixValid", dst.slaveSuffixValid, outError)) return false;
    if (!TryCopyContiguousField(record, fields, "heatmapMatrix", DvrFmt::Dvr2ValueType::Int16, DvrFmt::Dvr2FieldRank::Matrix, sizeof(dst.heatmapMatrix), dst.heatmapMatrix, outError)) return false;
    if (!TryCopyContiguousField(record, fields, "masterSuffix.words", DvrFmt::Dvr2ValueType::UInt16, DvrFmt::Dvr2FieldRank::Array, sizeof(dst.masterSuffix.words), dst.masterSuffix.words, outError)) return false;
    if (!TryCopyContiguousField(record, fields, "slaveSuffix.words", DvrFmt::Dvr2ValueType::UInt16, DvrFmt::Dvr2FieldRank::Array, sizeof(dst.slaveSuffix.words), dst.slaveSuffix.words, outError)) return false;

    for (uint32_t i = 0; i < DvrFmt::kTouchPacketCount; ++i) {
        if (!TryReadBoolStridedField(record, fields, "touchPackets[].valid", i, dst.touch.output.touchPackets[i].valid, outError)) return false;
        if (!TryReadStridedField(record, fields, "touchPackets[].reportId", DvrFmt::Dvr2ValueType::UInt8, i, dst.touch.output.touchPackets[i].reportId, outError)) return false;
        if (!TryReadStridedField(record, fields, "touchPackets[].length", DvrFmt::Dvr2ValueType::UInt8, i, dst.touch.output.touchPackets[i].length, outError)) return false;
        if (!TryReadStridedField(record, fields, "touchPackets[].bytes", DvrFmt::Dvr2ValueType::UInt8, i, dst.touch.output.touchPackets[i].bytes, outError)) return false;
    }

#if EGOTOUCH_DIAG
    if (!TryCopyContiguousField(record, fields, "touchZones", DvrFmt::Dvr2ValueType::UInt8, DvrFmt::Dvr2FieldRank::Array, sizeof(dst.touch.debug.touchZones), dst.touch.debug.touchZones.data(), outError)) return false;
    if (!TryCopyContiguousField(record, fields, "peakZones", DvrFmt::Dvr2ValueType::UInt8, DvrFmt::Dvr2FieldRank::Array, sizeof(dst.touch.debug.peakZones), dst.touch.debug.peakZones.data(), outError)) return false;
#endif

    auto& stylusInput = dst.stylus.input;
    auto& stylusOutput = dst.stylus.output;
    auto& stylusInterop = dst.stylus.interop;
    auto& stylusPoint = stylusOutput.point;
    auto& stylusPressure = dst.stylus.runtime.Active().pressure;
    auto& stylusRawGrid = dst.stylus.runtime.hpp3.rawGrid.grid;

    if (!TryReadBoolScalarField(record, fields, "stylus.slaveValid", stylusInput.slaveValid, outError)) return false;
    if (!TryReadBoolScalarField(record, fields, "stylus.checksumOk", stylusInput.checksumOk, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.slaveWordOffset", DvrFmt::Dvr2ValueType::UInt8, stylusInput.slaveWordOffset, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.checksum16", DvrFmt::Dvr2ValueType::UInt16, stylusInput.checksum16, outError)) return false;
    if (!TryReadBoolScalarField(record, fields, "stylus.tx1BlockValid", stylusInput.tx1BlockValid, outError)) return false;
    if (!TryReadBoolScalarField(record, fields, "stylus.tx2BlockValid", stylusInput.tx2BlockValid, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.status", DvrFmt::Dvr2ValueType::UInt32, stylusInput.status, outError)) return false;
    if (!TryCopyContiguousField(record, fields, "stylus.btPressure", DvrFmt::Dvr2ValueType::UInt16, DvrFmt::Dvr2FieldRank::Array, sizeof(stylusInput.btSample.pressure), stylusInput.btSample.pressure.data(), outError)) return false;
    if (!TryCopyContiguousField(record, fields, "stylus.btRawPressure", DvrFmt::Dvr2ValueType::UInt16, DvrFmt::Dvr2FieldRank::Array, sizeof(stylusInput.btSample.rawPressure), stylusInput.btSample.rawPressure.data(), outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.btSeq", DvrFmt::Dvr2ValueType::UInt32, stylusInput.btSample.seq, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.btFreq1", DvrFmt::Dvr2ValueType::UInt8, stylusInput.btSample.freq1, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.btFreq2", DvrFmt::Dvr2ValueType::UInt8, stylusInput.btSample.freq2, outError)) return false;
    if (!TryReadBoolScalarField(record, fields, "stylus.btHasSample", stylusInput.btSample.hasSample, outError)) return false;
    if (!TryReadBoolScalarField(record, fields, "stylus.btHasFreq", stylusInput.btSample.hasFreq, outError)) return false;

    if (!TryReadBoolScalarField(record, fields, "stylus.output.valid", stylusOutput.valid, outError)) return false;
    if (!TryReadBoolScalarField(record, fields, "stylus.output.inRange", stylusOutput.inRange, outError)) return false;
    if (!TryReadBoolScalarField(record, fields, "stylus.output.tipDown", stylusOutput.tipDown, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.pressure", DvrFmt::Dvr2ValueType::UInt16, stylusOutput.pressure, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.output.confidence", DvrFmt::Dvr2ValueType::Float32, stylusOutput.confidence, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.pipelineStage", DvrFmt::Dvr2ValueType::UInt8, stylusOutput.pipelineStage, outError)) return false;

    if (!TryReadBoolScalarField(record, fields, "stylus.output.packet.valid", stylusOutput.packet.valid, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.output.packet.reportId", DvrFmt::Dvr2ValueType::UInt8, stylusOutput.packet.reportId, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.output.packet.length", DvrFmt::Dvr2ValueType::UInt8, stylusOutput.packet.length, outError)) return false;
    if (!TryCopyContiguousField(record, fields, "stylus.output.packet.bytes", DvrFmt::Dvr2ValueType::UInt8, DvrFmt::Dvr2FieldRank::Array, sizeof(stylusOutput.packet.bytes), stylusOutput.packet.bytes.data(), outError)) return false;

    if (!TryReadScalarField(record, fields, "stylus.signalX", DvrFmt::Dvr2ValueType::UInt16, stylusInterop.signalX, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.signalY", DvrFmt::Dvr2ValueType::UInt16, stylusInterop.signalY, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.maxRawPeak", DvrFmt::Dvr2ValueType::UInt16, stylusInterop.maxRawPeak, outError)) return false;
    if (!TryReadBoolScalarField(record, fields, "stylus.interop.recheckEnabled", stylusInterop.recheckEnabled, outError)) return false;
    if (!TryReadBoolScalarField(record, fields, "stylus.interop.recheckPassed", stylusInterop.recheckPassed, outError)) return false;
    if (!TryReadBoolScalarField(record, fields, "stylus.interop.recheckOverlap", stylusInterop.recheckOverlap, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.interop.recheckThreshold", DvrFmt::Dvr2ValueType::UInt16, stylusInterop.recheckThreshold, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.interop.recheckThresholdMulti", DvrFmt::Dvr2ValueType::UInt16, stylusInterop.recheckThresholdMulti, outError)) return false;
    if (!TryReadBoolScalarField(record, fields, "stylus.interop.touchNullLike", stylusInterop.touchNullLike, outError)) return false;
    if (!TryReadBoolScalarField(record, fields, "stylus.interop.touchSuppressActive", stylusInterop.touchSuppressActive, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.interop.touchSuppressFrames", DvrFmt::Dvr2ValueType::UInt8, stylusInterop.touchSuppressFrames, outError)) return false;

    if (!TryReadBoolScalarField(record, fields, "stylus.point.valid", stylusPoint.valid, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.point.x", DvrFmt::Dvr2ValueType::Float32, stylusPoint.x, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.point.y", DvrFmt::Dvr2ValueType::Float32, stylusPoint.y, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.point.reportX", DvrFmt::Dvr2ValueType::UInt16, stylusPoint.reportX, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.point.reportY", DvrFmt::Dvr2ValueType::UInt16, stylusPoint.reportY, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.point.pressure", DvrFmt::Dvr2ValueType::UInt16, stylusPoint.pressure, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.point.rawPressure", DvrFmt::Dvr2ValueType::UInt16, stylusPoint.rawPressure, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.point.mappedPressure", DvrFmt::Dvr2ValueType::UInt16, stylusPoint.mappedPressure, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.point.peakTx1", DvrFmt::Dvr2ValueType::UInt16, stylusPoint.peakTx1, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.point.peakTx2", DvrFmt::Dvr2ValueType::UInt16, stylusPoint.peakTx2, outError)) return false;
    if (!TryReadBoolScalarField(record, fields, "stylus.point.tiltValid", stylusPoint.tiltValid, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.point.preTiltX", DvrFmt::Dvr2ValueType::Int16, stylusPoint.preTiltX, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.point.preTiltY", DvrFmt::Dvr2ValueType::Int16, stylusPoint.preTiltY, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.point.tiltX", DvrFmt::Dvr2ValueType::Int16, stylusPoint.tiltX, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.point.tiltY", DvrFmt::Dvr2ValueType::Int16, stylusPoint.tiltY, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.point.tiltMagnitude", DvrFmt::Dvr2ValueType::Float32, stylusPoint.tiltMagnitude, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.point.tiltAzimuthDeg", DvrFmt::Dvr2ValueType::Float32, stylusPoint.tiltAzimuthDeg, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.point.tx1X", DvrFmt::Dvr2ValueType::Float32, stylusPoint.tx1X, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.point.tx1Y", DvrFmt::Dvr2ValueType::Float32, stylusPoint.tx1Y, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.point.tx2X", DvrFmt::Dvr2ValueType::Float32, stylusPoint.tx2X, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.point.tx2Y", DvrFmt::Dvr2ValueType::Float32, stylusPoint.tx2Y, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.point.confidence", DvrFmt::Dvr2ValueType::Float32, stylusPoint.confidence, outError)) return false;

    stylusPressure.btSample = stylusInput.btSample;
    stylusPressure.rawPressure = stylusPoint.rawPressure;
    stylusPressure.mappedPressure = stylusPoint.mappedPressure;
    stylusPressure.outputPressure = stylusOutput.pressure;
    stylusPressure.btSeq = stylusInput.btSample.seq;
    if (!TryReadBoolScalarField(record, fields, "stylus.pressureIsReal", stylusPressure.pressureIsReal, outError)) return false;
    if (!TryReadScalarField(record, fields, "stylus.predictedAgeFrames", DvrFmt::Dvr2ValueType::UInt8, stylusPressure.predictedAgeFrames, outError)) return false;
    if (!TryReadRawGridBlock(record,
                             fields,
                             "stylus.runtime.hpp3.rawGrid.grid.tx1.valid",
                             "stylus.runtime.hpp3.rawGrid.grid.tx1.anchorRow",
                             "stylus.runtime.hpp3.rawGrid.grid.tx1.anchorCol",
                             "stylus.runtime.hpp3.rawGrid.grid.tx1.grid",
                             stylusRawGrid.tx1,
                             outError)) return false;
    if (!TryReadRawGridBlock(record,
                             fields,
                             "stylus.runtime.hpp3.rawGrid.grid.tx2.valid",
                             "stylus.runtime.hpp3.rawGrid.grid.tx2.anchorRow",
                             "stylus.runtime.hpp3.rawGrid.grid.tx2.anchorCol",
                             "stylus.runtime.hpp3.rawGrid.grid.tx2.grid",
                             stylusRawGrid.tx2,
                             outError)) return false;
#if EGOTOUCH_DIAG
    dst.stylus.debug.coord.rawPressure = stylusPoint.rawPressure;
    dst.stylus.debug.coord.mappedPressure = stylusPoint.mappedPressure;
    dst.stylus.debug.coord.btSeq = stylusInput.btSample.seq;
    dst.stylus.debug.coord.predictedAgeFrames = stylusPressure.predictedAgeFrames;
    dst.stylus.debug.coord.pressureIsReal = stylusPressure.pressureIsReal;
#endif

    if (!TryReadScalarField(record, fields, "contactCount", DvrFmt::Dvr2ValueType::UInt32, u32, outError)) return false;
    if (!ValidateRequiredContactFieldsPresent(fields, u32, outError)) return false;
    uint32_t contactCapacity = DvrFmt::kMaxContacts;
    if (const auto* contactsField = DvrFmt::FindField(fields, "contacts[]")) {
        contactCapacity = std::min<uint32_t>(contactCapacity, contactsField->elementCount);
    }
    const uint32_t contactCount = std::min<uint32_t>(u32, contactCapacity);
    dst.touch.output.contacts.clear();
    dst.touch.output.contacts.reserve(contactCount);
    for (uint32_t i = 0; i < contactCount; ++i) {
        Solvers::TouchContact tc{};
        if (!TryReadStridedField(record, fields, "contacts[].id", DvrFmt::Dvr2ValueType::Int32, i, tc.id, outError)) return false;
        if (!TryReadStridedField(record, fields, "contacts[].x", DvrFmt::Dvr2ValueType::Float32, i, tc.x, outError)) return false;
        if (!TryReadStridedField(record, fields, "contacts[].y", DvrFmt::Dvr2ValueType::Float32, i, tc.y, outError)) return false;
        if (!TryReadStridedField(record, fields, "contacts[].state", DvrFmt::Dvr2ValueType::Int32, i, tc.state, outError)) return false;
        if (!TryReadStridedField(record, fields, "contacts[].area", DvrFmt::Dvr2ValueType::Int32, i, tc.area, outError)) return false;
        if (!TryReadStridedField(record, fields, "contacts[].signalSum", DvrFmt::Dvr2ValueType::Int32, i, tc.signalSum, outError)) return false;
        if (!TryReadStridedField(record, fields, "contacts[].sizeMm", DvrFmt::Dvr2ValueType::Float32, i, tc.sizeMm, outError)) return false;
        if (!TryReadBoolStridedField(record, fields, "contacts[].isEdge", i, tc.isEdge, outError)) return false;
        if (!TryReadBoolStridedField(record, fields, "contacts[].isReported", i, tc.isReported, outError)) return false;
        if (!TryReadStridedField(record, fields, "contacts[].prevIndex", DvrFmt::Dvr2ValueType::Int32, i, tc.prevIndex, outError)) return false;
        if (!TryReadStridedField(record, fields, "contacts[].debugFlags", DvrFmt::Dvr2ValueType::Int32, i, tc.debugFlags, outError)) return false;
        if (!TryReadStridedField(record, fields, "contacts[].edgeFlags", DvrFmt::Dvr2ValueType::UInt32, i, tc.edgeFlags, outError)) return false;
        if (!TryReadStridedField(record, fields, "contacts[].centroidEdgeFlags", DvrFmt::Dvr2ValueType::UInt8, i, tc.centroidEdgeFlags, outError)) return false;
        if (!TryReadStridedField(record, fields, "contacts[].ecFlags", DvrFmt::Dvr2ValueType::UInt32, i, tc.ecFlags, outError)) return false;
        if (!TryReadStridedField(record, fields, "contacts[].edgeDistX", DvrFmt::Dvr2ValueType::Float32, i, tc.edgeDistX, outError)) return false;
        if (!TryReadStridedField(record, fields, "contacts[].edgeDistY", DvrFmt::Dvr2ValueType::Float32, i, tc.edgeDistY, outError)) return false;
        if (!TryReadStridedField(record, fields, "contacts[].rawXBeforeEC", DvrFmt::Dvr2ValueType::Float32, i, tc.rawXBeforeEC, outError)) return false;
        if (!TryReadStridedField(record, fields, "contacts[].rawYBeforeEC", DvrFmt::Dvr2ValueType::Float32, i, tc.rawYBeforeEC, outError)) return false;
        if (!TryReadStridedField(record, fields, "contacts[].ecWidthX", DvrFmt::Dvr2ValueType::UInt8, i, tc.ecWidthX, outError)) return false;
        if (!TryReadStridedField(record, fields, "contacts[].ecWidthY", DvrFmt::Dvr2ValueType::UInt8, i, tc.ecWidthY, outError)) return false;
        if (!TryReadStridedField(record, fields, "contacts[].lifeFlags", DvrFmt::Dvr2ValueType::UInt32, i, tc.lifeFlags, outError)) return false;
        if (!TryReadStridedField(record, fields, "contacts[].reportFlags", DvrFmt::Dvr2ValueType::UInt32, i, tc.reportFlags, outError)) return false;
        if (!TryReadStridedField(record, fields, "contacts[].reportEvent", DvrFmt::Dvr2ValueType::Int32, i, tc.reportEvent, outError)) return false;
        dst.touch.output.contacts.push_back(tc);
    }

#if EGOTOUCH_DIAG
    u32 = 0;
    if (!TryReadScalarField(record, fields, "peakCount", DvrFmt::Dvr2ValueType::UInt32, u32, outError)) return false;
    uint32_t peakCapacity = DvrFmt::kMaxPeaks;
    if (const auto* peaksField = DvrFmt::FindField(fields, "peaks[]")) {
        peakCapacity = std::min<uint32_t>(peakCapacity, peaksField->elementCount);
    }
    const uint32_t peakCount = std::min<uint32_t>(u32, peakCapacity);
    dst.touch.debug.peaks.clear();
    dst.touch.debug.peaks.reserve(peakCount);
    for (uint32_t i = 0; i < peakCount; ++i) {
        Solvers::TouchPeak tp{};
        if (!TryReadStridedField(record, fields, "peaks[].r", DvrFmt::Dvr2ValueType::Int32, i, tp.r, outError)) return false;
        if (!TryReadStridedField(record, fields, "peaks[].c", DvrFmt::Dvr2ValueType::Int32, i, tp.c, outError)) return false;
        if (!TryReadStridedField(record, fields, "peaks[].z", DvrFmt::Dvr2ValueType::Int16, i, tp.z, outError)) return false;
        if (!TryReadStridedField(record, fields, "peaks[].id", DvrFmt::Dvr2ValueType::UInt8, i, tp.id, outError)) return false;
        dst.touch.debug.peaks.push_back(tp);
    }
#endif

    if (!TryReadScalarField(record, fields, "rawDataLength", DvrFmt::Dvr2ValueType::UInt16, u16, outError)) return false;
    const auto* rawField = DvrFmt::FindField(fields, "rawData");
    if (!rawField) {
        SynthesizeRawGridFromLegacyFields(record, fields, dst);
        return true;
    }
    if (!ValidateField(*rawField, record.size(), "rawData", DvrFmt::Dvr2ValueType::UInt8, DvrFmt::Dvr2FieldRank::Array, outError)) return false;
    if (rawField->elementSize != sizeof(uint8_t)) {
        if (outError) *outError = "DVR2 frame schema rawData element size mismatch";
        return false;
    }
    const size_t rawLen = std::min<size_t>({u16, Frame::kTotalFrameSize, rawField->size});
    if (!ValidateContiguousFieldBounds(*rawField, record.size(), rawLen, "rawData", outError)) return false;
    SynthesizeRawGridFromLegacyFields(record, fields, dst);
#if EGOTOUCH_DIAG
    dst.rawData.assign(record.data() + rawField->offset, record.data() + rawField->offset + rawLen);
    dst.rawPtr = dst.rawData.empty() ? nullptr : dst.rawData.data();
    dst.rawLen = dst.rawData.size();
#else
    dst.rawPtr = nullptr;
    dst.rawLen = 0;
#endif
    return true;
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

bool ReadFrameSchemaSection(std::ifstream& in,
                            const Dvr2SectionEntry& section,
                            Dvr2FrameSchemaHeader& outHeader,
                            std::vector<Dvr2FieldDef>& outFields,
                            std::string* outError) {
    outHeader = {};
    outFields.clear();
    if (section.size < sizeof(Dvr2FrameSchemaHeader)) {
        if (outError) *outError = "DVR2 frame schema section is truncated";
        return false;
    }
    in.seekg(static_cast<std::streamoff>(section.offset), std::ios::beg);
    if (!in.good()) {
        if (outError) *outError = "invalid DVR2 frame schema offset";
        return false;
    }
    if (!ReadAll(in, &outHeader, sizeof(outHeader))) {
        if (outError) *outError = "failed to read DVR2 frame schema header";
        return false;
    }
    if (outHeader.fieldRecordSize != sizeof(Dvr2FieldDef)) {
        if (outError) *outError = "unsupported DVR2 frame schema field record size";
        return false;
    }
    const uint64_t expectedSize = sizeof(Dvr2FrameSchemaHeader) +
        static_cast<uint64_t>(outHeader.fieldCount) * sizeof(Dvr2FieldDef);
    if (section.size != expectedSize) {
        if (outError) *outError = "DVR2 frame schema section size mismatch";
        return false;
    }
    outFields.resize(outHeader.fieldCount);
    if (!outFields.empty() && !ReadAll(in, outFields.data(), outFields.size() * sizeof(Dvr2FieldDef))) {
        if (outError) *outError = "failed to read DVR2 frame schema fields";
        return false;
    }
    if (DvrFmt::ComputeFieldSchemaHash(outFields) != outHeader.schemaHash) {
        if (outError) *outError = "DVR2 frame schema hash mismatch";
        return false;
    }
    return true;
}

bool ValidateFrameSchema(const Dvr2FrameSchemaHeader& schemaHeader,
                         const Dvr2MetaSection& meta,
                         const std::vector<Dvr2FieldDef>& fields,
                         std::string* outError) {
    if (schemaHeader.fieldCount == 0 || fields.empty() || schemaHeader.fieldCount != fields.size()) {
        if (outError) *outError = "DVR2 frame schema contains no fields";
        return false;
    }
    if (schemaHeader.schemaHash != meta.frameSchemaHash) {
        if (outError) *outError = "DVR2 meta/frame schema hash mismatch";
        return false;
    }
    if (schemaHeader.frameRecordSize != meta.frameRecordSize) {
        if (outError) *outError = "DVR2 meta/frame schema record size mismatch";
        return false;
    }
    for (size_t i = 0; i < fields.size(); ++i) {
        const auto& field = fields[i];
        const auto* end = std::find(field.path, field.path + sizeof(field.path), '\0');
        const std::string_view path(field.path, static_cast<size_t>(end - field.path));
        if (path.empty() || field.size == 0 || field.elementSize == 0 || field.elementCount == 0) {
            if (outError) *outError = "DVR2 frame schema contains an invalid field definition";
            return false;
        }
        if (field.valueType > static_cast<uint8_t>(DvrFmt::Dvr2ValueType::Bytes) ||
            field.rank > static_cast<uint8_t>(DvrFmt::Dvr2FieldRank::StructArray)) {
            if (outError) {
                *outError = "DVR2 frame schema field has invalid type/rank: ";
                outError->append(path);
            }
            return false;
        }
        if (FieldExtent(field) > meta.frameRecordSize) {
            if (outError) {
                *outError = "DVR2 frame schema field exceeds record bounds: ";
                outError->append(path);
            }
            return false;
        }
        for (size_t j = i + 1; j < fields.size(); ++j) {
            const auto* otherEnd = std::find(fields[j].path, fields[j].path + sizeof(fields[j].path), '\0');
            const std::string_view otherPath(fields[j].path, static_cast<size_t>(otherEnd - fields[j].path));
            if (field.fieldId == fields[j].fieldId || path == otherPath) {
                if (outError) *outError = "DVR2 frame schema contains duplicate fields";
                return false;
            }
        }
    }
    return true;
}

bool ReadDynamicDebugSchemaSection(std::ifstream& in,
                                   const Dvr2SectionEntry& section,
                                   DynamicDebugSchema& outSchema,
                                   std::string* outError) {
    outSchema = {};
    if (section.size < sizeof(Dvr2DynamicDebugSchemaHeader)) {
        if (outError) *outError = "DVR2 dynamic schema section is truncated";
        return false;
    }

    in.seekg(static_cast<std::streamoff>(section.offset), std::ios::beg);
    if (!in.good()) {
        if (outError) *outError = "invalid DVR2 dynamic schema offset";
        return false;
    }

    Dvr2DynamicDebugSchemaHeader header{};
    if (!ReadAll(in, &header, sizeof(header))) {
        if (outError) *outError = "failed to read DVR2 dynamic schema header";
        return false;
    }
    if (header.recordSize != sizeof(Ipc::DebugFieldSchemaWire)) {
        if (outError) *outError = "unsupported DVR2 dynamic schema record size";
        return false;
    }

    const uint64_t requiredSize = sizeof(Dvr2DynamicDebugSchemaHeader) +
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
                                   std::vector<DynamicDebugFrame>& outFrames,
                                   std::string* outError) {
    outFrames.clear();
    if (section.size < sizeof(Dvr2DynamicDebugValuesHeader)) {
        if (outError) *outError = "DVR2 dynamic values section is truncated";
        return false;
    }

    in.seekg(static_cast<std::streamoff>(section.offset), std::ios::beg);
    if (!in.good()) {
        if (outError) *outError = "invalid DVR2 dynamic values offset";
        return false;
    }

    Dvr2DynamicDebugValuesHeader header{};
    if (!ReadAll(in, &header, sizeof(header))) {
        if (outError) *outError = "failed to read DVR2 dynamic values header";
        return false;
    }
    if (header.frameCount != expectedFrameCount) {
        if (outError) *outError = "DVR2 dynamic values frame count mismatch";
        return false;
    }

    uint64_t bytesConsumed = sizeof(Dvr2DynamicDebugValuesHeader);
    outFrames.resize(header.frameCount);
    for (uint32_t frameIndex = 0; frameIndex < header.frameCount; ++frameIndex) {
        if (bytesConsumed + sizeof(Dvr2DynamicDebugFrameHeader) > section.size) {
            if (outError) *outError = "DVR2 dynamic values frame header exceeds section bounds";
            return false;
        }

        Dvr2DynamicDebugFrameHeader frameHeader{};
        if (!ReadAll(in, &frameHeader, sizeof(frameHeader))) {
            if (outError) *outError = "failed to read DVR2 dynamic frame header";
            return false;
        }
        bytesConsumed += sizeof(Dvr2DynamicDebugFrameHeader);

        const uint64_t frameBytes = static_cast<uint64_t>(frameHeader.sampleCount) * sizeof(Dvr2DynamicDebugSample);
        if (bytesConsumed + frameBytes > section.size) {
            if (outError) *outError = "DVR2 dynamic frame exceeds section bounds";
            return false;
        }

        auto& frame = outFrames[frameIndex];
        frame.samples.reserve(frameHeader.sampleCount);
        for (uint32_t sampleIndex = 0; sampleIndex < frameHeader.sampleCount; ++sampleIndex) {
            Dvr2DynamicDebugSample wire{};
            if (!ReadAll(in, &wire, sizeof(wire))) {
                if (outError) *outError = "failed to read DVR2 dynamic sample";
                return false;
            }

            DynamicDebugSample sample{};
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

bool ReadRuntimeConfigSchemaSection(std::ifstream& in,
                                    const Dvr2SectionEntry& section,
                                    RuntimeConfigSnapshot& outSnapshot,
                                    std::string* outError) {
    outSnapshot = {};
    if (section.size < sizeof(Dvr2RuntimeConfigSchemaHeader)) {
        if (outError) *outError = "DVR2 runtime config schema section is truncated";
        return false;
    }

    in.seekg(static_cast<std::streamoff>(section.offset), std::ios::beg);
    if (!in.good()) {
        if (outError) *outError = "invalid DVR2 runtime config schema offset";
        return false;
    }

    Dvr2RuntimeConfigSchemaHeader header{};
    if (!ReadAll(in, &header, sizeof(header))) {
        if (outError) *outError = "failed to read DVR2 runtime config schema header";
        return false;
    }
    if (header.recordSize != sizeof(Dvr2RuntimeConfigFieldDef)) {
        if (outError) *outError = "unsupported DVR2 runtime config schema record size";
        return false;
    }
    if (header.fieldCount == 0) {
        if (outError) *outError = "DVR2 runtime config schema contains no fields";
        return false;
    }

    const uint64_t expectedSize = sizeof(Dvr2RuntimeConfigSchemaHeader) +
        static_cast<uint64_t>(header.fieldCount) * sizeof(Dvr2RuntimeConfigFieldDef);
    if (section.size != expectedSize) {
        if (outError) *outError = "DVR2 runtime config schema section size mismatch";
        return false;
    }

    std::vector<Dvr2RuntimeConfigFieldDef> wireFields(header.fieldCount);
    if (!ReadAll(in, wireFields.data(), wireFields.size() * sizeof(Dvr2RuntimeConfigFieldDef))) {
        if (outError) *outError = "failed to read DVR2 runtime config schema records";
        return false;
    }
    if (DvrFmt::ComputeRuntimeConfigSchemaHash(wireFields) != header.schemaHash) {
        if (outError) *outError = "DVR2 runtime config schema hash mismatch";
        return false;
    }

    outSnapshot.schemaHash = header.schemaHash;
    outSnapshot.fields.reserve(wireFields.size());
    for (const auto& wire : wireFields) {
        if (!IsValidRuntimeConfigValueType(wire.valueType)) {
            if (outError) *outError = "DVR2 runtime config schema contains an invalid value type";
            return false;
        }

        RuntimeConfigField field{};
        field.fieldId = wire.fieldId;
        field.valueType = static_cast<DvrFmt::Dvr2ConfigValueType>(wire.valueType);
        field.category = wire.category;
        field.minValue = wire.minValue;
        field.maxValue = wire.maxValue;
        field.section = FixedStringToString(wire.section, sizeof(wire.section));
        field.key = FixedStringToString(wire.key, sizeof(wire.key));
        field.displayName = FixedStringToString(wire.displayName, sizeof(wire.displayName));
        field.moduleTag = FixedStringToString(wire.moduleTag, sizeof(wire.moduleTag));
        field.unit = FixedStringToString(wire.unit, sizeof(wire.unit));
        outSnapshot.fields.push_back(std::move(field));
    }
    if (!RuntimeConfigSchemaHasUniqueFields(outSnapshot.fields)) {
        if (outError) *outError = "DVR2 runtime config schema contains duplicate or invalid fields";
        return false;
    }
    return true;
}

bool ReadRuntimeConfigValuesSection(std::ifstream& in,
                                    const Dvr2SectionEntry& section,
                                    RuntimeConfigSnapshot& snapshot,
                                    std::string* outError) {
    snapshot.values.clear();
    if (section.size < sizeof(Dvr2RuntimeConfigValuesHeader)) {
        if (outError) *outError = "DVR2 runtime config values section is truncated";
        return false;
    }

    in.seekg(static_cast<std::streamoff>(section.offset), std::ios::beg);
    if (!in.good()) {
        if (outError) *outError = "invalid DVR2 runtime config values offset";
        return false;
    }

    Dvr2RuntimeConfigValuesHeader header{};
    if (!ReadAll(in, &header, sizeof(header))) {
        if (outError) *outError = "failed to read DVR2 runtime config values header";
        return false;
    }
    if (header.recordSize != sizeof(Dvr2RuntimeConfigValueRecord)) {
        if (outError) *outError = "unsupported DVR2 runtime config value record size";
        return false;
    }
    if (header.schemaHash != snapshot.schemaHash) {
        if (outError) *outError = "DVR2 runtime config values schema hash mismatch";
        return false;
    }
    if (header.valueCount != snapshot.fields.size()) {
        if (outError) *outError = "DVR2 runtime config value count mismatch";
        return false;
    }

    const uint64_t expectedSize = sizeof(Dvr2RuntimeConfigValuesHeader) +
        static_cast<uint64_t>(header.valueCount) * sizeof(Dvr2RuntimeConfigValueRecord);
    if (section.size != expectedSize) {
        if (outError) *outError = "DVR2 runtime config values section size mismatch";
        return false;
    }

    snapshot.values.reserve(header.valueCount);
    for (uint16_t i = 0; i < header.valueCount; ++i) {
        Dvr2RuntimeConfigValueRecord wire{};
        if (!ReadAll(in, &wire, sizeof(wire))) {
            if (outError) *outError = "failed to read DVR2 runtime config value record";
            return false;
        }
        if (!IsValidRuntimeConfigValueType(wire.valueType)) {
            if (outError) *outError = "DVR2 runtime config values contain an invalid value type";
            return false;
        }
        if (wire.stringLength >= sizeof(wire.stringValue)) {
            if (outError) *outError = "DVR2 runtime config string value is too long";
            return false;
        }

        RuntimeConfigValue value{};
        value.fieldId = wire.fieldId;
        value.valueType = static_cast<DvrFmt::Dvr2ConfigValueType>(wire.valueType);
        value.valid = (wire.flags & 0x1u) != 0;
        value.rawValue = wire.rawValue;
        value.stringValue.assign(wire.stringValue, wire.stringValue + wire.stringLength);
        snapshot.values.push_back(std::move(value));
    }
    return ValidateRuntimeConfigPayload(snapshot, outError);
}

} // namespace

std::filesystem::path ResolveReplayBinaryPath(const std::filesystem::path& input) {
    return input;
}

bool ReadBinaryFile(const std::filesystem::path& filePath,
                       std::vector<Solvers::HeatmapFrame>& outFrames,
                       int& outVersion,
                       uint32_t* outFlags,
                       std::string* outError,
                       DynamicDebugSchema* outDynamicSchema,
                       std::vector<DynamicDebugFrame>* outDynamicFrames,
                       RuntimeConfigSnapshot* outRuntimeConfig) {
    outFrames.clear();
    outVersion = 0;
    if (outFlags) *outFlags = 0;
    if (outError) outError->clear();
    if (outDynamicSchema) *outDynamicSchema = {};
    if (outDynamicFrames) outDynamicFrames->clear();
    if (outRuntimeConfig) *outRuntimeConfig = {};

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
        if (outError) *outError = "unsupported legacy DVR format; only schema DVR2 .dvrbin files are accepted";
        return false;
    }
    if (!std::equal(dvr2Magic.begin(), dvr2Magic.end(), header.magic)) {
        if (outError) *outError = "invalid DVR2 magic";
        return false;
    }
    if (header.headerSize != sizeof(Dvr2FileHeader) || header.tocOffset != sizeof(Dvr2FileHeader)) {
        if (outError) *outError = "invalid DVR2 header layout";
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
    const auto* frameSchemaSection = FindSection(sections, Dvr2SectionType::FrameSchema);
    const auto* indexSection = FindSection(sections, Dvr2SectionType::Index);
    const auto* framesSection = FindSection(sections, Dvr2SectionType::Frames);
    const auto* dynamicSchemaSection = FindSection(sections, Dvr2SectionType::DynamicDebugSchema);
    const auto* dynamicValuesSection = FindSection(sections, Dvr2SectionType::DynamicDebugValues);
    const auto* runtimeConfigSchemaSection = FindSection(sections, Dvr2SectionType::RuntimeConfigSchema);
    const auto* runtimeConfigValuesSection = FindSection(sections, Dvr2SectionType::RuntimeConfigValues);
    if (!metaSection || !frameSchemaSection || !indexSection || !framesSection) {
        if (outError) *outError = "DVR2 file is missing required schema sections";
        return false;
    }
    if (metaSection->version != 1 || frameSchemaSection->version != 1 || indexSection->version != 1 || framesSection->version != 1) {
        if (outError) *outError = "unsupported DVR2 section version";
        return false;
    }
    if (metaSection->size != sizeof(Dvr2MetaSection)) {
        if (outError) *outError = "DVR2 meta section size mismatch";
        return false;
    }

    in.seekg(static_cast<std::streamoff>(metaSection->offset), std::ios::beg);
    Dvr2MetaSection meta{};
    if (!ReadAll(in, &meta, sizeof(meta))) {
        if (outError) *outError = "failed to read DVR2 meta section";
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
    if (meta.txCount != Frame::kTxCount || meta.rxCount != Frame::kRxCount ||
        meta.masterSuffixWords != Frame::kMasterSuffixWords ||
        meta.slaveSuffixWords != Frame::kSlaveSuffixWords ||
        meta.maxContacts == 0 || meta.maxPeaks == 0 ||
        meta.rawFrameSize != Frame::kTotalFrameSize) {
        if (outError) *outError = "DVR2 meta dimensions are unsupported";
        return false;
    }
    if (meta.frameRecordSize == 0) {
        if (outError) *outError = "DVR2 frame record size is zero";
        return false;
    }

    Dvr2FrameSchemaHeader schemaHeader{};
    std::vector<Dvr2FieldDef> frameFields;
    if (!ReadFrameSchemaSection(in, *frameSchemaSection, schemaHeader, frameFields, outError)) {
        return false;
    }
    if (!ValidateFrameSchema(schemaHeader, meta, frameFields, outError)) {
        return false;
    }

    if (indexSection->size != static_cast<uint64_t>(meta.frameCount) * sizeof(Dvr2IndexEntry)) {
        if (outError) *outError = "DVR2 index section size mismatch";
        return false;
    }
    if (framesSection->size != static_cast<uint64_t>(meta.frameCount) * meta.frameRecordSize) {
        if (outError) *outError = "DVR2 frame section size mismatch";
        return false;
    }

    std::vector<Dvr2IndexEntry> index(meta.frameCount);
    in.seekg(static_cast<std::streamoff>(indexSection->offset), std::ios::beg);
    if (!ReadAll(in, index.data(), index.size() * sizeof(Dvr2IndexEntry))) {
        if (outError) *outError = "failed to read DVR2 index section";
        return false;
    }

    outFrames.reserve(meta.frameCount);
    std::vector<uint8_t> record(meta.frameRecordSize);
    const uint64_t framesEnd = framesSection->offset + framesSection->size;
    for (uint32_t i = 0; i < meta.frameCount; ++i) {
        if (index[i].frameSize != meta.frameRecordSize) {
            if (outError) *outError = "unsupported DVR2 frame size";
            return false;
        }
        if (index[i].frameOffset < framesSection->offset || index[i].frameOffset + index[i].frameSize > framesEnd) {
            if (outError) *outError = "DVR2 frame index points outside frame section";
            return false;
        }
        in.seekg(static_cast<std::streamoff>(index[i].frameOffset), std::ios::beg);
        if (!in.good()) {
            if (outError) *outError = "invalid DVR2 frame offset";
            return false;
        }
        if (!ReadAll(in, record.data(), record.size())) {
            if (outError) *outError = "failed to read DVR2 frame payload";
            return false;
        }
        Solvers::HeatmapFrame frame{};
        if (!PopulateHeatmapFrameFromRecordBytes(record, frameFields, frame, outError)) {
            if (outError && outError->empty()) {
                *outError = "failed to decode DVR2 frame record";
            }
            return false;
        }
        outFrames.push_back(std::move(frame));
    }

    if ((header.flags & DvrFmt::kDvrFlagHasDynamicDebug) != 0) {
        if (!dynamicSchemaSection || !dynamicValuesSection) {
            if (outError) *outError = "DVR2 file is missing dynamic debug sections";
            return false;
        }
        if (dynamicSchemaSection->version != 1 || dynamicValuesSection->version != 1) {
            if (outError) *outError = "unsupported DVR2 dynamic debug section version";
            return false;
        }

        DynamicDebugSchema parsedSchema{};
        if (!ReadDynamicDebugSchemaSection(in, *dynamicSchemaSection, parsedSchema, outError)) {
            return false;
        }

        std::vector<DynamicDebugFrame> parsedFrames;
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

    const bool hasRuntimeConfigFlag = (header.flags & DvrFmt::kDvrFlagHasRuntimeConfig) != 0;
    if (hasRuntimeConfigFlag || runtimeConfigSchemaSection || runtimeConfigValuesSection) {
        if (!runtimeConfigSchemaSection || !runtimeConfigValuesSection) {
            if (outError) *outError = "DVR2 file is missing runtime config sections";
            return false;
        }
        if (runtimeConfigSchemaSection->version != 1 || runtimeConfigValuesSection->version != 1) {
            if (outError) *outError = "unsupported DVR2 runtime config section version";
            return false;
        }

        RuntimeConfigSnapshot parsedRuntimeConfig{};
        if (!ReadRuntimeConfigSchemaSection(in, *runtimeConfigSchemaSection, parsedRuntimeConfig, outError)) {
            return false;
        }
        if (!ReadRuntimeConfigValuesSection(in, *runtimeConfigValuesSection, parsedRuntimeConfig, outError)) {
            return false;
        }
        if (outRuntimeConfig) {
            *outRuntimeConfig = std::move(parsedRuntimeConfig);
        }
    }

    outVersion = static_cast<int>(header.formatVersion);
    if (outFlags) *outFlags = header.flags;
    return true;
}

} // namespace Dvr
