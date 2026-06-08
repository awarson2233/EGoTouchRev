# Config 重构任务清单

> 当前权威: 本文顶部“Config v3 当前主线”与 [implementation_plan.md](implementation_plan.md)。历史 artifact 只作背景，不作为当前实现事实。
> 原则: connected mode 只走 Config v3 IPC；本地 binder/YAML fallback 仅保留为 Service 不可用/离线路径；`default.yaml` + `overrides.yaml` 是启动/持久化输入，`ConfigRuntime` 是运行时事务权威。

状态: `[ ]` 待开始 `[~]` 进行中 `[x]` 已完成 `[-]` 已取消

---

## Config v3 当前主线

- [x] P0 Catalog/KeyId/TLV/Runtime/App schema 地基
- [x] P1-1 Service-driven `GetConfigCatalogV3` / `GetConfigSnapshotV3` 分页 IPC
- [x] P1-2 App connected mode 删除 legacy `GetConfigSnapshot=42` fallback；本地 binder/YAML fallback 仅保留为 Service 不可用/离线路径
- [x] P1-3 Catalog strategy fields：`ConfigScope` / `ConfigApplyTiming` / `ConfigPersistPolicy` 进入 schema、descriptor、binding 与 v3 catalog payload
- [x] P1-4 `IConfigTarget` registry：`ConfigRuntime` 通过 target validate 后 commit，失败不落库；默认 target 为 `ServicePolicyTarget` / `PipelineConfigTarget`
- [x] P1-5 runtime-derived `default.yaml` drift check；`ConfigDefaultYamlDriftTest` 校验 catalog/schema/default 一致性
- [x] P1-6 v3 Patch/Persist result 完整化；支持 restart-required staged、persist、restart reload 语义
- [x] P1-7 App `ConfigDraft` 拆分 snapshot cache、editable draft、dirty baseline、per-key apply/persist state
- [x] P2 legacy fixed ABI cleanup：legacy config command `40-45` 仅保留 unsupported tombstone；connected config 主路径只保留 v3 `46-49`
- [x] Step 5 build macro cleanup：runtime config 为唯一产品路径；`EGOTOUCH_CONFIG_ENABLED` / `EGOTOUCH_ENABLE_RUNTIME_CONFIG` 不再作为源码分支
- [x] Step 6 packaging/e2e verification gate：build output config、install/WiX config layout、persist/restart evidence、runtime-bound/UserOverride/live key coverage 均有自动化测试
- [x] Step 7 docs/API sync：本文、[implementation_plan.md](implementation_plan.md)、[ipc_protocol.md](api/ipc_protocol.md)、[config_framework_api.md](api/config_framework_api.md) 与当前代码同步

## 后续执行 Gate

| Gate | 状态 | 当前事实 | 验收方式 |
|------|------|----------|----------|
| Step 1 P1-6 final review/fix | [x] 已完成 | `ApplyConfigPatchV3` / `PersistConfigV3` wire/result 已落地；semantic reject 走 result `Rejected`，malformed request 才 IPC failure | `ConfigRuntimeTest`、App draft tests、IPC ABI tests |
| Step 2 P1-7 ConfigDraft | [x] 已完成 | App connected mode 拆分 catalog defaults、service snapshot、editable draft、dirty baseline、per-key state | `EGoTouchApp.ServiceProxyCatalogSchemaTest` |
| Step 3 P2 Common/Service legacy IPC cleanup | [x] 已完成 | legacy config fixed ABI public structs 删除；legacy command `40-45` unsupported tombstone；v3 `46-49` 是 connected config 主路径 | `IPCCoreProtocolAbiTest`、legacy grep |
| Step 4 P2 App connected legacy cleanup + offline fallback | [x] 已完成 | connected mode 只走 v3 catalog/snapshot/patch/persist；offline/local fallback 保留 | `EGoTouchApp.ServiceProxyCatalogSchemaTest` |
| Step 5 build macro cleanup | [x] 已完成 | runtime config 不再由 build macro 控制产品路径 | macro grep |
| Step 6 packaging/e2e verification | [x] 已完成 | `config/default.yaml` 复制到 exe 输出目录；CMake install 与 WiX source 打包 `config/default.yaml`；runtime persist/restart reload 由测试覆盖 | `PackagingConfigLayoutTest`、`ConfigRuntimeTest` |
| Step 7 final docs/API sync | [x] 已完成 | docs/API 描述当前实现，不把历史目标态 API 当当前事实 | `git diff --check`、文档 grep |

## 本次收尾实现范围

