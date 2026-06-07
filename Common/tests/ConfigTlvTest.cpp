#include "config/ConfigTlv.h"

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <limits>
#include <utility>
#include <vector>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "[FAIL] " << message << "\n";
        std::exit(1);
    }
}

void AppendU16(std::vector<uint8_t>& bytes, uint16_t value) {
    bytes.push_back(static_cast<uint8_t>(value & 0x00FFu));
    bytes.push_back(static_cast<uint8_t>((value >> 8) & 0x00FFu));
}

void AppendU32(std::vector<uint8_t>& bytes, uint32_t value) {
    AppendU16(bytes, static_cast<uint16_t>(value & 0xFFFFu));
    AppendU16(bytes, static_cast<uint16_t>((value >> 16) & 0xFFFFu));
}

void AppendV3Header(std::vector<uint8_t>& bytes, uint32_t magic, uint32_t count) {
    AppendU32(bytes, magic);
    AppendU16(bytes, 1);
    AppendU32(bytes, 1);
    AppendU32(bytes, 1);
    AppendU32(bytes, count);
}

void AppendV3String(std::vector<uint8_t>& bytes, const char* value) {
    uint16_t len = 0;
    while (value[len] != '\0') {
        ++len;
    }
    AppendU16(bytes, len);
    for (uint16_t i = 0; i < len; ++i) {
        bytes.push_back(static_cast<uint8_t>(value[i]));
    }
}

void AppendMinimalCatalogEntry(std::vector<uint8_t>& bytes, uint8_t uiType, uint8_t runtimeBinding) {
    AppendV3String(bytes, "service.auto_mode");
    AppendU16(bytes, static_cast<uint16_t>(Config::ConfigKeyId::SvcAutoMode));
    bytes.push_back(uiType);
    bytes.push_back(static_cast<uint8_t>(Config::ConfigValueType::Bool));
    bytes.push_back(1);
    bytes.push_back(0);
    AppendV3String(bytes, "Auto Mode");
    AppendV3String(bytes, "");
    AppendV3String(bytes, "Service");
    AppendU16(bytes, 0);
    bytes.push_back(runtimeBinding);
    bytes.push_back(0);
}

template <typename Fn>
void RequireThrows(Fn&& fn, const char* message) {
    try {
        fn();
    } catch (const std::exception&) {
        return;
    }
    Require(false, message);
}

void AppendEntry(std::vector<uint8_t>& bytes, uint16_t keyId, uint8_t valueType, const char* value) {
    uint16_t len = 0;
    while (value[len] != '\0') {
        ++len;
    }
    AppendU16(bytes, keyId);
    bytes.push_back(valueType);
    AppendU16(bytes, len);
    for (uint16_t i = 0; i < len; ++i) {
        bytes.push_back(static_cast<uint8_t>(value[i]));
    }
}

std::vector<uint8_t> Header(uint16_t version, uint16_t count) {
    std::vector<uint8_t> bytes;
    AppendU16(bytes, version);
    AppendU16(bytes, count);
    return bytes;
}

void RequireStatus(const std::vector<uint8_t>& bytes, Config::ConfigTlvParseStatus status, const char* message) {
    const auto result = Config::tryDeserializePatch(bytes.data(), bytes.size());
    Require(result.status == status, message);
}

} // namespace

