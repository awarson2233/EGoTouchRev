#include "TouchSolver/TouchTracker.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

using Solvers::HeatmapFrame;
using Solvers::TouchContact;
using Solvers::Touch::TouchTracker;

struct ContactSpec {
    float x = 0.0f;
    float y = 0.0f;
    int area = 0;
    int signalSum = 0;
    float sizeMm = 0.0f;
};

struct StylusSpec {
    bool valid = false;
    float x = 0.0f;
    float y = 0.0f;
    uint16_t pressure = 0;
    uint16_t signalX = 0;
    uint16_t signalY = 0;
    uint8_t animState = 0;
};

struct TrackerHarness {
    TouchTracker tracker;
    uint64_t timestamp = 0;

    HeatmapFrame Run(std::initializer_list<ContactSpec> contacts, const StylusSpec& stylus) {
        HeatmapFrame frame;
        timestamp += 8;
        frame.timestamp = timestamp;
        for (const auto& spec : contacts) {
            TouchContact contact;
            contact.x = spec.x;
            contact.y = spec.y;
            contact.area = spec.area;
            contact.signalSum = spec.signalSum;
            contact.sizeMm = spec.sizeMm;
            frame.contacts.push_back(contact);
        }

        frame.stylus.point.valid = stylus.valid;
        frame.stylus.point.x = stylus.x * 1024.0f;
        frame.stylus.point.y = stylus.y * 1024.0f;
        frame.stylus.pressure = stylus.pressure;
        frame.stylus.signalX = stylus.signalX;
        frame.stylus.signalY = stylus.signalY;
        frame.stylus.maxRawPeak = std::max(stylus.signalX, stylus.signalY);
        frame.stylus.animState = stylus.animState;

        tracker.Process(frame);
        return frame;
    }
};

StylusSpec MakeStylusSpec(bool valid,
                          float x,
                          float y,
                          uint16_t pressure,
                          uint16_t signalX,
                          uint16_t signalY,
                          uint8_t animState) {
    StylusSpec stylus;
    stylus.valid = valid;
    stylus.x = x;
    stylus.y = y;
    stylus.pressure = pressure;
    stylus.signalX = signalX;
    stylus.signalY = signalY;
    stylus.animState = animState;
    return stylus;
}

std::vector<const TouchContact*> VisibleContacts(const HeatmapFrame& frame) {
    std::vector<const TouchContact*> out;
    for (const auto& contact : frame.contacts) {
        if (contact.isReported) out.push_back(&contact);
    }
    return out;
}

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void TestWeakTouchNearStylusIsSuppressedUsingStylusCoordinates() {
    TrackerHarness harness;
    const StylusSpec stylus = MakeStylusSpec(true, 12.5f, 7.75f, 180, 500, 2200, 2);
    const auto frame = harness.Run({
        ContactSpec{12.5f, 7.75f, 3, 160, 0.8f}
    }, stylus);

    Require(frame.contacts.empty(), "weak overlap contact should be locally suppressed");
    Require(frame.stylus.touchSuppressActive, "stylus suppress flag should be active");
    Require(frame.stylus.recheckOverlap, "stylus overlap flag should be set");
    Require(frame.stylus.touchSuppressFrames > 0, "suppress hold should be armed");
}

void TestStrongTouchNearStylusIsPreserved() {
    TrackerHarness harness;
    const StylusSpec stylus = MakeStylusSpec(true, 18.0f, 12.0f, 160, 700, 2100, 2);
    const auto frame = harness.Run({
        ContactSpec{18.1f, 12.1f, 18, 7200, 3.2f}
    }, stylus);

    const auto visible = VisibleContacts(frame);
    Require(visible.size() == 1, "strong real touch near stylus should be preserved");
    Require(visible[0]->signalSum == 7200, "preserved touch should keep original signal");
}

void TestFarTouchIsNotMisSuppressed() {
    TrackerHarness harness;
    const StylusSpec stylus = MakeStylusSpec(true, 8.0f, 8.0f, 140, 600, 2000, 2);
    const auto frame = harness.Run({
        ContactSpec{24.0f, 20.0f, 10, 1200, 1.9f}
    }, stylus);

    const auto visible = VisibleContacts(frame);
    Require(visible.size() == 1, "far touch should remain visible");
    Require(!frame.stylus.recheckOverlap, "far touch should not look overlapped with stylus");
}

void TestAftKeepsSuppressingRecentWeakTouchAfterStylusLeaves() {
    TrackerHarness harness;

    const auto seed = harness.Run({}, MakeStylusSpec(true, 10.0f, 10.0f, 200, 640, 2400, 2));
    Require(seed.contacts.empty(), "seed stylus frame should not create contacts");

    const auto aftFrame = harness.Run({
        ContactSpec{10.2f, 10.1f, 4, 180, 0.9f}
    }, StylusSpec{});

    Require(aftFrame.contacts.size() == 1, "AFT frame should still keep hidden track state");
    Require(!aftFrame.contacts[0].isReported, "recent weak touch should be hidden by AFT");
    Require(aftFrame.contacts[0].id > 0, "AFT-hidden touch should still receive a track id");

    const auto aftHold = harness.Run({
        ContactSpec{10.3f, 10.0f, 4, 180, 0.9f}
    }, StylusSpec{});
    Require(aftHold.contacts.size() == 1, "AFT hold frame should keep the tracked touch");
    Require(!aftHold.contacts[0].isReported, "AFT hold should continue suppressing weak touch");
}

} // namespace

int main() {
    try {
        TestWeakTouchNearStylusIsSuppressedUsingStylusCoordinates();
        TestStrongTouchNearStylusIsPreserved();
        TestFarTouchIsNotMisSuppressed();
        TestAftKeepsSuppressingRecentWeakTouchAfterStylusLeaves();
        std::cout << "[TEST] TouchTracker stylus suppress tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
