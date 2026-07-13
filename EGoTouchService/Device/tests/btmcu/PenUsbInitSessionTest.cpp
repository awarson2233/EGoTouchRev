#include "btmcu/PenUsbInitSession.h"
#include "btmcu/PenUsbTypes.h"
#include "TestRequire.h"

#include <iostream>

namespace {

using DeviceTests::Require;
using EC = Himax::Pen::PenUsbEventCode;
using Action = Himax::Pen::PenUsbInitAction;

void TestFactoryInitAdvancesOnScreenStatusResponse() {
    Himax::Pen::PenUsbInitSession session;

    auto start = session.OnConnected();
    Require(start == Action::SendInitialQueries,
            "connection should send 0x7101 and the first 0x7701 query");

    auto penConnStatus = session.OnEvent(EC::PenConnStatus);
    Require(penConnStatus == Action::None,
            "0x71 should not be required before the first 0x7701 query");

    auto afterFirstMcuStatus = session.OnEvent(EC::PenScreenStatus);
    Require(afterFirstMcuStatus == Action::SendSecondMcuStatusQuery,
            "0x77 response to first 0x7701 should request second 0x7701 query");

    auto afterParamRequest = session.OnEvent(EC::PenRepParam);
    Require(afterParamRequest == Action::SendFactoryInitProtocolParams,
            "0x7B after second 0x7701 should send 0x7D01 init params");

    auto duplicateParamRequest = session.OnEvent(EC::PenRepParam);
    Require(duplicateParamRequest == Action::None,
            "duplicate 0x7B should not resend 0x7D01");
}

void TestOutOfOrderParamRequestIsIgnored() {
    Himax::Pen::PenUsbInitSession session;
    Require(session.OnConnected() == Action::SendInitialQueries,
            "connection should start init session");
    Require(session.OnEvent(EC::PenRepParam) == Action::None,
            "0x7B before 0x77 should not send init params");
    Require(session.OnEvent(EC::PenScreenStatus) == Action::SendSecondMcuStatusQuery,
            "session should still wait for 0x77 after out-of-order 0x7B");
}

void TestDuplicateMcuStatusDoesNotRepeatSecondQuery() {
    Himax::Pen::PenUsbInitSession session;
    Require(session.OnConnected() == Action::SendInitialQueries,
            "connection should start init session");
    Require(session.OnEvent(EC::PenScreenStatus) == Action::SendSecondMcuStatusQuery,
            "first 0x77 should trigger second query");
    Require(session.OnEvent(EC::PenScreenStatus) == Action::None,
            "duplicate 0x77 should not trigger another second query");
}

void TestRunningStateIgnoresFurtherEvents() {
    Himax::Pen::PenUsbInitSession session;
    session.OnConnected();
    session.OnEvent(EC::PenScreenStatus);
    Require(session.OnEvent(EC::PenRepParam) == Action::SendFactoryInitProtocolParams,
            "0x7B should transition session to running");
    Require(session.OnEvent(EC::PenScreenStatus) == Action::None,
            "running state should ignore 0x77");
    Require(session.OnEvent(EC::PenRepParam) == Action::None,
            "running state should ignore 0x7B");
}

void TestReconnectResetsSession() {
    Himax::Pen::PenUsbInitSession session;
    session.OnConnected();
    session.OnEvent(EC::PenScreenStatus);
    session.OnEvent(EC::PenRepParam);

    Require(session.OnConnected() == Action::SendInitialQueries,
            "reconnect should restart init query sequence");
    Require(session.OnEvent(EC::PenScreenStatus) == Action::SendSecondMcuStatusQuery,
            "reconnected session should accept first 0x77 again");
    Require(session.OnEvent(EC::PenRepParam) == Action::SendFactoryInitProtocolParams,
            "reconnected session should send init params again");
}

} // namespace

int main() {
    try {
        TestFactoryInitAdvancesOnScreenStatusResponse();
        TestOutOfOrderParamRequestIsIgnored();
        TestDuplicateMcuStatusDoesNotRepeatSecondQuery();
        TestRunningStateIgnoresFurtherEvents();
        TestReconnectResetsSession();
        std::cout << "[TEST] Device Pen USB init session tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
