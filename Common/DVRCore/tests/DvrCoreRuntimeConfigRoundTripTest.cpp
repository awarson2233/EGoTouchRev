#include "DvrCoreTestSupport.h"
#include "config/ConfigStore.h"

#include <iostream>
#include <limits>

namespace {

void TestRuntimeConfigRoundTrip() {
    const auto path = DvrCoreTest::TempPath("runtime_config_roundtrip");
    const std::vector<Dvr::DvrFrameSlot> frames{DvrCoreTest::MakeFrameSlot()};
    const auto snapshot = DvrCoreTest::MakeRuntimeConfigSnapshot();

    uint32_t writeFlags = 0;
    DvrCoreTest::Require(Dvr::WriteBinaryFile(path, frames, nullptr, nullptr, &snapshot, &writeFlags), "DVR2 write with runtime config should succeed");
    DvrCoreTest::Require((writeFlags & Dvr::Format::kDvrFlagHasRuntimeConfig) != 0, "DVR2 write should mark runtime config");

    std::vector<Solvers::HeatmapFrame> readFrames;
    Dvr::RuntimeConfigSnapshot readConfig;
    int version = 0;
    uint32_t readFlags = 0;
    std::string error;
    DvrCoreTest::Require(Dvr::ReadBinaryFile(path, readFrames, version, &readFlags, &error, nullptr, nullptr, &readConfig),
                         error.empty() ? "DVR2 read with runtime config should succeed" : error.c_str());

    DvrCoreTest::Require(version == Dvr::Format::kCurrentDvrFormatVersion, "DVR2 version should round-trip");
    DvrCoreTest::Require(readFrames.size() == 1, "DVR2 frame count should round-trip");
    DvrCoreTest::Require((readFlags & Dvr::Format::kDvrFlagHasRuntimeConfig) != 0, "DVR2 read should mark runtime config");
    DvrCoreTest::Require(readConfig.fields.size() == snapshot.fields.size(), "runtime config field count should round-trip");
    DvrCoreTest::Require(readConfig.values.size() == snapshot.values.size(), "runtime config value count should round-trip");
    DvrCoreTest::Require(readConfig.schemaHash != 0, "runtime config schema hash should be populated");
    DvrCoreTest::Require(readConfig.fields[0].section == "TouchPipeline" && readConfig.fields[0].key == "BaselineEnabled", "runtime bool field identity should round-trip");
    DvrCoreTest::Require(readConfig.values[0].rawValue == 1, "runtime bool value should round-trip");
    DvrCoreTest::Require(readConfig.values[1].rawValue == static_cast<uint64_t>(static_cast<int64_t>(-42)), "runtime int32 value should round-trip");
    DvrCoreTest::Require(readConfig.values[2].rawValue == 3, "runtime uint8 value should round-trip");
    DvrCoreTest::Require(readConfig.values[3].rawValue == 4096, "runtime uint16 value should round-trip");
    DvrCoreTest::Require(readConfig.values[4].rawValue == 0x3F000000u, "runtime float32 bits should round-trip");
    DvrCoreTest::Require(readConfig.values[5].rawValue == 0x3FF8000000000000ull, "runtime float64 bits should round-trip");
    DvrCoreTest::Require(readConfig.values[6].stringValue == "full", "runtime string value should round-trip");
    DvrCoreTest::Require(readConfig.values[7].rawValue == 0x7FFFFFFFull, "runtime uint32 safe value should round-trip");
    DvrCoreTest::Require(readConfig.values[8].rawValue == 0x80000000ull, "runtime uint32 overflow value should round-trip");

    const auto configStore = readConfig.toConfigStore();
    DvrCoreTest::Require(configStore.getOr<bool>("TouchPipeline.BaselineEnabled", false),
                         "runtime config bool should convert to ConfigStore");
    DvrCoreTest::Require(configStore.getOr<int32_t>("TouchPipeline.Threshold", 0) == -42,
                         "runtime config int32 should convert to ConfigStore");
    DvrCoreTest::Require(configStore.getOr<int32_t>("Stylus.PressureLimit", 0) == 4096,
                         "runtime config uint16 should convert to ConfigStore int32");
    DvrCoreTest::Require(configStore.getOr<float>("Filter.Alpha", 0.0f) == 0.5f,
                         "runtime config float32 should convert to ConfigStore");
    DvrCoreTest::Require(configStore.getOr<std::string>("Service.desired_mode", "") == "full",
                         "runtime config string should convert to ConfigStore");
    DvrCoreTest::Require(configStore.getOr<int32_t>("Service.safe_counter", 0) == std::numeric_limits<int32_t>::max(),
                         "runtime config uint32 within int32 range should convert to ConfigStore");
    DvrCoreTest::Require(!configStore.has("Service.too_large_counter"),
                         "runtime config uint32 above int32 range should be skipped");

    std::filesystem::remove(path);
}

void TestInvalidRuntimeConfigIsNotPersisted() {
    const auto path = DvrCoreTest::TempPath("invalid_runtime_config");
    const std::vector<Dvr::DvrFrameSlot> frames{DvrCoreTest::MakeFrameSlot()};
    auto snapshot = DvrCoreTest::MakeRuntimeConfigSnapshot();
    snapshot.values.pop_back();

    uint32_t writeFlags = 0;
    DvrCoreTest::Require(Dvr::WriteBinaryFile(path, frames, nullptr, nullptr, &snapshot, &writeFlags), "DVR2 write should succeed without invalid runtime config sections");
    DvrCoreTest::Require((writeFlags & Dvr::Format::kDvrFlagHasRuntimeConfig) == 0, "invalid runtime config should not be marked persisted");

    std::vector<Solvers::HeatmapFrame> readFrames;
    Dvr::RuntimeConfigSnapshot readConfig;
    int version = 0;
    uint32_t readFlags = 0;
    std::string error;
    DvrCoreTest::Require(Dvr::ReadBinaryFile(path, readFrames, version, &readFlags, &error, nullptr, nullptr, &readConfig),
                         error.empty() ? "DVR2 read without persisted invalid runtime config should succeed" : error.c_str());
    DvrCoreTest::Require((readFlags & Dvr::Format::kDvrFlagHasRuntimeConfig) == 0, "read should omit runtime config flag for invalid snapshot");
    DvrCoreTest::Require(readConfig.Empty(), "read config should be empty when invalid snapshot is not persisted");

    std::filesystem::remove(path);
}

void TestDvrWithoutRuntimeConfigRemainsReadable() {
    const auto path = DvrCoreTest::TempPath("no_runtime_config");
    const std::vector<Dvr::DvrFrameSlot> frames{DvrCoreTest::MakeFrameSlot()};

    uint32_t writeFlags = 0;
    DvrCoreTest::Require(Dvr::WriteBinaryFile(path, frames, nullptr, nullptr, nullptr, &writeFlags), "DVR2 write without runtime config should succeed");
    DvrCoreTest::Require((writeFlags & Dvr::Format::kDvrFlagHasRuntimeConfig) == 0, "DVR2 write should omit runtime config flag when absent");

    std::vector<Solvers::HeatmapFrame> readFrames;
    Dvr::RuntimeConfigSnapshot readConfig;
    int version = 0;
    uint32_t readFlags = 0;
    std::string error;
    DvrCoreTest::Require(Dvr::ReadBinaryFile(path, readFrames, version, &readFlags, &error, nullptr, nullptr, &readConfig),
                         error.empty() ? "DVR2 read without runtime config should succeed" : error.c_str());
    DvrCoreTest::Require(readFrames.size() == 1, "DVR2 frame count should round-trip without runtime config");
    DvrCoreTest::Require((readFlags & Dvr::Format::kDvrFlagHasRuntimeConfig) == 0, "DVR2 read should omit runtime config flag when absent");
    DvrCoreTest::Require(readConfig.Empty(), "runtime config should be empty when sections are absent");

    std::filesystem::remove(path);
}

} // namespace

int main() {
    try {
        TestRuntimeConfigRoundTrip();
        TestInvalidRuntimeConfigIsNotPersisted();
        TestDvrWithoutRuntimeConfigRemainsReadable();
        std::cout << "[TEST] DVRCore runtime config round-trip tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
