# Config 配置系统重构方案 — 纯运行时 Config-Driven 架构

> 日期: 2026-06-05 | 策略: 完全重构, 不兼容旧格式 | 原则: 配置文件是唯一权威数据源

> 当前实现进度: 截至 2026-06-07，P1-3 Catalog 策略字段和 P1-4 `IConfigTarget` registry 已合入。当前架构仍是 v3 过渡态：Catalog/Snapshot v3 IPC 已落地，ConfigRuntime 已通过 target validate/rollback/action plan 管理 live patch；`default.yaml` generator、完整 v3 Patch/Persist result、App `ConfigDraft`、legacy fixed ABI cleanup 仍待完成。

---

## 一、目标架构

### 1.1 核心原则

1. **配置文件是唯一权威数据源** — 所有可配置值来自 `default.yaml`, 用户覆盖来自 `overrides.yaml`
2. **无代码生成, 无编译期硬编码** — 修改配置不需要重新编译; 发行时携带配置文件
3. **运行时加载 + Schema 校验** — 启动时加载, 类型错误/越界/缺键在启动阶段精确报告

### 1.2 格式选择

| 维度 | INI | JSON | YAML |
|------|-----|------|------|
| 嵌套结构 | :x: | :white_check_mark: | :white_check_mark: |
| 注释支持 | :white_check_mark: | :x: | :white_check_mark: |
| 类型系统 | :x: 全字符串 | :white_check_mark: | :white_check_mark: |
| 人工可读 | :yellow_circle: | :yellow_circle: | :white_check_mark: |
| C++ 依赖 | 手写 | nlohmann/json (header-only) | yaml-cpp (~1MB) |

**选择: YAML**。备选: 如 yaml-cpp 二进制增量超标, 切换 nlohmann/json + JSONC 注释扩展。

### 1.3 配置文件布局

```
安装目录/
├── EGoTouchService.exe
├── EGoTouchApp.exe
└── config/
    ├── default.yaml       ← 出厂默认 (随产品发行, 所有键+文档注释)
    └── overrides.yaml     ← 用户覆盖 (首次修改配置时自动创建)
```

加载优先级:
1. `--config <dir>` CLI 参数
2. `EGOTOUCH_CONFIG_DIR` 环境变量
3. `./config/` 可执行文件同目录
4. 启动失败 — 必须找到 `default.yaml`

### 1.4 架构分层

```mermaid
graph TB
    subgraph L4["Layer 4: Config UI / CLI"]
        UI["ConfigUIRenderer<br/>ImGui 面板"]
        CLI["--config CLI"]
    end

    subgraph L3["Layer 3: IPC Transport"]
        IPC["TLV ConfigSnapshot<br/>TLV ConfigPatch<br/>PersistConfig"]
    end

    subgraph L2["Layer 2: ConfigStore (Runtime)"]
        direction TB

        YAML["config/default.yaml<br/>config/overrides.yaml"]
        PARSER["YamlParser::load()"]
        VALIDATOR["SchemaValidator<br/>类型/范围/必填校验"]
        STORE["ConfigStore<br/>path → value 存储"]
        CATALOG["ConfigCatalog<br/>keyId/type/default/range/policy"]
        RUNTIME["ConfigRuntime<br/>target validate + action plan"]
        BINDER["ConfigBinder<br/>YAML path → C++ member 绑定"]
        ACCESSOR["ConfigAccessor<br/>成员直接访问 (热路径)"]
        SAVE["saveOverrides()<br/>仅写变更键"]

        YAML --> PARSER
        PARSER --> VALIDATOR
        VALIDATOR --> STORE
        STORE --> CATALOG
        CATALOG --> RUNTIME
        RUNTIME --> BINDER
        BINDER --> ACCESSOR
        STORE --> SAVE
    end

    subgraph L1["Layer 1: Module Binding"]
        TOUCH["TouchPipeline<br/>registerBindings() + applyConfig()"]
        STYLUS["StylusPipeline<br/>registerBindings() + applyConfig()"]
        SVC["ServiceConfig"]
        TARGETS["IConfigTarget<br/>ServicePolicyTarget / PipelineConfigTarget"]
        DVR["DVR Runtime"]
    end

    UI --> IPC
    IPC --> RUNTIME
    BINDER --> TOUCH
    BINDER --> STYLUS
    BINDER --> SVC
    RUNTIME --> TARGETS
    TARGETS --> TOUCH
    TARGETS --> STYLUS
    TARGETS --> SVC
    BINDER --> DVR
```

