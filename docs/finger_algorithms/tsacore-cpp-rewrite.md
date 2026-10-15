# TSACore 的 C++ 实现：状态布局、比对方法与改写规则

## 概念

- **TsaCore**：原厂 `TSACore.dll` 的 C++ 复刻，源码在 [TsaCore/src](/EGoTouchService/Solvers/TouchSolver/TsaCore/src)，编成静态库 `TsaCore`。算法细节（判定条件、阈值、运算顺序、整数位宽与符号、截断与舍入点）与原 DLL 逐位一致；性能优化交给编译器。
- **原 DLL / Ghidra**：语义的最终依据是 Ghidra 里的 `TSACore.dll` 反汇编。代码里每个函数定义上方一行注释记原名和地址（`// IDT_Process @ 0x6ba6...`），对照时直接跳过去。
- **参考版本**：git 标签 `tsa-reference` 指向的提交，比对时在 `build/tsa-reference/<提交>` 下的 git worktree 里单独构建，作为金标准。
- **.bss 布局**：原 DLL 的 .bss（VA `0x6bb7f000..0x6bcad000`）在 C++ 里是一个结构 `BssLayout`，各模块的全局状态是其中按原地址排列的槽。
- **状态摘要**：每喂一帧之后，TSACore 全部算法状态（.bss + `TSA_MallocAndInit` 分配的堆缓冲）规范化后的哈希，定位差异时用。

## 架构

比对链路：每一步改动都要让当前构建与参考版本在全部录制、全部场景下的输出逐字节相同。

```mermaid
flowchart LR
    REC["录制 .dvrbin"] --> SC["场景<br/>plain / idle / pen-write<br/>（-Full 再加 charger / pen-hover / stylus）"]
    SC --> REF["参考版本 TsaRawReplay<br/>tag tsa-reference"]
    SC --> CUR["当前构建 TsaRawReplay"]
    REF --> CMP{"CSV 触点<br/>stderr 闭环与笔诊断"}
    CUR --> CMP
    CMP -- 全部相同 --> PASS["通过，提交这一步"]
    CMP -- 不同且布局相同 --> DIFF["-State：逐帧状态摘要<br/>第一处不同按符号名列字段"]
```

库内的状态分三类：.bss 里的模块状态、带初值的模块全局对象（原 .data / .rdata）、初始化时分配的堆缓冲。模块只经自己的访问函数取状态。

```mermaid
flowchart TB
    HOST["TsaHostShim / TsaRawReplay"] -- "TSA_* 导出" --> MOD
    subgraph LIB["静态库 TsaCore"]
        MOD["模块<br/>Solvers::Tsa::Core::模块"]
        ACC["访问函数<br/>Touches() / PrmtFlash() / Static() ..."]
        BSS[("g_bss : BssLayout<br/>原 .bss 排布")]
        DATA["模块全局对象<br/>g_touches 等当前数据集指针<br/>模式切换表、kLn、kAppList"]
        HEAP[("堆缓冲<br/>Mem::MallocAndInit")]
        MOD --> ACC
        ACC -- "Slot&lt;T&gt;(Bss().槽)" --> BSS
        ACC --> DATA
        ACC --> HEAP
    end
    REPLAY["TsaCore_StateBlocks"] -. ".bss 按原 VA + 堆块" .-> BSS
```

## 分点解释

### 1. 比对脚本 Compare-Reference.ps1

[Compare-Reference.ps1](/Tools/TsaRawReplay/Compare-Reference.ps1) 构建当前树（默认 preset `arm64-Debug-TsaCore`），参考版本只在第一次用到某个提交时构建，其回放结果按录制与场景缓存。每段录制默认跑三个场景：`plain`（原样回放并执行 AFE 闭环）、`idle`（中途注入 IDLE 退出并保持静止）、`pen-write`（注入模拟笔，覆盖笔相关的手指侧逻辑）；`-Full` 再加 `charger`、`pen-hover` 和经 StylusPipeline 的真实笔 `stylus`。比较 [TsaRawReplay](/Tools/TsaRawReplay/main.cpp) 的两样输出：stdout 的逐帧触点 CSV、stderr 的闭环请求与笔诊断。

只比输出、不比内部状态，所以状态布局可以随重构改变。`-State` 另外比较逐帧状态摘要：第一次不同的那一帧让两边各自 `--state-dump`，再用 `--state-diff` 按 [bss_symbols.txt](/EGoTouchService/Solvers/TouchSolver/raw/_metadata/bss_symbols.txt) 与 [image_macros.txt](/EGoTouchService/Solvers/TouchSolver/raw/_metadata/image_macros.txt) 把差异落到符号名和偏移上（如 `g_prevTouchesGrid+0x21d`），只在两边布局相同时有意义。`-Only` 按名字子串筛选运行项，`-Recordings` 接受多个文件或目录，`-Coverage` 改为统计覆盖率（写到 `build/tsa-coverage`）。

