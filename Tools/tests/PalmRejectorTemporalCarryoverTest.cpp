#include "TouchSolver/PalmRejector.hpp"
#include "TouchSolver/TouchPipeline.h"

#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

using Solvers::HeatmapFrame;
using Solvers::MacroZone;
using Solvers::Touch::PalmRejector;
using Solvers::TouchPipeline;

constexpr int kCols = 60;

int Cell(int r, int c) {
    return r * kCols + c;
}

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct ZoneSet {
    std::vector<std::vector<int>> storage;
    std::vector<MacroZone> zones;
};

ZoneSet MakeZones(std::initializer_list<std::initializer_list<int>> pixelGroups) {
    ZoneSet set;
    set.storage.reserve(pixelGroups.size());
    set.zones.reserve(pixelGroups.size());
    for (const auto& group : pixelGroups) {
        set.storage.emplace_back(group);
        const auto& pixels = set.storage.back();

        MacroZone zone;
        zone.pixels = std::span<const int>(pixels.data(), pixels.size());
        zone.area = static_cast<int>(pixels.size());
        zone.signalSum = static_cast<int>(pixels.size()) * 1000;
        zone.minR = 39;
        zone.maxR = 0;
        zone.minC = 59;
        zone.maxC = 0;
        for (int idx : pixels) {
            const int r = idx / kCols;
            const int c = idx % kCols;
            if (r < zone.minR) zone.minR = r;
            if (r > zone.maxR) zone.maxR = r;
            if (c < zone.minC) zone.minC = c;
            if (c > zone.maxC) zone.maxC = c;
        }
        set.zones.push_back(zone);
    }
    return set;
}

void ConfigureForAreaOnly(PalmRejector& rejector) {
    rejector.ResetTemporalState();
    rejector.m_areaThreshold = 4;
    rejector.m_signalSumThreshold = 1000000;
    rejector.m_areaMinForDensity = 1000;
    rejector.m_densityThresholdLow = 0.0f;
    rejector.m_elongatedEnabled = false;
}

void SeedDirectPalm(PalmRejector& rejector) {
    HeatmapFrame frame;
    auto seed = MakeZones({
        {Cell(10, 10), Cell(10, 11), Cell(11, 10), Cell(11, 11)}
    });
    rejector.Process(seed.zones, frame);
    Require(seed.zones.empty(), "direct palm seed should be rejected");
}

void TestTouchingFragmentInheritsPalm() {
    PalmRejector rejector;
    ConfigureForAreaOnly(rejector);
    SeedDirectPalm(rejector);

    HeatmapFrame frame;
    auto current = MakeZones({
        {Cell(12, 12)}
    });
    rejector.Process(current.zones, frame);
    Require(current.zones.empty(), "touching fragment should inherit palm");
}

void TestRectTouchIgnoresPixelShape() {
    PalmRejector rejector;
    ConfigureForAreaOnly(rejector);

    HeatmapFrame frame;
    auto seed = MakeZones({
        {Cell(10, 10), Cell(12, 12), Cell(12, 10), Cell(10, 12)}
    });
    rejector.Process(seed.zones, frame);
    Require(seed.zones.empty(), "seed should create a palm rect from bbox");

    auto current = MakeZones({
        {Cell(13, 11)}
    });
    rejector.Process(current.zones, frame);
    Require(current.zones.empty(), "rect carryover should match by bbox contact, not exact pixels");
}

void TestNonTouchingFragmentStays() {
    PalmRejector rejector;
    ConfigureForAreaOnly(rejector);
    SeedDirectPalm(rejector);

    HeatmapFrame frame;
    auto current = MakeZones({
        {Cell(13, 13)}
    });
    rejector.Process(current.zones, frame);
    Require(current.zones.size() == 1, "non-touching fragment should stay");
}

void TestPartialTouchingZonesOnlyRemoveMatches() {
    PalmRejector rejector;
    ConfigureForAreaOnly(rejector);
    SeedDirectPalm(rejector);

    HeatmapFrame frame;
    auto current = MakeZones({
        {Cell(12, 12)},
        {Cell(20, 20)}
    });
    rejector.Process(current.zones, frame);
    Require(current.zones.size() == 1, "only non-touching zone should remain");
    Require(current.zones[0].minR == 20 && current.zones[0].minC == 20,
            "remaining zone should be the non-touching fragment");
}

void TestNoPreviousPalmDoesNotInherit() {
    PalmRejector rejector;
    ConfigureForAreaOnly(rejector);

    HeatmapFrame frame;
    auto current = MakeZones({
        {Cell(12, 12)}
    });
    rejector.Process(current.zones, frame);
    Require(current.zones.size() == 1, "inheritance should not trigger without previous palm");
}

void TestPipelineIdleResetClearsTemporalPalmState() {
    TouchPipeline pipeline;
    ConfigureForAreaOnly(pipeline.m_palmReject);
    SeedDirectPalm(pipeline.m_palmReject);

    HeatmapFrame idleFrame;
    const bool ok = pipeline.Process(idleFrame);
    Require(ok, "idle process should succeed");

    HeatmapFrame frame;
    auto current = MakeZones({
        {Cell(12, 12)}
    });
    pipeline.m_palmReject.Process(current.zones, frame);
    Require(current.zones.size() == 1, "idle reset should clear previous palm mask");
}

} // namespace

int main() {
    try {
        TestTouchingFragmentInheritsPalm();
        TestRectTouchIgnoresPixelShape();
        TestNonTouchingFragmentStays();
        TestPartialTouchingZonesOnlyRemoveMatches();
        TestNoPreviousPalmDoesNotInherit();
        TestPipelineIdleResetClearsTemporalPalmState();
        std::cout << "[TEST] PalmRejector temporal carryover tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
