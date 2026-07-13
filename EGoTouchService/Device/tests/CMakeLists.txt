function(ego_add_device_test target test_name labels timeout_seconds)
    add_executable(${target} ${ARGN})
    target_include_directories(${target} PRIVATE
        "${DEVICE_ROOT}"
        "${DEVICE_ROOT}/tests/support"
    )
    add_test(NAME ${test_name} COMMAND ${target})
    set_tests_properties(${test_name} PROPERTIES
        LABELS "${labels}"
        TIMEOUT ${timeout_seconds}
    )
endfunction()

ego_add_device_test(DevicePenUsbProtocolPacketTest Device.PenUsb.ProtocolPacket
    "Device;Device.Unit;Device.PenUsb;NoHardware" 10
    "${DEVICE_ROOT}/tests/btmcu/PenUsbProtocolPacketTest.cpp")

ego_add_device_test(DevicePenUsbInitSessionTest Device.PenUsb.InitSession
    "Device;Device.Unit;Device.PenUsb;NoHardware" 10
    "${DEVICE_ROOT}/tests/btmcu/PenUsbInitSessionTest.cpp")

ego_add_device_test(DevicePenFactoryStatusFlagsTest Device.PenUsb.FactoryStatusFlags
    "Device;Device.Unit;Device.PenUsb;NoHardware" 10
    "${DEVICE_ROOT}/tests/btmcu/PenFactoryStatusFlagsTest.cpp")

ego_add_device_test(DevicePenModuleModelIdTest Device.PenUsb.PenModuleModelId
    "Device;Device.Unit;Device.PenUsb;NoHardware" 10
    "${DEVICE_ROOT}/tests/btmcu/PenModuleModelIdTest.cpp")

ego_add_device_test(DevicePenPressurePacketParserTest Device.PenPressure.PacketParser
    "Device;Device.Unit;Device.PenPressure;NoHardware" 10
    "${DEVICE_ROOT}/tests/btmcu/PenPressurePacketParserTest.cpp")

ego_add_device_test(DevicePenUsbTransportWin32LifecycleTest Device.PenUsb.TransportWin32Lifecycle
    "Device;Device.Unit;Device.PenUsb;Device.Win32;Timing;NoHardware" 10
    "${DEVICE_ROOT}/tests/btmcu/PenUsbTransportWin32LifecycleTest.cpp")
target_link_libraries(DevicePenUsbTransportWin32LifecycleTest PRIVATE Device Common)

ego_add_device_test(DeviceVhfReporterStylusPacketTest Device.Vhf.StylusPacket
    "Device;Device.Unit;Device.Vhf;NoHardware" 10
    "${DEVICE_ROOT}/tests/vhf/VhfReporterStylusPacketTest.cpp")
target_link_libraries(DeviceVhfReporterStylusPacketTest PRIVATE Solvers Common)

ego_add_device_test(DeviceVhfReporterTouchPacketTest Device.Vhf.TouchPacket
    "Device;Device.Unit;Device.Vhf;NoHardware" 10
    "${DEVICE_ROOT}/tests/vhf/VhfReporterTouchPacketTest.cpp")
target_link_libraries(DeviceVhfReporterTouchPacketTest PRIVATE Solvers Common)

ego_add_device_test(DeviceRuntimeSystemPowerPolicyTest Device.Runtime.SystemPowerPolicy
    "Device;Device.Runtime;Device.Win32;Timing;NoHardware" 20
    "${DEVICE_ROOT}/tests/runtime/DeviceRuntimeSystemPowerPolicyTest.cpp")
target_link_libraries(DeviceRuntimeSystemPowerPolicyTest PRIVATE Device Common)
set_tests_properties(Device.Runtime.SystemPowerPolicy PROPERTIES
    RESOURCE_LOCK DeviceRuntimeTiming
)

ego_add_device_test(DeviceRuntimePenStateTest Device.Runtime.PenState
    "Device;Device.Runtime;Device.PenUsb;NoHardware" 10
    "${DEVICE_ROOT}/tests/runtime/DeviceRuntimePenStateTest.cpp")
target_link_libraries(DeviceRuntimePenStateTest PRIVATE Device Common)
