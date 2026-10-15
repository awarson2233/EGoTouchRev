#pragma once
// ══════════════════════════════════════════════════════════════════════
// 状态访问：算法状态在 g_tsaMemoryImage 里，按原 DLL 的地址排布（.data / .rdata / .bss，
// VA 0x6bb4e000 起）。各模块经这里按原地址拿到带类型的引用；回放比对的状态摘要也按原地址
// 规范化，所以重构前后可以逐字节对比。
// ══════════════════════════════════════════════════════════════════════

#include <cstdint>
#include <cstring>
#include <type_traits>

extern "C" uint8_t g_tsaMemoryImage[];  // Common/ImageInit.cpp

namespace Solvers::Tsa::Core::Image {

inline constexpr uint64_t kBaseVa = 0x6bb4e000;
inline constexpr uint64_t kSize = 0x15f000;  // 到 0x6bcad000

/// 原 DLL 地址 va 处的对象。
template <class T>
[[nodiscard]] inline T& At(uint64_t va) noexcept {
    return *reinterpret_cast<T*>(g_tsaMemoryImage + (va - kBaseVa));
}

}  // namespace Solvers::Tsa::Core::Image

namespace Solvers::Tsa::Core {

/// 把对象的全部字节（包括填充）清零，对应原代码的 memset_s(&obj, sizeof obj, 0, sizeof obj)。
/// 填充字节也是状态的一部分，不能用 obj = {} 代替。
template <class T>
inline void ZeroFill(T& obj) noexcept {
    static_assert(std::is_trivially_copyable_v<T>);
    std::memset(&obj, 0, sizeof(T));
}

}  // namespace Solvers::Tsa::Core
