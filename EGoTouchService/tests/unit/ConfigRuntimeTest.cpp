#include "ConfigRuntime.h"

#include "config/ConfigKeyMap.h"
#include "config/ConfigTlv.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <memory>
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

bool HasAction(const Service::ConfigRuntime::V3ApplyResult& result, Service::ConfigApplyActionKind kind) {
    return std::any_of(result.applyActions.begin(), result.applyActions.end(),
        [kind](const Service::ConfigApplyAction& action) { return action.kind == kind; });
}

bool HasFailedTargetResult(const Service::ConfigRuntime::V3ApplyResult& result, std::string_view targetName) {
    return std::any_of(result.targetResults.begin(), result.targetResults.end(),
        [targetName](const Service::ConfigTargetResult& targetResult) {
            return targetResult.targetName == targetName && !targetResult.ok;
        });
}

std::vector<uint8_t> MakePatchPayload(std::initializer_list<Config::ConfigTlvEntry> entries) {
    Config::ConfigPatchTlv patch{};
    patch.entries.insert(patch.entries.end(), entries.begin(), entries.end());
    return Config::serializePatch(patch);
}

struct TempConfigDir {
    std::filesystem::path path;

    TempConfigDir() {
        const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
        path = std::filesystem::temp_directory_path() / ("egotouch-config-runtime-" + std::to_string(unique));
        std::filesystem::create_directories(path);
    }

    ~TempConfigDir() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

void CopyDefaultYamlTo(const std::filesystem::path& dir) {
    const auto source = std::filesystem::current_path() / "config" / "default.yaml";
    assert(std::filesystem::exists(source));
    std::filesystem::copy_file(source, dir / "default.yaml", std::filesystem::copy_options::overwrite_existing);
}

Config::ConfigValue ReadOverrideValue(const std::filesystem::path& dir, std::string_view path) {
    Config::ConfigStore overrides;
    overrides.loadFromYaml((dir / "overrides.yaml").string());
    assert(overrides.has(path));
    return overrides.get<Config::ConfigValue>(path);
}

Service::ConfigRuntime::V3ApplyResult ApplyV3Patch(Service::ConfigRuntime& runtime,
                                                   const std::vector<uint8_t>& payload) {
    const auto baseline = runtime.BuildSnapshotV3Blob();
    return runtime.ApplyConfigPatchV3(
        baseline.schemaVersion,
        baseline.snapshotVersion,
        payload.data(),
        payload.size());
}

class RejectAutoModeTarget final : public Service::IConfigTarget {
public:
    std::string_view name() const noexcept override { return "RejectAutoModeTarget"; }

    bool isInterested(const Service::ConfigChangeSet& changeSet) const override {
        return changeSet.containsPath("service.auto_mode");
    }

    Service::ConfigTargetResult validateConfig(const Config::ConfigStore&,
                                               const Service::ConfigChangeSet&) const override {
        Service::ConfigTargetResult result{};
        result.targetName = std::string(name());
        result.phase = Service::ConfigApplyPhase::Live;
        result.ok = false;
        result.message = "test target rejection";
        return result;
    }

    Service::ConfigTargetResult applyConfig(const Config::ConfigStore&,
                                            const Service::ConfigChangeSet&,
                                            Service::ConfigApplyPhase phase) const override {
        Service::ConfigTargetResult result{};
        result.targetName = std::string(name());
        result.phase = phase;
        result.ok = true;
        return result;
    }
};

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
    const auto payload = MakePatchPayload({Config::ConfigTlvEntry{*autoModeKeyId, Config::ConfigValueType::Bool, "false"}});

