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

void TestDim1NearCorrectionMetadata() {
    Solvers::Touch::EdgeCompensator compensator;
    std::vector<Solvers::TouchContact> contacts(1);
    contacts[0].x = 0.5f;
    contacts[0].y = 20.0f;
    contacts[0].state = Solvers::TouchStateDown;

    std::vector<Solvers::ZoneEdgeInfo> edgeInfos(1, MakeEdgeInfo(0, 2, 18, 22));
    edgeInfos[0].outerColSigSum = 300;
    edgeInfos[0].innerColSigSum = 100;
    edgeInfos[0].outerColMax = 240;
    edgeInfos[0].innerColMax = 80;

    compensator.Process(contacts, edgeInfos, Solvers::EdgeBounds{});

    Require(contacts[0].isEdge, "near-edge contact should remain marked as edge");
    Require((contacts[0].edgeFlags & 0x20) != 0, "edge flags should include boundary touch");
    Require((contacts[0].centroidEdgeFlags & 0x01) != 0, "centroid flags should include Dim1 near edge");
    Require((contacts[0].ecFlags & 0x100) != 0, "Dim1 correction flag should be set");
    Require(contacts[0].ecWidthX == 255, "edge width should clamp to 255 when outer sum dominates");
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
    edgeInfos[0].outerRowSigSum = 300;
    edgeInfos[0].innerRowSigSum = 100;
    edgeInfos[0].outerRowMax = 240;
    edgeInfos[0].innerRowMax = 80;

    compensator.Process(contacts, edgeInfos, Solvers::EdgeBounds{});

    Require((contacts[0].centroidEdgeFlags & 0x08) != 0, "centroid flags should include Dim2 far edge");
    Require((contacts[0].ecFlags & 0x200) != 0, "Dim2 correction flag should be set");
    Require(contacts[0].ecWidthY == 255, "Y edge width should clamp to 255");
    RequireNear(contacts[0].rawYBeforeEC, 39.5f, 0.0001f, "raw Y should be retained");
    Require(contacts[0].edgeDistY > 0.0f, "corrected Y edge distance should be populated");
    Require(std::fabs(contacts[0].y - 39.5f) > 0.0001f, "Y coordinate should be corrected");
}

void TestInnerZeroFallbackIsStrongEdge() {
    Solvers::Touch::EdgeCompensator compensator;
    std::vector<Solvers::TouchContact> contacts(1);
    contacts[0].x = 0.5f;
    contacts[0].y = 20.0f;

    std::vector<Solvers::ZoneEdgeInfo> edgeInfos(1, MakeEdgeInfo(0, 2, 18, 22));
    edgeInfos[0].outerColSigSum = 120;
    edgeInfos[0].innerColSigSum = 0;
    edgeInfos[0].outerColMax = 120;
    edgeInfos[0].innerColMax = 0;

    compensator.Process(contacts, edgeInfos, Solvers::EdgeBounds{});

    Require(contacts[0].ecWidthX == 255, "outer-only edge signal should be treated as strong edge width");
    Require((contacts[0].ecFlags & 0x100) != 0, "outer-only edge signal should still correct Dim1");
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
    edgeInfos[0].outerColSigSum = 300;
    edgeInfos[0].innerColSigSum = 100;

    compensator.Process(contacts, edgeInfos, Solvers::EdgeBounds{});
    rejector.Process(contacts, edgeInfos, Solvers::EdgeBounds{});

    Require(contacts[0].isReported, "corrected edge contact should remain reportable");
}

} // namespace

int main() {
    try {
        TestDim1NearCorrectionMetadata();
        TestDim2FarCorrectionMetadata();
        TestInnerZeroFallbackIsStrongEdge();
        TestNonEdgeContactIsUnchanged();
        TestEdgeRejectorDoesNotSuppressCorrectedContact();
        std::cout << "[TEST] Touch edge compensation tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
