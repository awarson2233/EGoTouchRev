#include "config/ConfigTlv.h"

#include "Logger.h"

#include <limits>
#include <set>
#include <utility>

namespace Config {
namespace {

constexpr uint16_t kCurrentVersion = 1;
constexpr size_t kSnapshotHeaderSize = sizeof(uint16_t) + sizeof(uint16_t);
constexpr size_t kEntryHeaderSize = sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint16_t);
constexpr size_t kMutationResultSize = sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint8_t);

void appendBytes(std::vector<uint8_t>& out, const void* data, size_t size)
{
    const auto* bytes = static_cast<const uint8_t*>(data);
    out.insert(out.end(), bytes, bytes + size);
}

void appendUint16Le(std::vector<uint8_t>& out, uint16_t value)
{
    out.push_back(static_cast<uint8_t>(value & 0x00FFu));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0x00FFu));
}

void appendUint8(std::vector<uint8_t>& out, uint8_t value)
{
    out.push_back(value);
}

bool readUint16Le(const uint8_t* data, size_t size, size_t& offset, uint16_t& value)
{
    if (offset > size || size - offset < sizeof(uint16_t)) {
        return false;
    }

    value = static_cast<uint16_t>(data[offset]) |
            static_cast<uint16_t>(static_cast<uint16_t>(data[offset + 1]) << 8);
    offset += sizeof(uint16_t);
    return true;
}

bool readUint8(const uint8_t* data, size_t size, size_t& offset, uint8_t& value)
{
    if (offset > size || size - offset < sizeof(uint8_t)) {
        return false;
    }

    value = data[offset];
    offset += sizeof(uint8_t);
    return true;
}

bool isKnownKeyId(ConfigKeyId keyId)
{
    return static_cast<uint16_t>(keyId) < static_cast<uint16_t>(ConfigKeyId::MaxKeyId);
}

bool isKnownValueType(ConfigValueType valueType)
{
    switch (valueType) {
    case ConfigValueType::Bool:
    case ConfigValueType::Int32:
    case ConfigValueType::Float:
    case ConfigValueType::String:
    case ConfigValueType::Null:
        return true;
    default:
        return false;
    }
}

void logParseIssue(const char* method, const ConfigTlvParseIssue& issue)
{
    LOG_WARN("Config", method, "Deserialize", "Config TLV parse failed: status={} offset={} entryIndex={} keyId=0x{:04X} valueType=0x{:02X}",
             toString(issue.status), issue.offset, issue.entryIndex, issue.rawKeyId, issue.rawValueType);
}

std::vector<uint8_t> serializeEntries(uint16_t version, const std::vector<ConfigTlvEntry>& entries)
{
    if (entries.size() > std::numeric_limits<uint16_t>::max()) {
        throw std::length_error("too many Config TLV entries");
    }

    std::vector<uint8_t> out;
    out.reserve(kSnapshotHeaderSize + entries.size() * kEntryHeaderSize);
    appendUint16Le(out, version);
    appendUint16Le(out, static_cast<uint16_t>(entries.size()));

    for (const ConfigTlvEntry& entry : entries) {
        if (entry.stringValue.size() > std::numeric_limits<uint16_t>::max()) {
            throw std::length_error("Config TLV entry payload is too large");
        }

        appendUint16Le(out, static_cast<uint16_t>(entry.keyId));
        appendUint8(out, static_cast<uint8_t>(entry.valueType));
        appendUint16Le(out, static_cast<uint16_t>(entry.stringValue.size()));
        appendBytes(out, entry.stringValue.data(), entry.stringValue.size());
    }

    return out;
}

