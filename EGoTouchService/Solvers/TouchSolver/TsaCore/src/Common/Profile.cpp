#include "Common/Profile.h"

#include <cstddef>

#include "../TsaCoreApi.h"

#if defined(TSACORE_PROFILE)
namespace Solvers::Tsa::Core::Profile {

namespace {

constexpr size_t kMaxEntries = 256;
TsaCore_ProfileEntry g_entries[kMaxEntries];
size_t g_count = 0;

}  // namespace

// 名字是调用处的字符串字面量，同一处每次都是同一个指针，按指针查表。
void Add(const char* name, uint64_t ns) noexcept {
    for (size_t i = 0; i < g_count; ++i) {
        if (g_entries[i].name == name) {
            g_entries[i].ns += ns;
            ++g_entries[i].count;
            return;
        }
    }
    if (g_count < kMaxEntries) g_entries[g_count++] = {name, ns, 1};
}

}  // namespace Solvers::Tsa::Core::Profile
#endif

extern "C" size_t TsaCore_ProfileEntries(TsaCore_ProfileEntry* out, size_t capacity) {
#if defined(TSACORE_PROFILE)
    namespace P = Solvers::Tsa::Core::Profile;
    if (out != nullptr) {
        for (size_t i = 0; i < P::g_count && i < capacity; ++i) out[i] = P::g_entries[i];
    }
    return P::g_count;
#else
    (void)out;
    (void)capacity;
    return 0;
#endif
}
