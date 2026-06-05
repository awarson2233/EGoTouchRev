#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>

namespace Config {

struct ConfigRange {
    double min = 0.0;
    double max = 0.0;
};

using ConfigValue = std::variant<
    bool,
    int32_t,
    float,
    std::string
>;

// 类型安全的取值辅助函数
template<typename T>
T getValue(const ConfigValue& v) {
    return std::get<T>(v);
}

template<typename T>
std::optional<T> tryGetValue(const ConfigValue& v) {
    if (std::holds_alternative<T>(v)) {
        return std::get<T>(v);
    }
    return std::nullopt;
}

// 序列化为字符串 (用于 YAML/JSON 写入)
inline std::string toString(const ConfigValue& v) {
    return std::visit([](const auto& val) -> std::string {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, bool>) {
            return val ? "true" : "false";
        } else if constexpr (std::is_same_v<T, std::string>) {
            return val;
        } else if constexpr (std::is_same_v<T, float>) {
            return std::to_string(val);
        } else {
            return std::to_string(val);
        }
    }, v);
}

} // namespace Config
