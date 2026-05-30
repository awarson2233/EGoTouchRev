#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace Service {

class IServiceEntryActions {
public:
    virtual ~IServiceEntryActions() = default;

    virtual bool InstallService() = 0;
    virtual bool UninstallService() = 0;
    virtual void InitializeServiceProcess() = 0;
    virtual void RunConsole() = 0;
    virtual bool StartScmDispatcher() = 0;
    virtual DWORD LastErrorCode() const = 0;
};

int ServiceEntryMain(int argc, wchar_t* argv[], IServiceEntryActions& actions);

} // namespace Service
