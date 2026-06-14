#include "TouchSolver/PalmBoxSuppressor.hpp"
#include "TouchSolver/ZoneExpander.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <span>
#include <stdexcept>
#include <vector>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct ZoneFixture {
    std::vector<Solvers::MacroZone> zones;
    std::vector<std::vector<int>> storage;

    int AddRect(int minR, int maxR, int minC, int maxC, int signal = 500) {
        storage.emplace_back();
        auto& pixels = storage.back();
        Solvers::MacroZone zone;
        zone.minR = minR;
        zone.maxR = maxR;
        zone.minC = minC;
        zone.maxC = maxC;
        for (int r = minR; r <= maxR; ++r) {
            for (int c = minC; c <= maxC; ++c) {
                pixels.push_back(r * 60 + c);
                zone.area += 1;
                zone.signalSum += signal;
            }
        }
        zone.pixels = std::span<const int>(pixels.data(), pixels.size());
        zones.push_back(zone);
        return static_cast<int>(zones.size()) - 1;
    }
};

Solvers::Touch::Peak MakePeak(int row, int col, int zoneIndex, int id = 1, int16_t signal = 900) {
    Solvers::Touch::Peak peak;
    peak.r = row;
    peak.c = col;
    peak.z = signal;
    peak.id = static_cast<uint8_t>(id);
    peak.macroZoneIndex = zoneIndex;
    return peak;
}

Solvers::Touch::MacroZoneFeature MakeFeature(int zoneIndex, Solvers::Touch::PalmClass palmClass) {
    Solvers::Touch::MacroZoneFeature feature;
    feature.zoneIndex = zoneIndex;
    feature.palmClass = palmClass;
    feature.palmScore = palmClass == Solvers::Touch::PalmClass::PalmLikely ? 0.8f : 0.0f;
    feature.fingerScore = palmClass == Solvers::Touch::PalmClass::FingerLikely ? 0.8f : 0.0f;
    return feature;
}

Solvers::Touch::PeakEvaluation MakeEval(bool allowContact = true) {
    Solvers::Touch::PeakEvaluation eval;
    eval.allowContact = allowContact;
    return eval;
}

void FillFrameForPeaks(Solvers::HeatmapFrame& frame, std::span<const Solvers::Touch::Peak> peaks) {
    std::fill(&frame.heatmapMatrix[0][0], &frame.heatmapMatrix[0][0] + 40 * 60, int16_t{0});
    for (const auto& peak : peaks) {
        frame.heatmapMatrix[peak.r][peak.c] = peak.z;
    }
}

void TestExpandedPalmBoxSuppressesTouchingZone() {
    ZoneFixture fixture;
    const int palmZone = fixture.AddRect(10, 15, 10, 15);
    const int fingerZone = fixture.AddRect(10, 12, 17, 17);

    std::vector<Solvers::Touch::MacroZoneFeature> features{
        MakeFeature(palmZone, Solvers::Touch::PalmClass::PalmLikely),
        MakeFeature(fingerZone, Solvers::Touch::PalmClass::FingerLikely)
    };
    std::vector<Solvers::Touch::Peak> peaks{
        MakePeak(12, 12, palmZone, 1),
        MakePeak(11, 17, fingerZone, 2)
    };
    std::vector<Solvers::Touch::PeakEvaluation> evals{
        MakeEval(false),
        MakeEval(true)
    };

    Solvers::Touch::PalmBoxSuppressor suppressor;
    suppressor.m_expandCols = 9;
    suppressor.Process(fixture.zones, features, peaks, evals);
    const auto adjusted = suppressor.GetEvaluations();

    Require(suppressor.GetPalmBoxes().size() == 1, "palm zone should create one palm box");
    Require(!adjusted[1].allowContact, "expanded palm box should suppress touching finger zone");
    Require((adjusted[1].evalFlags & Solvers::Touch::PalmReasonPalmBoxSuppressed) != 0,
            "suppressed peak should carry palm box reason flag");

    Solvers::HeatmapFrame frame{};
    FillFrameForPeaks(frame, peaks);
    Solvers::Touch::ZoneExpander expander;
    expander.m_dilateErode = false;
    expander.Process(frame, peaks, 100, adjusted);
    Require(frame.touch.output.contacts.empty(), "suppressed finger peak should not emit contact");
}