### 1.5 加载流程

```mermaid
sequenceDiagram
    participant YAML as config/*.yaml
    participant PARSER as YamlParser
    participant VAL as SchemaValidator
    participant STORE as ConfigStore
    participant RUNTIME as ConfigRuntime
    participant TARGET as IConfigTarget
    participant MOD as Pipeline/Service

    Note over YAML,MOD: 启动阶段

    YAML->>PARSER: 加载 default.yaml
    YAML->>PARSER: 加载 overrides.yaml (可选)
    PARSER->>STORE: 合并为统一配置树

    MOD->>RUNTIME: register target / bindings
    Note over RUNTIME: Catalog 描述 keyId/type/range/applyTiming/persistPolicy

    STORE->>VAL: validate(schema)
    Note over VAL: 类型检查 / 范围校验 / 缺键报告
    VAL-->>STORE: ValidationResult

    STORE->>TARGET: apply startup config
    TARGET->>MOD: 写入成员字段

    Note over YAML,MOD: 运行时阶段

    MOD->>MOD: 热路径直接读成员字段 (零开销)

    Note over YAML,MOD: 配置修改 (App 通过 IPC)

    APP->>RUNTIME: ApplyConfigPatch(TLV)
    RUNTIME->>TARGET: validate(candidate, changeSet)
    TARGET-->>RUNTIME: ValidationResult
    RUNTIME->>RUNTIME: commit store after all targets pass
    RUNTIME-->>TARGET: action plan
    TARGET->>MOD: 更新成员字段 (mutex 外执行外部 apply)
    RUNTIME->>YAML: saveOverrides()
```

### 1.6 热路径读取模型

```mermaid
flowchart LR
    subgraph Startup["启动/重载 (一次性)"]
        direction TB
        A1["ConfigStore::get&lt;int&gt;('path')"] --> A2["写入成员字段"]
        A2 --> A3["m_baseline.alphaShift = ..."]
    end

    subgraph HotPath["每帧处理 (零开销)"]
        direction TB
        B1["直接成员访问"] --> B2["int alpha = m_baseline.alphaShift"]
    end

    Startup -.->|"一次性缓存"| HotPath

    Note1["ConfigStore 不用于热路径<br/>Pipeline::applyConfig() 一次性读取并缓存"]
```

---

## 二、核心组件设计

### 2.1 ConfigStore — 统一配置存储

```cpp
// Common/include/config/ConfigStore.h

class ConfigStore {
public:
    // ── 加载 ──
    void loadFromYaml(const std::string& path);          // 加载并合并 YAML
    ValidationResult validate(const Schema& schema) const;

    // ── 读取 ──
    template<typename T>
    T get(std::string_view path) const;

    template<typename T>
    T getOr(std::string_view path, T fallback) const;    // 缺键回退

    // ── 写入 ──
    template<typename T>
    void set(std::string_view path, T value);

    // ── UI / IPC ──
    ConfigSnapshot snapshot() const;
    ConfigMutationResult applyPatch(const ConfigPatch& patch);

    // ── 持久化 ──
    void saveToYaml(const std::string& path) const;
    void saveOverrides(const std::string& path);         // 仅写与 default 不同的键

    // ── 元数据 ──
    std::vector<std::string> allPaths() const;
    bool has(std::string_view path) const;

private:
    struct Entry {
        ConfigValue value;
        ConfigValue defaultValue;
        ConfigValue minVal, maxVal;
        std::string description;
    };
    std::unordered_map<std::string, Entry> m_entries;
};
```

### 2.2 ConfigBinder — YAML path → C++ member 绑定

替代 codegen 的核心机制。每个模块用一行 C++ 声明一个绑定:

