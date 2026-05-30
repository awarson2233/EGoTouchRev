#include "DiagnosticsWorkbenchInternal.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct TempDirectory {
    std::filesystem::path path;

    explicit TempDirectory(const char* name) {
        path = std::filesystem::temp_directory_path() / name;
        std::filesystem::remove_all(path);
        std::filesystem::create_directories(path);
    }

    ~TempDirectory() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

void TestLabelsAndFpsClassification() {
    Require(App::ClassifyFps(100) == App::FpsStatusClass::Good, "100fps should be Good");
    Require(App::ClassifyFps(50) == App::FpsStatusClass::Warn, "50fps should be Warn");
    Require(App::ClassifyFps(49) == App::FpsStatusClass::Bad, "49fps should be Bad");
    Require(std::string(App::FrameSourceModeLabel(App::FrameSourceMode::Live)) == "Live", "Live label mismatch");
    Require(std::string(App::FrameSourceModeLabel(App::FrameSourceMode::Playback)) == "Playback", "Playback label mismatch");
    Require(std::string(App::FrameSourceModeLabel(static_cast<App::FrameSourceMode>(99))) == "Unknown", "Unknown frame source label mismatch");
    Require(std::string(App::TouchStateLabel(Solvers::TouchStateDown)) == "Down", "Touch down label mismatch");
    Require(std::string(App::TouchStateLabel(Solvers::TouchStateMove)) == "Move", "Touch move label mismatch");
    Require(std::string(App::TouchStateLabel(Solvers::TouchStateUp)) == "Up", "Touch up label mismatch");
    Require(std::string(App::TouchStateLabel(99)) == "UNK", "Unknown touch state label mismatch");
    Require(std::string(App::TouchReportEventLabel(Solvers::TouchReportIdle)) == "Idle", "Idle event label mismatch");
    Require(std::string(App::TouchReportEventLabel(Solvers::TouchReportDown)) == "Down", "Down event label mismatch");
    Require(std::string(App::TouchReportEventLabel(Solvers::TouchReportMove)) == "Move", "Move event label mismatch");
    Require(std::string(App::TouchReportEventLabel(Solvers::TouchReportUp)) == "Up", "Up event label mismatch");
    Require(std::string(App::TouchReportEventLabel(99)) == "UNK", "Unknown event label mismatch");
}

void TestTouchPacketBytes() {
    Solvers::TouchPacket packet{};
    packet.bytes[0] = 0x01;
    packet.bytes[1] = 0x0a;
    packet.bytes[2] = 0xff;
    const std::string text = App::TouchPacketBytes(packet);
    Require(text.find("01 0a ff") == 0, "TouchPacketBytes should format lowercase two-digit hex bytes");
}

void TestDvrImportSelectionValidation() {
    TempDirectory root("egotouch_app_dvr_import_root");
    const auto validFile = root.path / "capture.dvrbin";
    const auto wrongExtension = root.path / "capture.txt";
    const auto outsideDir = std::filesystem::temp_directory_path() / "egotouch_app_dvr_import_outside";
    const auto outsideFile = outsideDir / "outside.dvrbin";
    std::filesystem::create_directories(outsideDir);
    { auto f = std::ofstream(validFile); }
    { auto f = std::ofstream(wrongExtension); }
    { auto f = std::ofstream(outsideFile); }

    const auto ok = App::ValidateDvrImportSelection(root.path, validFile);
    Require(ok.ok, "valid DVR import selection should pass");
    Require(ok.canonicalPath.extension() == ".dvrbin", "valid DVR import should return canonical .dvrbin path");

    const auto extError = App::ValidateDvrImportSelection(root.path, wrongExtension);
    Require(!extError.ok, "wrong extension should fail validation");
    Require(extError.status.find("please select a .dvrbin file") != std::string::npos,
            "wrong extension status mismatch");

    const auto outsideError = App::ValidateDvrImportSelection(root.path, outsideFile);
    Require(!outsideError.ok, "outside root selection should fail validation");
    Require(outsideError.status.find("selection must stay under configured export root") != std::string::npos,
            "outside root status mismatch");

    std::error_code ec;
    std::filesystem::remove_all(outsideDir, ec);
}

void TestDvrCsvExportPathAndClamp() {
    const auto path = App::BuildDvrCsvExportPath("C:/exports", 123);
    Require(path.generic_string().find("C:/exports/dvr/dvr123") != std::string::npos,
            "DVR CSV export path should include export root, dvr folder, and timestamp");
    Require(App::ClampU8(-1) == 0, "ClampU8 should clamp negative values");
    Require(App::ClampU8(0) == 0, "ClampU8 should keep zero");
    Require(App::ClampU8(255) == 255, "ClampU8 should keep 255");
    Require(App::ClampU8(300) == 255, "ClampU8 should clamp values above 255");
}

} // namespace

int main() {
    try {
        TestLabelsAndFpsClassification();
        TestTouchPacketBytes();
        TestDvrImportSelectionValidation();
        TestDvrCsvExportPathAndClamp();
        std::cout << "[TEST] DiagnosticsWorkbench logic tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
