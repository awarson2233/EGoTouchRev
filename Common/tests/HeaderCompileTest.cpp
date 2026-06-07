#include "ConfigSync.h"
#include "config/ConfigCatalog.h"
#include "config/ConfigKeyId.h"
#include "config/ConfigKeyMap.h"
#include "config/ConfigSchemaSnapshot.h"
#include "FrameLayout.h"
#include "GuiLogSink.h"
#include "IpcPipeClient.h"
#include "IpcPipeServer.h"
#include "IpcProtocol.h"
#include "IpcSecurity.h"
#include "Logger.h"
#include "PenButtonConfig.h"
#include "SharedFrameBuffer.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <type_traits>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void TestCommonHeadersExposeExpectedTypes() {
    Require(Frame::kTotalFrameSize == 5402, "FrameLayout.h should expose frame constants");
    Require(Common::GuiLogSink::kMaxLines == 2000, "GuiLogSink.h should expose kMaxLines");
    Require(ToString(PenButtonMode::OemCustom) != nullptr, "PenButtonConfig.h should expose ToString(PenButtonMode)");
    Require(Common::Logger::Get() == nullptr, "Logger.h should expose Logger::Get without requiring initialization");
    static_assert(std::is_class_v<Config::ConfigCatalog>);
    static_assert(std::is_class_v<Config::ConfigCatalogBuilder>);
    Require(static_cast<uint16_t>(Config::ConfigKeyId::MaxKeyId) == 0x0300, "ConfigKeyId.h should expose MaxKeyId");
}

void TestIpcCompatibilityHeadersCompile() {
    static_assert(std::is_class_v<Ipc::ConfigDirtyFlag>);
    static_assert(std::is_class_v<Ipc::IpcPipeClient>);
    static_assert(std::is_class_v<Ipc::IpcPipeServer>);
    static_assert(std::is_class_v<Ipc::SharedFrameWriter>);
    static_assert(std::is_class_v<Ipc::SharedFrameReader>);
    static_assert(std::is_class_v<Ipc::ScopedSecurityDescriptor>);
    Require(Ipc::kIpcProtocolVersion == 2, "IpcProtocol.h should expose protocol version");
    Require(sizeof(Ipc::IpcRequest{}.param) == 256, "IpcProtocol.h should expose request parameter ABI");
    Require(sizeof(Ipc::IpcResponse{}.data) == 4096, "IpcProtocol.h should expose response payload ABI");
}

} // namespace

int main() {
    try {
        TestCommonHeadersExposeExpectedTypes();
        TestIpcCompatibilityHeadersCompile();
        std::cout << "[TEST] CommonHeaderCompileTest passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] CommonHeaderCompileTest failed: " << ex.what() << '\n';
        return 1;
    }
}
