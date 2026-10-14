#include "StateTrace.h"

#include "TsaCore/TsaCoreApi.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace TsaReplay {

namespace {

// 规范地址的高 16 位区分三类目标；镜像块自身的地址就是原 VA，不带标记
constexpr uint64_t kImagePointerTag = 0x494D000000000000ull;   // 指向镜像：低位是 VA
constexpr uint64_t kHeapTag = 0x4850000000000000ull;           // 堆缓冲：槽位 VA << 20 | 偏移
constexpr uint64_t kOrphanTag = 0x4F52000000000000ull;         // 无主缓冲：分配次序 << 20 | 偏移
constexpr int kHeapOffsetBits = 20;                            // TSA_MallocAndInit 单块不超过 64 KB
// 指向算法状态之外的指针只保留类别：宿主模块里的函数与常量、栈上的临时量。它们的具体地址随构建而变，
// 算法也只把它们当作句柄用。
constexpr uint64_t kModulePointer = 0x4D4F000000000000ull;
constexpr uint64_t kStackPointer = 0x5354000000000000ull;

// 栈指针不能按取值范围去认：主线程的栈在 0x80_000F_xxxx 一带，相邻两个 32 位字段（一个标志字 0x80
// 加一个不大的计数）拼起来正好落进去。所以只认下面这几个确知会存栈地址的槽位：它们记的是调用方的
// 局部数组，调用返回后就是悬空值，算法不再读它。
constexpr uint64_t kStackPointerSlots[] = {
    0x6bb7f130ull,   // CMF_InitNoiseData 记下的噪声数组
    0x6bb7f138ull,   // 同上，第二个参数
    0x6bcac4c0ull,   // g_list 的表头，指向 SubTZ_Process 的局部缓冲
};

struct AddressRange {
    uintptr_t lo = 0;
    uintptr_t hi = 0;
    bool Contains(uint64_t v) const { return v >= lo && v < hi; }
};

AddressRange CurrentStack() {
    ULONG_PTR lo = 0, hi = 0;
    GetCurrentThreadStackLimits(&lo, &hi);
    return {lo, hi};
}

AddressRange HostModule() {
    const auto* base = reinterpret_cast<const uint8_t*>(GetModuleHandleW(nullptr));
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    return {reinterpret_cast<uintptr_t>(base), reinterpret_cast<uintptr_t>(base) + nt->OptionalHeader.SizeOfImage};
}

struct HeapBlock {
    const uint8_t* data = nullptr;
    uint64_t size = 0;
    uint64_t address = 0;   // 规范地址，0 = 还没找到主人
    int ordinal = 0;        // 分配次序
};

uint64_t LoadWord(const uint8_t* p) {
    uint64_t v;
    std::memcpy(&v, p, sizeof(v));
    return v;
}

uint64_t Mix(uint64_t h, uint64_t v) {
    h = (h ^ v) * 0x100000001B3ull;
    return h ^ (h >> 29);
}

struct SymbolTable {
    std::map<uint64_t, std::string> byAddress;

    void Add(uint64_t va, const std::string& name) {
        if (name.empty() || name[0] == '.') return;
        auto [it, inserted] = byAddress.emplace(va, name);
        // 同一地址有多个名字时留最像符号名的那个（DAT_/_refptr_ 是 Ghidra 的占位名）
        if (!inserted && (it->second.rfind("DAT_", 0) == 0 || it->second.rfind("_DAT_", 0) == 0)) it->second = name;
    }

    void Load(const std::string& rawRoot) {
        std::ifstream sym(rawRoot + "/_metadata/bss_symbols.txt");
        std::string line;
        while (std::getline(sym, line)) {
            unsigned long long va = 0;
            char name[256] = {};
            if (std::sscanf(line.c_str(), "0x%llx : %255s", &va, name) == 2) Add(va, name);
        }
        for (const char* header : {"/include/TSABssMemoryImage.h", "/include/TSACoreInternal.h"}) {
            std::ifstream in(rawRoot + header);
            while (std::getline(in, line)) {
                char name[256] = {};
                if (std::sscanf(line.c_str(), "#define %255s", name) != 1) continue;
                const size_t at = line.find("0x6b");
                if (at == std::string::npos) continue;
                std::string n = name;
                if (n.rfind("_refptr_", 0) == 0) n = n.substr(8);
                if (byAddress.count(std::strtoull(line.c_str() + at, nullptr, 16)) == 0) {
                    Add(std::strtoull(line.c_str() + at, nullptr, 16), n);
                }
            }
        }
    }

