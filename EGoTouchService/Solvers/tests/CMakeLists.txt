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
    target_link_libraries(${name} PRIVATE Solvers Common SolversTestSupport)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES
        LABELS "solvers;solvers.unit;solvers.${category}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/unit"
        TIMEOUT 30
    )
endfunction()

function(ego_add_touch_unit name)
    add_executable(${name} ${ARGN})
    target_include_directories(${name} PRIVATE
        "${CMAKE_SOURCE_DIR}/EGoTouchService/Solvers"
        "${CMAKE_SOURCE_DIR}/EGoTouchService/Solvers/TouchSolver"
    )
    target_link_libraries(${name} PRIVATE Common SolversTestSupport)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES
        LABELS "solvers;solvers.unit;solvers.touch"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/unit"
        TIMEOUT 30
    )
endfunction()

function(ego_add_solvers_config_unit name category)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE Solvers Common SolversTestSupport)
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

ego_add_solvers_unit(SolversUnit_TouchStylusSuppress touch
    unit/touch/TouchStylusSuppressTest.cpp
)
ego_add_solvers_unit(SolversUnit_TouchPalmBoxSuppress touch
    unit/touch/TouchPalmBoxSuppressTest.cpp
)

ego_add_solvers_unit(SolversUnit_StylusHpp2Pipeline stylus
    unit/stylus/StylusHpp2PipelineTest.cpp
)
ego_add_solvers_config_unit(SolversUnit_StylusPipelineConfigRoundTrip stylus
    unit/stylus/StylusPipelineConfigRoundTripTest.cpp
)




