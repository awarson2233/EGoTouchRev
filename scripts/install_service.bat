@echo off
:: EGoTouchService Install Script (dev build)
:: Installs from build/ directory directly
:: Requires Administrator privileges

net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Administrator privileges required. Right-click "Run as administrator".
    pause
    exit /b 1
)

echo === EGoTouchService Install (Dev Build) ===
echo.
echo Binary: %~dp0EGoTouchService.exe
echo.

:: Stop and remove old service if exists
sc query EGoTouchService >nul 2>&1
if %errorlevel% equ 0 (
    echo [INFO] Stopping existing service...
    sc stop EGoTouchService >nul 2>&1
    timeout /t 3 /nobreak >nul
    echo [INFO] Removing existing service...
    sc delete EGoTouchService >nul 2>&1
    timeout /t 2 /nobreak >nul
)

:: Create data directory
if not exist "C:\ProgramData\EGoTouchRev" mkdir "C:\ProgramData\EGoTouchRev"
if not exist "C:\ProgramData\EGoTouchRev\logs" mkdir "C:\ProgramData\EGoTouchRev\logs"

:: Install service pointing to build directory
sc create EGoTouchService binPath= "%~dp0EGoTouchService.exe" start= auto
if %errorlevel% neq 0 (
    echo [ERROR] Failed to create service.
    pause
    exit /b 1
)

:: Failure recovery: restart after 5s / 10s / 30s, reset counter after 24h
sc failure EGoTouchService reset= 86400 actions= restart/5000/restart/10000/restart/30000

:: Description
sc description EGoTouchService "EGoTouch Capacitive Touch Controller Driver Service (Dev Build)"

:: Start the service
echo [INFO] Starting service...
sc start EGoTouchService
timeout /t 2 /nobreak >nul

sc query EGoTouchService | findstr STATE
echo.
echo [OK] Install complete. Service registered from:
echo     %~dp0EGoTouchService.exe
echo.
pause
