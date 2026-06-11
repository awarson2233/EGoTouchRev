@echo off
:: EGoTouchService Debug Install Script
:: Installs from current directory
:: Requires Administrator privileges

net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Administrator privileges required. Right-click "Run as administrator".
    pause
    exit /b 1
)

echo === EGoTouchService Debug Install ===
echo.
echo Binary: %~dp0..\build\arm64-Debug\EGoTouchService.exe
echo.

:: Stop and remove old debug service if exists
sc query EGoTouchServiceDebug >nul 2>&1
if %errorlevel% equ 0 (
    echo [INFO] Stopping existing debug service...
    sc stop EGoTouchServiceDebug >nul 2>&1
    timeout /t 3 /nobreak >nul
    echo [INFO] Removing existing debug service...
    sc delete EGoTouchServiceDebug >nul 2>&1
    timeout /t 2 /nobreak >nul
)

:: Create data directory
if not exist "C:\ProgramData\EGoTouchRev" mkdir "C:\ProgramData\EGoTouchRev"
if not exist "C:\ProgramData\EGoTouchRev\logs" mkdir "C:\ProgramData\EGoTouchRev\logs"

:: Install service pointing to debug build directory (Manual start)
sc create EGoTouchServiceDebug binPath= "%~dp0..\build\arm64-Debug\EGoTouchService.exe" start= demand
if %errorlevel% neq 0 (
    echo [ERROR] Failed to create service.
    pause
    exit /b 1
)

:: Failure recovery: restart after 5s / 10s / 30s, reset counter after 24h
sc failure EGoTouchServiceDebug reset= 86400 actions= restart/5000/restart/10000/restart/30000

:: Description
sc description EGoTouchServiceDebug "EGoTouch Capacitive Touch Controller Driver Service (Debug)"

:: Start the service is removed for debug install, user needs to start it manually or attach debugger first
echo [INFO] Debug service installed. You can start it manually or attach a debugger.

sc query EGoTouchServiceDebug | findstr STATE
echo.
echo [OK] Debug Install complete. Service registered as EGoTouchServiceDebug (Manual Start) from:
echo     %~dp0..\build\arm64-Debug\EGoTouchService.exe
echo.
pause
