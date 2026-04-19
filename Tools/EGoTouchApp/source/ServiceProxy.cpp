#include "ServiceProxyInternal.h"

#include <sstream>

namespace App {

const std::string kConfigPath = "C:/ProgramData/EGoTouchRev/config.ini";

std::string TrimCopy(std::string_view input) {
    if (input.size() >= 3 &&
        static_cast<unsigned char>(input[0]) == 0xEF &&
        static_cast<unsigned char>(input[1]) == 0xBB &&
        static_cast<unsigned char>(input[2]) == 0xBF) {
        input.remove_prefix(3);
    }
    const size_t start = input.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) return {};
    const size_t end = input.find_last_not_of(" \t\r\n");
    return std::string(input.substr(start, end - start + 1));
}

bool ParseIniKeyValue(std::string_view line, std::string& key, std::string& value) {
    const size_t eq = line.find('=');
    if (eq == std::string_view::npos) return false;
    key = TrimCopy(line.substr(0, eq));
    value = TrimCopy(line.substr(eq + 1));
    return !key.empty();
}

bool IsLegacyTouchSection(const std::string& section) {
    return section == "Master Frame Parser" ||
           section == "Baseline Subtraction" ||
           section == "CMF Processor" ||
           section == "Grid IIR Processor" ||
           section == "Feature Extractor (4.1/4.2)" ||
           section == "Touch Tracker (IDT)" ||
           section == "Coordinate Filter (1 Euro)" ||
           section == "TouchGestureStateMachine";
}

std::optional<std::string> MapLegacyTouchKey(const std::string& section,
                                             const std::string& key) {
    if (section == "Master Frame Parser") {
        if (key == "Enabled") return std::string("FrameParserEnabled");
        return std::nullopt;
    }
    if (section == "Baseline Subtraction") {
        if (key == "Enabled") return std::string("BaselineEnabled");
        return key;
    }
    if (section == "CMF Processor") {
        if (key == "Enabled") return std::string("CMFEnabled");
        if (key == "DimensionMode") return std::string("CMFDimensionMode");
        if (key == "ExclusionThreshold") return std::string("CMFExclusionThreshold");
        if (key == "MaxCorrection") return std::string("CMFMaxCorrection");
        return key;
    }
    if (section == "Grid IIR Processor") {
        if (key == "Enabled") return std::string("GridIIREnabled");
        return key;
    }
    if (section == "Feature Extractor (4.1/4.2)") {
        if (key == "Enabled") return std::nullopt;
        return key;
    }
    if (section == "Touch Tracker (IDT)") {
        if (key == "Enabled") return std::string("TrackerEnabled");
        return key;
    }
    if (section == "Coordinate Filter (1 Euro)") {
        if (key == "Enabled") return std::string("CoordFilterEnabled");
        return key;
    }
    if (section == "TouchGestureStateMachine") {
        if (key == "Enabled") return std::string("GestureEnabled");
        return key;
    }
    return std::nullopt;
}

TouchPipelineModuleEnableState CaptureTouchPipelineModuleEnableState(
    const Solvers::TouchPipeline& pipeline) {
    TouchPipelineModuleEnableState state;
    state.baselineEnabled = pipeline.m_baseline.m_enabled;
    state.cmfEnabled = pipeline.m_cmf.m_enabled;
    state.gridIIREnabled = pipeline.m_gridIIR.m_enabled;
    state.trackerEnabled = pipeline.m_tracker.m_enabled;
    state.coordFilterEnabled = pipeline.m_coordFilter.m_enabled;
    state.gestureEnabled = pipeline.m_gesture.m_enabled;
    return state;
}

void ApplyTouchPipelineModuleEnableState(
    Solvers::TouchPipeline& pipeline,
    const TouchPipelineModuleEnableState& state) {
    pipeline.m_baseline.m_enabled = state.baselineEnabled;
    pipeline.m_cmf.m_enabled = state.cmfEnabled;
    pipeline.m_gridIIR.m_enabled = state.gridIIREnabled;
    pipeline.m_tracker.m_enabled = state.trackerEnabled;
    pipeline.m_coordFilter.m_enabled = state.coordFilterEnabled;
    pipeline.m_gesture.m_enabled = state.gestureEnabled;
}

std::string BuildServiceConfigSection(bool modeFull,
                                      bool autoMode,
                                      bool stylusVhfEnabled) {
    std::ostringstream out;
    out << "[Service]\n";
    out << "mode=" << (modeFull ? "full" : "touch_only") << "\n";
    out << "auto_mode=" << (autoMode ? "1" : "0") << "\n";
    out << "stylus_vhf_enabled=" << (stylusVhfEnabled ? "1" : "0") << "\n";
    return out.str();
}

