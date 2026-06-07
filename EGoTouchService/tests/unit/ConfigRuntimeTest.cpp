#include "ConfigRuntime.h"

#include "config/ConfigKeyMap.h"
#include "config/ConfigTlv.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

namespace {

bool CatalogHasPath(const Config::ConfigV3CatalogPayload& catalog, std::string_view path) {
    return std::any_of(catalog.entries.begin(), catalog.entries.end(),
        [path](const Config::ConfigDescriptor& entry) { return entry.path == path; });
}

const Config::ConfigV3SnapshotEntry* FindSnapshotEntry(const Config::ConfigV3SnapshotPayload& snapshot, Config::ConfigKeyId keyId) {
    const auto it = std::find_if(snapshot.entries.begin(), snapshot.entries.end(),
        [keyId](const Config::ConfigV3SnapshotEntry& entry) { return entry.keyId == keyId; });
    return it == snapshot.entries.end() ? nullptr : &*it;
}

Ipc::ConfigTlvChunkRequestWire MakeSingleChunk(uint16_t sessionId, const std::vector<uint8_t>& payload) {
    Ipc::ConfigTlvChunkRequestWire chunk{};
    chunk.wireVersion = Ipc::kIpcProtocolVersion;
    chunk.sessionId = sessionId;
    chunk.flags = Ipc::kConfigTlvChunkFirst | Ipc::kConfigTlvChunkLast;
    chunk.totalLen = static_cast<uint16_t>(payload.size());
    chunk.chunkLen = static_cast<uint16_t>(payload.size());
    std::memcpy(chunk.bytes, payload.data(), payload.size());
    return chunk;
}

} // namespace

int main() {
    Service::ConfigRuntime runtime;
    assert(runtime.Initialize("", [](const Config::ConfigStore&) { return true; }));

    const auto initial = runtime.ServiceState();
    assert(initial.autoMode);

    const auto catalogBlob = runtime.BuildCatalogV3Blob();
    assert(!catalogBlob.bytes.empty());
    assert(catalogBlob.schemaVersion != 0);
    assert(catalogBlob.checksum != 0);
    const auto catalog = Config::deserializeConfigV3Catalog(catalogBlob.bytes.data(), catalogBlob.bytes.size());
    assert(catalog.schemaVersion == catalogBlob.schemaVersion);
    assert(CatalogHasPath(catalog, "service.mode"));
    assert(CatalogHasPath(catalog, "touch.signal_cond.baseline_no_finger_max_step"));
    assert(CatalogHasPath(catalog, "stylus.sp.iir_max_coef"));

    const auto snapshotBlob = runtime.BuildSnapshotV3Blob();
    assert(!snapshotBlob.bytes.empty());
    assert(snapshotBlob.schemaVersion == catalogBlob.schemaVersion);
    assert(snapshotBlob.snapshotVersion != 0);
    assert(snapshotBlob.checksum != 0);
    const auto snapshot = Config::deserializeConfigV3Snapshot(snapshotBlob.bytes.data(), snapshotBlob.bytes.size());
    assert(snapshot.schemaVersion == snapshotBlob.schemaVersion);
    assert(snapshot.snapshotVersion == snapshotBlob.snapshotVersion);
    const auto serviceModeKeyId = Config::tryKeyIdForPath("service.mode");
    assert(serviceModeKeyId.has_value());
    const auto* serviceMode = FindSnapshotEntry(snapshot, *serviceModeKeyId);
    assert(serviceMode != nullptr);
    assert(Config::getValue<std::string>(serviceMode->value) == "full");

    const auto autoModeKeyId = Config::tryKeyIdForPath("service.auto_mode");
    assert(autoModeKeyId.has_value());
    Config::ConfigPatchTlv patch{};
    patch.entries.push_back(Config::ConfigTlvEntry{*autoModeKeyId, Config::ConfigValueType::Bool, "false"});
    const auto payload = Config::serializePatch(patch);
    auto chunk = MakeSingleChunk(7, payload);

    const auto applied = runtime.ApplyTlvChunk(chunk);
    assert(applied.status == Ipc::IpcStatusCode::Ok);
    assert(applied.completed);
    assert(applied.changedCount == 1);
    assert(!applied.desiredServiceConfig.autoMode);

    const auto changedCatalogBlob = runtime.BuildCatalogV3Blob();
    const auto changedSnapshotBlob = runtime.BuildSnapshotV3Blob();
    assert(changedCatalogBlob.schemaVersion == catalogBlob.schemaVersion);
    assert(changedSnapshotBlob.bytes != snapshotBlob.bytes);
    assert(changedSnapshotBlob.snapshotVersion != snapshotBlob.snapshotVersion);
    assert(changedSnapshotBlob.checksum != snapshotBlob.checksum);
    const auto changedSnapshot = Config::deserializeConfigV3Snapshot(changedSnapshotBlob.bytes.data(), changedSnapshotBlob.bytes.size());
    const auto* autoMode = FindSnapshotEntry(changedSnapshot, *autoModeKeyId);
    assert(autoMode != nullptr);
    assert(!Config::getValue<bool>(autoMode->value));

    const auto beforeRejectSnapshotBlob = runtime.BuildSnapshotV3Blob();
    auto invalidChunk = MakeSingleChunk(8, payload);
    invalidChunk.bytes[0] ^= 0xFF;
    const auto rejected = runtime.ApplyTlvChunk(invalidChunk);
    assert(rejected.status == Ipc::IpcStatusCode::InvalidRequest);
    assert(!runtime.ServiceState().autoMode);
    const auto afterRejectSnapshotBlob = runtime.BuildSnapshotV3Blob();
    assert(afterRejectSnapshotBlob.bytes == beforeRejectSnapshotBlob.bytes);
    assert(afterRejectSnapshotBlob.snapshotVersion == beforeRejectSnapshotBlob.snapshotVersion);
    assert(afterRejectSnapshotBlob.checksum == beforeRejectSnapshotBlob.checksum);

    return 0;
}