    const auto applied = ApplyV3Patch(runtime, payload);
    assert(applied.ipcStatus == Ipc::IpcStatusCode::Ok);
    assert(applied.status == Ipc::ConfigV3MutationStatus::Ok);
    assert(applied.changedCount == 1);
    assert(applied.appliedCount == 1);
    assert(!applied.desiredServiceConfig.autoMode);
    assert(HasAction(applied, Service::ConfigApplyActionKind::ServicePolicy));
    assert(!HasAction(applied, Service::ConfigApplyActionKind::PipelineRuntime));

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
    auto invalidPayload = payload;
    invalidPayload[0] ^= 0xFF;
    const auto rejected = ApplyV3Patch(runtime, invalidPayload);
    assert(rejected.ipcStatus == Ipc::IpcStatusCode::InvalidRequest);
    assert(rejected.status == Ipc::ConfigV3MutationStatus::Rejected);
    assert(!runtime.ServiceState().autoMode);
    const auto afterRejectSnapshotBlob = runtime.BuildSnapshotV3Blob();
    assert(afterRejectSnapshotBlob.bytes == beforeRejectSnapshotBlob.bytes);
    assert(afterRejectSnapshotBlob.snapshotVersion == beforeRejectSnapshotBlob.snapshotVersion);
    assert(afterRejectSnapshotBlob.checksum == beforeRejectSnapshotBlob.checksum);

    const auto touchStepKeyId = Config::tryKeyIdForPath("touch.signal_cond.baseline_no_finger_max_step");
    assert(touchStepKeyId.has_value());
    const auto touchPayload = MakePatchPayload({Config::ConfigTlvEntry{*touchStepKeyId, Config::ConfigValueType::Int32, "513"}});
    const auto touchApplied = ApplyV3Patch(runtime, touchPayload);
    assert(touchApplied.ipcStatus == Ipc::IpcStatusCode::Ok);
    assert(touchApplied.status == Ipc::ConfigV3MutationStatus::Ok);
    assert(touchApplied.changedCount == 1);
    assert(touchApplied.appliedCount == 1);
    assert(HasAction(touchApplied, Service::ConfigApplyActionKind::PipelineRuntime));

    const auto iirMaxKeyId = Config::tryKeyIdForPath("stylus.sp.iir_max_coef");
    const auto iirLowHoverKeyId = Config::tryKeyIdForPath("stylus.sp.iir_coef_low_hover");
    assert(iirMaxKeyId.has_value());
    assert(iirLowHoverKeyId.has_value());
    const auto beforeIirReject = runtime.BuildSnapshotV3Blob();
    const auto invalidIirPayload = MakePatchPayload({
        Config::ConfigTlvEntry{*iirMaxKeyId, Config::ConfigValueType::Int32, "1"},
        Config::ConfigTlvEntry{*iirLowHoverKeyId, Config::ConfigValueType::Int32, "2"},
    });
    const auto iirRejected = ApplyV3Patch(runtime, invalidIirPayload);
    assert(iirRejected.ipcStatus == Ipc::IpcStatusCode::Ok);
    assert(iirRejected.status == Ipc::ConfigV3MutationStatus::Rejected);
    assert(HasFailedTargetResult(iirRejected, "PipelineConfigTarget"));
    const auto afterIirReject = runtime.BuildSnapshotV3Blob();
    assert(afterIirReject.bytes == beforeIirReject.bytes);
    assert(afterIirReject.snapshotVersion == beforeIirReject.snapshotVersion);
    assert(afterIirReject.checksum == beforeIirReject.checksum);

    Service::ConfigRuntime v3Runtime;
    assert(v3Runtime.Initialize("", [](const Config::ConfigStore&) { return true; }));
    const auto v3AutoModeKeyId = Config::tryKeyIdForPath("service.auto_mode");
    assert(v3AutoModeKeyId.has_value());
    const auto v3LivePayload = MakePatchPayload({Config::ConfigTlvEntry{*v3AutoModeKeyId, Config::ConfigValueType::Bool, "false"}});
    const auto v3LiveApplied = ApplyV3Patch(v3Runtime, v3LivePayload);
    assert(v3LiveApplied.ipcStatus == Ipc::IpcStatusCode::Ok);
    assert(v3LiveApplied.status == Ipc::ConfigV3MutationStatus::Ok);
    assert(v3LiveApplied.changedCount == 1);
    assert(v3LiveApplied.appliedCount == 1);
    assert(v3LiveApplied.restartRequiredCount == 0);
    assert(!v3Runtime.ServiceState().autoMode);
    assert(HasAction(v3LiveApplied, Service::ConfigApplyActionKind::ServicePolicy));

