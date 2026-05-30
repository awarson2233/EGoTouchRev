#include "DiagnosticsWorkbenchInternal.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace App {

FpsStatusClass ClassifyFps(int fps) {
    if (fps >= 100) return FpsStatusClass::Good;
    if (fps >= 50) return FpsStatusClass::Warn;
    return FpsStatusClass::Bad;
}

const char* FrameSourceModeLabel(FrameSourceMode mode) {
    switch (mode) {
    case FrameSourceMode::Live: return "Live";
    case FrameSourceMode::Playback: return "Playback";
    default: return "Unknown";
    }
}

const char* TouchStateLabel(int state) {
    switch (state) {
    case Solvers::TouchStateDown: return "Down";
    case Solvers::TouchStateMove: return "Move";
    case Solvers::TouchStateUp: return "Up";
    default: return "UNK";
    }
}

const char* TouchReportEventLabel(int event) {
    switch (event) {
    case Solvers::TouchReportIdle: return "Idle";
    case Solvers::TouchReportDown: return "Down";
    case Solvers::TouchReportMove: return "Move";
    case Solvers::TouchReportUp: return "Up";
    default: return "UNK";
    }
}

std::string TouchPacketBytes(const Solvers::TouchPacket& packet) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < packet.bytes.size(); ++i) {
        oss << std::setw(2) << static_cast<unsigned int>(packet.bytes[i]);
        if (i + 1 < packet.bytes.size()) {
            oss << ' ';
        }
    }
    return oss.str();
}

DvrImportValidationResult ValidateDvrImportSelection(
    const std::filesystem::path& importRoot,
    const std::filesystem::path& selectedFile) {
    DvrImportValidationResult result{};
    std::error_code ec;
    const auto canonicalRoot = std::filesystem::weakly_canonical(importRoot, ec);
    if (ec) {
        result.status = "Import failed: export root is unavailable";
        return result;
    }

    const auto canonicalFile = std::filesystem::weakly_canonical(selectedFile, ec);
    if (ec) {
        result.status = "Import failed: selected file is unavailable";
        return result;
    }
    if (canonicalFile.extension() != ".dvrbin") {
        result.status = "Import failed: please select a .dvrbin file";
        return result;
    }

    const auto relativeToRoot = canonicalFile.lexically_relative(canonicalRoot);
    const bool inRoot = !relativeToRoot.empty() &&
        (*relativeToRoot.begin() != std::filesystem::path(".."));
    if (!inRoot) {
        result.status = "Import failed: selection must stay under configured export root";
        return result;
    }

    result.ok = true;
    result.canonicalPath = canonicalFile;
    return result;
}

std::filesystem::path BuildDvrCsvExportPath(const std::filesystem::path& exportRoot,
                                            uint64_t frameTimestamp) {
    std::filesystem::path outputDir(exportRoot);
    outputDir /= "dvr";
    outputDir /= "dvr" + std::to_string(static_cast<unsigned long long>(frameTimestamp));
    return outputDir;
}

uint8_t ClampU8(int value) {
    return static_cast<uint8_t>(std::clamp(value, 0, 255));
}

} // namespace App
