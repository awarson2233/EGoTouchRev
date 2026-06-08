# Config 重构任务清单

> 基于: [implementation_plan.md](implementation_plan.md) | 当前权威: 本文顶部“Config v3 当前主线”与 [implementation_plan.md](implementation_plan.md) 的“后续实施编排 Gate”
> 历史参考: `docs/artifacts/config_v3_full_upgrade_plan.md` 是 P1-2 后 artifact，不再作为当前权威进度；其中对 strategy fields / IConfigTarget 的未落地描述已被 P1-3/P1-4 后续实现覆盖。
> 原则: 分阶段迁移到 Config v3；最终删除 legacy fixed ABI fallback，本地 fallback 保留为 Service 不可用/离线场景

状态: `[ ]` 待开始 `[~]` 进行中 `[x]` 已完成 `[-]` 已取消

> 进度说明：本文档是早期 YAML/ConfigStore/Binder/TLV 迁移任务清单，保留历史拆分。当前 v3 主线进度见下方“Config v3 当前主线”。

---

## Config v3 当前主线

- [x] P0 Catalog/KeyId/TLV/Runtime/App schema 地基
- [x] P1-1 Service-driven `GetConfigCatalogV3` / `GetConfigSnapshotV3` 分页 IPC
- [x] P1-2 App connected mode 删除 legacy `GetConfigSnapshot=42` fallback；本地 binder/YAML fallback 仅保留为 Service 不可用/离线路径
- [x] P1-3 Catalog strategy fields：`ConfigScope` / `ConfigApplyTiming` / `ConfigPersistPolicy` 进入 schema、descriptor、binding 与 v3 catalog payload；catalog wire version 升为 2，snapshot 保持 version 1；App/UI live patch 过滤 `ReadOnly` / `StartupOnly` / `RestartRequired`
- [x] P1-4 `IConfigTarget` / target registry：`ConfigRuntime` 通过 target validate 后 commit，失败不落库；默认注册 `ServicePolicyTarget` / `PipelineConfigTarget`；ServiceHost 按 apply action plan 在 runtime mutex 外执行 pipeline apply；IIR 关系校验迁入 pipeline target
- [x] P1-5 runtime-derived `default.yaml` drift check；已接入 CTest，校验 Catalog/schema/default 一致性，避免 `config/default.yaml` 漂移
- [x] P1-6 v3 Patch/Persist result 完整化；`ApplyConfigPatchV3` / `PersistConfigV3` 已落地，支持 restart-required staged/persist/restart 语义（当前 patch payload cap: 240 bytes）
- [ ] P1-7 App `ConfigDraft` 完整拆分 snapshot cache、editable draft、dirty baseline、apply/persist state
- [ ] P2 legacy fixed ABI cleanup：删除 Service/Common 旧 `ConfigSnapshotWire` / `ApplyConfigPatchRequestWire` 主路径；保留本地离线 fallback

## 后续执行 Gate

> 每个 impl agent 返回后必须由 review agent 审查；未通过则回同一 impl/fix agent 修正。review 通过时只输出文档摘要，文档由主 agent 更新。
> 所有 subagent prompt 必须明确要求 `gpt-5.5 xhigh`。任务审查 agent 负责先定义接口/协议 gate、修改边界、串行/并行关系；review agent 负责实现后的验收。

