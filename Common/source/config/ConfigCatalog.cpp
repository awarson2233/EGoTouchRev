#include "config/ConfigCatalog.h"

#include "config/ConfigBinder.h"
#include "config/ConfigKeyMap.h"
#include "config/ConfigStore.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace Config {
namespace {

bool hasEffectiveKeyId(ConfigKeyId keyId)
{
    return keyId != ConfigKeyId::MaxKeyId;
}

bool boundToRuntime(ConfigRuntimeBinding runtimeBinding)
{
    return runtimeBinding == ConfigRuntimeBinding::LiveSetter ||
           runtimeBinding == ConfigRuntimeBinding::ManualLiveApply;
}

ConfigUiType uiTypeFromTypeName(std::string_view typeName, const ConfigValue& defaultValue)
{
    if (typeName == "bool") return ConfigUiType::Bool;
    if (typeName == "int") return ConfigUiType::Int32;
    if (typeName == "float") return ConfigUiType::Float;
    if (typeName == "enum") return ConfigUiType::Enum;
    if (typeName == "string") return ConfigUiType::String;
    return deriveUiType(defaultValue);
}

void sortDescriptors(std::vector<ConfigDescriptor>& descriptors)
{
    std::ranges::sort(descriptors, [](const ConfigDescriptor& lhs, const ConfigDescriptor& rhs) {
        if (lhs.keyId != rhs.keyId) {
            return static_cast<uint16_t>(lhs.keyId) < static_cast<uint16_t>(rhs.keyId);
        }
        return lhs.path < rhs.path;
    });
}

} // namespace

ConfigCatalog::ConfigCatalog(std::vector<ConfigDescriptor> descriptors)
    : m_descriptors(std::move(descriptors))
{
    sortDescriptors(m_descriptors);
    m_indexByPath.reserve(m_descriptors.size());
    m_indexByKeyId.reserve(m_descriptors.size());

    for (size_t index = 0; index < m_descriptors.size(); ++index) {
        const auto& descriptor = m_descriptors[index];
        if (!m_indexByPath.emplace(descriptor.path, index).second) {
            throw std::invalid_argument("ConfigCatalog: duplicate path: " + descriptor.path);
        }
        if (hasEffectiveKeyId(descriptor.keyId) &&
            !m_indexByKeyId.emplace(descriptor.keyId, index).second) {
            throw std::invalid_argument("ConfigCatalog: duplicate keyId");
        }
    }
}

std::optional<std::reference_wrapper<const ConfigDescriptor>> ConfigCatalog::findByPath(std::string_view path) const
{
    const auto it = m_indexByPath.find(std::string{path});
    if (it == m_indexByPath.end()) {
        return std::nullopt;
    }
    return std::cref(m_descriptors[it->second]);
}

std::optional<std::reference_wrapper<const ConfigDescriptor>> ConfigCatalog::findByKeyId(ConfigKeyId keyId) const
{
    const auto it = m_indexByKeyId.find(keyId);
    if (it == m_indexByKeyId.end()) {
        return std::nullopt;
    }
    return std::cref(m_descriptors[it->second]);
}

ConfigSchemaSnapshot ConfigCatalog::toSchemaSnapshot() const
{
    ConfigSchemaSnapshot snapshot;
    snapshot.entries.reserve(m_descriptors.size());
    for (const auto& descriptor : m_descriptors) {
        snapshot.entries.push_back(schemaEntryFromDescriptor(descriptor));
    }
    return snapshot;
}

ConfigCatalogBuilder& ConfigCatalogBuilder::add(ConfigDescriptor descriptor)
{
    m_descriptors.push_back(std::move(descriptor));
    return *this;
}

ConfigCatalogBuilder& ConfigCatalogBuilder::addSnapshot(const ConfigSchemaSnapshot& snapshot)
{
    for (const auto& entry : snapshot.entries) {
        add(descriptorFromSchemaEntry(entry));
    }
    return *this;
}

ConfigCatalogBuilder& ConfigCatalogBuilder::addBindings(std::span<const BindingEntry> bindings)
{
    for (const auto& binding : bindings) {
        ConfigDescriptor descriptor;
        descriptor.path = binding.yamlPath;
        descriptor.keyId = binding.keyId.value_or(ConfigKeyId::MaxKeyId);
        descriptor.defaultValue = binding.defaultValue;
        descriptor.uiType = uiTypeFromTypeName(binding.typeName, binding.defaultValue);
        descriptor.range = binding.range;
        descriptor.displayName = binding.displayName.empty() ? deriveDisplayName(binding.yamlPath) : binding.displayName;
        descriptor.description = binding.description;
        descriptor.moduleTag = binding.moduleTag.empty() ? deriveModuleTag(binding.yamlPath) : binding.moduleTag;
        descriptor.enumMapping = binding.enumMapping;
        descriptor.runtimeBinding = binding.runtimeBinding;
        descriptor.boundToRuntime = boundToRuntime(binding.runtimeBinding);
        add(std::move(descriptor));
    }
    return *this;
}

