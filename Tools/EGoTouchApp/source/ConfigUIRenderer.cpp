#include "ConfigUIRenderer.h"
#include "imgui.h"

namespace App {

void ConfigUIRenderer::RenderConfigSchema(
    const std::vector<Engine::ConfigParam>& schema,
    const std::string& sectionName) {

    if (schema.empty()) return;

    for (const auto& param : schema) {
        switch (param.type) {
            case Engine::ConfigParam::Bool:
                ImGui::Checkbox(param.displayName.c_str(),
                               static_cast<bool*>(param.valuePtr));
                break;

            case Engine::ConfigParam::Int:
                if (param.maxVal > param.minVal) {
                    ImGui::SliderInt(param.displayName.c_str(),
                                    static_cast<int*>(param.valuePtr),
                                    static_cast<int>(param.minVal),
                                    static_cast<int>(param.maxVal));
                } else {
                    ImGui::InputInt(param.displayName.c_str(),
                                   static_cast<int*>(param.valuePtr));
                }
                break;

            case Engine::ConfigParam::Float:
                if (param.maxVal > param.minVal) {
                    ImGui::SliderFloat(param.displayName.c_str(),
                                      static_cast<float*>(param.valuePtr),
                                      param.minVal, param.maxVal);
                } else {
                    ImGui::InputFloat(param.displayName.c_str(),
                                     static_cast<float*>(param.valuePtr));
                }
                break;

            case Engine::ConfigParam::Double:
                if (param.maxVal > param.minVal) {
                    float fval = static_cast<float>(*static_cast<double*>(param.valuePtr));
                    if (ImGui::SliderFloat(param.displayName.c_str(), &fval,
                                          param.minVal, param.maxVal)) {
                        *static_cast<double*>(param.valuePtr) = static_cast<double>(fval);
                    }
                } else {
                    double* dptr = static_cast<double*>(param.valuePtr);
                    float fval = static_cast<float>(*dptr);
                    if (ImGui::InputFloat(param.displayName.c_str(), &fval)) {
                        *dptr = static_cast<double>(fval);
                    }
                }
                break;

            case Engine::ConfigParam::String:
                // String support can be added if needed
                ImGui::Text("%s: (string)", param.displayName.c_str());
                break;
        }
    }
}

} // namespace App