| Gate | 状态 | 任务内容 | 修改范围 | 预期行为 | 验收方式 | 并行规则 |
|------|------|----------|----------|----------|----------|----------|
| Step 1 P1-6 final review/fix | [x] 已完成 | 冻结并验收 `ApplyConfigPatchV3` / `PersistConfigV3` wire/result；完成 restart-required staged + persist 语义 | IPC wire/client/server, `ConfigRuntime`, `ServiceHost`, `ServiceProxy`, ABI/runtime/App tests | semantic reject 返回 result wire `Rejected`; malformed request 才 IPC failure; `VersionMismatch` refresh/retry; restart-required 不 live apply; persist policy skip/count | `ctest --preset arm64-Debug --output-on-failure` 40/40 passed；full build 受运行中 `EGoTouchService.exe` 锁定影响，相关 target build passed | 已串行完成 |
| Step 2 P1-7 ConfigDraft | [ ] 待开始 | 拆分 Service catalog cache、snapshot cache、editable draft、dirty baseline、apply/persist state | App `ServiceProxy`, `ConfigUIRenderer`, inspector, App tests | refresh 不覆盖 dirty draft; apply/persist 状态可表达 rejected、unpersisted、restart-required、VersionMismatch rebase | App draft tests + full ctest | 串行；不可与 App legacy cleanup 并行 |
| Step 3 P2 Common/Service legacy IPC cleanup | [ ] 待开始 | 删除/隔离 Service/Common legacy fixed ABI 主路径 | Common IPC, ServiceHost, IPC ABI tests | connected config IPC 只保留 v3 catalog/snapshot/patch/persist；旧 command tombstone 或 unsupported | `git grep` legacy symbols; build; ctest | 可与 Step 4 并行，独立 worktree |
| Step 4 P2 App connected legacy cleanup + offline fallback | [ ] 待开始 | 清理 App connected legacy merge/apply/helper；保留离线 binder/YAML fallback | App `ServiceProxy`, inspector, App fallback tests | connected 只走 v3；offline fallback 仍可初始化 schema/store；v3 fetch failure 不伪装成 legacy snapshot | App connected/offline tests + full ctest | 可与 Step 3 并行；不可与 Step 2 并行 |
| Step 5 build macro cleanup | [ ] 待开始 | 清理 `EGOTOUCH_CONFIG_ENABLED` / `EGOTOUCH_ENABLE_RUNTIME_CONFIG` 双路径 | CMake/presets, SolverBuildConfig, guarded Service/App/Solver code | runtime config 成为唯一主路径；disabled runtime UI/branches 移除或降为兼容 shim | grep macros; arm64-Debug/Release/amd64-Debug configure/build gates | 默认串行 |
| Step 6 packaging/e2e verification | [ ] 待开始 | 验证 build output config、startup、connected edit/apply/persist、restart persistence | 优先只读；必要时补 e2e tests/scripts | `config/default.yaml` 随 exe；overrides 仅保存差异且可重启保持 | ctest; preset builds; config copy/install check; persist/restart evidence | 可并行只读验证 |
| Step 7 final docs/API sync | [ ] 待开始 | 同步实施文档、任务清单、IPC/API 文档 | docs only | 文档与代码、grep、测试证据一致；legacy 主路径描述删除或标历史 | docs diff-check; task checklist audit | 仅主 agent 写 docs |

### Gate 明细 (Tesla 输出)

#### Step 1: P1-6 Final Review/Fix

- 状态: 已完成并通过 review。`ApplyConfigPatchV3=48` / `PersistConfigV3=49` 已落地；`ctest --preset arm64-Debug --output-on-failure` 40/40 passed。full build 仅因运行中 `EGoTouchService.exe` 锁定输出 exe 受阻。
- 任务内容: 冻结并验收 v3 patch/persist 接口语义；明确 semantic reject、malformed/internal error、VersionMismatch、RestartRequired staged、persist policy skip 行为。
- 修改范围: Common IPC wire/client/server、`ConfigRuntime`、`ServiceHost`、App `ServiceProxy`、ABI/runtime/App tests。
- 禁止范围: 不做 `ConfigDraft` 结构化重构；不删除 legacy fixed ABI；不清理 build macro；不让 subagent 修改 docs。
- 预期行为: 合法 v3 patch 的 schema/target/policy 拒绝通过 IPC success + result wire `Rejected` 返回；malformed request 才 IPC failure；`VersionMismatch` 触发 App refresh/retry 且 retry response 校验 `wireVersion`；`RestartRequired` key 可 staged/persist 但不 live apply；persist 只保存允许持久化的 key 并统计 skip；target validate 失败不 commit runtime store。
- 验收方式: `git diff --check`；`cmake --preset arm64-Debug`；`cmake --build --preset arm64-Debug`，若 exe lock 阻断则记录进程证据并构建相关 test targets；`ctest --preset arm64-Debug --output-on-failure`；测试覆盖 rejected result propagation、retry invalid wireVersion、VersionMismatch rebase、RestartRequired staged、persist skip count。
- 编排规则: 已串行完成；1 个 impl/fix agent -> 1 个 review agent；review 通过时输出 `Accepted Change Summary for Docs`。

#### Step 2: P1-7 ConfigDraft