    Service::ConfigRuntime restartRuntime;
    assert(restartRuntime.Initialize("", [](const Config::ConfigStore&) { return true; }));
    const auto serviceModeKeyId2 = Config::tryKeyIdForPath("service.mode");
    assert(serviceModeKeyId2.has_value());
    const auto restartPayload = MakePatchPayload({Config::ConfigTlvEntry{*serviceModeKeyId2, Config::ConfigValueType::String, "touch_only"}});
    const auto restartApplied = ApplyV3Patch(restartRuntime, restartPayload);
    assert(restartApplied.ipcStatus == Ipc::IpcStatusCode::Ok);
    assert(restartApplied.status == Ipc::ConfigV3MutationStatus::Ok);
    assert(restartApplied.changedCount == 1);
    assert(restartApplied.appliedCount == 0);
    assert(restartApplied.restartRequiredCount == 1);
    assert(restartApplied.desiredServiceConfig.mode == Service::ServiceMode::Full);
    assert(!HasAction(restartApplied, Service::ConfigApplyActionKind::ServicePolicy));
    assert(restartRuntime.SnapshotStore().getOr<std::string>("service.mode", "") == "touch_only");

    TempConfigDir tempConfig;
    CopyDefaultYamlTo(tempConfig.path);
    Service::ConfigRuntime persistRuntime;
    assert(persistRuntime.Initialize(tempConfig.path.string(), [](const Config::ConfigStore&) { return true; }));
    const auto persistedRestart = ApplyV3Patch(persistRuntime, restartPayload);
    assert(persistedRestart.ipcStatus == Ipc::IpcStatusCode::Ok);
    assert(persistedRestart.restartRequiredCount == 1);
    const auto persistResult = persistRuntime.PersistConfigV3();
    assert(persistResult.ipcStatus == Ipc::IpcStatusCode::Ok);
    assert(persistResult.status == Ipc::ConfigV3MutationStatus::Ok);
    assert(persistResult.persistedCount >= 1);
    assert(Config::getValue<std::string>(ReadOverrideValue(tempConfig.path, "service.mode")) == "touch_only");
    assert(std::filesystem::file_size(tempConfig.path / "overrides.yaml") != 0);

    Service::ConfigRuntime v3RejectRuntime;
    assert(v3RejectRuntime.Initialize("", [](const Config::ConfigStore&) { return true; }));
    const auto beforeV3Reject = v3RejectRuntime.BuildSnapshotV3Blob();
    const auto startupOnlyRejected = ApplyV3Patch(
        v3RejectRuntime,
        MakePatchPayload({Config::ConfigTlvEntry{*serviceModeKeyId2, Config::ConfigValueType::String, "invalid_mode"}}));
    assert(startupOnlyRejected.ipcStatus == Ipc::IpcStatusCode::Ok);
    assert(startupOnlyRejected.status == Ipc::ConfigV3MutationStatus::Rejected);
    assert(startupOnlyRejected.rejectedCount == 1);
    const auto afterV3Reject = v3RejectRuntime.BuildSnapshotV3Blob();
    assert(afterV3Reject.bytes == beforeV3Reject.bytes);
    assert(afterV3Reject.snapshotVersion == beforeV3Reject.snapshotVersion);
    assert(afterV3Reject.checksum == beforeV3Reject.checksum);

    Service::ConfigRuntime rejectingRuntime;
    rejectingRuntime.RegisterConfigTarget(std::make_unique<RejectAutoModeTarget>());
    assert(rejectingRuntime.Initialize("", [](const Config::ConfigStore&) { return true; }));
    const auto rejectingBefore = rejectingRuntime.BuildSnapshotV3Blob();
    const auto targetRejected = ApplyV3Patch(rejectingRuntime, payload);
    assert(targetRejected.ipcStatus == Ipc::IpcStatusCode::Ok);
    assert(targetRejected.status == Ipc::ConfigV3MutationStatus::Rejected);
    assert(HasFailedTargetResult(targetRejected, "RejectAutoModeTarget"));
    assert(rejectingRuntime.ServiceState().autoMode);
    const auto rejectingAfter = rejectingRuntime.BuildSnapshotV3Blob();
    assert(rejectingAfter.bytes == rejectingBefore.bytes);
    assert(rejectingAfter.snapshotVersion == rejectingBefore.snapshotVersion);
    assert(rejectingAfter.checksum == rejectingBefore.checksum);

    return 0;
}
