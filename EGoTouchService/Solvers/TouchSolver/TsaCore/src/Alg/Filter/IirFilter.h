#pragma once
// 一阶 IIR：out = (weight·current + (2^shift − weight)·previous) / 2^shift。

#include <cstdint>

namespace Solvers::Tsa::Core::Iir {

/// 一阶 IIR。注意 2^shift 在算 previous 的权重时只保留低 8 位（shift ≥ 8 时 previous 的权重为 −weight），
/// 与原实现一致。
[[nodiscard]] int Process(int current, int previous, uint8_t weight, uint8_t shift) noexcept;

/// 同 Process，结果再向 current 挪 1，保证 previous 最终能追上 current。
[[nodiscard]] int ProcessConverging(int current, int previous, uint8_t weight, uint8_t shift) noexcept;

}  // namespace Solvers::Tsa::Core::Iir

namespace Solvers::Tsa::Core::Aiir {

/// 自适应 IIR 的权重：位移量 delta 越大，越信任当前值（见 AiirPrmt）。delta 非零时权重至少为 1。
[[nodiscard]] uint8_t GetCoef(uint16_t delta) noexcept;

}  // namespace Solvers::Tsa::Core::Aiir
