#include "ConfigUIRenderer.h"
#include "imgui.h"
#include <algorithm>
#include <optional>

namespace App {

void ConfigUIRenderer::RenderConfigSchema(
    const std::vector<Solvers::ConfigParam>& schema,
    const std::string& sectionName,
    std::optional<Solvers::ConfigParam::Category> filterCategory) {

    if (schema.empty()) return;

    for (const auto& param : schema) {
        if (filterCategory.has_value() && param.category != filterCategory.value()) {
            continue;
        }

        // Appending ## makes the ID unique in ImGui without rendering the suffix
        std::string label = param.displayName + "##" + sectionName + "_" + param.key;
        
        switch (param.type) {
            case Solvers::ConfigParam::Bool:
                ImGui::Checkbox(label.c_str(),
                               static_cast<bool*>(param.valuePtr));
                break;

            case Solvers::ConfigParam::Int: {
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

            case Solvers::ConfigParam::Float: {
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

            case Solvers::ConfigParam::Double: {
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

            case Solvers::ConfigParam::String:
                // String support can be added if needed
                ImGui::Text("%s: (string)", label.c_str());
                break;
        }
    }
}

void ConfigUIRenderer::RenderConfigSchemaByModule(
    const std::vector<Solvers::ConfigParam>& schema,
    const std::string& moduleTag) {

    for (const auto& param : schema) {
        if (param.moduleTag != moduleTag) continue;

        std::string label = param.displayName + "##mod_" + moduleTag + "_" + param.key;

        switch (param.type) {
            case Solvers::ConfigParam::Bool:
                ImGui::Checkbox(label.c_str(),
                               static_cast<bool*>(param.valuePtr));
                break;

            case Solvers::ConfigParam::Int: {
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

            case Solvers::ConfigParam::Float: {
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

            case Solvers::ConfigParam::Double: {
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

            case Solvers::ConfigParam::String:
                ImGui::Text("%s: (string)", label.c_str());
                break;
        }
    }
}

} // namespace App
