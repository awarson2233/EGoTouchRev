#include "SharedFrameBuffer.h"
#include <iostream>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

int main() {
    std::cout << "[TEST] Starting SharedFrameBuffer test...\n";

    Ipc::SharedFrameWriter writer;

    // Test successful creation
    const wchar_t* valid_name = L"Local\\EGoTouchSharedFrameTest";
    if (!writer.Create(valid_name)) {
        std::cerr << "[TEST] Failed: Could not create SharedFrameWriter with valid name." << std::endl;
        return 1;
    }

    if (!writer.IsOpen()) {
        std::cerr << "[TEST] Failed: Writer should report IsOpen() == true after successful creation." << std::endl;
        return 2;
    }

    // Since we successfully created it, close it.
    writer.Close();

    if (writer.IsOpen()) {
        std::cerr << "[TEST] Failed: Writer should report IsOpen() == false after Close()." << std::endl;
        return 3;
    }

    // Try to create with an invalid name (e.g. empty string or nullptr to trigger failure if possible, or using an invalid character in name)
    // Note: For CreateFileMappingW, if lpName is an invalid string, it might fail. Let's pass a known bad character or something that causes ERROR_INVALID_NAME.
    // L"\\" is an invalid name because a name cannot contain backslashes except for the "Global\\" or "Local\\" prefixes.
    const wchar_t* invalid_name = L"Invalid\\Name/With*Bad?Chars";
    if (writer.Create(invalid_name)) {
        std::cerr << "[TEST] Failed: Create should fail with an invalid name." << std::endl;
        writer.Close();
        return 4;
    }

    if (writer.IsOpen()) {
        std::cerr << "[TEST] Failed: Writer should not be open after a failed Create()." << std::endl;
        return 5;
    }

    std::cout << "[TEST] SharedFrameBuffer test passed.\n";
    return 0;
}
