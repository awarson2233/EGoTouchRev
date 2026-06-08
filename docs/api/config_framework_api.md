# Config Framework API 文档

> EGoTouchRev 当前 Config v3 实现 API | 日期: 2026-06-08

---

## 1. 范围

本文描述当前代码中已经实现并由本地测试覆盖的 Config API。它不是目标态设计稿；以下头文件是事实来源：

| 模块 | 事实来源 |
|------|----------|
| 值类型 | `Common/include/config/ConfigValue.h` |
| 路径解析 | `Common/include/config/ConfigPath.h` |
| 存储 | `Common/include/config/ConfigStore.h` |
| 绑定 | `Common/include/config/ConfigBinder.h` |
| schema snapshot | `Common/include/config/ConfigSchemaSnapshot.h` |
| schema validation | `Common/include/config/SchemaValidator.h` |
| YAML parser | `Common/include/config/YamlParser.h` |
| keyId map | `Common/include/config/ConfigKeyId.h`, `Common/include/config/ConfigKeyMap.h` |
| TLV / v3 payload | `Common/include/config/ConfigTlv.h`, `Common/IPCCore/include/Ipc/IpcProtocol.h` |
| Service runtime | `EGoTouchService/include/ConfigRuntime.h`, `EGoTouchService/include/ConfigTarget.h` |
| DVR runtime config snapshot | `Common/DVRCore/include/DvrTypes.h` |

---

## 2. ConfigValue

`Config::ConfigValue` 当前是四种 C++ 值的 `std::variant`：

```cpp
using ConfigValue = std::variant<bool, int32_t, float, std::string>;
```

当前辅助 API：

```cpp
template<typename T>
T getValue(const ConfigValue& v);

template<typename T>
std::optional<T> tryGetValue(const ConfigValue& v);

std::string toString(const ConfigValue& v);
```

当前没有 `fromString(...)`、`typeOf(...)` 或独立的 `Config::Type` 枚举。IPC/TLV 层的值类型使用 `ConfigValueType`，UI/schema 层使用 `ConfigUiType`。

`ConfigRange` 当前是：

```cpp
struct ConfigRange {
    double min = 0.0;
    double max = 0.0;
};
```

---

## 3. ConfigPath

配置目录解析函数：

```cpp
std::optional<ConfigPaths> resolve(
    const std::optional<std::string>& cliOverride = std::nullopt);
```

`ConfigPaths` 字段：

| 字段 | 说明 |
|------|------|
| `defaultConfig` | `config/default.yaml` 完整路径 |
| `overrideConfig` | `config/overrides.yaml` 完整路径 |
| `baseDir` | `config/` 目录 |
| `overrideExists` | `overrides.yaml` 是否存在 |

解析优先级：

1. CLI `--config <dir>` override。
2. `EGOTOUCH_CONFIG_DIR` 环境变量。
3. 可执行文件同目录下的 `config/`。
4. 找不到 `default.yaml` 时返回 `std::nullopt`。

`resolve()` 只解析路径和验证 `default.yaml` 存在，不负责加载 YAML。

---

## 4. ConfigStore

`ConfigStore` 是当前 runtime config 的扁平 key/value 存储。公开 API：

```cpp
class ConfigStore {
public:
    void loadFromYaml(const std::string& path);
    ValidationResult validate() const;

    template<typename T>
    T get(std::string_view path) const;

    template<typename T>
    T getOr(std::string_view path, T fallback) const;

    template<typename T>
    void set(std::string_view path, T value);

    void saveToYaml(const std::string& path) const;
    void saveOverrides(const std::string& path, const ConfigStore& defaults);

    std::vector<std::string> allPaths() const;
    bool has(std::string_view path) const;
    void mergeFrom(const ConfigStore& other);
};
```

行为要点：

- path 使用点号路径，例如 `touch.peak_detection.threshold`。
- `get<T>()` 缺键或类型不匹配会经 `std::get<T>` / runtime error 失败；调用方应在受控路径使用。
- `getOr<T>()` 缺键或类型不匹配返回 fallback。
- `saveOverrides(path, defaults)` 只写出与 `defaults` 不同的值。
- 当前没有 `snapshot()`、`applyPatch()`、`diffKeys()` 或 `ConfigException` 公开 API。

典型加载顺序：

```cpp
Config::ConfigStore store;
store.loadFromYaml(paths.defaultConfig);
if (paths.overrideExists) {
    Config::ConfigStore overrides;
    overrides.loadFromYaml(paths.overrideConfig);
    store.mergeFrom(overrides);
}
```

---

## 5. ConfigBinder

`ConfigBinder` 负责把 YAML path 绑定到 C++ 成员，并生成 schema metadata。当前 `BindingEntry` 包含 path、getter/setter、default value、range、description、static keyId、runtime binding、scope、apply timing、persist policy 和 enum mapping。

