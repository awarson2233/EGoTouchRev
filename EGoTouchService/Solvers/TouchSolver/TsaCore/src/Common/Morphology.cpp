#include "Common/Morphology.h"

#include <algorithm>
#include <array>
#include <cstddef>

#include "Common/Assert.h"

namespace Solvers::Tsa::Core::Morphology {

namespace {

/// 原代码在栈上拷贝一份处理前的图，容量 0x2648 字节；更大的图在原代码里拷贝失败（运行库报无效参数）。
constexpr size_t kMaxCells = 0x2648 / sizeof(int16_t);
using Copy = std::array<int16_t, kMaxCells>;

void Snapshot(Copy& copy, const int16_t* img, int dim1Len, int dim2Len) {
    const auto n = static_cast<size_t>(dim1Len) * static_cast<size_t>(dim2Len);
    TsaAssert(n <= kMaxCells);
    std::copy_n(img, std::min(n, kMaxCells), copy.begin());
}

/// 以 (dim1, dim2) 为中心的邻域，裁到图内。
struct Box {
    int dim1Lo;
    int dim1Hi;
    int dim2Lo;
    int dim2Hi;
};
Box Neighborhood(int dim1Len, int dim2Len, int dim1, int dim2, int radius1, int radius2) {
    return {std::max(dim1 - radius1, 0), std::min(radius1 + dim1, dim1Len - 1), std::max(dim2 - radius2, 0),
            std::min(radius2 + dim2, dim2Len - 1)};
}

// Image_IsWhiteInNeighbor @ 0x6ba6504f
bool HasSetInNeighborhood(const int16_t* img, int dim1Len, int dim2Len, int dim1, int dim2, int radius1, int radius2) {
    const Box b = Neighborhood(dim1Len, dim2Len, dim1, dim2, radius1, radius2);
    for (int r = b.dim2Lo; r <= b.dim2Hi; ++r) {
        for (int c = b.dim1Lo; c <= b.dim1Hi; ++c) {
            if (img[c + r * dim1Len] != 0) return true;
        }
    }
    return false;
}

// Image_ClearNeighbor @ 0x6ba64f90
void ClearNeighborhood(int16_t* img, int dim1Len, int dim2Len, int dim1, int dim2, int radius1, int radius2) {
    const Box b = Neighborhood(dim1Len, dim2Len, dim1, dim2, radius1, radius2);
    for (int r = b.dim2Lo; r <= b.dim2Hi; ++r) {
        for (int c = b.dim1Lo; c <= b.dim1Hi; ++c) img[c + r * dim1Len] = 0;
    }
}

// Image_CrossErosionDim1 @ 0x6ba6540b
void CrossErosionDim1(int16_t* img, const int16_t* src, uint32_t dim1Len, int dim2Len) {
    for (uint32_t i = dim1Len * dim2Len; i-- != 0;) {
        if (src[i] != 0) continue;
        const uint32_t row = i / dim1Len;
        const uint32_t col = i % dim1Len;
        const bool aboveClear = row == 0 || src[i - dim1Len] == 0;
        const bool belowClear = dim2Len - 1U == row || src[i + dim1Len] == 0;
        if (!aboveClear || !belowClear) continue;
        if (static_cast<int>(col - 1) >= 0) img[i - 1] = 0;
        if (static_cast<int>(col + 1) < static_cast<int>(dim1Len)) img[i + 1] = 0;
    }
}

// Image_CrossErosionDim2 @ 0x6ba65518
// 判断下面一格是否在图内时，原代码拿行号与列数（而不是行数）比。
void CrossErosionDim2(int16_t* img, const int16_t* src, uint32_t dim1Len, int dim2Len) {
    for (uint32_t i = dim1Len * dim2Len; i-- != 0;) {
        if (src[i] != 0) continue;
        const uint32_t row = i / dim1Len;
        const uint32_t col = i % dim1Len;
        const bool leftClear = col == 0 || src[i - 1] == 0;
        const bool rightClear = dim1Len - 1 == col || src[i + 1] == 0;
        if (!leftClear || !rightClear) continue;
        if (static_cast<int>(row - 1) >= 0) img[i - dim1Len] = 0;
        if (static_cast<int>(row + 1) < static_cast<int>(dim1Len)) img[i + dim1Len] = 0;
    }
}

}  // namespace

// Image_Dilation @ 0x6ba6511d
void Dilation(int16_t* img, int dim1Len, int dim2Len, int radius1, int radius2) {
    Copy before;
    Snapshot(before, img, dim1Len, dim2Len);
    for (int c = 0; c < dim1Len; ++c) {
        for (int r = 0; r < dim2Len; ++r) {
            if (before[c + r * dim1Len] == 0 && HasSetInNeighborhood(before.data(), dim1Len, dim2Len, c, r, radius1, radius2)) {
                img[c + r * dim1Len] = 1;
            }
        }
    }
}

// Image_Erosion @ 0x6ba652ac
void Erosion(int16_t* img, int dim1Len, int dim2Len, int radius1, int radius2) {
    Copy before;
    Snapshot(before, img, dim1Len, dim2Len);
    for (int r = 0; r < dim2Len; ++r) {
        for (int c = 0; c < dim1Len; ++c) {
            if (before[c + r * dim1Len] == 0) ClearNeighborhood(img, dim1Len, dim2Len, c, r, radius1, radius2);
        }
    }
}

// Image_CrossErosion @ 0x6ba65625
void CrossErosion(int16_t* img, int dim1Len, int dim2Len, bool alongDim1, bool alongDim2) {
    Copy before;
    Snapshot(before, img, dim1Len, dim2Len);
    if (alongDim1) CrossErosionDim1(img, before.data(), static_cast<uint32_t>(dim1Len), dim2Len);
    if (alongDim2) CrossErosionDim2(img, before.data(), static_cast<uint32_t>(dim1Len), dim2Len);
}

}  // namespace Solvers::Tsa::Core::Morphology

extern "C" {
namespace M = Solvers::Tsa::Core::Morphology;
void Image_Dilation(void* img, int dim1Len, int dim2Len, uint32_t radius1, uint32_t radius2) {
    M::Dilation(static_cast<int16_t*>(img), dim1Len, dim2Len, static_cast<int>(radius1), static_cast<int>(radius2));
}
void Image_Erosion(void* img, int dim1Len, int dim2Len, uint32_t radius1, uint32_t radius2) {
    M::Erosion(static_cast<int16_t*>(img), dim1Len, dim2Len, static_cast<int>(radius1), static_cast<int>(radius2));
}
void Image_CrossErosion(void* img, int dim1Len, int dim2Len, int alongDim1, int alongDim2) {
    M::CrossErosion(static_cast<int16_t*>(img), dim1Len, dim2Len, alongDim1 != 0, alongDim2 != 0);
}
}  // extern "C"