- 状态: 待开始，下一步串行 gate。
- 任务内容: 先确立 App 侧 `ConfigDraft` 状态模型，再实施；拆分 Service catalog cache、Service snapshot cache、editable draft、dirty baseline、apply state、persist state；UI/ServiceProxy 不再把 `m_configStore` 同时当 snapshot、draft、local preview、apply input。
- 修改范围: `Tools/EGoTouchApp/include/ServiceProxy.h`、`Tools/EGoTouchApp/source/ServiceProxy.Config.cpp`、`Tools/EGoTouchApp/source/DiagnosticsWorkbench.Inspector.cpp`、`Tools/EGoTouchApp/source/ConfigUIRenderer.cpp`、`Tools/EGoTouchApp/tests/ServiceProxyCatalogSchemaTest.cpp`，可新增 App draft tests。
- 禁止范围: 不修改 IPC wire layout；不删除 Service/Common legacy fixed ABI；不清理 `EGOTOUCH_CONFIG_ENABLED`；不让 subagent 修改 docs。
- 预期行为: Refresh snapshot 不覆盖 dirty draft；apply+persist 成功后清除对应 dirty key；apply 成功但 persist 失败后 dirty key 保留为 live-applied/unpersisted；apply rejected 后 failed key 保留 dirty 并显示错误状态；VersionMismatch 后 refresh snapshot，再用当前 dirty draft rebase 构造 patch；RestartRequired key 显示 staged/restart-required，不显示为 live-applied；offline/local fallback 仍能初始化本地 schema/default draft；connected mode 只消费 Service v3 catalog/snapshot。
- 验收方式: App draft tests 覆盖 snapshot refresh does not clobber dirty draft、apply ok + persist fail keeps dirty、rejected key remains dirty with error、VersionMismatch rebase uses refreshed baseline、RestartRequired staged state；`git diff --check`；`cmake --build --preset arm64-Debug`；`ctest --preset arm64-Debug --output-on-failure`。
- 编排规则: 串行；不可与任何 App cleanup 并行；可并行派只读 audit agent 列 UI 写入点但不得写代码。review 重点确认 `ConfigDraft` 是真实分离，而不是重命名 `m_configStore`。

#### Step 3: P2 Common/Service Legacy IPC Cleanup

- 状态: 待开始，必须在 P1-7 后。
- 任务内容: 删除或隔离 Service/Common 侧 legacy fixed ABI 主路径；connected config IPC 主路径只保留 v3 catalog/snapshot/patch/persist；旧 `ConfigSnapshotWire` / `ApplyConfigPatchRequestWire` / legacy handler 不再作为 Service 主路径存在。
- 修改范围: `Common/IPCCore/include/Ipc/IpcProtocol.h`、`Common/IPCCore/include/Ipc/IpcPipeClient.h`、`Common/IPCCore/source/IpcPipeClient.cpp`、`Common/IPCCore/source/IpcPipeServer.cpp`、`Common/IPCCore/tests/IpcProtocolAbiTest.cpp`、`EGoTouchService/include/ServiceHost.h`、`EGoTouchService/source/ServiceHost.cpp`，以及必要 Service IPC tests。
- 禁止范围: 不修改 App draft 状态模型；不删除 offline/local YAML fallback；不清理 build macro；不让 subagent 修改 docs。
- 预期行为: Service 不再处理 connected legacy fixed config snapshot/apply command；IPC ABI tests 更新为 v3 canonical path；旧 command 值如需保留 tombstone，必须明确 unsupported；v3 `GetConfigCatalogV3` / `GetConfigSnapshotV3` / `ApplyConfigPatchV3` / `PersistConfigV3` 仍通过。
- 验收方式: `git grep` `ConfigSnapshotWire`、`ApplyConfigPatchRequestWire`、`HandleIpcGetConfigSnapshot` 和 legacy config apply handler，结果只允许 docs、历史注释或明确 tombstone tests；`git diff --check`；`cmake --build --preset arm64-Debug`；`ctest --preset arm64-Debug --output-on-failure`。
- 编排规则: 可与 Step 4 并行但必须分 worktree；`IpcProtocol.h`、`ServiceHost.cpp` 本步骤独占，App cleanup agent 不得写。review 重点确认未误删 v3 paging 或 v3 patch/persist。

#### Step 4: P2 App Connected Legacy Cleanup + Offline Fallback Preservation

