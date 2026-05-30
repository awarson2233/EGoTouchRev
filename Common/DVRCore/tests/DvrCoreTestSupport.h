#pragma once

#include "DvrBinaryIO.h"
#include "DvrFormat.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace DvrCoreTest {

inline void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

inline std::filesystem::path TempPath(const char* name) {
    const auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
        (std::string("egotouch_") + name + "_" + std::to_string(ticks) + ".dvrbin");
}

inline Dvr::DvrFrameSlot MakeFrameSlot(uint64_t timestamp = 12345, uint64_t epochUs = 67890, uint64_t seq = 42) {
    Dvr::DvrFrameSlot frame{};
    frame.timestamp = timestamp;
    frame.receiveSystemEpochUs = epochUs;
    frame.dvrSeq = seq;
    frame.masterWasRead = true;
    frame.masterSuffixValid = true;
    frame.slaveSuffixValid = true;
    frame.masterSuffix.words[0] = 0x1111;
    frame.masterSuffix.words[1] = 0x2222;
    frame.slaveSuffix.words[0] = 0x3333;
    frame.slaveSuffix.words[1] = 0x4444;
    frame.heatmapMatrix[0][0] = 101;
    frame.heatmapMatrix[7][9] = -202;
    frame.touchPackets[0].valid = 1;
    frame.touchPackets[0].reportId = 0x01;
    frame.touchPackets[0].length = 0x20;
    frame.touchPackets[0].bytes[0] = 0xAA;
    frame.touchPackets[0].bytes[31] = 0x55;
    frame.touchZones[0] = 3;
    frame.peakZones[1] = 4;
    frame.stylus.slaveValid = true;
    frame.stylus.checksumOk = true;
    frame.stylus.slaveWordOffset = 12;
    frame.stylus.checksum16 = 0xBEEF;
    frame.stylus.tx1BlockValid = true;
    frame.stylus.tx2BlockValid = true;
    frame.stylus.status = 0x12345678;
    frame.stylus.pressure = 321;
    frame.stylus.btPressure[0] = 10;
    frame.stylus.btRawPressure[0] = 20;
    frame.stylus.btSeq = 77;
    frame.stylus.btFreq1 = 5;
    frame.stylus.btFreq2 = 6;
    frame.stylus.btHasSample = true;
    frame.stylus.btHasFreq = true;
    frame.stylus.signalX = 123;
    frame.stylus.signalY = 456;
    frame.stylus.maxRawPeak = 789;
    frame.stylus.pipelineStage = 2;
    frame.stylus.outputValid = true;
    frame.stylus.inRange = true;
    frame.stylus.tipDown = true;
    frame.stylus.outputConfidence = 0.75f;
    frame.stylus.recheckEnabled = true;
    frame.stylus.recheckPassed = false;
    frame.stylus.recheckOverlap = true;
    frame.stylus.recheckThreshold = 30;
    frame.stylus.recheckThresholdMulti = 60;
    frame.stylus.touchNullLike = true;
    frame.stylus.touchSuppressActive = true;
    frame.stylus.touchSuppressFrames = 8;
    frame.stylus.pressureIsReal = true;
    frame.stylus.predictedAgeFrames = 3;
    frame.stylus.packet.valid = 1;
    frame.stylus.packet.reportId = 0x08;
    frame.stylus.packet.length = 17;
    frame.stylus.packet.bytes[0] = 0x12;
    frame.stylus.packet.bytes[16] = 0x34;
    frame.stylus.point.valid = true;
    frame.stylus.point.x = 12.5f;
    frame.stylus.point.y = 34.5f;
    frame.stylus.point.reportX = 125;
    frame.stylus.point.reportY = 345;
    frame.stylus.point.pressure = 456;
    frame.stylus.point.rawPressure = 567;
    frame.stylus.point.mappedPressure = 678;
    frame.stylus.point.peakTx1 = 11;
    frame.stylus.point.peakTx2 = 22;
    frame.stylus.point.tiltValid = true;
    frame.stylus.point.preTiltX = -1;
    frame.stylus.point.preTiltY = -2;
    frame.stylus.point.tiltX = 3;
    frame.stylus.point.tiltY = 4;
    frame.stylus.point.tiltMagnitude = 5.5f;
    frame.stylus.point.tiltAzimuthDeg = 6.5f;
    frame.stylus.point.tx1X = 7.5f;
    frame.stylus.point.tx1Y = 8.5f;
    frame.stylus.point.tx2X = 9.5f;
    frame.stylus.point.tx2Y = 10.5f;
    frame.stylus.point.confidence = 0.9f;
    frame.contacts[0].id = 7;
    frame.contacts[0].x = 100.25f;
    frame.contacts[0].y = 200.5f;
    frame.contacts[0].state = 2;
    frame.contacts[0].area = 9;
    frame.contacts[0].signalSum = 1000;
    frame.contacts[0].sizeMm = 4.5f;
    frame.contacts[0].isEdge = 1;
    frame.contacts[0].isReported = 1;
    frame.contactCount = 1;
    frame.peaks[0] = {3, 4, 500, 9, 0};
    frame.peakCount = 1;
    frame.rawDataLength = 4;
    frame.rawData[0] = 0x10;
    frame.rawData[1] = 0x20;
    frame.rawData[2] = 0x30;
    frame.rawData[3] = 0x40;
    return frame;
}

