#include "config/ConfigSchemaSnapshot.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace Config {
namespace {

bool startsWith(std::string_view text, std::string_view prefix)
{
    return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
}

std::string titleToken(std::string_view token)
{
    std::string result{token};
    if (result.empty()) {
        return result;
    }

    const bool containsDigit = std::ranges::any_of(result, [](unsigned char ch) {
        return std::isdigit(ch) != 0;
    });
    if (containsDigit) {
        std::ranges::transform(result, result.begin(), [](unsigned char ch) {
            return static_cast<char>(std::toupper(ch));
        });
        return result;
    }

    std::ranges::transform(result, result.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    result.front() = static_cast<char>(std::toupper(static_cast<unsigned char>(result.front())));
    return result;
}

std::string titleSnake(std::string_view segment)
{
    std::string result;
    size_t start = 0;
    while (start <= segment.size()) {
        const auto end = segment.find('_', start);
        const auto count = (end == std::string_view::npos) ? segment.size() - start : end - start;
        if (count > 0) {
            if (!result.empty()) {
                result.push_back(' ');
            }
            result += titleToken(segment.substr(start, count));
        }
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1;
    }
    return result;
}

std::string fallbackModuleTag(std::string_view yamlPath)
{
    const auto dot = yamlPath.find('.');
    const auto prefix = yamlPath.substr(0, dot == std::string_view::npos ? yamlPath.size() : dot);
    if (prefix.empty()) {
        return "Other";
    }
    return titleSnake(prefix) + " / Other";
}

} // namespace

std::string deriveModuleTag(std::string_view yamlPath)
{
    if (startsWith(yamlPath, "service.")) return "Service";
    if (startsWith(yamlPath, "stylus.sp.frame_parser")) return "Stylus / Frame Parser";

    if (startsWith(yamlPath, "stylus.sp.peak_") ||
        startsWith(yamlPath, "stylus.sp.coordinate_") ||
        startsWith(yamlPath, "stylus.sp.tilt_") ||
        yamlPath == "stylus.sp.signal_floor") {
        return "Stylus / Data Solve";
    }

    if (startsWith(yamlPath, "stylus.sp.post_pressure") ||
        startsWith(yamlPath, "stylus.sp.fake_pressure") ||
        startsWith(yamlPath, "stylus.sp.tip_down") ||
        startsWith(yamlPath, "stylus.sp.bt_") ||
        startsWith(yamlPath, "stylus.sp.pressure_")) {
        return "Stylus / Pressure";
    }

    if (startsWith(yamlPath, "stylus.sp.edge_") ||
        startsWith(yamlPath, "stylus.sp.noise_") ||
        startsWith(yamlPath, "stylus.sp.linear_") ||
        startsWith(yamlPath, "stylus.sp.coor_") ||
        startsWith(yamlPath, "stylus.sp.iir_") ||
        startsWith(yamlPath, "stylus.sp.aft_") ||
        startsWith(yamlPath, "stylus.sp.lock_")) {
        return "Stylus / Coordinate";
    }

    return fallbackModuleTag(yamlPath);
}

std::string deriveDisplayName(std::string_view yamlPath)
{
    const auto lastDot = yamlPath.rfind('.');
    const auto lastSegment = yamlPath.substr(lastDot == std::string_view::npos ? 0 : lastDot + 1);

    if (lastSegment == "enabled" && lastDot != std::string_view::npos && lastDot > 0) {
        const auto parentEnd = lastDot;
        const auto parentStartDot = yamlPath.rfind('.', parentEnd - 1);
        const auto parentStart = parentStartDot == std::string_view::npos ? 0 : parentStartDot + 1;
        const auto parentSegment = yamlPath.substr(parentStart, parentEnd - parentStart);
        return titleSnake(parentSegment) + " Enabled";
    }

    return titleSnake(lastSegment);
}

ConfigUiType deriveUiType(const ConfigValue& value)
{
    if (std::holds_alternative<bool>(value)) return ConfigUiType::Bool;
    if (std::holds_alternative<int32_t>(value)) return ConfigUiType::Int32;
    if (std::holds_alternative<float>(value)) return ConfigUiType::Float;
    if (std::holds_alternative<std::string>(value)) return ConfigUiType::String;
    return ConfigUiType::String;
}

} // namespace Config