void TestPalmSourceZoneAllowedPeakIsSuppressed() {
    ZoneFixture fixture;
    const int palmZone = fixture.AddRect(10, 15, 10, 15);

    std::vector<Solvers::Touch::MacroZoneFeature> features{
        MakeFeature(palmZone, Solvers::Touch::PalmClass::PalmLikely)
    };
    std::vector<Solvers::Touch::Peak> peaks{
        MakePeak(12, 12, palmZone, 1),
        MakePeak(14, 14, palmZone, 2)
    };
    std::vector<Solvers::Touch::PeakEvaluation> evals{
        MakeEval(false),
        MakeEval(true)
    };
    evals[0].palmClass = Solvers::Touch::PalmClass::PalmLikely;
    evals[0].zonePalmClass = Solvers::Touch::PalmClass::PalmLikely;
    evals[0].palmEvidenceOnly = true;
    evals[1].palmClass = Solvers::Touch::PalmClass::FingerLikely;
    evals[1].zonePalmClass = Solvers::Touch::PalmClass::PalmLikely;

    Solvers::Touch::PalmBoxSuppressor suppressor;
    suppressor.m_expandRows = 9;
    suppressor.m_expandCols = 10;
    suppressor.Process(fixture.zones, features, peaks, evals);
    const auto adjusted = suppressor.GetEvaluations();

    Require(suppressor.GetPalmBoxes().size() == 1, "palm zone should create one palm box");
    Require(!adjusted[1].allowContact, "allowed peak inside palm source zone should be suppressed");
    Require((adjusted[1].evalFlags & Solvers::Touch::PalmReasonPalmBoxSuppressed) != 0,
            "same-zone suppressed peak should carry palm box reason flag");

    Solvers::HeatmapFrame frame{};
    FillFrameForPeaks(frame, peaks);
    Solvers::Touch::ZoneExpander expander;
    expander.m_dilateErode = false;
    expander.Process(frame, peaks, 100, adjusted);
    Require(frame.touch.output.contacts.empty(), "same-zone suppressed peak should not emit contact");
}

void TestDistantZoneIsNotSuppressed() {
    ZoneFixture fixture;
    const int palmZone = fixture.AddRect(10, 15, 10, 15);
    const int fingerZone = fixture.AddRect(25, 27, 25, 27);

    std::vector<Solvers::Touch::MacroZoneFeature> features{
        MakeFeature(palmZone, Solvers::Touch::PalmClass::PalmLikely),
        MakeFeature(fingerZone, Solvers::Touch::PalmClass::FingerLikely)
    };
    std::vector<Solvers::Touch::Peak> peaks{
        MakePeak(12, 12, palmZone, 1),
        MakePeak(26, 26, fingerZone, 2)
    };
    std::vector<Solvers::Touch::PeakEvaluation> evals{MakeEval(false), MakeEval(true)};

    Solvers::Touch::PalmBoxSuppressor suppressor;
    suppressor.Process(fixture.zones, features, peaks, evals);
    Require(suppressor.GetEvaluations()[1].allowContact, "distant finger zone should remain allowed");
}

