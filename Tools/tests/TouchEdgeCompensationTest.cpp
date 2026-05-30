#include "TouchSolver/EdgeCompensation.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void RequireNear(float actual, float expected, float epsilon, const char* message) {
    if (std::fabs(actual - expected) > epsilon) {
        throw std::runtime_error(message);
    }
}

Solvers::ZoneEdgeInfo MakeEdgeInfo(uint8_t minCol, uint8_t maxCol,
                                   uint8_t minRow, uint8_t maxRow) {
    Solvers::ZoneEdgeInfo info;
    info.minCol = minCol;
    info.maxCol = maxCol;
    info.minRow = minRow;
    info.maxRow = maxRow;
    Solvers::TZ_GetEdgeTouchedFlag(info);
    return info;
}

void TestDefaultProfilesMatchRuntimeConfig() {
    Solvers::Touch::EdgeCompensator compensator;
    RequireNear(compensator.m_ecStrength, 1.0f, 0.0001f, "EC strength should match runtime config");
    RequireNear(compensator.m_ecFullCompRange, 0.5f, 0.0001f, "EC full range should match runtime config");
    RequireNear(compensator.m_ecBlendRange, 0.505f, 0.0001f, "EC blend range should match runtime config");
    for (int edge = 0; edge < 4; ++edge) {
        Require(compensator.m_profiles[edge].numSegments == 3, "EC edge should use three runtime-config segments");
        Require(compensator.m_profiles[edge].segments[0].touchSizeThreshold == 64, "EC segment 0 width should match runtime config");
        Require(compensator.m_profiles[edge].segments[0].lutIdxLow == 2, "EC segment 0 low LUT should match runtime config");
        Require(compensator.m_profiles[edge].segments[0].lutIdxHigh == 32, "EC segment 0 high LUT should match runtime config");
        Require(compensator.m_profiles[edge].segments[1].touchSizeThreshold == 128, "EC segment 1 width should match runtime config");
        Require(compensator.m_profiles[edge].segments[1].lutIdxLow == 32, "EC segment 1 low LUT should match runtime config");
        Require(compensator.m_profiles[edge].segments[1].lutIdxHigh == 96, "EC segment 1 high LUT should match runtime config");
        Require(compensator.m_profiles[edge].segments[2].touchSizeThreshold == 255, "EC segment 2 width should match runtime config");
        Require(compensator.m_profiles[edge].segments[2].lutIdxLow == 96, "EC segment 2 low LUT should match runtime config");
        Require(compensator.m_profiles[edge].segments[2].lutIdxHigh == 192, "EC segment 2 high LUT should match runtime config");
    }
}

void TestDim1NearCorrectionMetadata() {
    Solvers::Touch::EdgeCompensator compensator;
    std::vector<Solvers::TouchContact> contacts(1);
    contacts[0].x = 0.5f;
    contacts[0].y = 20.0f;
    contacts[0].state = Solvers::TouchStateDown;

    std::vector<Solvers::ZoneEdgeInfo> edgeInfos(1, MakeEdgeInfo(0, 2, 18, 22));
    edgeInfos[0].colEdgeWidth = 3;

    compensator.Process(contacts, edgeInfos, Solvers::EdgeBounds{});

    Require(contacts[0].isEdge, "near-edge contact should remain marked as edge");
    Require((contacts[0].edgeFlags & 0x20) != 0, "edge flags should include boundary touch");
    Require((contacts[0].centroidEdgeFlags & 0x01) != 0, "centroid flags should include Dim1 near edge");
    Require((contacts[0].ecFlags & 0x100) != 0, "Dim1 correction flag should be set");
    Require(contacts[0].ecWidthX == 3, "edge width should come from threshold-scanned column edge width");
    RequireNear(contacts[0].rawXBeforeEC, 0.5f, 0.0001f, "raw X should be retained");
    Require(contacts[0].edgeDistX > 0.0f, "corrected X edge distance should be populated");
    Require(std::fabs(contacts[0].x - 0.5f) > 0.0001f, "X coordinate should be corrected");
}

void TestDim2FarCorrectionMetadata() {
    Solvers::Touch::EdgeCompensator compensator;
    std::vector<Solvers::TouchContact> contacts(1);
    contacts[0].x = 30.0f;
    contacts[0].y = 39.5f;
    contacts[0].state = Solvers::TouchStateDown;

    std::vector<Solvers::ZoneEdgeInfo> edgeInfos(1, MakeEdgeInfo(28, 32, 37, 39));
    edgeInfos[0].rowEdgeWidth = 4;

    compensator.Process(contacts, edgeInfos, Solvers::EdgeBounds{});

    Require((contacts[0].centroidEdgeFlags & 0x08) != 0, "centroid flags should include Dim2 far edge");
    Require((contacts[0].ecFlags & 0x200) != 0, "Dim2 correction flag should be set");
    Require(contacts[0].ecWidthY == 4, "Y edge width should come from threshold-scanned row edge width");
    RequireNear(contacts[0].rawYBeforeEC, 39.5f, 0.0001f, "raw Y should be retained");
    Require(contacts[0].edgeDistY > 0.0f, "corrected Y edge distance should be populated");
    Require(std::fabs(contacts[0].y - 39.5f) > 0.0001f, "Y coordinate should be corrected");
}

