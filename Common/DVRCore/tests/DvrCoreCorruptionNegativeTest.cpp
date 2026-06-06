#include "DvrCoreTestSupport.h"

#include <cstring>
#include <iostream>
#include <vector>

namespace {

template <typename T>
T ReadPod(const std::vector<uint8_t>& bytes, size_t offset) {
    DvrCoreTest::Require(offset + sizeof(T) <= bytes.size(), "corruption helper read out of range");
    T value{};
    std::memcpy(&value, bytes.data() + offset, sizeof(T));
    return value;
}

template <typename T>
void WritePod(std::vector<uint8_t>& bytes, size_t offset, const T& value) {
    DvrCoreTest::Require(offset + sizeof(T) <= bytes.size(), "corruption helper write out of range");
    std::memcpy(bytes.data() + offset, &value, sizeof(T));
}

std::filesystem::path WriteValidFile(const char* name, bool withRuntimeConfig = false) {
    const auto path = DvrCoreTest::TempPath(name);
    const std::vector<Dvr::DvrFrameSlot> frames{DvrCoreTest::MakeFrameSlot()};
    const auto runtimeConfig = DvrCoreTest::MakeRuntimeConfigSnapshot();
    uint32_t flags = 0;
    DvrCoreTest::Require(Dvr::WriteBinaryFile(path, frames, nullptr, nullptr, withRuntimeConfig ? &runtimeConfig : nullptr, &flags), "valid DVR2 seed write should succeed");
    return path;
}

size_t SectionEntryOffset(const std::vector<uint8_t>& bytes, Dvr::Format::Dvr2SectionType sectionType) {
    const auto header = ReadPod<Dvr::Format::Dvr2FileHeader>(bytes, 0);
    for (uint32_t i = 0; i < header.sectionCount; ++i) {
        const size_t offset = static_cast<size_t>(header.tocOffset) + static_cast<size_t>(i) * sizeof(Dvr::Format::Dvr2SectionEntry);
        const auto section = ReadPod<Dvr::Format::Dvr2SectionEntry>(bytes, offset);
        if (section.type == static_cast<uint32_t>(sectionType)) return offset;
    }
    DvrCoreTest::Require(false, "requested section not found");
    return 0;
}

Dvr::Format::Dvr2SectionEntry SectionEntry(const std::vector<uint8_t>& bytes, Dvr::Format::Dvr2SectionType sectionType) {
    return ReadPod<Dvr::Format::Dvr2SectionEntry>(bytes, SectionEntryOffset(bytes, sectionType));
}

std::vector<Dvr::Format::Dvr2FieldDef> ReadFrameFields(const std::vector<uint8_t>& bytes,
                                                       const Dvr::Format::Dvr2SectionEntry& schemaSection,
                                                       Dvr::Format::Dvr2FrameSchemaHeader& schemaHeader) {
    schemaHeader = ReadPod<Dvr::Format::Dvr2FrameSchemaHeader>(bytes, static_cast<size_t>(schemaSection.offset));
    std::vector<Dvr::Format::Dvr2FieldDef> fields(schemaHeader.fieldCount);
    const size_t fieldsOffset = static_cast<size_t>(schemaSection.offset) + sizeof(Dvr::Format::Dvr2FrameSchemaHeader);
    for (uint32_t i = 0; i < schemaHeader.fieldCount; ++i) {
        fields[i] = ReadPod<Dvr::Format::Dvr2FieldDef>(bytes, fieldsOffset + static_cast<size_t>(i) * sizeof(Dvr::Format::Dvr2FieldDef));
    }
    return fields;
}

Dvr::Format::Dvr2FieldDef& RequireField(std::vector<Dvr::Format::Dvr2FieldDef>& fields, const char* path) {
    for (auto& field : fields) {
        if (std::strncmp(field.path, path, sizeof(field.path)) == 0) return field;
    }
    DvrCoreTest::Require(false, "test field not found in schema");
    return fields.front();
}

void WriteFrameFieldsAndHashes(std::vector<uint8_t>& bytes,
                               const Dvr::Format::Dvr2SectionEntry& schemaSection,
                               Dvr::Format::Dvr2FrameSchemaHeader schemaHeader,
                               const std::vector<Dvr::Format::Dvr2FieldDef>& fields) {
    schemaHeader.schemaHash = Dvr::Format::ComputeFieldSchemaHash(fields);
    WritePod(bytes, static_cast<size_t>(schemaSection.offset), schemaHeader);
    const size_t fieldsOffset = static_cast<size_t>(schemaSection.offset) + sizeof(Dvr::Format::Dvr2FrameSchemaHeader);
    for (uint32_t i = 0; i < schemaHeader.fieldCount; ++i) {
        WritePod(bytes, fieldsOffset + static_cast<size_t>(i) * sizeof(Dvr::Format::Dvr2FieldDef), fields[i]);
    }

    const auto metaSection = SectionEntry(bytes, Dvr::Format::Dvr2SectionType::Meta);
    auto meta = ReadPod<Dvr::Format::Dvr2MetaSection>(bytes, static_cast<size_t>(metaSection.offset));
    meta.frameSchemaHash = schemaHeader.schemaHash;
    WritePod(bytes, static_cast<size_t>(metaSection.offset), meta);
}

void CorruptAndExpectFailure(const char* name, bool withRuntimeConfig, void (*mutate)(std::vector<uint8_t>&)) {
    const auto path = WriteValidFile(name, withRuntimeConfig);
    auto bytes = DvrCoreTest::ReadBytes(path);
    mutate(bytes);
    DvrCoreTest::WriteBytes(path, bytes);
    DvrCoreTest::ExpectReadFailure(path, "corrupted DVR2 file should fail to read");
    std::filesystem::remove(path);
}

void TestBadMagicFails() {
    CorruptAndExpectFailure("bad_magic", false, [](std::vector<uint8_t>& bytes) {
        bytes[0] = 'X';
    });
}

void TestLegacyMagicFails() {
    CorruptAndExpectFailure("legacy_magic", false, [](std::vector<uint8_t>& bytes) {
        std::memcpy(bytes.data(), Dvr::Format::kLegacyDvrMagic.data(), Dvr::Format::kLegacyDvrMagic.size());
    });
}

void TestMissingRequiredSectionFails() {
    CorruptAndExpectFailure("missing_index", false, [](std::vector<uint8_t>& bytes) {
        auto offset = SectionEntryOffset(bytes, Dvr::Format::Dvr2SectionType::Index);
        auto section = ReadPod<Dvr::Format::Dvr2SectionEntry>(bytes, offset);
        section.type = 0xFFFFFFFFu;
        WritePod(bytes, offset, section);
    });
}

void TestUnsupportedSectionVersionFails() {
    CorruptAndExpectFailure("section_version", false, [](std::vector<uint8_t>& bytes) {
        auto offset = SectionEntryOffset(bytes, Dvr::Format::Dvr2SectionType::Meta);
        auto section = ReadPod<Dvr::Format::Dvr2SectionEntry>(bytes, offset);
        section.version = 2;
        WritePod(bytes, offset, section);
    });
}

void TestFrameSchemaHashMismatchFails() {
    CorruptAndExpectFailure("schema_hash", false, [](std::vector<uint8_t>& bytes) {
        const auto schemaSection = SectionEntry(bytes, Dvr::Format::Dvr2SectionType::FrameSchema);
        auto schemaHeader = ReadPod<Dvr::Format::Dvr2FrameSchemaHeader>(bytes, static_cast<size_t>(schemaSection.offset));
        schemaHeader.schemaHash ^= 0x01020304u;
        WritePod(bytes, static_cast<size_t>(schemaSection.offset), schemaHeader);
    });
}

void TestIndexOutsideFramesFails() {
    CorruptAndExpectFailure("index_outside", false, [](std::vector<uint8_t>& bytes) {
        const auto indexSection = SectionEntry(bytes, Dvr::Format::Dvr2SectionType::Index);
        const auto framesSection = SectionEntry(bytes, Dvr::Format::Dvr2SectionType::Frames);
        auto index = ReadPod<Dvr::Format::Dvr2IndexEntry>(bytes, static_cast<size_t>(indexSection.offset));
        index.frameOffset = framesSection.offset + framesSection.size + 1;
        WritePod(bytes, static_cast<size_t>(indexSection.offset), index);
    });
}

void TestContiguousFieldBoundsFails() {
    CorruptAndExpectFailure("contiguous_bounds", false, [](std::vector<uint8_t>& bytes) {
        const auto schemaSection = SectionEntry(bytes, Dvr::Format::Dvr2SectionType::FrameSchema);
        Dvr::Format::Dvr2FrameSchemaHeader schemaHeader{};
        auto fields = ReadFrameFields(bytes, schemaSection, schemaHeader);
        auto& field = RequireField(fields, "heatmapMatrix");
        field.offset = schemaHeader.frameRecordSize - static_cast<uint32_t>(field.elementSize);
        field.elementCount = 1;
        field.stride = field.elementSize;
        WriteFrameFieldsAndHashes(bytes, schemaSection, schemaHeader, fields);
    });
}

void TestRawDataBoundsFails() {
    CorruptAndExpectFailure("raw_data_bounds", false, [](std::vector<uint8_t>& bytes) {
        const auto schemaSection = SectionEntry(bytes, Dvr::Format::Dvr2SectionType::FrameSchema);
        Dvr::Format::Dvr2FrameSchemaHeader schemaHeader{};
        auto fields = ReadFrameFields(bytes, schemaSection, schemaHeader);
        auto& field = RequireField(fields, "rawData");
        field.offset = schemaHeader.frameRecordSize - 2;
        field.size = 4;
        field.elementCount = 1;
        field.stride = field.elementSize;
        WriteFrameFieldsAndHashes(bytes, schemaSection, schemaHeader, fields);
    });
}

void TestRuntimeConfigHashMismatchFails() {
    CorruptAndExpectFailure("runtime_hash", true, [](std::vector<uint8_t>& bytes) {
        const auto valuesSection = SectionEntry(bytes, Dvr::Format::Dvr2SectionType::RuntimeConfigValues);
        auto valuesHeader = ReadPod<Dvr::Format::Dvr2RuntimeConfigValuesHeader>(bytes, static_cast<size_t>(valuesSection.offset));
        valuesHeader.schemaHash ^= 0xA5A5A5A5u;
        WritePod(bytes, static_cast<size_t>(valuesSection.offset), valuesHeader);
    });
}

void TestRuntimeConfigValueTypeMismatchFails() {
    CorruptAndExpectFailure("runtime_type", true, [](std::vector<uint8_t>& bytes) {
        const auto valuesSection = SectionEntry(bytes, Dvr::Format::Dvr2SectionType::RuntimeConfigValues);
        const size_t firstValueOffset = static_cast<size_t>(valuesSection.offset) + sizeof(Dvr::Format::Dvr2RuntimeConfigValuesHeader);
        auto value = ReadPod<Dvr::Format::Dvr2RuntimeConfigValueRecord>(bytes, firstValueOffset);
        value.valueType = static_cast<uint8_t>(Dvr::Format::Dvr2ConfigValueType::String);
        WritePod(bytes, firstValueOffset, value);
    });
}

} // namespace

int main() {
    try {
        TestBadMagicFails();
        TestLegacyMagicFails();
        TestMissingRequiredSectionFails();
        TestUnsupportedSectionVersionFails();
        TestFrameSchemaHashMismatchFails();
        TestIndexOutsideFramesFails();
        TestContiguousFieldBoundsFails();
        TestRawDataBoundsFails();
        TestRuntimeConfigHashMismatchFails();
        TestRuntimeConfigValueTypeMismatchFails();
        std::cout << "[TEST] DVRCore corruption negative tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
