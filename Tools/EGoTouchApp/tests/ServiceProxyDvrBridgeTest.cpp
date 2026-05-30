#include "DvrBinaryIO.h"
#include "DvrFormat.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

App::DvrRuntimeConfigSnapshot MakeRuntimeConfigSnapshot() {
    App::DvrRuntimeConfigSnapshot snapshot;

    App::DvrRuntimeConfigField boolField{};
    boolField.fieldId = 1;
    boolField.valueType = Dvr::Format::Dvr2ConfigValueType::Bool;
    boolField.section = "TouchPipeline";
    boolField.key = "BaselineEnabled";
    boolField.displayName = "Baseline Enabled";
    boolField.moduleTag = "baseline";
    snapshot.fields.push_back(std::move(boolField));

    App::DvrRuntimeConfigField stringField{};
    stringField.fieldId = 2;
    stringField.valueType = Dvr::Format::Dvr2ConfigValueType::String;
    stringField.section = "Service";
    stringField.key = "desired_mode";
    stringField.displayName = "Desired Service Mode";
    snapshot.fields.push_back(std::move(stringField));

    App::DvrRuntimeConfigValue boolValue{};
    boolValue.fieldId = 1;
    boolValue.valueType = Dvr::Format::Dvr2ConfigValueType::Bool;
    boolValue.valid = true;
    boolValue.rawValue = 1;
    snapshot.values.push_back(std::move(boolValue));

    App::DvrRuntimeConfigValue stringValue{};
    stringValue.fieldId = 2;
    stringValue.valueType = Dvr::Format::Dvr2ConfigValueType::String;
    stringValue.valid = true;
    stringValue.stringValue = "full";
    snapshot.values.push_back(std::move(stringValue));

    return snapshot;
}

Dvr::DvrFrameSlot MakeFrameSlot() {
    Dvr::DvrFrameSlot frame{};
    frame.timestamp = 12345;
    frame.receiveSystemEpochUs = 67890;
    frame.dvrSeq = 42;
    frame.masterWasRead = true;
    return frame;
}

void TestResolveReplayBinaryPathBridge() {
    const auto dvrbinPath = std::filesystem::path("C:/captures/session.dvrbin");
    Require(App::ResolveReplayBinaryPath(dvrbinPath) == Dvr::ResolveReplayBinaryPath(dvrbinPath),
            "App ResolveReplayBinaryPath should forward .dvrbin paths to DVRCore");

    const auto directoryPath = std::filesystem::path("C:/captures/session");
    Require(App::ResolveReplayBinaryPath(directoryPath) == Dvr::ResolveReplayBinaryPath(directoryPath),
            "App ResolveReplayBinaryPath should forward directory paths to DVRCore");
}

void TestRuntimeConfigBridgeRoundTrip() {
    const auto path = std::filesystem::temp_directory_path() / "egotouch_runtime_config_roundtrip.dvrbin";
    const std::vector<Dvr::DvrFrameSlot> frames{MakeFrameSlot()};
    const auto snapshot = MakeRuntimeConfigSnapshot();

    uint32_t writeFlags = 0;
    Require(App::WriteDvrBinaryFile(path, frames, nullptr, nullptr, &snapshot, &writeFlags),
            "DVR2 write with runtime config should succeed");
    Require((writeFlags & Dvr::Format::kDvrFlagHasRuntimeConfig) != 0,
            "DVR2 write flags should mark runtime config");

    std::vector<Solvers::HeatmapFrame> readFrames;
    App::DvrRuntimeConfigSnapshot readConfig;
    int version = 0;
    uint32_t readFlags = 0;
    std::string error;
    Require(App::ReadDvrBinaryFile(path, readFrames, version, &readFlags, &error, nullptr, nullptr, &readConfig),
            error.empty() ? "DVR2 read with runtime config should succeed" : error.c_str());

    Require(version == Dvr::Format::kCurrentDvrFormatVersion, "DVR2 version should round-trip");
    Require(readFrames.size() == 1, "DVR2 frame count should round-trip");
    Require((readFlags & Dvr::Format::kDvrFlagHasRuntimeConfig) != 0,
            "DVR2 read flags should mark runtime config");
    Require(readConfig.fields.size() == 2, "runtime config field count should round-trip");
    Require(readConfig.values.size() == 2, "runtime config value count should round-trip");
    Require(readConfig.schemaHash != 0, "runtime config schema hash should be populated");
    Require(readConfig.fields[0].section == "TouchPipeline", "runtime config section should round-trip");
    Require(readConfig.fields[0].key == "BaselineEnabled", "runtime config key should round-trip");
    Require(readConfig.values[0].rawValue == 1, "runtime config bool value should round-trip");
    Require(readConfig.values[1].stringValue == "full", "runtime config string value should round-trip");

    std::filesystem::remove(path);
}

void TestDvrWithoutRuntimeConfigRemainsReadable() {
    const auto path = std::filesystem::temp_directory_path() / "egotouch_no_runtime_config_roundtrip.dvrbin";
    const std::vector<Dvr::DvrFrameSlot> frames{MakeFrameSlot()};

    uint32_t writeFlags = 0;
    Require(App::WriteDvrBinaryFile(path, frames, nullptr, nullptr, nullptr, &writeFlags),
            "DVR2 write without runtime config should succeed");
    Require((writeFlags & Dvr::Format::kDvrFlagHasRuntimeConfig) == 0,
            "DVR2 write flags should omit runtime config when absent");

    std::vector<Solvers::HeatmapFrame> readFrames;
    App::DvrRuntimeConfigSnapshot readConfig;
    int version = 0;
    uint32_t readFlags = 0;
    std::string error;
    Require(App::ReadDvrBinaryFile(path, readFrames, version, &readFlags, &error, nullptr, nullptr, &readConfig),
            error.empty() ? "DVR2 read without runtime config should succeed" : error.c_str());

    Require(readFrames.size() == 1, "DVR2 frame count should round-trip without runtime config");
    Require((readFlags & Dvr::Format::kDvrFlagHasRuntimeConfig) == 0,
            "DVR2 read flags should omit runtime config when absent");
    Require(readConfig.Empty(), "runtime config should be empty when sections are absent");

    std::filesystem::remove(path);
}

} // namespace

int main() {
    try {
        TestResolveReplayBinaryPathBridge();
        TestRuntimeConfigBridgeRoundTrip();
        TestDvrWithoutRuntimeConfigRemainsReadable();
        std::cout << "[TEST] ServiceProxy DVR bridge tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
