#include "DvrCoreTestSupport.h"

#include <iostream>

#ifndef EGO_TEST_FIXTURE_DIR
#define EGO_TEST_FIXTURE_DIR ""
#endif

namespace {

std::filesystem::path FixturePath() {
    return std::filesystem::path(EGO_TEST_FIXTURE_DIR) / "dvrbin" / "dataset.dvrbin";
}

void TestDatasetFixtureCompatibility() {
    const auto path = FixturePath();
    DvrCoreTest::Require(std::filesystem::exists(path), "dataset.dvrbin fixture should exist");

    std::vector<Solvers::HeatmapFrame> frames;
    Dvr::DynamicDebugSchema dynamicSchema;
    std::vector<Dvr::DynamicDebugFrame> dynamicFrames;
    Dvr::RuntimeConfigSnapshot runtimeConfig;
    int version = 0;
    uint32_t flags = 0;
    std::string error;
    DvrCoreTest::Require(Dvr::ReadBinaryFile(path, frames, version, &flags, &error, &dynamicSchema, &dynamicFrames, &runtimeConfig),
                         error.empty() ? "DVR2 fixture should be readable" : error.c_str());

    DvrCoreTest::Require(version == Dvr::Format::kCurrentDvrFormatVersion, "fixture should be DVR2 v6");
    DvrCoreTest::Require(flags == 7u, "fixture flags should match v6 4-section golden");
    DvrCoreTest::Require((flags & Dvr::Format::kDvrFlagHasRuntimeConfig) == 0, "fixture should not mark runtime config");
    DvrCoreTest::Require((flags & Dvr::Format::kDvrFlagHasDynamicDebug) == 0, "fixture should not mark dynamic debug");
    DvrCoreTest::Require(frames.size() == 480, "fixture frame count should match golden");
    DvrCoreTest::Require(frames.front().timestamp == 100000, "fixture first timestamp should match golden");
    DvrCoreTest::Require(frames.back().timestamp == 2016000, "fixture last timestamp should match golden");
    DvrCoreTest::Require(frames.front().receiveSystemEpochUs == 1710000000000000ull, "fixture first epoch should match golden");
    DvrCoreTest::Require(frames.back().receiveSystemEpochUs == 1710000001916000ull, "fixture last epoch should match golden");
    DvrCoreTest::Require(dynamicSchema.Empty(), "fixture dynamic schema should be empty");
    DvrCoreTest::Require(dynamicFrames.empty(), "fixture dynamic frames should be empty");
    DvrCoreTest::Require(runtimeConfig.Empty(), "fixture runtime config should be empty");

    const auto bytes = DvrCoreTest::ReadBytes(path);
    DvrCoreTest::Require(bytes.size() >= sizeof(Dvr::Format::Dvr2FileHeader), "fixture should contain header");
    const auto* header = reinterpret_cast<const Dvr::Format::Dvr2FileHeader*>(bytes.data());
    DvrCoreTest::Require(header->sectionCount == 4, "fixture should remain a 4-section file");
    DvrCoreTest::Require(header->flags == flags, "fixture header flags should match reader flags");
    const auto* toc = reinterpret_cast<const Dvr::Format::Dvr2SectionEntry*>(bytes.data() + header->tocOffset);
    const auto* meta = reinterpret_cast<const Dvr::Format::Dvr2MetaSection*>(bytes.data() + toc[0].offset);
    DvrCoreTest::Require(meta->frameSchemaHash == 2892734638u, "fixture frame schema hash should match golden");
    DvrCoreTest::Require(meta->frameRecordSize == 11512u, "fixture legacy frame record size should match golden");
}

} // namespace

int main() {
    try {
        TestDatasetFixtureCompatibility();
        std::cout << "[TEST] DVRCore fixture compatibility tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
