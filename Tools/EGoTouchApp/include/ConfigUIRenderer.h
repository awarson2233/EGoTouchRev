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
        const std::vector<Solvers::ConfigParam>& schema,
        const std::string& sectionName,
        std::optional<Solvers::ConfigParam::Category> filterCategory = std::nullopt);

    /// Render only params whose moduleTag matches the given tag.
    static void RenderConfigSchemaByModule(
        const std::vector<Solvers::ConfigParam>& schema,
        const std::string& moduleTag);
};

} // namespace App
