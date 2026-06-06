#pragma once

#include "ConfigKeyMap.h"
#include "ConfigSchemaSnapshot.h"
#include "ConfigValue.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace Config {

class ConfigStore;

// 单个绑定的描述
struct BindingEntry {
    std::string yamlPath;
    std::function<void(const ConfigValue&)> setter;   // 将值写入成员
    std::function<ConfigValue()> getter;               // 从成员读取值
    ConfigValue defaultValue;
    std::string typeName;    // "bool" / "int" / "float" / "string" / "enum"
    std::optional<ConfigKeyId> keyId;    // 来自 ConfigKeyMap 的静态映射
    std::optional<ConfigRange> range;
    std::string displayName;              // 来自 deriveDisplayName()
    std::string description;
    std::string moduleTag;                // 来自 deriveModuleTag()
    std::vector<std::pair<int, std::string>> enumMapping;  // 枚举值映射
    ConfigRuntimeBinding runtimeBinding = ConfigRuntimeBinding::SchemaOnly;
};

class ConfigBinder {
public:
    // ── 基础绑定: 值类型 (int, float, bool, string) ──
    template<typename Struct, typename T>
    void bind(std::string_view yamlPath,
              T Struct::*member, Struct& instance,
              T defaultValue,
              ConfigRange range = {},
              std::string_view description = "",
              ConfigRuntimeBinding runtimeBinding = ConfigRuntimeBinding::LiveSetter);

    // ── 枚举绑定 ──
    template<typename Struct, typename Enum>
    void bindEnum(std::string_view yamlPath,
                  Enum Struct::*member, Struct& instance,
                  Enum defaultValue,
                  std::span<const std::pair<Enum, std::string>> enumMapping,
                  std::string_view description = "",
                  ConfigRuntimeBinding runtimeBinding = ConfigRuntimeBinding::LiveSetter);

    // ── 只读 schema 绑定: 用于 uint8/uint16/uint32 等由 applyConfig() 手动转换的键 ──
    void bindSchema(std::string_view yamlPath,
                    ConfigValue defaultValue,
                    std::string_view typeName,
                    ConfigRange range = {},
                    std::string_view description = "",
                    ConfigRuntimeBinding runtimeBinding = ConfigRuntimeBinding::SchemaOnly);

    // ── 从 ConfigStore 读取值并写入所有绑定成员 ──
    void apply(const ConfigStore& store);

    // ── 将所有默认值写入 ConfigStore ──
    void writeDefaults(ConfigStore& store) const;

    // ── 从绑定生成 Schema (用于校验) ──
    void populateSchema(ConfigStore& schemaStore) const;

    // ── 返回所有绑定的当前 schema + 当前值 ──
    ConfigSchemaSnapshot snapshot() const;

    // ── 将当前绑定值写入 ConfigStore ──
    void writeCurrent(ConfigStore& store) const;

    // ── 返回所有不重复的 moduleTag ──
    std::vector<std::string> moduleTags() const;

    // ── 访问绑定列表 (用于 UI / 调试) ──
    const std::vector<BindingEntry>& bindings() const { return m_bindings; }

private:
    std::vector<BindingEntry> m_bindings;

    // 辅助：确定类型名
    template<typename T>
    static std::string typeNameFor();

    // 辅助：从 ConfigValue 提取值并赋值给成员
    template<typename T>
    static T extractValue(const ConfigValue& cv, T fallback);
};

// ── 模板实现 ──

template<typename T>
std::string ConfigBinder::typeNameFor() {
    if constexpr (std::is_same_v<T, bool>) return "bool";
    else if constexpr (std::is_same_v<T, int32_t>) return "int";
    else if constexpr (std::is_same_v<T, float>) return "float";
    else if constexpr (std::is_same_v<T, std::string>) return "string";
    else return "unknown";
}

template<typename T>
T ConfigBinder::extractValue(const ConfigValue& cv, T fallback) {
    if (auto opt = tryGetValue<T>(cv)) return *opt;
    // 数值类型兼容转换
    if constexpr (std::is_same_v<T, int32_t>) {
        if (auto opt = tryGetValue<float>(cv)) return static_cast<int32_t>(*opt);
    }
    if constexpr (std::is_same_v<T, float>) {
        if (auto opt = tryGetValue<int32_t>(cv)) return static_cast<float>(*opt);
    }
    return fallback;
}

template<typename Struct, typename T>
void ConfigBinder::bind(std::string_view yamlPath,
                        T Struct::*member, Struct& instance,
                        T defaultValue,
                        ConfigRange range,
                        std::string_view description,
                        ConfigRuntimeBinding runtimeBinding) {
    BindingEntry entry;
    entry.yamlPath = yamlPath;
    entry.description = description;
    entry.defaultValue = ConfigValue(defaultValue);
    entry.typeName = typeNameFor<T>();
    entry.keyId = tryKeyIdForPath(yamlPath);
    entry.displayName = deriveDisplayName(yamlPath);
    entry.moduleTag = deriveModuleTag(yamlPath);
    entry.runtimeBinding = runtimeBinding;
    if (range.min != 0.0 || range.max != 0.0) {
        entry.range = range;
    }

    // 捕获 setter
    entry.setter = [&instance, member](const ConfigValue& cv) {
        instance.*member = extractValue<T>(cv, instance.*member);
    };

    // 捕获 getter
    entry.getter = [&instance, member]() -> ConfigValue {
        return ConfigValue(instance.*member);
    };

    m_bindings.push_back(std::move(entry));
}

template<typename Struct, typename Enum>
void ConfigBinder::bindEnum(std::string_view yamlPath,
                            Enum Struct::*member, Struct& instance,
                            Enum defaultValue,
                            std::span<const std::pair<Enum, std::string>> enumMapping,
                            std::string_view description,
                            ConfigRuntimeBinding runtimeBinding) {
    BindingEntry entry;
    entry.yamlPath = yamlPath;
    entry.description = description;
    entry.defaultValue = ConfigValue(std::string{}); // 枚举存为 string
    entry.typeName = "enum";
    entry.keyId = tryKeyIdForPath(yamlPath);
    entry.displayName = deriveDisplayName(yamlPath);
    entry.moduleTag = deriveModuleTag(yamlPath);
    entry.runtimeBinding = runtimeBinding;

    for (const auto& [val, name] : enumMapping) {
        const auto intValue = static_cast<int>(val);
        entry.enumMapping.emplace_back(intValue, name);
        if (val == defaultValue) {
            entry.defaultValue = ConfigValue(name);
        }
    }

    // setter: string → enum
    entry.setter = [&instance, member, mapping = entry.enumMapping](const ConfigValue& cv) {
        if (auto str = tryGetValue<std::string>(cv)) {
            for (const auto& [val, name] : mapping) {
                if (name == *str) {
                    instance.*member = static_cast<Enum>(val);
                    return;
                }
            }
        }
    };

    // getter: enum → string
    entry.getter = [&instance, member, mapping = entry.enumMapping]() -> ConfigValue {
        const int current = static_cast<int>(instance.*member);
        for (const auto& [val, name] : mapping) {
            if (val == current) {
                return ConfigValue(name);
            }
        }
        return ConfigValue(std::string{});
    };

    m_bindings.push_back(std::move(entry));
}

} // namespace Config
