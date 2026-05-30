#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace Ipc {

constexpr LPCWSTR kAdminOnlyObjectSddl = L"D:P(A;;GA;;;SY)(A;;GA;;;BA)";

class ScopedSecurityDescriptor {
public:
    ScopedSecurityDescriptor() = default;
    ~ScopedSecurityDescriptor() { Reset(); }
    ScopedSecurityDescriptor(const ScopedSecurityDescriptor&) = delete;
    ScopedSecurityDescriptor& operator=(const ScopedSecurityDescriptor&) = delete;

    ScopedSecurityDescriptor(ScopedSecurityDescriptor&& other) noexcept
        : m_value(other.m_value) {
        other.m_value = nullptr;
    }

    ScopedSecurityDescriptor& operator=(ScopedSecurityDescriptor&& other) noexcept {
        if (this != &other) {
            Reset();
            m_value = other.m_value;
            other.m_value = nullptr;
        }
        return *this;
    }

    void Reset(PSECURITY_DESCRIPTOR value = nullptr) noexcept;
    PSECURITY_DESCRIPTOR Get() const noexcept { return m_value; }

private:
    PSECURITY_DESCRIPTOR m_value = nullptr;
};

bool BuildSecurityAttributes(LPCWSTR sddl,
                             SECURITY_ATTRIBUTES& sa,
                             ScopedSecurityDescriptor& sd) noexcept;

bool BuildAdminOnlySecurityAttributes(SECURITY_ATTRIBUTES& sa,
                                      ScopedSecurityDescriptor& sd) noexcept;

} // namespace Ipc
