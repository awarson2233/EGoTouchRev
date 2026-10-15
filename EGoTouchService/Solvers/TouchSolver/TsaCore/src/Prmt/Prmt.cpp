#include "App/Afe.h"
#include "Prmt/TsaPrmt.h"

#include <cstring>

#include "Common/Assert.h"
#include "Prmt/Prmt.h"
#include "Prmt/ProjectTables.h"

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

// TSAPrmt_LoadProject @ 0x6bb3b93a
// TSAPrmt_FlashPrmtLoadProject 对任何名字都装得出一套表（不认识的名字装 DFLT000000），总是成功。
bool LoadProject(const char* name) {
    if (name == nullptr) return false;
    LoadProjectTables(name);
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
