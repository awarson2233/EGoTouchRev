@echo off
setlocal

net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Administrator privileges required. Right-click "Run as administrator".
    pause
    exit /b 1
)

set "SRC=%~dp0config.ini"
set "DST_DIR=C:\ProgramData\EGoTouchRev"
set "DST=%DST_DIR%\config.ini"
set "BAK=%DST_DIR%\config.ini.bak"

echo === Apply Local EGoTouch Config ===
echo.
echo Source: %SRC%
echo Target: %DST%
echo.

if not exist "%SRC%" (
    echo [ERROR] Local config not found:
    echo         %SRC%
    echo.
    echo Place your local config at scripts\config.ini and run this script again.
    pause
    exit /b 1
)

if not exist "%DST_DIR%" mkdir "%DST_DIR%"
if not exist "%DST_DIR%\logs" mkdir "%DST_DIR%\logs"

if exist "%DST%" (
    copy /Y "%DST%" "%BAK%" >nul
    echo [INFO] Backed up existing config to:
    echo        %BAK%
)

copy /Y "%SRC%" "%DST%" >nul
if %errorlevel% neq 0 (
    echo [ERROR] Failed to copy local config.
    pause
    exit /b 1
)

echo [OK] Local config copied.

sc query EGoTouchService >nul 2>&1
if %errorlevel% equ 0 (
    echo [INFO] Restarting EGoTouchService to apply config...
    sc stop EGoTouchService >nul 2>&1
    timeout /t 2 /nobreak >nul
    sc start EGoTouchService >nul 2>&1
    timeout /t 2 /nobreak >nul
    sc query EGoTouchService | findstr STATE
) else (
    echo [INFO] EGoTouchService is not installed. Config will take effect on next start.
)

echo.
echo [DONE] Applied %SRC% to %DST%
echo.
pause

