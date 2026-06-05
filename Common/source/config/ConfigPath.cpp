#include "config/ConfigPath.h"

#include "Logger.h"

#include <cstdlib>
#include <filesystem>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace Config {

static std::string exeDir() {
#ifdef _WIN32
    std::vector<wchar_t> buf(MAX_PATH);
    DWORD len = GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
    if (len == 0 || len >= buf.size()) {
        return ".";
    }
    std::filesystem::path exePath(buf.data(), buf.data() + len);
    return exePath.parent_path().string();
#else
    // Non-Windows fallback — 尝试 /proc/self/exe
    // 本项目仅支持 Windows，此处仅保证编译通过
    return ".";
#endif
}

std::optional<ConfigPaths> resolve(const std::optional<std::string>& cliOverride) {
    namespace fs = std::filesystem;

    // 1. CLI 覆盖
    if (cliOverride.has_value() && !cliOverride->empty()) {
        fs::path dir(*cliOverride);
        fs::path defaultYaml = dir / "default.yaml";
        if (fs::exists(defaultYaml)) {
            ConfigPaths paths;
            paths.baseDir = dir.string();
            paths.defaultConfig = defaultYaml.string();
            paths.overrideConfig = (dir / "overrides.yaml").string();
            paths.overrideExists = fs::exists(paths.overrideConfig);
            LOG_INFO("Config", __func__, "Path", "Using config dir (CLI): {}", paths.baseDir);
            return paths;
        }
        LOG_WARN("Config", __func__, "Path", "CLI config dir '{}' has no default.yaml, falling back", *cliOverride);
    }

    // 2. 环境变量
    const char* envDir = std::getenv("EGOTOUCH_CONFIG_DIR");
    if (envDir && envDir[0] != '\0') {
        fs::path dir(envDir);
        fs::path defaultYaml = dir / "default.yaml";
        if (fs::exists(defaultYaml)) {
            ConfigPaths paths;
            paths.baseDir = dir.string();
            paths.defaultConfig = defaultYaml.string();
            paths.overrideConfig = (dir / "overrides.yaml").string();
            paths.overrideExists = fs::exists(paths.overrideConfig);
            LOG_INFO("Config", __func__, "Path", "Using config dir (env): {}", paths.baseDir);
            return paths;
        }
        LOG_WARN("Config", __func__, "Path", "Env config dir '{}' has no default.yaml, falling back", envDir);
    }

    // 3. ./config/ 可执行文件同目录
    fs::path exePath = exeDir();
    fs::path defaultDir = exePath / "config";
    fs::path defaultYaml = defaultDir / "default.yaml";
    if (fs::exists(defaultYaml)) {
        ConfigPaths paths;
        paths.baseDir = defaultDir.string();
        paths.defaultConfig = defaultYaml.string();
        paths.overrideConfig = (defaultDir / "overrides.yaml").string();
        paths.overrideExists = fs::exists(paths.overrideConfig);
        LOG_INFO("Config", __func__, "Path", "Using config dir (exe-relative): {}", paths.baseDir);
        return paths;
    }

    // 4. 启动失败
    LOG_ERROR("Config", __func__, "Path", "Cannot find config/default.yaml in any search path");
    return std::nullopt;
}

} // namespace Config