- 状态: 待开始，必须在 P1-7 后。
- 任务内容: 清理 App connected mode 中遗留的 legacy merge/apply/helper 路径；保留 Service 不可用/离线场景的本地 binder/YAML fallback；明确 connected 与 offline 两种模式的入口和边界。
- 修改范围: `Tools/EGoTouchApp/include/ServiceProxy.h`、`Tools/EGoTouchApp/source/ServiceProxy.Config.cpp`、`Tools/EGoTouchApp/source/DiagnosticsWorkbench.Inspector.cpp`、`Tools/EGoTouchApp/tests/ServiceProxyConfigMergeTest.cpp`、`Tools/EGoTouchApp/tests/ServiceProxyCatalogSchemaTest.cpp`，以及 App-local fallback tests。
- 禁止范围: 不写 `IpcProtocol.h`；不写 `ServiceHost.cpp`；不改 Runtime target logic；不清理 build macro；不让 subagent 修改 docs。
- 预期行为: connected mode apply/save 只走 v3 patch/persist；connected mode snapshot/catalog 只走 v3；legacy INI merge helpers 若只服务旧 connected path，应删除；offline/local fallback 仍可加载 local schema/default store，UI 可离线展示/编辑本地配置；offline fallback 不得在 connected v3 fetch 失败时伪装成 Service snapshot fallback，除非明确处于 disconnected/offline mode。
- 验收方式: `git grep` connected legacy helper symbols，确认只剩 offline fallback 或删除；App tests 覆盖 connected does not call legacy snapshot/apply、offline fallback still initializes schema/store、v3 fetch failure in connected mode does not silently replace Service state with legacy snapshot；`git diff --check`；`cmake --build --preset arm64-Debug`；`ctest --preset arm64-Debug --output-on-failure`。
- 编排规则: 可与 Step 3 并行，独立 worktree；不可与 Step 2 并行。review 重点确认 connected/offline 边界清楚且未破坏本地 fallback。

#### Step 5: EGOTOUCH_CONFIG_ENABLED / Build Macro Cleanup

- 状态: 待开始，必须在 P2 legacy cleanup 后。
- 任务内容: 清理 `EGOTOUCH_ENABLE_RUNTIME_CONFIG` / `EGOTOUCH_CONFIG_ENABLED` 双路径；让 runtime config 成为唯一主路径，删除 UI/Service/Solvers 中 disabled runtime config 分支或收敛为测试专用。
- 修改范围: `CMakeLists.txt`、`CMakePresets.json`、`EGoTouchService/Solvers/SolverBuildConfig.h`、`EGoTouchService/Solvers/tests/CMakeLists.txt`、`EGoTouchService/source/ConfigRuntime.cpp`、`EGoTouchService/source/ServiceHost.cpp`、`Tools/EGoTouchApp/source/DiagnosticsWorkbench.Inspector.cpp`，以及相关 tests。
- 禁止范围: 不修改 IPC wire layout；不重新设计 ConfigDraft；不删除 offline fallback；不让 subagent 修改 docs。
- 预期行为: 默认构建不再依赖 runtime config off/on 分叉；App 不再显示 runtime config disabled 的产品路径 UI；Solver/Service/App 测试仍能构建；CMake preset 中相关 option 被删除或降级为无害兼容项。
- 验收方式: `git grep EGOTOUCH_CONFIG_ENABLED`；`git grep EGOTOUCH_ENABLE_RUNTIME_CONFIG`，只允许文档、历史说明或明确兼容 shim；`cmake --preset arm64-Debug`；`cmake --preset arm64-Release`；`cmake --preset amd64-Debug`；至少 arm64-Debug full build + ctest，其他 preset 至少 configure + core targets build。
- 编排规则: 默认串行，`CMakeLists.txt` 必须独占或严格分区；不可与 Step 3/4 并行。review 重点确认 build matrix 行为没有被意外改变。

#### Step 6: Packaging/E2E Verification

- 状态: 待开始，必须在 Step 5 后。
- 任务内容: 验证最终强约束链路：Catalog/default.yaml、Service startup、App connected v3 edit/apply/persist、restart 后保持配置；验证 config 文件复制/安装。
- 修改范围: 优先只读验证；如必须补测试，可写 CMake packaging tests、integration/e2e test scripts、existing test registration。
- 禁止范围: 不做大范围功能重构；不改 IPC 协议；不改 Draft 模型；不让 subagent 修改 docs，验证摘要交给主 agent。
- 预期行为: build output 目录存在 `config/default.yaml`；首次启动加载 default.yaml 且 schema validation 通过；App connected mode 获取 catalog/snapshot；修改 live key 后 apply 生效；修改 restart-required key 后 staged/persist，不 live apply；`overrides.yaml` 只保存与 default 不同且允许 persist 的 key；Service restart 后 default + overrides 合并，配置保持。
- 验收方式: `ctest --preset arm64-Debug --output-on-failure`；`cmake --build --preset arm64-Debug`；`cmake --build --preset arm64-Release` 或至少核心 targets；`cmake --build --preset amd64-Debug` 或至少核心 targets；验证 build output `.exe` 同目录下有 `config/default.yaml`；有可复现命令或测试记录证明 persist/restart 行为。
- 编排规则: 可派多个只读 verification agents 并行；若需要写测试，必须先停下并由主 agent 分配单一写入 owner。review 重点确认验证覆盖实际目标，而不只是跑单测。

