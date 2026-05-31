#include "ServiceConfigCore.h"
#include "TempConfig.h"
#include "TestAssert.h"

#include <filesystem>
#include <string>

namespace {

bool MissingFileUsesDefaults() {
    const auto cfg = Service::ParseServiceConfig(
        (std::filesystem::temp_directory_path() / "egotouch-service-missing-config.ini").string());

    REQUIRE_EQ(static_cast<int>(cfg.mode), static_cast<int>(Service::ServiceMode::Full));
    REQUIRE_EQ(cfg.autoMode, true);
    REQUIRE_EQ(cfg.stylusVhfEnabled, true);
    REQUIRE_EQ(static_cast<int>(cfg.penButtonMode), static_cast<int>(PenButtonMode::OemCustom));
    REQUIRE_EQ(static_cast<int>(cfg.penButtonRoute), static_cast<int>(PenButtonRoute::VhfOnly));
    REQUIRE_EQ(cfg.penButtonRouteExplicit, false);
    return true;
}

bool ParsesOnlyServiceSection() {
    TempConfigFile file("service-config-parser-section");
    file.Write(R"ini(
# ignored
[TouchPipeline]
mode=touch_only
auto_mode=0

[Service]
mode=touch_only
auto_mode=no
stylus_vhf_enabled=on
pen_button_mode=1
pen_button_route=2

[Other]
auto_mode=1
)ini");

    const auto cfg = Service::ParseServiceConfig(file.Path());
    REQUIRE_EQ(static_cast<int>(cfg.mode), static_cast<int>(Service::ServiceMode::TouchOnly));
    REQUIRE_EQ(cfg.autoMode, false);
    REQUIRE_EQ(cfg.stylusVhfEnabled, true);
    REQUIRE_EQ(static_cast<int>(cfg.penButtonMode), 1);
    REQUIRE_EQ(static_cast<int>(cfg.penButtonRoute), 2);
    REQUIRE_EQ(cfg.penButtonRouteExplicit, true);
    return true;
}

bool BoolVariantsAndInvalidModeFallback() {
    TempConfigFile enabled("service-config-parser-bool-enabled");
    enabled.Write(R"ini(
[Service]
mode=unknown
auto_mode=yes
stylus_vhf_enabled=true
)ini");
    auto cfg = Service::ParseServiceConfig(enabled.Path());
    REQUIRE_EQ(static_cast<int>(cfg.mode), static_cast<int>(Service::ServiceMode::Full));
    REQUIRE_EQ(cfg.autoMode, true);
    REQUIRE_EQ(cfg.stylusVhfEnabled, true);

    TempConfigFile disabled("service-config-parser-bool-disabled");
    disabled.Write(R"ini(
[Service]
auto_mode=off
stylus_vhf_enabled=0
)ini");
    cfg = Service::ParseServiceConfig(disabled.Path());
    REQUIRE_EQ(cfg.autoMode, false);
    REQUIRE_EQ(cfg.stylusVhfEnabled, false);
    REQUIRE_EQ(cfg.penButtonRouteExplicit, false);
    return true;
}

bool PenFieldsClampToWireRange() {
    TempConfigFile low("service-config-parser-clamp-low");
    low.Write(R"ini(
[Service]
pen_button_mode=-1
pen_button_route=-99
)ini");
    auto cfg = Service::ParseServiceConfig(low.Path());
    REQUIRE_EQ(static_cast<int>(cfg.penButtonMode), 0);
    REQUIRE_EQ(static_cast<int>(cfg.penButtonRoute), 0);
    REQUIRE_EQ(cfg.penButtonRouteExplicit, true);

    TempConfigFile high("service-config-parser-clamp-high");
    high.Write(R"ini(
[Service]
pen_button_mode=99
pen_button_route=7
)ini");
    cfg = Service::ParseServiceConfig(high.Path());
    REQUIRE_EQ(static_cast<int>(cfg.penButtonMode), 2);
    REQUIRE_EQ(static_cast<int>(cfg.penButtonRoute), 2);
    REQUIRE_EQ(cfg.penButtonRouteExplicit, true);
    return true;
}

} // namespace

int main() {
    int failures = 0;
    failures += RunTest(&MissingFileUsesDefaults, "MissingFileUsesDefaults");
    failures += RunTest(&ParsesOnlyServiceSection, "ParsesOnlyServiceSection");
    failures += RunTest(&BoolVariantsAndInvalidModeFallback, "BoolVariantsAndInvalidModeFallback");
    failures += RunTest(&PenFieldsClampToWireRange, "PenFieldsClampToWireRange");
    return failures == 0 ? 0 : 1;
}
