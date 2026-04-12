@echo off
echo ==============================================
echo EGoTouchRev - Build ^& Pack Release Version
echo ==============================================

cd /d "%~dp0\.."

echo.
echo [1/5] Building all CMake targets ...
cmake --build build --config Release
if %errorlevel% neq 0 (
    echo [ERROR] CMake build failed.
    exit /b %errorlevel%
)

echo.
echo [2/5] Copying arm64 VC++ CRT dependencies...
mkdir build\vcredist_arm64 2>nul
if defined VCToolsRedistDir (
    copy /Y "%VCToolsRedistDir%arm64\Microsoft.VC14*.CRT\*.dll" build\vcredist_arm64\
) else (
    copy /Y "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Redist\MSVC\14.50.35710\arm64\Microsoft.VC145.CRT\*.dll" build\vcredist_arm64\
)

echo.
echo [3/5] Packing Release Installers (English and Chinese)...
:: WiX v4 build command requires wix.exe
:: Make sure wix is installed on the user system.
wix build -ext WixToolset.UI.wixext -arch arm64 -culture en-US scripts\wix\EGoTouchSetup.wxs -loc scripts\wix\en-US.wxl -out build\EGoTouchSetup_en-US.msi
if %errorlevel% neq 0 (
    echo [ERROR] WiX build en-US failed.
    exit /b %errorlevel%
)

wix build -ext WixToolset.UI.wixext -arch arm64 -culture zh-CN scripts\wix\EGoTouchSetup.wxs -loc scripts\wix\zh-CN.wxl -out build\EGoTouchSetup_zh-CN.msi
if %errorlevel% neq 0 (
    echo [ERROR] WiX build zh-CN failed.
    exit /b %errorlevel%
)

echo [4/5] Packing Native ARM64 Bootstrapper (Multi-Language Setup.exe)...
wix build -ext WixToolset.BootstrapperApplications.wixext -arch arm64 scripts\wix\EGoTouchSetupBundle.wxs -out build\EGoTouchSetup_ARM64.exe
if %errorlevel% neq 0 (
    echo [ERROR] WiX Bundle build failed.
    exit /b %errorlevel%
)

echo [5/5] Build Successful! 
echo Release installers and Setup.exe have been generated at: build\
echo ==============================================

exit /b 0
