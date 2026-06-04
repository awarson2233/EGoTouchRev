#include "DvrBinaryIO.h"
#include "DvrFormat.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <fstream>
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

bool WriteAll(std::ofstream& out, const void* data, size_t bytes) {
    out.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(bytes));
    return static_cast<bool>(out);
}

std::array<char, 8> MakeDvr2Magic() {
    return DvrFmt::kDvr2Magic;
}

uint32_t ComputeDvrBinaryFlags(const std::vector<DvrFrameSlot>& frames) {
    uint32_t flags = DvrFmt::kDvrFlagHasStylusDiagnostics | DvrFmt::kDvrFlagHasStructuredSuffix;
    for (const auto& frame : frames) {
        if (frame.receiveSystemEpochUs != 0) {
            flags |= DvrFmt::kDvrFlagHasReceiveSystemEpochUs;
        }
    }
    return flags;
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

bool DynamicDebugSamplesMatchSchema(const DynamicDebugSchema& schema,
                                    const DvrDynamicDebugFrameSlot& frame) {
    if (frame.sampleCount != schema.fields.size()) return false;
    for (size_t i = 0; i < schema.fields.size(); ++i) {
        if (frame.samples[i].fieldId != schema.fields[i].fieldId) return false;
        if (static_cast<Ipc::DebugValueType>(frame.samples[i].valueType) != schema.fields[i].valueType) return false;
    }
    return true;
}

bool CanPersistDynamicDebug(const std::vector<DvrDynamicDebugFrameSlot>* frames,
                            const DynamicDebugSchema* dynamicSchema,
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

void CopyRawGridBlockToWire(const Solvers::Stylus::Hpp3::FreqBlock& src,
                            DvrFmt::Dvr2StylusRawGridBlockRecord& dst) {
    dst.anchorRow = src.anchorRow;
    dst.anchorCol = src.anchorCol;
    std::memcpy(dst.grid, src.grid, sizeof(dst.grid));
    dst.valid = src.valid ? 1 : 0;
}

Dvr2FramePayload MakeFramePayload(const DvrFrameSlot& src) {
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
    std::memcpy(frame.touchZones, src.touchZones, sizeof(frame.touchZones));
    std::memcpy(frame.peakZones, src.peakZones, sizeof(frame.peakZones));

    for (size_t i = 0; i < DvrFmt::kTouchPacketCount; ++i) {
        frame.touchPackets[i].valid = src.touchPackets[i].valid ? 1 : 0;
        frame.touchPackets[i].reportId = src.touchPackets[i].reportId;
        frame.touchPackets[i].length = src.touchPackets[i].length;
        std::memcpy(frame.touchPackets[i].bytes, src.touchPackets[i].bytes, sizeof(frame.touchPackets[i].bytes));
    }

    frame.stylus.slaveValid = src.stylus.slaveValid ? 1 : 0;
    frame.stylus.checksumOk = src.stylus.checksumOk ? 1 : 0;
    frame.stylus.slaveWordOffset = src.stylus.slaveWordOffset;
    frame.stylus.tx1BlockValid = src.stylus.tx1BlockValid ? 1 : 0;
    frame.stylus.tx2BlockValid = src.stylus.tx2BlockValid ? 1 : 0;
    frame.stylus.outputValid = src.stylus.outputValid ? 1 : 0;
    frame.stylus.inRange = src.stylus.inRange ? 1 : 0;
    frame.stylus.tipDown = src.stylus.tipDown ? 1 : 0;
    frame.stylus.status = src.stylus.status;
    frame.stylus.checksum16 = src.stylus.checksum16;
    frame.stylus.pressure = src.stylus.pressure;
    std::memcpy(frame.stylus.btPressure, src.stylus.btPressure, sizeof(frame.stylus.btPressure));
    std::memcpy(frame.stylus.btRawPressure, src.stylus.btRawPressure, sizeof(frame.stylus.btRawPressure));
    frame.stylus.btSeq = src.stylus.btSeq;
    frame.stylus.btFreq1 = src.stylus.btFreq1;
    frame.stylus.btFreq2 = src.stylus.btFreq2;
    frame.stylus.btHasSample = src.stylus.btHasSample ? 1 : 0;
    frame.stylus.btHasFreq = src.stylus.btHasFreq ? 1 : 0;
    frame.stylus.signalX = src.stylus.signalX;
    frame.stylus.signalY = src.stylus.signalY;
    frame.stylus.maxRawPeak = src.stylus.maxRawPeak;
    frame.stylus.pipelineStage = src.stylus.pipelineStage;
    frame.stylus.recheckEnabled = src.stylus.recheckEnabled ? 1 : 0;
    frame.stylus.recheckPassed = src.stylus.recheckPassed ? 1 : 0;
    frame.stylus.recheckOverlap = src.stylus.recheckOverlap ? 1 : 0;
    frame.stylus.recheckThreshold = src.stylus.recheckThreshold;
    frame.stylus.recheckThresholdMulti = src.stylus.recheckThresholdMulti;
    frame.stylus.touchNullLike = src.stylus.touchNullLike ? 1 : 0;
    frame.stylus.touchSuppressActive = src.stylus.touchSuppressActive ? 1 : 0;
    frame.stylus.touchSuppressFrames = src.stylus.touchSuppressFrames;
    frame.stylus.pressureIsReal = src.stylus.pressureIsReal ? 1 : 0;
    frame.stylus.predictedAgeFrames = src.stylus.predictedAgeFrames;
    frame.stylus.outputConfidence = src.stylus.outputConfidence;
    CopyRawGridBlockToWire(src.stylus.rawGrid.tx1, frame.stylus.rawGrid.tx1);
    CopyRawGridBlockToWire(src.stylus.rawGrid.tx2, frame.stylus.rawGrid.tx2);
    frame.stylus.packet.valid = src.stylus.packet.valid ? 1 : 0;
    frame.stylus.packet.reportId = src.stylus.packet.reportId;
    frame.stylus.packet.length = src.stylus.packet.length;
    std::memcpy(frame.stylus.packet.bytes, src.stylus.packet.bytes, sizeof(frame.stylus.packet.bytes));
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
    frame.stylus.point.tiltValid = src.stylus.point.tiltValid ? 1 : 0;
    frame.stylus.point.preTiltX = src.stylus.point.preTiltX;
    frame.stylus.point.preTiltY = src.stylus.point.preTiltY;
    frame.stylus.point.tiltX = src.stylus.point.tiltX;
    frame.stylus.point.tiltY = src.stylus.point.tiltY;
    frame.stylus.point.tiltMagnitude = src.stylus.point.tiltMagnitude;
    frame.stylus.point.tiltAzimuthDeg = src.stylus.point.tiltAzimuthDeg;
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
        frame.contacts[i].sizeMm = src.contacts[i].sizeMm;
        frame.contacts[i].edgeDistX = src.contacts[i].edgeDistX;
        frame.contacts[i].edgeDistY = src.contacts[i].edgeDistY;
        frame.contacts[i].rawXBeforeEC = src.contacts[i].rawXBeforeEC;
        frame.contacts[i].rawYBeforeEC = src.contacts[i].rawYBeforeEC;
        frame.contacts[i].prevIndex = src.contacts[i].prevIndex;
        frame.contacts[i].debugFlags = src.contacts[i].debugFlags;
        frame.contacts[i].edgeFlags = src.contacts[i].edgeFlags;
        frame.contacts[i].ecFlags = src.contacts[i].ecFlags;
        frame.contacts[i].lifeFlags = src.contacts[i].lifeFlags;
        frame.contacts[i].reportFlags = src.contacts[i].reportFlags;
        frame.contacts[i].reportEvent = src.contacts[i].reportEvent;
        frame.contacts[i].isEdge = src.contacts[i].isEdge;
        frame.contacts[i].isReported = src.contacts[i].isReported;
        frame.contacts[i].centroidEdgeFlags = src.contacts[i].centroidEdgeFlags;
        frame.contacts[i].ecWidthX = src.contacts[i].ecWidthX;
        frame.contacts[i].ecWidthY = src.contacts[i].ecWidthY;
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

} // namespace

bool WriteBinaryFile(const std::filesystem::path& filePath,
                        const std::vector<DvrFrameSlot>& frames,
                        const DynamicDebugSchema* dynamicSchema,
                        const std::vector<DvrDynamicDebugFrameSlot>* dynamicFrames,
                        const RuntimeConfigSnapshot* runtimeConfig,
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

    DynamicDebugSchema effectiveDynamicSchema{};
    if (dynamicSchema) {
        effectiveDynamicSchema = *dynamicSchema;
    }

    const bool hasDynamicDebug = CanPersistDynamicDebug(dynamicFrames, dynamicSchema, frames.size());
    if (hasDynamicDebug) {
        flags |= DvrFmt::kDvrFlagHasDynamicDebug;
    }

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

    const bool hasRuntimeConfig = CanPersistRuntimeConfig(runtimeConfig);
    if (hasRuntimeConfig) {
        flags |= DvrFmt::kDvrFlagHasRuntimeConfig;
    }
    if (outFlags) *outFlags = flags;

    Dvr2RuntimeConfigSchemaHeader runtimeConfigSchemaHeader{};
    Dvr2RuntimeConfigValuesHeader runtimeConfigValuesHeader{};
    std::vector<Dvr2RuntimeConfigFieldDef> runtimeConfigFieldRecords;
    std::vector<Dvr2RuntimeConfigValueRecord> runtimeConfigValueRecords;
    if (hasRuntimeConfig) {
        runtimeConfigFieldRecords.reserve(runtimeConfig->fields.size());
        for (const auto& field : runtimeConfig->fields) {
            runtimeConfigFieldRecords.push_back(MakeRuntimeConfigFieldDef(field));
        }
        const uint32_t configSchemaHash = DvrFmt::ComputeRuntimeConfigSchemaHash(runtimeConfigFieldRecords);

        runtimeConfigValueRecords.reserve(runtimeConfig->values.size());
        for (const auto& value : runtimeConfig->values) {
            runtimeConfigValueRecords.push_back(MakeRuntimeConfigValueRecord(value));
        }

        runtimeConfigSchemaHeader.fieldCount = static_cast<uint16_t>(runtimeConfigFieldRecords.size());
        runtimeConfigSchemaHeader.schemaHash = configSchemaHash;
        runtimeConfigSchemaHeader.recordSize = sizeof(Dvr2RuntimeConfigFieldDef);
        runtimeConfigValuesHeader.valueCount = static_cast<uint16_t>(runtimeConfigValueRecords.size());
        runtimeConfigValuesHeader.recordSize = sizeof(Dvr2RuntimeConfigValueRecord);
        runtimeConfigValuesHeader.schemaHash = configSchemaHash;
    }

    Dvr2FileHeader header{};
    const auto magic = MakeDvr2Magic();
    std::copy(magic.begin(), magic.end(), header.magic);
    header.formatVersion = static_cast<uint16_t>(DvrFmt::kCurrentDvrFormatVersion);
    header.sectionCount = 4u + (hasDynamicDebug ? 2u : 0u) + (hasRuntimeConfig ? 2u : 0u);
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

    if (hasRuntimeConfig) {
        const uint64_t runtimeConfigSchemaOffset = nextOffset;
        const uint64_t runtimeConfigSchemaSize = sizeof(Dvr2RuntimeConfigSchemaHeader) +
            static_cast<uint64_t>(runtimeConfigFieldRecords.size()) * sizeof(Dvr2RuntimeConfigFieldDef);
        nextOffset += runtimeConfigSchemaSize;
        const uint64_t runtimeConfigValuesOffset = nextOffset;
        const uint64_t runtimeConfigValuesSize = sizeof(Dvr2RuntimeConfigValuesHeader) +
            static_cast<uint64_t>(runtimeConfigValueRecords.size()) * sizeof(Dvr2RuntimeConfigValueRecord);
        nextOffset += runtimeConfigValuesSize;
        sections.push_back({static_cast<uint32_t>(Dvr2SectionType::RuntimeConfigSchema), 1, runtimeConfigSchemaOffset, runtimeConfigSchemaSize});
        sections.push_back({static_cast<uint32_t>(Dvr2SectionType::RuntimeConfigValues), 1, runtimeConfigValuesOffset, runtimeConfigValuesSize});
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
    if (hasRuntimeConfig) {
        if (!WriteAll(out, &runtimeConfigSchemaHeader, sizeof(runtimeConfigSchemaHeader))) return false;
        if (!runtimeConfigFieldRecords.empty() && !WriteAll(out, runtimeConfigFieldRecords.data(), runtimeConfigFieldRecords.size() * sizeof(Dvr2RuntimeConfigFieldDef))) return false;
        if (!WriteAll(out, &runtimeConfigValuesHeader, sizeof(runtimeConfigValuesHeader))) return false;
        if (!runtimeConfigValueRecords.empty() && !WriteAll(out, runtimeConfigValueRecords.data(), runtimeConfigValueRecords.size() * sizeof(Dvr2RuntimeConfigValueRecord))) return false;
    }
    return true;
}

} // namespace Dvr