    std::string Describe(uint64_t va) const {
        char buf[320];
        auto it = byAddress.upper_bound(va);
        if (it == byAddress.begin()) {
            std::snprintf(buf, sizeof(buf), "0x%llx", static_cast<unsigned long long>(va));
            return buf;
        }
        --it;
        if (va == it->first) std::snprintf(buf, sizeof(buf), "%s", it->second.c_str());
        else std::snprintf(buf, sizeof(buf), "%s+0x%llx", it->second.c_str(), static_cast<unsigned long long>(va - it->first));
        return buf;
    }
};

std::string DescribeAddress(uint64_t address, const SymbolTable& symbols) {
    char buf[400];
    const uint64_t tag = address & 0xFFFF000000000000ull;
    if (tag == kHeapTag) {
        const uint64_t slot = (address & ~tag) >> kHeapOffsetBits;
        const uint64_t offset = address & ((1ull << kHeapOffsetBits) - 1);
        std::snprintf(buf, sizeof(buf), "*%s+0x%llx", symbols.Describe(slot).c_str(), static_cast<unsigned long long>(offset));
    } else if (tag == kOrphanTag) {
        std::snprintf(buf, sizeof(buf), "heap#%llu+0x%llx", static_cast<unsigned long long>((address & ~tag) >> kHeapOffsetBits),
                      static_cast<unsigned long long>(address & ((1ull << kHeapOffsetBits) - 1)));
    } else {
        std::snprintf(buf, sizeof(buf), "%s [0x%llx]", symbols.Describe(address).c_str(), static_cast<unsigned long long>(address));
    }
    return buf;
}

std::string Hex(const uint8_t* p, size_t n) {
    std::string s;
    char b[4];
    for (size_t i = 0; i < n; ++i) {
        std::snprintf(b, sizeof(b), "%02x", p[i]);
        s += b;
        if (i % 8 == 7 && i + 1 < n) s += ' ';
    }
    return s;
}

} // namespace

