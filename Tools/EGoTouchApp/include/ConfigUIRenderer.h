#pragma once
#include "ConfigSchema.h"
#include <vector>
#include <string>
#include <optional>

namespace App {

// 将 ConfigParam 渲染为 ImGui 控件
class ConfigUIRenderer {
public:
    static void RenderConfigSchema(
        const std::vector<Engine::ConfigParam>& schema,
        const std::string& sectionName,
        std::optional<Engine::ConfigParam::Category> filterCategory = std::nullopt);
};

} // namespace App
