#pragma once

#include "ConfigKeyId.h"
#include "ConfigSchemaSnapshot.h"
#include "ConfigValue.h"

#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Config {

class ConfigBinder;
class ConfigStore;
struct BindingEntry;

struct ConfigDescriptor {
    ConfigKeyId keyId = ConfigKeyId::MaxKeyId;
    std::string path;
    ConfigValue defaultValue;
    ConfigUiType uiType = ConfigUiType::String;
    std::optional<ConfigRange> range;
    std::string displayName;
    std::string description;
    std::string moduleTag;
    std::vector<std::pair<int, std::string>> enumMapping;
    ConfigRuntimeBinding runtimeBinding = ConfigRuntimeBinding::SchemaOnly;
    bool boundToRuntime = false;
};

class ConfigCatalog {
public:
    ConfigCatalog() = default;
    explicit ConfigCatalog(std::vector<ConfigDescriptor> descriptors);

    const std::vector<ConfigDescriptor>& descriptors() const { return m_descriptors; }
    std::optional<std::reference_wrapper<const ConfigDescriptor>> findByPath(std::string_view path) const;
    std::optional<std::reference_wrapper<const ConfigDescriptor>> findByKeyId(ConfigKeyId keyId) const;

    ConfigSchemaSnapshot toSchemaSnapshot() const;

private:
    std::vector<ConfigDescriptor> m_descriptors;
    std::unordered_map<std::string, size_t> m_indexByPath;
    std::unordered_map<ConfigKeyId, size_t> m_indexByKeyId;
};

class ConfigCatalogBuilder {
public:
    ConfigCatalogBuilder& add(ConfigDescriptor descriptor);
    ConfigCatalogBuilder& addSnapshot(const ConfigSchemaSnapshot& snapshot);
    ConfigCatalogBuilder& addBindings(std::span<const BindingEntry> bindings);
    ConfigCatalogBuilder& addDefaults(const ConfigStore& defaults);

    ConfigCatalog build() &&;

    static ConfigCatalog fromSnapshot(const ConfigSchemaSnapshot& snapshot);
    static ConfigCatalog fromBindings(std::span<const BindingEntry> bindings);
    static ConfigCatalog fromDefaultsAndBinder(const ConfigStore& defaults, const ConfigBinder& binder);

private:
    std::vector<ConfigDescriptor> m_descriptors;
};

ConfigDescriptor descriptorFromSchemaEntry(const ConfigSchemaEntry& entry);
ConfigSchemaEntry schemaEntryFromDescriptor(const ConfigDescriptor& descriptor);
ConfigCatalog BuildConfigCatalog(const ConfigStore& defaults, const ConfigBinder& binder);
ConfigSchemaSnapshot BuildSchemaSnapshot(const ConfigCatalog& catalog);

} // namespace Config
