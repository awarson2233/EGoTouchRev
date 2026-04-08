#pragma once
#include "AsaTypes.h"

namespace Asa {

/// Pitch-periodic polynomial compensation coefficients (from factory calibration).
/// Mirrors TSACore CoorMultiOrderFitCompensate parameters stored in Flash.
struct PitchCompensation {
    double c[4] = {0, 0, 0, 0};  // c0 + c1*x + c2*x² + c3*x³
    bool   enabled = false;
};

/// CoordinateSolver — Triangle/Gravity interpolation on 1D projections
/// Mirrors TX1CoordinateProcess + GetCoordinateByTriangleOf
class CoordinateSolver {
public:
    /// Solve coordinates from 1D projection signals
    /// @param proj  1D projections from GridPeakDetector::ProjectTo1D
    /// @param gridDimRow  Number of grid rows for clamping (default 9)
    /// @param gridDimCol  Number of grid cols for clamping (default 9)
    /// @return Coordinate in 0x400 units
    AsaCoorResult Solve(const AsaProjection& proj,
                        int gridDimRow = kGridDim,
                        int gridDimCol = kGridDim);

    // Configuration: algorithm selection
    // DAT_1820d630 bit0: 0=Triangle, 1=Gravity.
    // Gaokun flash: DAT_1820d630 = 0x0E → bit0=0 → Triangle.
    bool useTriangle = true;  // true → triangle (Gaokun default)

    // DAT_1820d630 bit3: edge compensation secondary logic enable
    // Gaokun flash: 0x0E & 8 = 8 → enabled
    bool edgeCompBit3 = true;

    // Triangle interpolation edge parameters (from g_asaPrmt flash)
    // Gaokun: Dim1=[50, 5000, 5000], Dim2=[50, 4500, 3700]
    // [0]=param1 (edge ratio factor), [1]=sumThreshold_L, [2]=sumThreshold_R
    uint16_t triParamDim1[3] = {50, 5000, 5000};
    uint16_t triParamDim2[3] = {50, 4500, 3700};

    // ── P0: Pitch-periodic polynomial compensation ──
    // Mirrors CoorMultiOrderFitCompensate from TSACore.
    // Grid-mode coefficients from Gaokun flash: DAT_1820d6C8 / DAT_1820d708
    PitchCompensation pitchCompDim1 = {
        {0.0, -1.710915149066293, 0.005959771652221362, -5.113555667385272e-6},
        true  // enabled
    };
    PitchCompensation pitchCompDim2 = {
        {0.0, -1.449572649572650, 0.006942992120726496, -3.739316239316239e-6},
        true  // enabled
    };

    // ── P0: Gravity noise floor ──
    // Signals below this threshold are excluded from centroid calculation.
    // Mirrors the noiseFloor parameter passed to UpdateTX1GravityData.
    int32_t gravityNoiseFloor = 0;

    // ── P0: Gravity fictitious edge enable ──
    // When true, mirror-extend signals at grid boundaries to prevent
    // edge-biased centroid shift (mirrors GetFictiousEdge).
    bool gravityFictitiousEdge = true;

private:
    /// Triangle interpolation using 3 points (mid-grid)
    /// Params are int to match TSACore calling convention
    static int32_t TriangleAlgUsing3Point(int left, int center, int right);

    /// Edge compensation: creates virtual neighbor for edge interpolation
    /// Mirrors TSACore EdgeCompensating exactly
    int32_t EdgeCompensating(int peak, int n1, int n2,
                             uint16_t param1, uint16_t param5);

    /// Triangle interpolation at grid edge
    int32_t TriangleAlgEdge(int peak, int n1, int n2,
                            uint16_t param1, uint16_t param2);

    /// Solve one dimension using triangle interpolation
    int32_t SolveByTriangle(
        const int32_t* signal, int peakIdx, int len,
        const uint16_t* edgeParam);

    /// Gravity (centroid) interpolation on 1D signal
    int32_t SolveByGravity(const int32_t* signal, int len, int peakIdx);

    /// Apply pitch-periodic polynomial compensation to one coordinate
    static int32_t ApplyPitchCompensation(int32_t coor,
                                          const PitchCompensation& comp);
};

} // namespace Asa
