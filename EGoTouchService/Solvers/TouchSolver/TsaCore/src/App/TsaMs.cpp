// 互容主流程的 IDLE 判定（原 src/App/TSA_MS.c 中留下的部分）：握持、基线、触点异常、门限切换
// 都允许时才可以进 IDLE，否则在 TsaStatic::activeReason 里记下原因。

#include "Alg/Baseline/Baseline.h"
#include "Alg/Touch/TouchData.h"
#include "Alg/TouchThold/TouchThold.h"
#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Legacy/RawApi.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::Ms {

namespace {

constexpr uint32_t kTouchExcHw = 0x400;  // TouchTable::exceptionFlags：只有硬件异常

// TSAMS_IsReadyForIDLEMode @ 0x6ba9998b（HardwareAnalyzer 从不阻止进 IDLE）
bool IsReadyForIdleMode() {
    TsaStatic& s = Static();
    bool ready = true;
    if (TSA_IsGripOKToIDLE() == 0) {
        if (Prpt().touchMode != 0) {
            s.activeReason |= 0x40000;
            ready = false;
        }
        if (!Baseline::IsOkToIdle()) {
            ready = false;
            s.activeReason |= 0x100000;
        }
    }
    const uint32_t ex = Touches().exceptionFlags;
    if (ex != 0 && ex != kTouchExcHw) {
        ready = false;
        s.activeReason |= 0x80000;
    }
    if (Baseline::ResetInDebounce()) {
        ready = false;
        s.activeReason |= 0x200000;
    }
    if (TouchThold::IsSwitching()) {
        ready = false;
        s.activeReason |= 0x400000;
    }
    return ready;
}

}  // namespace

// TSAMS_IsOKToEnterIDLEMode @ 0x6ba99ad6
// 另外互容主流程初始化后 idleAfterInitMs 内不进 IDLE。
bool IsOkToEnterIdleMode() {
    TsaStatic& s = Static();
    const bool ready = IsReadyForIdleMode();
    if (!ready) s.activeReason |= 0x10000;
    bool ok = ready;
    if (s.curTimeMs < PrmtFlash().idleAfterInitMs + s.msInitTime) {
        ok = false;
        s.activeReason |= 0x2000000;
    }
    if (s.curTimeMs < s.msInitTime) s.msInitTime = s.curTimeMs;
    return ok;
}

}  // namespace Solvers::Tsa::Core::Ms

extern "C" {
uint8_t TSAMS_IsOKToEnterIDLEMode(void) { return Solvers::Tsa::Core::Ms::IsOkToEnterIdleMode() ? 1 : 0; }
// 折叠屏重映射被裁掉，直接是 Dif 缓冲
int16_t* TSAMS_GetDifPtr(void) { return Solvers::Tsa::Core::Buffers().dif; }
}
