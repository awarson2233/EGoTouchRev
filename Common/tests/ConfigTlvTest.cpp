#include "config/ConfigTlv.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
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

    auto unknownType = Header(1, 1);
    AppendEntry(unknownType, static_cast<uint16_t>(ConfigKeyId::SvcAutoMode), 0x7Fu, "true");
    RequireStatus(unknownType, ConfigTlvParseStatus::UnknownValueType, "unknown value type is structured");

    auto duplicate = Header(1, 2);
    AppendEntry(duplicate, static_cast<uint16_t>(ConfigKeyId::SvcAutoMode), static_cast<uint8_t>(ConfigValueType::Bool), "true");
    AppendEntry(duplicate, static_cast<uint16_t>(ConfigKeyId::SvcAutoMode), static_cast<uint8_t>(ConfigValueType::Bool), "false");
    RequireStatus(duplicate, ConfigTlvParseStatus::DuplicateKey, "duplicate key is structured");

    auto empty = Header(1, 0);
    RequireStatus(empty, ConfigTlvParseStatus::EmptyPatch, "empty patch is structured");

    auto truncatedEntry = Header(1, 1);
    AppendU16(truncatedEntry, static_cast<uint16_t>(ConfigKeyId::SvcAutoMode));
    truncatedEntry.push_back(static_cast<uint8_t>(ConfigValueType::Bool));
    AppendU16(truncatedEntry, 8);
    truncatedEntry.push_back('t');
    RequireStatus(truncatedEntry, ConfigTlvParseStatus::TruncatedEntry, "truncated entry is structured");

    auto trailing = payload;
    trailing.push_back(0xEE);
    RequireStatus(trailing, ConfigTlvParseStatus::TrailingBytes, "trailing bytes are structured");

    std::cout << "[PASS] ConfigTlvTest\n";
    return 0;
}
