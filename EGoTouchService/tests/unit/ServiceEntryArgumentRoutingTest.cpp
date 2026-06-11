#include "ServiceEntry.h"
#include "TestAssert.h"

namespace {

#if EGOTOUCH_SERVICE_ENABLE_IPC
struct FakeActions final : Service::IServiceEntryActions {
    bool installResult = true;
    bool uninstallResult = true;
    bool dispatcherResult = true;
    DWORD lastError = ERROR_SUCCESS;

    int installCalls = 0;
    int uninstallCalls = 0;
    int initializeCalls = 0;
    int consoleCalls = 0;
    int dispatcherCalls = 0;

    bool InstallService() override { ++installCalls; return installResult; }
    bool UninstallService() override { ++uninstallCalls; return uninstallResult; }
    void InitializeServiceProcess() override { ++initializeCalls; }
    void RunConsole() override { ++consoleCalls; }
    bool StartScmDispatcher() override { ++dispatcherCalls; return dispatcherResult; }
    DWORD LastErrorCode() const override { return lastError; }
};

int Invoke(FakeActions& actions, std::initializer_list<const wchar_t*> args) {
    wchar_t* argv[4]{};
    int argc = 0;
    for (const wchar_t* arg : args) {
        argv[argc++] = const_cast<wchar_t*>(arg);
    }
    return Service::ServiceEntryMain(argc, argv, actions);
}

bool InstallRouteDoesNotInitializeRuntime() {
    FakeActions actions;
    REQUIRE_EQ(Invoke(actions, {L"EGoTouchService.exe", L"--install"}), 0);
    REQUIRE_EQ(actions.installCalls, 1);
    REQUIRE_EQ(actions.initializeCalls, 0);
    REQUIRE_EQ(actions.dispatcherCalls, 0);
    REQUIRE_EQ(actions.consoleCalls, 0);
    return true;
}

bool UninstallFailureReturnsFailure() {
    FakeActions actions;
    actions.uninstallResult = false;
    REQUIRE_EQ(Invoke(actions, {L"EGoTouchService.exe", L"--uninstall"}), 1);
    REQUIRE_EQ(actions.uninstallCalls, 1);
    REQUIRE_EQ(actions.initializeCalls, 0);
    REQUIRE_EQ(actions.dispatcherCalls, 0);
    return true;
}

bool ConsoleRouteSkipsScmDispatcher() {
    FakeActions actions;
    REQUIRE_EQ(Invoke(actions, {L"EGoTouchService.exe", L"--console"}), 0);
    REQUIRE_EQ(actions.initializeCalls, 1);
#if EGOTOUCH_SERVICE_ENABLE_IPC
    REQUIRE_EQ(actions.consoleCalls, 1);
    REQUIRE_EQ(actions.dispatcherCalls, 0);
#else
    REQUIRE_EQ(actions.consoleCalls, 0);
    REQUIRE_EQ(actions.dispatcherCalls, 1);
#endif
    return true;
}

bool ScmSuccessDoesNotFallbackToConsole() {
    FakeActions actions;
    actions.dispatcherResult = true;
    REQUIRE_EQ(Invoke(actions, {L"EGoTouchService.exe"}), 0);
    REQUIRE_EQ(actions.initializeCalls, 1);
    REQUIRE_EQ(actions.dispatcherCalls, 1);
    REQUIRE_EQ(actions.consoleCalls, 0);
    return true;
}

bool ScmControllerConnectFailureFallsBackToConsole() {
    FakeActions actions;
    actions.dispatcherResult = false;
    actions.lastError = ERROR_FAILED_SERVICE_CONTROLLER_CONNECT;
    REQUIRE_EQ(Invoke(actions, {L"EGoTouchService.exe"}), 0);
    REQUIRE_EQ(actions.initializeCalls, 1);
    REQUIRE_EQ(actions.dispatcherCalls, 1);
#if EGOTOUCH_SERVICE_ENABLE_IPC
    REQUIRE_EQ(actions.consoleCalls, 1);
#else
    REQUIRE_EQ(actions.consoleCalls, 0);
#endif
    return true;
}

bool OtherScmFailureDoesNotFallbackToConsole() {
    FakeActions actions;
    actions.dispatcherResult = false;
    actions.lastError = ERROR_ACCESS_DENIED;
    REQUIRE_EQ(Invoke(actions, {L"EGoTouchService.exe"}), 0);
    REQUIRE_EQ(actions.initializeCalls, 1);
    REQUIRE_EQ(actions.dispatcherCalls, 1);
    REQUIRE_EQ(actions.consoleCalls, 0);
    return true;
}

} // namespace

int main() {
    int failures = 0;
    failures += RunTest(&InstallRouteDoesNotInitializeRuntime, "InstallRouteDoesNotInitializeRuntime");
    failures += RunTest(&UninstallFailureReturnsFailure, "UninstallFailureReturnsFailure");
    failures += RunTest(&ConsoleRouteSkipsScmDispatcher, "ConsoleRouteSkipsScmDispatcher");
    failures += RunTest(&ScmSuccessDoesNotFallbackToConsole, "ScmSuccessDoesNotFallbackToConsole");
    failures += RunTest(&ScmControllerConnectFailureFallsBackToConsole, "ScmControllerConnectFailureFallsBackToConsole");
    failures += RunTest(&OtherScmFailureDoesNotFallbackToConsole, "OtherScmFailureDoesNotFallbackToConsole");
    return failures == 0 ? 0 : 1;
}
#else
} // namespace
int main() {
    return 0;
}
#endif