```cpp
// TouchPipeline.cpp

void TouchPipeline::registerBindings(ConfigBinder& binder) {
    using Range = ConfigRange;

    // 每个键: (YAML路径, 成员指针, 默认值, 范围, 描述)
    binder.bind("touch.signal_cond.baseline_bg_alpha_shift",
                &BaselineParams::backgroundAlphaShift, m_baseline,
                3, Range{0, 15}, "Background alpha shift for baseline tracking");

    binder.bind("touch.signal_cond.baseline_no_finger_alpha_shift",
                &BaselineParams::noFingerAlphaShift, m_baseline,
                3, Range{0, 15}, "No-finger alpha shift for baseline tracking");

    binder.bind("touch.signal_cond.baseline_recovery_max_frames",
                &BaselineParams::recoveryMaxFrames, m_baseline,
                30, Range{1, 120}, "Max frames for baseline recovery");

    // 模块开关 (bool 键)
    binder.bind("touch.frame_parser.enabled",
                &TouchPipeline::m_frameParserEnabled, *this,
                true, "Frame Parser enable switch");
}

void TouchPipeline::applyConfig(const ConfigStore& store) {
    // 可选的显式读取 — 或者 ConfigBinder::apply() 自动完成
    // 此处可做 "键 X 变更 → 重置模块 Y" 的联动逻辑
}
```

`ConfigBinder` 接口:

```cpp
class ConfigBinder {
public:
    // 基础绑定: 值类型 (int, float, bool)
    template<typename Struct, typename T>
    void bind(std::string_view yamlPath,
              T Struct::*member, Struct& instance,
              T defaultValue,
              ConfigRange range = {},
              std::string_view description = "");

    // 枚举绑定
    template<typename Struct, typename Enum>
    void bind(std::string_view yamlPath,
              Enum Struct::*member, Struct& instance,
              Enum defaultValue,
              std::span<const std::pair<Enum, const char*>> enumMapping,
              std::string_view description = "");

    // 从 ConfigStore 读取值并写入所有绑定成员
    void apply(const ConfigStore& store);

    // 从绑定生成 Schema (用于校验)
    Schema toSchema() const;

    // 从绑定生成 ConfigStore 默认值 (用于首次写入 overrides.yaml)
    void writeDefaults(ConfigStore& store) const;

private:
    std::vector<BindingEntry> m_bindings;
};
```

### 2.3 SchemaValidator — 启动时校验

```cpp
struct ValidationIssue {
    enum Severity { Error, Warning };
    Severity severity;
    std::string path;       // "touch.signal_cond.baseline_bg_alpha_shift"
    std::string message;    // "value 500 exceeds max 2000, clamped to 2000"
};

struct ValidationResult {
    bool ok() const { return errors.empty(); }
    std::vector<ValidationIssue> errors;
    std::vector<ValidationIssue> warnings;

    void logAll() const;  // LOG_ERROR / LOG_WARN 每个问题
};

class SchemaValidator {
public:
    // 从 Schema 文件加载规则 (config/schema.yaml)
    static Schema loadFromYaml(const std::string& path);

    // 从 ConfigBinder 自动推导 Schema
    static Schema fromBinder(const ConfigBinder& binder);

    // 校验 ConfigStore 中的值
    ValidationResult validate(const ConfigStore& store, const Schema& schema);
};
```

### 2.4 ConfigValue — 类型安全的配置值

```cpp
using ConfigValue = std::variant<
    bool,
    int32_t,
    float,
    std::string
>;

// 辅助函数
template<typename T> T getValue(const ConfigValue& v);
template<typename T> std::optional<T> tryGetValue(const ConfigValue& v);

// 序列化 (用于 YAML/JSON 写入)
std::string toString(const ConfigValue& v);
```

### 2.5 ConfigPath — 路径工具

```cpp
namespace Config {

struct ConfigPaths {
    std::string defaultConfig;   // config/default.yaml
    std::string overrideConfig;  // config/overrides.yaml
    std::string baseDir;         // config/
};

// 按优先级解析配置目录
ConfigPaths resolve(std::optional<std::string> cliOverride = {});

// 分隔符: "touch.signal_cond.baseline_bg_alpha_shift"
// 对应 YAML: touch → signal_cond → baseline_bg_alpha_shift

} // namespace Config
```

### 2.6 ConfigCatalog 策略字段 — v3 契约元数据

当前 v3 catalog 已携带策略字段，并通过 catalog payload version 2 传输；snapshot payload 仍保持 version 1。

```cpp
enum class ConfigScope : uint8_t {
    ServicePolicy,
    TouchPipeline,
    StylusPipeline,
    DeviceRuntime,
    Debug,
};

enum class ConfigApplyTiming : uint8_t {
    LiveImmediate,
    FrameBoundary,
    RestartRequired,
    StartupOnly,
    ReadOnly,
};

enum class ConfigPersistPolicy : uint8_t {
    Persistable,
    RuntimeOnly,
    GeneratedDefault,
    Deprecated,
};
```