参考版本只在算法行为**有意**改变时挪动：对齐原 DLL 的修正单独提交，确认效果后把 `tsa-reference` 挪到该提交；纯重构的提交不挪标签。

### 2. 状态布局

```mermaid
flowchart LR
    GEN["bss_symbols.txt<br/>原地址"] --> H["Common/Bss.h<br/>struct BssLayout { std::byte 槽[大小]; ... }"]
    H --> SA["static_assert<br/>每个槽的偏移 = 原 VA − 0x6bb7f000"]
    MODS["模块访问函数"] -- "Slot&lt;T, 偏移&gt;(Bss().槽)" --> H
    H -- "static_assert(sizeof T ≤ 槽)" --> MODS
```

- **为什么 .bss 整体保持原排布**：原代码有依赖相邻对象的越界读写，落点取决于对象之间的相对位置。例如新按下的触点历史下标为 `kNoTouch`（20），代码照样按它读写历史表第 20 个单元，落在 `g_prevTouchesGrid` 之后的 `g_prevTouchInvalidID` 与 `g_touchesGrid` 上（Debounce、EdgeRecovery、HotZoneAdjust、TouchAction、HandGesture、AntiTouch、GripFilter、FiException 等都有这种访问）；子区域下标超过 `kMaxUnitNum` 时 `SubTouchZone::UnitAt` 读到 `g_subTZ` 之后的 `g_pkAccessedFlagSubTZ`、`g_tzAccess`。所以 [Bss.h](/EGoTouchService/Solvers/TouchSolver/TsaCore/src/Common/Bss.h) 把 .bss 定义成一个结构，槽按原地址排列，没有模块用到的区间是 `reserved`；`g_bss` 在 [Bss.cpp](/EGoTouchService/Solvers/TouchSolver/TsaCore/src/Common/Bss.cpp) 里定义，静态零初始化。
- **槽与类型**：槽是 `std::byte` 数组，模块用 `Slot<T>(Bss().槽)`（或带偏移的 `Slot<T, 0x98>`，用于落在某个大对象内部的成员，如 `g_tsaBufTz` 在缓冲表 `bufferTable` 的 `+0x98`）取得类型化引用；`Slot` 用 `static_assert` 检查 `T` 放得进槽。状态类型是平凡结构体，字节数组隐式创建其中的对象。每个结构体用 `static_assert` 校验具名字段的偏移和总大小，没人用到的字节保持 `reserved`。
- **槽的大小**：槽的位置来自符号表，大小取「访问它的类型的 sizeof」与「到下一个符号的距离」的较大者；跨多个符号的模块状态结构体（`HardwareAnalyzer::Counters`、`PressureDrift`/`RfNoise`/`ToeSynaBl` 的 `State`、Afe 的 `Slots`、`HandBuffers`）占住整个结构体的范围。新增状态时从相邻的 `reserved` 区间里切出一个槽，放在它的原地址上。
- **原 .data / .rdata**：带初值的对象改成所在模块的全局对象，初值写成 C++ 初始化器：当前数据集指针 `g_touches` / `g_prevTouches` / `g_peaks` / `g_prevPeaks`（[DataSwitch.cpp](/EGoTouchService/Solvers/TouchSolver/TsaCore/src/App/DataSwitch.cpp)，初值指向 Grid 套，`ToHand` / `ToGrid` 切换）、模式切换防抖表与优先级表（[TouchMode.cpp](/EGoTouchService/Solvers/TouchSolver/TsaCore/src/Alg/TouchMode/TouchMode.cpp)）、`g_thenarExternal`、Sys 的两个一次性标志；只读表改成 `constexpr`（CTD 的 `kLn`、HotZoneAdjust 的 `kAppList`）。它们与 .bss 的相对位置没有任何代码依赖。`Static()` 恒指向 `staticGrid`，原 DLL 里那个指针从不切换。
- **机型参数表**：W273AS2700 的 FlashPrmt、CMF PCA、ASA 表在 [ProjectTables.cpp](/EGoTouchService/Solvers/TouchSolver/TsaCore/src/Prmt/ProjectTables.cpp) 里是常量数组，`Prmt::LoadProject` 时拷进各自的 .bss 槽（原 DLL 同样是运行期装入 .bss）。
- **单例与初始化**：整个进程只有一套状态，模块函数不传上下文参数。没有显式的状态初始化入口：.bss 零初始化、全局对象带初值，与原 DLL 加载时一致；宿主直接调用 `TSA_InitAFEProject_UI`。
- **整对象清零**：用 `ZeroFill`（对应原代码的 `memset_s(&obj, sizeof obj, 0, sizeof obj)`），填充字节也是状态，`obj = {}` 不保证清零填充。

