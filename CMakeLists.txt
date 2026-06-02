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

set(CMAKE_CTEST_ARGUMENTS "--verbose" CACHE STRING "Arguments passed to ctest by generated test targets")
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
set(IPC_ROOT "${COMMON_ROOT}/IPCCore")
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
file(GLOB IPC_SOURCES "${IPC_ROOT}/source/*.cpp")
file(GLOB IPC_HEADERS "${IPC_ROOT}/include/Ipc/*.h")

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
if (WIN32)
    target_link_libraries(Common PUBLIC advapi32)
endif()

# IPCCore library: IPC protocol ABI, named pipe transport, and shared-memory transport
add_library(IPCCore STATIC
    ${IPC_SOURCES}
    ${IPC_HEADERS}
)

target_include_directories(IPCCore PUBLIC
    "${IPC_ROOT}/include"
)
target_link_libraries(IPCCore PUBLIC Common)
if (WIN32)
    target_link_libraries(IPCCore PRIVATE advapi32)
endif()

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
list(FILTER DEVICE_SOURCES EXCLUDE REGEX "[/\\\\]tests[/\\\\]")
list(FILTER DEVICE_HEADERS EXCLUDE REGEX "[/\\\\]tests[/\\\\]")

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
    EGoTouchService/Solvers/TouchSolver/TouchPipelineConfigKeys.cpp
    # StylusSolver (official ASA pipeline — modules are header-only .hpp)
    EGoTouchService/Solvers/StylusSolver/StylusPipeline.cpp
    EGoTouchService/Solvers/StylusSolver/StylusPipelineConfigKeys.cpp
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