当前已落地行为:

- `ConfigSchemaEntry` / `ConfigDescriptor` / `BindingEntry` round-trip 这些字段。
- `GetConfigCatalogV3` 输出 catalog v2，App 解析后按 `applyTiming` 过滤不可 live patch 的键。
- `service.mode` 标记为 `RestartRequired`，不会被 App 当作 live immediate patch 发送。

仍待完成:

- `RestartRequired` 的 staged patch + persist + restart 生效语义。
- `GeneratedDefault` / `Persistable` 在 v3 Persist 与 generator 中的最终消费。

### 2.7 IConfigTarget — 事务化 validate/apply 边界

当前 `ConfigRuntime` 已支持 target registry。TLV live patch 的关键路径:

```text
parse TLV -> schema/type/range validation -> candidate ConfigStore
-> ConfigChangeSet -> all targets validate
-> commit store only after all targets pass
-> produce apply action plan
-> ServiceHost executes external DeviceRuntime/Pipeline apply outside ConfigRuntime mutex
```

已落地 target:

| Target | Scope | 当前职责 |
|--------|-------|----------|
| `ServicePolicyTarget` | `service.*` | 生成 service policy action plan; 保持 restart-required 不走 live immediate |
| `PipelineConfigTarget` | `touch.*`, `stylus.*` | 校验 pipeline live patch; `StylusIirCoefficientsWithinMax` 已迁入 target validate; 生成 pipeline apply action |

设计约束:

- target validate 失败不得 commit `ConfigStore`。
- target 不应在 `ConfigRuntime` mutex 内执行阻塞 I/O 或回调 runtime。
- 外部设备/pipeline apply 必须通过 action plan 在 mutex 外执行。

---

## 三、配置文件定义

### 3.1 `config/default.yaml` — 出厂默认配置

```yaml
# EGoTouchRev 出厂默认配置 v2
# 每个键均有文档注释。用户覆盖请编辑 config/overrides.yaml。

service:
  mode: full                     # full | touch_only
  auto_mode: true
  stylus_vhf_enabled: true
  pen_button_mode: oem_custom    # oem_custom | native_barrel | native_eraser
  pen_button_route: vhf_only     # vhf_only | win32_only | vhf_and_win32

touch:
  signal_cond:
    baseline_bg_alpha_shift: 3       # 0..15  背景基线 alpha 移位
    baseline_bg_max_step: 512        # 1..2048 背景基线最大步长
    baseline_no_finger_alpha_shift: 3  # 0..15  无手指基线 alpha 移位
    baseline_no_finger_max_step: 512   # 1..2048 无手指基线最大步长
    baseline_recovery_alpha_shift: 2   # 0..15  恢复阶段 alpha 移位
    baseline_recovery_max_frames: 30   # 1..120 恢复阶段最大帧数
    baseline_recovery_max_step: 256    # 1..2048 恢复阶段最大步长

  frame_parser:
    enabled: true

  # ── 可选调参键 (注释掉则使用 Schema 默认值) ──
  # peak_detection:
  #   threshold: 120
  #   max_peaks: 10
  # palm_rejection:
  #   enabled: true
  #   area_threshold: 500

stylus:
  hpp2:
    enabled: true
    sensor_tx_count: 60            # 1..100
    sensor_rx_count: 40            # 1..100
  # ... (全部 ~65 键在此列出)

dvr:
  # ... (DVR 配置键)
```

### 3.2 `config/schema.yaml` — 类型与约束 (可选)

```yaml
# Schema 定义: 当 default.yaml 缺键时提供默认值和约束
# 也可以完全从 ConfigBinder::toSchema() 自动生成, 此文件为文档参考

service:
  mode: { type: enum, default: full, values: [full, touch_only] }
  auto_mode: { type: bool, default: true }
  stylus_vhf_enabled: { type: bool, default: true }
  pen_button_mode: { type: enum, default: oem_custom, values: [oem_custom, native_barrel, native_eraser] }
  pen_button_route: { type: enum, default: vhf_only, values: [vhf_only, win32_only, vhf_and_win32] }

touch:
  signal_cond:
    baseline_bg_alpha_shift: { type: int, default: 3, min: 0, max: 15 }
    # ...
```

