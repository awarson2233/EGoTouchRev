#include "DvrCoreTestSupport.h"

#include <cstddef>
#include <iostream>
#include <type_traits>

namespace {

void TestFormatConstants() {
    namespace F = Dvr::Format;
    DvrCoreTest::Require(F::kCurrentDvrFormatVersion == 6, "DVR2 current version should remain 6");
    DvrCoreTest::Require(F::kDvr2Magic[0] == 'E' && F::kDvr2Magic[6] == '2' && F::kDvr2Magic[7] == '\0', "DVR2 magic mismatch");
    DvrCoreTest::Require(F::kLegacyDvrMagic[6] == 'B' && F::kLegacyDvrMagic[7] == '1', "legacy magic mismatch");
    DvrCoreTest::Require(F::kDvrFlagHasStylusDiagnostics == 0x01, "stylus diagnostics flag bit changed");
    DvrCoreTest::Require(F::kDvrFlagHasStructuredSuffix == 0x02, "structured suffix flag bit changed");
    DvrCoreTest::Require(F::kDvrFlagHasReceiveSystemEpochUs == 0x04, "receive epoch flag bit changed");
    DvrCoreTest::Require(F::kDvrFlagHasDynamicDebug == 0x08, "dynamic debug flag bit changed");
    DvrCoreTest::Require(F::kDvrFlagHasRuntimeConfig == 0x10, "runtime config flag bit changed");
    DvrCoreTest::Require(static_cast<uint32_t>(F::Dvr2SectionType::Meta) == 1, "Meta section id changed");
    DvrCoreTest::Require(static_cast<uint32_t>(F::Dvr2SectionType::Index) == 2, "Index section id changed");
    DvrCoreTest::Require(static_cast<uint32_t>(F::Dvr2SectionType::Frames) == 3, "Frames section id changed");
    DvrCoreTest::Require(static_cast<uint32_t>(F::Dvr2SectionType::FrameSchema) == 6, "FrameSchema section id changed");
    DvrCoreTest::Require(static_cast<uint32_t>(F::Dvr2SectionType::RuntimeConfigSchema) == 7, "RuntimeConfigSchema section id changed");
    DvrCoreTest::Require(static_cast<uint32_t>(F::Dvr2SectionType::RuntimeConfigValues) == 8, "RuntimeConfigValues section id changed");
}

void TestWireLayout() {
    namespace F = Dvr::Format;
    DvrCoreTest::Require(sizeof(F::Dvr2FileHeader) == 32, "Dvr2FileHeader size changed");
    DvrCoreTest::Require(sizeof(F::Dvr2SectionEntry) == 24, "Dvr2SectionEntry size changed");
    DvrCoreTest::Require(sizeof(F::Dvr2MetaSection) == 64, "Dvr2MetaSection size changed");
    DvrCoreTest::Require(sizeof(F::Dvr2FrameSchemaHeader) == 32, "Dvr2FrameSchemaHeader size changed");
    DvrCoreTest::Require(sizeof(F::Dvr2FieldDef) == 164, "Dvr2FieldDef size changed");
    DvrCoreTest::Require(sizeof(F::Dvr2IndexEntry) == 32, "Dvr2IndexEntry size changed");
    DvrCoreTest::Require(sizeof(F::Dvr2ContactRecord) == 80, "Dvr2ContactRecord size changed");
    DvrCoreTest::Require(sizeof(F::Dvr2PeakRecord) == 12, "Dvr2PeakRecord size changed");
    DvrCoreTest::Require(sizeof(F::Dvr2TouchPacketRecord) == 36, "Dvr2TouchPacketRecord size changed");
    DvrCoreTest::Require(sizeof(F::Dvr2StylusDataRecord) == 152, "Dvr2StylusDataRecord size changed");
    DvrCoreTest::Require(sizeof(F::Dvr2RuntimeConfigSchemaHeader) == 16, "Dvr2RuntimeConfigSchemaHeader size changed");
    DvrCoreTest::Require(sizeof(F::Dvr2RuntimeConfigFieldDef) == 224, "Dvr2RuntimeConfigFieldDef size changed");
    DvrCoreTest::Require(sizeof(F::Dvr2RuntimeConfigValuesHeader) == 16, "Dvr2RuntimeConfigValuesHeader size changed");
    DvrCoreTest::Require(sizeof(F::Dvr2RuntimeConfigValueRecord) == 144, "Dvr2RuntimeConfigValueRecord size changed");
    DvrCoreTest::Require(sizeof(F::Dvr2FramePayload) == 17016, "Dvr2FramePayload size changed");
    DvrCoreTest::Require(offsetof(F::Dvr2FrameCore, heatmapMatrix) == 28, "heatmapMatrix offset changed");
    DvrCoreTest::Require(offsetof(F::Dvr2FrameCore, masterSuffix) == 4828, "masterSuffix offset changed");
    DvrCoreTest::Require(offsetof(F::Dvr2FrameCore, touchPackets) == 5416, "touchPackets offset changed");
    DvrCoreTest::Require(offsetof(F::Dvr2FrameCore, stylus) == 10288, "stylus offset changed");
    DvrCoreTest::Require(offsetof(F::Dvr2FrameCore, contacts) == 10440, "contacts offset changed");
    DvrCoreTest::Require(offsetof(F::Dvr2FrameCore, peaks) == 11244, "peaks offset changed");
    DvrCoreTest::Require(offsetof(F::Dvr2FramePayload, rawDataLength) == 11608, "rawDataLength offset changed");
    DvrCoreTest::Require(offsetof(F::Dvr2FramePayload, rawData) == 11610, "rawData offset changed");
    DvrCoreTest::Require(std::is_trivially_copyable_v<F::Dvr2FramePayload>, "Dvr2FramePayload should be trivially copyable");
    DvrCoreTest::Require(std::is_standard_layout_v<F::Dvr2FramePayload>, "Dvr2FramePayload should be standard layout");
}

} // namespace

int main() {
    try {
        TestFormatConstants();
        TestWireLayout();
        std::cout << "[TEST] DVRCore format/layout tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
