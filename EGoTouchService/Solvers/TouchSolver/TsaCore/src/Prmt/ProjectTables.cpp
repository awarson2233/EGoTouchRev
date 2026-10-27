// 机型参数表的装入，对应 TSAPrmt.dll!TSAFlashPrmt_LoadProject（原版经 g_prmtCallBack 调过去）。
// 表数据见 ProjectTableData.inc。

#include "Prmt/ProjectTables.h"

#include <cstring>
#include <iterator>

#include "../TsaCore.h"

#include "Common/Bss.h"

namespace Solvers::Tsa::Core::Prmt {

namespace {

constexpr size_t kFlashSize = 0x1890;
constexpr size_t kCmfPcaSize = 0x132c;
constexpr size_t kAsaSize = 0xe40;

struct ProjectTableSet {
    const uint8_t (&flash)[kFlashSize];
    const uint8_t (&cmfPca)[kCmfPcaSize];
    const uint8_t (&asa)[kAsaSize];

    [[nodiscard]] const char* Name() const noexcept { return reinterpret_cast<const char*>(flash); }
};

#include "Prmt/ProjectTableData.inc"

template <size_t N, size_t M>
void Load(std::byte (&slot)[N], const uint8_t (&table)[M]) {
    static_assert(M <= N);
    std::memcpy(slot, table, M);
}

// 机型名与某张表开头的名字相同就选那张，否则选最后一项 DFLT000000（W273AS2710 就落在这里）
const ProjectTableSet& FindProjectTables(const char* name) {
    for (const ProjectTableSet& set : kProjectTableSets) {
        if (std::strcmp(name, set.Name()) == 0) return set;
    }
    return kProjectTableSets[std::size(kProjectTableSets) - 1];
}

// FlashPrmt 里 TX1 笔坐标的 dim1 走线间距表（ASA 的 TX1CoordinateProcess 读 g_tsaPrmtFlash + 0xA0）
constexpr size_t kDim1PitchMapOffset = 0xa0;

}  // namespace

const char* LoadProjectTables(const char* name) {
    const ProjectTableSet& chosen = FindProjectTables(name);
    Load(Bss().prmtFlash, chosen.flash);
    Load(Bss().prmtFlashCmfPca, chosen.cmfPca);
    Load(Bss().prmtFlashAsa, chosen.asa);
    return chosen.Name();
}

}  // namespace Solvers::Tsa::Core::Prmt

namespace Solvers::Tsa::Core {

std::array<double, kPitchMapSize> Dim1PitchMap(const char* projectName) {
    std::array<double, kPitchMapSize> map{};
    const auto& flash = Prmt::FindProjectTables(projectName).flash;
    static_assert(Prmt::kDim1PitchMapOffset + sizeof(map) <= sizeof(flash));
    std::memcpy(map.data(), flash + Prmt::kDim1PitchMapOffset, sizeof(map));
    return map;
}

}  // namespace Solvers::Tsa::Core