inline Dvr::RuntimeConfigSnapshot MakeRuntimeConfigSnapshot() {
    Dvr::RuntimeConfigSnapshot snapshot;
    auto addField = [&](uint32_t id, Dvr::Format::Dvr2ConfigValueType type, std::string section, std::string key) {
        Dvr::RuntimeConfigField field{};
        field.fieldId = id;
        field.valueType = type;
        field.section = std::move(section);
        field.key = std::move(key);
        field.displayName = field.key;
        field.moduleTag = "dvrcore-test";
        field.unit = "u";
        snapshot.fields.push_back(std::move(field));
    };
    addField(1, Dvr::Format::Dvr2ConfigValueType::Bool, "TouchPipeline", "BaselineEnabled");
    addField(2, Dvr::Format::Dvr2ConfigValueType::Int32, "TouchPipeline", "Threshold");
    addField(3, Dvr::Format::Dvr2ConfigValueType::UInt8, "Stylus", "Mode");
    addField(4, Dvr::Format::Dvr2ConfigValueType::UInt16, "Stylus", "PressureLimit");
    addField(5, Dvr::Format::Dvr2ConfigValueType::Float32, "Filter", "Alpha");
    addField(6, Dvr::Format::Dvr2ConfigValueType::Float64, "Filter", "Beta");
    addField(7, Dvr::Format::Dvr2ConfigValueType::String, "Service", "desired_mode");

    auto addValue = [&](uint32_t id, Dvr::Format::Dvr2ConfigValueType type, uint64_t raw, std::string text = {}) {
        Dvr::RuntimeConfigValue value{};
        value.fieldId = id;
        value.valueType = type;
        value.valid = true;
        value.rawValue = raw;
        value.stringValue = std::move(text);
        snapshot.values.push_back(std::move(value));
    };
    addValue(1, Dvr::Format::Dvr2ConfigValueType::Bool, 1);
    addValue(2, Dvr::Format::Dvr2ConfigValueType::Int32, static_cast<uint64_t>(static_cast<int64_t>(-42)));
    addValue(3, Dvr::Format::Dvr2ConfigValueType::UInt8, 3);
    addValue(4, Dvr::Format::Dvr2ConfigValueType::UInt16, 4096);
    uint32_t f32 = 0x3F000000u;
    uint64_t f64 = 0x3FF8000000000000ull;
    addValue(5, Dvr::Format::Dvr2ConfigValueType::Float32, f32);
    addValue(6, Dvr::Format::Dvr2ConfigValueType::Float64, f64);
    addValue(7, Dvr::Format::Dvr2ConfigValueType::String, 0, "full");
    return snapshot;
}

inline std::vector<uint8_t> ReadBytes(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    Require(in.is_open(), "failed to open binary for reading");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

inline void WriteBytes(const std::filesystem::path& path, const std::vector<uint8_t>& bytes) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    Require(out.is_open(), "failed to open binary for writing");
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    Require(static_cast<bool>(out), "failed to write binary bytes");
}

inline void ExpectReadFailure(const std::filesystem::path& path, const char* message) {
    std::vector<Solvers::HeatmapFrame> frames;
    int version = 0;
    std::string error;
    const bool ok = Dvr::ReadBinaryFile(path, frames, version, nullptr, &error);
    Require(!ok, message);
    Require(!error.empty(), "negative read should report an error");
}

} // namespace DvrCoreTest