bool StateSnapshot::Capture() {
    blocks.clear();
    orphanHeapBlocks = 0;

    const size_t count = TSACoreRaw_StateBlocks(nullptr, 0);
    if (count == static_cast<size_t>(-1) || count == 0) return false;
    std::vector<TSACoreRaw_StateBlock> raw(count);
    if (TSACoreRaw_StateBlocks(raw.data(), raw.size()) != count) return false;

    const uint8_t* const image = raw[0].data;
    const uint64_t imageSize = raw[0].size;
    const uint64_t imageVa = raw[0].va;

    std::vector<HeapBlock> heap(count - 1);
    for (size_t i = 1; i < count; ++i) {
        heap[i - 1].data = raw[i].data;
        heap[i - 1].size = raw[i].size;
        heap[i - 1].ordinal = static_cast<int>(i - 1);
    }
    std::sort(heap.begin(), heap.end(), [](const HeapBlock& a, const HeapBlock& b) { return a.data < b.data; });
    const uintptr_t heapLo = heap.empty() ? 0 : reinterpret_cast<uintptr_t>(heap.front().data);
    const uintptr_t heapHi = heap.empty() ? 0 : reinterpret_cast<uintptr_t>(heap.back().data) + heap.back().size;

    // 落在某块缓冲内（含末尾后一位）的地址 → 那块缓冲
    const auto findHeap = [&](uint64_t v) -> HeapBlock* {
        if (v < heapLo || v > heapHi) return nullptr;
        auto it = std::upper_bound(heap.begin(), heap.end(), v, [](uint64_t value, const HeapBlock& b) {
            return value < reinterpret_cast<uintptr_t>(b.data);
        });
        if (it == heap.begin()) return nullptr;
        --it;
        return v - reinterpret_cast<uintptr_t>(it->data) <= it->size ? &*it : nullptr;
    };

    // 缓冲的主人：镜像里第一个（VA 最小的）正好指向缓冲起点的槽位
    for (uint64_t off = 0; off + 8 <= imageSize; off += 8) {
        const uint64_t v = LoadWord(image + off);
        if (v < heapLo || v >= heapHi) continue;
        HeapBlock* b = findHeap(v);
        if (b != nullptr && b->address == 0 && v == reinterpret_cast<uintptr_t>(b->data)) {
            b->address = kHeapTag | ((imageVa + off) << kHeapOffsetBits);
        }
    }
    for (HeapBlock& b : heap) {
        if (b.address != 0) continue;
        b.address = kOrphanTag | (static_cast<uint64_t>(b.ordinal) << kHeapOffsetBits);
        ++orphanHeapBlocks;
    }

    const uintptr_t imageLo = reinterpret_cast<uintptr_t>(image);
    const AddressRange stack = CurrentStack();
    const AddressRange module = HostModule();
    const auto canonical = [&](uint64_t v) -> uint64_t {
        if (v - imageLo < imageSize) return kImagePointerTag | (imageVa + (v - imageLo));
        if (const HeapBlock* b = findHeap(v)) return b->address + (v - reinterpret_cast<uintptr_t>(b->data));
        if (module.Contains(v)) return kModulePointer;
        return v;
    };
    const auto addBlock = [&](uint64_t address, const uint8_t* data, uint64_t size) {
        Block block;
        block.address = address;
        block.bytes.assign(data, data + size);
        for (uint64_t off = 0; off + 8 <= size; off += 8) {
            const uint64_t v = LoadWord(data + off);
            if (v < 0x10000) continue;
            const uint64_t c = canonical(v);
            if (c != v) std::memcpy(block.bytes.data() + off, &c, sizeof(c));
        }
        blocks.push_back(std::move(block));
    };
    addBlock(imageVa, image, imageSize);
    for (const uint64_t slot : kStackPointerSlots) {
        uint8_t* p = blocks[0].bytes.data() + (slot - imageVa);
        if (stack.Contains(LoadWord(p))) std::memcpy(p, &kStackPointer, sizeof(kStackPointer));
    }
    for (const HeapBlock& b : heap) addBlock(b.address, b.data, b.size);
    std::sort(blocks.begin(), blocks.end(), [](const Block& a, const Block& b) { return a.address < b.address; });
    return true;
}

uint64_t StateSnapshot::Digest() const {
    uint64_t h = 0x9E3779B97F4A7C15ull;
    for (const Block& b : blocks) {
        h = Mix(h, b.address);
        h = Mix(h, b.bytes.size());
        size_t off = 0;
        for (; off + 8 <= b.bytes.size(); off += 8) h = Mix(h, LoadWord(b.bytes.data() + off));
        for (; off < b.bytes.size(); ++off) h = Mix(h, b.bytes[off]);
    }
    return h;
}

size_t StateSnapshot::TotalBytes() const {
    size_t n = 0;
    for (const Block& b : blocks) n += b.bytes.size();
    return n;
}

bool StateSnapshot::Write(const std::string& path) const {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    const uint32_t magic = 0x53415354;   // "TSAS"
    const uint32_t count = static_cast<uint32_t>(blocks.size());
    out.write(reinterpret_cast<const char*>(&magic), 4);
    out.write(reinterpret_cast<const char*>(&count), 4);
    for (const Block& b : blocks) {
        const uint64_t size = b.bytes.size();
        out.write(reinterpret_cast<const char*>(&b.address), 8);
        out.write(reinterpret_cast<const char*>(&size), 8);
        out.write(reinterpret_cast<const char*>(b.bytes.data()), static_cast<std::streamsize>(size));
    }
    return static_cast<bool>(out);
}

bool StateSnapshot::Read(const std::string& path) {
    blocks.clear();
    std::ifstream in(path, std::ios::binary);
    uint32_t magic = 0, count = 0;
    in.read(reinterpret_cast<char*>(&magic), 4);
    in.read(reinterpret_cast<char*>(&count), 4);
    if (!in || magic != 0x53415354) return false;
    for (uint32_t i = 0; i < count; ++i) {
        Block b;
        uint64_t size = 0;
        in.read(reinterpret_cast<char*>(&b.address), 8);
        in.read(reinterpret_cast<char*>(&size), 8);
        if (!in || size > (1ull << 28)) return false;
        b.bytes.resize(size);
        in.read(reinterpret_cast<char*>(b.bytes.data()), static_cast<std::streamsize>(size));
        if (!in) return false;
        blocks.push_back(std::move(b));
    }
    return true;
}

