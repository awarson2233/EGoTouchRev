#pragma once

#include "config/ConfigSchemaSnapshot.h"

#include <string>
#include <vector>

namespace Config {
class ConfigStore;
}

namespace App {

class ConfigUIRenderer {
public:
    static void RenderConfigStore(
        const Config::ConfigSchemaSnapshot& schema,
        Config::ConfigStore& values,
        const std::string& sectionName);

    static void RenderConfigStoreByModule(
        const Config::ConfigSchemaSnapshot& schema,
        Config::ConfigStore& values,
        const std::string& moduleTag);

    static std::vector<std::string> CollectModuleTags(
        const Config::ConfigSchemaSnapshot& schema);
};

} // namespace App
