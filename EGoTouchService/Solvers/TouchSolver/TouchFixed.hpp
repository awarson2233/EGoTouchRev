#pragma once

// ══════════════════════════════════════════════════════════════════════
// TouchFixed —— TSACore 手指管线的定点算术基元。
//
// TSACore 是为 MCU 写的 C 代码，它的数值行为有三条贯穿全管线的约定，这三条
// 都不是「实现细节」而是**规范的一部分**：偏离任何一条都会在某些输入下产出
// 与原版不同的坐标。
//
//   1. 16 位写回是**回绕**，不是饱和。原版一律 `(int16)x` 截断。
//   2. 所有 `>>` 都是「负数先加 2^k − 1 再算术右移」的**向零取整**，
//      不是 C 的算术右移（向下取整）。
//   3. 部分 32 位累加（最典型的是 IDT 匈牙利的行列势）依赖**自然回绕**。
//      文档 05 §2.4 给出了 3×3 反例：slack = 2 300 576 607 回绕后会改变配对。
//      用 int64 或饱和「修正」它就是偏离原版。
//
// 另有一条容易被忽略的：`BLIIR_DoUpdate` 的差值是 **uint16 零扩展**后相减，
// 不是有符号 int16 相减（文档 01 §6）。
//
// 全管线禁止再手写移位取整、饱和与回绕，一律走这里。
// ══════════════════════════════════════════════════════════════════════

#include <cstdint>
#include <type_traits>

namespace Solvers::Touch::Fixed {

// ── 16 位回绕写回 ─────────────────────────────────────────────────────
/// 对位原版的 `(int16)x`：丢弃高位，保留低 16 位并按有符号解释。
/// 与 std::clamp 到 [INT16_MIN, INT16_MAX] 的**饱和**语义不同，不可互换。
[[nodiscard]] constexpr int16_t Wrap16(int32_t v) noexcept {
    return static_cast<int16_t>(static_cast<uint16_t>(static_cast<uint32_t>(v) & 0xFFFFu));
}

/// 对位原版的 `(uint16)x`。
[[nodiscard]] constexpr uint16_t WrapU16(int32_t v) noexcept {
    return static_cast<uint16_t>(static_cast<uint32_t>(v) & 0xFFFFu);
}

// ── 32 位回绕加减 ─────────────────────────────────────────────────────
// 有符号溢出在 C++ 里是 UB，编译器可以据此做出任意假设。走无符号域绕一圈
// 是唯一既定义良好、又与原版汇编 `ADD/SUB` 行为一致的写法。
[[nodiscard]] constexpr int32_t WrapAdd32(int32_t a, int32_t b) noexcept {
    return static_cast<int32_t>(static_cast<uint32_t>(a) + static_cast<uint32_t>(b));
}

[[nodiscard]] constexpr int32_t WrapSub32(int32_t a, int32_t b) noexcept {
    return static_cast<int32_t>(static_cast<uint32_t>(a) - static_cast<uint32_t>(b));
}

[[nodiscard]] constexpr int32_t WrapMul32(int32_t a, int32_t b) noexcept {
    return static_cast<int32_t>(static_cast<uint32_t>(a) * static_cast<uint32_t>(b));
}

// ── 向零取整的右移 ────────────────────────────────────────────────────
/// 对位原版编译器对 `x / (1 << k)` 生成的惯用式：
///     负数先加 (2^k − 1) 再算术右移。
/// 例：ShrTowardZero(-3, 1) == -1（C 的 -3 >> 1 是 -2）。
[[nodiscard]] constexpr int32_t ShrTowardZero(int32_t x, int k) noexcept {
    const int32_t bias = (static_cast<int32_t>(1) << k) - 1;
    return (x < 0 ? (x + bias) : x) >> k;
}

/// 少数几处原版写的是**无符号**右移（如 TZ_GetSigTholdCommon 的 >> 7），
/// 语义是向下取整。单独给一个名字，避免与上面混用。
[[nodiscard]] constexpr int32_t ShrUnsigned(int32_t x, int k) noexcept {
    return static_cast<int32_t>(static_cast<uint32_t>(x) >> k);
}

// ── 截断除 ────────────────────────────────────────────────────────────
/// C 的 `/` 本就向零截断，这里只是给它一个显式名字，便于在代码里
/// 与「应当向下取整」的场合区分开，并统一做除零保护。
/// 除零时返回 0，对位原版 `m0 == 0 → 0` 之类的显式短路。
[[nodiscard]] constexpr int32_t TruncDiv(int32_t a, int32_t b) noexcept {
    return (b == 0) ? 0 : (a / b);
}

// ── uint16 零扩展相减 ─────────────────────────────────────────────────
/// 对位 BLIIR_DoUpdate 的差值计算：两个 int16 先按 **uint16 零扩展**
/// 提升到 int，再相减。与有符号相减在任一侧为负时结果不同。
///     ZeroExtSub(-1, 0) == 65535   （有符号相减是 -1）
[[nodiscard]] constexpr int32_t ZeroExtSub(int16_t a, int16_t b) noexcept {
    return static_cast<int32_t>(static_cast<uint16_t>(a))
         - static_cast<int32_t>(static_cast<uint16_t>(b));
}

// ── 整数平方根 ────────────────────────────────────────────────────────
/// 对位 `Misc_SqrtUint32`：逐位试探的整数平方根，结果**向下取整**。
/// 模块 05（IDT 距离）、07（AIIR 距离、JIT 死区、折返路程）共用同一实现，
/// 任何一处换成 std::sqrt 都会在边界值上产生 ±1 的差异。
[[nodiscard]] constexpr uint32_t Misc_SqrtUint32(uint32_t v) noexcept {
    uint32_t r = 0;
    // 从高位往低位试探，避免 (r+1)*(r+1) 在 v 接近 2^32 时溢出。
    for (uint32_t bit = 1u << 15; bit != 0; bit >>= 1) {
        const uint32_t t = r | bit;
        if (t <= v / t) r = t;   // 等价于 t*t <= v，但不溢出
    }
    return r;
}

// ── 自检 ──────────────────────────────────────────────────────────────
static_assert(Misc_SqrtUint32(0) == 0 && Misc_SqrtUint32(1) == 1);
static_assert(Misc_SqrtUint32(15) == 3 && Misc_SqrtUint32(16) == 4);
static_assert(Misc_SqrtUint32(0xFFFFFFFFu) == 65535, "上界不得溢出");

static_assert(Wrap16(0x8000) == INT16_MIN, "Wrap16 必须回绕而非饱和");
static_assert(Wrap16(0x17FFF) == 0x7FFF, "Wrap16 只保留低 16 位");
static_assert(Wrap16(0x1FFFF) == -1, "Wrap16 必须丢弃高位");
static_assert(ShrTowardZero(-3, 1) == -1, "ShrTowardZero 必须向零取整");
static_assert(ShrTowardZero(3, 1) == 1, "非负路径与算术右移一致");
static_assert(ShrTowardZero(-256, 8) == -1, "整除时不受 bias 影响");
static_assert(ShrUnsigned(-1, 7) == 0x01FFFFFF, "ShrUnsigned 走无符号域");
static_assert(TruncDiv(-7, 2) == -3, "TruncDiv 向零截断");
static_assert(TruncDiv(1, 0) == 0, "除零短路为 0");
static_assert(ZeroExtSub(static_cast<int16_t>(-1), 0) == 65535, "零扩展后相减");
static_assert(WrapAdd32(INT32_MAX, 1) == INT32_MIN, "32 位加法必须回绕");

} // namespace Solvers::Touch::Fixed
