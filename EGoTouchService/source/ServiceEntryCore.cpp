#include "ServiceEntry.h"

#include <string_view>

namespace Service {

int ServiceEntryMain(int argc, wchar_t* argv[], IServiceEntryActions& actions) {
#if EGOTOUCH_SERVICE_ENABLE_IPC
    // 解析管理命令（不需要 Logger）
    if (argc >= 2) {
        std::wstring_view arg1(argv[1]);
        if (arg1 == L"--install") return actions.InstallService() ? 0 : 1;
        if (arg1 == L"--uninstall") return actions.UninstallService() ? 0 : 1;
    }
#endif

    actions.InitializeServiceProcess();

#if EGOTOUCH_SERVICE_ENABLE_IPC
    const bool consoleMode =
        (argc >= 2 && std::wstring_view(argv[1]) == L"--console");

    if (consoleMode) {
        actions.RunConsole();
        return 0;
    }
#endif

    if (!actions.StartScmDispatcher()) {
#if EGOTOUCH_SERVICE_ENABLE_IPC
        const DWORD err = actions.LastErrorCode();
        if (err == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) {
            // 双击运行或无 SCM 环境 → 退回控制台
            actions.RunConsole();
        }
#endif
    }

    return 0;
}

} // namespace Service