如果选择从 ConfigBinder 自动推导, 此文件可以不存在。手动维护 `schema.yaml` 的优点是: 可以在不重新编译的情况下修改默认值或范围约束。

---

## 四、IPC 协议

### 4.1 TLV 自描述格式

```mermaid
flowchart LR
    subgraph Entry["ConfigTlvEntry (每条记录 5+N bytes)"]
        direction LR
        K["keyId<br/>uint16_t"] --> T["valueType<br/>uint8_t"]
        T --> L["valueLen<br/>uint16_t"]
        L --> V["valuePayload<br/>N bytes"]
    end

    subgraph Snapshot["ConfigSnapshot 消息"]
        direction LR
        V1["version<br/>uint16_t"] --> C1["entryCount<br/>uint16_t"]
        C1 --> E1["entries..."]
    end

    subgraph Patch["ConfigPatch 消息 (仅变更键)"]
        direction LR
        V2["version<br/>uint16_t"] --> C2["entryCount<br/>uint16_t"]
        C2 --> E2["entries..."]
    end
```

### 4.2 Key ID 分配

```cpp
// 编译时枚举, Service/App 两端共享同一份定义
enum class ConfigKeyId : uint16_t {
    // Service:    0x0000-0x00FF
    SvcMode               = 0x0000,
    SvcAutoMode           = 0x0001,
    SvcStylusVhfEnabled   = 0x0002,
    SvcPenButtonMode      = 0x0003,
    SvcPenButtonRoute     = 0x0004,

    // Touch:      0x0100-0x01FF
    TouchBaselineBgAlphaShift = 0x0100,
    TouchBaselineBgMaxStep    = 0x0101,
    // ... 其余 touch 键按 registerBindings 调用顺序分配

    // Stylus:     0x0200-0x02FF
    StylusHpp2Enabled = 0x0200,
    // ...

    // DVR:        0x0300-0x03FF

    // 未知 keyId → 安全跳过 (向前兼容)
};
```

### 4.3 IPC 命令

```mermaid
sequenceDiagram
    participant APP as EGoTouchApp
    participant IPC as IPC Pipe
    participant SVC as ServiceHost

    Note over APP,SVC: 启动 / 连接

    APP->>SVC: GetConfigSnapshot
    SVC-->>APP: ConfigSnapshot (所有 active 键)

    Note over APP,SVC: 用户修改配置

    APP->>SVC: ApplyConfigPatch (仅变更键)
    SVC-->>APP: ConfigMutationResult<br/>(changed / applied / restartRequired)

    APP->>SVC: PersistConfig
    SVC->>SVC: 写入 config/overrides.yaml

    Note over APP,SVC: 外部修改配置文件后

    APP->>SVC: ReloadConfig
    SVC->>SVC: 重新加载 YAML → apply(bindings)
    SVC-->>APP: ConfigSnapshot (全量)
```

---

## 五、打包策略

### 5.1 CMake 规则

```cmake
# CMakeLists.txt

# 构建后复制 config/ 到输出目录
add_custom_command(TARGET EGoTouchService POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/config"
        "$<TARGET_FILE_DIR:EGoTouchService>/config"
    COMMENT "Deploy config files to build output"
)

# CPack / MSI 安装规则
install(DIRECTORY config/
    DESTINATION config
    COMPONENT Config
    FILES_MATCHING
    PATTERN "*.yaml"
)

install(TARGETS EGoTouchService EGoTouchApp
    RUNTIME DESTINATION .
    COMPONENT Runtime
)
```

### 5.2 发行包结构

```
EGoTouchRev-2.0.0/
├── EGoTouchService.exe
├── EGoTouchApp.exe
├── config/
│   ├── default.yaml          ← 出厂默认 (所有键 + 完整注释, 只读参考)
│   └── overrides.yaml        ← 用户覆盖 (首次修改时自动创建, 仅写变更键)
└── README.txt
```

### 5.3 overrides.yaml 生成策略

```
首次启动:
  config/overrides.yaml 不存在 → 不是错误 → 使用 default.yaml 全部值

用户修改配置 (App → IPC → Service):
  Service 收到 PersistConfig:
    1. 计算 default.yaml 与当前 ConfigStore 的差异
    2. 仅将 differ 的键写入 overrides.yaml
    3. 保持 YAML 结构 + 注释头

下次启动:
  default.yaml → ConfigStore
  overrides.yaml → ConfigStore (覆盖同名键)
  Schema 校验 → 应用
```

