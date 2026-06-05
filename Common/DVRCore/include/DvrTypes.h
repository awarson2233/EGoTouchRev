#pragma once

#include "DvrFormat.h"
#include "Ipc/IpcProtocol.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Config {
class ConfigStore;
}

namespace Dvr {

struct DynamicDebugField {
    uint16_t fieldId = 0;
    Ipc::DebugValueType valueType = Ipc::DebugValueType::UInt32;
    Ipc::DebugSourceKind sourceKind = Ipc::DebugSourceKind::DerivedField;
    int16_t sourceIndex = -1;
    uint8_t uiOrder = 0;
    Ipc::DebugDvrTarget dvrTarget = Ipc::DebugDvrTarget::None;
    Ipc::DebugDvrPositionMode dvrPositionMode = Ipc::DebugDvrPositionMode::Append;
    int16_t dvrIndex = -1;
    std::string key;
    std::string displayName;
    std::string unit;
    std::string uiGroup;
    std::string dvrColumnName;
    std::string dvrAnchor;
};

struct DynamicDebugValue {
    Ipc::DebugValueType valueType = Ipc::DebugValueType::UInt32;
    bool valid = false;
    uint64_t rawValue = 0;
};

struct DynamicDebugSample {
    uint16_t fieldId = 0;
    DynamicDebugValue value{};
};

struct DynamicDebugSchema {
    std::vector<DynamicDebugField> fields;
    uint16_t schemaVersion = 0;
    uint32_t schemaHash = 0;

    bool Empty() const { return fields.empty(); }
};

struct DynamicDebugFrame {
    std::vector<DynamicDebugSample> samples;
};

struct RuntimeConfigField {
    uint32_t fieldId = 0;
    Format::Dvr2ConfigValueType valueType = Format::Dvr2ConfigValueType::String;
    uint8_t category = 0;
    float minValue = 0.0f;
    float maxValue = 0.0f;
    std::string section;
    std::string key;
    std::string displayName;
    std::string moduleTag;
    std::string unit;
};

struct RuntimeConfigValue {
    uint32_t fieldId = 0;
    Format::Dvr2ConfigValueType valueType = Format::Dvr2ConfigValueType::String;
    bool valid = false;
    uint64_t rawValue = 0;
    std::string stringValue;
};

// DVR runtime configuration is persisted as schema/value snapshot sections in DVR2
// files, not as a live mutable config owner. ConfigStore integration should rebuild
// a store from this captured schema/value pair when playback/import needs it.
struct RuntimeConfigSnapshot {
    std::vector<RuntimeConfigField> fields;
    std::vector<RuntimeConfigValue> values;
    uint32_t schemaHash = 0;

    bool Empty() const { return fields.empty() || values.empty(); }
    Config::ConfigStore toConfigStore() const;
};

} // namespace Dvr

namespace App {

using DynamicDebugField = Dvr::DynamicDebugField;
using DynamicDebugValue = Dvr::DynamicDebugValue;
using DvrDynamicDebugSample = Dvr::DynamicDebugSample;
using DvrDynamicDebugSchema = Dvr::DynamicDebugSchema;
using DvrDynamicDebugFrame = Dvr::DynamicDebugFrame;
using DvrRuntimeConfigField = Dvr::RuntimeConfigField;
using DvrRuntimeConfigValue = Dvr::RuntimeConfigValue;
using DvrRuntimeConfigSnapshot = Dvr::RuntimeConfigSnapshot;

} // namespace App
