cmake_minimum_required(VERSION 3.20)
project(EGoTouchRev VERSION 1.4.0 LANGUAGES C CXX) # x-release-please-version

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
        file(RELATIVE_PATH clangd_compilation_database "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_BINARY_DIR}")
        string(REPLACE "\\" "/" clangd_compilation_database "${clangd_compilation_database}")
        string(APPEND CLANGD_CONTENT "  CompilationDatabase: ${clangd_compilation_database}\n")
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

# --- Global Optimization & Debug Options ---
option(EGO_GENERATE_PDB "Generate PDB debug symbols in Release build" ON)

if(MSVC OR (WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC"))
    # 区分 MSVC 兼容的编译驱动（cl.exe / clang-cl.exe）与常规 GNU 风格的 clang 驱动
    if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC" OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
        if(EGO_GENERATE_PDB)
            add_compile_options($<$<CONFIG:Release>:/Zi>)
            add_link_options($<$<CONFIG:Release>:/DEBUG:FULL>)
        endif()
        add_link_options($<$<CONFIG:Release>:/OPT:REF> $<$<CONFIG:Release>:/OPT:ICF>)
    else()
        # 常规 clang.exe/clang++.exe 驱动模式且目标为 MSVC 环境时
        if(EGO_GENERATE_PDB)
            add_compile_options($<$<CONFIG:Release>:-g>)
            add_link_options($<$<CONFIG:Release>:-Wl,-debug>)
        endif()
        add_link_options($<$<CONFIG:Release>:-Wl,-opt:ref> $<$<CONFIG:Release>:-Wl,-opt:icf>)
    endif()
endif()

# 启用全局 LTO (Link-Time Optimization)
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT error)
if(ipo_supported)
    message(STATUS "Link-Time Optimization (LTO/IPO) is supported. Enabling for Release configuration...")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
else()
    message(STATUS "Link-Time Optimization (LTO/IPO) is not supported: ${error}")
endif()

# --- SIMD options ---
# This project targets a specific platform; enable NEON by default.
option(HIMAX_ENABLE_NEON "Enable ARM NEON intrinsics when available" ON)

# --- Build Controls ---
option(EGO_BUILD_TOOLS "Build the diagnostic tools (BtMcuTestTool, TsaRawReplay)" ON)

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



# Common library: Pure utilities (Logger, SharedFrameBuffer) - NO ImGui
add_library(Common STATIC
    ${COMMON_SOURCES}
    ${COMMON_HEADERS}
)

target_sources(Common PRIVATE
    "${COMMON_ROOT}/source/config/ConfigStore.cpp"
    "${COMMON_ROOT}/source/config/ConfigBinder.cpp"
    "${COMMON_ROOT}/source/config/ConfigSchemaSnapshot.cpp"
    "${COMMON_ROOT}/source/config/SchemaValidator.cpp"
    "${COMMON_ROOT}/source/config/ConfigTlv.cpp"
    "${COMMON_ROOT}/source/config/ConfigKeyMap.cpp"
    "${COMMON_ROOT}/source/config/ConfigCatalog.cpp"
)

target_include_directories(Common PUBLIC
    "${COMMON_ROOT}/include"
)
# Common links Solvers for SharedFrameBuffer (EngineTypes.h)
# Forward-declare include path; Solvers target linked below after Solvers is defined
target_compile_definitions(Common PRIVATE
    $<$<CONFIG:Release>:EGOTOUCH_LOGGER_SYNC_FILE_ONLY=1>
)

# 启用预编译头 (PCH) 以减少编译时的重复解析开销
target_precompile_headers(Common PRIVATE
    <vector>
    <string>
    <memory>
    <unordered_map>
    <algorithm>
)
if (WIN32)
    target_precompile_headers(Common PRIVATE <Windows.h>)
endif()

if (WIN32)
    target_link_libraries(Common PUBLIC advapi32)
endif()

# IPCCore library: IPC protocol ABI, named pipe transport, and shared-memory transport
# 管道与共享内存的实现目前只有 Windows 版；其他平台只导出协议头文件（DVRCore 用到其中的类型）
if (WIN32)
    add_library(IPCCore STATIC
        ${IPC_SOURCES}
        ${IPC_HEADERS}
    )
    target_include_directories(IPCCore PUBLIC
        "${IPC_ROOT}/include"
    )
    target_link_libraries(IPCCore PUBLIC Common)
    target_link_libraries(IPCCore PRIVATE advapi32)
else()
    add_library(IPCCore INTERFACE)
    target_include_directories(IPCCore INTERFACE
        "${IPC_ROOT}/include"
    )
    target_link_libraries(IPCCore INTERFACE Common)
endif()

# ImGuiCommon library: UI framework (only for diagnostic tools)
if (EGO_BUILD_TOOLS AND WIN32)
    add_library(ImGuiCommon STATIC
        ${IMGUI_SOURCES}
    )

    target_include_directories(ImGuiCommon PUBLIC
        "${IMGUI_ROOT}"
    )
endif()

set(DEVICE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/EGoTouchService/Device")

# --- Port Module (Platform Ports) ---
# 平台相关的部分抽成接口（EGoTouchService/Port/*.h），每个平台在 Port/<平台>/ 下给出实现，这里按平台挑一份
set(PORT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/EGoTouchService/Port")
if (WIN32)
    set(PORT_PLATFORM_DIR "${PORT_ROOT}/win32")
else()
    set(PORT_PLATFORM_DIR "${PORT_ROOT}/linux")
endif()
file(GLOB PORT_SOURCES "${PORT_PLATFORM_DIR}/*.cpp")
file(GLOB PORT_HEADERS "${PORT_ROOT}/*.h")

add_library(Port STATIC
    ${PORT_SOURCES}
    ${PORT_HEADERS}
)
target_include_directories(Port PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/EGoTouchService"
    "${DEVICE_ROOT}"
)
if (NOT WIN32)
    # himax-thp 驱动的用户态 ABI（himax_thp.h），内核模块与 TouchBusLinux 共用
    target_include_directories(Port PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Kernel/himax-thp")
endif()

# --- HimaxCore：芯片协议与生命周期（himax/Himax*.cpp），只经 Port 访问总线，各平台共用 ---
file(GLOB HIMAX_CORE_SOURCES "${DEVICE_ROOT}/himax/Himax*.cpp")
add_library(HimaxCore STATIC ${HIMAX_CORE_SOURCES})
target_include_directories(HimaxCore PUBLIC "${DEVICE_ROOT}")
target_link_libraries(HimaxCore PUBLIC Port Common)

# --- Device Module (Hardware Abstraction) ---
# Device / Host / 服务外壳与依赖它们的工具目前只有 Windows 实现；其他平台只构建算法层与 TsaRawReplay
if (WIN32)
file(GLOB_RECURSE DEVICE_SOURCES "${DEVICE_ROOT}/*.cpp")
file(GLOB_RECURSE DEVICE_HEADERS "${DEVICE_ROOT}/*.h")
list(FILTER DEVICE_SOURCES EXCLUDE REGEX "[/\\\\]tests[/\\\\]")
list(FILTER DEVICE_HEADERS EXCLUDE REGEX "[/\\\\]tests[/\\\\]")
list(FILTER DEVICE_SOURCES EXCLUDE REGEX "[/\\\\]himax[/\\\\]Himax[^/\\\\]*\\.cpp$")

add_library(Device STATIC
    ${DEVICE_SOURCES}
    ${DEVICE_HEADERS}
)

target_compile_definitions(Device PUBLIC
    $<$<BOOL:${HIMAX_ENABLE_NEON}>:HIMAX_ENABLE_NEON=1>
    $<$<NOT:$<BOOL:${HIMAX_ENABLE_NEON}>>:HIMAX_ENABLE_NEON=0>
    EGOTOUCH_SERVICE_ENABLE_IPC=$<IF:$<CONFIG:Debug>,1,0>
)


target_include_directories(Device PUBLIC
    "${DEVICE_ROOT}"
)
target_link_libraries(Device PUBLIC HimaxCore Common Host Solvers setupapi hid)
endif()

# --- Solvers Module (Touch Algorithm) ---
set(SOLVERS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/EGoTouchService/Solvers")
set(SOLVERS_SOURCES
    # TSACore 宿主侧：AFE 适配层与外观类（初始化、ctrl flags、闭环与输出映射）
    EGoTouchService/Solvers/TouchSolver/TsaCore/HimaxAfeAdapter.cpp
    EGoTouchService/Solvers/TouchSolver/TsaCore/TsaCoreSolver.cpp
    # StylusSolver (official ASA pipeline — modules are header-only .hpp)
    EGoTouchService/Solvers/StylusSolver/StylusPipeline.cpp
)

function(ego_configure_solvers_target target)
    target_compile_definitions(${target} PUBLIC
        $<$<BOOL:${HIMAX_ENABLE_NEON}>:HIMAX_ENABLE_NEON=1>
        $<$<NOT:$<BOOL:${HIMAX_ENABLE_NEON}>>:HIMAX_ENABLE_NEON=0>
    )

    target_include_directories(${target} PUBLIC
        "${SOLVERS_ROOT}"
        "${SOLVERS_ROOT}/TouchSolver"
        "${SOLVERS_ROOT}/StylusSolver"
        "${SOLVERS_ROOT}/StylusSolver/shared"
        "${SOLVERS_ROOT}/StylusSolver/hpp2"
        "${SOLVERS_ROOT}/StylusSolver/hpp3"
    )
    target_link_libraries(${target} PUBLIC Common)
endfunction()

add_library(Solvers STATIC ${SOLVERS_SOURCES})
ego_configure_solvers_target(Solvers)

# TSACore 手指管线：由 Ghidra 反编译的 TSACore.dll 改写成的 C++（TsaCore/src/），对宿主的接口是
# TsaCore/TsaCore.h。全局状态在 g_bss 里（按原 DLL 的 .bss 排布），进程内单例。raw/ 只留分析文档与符号元数据。
# 原代码依赖类型双关与有符号回绕，必须关掉严格别名并启用 -fwrapv；改写后的代码沿用同样的语义。
# -ffp-contract=off：原 DLL 是 x64，没有乘加融合；ARM64 上默认融合会让浮点中间量差出末位。
# -ftrivial-auto-var-init=zero：原代码有把未赋值的局部量（结构体尾部的填充）拷进状态的地方，
# 不清零的话状态随栈上的残留值变，重构前后无从比对。
set(TSACORE_RAW_ROOT "${SOLVERS_ROOT}/TouchSolver/raw")
set(TSACORE_SRC_ROOT "${SOLVERS_ROOT}/TouchSolver/TsaCore/src")
file(GLOB_RECURSE TSACORE_CPP_SOURCES CONFIGURE_DEPENDS "${TSACORE_SRC_ROOT}/*.cpp")
add_library(TsaCore STATIC ${TSACORE_CPP_SOURCES})
target_include_directories(TsaCore PRIVATE "${TSACORE_SRC_ROOT}")
target_compile_options(TsaCore PRIVATE
    -fno-strict-aliasing
    -fwrapv
    -ffp-contract=off
    -ftrivial-auto-var-init=zero
)
# 比对与覆盖率用：给算法库单独追加编译选项（如 -O2、-fprofile-instr-generate -fcoverage-mapping），
# 回放工具其余部分保持 Debug
set(EGO_TSACORE_EXTRA_FLAGS "" CACHE STRING "Extra compile options for the TSACore algorithm library")
if (EGO_TSACORE_EXTRA_FLAGS)
    separate_arguments(_ego_tsacore_extra NATIVE_COMMAND "${EGO_TSACORE_EXTRA_FLAGS}")
    target_compile_options(TsaCore PRIVATE ${_ego_tsacore_extra})
endif()
# 分段计时（Common/Profile.h）：主流程每个步骤记累计耗时，回放 --profile 打印。默认关，打开后有计时开销
option(EGO_TSACORE_PROFILE "Time each TSACore pipeline step (TsaRawReplay --profile)" OFF)
if (EGO_TSACORE_PROFILE)
    target_compile_definitions(TsaCore PRIVATE TSACORE_PROFILE)
endif()
# TsaCoreSolver 直接调用 TsaCore 的导出
target_link_libraries(Solvers PUBLIC TsaCore)
# Let Common see Solvers headers for SharedFrameBuffer
target_include_directories(Common PUBLIC "${SOLVERS_ROOT}")

# --- Host Module (System Integration) ---
if (WIN32)
    set(HOST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/EGoTouchService/Host")
    file(GLOB HOST_SOURCES "${HOST_ROOT}/*.cpp")
    file(GLOB HOST_HEADERS "${HOST_ROOT}/*.h")

    add_library(Host STATIC
        ${HOST_SOURCES}
        ${HOST_HEADERS}
    )
    target_include_directories(Host PUBLIC "${HOST_ROOT}")
    target_link_libraries(Host PUBLIC Common advapi32)
endif()

# --- DVR Module ---
set(DVR_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/Common/DVRCore")
set(IPCCORE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/Common/IPCCore")
file(GLOB DVR_SOURCES "${DVR_ROOT}/source/*.cpp")
file(GLOB DVR_HEADERS "${DVR_ROOT}/include/*.h")

function(ego_configure_dvrcore_target target solvers_target)
    target_include_directories(${target} PUBLIC
        "${DVR_ROOT}/include"
        "${IPCCORE_ROOT}/include/Ipc"
    )
    target_link_libraries(${target} PUBLIC ${solvers_target} IPCCore Common)
endfunction()

add_library(DVRCore STATIC
    ${DVR_SOURCES}
    ${DVR_HEADERS}
)
ego_configure_dvrcore_target(DVRCore Solvers)

function(ego_copy_config_to_output target)
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config/default.yaml")
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${target}>/config"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/config/default.yaml"
                "$<TARGET_FILE_DIR:${target}>/config/default.yaml"
            COMMENT "Copy config/default.yaml for ${target}"
        )
    endif()
endfunction()

# --- EGoTouchService (Production Service Shell) ---
if (WIN32)
set(SERVICE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/EGoTouchService")

add_library(EGoTouchServiceCore STATIC
    "${SERVICE_ROOT}/source/ConfigRuntime.cpp"
    "${SERVICE_ROOT}/source/ServiceConfigCore.cpp"
    "${SERVICE_ROOT}/source/ServiceEntryCore.cpp"
)
target_include_directories(EGoTouchServiceCore PUBLIC
    "${SERVICE_ROOT}/include"
)
target_link_libraries(EGoTouchServiceCore PUBLIC Common Solvers $<$<CONFIG:Debug>:IPCCore> advapi32)
target_compile_definitions(EGoTouchServiceCore PUBLIC
    EGOTOUCH_SERVICE_ENABLE_IPC=$<IF:$<CONFIG:Debug>,1,0>
)


add_executable(EGoTouchService
    "${SERVICE_ROOT}/source/ServiceEntry.cpp"
    "${SERVICE_ROOT}/source/ServiceShell.cpp"
    "${SERVICE_ROOT}/source/ServiceHost.cpp"
)
target_include_directories(EGoTouchService PRIVATE
    "${SERVICE_ROOT}/include"
)
target_link_libraries(EGoTouchService PRIVATE EGoTouchServiceCore $<$<CONFIG:Debug>:IPCCore> Device Host Common advapi32 setupapi hid powrprof user32)
target_compile_definitions(EGoTouchService PRIVATE
    EGOTOUCH_SERVICE_ENABLE_IPC=$<IF:$<CONFIG:Debug>,1,0>
)

ego_copy_config_to_output(EGoTouchService)

install(TARGETS EGoTouchService
    RUNTIME DESTINATION "."
    COMPONENT Runtime
)
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config/default.yaml")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/config/default.yaml"
        DESTINATION "config"
        COMPONENT Runtime
    )
endif()

# EGoTouchService uses wmain (console subsystem) for service/console dual mode
if(MSVC)
    set_target_properties(EGoTouchService PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
elseif(WIN32)
    target_link_options(EGoTouchService PRIVATE -municode)
endif()
endif()

# --- BtMcuTestTool (ImGui BT Protocol Validator) ---
if (EGO_BUILD_TOOLS AND WIN32)
    set(IMGUI_BACKEND_SOURCES
        "${IMGUI_ROOT}/backends/imgui_impl_win32.cpp"
        "${IMGUI_ROOT}/backends/imgui_impl_dx11.cpp"
    )
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
endif()

# --- TsaRawReplay：把 .dvrbin 录制灌进 TsaCore，逐帧打印触点；Compare-Reference.ps1 用它比对与计时 ---
if (EGO_BUILD_TOOLS)
    add_executable(TsaRawReplay Tools/TsaRawReplay/main.cpp Tools/TsaRawReplay/StateTrace.cpp)
    target_link_libraries(TsaRawReplay PRIVATE DVRCore Solvers Common)
    target_compile_definitions(TsaRawReplay PRIVATE TSA_RAW_ROOT="${TSACORE_RAW_ROOT}")
    if (EGO_TSACORE_EXTRA_FLAGS MATCHES "fprofile-instr-generate")
        target_link_options(TsaRawReplay PRIVATE -fprofile-instr-generate)
    endif()
endif()

# --- EGoTouchRecorder：给测试者的只读录制工具（触摸帧 + BT MCU → .dvrbin）。只在 Debug 下构建，
# DeviceRuntime 的逐帧回调只在 Debug 里编入 ---
if (EGO_BUILD_TOOLS AND WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_executable(EGoTouchRecorder Tools/EGoTouchRecorder/main.cpp)
    target_link_libraries(EGoTouchRecorder PRIVATE Device DVRCore Solvers Host Common shell32 ole32 advapi32)
    if (WIN32)
        target_link_options(EGoTouchRecorder PRIVATE "LINKER:/MANIFESTUAC:level='requireAdministrator'")
    endif()
endif()

# --- ChipHidProbe：运行时把 master 接口模式字改成 1，读片上 HID 报告（实验用）---
if (EGO_BUILD_TOOLS AND WIN32)
    add_executable(ChipHidProbe Tools/ChipHidProbe/main.cpp)
    target_link_libraries(ChipHidProbe PRIVATE Device Common advapi32 winmm)
    if (WIN32)
        target_link_options(ChipHidProbe PRIVATE "LINKER:/MANIFESTUAC:level='requireAdministrator'")
    endif()
endif()

# --- ChipHidProbeB：方案 B 验证，if_mode 保持 0，运行时热补丁 P1–P5，解析帧尾 HID 报告 ---
# 不要求管理员（asInvoker）：服务由外部先停好，SPB 设备直接开，便于非提权连跑。
if (EGO_BUILD_TOOLS AND WIN32)
    add_executable(ChipHidProbeB Tools/ChipHidProbeB/main.cpp)
    target_link_libraries(ChipHidProbeB PRIVATE Device Common advapi32 winmm)
endif()

# --- HimaxThpProbe：验证两个平台的总线端口（Chip::Init、取帧、按 Chip 的规则校验两路帧）---
if (EGO_BUILD_TOOLS)
    add_executable(HimaxThpProbe Tools/HimaxThpProbe/main.cpp)
    target_link_libraries(HimaxThpProbe PRIVATE HimaxCore Common)
endif()

# --- egotouchd：Linux 触摸守护进程（himax-thp 取帧 → TSACore → uhid 触摸屏）---
if (NOT WIN32)
    add_executable(egotouchd EGoTouchService/linux/egotouchd.cpp)
    target_include_directories(egotouchd PRIVATE "${DEVICE_ROOT}")
    target_link_libraries(egotouchd PRIVATE HimaxCore Solvers Port Common)
    install(TARGETS egotouchd RUNTIME DESTINATION bin COMPONENT Runtime)
endif()