---

## 六、实施计划

### 当前 v3 主线进度 (2026-06-07)

| 阶段 | 状态 | 当前结果 |
|------|------|----------|
| P0 Catalog/KeyId/TLV/Runtime/App schema 地基 | 已完成 | 静态 keyId、structured TLV parse、`ConfigRuntime` facade、App schema/keyId 收敛 |
| P1-1 Service-driven Catalog/Snapshot IPC | 已完成 | `GetConfigCatalogV3` / `GetConfigSnapshotV3` 分页 IPC 落地 |
| P1-2 App connected legacy snapshot fallback 删除 | 已完成 | connected mode 不再 fallback 到 legacy `GetConfigSnapshot=42`; 本地 fallback 仅用于 Service 不可用/离线 |
| P1-3 Catalog strategy fields | 已完成 | `ConfigScope` / `ConfigApplyTiming` / `ConfigPersistPolicy` 已进入 catalog v2 round-trip; App live patch 过滤不可 live apply 键 |
| P1-4 `IConfigTarget` registry | 已完成 | `ConfigRuntime` 通过 target validate 后 commit，失败 rollback; `ServicePolicyTarget` / `PipelineConfigTarget` 默认注册; apply action 在 mutex 外执行 |
| P1-5 Catalog-to-`default.yaml` generator/check | 待完成 | 生成 YAML 与仓库 `config/default.yaml` 语义等价，并接入 CI/CMake 校验 |
| P1-6 v3 Patch/Persist result | 待完成 | 替换过渡 `ApplyConfigTlvChunk` / legacy persist 语义，支持 per-key result 与 restart-required staged flow |
| P1-7 App `ConfigDraft` | 待完成 | 拆分 Service snapshot cache、用户 draft、dirty baseline、apply/persist state |
| P2 legacy fixed ABI cleanup | 待完成 | 删除 Service/Common 旧 fixed ABI 主路径；本地离线 fallback 保留 |

### Phase 0: 依赖 + 基础组件 (1-2 天)

```mermaid
graph LR
    A["引入 yaml-cpp<br/>CMake FetchContent"] --> B["ConfigValue<br/>variant 类型"]
    B --> C["YamlParser<br/>load/save"]
    C --> D["ConfigStore<br/>get/set/path"]
    D --> E["单元测试<br/>YAML round-trip"]
```

| 产出 | 文件 |
|------|------|
| yaml-cpp 集成 | `CMakeLists.txt` |
| ConfigValue | `Common/include/config/ConfigValue.h` |
| YamlParser | `Common/include/config/YamlParser.h`, `Common/source/config/YamlParser.cpp` |
| ConfigStore (基础) | `Common/include/config/ConfigStore.h`, `Common/source/config/ConfigStore.cpp` |

### Phase 1: Schema + Binder + 默认配置 (2-3 天)

```mermaid
graph LR
    A["ConfigBinder<br/>bind(path, member, default, range)"] --> B["SchemaValidator<br/>validate(store, schema)"]
    B --> C["config/default.yaml<br/>一次性从旧源生成"]
    C --> D["校验测试<br/>类型错误/越界/缺键"]
```

| 产出 | 文件 |
|------|------|
| ConfigBinder | `Common/include/config/ConfigBinder.h` |
| SchemaValidator | `Common/include/config/SchemaValidator.h` |
| default.yaml | `config/default.yaml` (完整, 所有键+注释) |

### Phase 2: Pipeline 迁移 (3-4 天)

```mermaid
graph LR
    A["TouchPipeline<br/>registerBindings() + applyConfig()"] --> B["StylusPipeline<br/>同上模式"]
    B --> C["ServiceConfig<br/>同上模式"]
    C --> D["DVR<br/>同上模式"]
    D --> E["删除所有旧解析代码<br/>codegen / 手写 INI / constexpr 默认值"]
```

