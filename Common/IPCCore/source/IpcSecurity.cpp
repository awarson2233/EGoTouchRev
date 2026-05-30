#include "Ipc/IpcSecurity.h"

#include <sddl.h>

namespace Ipc {

void ScopedSecurityDescriptor::Reset(PSECURITY_DESCRIPTOR value) noexcept {
    if (m_value) {
        LocalFree(m_value);
    }
    m_value = value;
}

bool BuildSecurityAttributes(LPCWSTR sddl,
                             SECURITY_ATTRIBUTES& sa,
                             ScopedSecurityDescriptor& sd) noexcept {
    sd.Reset();

    PSECURITY_DESCRIPTOR raw = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
            sddl, SDDL_REVISION_1, &raw, nullptr)) {
        return false;
    }

    sd.Reset(raw);
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = sd.Get();
    sa.bInheritHandle = FALSE;
    return true;
}

bool BuildAdminOnlySecurityAttributes(SECURITY_ATTRIBUTES& sa,
                                      ScopedSecurityDescriptor& sd) noexcept {
    return BuildSecurityAttributes(kAdminOnlyObjectSddl, sa, sd);
}

} // namespace Ipc
