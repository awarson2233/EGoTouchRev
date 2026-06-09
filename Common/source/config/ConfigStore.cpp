#include "config/ConfigStore.h"

#include "Logger.h"
#include "config/YamlParser.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cmath>
#include <limits>
#include <optional>
#include <string_view>

namespace Config {
namespace {

std::string toLower(std::string value) {
    std::ranges::transform(value, value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::vector<std::string> splitPath(std::string_view path) {
    std::vector<std::string> parts;
    size_t start = 0;
    while (start <= path.size()) {
        const auto end = path.find('.', start);
        const auto count = (end == std::string_view::npos) ? path.size() - start : end - start;
        if (count > 0) {
            parts.emplace_back(path.substr(start, count));
        }
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1;
    }
    return parts;
}

bool parseInt32(std::string_view text, int32_t& value) {
    const auto* begin = text.data();
    const auto* end = text.data() + text.size();
    auto [ptr, ec] = std::from_chars(begin, end, value);
    return ec == std::errc{} && ptr == end;
}

bool parseFloat(std::string_view text, float& value) {
    if (text.find_first_of(".eE") == std::string_view::npos) {
        return false;
    }

    const auto* begin = text.data();
    const auto* end = text.data() + text.size();
    auto [ptr, ec] = std::from_chars(begin, end, value, std::chars_format::general);
    return ec == std::errc{} && ptr == end && std::isfinite(value);
}

ConfigValue scalarToConfigValue(const YAML::Node& node) {
    const auto scalar = node.Scalar();
    const auto lower = toLower(scalar);
    if (lower == "true") {
        return true;
    }
    if (lower == "false") {
        return false;
    }

    int32_t intValue = 0;
    if (parseInt32(scalar, intValue)) {
        return intValue;
    }

    float floatValue = 0.0f;
    if (parseFloat(scalar, floatValue)) {
        return floatValue;
    }

    return scalar;
}

void assignConfigValue(YAML::Node node, const std::string& key, const ConfigValue& value) {
    std::visit([&node, &key](const auto& typedValue) {
        node[key] = typedValue;
    }, value);
}

void assignConfigValueAtPath(YAML::Node node,
                             const std::vector<std::string>& parts,
                             size_t index,
                             const ConfigValue& value) {
    if (index + 1 == parts.size()) {
        assignConfigValue(node, parts[index], value);
        return;
    }
    assignConfigValueAtPath(node[parts[index]], parts, index + 1, value);
}

std::optional<double> numericValue(const ConfigValue& value) {
    return std::visit([](const auto& typedValue) -> std::optional<double> {
        using ValueType = std::decay_t<decltype(typedValue)>;
        if constexpr (std::is_same_v<ValueType, int32_t> || std::is_same_v<ValueType, float>) {
            return static_cast<double>(typedValue);
        } else {
            return std::nullopt;
        }
    }, value);
}

} // namespace

void ConfigStore::loadFromYaml(const std::string& path) {
    m_entries.clear();
    const auto root = YamlParser::load(path);
    flattenYaml(root, {});
}

ValidationResult ConfigStore::validate() const {
    ValidationResult result;

    for (const auto& [path, entry] : m_entries) {
        if (entry.defaultValue.has_value() && entry.value.index() != entry.defaultValue->index()) {
            result.errors.push_back({
                ValidationIssue::Error,
                path,
                "type mismatch with default value"
            });
            continue;
        }

        if (entry.range.has_value()) {
            const auto value = numericValue(entry.value);
            if (!value.has_value()) {
                result.errors.push_back({
                    ValidationIssue::Error,
                    path,
                    "range validation requires a numeric value"
                });
                continue;
            }

            if (*value < entry.range->min || *value > entry.range->max) {
                result.errors.push_back({
                    ValidationIssue::Error,
                    path,
                    "value is outside configured range"
                });
            }
        }
    }

    return result;
}

void ConfigStore::saveToYaml(const std::string& path) const {
    YamlParser::save(path, unflattenToYaml());
}

void ConfigStore::saveOverrides(const std::string& path, const ConfigStore& defaults) const {
    saveOverrides(path, defaults, std::span<const std::string_view>{});
}

void ConfigStore::saveOverrides(const std::string& path,
                                const ConfigStore& defaults,
                                std::span<const std::string_view> forcedOverridePaths) const {
    ConfigStore overrides;
    for (const auto& [key, entry] : m_entries) {
        const auto defaultIt = defaults.m_entries.find(key);
        const bool forcedOverride = std::ranges::any_of(
            forcedOverridePaths,
            [&key](std::string_view forcedPath) { return forcedPath == key; });
        if (forcedOverride || defaultIt == defaults.m_entries.end() || entry.value != defaultIt->second.value) {
            overrides.m_entries.emplace(key, entry);
        }
    }

    YamlParser::save(path, overrides.unflattenToYaml());
}

std::vector<std::string> ConfigStore::allPaths() const {
    std::vector<std::string> paths;
    paths.reserve(m_entries.size());
    for (const auto& [key, _] : m_entries) {
        paths.push_back(key);
    }
    std::ranges::sort(paths);
    return paths;
}

bool ConfigStore::has(std::string_view path) const {
    const auto key = resolvePath(path);
    return m_entries.contains(key);
}

void ConfigStore::mergeFrom(const ConfigStore& other) {
    for (const auto& [key, entry] : other.m_entries) {
        m_entries[key] = entry;
    }
}

std::string ConfigStore::resolvePath(std::string_view path) const {
    return std::string(path);
}

void ConfigStore::flattenYaml(const YAML::Node& node, const std::string& prefix) {
    if (!node || node.IsNull()) {
        return;
    }

    if (node.IsScalar()) {
        if (!prefix.empty()) {
            m_entries[prefix].value = scalarToConfigValue(node);
        }
        return;
    }

    if (node.IsMap()) {
        for (const auto& item : node) {
            const auto key = item.first.as<std::string>();
            const auto childPath = prefix.empty() ? key : prefix + "." + key;
            flattenYaml(item.second, childPath);
        }
        return;
    }

    if (node.IsSequence()) {
        LOG_WARN("Config", __func__, "YAML", "Ignoring unsupported sequence node at path: {}", prefix);
    }
}

YAML::Node ConfigStore::unflattenToYaml() const {
    YAML::Node root(YAML::NodeType::Map);
    const auto paths = allPaths();

    for (const auto& path : paths) {
        const auto parts = splitPath(path);
        if (parts.empty()) {
            continue;
        }

        assignConfigValueAtPath(root, parts, 0, m_entries.at(path).value);
    }

    return root;
}

} // namespace Config
