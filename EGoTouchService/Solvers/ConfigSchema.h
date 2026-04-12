#pragma once
#include <string>
#include <vector>
#include <iosfwd>

namespace Solvers {

// 配置参数元数据（无 UI 依赖）
struct ConfigParam {
    enum Type { Bool, Int, Float, Double, String };
    enum Category { General = 0, Solver, Filter, Behavior, Output, Advanced };

    std::string key;
    std::string displayName;
    Type type;
    void* valuePtr;
    float minVal = 0.0f;
    float maxVal = 0.0f;
    Category category = General;
    std::string moduleTag;   // UI sub-tab filter (e.g. "Signal Conditioning")

    ConfigParam(const std::string& k, const std::string& name, Type t, void* ptr, Category cat = General)
        : key(k), displayName(name), type(t), valuePtr(ptr), category(cat) {}

    ConfigParam(const std::string& k, const std::string& name, Type t, void* ptr, float min, float max, Category cat = General)
        : key(k), displayName(name), type(t), valuePtr(ptr), minVal(min), maxVal(max), category(cat) {}

    ConfigParam& Module(const std::string& tag) { moduleTag = tag; return *this; }
};

// 配置提供者接口（替代 DrawConfigUI）
class IConfigProvider {
public:
    virtual ~IConfigProvider() = default;
    virtual std::vector<ConfigParam> GetConfigSchema() const = 0;
    virtual void SaveConfig(std::ostream& out) const = 0;
    virtual void LoadConfig(const std::string& key, const std::string& value) = 0;
};

} // namespace Solvers
