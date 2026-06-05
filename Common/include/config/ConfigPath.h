#pragma once

#include <optional>
#include <string>

namespace Config {

struct ConfigPaths {
    std::string defaultConfig;   // config/default.yaml 的完整路径
    std::string overrideConfig;  // config/overrides.yaml 的完整路径
    std::string baseDir;         // config/ 目录路径
    bool overrideExists;         // overrides.yaml 是否存在
};

// 按优先级解析配置目录:
// 1. cliOverride — --config <dir> CLI 参数
// 2. EGOTOUCH_CONFIG_DIR 环境变量
// 3. ./config/ 可执行文件同目录
// 4. 返回 nullopt (调用者应报错退出)
//
// resolve() 不包含加载逻辑，仅解析路径并验证 default.yaml 存在。
std::optional<ConfigPaths> resolve(const std::optional<std::string>& cliOverride = std::nullopt);

} // namespace Config
