#include "TouchSolver/PeakDetector.hpp"
#include "TouchSolver/ZoneExpander.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

Solvers::MacroZone MakeZoneFromFrame(const Solvers::HeatmapFrame& frame,
                                     std::vector<int>& pixels,
                                     int threshold) {
    Solvers::MacroZone zone;
    zone.minR = 39;
    zone.maxR = 0;
    zone.minC = 59;
    zone.maxC = 0;
    for (int r = 0; r < 40; ++r) {
        for (int c = 0; c < 60; ++c) {
            const int16_t sig = frame.heatmapMatrix[r][c];
            if (sig < threshold) continue;
            pixels.push_back(r * 60 + c);
            zone.area += 1;
            zone.signalSum += sig;
            zone.minR = std::min(zone.minR, r);
            zone.maxR = std::max(zone.maxR, r);
            zone.minC = std::min(zone.minC, c);
            zone.maxC = std::max(zone.maxC, c);
        }
    }
    zone.pixels = std::span<const int>(pixels.data(), pixels.size());
    return zone;
}

bool HasPeakAt(std::span<const Solvers::Touch::Peak> peaks, int row, int col) {
    for (const auto& peak : peaks) {
        if (peak.r == row && peak.c == col) return true;
    }
    return false;
}

const Solvers::TouchContact* FindContactById(const std::vector<Solvers::TouchContact>& contacts, int id) {
    for (const auto& contact : contacts) {
        if (contact.id == id) return &contact;
    }
    return nullptr;
}

void BuildCloseTwoPeakFrame(Solvers::HeatmapFrame& frame) {
    frame.heatmapMatrix[20][20] = 900;
    frame.heatmapMatrix[19][20] = 420;
    frame.heatmapMatrix[21][20] = 410;
    frame.heatmapMatrix[20][19] = 430;
    frame.heatmapMatrix[19][19] = 260;
    frame.heatmapMatrix[21][19] = 250;
    frame.heatmapMatrix[19][21] = 300;
    frame.heatmapMatrix[21][21] = 290;

    frame.heatmapMatrix[20][21] = 350;

    frame.heatmapMatrix[20][22] = 820;
    frame.heatmapMatrix[19][22] = 390;
    frame.heatmapMatrix[21][22] = 380;
    frame.heatmapMatrix[20][23] = 360;
    frame.heatmapMatrix[19][23] = 240;
    frame.heatmapMatrix[21][23] = 230;
}

void BuildMergedSplitFrame(Solvers::HeatmapFrame& frame) {
    frame.heatmapMatrix[20][20] = 900;
    frame.heatmapMatrix[19][20] = 500;
    frame.heatmapMatrix[21][20] = 480;
    frame.heatmapMatrix[20][19] = 520;
    frame.heatmapMatrix[19][19] = 320;
    frame.heatmapMatrix[21][19] = 300;
    frame.heatmapMatrix[18][20] = 260;
    frame.heatmapMatrix[22][20] = 250;
    frame.heatmapMatrix[20][18] = 240;

    frame.heatmapMatrix[20][21] = 220;

    frame.heatmapMatrix[20][22] = 820;
    frame.heatmapMatrix[19][22] = 440;
    frame.heatmapMatrix[21][22] = 430;
    frame.heatmapMatrix[20][23] = 450;
    frame.heatmapMatrix[19][23] = 250;
    frame.heatmapMatrix[21][23] = 240;
}

void TestPeakDetectorPreservesTwoCloseSaddledPeaks() {
    Solvers::HeatmapFrame frame;
    BuildCloseTwoPeakFrame(frame);

    std::vector<int> pixels;
    std::vector<Solvers::MacroZone> zones{MakeZoneFromFrame(frame, pixels, 130)};

    Solvers::Touch::PeakDetector detector;
    detector.m_threshold = 130;
    detector.m_z8Radius = 2;
    detector.m_pressureDriftFilter = false;
    detector.m_edgePeakFilter = false;
    detector.m_macroZoneMinArea = 1;
    detector.Detect(frame, zones);

    const auto peaks = detector.GetPeaks();
    Require(peaks.size() == 2, "close saddled peaks should both survive peak detection");
    Require(HasPeakAt(peaks, 20, 20), "left close peak should survive");
    Require(HasPeakAt(peaks, 20, 22), "right close peak should survive");
}

void TestZoneExpanderPartitionsMergedTwoPeakZone() {
    Solvers::HeatmapFrame frame;
    BuildMergedSplitFrame(frame);

    Solvers::Touch::Peak rightPeak;
    rightPeak.r = 20;
    rightPeak.c = 22;
    rightPeak.z = 820;
    rightPeak.id = 22;

    Solvers::Touch::Peak leftPeak;
    leftPeak.r = 20;
    leftPeak.c = 20;
    leftPeak.z = 900;
    leftPeak.id = 20;

    std::vector<Solvers::Touch::Peak> peaks{rightPeak, leftPeak};

    Solvers::Touch::ZoneExpander expander;
    expander.m_dilateErode = false;
    expander.Process(frame, peaks, 130, {});

    Require(frame.contacts.size() == 2, "merged two-peak zone should emit two contacts");

    const auto* leftContact = FindContactById(frame.contacts, 20);
    const auto* rightContact = FindContactById(frame.contacts, 22);
    Require(leftContact != nullptr, "left peak id should be preserved in output contact");
    Require(rightContact != nullptr, "right peak id should be preserved in output contact");
    Require(std::fabs(leftContact->x - rightContact->x) >= 1.0f,
            "partitioned contacts should remain spatially separated");
    Require(leftContact->x < rightContact->x,
            "left contact should remain left of right contact");
    Require(leftContact->signalSum > rightContact->signalSum,
            "stronger/larger lobe should receive greater partition signal");
    Require(leftContact->area > rightContact->area,
            "larger lobe should receive greater partition area");
}

} // namespace

int main() {
    try {
        TestPeakDetectorPreservesTwoCloseSaddledPeaks();
        TestZoneExpanderPartitionsMergedTwoPeakZone();
        std::cout << "[TEST] Touch close split tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