| 产出 | 删除 |
|------|------|
| `TouchPipeline::registerBindings()` | `TouchPipelineConfigKeys.h/.cpp` |
| `TouchPipeline::applyConfig()` | `TouchPipeline::LoadConfig()` 旧实现 |
| `StylusPipeline::registerBindings()` | `StylusPipelineConfigKeys.h/.cpp` |
| `StylusPipeline::applyConfig()` | `StylusPipeline::LoadConfig()` 旧实现 |
| `ServiceConfig::registerBindings()` | `ServiceConfigCore.cpp` 手写 INI 解析 |
| DVR 配置绑定 | DVR 独立解析代码 |
| — | `config_key_sync.py` |
| — | `IsFrozenCurrentTouchConfigKey()` |
| — | 三处重复的 `TrimCopy`/`ParseBoolValue`/`ParseIniKeyValue` |
| — | `IsLegacyTouchSection()` / `MapLegacyTouchKey()` |

### Phase 3: IPC 协议 (2 天)

```mermaid
graph LR
    A["ConfigKeyId enum<br/>+ TLV struct"] --> B["Service 侧<br/>GetConfigSnapshot / ApplyConfigPatch / PersistConfig"]
    B --> C["App 侧<br/>ServiceProxy 切换 ConfigStore"]
    C --> D["集成测试<br/>IPC round-trip"]
```

| 产出 | 文件 |
|------|------|
| TLV 协议定义 | `IpcProtocol.h` |
| Service IPC handler | `ServiceHost.cpp` |
| App 侧适配 | `ServiceProxy.Config.cpp`, `ServiceProxy.cpp` |

### Phase 4: UI + 打包 (1-2 天)

```mermaid
graph LR
    A["ConfigUIRenderer<br/>从 ConfigStore 获取 schema"] --> B["CMake 打包<br/>config/ 复制到输出目录"]
    B --> C["首次运行测试<br/>default.yaml → apply → 正常工作"]
```

| 产出 | 文件 |
|------|------|
| UI 适配 | `ConfigUIRenderer.cpp` |
| 打包规则 | `CMakeLists.txt` |
| 端到端验证 | 手动测试 |

---

## 七、代码量变化

| 类别 | 当前 | 目标 | 增量 |
|------|------|------|------|
| 配置解析代码 (手写 + codegen) | ~1800 行 | 0 行 | -1800 |
| config_key_sync.py | 1149 行 | 0 行 (删除) | -1149 |
| ConfigStore + Binder + YamlParser (框架) | 0 行 | ~600 行 | +600 |
| registerBindings() 声明 (4 个模块) | 0 行 | ~200 行 | +200 |
| config/default.yaml (文档化配置) | 0 行 (分散在 YAML+C++ 中) | ~400 行 | +400 |
| **净变化** | — | — | **~ -1750 行** |

---

## 八、新增配置键的流程对比

**当前 (6 步, 涉及 Python 脚本)**:
1. 编辑 `config/touch_pipeline_config.yaml`
2. 运行 `python config_key_sync.py generate`
3. 在 `TouchPipeline.h` 添加成员字段
4. 更新 `IsFrozenCurrentTouchConfigKey()`
5. 更新 Python 脚本中 `_touch_member_expr()` 映射表
6. 重新编译

**目标 (4 步, 纯 C++ + YAML)**:
1. 编辑 `config/default.yaml` — 添加键和值
2. 在模块中添加成员字段 (如需要)
3. `binder.bind("path", &member, default, range, desc)` — 一行 C++
4. 重新编译

---

## 九、风险

| 风险 | 缓解 |
|------|------|
| yaml-cpp 二进制增量 (~1MB) | Phase 0 实测; 超标则切换 nlohmann/json (header-only) |
| YAML 手写格式错误 → 启动失败 | SchemaValidator 给出精确路径 + 期望类型; 启动日志完整 |
| ConfigStore::get() 用于热路径 | 设计约束: 仅 `applyConfig()` 一次性读取并缓存, 热路径直接成员访问 |

---

## 十、Verification Plan

| Phase | 验证 |
|-------|------|
| 0 | YAML → ConfigStore → 读取值 round-trip 单元测试 |
| 1 | Schema 校验覆盖: 类型错误 / 越界 / 缺键; `default.yaml` 成功加载 |
| 2 | 所有现有 Pipeline round-trip 测试通过 (行为不变); arm64 Debug/Release 编译 |
| 3 | IPC TLV 序列化/反序列化 round-trip; App ↔ Service 集成测试 |
| 4 | 端到端: 启动 → 加载配置 → ImGui 面板正常 → 修改 → 持久化 → 重启 → 配置保持 |
