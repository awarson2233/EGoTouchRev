// 轮廓与圆度 / 椭圆度（TZ_Contour.c）：把区域外接框附近的信号按双三次插值放大到每格 4×4 个点，从最强峰
// 正上方的边界出发沿门限等值线描出轮廓，再由轮廓点求圆度与椭圆度。

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <vector>

#include "Alg/Peak/PeakData.h"
#include "Alg/TouchZone/TouchZone.h"
#include "Alg/TouchZone/TouchZoneInternal.h"
#include "Common/Bss.h"
#include "Common/TsaMath.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::TouchZone {

namespace {

constexpr int kHrScale = 4;                 // 每格插值成 4×4 个点
constexpr int kPhaseNum = 5;                // 插值核的相位数（0..4 对应距左邻格 0..1 格）
constexpr int16_t kMaxEdgeDot = 150;
constexpr double kAxisToRadius = 2.449489742783178;  // √6：AxisFromMoments 的轴长为 √(6λ)
constexpr double kPi = 3.14159265;

/// 轮廓贴到的屏幕边，EdgeSides 返回的位按它编号。
enum Side : uint32_t { kLeft, kRight, kTop, kBottom, kSideNum };

struct Offset {
    int8_t dim1;
    int8_t dim2;
};
/// 8 个方向，从正上方起逆时针（dim2 向下为正）。
constexpr std::array<Offset, 8> kDirs = {{{0, -1}, {-1, -1}, {-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}}};

using Kernel = float[kPhaseNum][4];
/// g_afu：双三次插值核，[相位][抽头]。
Kernel& Kernels() { return Slot<Kernel>(Bss().afu); }
/// edgeDot_11595：镜像补全用的轮廓点副本。
EdgeDot* MirrorDots() { return &Slot<EdgeDot>(Bss().contourEdgeDot); }

using Window = std::array<std::array<int, 4>, 4>;

// TZ_BiCubicInterEdge @ 0x6ba85db3 的取窗部分
// 以 (dim1, dim2) 为左上角的 4×4 窗口；窗口越出屏幕的一侧（每个方向最多一侧）用相邻行列补齐。
void LoadWindow(int dim1, int dim2, Window& w) {
    const FlashPrmt& fl = PrmtFlash();
    const int16_t* sig = ZoneSig();
    int c0 = 0, c1 = 4, r0 = 0, r1 = 4;
    if (dim1 < 0) {
        c0 = 1;
    } else if (fl.dim1Len - 4 < dim1) {
        c1 = 3;
    }
    if (dim2 < 0) {
        r0 = 1;
    } else if (fl.dim2Len - 4 < dim2) {
        r1 = 3;
    }
    w = {};
    for (int r = r0; r < r1; ++r) {
        for (int c = c0; c < c1; ++c) w[r][c] = sig[static_cast<uint16_t>(fl.dim1Len * (r + dim2) + c + dim1)];
    }
    if (c0 != 0) {
        for (auto& row : w) row[0] = row[1];
    }
    if (c1 != 4) {
        for (auto& row : w) row[3] = row[2];
    }
    if (r0 != 0) w[0] = w[1];
    if (r1 != 4) w[3] = w[2];
}

// TZ_BiCubicInter4x4 @ 0x6ba85c95（求和次序按原汇编：w3·v3 + ((w0·v0 + w1·v1) + w2·v2)）
float Weighted(const Window& w, const float (&wDim1)[4], const float (&wDim2)[4]) {
    float acc = 0.0f;
    for (int r = 0; r < 4; ++r) {
        const auto& v = w[r];
        const float row = wDim1[3] * static_cast<float>(v[3]) +
                          ((wDim1[0] * static_cast<float>(v[0]) + wDim1[1] * static_cast<float>(v[1])) + wDim1[2] * static_cast<float>(v[2]));
        acc = acc + row * wDim2[r];
    }
    return acc;
}

// TZ_GetDifHR @ 0x6ba85fac / TZ_BiCubicInter @ 0x6ba85a85
// 高分辨率点 (x, y) 处的插值信号；超出区域的插值范围时为 0。
int HighResSig(const TouchZoneUnit& z, int x, int y) {
    if (x < z.hrMinDim1 || z.hrMaxDim1 < x || y < z.hrMinDim2 || z.hrMaxDim2 < y) return 0;
    const int cellDim1 = x / kHrScale;
    const int cellDim2 = y / kHrScale;
    Window w;
    LoadWindow(cellDim1 - 1, cellDim2 - 1, w);
    return static_cast<int>(Weighted(w, Kernels()[x - cellDim1 * kHrScale], Kernels()[y - cellDim2 * kHrScale]));
}

// TZ_Interpolate @ 0x6ba8613f
// 插值范围取外接框外扩一格（夹在屏内）。原代码还把范围内逐点的插值与按门限二值化的结果写进
// g_tzBufferHighRes / g_tzBufferBin（TZ_CalcHighResDif），没有模块读，不做；轮廓跟踪按需现算。
void SetInterpolationRange(TouchZoneUnit& z) {
    const FlashPrmt& fl = PrmtFlash();
    z.hrMinDim1 = static_cast<int16_t>(std::max(z.minDim1 - 1, 0) * kHrScale);
    z.hrMaxDim1 = static_cast<int16_t>(std::min(z.maxDim1 + 1, fl.dim1Len - 1) * kHrScale);
    z.hrMinDim2 = static_cast<int16_t>(std::max(z.minDim2 - 1, 0) * kHrScale);
    z.hrMaxDim2 = static_cast<int16_t>(std::min(z.maxDim2 + 1, fl.dim2Len - 1) * kHrScale);
}

/// 一次轮廓跟踪里 HighResSig 的备忘：跟踪的相邻步会反复插值同一个点。插值只取决于点坐标与本区域的
/// 插值范围、区域信号（跟踪期间不变），按插值范围开一张表，结果与每次现算相同。
class HighResCache {
public:
    explicit HighResCache(const TouchZoneUnit& z)
        : z_(z), x0_(z.hrMinDim1), y0_(z.hrMinDim2), w_(std::max(z.hrMaxDim1 - z.hrMinDim1 + 1, 0)),
          h_(std::max(z.hrMaxDim2 - z.hrMinDim2 + 1, 0)) {
        Memo().assign(static_cast<size_t>(w_) * h_, kUnset);
    }

