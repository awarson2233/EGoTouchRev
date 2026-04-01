cmake_minimum_required(VERSION 3.20)
project(EGoTouchRev VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Platform compatibility ---
# Prevent Windows.h from defining min/max macros that conflict with std::min/std::max
# Suppress MSVC CRT deprecation warnings (e.g. localtime -> localtime_s)
if (WIN32)
    add_compile_definitions(NOMINMAX _CRT_SECURE_NO_WARNINGS)
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

add_library(Common STATIC
    ${COMMON_SOURCES}
    ${COMMON_HEADERS}
    ${IMGUI_SOURCES}
)

target_include_directories(Common PUBLIC
    "${COMMON_ROOT}/include"
    "${IMGUI_ROOT}"
)
# Common links Engine for SharedFrameBuffer (EngineTypes.h)
# Forward-declare include path; Engine target linked below after Engine is defined
target_link_libraries(Common PUBLIC spdlog::spdlog)

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
target_link_libraries(Device PUBLIC Common Host Engine setupapi hid)

# --- Engine Module (Touch Algorithm) ---
set(ENGINE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/EGoTouchService/Engine")

add_library(Engine STATIC
    # Root (Infrastructure)
    EGoTouchService/Engine/FramePipeline.cpp
    # Preprocessing
    EGoTouchService/Engine/Preprocessing/MasterFrameParser.cpp
    EGoTouchService/Engine/Preprocessing/BaselineSubtraction.cpp
    EGoTouchService/Engine/Preprocessing/CMFProcessor.cpp
    EGoTouchService/Engine/Preprocessing/GridIIRProcessor.cpp
    EGoTouchService/Engine/Preprocessing/SignalConditioningFilter.cpp
    EGoTouchService/Engine/Preprocessing/GaussianFilter.cpp
    # TouchSolver
    EGoTouchService/Engine/TouchSolver/PeakDetector.cpp
    EGoTouchService/Engine/TouchSolver/ZoneExpander.cpp
    EGoTouchService/Engine/TouchSolver/EdgeCompensation.cpp
    EGoTouchService/Engine/TouchSolver/TouchSize.cpp
    EGoTouchService/Engine/TouchSolver/EdgeRejection.cpp
    EGoTouchService/Engine/TouchSolver/CoordinateFilter.cpp
    EGoTouchService/Engine/TouchSolver/FeatureExtractor.cpp
    # Reporting
    EGoTouchService/Engine/Reporting/TouchTracker.cpp
    EGoTouchService/Engine/Reporting/TouchGestureStateMachine.cpp
    # StylusSolver (official ASA pipeline)
    EGoTouchService/Engine/StylusSolver/GridPeakDetector.cpp
    EGoTouchService/Engine/StylusSolver/CoordinateSolver.cpp
    EGoTouchService/Engine/StylusSolver/CoorPostProcessor.cpp
    EGoTouchService/Engine/StylusSolver/StylusPipeline.cpp
)

target_compile_definitions(Engine PUBLIC
    $<$<BOOL:${HIMAX_ENABLE_NEON}>:HIMAX_ENABLE_NEON=1>
    $<$<NOT:$<BOOL:${HIMAX_ENABLE_NEON}>>:HIMAX_ENABLE_NEON=0>
)

target_include_directories(Engine PUBLIC
    "${ENGINE_ROOT}"
    "${ENGINE_ROOT}/Preprocessing"
    "${ENGINE_ROOT}/TouchSolver"
    "${ENGINE_ROOT}/Reporting"
    "${ENGINE_ROOT}/StylusSolver"
)
target_link_libraries(Engine PUBLIC Common)
# Let Common see Engine headers for SharedFrameBuffer
target_include_directories(Common PUBLIC "${ENGINE_ROOT}")

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
target_link_libraries(EGoTouchApp PRIVATE Device Engine Host Common d3d11 d3dcompiler dwmapi setupapi)

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
target_link_libraries(BtMcuTestTool PRIVATE Device Common d3d11 d3dcompiler dwmapi setupapi)

option(EGO_BUILD_TESTS "Build local test executables" ON)
if (EGO_BUILD_TESTS)
    add_executable(HostSystemStateMonitorTest
        Tools/tests/SystemStateMonitorNamedEventTest.cpp
    )
    target_include_directories(HostSystemStateMonitorTest PRIVATE "${HOST_ROOT}/include")
    target_link_libraries(HostSystemStateMonitorTest PRIVATE Host Common)

    add_executable(EngineRawdataBenchmarkTest
        Tools/tests/RawdataBenchmarkTest.cpp
    )
    target_link_libraries(EngineRawdataBenchmarkTest PRIVATE Engine Common)
endif()


