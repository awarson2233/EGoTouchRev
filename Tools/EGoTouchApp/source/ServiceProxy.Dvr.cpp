#include "ServiceProxyInternal.h"
#include "DvrFormat.h"
#include "Logger.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

namespace App {

namespace {

constexpr const char* kExportRootDir = "C:/ProgramData/EGoTouchRev/exports";

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

bool WriteAll(std::ofstream& out, const void* data, size_t bytes) {
    out.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(bytes));
    return static_cast<bool>(out);
}

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
    uint32_t flags = DvrFmt::kDvrFlagHasStylusDiagnostics | DvrFmt::kDvrFlagHasStructuredSuffix;
    for (const auto& frame : frames) {
        if (frame.receiveSystemEpochUs != 0) {
            flags |= DvrFmt::kDvrFlagHasReceiveSystemEpochUs;
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

bool DynamicDebugSamplesMatchSchema(const DvrDynamicDebugSchema& schema,
                                    const Dvr::DvrDynamicDebugFrameSlot& frame) {
    if (frame.sampleCount != schema.fields.size()) return false;
    for (size_t i = 0; i < schema.fields.size(); ++i) {
        if (frame.samples[i].fieldId != schema.fields[i].fieldId) return false;
        if (static_cast<Ipc::DebugValueType>(frame.samples[i].valueType) != schema.fields[i].valueType) return false;
    }
    return true;
}

bool CanPersistDynamicDebug(const std::vector<Dvr::DvrDynamicDebugFrameSlot>* frames,
                            const DvrDynamicDebugSchema* dynamicSchema,
                            size_t expectedFrameCount) {
    if (!frames || !dynamicSchema || dynamicSchema->fields.empty()) return false;
    if (frames->size() != expectedFrameCount) return false;
    if (!HasUniqueDynamicFieldIds(*dynamicSchema)) return false;
    for (const auto& frame : *frames) {
        if (!DynamicDebugSamplesMatchSchema(*dynamicSchema, frame)) {
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

Dvr2FramePayload MakeFramePayload(const Dvr::DvrFrameSlot& src) {
    Dvr2FramePayload dst{};
    auto& frame = dst.frame;
    frame.timestamp = src.timestamp;
    frame.receiveSystemEpochUs = src.receiveSystemEpochUs;
    frame.dvrSeq = src.dvrSeq;
    frame.masterWasRead = src.masterWasRead ? 1 : 0;
    frame.masterSuffixValid = src.masterSuffixValid ? 1 : 0;
    frame.slaveSuffixValid = src.slaveSuffixValid ? 1 : 0;
    std::memcpy(frame.heatmapMatrix, src.heatmapMatrix, sizeof(frame.heatmapMatrix));
    std::memcpy(frame.masterSuffix, src.masterSuffix.words, sizeof(frame.masterSuffix));
    std::memcpy(frame.slaveSuffix, src.slaveSuffix.words, sizeof(frame.slaveSuffix));

    frame.stylus.slaveValid = src.stylus.slaveValid ? 1 : 0;
    frame.stylus.checksumOk = src.stylus.checksumOk ? 1 : 0;
    frame.stylus.tx1BlockValid = src.stylus.tx1BlockValid ? 1 : 0;
    frame.stylus.tx2BlockValid = src.stylus.tx2BlockValid ? 1 : 0;
    frame.stylus.status = src.stylus.status;
    frame.stylus.pressure = src.stylus.pressure;
    std::memcpy(frame.stylus.btRawPressure, src.stylus.btRawPressure, sizeof(frame.stylus.btRawPressure));
    frame.stylus.signalX = src.stylus.signalX;
    frame.stylus.signalY = src.stylus.signalY;
    frame.stylus.maxRawPeak = src.stylus.maxRawPeak;
    frame.stylus.pipelineStage = src.stylus.pipelineStage;
    frame.stylus.point.valid = src.stylus.point.valid ? 1 : 0;
    frame.stylus.point.x = src.stylus.point.x;
    frame.stylus.point.y = src.stylus.point.y;
    frame.stylus.point.reportX = src.stylus.point.reportX;
    frame.stylus.point.reportY = src.stylus.point.reportY;
    frame.stylus.point.pressure = src.stylus.point.pressure;
    frame.stylus.point.rawPressure = src.stylus.point.rawPressure;
    frame.stylus.point.mappedPressure = src.stylus.point.mappedPressure;
    frame.stylus.point.peakTx1 = src.stylus.point.peakTx1;
    frame.stylus.point.peakTx2 = src.stylus.point.peakTx2;
    frame.stylus.point.tx1X = src.stylus.point.tx1X;
    frame.stylus.point.tx1Y = src.stylus.point.tx1Y;
    frame.stylus.point.tx2X = src.stylus.point.tx2X;
    frame.stylus.point.tx2Y = src.stylus.point.tx2Y;
    frame.stylus.point.confidence = src.stylus.point.confidence;

    frame.contactCount = std::min<uint32_t>(src.contactCount, DvrFmt::kMaxContacts);
    for (uint32_t i = 0; i < frame.contactCount; ++i) {
        frame.contacts[i].id = src.contacts[i].id;
        frame.contacts[i].x = src.contacts[i].x;
        frame.contacts[i].y = src.contacts[i].y;
        frame.contacts[i].state = src.contacts[i].state;
        frame.contacts[i].area = src.contacts[i].area;
        frame.contacts[i].signalSum = src.contacts[i].signalSum;
    }

    frame.peakCount = std::min<uint32_t>(src.peakCount, DvrFmt::kMaxPeaks);
    for (uint32_t i = 0; i < frame.peakCount; ++i) {
        frame.peaks[i].r = src.peaks[i].r;
        frame.peaks[i].c = src.peaks[i].c;
        frame.peaks[i].z = src.peaks[i].z;
        frame.peaks[i].id = src.peaks[i].id;
    }

    dst.rawDataLength = std::min<uint16_t>(src.rawDataLength, Frame::kTotalFrameSize);
    if (dst.rawDataLength != 0) {
        std::memcpy(dst.rawData, src.rawData, dst.rawDataLength);
    }
    return dst;
}

uint64_t FieldExtent(const Dvr2FieldDef& field) {
    const uint64_t elementCount = field.elementCount == 0 ? 1 : field.elementCount;
    const uint64_t elementSize = field.elementSize == 0 ? field.size : field.elementSize;
    const uint64_t stride = field.stride == 0 ? elementSize : field.stride;
    return static_cast<uint64_t>(field.offset) + (elementCount - 1) * stride + elementSize;
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

bool PopulateHeatmapFrameFromRecordBytes(const std::vector<uint8_t>& record,
                                         const std::vector<Dvr2FieldDef>& fields,
                                         Solvers::HeatmapFrame& dst,
                                         std::string* outError) {
    dst = {};

    uint8_t u8 = 0;
    uint16_t u16 = 0;
    uint32_t u32 = 0;
    if (!ReadScalarField(record, fields, "timestamp", DvrFmt::Dvr2ValueType::UInt64, dst.timestamp, outError)) return false;
    if (!ReadScalarField(record, fields, "receiveSystemEpochUs", DvrFmt::Dvr2ValueType::UInt64, dst.receiveSystemEpochUs, outError)) return false;
    if (!ReadScalarField(record, fields, "masterWasRead", DvrFmt::Dvr2ValueType::Bool, u8, outError)) return false;
    dst.masterWasRead = u8 != 0;
    if (!ReadScalarField(record, fields, "masterSuffixValid", DvrFmt::Dvr2ValueType::Bool, u8, outError)) return false;
    dst.masterSuffixValid = u8 != 0;
    if (!ReadScalarField(record, fields, "slaveSuffixValid", DvrFmt::Dvr2ValueType::Bool, u8, outError)) return false;
    dst.slaveSuffixValid = u8 != 0;
    if (!CopyContiguousField(record, fields, "heatmapMatrix", DvrFmt::Dvr2ValueType::Int16, DvrFmt::Dvr2FieldRank::Matrix, sizeof(dst.heatmapMatrix), dst.heatmapMatrix, outError)) return false;
    if (!CopyContiguousField(record, fields, "masterSuffix.words", DvrFmt::Dvr2ValueType::UInt16, DvrFmt::Dvr2FieldRank::Array, sizeof(dst.masterSuffix.words), dst.masterSuffix.words, outError)) return false;
    if (!CopyContiguousField(record, fields, "slaveSuffix.words", DvrFmt::Dvr2ValueType::UInt16, DvrFmt::Dvr2FieldRank::Array, sizeof(dst.slaveSuffix.words), dst.slaveSuffix.words, outError)) return false;

    if (!ReadScalarField(record, fields, "stylus.slaveValid", DvrFmt::Dvr2ValueType::Bool, u8, outError)) return false;
    dst.stylus.input.slaveValid = u8 != 0;
    if (!ReadScalarField(record, fields, "stylus.checksumOk", DvrFmt::Dvr2ValueType::Bool, u8, outError)) return false;
    dst.stylus.input.checksumOk = u8 != 0;
    if (!ReadScalarField(record, fields, "stylus.tx1BlockValid", DvrFmt::Dvr2ValueType::Bool, u8, outError)) return false;
    dst.stylus.input.tx1BlockValid = u8 != 0;
    if (!ReadScalarField(record, fields, "stylus.tx2BlockValid", DvrFmt::Dvr2ValueType::Bool, u8, outError)) return false;
    dst.stylus.input.tx2BlockValid = u8 != 0;
    if (!ReadScalarField(record, fields, "stylus.status", DvrFmt::Dvr2ValueType::UInt32, dst.stylus.input.status, outError)) return false;
    if (!ReadScalarField(record, fields, "stylus.pressure", DvrFmt::Dvr2ValueType::UInt16, dst.stylus.output.pressure, outError)) return false;
    if (!CopyContiguousField(record, fields, "stylus.btRawPressure", DvrFmt::Dvr2ValueType::UInt16, DvrFmt::Dvr2FieldRank::Array, sizeof(dst.stylus.input.btSample.rawPressure), dst.stylus.input.btSample.rawPressure.data(), outError)) return false;
    if (!ReadScalarField(record, fields, "stylus.signalX", DvrFmt::Dvr2ValueType::UInt16, dst.stylus.interop.signalX, outError)) return false;
    if (!ReadScalarField(record, fields, "stylus.signalY", DvrFmt::Dvr2ValueType::UInt16, dst.stylus.interop.signalY, outError)) return false;
    if (!ReadScalarField(record, fields, "stylus.maxRawPeak", DvrFmt::Dvr2ValueType::UInt16, dst.stylus.interop.maxRawPeak, outError)) return false;
    if (!ReadScalarField(record, fields, "stylus.pipelineStage", DvrFmt::Dvr2ValueType::UInt8, dst.stylus.output.pipelineStage, outError)) return false;

    if (!ReadScalarField(record, fields, "stylus.point.valid", DvrFmt::Dvr2ValueType::Bool, u8, outError)) return false;
    dst.stylus.output.point.valid = u8 != 0;
    if (!ReadScalarField(record, fields, "stylus.point.x", DvrFmt::Dvr2ValueType::Float32, dst.stylus.output.point.x, outError)) return false;
    if (!ReadScalarField(record, fields, "stylus.point.y", DvrFmt::Dvr2ValueType::Float32, dst.stylus.output.point.y, outError)) return false;
    if (!ReadScalarField(record, fields, "stylus.point.reportX", DvrFmt::Dvr2ValueType::UInt16, dst.stylus.output.point.reportX, outError)) return false;
    if (!ReadScalarField(record, fields, "stylus.point.reportY", DvrFmt::Dvr2ValueType::UInt16, dst.stylus.output.point.reportY, outError)) return false;
    if (!ReadScalarField(record, fields, "stylus.point.pressure", DvrFmt::Dvr2ValueType::UInt16, dst.stylus.output.point.pressure, outError)) return false;
    if (!ReadScalarField(record, fields, "stylus.point.rawPressure", DvrFmt::Dvr2ValueType::UInt16, dst.stylus.output.point.rawPressure, outError)) return false;
    if (!ReadScalarField(record, fields, "stylus.point.mappedPressure", DvrFmt::Dvr2ValueType::UInt16, dst.stylus.output.point.mappedPressure, outError)) return false;
    if (!ReadScalarField(record, fields, "stylus.point.peakTx1", DvrFmt::Dvr2ValueType::UInt16, dst.stylus.output.point.peakTx1, outError)) return false;
    if (!ReadScalarField(record, fields, "stylus.point.peakTx2", DvrFmt::Dvr2ValueType::UInt16, dst.stylus.output.point.peakTx2, outError)) return false;
    if (!ReadScalarField(record, fields, "stylus.point.tx1X", DvrFmt::Dvr2ValueType::Float32, dst.stylus.output.point.tx1X, outError)) return false;
    if (!ReadScalarField(record, fields, "stylus.point.tx1Y", DvrFmt::Dvr2ValueType::Float32, dst.stylus.output.point.tx1Y, outError)) return false;
    if (!ReadScalarField(record, fields, "stylus.point.tx2X", DvrFmt::Dvr2ValueType::Float32, dst.stylus.output.point.tx2X, outError)) return false;
    if (!ReadScalarField(record, fields, "stylus.point.tx2Y", DvrFmt::Dvr2ValueType::Float32, dst.stylus.output.point.tx2Y, outError)) return false;
    if (!ReadScalarField(record, fields, "stylus.point.confidence", DvrFmt::Dvr2ValueType::Float32, dst.stylus.output.point.confidence, outError)) return false;

    if (!ReadScalarField(record, fields, "contactCount", DvrFmt::Dvr2ValueType::UInt32, u32, outError)) return false;
    const uint32_t contactCount = std::min<uint32_t>(u32, DvrFmt::kMaxContacts);
    dst.contacts.clear();
    dst.contacts.reserve(contactCount);
    for (uint32_t i = 0; i < contactCount; ++i) {
        Solvers::TouchContact tc{};
        if (!ReadStridedField(record, fields, "contacts[].id", DvrFmt::Dvr2ValueType::Int32, i, tc.id, outError)) return false;
        if (!ReadStridedField(record, fields, "contacts[].x", DvrFmt::Dvr2ValueType::Float32, i, tc.x, outError)) return false;
        if (!ReadStridedField(record, fields, "contacts[].y", DvrFmt::Dvr2ValueType::Float32, i, tc.y, outError)) return false;
        if (!ReadStridedField(record, fields, "contacts[].state", DvrFmt::Dvr2ValueType::Int32, i, tc.state, outError)) return false;
        if (!ReadStridedField(record, fields, "contacts[].area", DvrFmt::Dvr2ValueType::Int32, i, tc.area, outError)) return false;
        if (!ReadStridedField(record, fields, "contacts[].signalSum", DvrFmt::Dvr2ValueType::Int32, i, tc.signalSum, outError)) return false;
        dst.contacts.push_back(tc);
    }

#if EGOTOUCH_DIAG
    if (!ReadScalarField(record, fields, "peakCount", DvrFmt::Dvr2ValueType::UInt32, u32, outError)) return false;
    const uint32_t peakCount = std::min<uint32_t>(u32, DvrFmt::kMaxPeaks);
    dst.peaks.clear();
    dst.peaks.reserve(peakCount);
    for (uint32_t i = 0; i < peakCount; ++i) {
        Solvers::TouchPeak tp{};
        if (!ReadStridedField(record, fields, "peaks[].r", DvrFmt::Dvr2ValueType::Int32, i, tp.r, outError)) return false;
        if (!ReadStridedField(record, fields, "peaks[].c", DvrFmt::Dvr2ValueType::Int32, i, tp.c, outError)) return false;
        if (!ReadStridedField(record, fields, "peaks[].z", DvrFmt::Dvr2ValueType::Int16, i, tp.z, outError)) return false;
        if (!ReadStridedField(record, fields, "peaks[].id", DvrFmt::Dvr2ValueType::UInt8, i, tp.id, outError)) return false;
        dst.peaks.push_back(tp);
    }
#endif

    if (!ReadScalarField(record, fields, "rawDataLength", DvrFmt::Dvr2ValueType::UInt16, u16, outError)) return false;
    const auto* rawField = RequireField(fields, "rawData", outError);
    if (!rawField) return false;
    if (!ValidateField(*rawField, record.size(), "rawData", DvrFmt::Dvr2ValueType::UInt8, DvrFmt::Dvr2FieldRank::Array, outError)) return false;
    if (rawField->size != Frame::kTotalFrameSize) {
        if (outError) *outError = "DVR2 frame schema rawData size mismatch";
        return false;
    }
    const size_t rawLen = std::min<size_t>(u16, Frame::kTotalFrameSize);
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
    if (schemaHeader.fieldCount == 0 || fields.empty()) {
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
    for (const auto& path : {
             "timestamp", "receiveSystemEpochUs", "dvrSeq", "masterWasRead",
             "masterSuffixValid", "slaveSuffixValid", "heatmapMatrix",
             "masterSuffix.words", "slaveSuffix.words", "stylus.slaveValid",
             "stylus.checksumOk", "stylus.tx1BlockValid", "stylus.tx2BlockValid",
             "stylus.status", "stylus.pressure", "stylus.btRawPressure",
             "stylus.signalX", "stylus.signalY", "stylus.maxRawPeak",
             "stylus.pipelineStage", "stylus.point.valid", "stylus.point.x",
             "stylus.point.y", "stylus.point.reportX", "stylus.point.reportY",
             "stylus.point.pressure", "stylus.point.rawPressure",
             "stylus.point.mappedPressure", "stylus.point.peakTx1",
             "stylus.point.peakTx2", "stylus.point.tx1X", "stylus.point.tx1Y",
             "stylus.point.tx2X", "stylus.point.tx2Y", "stylus.point.confidence",
             "contacts[]", "contacts[].id", "contacts[].x", "contacts[].y",
             "contacts[].state", "contacts[].area", "contacts[].signalSum",
             "contactCount", "peaks[]", "peaks[].r", "peaks[].c", "peaks[].z",
             "peaks[].id", "peakCount", "rawDataLength", "rawData"}) {
        if (!RequireField(fields, path, outError)) return false;
    }
    return true;
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
                                   std::vector<DvrDynamicDebugFrame>& outFrames,
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
                        const std::vector<Dvr::DvrDynamicDebugFrameSlot>* dynamicFrames,
                        uint32_t* outFlags) {
    if (frames.empty()) return false;

    uint32_t flags = ComputeDvrBinaryFlags(frames);

    std::vector<Dvr2FramePayload> records;
    records.reserve(frames.size());
    for (const auto& frame : frames) {
        records.push_back(MakeFramePayload(frame));
    }

    const auto frameSchema = DvrFmt::BuildFrameSchema();
    const uint32_t frameSchemaHash = DvrFmt::ComputeFieldSchemaHash(frameSchema);
    Dvr2FrameSchemaHeader frameSchemaHeader{};
    frameSchemaHeader.schemaHash = frameSchemaHash;
    frameSchemaHeader.fieldCount = static_cast<uint32_t>(frameSchema.size());
    frameSchemaHeader.fieldRecordSize = sizeof(Dvr2FieldDef);
    frameSchemaHeader.frameRecordSize = sizeof(Dvr2FramePayload);

    DvrDynamicDebugSchema effectiveDynamicSchema{};
    if (dynamicSchema) {
        effectiveDynamicSchema = *dynamicSchema;
    }

    const bool hasDynamicDebug = CanPersistDynamicDebug(dynamicFrames, dynamicSchema, frames.size());
    if (hasDynamicDebug) {
        flags |= DvrFmt::kDvrFlagHasDynamicDebug;
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

    Dvr2DynamicDebugSchemaHeader dynamicSchemaHeader{};
    std::vector<uint8_t> dynamicValuesBlob;
    if (hasDynamicDebug) {
        Dvr2DynamicDebugValuesHeader valuesHeader{};
        valuesHeader.frameCount = static_cast<uint32_t>(frames.size());
        dynamicValuesBlob.insert(dynamicValuesBlob.end(),
                                 reinterpret_cast<const uint8_t*>(&valuesHeader),
                                 reinterpret_cast<const uint8_t*>(&valuesHeader) + sizeof(valuesHeader));
        for (const auto& frame : *dynamicFrames) {
            Dvr2DynamicDebugFrameHeader frameHeader{};
            frameHeader.sampleCount = frame.sampleCount;
            dynamicValuesBlob.insert(dynamicValuesBlob.end(),
                                     reinterpret_cast<const uint8_t*>(&frameHeader),
                                     reinterpret_cast<const uint8_t*>(&frameHeader) + sizeof(frameHeader));
            for (uint16_t i = 0; i < frame.sampleCount; ++i) {
                const auto& sample = frame.samples[i];
                Dvr2DynamicDebugSample wire{};
                wire.fieldId = sample.fieldId;
                wire.valueType = sample.valueType;
                wire.flags = sample.valid ? 0x1u : 0x0u;
                wire.rawValue = sample.rawValue;
                dynamicValuesBlob.insert(dynamicValuesBlob.end(),
                                         reinterpret_cast<const uint8_t*>(&wire),
                                         reinterpret_cast<const uint8_t*>(&wire) + sizeof(wire));
            }
        }
    }

    Dvr2FileHeader header{};
    const auto magic = MakeDvr2Magic();
    std::copy(magic.begin(), magic.end(), header.magic);
    header.formatVersion = static_cast<uint16_t>(DvrFmt::kCurrentDvrFormatVersion);
    header.sectionCount = hasDynamicDebug ? 6u : 4u;
    header.tocOffset = sizeof(Dvr2FileHeader);
    header.flags = flags;

    const uint64_t tocSize = static_cast<uint64_t>(header.sectionCount) * sizeof(Dvr2SectionEntry);
    const uint64_t metaOffset = header.tocOffset + tocSize;
    const uint64_t metaSize = sizeof(Dvr2MetaSection);
    const uint64_t frameSchemaOffset = metaOffset + metaSize;
    const uint64_t frameSchemaSize = sizeof(Dvr2FrameSchemaHeader) + static_cast<uint64_t>(frameSchema.size()) * sizeof(Dvr2FieldDef);
    const uint64_t indexOffset = frameSchemaOffset + frameSchemaSize;
    const uint64_t indexSize = static_cast<uint64_t>(records.size()) * sizeof(Dvr2IndexEntry);
    const uint64_t framesOffset = indexOffset + indexSize;
    const uint64_t framesSize = static_cast<uint64_t>(records.size()) * sizeof(Dvr2FramePayload);
    uint64_t nextOffset = framesOffset + framesSize;

    std::vector<Dvr2SectionEntry> sections;
    sections.reserve(header.sectionCount);
    sections.push_back({static_cast<uint32_t>(Dvr2SectionType::Meta), 1, metaOffset, metaSize});
    sections.push_back({static_cast<uint32_t>(Dvr2SectionType::FrameSchema), 1, frameSchemaOffset, frameSchemaSize});
    sections.push_back({static_cast<uint32_t>(Dvr2SectionType::Index), 1, indexOffset, indexSize});
    sections.push_back({static_cast<uint32_t>(Dvr2SectionType::Frames), 1, framesOffset, framesSize});

    if (hasDynamicDebug) {
        const uint64_t dynamicSchemaOffset = nextOffset;
        const uint64_t dynamicSchemaSize = sizeof(Dvr2DynamicDebugSchemaHeader) +
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

    Dvr2MetaSection meta{};
    meta.frameCount = static_cast<uint32_t>(records.size());
    meta.flags = flags;
    meta.frameRecordSize = sizeof(Dvr2FramePayload);
    meta.frameSchemaHash = frameSchemaHash;

    std::vector<Dvr2IndexEntry> index(records.size());
    for (size_t i = 0; i < records.size(); ++i) {
        index[i].timestamp = records[i].frame.timestamp;
        index[i].receiveSystemEpochUs = records[i].frame.receiveSystemEpochUs;
        index[i].frameOffset = framesOffset + static_cast<uint64_t>(i) * sizeof(Dvr2FramePayload);
        index[i].frameSize = static_cast<uint32_t>(sizeof(Dvr2FramePayload));
    }

    std::ofstream out(filePath, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return false;
    if (!WriteAll(out, &header, sizeof(header))) return false;
    if (!WriteAll(out, sections.data(), sections.size() * sizeof(Dvr2SectionEntry))) return false;
    if (!WriteAll(out, &meta, sizeof(meta))) return false;
    if (!WriteAll(out, &frameSchemaHeader, sizeof(frameSchemaHeader))) return false;
    if (!WriteAll(out, frameSchema.data(), frameSchema.size() * sizeof(Dvr2FieldDef))) return false;
    if (!WriteAll(out, index.data(), index.size() * sizeof(Dvr2IndexEntry))) return false;
    if (!WriteAll(out, records.data(), records.size() * sizeof(Dvr2FramePayload))) return false;
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
        if (outError) *outError = "unsupported legacy DVR format; only schema DVR2 .dvrbin files are accepted";
        return false;
    }
    if (!std::equal(dvr2Magic.begin(), dvr2Magic.end(), header.magic)) {
        if (outError) *outError = "invalid DVR2 magic";
        return false;
    }
    if (header.formatVersion != DvrFmt::kCurrentDvrFormatVersion) {
        if (outError) *outError = "unsupported DVR2 format version";
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
        meta.maxContacts != DvrFmt::kMaxContacts || meta.maxPeaks != DvrFmt::kMaxPeaks ||
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
        for (int i = 0; i < 4; ++i) {
            slaveRows.push_back({"BtRawPressure" + std::to_string(i), std::to_string(stylusInput.btSample.rawPressure[static_cast<size_t>(i)])});
        }
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
        std::vector<Dvr::DvrDynamicDebugFrameSlot> dynamicFrameSlots;
        if (m_dvrDynamicDebugBuffer) {
            dynamicFrameSlots = m_dvrDynamicDebugBuffer->GetSnapshot();
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

        std::vector<Dvr::DvrDynamicDebugFrameSlot> preTriggerDynamicFrames;
        const std::vector<Dvr::DvrDynamicDebugFrameSlot>* dynamicFramesForExport = nullptr;
        if (!dynamicFrameSlots.empty()) {
            bool allDynamicFramesMatched = true;
            preTriggerDynamicFrames.reserve(preTriggerFrames.size());
            for (const auto& frame : preTriggerFrames) {
                const auto match = std::find_if(dynamicFrameSlots.begin(), dynamicFrameSlots.end(), [seq = frame.dvrSeq](const auto& dynamicFrame) {
                    return dynamicFrame.dvrSeq == seq;
                });
                if (match == dynamicFrameSlots.end()) {
                    allDynamicFramesMatched = false;
                    break;
                }
                preTriggerDynamicFrames.push_back(*match);
            }
            if (allDynamicFramesMatched) {
                dynamicFramesForExport = &preTriggerDynamicFrames;
            } else {
                LOG_WARN("App", "TriggerDvrBinaryExport", "IPC", "Dynamic debug frames did not cover all exported DVR frames; exporting static frame schema only.");
            }
        }

        const fs::path replayBinPath = dir / (datasetName + ".dvrbin");
        const auto dynamicSchema = GetCurrentDvrDynamicDebugSchema();
        if (!WriteDvrBinaryFile(replayBinPath, preTriggerFrames, &dynamicSchema, dynamicFramesForExport, nullptr)) {
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