    int operator()(int x, int y) const {
        if (x < x0_ || y < y0_ || x - x0_ >= w_ || y - y0_ >= h_) return HighResSig(z_, x, y);
        int& v = Memo()[static_cast<size_t>(y - y0_) * w_ + (x - x0_)];
        if (v == kUnset) v = HighResSig(z_, x, y);
        return v;
    }

private:
    static constexpr int kUnset = INT32_MIN;  // 插值结果是 int16 信号的加权和，不会取到它
    static std::vector<int>& Memo() {
        static std::vector<int> memo;
        return memo;
    }
    const TouchZoneUnit& z_;
    int x0_, y0_, w_, h_;
};

// TZ_FindContourFirstDotDim2 @ 0x6ba86380
// 从峰所在行往上找，返回最上面一个仍达到门限的点（到插值范围顶端为止）。
int FirstDotDim2(const TouchZoneUnit& z, const HighResCache& sig, int x, int thold, uint8_t peak) {
    for (int y = Peaks().entry[peak].dim2 * kHrScale;; --y) {
        if (y < z.hrMinDim2) return y;
        if (sig(x, y) < thold) return y + 1;
        if (y == z.hrMinDim2) return y;
    }
}

// TZ_PushEdgeDot @ 0x6ba8641c
void PushEdgeDot(TouchZoneUnit& z, int x, int y) {
    if (z.edgeDotCount >= kMaxEdgeDot) return;
    z.edgeDot[z.edgeDotCount] = {static_cast<int16_t>(x), static_cast<int16_t>(y)};
    ++z.edgeDotCount;
}

// TZ_FindContourDot @ 0x6ba8649b
// 从最强峰正上方的边界点出发做 Moore 邻域跟踪：每一步从来向的下一个方向起逆时针找第一个超过门限的点，
// 绕回起点或找不到时结束。（原代码结束时在 IDE 里画出轮廓，本机型已裁掉。）
void TraceEdge(TouchZoneUnit& z) {
    const int thold = z.sigThold;
    const uint8_t peak = z.maxSigPeakIdx;
    const HighResCache sig(z);
    int x = Peaks().entry[peak].dim1 * kHrScale;
    int y = FirstDotDim2(z, sig, x, thold, peak);
    z.edgeDotCount = 0;
    PushEdgeDot(z, x, y);
    uint32_t back = 0;
    for (;;) {
        bool moved = false;
        for (uint32_t k = 0; k < kDirs.size(); ++k) {
            const uint32_t d = (back + k + 1) & 7;
            const int nx = x + kDirs[d].dim1;
            const int ny = y + kDirs[d].dim2;
            if (z.edgeDot[0].dim1 == nx && z.edgeDot[0].dim2 == ny) break;
            if (thold < sig(nx, ny)) {
                moved = true;
                x = nx;
                y = ny;
                back = (d + 4) & 7;
                PushEdgeDot(z, x, y);
                break;
            }
        }
        if (!moved) return;
    }
}

// TZ_GetCircleRate @ 0x6ba866ad
// 轮廓点到其重心距离的变异系数（×100）：√(n·Σd² / (Σd)² − 1)。
void CircleRate(TouchZoneUnit& z) {
    const int n = z.edgeDotCount;
    double cx = 0.0;
    double cy = 0.0;
    for (int i = 0; i < n; ++i) {
        cx = cx + static_cast<double>(z.edgeDot[i].dim1);
        cy = cy + static_cast<double>(z.edgeDot[i].dim2);
    }
    if (n != 0) {
        cx = cx / static_cast<double>(n);
        cy = cy / static_cast<double>(n);
    }
    double sum = 0.0;
    double sqSum = 0.0;
    for (int i = 0; i < n; ++i) {
        const double dx = static_cast<double>(z.edgeDot[i].dim1) - cx;
        const double dy = static_cast<double>(z.edgeDot[i].dim2) - cy;
        const double d2 = dx * dx + dy * dy;
        sum = sum + std::sqrt(d2);
        sqSum = sqSum + d2;
    }
    z.circleRate = n == 0 ? 0 : static_cast<int16_t>(static_cast<int>(std::sqrt((static_cast<double>(n) * sqSum) / (sum * sum) - 1.0) * 100.0));
}

// TZ_GetERInernal @ 0x6ba86b64 / TZ_CalcAxisMass @ 0x6ba86978
// 由轮廓点的各阶矩求椭圆，把点变换到椭圆主轴坐标系并沿短轴按长短轴比拉伸，再求与 CircleRate 同样的离散度。
int16_t EllipseRate(const EdgeDot* dots, int n) {
    double m[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};  // 个数、Σx、Σy、Σxy、Σx²、Σy²
    for (int i = 0; i < n; ++i) {
        const int x = dots[i].dim1;
        const int y = dots[i].dim2;
        m[0] = m[0] + 1.0;
        m[1] = m[1] + static_cast<double>(x);
        m[2] = m[2] + static_cast<double>(y);
        m[3] = m[3] + static_cast<double>(x * y);
        m[4] = m[4] + static_cast<double>(x * x);
        m[5] = m[5] + static_cast<double>(y * y);
    }
    if (m[0] == 0.0) m[0] = 1.0;
    const double cx = m[1] / m[0];
    const double cy = m[2] / m[0];
    uint16_t major = 0;
    uint16_t minor = 0;
    int8_t angle = 0;
    AxisFromMoments(m[0], m[1], m[2], m[3], m[4], m[5], &major, &minor, &angle);
    major = static_cast<uint16_t>(static_cast<int>(static_cast<double>(major) / kAxisToRadius));
    minor = static_cast<uint16_t>(static_cast<int>(static_cast<double>(minor) / kAxisToRadius));
    const double theta = (static_cast<double>(angle) * kPi) / 180.0;
    if (minor == 0) minor = 1;
    const double stretch = static_cast<double>(major) / static_cast<double>(minor);
    const double c = std::cos(theta);
    const double s = std::sin(theta);
    double sum = 0.0;
    double sqSum = 0.0;
    for (int i = 0; i < n; ++i) {
        const double dx = static_cast<double>(dots[i].dim1) - cx;
        const double dy = static_cast<double>(dots[i].dim2) - cy;
        const double u = dx * c + dy * s;
        const double v = (-dx * s + dy * c) * stretch;
        const double d2 = u * u + v * v;
        sqSum = sqSum + d2;
        sum = sum + std::sqrt(d2);
    }
    if (n == 0 || sum == 0.0) return 0;
    return static_cast<int16_t>(static_cast<int>(std::sqrt((static_cast<double>(n) * sqSum) / (sum * sum) - 1.0) * 100.0));
}

// TZ_GetEdgeDotType @ 0x6ba8700e
// 点贴到的屏幕边（离边不到半格），按 Side 编号的位。
uint32_t EdgeSides(int x, int y) {
    const FlashPrmt& fl = PrmtFlash();
    uint32_t sides = 0;
    if (x >= (fl.dim1Len - 1) * kHrScale) {
        sides = 1u << kRight;
    } else if (x < 2) {
        sides = 1u << kLeft;
    }
    if (y >= (fl.dim2Len - 1) * kHrScale) {
        sides |= 1u << kBottom;
    } else if (y < 2) {
        sides |= 1u << kTop;
    }
    return sides;
}

// TZ_CalcEdgeCount @ 0x6ba8707e / TZ_GetMaxEdgeType @ 0x6ba87437
// 贴点最多的一边（并列时取编号小的）。
uint32_t MostTouchedSide(const TouchZoneUnit& z, std::array<uint32_t, kSideNum>& count) {
    count = {};
    for (int i = 0; i < z.edgeDotCount; ++i) {
        const uint32_t sides = EdgeSides(z.edgeDot[i].dim1, z.edgeDot[i].dim2);
        for (uint32_t k = 0; k < kSideNum; ++k) {
            if ((sides & (1u << k)) != 0) ++count[k];
        }
    }
    uint32_t best = 0;
    for (uint32_t k = 0; k < kSideNum; ++k) {
        if (count[best] < count[k]) best = k;
    }
    return best;
}

// TZ_RemoveEdgeDot @ 0x6ba87167
// 去掉贴在 side 上的点，其余按序前移。
void RemoveSideDots(EdgeDot* dots, int16_t& n, uint32_t side) {
    int16_t kept = 0;
    for (int i = 0; i < n; ++i) {
        const EdgeDot d = dots[i];
        if ((EdgeSides(d.dim1, d.dim2) & (1u << side)) == 0) dots[kept++] = d;
    }
    n = kept;
}

// TZ_MirrorEdgeDot @ 0x6ba872cc
// 把点关于 side 那条边（左 / 上边在 1，右 / 下边在 (len − 1) × 4）的镜像追加在后面，总数不超过 kMaxEdgeDot。
void AppendMirror(EdgeDot* dots, int16_t& n, uint32_t side) {
    const FlashPrmt& fl = PrmtFlash();
    int16_t out = n;
    for (int i = 0; i < n && out < kMaxEdgeDot; ++i, ++out) {
        EdgeDot d = dots[i];
        if (side == kRight) {
            d.dim1 = static_cast<int16_t>((fl.dim1Len - 1) * 8 - d.dim1);
        } else if (side == kLeft) {
            d.dim1 = static_cast<int16_t>(2 - d.dim1);
        } else if (side == kTop) {
            d.dim2 = static_cast<int16_t>(2 - d.dim2);
        } else if (side == kBottom) {
            d.dim2 = static_cast<int16_t>((fl.dim2Len - 1) * 8 - d.dim2);
        }
        dots[out] = d;
    }
    n = out;
}

// TZ_GetEllipseRateMirror @ 0x6ba8749f / TZ_CopyEdgeDot @ 0x6ba87246
// 轮廓贴边时被屏幕截断：去掉贴点最多那一边上的点，补上其余点的镜像后再求椭圆度。不贴边时同 ellipseRate。
int16_t EllipseRateMirror(const TouchZoneUnit& z) {
    std::array<uint32_t, kSideNum> count;
    const uint32_t side = MostTouchedSide(z, count);
    if (count[side] == 0) return z.ellipseRate;
    EdgeDot* dots = MirrorDots();
    std::copy_n(z.edgeDot, std::max<int>(z.edgeDotCount, 0), dots);
    int16_t n = z.edgeDotCount;
    RemoveSideDots(dots, n, side);
    AppendMirror(dots, n, side);
    return EllipseRate(dots, n);
}

}  // namespace

// TZ_ContourInit @ 0x6ba8590b
// 每个相位 f 的 4 个抽头是 CubicKernel 在 f+1、f、1−f、2−f 处的值。
void ContourInit() {
    Kernel& k = Kernels();
    for (int phase = 0; phase < kPhaseNum; ++phase) {
        const float f = static_cast<float>(phase) / static_cast<float>(kHrScale);
        k[phase][0] = static_cast<float>(Math::CubicKernel(static_cast<double>(f + 1.0f)));
        k[phase][1] = static_cast<float>(Math::CubicKernel(static_cast<double>(f)));
        k[phase][2] = static_cast<float>(Math::CubicKernel(static_cast<double>(1.0f - f)));
        k[phase][3] = static_cast<float>(Math::CubicKernel(static_cast<double>(2.0f - f)));
    }
}

// TZ_GetContour @ 0x6ba87571
void GetContour(TouchZoneUnit& z) {
    SetInterpolationRange(z);
    TraceEdge(z);
    CircleRate(z);
    z.ellipseRate = EllipseRate(z.edgeDot, z.edgeDotCount);
    z.ellipseRateMirror = EllipseRateMirror(z);
}

}  // namespace Solvers::Tsa::Core::TouchZone