template <typename T>
T deserializeEntriesLegacy(const uint8_t* data, size_t size, const char* method)
{
    T result{};
    if (data == nullptr || size < kSnapshotHeaderSize) {
        ConfigTlvParseIssue issue{ConfigTlvParseStatus::TruncatedHeader, 0, 0, 0, 0};
        logParseIssue(method, issue);
        return result;
    }

    size_t offset = 0;
    uint16_t version = 0;
    uint16_t entryCount = 0;
    if (!readUint16Le(data, size, offset, version) || !readUint16Le(data, size, offset, entryCount)) {
        ConfigTlvParseIssue issue{ConfigTlvParseStatus::TruncatedHeader, offset, 0, 0, 0};
        logParseIssue(method, issue);
        return T{};
    }

    if (version != kCurrentVersion) {
        ConfigTlvParseIssue issue{ConfigTlvParseStatus::UnsupportedVersion, 0, 0, version, 0};
        logParseIssue(method, issue);
        return T{};
    }

    result.version = version;
    result.entries.reserve(entryCount);

    for (uint16_t i = 0; i < entryCount; ++i) {
        uint16_t rawKeyId = 0;
        uint8_t rawValueType = 0;
        uint16_t valueLen = 0;
        if (!readUint16Le(data, size, offset, rawKeyId) ||
            !readUint8(data, size, offset, rawValueType) ||
            !readUint16Le(data, size, offset, valueLen)) {
            ConfigTlvParseIssue issue{ConfigTlvParseStatus::TruncatedEntry, offset, i, rawKeyId, rawValueType};
            logParseIssue(method, issue);
            return T{};
        }

        if (offset > size || size - offset < valueLen) {
            ConfigTlvParseIssue issue{ConfigTlvParseStatus::TruncatedEntry, offset, i, rawKeyId, rawValueType};
            logParseIssue(method, issue);
            return T{};
        }

        std::string valuePayload(reinterpret_cast<const char*>(data + offset), valueLen);
        offset += valueLen;

        ConfigTlvEntry entry{
            .keyId = static_cast<ConfigKeyId>(rawKeyId),
            .valueType = static_cast<ConfigValueType>(rawValueType),
            .stringValue = std::move(valuePayload),
        };

        if (!isKnownKeyId(entry.keyId)) {
            LOG_WARN("Config", method, "Deserialize", "unknown keyId: 0x{:04X}", rawKeyId);
        }
        if (!isKnownValueType(entry.valueType)) {
            LOG_WARN("Config", method, "Deserialize", "unknown valueType: 0x{:02X}", rawValueType);
        }

        result.entries.push_back(std::move(entry));
    }

    if (offset != size) {
        LOG_WARN("Config", method, "Deserialize", "ignored {} trailing byte(s)", size - offset);
    }

    return result;
}

ConfigTlvParseResult fail(ConfigTlvParseStatus status, size_t offset, uint16_t entryIndex = 0,
                          uint16_t rawKeyId = 0, uint8_t rawValueType = 0)
{
    ConfigTlvParseResult result{};
    result.status = status;
    result.issue = ConfigTlvParseIssue{status, offset, entryIndex, rawKeyId, rawValueType};
    return result;
}

} // namespace

const char* toString(ConfigTlvParseStatus status) noexcept
{
    switch (status) {
    case ConfigTlvParseStatus::Ok: return "Ok";
    case ConfigTlvParseStatus::EmptyPatch: return "EmptyPatch";
    case ConfigTlvParseStatus::UnsupportedVersion: return "UnsupportedVersion";
    case ConfigTlvParseStatus::TruncatedHeader: return "TruncatedHeader";
    case ConfigTlvParseStatus::TruncatedEntry: return "TruncatedEntry";
    case ConfigTlvParseStatus::UnknownKeyId: return "UnknownKeyId";
    case ConfigTlvParseStatus::UnknownValueType: return "UnknownValueType";
    case ConfigTlvParseStatus::DuplicateKey: return "DuplicateKey";
    case ConfigTlvParseStatus::TrailingBytes: return "TrailingBytes";
    case ConfigTlvParseStatus::InvalidArgument: return "InvalidArgument";
    }
    return "Unknown";
}

std::vector<uint8_t> serializeSnapshot(const ConfigSnapshotTlv& snapshot)
{
    return serializeEntries(snapshot.version, snapshot.entries);
}

ConfigSnapshotTlv deserializeSnapshot(const uint8_t* data, size_t size)
{
    return deserializeEntriesLegacy<ConfigSnapshotTlv>(data, size, "deserializeSnapshot");
}

std::vector<uint8_t> serializePatch(const ConfigPatchTlv& patch)
{
    return serializeEntries(patch.version, patch.entries);
}