void TestOnlyOutermostCellTriggersCorrection() {
    Solvers::Touch::EdgeCompensator compensator;
    compensator.m_ecFullCompRange = 4.0f;
    std::vector<Solvers::TouchContact> contacts(2);
    contacts[0].x = 0.5f;
    contacts[0].y = 20.0f;
    contacts[1].x = 1.5f;
    contacts[1].y = 20.0f;

    std::vector<Solvers::ZoneEdgeInfo> edgeInfos(2, MakeEdgeInfo(0, 2, 18, 22));
    compensator.Process(contacts, edgeInfos, Solvers::EdgeBounds{});

    Require((contacts[0].ecFlags & 0x100) != 0, "outermost cell should receive Dim1 correction");
    Require((contacts[1].centroidEdgeFlags & 0x01) == 0, "second cell should not receive Dim1 edge direction");
    Require((contacts[1].ecFlags & 0x100) == 0, "second cell should not receive Dim1 correction");
    RequireNear(contacts[1].x, 1.5f, 0.0001f, "second cell X should not change");
}

void TestStrengthScalesCorrectionAmplitude() {
    std::vector<Solvers::ZoneEdgeInfo> edgeInfos(1, MakeEdgeInfo(0, 2, 18, 22));

    Solvers::Touch::EdgeCompensator fullCompensator;
    fullCompensator.m_ecStrength = 1.0f;
    std::vector<Solvers::TouchContact> fullContacts(1);
    fullContacts[0].x = 0.5f;
    fullContacts[0].y = 20.0f;
    fullCompensator.Process(fullContacts, edgeInfos, Solvers::EdgeBounds{});

    Solvers::Touch::EdgeCompensator reducedCompensator;
    reducedCompensator.m_ecStrength = 0.35f;
    std::vector<Solvers::TouchContact> reducedContacts(1);
    reducedContacts[0].x = 0.5f;
    reducedContacts[0].y = 20.0f;
    reducedCompensator.Process(reducedContacts, edgeInfos, Solvers::EdgeBounds{});

    const float fullDelta = std::fabs(fullContacts[0].x - 0.5f);
    const float reducedDelta = std::fabs(reducedContacts[0].x - 0.5f);
    Require(reducedDelta > 0.0f, "reduced EC strength should still apply correction");
    Require(reducedDelta < fullDelta, "reduced EC strength should reduce correction amplitude");
}

void TestEdgeWidthScansThreshold() {
    Solvers::ZoneEdgeInfo info;
    int16_t heatmap[40][60] = {};
    for (int row = 8; row <= 12; ++row) {
        heatmap[row][0] = 320;
    }
    Solvers::TZ_UpdateEdgeInfo(info, 320, 0, 10, 7);
    Solvers::TZ_UpdateEdgeInfo(info, 320, 0, 11, 7);
    Solvers::TZ_GetEdgeWidth(info, heatmap, 300);

    Require(info.colEdgeWidth == 5, "column edge width should scan contiguous cells above threshold");

    Solvers::ZoneEdgeInfo farInfo;
    int16_t farHeatmap[40][60] = {};
    for (int row = 0; row <= 2; ++row) {
        farHeatmap[row][59] = 320;
    }
    Solvers::TZ_UpdateEdgeInfo(farInfo, 320, 59, 0, 7);
    Solvers::TZ_GetEdgeWidth(farInfo, farHeatmap, 300);
    Require(farInfo.colEdgeWidth == 3, "far column edge width should initialize max-side scan state");
}

void TestNonEdgeContactIsUnchanged() {
    Solvers::Touch::EdgeCompensator compensator;
    std::vector<Solvers::TouchContact> contacts(1);
    contacts[0].x = 20.5f;
    contacts[0].y = 11.5f;

    std::vector<Solvers::ZoneEdgeInfo> edgeInfos(1, MakeEdgeInfo(20, 22, 10, 12));
    compensator.Process(contacts, edgeInfos, Solvers::EdgeBounds{});

    Require(!contacts[0].isEdge, "non-edge contact should not be marked as edge");
    Require(contacts[0].ecFlags == 0, "non-edge contact should not receive EC flags");
    RequireNear(contacts[0].x, 20.5f, 0.0001f, "non-edge X should not change");
    RequireNear(contacts[0].y, 11.5f, 0.0001f, "non-edge Y should not change");
}

void TestEdgeRejectorDoesNotSuppressCorrectedContact() {
    Solvers::Touch::EdgeCompensator compensator;
    Solvers::Touch::EdgeRejector rejector;
    std::vector<Solvers::TouchContact> contacts(1);
    contacts[0].x = 0.5f;
    contacts[0].y = 20.0f;
    contacts[0].state = Solvers::TouchStateDown;

    std::vector<Solvers::ZoneEdgeInfo> edgeInfos(1, MakeEdgeInfo(0, 2, 18, 22));
    edgeInfos[0].colEdgeWidth = 3;

    compensator.Process(contacts, edgeInfos, Solvers::EdgeBounds{});
    rejector.Process(contacts, edgeInfos, Solvers::EdgeBounds{});

    Require(contacts[0].isReported, "corrected edge contact should remain reportable");
}

} // namespace

int main() {
    try {
        TestDefaultProfilesMatchRuntimeConfig();
        TestDim1NearCorrectionMetadata();
        TestDim2FarCorrectionMetadata();
        TestOnlyOutermostCellTriggersCorrection();
        TestStrengthScalesCorrectionAmplitude();
        TestEdgeWidthScansThreshold();
        TestNonEdgeContactIsUnchanged();
        TestEdgeRejectorDoesNotSuppressCorrectedContact();
        std::cout << "[TEST] Touch edge compensation tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
