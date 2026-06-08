#include "DvrTypes.h"

#include "config/ConfigStore.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <limits>
#include <string>

namespace Dvr {
namespace {

std::string ConfigPathForField(const RuntimeConfigField& field) {
    if (field.section.empty()) {
        return field.key;
    }
    if (field.key.empty()) {
        return field.section;
    }
    return field.section + "." + field.key;
}

} // namespace

Config::ConfigStore RuntimeConfigSnapshot::toConfigStore() const {
    Config::ConfigStore store;

    for (const auto& field : fields) {
        const auto valueIt = std::find_if(values.begin(), values.end(),
            [&](const RuntimeConfigValue& value) { return value.fieldId == field.fieldId; });
        if (valueIt == values.end() || !valueIt->valid || valueIt->valueType != field.valueType) {
            continue;
        }

        const auto path = ConfigPathForField(field);
        switch (field.valueType) {
        case Format::Dvr2ConfigValueType::Bool:
            store.set<bool>(path, valueIt->rawValue != 0);
            break;
        case Format::Dvr2ConfigValueType::UInt8:
            if (valueIt->rawValue > std::numeric_limits<uint8_t>::max()) {
                continue;
            }
            store.set<int32_t>(path, static_cast<int32_t>(valueIt->rawValue));
            break;
        case Format::Dvr2ConfigValueType::UInt16:
            if (valueIt->rawValue > std::numeric_limits<uint16_t>::max()) {
                continue;
            }
            store.set<int32_t>(path, static_cast<int32_t>(valueIt->rawValue));
            break;
        case Format::Dvr2ConfigValueType::UInt32:
            if (valueIt->rawValue > static_cast<uint64_t>(std::numeric_limits<int32_t>::max())) {
                continue;
            }
            store.set<int32_t>(path, static_cast<int32_t>(valueIt->rawValue));
            break;
        case Format::Dvr2ConfigValueType::Int32:
            store.set<int32_t>(path, std::bit_cast<int32_t>(static_cast<uint32_t>(valueIt->rawValue & 0xFFFFFFFFull)));
            break;
        case Format::Dvr2ConfigValueType::Float32:
            store.set<float>(path, std::bit_cast<float>(static_cast<uint32_t>(valueIt->rawValue)));
            break;
        case Format::Dvr2ConfigValueType::Float64:
            store.set<float>(path, static_cast<float>(std::bit_cast<double>(valueIt->rawValue)));
            break;
        case Format::Dvr2ConfigValueType::String:
        default:
            store.set<std::string>(path, valueIt->stringValue);
            break;
        }
    }

    return store;
}

} // namespace Dvr
