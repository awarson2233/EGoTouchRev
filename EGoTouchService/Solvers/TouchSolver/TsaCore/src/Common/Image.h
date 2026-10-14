#pragma once
// ══════════════════════════════════════════════════════════════════════
// 过渡期的状态访问：算法状态仍然在 raw 的 g_tsaMemoryImage 里，按原 DLL 的地址排布
// （.data / .rdata / .bss，VA 0x6bb4e000 起）。已经转成 C++ 的模块经这里按原地址拿到
// 带类型的引用，还没转换的 C 代码读写的是同一块内存，两边始终一致；回放比对的状态摘要
// 也按原地址规范化，所以转换前后可以逐字节对比。
//
// 全部模块转换完以后，状态改为 Context 的成员，这个头文件随之删除。
// ══════════════════════════════════════════════════════════════════════

#include <cstdint>
#include <cstring>
#include <type_traits>

extern "C" uint8_t g_tsaMemoryImage[];  // raw/Common/TSABssMemoryImage.c

namespace Solvers::Tsa::Core::Image {

inline constexpr uint64_t kBaseVa = 0x6bb4e000;

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
