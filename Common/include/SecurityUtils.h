#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <sddl.h>

namespace Security {

class ScopedSecurityAttributes {
public:
    ScopedSecurityAttributes() {
        m_sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        m_sa.bInheritHandle = FALSE;
        m_sa.lpSecurityDescriptor = nullptr;

        // Secure SDDL:
        // D: DACL
        // (A;;GA;;;SY) -> Allow Generic All for SYSTEM
        // (A;;GA;;;BA) -> Allow Generic All for Built-in Administrators
        // (A;;GRGW;;;BU) -> Allow Generic Read/Write for Built-in Users
        LPCWSTR sddl = L"D:(A;;GA;;;SY)(A;;GA;;;BA)(A;;GRGW;;;BU)";
        ConvertStringSecurityDescriptorToSecurityDescriptorW(
            sddl, SDDL_REVISION_1, &m_sa.lpSecurityDescriptor, nullptr);
    }

    ~ScopedSecurityAttributes() {
        if (m_sa.lpSecurityDescriptor != nullptr) {
            LocalFree(m_sa.lpSecurityDescriptor);
            m_sa.lpSecurityDescriptor = nullptr;
        }
    }

    // Disable copy
    ScopedSecurityAttributes(const ScopedSecurityAttributes&) = delete;
    ScopedSecurityAttributes& operator=(const ScopedSecurityAttributes&) = delete;

    // Move constructor
    ScopedSecurityAttributes(ScopedSecurityAttributes&& other) noexcept {
        m_sa = other.m_sa;
        other.m_sa.lpSecurityDescriptor = nullptr;
    }

    // Move assignment
    ScopedSecurityAttributes& operator=(ScopedSecurityAttributes&& other) noexcept {
        if (this != &other) {
            if (m_sa.lpSecurityDescriptor != nullptr) {
                LocalFree(m_sa.lpSecurityDescriptor);
            }
            m_sa = other.m_sa;
            other.m_sa.lpSecurityDescriptor = nullptr;
        }
        return *this;
    }

    SECURITY_ATTRIBUTES* get() {
        return m_sa.lpSecurityDescriptor ? &m_sa : nullptr;
    }

private:
    SECURITY_ATTRIBUTES m_sa{};
};

} // namespace Security
