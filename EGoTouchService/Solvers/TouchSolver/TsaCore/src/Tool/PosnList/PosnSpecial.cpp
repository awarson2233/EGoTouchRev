#include "Tool/PosnList/PosnSpecial.h"

#include <cstring>

#include "App/TsaBuffer.h"

namespace Solvers::Tsa::Core::PosnSpecial {

namespace {

int16_t DifAt(uint16_t cell) { return Buffers().dif[cell]; }

bool IsPosEmpty() { return Global().posCount == 0; }
bool IsNegEmpty() { return Global().negHead == Global().capacity; }

// PosnSpecial_ResetZone @ 0x6bb420ba
void ResetZone(uint8_t idx) {
    Zone& zone = Global().zone[idx];
    zone.maxSig = INT16_MIN;
    zone.minSig = INT16_MAX;
    zone.sigSum = 0;
    zone.cellNum = 0;
}

// PosnSpecial_NextZone @ 0x6bb42163
void NextZone() {
    List& list = Global();
    if (list.zoneIdx < kZoneNum - 1) ResetZone(++list.zoneIdx);
}

// PosnSpecial_UpdateSign @ 0x6bb42265
// 当前一端取空时换到另一端，并开新区间。
void UpdateSign() {
    List& list = Global();
    if (list.sign == 1) {
        if (IsNegEmpty()) {
            NextZone();
            list.sign = 0;
        }
    } else if (IsPosEmpty()) {
        NextZone();
        list.sign = 1;
    }
}

// PosnSpecial_AddCellInZone @ 0x6bb421ae
void AddCellInZone(uint16_t cell) {
    const int16_t sig = DifAt(cell);
    Zone& zone = Global().zone[Global().zoneIdx];
    if (zone.maxSig < sig) zone.maxSig = sig;
    if (sig < zone.minSig) zone.minSig = sig;
    zone.sigSum += sig;
    ++zone.cellNum;
}

}  // namespace

// PosnSpecial_Init @ 0x6bb422b6
void Init(uint16_t* cells, uint16_t capacity) {
    List& list = Global();
    std::memset(&list, 0, sizeof list);
    list.prevSign = 1;
    list.field10 = 1;
    list.cells = cells;
    list.capacity = capacity;
    list.negHead = capacity;
    ResetZone(0);
}

// PosnSpecial_ReBase @ 0x6bb423b7
void ReBase() {
    List& list = Global();
    list.numSignChange = 0;
    list.numValidSensor = 0;
    list.numEdgeSensor = 0;
    list.sumEdgeSensor = 0;
    list.lastSign = kNoSign;
}

// PosnSpecial_Put @ 0x6bb42569
void Put(uint16_t cell) {
    List& list = Global();
    if (list.posCount >= list.negHead) return;
    if (DifAt(cell) < 0) {
        list.cells[--list.negHead] = cell;
    } else {
        list.cells[list.posCount++] = cell;
    }
}

// PosnSpecial_Get @ 0x6bb42624
uint16_t Get() {
    UpdateSign();
    List& list = Global();
    uint16_t cell;
    if (list.sign == 0) {
        cell = list.cells[--list.posCount];
        if (list.prevSign != 0) {
            ++list.posMarker;
            list.prevSign = 0;
        }
    } else {
        cell = list.cells[list.negHead++];
        if (list.prevSign != list.sign) {
            ++list.negMarker;
            list.prevSign = list.sign;
        }
    }
    AddCellInZone(cell);
    return cell;
}

// PosnSpecial_IsEmpty @ 0x6bb42761
bool IsEmpty() { return IsPosEmpty() && IsNegEmpty(); }

// PosnSpecial_Record @ 0x6bb42487
void Record(int16_t sig, int16_t thold, bool valid) {
    List& list = Global();
    if (thold < sig && valid) ++list.numValidSensor;
    const uint8_t sign = static_cast<uint16_t>(sig) >> 15;
    if (list.lastSign != sign) {
        if (list.lastSign != kNoSign) ++list.numSignChange;
        list.lastSign = sign;
    }
}

// PosnSpecial_Record2 @ 0x6bb4251d
void Record2(int16_t sig, int16_t thold) {
    List& list = Global();
    if (thold < sig) ++list.numEdgeSensor;
    list.sumEdgeSensor += sig;
}

// PosnSpecial_GetNumSignChange @ 0x6bb42444
uint8_t GetNumSignChange() { return Global().numSignChange; }
// PosnSpecial_GetNumValidSensor @ 0x6bb42455
uint8_t GetNumValidSensor() { return Global().numValidSensor; }
// PosnSpecial_GetNumEdgeSensor @ 0x6bb42466
uint8_t GetNumEdgeSensor() { return Global().numEdgeSensor; }
// PosnSpecial_GetSumEdgeSensor @ 0x6bb42477
int32_t GetSumEdgeSensor() { return Global().sumEdgeSensor; }

// PosnSpecial_GetPosMarker @ 0x6bb423f7
uint8_t GetPosMarker() {
    List& list = Global();
    if (list.posMarker == 0) list.posMarker = 1;
    return list.posMarker;
}

}  // namespace Solvers::Tsa::Core::PosnSpecial

namespace PS = Solvers::Tsa::Core::PosnSpecial;

extern "C" {
void PosnSpecial_Init(uint16_t* cells, uint16_t capacity) { PS::Init(cells, capacity); }
void PosnSpecial_Put(uint16_t cell) { PS::Put(cell); }
uint16_t PosnSpecial_Get(void) { return PS::Get(); }
char PosnSpecial_GetPosMarker(void) { return static_cast<char>(PS::GetPosMarker()); }
}  // extern "C"
