#if __has_include("Ipc/IpcSecurity.h")
#include "Ipc/IpcSecurity.h"
#else
#include "IpcSecurity.h"
#endif

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <utility>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "[FAIL] " << message << "\n";
        std::exit(1);
    }
}

} // namespace

int main() {
    using namespace Ipc;

    SECURITY_ATTRIBUTES sa{};
    ScopedSecurityDescriptor sd;
    Require(BuildSecurityAttributes(kAdminOnlyObjectSddl, sa, sd), "admin-only SDDL converts successfully");
    Require(sa.nLength == sizeof(SECURITY_ATTRIBUTES), "SECURITY_ATTRIBUTES length is initialized");
    Require(sa.lpSecurityDescriptor == sd.Get(), "SECURITY_ATTRIBUTES references scoped descriptor");
    Require(sa.lpSecurityDescriptor != nullptr, "security descriptor is non-null");
    Require(sa.bInheritHandle == FALSE, "handles are not inheritable by default");

    PSECURITY_DESCRIPTOR firstDescriptor = sd.Get();
    SECURITY_ATTRIBUTES sa2{};
    ScopedSecurityDescriptor sd2;
    Require(BuildAdminOnlySecurityAttributes(sa2, sd2), "BuildAdminOnlySecurityAttributes converts successfully");
    Require(sa2.lpSecurityDescriptor == sd2.Get(), "admin helper wires descriptor into SECURITY_ATTRIBUTES");
    Require(sa2.lpSecurityDescriptor != nullptr, "admin helper descriptor is non-null");
    Require(sd.Get() == firstDescriptor, "separate build does not disturb existing scoped descriptor");

    SECURITY_ATTRIBUTES invalidSa{};
    ScopedSecurityDescriptor invalidSd;
    invalidSa.nLength = 1234;
    invalidSa.lpSecurityDescriptor = reinterpret_cast<PSECURITY_DESCRIPTOR>(static_cast<uintptr_t>(0x1));
    invalidSa.bInheritHandle = TRUE;
    Require(!BuildSecurityAttributes(L"not a valid sddl", invalidSa, invalidSd), "invalid SDDL fails conversion");
    Require(invalidSd.Get() == nullptr, "invalid SDDL leaves scoped descriptor empty");
    Require(invalidSa.nLength == 1234, "invalid SDDL does not rewrite SECURITY_ATTRIBUTES length");
    Require(invalidSa.lpSecurityDescriptor == reinterpret_cast<PSECURITY_DESCRIPTOR>(static_cast<uintptr_t>(0x1)), "invalid SDDL does not rewrite SECURITY_ATTRIBUTES descriptor");
    Require(invalidSa.bInheritHandle == TRUE, "invalid SDDL does not rewrite inherit flag");

    ScopedSecurityDescriptor moved{std::move(sd2)};
    Require(moved.Get() == sa2.lpSecurityDescriptor, "ScopedSecurityDescriptor move constructor transfers ownership");
    Require(sd2.Get() == nullptr, "ScopedSecurityDescriptor move constructor clears source");

    ScopedSecurityDescriptor assigned;
    assigned = std::move(moved);
    Require(assigned.Get() == sa2.lpSecurityDescriptor, "ScopedSecurityDescriptor move assignment transfers ownership");
    Require(moved.Get() == nullptr, "ScopedSecurityDescriptor move assignment clears source");

    assigned.Reset();
    Require(assigned.Get() == nullptr, "ScopedSecurityDescriptor Reset clears descriptor");
    sd.Reset();
    Require(sd.Get() == nullptr, "original ScopedSecurityDescriptor Reset clears descriptor");

    std::cout << "[PASS] IpcSecurityAttributesTest\n";
    return 0;
}
