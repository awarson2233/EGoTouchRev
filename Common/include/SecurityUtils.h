#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <sddl.h>
#include <memory>
#include <string>

namespace Security {

class ScopedSecurityAttributes {
public:
    ScopedSecurityAttributes(const wchar_t* sddl = L"D:(A;;GA;;;SY)(A;;GA;;;BA)(A;;GRGW;;;BU)") {
        m_sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        m_sa.bInheritHandle = FALSE;
        m_sa.lpSecurityDescriptor = nullptr;

        if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
                sddl,
                SDDL_REVISION_1,
                &m_sa.lpSecurityDescriptor,
                nullptr)) {
            m_sa.lpSecurityDescriptor = nullptr;
        }
    }

    ~ScopedSecurityAttributes() {
        if (m_sa.lpSecurityDescriptor != nullptr) {
            LocalFree(m_sa.lpSecurityDescriptor);
            m_sa.lpSecurityDescriptor = nullptr;
        }
    }

    // Non-copyable
    ScopedSecurityAttributes(const ScopedSecurityAttributes&) = delete;
    ScopedSecurityAttributes& operator=(const ScopedSecurityAttributes&) = delete;

    // Non-movable (can add move support if needed, but simple RAII wrapper for now)
    ScopedSecurityAttributes(ScopedSecurityAttributes&&) = delete;
    ScopedSecurityAttributes& operator=(ScopedSecurityAttributes&&) = delete;

    SECURITY_ATTRIBUTES* get() {
        return m_sa.lpSecurityDescriptor ? &m_sa : nullptr;
    }

private:
    SECURITY_ATTRIBUTES m_sa{};
};

} // namespace Security
