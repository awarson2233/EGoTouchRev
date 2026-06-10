/// EGoTouchService 入口点
/// 双模式启动：Windows SCM 服务 或 --console 控制台调试
/// 管理命令：--install / --uninstall

#include "ServiceEntry.h"
#include "ServiceShell.h"
#include "Logger.h"
#if EGOTOUCH_SERVICE_ENABLE_IPC
#include "GuiLogSink.h"
#endif

#include <string_view>

// ── 服务自注册 / 自卸载 ──────────────────────────────────

static bool EnsureDataDirectory() {
    CreateDirectoryW(L"C:\\ProgramData\\EGoTouchRev", nullptr);
    CreateDirectoryW(L"C:\\ProgramData\\EGoTouchRev\\logs", nullptr);
    return true;
}

static bool InstallService() {
    wchar_t exePath[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);

    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
    if (!scm) {
        wprintf(L"[ERROR] OpenSCManager failed (err=%lu). Run as Administrator.\n",
                GetLastError());
        return false;
    }

    SC_HANDLE svc = CreateServiceW(
        scm,
        Service::kServiceName,
        L"EGoTouch Capacitive Touch Driver",
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,          // 开机自启
        SERVICE_ERROR_NORMAL,
        exePath,
        nullptr, nullptr, nullptr, nullptr, nullptr);

    if (!svc) {
        DWORD err = GetLastError();
        if (err == ERROR_SERVICE_EXISTS) {
            wprintf(L"[WARN] Service already exists.\n");
        } else {
            wprintf(L"[ERROR] CreateService failed (err=%lu).\n", err);
        }
        CloseServiceHandle(scm);
        return err == ERROR_SERVICE_EXISTS;
    }

    // 崩溃恢复策略：5s → 10s → 30s 重启，24h 重置计数器
    SC_ACTION actions[3] = {
        { SC_ACTION_RESTART, 5000 },
        { SC_ACTION_RESTART, 10000 },
        { SC_ACTION_RESTART, 30000 },
    };
    SERVICE_FAILURE_ACTIONSW failCfg{};
    failCfg.dwResetPeriod = 86400;  // 24h
    failCfg.cActions = 3;
    failCfg.lpsaActions = actions;
    ChangeServiceConfig2W(svc, SERVICE_CONFIG_FAILURE_ACTIONS, &failCfg);

    // 服务描述
    SERVICE_DESCRIPTIONW desc{};
    desc.lpDescription = const_cast<wchar_t*>(
        L"EGoTouch 电容触控控制器驱动服务 — 管理触摸屏帧采集、算法处理与 HID 注入。");
    ChangeServiceConfig2W(svc, SERVICE_CONFIG_DESCRIPTION, &desc);

    CloseServiceHandle(svc);
    CloseServiceHandle(scm);

    EnsureDataDirectory();

    wprintf(L"[OK] Service installed successfully.\n");
    wprintf(L"     Start: sc start %s\n", Service::kServiceName);
    return true;
}

static bool UninstallService() {
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (!scm) {
        wprintf(L"[ERROR] OpenSCManager failed (err=%lu). Run as Administrator.\n",
                GetLastError());
        return false;
    }

    SC_HANDLE svc = OpenServiceW(scm, Service::kServiceName,
                                 SERVICE_STOP | DELETE | SERVICE_QUERY_STATUS);
    if (!svc) {
        wprintf(L"[ERROR] OpenService failed (err=%lu).\n", GetLastError());
        CloseServiceHandle(scm);
        return false;
    }

    // 尝试停止
    SERVICE_STATUS status{};
    ControlService(svc, SERVICE_CONTROL_STOP, &status);
    // 等待停止（最多 10 秒）
    for (int i = 0; i < 20; ++i) {
        QueryServiceStatus(svc, &status);
        if (status.dwCurrentState == SERVICE_STOPPED) break;
        Sleep(500);
    }

    BOOL ok = DeleteService(svc);
    CloseServiceHandle(svc);
    CloseServiceHandle(scm);

    if (ok) {
        wprintf(L"[OK] Service uninstalled.\n");
    } else {
        wprintf(L"[ERROR] DeleteService failed (err=%lu).\n", GetLastError());
    }
    return ok != FALSE;
}

// ── 主入口 seam ─────────────────────────────────────────

class ProductionServiceEntryActions final : public Service::IServiceEntryActions {
public:
    bool InstallService() override { return ::InstallService(); }
    bool UninstallService() override { return ::UninstallService(); }

    void InitializeServiceProcess() override {
        // Hide console window — logs are forwarded to App via IPC GetLogs
        if (HWND hw = GetConsoleWindow()) ShowWindow(hw, SW_HIDE);

        EnsureDataDirectory();

        // Elevate process priority for real-time touch processing
        if (!SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS)) {
            // Fallback: try HIGH if REALTIME fails (e.g. insufficient privileges)
            SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
        }

#if EGOTOUCH_SERVICE_ENABLE_IPC
        Common::Logger::Init("EGoTouchService", "C:/ProgramData/EGoTouchRev/logs/",
                              Common::GuiLogSink::Instance());
#else
        Common::Logger::Init("EGoTouchService", "C:/ProgramData/EGoTouchRev/logs/", nullptr);
#endif

        LOG_INFO("Service", __func__, "Boot", "Process priority set to REALTIME_PRIORITY_CLASS.");
    }

    void RunConsole() override {
        Service::ServiceShell::Instance()->RunAsConsole();
    }

    bool StartScmDispatcher() override {
        SERVICE_TABLE_ENTRYW table[] = {
            { const_cast<wchar_t*>(Service::kServiceName),
              Service::ServiceShell::SvcMain },
            { nullptr, nullptr }
        };
        return StartServiceCtrlDispatcherW(table) != FALSE;
    }

    DWORD LastErrorCode() const override {
        return GetLastError();
    }
};

int wmain(int argc, wchar_t* argv[]) {
    ProductionServiceEntryActions actions;
    const int result = Service::ServiceEntryMain(argc, argv, actions);
    Common::Logger::Shutdown();
    return result;
}
