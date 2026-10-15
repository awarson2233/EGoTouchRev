#pragma once
// 调参表（TtPrmt）与运行时参数表（RtPrmt）共用：表项与机型参数字段的对应关系。
// 每项的值按字段的宽度与符号读成 int32，写回时截断成字段宽度。只在 Prmt 目录内使用。

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <utility>

#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::Tuning {

enum class FieldKind : uint8_t { kNone, kU8, kS8, kU16, kS16, k32 };

/// 某个参数结构体里的一个字段：字节偏移与读写方式。
struct FieldRef {
    uint16_t offset;
    FieldKind kind;
};

template <class T>
consteval FieldKind KindOf() {
    if constexpr (std::is_same_v<T, uint8_t>) return FieldKind::kU8;
    else if constexpr (std::is_same_v<T, int8_t>) return FieldKind::kS8;
    else if constexpr (std::is_same_v<T, uint16_t>) return FieldKind::kU16;
    else if constexpr (std::is_same_v<T, int16_t>) return FieldKind::kS16;
    else {
        static_assert(sizeof(T) == 4);
        return FieldKind::k32;
    }
}

/// 上下限的来历。
enum class Bounds : uint8_t {
    kFixed,        // 表里给定的常数
    kObsolete,     // 标为 obsolete 的项：上下限都等于当前值
    kAroundValue,  // 上限为值的 2 倍，下限为值减去它的 1/4
    kDerived,      // 取决于其它机型参数，由各表的 Init 单独填
};

/// 表项的静态描述：对应的字段、上下限与说明。
struct EntrySpec {
    FieldRef field;
    Bounds bounds;
    int32_t maxValue;
    int32_t minValue;
    const char* description;
};

template <class S>
[[nodiscard]] inline int32_t Read(const S& base, FieldRef f) noexcept {
    const auto* p = reinterpret_cast<const uint8_t*>(&base) + f.offset;
    switch (f.kind) {
    case FieldKind::kU8: return *p;
    case FieldKind::kS8: return static_cast<int8_t>(*p);
    case FieldKind::kU16: { uint16_t v; std::memcpy(&v, p, 2); return v; }
    case FieldKind::kS16: { int16_t v; std::memcpy(&v, p, 2); return v; }
    case FieldKind::k32: { int32_t v; std::memcpy(&v, p, 4); return v; }
    case FieldKind::kNone: break;
    }
    return 0;
}

template <class S>
inline void Write(S& base, FieldRef f, int32_t value) noexcept {
    auto* p = reinterpret_cast<uint8_t*>(&base) + f.offset;
    switch (f.kind) {
    case FieldKind::kU8:
    case FieldKind::kS8: *p = static_cast<uint8_t>(value); break;
    case FieldKind::kU16:
    case FieldKind::kS16: { const auto v = static_cast<uint16_t>(value); std::memcpy(p, &v, 2); break; }
    case FieldKind::k32: std::memcpy(p, &value, 4); break;
    case FieldKind::kNone: break;
    }
}

/// 按描述填一项：当前值与缺省值都取 value，上下限按 bounds（kDerived 先按表里的常数，由调用方再改）。
inline void Fill(TuningEntry& entry, const EntrySpec& spec, int32_t value) noexcept {
    entry.value = value;
    entry.defaultValue = value;
    switch (spec.bounds) {
    case Bounds::kObsolete:
        entry.maxValue = value;
        entry.minValue = value;
        break;
    case Bounds::kAroundValue:
        entry.maxValue = value * 2;
        entry.minValue = value - value / 4;
        break;
    case Bounds::kFixed:
    case Bounds::kDerived:
        entry.maxValue = spec.maxValue;
        entry.minValue = spec.minValue;
        break;
    }
    entry.description = spec.description;
}

}  // namespace Solvers::Tsa::Core::Tuning

/// Struct 里 member 字段的 FieldRef（member 可以是嵌套成员，如 cmfTholdAlt.amp）。
#define TSA_PRMT_FIELD(Struct, member)                                              \
    (::Solvers::Tsa::Core::Tuning::FieldRef{                                        \
        static_cast<uint16_t>(offsetof(Struct, member)),                            \
        ::Solvers::Tsa::Core::Tuning::KindOf<decltype(std::declval<Struct&>().member)>()})
