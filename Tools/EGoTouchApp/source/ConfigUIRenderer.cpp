#include "ConfigUIRenderer.h"
#include "config/ConfigStore.h"
#include "imgui.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace App {
namespace {

bool IsLiveEditableEntry(const Config::ConfigSchemaEntry& entry) {
    return entry.boundToRuntime &&
           (entry.runtimeBinding == Config::ConfigRuntimeBinding::LiveSetter ||
            entry.runtimeBinding == Config::ConfigRuntimeBinding::ManualLiveApply) &&
           Config::isLiveApplyTiming(entry.applyTiming);
}

ImVec4 ApplyStateColor(ConfigUIApplyState state) {
    switch (state) {
    case ConfigUIApplyState::Clean: return ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
    case ConfigUIApplyState::Pending: return ImVec4(1.0f, 0.78f, 0.20f, 1.0f);
    case ConfigUIApplyState::LiveApplied: return ImVec4(0.30f, 0.85f, 0.35f, 1.0f);
    case ConfigUIApplyState::StagedRestartRequired: return ImVec4(1.0f, 0.62f, 0.20f, 1.0f);
    case ConfigUIApplyState::Failed: return ImVec4(1.0f, 0.35f, 0.30f, 1.0f);
    }
    return ImGui::GetStyleColorVec4(ImGuiCol_Text);
}

ImVec4 PersistStateColor(ConfigUIPersistState state) {
    switch (state) {
    case ConfigUIPersistState::NotAttempted: return ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
    case ConfigUIPersistState::Persisted: return ImVec4(0.30f, 0.85f, 0.35f, 1.0f);
    case ConfigUIPersistState::Unpersisted: return ImVec4(1.0f, 0.78f, 0.20f, 1.0f);
    case ConfigUIPersistState::Failed: return ImVec4(1.0f, 0.35f, 0.30f, 1.0f);
    }
    return ImGui::GetStyleColorVec4(ImGuiCol_Text);
}

bool DrawEntryBadges(const Config::ConfigSchemaEntry& entry,
                     const std::optional<ConfigUIPathState>& pathState) {
    const ImGuiStyle& style = ImGui::GetStyle();
    const char* scope = ConfigScopeBadge(entry.scope);
    const char* timing = ConfigApplyTimingBadge(entry.applyTiming);
    const char* policy = ConfigPersistPolicyBadge(entry.persistPolicy);
    const float strategyWidth = ImGui::CalcTextSize("[Stylus] [Restart] [UserOverride]").x;
    const bool sameLine = ImGui::GetContentRegionAvail().x > strategyWidth + style.ItemInnerSpacing.x;
    bool hovered = false;

    if (sameLine) {
        ImGui::SameLine();
    } else {
        ImGui::Indent(style.IndentSpacing);
    }

    ImGui::TextDisabled("[%s] [%s] [%s]", scope, timing, policy);
    hovered = hovered || ImGui::IsItemHovered();

    if (pathState.has_value()) {
        ImGui::SameLine();
        ImGui::TextColored(
            ApplyStateColor(pathState->applyState),
            "[Apply:%s%s]",
            ConfigApplyStateBadge(pathState->applyState),
            pathState->dirty ? "*" : "");
        hovered = hovered || ImGui::IsItemHovered();

        ImGui::SameLine();
        ImGui::TextColored(
            PersistStateColor(pathState->persistState),
            "[Persist:%s]",
            ConfigPersistStateBadge(pathState->persistState));
        hovered = hovered || ImGui::IsItemHovered();
    }

    if (!sameLine) {
        ImGui::Unindent(style.IndentSpacing);
    }

    return hovered;
}

void DrawEntryTooltip(const Config::ConfigSchemaEntry& entry,
                      const std::optional<ConfigUIPathState>& pathState) {
    ImGui::BeginTooltip();
    if (!entry.description.empty()) {
        ImGui::TextWrapped("%s", entry.description.c_str());
        ImGui::Separator();
    }
    ImGui::Text("Path: %s", entry.yamlPath.c_str());
    ImGui::Text("Scope: %s", ConfigScopeBadge(entry.scope));
    ImGui::Text("Apply: %s", ConfigApplyTimingBadge(entry.applyTiming));
    ImGui::Text("Persist: %s", ConfigPersistPolicyBadge(entry.persistPolicy));
    if (pathState.has_value()) {
        ImGui::Separator();
        ImGui::Text("Apply State: %s%s",
                    ConfigApplyStateBadge(pathState->applyState),
                    pathState->dirty ? " (dirty)" : "");
        ImGui::Text("Persist State: %s", ConfigPersistStateBadge(pathState->persistState));
        if (pathState->failedKeyId != Config::ConfigKeyId::MaxKeyId) {
            ImGui::Text("failedKeyId: 0x%04X",
                        static_cast<unsigned int>(static_cast<uint16_t>(pathState->failedKeyId)));
        }
        if (!pathState->errorMessage.empty()) {
            ImGui::TextWrapped("errorMessage: %s", pathState->errorMessage.c_str());
        }
    }
    ImGui::EndTooltip();
}

} // namespace