void TestPalmBoxIdStaysStableAcrossSmallMotion() {
    Solvers::Touch::PalmBoxSuppressor suppressor;

    ZoneFixture frame1;
    const int zone1 = frame1.AddRect(10, 15, 10, 15);
    std::vector<Solvers::Touch::MacroZoneFeature> features1{MakeFeature(zone1, Solvers::Touch::PalmClass::PalmLikely)};
    std::vector<Solvers::Touch::Peak> peaks1{MakePeak(12, 12, zone1, 1)};
    std::vector<Solvers::Touch::PeakEvaluation> evals1{MakeEval(false)};
    suppressor.Process(frame1.zones, features1, peaks1, evals1);
    Require(suppressor.GetPalmBoxes().size() == 1, "first palm frame should create a palm box");
    const int id = suppressor.GetPalmBoxes()[0].id;

    ZoneFixture frame2;
    const int zone2 = frame2.AddRect(11, 16, 11, 16);
    std::vector<Solvers::Touch::MacroZoneFeature> features2{MakeFeature(zone2, Solvers::Touch::PalmClass::PalmLikely)};
    std::vector<Solvers::Touch::Peak> peaks2{MakePeak(13, 13, zone2, 1)};
    std::vector<Solvers::Touch::PeakEvaluation> evals2{MakeEval(false)};
    suppressor.Process(frame2.zones, features2, peaks2, evals2);

    Require(suppressor.GetPalmBoxes().size() == 1, "moved palm should still have one palm box");
    Require(suppressor.GetPalmBoxes()[0].id == id, "moved palm box should keep its ID");
}

void TestPalmBoxStaysWhilePeakDomainInside() {
    Solvers::Touch::PalmBoxSuppressor suppressor;

    ZoneFixture frame1;
    const int palmZone = frame1.AddRect(10, 15, 10, 15);
    std::vector<Solvers::Touch::MacroZoneFeature> features1{MakeFeature(palmZone, Solvers::Touch::PalmClass::PalmLikely)};
    std::vector<Solvers::Touch::Peak> peaks1{MakePeak(12, 12, palmZone, 1)};
    std::vector<Solvers::Touch::PeakEvaluation> evals1{MakeEval(false)};
    suppressor.Process(frame1.zones, features1, peaks1, evals1);
    const int id = suppressor.GetPalmBoxes()[0].id;

    ZoneFixture frame2;
    const int fingerZone = frame2.AddRect(12, 12, 12, 12);
    std::vector<Solvers::Touch::MacroZoneFeature> features2{MakeFeature(fingerZone, Solvers::Touch::PalmClass::FingerLikely)};
    std::vector<Solvers::Touch::Peak> peaks2{MakePeak(12, 12, fingerZone, 2)};
    std::vector<Solvers::Touch::PeakEvaluation> evals2{MakeEval(true)};
    suppressor.Process(frame2.zones, features2, peaks2, evals2);

    Require(suppressor.GetPalmBoxes().size() == 1, "old palm box should stay while peak-domain remains inside");
    Require(suppressor.GetPalmBoxes()[0].id == id, "kept palm box should preserve ID");
    Require(!suppressor.GetEvaluations()[0].allowContact, "inside peak-domain should be suppressed by kept palm box");
}

void TestPalmBoxDisappearsWithoutPeakDomain() {
    Solvers::Touch::PalmBoxSuppressor suppressor;

    ZoneFixture frame1;
    const int palmZone = frame1.AddRect(10, 15, 10, 15);
    std::vector<Solvers::Touch::MacroZoneFeature> features1{MakeFeature(palmZone, Solvers::Touch::PalmClass::PalmLikely)};
    std::vector<Solvers::Touch::Peak> peaks1{MakePeak(12, 12, palmZone, 1)};
    std::vector<Solvers::Touch::PeakEvaluation> evals1{MakeEval(false)};
    suppressor.Process(frame1.zones, features1, peaks1, evals1);
    Require(suppressor.GetPalmBoxes().size() == 1, "first palm frame should create a palm box");

    std::vector<Solvers::Touch::MacroZoneFeature> noFeatures;
    std::vector<Solvers::Touch::Peak> noPeaks;
    std::vector<Solvers::Touch::PeakEvaluation> noEvals;
    std::vector<Solvers::MacroZone> noZones;
    suppressor.Process(noZones, noFeatures, noPeaks, noEvals);
    Require(suppressor.GetPalmBoxes().empty(), "palm box should disappear without peak-domain inside");
}

