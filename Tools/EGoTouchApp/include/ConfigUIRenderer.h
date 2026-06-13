#pragma once

#include "config/ConfigSchemaSnapshot.h"

#include <functional>
#include <optional>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace Config {
class ConfigStore;
}

namespace App {

enum class ConfigUIApplyState : uint8_t {
    Clean = 0,
    Pending,
    LiveApplied,
    StagedRestartRequired,
    Failed,
};

enum class ConfigUIPersistState : uint8_t {
    NotAttempted = 0,
    Persisted,
    Unpersisted,
    Failed,
};

struct ConfigUIPathState {
    bool dirty = false;
    ConfigUIApplyState applyState = ConfigUIApplyState::Clean;
    ConfigUIPersistState persistState = ConfigUIPersistState::NotAttempted;
    Config::ConfigKeyId failedKeyId = Config::ConfigKeyId::MaxKeyId;
    std::string errorMessage;
};

inline const char* ConfigScopeBadge(Config::ConfigScope scope) noexcept {
    switch (scope) {
    case Config::ConfigScope::RuntimeOnly: return "Runtime";
    case Config::ConfigScope::ServicePolicy: return "Service";
    case Config::ConfigScope::TouchPipeline: return "Touch";
    case Config::ConfigScope::StylusPipeline: return "Stylus";
    case Config::ConfigScope::Debug: return "Debug";
    }
    return "Scope?";
}

inline const char* ConfigApplyTimingBadge(Config::ConfigApplyTiming timing) noexcept {
    switch (timing) {
    case Config::ConfigApplyTiming::ReadOnly: return "ReadOnly";
    case Config::ConfigApplyTiming::Immediate: return "Immediate";
    case Config::ConfigApplyTiming::FrameBoundary: return "Frame";
    case Config::ConfigApplyTiming::Manual: return "Manual";
    case Config::ConfigApplyTiming::RestartRequired: return "Restart";
    case Config::ConfigApplyTiming::StartupOnly: return "Startup";
    }
    return "Timing?";
}

inline const char* ConfigPersistPolicyBadge(Config::ConfigPersistPolicy policy) noexcept {
    switch (policy) {
    case Config::ConfigPersistPolicy::RuntimeOnly: return "RuntimeOnly";
    case Config::ConfigPersistPolicy::UserOverride: return "UserOverride";
    case Config::ConfigPersistPolicy::GeneratedDefault: return "Generated";
    case Config::ConfigPersistPolicy::Deprecated: return "Deprecated";
    }
    return "Persist?";
}

inline const char* ConfigApplyStateBadge(ConfigUIApplyState state) noexcept {
    switch (state) {
    case ConfigUIApplyState::Clean: return "Clean";
    case ConfigUIApplyState::Pending: return "Pending";
    case ConfigUIApplyState::LiveApplied: return "LiveApplied";
    case ConfigUIApplyState::StagedRestartRequired: return "StagedRestartRequired";
    case ConfigUIApplyState::Failed: return "Failed";
    }
    return "Apply?";
}

inline const char* ConfigPersistStateBadge(ConfigUIPersistState state) noexcept {
    switch (state) {
    case ConfigUIPersistState::NotAttempted: return "NotAttempted";
    case ConfigUIPersistState::Persisted: return "Persisted";
    case ConfigUIPersistState::Unpersisted: return "Unpersisted";
    case ConfigUIPersistState::Failed: return "Failed";
    }
    return "Persist?";
}

class ConfigUIRenderer {
public:
    using ConfigPathStateProvider =
        std::function<std::optional<ConfigUIPathState>(std::string_view path)>;

    static void RenderConfigStore(
        const Config::ConfigSchemaSnapshot& schema,
        Config::ConfigStore& values,
        const std::string& sectionName,
        std::vector<std::string>* changedPaths = nullptr,
        ConfigPathStateProvider pathStateProvider = {},
        bool editable = true);

    static void RenderConfigStoreByModule(
        const Config::ConfigSchemaSnapshot& schema,
        Config::ConfigStore& values,
        const std::string& moduleTag,
        std::vector<std::string>* changedPaths = nullptr,
        ConfigPathStateProvider pathStateProvider = {},
        bool editable = true);

    static std::vector<std::string> CollectModuleTags(
        const Config::ConfigSchemaSnapshot& schema);
};

} // namespace App
