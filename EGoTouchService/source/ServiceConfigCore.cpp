#include "ServiceConfigCore.h"

#include "config/ConfigStore.h"

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <variant>

namespace Service {

namespace {

std::string Normalize(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    for (char& ch : value) {
        if (ch == ' ' || ch == '+' || ch == '-') {
            ch = '_';
        }
    }
    while (value.find("__") != std::string::npos) {
        value.replace(value.find("__"), 2, "_");
    }
    return value;
}

PenButtonMode ParsePenButtonMode(const Config::ConfigStore& store, PenButtonMode fallback) {
    if (store.has("service.pen_button_mode")) {
        const auto value = store.get<Config::ConfigValue>("service.pen_button_mode");
        if (const auto parsed = ParsePenButtonModeValue(value)) {
            return *parsed;
        }
    }
    return fallback;
}

PenButtonRoute ParsePenButtonRoute(const Config::ConfigStore& store,
                                   PenButtonRoute fallback,
                                   bool& explicitRoute) {
    if (store.has("service.pen_button_route")) {
        explicitRoute = true;
        const auto value = store.get<Config::ConfigValue>("service.pen_button_route");
        if (const auto parsed = ParsePenButtonRouteValue(value)) {
            return *parsed;
        }
        explicitRoute = false;
    }
    return fallback;
}

} // namespace

const char* ServiceModeToConfig(ServiceMode mode) {
    return mode == ServiceMode::Full ? "full" : "touch_only";
}

std::optional<PenButtonMode> ParsePenButtonModeValue(const Config::ConfigValue& value) {
    if (const auto* text = std::get_if<std::string>(&value)) {
        const auto normalized = Normalize(*text);
        if (normalized == "oem_custom") return PenButtonMode::OemCustom;
        if (normalized == "native_barrel") return PenButtonMode::NativeBarrel;
        if (normalized == "native_eraser") return PenButtonMode::NativeEraser;
        return std::nullopt;
    }
    if (const auto* numeric = std::get_if<int32_t>(&value)) {
        if (*numeric >= 0 && *numeric <= 2) {
            return static_cast<PenButtonMode>(*numeric);
        }
    }
    return std::nullopt;
}

std::optional<PenButtonRoute> ParsePenButtonRouteValue(const Config::ConfigValue& value) {
    if (const auto* text = std::get_if<std::string>(&value)) {
        const auto normalized = Normalize(*text);
        if (normalized == "vhf_only") return PenButtonRoute::VhfOnly;
        if (normalized == "win32_only") return PenButtonRoute::Win32Only;
        if (normalized == "vhf_and_win32" || normalized == "vhf_win32") return PenButtonRoute::VhfAndWin32;
        return std::nullopt;
    }
    if (const auto* numeric = std::get_if<int32_t>(&value)) {
        if (*numeric >= 0 && *numeric <= 2) {
            return static_cast<PenButtonRoute>(*numeric);
        }
    }
    return std::nullopt;
}

void ApplyLegacyServiceModeMigration(Config::ConfigStore& target, const Config::ConfigStore& source) {
    if (source.has("service.mode") || !source.has("service.mode.full")) {
        return;
    }

    target.set<std::string>(
        "service.mode",
        source.getOr<bool>("service.mode.full", true) ? "full" : "touch_only");
}

void ApplyConfig(ServiceConfigState& state, const Config::ConfigStore& store) {
    if (store.has("service.mode")) {
        const auto mode = Normalize(store.getOr<std::string>("service.mode", ServiceModeToConfig(state.mode)));
        if (mode == "full") {
            state.mode = ServiceMode::Full;
        } else if (mode == "touch_only") {
            state.mode = ServiceMode::TouchOnly;
        }
    } else if (store.has("service.mode.full")) {
        state.mode = store.getOr<bool>("service.mode.full", state.mode == ServiceMode::Full)
            ? ServiceMode::Full
            : ServiceMode::TouchOnly;
    }

    state.autoMode = store.getOr<bool>("service.auto_mode", state.autoMode);
    state.stylusVhfEnabled = store.getOr<bool>("service.stylus_vhf_enabled", state.stylusVhfEnabled);
    state.penButtonMode = ParsePenButtonMode(store, state.penButtonMode);
    state.penButtonRoute = ParsePenButtonRoute(store, state.penButtonRoute, state.penButtonRouteExplicit);
}

ReloadServiceConfigResult DiffServiceConfig(const ServiceConfigState& current,
                                            const ServiceConfigState& reloaded,
                                            bool runtimeAvailable) {
    ReloadServiceConfigResult result{};

    const bool modeChanged = (current.mode != reloaded.mode);
    const bool autoModeChanged = (current.autoMode != reloaded.autoMode);
    const bool stylusVhfChanged = (current.stylusVhfEnabled != reloaded.stylusVhfEnabled);
    const bool penButtonModeChanged = (current.penButtonMode != reloaded.penButtonMode);
    const bool penButtonRouteChanged =
        (current.penButtonRoute != reloaded.penButtonRoute) ||
        (current.penButtonRouteExplicit != reloaded.penButtonRouteExplicit);

    if (modeChanged) {
        result.changedFields |= ToServiceConfigFieldBit(ServiceConfigField::Mode);
        result.restartRequiredFields |= ToServiceConfigFieldBit(ServiceConfigField::Mode);
    }
    if (autoModeChanged) {
        result.changedFields |= ToServiceConfigFieldBit(ServiceConfigField::AutoMode);
    }
    if (stylusVhfChanged) {
        result.changedFields |= ToServiceConfigFieldBit(ServiceConfigField::StylusVhfEnabled);
    }
    if (penButtonModeChanged) {
        result.changedFields |= ToServiceConfigFieldBit(ServiceConfigField::PenButtonMode);
    }
    if (penButtonRouteChanged) {
        result.changedFields |= ToServiceConfigFieldBit(ServiceConfigField::PenButtonRoute);
    }

    if (runtimeAvailable) {
        result.appliedFields |= static_cast<uint8_t>(
            (autoModeChanged ? ToServiceConfigFieldBit(ServiceConfigField::AutoMode) : 0u) |
            (stylusVhfChanged ? ToServiceConfigFieldBit(ServiceConfigField::StylusVhfEnabled) : 0u) |
            (penButtonModeChanged ? ToServiceConfigFieldBit(ServiceConfigField::PenButtonMode) : 0u) |
            (penButtonRouteChanged ? ToServiceConfigFieldBit(ServiceConfigField::PenButtonRoute) : 0u));
    }

    return result;
}

} // namespace Service