# --- DVR Module ---
set(APP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/Tools/EGoTouchApp")
set(DVR_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/Common/DVRCore")
set(IPCCORE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/Common/IPCCore")
file(GLOB DVR_SOURCES "${DVR_ROOT}/source/*.cpp")
file(GLOB DVR_HEADERS "${DVR_ROOT}/include/*.h")

add_library(DVRCore STATIC
    ${DVR_SOURCES}
    ${DVR_HEADERS}
)

target_include_directories(DVRCore PUBLIC
    "${DVR_ROOT}/include"
    "${IPCCORE_ROOT}/include/Ipc"
)
target_link_libraries(DVRCore PUBLIC Solvers IPCCore Common)

# --- EGoTouchApp (Debug Diagnostics Workbench) ---
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
    "${IPCCORE_ROOT}/include/Ipc"
    "${IMGUI_ROOT}"
    "${IMGUI_ROOT}/backends"
)

# Link D3D11 for the ImGui DX11 backend
target_compile_definitions(EGoTouchApp PRIVATE EGOTOUCH_DIAGNOSTICS)
target_link_libraries(EGoTouchApp PRIVATE DVRCore IPCCore Device Solvers Host Common ImGuiCommon d3d11 d3dcompiler dwmapi setupapi)
if(MSVC)
    target_link_options(EGoTouchApp PRIVATE "/MANIFESTUAC:level='requireAdministrator'")
endif()

# --- EGoTouchService (Production Service Shell) ---
set(SERVICE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/EGoTouchService")

add_library(EGoTouchServiceCore STATIC
    "${SERVICE_ROOT}/source/ServiceConfigCore.cpp"
    "${SERVICE_ROOT}/source/ServiceEntryCore.cpp"
)
target_include_directories(EGoTouchServiceCore PUBLIC
    "${SERVICE_ROOT}/include"
)
target_link_libraries(EGoTouchServiceCore PUBLIC Common advapi32)

add_executable(EGoTouchService
    "${SERVICE_ROOT}/source/ServiceEntry.cpp"
    "${SERVICE_ROOT}/source/ServiceShell.cpp"
    "${SERVICE_ROOT}/source/ServiceHost.cpp"
)
target_include_directories(EGoTouchService PRIVATE
    "${SERVICE_ROOT}/include"
)
target_link_libraries(EGoTouchService PRIVATE EGoTouchServiceCore DVRCore IPCCore Device Host Common advapi32 setupapi hid)

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
    function(ego_add_common_test target source)
        add_executable(${target} ${source})
        target_include_directories(${target} PRIVATE "${COMMON_ROOT}/include")
        target_link_libraries(${target} PRIVATE Common)
        add_test(NAME ${target} COMMAND ${target})
        set_tests_properties(${target} PROPERTIES
            LABELS "Common;unit;device-free"
            TIMEOUT 10
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:${target}>"
        )
    endfunction()

    ego_add_common_test(CommonLoggerTest Common/tests/LoggerTest.cpp)
    ego_add_common_test(CommonGuiLogSinkTest Common/tests/GuiLogSinkTest.cpp)
    ego_add_common_test(CommonFrameLayoutTest Common/tests/FrameLayoutTest.cpp)
    ego_add_common_test(CommonPenButtonConfigTest Common/tests/PenButtonConfigTest.cpp)
    ego_add_common_test(CommonHeaderCompileTest Common/tests/HeaderCompileTest.cpp)
    target_include_directories(CommonHeaderCompileTest PRIVATE "${IPC_ROOT}/include")

    add_executable(HostSystemStateMonitorTest
        EGoTouchService/Host/tests/SystemStateMonitorNamedEventTest.cpp
    )
    target_link_libraries(HostSystemStateMonitorTest PRIVATE Host Common)

    add_subdirectory(EGoTouchService/Solvers/tests)


    if (WIN32 AND TARGET IPCCore)
        add_executable(IPCCoreProtocolAbiTest
            Common/IPCCore/tests/IpcProtocolAbiTest.cpp
        )
        target_link_libraries(IPCCoreProtocolAbiTest PRIVATE IPCCore)

        add_executable(IPCCoreSecurityAttributesTest
            Common/IPCCore/tests/IpcSecurityAttributesTest.cpp
        )
        target_link_libraries(IPCCoreSecurityAttributesTest PRIVATE IPCCore)

        add_executable(IPCCoreSharedFrameBufferTest
            Common/IPCCore/tests/IpcSharedFrameBufferTest.cpp
        )
        target_link_libraries(IPCCoreSharedFrameBufferTest PRIVATE IPCCore)

        add_executable(IPCCorePipeLoopbackTest
            Common/IPCCore/tests/IpcPipeLoopbackTest.cpp
        )
        target_link_libraries(IPCCorePipeLoopbackTest PRIVATE IPCCore)
    endif()


    add_executable(EGoTouchApp_ServiceProxyConfigMergeTest
        Tools/EGoTouchApp/tests/ServiceProxyConfigMergeTest.cpp
        Tools/EGoTouchApp/source/ServiceProxy.cpp
    )
    target_include_directories(EGoTouchApp_ServiceProxyConfigMergeTest PRIVATE
        "${APP_ROOT}/include"
        "${APP_ROOT}/source"
        "${IPCCORE_ROOT}/include/Ipc"
    )
    target_link_libraries(EGoTouchApp_ServiceProxyConfigMergeTest PRIVATE DVRCore Solvers Common)

    function(add_dvrcore_test target source labels timeout)
        add_executable(${target} ${source})
        target_link_libraries(${target} PRIVATE DVRCore Solvers Common)
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test-workdirs/${target}")
        add_test(NAME ${target} COMMAND ${target})
        set_tests_properties(${target} PROPERTIES
            LABELS "${labels}"
            TIMEOUT ${timeout}
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test-workdirs/${target}"
        )
    endfunction()

    add_dvrcore_test(DvrCoreFormatLayoutTest
        Common/DVRCore/tests/DvrCoreFormatLayoutTest.cpp
        "dvrcore;unit;device-free"
        10
    )
    add_dvrcore_test(DvrCoreBinaryRoundTripTest
        Common/DVRCore/tests/DvrCoreBinaryRoundTripTest.cpp
        "dvrcore;roundtrip;device-free"
        15
    )
    add_dvrcore_test(DvrCoreRuntimeConfigRoundTripTest
        Common/DVRCore/tests/DvrCoreRuntimeConfigRoundTripTest.cpp
        "dvrcore;runtime-config;roundtrip;device-free"
        15
    )
    add_dvrcore_test(DvrCoreFixtureCompatibilityTest
        Common/DVRCore/tests/DvrCoreFixtureCompatibilityTest.cpp
        "dvrcore;fixture;compatibility;device-free"
        15
    )
    target_compile_definitions(DvrCoreFixtureCompatibilityTest PRIVATE
        EGO_TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Common/DVRCore/tests/fixtures"
    )
    add_dvrcore_test(DvrCoreCorruptionNegativeTest
        Common/DVRCore/tests/DvrCoreCorruptionNegativeTest.cpp
        "dvrcore;negative;corruption;device-free"
        15
    )

    add_executable(EGoTouchApp_ServiceProxyRuntimeConfigSnapshotTest
        Tools/EGoTouchApp/tests/ServiceProxyRuntimeConfigSnapshotTest.cpp
        Tools/EGoTouchApp/source/ServiceProxy.cpp
    )
    target_include_directories(EGoTouchApp_ServiceProxyRuntimeConfigSnapshotTest PRIVATE
        "${APP_ROOT}/include"
        "${APP_ROOT}/source"
        "${IPCCORE_ROOT}/include/Ipc"
    )
    target_link_libraries(EGoTouchApp_ServiceProxyRuntimeConfigSnapshotTest PRIVATE DVRCore Solvers Common)

    add_executable(EGoTouchApp_ServiceProxyDvrBridgeTest
        Tools/EGoTouchApp/tests/ServiceProxyDvrBridgeTest.cpp
        Tools/EGoTouchApp/source/ServiceProxy.Dvr.cpp
        Tools/EGoTouchApp/source/ServiceProxy.DynamicDebug.cpp
        Tools/EGoTouchApp/source/ServiceProxy.cpp
    )
    target_include_directories(EGoTouchApp_ServiceProxyDvrBridgeTest PRIVATE
        "${APP_ROOT}/include"
        "${APP_ROOT}/source"
        "${IPCCORE_ROOT}/include/Ipc"
    )
    target_link_libraries(EGoTouchApp_ServiceProxyDvrBridgeTest PRIVATE DVRCore Solvers Common)

    add_executable(EGoTouchApp_DiagnosticsWorkbenchLogicTest
        Tools/EGoTouchApp/tests/DiagnosticsWorkbenchLogicTest.cpp
        Tools/EGoTouchApp/source/DiagnosticsWorkbenchLogic.cpp
    )
    target_include_directories(EGoTouchApp_DiagnosticsWorkbenchLogicTest PRIVATE
        "${APP_ROOT}/include"
        "${APP_ROOT}/source"
        "${IPCCORE_ROOT}/include/Ipc"
    )
    target_link_libraries(EGoTouchApp_DiagnosticsWorkbenchLogicTest PRIVATE DVRCore Solvers Common)


    add_subdirectory("${DEVICE_ROOT}/tests")

    add_test(NAME HostSystemStateMonitorTest COMMAND HostSystemStateMonitorTest)
    set_tests_properties(HostSystemStateMonitorTest PROPERTIES
        LABELS "host;integration;windows;device-free;named-event"
        TIMEOUT 15
        RESOURCE_LOCK "host-system-state-named-events"
    )

    if (WIN32 AND TARGET IPCCore)
        add_test(NAME IPCCoreProtocolAbiTest COMMAND IPCCoreProtocolAbiTest)
        set_tests_properties(IPCCoreProtocolAbiTest PROPERTIES
            LABELS "ipccore;unit;device-free"
            TIMEOUT 10
        )

        add_test(NAME IPCCoreSecurityAttributesTest COMMAND IPCCoreSecurityAttributesTest)
        set_tests_properties(IPCCoreSecurityAttributesTest PROPERTIES
            LABELS "ipccore;unit;windows;device-free"
            TIMEOUT 10
        )

        add_test(NAME IPCCoreSharedFrameBufferTest COMMAND IPCCoreSharedFrameBufferTest)
        set_tests_properties(IPCCoreSharedFrameBufferTest PROPERTIES
            LABELS "ipccore;integration;windows;shared-memory;device-free"
            TIMEOUT 20
            RESOURCE_LOCK "ipccore-shared-memory"
        )

        add_test(NAME IPCCorePipeLoopbackTest COMMAND IPCCorePipeLoopbackTest)
        set_tests_properties(IPCCorePipeLoopbackTest PROPERTIES
            LABELS "ipccore;integration;windows;named-pipe;admin;device-free"
            TIMEOUT 20
            RESOURCE_LOCK "ipccore-named-pipe"
        )
    endif()

    set(SERVICE_TEST_ROOT "${SERVICE_ROOT}/tests")

    add_executable(ServiceConfigParserTest
        "${SERVICE_TEST_ROOT}/unit/ServiceConfigParserTest.cpp"
        "${SERVICE_TEST_ROOT}/support/TempConfig.cpp"
    )
    target_include_directories(ServiceConfigParserTest PRIVATE "${SERVICE_TEST_ROOT}/support")
    target_link_libraries(ServiceConfigParserTest PRIVATE EGoTouchServiceCore)

    add_executable(ServiceHostConfigMutationTest
        "${SERVICE_TEST_ROOT}/unit/ServiceHostConfigMutationTest.cpp"
    )
    target_include_directories(ServiceHostConfigMutationTest PRIVATE "${SERVICE_TEST_ROOT}/support")
    target_link_libraries(ServiceHostConfigMutationTest PRIVATE EGoTouchServiceCore)

    add_executable(ServiceEntryArgumentRoutingTest
        "${SERVICE_TEST_ROOT}/unit/ServiceEntryArgumentRoutingTest.cpp"
    )
    target_include_directories(ServiceEntryArgumentRoutingTest PRIVATE "${SERVICE_TEST_ROOT}/support")
    target_link_libraries(ServiceEntryArgumentRoutingTest PRIVATE EGoTouchServiceCore)

    add_executable(ServiceHostLifecycleFakeIntegrationTest
        "${SERVICE_TEST_ROOT}/integration/ServiceHostLifecycleFakeIntegrationTest.cpp"
    )
    target_include_directories(ServiceHostLifecycleFakeIntegrationTest PRIVATE "${SERVICE_TEST_ROOT}/support")
    target_link_libraries(ServiceHostLifecycleFakeIntegrationTest PRIVATE EGoTouchServiceCore)

    add_test(NAME EGoTouchApp.ServiceProxyConfigMergeTest COMMAND EGoTouchApp_ServiceProxyConfigMergeTest)
    set_tests_properties(EGoTouchApp.ServiceProxyConfigMergeTest PROPERTIES
        LABELS "App;EGoTouchApp;ServiceProxy;PureLogic"
        TIMEOUT 30
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    add_test(NAME EGoTouchApp.ServiceProxyRuntimeConfigSnapshotTest COMMAND EGoTouchApp_ServiceProxyRuntimeConfigSnapshotTest)
    set_tests_properties(EGoTouchApp.ServiceProxyRuntimeConfigSnapshotTest PROPERTIES
        LABELS "App;EGoTouchApp;ServiceProxy;PureLogic"
        TIMEOUT 30
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    add_test(NAME EGoTouchApp.ServiceProxyDvrBridgeTest COMMAND EGoTouchApp_ServiceProxyDvrBridgeTest)
    set_tests_properties(EGoTouchApp.ServiceProxyDvrBridgeTest PROPERTIES
        LABELS "App;EGoTouchApp;ServiceProxy;DVRBridge;IntegrationLight"
        TIMEOUT 60
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    add_test(NAME EGoTouchApp.DiagnosticsWorkbenchLogicTest COMMAND EGoTouchApp_DiagnosticsWorkbenchLogicTest)
    set_tests_properties(EGoTouchApp.DiagnosticsWorkbenchLogicTest PROPERTIES
        LABELS "App;EGoTouchApp;DiagnosticsWorkbench;PureLogic"
        TIMEOUT 30
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

    add_test(NAME ServiceConfigParserTest COMMAND ServiceConfigParserTest)
    add_test(NAME ServiceHostConfigMutationTest COMMAND ServiceHostConfigMutationTest)
    add_test(NAME ServiceEntryArgumentRoutingTest COMMAND ServiceEntryArgumentRoutingTest)
    add_test(NAME ServiceHostLifecycleFakeIntegrationTest COMMAND ServiceHostLifecycleFakeIntegrationTest)

    set_tests_properties(ServiceConfigParserTest PROPERTIES
        LABELS "unit;service;windows"
        TIMEOUT 10
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:ServiceConfigParserTest>")
    set_tests_properties(ServiceHostConfigMutationTest PROPERTIES
        LABELS "unit;service;windows"
        TIMEOUT 10
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:ServiceHostConfigMutationTest>")
    set_tests_properties(ServiceEntryArgumentRoutingTest PROPERTIES
        LABELS "unit;service;windows"
        TIMEOUT 10
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:ServiceEntryArgumentRoutingTest>")
    set_tests_properties(ServiceHostLifecycleFakeIntegrationTest PROPERTIES
        LABELS "integration;service;windows"
        TIMEOUT 10
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:ServiceHostLifecycleFakeIntegrationTest>")
endif()


