#pragma once
#include "ConfigSchema.h"
#include <vector>
#include <string>

namespace App {

// 将 ConfigParam 渲染为 ImGui 控件
class ConfigUIRenderer {
public:
    static void RenderConfigSchema(
        const std::vector<Engine::ConfigParam>& schema,
        const std::string& sectionName);
};

} // namespace App
