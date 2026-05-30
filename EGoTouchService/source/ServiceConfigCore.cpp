#include "ServiceConfigCore.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <string>
#include <string_view>

namespace Service {
namespace {

std::string TrimCopy(std::string_view input) {
    const size_t start = input.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) return {};
    const size_t end = input.find_last_not_of(" \t\r\n");
    return std::string(input.substr(start, end - start + 1));
}

std::string ToLowerCopy(std::string_view input) {
    std::string out;
    out.reserve(input.size());
    for (char ch : input) {
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return out;
}

bool ParseBoolValue(std::string_view value) {
    const std::string lowered = ToLowerCopy(TrimCopy(value));
    return lowered == "1" || lowered == "true" || lowered == "yes" || lowered == "on";
}

bool ParseIniKeyValue(std::string_view line, std::string& key, std::string& value) {
    const size_t eq = line.find('=');
    if (eq == std::string_view::npos) return false;
    key = TrimCopy(line.substr(0, eq));
    value = TrimCopy(line.substr(eq + 1));
    return !key.empty();
}

} // namespace

const char* ServiceModeToConfig(ServiceMode mode) {
    return mode == ServiceMode::Full ? "full" : "touch_only";
}

ServiceConfigState ParseServiceConfig(const std::string& configPath) {
    ServiceConfigState parsed{};

    std::ifstream cfg(configPath);
    if (!cfg.is_open()) {
        return parsed;
    }

    std::string line;
    bool inServiceSection = false;
    while (std::getline(cfg, line)) {
        const std::string trimmed = TrimCopy(line);
        if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#') continue;
        if (trimmed.front() == '[' && trimmed.back() == ']') {
            inServiceSection = (trimmed == "[Service]");
            continue;
        }
        if (!inServiceSection) continue;

        std::string key;
        std::string val;
        if (!ParseIniKeyValue(trimmed, key, val)) continue;

        if (key == "mode") {
            parsed.mode = (val == "touch_only") ? ServiceMode::TouchOnly : ServiceMode::Full;
        } else if (key == "auto_mode") {
            parsed.autoMode = ParseBoolValue(val);
        } else if (key == "stylus_vhf_enabled") {
            parsed.stylusVhfEnabled = ParseBoolValue(val);
        } else if (key == "pen_button_mode") {
            const int ival = std::atoi(val.c_str());
            parsed.penButtonMode = static_cast<PenButtonMode>(std::clamp(ival, 0, 2));
        } else if (key == "pen_button_route") {
            const int ival = std::atoi(val.c_str());
            parsed.penButtonRoute = static_cast<PenButtonRoute>(std::clamp(ival, 0, 2));
        }
    }

    return parsed;
}

ReloadServiceConfigResult DiffServiceConfig(const ServiceConfigState& current,
                                            const ServiceConfigState& reloaded,
                                            bool runtimeAvailable) {
    ReloadServiceConfigResult result{};

    const bool modeChanged = (current.mode != reloaded.mode);
    const bool autoModeChanged = (current.autoMode != reloaded.autoMode);
    const bool stylusVhfChanged = (current.stylusVhfEnabled != reloaded.stylusVhfEnabled);
    const bool penButtonModeChanged = (current.penButtonMode != reloaded.penButtonMode);
    const bool penButtonRouteChanged = (current.penButtonRoute != reloaded.penButtonRoute);

    if (modeChanged) {
        result.changedFields |= ToServiceConfigFieldBit(ServiceConfigField::Mode);
        result.restartRequiredFields |= ToServiceConfigFieldBit(ServiceConfigField::Mode);
    }
    if (autoModeChanged) {
        result.changedFields |= ToServiceConfigFieldBit(ServiceConfigField::AutoMode);
    }
    if (stylusVhfChanged) {
        result.changedFields |= ToServiceConfigFieldBit(ServiceConfigField::StylusVhfEnabled);
    }
    if (penButtonModeChanged) {
        result.changedFields |= ToServiceConfigFieldBit(ServiceConfigField::PenButtonMode);
    }
    if (penButtonRouteChanged) {
        result.changedFields |= ToServiceConfigFieldBit(ServiceConfigField::PenButtonRoute);
    }

    if (runtimeAvailable) {
        result.appliedFields |= static_cast<uint8_t>(
            (autoModeChanged ? ToServiceConfigFieldBit(ServiceConfigField::AutoMode) : 0u) |
            (stylusVhfChanged ? ToServiceConfigFieldBit(ServiceConfigField::StylusVhfEnabled) : 0u) |
            (penButtonModeChanged ? ToServiceConfigFieldBit(ServiceConfigField::PenButtonMode) : 0u) |
            (penButtonRouteChanged ? ToServiceConfigFieldBit(ServiceConfigField::PenButtonRoute) : 0u));
    }

    return result;
}

} // namespace Service