#### Step 7: Final Docs/API Sync

- 状态: 待开始；每个代码阶段 review 通过后可增量同步一次，最终 sync 必须在 Step 6 后。
- 任务内容: 主 agent 根据所有通过 review 的 `Accepted Change Summary for Docs` 更新文档；同步 `docs/implementation_plan.md`、`docs/task.md`、必要的 `docs/api/ipc_protocol.md` / config framework API docs。
- 修改范围: `docs/implementation_plan.md`、`docs/task.md`、`docs/api/ipc_protocol.md`、`docs/api/config_framework_api.md`，需要时更新 architecture docs 中过时的 legacy 描述。
- 禁止范围: 不改源码；不让 subagent 写 docs。
- 预期行为: 当前进度准确标记 P1-6、P1-7、P2 cleanup、macro cleanup、packaging/e2e 状态；API docs 反映 v3 canonical IPC，不再把旧 fixed ABI 描述为主路径；明确 offline fallback 保留边界；明确 default.yaml drift check 和 generator/check 现状。
- 验收方式: `git diff --check docs/implementation_plan.md docs/task.md docs/api/ipc_protocol.md docs/api/config_framework_api.md`；文档中旧 legacy 主路径描述被替换或标注为历史/删除；task checklist 与实际 grep/test 结果一致。
- 编排规则: docs 写入不并行，由主 agent 独占；可派只读 docs audit agent，但不得修改文件。review 重点确认文档不超前声明未验证完成。

---

## Phase 0: 依赖 + 基础组件

### 0.1 yaml-cpp 集成
- [x] 0.1.1 CMake FetchContent 引入 yaml-cpp
- [~] 0.1.2 在 arm64-Debug / arm64-Release / amd64-Debug 下编译通过
- [ ] 0.1.3 实测 arm64-Release 的二进制增量, 确认是否需切换到 nlohmann/json

### 0.2 ConfigValue 类型
- [x] 0.2.1 `Common/include/config/ConfigValue.h` — `std::variant<bool, int32_t, float, std::string>`
- [x] 0.2.2 `getValue<T>()` / `tryGetValue<T>()` 辅助函数
- [x] 0.2.3 `toString()` 序列化
- [ ] 0.2.4 单元测试: 各类型构造 / 访问 / 转换

### 0.3 YamlParser
- [x] 0.3.1 `Common/include/config/YamlParser.h` — `load(path)` / `save(path, node)`
- [x] 0.3.2 `Common/source/config/YamlParser.cpp` — yaml-cpp 封装
- [x] 0.3.3 错误处理: 文件不存在 / 格式错误 / 路径不可写
- [ ] 0.3.4 单元测试: YAML round-trip

### 0.4 ConfigStore (基础)
- [x] 0.4.1 `Common/include/config/ConfigStore.h` — `get<T>(path)` / `set<T>(path, value)`
- [x] 0.4.2 `Common/source/config/ConfigStore.cpp` — 基于 hash map 的 flat key-value 存储
- [x] 0.4.3 `has(path)` / `allPaths()` 遍历接口
- [x] 0.4.4 `loadFromYaml(path)` — YAML 文件 → ConfigStore
- [x] 0.4.5 `saveToYaml(path)` — ConfigStore → YAML 文件 (保持结构)
- [x] 0.4.6 merge 路径解析 — 点号分隔的 YAML 路径解析 (如 `touch.signal_cond.key`)
- [ ] 0.4.7 单元测试: get/set round-trip / YAML 加载保存

---

## Phase 1: Schema + Binder + 默认配置

### 1.1 ConfigBinder
- [x] 1.1.1 `Common/include/config/ConfigBinder.h` — 核心绑定 DSL
- [x] 1.1.2 `bind<T>(path, memberPtr, instance, default, range, desc)` 模板实现
- [x] 1.1.3 枚举类型 `bindEnum()` 重载 (enum → string 映射)
- [x] 1.1.4 `apply(ConfigStore&)` — 从 Store 读取值写入所有绑定成员
- [x] 1.1.5 `populateSchema(ConfigStore&)` — 从绑定生成 Schema
- [x] 1.1.6 `writeDefaults(ConfigStore&)` — 将默认值写入 ConfigStore
- [ ] 1.1.7 单元测试: 绑定 → apply → 成员值一致 / 缺键使用默认值

