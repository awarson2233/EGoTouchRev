#include "App/Afe.h"
#include "Prmt/TsaPrmt.h"

#include <cstring>

#include "Common/Assert.h"
#include "Legacy/RawApi.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::Prmt {

namespace {

constexpr uint8_t kMaxDim1 = 0x50;
constexpr uint8_t kMaxDim2 = 0x50;
constexpr uint16_t kMaxSensorNum = 0x1324;

// TSAPrmt_Validate @ 0x6bb3b9ec
void Validate() {
    const FlashPrmt& flash = PrmtFlash();
    TsaAssert(flash.dim1Len <= kMaxDim1);
    TsaAssert(flash.dim2Len <= kMaxDim2);
    TsaAssert(flash.sensorNum <= kMaxSensorNum);
}

// TSAPrmt_ReInit @ 0x6bb3b8e5
// RamPrmt 的 Init / Process 在本版本中是空函数。
void ReInit() {
    TtPrmt::Init();
    RtPrmt::Init();
    ConstPrmt::Init();
    Reset();
}

}  // namespace

// TSAPrmt_Init @ 0x6bb3b90d
// 原代码经 TSAPrmt_FlashPrmtLoadProject 装入机型参数，它总是返回成功。
bool Init() {
    TSACoreRaw_LoadProjectTables();
    ReInit();
    return true;
}

// TSAPrmt_LoadProject @ 0x6bb3b93a
bool LoadProject(const char* name) {
    if (name == nullptr) return false;
    TSACoreRaw_LoadProjectTables();
    if (std::strcmp(GetProjectName(), "DFLT000000") == 0) {
        FlashPrmt& flash = PrmtFlash();
        flash.dim1Len = static_cast<uint8_t>(Afe::ColNum());
        flash.dim2Len = static_cast<uint8_t>(Afe::RowNum());
        flash.sensorNum = static_cast<uint16_t>(flash.dim1Len * flash.dim2Len);
    }
    ReInit();
    return true;
}

// TSAPrmt_Reset @ 0x6bb3b740
void Reset() {
    DynPrmt::Init();
    Validate();
}

// TSAPrmt_PreProcess @ 0x6bb3bab7
void PreProcess() { DynPrmt::PreProcess(); }

// TSAPrmt_Process @ 0x6bb3bacb
// ConstPrmt 与 RamPrmt 的 Process 在本版本中是空函数。
void Process() {
    TtPrmt::Process();
    RtPrmt::Process();
    DynPrmt::Process();
    Validate();
}

// TSAPrmt_GetProjectName @ 0x6bb3b9df
const char* GetProjectName() { return ProjectName(); }

}  // namespace Solvers::Tsa::Core::Prmt

// ══════════════════════════════════════════════════════════════════════
// 过渡期：供尚未改写的 raw 代码调用，签名与 raw/include/TSAFuncDecls.h 一致。
// ══════════════════════════════════════════════════════════════════════

namespace Prmt = Solvers::Tsa::Core::Prmt;

extern "C" {

bool TSAPrmt_Init(void) { return Prmt::Init(); }
uint64_t TSAPrmt_LoadProject(int64_t name) { return Prmt::LoadProject(reinterpret_cast<const char*>(name)) ? 1 : 0; }
void TSAPrmt_Reset(void) { Prmt::Reset(); }
void TSAPrmt_PreProcess(void) { Prmt::PreProcess(); }
void TSAPrmt_Process(void) { Prmt::Process(); }
char* TSAPrmt_GetProjectName(void) { return const_cast<char*>(Prmt::GetProjectName()); }

}  // extern "C"
