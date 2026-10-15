#include "App/Scenario.h"
#include "Prmt/TsaPrmt.h"

#include <array>
#include <cstring>

#include "Legacy/RawApi.h"
#include "Prmt/Prmt.h"
#include "Prmt/TuningField.h"

namespace Solvers::Tsa::Core::RtPrmt {

namespace {

using Tuning::Bounds;
using Tuning::EntrySpec;
using Tuning::FieldKind;
using Tuning::FieldRef;

#define FLASH(member) TSA_PRMT_FIELD(FlashPrmt, member)
#define DYN(member) TSA_PRMT_FIELD(DynamicPrmt, member)

constexpr FieldRef kNoMirror{0, FieldKind::kNone};

struct RtSpec {
    const char* key;  // TSA_ScenarioProcess 按它改写
    EntrySpec entry;
    FieldRef mirror;  // 同时写回的动态门限字段
};

// g_tsaRTPrmt 的各项。键名在原 DLL 里是反射描述 TsaRTPrmtStruct（0x6bb4fcc0），这里与其余描述合在一起。
constexpr std::array<RtSpec, kRtPrmtNum> kSpecs = {{
    /*  0 */ {"fingerThold", {FLASH(fingerThold), Bounds::kAroundValue, 0, 0, "touch threshold for finger mode;\ntuning based on LTI of 4mm finger without film and 5mm finger with film in device floating situation;"}, DYN(fingerThold)},
    /*  1 */ {"clickSuppressDistance", {FLASH(clickSuppressDistance), Bounds::kFixed, 100, 0, "non-axis-change related moving distance(pixel) threshold to determine click or flick action, which will be applied to determine whether to apply click suppression or not;\nthe more threshold is the better click performance and poorer flick performance will be;"}, DYN(clickSuppressDistance)},
    /*  2 */ {"maxClickSuppressDistance", {FLASH(maxClickSuppressDistance), Bounds::kFixed, 150, 0, "moving distance threshold(pixel) to determine whether to apply click suppression or not;\nthe more threshold is the better click performance and poorer flick performance will be;"}, DYN(maxClickSuppressDistance)},
    /*  3 */ {"sigMult", {FLASH(sigMult), Bounds::kFixed, 96, 16, "128 scale up ratio to peak signal to determine touch zone traversal threshold for default situation;\nthe more threshold is the better multi-finger performance and poorer fat-finger performance will be;"}, kNoMirror},
    /*  4 */ {"sigMultNormal", {FLASH(sigMultNormal), Bounds::kFixed, 96, 48, "128 scale up ratio to peak signal to determine touch zone traversal threshold for device grounding situation;\nthe more threshold is the better multi-finger performance and poorer fat-finger performance will be;"}, kNoMirror},
    /*  5 */ {"sigMultSD", {FLASH(sigMultSD), Bounds::kFixed, 96, 16, "128 scale up ratio to peak signal to determine touch zone traversal threshold in case device floating detected;\nthe more threshold is the better multi-finger performance and poorer fat-finger performance will be;"}, kNoMirror},
    /*  6 */ {"accThold", {FLASH(accThold), Bounds::kFixed, 24, 8, "acceleration threshold to determine whether to do line connection between 2 touches;\nthe more threshold is the better line drawing performance and poorer 2 closed finger tapping performance will be;"}, kNoMirror},
    /*  7 */ {"dbEnterIDLE", {FLASH(dbEnterIdle), Bounds::kFixed, 60000, 0, "debounce time threshold in ms to enter IDLE mode after being allowed;\nthe more threshold is the better response time but more power will be consumed;"}, kNoMirror},
    /*  8 */ {"sigRatioToDirectReport", {FLASH(sigRatioToDirectReport), Bounds::kFixed, 48, 16, "16 scale up ratio to touch threshold to determine whether to report touch directly or debounce the touch;\nthe less threshold is the better response time but more sensitive to noise;"}, DYN(sigRatioToDirectReport)},
    /*  9 */ {"distToEnterGloveMode", {FLASH(distToEnterGloveMode), Bounds::kFixed, 30, 0, "distance threshold in mm to activate glove mode;\nthe less threshold is the easier to enter glove mode and the higher possibility for ghost touch;"}, DYN(distToEnterGloveMode)},
    /* 10 */ {"clickDuration", {FLASH(clickDuration), Bounds::kFixed, 60000, 0, "time threshold in ms to allowed moving after click;\nthe less threshold is the easier for move and the higher for better click;"}, DYN(clickDuration)},
    /* 11 */ {"volumeActiveAreaStart", {FLASH(volumeActiveAreaStart), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeActiveAreaStart"}, kNoMirror},
    /* 12 */ {"volumeActiveAreaEnd", {FLASH(volumeActiveAreaEnd), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeActiveAreaEnd"}, kNoMirror},
    /* 13 */ {"volumeGreenAreaStart", {FLASH(volumeGreenAreaStart), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeGreenAreaStart"}, kNoMirror},
    /* 14 */ {"volumeGreenAreaEnd", {FLASH(volumeGreenAreaEnd), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeGreenAreaEnd"}, kNoMirror},
    /* 15 */ {"volumeSideEnabled", {FLASH(volumeSideEnabled), Bounds::kFixed, INT32_MAX, INT32_MIN, "volumeSideEnabled"}, kNoMirror},
}};

#undef FLASH
#undef DYN

// TSARTPrmt_SyncWithTTPrmt @ 0x6bb3d210
// 缺省表的当前值重新取自机型参数（调参表已在这之前写回）。
void SyncWithTtPrmt() {
    const FlashPrmt& flash = PrmtFlash();
    RtPrmtTable& defaults = PrmtRunTimeDefault();
    for (size_t k = 0; k < kRtPrmtNum; ++k) defaults[k].value = Tuning::Read(flash, kSpecs[k].entry.field);
}

// TSARTPrmt_Reset @ 0x6bb3d8fe
void Reset() {
    SyncWithTtPrmt();
    PrmtRunTime() = PrmtRunTimeDefault();
}

}  // namespace

// TSARTPrmt_Init @ 0x6bb3d3e0
void Init() {
    SyncWithTtPrmt();
    RtPrmtTable& defaults = PrmtRunTimeDefault();
    for (size_t k = 0; k < kRtPrmtNum; ++k) Tuning::Fill(defaults[k], kSpecs[k].entry, defaults[k].value);
    PrmtRunTime() = defaults;
}

// TSARTPrmt_Process @ 0x6bb3d933
// 每帧从机型参数重建，交给场景处理按键名改写，再写回机型参数与动态门限。
void Process() {
    Reset();
    Scenario::Process();
    FlashPrmt& flash = PrmtFlash();
    DynamicPrmt& dyn = PrmtDynamic();
    const RtPrmtTable& table = PrmtRunTime();
    for (size_t k = 0; k < kRtPrmtNum; ++k) {
        Tuning::Write(flash, kSpecs[k].entry.field, table[k].value);
        Tuning::Write(dyn, kSpecs[k].mirror, table[k].value);
    }
}

// TSARTPrmt_SetKeyValueWithReturn @ 0x6bb3dde9
// 原代码经 TSARTPrmt_GetKeyIndex / GetAddrOfIndexedKey / CopyMember 查表与赋值。
bool SetKeyValue(const char* key, int32_t value) {
    if (key == nullptr) return false;
    for (size_t k = 0; k < kRtPrmtNum; ++k) {
        if (std::strcmp(key, kSpecs[k].key) != 0) continue;
        TuningEntry& entry = PrmtRunTime()[k];
        entry.value = value;
        if (entry.maxValue < entry.value) entry.value = entry.maxValue;
        if (entry.value < entry.minValue) entry.value = entry.minValue;
        return true;
    }
    return false;
}

}  // namespace Solvers::Tsa::Core::RtPrmt

// ══════════════════════════════════════════════════════════════════════
// 过渡期：供尚未改写的 raw 代码调用，签名与 raw/include/TSAFuncDecls.h 一致。
// ══════════════════════════════════════════════════════════════════════

// TSARTPrmt_SetKeyValue @ 0x6bb3de92
extern "C" void TSARTPrmt_SetKeyValue(uint64_t key, uint32_t value) {
    Solvers::Tsa::Core::RtPrmt::SetKeyValue(reinterpret_cast<const char*>(key), static_cast<int32_t>(value));
}