### 1.2 SchemaValidator
- [x] 1.2.1 `Common/include/config/SchemaValidator.h`
- [x] 1.2.2 Schema 数据由 ConfigBinder 的 BindingEntry 提供
- [x] 1.2.3 `validate(ConfigStore, ConfigBinder)` → `ValidationResult`
- [x] 1.2.4 校验规则: 类型不匹配 (Error) / 越界 (Error) / 缺键 (Warning) / 枚举值 (Error) / 未知类型 (Warning)
- [~] 1.2.5 从 YAML 文件加载 Schema (未实现; 由 ConfigBinder 自动生成替代)
- [ ] 1.2.6 单元测试: 每种校验失败场景

### 1.3 ConfigPath
- [x] 1.3.1 `Common/include/config/ConfigPath.h` — `resolve(cliOverride)` 函数
- [x] 1.3.2 4 级优先级: CLI → 环境变量 → `./config/` → 启动失败
- [ ] 1.3.3 合并 `default.yaml` + `overrides.yaml` (在 ServiceHost 启动流程中实现)

### 1.4 生成 config/default.yaml
- [x] 1.4.1 从 `config/touch_pipeline_config.yaml` + `config/stylus_pipeline_config.yaml` 提取 active 键 → 生成 `config/default.yaml`
- [x] 1.4.2 为每个键补全文档注释和范围
- [~] 1.4.3 验证: 所有当前 active 键的默认值与 C++ constexpr 值一致 (待编译后验证)
- [x] 1.4.6 Catalog/default.yaml drift check: `ConfigDefaultYamlDriftTest` 从 runtime factory defaults/schema 生成 YAML 并与仓库 `config/default.yaml` 语义比较
- [ ] 1.4.4 所有 frozen 键的默认值与旧 constexpr 值一致 (frozen 键暂不进入 default.yaml)
- [ ] 1.4.5 清理旧的 `config/touch_pipeline_config.yaml` / `config/stylus_pipeline_config.yaml` (在 Phase 2 完成后清理)

---

## Phase 2: Pipeline 迁移

### 2.1 TouchPipeline
- [x] 2.1.1 在 `TouchPipeline.h` 中声明 `registerBindings(ConfigBinder&)` / `applyConfig(const ConfigStore&)`
- [x] 2.1.2 实现 `registerBindings()` — 所有 active 键 (8 个)
- [x] 2.1.3 实现 `applyConfig()` — 一次性从 ConfigStore 读取并缓存到成员字段
- [ ] 2.1.4 ServiceHost 启动时调用: `binder → apply → TouchPipeline`
- [ ] 2.1.5 重载时调用: `ConfigStore::loadFromYaml() → binder.apply()`
- [ ] 2.1.6 删除 `TouchPipelineConfigKeys.h`
- [ ] 2.1.7 删除 `TouchPipelineConfigKeys.cpp`
- [ ] 2.1.8 删除 `TouchPipeline::LoadConfig()` / `SaveConfig()` / `GetConfigSchema()` 旧实现
- [ ] 2.1.9 删除 `IsFrozenCurrentTouchConfigKey()`
- [ ] 2.1.10 TouchPipelineConfigRoundTripTest 适配新接口并通过

### 2.2 StylusPipeline
- [x] 2.2.1 声明 `registerBindings()` / `applyConfig()`
- [x] 2.2.2 实现 `registerBindings()` — ~40 bindable 键 (bool + int), uint 成员由 applyConfig 处理
- [x] 2.2.3 实现 `applyConfig()` — 含 disable-flag reset 逻辑
- [ ] 2.2.4 删除 `StylusPipelineConfigKeys.h`
- [ ] 2.2.5 删除 `StylusPipelineConfigKeys.cpp`
- [ ] 2.2.6 删除 `StylusPipeline::LoadConfig()` / `SaveConfig()` / `GetConfigSchema()` 旧实现
- [ ] 2.2.7 StylusPipelineConfigRoundTripTest 适配新接口并通过

