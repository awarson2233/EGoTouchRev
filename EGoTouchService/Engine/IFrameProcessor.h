#pragma once
#include "EngineTypes.h"
#include "ConfigSchema.h"
#include <string>
#include <iostream>
#include <map>
#include <vector>

namespace Engine {

/**
 * @brief 算法模块核心接口 (Algorithm Module Interface)
 * 
 * IFrameProcessor 定义了 EGoTouch 数据处理管线中单个功能单元的标准接口。
 * 每一个具体的算法步骤（如：底噪扣除、高斯滤波、质心提取等）都必须继承自该接口并实现 Process 方法。
 * 
 * 模块设计原则：
 * 1. 顺序无关性：在合理排序后，各模块应独立完成其功能。
 * 2. 帧驱动：每一帧数据流入 Pipeline 后由各 Processor 链式处理。
 * 3. 可控性：支持动态使能/禁用，并可通过 UI 导出特定参数供调试。
 */
class IFrameProcessor {
public:
    virtual ~IFrameProcessor() = default;

    // Process frame. Return false to Drop the frame
    virtual bool Process(HeatmapFrame& frame) = 0;

    // Get module name
    virtual std::string GetName() const = 0;

    // Get/Set enable status
    virtual bool IsEnabled() const { return m_enabled; }
    virtual void SetEnabled(bool enabled) { m_enabled = enabled; }

    // Configuration schema (replaces DrawConfigUI)
    virtual std::vector<ConfigParam> GetConfigSchema() const {
        return {ConfigParam("Enabled", "Enable Processor", ConfigParam::Bool,
                           const_cast<bool*>(&m_enabled))};
    }

    // Configuration Serialization
    virtual void SaveConfig(std::ostream& out) const {
        out << "Enabled=" << m_enabled << "\n";
    }

    virtual void LoadConfig(const std::string& key, const std::string& value) {
        if (key == "Enabled") m_enabled = (value == "1" || value == "true");
    }

protected:
    bool m_enabled = true;
};

} // namespace Engine