int main() {
    using namespace Config;

    ConfigPatchTlv patch{};
    patch.entries.push_back(ConfigTlvEntry{ConfigKeyId::SvcAutoMode, ConfigValueType::Bool, "true"});
    patch.entries.push_back(ConfigTlvEntry{ConfigKeyId::SvcPenButtonMode, ConfigValueType::String, "native_barrel"});

    const auto payload = serializePatch(patch);
    const auto parsed = tryDeserializePatch(payload.data(), payload.size());
    Require(parsed.ok(), "roundtrip parse succeeds");
    Require(parsed.patch.version == 1, "roundtrip version remains 1");
    Require(parsed.patch.entries.size() == 2, "roundtrip entry count matches");
    Require(parsed.patch.entries[0].keyId == ConfigKeyId::SvcAutoMode, "roundtrip key matches");
    Require(parsed.patch.entries[0].valueType == ConfigValueType::Bool, "roundtrip type matches");
    Require(parsed.patch.entries[0].stringValue == "true", "roundtrip value matches");

    const auto legacy = deserializePatch(payload.data(), payload.size());
    Require(legacy.entries.size() == 2, "legacy deserializePatch remains usable");

    std::vector<uint8_t> truncatedHeader{0x01, 0x00, 0x01};
    RequireStatus(truncatedHeader, ConfigTlvParseStatus::TruncatedHeader, "truncated header is structured");

    auto unsupported = Header(2, 1);
    AppendEntry(unsupported, static_cast<uint16_t>(ConfigKeyId::SvcAutoMode), static_cast<uint8_t>(ConfigValueType::Bool), "true");
    RequireStatus(unsupported, ConfigTlvParseStatus::UnsupportedVersion, "unsupported version is structured");

    auto unknownKey = Header(1, 1);
    AppendEntry(unknownKey, static_cast<uint16_t>(ConfigKeyId::MaxKeyId), static_cast<uint8_t>(ConfigValueType::Bool), "true");
    RequireStatus(unknownKey, ConfigTlvParseStatus::UnknownKeyId, "unknown key is structured");

    auto unmappedKey = Header(1, 1);
    AppendEntry(unmappedKey, 0x00FEu, static_cast<uint8_t>(ConfigValueType::Bool), "true");
    RequireStatus(unmappedKey, ConfigTlvParseStatus::UnknownKeyId, "unmapped key below MaxKeyId is structured");

    auto unknownType = Header(1, 1);
    AppendEntry(unknownType, static_cast<uint16_t>(ConfigKeyId::SvcAutoMode), 0x7Fu, "true");
    RequireStatus(unknownType, ConfigTlvParseStatus::UnknownValueType, "unknown value type is structured");

    auto duplicate = Header(1, 2);
    AppendEntry(duplicate, static_cast<uint16_t>(ConfigKeyId::SvcAutoMode), static_cast<uint8_t>(ConfigValueType::Bool), "true");
    AppendEntry(duplicate, static_cast<uint16_t>(ConfigKeyId::SvcAutoMode), static_cast<uint8_t>(ConfigValueType::Bool), "false");
    RequireStatus(duplicate, ConfigTlvParseStatus::DuplicateKey, "duplicate key is structured");

    auto empty = Header(1, 0);
    RequireStatus(empty, ConfigTlvParseStatus::EmptyPatch, "empty patch is structured");

    auto emptyTrailing = Header(1, 0);
    emptyTrailing.push_back(0xEE);
    RequireStatus(emptyTrailing, ConfigTlvParseStatus::TrailingBytes, "empty patch trailing bytes are structured");

    auto truncatedEntry = Header(1, 1);
    AppendU16(truncatedEntry, static_cast<uint16_t>(ConfigKeyId::SvcAutoMode));
    truncatedEntry.push_back(static_cast<uint8_t>(ConfigValueType::Bool));
    AppendU16(truncatedEntry, 8);
    truncatedEntry.push_back('t');
    RequireStatus(truncatedEntry, ConfigTlvParseStatus::TruncatedEntry, "truncated entry is structured");

    auto trailing = payload;
    trailing.push_back(0xEE);
    RequireStatus(trailing, ConfigTlvParseStatus::TrailingBytes, "trailing bytes are structured");

    std::vector<uint8_t> oversizedCatalogCount;
    AppendV3Header(oversizedCatalogCount, 0x33435643u, std::numeric_limits<uint32_t>::max());
    RequireThrows([&] { (void)deserializeConfigV3Catalog(oversizedCatalogCount.data(), oversizedCatalogCount.size()); },
                  "v3 catalog rejects impossible count before reserve");

    std::vector<uint8_t> oversizedSnapshotCount;
    AppendV3Header(oversizedSnapshotCount, 0x33535643u, std::numeric_limits<uint32_t>::max());
    RequireThrows([&] { (void)deserializeConfigV3Snapshot(oversizedSnapshotCount.data(), oversizedSnapshotCount.size()); },
                  "v3 snapshot rejects impossible count before reserve");

    ConfigV3CatalogPayload largeEnumPayload{};
    ConfigDescriptor largeEnumDescriptor{};
    largeEnumDescriptor.path = "service.auto_mode";
    largeEnumDescriptor.keyId = ConfigKeyId::SvcAutoMode;
    largeEnumDescriptor.defaultValue = true;
    largeEnumDescriptor.enumMapping.resize(static_cast<size_t>(std::numeric_limits<uint16_t>::max()) + 1u);
    largeEnumPayload.entries.push_back(std::move(largeEnumDescriptor));
    RequireThrows([&] { (void)serializeConfigV3Catalog(largeEnumPayload); }, "v3 catalog rejects oversized enum mapping");

    std::vector<uint8_t> invalidUiType;
    AppendV3Header(invalidUiType, 0x33435643u, 1);
    AppendMinimalCatalogEntry(invalidUiType, 0xFFu, static_cast<uint8_t>(ConfigRuntimeBinding::SchemaOnly));
    RequireThrows([&] { (void)deserializeConfigV3Catalog(invalidUiType.data(), invalidUiType.size()); }, "v3 catalog rejects invalid ui type");

    std::vector<uint8_t> invalidRuntimeBinding;
    AppendV3Header(invalidRuntimeBinding, 0x33435643u, 1);
    AppendMinimalCatalogEntry(invalidRuntimeBinding, static_cast<uint8_t>(ConfigUiType::Bool), 0xFFu);
    RequireThrows([&] { (void)deserializeConfigV3Catalog(invalidRuntimeBinding.data(), invalidRuntimeBinding.size()); },
                  "v3 catalog rejects invalid runtime binding");

    std::cout << "[PASS] ConfigTlvTest\n";
    return 0;
}
