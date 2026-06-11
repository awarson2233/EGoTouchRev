#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace Service {

class IServiceEntryActions {
public:
    virtual ~IServiceEntryActions() = default;

#if EGOTOUCH_SERVICE_ENABLE_IPC
    virtual bool InstallService() = 0;
    virtual bool UninstallService() = 0;
#endif
    virtual void InitializeServiceProcess() = 0;
#if EGOTOUCH_SERVICE_ENABLE_IPC
    virtual void RunConsole() = 0;
#endif
    virtual bool StartScmDispatcher() = 0;
    virtual DWORD LastErrorCode() const = 0;
};

int ServiceEntryMain(int argc, wchar_t* argv[], IServiceEntryActions& actions);

} // namespace Service
