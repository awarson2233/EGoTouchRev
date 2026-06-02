# EGoTouchRev (Rebuild) 🚀

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Windows_11_ARM64-lightgrey.svg)]()
[![Build](https://img.shields.io/badge/Build-CMake_|_WiX_v4-orange.svg)]()

**EGoTouchRev** is a community-driven, open-source replacement driver and service suite for proprietary capacitive touch controllers (primarily targeting Himax IC and accompanying BT-MCU stylus input). 

Developed entirely through clean-room reverse engineering, this project aims to provide a reliable, modular, and natively built touch interaction layer with an advanced algorithmic pipeline for Windows on ARM64 (WoA) devices.

---

## 🌟 Key Features

* **Native System Service (`EGoTouchService`)**: A rock-solid, C++ driven Windows service (`LocalSystem`) handling continuous touch frame acquisition, lifecycle monitoring, and seamless HID report injection.
* **Hardware Integration**: Deep protocol-level integration with **Himax capacitive chips** for ultra-responsive multi-touch handling. *(Note: Active Stylus / BT-MCU integration is temporarily disconnected in this branch to focus on core touch stability)*.
* **Custom Processing Pipeline**: Overhauled from scratch with anti-jitter, anti-bounce, and 1 Euro filtering for unparalleled touch smoothing and coordinate accuracy.
* **Advanced Diagnostics**: Ships with `EGoTouchApp`, a native diagnostic GUI application (Diagnostics Workbench) for real-time visualization, raw data monitoring, and event logging.
* **100% ARM64 Native Deployment**: Fully customized WiX v4 build toolkit ensuring that no WOW64 emulation is required—deployment is completely native to modern ARM64 systems.

---

## 💻 Installation

We provide **Pure ARM64 MSI Installers** packaged using WiX Toolset v4. Commit and PR builds only compile and run smoke tests; release installers are built from `vMAJOR.MINOR.PATCH` tags and attached directly to GitHub Releases.

### Easy Install (End-Users)
1. Download the latest `EGoTouchSetup_arm64_vX.Y.Z.msi` from the [Releases](#) page.
   - `EGoTouchSetup_arm64_vX.Y.Z.msi`: core service installer.
   - `EGoTouchTestSetup_arm64_vX.Y.Z.msi`: service plus diagnostic tools.
2. Double click to run the setup wizard.
3. The installer will automatically deploy the binaries and configure `EGoTouchService` to launch silently in the background.

*(**Note**: Elevated Administrator privileges are strictly required to configure the system service).*

### Build from Source (Developers)

This project relies on **CMake** for code compilation and **WiX v4** for MSI packaging. Release automation uses the Git tag as the public version, while WiX receives the numeric MSI version without the leading `v`.

```powershell
# 1. Compile the binaries
cmake -G "Ninja" -S . -B build `
      -DCMAKE_BUILD_TYPE=Release `
      -DCMAKE_C_COMPILER=clang-cl `
      -DCMAKE_CXX_COMPILER=clang-cl `
      -DHIMAX_ENABLE_NEON=ON `
      -DEGO_BUILD_TESTS=ON
cmake --build build --config Release --parallel

# 2. Package the MSI Installer (Requires .NET and WiX)
dotnet tool install --global wix
wix eula accept wix7
wix extension add -g WixToolset.UI.wixext
wix build -ext WixToolset.UI.wixext -arch arm64 -d BuildVersion=1.2.3 scripts\EGoTouchSetup.wxs -loc scripts\zh-CN.wxl -out build\EGoTouchSetup_arm64_v1.2.3.msi
```

---

## 🏗️ Architecture Layout

- `EGoTouchService/` - The core Windows Service engine fetching and computing hardware reports.
  - `Device/` - Hardware abstraction layers (Himax, BT/MCU protocols).
  - `Engine/` - The algorithmic heartbeat (Touch pipelines, 1 Euro Filters, pressure parsing).
  - `Host/` - System and Windows OS interfaces (HID Injection, ACPI/Lid monitoring).
- `Tools/EGoTouchApp/` - The visual diagnostic dashboard for monitoring sensor states.
- `scripts/` - Inno Setup / WiX installation scripts and environment configuration tools.

---

## ⚖️ Legal Disclaimers

**IMPORTANT**: This software is provided exclusively for **non-commercial, personal use** and **educational research (interoperability)**.

This project is **NOT affiliated** with Himax, the original device manufacturer(s), or any associated trademark holder. It utilizes original community code synthesized via reverse-engineering to ensure hardware interoperability on alternative operating environments. 

**Use at Your Own Risk:** Modifying low-level hardware drivers carries inherent risks. The authors and contributors shall not be held liable for any hardware bricking, system corruption, or potential third-party TOS violations resulting from the use of this software. 

For the complete text, please refer to the [LICENSE](LICENSE) file. 
