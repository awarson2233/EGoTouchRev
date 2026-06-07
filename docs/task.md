# Config 重构任务清单

> 基于: [implementation_plan.md](implementation_plan.md) | 当前权威进度见: [config_v3_full_upgrade_plan.md](artifacts/config_v3_full_upgrade_plan.md)  
> 原则: 分阶段迁移到 Config v3；最终删除 legacy fixed ABI fallback，本地 fallback 保留为 Service 不可用/离线场景

状态: `[ ]` 待开始 `[~]` 进行中 `[x]` 已完成 `[-]` 已取消

> 进度说明：本文档是早期 YAML/ConfigStore/Binder/TLV 迁移任务清单，保留历史拆分。P0 与 P1-1 的最新状态以 [config_v3_full_upgrade_plan.md](artifacts/config_v3_full_upgrade_plan.md) 为准。

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
- [ ] 3.1.6 单元测试: TLV round-trip / 未知 keyId 跳过 / 版本不匹配检测

### 3.2 Service 侧 IPC Handler
- [x] 3.2.1 `HandleIpcGetConfigCatalogV3` — ConfigRuntime catalog blob → paged IPC response
- [x] 3.2.2 `HandleIpcGetConfigSnapshotV3` — ConfigRuntime snapshot blob → paged IPC response
- [~] 3.2.3 `HandleIpcApplyConfigPatch` — 当前由 `ApplyConfigTlvChunk` 承担过渡 live patch；后续升级为 v3 patch/result
- [ ] 3.2.4 `HandleIpcPersistConfigV3` — 按 Catalog `persistPolicy` 保存 overrides
- [ ] 3.2.5 `HandleIpcReloadConfig` — 重新加载 YAML → apply bindings → 返回 v3 snapshot
- [ ] 3.2.6 删除旧的 `ConfigSnapshotWire` / `ApplyConfigPatchRequestWire` 固定布局 struct（依赖 App connected mode 全面切 v3）
- [ ] 3.2.7 删除旧的 IPC handler 中对应的旧格式序列化代码（只删除 legacy fixed ABI fallback；不删除本地离线 fallback）

### 3.3 App 侧适配
- [x] 3.3.1 `ServiceProxy` connected mode 优先通过 `GetConfigCatalogV3` / `GetConfigSnapshotV3` 获取 Service catalog/snapshot
- [~] 3.3.2 新增 `ConfigDraft`，拆分 Service snapshot cache、用户 draft、dirty baseline version（当前仅完成轻量 v3 baseline version 记录）
- [x] 3.3.3 删除 connected mode 对 legacy `GetConfigSnapshot=42` 的 fallback
- [x] 3.3.4 保留本地 binder/YAML fallback 作为 Service 不可用/离线场景，不作为 legacy 删除目标
- [ ] 3.3.5 `ServiceProxy::SaveConfig()` / Apply 流程升级为 v3 Patch + v3 Persist result
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

---

## 进度总览

> 下表是 2026-06-05 早期任务拆分的历史统计，不再作为当前 Config v3 权威进度。当前状态：P0 地基、P1-1 `GetConfigCatalogV3` / `GetConfigSnapshotV3`、P1-2 connected mode legacy snapshot fallback 删除已完成；下一步是 Catalog 策略字段、`IConfigTarget`、v3 Patch/Persist、default.yaml generator/check。详见 [config_v3_full_upgrade_plan.md](artifacts/config_v3_full_upgrade_plan.md)。

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

> 最后更新: 2026-06-07 (同步 P1-2 connected mode legacy snapshot fallback 已删除；Service/Common legacy fixed ABI cleanup 后续处理，本地 fallback 保留为离线/Service 不可用路径)