公开 API：

```cpp
class ConfigBinder {
public:
    template<typename Struct, typename T>
    void bind(std::string_view yamlPath,
              T Struct::*member,
              Struct& instance,
              T defaultValue,
              ConfigRange range = {},
              std::string_view description = "",
              ConfigRuntimeBinding runtimeBinding = ConfigRuntimeBinding::LiveSetter);

    template<typename Struct, typename Enum>
    void bindEnum(std::string_view yamlPath,
                  Enum Struct::*member,
                  Struct& instance,
                  Enum defaultValue,
                  std::span<const std::pair<Enum, std::string>> enumMapping,
                  std::string_view description = "",
                  ConfigRuntimeBinding runtimeBinding = ConfigRuntimeBinding::LiveSetter);

    void bindSchema(std::string_view yamlPath,
                    ConfigValue defaultValue,
                    std::string_view typeName,
                    ConfigRange range = {},
                    std::string_view description = "",
                    ConfigRuntimeBinding runtimeBinding = ConfigRuntimeBinding::SchemaOnly);

    void apply(const ConfigStore& store);
    void writeDefaults(ConfigStore& store) const;
    void populateSchema(ConfigStore& schemaStore) const;
    ConfigSchemaSnapshot snapshot() const;
    void writeCurrent(ConfigStore& store) const;
    std::vector<std::string> moduleTags() const;
    const std::vector<BindingEntry>& bindings() const;
};
```

当前没有 `toSchema()`、`readMembers()` 或 `bindingCount()` 公开 API。读取当前成员值应使用 `writeCurrent()` 或 `snapshot()`。

`runtimeBinding` 会推导：

| runtimeBinding | 典型用途 |
|----------------|----------|
| `SchemaOnly` | catalog/schema 可见，但没有自动 live setter |
| `LiveSetter` | binder 可直接写入成员，通常可 live apply |
| `ManualLiveApply` | 由 target 手动转换和应用，例如部分 pipeline runtime key |
| `Removed` | 已废弃或移除键 |

---

## 6. ConfigSchemaSnapshot

`ConfigSchemaSnapshot` 是 catalog/schema 的内部中间表示：

```cpp
struct ConfigSchemaSnapshot {
    std::vector<ConfigSchemaEntry> entries;
};
```

`ConfigSchemaEntry` 当前包含：

| 字段 | 说明 |
|------|------|
| `yamlPath` | 点号路径 |
| `keyId` | static `ConfigKeyId`，缺失时为 `MaxKeyId` |
| `uiType` | `Bool` / `Int32` / `Float` / `String` / `Enum` |
| `defaultValue` | factory default |
| `currentValue` | 当前 runtime value |
| `range` | 可选 range |
| `displayName` / `description` / `moduleTag` | UI/catalog metadata |
| `enumMapping` | enum 值到字符串 |
| `runtimeBinding` | schema/live/manual/removed |
| `boundToRuntime` | 是否有 live setter 或 manual apply |
| `scope` | service/touch/stylus/debug/runtime |
| `applyTiming` | read-only/immediate/frame-boundary/manual/restart/startup |
| `persistPolicy` | runtime-only/user-override/generated/deprecated |

辅助函数包括 `deriveModuleTag()`、`deriveDisplayName()`、`deriveUiType()`、`deriveConfigScope()`、`deriveConfigApplyTiming()`、`deriveConfigPersistPolicy()` 和 `isLiveApplyTiming()`。

---

## 7. SchemaValidator

当前 validator 以 `ConfigBinder` 为 schema 来源：

```cpp
struct ValidationIssue {
    enum Severity { Error, Warning };
    Severity severity;
    std::string path;
    std::string message;
};

struct ValidationResult {
    bool ok() const;
    std::vector<ValidationIssue> errors;
    std::vector<ValidationIssue> warnings;
    void logAll() const;
};

class SchemaValidator {
public:
    static ValidationResult validate(
        const ConfigStore& store,
        const ConfigBinder& binder);
};
```

当前没有 `Schema::fromYaml()`、`SchemaValidator::fromBinder()` 或实例方法 `validate(store, schema)`。

---

## 8. YamlParser

当前 `YamlParser` 是薄封装：

```cpp
class YamlParser {
public:
    static YAML::Node load(const std::string& filePath);
    static void save(const std::string& filePath, const YAML::Node& node);
    static YAML::Node merge(const YAML::Node& base, const YAML::Node& overlay);
};
```

当前没有 `populateStore()`、`fromStore()` 或 `saveDiff()` 公开 API；`ConfigStore` 自己负责 YAML flatten/unflatten 和 overrides 输出。

---

