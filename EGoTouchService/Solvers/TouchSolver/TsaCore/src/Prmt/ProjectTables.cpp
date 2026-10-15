// 机型参数表的装入，对应 TSAPrmt.dll!TSAFlashPrmt_LoadProject（原版经 g_prmtCallBack 调过去）。
// 表数据见 ProjectTableData.inc。

#include "Prmt/ProjectTables.h"

#include <cstring>
#include <iterator>

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

}  // namespace

// 机型名与某张表开头的名字相同就装那张，否则装最后一项 DFLT000000（W273AS2710 就落在这里）
const char* LoadProjectTables(const char* name) {
    const ProjectTableSet* chosen = &kProjectTableSets[std::size(kProjectTableSets) - 1];
    for (const ProjectTableSet& set : kProjectTableSets) {
        if (std::strcmp(name, set.Name()) == 0) {
            chosen = &set;
            break;
        }
    }
    Load(Bss().prmtFlash, chosen->flash);
    Load(Bss().prmtFlashCmfPca, chosen->cmfPca);
    Load(Bss().prmtFlashAsa, chosen->asa);
    return chosen->Name();
}

}  // namespace Solvers::Tsa::Core::Prmt
