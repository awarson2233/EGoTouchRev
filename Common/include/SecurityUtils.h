#pragma once

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
        if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
                L"D:(A;;GA;;;SY)(A;;GA;;;BA)(A;;GRGW;;;BU)",
                SDDL_REVISION_1,
                &m_sa.lpSecurityDescriptor,
                nullptr)) {
            // Fallback to default security descriptor if conversion fails
            m_sa.lpSecurityDescriptor = nullptr;
        }
    }

    ~ScopedSecurityAttributes() {
        if (m_sa.lpSecurityDescriptor) {
            LocalFree(m_sa.lpSecurityDescriptor);
            m_sa.lpSecurityDescriptor = nullptr;
        }
    }

    // Delete copy and move semantics to prevent double free
    ScopedSecurityAttributes(const ScopedSecurityAttributes&) = delete;
    ScopedSecurityAttributes& operator=(const ScopedSecurityAttributes&) = delete;
    ScopedSecurityAttributes(ScopedSecurityAttributes&&) = delete;
    ScopedSecurityAttributes& operator=(ScopedSecurityAttributes&&) = delete;

    SECURITY_ATTRIBUTES* get() {
        return m_sa.lpSecurityDescriptor ? &m_sa : nullptr;
    }

private:
    SECURITY_ATTRIBUTES m_sa{};
};

} // namespace Security
