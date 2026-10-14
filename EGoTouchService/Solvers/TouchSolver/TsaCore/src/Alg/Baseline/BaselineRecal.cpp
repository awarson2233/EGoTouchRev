// 系统复位后的重新校准判定（原 Baseline/BLRecal.c）与整屏偏移补偿（原 Baseline/Baseline.c）。

#include <cstdlib>

#include "Alg/Baseline/Baseline.h"
#include "Alg/Baseline/BaselineInternal.h"
#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Legacy/RawApi.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::Baseline {

namespace {

constexpr uint8_t kRecalSettleFrames = 5;   // 复位后先跳过的帧数
constexpr uint8_t kRecalCheckFrames = 15;   // 到这一帧为止统计越界
constexpr uint8_t kRecalMinHits = 3;        // 越界帧数达到它即请求重新校准
constexpr uint64_t kAutoCalMinMs = 300000;  // 自动校准开始 5 分钟后才允许退出
constexpr uint32_t kSysReset = 0x1;
constexpr uint32_t kCtrlRecal = 0x2;        // TsaStatic::specialControlFlags
constexpr uint32_t kCtrlCompensated = 0x100;
constexpr int kCompensateMin = 5;           // 偏移超过 4 才补偿
constexpr int16_t kCompensateStep = 5;

bool AutoCalibration() { return static_cast<char>(AFE_IsAutoCalibration()) != 0; }

// raw 的上下边界越出 [center - range/2, center + range/2]，或上下边界之差超过 range。
bool BoundaryOut() {
    const TsaPrpt& prpt = Prpt();
    const FlashPrmt& fl = PrmtFlash();
    const uint32_t half = fl.recalRawRange >> 1;
    return static_cast<int>(fl.recalRawRange) < prpt.boundaryPositive - prpt.boundaryNegative ||
           half + fl.recalRawCenter < prpt.boundaryPositive ||
           static_cast<int>(prpt.boundaryNegative) < static_cast<int>(fl.recalRawCenter - half);
}

}  // namespace

namespace Recal {

// BLRecal_Init @ 0x6ba47430
void Init() {
    TsaStatic& stat = Static();
    stat.recalFrames = 0xff;
    stat.recalBoundaryHits = 0;
}

// BLRecal_Process @ 0x6ba47458
// 系统复位后第 5..14 帧里，无触摸且 raw 边界越界的帧数达到 3 帧时（非自动校准）请求重新校准；
// 自动校准超过 5 分钟、AFE 请求重新校准且基线已稳定更新时退出特殊控制模式。
void Process() {
    TsaStatic& stat = Static();
    const TsaPrpt& prpt = Prpt();
    stat.justExitedSpecialCtrlMode = 0;
    if ((stat.systemFlags & kSysReset) != 0) {
        stat.recalFrames = 0;
        stat.recalBoundaryHits = 0;
    }
    if (stat.recalFrames < kRecalSettleFrames) {
        ++stat.recalFrames;
    } else if (stat.recalFrames < kRecalCheckFrames) {
        ++stat.recalFrames;
        const int16_t finger = PrmtFlash().fingerThold;
        if (prpt.sigPeakPositive < finger && -prpt.sigPeakNegative < finger && BoundaryOut()) ++stat.recalBoundaryHits;
    } else {
        if (kRecalMinHits <= stat.recalBoundaryHits && !AutoCalibration()) {
            stat.justExitedSpecialCtrlMode = 1;
            stat.specialControlFlags |= kCtrlRecal;
        }
        stat.recalBoundaryHits = 0;
    }
    if (AutoCalibration() && kAutoCalMinMs < stat.curTimeMs - stat.autoCalibrationStartTime && AFE_IsRecalRequested() &&
        stat.blUpdatedThisFrame != 0 && 2 < static_cast<uint16_t>(prpt.blStageFrames)) {
        stat.justExitedSpecialCtrlMode = 1;
    }
}

}  // namespace Recal

// Baseline_Compensation @ 0x6ba46d00
// 负向偏移优先；偏移超过 4 且没有 raw 跳变时，基线整体向偏移方向移 5（信号反向时反过来）。
void Compensation() {
    const TsaPrpt& prpt = Prpt();
    TsaStatic& stat = Static();
    int16_t offset = 0;
    if (prpt.field302 < 0) {
        offset = prpt.field302;
    } else if (0 < prpt.field300) {
        offset = prpt.field300;
    }
    if (std::abs(offset) < kCompensateMin || stat.rawJumpLatched != 0) return;
    const int16_t step = offset < 1 ? static_cast<int16_t>(-kCompensateStep) : kCompensateStep;
    int16_t* bl = Buffers().bl;
    const uint16_t n = PrmtFlash().sensorNum;
    for (uint16_t i = 0; i < n; ++i) bl[i] = static_cast<int16_t>(stat.signalReverted == 0 ? bl[i] - step : bl[i] + step);
    stat.specialControlFlags |= kCtrlCompensated;
}

}  // namespace Solvers::Tsa::Core::Baseline

// raw 的 CMF 还在调用的入口
extern "C" void Baseline_Compensation(void) { Solvers::Tsa::Core::Baseline::Compensation(); }