## 9. ConfigKeyId / ConfigKeyMap

`ConfigKeyId` 是 IPC ABI，不是运行时临时编号。当前分区：

| 范围 | 所属 |
|------|------|
| `0x0000-0x00FF` | Service policy |
| `0x0100-0x01FF` | Touch pipeline |
| `0x0200-0x02FF` | Stylus pipeline |
| `0x0300` | `MaxKeyId` sentinel |

当前 worktree 已为 `config/default.yaml` 中 runtime-bound/UserOverride/live-editable touch leaf keys 补齐 static `ConfigKeyId` 和 `ConfigKeyMap` 双向映射，Touch 新增 ID 保持从既有 `0x0108` 起追加到 `0x0153`。

`ConfigKeyMap` 公开 API：

```cpp
const std::unordered_map<ConfigKeyId, std::string>& keyIdToPath();
const std::unordered_map<std::string, ConfigKeyId>& pathToKeyId();
void registerKeyMapping(ConfigKeyId id, std::string_view yamlPath);
void registerRuntimeKeyMappings(const ConfigBinder& binder);
std::optional<std::string_view> tryPathForKeyId(ConfigKeyId id);
std::optional<ConfigKeyId> tryKeyIdForPath(std::string_view yamlPath);
ConfigSchemaSnapshot BuildMergedSchema(const ConfigStore& defaults, const ConfigBinder& binder);
```

v3 connected config 要求 patchable `UserOverride` key 必须有 static keyId。`ConfigDefaultYamlDriftTest` 当前 gate：

- default.yaml leaf key 必须能进入 schema。
- patchable `UserOverride` key 的 `keyId != MaxKeyId`。
- `tryPathForKeyId(keyId)` 必须 round-trip 回 YAML path。
- `tryKeyIdForPath(path)` 必须 round-trip 回 keyId。

---

## 10. TLV / IPC v3

TLV value type：

```cpp
enum class ConfigValueType : uint8_t {
    Bool = 0x00,
    Int32 = 0x01,
    Float = 0x02,
    String = 0x03,
    Null = 0x04,
};
```

当前 patch payload：

```cpp
struct ConfigPatchTlv {
    uint16_t version = 1;
    std::vector<ConfigTlvEntry> entries;
};
```

v3 catalog/snapshot payload：

```cpp
struct ConfigV3CatalogPayload {
    uint16_t version = 2;
    uint32_t schemaVersion = 0;
    uint32_t snapshotVersion = 0;
    std::vector<ConfigDescriptor> entries;
};

struct ConfigV3SnapshotPayload {
    uint16_t version = 1;
    uint32_t schemaVersion = 0;
    uint32_t snapshotVersion = 0;
    std::vector<ConfigV3SnapshotEntry> entries;
};
```

Connected IPC 使用 `GetConfigCatalogV3`、`GetConfigSnapshotV3`、`ApplyConfigPatchV3`、`PersistConfigV3`。legacy config command `40-45` 是 tombstone，不是当前 mutation/read path。

---

## 11. ConfigRuntime

`Service::ConfigRuntime` 是 Service 端 v3 配置会话所有者。公开 API：

```cpp
class ConfigRuntime {
public:
    using StartupValidator = std::function<bool(const Config::ConfigStore&)>;

    static Config::ConfigStore BuildFactoryDefaultStore();
    static Config::ConfigSchemaSnapshot BuildFactoryDefaultSchema();

    void RegisterConfigTarget(std::unique_ptr<IConfigTarget> target);
    bool Initialize(const std::string& configPath,
                    const StartupValidator& validateStartupConfig);
    ConfigV3Blob BuildCatalogV3Blob() const;
    ConfigV3Blob BuildSnapshotV3Blob() const;
    Config::ConfigStore SnapshotStore() const;
    ServiceConfigState ServiceState() const;
    void WriteServiceState(const ServiceConfigState& config);
    bool PersistServicePolicyConfig(const ServiceConfigState& config);
    V3ApplyResult ApplyConfigPatchV3(uint32_t baseSchemaVersion,
                                     uint32_t baseSnapshotVersion,
                                     const uint8_t* data,
                                     size_t size);
    V3PersistResult PersistConfigV3();
};
```

行为要点：

- `Initialize()` 加载 `default.yaml` 和可选 `overrides.yaml`，构建 schema，并注册默认 target。
- `BuildCatalogV3Blob()` / `BuildSnapshotV3Blob()` 返回 bytes、schema version、snapshot version、checksum。
- `ApplyConfigPatchV3()` 校验 base versions、TLV、keyId/path、schema policy 和 target validation。
- live key 可即时应用；restart-required key 被接受并 staged 到 desired config。
- `PersistConfigV3()` 按 `ConfigPersistPolicy::UserOverride` 写 `overrides.yaml`，默认值不写入。

