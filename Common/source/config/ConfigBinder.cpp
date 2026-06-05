#include "config/ConfigBinder.h"

#include "Logger.h"
#include "config/ConfigStore.h"

#include <exception>

namespace Config {

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

} // namespace Config
