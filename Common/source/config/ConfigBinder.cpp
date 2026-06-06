#include "config/ConfigBinder.h"

#include "Logger.h"
#include "config/ConfigStore.h"

#include <algorithm>
#include <exception>
#include <set>

namespace Config {

void ConfigBinder::bindSchema(std::string_view yamlPath,
                              ConfigValue defaultValue,
                              std::string_view typeName,
                              ConfigRange range,
                              std::string_view description,
                              ConfigRuntimeBinding runtimeBinding) {
    BindingEntry entry;
    entry.yamlPath = yamlPath;
    entry.description = description;
    entry.defaultValue = std::move(defaultValue);
    entry.typeName = typeName;
    entry.keyId = tryKeyIdForPath(yamlPath);
    entry.displayName = deriveDisplayName(yamlPath);
    entry.moduleTag = deriveModuleTag(yamlPath);
    entry.runtimeBinding = runtimeBinding;
    if (range.min != 0.0 || range.max != 0.0) {
        entry.range = range;
    }
    entry.setter = [](const ConfigValue&) {};
    entry.getter = [value = entry.defaultValue]() -> ConfigValue { return value; };
    m_bindings.push_back(std::move(entry));
}

void ConfigBinder::apply(const ConfigStore& store) {
    for (auto& binding : m_bindings) {
        // 从 ConfigStore 读取值，如果键不存在则使用默认值
        if (store.has(binding.yamlPath)) {
            try {
                ConfigValue value = store.get<ConfigValue>(binding.yamlPath);
                binding.setter(value);
            } catch (const std::exception& ex) {
                LOG_WARN("Config", __func__, "Binder",
                         "Failed to apply config key '{}': {}, using default",
                         binding.yamlPath, ex.what());
                binding.setter(binding.defaultValue);
            }
        } else {
            // 键不存在，使用默认值
            binding.setter(binding.defaultValue);
        }
    }
}

void ConfigBinder::writeDefaults(ConfigStore& store) const {
    for (const auto& binding : m_bindings) {
        store.set<ConfigValue>(binding.yamlPath, binding.defaultValue);
    }
}

void ConfigBinder::populateSchema(ConfigStore& schemaStore) const {
    for (const auto& binding : m_bindings) {
        schemaStore.set<ConfigValue>(binding.yamlPath, binding.defaultValue);
        // 注意: range 信息需要另外的机制传递，这里仅存储默认值
        // SchemaValidator 可以通过 ConfigBinder 的 binding.defaultValue/range 校验
    }
}

ConfigSchemaSnapshot ConfigBinder::snapshot() const {
    ConfigSchemaSnapshot result;
    result.entries.reserve(m_bindings.size());
    for (const auto& b : m_bindings) {
        ConfigSchemaEntry entry;
        entry.yamlPath = b.yamlPath;
        entry.keyId = b.keyId.value_or(ConfigKeyId::MaxKeyId);
        entry.defaultValue = b.defaultValue;
        entry.currentValue = b.getter();
        entry.range = b.range;
        entry.displayName = b.displayName.empty() ? deriveDisplayName(b.yamlPath) : b.displayName;
        entry.description = b.description;
        entry.moduleTag = b.moduleTag.empty() ? deriveModuleTag(b.yamlPath) : b.moduleTag;
        entry.enumMapping = b.enumMapping;
        entry.runtimeBinding = b.runtimeBinding;
        entry.boundToRuntime = b.runtimeBinding == ConfigRuntimeBinding::LiveSetter ||
                               b.runtimeBinding == ConfigRuntimeBinding::ManualLiveApply;

        if (b.typeName == "enum") {
            entry.uiType = ConfigUiType::Enum;
        } else {
            entry.uiType = deriveUiType(b.defaultValue);
        }

        result.entries.push_back(std::move(entry));
    }

    std::ranges::sort(result.entries, [](const ConfigSchemaEntry& lhs, const ConfigSchemaEntry& rhs) {
        if (lhs.keyId != rhs.keyId) {
            return static_cast<uint16_t>(lhs.keyId) < static_cast<uint16_t>(rhs.keyId);
        }
        return lhs.yamlPath < rhs.yamlPath;
    });
    return result;
}

void ConfigBinder::writeCurrent(ConfigStore& store) const {
    for (const auto& binding : m_bindings) {
        store.set<ConfigValue>(binding.yamlPath, binding.getter());
    }
}

std::vector<std::string> ConfigBinder::moduleTags() const {
    std::set<std::string> uniqueTags;
    for (const auto& binding : m_bindings) {
        uniqueTags.insert(binding.moduleTag.empty() ? deriveModuleTag(binding.yamlPath) : binding.moduleTag);
    }
    return {uniqueTags.begin(), uniqueTags.end()};
}

} // namespace Config
