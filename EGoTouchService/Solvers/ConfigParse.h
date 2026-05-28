#pragma once

#include "Logger.h"

#include <cctype>
#include <cmath>
#include <stdexcept>
#include <string>

namespace Solvers {

class ConfigParseError : public std::runtime_error {
public:
    explicit ConfigParseError(const char* message)
        : std::runtime_error(message) {}
};

inline bool HasOnlyTrailingSpace(const std::string& value, size_t offset) noexcept {
    while (offset < value.size()) {
        if (!std::isspace(static_cast<unsigned char>(value[offset]))) {
            return false;
        }
        ++offset;
    }
    return true;
}

inline int ParseConfigInt(const std::string&, const std::string& value) {
    try {
        size_t parsed = 0;
        const int result = std::stoi(value, &parsed);
        if (!HasOnlyTrailingSpace(value, parsed)) {
            throw ConfigParseError("invalid integer");
        }
        return result;
    } catch (const ConfigParseError&) {
        throw;
    } catch (const std::exception&) {
        throw ConfigParseError("invalid integer");
    }
}

inline float ParseConfigFloat(const std::string&, const std::string& value) {
    try {
        size_t parsed = 0;
        const float result = std::stof(value, &parsed);
        if (!HasOnlyTrailingSpace(value, parsed) || !std::isfinite(result)) {
            throw ConfigParseError("invalid finite float");
        }
        return result;
    } catch (const ConfigParseError&) {
        throw;
    } catch (const std::exception&) {
        throw ConfigParseError("invalid finite float");
    }
}

inline bool ParseConfigBool(const std::string&, const std::string& value) {
    if (value == "1" || value == "true") {
        return true;
    }
    if (value == "0" || value == "false") {
        return false;
    }
    throw ConfigParseError("invalid boolean");
}

inline void LogConfigParseWarning(const char* component,
                                  const char* functionName,
                                  const std::string& key,
                                  const std::string& value,
                                  const ConfigParseError& error) {
    LOG_WARN(component, functionName, "Config",
             "Ignoring config value {}='{}': {}",
             key, value, error.what());
}

} // namespace Solvers
