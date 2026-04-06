#include "ConfigUIRenderer.h"
#include "imgui.h"
#include <algorithm>

namespace App {

void ConfigUIRenderer::RenderConfigSchema(
    const std::vector<Engine::ConfigParam>& schema,
    const std::string& sectionName) {

    if (schema.empty()) return;

    for (const auto& param : schema) {
        // Appending ## makes the ID unique in ImGui without rendering the suffix
        std::string label = param.displayName + "##" + sectionName + "_" + param.key;
        
        switch (param.type) {
            case Engine::ConfigParam::Bool:
                ImGui::Checkbox(label.c_str(),
                               static_cast<bool*>(param.valuePtr));
                break;

            case Engine::ConfigParam::Int: {
                int* ptr = static_cast<int*>(param.valuePtr);
                if (param.maxVal > param.minVal) {
                    if (ImGui::SliderInt(label.c_str(), ptr,
                                         static_cast<int>(param.minVal),
                                         static_cast<int>(param.maxVal))) {
                        *ptr = std::clamp(*ptr, static_cast<int>(param.minVal), static_cast<int>(param.maxVal));
                    }
                } else {
                    ImGui::InputInt(label.c_str(), ptr);
                }
                break;
            }

            case Engine::ConfigParam::Float: {
                float* ptr = static_cast<float*>(param.valuePtr);
                if (param.maxVal > param.minVal) {
                    if (ImGui::SliderFloat(label.c_str(), ptr, param.minVal, param.maxVal)) {
                        *ptr = std::clamp(*ptr, param.minVal, param.maxVal);
                    }
                } else {
                    ImGui::InputFloat(label.c_str(), ptr);
                }
                break;
            }

            case Engine::ConfigParam::Double: {
                double* ptr = static_cast<double*>(param.valuePtr);
                if (param.maxVal > param.minVal) {
                    float fval = static_cast<float>(*ptr);
                    if (ImGui::SliderFloat(label.c_str(), &fval, param.minVal, param.maxVal)) {
                        fval = std::clamp(fval, param.minVal, param.maxVal);
                        *ptr = static_cast<double>(fval);
                    }
                } else {
                    float fval = static_cast<float>(*ptr);
                    if (ImGui::InputFloat(label.c_str(), &fval)) {
                        *ptr = static_cast<double>(fval);
                    }
                }
                break;
            }

            case Engine::ConfigParam::String:
                // String support can be added if needed
                ImGui::Text("%s: (string)", label.c_str());
                break;
        }
    }
}

} // namespace App