- [x] `ConfigKeyId` / `ConfigKeyMap` 覆盖所有当前 `TouchPipeline::registerBindings()` 中 runtime-bound/UserOverride/live patchable `touch.*` key。
- [x] `ConfigDefaultYamlDriftTest` 要求 patchable `UserOverride` key 必须有静态 `ConfigKeyId`，并校验 path/keyId 双向 round-trip。
- [x] `ConfigRuntimeTest` 覆盖 live key、restart-required key、新映射 touch key 的 patch、persist、restart reload；同时校验 `overrides.yaml` 只保存与 default 不同的 `UserOverride` key。
- [x] `RuntimeConfigSnapshot::toConfigStore()` 已实现，DVR runtime config bool/int/bounded uint/float/string 可转换到 `ConfigStore`；`UInt32 > INT32_MAX` 因 `ConfigStore` 无 unsigned 32-bit 类型而跳过。
- [x] `BuildRuntimeConfigSnapshotFromState()` / `ServiceProxy::CaptureRuntimeConfigSnapshot()` 已实现并注册 `EGoTouchApp.ServiceProxyRuntimeConfigSnapshotTest`。
- [x] CMake `install()` 规则安装 `config/default.yaml`；WiX `EGoTouchSetup.wxs` / `EGoTouchTestSetup.wxs` 通过 `$(var.BuildOutputDir)\config\default.yaml` 引用当前 build output。
- [x] 新增 `PackagingConfigLayoutTest`，校验 `EGoTouchService.exe` / `EGoTouchApp.exe` 输出目录旁的 `config/default.yaml`、真实 install prefix、以及 WiX `DefaultConfigYAML` source 文件均与仓库默认配置 hash 一致。
- [x] 当前默认 gate 不启动/停止真实 Windows service process；service-process e2e 仅在 dedicated disposable service/hardware harness 上 opt-in。默认证据是 `ConfigRuntimeTest` + `PackagingConfigLayoutTest` + App fake v3 IPC tests。
- [x] YAML allowlist 当前不需要：`default.yaml` leaf、binder schema entry、static `ConfigKeyId` 当前均为 130/130/130 对齐。

---

## 历史 Phase 清单状态修正

### Phase 1: Schema + 默认配置

- [x] 1.4.6 Catalog/default.yaml drift check：`ConfigDefaultYamlDriftTest` 从 runtime factory defaults/schema 生成 YAML 并与仓库 `config/default.yaml` 语义比较。
- [x] 1.4.7 patchable keyId coverage check：patchable `UserOverride` key 必须有静态 `ConfigKeyId`，且 `tryPathForKeyId()` / `tryKeyIdForPath()` 双向一致。

### Phase 2: Pipeline / DVR Runtime

- [x] 2.1.2 `TouchPipeline::registerBindings()` 已覆盖当前 active touch runtime keys。
- [x] 2.1.3 `TouchPipeline::applyConfig()` 从 `ConfigStore` 一次性读取并缓存成员字段。
- [x] 2.4.2 `RuntimeConfigSnapshot::toConfigStore()` 已实现，`UInt32` 超出 `int32_t` 表示范围时跳过。
- [x] 2.4.4 `DvrCoreRuntimeConfigRoundTripTest` 覆盖 runtime config DVR2 round-trip、`ConfigStore` 转换与 `UInt32` 边界。

### Phase 3: IPC 协议

- [x] 3.1.4 keyId ↔ YAML path 双向映射；service、stylus、全部当前 touch runtime-bound keys 均为静态映射。
- [x] 3.2.6 旧 `ConfigSnapshotWire` / `ApplyConfigPatchRequestWire` 等 fixed ABI public structs 已从 active IPC header 删除。
- [x] 3.2.7 旧 config fixed ABI handler 主路径已删除；legacy command `40-45` 仅为 unsupported tombstone。
- [x] 3.3.5 App apply/save 使用 v3 Patch + v3 Persist result。

### Phase 4: UI + 打包

- [x] 4.2.1 `POST_BUILD` 规则复制 `config/default.yaml` 到输出目录。
- [x] 4.2.2 `install()` 规则安装 `config/default.yaml`。
- [x] 4.2.3 `PackagingConfigLayoutTest` 验证 build output `config/default.yaml` 在 exe 同目录且内容一致，验证 `cmake --install` 的 install prefix，并验证 WiX `DefaultConfigYAML` source 文件存在且 hash 一致。
- [x] 4.3.2 配置修改 persist 行为由 `ConfigRuntimeTest` 覆盖。
- [x] 4.3.3 `overrides.yaml` 只包含与 default 不同的 `UserOverride` key，由 `ConfigRuntimeTest` 覆盖。
- [x] 4.3.4 Service restart reload 后保持 persisted live/restart-required key，由 `ConfigRuntimeTest` 覆盖。

## 验证命令

```powershell
cmake --preset arm64-Debug
cmake --build --preset arm64-Debug --target ConfigRuntimeTest ConfigDefaultYamlDriftTest DvrCoreRuntimeConfigRoundTripTest EGoTouchApp_ServiceProxyRuntimeConfigSnapshotTest EGoTouchApp EGoTouchService
cmake --install build/arm64-Debug --prefix build/arm64-Debug/test-workdirs/manual-install-check
ctest --test-dir build/arm64-Debug -R "ConfigRuntimeTest|ConfigDefaultYamlDriftTest|DvrCoreRuntimeConfigRoundTripTest|EGoTouchApp\.ServiceProxyRuntimeConfigSnapshotTest|PackagingConfigLayoutTest|EGoTouchApp\.ServiceProxyCatalogSchemaTest" --output-on-failure
git diff --check
```

> 最后更新: 2026-06-08（Config v3 收尾实现：key coverage、runtime persist/restart、DVR snapshot、packaging layout、docs/API sync）