void ConfigUIRenderer::RenderConfigStore(
    const Config::ConfigSchemaSnapshot& schema,
    Config::ConfigStore& values,
    const std::string& sectionName,
    std::vector<std::string>* changedPaths,
    ConfigPathStateProvider pathStateProvider) {

    (void)sectionName;

    auto recordChange = [changedPaths](const std::string& path) {
        if (changedPaths) {
            changedPaths->push_back(path);
        }
    };

    for (const auto& entry : schema.entries) {
        Config::ConfigValue currentValue;
        if (values.has(entry.yamlPath)) {
            currentValue = values.get<Config::ConfigValue>(entry.yamlPath);
        } else if (entry.uiType != Config::ConfigUiType::String || !Config::toString(entry.currentValue).empty()) {
            currentValue = entry.currentValue;
        } else {
            currentValue = entry.defaultValue;
        }

        const std::string label = entry.displayName + "##" + entry.yamlPath;
        const bool hasRange = entry.range.has_value();
        const bool liveEditable = IsLiveEditableEntry(entry);
        const std::optional<ConfigUIPathState> pathState =
            pathStateProvider ? pathStateProvider(entry.yamlPath) : std::nullopt;
        if (!liveEditable) {
            ImGui::BeginDisabled();
        }

        switch (entry.uiType) {
            case Config::ConfigUiType::Bool: {
                bool val = Config::tryGetValue<bool>(currentValue).value_or(false);
                if (ImGui::Checkbox(label.c_str(), &val)) {
                    values.set<bool>(entry.yamlPath, val);
                    recordChange(entry.yamlPath);
                }
                break;
            }

            case Config::ConfigUiType::Int32: {
                int val = Config::tryGetValue<int32_t>(currentValue).value_or(0);
                const int minV = hasRange ? static_cast<int>(entry.range->min) : 0;
                const int maxV = hasRange ? static_cast<int>(entry.range->max) : 100;
                if (hasRange) {
                    if (ImGui::SliderInt(label.c_str(), &val, minV, maxV)) {
                        values.set<int32_t>(entry.yamlPath, static_cast<int32_t>(val));
                        recordChange(entry.yamlPath);
                    }
                } else if (ImGui::InputInt(label.c_str(), &val)) {
                    values.set<int32_t>(entry.yamlPath, static_cast<int32_t>(val));
                    recordChange(entry.yamlPath);
                }
                break;
            }

            case Config::ConfigUiType::Float: {
                float val = Config::tryGetValue<float>(currentValue).value_or(0.0f);
                const float minF = hasRange ? static_cast<float>(entry.range->min) : 0.0f;
                const float maxF = hasRange ? static_cast<float>(entry.range->max) : 1.0f;
                if (hasRange) {
                    if (ImGui::SliderFloat(label.c_str(), &val, minF, maxF)) {
                        values.set<float>(entry.yamlPath, val);
                        recordChange(entry.yamlPath);
                    }
                } else if (ImGui::InputFloat(label.c_str(), &val)) {
                    values.set<float>(entry.yamlPath, val);
                    recordChange(entry.yamlPath);
                }
                break;
            }

            case Config::ConfigUiType::Enum: {
                auto strVal = Config::tryGetValue<std::string>(currentValue).value_or(std::string{});
                int selectedIdx = 0;
                std::vector<const char*> items;
                items.reserve(entry.enumMapping.size());
                for (size_t i = 0; i < entry.enumMapping.size(); ++i) {
                    items.push_back(entry.enumMapping[i].second.c_str());
                    if (entry.enumMapping[i].second == strVal) selectedIdx = static_cast<int>(i);
                }
                if (!items.empty() && ImGui::Combo(label.c_str(), &selectedIdx, items.data(), static_cast<int>(items.size()))) {
                    if (selectedIdx >= 0 && selectedIdx < static_cast<int>(entry.enumMapping.size())) {
                        values.set<std::string>(entry.yamlPath, entry.enumMapping[selectedIdx].second);
                        recordChange(entry.yamlPath);
                    }
                }
                break;
            }

            case Config::ConfigUiType::String: {
                auto strVal = Config::tryGetValue<std::string>(currentValue).value_or(std::string{});
                char buf[256]{};
                const size_t copyLen = std::min(strVal.size(), sizeof(buf) - 1);
                std::memcpy(buf, strVal.data(), copyLen);
                if (ImGui::InputText(label.c_str(), buf, sizeof(buf))) {
                    values.set<std::string>(entry.yamlPath, std::string(buf));
                    recordChange(entry.yamlPath);
                }
                break;
            }
        }

        const bool valueHovered = ImGui::IsItemHovered();
        if (!liveEditable) {
            ImGui::EndDisabled();
        }

        const bool badgesHovered = DrawEntryBadges(entry, pathState);
        if (valueHovered || badgesHovered) {
            DrawEntryTooltip(entry, pathState);
        }
    }
}

void ConfigUIRenderer::RenderConfigStoreByModule(
    const Config::ConfigSchemaSnapshot& schema,
    Config::ConfigStore& values,
    const std::string& moduleTag,
    std::vector<std::string>* changedPaths,
    ConfigPathStateProvider pathStateProvider) {

    Config::ConfigSchemaSnapshot filtered;
    for (const auto& entry : schema.entries) {
        if (entry.moduleTag == moduleTag) {
            filtered.entries.push_back(entry);
        }
    }

    RenderConfigStore(filtered, values, moduleTag, changedPaths, std::move(pathStateProvider));
}

std::vector<std::string> ConfigUIRenderer::CollectModuleTags(
    const Config::ConfigSchemaSnapshot& schema) {

    std::vector<std::string> tags;
    tags.reserve(schema.entries.size());
    for (const auto& entry : schema.entries) {
        if (!entry.moduleTag.empty()) {
            tags.push_back(entry.moduleTag);
        }
    }

    std::sort(tags.begin(), tags.end());
    tags.erase(std::unique(tags.begin(), tags.end()), tags.end());
    return tags;
}

} // namespace App