std::string BuildTouchPipelineConfigSection(
    const Solvers::TouchPipeline& pipeline,
    const TouchPipelineModuleEnableState* persistedModuleState) {
    std::ostringstream saved;
    pipeline.SaveConfig(saved);
    const std::string savedText = saved.str();

    std::ostringstream out;
    out << "[TouchPipeline]\n";
    if (!persistedModuleState) {
        out << savedText;
        return out.str();
    }

    auto persistedValueForKey = [persistedModuleState](const std::string& key) -> const char* {
        if (key == "BaselineEnabled") return persistedModuleState->baselineEnabled ? "1" : "0";
        if (key == "CMFEnabled") return persistedModuleState->cmfEnabled ? "1" : "0";
        if (key == "GridIIREnabled") return persistedModuleState->gridIIREnabled ? "1" : "0";
        if (key == "TrackerEnabled") return persistedModuleState->trackerEnabled ? "1" : "0";
        if (key == "CoordFilterEnabled") return persistedModuleState->coordFilterEnabled ? "1" : "0";
        if (key == "GestureEnabled") return persistedModuleState->gestureEnabled ? "1" : "0";
        return nullptr;
    };

    std::istringstream in(savedText);
    std::string line;
    while (std::getline(in, line)) {
        std::string key;
        std::string value;
        if (ParseIniKeyValue(line, key, value)) {
            if (const char* persistedValue = persistedValueForKey(key)) {
                out << key << '=' << persistedValue << "\n";
                continue;
            }
        }
        out << line << "\n";
    }
    return out.str();
}

std::string BuildStylusPipelineConfigSection(
    const Solvers::StylusPipeline& pipeline) {
    std::ostringstream out;
    out << "[StylusPipeline]\n";
    pipeline.SaveConfig(out);
    return out.str();
}

std::string MergeServiceProxyConfigSections(
    std::string_view existingText,
    std::string_view serviceSection,
    std::string_view touchSection,
    std::string_view stylusSection) {
    auto isCanonicalSection = [](const std::string& section) {
        return section == "Service" ||
               section == "TouchPipeline" ||
               section == "StylusPipeline" ||
               IsLegacyTouchSection(section);
    };

    auto appendSection = [](std::string& out, std::string_view sectionText) {
        if (sectionText.empty()) return;
        if (!out.empty() && out.back() != '\n') out.push_back('\n');
        out.append(sectionText);
        if (out.empty() || out.back() != '\n') out.push_back('\n');
        out.push_back('\n');
    };

    auto extractSectionName = [](std::string_view headerLine) -> std::string {
        if (headerLine.size() >= 2 && headerLine.front() == '[' && headerLine.back() == ']') {
            return TrimCopy(headerLine.substr(1, headerLine.size() - 2));
        }
        return {};
    };

    std::string merged;
    bool wroteService = false;
    bool wroteTouch = false;
    bool wroteStylus = false;
    size_t pos = 0;

    while (pos < existingText.size()) {
        const size_t next = existingText.find('\n', pos);
        const size_t lineEnd = (next == std::string_view::npos) ? existingText.size() : next;
        const std::string_view lineView = existingText.substr(pos, lineEnd - pos);
        const bool hasNewline = next != std::string_view::npos;
        const std::string trimmed = TrimCopy(lineView);

        if (!trimmed.empty() && trimmed.front() == '[' && trimmed.back() == ']') {
            const std::string section = extractSectionName(trimmed);
            size_t sectionEnd = hasNewline ? next + 1 : lineEnd;
            while (sectionEnd < existingText.size()) {
                const size_t itemNext = existingText.find('\n', sectionEnd);
                const size_t itemEnd = (itemNext == std::string_view::npos) ? existingText.size() : itemNext;
                const std::string_view itemView = existingText.substr(sectionEnd, itemEnd - sectionEnd);
                const std::string itemTrimmed = TrimCopy(itemView);
                if (!itemTrimmed.empty() && itemTrimmed.front() == '[' && itemTrimmed.back() == ']') {
                    break;
                }
                sectionEnd = (itemNext == std::string_view::npos) ? itemEnd : itemNext + 1;
            }

            if (isCanonicalSection(section)) {
                if (section == "Service" && !wroteService) {
                    appendSection(merged, serviceSection);
                    wroteService = true;
                } else if ((section == "TouchPipeline" || IsLegacyTouchSection(section)) && !wroteTouch) {
                    appendSection(merged, touchSection);
                    wroteTouch = true;
                } else if (section == "StylusPipeline" && !wroteStylus) {
                    appendSection(merged, stylusSection);
                    wroteStylus = true;
                }
                pos = sectionEnd;
                continue;
            }
        }

        merged.append(lineView);
        if (hasNewline) merged.push_back('\n');
        pos = hasNewline ? next + 1 : lineEnd;
    }

    if (!wroteService) appendSection(merged, serviceSection);
    if (!wroteTouch) appendSection(merged, touchSection);
    if (!wroteStylus) appendSection(merged, stylusSection);

    if (!merged.empty() && merged.back() == '\n') {
        while (merged.size() > 1 && merged.back() == '\n' && merged[merged.size() - 2] == '\n') {
            merged.pop_back();
        }
    }
    if (!merged.empty() && merged.back() != '\n') merged.push_back('\n');
    return merged;
}

} // namespace App
