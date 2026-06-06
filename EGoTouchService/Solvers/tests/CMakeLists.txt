set(EGO_SOLVERS_CONFIG_TEST_TARGET Solvers)
if (TARGET SolversRuntimeConfig)
    set(EGO_SOLVERS_CONFIG_TEST_TARGET SolversRuntimeConfig)
endif()

add_library(SolversTestSupport INTERFACE)
target_include_directories(SolversTestSupport INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/support"
)
target_link_libraries(SolversTestSupport INTERFACE Common)

file(MAKE_DIRECTORY
    "${CMAKE_CURRENT_BINARY_DIR}/unit"
    "${CMAKE_CURRENT_BINARY_DIR}/perf"
)

function(ego_add_solvers_unit name category)
    add_executable(${name} ${ARGN})
    target_compile_definitions(${name} PRIVATE EGOTOUCH_CONFIG_ENABLED=${EGO_SOLVERS_RUNTIME_CONFIG_ENABLED})
    target_link_libraries(${name} PRIVATE Solvers Common SolversTestSupport)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES
        LABELS "solvers;solvers.unit;solvers.${category}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/unit"
        TIMEOUT 30
    )
endfunction()

function(ego_add_solvers_config_unit name category)
    add_executable(${name} ${ARGN})
    target_compile_definitions(${name} PRIVATE EGOTOUCH_CONFIG_ENABLED=1)
    target_link_libraries(${name} PRIVATE ${EGO_SOLVERS_CONFIG_TEST_TARGET} Common SolversTestSupport)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES
        LABELS "solvers;solvers.unit;solvers.${category};solvers.config"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/unit"
        TIMEOUT 30
    )
endfunction()

function(ego_add_solvers_config name category)
    ego_add_solvers_config_unit(${name} ${category} ${ARGN})
endfunction()

function(ego_add_solvers_perf name)
    add_executable(${name} ${ARGN})
    target_compile_definitions(${name} PRIVATE EGOTOUCH_CONFIG_ENABLED=${EGO_SOLVERS_RUNTIME_CONFIG_ENABLED})
    target_link_libraries(${name} PRIVATE DVRCore Solvers Common SolversTestSupport)
    target_include_directories(${name} PRIVATE "${APP_ROOT}/include")
    add_test(
        NAME ${name}
        COMMAND ${name}
            --dataset "${CMAKE_SOURCE_DIR}/Common/DVRCore/tests/fixtures/dvrbin/dataset.dvrbin"
            --frames 120
            --mode linked
    )
    set_tests_properties(${name} PROPERTIES
        LABELS "solvers;solvers.perf;slow;windows.priority"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/perf"
        TIMEOUT 60
    )
endfunction()

ego_add_solvers_unit(SolversUnit_TouchBaselineTracker touch
    unit/touch/TouchBaselineTrackerTest.cpp
)
ego_add_solvers_unit(SolversUnit_TouchTrackerGapRelink touch
    unit/touch/TouchTrackerGapRelinkTest.cpp
)
ego_add_solvers_unit(SolversUnit_TouchStylusSuppress touch
    unit/touch/TouchStylusSuppressTest.cpp
)
ego_add_solvers_unit(SolversUnit_TouchEdgeCompensation touch
    unit/touch/TouchEdgeCompensationTest.cpp
)
# TouchPalmClassifierTest disabled: uses deleted LoadConfig/SaveConfig
# ego_add_solvers_unit(SolversUnit_TouchPalmClassifier touch
#     unit/touch/TouchPalmClassifierTest.cpp
# )
ego_add_solvers_unit(SolversUnit_TouchCloseSplit touch
    unit/touch/TouchCloseSplitTest.cpp
)

# StylusLinearFilterTest/StylusHpp3PostPressureTest disabled: use deleted LoadConfig/SaveConfig
# ego_add_solvers_unit(SolversUnit_StylusLinearFilter stylus
#     unit/stylus/StylusLinearFilterTest.cpp
# )
# ego_add_solvers_unit(SolversUnit_StylusHpp3PostPressure stylus
#     unit/stylus/StylusHpp3PostPressureTest.cpp
# )
ego_add_solvers_unit(SolversUnit_StylusHpp2Pipeline stylus
    unit/stylus/StylusHpp2PipelineTest.cpp
)
ego_add_solvers_config_unit(SolversUnit_StylusPipelineConfigRoundTrip stylus
    unit/stylus/StylusPipelineConfigRoundTripTest.cpp
)

# RawdataBenchmark disabled: uses deleted LoadConfig/SaveConfig + old INI config
# ego_add_solvers_perf(SolversPerf_RawdataBenchmark
#     perf/RawdataBenchmark.cpp
# )
