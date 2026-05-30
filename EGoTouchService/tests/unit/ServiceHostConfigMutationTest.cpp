#include "ServiceConfigCore.h"
#include "TestAssert.h"

namespace {

constexpr uint8_t Bit(Service::ServiceConfigField field) {
    return Service::ToServiceConfigFieldBit(field);
}

bool NoChangesProduceNoFieldMasks() {
    Service::ServiceConfigState current{};
    const auto result = Service::DiffServiceConfig(current, current, true);
    REQUIRE_EQ(static_cast<int>(result.changedFields), 0);
    REQUIRE_EQ(static_cast<int>(result.appliedFields), 0);
    REQUIRE_EQ(static_cast<int>(result.restartRequiredFields), 0);
    return true;
}

bool ModeChangeRequiresRestartButDoesNotApply() {
    Service::ServiceConfigState current{};
    Service::ServiceConfigState reloaded = current;
    reloaded.mode = Service::ServiceMode::TouchOnly;

    const auto result = Service::DiffServiceConfig(current, reloaded, true);
    REQUIRE_EQ(static_cast<int>(result.changedFields), static_cast<int>(Bit(Service::ServiceConfigField::Mode)));
    REQUIRE_EQ(static_cast<int>(result.appliedFields), 0);
    REQUIRE_EQ(static_cast<int>(result.restartRequiredFields), static_cast<int>(Bit(Service::ServiceConfigField::Mode)));
    return true;
}

bool PolicyChangesApplyWhenRuntimeAvailable() {
    Service::ServiceConfigState current{};
    Service::ServiceConfigState reloaded = current;
    reloaded.autoMode = false;
    reloaded.stylusVhfEnabled = false;
    reloaded.penButtonMode = PenButtonMode::NativeBarrel;
    reloaded.penButtonRoute = PenButtonRoute::Win32Only;

    const uint8_t expected = static_cast<uint8_t>(
        Bit(Service::ServiceConfigField::AutoMode) |
        Bit(Service::ServiceConfigField::StylusVhfEnabled) |
        Bit(Service::ServiceConfigField::PenButtonMode) |
        Bit(Service::ServiceConfigField::PenButtonRoute));

    const auto result = Service::DiffServiceConfig(current, reloaded, true);
    REQUIRE_EQ(static_cast<int>(result.changedFields), static_cast<int>(expected));
    REQUIRE_EQ(static_cast<int>(result.appliedFields), static_cast<int>(expected));
    REQUIRE_EQ(static_cast<int>(result.restartRequiredFields), 0);
    return true;
}

bool PolicyChangesDoNotApplyWithoutRuntime() {
    Service::ServiceConfigState current{};
    Service::ServiceConfigState reloaded = current;
    reloaded.autoMode = false;
    reloaded.penButtonRoute = PenButtonRoute::Win32Only;

    const uint8_t expectedChanged = static_cast<uint8_t>(
        Bit(Service::ServiceConfigField::AutoMode) |
        Bit(Service::ServiceConfigField::PenButtonRoute));

    const auto result = Service::DiffServiceConfig(current, reloaded, false);
    REQUIRE_EQ(static_cast<int>(result.changedFields), static_cast<int>(expectedChanged));
    REQUIRE_EQ(static_cast<int>(result.appliedFields), 0);
    REQUIRE_EQ(static_cast<int>(result.restartRequiredFields), 0);
    return true;
}

} // namespace

int main() {
    int failures = 0;
    failures += RunTest(&NoChangesProduceNoFieldMasks, "NoChangesProduceNoFieldMasks");
    failures += RunTest(&ModeChangeRequiresRestartButDoesNotApply, "ModeChangeRequiresRestartButDoesNotApply");
    failures += RunTest(&PolicyChangesApplyWhenRuntimeAvailable, "PolicyChangesApplyWhenRuntimeAvailable");
    failures += RunTest(&PolicyChangesDoNotApplyWithoutRuntime, "PolicyChangesDoNotApplyWithoutRuntime");
    return failures == 0 ? 0 : 1;
}
