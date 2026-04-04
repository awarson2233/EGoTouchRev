@echo off
echo ==============================================
echo EGoTouchRev - Build ^& Pack Test Version
echo ==============================================

cd /d "%~dp0\.."

echo.
echo [1/3] Building all CMake targets (Service, App, Tools)...
cmake --build build --config Release
if %errorlevel% neq 0 (
    echo [ERROR] CMake build failed.
    exit /b %errorlevel%
)

echo.
echo [2/3] Preparing MSI Component variables...
:: WiX v4 build command requires wix.exe
:: Make sure wix is installed on the user system.
wix build -ext WixToolset.UI.wixext -arch x64 scripts\EGoTouchTestSetup.wxs -out build\EGoTouchTestSetup.msi
if %errorlevel% neq 0 (
    echo [ERROR] WiX build failed.
    exit /b %errorlevel%
)

echo.
echo [3/3] Build Successful! 
echo Test installer has been generated at: build\EGoTouchTestSetup.msi
echo ==============================================
exit /b 0