size_t DiffSnapshots(const StateSnapshot& a, const StateSnapshot& b, const std::string& rawRoot,
                     const std::string& symbolFilter, int maxRanges) {
    SymbolTable symbols;
    symbols.Load(rawRoot);

    size_t differing = 0;
    int ranges = 0;
    std::map<std::string, size_t> perSymbol;
    const auto symbolOf = [&](uint64_t address) {
        std::string s = DescribeAddress(address, symbols);
        const size_t cut = s.find_first_of("+ ");
        return cut == std::string::npos ? s : s.substr(0, cut);
    };

    size_t ia = 0, ib = 0;
    while (ia < a.blocks.size() || ib < b.blocks.size()) {
        const bool onlyA = ib == b.blocks.size() || (ia < a.blocks.size() && a.blocks[ia].address < b.blocks[ib].address);
        const bool onlyB = !onlyA && (ia == a.blocks.size() || b.blocks[ib].address < a.blocks[ia].address);
        if (onlyA || onlyB) {
            const auto& blk = onlyA ? a.blocks[ia++] : b.blocks[ib++];
            std::printf("block only in %s: %s (%zu bytes)\n", onlyA ? "A" : "B", DescribeAddress(blk.address, symbols).c_str(),
                        blk.bytes.size());
            differing += blk.bytes.size();
            continue;
        }
        const auto& ba = a.blocks[ia++];
        const auto& bb = b.blocks[ib++];
        if (ba.bytes.size() != bb.bytes.size()) {
            std::printf("block size differs: %s A=%zu B=%zu\n", DescribeAddress(ba.address, symbols).c_str(), ba.bytes.size(),
                        bb.bytes.size());
        }
        const size_t n = std::min(ba.bytes.size(), bb.bytes.size());
        for (size_t i = 0; i < n;) {
            if (ba.bytes[i] == bb.bytes[i]) {
                ++i;
                continue;
            }
            // 把相邻（间隔不到 8 字节）的差异并成一段
            size_t end = i + 1, last = i;
            while (end < n && end - last <= 8) {
                if (ba.bytes[end] != bb.bytes[end]) last = end;
                ++end;
            }
            const size_t len = last - i + 1;
            for (size_t k = i; k <= last; ++k) {
                if (ba.bytes[k] != bb.bytes[k]) {
                    ++differing;
                    ++perSymbol[symbolOf(ba.address + k)];
                }
            }
            const bool shown = symbolFilter.empty() ? ranges < maxRanges
                                                    : symbolOf(ba.address + i).find(symbolFilter) != std::string::npos;
            ++ranges;
            if (shown) {
                const size_t show = std::min<size_t>(len, 24);
                std::printf("%-58s len=%-5zu A=%s%s\n%-58s           B=%s%s\n", DescribeAddress(ba.address + i, symbols).c_str(), len,
                            Hex(ba.bytes.data() + i, show).c_str(), len > show ? "..." : "", "",
                            Hex(bb.bytes.data() + i, show).c_str(), len > show ? "..." : "");
            }
            i = last + 1;
        }
    }
    if (differing == 0) {
        std::printf("states are identical (%zu bytes in %zu blocks)\n", a.TotalBytes(), a.blocks.size());
        return 0;
    }
    std::printf("\n%zu bytes differ in %d ranges; by symbol:\n", differing, ranges);
    std::vector<std::pair<std::string, size_t>> sorted(perSymbol.begin(), perSymbol.end());
    std::sort(sorted.begin(), sorted.end(), [](const auto& x, const auto& y) { return x.second > y.second; });
    for (size_t i = 0; i < sorted.size() && i < 40; ++i) std::printf("  %8zu  %s\n", sorted[i].second, sorted[i].first.c_str());
    return differing;
}

} // namespace TsaReplay