`ConfigRuntimeTest` 当前覆盖 live key、restart-required key、新 touch key patch、persist、fresh runtime restart reload。

---

## 12. IConfigTarget

Service runtime target 接口：

```cpp
class IConfigTarget {
public:
    virtual ~IConfigTarget() = default;
    virtual std::string_view name() const noexcept = 0;
    virtual bool isInterested(const ConfigChangeSet& changeSet) const = 0;
    virtual ConfigTargetResult validateConfig(
        const Config::ConfigStore& candidate,
        const ConfigChangeSet& changeSet) const = 0;
    virtual ConfigTargetResult applyConfig(
        const Config::ConfigStore& candidate,
        const ConfigChangeSet& changeSet,
        ConfigApplyPhase phase) const = 0;
};
```

`ConfigChangeSet` 携带 path、keyId、旧值、新值和 entryCount。`ConfigTargetResult` 可返回一个或多个 `ConfigApplyAction`，用于 Service policy 或 pipeline runtime apply。

---

## 13. DVR Runtime Config Snapshot

DVR2 runtime config 是录制时的 schema/value snapshot，不是新的 live config owner。

当前类型：

```cpp
struct RuntimeConfigSnapshot {
    std::vector<RuntimeConfigField> fields;
    std::vector<RuntimeConfigValue> values;
    uint32_t schemaHash = 0;

    bool Empty() const;
    Config::ConfigStore toConfigStore() const;
};
```

`RuntimeConfigSnapshot::toConfigStore()` 当前将有效且 schema/value type 匹配的 value 转为 `ConfigStore`：

| DVR value type | ConfigStore value |
|----------------|-------------------|
| `Bool` | `bool` |
| `Int32` | `int32_t` |
| `UInt8` / `UInt16` | `int32_t` |
| `UInt32` | `int32_t` only when `value <= INT32_MAX`; larger values are skipped |
| `Float32` / `Float64` | `float` |
| `String` | `std::string` |

path 由 `RuntimeConfigField.section` 和 `RuntimeConfigField.key` 组成；两者都有值时使用 `section.key`。

`ServiceProxy::CaptureRuntimeConfigSnapshot()` 当前从 App/Service runtime state、TouchPipeline 和 StylusPipeline 生成 DVR runtime config snapshot，fieldId 连续分配，并用 DVR2 schema hash 记录 schema identity。`EGoTouchApp.ServiceProxyRuntimeConfigSnapshotTest` 和 `DvrCoreRuntimeConfigRoundTripTest` 覆盖该路径。

---

## 14. Packaging / Install Layout

当前 build 和 packaging 对 config layout 的约束：

- `EGoTouchApp` 和 `EGoTouchService` build output 旁必须存在 `config/default.yaml`。
- `install(FILES config/default.yaml DESTINATION config COMPONENT Runtime)` 安装默认配置。
- `scripts/EGoTouchSetup.wxs` 和 `scripts/EGoTouchTestSetup.wxs` 均包含 `CONFIGFOLDER` 和 `DefaultConfigComp`，通过 `$(var.BuildOutputDir)\config\default.yaml` 引用当前 build output。
- `PackagingConfigLayoutTest` 通过 `cmake/VerifyConfigLayout.cmake` 校验 exe 旁 config、真实 `cmake --install` prefix、hash 与仓库 `config/default.yaml` 一致，并校验 WiX `DefaultConfigYAML` source 文件存在且 hash 一致。

---

## 15. 当前不应再引用为事实的旧目标态 API

以下名称可能出现在历史设计文档中，但不是当前公开 API：

| 旧名称 | 当前事实 |
|--------|----------|
| `ConfigValue fromString(...)` | 未实现 |
| `Type typeOf(...)` / `Config::Type` | 未实现；使用 `ConfigValueType` 或 `ConfigUiType` |
| `ConfigStore::snapshot()` | 未实现；runtime snapshot 由 `ConfigRuntime::BuildSnapshotV3Blob()` 生成 |
| `ConfigStore::applyPatch()` | 未实现；patch 由 `ConfigRuntime::ApplyConfigPatchV3()` 处理 |
| `ConfigStore::diffKeys()` | 未实现；persist diff 使用 `saveOverrides(path, defaults)` |
| `ConfigException` | 未实现 |
| `ConfigBinder::toSchema()` | 未实现；使用 `snapshot()` / `populateSchema()` |
| `ConfigBinder::readMembers()` | 未实现；使用 `writeCurrent()` |
| `Schema::fromYaml()` | 未实现 |
| `SchemaValidator::fromBinder()` | 未实现 |
| `YamlParser::populateStore()` / `fromStore()` / `saveDiff()` | 未实现 |
