#pragma once

#include "ConfigKeyId.h"
#include "ConfigValue.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace Config {

// ── 值类型标签 ──
enum class ConfigValueType : uint8_t {
    Bool    = 0x00,
    Int32   = 0x01,
    Float   = 0x02,
    String  = 0x03,
    Null    = 0x04,   // 用于 delete key
};

// ── 单条 TLV 记录: 5 + N bytes ──
struct ConfigTlvEntry {
    ConfigKeyId keyId;
    ConfigValueType valueType;
    std::string stringValue;   // bool/int/float 序列化为 string 用于 IPC（保持简单）
};

// ── Snapshot: 全量快照 ──
struct ConfigSnapshotTlv {
    uint16_t version = 1;
    std::vector<ConfigTlvEntry> entries;
};

// ── Patch: 增量变更 ──
struct ConfigPatchTlv {
    uint16_t version = 1;
    std::vector<ConfigTlvEntry> entries;  // 仅变更的键
};

enum class ConfigTlvParseStatus : uint8_t {
    Ok = 0,
    EmptyPatch = 1,
    UnsupportedVersion = 2,
    TruncatedHeader = 3,
    TruncatedEntry = 4,
    UnknownKeyId = 5,
    UnknownValueType = 6,
    DuplicateKey = 7,
    TrailingBytes = 8,
    InvalidArgument = 9,
};

struct ConfigTlvParseIssue {
    ConfigTlvParseStatus status = ConfigTlvParseStatus::Ok;
    size_t offset = 0;
    uint16_t entryIndex = 0;
    uint16_t rawKeyId = 0;
    uint8_t rawValueType = 0;
};

struct ConfigTlvParseResult {
    ConfigTlvParseStatus status = ConfigTlvParseStatus::Ok;
    ConfigPatchTlv patch;
    ConfigTlvParseIssue issue;

    [[nodiscard]] bool ok() const noexcept { return status == ConfigTlvParseStatus::Ok; }
};

const char* toString(ConfigTlvParseStatus status) noexcept;

ConfigTlvParseResult tryDeserializePatch(const uint8_t* data, size_t size);
ConfigTlvParseResult deserializePatchDetailed(const uint8_t* data, size_t size);

// ── Mutation Result ──
struct ConfigMutationResultTlv {
    uint16_t changedCount = 0;
    uint16_t appliedCount = 0;
    bool restartRequired = false;
};

// ── 序列化 ──

// 将 Snapshot 序列化为原始字节 (wire format)
std::vector<uint8_t> serializeSnapshot(const ConfigSnapshotTlv& snapshot);

// 从原始字节反序列化
ConfigSnapshotTlv deserializeSnapshot(const uint8_t* data, size_t size);

// 将 Patch 序列化为原始字节
std::vector<uint8_t> serializePatch(const ConfigPatchTlv& patch);

// 从原始字节反序列化
ConfigPatchTlv deserializePatch(const uint8_t* data, size_t size);

// 将 MutationResult 序列化为原始字节
std::vector<uint8_t> serializeMutationResult(const ConfigMutationResultTlv& result);

// 从原始字节反序列化
ConfigMutationResultTlv deserializeMutationResult(const uint8_t* data, size_t size);

} // namespace Config
