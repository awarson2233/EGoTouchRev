#pragma once

#include "PenButtonConfig.h"

#include <cstdint>

namespace Config {
class ConfigStore;
}

namespace Service {

/// Service runtime topology selected by [Service].mode.
enum class ServiceMode {
    Full,
    TouchOnly,
};

struct ServiceConfigState {
    ServiceMode mode = ServiceMode::Full;
    bool autoMode = true;
    bool stylusVhfEnabled = true;
    PenButtonMode penButtonMode = PenButtonMode::OemCustom;
    PenButtonRoute penButtonRoute = PenButtonRoute::VhfOnly;
    bool penButtonRouteExplicit = false;
};

struct ReloadServiceConfigResult {
    uint8_t changedFields = 0;
    uint8_t appliedFields = 0;
    uint8_t restartRequiredFields = 0;
};

enum class ServiceConfigField : uint8_t {
    Mode = 0,
    AutoMode = 1,
    StylusVhfEnabled = 2,
    PenButtonMode = 3,
    PenButtonRoute = 4,
};

constexpr uint8_t ToServiceConfigFieldBit(ServiceConfigField field) {
    return static_cast<uint8_t>(1u << static_cast<uint8_t>(field));
}

const char* ServiceModeToConfig(ServiceMode mode);
void ApplyConfig(ServiceConfigState& state, const Config::ConfigStore& store);
ReloadServiceConfigResult DiffServiceConfig(const ServiceConfigState& current,
                                            const ServiceConfigState& reloaded,
                                            bool runtimeAvailable);

} // namespace Service
