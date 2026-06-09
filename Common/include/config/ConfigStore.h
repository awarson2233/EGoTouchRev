#pragma once

#include "ConfigValue.h"
#include "SchemaValidator.h"
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace Config {

class ConfigStore {
public:
    // ── 加载 ──
    void loadFromYaml(const std::string& path);
    ValidationResult validate() const;

    // ── 读取 ──
    template<typename T>
    T get(std::string_view path) const;

    template<typename T>
    T getOr(std::string_view path, T fallback) const;

    // ── 写入 ──
    template<typename T>
    void set(std::string_view path, T value);

    // ── 持久化 ──
    void saveToYaml(const std::string& path) const;
    void saveOverrides(const std::string& path, const ConfigStore& defaults) const;
    void saveOverrides(const std::string& path,
                       const ConfigStore& defaults,
                       std::span<const std::string_view> forcedOverridePaths) const;

    // ── 元数据 ──
    std::vector<std::string> allPaths() const;
    bool has(std::string_view path) const;

    // ── 合并 ──
    void mergeFrom(const ConfigStore& other);

private:
    struct Entry {
        ConfigValue value;
        std::optional<ConfigValue> defaultValue;
        std::optional<ConfigRange> range;
        std::string description;
    };
    std::unordered_map<std::string, Entry> m_entries;

    // 内部：点号分隔路径解析（如 "touch.signal_cond.key"）
    std::string resolvePath(std::string_view path) const;
    // 内部：YAML 扁平化（YAML::Node tree → flat key-value）
    void flattenYaml(const YAML::Node& node, const std::string& prefix);
    // 内部：flat key-value → YAML 树
    YAML::Node unflattenToYaml() const;
};

// ── 模板实现（header-only）──

template<typename T>
T ConfigStore::get(std::string_view path) const {
    auto key = resolvePath(path);
    auto it = m_entries.find(key);
    if (it == m_entries.end()) {
        throw std::runtime_error("ConfigStore: key not found: " + std::string(path));
    }
    return getValue<T>(it->second.value);
}

template<typename T>
T ConfigStore::getOr(std::string_view path, T fallback) const {
    auto key = resolvePath(path);
    auto it = m_entries.find(key);
    if (it == m_entries.end()) {
        return fallback;
    }
    auto opt = tryGetValue<T>(it->second.value);
    return opt.value_or(fallback);
}

template<typename T>
void ConfigStore::set(std::string_view path, T value) {
    auto key = resolvePath(path);
    m_entries[key].value = ConfigValue(value);
}

template<>
inline ConfigValue ConfigStore::get<ConfigValue>(std::string_view path) const {
    auto key = resolvePath(path);
    auto it = m_entries.find(key);
    if (it == m_entries.end()) {
        throw std::runtime_error("ConfigStore: key not found: " + std::string(path));
    }
    return it->second.value;
}

template<>
inline ConfigValue ConfigStore::getOr<ConfigValue>(std::string_view path, ConfigValue fallback) const {
    auto key = resolvePath(path);
    auto it = m_entries.find(key);
    if (it == m_entries.end()) {
        return fallback;
    }
    return it->second.value;
}

template<>
inline void ConfigStore::set<ConfigValue>(std::string_view path, ConfigValue value) {
    auto key = resolvePath(path);
    m_entries[key].value = std::move(value);
}

} // namespace Config
