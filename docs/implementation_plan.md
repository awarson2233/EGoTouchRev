# Config 配置系统重构方案 — 当前实现计划

> 日期: 2026-06-08
> 当前事实: Config v3 connected mode 已成为主路径；legacy fixed config ABI 仅保留 unsupported tombstone；本次收尾完成 packaging/e2e gate 与 docs/API sync。

---

## 1. 当前架构事实

`default.yaml` + `overrides.yaml` 是启动加载和持久化输入，`ConfigRuntime` 是运行时事务权威。App connected mode 通过 v3 IPC 获取 catalog/snapshot、发送 patch、触发 persist；本地 binder/YAML fallback 只用于 Service 不可用/离线路径。

关键组件：

| 组件 | 当前职责 | 主要文件 |
|------|----------|----------|
| `ConfigStore` | flat path → `ConfigValue` 存储；YAML load/save；merge；按 default 保存 override diff | `Common/include/config/ConfigStore.h`, `Common/source/config/ConfigStore.cpp` |
| `ConfigBinder` | YAML path → C++ member/schema binding；生成 defaults/schema/current snapshot | `Common/include/config/ConfigBinder.h`, `Common/source/config/ConfigBinder.cpp` |
| `ConfigCatalog` | 合并 defaults + binder metadata，生成 descriptor/schema snapshot | `Common/include/config/ConfigCatalog.h`, `Common/source/config/ConfigCatalog.cpp` |
| `ConfigKeyMap` | 静态 `ConfigKeyId` ↔ YAML path 双向映射；v3 patch/snapshot 只使用静态 keyId | `Common/include/config/ConfigKeyId.h`, `Common/source/config/ConfigKeyMap.cpp` |
| `ConfigRuntime` | startup load、schema validation、v3 catalog/snapshot、patch transaction、persist | `EGoTouchService/include/ConfigRuntime.h`, `EGoTouchService/source/ConfigRuntime.cpp` |
| `IConfigTarget` | target interested/validate/apply action plan 边界 | `EGoTouchService/include/ConfigTarget.h` |
| App `ConfigDraft` | catalog defaults、service snapshot、editable draft、dirty baseline、per-key apply/persist state | `Tools/EGoTouchApp/include/ServiceProxy.h`, `Tools/EGoTouchApp/source/ServiceProxy.Config.cpp` |
| DVR runtime config snapshot | DVR2 runtime config schema/value capture 与 `ConfigStore` conversion | `Common/DVRCore/include/DvrTypes.h`, `Common/DVRCore/source/DvrTypes.cpp`, `Tools/EGoTouchApp/source/ServiceProxy.cpp` |

## 2. Config v3 IPC 当前契约

| Command | 值 | 当前行为 |
|---------|----|----------|
| `ReloadConfig` | 40 | legacy tombstone，connected Service IPC 返回 `UnsupportedCommand` |
| `SaveConfig` | 41 | legacy tombstone，connected Service IPC 返回 `UnsupportedCommand` |
| `GetConfigSnapshot` | 42 | legacy tombstone，connected Service IPC 返回 `UnsupportedCommand` |
| `ApplyConfigPatch` | 43 | legacy tombstone，connected Service IPC 返回 `UnsupportedCommand` |
| `PersistConfig` | 44 | legacy tombstone，connected Service IPC 返回 `UnsupportedCommand` |
| `ApplyConfigTlvChunk` | 45 | legacy tombstone，connected Service IPC 返回 `UnsupportedCommand` |
| `GetConfigCatalogV3` | 46 | paged catalog v3 payload |
| `GetConfigSnapshotV3` | 47 | paged snapshot v3 payload |
| `ApplyConfigPatchV3` | 48 | request carries base schema/snapshot version + TLV patch payload, cap 240 bytes |
| `PersistConfigV3` | 49 | persists only `UserOverride` entries to `overrides.yaml`; other policies are skipped |

Patch result status:

- `Ok`: patch committed and either live-applied or staged.
- `NoChanges`: valid base version, no effective value changes.
- `VersionMismatch`: App must refresh v3 snapshot/catalog and retry.
- `Rejected`: syntactically valid request rejected by schema/target/policy; IPC transport still succeeds.
- `PersistFailed`: persist operation failed.

Malformed wire requests use IPC failure (`InvalidRequest`, `InvalidState`, `InternalError`) instead of semantic result.

## 3. KeyId Coverage Gate

All patchable `UserOverride` entries must have a static `ConfigKeyId`. This is enforced by `ConfigDefaultYamlDriftTest`:

```text
entry.boundToRuntime
entry.persistPolicy == UserOverride
entry.applyTiming is live or RestartRequired
=> keyId != MaxKeyId
=> tryPathForKeyId(keyId) == yamlPath
=> tryKeyIdForPath(yamlPath) == keyId
```

