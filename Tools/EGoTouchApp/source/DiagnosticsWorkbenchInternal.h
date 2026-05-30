#pragma once

#include "ServiceProxyTypes.h"
#include "SolverTypes.h"

#include <filesystem>
#include <string>

namespace App {

enum class FpsStatusClass {
    Good,
    Warn,
    Bad,
};

struct DvrImportValidationResult {
    bool ok = false;
    std::filesystem::path canonicalPath;
    std::string status;
};

FpsStatusClass ClassifyFps(int fps);
const char* FrameSourceModeLabel(FrameSourceMode mode);
const char* TouchStateLabel(int state);
const char* TouchReportEventLabel(int event);
std::string TouchPacketBytes(const Solvers::TouchPacket& packet);
DvrImportValidationResult ValidateDvrImportSelection(
    const std::filesystem::path& importRoot,
    const std::filesystem::path& selectedFile);
std::filesystem::path BuildDvrCsvExportPath(const std::filesystem::path& exportRoot,
                                            uint64_t frameTimestamp);
uint8_t ClampU8(int value);

} // namespace App