### 2.3 ServiceConfig
- [x] 2.3.1 为 `ServiceConfigState` 实现 `registerBindings()` / `applyConfig()`
- [ ] 2.3.2 重构 `ServiceConfigCore.cpp` — 删除手写 INI 解析, 改用 ConfigStore
- [ ] 2.3.3 删除 `ParseServiceConfig()` 手写实现
- [ ] 2.3.4 `DiffServiceConfig` 由 ConfigBinder 自动处理 (compare snapshot)
- [ ] 2.3.5 删除 `EGOTOUCH_CONFIG_ENABLED` 宏定义和相关 #ifdef
- [ ] 2.3.6 ServiceConfigParserTest / ServiceHostConfigMutationTest 适配并通过

### 2.4 DVR Runtime
- [x] 2.4.1 确认 DVR 所有可配置键 (DVR 使用二进制快照，非 live config owner)
- [x] 2.4.2 在 `RuntimeConfigSnapshot` 添加 `toConfigStore()` 衔接点签名
- [ ] 2.4.3 删除旧 DVR 配置解析代码 (暂不需要)
- [ ] 2.4.4 DvrCoreRuntimeConfigRoundTripTest 适配并通过

### 2.5 清理
- [ ] 2.5.1 删除 `config_key_sync.py`
- [ ] 2.5.2 删除 `EGoTouchService/Solvers/ConfigParse.h` (ParseConfigInt/Float/Bool)
- [ ] 2.5.3 删除 `EGoTouchService/Solvers/ConfigSchema.h` 中的 `IConfigProvider` (被 ConfigBinder 替代)
- [ ] 2.5.4 清理 `ServiceHost.cpp` 中的 `LoadPipelineConfig` 模板 + `WriteCanonicalConfig`
- [ ] 2.5.5 清理 `ServiceProxy.Config.cpp` 中的旧 INI 合并/写回逻辑
- [ ] 2.5.6 清理三处重复的 `TrimCopy` / `ParseBoolValue` / `ParseIniKeyValue`
- [ ] 2.5.7 清理 `IsLegacyTouchSection()` / `MapLegacyTouchKey()` 遗留兼容函数
- [ ] 2.5.8 清理 `CMakeLists.txt` 中 `EGOTOUCH_ENABLE_RUNTIME_CONFIG` + `EGOTOUCH_CONFIG_ENABLED` 构建选项
- [ ] 2.5.9 [[IRRADIATION]]: 搜索 `EGOTOUCH_CONFIG_ENABLED` 和 `EGOTOUCH_DIAG` 使用处, 确认全部清理

---

## Phase 3: IPC 协议

### 3.1 TLV 协议定义
- [x] 3.1.1 定义 `ConfigKeyId` enum (Service/Touch/Stylus/DVR 分区)
- [x] 3.1.2 定义 `ConfigTlvEntry` / `ConfigSnapshotTlv` / `ConfigPatchTlv` struct
- [x] 3.1.3 `ConfigMutationResultTlv` struct
- [x] 3.1.4 keyId ↔ YAML path 双向映射 (ConfigKeyMap)
- [x] 3.1.5 TLV 序列化/反序列化函数
- [x] 3.1.6 单元测试: TLV round-trip / 未知 keyId / duplicate / trailing bytes / 版本不匹配检测
- [x] 3.1.7 v3 Catalog 策略字段: `scope` / `applyTiming` / `persistPolicy` round-trip; catalog payload version 2, snapshot payload version 1

### 3.2 Service 侧 IPC Handler
- [x] 3.2.1 `HandleIpcGetConfigCatalogV3` — ConfigRuntime catalog blob → paged IPC response
- [x] 3.2.2 `HandleIpcGetConfigSnapshotV3` — ConfigRuntime snapshot blob → paged IPC response
- [x] 3.2.3 `HandleIpcApplyConfigPatchV3` — v3 patch/result 已落地；semantic reject 通过 result wire `Rejected` 返回，malformed request 才 IPC failure
- [x] 3.2.4 `HandleIpcPersistConfigV3` — 按 Catalog `UserOverride` persist policy 保存 overrides，并统计 skipped/persisted
- [ ] 3.2.5 `HandleIpcReloadConfig` — 重新加载 YAML → apply bindings → 返回 v3 snapshot
- [ ] 3.2.6 删除旧的 `ConfigSnapshotWire` / `ApplyConfigPatchRequestWire` 固定布局 struct（依赖 App connected mode 全面切 v3）
- [ ] 3.2.7 删除旧的 IPC handler 中对应的旧格式序列化代码（只删除 legacy fixed ABI fallback；不删除本地离线 fallback）
- [x] 3.2.8 `IConfigTarget` registry — `ServicePolicyTarget` / `PipelineConfigTarget` 默认注册；target validate 失败不 commit；pipeline apply 在 ConfigRuntime mutex 外执行

