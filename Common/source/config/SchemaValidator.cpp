#include "config/SchemaValidator.h"

#include "Logger.h"
#include "config/ConfigBinder.h"
#include "config/ConfigStore.h"
#include "config/ConfigValue.h"

#include <cstdint>
#include <exception>
#include <string>

namespace Config {
namespace {

bool isNumericValue(const ConfigValue& value) {
    return tryGetValue<int32_t>(value).has_value() || tryGetValue<float>(value).has_value();
}

double numericValueOrZero(const ConfigValue& value) {
    if (auto intValue = tryGetValue<int32_t>(value)) {
        return static_cast<double>(*intValue);
    }
    if (auto floatValue = tryGetValue<float>(value)) {
        return static_cast<double>(*floatValue);
    }
    return 0.0;
}

bool enumContainsValue(const BindingEntry& binding, const std::string& value) {
    for (const auto& [_, name] : binding.enumMapping) {
        if (name == value) {
            return true;
        }
    }
    return false;
}

} // namespace

void ValidationResult::logAll() const {
    for (const auto& issue : errors) {
        LOG_ERROR("Config", __func__, "Validate", "[{}] {}", issue.path, issue.message);
    }
    for (const auto& issue : warnings) {
        LOG_WARN("Config", __func__, "Validate", "[{}] {}", issue.path, issue.message);
    }
}

ValidationResult SchemaValidator::validate(const ConfigStore& store, const ConfigBinder& binder) {
    ValidationResult result;

    for (const auto& binding : binder.bindings()) {
        if (!store.has(binding.yamlPath)) {
            // 键不在 store 中 → 使用默认值 (Warning)
            result.warnings.push_back({
                ValidationIssue::Warning,
                binding.yamlPath,
                "key not found in config, will use default value"
            });
            continue;
        }

        ConfigValue value;
        try {
            value = store.get<ConfigValue>(binding.yamlPath);
        } catch (const std::exception& ex) {
            result.errors.push_back({
                ValidationIssue::Error,
                binding.yamlPath,
                std::string("failed to read config value: ") + ex.what()
            });
            continue;
        }

        bool typeMatches = false;
        if (binding.typeName == "bool") {
            typeMatches = tryGetValue<bool>(value).has_value();
        } else if (binding.typeName == "int" || binding.typeName == "float") {
            // ConfigBinder 支持 int32_t/float 互转，因此数值类型互相兼容。
            typeMatches = isNumericValue(value);
        } else if (binding.typeName == "string") {
            typeMatches = tryGetValue<std::string>(value).has_value();
        } else if (binding.typeName == "enum") {
            if (auto stringValue = tryGetValue<std::string>(value)) {
                typeMatches = enumContainsValue(binding, *stringValue);
            }
        } else {
            result.warnings.push_back({
                ValidationIssue::Warning,
                binding.yamlPath,
                "binding has unknown type name"
            });
            continue;
        }

        if (!typeMatches) {
            result.errors.push_back({
                ValidationIssue::Error,
                binding.yamlPath,
                "type mismatch with binding schema"
            });
            continue;
        }

        if (binding.range.has_value()) {
            if (!isNumericValue(value)) {
                result.errors.push_back({
                    ValidationIssue::Error,
                    binding.yamlPath,
                    "range validation requires a numeric value"
                });
                continue;
            }

            const double numVal = numericValueOrZero(value);
            if (numVal < binding.range->min || numVal > binding.range->max) {
                result.errors.push_back({
                    ValidationIssue::Error,
                    binding.yamlPath,
                    "value outside valid range"
                });
            }
        }
    }

    return result;
}

} // namespace Config