Current static mapping covers service keys, all current `TouchPipeline::registerBindings()` `touch.*` keys, and all current `StylusPipeline::registerBindings()` / `bindSchema()` `stylus.*` keys. `registerRuntimeKeyMappings()` is a compatibility entry point only; runtime dynamic keyId allocation is not the v3 contract.

## 4. Runtime Persist/Restart Evidence

`ConfigRuntimeTest` now covers:

- live service key patch: `service.auto_mode=false`, live applied and persisted.
- restart-required key patch: `service.mode=touch_only`, staged, persisted, not live-applied before restart.
- newly mapped touch key patch: `touch.peak_detection.threshold=351`, live pipeline action and persisted.
- `overrides.yaml` contains only changed `UserOverride` keys; unchanged `service.stylus_vhf_enabled` is not written.
- a fresh `ConfigRuntime::Initialize(configDir)` reloads default + overrides and preserves the persisted service/touch values.

This is device-free evidence for Step 6 restart persistence semantics. It does not start/stop a real Windows service.

## 5. Packaging Layout Gate

Build output and installer layout are validated by `PackagingConfigLayoutTest`:

- `EGoTouchService.exe` output directory contains `config/default.yaml`.
- `EGoTouchApp.exe` output directory contains `config/default.yaml`.
- output `default.yaml` SHA-256 matches repository `config/default.yaml`.
- `cmake --install` into the test install prefix produces `EGoTouchService.exe`, `EGoTouchApp.exe`, and `config/default.yaml`.
- WiX `DefaultConfigYAML` uses `$(var.BuildOutputDir)\config\default.yaml`; the test expands this source path for the active build dir and verifies the file exists and hash-matches repository `config/default.yaml`.

CMake install rules also install:

```text
EGoTouchApp.exe      -> .
EGoTouchService.exe  -> .
config/default.yaml  -> config/default.yaml
```

## 6. DVR Runtime Config Snapshot

DVR runtime config is captured as DVR2 schema/value sections, not as a live mutable config owner.

Current implementation:

- `ServiceProxy::CaptureRuntimeConfigSnapshot()` reads Service/App atomics plus selected pipeline fields.
- `BuildRuntimeConfigSnapshotFromState()` builds contiguous field/value ids and computes DVR2 runtime config schema hash.
- `RuntimeConfigSnapshot::toConfigStore()` converts bool, signed int, bounded unsigned int, float32, float64, and string runtime config values to `ConfigStore`; `UInt32` values above `INT32_MAX` are skipped because `ConfigStore` has no unsigned 32-bit value type.
- `DvrCoreRuntimeConfigRoundTripTest` validates DVR2 write/read and `toConfigStore()` conversion.
- `EGoTouchApp.ServiceProxyRuntimeConfigSnapshotTest` validates App snapshot fields, types, field/value alignment, and schema metadata.

## 7. Gate Status

| Gate | 状态 | 证据 |
|------|------|------|
| P1-6 Patch/Persist result | [x] | `ConfigRuntimeTest`, App config draft tests |
| P1-7 ConfigDraft | [x] | `EGoTouchApp.ServiceProxyCatalogSchemaTest` |
| P2 legacy fixed ABI cleanup | [x] | IPC command tombstone tests and active-source grep |
| Build macro cleanup | [x] | active-source macro grep |
| Step 6 packaging/e2e verification | [x] | `PackagingConfigLayoutTest`, `ConfigRuntimeTest`, DVR/App runtime config tests |
| Step 7 docs/API sync | [x] | current docs match header/wire/test facts |

## 8. Verification Commands

```powershell
cmake --preset arm64-Debug
cmake --build --preset arm64-Debug --target ConfigRuntimeTest ConfigDefaultYamlDriftTest DvrCoreRuntimeConfigRoundTripTest EGoTouchApp_ServiceProxyRuntimeConfigSnapshotTest EGoTouchApp EGoTouchService
ctest --test-dir build/arm64-Debug -R "ConfigRuntimeTest|ConfigDefaultYamlDriftTest|DvrCoreRuntimeConfigRoundTripTest|EGoTouchApp\.ServiceProxyRuntimeConfigSnapshotTest|PackagingConfigLayoutTest|EGoTouchApp\.ServiceProxyCatalogSchemaTest" --output-on-failure
git diff --check
```

## 9. Remaining Non-Gate Work

The following historical items remain outside this P0 finalization gate:

- Remove old generated/config parsing source files that are no longer active product paths.
- Expand App UI polish for per-row strategy badges and per-key state display.
- Add real service-process e2e if a hardware/service harness is available.
- Decide whether YAML-only/non-user-override allowlist is needed.
