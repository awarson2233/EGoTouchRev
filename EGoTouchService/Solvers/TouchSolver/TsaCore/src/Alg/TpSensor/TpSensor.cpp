#include "Alg/TpSensor/TpSensor.h"

#include <algorithm>

#include "Alg/Sensor/Sensor.h"
#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Common/Assert.h"
#include "Common/Bss.h"
#include "Common/Memory.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::TpSensor {

namespace {

constexpr uint32_t kSysNoBrokenSensorComp = 0x80000;  // TsaStatic::systemFlags：本帧不做坏点补偿
constexpr uint8_t kMaxBrokenSensorNum = 100;
constexpr uint16_t kUnitDifScale = 0x400;              // difScale 1.0

struct BrokenSensor {
    uint8_t dim1;
    uint8_t dim2;
    uint8_t type;
};

/// g_tpSensorBrokenList：本帧报上来的坏传感器。
struct BrokenSensorList {
    uint8_t count;
    BrokenSensor entry[kMaxBrokenSensorNum];
};
static_assert(sizeof(BrokenSensorList) == 1 + 3 * kMaxBrokenSensorNum);

uint8_t*& FlagBuf() { return Slot<uint8_t*>(Bss().tpSensorFlag); }
int8_t*& Dim1OffsetBuf() { return Slot<int8_t*>(Bss().tpSensorDim1Offset); }
int8_t*& Dim2OffsetBuf() { return Slot<int8_t*>(Bss().tpSensorDim2Offset); }
uint16_t*& DifScaleBuf() { return Slot<uint16_t*>(Bss().tpSensorDifScale); }
BrokenSensorList& Broken() { return Slot<BrokenSensorList>(Bss().tpSensorBrokenList); }

uint16_t Cell(int dim1, int dim2) { return static_cast<uint16_t>(PrmtFlash().dim1Len * dim2 + dim1); }

// TPSensor_IsCompensationNeeded @ 0x6ba6c3bf
bool IsCompensationNeeded() {
    return PrmtFlash().brokenSensorCompensation != 0 && (Static().systemFlags & kSysNoBrokenSensorComp) == 0;
}

// TPSensor_UpdateBrokenSensorTable @ 0x6ba6bb78
void UpdateBrokenSensorTable() {
    uint8_t* flags = FlagBuf();
    for (uint16_t i = 0; i < PrmtFlash().sensorNum; ++i) flags[i] &= static_cast<uint8_t>(~kFlagBrokenMask);
    const BrokenSensorList& list = Broken();
    for (int k = 0; k < list.count; ++k) {
        const BrokenSensor& s = list.entry[k];
        flags[Cell(s.dim1, s.dim2)] |= s.type & kFlagBrokenMask;
    }
}

// TPSensor_ProcessSensor @ 0x6ba6c0e5
// 8 个方向上「近邻 × 2 − 次近邻」的线性外推取平均，再除以 2；上限为近邻最大值的 5/4，下限 0。
// 没有一个方向的两格都有效时置 0。
void ProcessSensor(const BrokenSensor& s) {
    int16_t* dif = Buffers().dif;
    int sum = 0;
    int n = 0;
    int16_t maxNear = INT16_MIN;
    for (int d1 = -1; d1 < 2; ++d1) {
        for (int d2 = -1; d2 < 2; ++d2) {
            if (d1 == 0 && d2 == 0) continue;
            const int nearDim1 = s.dim1 + d1;
            const int nearDim2 = s.dim2 + d2;
            const int farDim1 = s.dim1 + d1 * 2;
            const int farDim2 = s.dim2 + d2 * 2;
            if (!IsSensorValid(nearDim1, nearDim2) || !IsSensorValid(farDim1, farDim2)) continue;
            const int16_t nearDif = dif[Cell(nearDim1, nearDim2)];
            sum += (nearDif * 2 - dif[Cell(farDim1, farDim2)]) * 0x100 / 0x80;
            ++n;
            maxNear = std::max(maxNear, nearDif);
        }
    }
    const uint16_t cell = Cell(s.dim1, s.dim2);
    if (n == 0) {
        dif[cell] = 0;
        return;
    }
    int v = sum / n * 8 / 16;
    const auto cap = static_cast<int16_t>(maxNear * 5 / 4);
    if (cap < v) v = cap;
    if (v < 0) v = 0;
    dif[cell] = static_cast<int16_t>(v);
}

// TPSensor_CompensateBrokenSensor @ 0x6ba6c403
void CompensateBrokenSensor() {
    if (!IsCompensationNeeded() || Broken().count >= kMaxBrokenSensorNum) return;
    UpdateBrokenSensorTable();
    const BrokenSensorList& list = Broken();
    for (int k = 0; k < list.count; ++k) ProcessSensor(list.entry[k]);
}

// TPSensor_TranslateRaw @ 0x6ba6c6c7
void TranslateRaw() {
    const FlashPrmt& fl = PrmtFlash();
    int16_t* raw = Buffers().raw;
    for (uint16_t k = 0; k < fl.sensorTranslateNum; ++k) {
        const SensorTranslate& t = fl.sensorTranslate[k];
        const uint16_t cell = Cell(t.dim1, t.dim2);
        raw[cell] = static_cast<int16_t>(raw[cell] + t.rawOffset);
    }
}

// TPSensor_ScalingDif @ 0x6ba6c52c
void ScalingDif() {
    const FlashPrmt& fl = PrmtFlash();
    int16_t* dif = Buffers().dif;
    for (int k = 0; k < fl.sensorTranslateNum; ++k) {
        const SensorTranslate& t = fl.sensorTranslate[k];
        const uint16_t cell = Cell(t.dim1, t.dim2);
        dif[cell] = static_cast<int16_t>(t.difScale * dif[cell] / 1024);
    }
}

}  // namespace

