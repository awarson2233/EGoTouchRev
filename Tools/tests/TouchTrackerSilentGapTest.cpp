#include "TouchSolver/CoordinateFilter.hpp"
#include "TouchSolver/TouchGestureStateMachine.hpp"
#include "TouchSolver/TouchTracker.hpp"

#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

using Solvers::HeatmapFrame;
using Solvers::TouchContact;
using Solvers::TouchLifeSilentGap;
using Solvers::TouchReportDown;
using Solvers::TouchReportMove;
using Solvers::TouchReportUp;
using Solvers::Touch::CoordinateFilter;
using Solvers::Touch::TouchGestureStateMachine;
using Solvers::Touch::TouchTracker;

struct PipelineHarness {
    TouchTracker tracker;
    CoordinateFilter filter;
    TouchGestureStateMachine gesture;
    uint64_t timestamp = 0;

    HeatmapFrame Run(std::initializer_list<std::pair<float, float>> points) {
        HeatmapFrame frame;
        timestamp += 8;
        frame.timestamp = timestamp;
        for (const auto& [x, y] : points) {
            TouchContact c;
            c.x = x;
            c.y = y;
            c.area = 12;
            c.signalSum = 1200;
            frame.contacts.push_back(c);
        }
        tracker.Process(frame);
        filter.Process(frame);
        gesture.Process(frame);
        return frame;
    }
};

std::vector<const TouchContact*> VisibleContacts(const HeatmapFrame& frame) {
    std::vector<const TouchContact*> out;
    for (const auto& c : frame.contacts) {
        if (c.isReported) out.push_back(&c);
    }
    return out;
}

const TouchContact* FindContactById(const HeatmapFrame& frame, int id) {
    for (const auto& c : frame.contacts) {
        if (c.id == id) return &c;
    }
    return nullptr;
}

const TouchContact* FindVisibleById(const HeatmapFrame& frame, int id) {
    for (const auto& c : frame.contacts) {
        if (c.id == id && c.isReported) return &c;
    }
    return nullptr;
}

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void TestSingleFingerSilentGapRelink() {
    PipelineHarness h;
    const auto f1 = h.Run({{10.0f, 10.0f}});
    const auto v1 = VisibleContacts(f1);
    Require(v1.size() == 1 && v1[0]->reportEvent == TouchReportDown, "frame1 should report one Down");
    const int id = v1[0]->id;

    const auto f2 = h.Run({{12.0f, 10.0f}});
    const auto v2 = VisibleContacts(f2);
    Require(v2.size() == 1 && v2[0]->id == id && v2[0]->reportEvent == TouchReportMove, "frame2 should keep same id as Move");

    const auto f3 = h.Run({});
    Require(VisibleContacts(f3).empty(), "gap frame should stay silent");
    const auto* hidden = FindContactById(f3, id);
    Require(hidden != nullptr, "silent gap contact should stay in frame");
    Require(!hidden->isReported && (hidden->lifeFlags & TouchLifeSilentGap) != 0, "silent gap contact should be hidden");

    const auto f4 = h.Run({{14.0f, 10.0f}});
    const auto* resumed = FindVisibleById(f4, id);
    Require(resumed != nullptr, "relinked finger should recover original id");
    Require(resumed->reportEvent == TouchReportMove, "relinked finger should resume as Move");
}

void TestSingleFingerGapTimeout() {
    PipelineHarness h;
    const int id = VisibleContacts(h.Run({{10.0f, 10.0f}}))[0]->id;
    h.Run({{12.0f, 10.0f}});

    const auto gap1 = h.Run({});
    Require(FindContactById(gap1, id) != nullptr, "first gap frame should keep hidden contact");
    const auto gap2 = h.Run({});
    Require(FindContactById(gap2, id) != nullptr, "second gap frame should keep hidden contact");

    const auto timeout = h.Run({});
    const auto* up = FindVisibleById(timeout, id);
    Require(up != nullptr && up->reportEvent == TouchReportUp, "timeout frame should emit Up");
    const auto* stale = FindContactById(timeout, id);
    Require(stale != nullptr && (stale->lifeFlags & TouchLifeSilentGap) == 0, "timeout frame should no longer expose silent gap state");
}

void TestTwoFingerRelinkKeepsOtherFinger() {
    PipelineHarness h;
    const auto first = h.Run({{10.0f, 10.0f}, {30.0f, 10.0f}});
    const auto vis1 = VisibleContacts(first);
    Require(vis1.size() == 2, "frame1 should report two fingers");
    const int idA = vis1[0]->id;
    const int idB = vis1[1]->id;

    h.Run({{12.0f, 10.0f}, {28.0f, 10.0f}});

    const auto gap = h.Run({{26.0f, 10.0f}});
    Require(FindVisibleById(gap, idB) != nullptr, "remaining finger should keep visible id");
    const auto* hiddenA = FindContactById(gap, idA);
    Require(hiddenA != nullptr && !hiddenA->isReported, "missing finger should enter hidden silent gap");

    const auto resume = h.Run({{14.0f, 10.0f}, {24.0f, 10.0f}});
    Require(FindVisibleById(resume, idA) != nullptr, "recovered finger should relink to original id");
    Require(FindVisibleById(resume, idB) != nullptr, "other finger should keep original id");
}

void TestAmbiguousSilentGapDoesNotHijackOldIds() {
    PipelineHarness h;
    const auto first = h.Run({{10.0f, 10.0f}, {20.0f, 10.0f}});
    const auto vis1 = VisibleContacts(first);
    Require(vis1.size() == 2, "frame1 should report two fingers");
    const int oldA = vis1[0]->id;
    const int oldB = vis1[1]->id;

    h.Run({{12.0f, 10.0f}, {18.0f, 10.0f}});
    const auto bothGap = h.Run({});
    Require(FindContactById(bothGap, oldA) != nullptr && FindContactById(bothGap, oldB) != nullptr, "both fingers should enter silent gap");

    const auto ambiguous = h.Run({{15.0f, 10.0f}, {15.0f, 10.0f}});
    const auto visible = VisibleContacts(ambiguous);
    Require(visible.size() == 2, "ambiguous frame should still output two current contacts");
    for (const auto* c : visible) {
        Require(c->id != oldA && c->id != oldB, "ambiguous relink should not hijack old ids");
    }
    const auto* hiddenA = FindContactById(ambiguous, oldA);
    const auto* hiddenB = FindContactById(ambiguous, oldB);
    Require(hiddenA != nullptr && !hiddenA->isReported, "oldA should stay hidden when relink is ambiguous");
    Require(hiddenB != nullptr && !hiddenB->isReported, "oldB should stay hidden when relink is ambiguous");
}

} // namespace

int main() {
    try {
        TestSingleFingerSilentGapRelink();
        TestSingleFingerGapTimeout();
        TestTwoFingerRelinkKeepsOtherFinger();
        TestAmbiguousSilentGapDoesNotHijackOldIds();
        std::cout << "[TEST] TouchTracker silent-gap relink tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
