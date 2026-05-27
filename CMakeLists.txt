cmake_minimum_required(VERSION 3.20)
project(EGoTouchRev VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 自动生成 .clangd 配置文件以支持 clangd 自动识别系统头文件和 Windows Kits
if (WIN32 AND CMAKE_EXPORT_COMPILE_COMMANDS)
    set(CLANGD_FILE "${CMAKE_CURRENT_SOURCE_DIR}/.clangd")
    message(STATUS "Checking/Generating .clangd config for clangd language server...")
    
    # 查找本地最新的 MSVC 包含路径
    set(MSVC_INCLUDE_DIRS "")
    file(GLOB vs_msvc_dirs 
        "C:/Program Files/Microsoft Visual Studio/2022/*/VC/Tools/MSVC/*"
        "C:/Program Files/Microsoft Visual Studio/18/*/VC/Tools/MSVC/*"
        "C:/Program Files (x86)/Microsoft Visual Studio/2022/*/VC/Tools/MSVC/*"
    )
    if (vs_msvc_dirs)
        list(SORT vs_msvc_dirs ORDER DESCENDING)
        list(GET vs_msvc_dirs 0 latest_msvc_dir)
        if (EXISTS "${latest_msvc_dir}/include")
            list(APPEND MSVC_INCLUDE_DIRS 
                "${latest_msvc_dir}/include"
                "${latest_msvc_dir}/atlmfc/include"
            )
        endif()
    endif()

    # 查找本地最新的 Windows SDK 包含路径
    set(WIN_SDK_INCLUDE_DIRS "")
    file(GLOB sdk_dirs "C:/Program Files (x86)/Windows Kits/10/Include/*")
    if (sdk_dirs)
        list(SORT sdk_dirs ORDER DESCENDING)
        foreach(sdk_dir IN LISTS sdk_dirs)
            if (sdk_dir MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+")
                set(latest_sdk_dir "${sdk_dir}")
                break()
            endif()
        endforeach()
        
        if (latest_sdk_dir)
            list(APPEND WIN_SDK_INCLUDE_DIRS
                "${latest_sdk_dir}/ucrt"
                "${latest_sdk_dir}/shared"
                "${latest_sdk_dir}/um"
                "${latest_sdk_dir}/winrt"
                "${latest_sdk_dir}/cppwinrt"
            )
        endif()
    endif()
    
    # 构造 .clangd 的配置内容
    set(SYSTEM_INCLUDES ${MSVC_INCLUDE_DIRS} ${WIN_SDK_INCLUDE_DIRS})
    if (SYSTEM_INCLUDES)
        set(CLANGD_CONTENT "CompileFlags:\n")
        string(APPEND CLANGD_CONTENT "  CompilationDatabase: build\n")
        string(APPEND CLANGD_CONTENT "  Add:\n")
        
        foreach(dir IN LISTS SYSTEM_INCLUDES)
            string(REPLACE "\\" "/" dir_clean "${dir}")
            string(APPEND CLANGD_CONTENT "    - /imsvc\n")
            string(APPEND CLANGD_CONTENT "    - ${dir_clean}\n")
        endforeach()
        
        file(WRITE "${CLANGD_FILE}" "${CLANGD_CONTENT}")
        message(STATUS "Auto-generated .clangd config successfully at: ${CLANGD_FILE}")
    else()
        message(WARNING "Could not auto-detect MSVC or Windows SDK directories. .clangd was not generated.")
    endif()
endif()

include(CTest)
enable_testing()
# Use static MSVC runtime to avoid missing VCRUNTIME140.dll on target machines
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# --- Platform compatibility ---
# Prevent Windows.h from defining min/max macros that conflict with std::min/std::max
# Suppress MSVC CRT deprecation warnings (e.g. localtime -> localtime_s)
if (WIN32)
    add_compile_definitions(NOMINMAX _CRT_SECURE_NO_WARNINGS)
    if(MSVC AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(/Zc:preprocessor)
    endif()
endif()

if(MSVC)
    add_compile_options($<$<CONFIG:Release>:/Zi>)
    add_link_options($<$<CONFIG:Release>:/DEBUG:FULL> $<$<CONFIG:Release>:/OPT:REF> $<$<CONFIG:Release>:/OPT:ICF>)
endif()

# --- SIMD options ---
# This project targets a specific platform; enable NEON by default.
option(HIMAX_ENABLE_NEON "Enable ARM NEON intrinsics when available" ON)

# --- Common Library ---
# Use the source directory as the root for subprojects/resources so paths
# resolve correctly whether building in-tree or out-of-tree.
set(COMMON_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/Common")
set(IMGUI_ROOT "${COMMON_ROOT}/imgui-docking")

# Collect ImGui source files
file(GLOB IMGUI_SOURCES
    "${IMGUI_ROOT}/imgui.cpp"
    "${IMGUI_ROOT}/imgui_demo.cpp"
    "${IMGUI_ROOT}/imgui_draw.cpp"
    "${IMGUI_ROOT}/imgui_tables.cpp"
    "${IMGUI_ROOT}/imgui_widgets.cpp"
)

file(GLOB COMMON_SOURCES "${COMMON_ROOT}/source/*.cpp")
file(GLOB COMMON_HEADERS "${COMMON_ROOT}/include/*.h")

add_subdirectory("${COMMON_ROOT}/spdlog-1.17.0" "spdlog_build")

# Common library: Pure utilities (Logger, SharedFrameBuffer) - NO ImGui
add_library(Common STATIC
    ${COMMON_SOURCES}
    ${COMMON_HEADERS}
)

target_include_directories(Common PUBLIC
    "${COMMON_ROOT}/include"
)
# Common links Solvers for SharedFrameBuffer (EngineTypes.h)
# Forward-declare include path; Solvers target linked below after Solvers is defined
target_link_libraries(Common PUBLIC spdlog::spdlog)

# ImGuiCommon library: UI framework (only for diagnostic tools)
add_library(ImGuiCommon STATIC
    ${IMGUI_SOURCES}
)

target_include_directories(ImGuiCommon PUBLIC
    "${IMGUI_ROOT}"
)

# --- Device Module (Hardware Abstraction) ---
set(DEVICE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/EGoTouchService/Device")
file(GLOB_RECURSE DEVICE_SOURCES "${DEVICE_ROOT}/*.cpp")
file(GLOB_RECURSE DEVICE_HEADERS "${DEVICE_ROOT}/*.h")

add_library(Device STATIC
    ${DEVICE_SOURCES}
    ${DEVICE_HEADERS}
)

target_compile_definitions(Device PUBLIC
    $<$<BOOL:${HIMAX_ENABLE_NEON}>:HIMAX_ENABLE_NEON=1>
    $<$<NOT:$<BOOL:${HIMAX_ENABLE_NEON}>>:HIMAX_ENABLE_NEON=0>
)

target_include_directories(Device PUBLIC
    "${DEVICE_ROOT}"
)
target_link_libraries(Device PUBLIC Common Host Solvers setupapi hid)

# --- Solvers Module (Touch Algorithm) ---
set(SOLVERS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/EGoTouchService/Solvers")

add_library(Solvers STATIC
    # TouchSolver (linear orchestrator — all algorithm modules are header-only .hpp)
    EGoTouchService/Solvers/TouchSolver/TouchPipeline.cpp
    # StylusSolver (official ASA pipeline — modules are header-only .hpp)
    EGoTouchService/Solvers/StylusSolver/StylusPipeline.cpp
)

target_compile_definitions(Solvers PUBLIC
    $<$<BOOL:${HIMAX_ENABLE_NEON}>:HIMAX_ENABLE_NEON=1>
    $<$<NOT:$<BOOL:${HIMAX_ENABLE_NEON}>>:HIMAX_ENABLE_NEON=0>
)

target_include_directories(Solvers PUBLIC
    "${SOLVERS_ROOT}"
    "${SOLVERS_ROOT}/TouchSolver"
    "${SOLVERS_ROOT}/StylusSolver"
)
target_link_libraries(Solvers PUBLIC Common)
# Let Common see Solvers headers for SharedFrameBuffer
target_include_directories(Common PUBLIC "${SOLVERS_ROOT}")

# --- Host Module (System Integration) ---
set(HOST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/EGoTouchService/Host")
file(GLOB HOST_SOURCES "${HOST_ROOT}/*.cpp")
file(GLOB HOST_HEADERS "${HOST_ROOT}/*.h")

add_library(Host STATIC
    ${HOST_SOURCES}
    ${HOST_HEADERS}
)
target_include_directories(Host PUBLIC "${HOST_ROOT}")
target_link_libraries(Host PUBLIC Common)
if (WIN32)
    target_link_libraries(Host PUBLIC advapi32)
endif()

# --- EGoTouchApp (Debug Diagnostics Workbench) ---
set(APP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/Tools/EGoTouchApp")
file(GLOB APP_SOURCES "${APP_ROOT}/source/*.cpp")
file(GLOB APP_HEADERS "${APP_ROOT}/include/*.h")

# ImGui Backend sources (DX11 + Win32)
set(IMGUI_BACKEND_SOURCES 
    "${IMGUI_ROOT}/backends/imgui_impl_win32.cpp"
    "${IMGUI_ROOT}/backends/imgui_impl_dx11.cpp"
)

add_executable(EGoTouchApp
    ${APP_SOURCES}
    ${APP_HEADERS}
    ${IMGUI_BACKEND_SOURCES}
)

target_include_directories(EGoTouchApp PRIVATE
    "${APP_ROOT}/include"
    "${IMGUI_ROOT}"
    "${IMGUI_ROOT}/backends"
)

# Link D3D11 for the ImGui DX11 backend
target_compile_definitions(EGoTouchApp PRIVATE EGOTOUCH_DIAGNOSTICS)
target_link_libraries(EGoTouchApp PRIVATE Device Solvers Host Common ImGuiCommon d3d11 d3dcompiler dwmapi setupapi)

# --- EGoTouchService (Production Service Shell) ---
set(SERVICE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/EGoTouchService")

add_executable(EGoTouchService
    "${SERVICE_ROOT}/source/ServiceEntry.cpp"
    "${SERVICE_ROOT}/source/ServiceShell.cpp"
    "${SERVICE_ROOT}/source/ServiceHost.cpp"
)
target_include_directories(EGoTouchService PRIVATE
    "${SERVICE_ROOT}/include"
)
target_link_libraries(EGoTouchService PRIVATE Device Host Common advapi32 setupapi hid)

# EGoTouchService uses wmain (console subsystem) for service/console dual mode
if(MSVC)
    set_target_properties(EGoTouchService PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
elseif(WIN32)
    target_link_options(EGoTouchService PRIVATE -municode)
endif()

# --- BtMcuTestTool (ImGui BT Protocol Validator) ---
add_executable(BtMcuTestTool
    Tools/BtMcuTestTool/BtMcuTestTool.cpp
    ${IMGUI_BACKEND_SOURCES}
)
target_include_directories(BtMcuTestTool PRIVATE
    "${DEVICE_ROOT}"
    "${COMMON_ROOT}/include"
    "${IMGUI_ROOT}"
    "${IMGUI_ROOT}/backends"
)
target_link_libraries(BtMcuTestTool PRIVATE Device Common ImGuiCommon d3d11 d3dcompiler dwmapi setupapi)

option(EGO_BUILD_TESTS "Build local test executables" ON)
if (EGO_BUILD_TESTS)
    add_executable(HostSystemStateMonitorTest
        Tools/tests/SystemStateMonitorNamedEventTest.cpp
    )
    target_include_directories(HostSystemStateMonitorTest PRIVATE "${HOST_ROOT}/include")
    target_link_libraries(HostSystemStateMonitorTest PRIVATE Host Common)

    add_executable(SolversRawdataBenchmarkTest
        Tools/tests/RawdataBenchmarkTest.cpp
    )
    target_link_libraries(SolversRawdataBenchmarkTest PRIVATE Solvers Common)

    add_executable(CommonLoggerTest
        Tools/tests/LoggerTest.cpp
    )
    target_include_directories(CommonLoggerTest PRIVATE "${COMMON_ROOT}/include")
    target_link_libraries(CommonLoggerTest PRIVATE Common)

    add_executable(TouchTrackerSilentGapTest
        Tools/tests/TouchTrackerSilentGapTest.cpp
    )
    target_link_libraries(TouchTrackerSilentGapTest PRIVATE Solvers Common)

    add_executable(TouchGridIIRProcessorTest
        Tools/tests/TouchGridIIRProcessorTest.cpp
    )
    target_link_libraries(TouchGridIIRProcessorTest PRIVATE Solvers Common)

    add_executable(TouchTrackerStylusSuppressTest
        Tools/tests/TouchTrackerStylusSuppressTest.cpp
    )
    target_link_libraries(TouchTrackerStylusSuppressTest PRIVATE Solvers Common)

    add_executable(TouchPipelineConfigRoundTripTest
        Tools/tests/TouchPipelineConfigRoundTripTest.cpp
    )
    target_link_libraries(TouchPipelineConfigRoundTripTest PRIVATE Solvers Common)

    add_executable(TouchEdgeCompensationTest
        Tools/tests/TouchEdgeCompensationTest.cpp
    )
    target_link_libraries(TouchEdgeCompensationTest PRIVATE Solvers Common)

    add_executable(TouchPalmRejectionMvpTest
        Tools/tests/TouchPalmRejectionMvpTest.cpp
    )
    target_link_libraries(TouchPalmRejectionMvpTest PRIVATE Solvers Common)

    add_executable(TouchCloseSplitTest
        Tools/tests/TouchCloseSplitTest.cpp
    )
    target_link_libraries(TouchCloseSplitTest PRIVATE Solvers Common)

    add_executable(ServiceProxyConfigMergeTest
        Tools/tests/ServiceProxyConfigMergeTest.cpp
        Tools/EGoTouchApp/source/ServiceProxy.cpp
    )
    target_include_directories(ServiceProxyConfigMergeTest PRIVATE
        "${APP_ROOT}/include"
    )
    target_link_libraries(ServiceProxyConfigMergeTest PRIVATE Solvers Common)

    add_executable(StylusLinearFilterProcessTest
        Tools/tests/StylusLinearFilterProcessTest.cpp
    )
    target_link_libraries(StylusLinearFilterProcessTest PRIVATE Solvers Common)

    add_executable(StylusHpp3PostPressureProcessTest
        Tools/tests/StylusHpp3PostPressureProcessTest.cpp
    )
    target_link_libraries(StylusHpp3PostPressureProcessTest PRIVATE Solvers Common)

    add_executable(VhfReporterStylusPacketTest
        Tools/tests/VhfReporterStylusPacketTest.cpp
    )
    target_link_libraries(VhfReporterStylusPacketTest PRIVATE Device Solvers Common)

    add_executable(VhfReporterTouchPacketTest
        Tools/tests/VhfReporterTouchPacketTest.cpp
    )
    target_link_libraries(VhfReporterTouchPacketTest PRIVATE Device Solvers Common)

    add_executable(PenUsbProtocolTest
        Tools/tests/PenUsbProtocolTest.cpp
    )
    target_link_libraries(PenUsbProtocolTest PRIVATE Device Common)

    add_executable(PenUsbInitSessionTest
        Tools/tests/PenUsbInitSessionTest.cpp
    )
    target_link_libraries(PenUsbInitSessionTest PRIVATE Device Common)

    add_executable(DeviceRuntimeSystemPowerPolicyTest
        Tools/tests/DeviceRuntimeSystemPowerPolicyTest.cpp
    )
    target_link_libraries(DeviceRuntimeSystemPowerPolicyTest PRIVATE Device Common)

    add_executable(PenFactoryStatusFlagsTest
        Tools/tests/PenFactoryStatusFlagsTest.cpp
    )
    target_link_libraries(PenFactoryStatusFlagsTest PRIVATE Device Common)

    add_executable(PenPressurePacketParserTest
        Tools/tests/PenPressurePacketParserTest.cpp
    )
    target_link_libraries(PenPressurePacketParserTest PRIVATE Device Common)

    add_test(NAME CommonLoggerTest COMMAND CommonLoggerTest)
    add_test(NAME HostSystemStateMonitorTest COMMAND HostSystemStateMonitorTest)
    add_test(NAME TouchTrackerSilentGapTest COMMAND TouchTrackerSilentGapTest)
    add_test(NAME TouchGridIIRProcessorTest COMMAND TouchGridIIRProcessorTest)
    add_test(NAME TouchTrackerStylusSuppressTest COMMAND TouchTrackerStylusSuppressTest)
    add_test(NAME TouchPipelineConfigRoundTripTest COMMAND TouchPipelineConfigRoundTripTest)
    add_test(NAME TouchEdgeCompensationTest COMMAND TouchEdgeCompensationTest)
    add_test(NAME TouchPalmRejectionMvpTest COMMAND TouchPalmRejectionMvpTest)
    add_test(NAME TouchCloseSplitTest COMMAND TouchCloseSplitTest)
    add_test(NAME ServiceProxyConfigMergeTest COMMAND ServiceProxyConfigMergeTest)
    add_test(NAME StylusLinearFilterProcessTest COMMAND StylusLinearFilterProcessTest)
    add_test(NAME StylusHpp3PostPressureProcessTest COMMAND StylusHpp3PostPressureProcessTest)
    add_test(NAME VhfReporterStylusPacketTest COMMAND VhfReporterStylusPacketTest)
    add_test(NAME VhfReporterTouchPacketTest COMMAND VhfReporterTouchPacketTest)
    add_test(NAME PenUsbProtocolTest COMMAND PenUsbProtocolTest)
    add_test(NAME PenUsbInitSessionTest COMMAND PenUsbInitSessionTest)
    add_test(NAME DeviceRuntimeSystemPowerPolicyTest COMMAND DeviceRuntimeSystemPowerPolicyTest)
    add_test(NAME PenFactoryStatusFlagsTest COMMAND PenFactoryStatusFlagsTest)
    add_test(NAME PenPressurePacketParserTest COMMAND PenPressurePacketParserTest)
endif()


