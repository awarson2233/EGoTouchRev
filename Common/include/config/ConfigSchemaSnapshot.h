#pragma once

#include "ConfigKeyId.h"
#include "ConfigValue.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Config {

// ── UI 类型分类 ──
enum class ConfigUiType : uint8_t {
    Bool,
    Int32,
    Float,
    String,
    Enum,
};

enum class ConfigRuntimeBinding : uint8_t {
    SchemaOnly,
    LiveSetter,
    ManualLiveApply,
    Removed,
};

// ── 单个配置键的完整元数据 ──
struct ConfigSchemaEntry {
    std::string yamlPath;
    ConfigKeyId keyId = ConfigKeyId::MaxKeyId;
    ConfigUiType uiType = ConfigUiType::String;
    ConfigValue defaultValue;           // 来自 default.yaml 或 binder default
    ConfigValue currentValue;           // 来自 ConfigStore 或 binder getter
    std::optional<ConfigRange> range;   // 来自 binder
    std::string displayName;            // 人工可读名称
    std::string description;            // 来自 binder
    std::string moduleTag;              // UI 模块分组 (e.g. "Touch / Signal Conditioning")
    std::vector<std::pair<int, std::string>> enumMapping;  // 枚举映射
    ConfigRuntimeBinding runtimeBinding = ConfigRuntimeBinding::SchemaOnly;
    bool boundToRuntime = false;        // 是否有 live setter 或 manual live apply
};

// ── 全量 schema snapshot ──
struct ConfigSchemaSnapshot {
    std::vector<ConfigSchemaEntry> entries;   // 按 keyId 排序
};

// ── 工具函数 ──

// 从路径推导模块标签
std::string deriveModuleTag(std::string_view yamlPath);

// 从路径推导显示名
std::string deriveDisplayName(std::string_view yamlPath);

// 从 ConfigValue 推导 UIConfigType
ConfigUiType deriveUiType(const ConfigValue& value);

} // namespace Config
