#include "config/ConfigTlv.h"

#include "config/ConfigCatalog.h"
#include "config/ConfigKeyMap.h"
#include "Logger.h"

#include <algorithm>
#include <cstring>
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
    return tryPathForKeyId(keyId).has_value();
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
        if (offset != size) {
            return fail(ConfigTlvParseStatus::TrailingBytes, offset);
        }
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

namespace {
constexpr uint32_t kConfigV3CatalogMagic = 0x33435643u; // 'CVC3'
constexpr uint32_t kConfigV3SnapshotMagic = 0x33535643u; // 'CVS3'
constexpr uint16_t kConfigV3Version = 1;
constexpr size_t kConfigV3MinCatalogEntrySize = sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint8_t);
constexpr size_t kConfigV3MinSnapshotEntrySize = sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint16_t);

void appendUint32Le(std::vector<uint8_t>& out, uint32_t value) {
    appendUint16Le(out, static_cast<uint16_t>(value & 0xFFFFu));
    appendUint16Le(out, static_cast<uint16_t>((value >> 16) & 0xFFFFu));
}

bool readUint32Le(const uint8_t* data, size_t size, size_t& offset, uint32_t& value) {
    uint16_t lo = 0;
    uint16_t hi = 0;
    if (!readUint16Le(data, size, offset, lo) || !readUint16Le(data, size, offset, hi)) {
        return false;
    }
    value = static_cast<uint32_t>(lo) | (static_cast<uint32_t>(hi) << 16);
    return true;
}

void appendString(std::vector<uint8_t>& out, const std::string& value) {
    if (value.size() > std::numeric_limits<uint16_t>::max()) {
        throw std::length_error("Config v3 string is too large");
    }
    appendUint16Le(out, static_cast<uint16_t>(value.size()));
    appendBytes(out, value.data(), value.size());
}

std::string readStringStrict(const uint8_t* data, size_t size, size_t& offset) {
    uint16_t len = 0;
    if (!readUint16Le(data, size, offset, len) || offset > size || size - offset < len) {
        throw std::runtime_error("Config v3 truncated string");
    }
    std::string value(reinterpret_cast<const char*>(data + offset), len);
    offset += len;
    return value;
}

void appendValue(std::vector<uint8_t>& out, const ConfigValue& value) {
    if (std::holds_alternative<bool>(value)) {
        appendUint8(out, static_cast<uint8_t>(ConfigValueType::Bool));
        appendUint8(out, std::get<bool>(value) ? 1 : 0);
    } else if (std::holds_alternative<int32_t>(value)) {
        appendUint8(out, static_cast<uint8_t>(ConfigValueType::Int32));
        appendUint32Le(out, static_cast<uint32_t>(std::get<int32_t>(value)));
    } else if (std::holds_alternative<float>(value)) {
        appendUint8(out, static_cast<uint8_t>(ConfigValueType::Float));
        uint32_t bits = 0;
        static_assert(sizeof(bits) == sizeof(float));
        std::memcpy(&bits, &std::get<float>(value), sizeof(bits));
        appendUint32Le(out, bits);
    } else {
        appendUint8(out, static_cast<uint8_t>(ConfigValueType::String));
        appendString(out, std::get<std::string>(value));
    }
}

void appendFloat64Le(std::vector<uint8_t>& out, double value) {
    uint64_t bits = 0;
    static_assert(sizeof(bits) == sizeof(double));
    std::memcpy(&bits, &value, sizeof(bits));
    appendUint32Le(out, static_cast<uint32_t>(bits & 0xFFFFFFFFu));
    appendUint32Le(out, static_cast<uint32_t>((bits >> 32) & 0xFFFFFFFFu));
}

bool readFloat64Le(const uint8_t* data, size_t size, size_t& offset, double& value) {
    uint32_t lo = 0;
    uint32_t hi = 0;
    if (!readUint32Le(data, size, offset, lo) || !readUint32Le(data, size, offset, hi)) {
        return false;
    }
    uint64_t bits = static_cast<uint64_t>(lo) | (static_cast<uint64_t>(hi) << 32);
    std::memcpy(&value, &bits, sizeof(value));
    return true;
}