void TestTouchingPalmObservationsMerge() {
    ZoneFixture fixture;
    const int palmA = fixture.AddRect(10, 12, 10, 12);
    const int palmB = fixture.AddRect(10, 12, 13, 15);
    std::vector<Solvers::Touch::MacroZoneFeature> features{
        MakeFeature(palmA, Solvers::Touch::PalmClass::PalmLikely),
        MakeFeature(palmB, Solvers::Touch::PalmClass::PalmLikely)
    };
    std::vector<Solvers::Touch::Peak> peaks{
        MakePeak(11, 11, palmA, 1),
        MakePeak(11, 14, palmB, 2)
    };
    std::vector<Solvers::Touch::PeakEvaluation> evals{MakeEval(false), MakeEval(false)};

    Solvers::Touch::PalmBoxSuppressor suppressor;
    suppressor.m_mergeGapCols = 1;
    suppressor.Process(fixture.zones, features, peaks, evals);
    Require(suppressor.GetPalmBoxes().size() == 1, "touching palm observations should merge into one box");
}

void TestMultiplePalmBoxesRemainIndependent() {
    ZoneFixture fixture;
    const int palmA = fixture.AddRect(5, 10, 5, 10);
    const int palmB = fixture.AddRect(25, 30, 35, 40);
    const int fingerA = fixture.AddRect(5, 6, 12, 12);
    const int fingerB = fixture.AddRect(25, 26, 42, 42);

    std::vector<Solvers::Touch::MacroZoneFeature> features{
        MakeFeature(palmA, Solvers::Touch::PalmClass::PalmLikely),
        MakeFeature(palmB, Solvers::Touch::PalmClass::PalmLikely),
        MakeFeature(fingerA, Solvers::Touch::PalmClass::FingerLikely),
        MakeFeature(fingerB, Solvers::Touch::PalmClass::FingerLikely)
    };
    std::vector<Solvers::Touch::Peak> peaks{
        MakePeak(7, 7, palmA, 1),
        MakePeak(27, 37, palmB, 2),
        MakePeak(5, 12, fingerA, 3),
        MakePeak(25, 42, fingerB, 4)
    };
    std::vector<Solvers::Touch::PeakEvaluation> evals{
        MakeEval(false), MakeEval(false), MakeEval(true), MakeEval(true)
    };

    Solvers::Touch::PalmBoxSuppressor suppressor;
    suppressor.m_expandCols = 9;
    suppressor.Process(fixture.zones, features, peaks, evals);

    Require(suppressor.GetPalmBoxes().size() == 2, "far palm observations should create two boxes");
    Require(suppressor.GetPalmBoxes()[0].id != suppressor.GetPalmBoxes()[1].id,
            "independent palm boxes should have distinct IDs");
    Require(!suppressor.GetEvaluations()[2].allowContact, "first palm box should suppress nearby finger");
    Require(!suppressor.GetEvaluations()[3].allowContact, "second palm box should suppress nearby finger");
}

} // namespace

int main() {
    try {
        TestExpandedPalmBoxSuppressesTouchingZone();
        TestPalmSourceZoneAllowedPeakIsSuppressed();
        TestDistantZoneIsNotSuppressed();
        TestPalmBoxIdStaysStableAcrossSmallMotion();
        TestPalmBoxStaysWhilePeakDomainInside();
        TestPalmBoxDisappearsWithoutPeakDomain();
        TestTouchingPalmObservationsMerge();
        TestMultiplePalmBoxesRemainIndependent();
    } catch (const std::exception& ex) {
        std::cerr << "TouchPalmBoxSuppressTest failed: " << ex.what() << '\n';
        return 1;
    }
    return 0;
}