// TPSensor_Malloc @ 0x6ba6b9b0
bool Malloc() {
    FlagBuf() = static_cast<uint8_t*>(Mem::Allocate(GridBufSize()));
    if (FlagBuf() == nullptr) return false;
    Dim1OffsetBuf() = static_cast<int8_t*>(Mem::Allocate(GridBufSize()));
    if (Dim1OffsetBuf() == nullptr) return false;
    Dim2OffsetBuf() = static_cast<int8_t*>(Mem::Allocate(GridBufSize()));
    if (Dim2OffsetBuf() == nullptr) return false;
    DifScaleBuf() = static_cast<uint16_t*>(Mem::Allocate(GridBufSize() * 2u));
    return DifScaleBuf() != nullptr;
}

// TPSensor_Free @ 0x6ba6ba80
void Free() {
    Mem::Release(FlagBuf());
    Mem::Release(Dim1OffsetBuf());
    Mem::Release(Dim2OffsetBuf());
    Mem::Release(DifScaleBuf());
}

// TPSensor_Init @ 0x6ba6be2e
void Init() {
    const FlashPrmt& fl = PrmtFlash();
    ClearChecked(Dim1OffsetBuf(), GridBufSize(), fl.sensorNum);
    ClearChecked(Dim2OffsetBuf(), GridBufSize(), fl.sensorNum);
    for (int k = 0; k < fl.sensorOffsetNum; ++k) {
        const SensorOffset& o = fl.sensorOffset[k];
        if ((o.flags & 1) == 0) continue;
        const uint16_t cell = Cell(o.dim1, o.dim2);
        Dim1OffsetBuf()[cell] = o.dim1Offset;
        Dim2OffsetBuf()[cell] = o.dim2Offset;
    }
    uint16_t* scale = DifScaleBuf();
    for (int i = 0; i < fl.sensorNum; ++i) scale[i] = kUnitDifScale;
    for (int k = 0; k < fl.sensorTranslateNum; ++k) {
        const SensorTranslate& t = fl.sensorTranslate[k];
        scale[Cell(t.dim1, t.dim2)] = t.difScale;
    }
}

// TPSensor_Reset @ 0x6ba6bb6b
void Reset() { Broken().count = 0; }

// TPSensor_ProcessRaw @ 0x6ba6c910
void ProcessRaw() { TranslateRaw(); }

// TPSensor_Process @ 0x6ba6c929
void Process() {
    CompensateBrokenSensor();
    ScalingDif();
}

uint8_t* Flags() noexcept { return FlagBuf(); }
const int8_t* Dim1Offset() noexcept { return Dim1OffsetBuf(); }
const int8_t* Dim2Offset() noexcept { return Dim2OffsetBuf(); }

// TPSensor_IsSensorButNotNotch @ 0x6ba6bd35
bool IsSensorButNotNotch(int dim1, int dim2) {
    const FlashPrmt& fl = PrmtFlash();
    if (static_cast<uint8_t>(dim1) >= fl.dim1Len || static_cast<uint8_t>(dim2) >= fl.dim2Len) return false;
    return (FlagBuf()[Cell(dim1, dim2)] & kFlagNotch) == 0;
}

// TPSensor_IsSensorValid @ 0x6ba6bdc1
bool IsSensorValid(int dim1, int dim2) {
    return IsSensorButNotNotch(dim1, dim2) && (FlagBuf()[Cell(dim1, dim2)] & kFlagBrokenMask) == 0;
}

// TPSensor_AddToBrokenSensorList @ 0x6ba6babf
bool AddToBrokenSensorList(uint8_t dim1, uint8_t dim2, uint8_t type) {
    BrokenSensorList& list = Broken();
    if (list.count >= kMaxBrokenSensorNum) return false;
    list.entry[list.count] = {dim1, dim2, type};
    ++list.count;
    return true;
}

// TPSensor_IsBrokenSensorInTouchZone @ 0x6ba6c46f
bool IsBrokenSensorInZone(uint8_t mapTag) {
    if (!IsCompensationNeeded()) return false;
    const BrokenSensorList& list = Broken();
    for (uint8_t k = 0; k < list.count; ++k) {
        if (Sensor::IsInZone(list.entry[k].dim1, list.entry[k].dim2, mapTag)) return true;
    }
    return false;
}

// TPSensor_RevertScalingDif @ 0x6ba6c635
int16_t RevertScalingDif(int16_t dif, uint8_t dim1, uint8_t dim2) {
    const uint16_t scale = DifScaleBuf()[Cell(dim1, dim2)];
    if (scale == 0) return dif;
    return static_cast<int16_t>(dif * 1024 / scale);
}

// TPSensor_RevertTranslateRaw @ 0x6ba6c7c5
void RevertTranslateRaw(int16_t* raw, uint32_t n) {
    if (raw == nullptr) return;
    const FlashPrmt& fl = PrmtFlash();
    for (uint16_t k = 0; k < fl.sensorTranslateNum; ++k) {
        const SensorTranslate& t = fl.sensorTranslate[k];
        const uint16_t cell = Cell(t.dim1, t.dim2);
        if (cell < n) raw[cell] = static_cast<int16_t>(raw[cell] - t.rawOffset);
    }
}

}  // namespace Solvers::Tsa::Core::TpSensor
