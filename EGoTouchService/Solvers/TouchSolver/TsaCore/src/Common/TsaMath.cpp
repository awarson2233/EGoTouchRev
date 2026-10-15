#include "Common/TsaMath.h"

namespace Solvers::Tsa::Core::Math {
namespace {

// 八分圆的 4 个扇区边界：255·(cos θ, sin θ)，θ = 0°, 11.25°, 22.5°, 33.75°, 45°（原 .rdata 的
// g_boundaryVectorXs / g_boundaryVectorYs）
constexpr uint8_t kBoundaryX[5] = {255, 251, 237, 213, 181};
constexpr uint8_t kBoundaryY[5] = {0, 49, 97, 142, 181};

}  // namespace

// TSAMath_ATan2 @ 0x6bb429d4
int ATan2(int16_t y, int16_t x) noexcept {
    const auto absY = static_cast<uint16_t>(y < 0 ? -y : y);
    const auto absX = static_cast<uint16_t>(x < 0 ? -x : x);

    // 第一象限内的角度，256 份 = 180°
    int16_t angle;
    if (absY == 0 && absX == 0) {
        angle = 256;
    } else if (absX < absY) {
        angle = static_cast<int16_t>(128 - static_cast<int8_t>(ArcTanOctant(absY, absX)));
    } else {
        angle = static_cast<int8_t>(ArcTanOctant(absX, absY));
    }

    if (y >= 0 && x < 0) {
        angle = static_cast<int16_t>(256 - angle);
    } else if (y < 0 && x < 0) {
        angle = static_cast<int16_t>(angle - 256);
    } else if (y < 0) {
        angle = static_cast<int16_t>(-angle);
    }
    return angle * 180 / 256;
}

// TSAMath_ArcTanOctant @ 0x6bb428a0
int ArcTanOctant(int major, int minor) noexcept {
    const auto a = static_cast<uint32_t>(major);
    const auto b = static_cast<uint32_t>(minor);

    // 先二分出扇区：最大的 s 使 tan θs ≤ b / a
    uint32_t sector = 0;
    for (uint32_t step = 2; step != 0; step >>= 1) {
        sector += step;
        if (kBoundaryX[sector] * b < kBoundaryY[sector] * a) sector -= step;
    }

    // 扇区内再分 16 级：在两条边界向量之间线性插值，同样二分。
    // 差值可能为负，按 32 位无符号回绕计算，比较的是回绕后的结果。
    const uint32_t x0 = kBoundaryX[sector] * b;
    const uint32_t x1 = kBoundaryX[sector + 1] * b;
    const uint32_t y0 = kBoundaryY[sector] * a;
    const uint32_t y1 = kBoundaryY[sector + 1] * a;
    uint32_t fine = 0;
    for (uint32_t step = 8; step != 0; step >>= 1) {
        fine += step;
        if (x0 * 16 + (x1 - x0) * fine < (y1 - y0) * fine + y0 * 16) fine -= step;
    }
    return static_cast<int>(fine + sector * 16);
}

// Misc_SqrtUint32 @ 0x6ba64d8b
int SqrtUint32(uint32_t v) noexcept {
    uint32_t bit = 0x8000;
    while (v < bit) bit >>= 1;
    uint32_t root = 0;
    for (; bit != 0; bit >>= 1) {
        root += bit;
        if (v < root * root) root -= bit;
    }
    return static_cast<int>(root);
}

// SinXDivX @ 0x6ba8573e：系数是 float 常量（a = −1 及 2、3、5、8、4），按原式的运算次序求值
double CubicKernel(double x) noexcept {
    constexpr float kA = -1.0f;
    const double ax = x < 0.0 ? -x : x;
    const double ax2 = ax * ax;
    const double ax3 = ax2 * ax;
    if (ax <= 1.0) {
        return (static_cast<double>(kA + 2.0f) * ax3 - static_cast<double>(kA + 3.0f) * ax2) + 1.0;
    }
    if (ax <= 2.0) {
        return ((static_cast<double>(kA) * ax3 - static_cast<double>(kA * 5.0f) * ax2) + static_cast<double>(kA * 8.0f) * ax) -
               static_cast<double>(kA * 4.0f);
    }
    return 0.0;
}

}  // namespace Solvers::Tsa::Core::Math

// ── 过渡期：未转换的 C 代码按原名调用 ──────────────────────────────
extern "C" int TSAMath_ATan2(uint16_t y, uint16_t x) {
    return Solvers::Tsa::Core::Math::ATan2(static_cast<int16_t>(y), static_cast<int16_t>(x));
}
