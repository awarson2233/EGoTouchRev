#include "DvrCsvExport.h"
#include "DvrFormat.h"
#include "TouchPipeline.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

namespace Dvr {

namespace {

namespace DvrFmt = Dvr::Format;

size_t FramePeakCount(const Solvers::HeatmapFrame& frame) {
#if EGOTOUCH_DIAG
    return frame.touch.debug.peaks.size();
#else
    (void)frame;
    return 0;
#endif
}

void WritePeaksCsvSection(std::ostream& out, const Solvers::HeatmapFrame& frame) {
    out << "--- Peaks ---\n";
    out << "Count," << FramePeakCount(frame) << "\n";
    out << "R,C,Z,ID\n";
#if EGOTOUCH_DIAG
    for (const auto& pk : frame.touch.debug.peaks) {
        out << pk.r << ',' << pk.c << ',' << pk.z << ',' << static_cast<unsigned int>(pk.id) << "\n";
    }
#endif
    out << "\n";
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

std::string FormatRuntimeConfigValue(const RuntimeConfigValue& value) {
    if (!value.valid) return "N/A";

    std::ostringstream out;
    switch (value.valueType) {
    case DvrFmt::Dvr2ConfigValueType::Bool:
        out << (((value.rawValue & 0x1ull) != 0) ? 1 : 0);
        break;
    case DvrFmt::Dvr2ConfigValueType::Int32:
        out << static_cast<int32_t>(static_cast<uint32_t>(value.rawValue & 0xFFFFFFFFu));
        break;
    case DvrFmt::Dvr2ConfigValueType::UInt8:
        out << static_cast<unsigned int>(value.rawValue & 0xFFu);
        break;
    case DvrFmt::Dvr2ConfigValueType::UInt16:
        out << static_cast<unsigned int>(value.rawValue & 0xFFFFu);
        break;
    case DvrFmt::Dvr2ConfigValueType::UInt32:
        out << static_cast<uint32_t>(value.rawValue & 0xFFFFFFFFu);
        break;
    case DvrFmt::Dvr2ConfigValueType::Float32: {
        const uint32_t bits = static_cast<uint32_t>(value.rawValue & 0xFFFFFFFFu);
        float fv = 0.0f;
        std::memcpy(&fv, &bits, sizeof(fv));
        out << fv;
        break;
    }
    case DvrFmt::Dvr2ConfigValueType::Float64: {
        const uint64_t bits = value.rawValue;
        double dv = 0.0;
        std::memcpy(&dv, &bits, sizeof(dv));
        out << dv;
        break;
    }
    case DvrFmt::Dvr2ConfigValueType::String:
        out << value.stringValue;
        break;
    }
    return out.str();
}

void WriteRuntimeConfigCsvSection(std::ostream& out,
                                  const RuntimeConfigSnapshot& snapshot) {
    if (snapshot.Empty() || !RuntimeConfigValuesMatchSchema(snapshot)) return;

    out << "--- Config Parameters ---\n";
    out << "ConfigSchemaHash," << snapshot.schemaHash << "\n";
    out << "ConfigFieldCount," << snapshot.fields.size() << "\n";

    std::string currentSection;
    for (size_t i = 0; i < snapshot.fields.size(); ++i) {
        const auto& field = snapshot.fields[i];
        const auto& value = snapshot.values[i];
        if (field.section != currentSection) {
            currentSection = field.section;
            out << '[' << currentSection << "]\n";
        }
        out << field.key << ',' << FormatRuntimeConfigValue(value) << "\n";
    }
    out << "\n";
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

const DynamicDebugSample* FindDynamicSample(const DynamicDebugFrame* frame, uint16_t fieldId) {
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

std::string FormatDynamicDebugValue(const DynamicDebugSample* sample) {
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
                      const DynamicDebugFrame* dynamicFrame) {
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
                      const DynamicDebugSchema* dynamicSchema,
                      const DynamicDebugFrame* dynamicFrame) {
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

} // namespace

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
                       const DynamicDebugSchema* dynamicSchema,
                       const DynamicDebugFrame* dynamicFrame,
                       const RuntimeConfigSnapshot* runtimeConfig) {
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

    if (runtimeConfig && !runtimeConfig->Empty()) {
        WriteRuntimeConfigCsvSection(out, *runtimeConfig);
    } else if (pipeline) {
        out << "--- Config Parameters ---\n";
        out << "[TouchPipeline]\n";
        std::ostringstream cfgStream;
        pipeline->SaveConfig(cfgStream);
        out << cfgStream.str();
        out << "\n";
    }

    WritePeaksCsvSection(out, frame);

    out << "--- Contacts ---\n";
    out << "Count," << frame.touch.output.contacts.size() << "\n";
    out << "ID,X,Y,State,Area,SignalSum,SizeMm,Reported,ReportEvent,LifeFlags,ReportFlags,DebugFlags\n";
    for (const auto& c : frame.touch.output.contacts) {
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
    out << "Packet0Valid," << (frame.touch.output.touchPackets[0].valid ? 1 : 0) << "\n";
    out << "Packet0Hex,";
    WriteCsvPacketLine(out, frame.touch.output.touchPackets[0].bytes);
    out << "\n";
    out << "Packet1Valid," << (frame.touch.output.touchPackets[1].valid ? 1 : 0) << "\n";
    out << "Packet1Hex,";
    WriteCsvPacketLine(out, frame.touch.output.touchPackets[1].bytes);
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
        masterRows.push_back({"ContactCount", std::to_string(frame.touch.output.contacts.size())});
        masterRows.push_back({"PeakCount", std::to_string(FramePeakCount(frame))});
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
        const auto& stylusRuntimePressure = frame.stylus.runtime.Active().pressure;
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
        slaveRows.push_back({"PressureIsReal", stylusRuntimePressure.pressureIsReal ? "1" : "0"});
        slaveRows.push_back({"PredictedAgeFrames", std::to_string(static_cast<unsigned int>(stylusRuntimePressure.predictedAgeFrames))});
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
        slaveRows.push_back({"LegacyPacketValid", stylusOutput.packet.valid ? "1" : "0"});
        slaveRows.push_back({"LegacyPacketHex", stylusOutput.packet.valid ? FormatCsvPacketBytes(stylusOutput.packet.bytes) : "N/A"});
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

} // namespace Dvr