ConfigValue readValueStrict(const uint8_t* data, size_t size, size_t& offset) {
    uint8_t rawType = 0;
    if (!readUint8(data, size, offset, rawType)) {
        throw std::runtime_error("Config v3 truncated value type");
    }
    switch (static_cast<ConfigValueType>(rawType)) {
    case ConfigValueType::Bool: {
        uint8_t v = 0;
        if (!readUint8(data, size, offset, v) || v > 1) throw std::runtime_error("Config v3 invalid bool");
        return v != 0;
    }
    case ConfigValueType::Int32: {
        uint32_t v = 0;
        if (!readUint32Le(data, size, offset, v)) throw std::runtime_error("Config v3 truncated int32");
        return static_cast<int32_t>(v);
    }
    case ConfigValueType::Float: {
        uint32_t bits = 0;
        float v = 0.0f;
        if (!readUint32Le(data, size, offset, bits)) throw std::runtime_error("Config v3 truncated float");
        std::memcpy(&v, &bits, sizeof(v));
        return v;
    }
    case ConfigValueType::String:
        return readStringStrict(data, size, offset);
    case ConfigValueType::Null:
        return std::string{};
    default:
        throw std::runtime_error("Config v3 unsupported value type");
    }
}

void appendHeader(std::vector<uint8_t>& out, uint32_t magic, uint16_t version, uint32_t schemaVersion, uint32_t snapshotVersion, size_t count) {
    if (count > std::numeric_limits<uint32_t>::max()) throw std::length_error("too many Config v3 entries");
    appendUint32Le(out, magic);
    appendUint16Le(out, version);
    appendUint32Le(out, schemaVersion);
    appendUint32Le(out, snapshotVersion);
    appendUint32Le(out, static_cast<uint32_t>(count));
}

uint32_t readHeaderStrict(const uint8_t* data, size_t size, size_t& offset, uint32_t expectedMagic, uint32_t& schemaVersion, uint32_t& snapshotVersion) {
    uint32_t magic = 0;
    uint16_t version = 0;
    uint32_t count = 0;
    if (data == nullptr || !readUint32Le(data, size, offset, magic) || !readUint16Le(data, size, offset, version) ||
        !readUint32Le(data, size, offset, schemaVersion) || !readUint32Le(data, size, offset, snapshotVersion) ||
        !readUint32Le(data, size, offset, count)) {
        throw std::runtime_error("Config v3 truncated header");
    }
    if (magic != expectedMagic) throw std::runtime_error("Config v3 invalid magic");
    if (version != kConfigV3Version) throw std::runtime_error("Config v3 unsupported version");
    return count;
}

void rejectEntryCountIfImpossible(uint32_t count, size_t remainingBytes, size_t minEntryBytes) {
    if (minEntryBytes == 0 || count > remainingBytes / minEntryBytes) {
        throw std::runtime_error("Config v3 entry count exceeds payload size");
    }
}

bool isKnownUiType(uint8_t uiType) {
    switch (static_cast<ConfigUiType>(uiType)) {
    case ConfigUiType::Bool:
    case ConfigUiType::Int32:
    case ConfigUiType::Float:
    case ConfigUiType::String:
    case ConfigUiType::Enum:
        return true;
    default:
        return false;
    }
}

bool isKnownRuntimeBinding(uint8_t binding) {
    switch (static_cast<ConfigRuntimeBinding>(binding)) {
    case ConfigRuntimeBinding::SchemaOnly:
    case ConfigRuntimeBinding::LiveSetter:
    case ConfigRuntimeBinding::ManualLiveApply:
    case ConfigRuntimeBinding::Removed:
        return true;
    default:
        return false;
    }
}
} // namespace

std::vector<uint8_t> serializeConfigV3Catalog(const ConfigV3CatalogPayload& payload) {
    std::vector<ConfigDescriptor> entries = payload.entries;
    std::stable_sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
        if (a.keyId != b.keyId) return static_cast<uint16_t>(a.keyId) < static_cast<uint16_t>(b.keyId);
        return a.path < b.path;
    });
    std::vector<uint8_t> out;
    appendHeader(out, kConfigV3CatalogMagic, payload.version, payload.schemaVersion, payload.snapshotVersion, entries.size());
    for (const auto& e : entries) {
        appendString(out, e.path);
        appendUint16Le(out, static_cast<uint16_t>(e.keyId));
        appendUint8(out, static_cast<uint8_t>(e.uiType));
        appendValue(out, e.defaultValue);
        appendUint8(out, e.range.has_value() ? 1 : 0);
        if (e.range) {
            appendFloat64Le(out, e.range->min);
            appendFloat64Le(out, e.range->max);
        }
        appendString(out, e.displayName);
        appendString(out, e.description);
        appendString(out, e.moduleTag);
        if (e.enumMapping.size() > std::numeric_limits<uint16_t>::max()) {
            throw std::length_error("Config v3 enum mapping is too large");
        }
        appendUint16Le(out, static_cast<uint16_t>(e.enumMapping.size()));
        for (const auto& [value, name] : e.enumMapping) {
            appendUint32Le(out, static_cast<uint32_t>(value));
            appendString(out, name);
        }
        appendUint8(out, static_cast<uint8_t>(e.runtimeBinding));
        appendUint8(out, e.boundToRuntime ? 1 : 0);
    }
    return out;
}