### 3. 状态摘要的规范化

[StateTrace.cpp](/Tools/TsaRawReplay/StateTrace.cpp) 经 `TsaCore_StateBlocks`（[Memory.cpp](/EGoTouchService/Solvers/TouchSolver/TsaCore/src/Common/Memory.cpp)）取得 .bss（`va` 为 `0x6bb7f000`）与全部存活的堆缓冲，然后：

- .bss 按原 VA 寻址；堆缓冲以「.bss 里第一个指向它的槽位」命名，与分配地址无关。
- 指向 .bss 或堆的指针改写成规范地址；指向宿主模块代码与常量（包括原 .data / .rdata 对象）的指针只保留类别；确知存放栈地址的三个槽位（CMF 噪声数组的两个参数、`g_list` 表头）只保留类别。栈指针不按取值范围识别，因为两个相邻 32 位字段（标志字 0x80 加一个计数）拼起来正好落在主线程栈的地址区间里。

### 4. 编译选项

| 选项 | 原因 |
|------|------|
| `-fno-strict-aliasing` `-fwrapv` | 原代码依赖类型双关（同一块状态按不同类型读写）与有符号回绕 |
| `-ffp-contract=off` | 原 DLL 是 x64，没有乘加融合；ARM64 默认融合会让浮点中间量差出末位 |
| `-ftrivial-auto-var-init=zero` | 原代码会把未赋值的局部量（结构体尾部填充）拷进状态，不清零时状态随栈上残留变化 |

这组选项是不同优化级别、不同堆布局下输出逐帧相同的前提；更深的性能优化交给编译器，不在源码里手工做。

### 5. 目录与命名

- 目录按原厂源码树组织（断言字符串里的 `../../src/Alg/...` 等路径）：`Alg/`（算法模块）、`App/`（主流程、Static、Buffer、DataSwitch）、`Common/`（Bss、Memory、Assert）、`Extension/`、`Prmt/`、`Rejection/`、`Tool/`、`WorkAround/`。原厂文件名未知的按函数前缀归入对应模块；一个原厂源文件可以按职责拆成几个文件。
- 命名空间根是 `Solvers::Tsa::Core`，原函数名前缀变成子命名空间（`IIR_Process` → `Iir::Process`）；只在本文件内调用的函数放进匿名命名空间。类型与函数 PascalCase，变量与字段 camelCase，常量 `kName`，注释中文。
- 宿主接口只有 [TsaCoreApi.h](/EGoTouchService/Solvers/TouchSolver/TsaCore/TsaCoreApi.h) 里按原导出签名声明的 `TSA_*`，加回放用的 `TsaCore_StateBlocks`。

### 6. 改写规则（不动算法细节的边界）

可以改的：

- 偏移访问换成结构体字段；无意义的临时名换成有意义的名字；`goto` 与无限循环加 `return` 改成结构化控制流；冗余的中间变量去掉。
- 断言换成 `TsaAssert(条件)`；日志输出已裁掉。
- 本机型参数下恒为常量的分支做常量传播并删除不可达部分；没有调用者的函数、只写不读的状态直接删除。
- 返回 `bool` 的判定：原代码 `setcc` 只写低字节，调用方只看低字节的直接用 `bool`。
- 整数表达式在对应位宽的模运算意义下的代数等价改写；逐项赋值的大表改成描述表加循环（前提是各项写入位置互不重叠）。

不能改的：

- 每个读写的位宽与符号：原代码用有符号 16 位读一个 `uint16_t` 字段，改写后也要 `static_cast<int16_t>`；截断、掩码、取符号操作保留语义。
- 除法与移位的取整方向；浮点运算的顺序与精度（`float` / `double` 不互换，不合并常量）。
- 有副作用的表达式的求值顺序、函数调用的次数与顺序；两个调用出现在同一表达式里时拆成语句（C++ 不规定 `a < b` 两侧的求值顺序）。
- 原代码不检查的下标照样不检查（见第 2 节的越界落点）；阈值、循环边界、提前返回、对超界索引的钳位。

发现与原 DLL 不一致的地方（例如类型推断错误导致的指针步长或符号错误），以 Ghidra 反汇编为准单独提交修正，提交说明里列出汇编证据与对回放的影响，再把 `tsa-reference` 挪到该提交。
