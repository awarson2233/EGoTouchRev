#pragma once

#include <string>
#include <vector>

namespace Config {

class ConfigBinder;
class ConfigStore;

struct ValidationIssue {
    enum Severity { Error, Warning };
    Severity severity;
    std::string path;
    std::string message;
};

struct ValidationResult {
    bool ok() const { return errors.empty(); }
    std::vector<ValidationIssue> errors;
    std::vector<ValidationIssue> warnings;
    void logAll() const;
};

class SchemaValidator {
public:
    // 从 ConfigBinder 的绑定信息校验 ConfigStore 中的所有值
    static ValidationResult validate(const ConfigStore& store, const ConfigBinder& binder);
};

} // namespace Config