ConfigV3CatalogPayload deserializeConfigV3Catalog(const uint8_t* data, size_t size) {
    size_t offset = 0;
    ConfigV3CatalogPayload payload{};
    uint32_t count = readHeaderStrict(data, size, offset, kConfigV3CatalogMagic, payload.schemaVersion, payload.snapshotVersion);
    rejectEntryCountIfImpossible(count, size - offset, kConfigV3MinCatalogEntrySize);
    payload.entries.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        ConfigDescriptor e;
        e.path = readStringStrict(data, size, offset);
        uint16_t key = 0;
        uint8_t uiType = 0;
        if (!readUint16Le(data, size, offset, key) || !readUint8(data, size, offset, uiType)) throw std::runtime_error("Config v3 truncated catalog entry");
        if (!isKnownUiType(uiType)) throw std::runtime_error("Config v3 invalid ui type");
        e.keyId = static_cast<ConfigKeyId>(key);
        e.uiType = static_cast<ConfigUiType>(uiType);
        e.defaultValue = readValueStrict(data, size, offset);
        uint8_t hasRange = 0;
        if (!readUint8(data, size, offset, hasRange)) throw std::runtime_error("Config v3 truncated range flag");
        if (hasRange > 1) throw std::runtime_error("Config v3 invalid range flag");
        if (hasRange) {
            ConfigRange range{};
            if (!readFloat64Le(data, size, offset, range.min) || !readFloat64Le(data, size, offset, range.max)) throw std::runtime_error("Config v3 truncated range");
            e.range = range;
        }
        e.displayName = readStringStrict(data, size, offset);
        e.description = readStringStrict(data, size, offset);
        e.moduleTag = readStringStrict(data, size, offset);
        uint16_t enumCount = 0;
        if (!readUint16Le(data, size, offset, enumCount)) throw std::runtime_error("Config v3 truncated enum count");
        for (uint16_t j = 0; j < enumCount; ++j) {
            uint32_t v = 0;
            if (!readUint32Le(data, size, offset, v)) throw std::runtime_error("Config v3 truncated enum value");
            e.enumMapping.emplace_back(static_cast<int>(v), readStringStrict(data, size, offset));
        }
        uint8_t binding = 0;
        uint8_t bound = 0;
        if (!readUint8(data, size, offset, binding) || !readUint8(data, size, offset, bound)) throw std::runtime_error("Config v3 truncated binding");
        if (!isKnownRuntimeBinding(binding)) throw std::runtime_error("Config v3 invalid runtime binding");
        if (bound > 1) throw std::runtime_error("Config v3 invalid bound flag");
        e.runtimeBinding = static_cast<ConfigRuntimeBinding>(binding);
        e.boundToRuntime = bound != 0;
        payload.entries.push_back(std::move(e));
    }
    if (offset != size) throw std::runtime_error("Config v3 trailing bytes");
    return payload;
}

std::vector<uint8_t> serializeConfigV3Snapshot(const ConfigV3SnapshotPayload& payload) {
    auto entries = payload.entries;
    std::stable_sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) { return static_cast<uint16_t>(a.keyId) < static_cast<uint16_t>(b.keyId); });
    std::vector<uint8_t> out;
    appendHeader(out, kConfigV3SnapshotMagic, payload.version, payload.schemaVersion, payload.snapshotVersion, entries.size());
    for (const auto& e : entries) {
        appendUint16Le(out, static_cast<uint16_t>(e.keyId));
        appendValue(out, e.value);
    }
    return out;
}

ConfigV3SnapshotPayload deserializeConfigV3Snapshot(const uint8_t* data, size_t size) {
    size_t offset = 0;
    ConfigV3SnapshotPayload payload{};
    uint32_t count = readHeaderStrict(data, size, offset, kConfigV3SnapshotMagic, payload.schemaVersion, payload.snapshotVersion);
    rejectEntryCountIfImpossible(count, size - offset, kConfigV3MinSnapshotEntrySize);
    payload.entries.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        uint16_t key = 0;
        if (!readUint16Le(data, size, offset, key)) throw std::runtime_error("Config v3 truncated snapshot entry");
        payload.entries.push_back(ConfigV3SnapshotEntry{static_cast<ConfigKeyId>(key), readValueStrict(data, size, offset)});
    }
    if (offset != size) throw std::runtime_error("Config v3 trailing bytes");
    return payload;
}

} // namespace Config