### 3.3 App 侧适配
- [x] 3.3.1 `ServiceProxy` connected mode 优先通过 `GetConfigCatalogV3` / `GetConfigSnapshotV3` 获取 Service catalog/snapshot
- [~] 3.3.2 新增 `ConfigDraft`，拆分 Service snapshot cache、用户 draft、dirty baseline version（当前仅完成轻量 v3 baseline version 记录）
- [x] 3.3.3 删除 connected mode 对 legacy `GetConfigSnapshot=42` 的 fallback
- [x] 3.3.4 保留本地 binder/YAML fallback 作为 Service 不可用/离线场景，不作为 legacy 删除目标
- [x] 3.3.5 `ServiceProxy::SaveConfig()` / Apply 流程升级为 v3 Patch + v3 Persist result；VersionMismatch 后 refresh v3 snapshot 并 retry，retry response 同样校验 wireVersion/status
- [ ] 3.3.6 删除 `MergeServiceProxyConfigSections()` 旧实现（如仍存在）
- [ ] 3.3.7 集成测试: App ↔ Service v3 IPC round-trip

---

## Phase 4: UI + 打包

### 4.1 ConfigUIRenderer 适配
- [ ] 4.1.1 从 ConfigStore 获取 schema (替代旧 `IConfigProvider::GetConfigSchema()`)
- [ ] 4.1.2 ImGui 控件绑定: 读取 → ConfigStore::get() / 写入 → 本地缓存 + 标记 dirty
- [ ] 4.1.3 "保存" 按钮: 构建 ConfigPatch → IPC ApplyConfigPatch → IPC PersistConfig
- [ ] 4.1.4 删除 `ConfigUIRenderer` 对旧 `IConfigProvider` / `ConfigParam` 的依赖

### 4.2 CMake 打包
- [x] 4.2.1 `POST_BUILD` 规则: 复制 `config/` 到输出目录
- [x] 4.2.2 `install()` 规则: Config 组件
- [ ] 4.2.3 验证: 构建后 `config/default.yaml` 在 .exe 同目录

### 4.3 端到端验证
- [ ] 4.3.1 首次启动: `default.yaml` 加载 → Schema 校验 → 应用到 Pipeline → Service 正常运行
- [ ] 4.3.2 配置修改: App → 修改值 → Save → `overrides.yaml` 生成 → Service Reload → 配置生效
- [ ] 4.3.3 `overrides.yaml` 仅包含与 default 不同的键
- [ ] 4.3.4 重启 Service: `default.yaml` + `overrides.yaml` 合并 → 校验 → 应用 → 配置与重启前一致
- [ ] 4.3.5 arm64-Debug / arm64-Release 编译通过且行为一致
- [ ] 4.3.6 所有现有单元测试通过 (无回归)
- [x] 4.3.7 `ctest --preset arm64-Debug --output-on-failure` 通过，40/40 tests passed（含 `ConfigDefaultYamlDriftTest`）

---

## 进度总览

> 下表是 2026-06-05 早期任务拆分的历史统计，不再作为当前 Config v3 权威进度。当前状态：P0、P1-1、P1-2、P1-3 Catalog 策略字段、P1-4 `IConfigTarget` registry、P1-5 default.yaml drift check、P1-6 v3 Patch/Persist result 已完成；下一步是完整 `ConfigDraft`、legacy fixed ABI cleanup、build macro cleanup、packaging/e2e。详见本文档顶部“Config v3 当前主线”。

| Phase | 任务数 | 已完成 | 状态 |
|-------|--------|--------|------|
| Phase 0 | 13 | 11 | 历史统计 |
| Phase 1 | 16 | 13 | 历史统计 |
| Phase 2 | 26 | 12 | 历史统计 |
| Phase 3 | 14 | 5 | 已被 P1-1 v3 IPC 部分覆盖 |
| Phase 4 | 11 | 2 | 历史统计 |
| **总计** | **80** | **41** | 历史统计 |

## 文档产出

- [x] D0.1 IPC 协议接口文档 → `docs/api/ipc_protocol.md`
- [x] D0.2 Config 框架 API 文档 → `docs/api/config_framework_api.md`
- [x] D0.3 Shared Memory ABI 文档 → `docs/api/shared_memory_abi.md`

---

> 最后更新: 2026-06-08 (同步 P1-6 v3 Patch/Persist result 已合入；Service/Common legacy fixed ABI cleanup 后续处理，本地 fallback 保留为离线/Service 不可用路径)