ConfigCatalogBuilder& ConfigCatalogBuilder::addDefaults(const ConfigStore& defaults)
{
    for (const auto& path : defaults.allPaths()) {
        ConfigDescriptor descriptor;
        descriptor.path = path;
        descriptor.keyId = tryKeyIdForPath(path).value_or(ConfigKeyId::MaxKeyId);
        descriptor.defaultValue = defaults.get<ConfigValue>(path);
        descriptor.uiType = deriveUiType(descriptor.defaultValue);
        descriptor.displayName = deriveDisplayName(path);
        descriptor.moduleTag = deriveModuleTag(path);
        add(std::move(descriptor));
    }
    return *this;
}

ConfigCatalog ConfigCatalogBuilder::build() &&
{
    std::unordered_map<std::string, size_t> indexByPath;
    std::vector<ConfigDescriptor> merged;
    indexByPath.reserve(m_descriptors.size());
    merged.reserve(m_descriptors.size());

    for (auto& descriptor : m_descriptors) {
        if (const auto it = indexByPath.find(descriptor.path); it != indexByPath.end()) {
            merged[it->second] = std::move(descriptor);
        } else {
            indexByPath.emplace(descriptor.path, merged.size());
            merged.push_back(std::move(descriptor));
        }
    }

    return ConfigCatalog(std::move(merged));
}

ConfigCatalog ConfigCatalogBuilder::fromSnapshot(const ConfigSchemaSnapshot& snapshot)
{
    ConfigCatalogBuilder builder;
    builder.addSnapshot(snapshot);
    return std::move(builder).build();
}

ConfigCatalog ConfigCatalogBuilder::fromBindings(std::span<const BindingEntry> bindings)
{
    ConfigCatalogBuilder builder;
    builder.addBindings(bindings);
    return std::move(builder).build();
}

ConfigCatalog ConfigCatalogBuilder::fromDefaultsAndBinder(const ConfigStore& defaults, const ConfigBinder& binder)
{
    ConfigCatalogBuilder builder;
    builder.addDefaults(defaults);
    builder.addBindings(binder.bindings());
    return std::move(builder).build();
}

ConfigDescriptor descriptorFromSchemaEntry(const ConfigSchemaEntry& entry)
{
    ConfigDescriptor descriptor;
    descriptor.keyId = entry.keyId;
    descriptor.path = entry.yamlPath;
    descriptor.defaultValue = entry.defaultValue;
    descriptor.uiType = entry.uiType;
    descriptor.range = entry.range;
    descriptor.displayName = entry.displayName;
    descriptor.description = entry.description;
    descriptor.moduleTag = entry.moduleTag;
    descriptor.enumMapping = entry.enumMapping;
    descriptor.runtimeBinding = entry.runtimeBinding;
    descriptor.boundToRuntime = entry.boundToRuntime;
    return descriptor;
}

ConfigSchemaEntry schemaEntryFromDescriptor(const ConfigDescriptor& descriptor)
{
    ConfigSchemaEntry entry;
    entry.yamlPath = descriptor.path;
    entry.keyId = descriptor.keyId;
    entry.uiType = descriptor.uiType;
    entry.defaultValue = descriptor.defaultValue;
    entry.currentValue = descriptor.defaultValue;
    entry.range = descriptor.range;
    entry.displayName = descriptor.displayName;
    entry.description = descriptor.description;
    entry.moduleTag = descriptor.moduleTag;
    entry.enumMapping = descriptor.enumMapping;
    entry.runtimeBinding = descriptor.runtimeBinding;
    entry.boundToRuntime = descriptor.boundToRuntime;
    return entry;
}

ConfigCatalog BuildConfigCatalog(const ConfigStore& defaults, const ConfigBinder& binder)
{
    return ConfigCatalogBuilder::fromDefaultsAndBinder(defaults, binder);
}

ConfigSchemaSnapshot BuildSchemaSnapshot(const ConfigCatalog& catalog)
{
    return catalog.toSchemaSnapshot();
}

} // namespace Config