ConfigTlvParseResult tryDeserializePatch(const uint8_t* data, size_t size)
{
    if (data == nullptr || size < kSnapshotHeaderSize) {
        return fail(data == nullptr ? ConfigTlvParseStatus::InvalidArgument : ConfigTlvParseStatus::TruncatedHeader, 0);
    }

    size_t offset = 0;
    uint16_t version = 0;
    uint16_t entryCount = 0;
    if (!readUint16Le(data, size, offset, version) || !readUint16Le(data, size, offset, entryCount)) {
        return fail(ConfigTlvParseStatus::TruncatedHeader, offset);
    }

    if (version != kCurrentVersion) {
        return fail(ConfigTlvParseStatus::UnsupportedVersion, 0, 0, version, 0);
    }
    if (entryCount == 0) {
        return fail(ConfigTlvParseStatus::EmptyPatch, offset);
    }

    ConfigTlvParseResult result{};
    result.patch.version = version;
    result.patch.entries.reserve(entryCount);
    std::set<uint16_t> seenKeys;

    for (uint16_t i = 0; i < entryCount; ++i) {
        uint16_t rawKeyId = 0;
        uint8_t rawValueType = 0;
        uint16_t valueLen = 0;
        if (!readUint16Le(data, size, offset, rawKeyId) ||
            !readUint8(data, size, offset, rawValueType) ||
            !readUint16Le(data, size, offset, valueLen)) {
            return fail(ConfigTlvParseStatus::TruncatedEntry, offset, i, rawKeyId, rawValueType);
        }
        if (!isKnownKeyId(static_cast<ConfigKeyId>(rawKeyId))) {
            return fail(ConfigTlvParseStatus::UnknownKeyId, offset, i, rawKeyId, rawValueType);
        }
        if (!isKnownValueType(static_cast<ConfigValueType>(rawValueType))) {
            return fail(ConfigTlvParseStatus::UnknownValueType, offset, i, rawKeyId, rawValueType);
        }
        if (!seenKeys.insert(rawKeyId).second) {
            return fail(ConfigTlvParseStatus::DuplicateKey, offset, i, rawKeyId, rawValueType);
        }
        if (offset > size || size - offset < valueLen) {
            return fail(ConfigTlvParseStatus::TruncatedEntry, offset, i, rawKeyId, rawValueType);
        }

        result.patch.entries.push_back(ConfigTlvEntry{
            .keyId = static_cast<ConfigKeyId>(rawKeyId),
            .valueType = static_cast<ConfigValueType>(rawValueType),
            .stringValue = std::string(reinterpret_cast<const char*>(data + offset), valueLen),
        });
        offset += valueLen;
    }

    if (offset != size) {
        return fail(ConfigTlvParseStatus::TrailingBytes, offset);
    }

    result.status = ConfigTlvParseStatus::Ok;
    result.issue = ConfigTlvParseIssue{};
    return result;
}

ConfigTlvParseResult deserializePatchDetailed(const uint8_t* data, size_t size)
{
    const auto result = tryDeserializePatch(data, size);
    if (!result.ok()) {
        logParseIssue("deserializePatchDetailed", result.issue);
    }
    return result;
}

ConfigPatchTlv deserializePatch(const uint8_t* data, size_t size)
{
    return deserializeEntriesLegacy<ConfigPatchTlv>(data, size, "deserializePatch");
}

std::vector<uint8_t> serializeMutationResult(const ConfigMutationResultTlv& result)
{
    std::vector<uint8_t> out;
    out.reserve(kMutationResultSize);
    appendUint16Le(out, result.changedCount);
    appendUint16Le(out, result.appliedCount);
    appendUint8(out, result.restartRequired ? uint8_t{1} : uint8_t{0});
    return out;
}

ConfigMutationResultTlv deserializeMutationResult(const uint8_t* data, size_t size)
{
    ConfigMutationResultTlv result{};
    if (data == nullptr || size < kMutationResultSize) {
        ConfigTlvParseIssue issue{ConfigTlvParseStatus::TruncatedHeader, 0, 0, 0, 0};
        logParseIssue("deserializeMutationResult", issue);
        return result;
    }

    size_t offset = 0;
    uint8_t restartRequired = 0;
    if (!readUint16Le(data, size, offset, result.changedCount) ||
        !readUint16Le(data, size, offset, result.appliedCount) ||
        !readUint8(data, size, offset, restartRequired)) {
        ConfigTlvParseIssue issue{ConfigTlvParseStatus::TruncatedEntry, offset, 0, 0, 0};
        logParseIssue("deserializeMutationResult", issue);
        return ConfigMutationResultTlv{};
    }

    result.restartRequired = restartRequired != 0;

    if (offset != size) {
        LOG_WARN("Config", "deserializeMutationResult", "Deserialize", "ignored {} trailing byte(s)", size - offset);
    }

    return result;
}

} // namespace Config
