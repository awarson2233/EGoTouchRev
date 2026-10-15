#pragma once
// TSACore 全部算法状态的规范化快照，用来判断两份实现（重构前后、不同构建）在同一段输入上是否
// 走到了完全相同的状态。
//
// 状态分两类内存：镜像（按原 DLL 的 VA 寻址）和 TSA_MallocAndInit 分出去的缓冲。缓冲没有固定地址，
// 按「镜像里哪个槽位指向它」定名；状态里凡是指向这两类内存的指针，都换成规范地址。这样快照与进程的
// 实际内存布局无关，两个进程的快照可以逐字节比较。

#include <cstdint>
#include <string>
#include <vector>

namespace TsaReplay {

/// 快照的规范化方式（地址标记、哪些槽位按栈指针处理）的版本
inline constexpr int kStateFormatVersion = 1;

struct StateSnapshot {
    struct Block {
        uint64_t address = 0;       // 规范地址：镜像块是原 VA，堆缓冲见 HeapAddress
        std::vector<uint8_t> bytes;
    };

    std::vector<Block> blocks;      // 按规范地址升序
    int orphanHeapBlocks = 0;       // 没有任何镜像槽位指向的缓冲，只能按分配次序定名

    /// 抓取当前状态。登记表溢出（快照不完整）时返回 false。
    bool Capture();
    uint64_t Digest() const;
    size_t TotalBytes() const;

    bool Write(const std::string& path) const;
    bool Read(const std::string& path);
};

/// 逐字节比较两份快照，把不同的区间连同符号名打到 stdout。rawRoot 是 raw 源码目录，符号名从
/// _metadata/bss_symbols.txt 和 _metadata/image_macros.txt 里取。symbolFilter 非空时只列符号名含这个子串的区间
/// （不限条数）。返回不同的字节数。
size_t DiffSnapshots(const StateSnapshot& a, const StateSnapshot& b, const std::string& rawRoot,
                     const std::string& symbolFilter = {}, int maxRanges = 40);

} // namespace TsaReplay
