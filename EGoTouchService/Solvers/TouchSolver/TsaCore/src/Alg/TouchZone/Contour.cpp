// 轮廓与圆度 / 椭圆度（TZ_Contour.c）：把区域外接框附近的信号按双三次插值放大到每格 4×4 个点，从最强峰
// 正上方的边界出发沿门限等值线描出轮廓，再由轮廓点求圆度与椭圆度。

#include <algorithm>
#include <array>
#include <bit>
#include <climits>
#include <cmath>

#include "Alg/Peak/PeakData.h"
#include "Alg/TouchZone/TouchZone.h"
#include "Alg/TouchZone/TouchZoneInternal.h"
#include "Common/Assert.h"
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

// TZ_BiCubicInterEdge @ 0x6ba85db3 的取窗部分
// 以 first 为起点的 4 条线（行或列）；窗口越出屏幕的一侧（每个方向最多一侧）用相邻的线补齐。返回第 k 条
// 实际取的线号。
int WindowLine(int first, int k, int len) {
    if (first < 0) return k == 0 ? first + 1 : first + k;
    if (len - 4 < first) return k == 3 ? first + 2 : first + k;
    return first + k;
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

/// 一次轮廓跟踪里的高分辨率插值（TZ_GetDifHR @ 0x6ba85fac / TZ_BiCubicInter @ 0x6ba85a85）：点 (x, y) 处
/// 的双三次插值，超出区域的插值范围时为 0。插值先按行求出 4 条行的横向插值，再按纵向的核加权
/// （TZ_BiCubicInter4x4 @ 0x6ba85c95，求和次序按原汇编：w3·v3 + ((w0·v0 + w1·v1) + w2·v2)，纵向从 0 起逐行累加）。
/// 跟踪的相邻步会反复插值同一个点、同一条行，两者都按区域的插值范围备忘；插值只取决于坐标与区域信号
/// （跟踪期间不变），浮点运算次序不变，结果与逐点现算相同。备忘表按最大阵列定长，每次只清用到的前缀。
class HighResCache {
public:
    explicit HighResCache(const TouchZoneUnit& z)
        : sig_(ZoneSig()), dim1Len_(PrmtFlash().dim1Len), dim2Len_(PrmtFlash().dim2Len), x0_(z.hrMinDim1),
          y0_(z.hrMinDim2), w_(std::max(z.hrMaxDim1 - z.hrMinDim1 + 1, 0)), h_(std::max(z.hrMaxDim2 - z.hrMinDim2 + 1, 0)) {
        TsaAssert(w_ <= kMaxHrLen && h_ <= kMaxHrLen);
        std::fill_n(g_pointMemo.begin(), static_cast<size_t>(w_) * h_, kUnset);
        // 行备忘覆盖插值范围内各点可能取到的行：所在格行的上一行到下两行
        rowBase_ = y0_ / kHrScale - 1;
        rows_ = h_ == 0 ? 0 : (y0_ + h_ - 1) / kHrScale + 2 - rowBase_ + 1;
        std::fill_n(g_rowMemo.begin(), static_cast<size_t>(w_) * std::max(rows_, 0), kRowUnset);
    }

    int operator()(int x, int y) const {
        if (x < x0_ || y < y0_ || x - x0_ >= w_ || y - y0_ >= h_) return 0;
        int& v = g_pointMemo[static_cast<size_t>(y - y0_) * w_ + (x - x0_)];
        if (v == kUnset) v = Compute(x, y);
        return v;
    }

private:
    static constexpr int kUnset = INT32_MIN;  // 插值结果是 int16 信号的加权和，不会取到它
    static constexpr uint32_t kRowUnset = 0x7fc00001;  // 一个 NaN 位型；有限整数的加权和不会是 NaN
    static constexpr int kMaxDimLen = 0x50;  // TSA_MAX_DIM
    /// 插值范围一边最多 (kMaxDimLen − 1) × 4 + 1 个点；行备忘最多覆盖 kMaxDimLen + 3 行。
    static constexpr int kMaxHrLen = (kMaxDimLen - 1) * kHrScale + 1;
    static constexpr int kMaxMemoRows = kMaxDimLen + 3;

    static inline std::array<int, size_t{kMaxHrLen} * kMaxHrLen> g_pointMemo{};
    static inline std::array<uint32_t, size_t{kMaxHrLen} * kMaxMemoRows> g_rowMemo{};

    int Compute(int x, int y) const {
        const int cellDim2 = y / kHrScale;
        const float(&wDim2)[4] = Kernels()[y - cellDim2 * kHrScale];
        float acc = 0.0f;
        for (int k = 0; k < 4; ++k) acc = acc + Row(x, WindowLine(cellDim2 - 1, k, dim2Len_)) * wDim2[k];
        return static_cast<int>(acc);
    }

    /// 第 row 行在 x 处的横向插值。
    float Row(int x, int row) const {
        const int r = row - rowBase_;
        uint32_t* slot = nullptr;
        if (0 <= r && r < rows_) {
            slot = &g_rowMemo[static_cast<size_t>(r) * w_ + (x - x0_)];
            if (*slot != kRowUnset) return std::bit_cast<float>(*slot);
        }
        const int cellDim1 = x / kHrScale;
        const float(&wDim1)[4] = Kernels()[x - cellDim1 * kHrScale];
        float v[4];
        for (int k = 0; k < 4; ++k) {
            const int col = WindowLine(cellDim1 - 1, k, dim1Len_);
            v[k] = static_cast<float>(sig_[static_cast<uint16_t>(dim1Len_ * row + col)]);
        }
        const float value = wDim1[3] * v[3] + ((wDim1[0] * v[0] + wDim1[1] * v[1]) + wDim1[2] * v[2]);
        if (slot != nullptr) *slot = std::bit_cast<uint32_t>(value);
        return value;
    }

    const int16_t* sig_;
    int dim1Len_, dim2Len_;
    int x0_, y0_, w_, h_;
    int rowBase_ = 0, rows_ = 0;
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
