#include "ServiceProxyInternal.h"
#include "config/ConfigKeyMap.h"
#include "config/ConfigPath.h"
#include "config/ConfigStore.h"
#include "config/ConfigTlv.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

const Config::ConfigSchemaEntry* FindEntry(const Config::ConfigSchemaSnapshot& schema,
                                           std::string_view path) {
    const auto it = std::ranges::find_if(schema.entries, [path](const Config::ConfigSchemaEntry& entry) {
        return entry.yamlPath == path;
    });
    return it == schema.entries.end() ? nullptr : &*it;
}

void RequireLiveMappedEntry(const Config::ConfigSchemaSnapshot& schema,
                            std::string_view path) {
    const auto* entry = FindEntry(schema, path);
    Require(entry != nullptr, "schema entry should exist");
    const auto catalogKeyId = Config::tryKeyIdForPath(path);
    Require(catalogKeyId.has_value(), "catalog keyId should exist for schema path");
    Require(entry->keyId == *catalogKeyId, "schema keyId should match shared key map");
    Require(entry->boundToRuntime, "schema entry should be bound to runtime");
    Require(entry->runtimeBinding == Config::ConfigRuntimeBinding::ManualLiveApply ||
            entry->runtimeBinding == Config::ConfigRuntimeBinding::LiveSetter,
            "schema entry should have runtime binding");
    Require(Config::isLiveApplyTiming(entry->applyTiming), "schema entry should be live patchable");
}

void RequireMappedRestartEntry(const Config::ConfigSchemaSnapshot& schema,
                               std::string_view path) {
    const auto* entry = FindEntry(schema, path);
    Require(entry != nullptr, "restart schema entry should exist");
    const auto catalogKeyId = Config::tryKeyIdForPath(path);
    Require(catalogKeyId.has_value(), "restart schema keyId should exist");
    Require(entry->keyId == *catalogKeyId, "restart schema keyId should match shared key map");
    Require(entry->scope == Config::ConfigScope::ServicePolicy, "restart schema entry should be service policy");
    Require(entry->applyTiming == Config::ConfigApplyTiming::RestartRequired,
            "restart schema entry should require restart");
    Require(!Config::isLiveApplyTiming(entry->applyTiming),
            "restart schema entry should not be live patchable");
}

void TestSchemaCoversServiceTouchAndStylusCatalogKeys() {
    const auto schema = App::BuildServiceProxyConfigSchemaSnapshotForTest();

    RequireMappedRestartEntry(schema, "service.mode");
    RequireLiveMappedEntry(schema, "service.auto_mode");
    RequireLiveMappedEntry(schema, "service.stylus_vhf_enabled");
    RequireLiveMappedEntry(schema, "service.pen_button_mode");
    RequireLiveMappedEntry(schema, "service.pen_button_route");
    RequireLiveMappedEntry(schema, "touch.signal_cond.baseline_no_finger_max_step");
    RequireLiveMappedEntry(schema, "stylus.sp.bt_freq_shift_debounce_frames");
    RequireLiveMappedEntry(schema, "stylus.sp.lock_flash_edge_y");
}

void TestDirtyPathKeyIdsRemainStable() {
    const std::pair<std::string_view, Config::ConfigKeyId> expected[] = {
        {"service.mode", Config::ConfigKeyId::SvcMode},
        {"service.auto_mode", Config::ConfigKeyId::SvcAutoMode},
        {"service.stylus_vhf_enabled", Config::ConfigKeyId::SvcStylusVhfEnabled},
        {"service.pen_button_mode", Config::ConfigKeyId::SvcPenButtonMode},
        {"service.pen_button_route", Config::ConfigKeyId::SvcPenButtonRoute},
        {"touch.signal_cond.baseline_no_finger_max_step", Config::ConfigKeyId::TouchBaselineNoFingerMaxStep},
        {"stylus.sp.bt_freq_shift_debounce_frames", Config::ConfigKeyId::StylusSpBtFreqShiftDebounceFrames},
    };

    for (const auto& [path, keyId] : expected) {
        const auto resolved = Config::tryKeyIdForPath(path);
        Require(resolved.has_value(), "dirty path should resolve to shared keyId");
        Require(*resolved == keyId, "dirty path keyId should remain stable");
    }
}

void TestDefaultYamlContainsCatalogBackedKeys() {
    const auto paths = Config::resolve();
    Require(paths.has_value(), "config/default.yaml should be resolvable");

    Config::ConfigStore store;
    store.loadFromYaml(paths->defaultConfig);

    Require(store.has("service.mode"), "default.yaml should contain service.mode");
    Require(store.has("service.pen_button_route"), "default.yaml should contain service.pen_button_route");
    Require(store.has("touch.signal_cond.baseline_no_finger_max_step"), "default.yaml should contain touch baseline key");
    Require(store.has("stylus.sp.bt_freq_shift_debounce_frames"), "default.yaml should contain stylus SP key");
    Require(store.has("stylus.sp.lock_flash_edge_y"), "default.yaml should contain stylus lock key");
}

Config::ConfigV3CatalogPayload BuildCatalogPayload() {
    Config::ConfigV3CatalogPayload payload{};
    payload.schemaVersion = 10;
    payload.snapshotVersion = 20;
    const auto schema = App::BuildServiceProxyConfigSchemaSnapshotForTest();
    payload.entries.reserve(schema.entries.size());
    for (const auto& entry : schema.entries) {
        payload.entries.push_back(Config::ConfigDescriptor{
            .keyId = entry.keyId,
            .path = entry.yamlPath,
            .defaultValue = entry.defaultValue,
            .uiType = entry.uiType,
            .range = entry.range,
            .displayName = entry.displayName,
            .description = entry.description,
            .moduleTag = entry.moduleTag,
            .enumMapping = entry.enumMapping,
            .runtimeBinding = entry.runtimeBinding,
            .boundToRuntime = entry.boundToRuntime,
            .scope = entry.scope,
            .applyTiming = entry.applyTiming,
            .persistPolicy = entry.persistPolicy,
        });
    }
    return payload;
}

void ApplyCatalog(App::ServiceProxy& proxy) {
    const auto catalogBytes = Config::serializeConfigV3Catalog(BuildCatalogPayload());
    Require(proxy.ApplyConfigV3CatalogBytesForTest(catalogBytes.data(), catalogBytes.size()),
            "v3 catalog payload should apply");
}

void ApplyCatalogWithPathPersistPolicy(App::ServiceProxy& proxy,
                                       std::string_view path,
                                       Config::ConfigPersistPolicy persistPolicy) {
    auto payload = BuildCatalogPayload();
    const auto it = std::ranges::find_if(payload.entries, [path](const Config::ConfigDescriptor& entry) {
        return entry.path == path;
    });
    Require(it != payload.entries.end(), "catalog path should exist for persist policy override");
    it->persistPolicy = persistPolicy;
    const auto catalogBytes = Config::serializeConfigV3Catalog(payload);
    Require(proxy.ApplyConfigV3CatalogBytesForTest(catalogBytes.data(), catalogBytes.size()),
            "v3 catalog payload with persist policy override should apply");
}

Ipc::IpcResponse MakeApplyConfigPatchV3Response(Ipc::ConfigV3MutationStatus status,
                                                uint16_t changedCount,
                                                uint16_t appliedCount,
                                                uint16_t restartRequiredCount,
                                                uint16_t rejectedCount = 0,
                                                Config::ConfigKeyId failedKeyId = Config::ConfigKeyId::MaxKeyId) {
    Ipc::IpcResponse response{};
    Ipc::ConfigV3ApplyResultWire result{};
    result.status = static_cast<uint8_t>(status);
    result.changedCount = changedCount;
    result.appliedCount = appliedCount;
    result.restartRequiredCount = restartRequiredCount;
    result.rejectedCount = rejectedCount;
    result.failedKeyId = static_cast<uint16_t>(failedKeyId);
    std::memcpy(response.data, &result, sizeof(result));
    response.dataLen = static_cast<uint16_t>(sizeof(result));
    Ipc::MarkSuccess(response);
    return response;
}

Ipc::IpcResponse MakeInvalidApplyConfigPatchV3WireVersionResponse() {
    Ipc::IpcResponse response = MakeApplyConfigPatchV3Response(Ipc::ConfigV3MutationStatus::Ok, 1, 1, 0);
    Ipc::ConfigV3ApplyResultWire result{};
    std::memcpy(&result, response.data, sizeof(result));
    result.wireVersion = Ipc::kIpcProtocolVersion + 1;
    std::memcpy(response.data, &result, sizeof(result));
    return response;
}

Ipc::IpcResponse MakePersistConfigV3Response(uint16_t persistedCount, uint16_t skippedCount) {
    Ipc::IpcResponse response{};
    Ipc::PersistConfigV3ResponseWire result{};
    result.status = static_cast<uint8_t>(Ipc::ConfigV3MutationStatus::Ok);
    result.persistedCount = persistedCount;
    result.skippedCount = skippedCount;
    std::memcpy(response.data, &result, sizeof(result));
    response.dataLen = static_cast<uint16_t>(sizeof(result));
    Ipc::MarkSuccess(response);
    return response;
}

Ipc::IpcResponse MakePersistConfigV3FailureResponse() {
    Ipc::IpcResponse response{};
    Ipc::MarkFailure(response, Ipc::IpcStatusCode::InternalError);
    return response;
}

void ApplySnapshotWithBaseline(App::ServiceProxy& proxy, uint32_t schemaVersion, uint32_t snapshotVersion) {
    Config::ConfigV3SnapshotPayload snapshot{};
    snapshot.schemaVersion = schemaVersion;
    snapshot.snapshotVersion = snapshotVersion;
    snapshot.entries.push_back({Config::ConfigKeyId::SvcMode, std::string("full")});
    snapshot.entries.push_back({Config::ConfigKeyId::SvcAutoMode, true});
    const auto bytes = Config::serializeConfigV3Snapshot(snapshot);
    Require(proxy.ApplyConfigV3SnapshotBytesForTest(bytes.data(), bytes.size()), "v3 snapshot baseline should apply");
}

std::vector<uint8_t> MakeSnapshotBytes(uint32_t schemaVersion, uint32_t snapshotVersion, bool autoMode = true) {
    Config::ConfigV3SnapshotPayload snapshot{};
    snapshot.schemaVersion = schemaVersion;
    snapshot.snapshotVersion = snapshotVersion;
    snapshot.entries.push_back({Config::ConfigKeyId::SvcMode, std::string("full")});
    snapshot.entries.push_back({Config::ConfigKeyId::SvcAutoMode, autoMode});
    return Config::serializeConfigV3Snapshot(snapshot);
}

void TestV3CatalogPayloadAppliesSchemaAndDefaults() {
    App::ServiceProxy proxy;
    ApplyCatalog(proxy);
    const auto& schema = proxy.GetConfigSchemaSnapshot();

    RequireMappedRestartEntry(schema, "service.mode");
    RequireLiveMappedEntry(schema, "touch.signal_cond.baseline_no_finger_max_step");
    RequireLiveMappedEntry(schema, "stylus.sp.bt_freq_shift_debounce_frames");
    Require(proxy.GetConfigDraftStore().has("service.mode"), "local fallback draft should remain populated");
    const auto versions = proxy.GetConfigV3BaselineVersionsForTest();
    Require(versions.catalogSchemaVersion == 10, "v3 catalog schemaVersion should be recorded");
    Require(versions.catalogSnapshotVersion == 20, "v3 catalog snapshotVersion should be recorded");
}

void TestConnectedApplyUsesConfigPatchV3() {
    App::ServiceProxy proxy;
    ApplyCatalog(proxy);
    ApplySnapshotWithBaseline(proxy, 10, 31);
    proxy.SetConfigDraftValue("service.auto_mode", false);
    proxy.SetConfigV3IpcTestResponses(
        true,
        MakeApplyConfigPatchV3Response(Ipc::ConfigV3MutationStatus::Ok, 1, 1, 0),
        MakePersistConfigV3Response(1, 0));

    Require(proxy.ApplyConfigStoreGlobally(), "connected apply should succeed through v3");
    Require(proxy.GetConfigV3ApplyRequestCountForTest() == 1, "connected apply should send one v3 patch request");
    Require(proxy.GetConfigV3PersistRequestCountForTest() == 1, "connected apply should persist through v3");
    Require(proxy.HasLastConfigV3ApplyRequestForTest(), "v3 patch request should be captured");
    const auto request = proxy.GetLastConfigV3ApplyRequestForTest();
    Require(Ipc::IsValidApplyConfigPatchV3Request(request), "captured v3 patch request should be ABI-valid");
    Require(request.baseSchemaVersion == 10, "v3 patch should use snapshot schema baseline");
    Require(request.baseSnapshotVersion == 31, "v3 patch should use snapshot version baseline");
    const auto patch = Config::deserializePatch(request.bytes, request.payloadBytes);
    Require(patch.entries.size() == 1, "v3 patch should contain one dirty entry");
    Require(patch.entries.front().keyId == Config::ConfigKeyId::SvcAutoMode, "v3 patch should contain service.auto_mode");
    const auto result = proxy.GetLastApplyConfigResult();
    Require(result.status == App::ApplyConfigStatus::LiveApplied, "live v3 apply should report LiveApplied");
    Require(result.liveApplied, "live v3 apply should set liveApplied");
    Require(!result.restartRequired, "live v3 apply should not require restart");
    Require(result.persistAttempted && result.persisted, "live v3 apply should persist through v3");
    const auto state = proxy.GetConfigDraftPathState("service.auto_mode");
    Require(!state.dirty, "persisted live apply should clear dirty state");
    Require(state.applyState == App::ConfigDraftApplyState::LiveApplied, "persisted live apply should retain live-applied key state");
    Require(state.persistState == App::ConfigDraftPersistState::Persisted, "persisted live apply should mark key persisted");
}

void TestConnectedApplyOkPersistFailKeepsDirty() {
    App::ServiceProxy proxy;
    ApplyCatalog(proxy);
    ApplySnapshotWithBaseline(proxy, 10, 33);
    proxy.SetConfigDraftValue("service.auto_mode", false);
    proxy.SetConfigV3IpcTestResponses(
        true,
        MakeApplyConfigPatchV3Response(Ipc::ConfigV3MutationStatus::Ok, 1, 1, 0),
        MakePersistConfigV3FailureResponse());

    Require(proxy.ApplyConfigStoreGlobally(), "apply should still succeed when persist fails after live apply");
    Require(proxy.GetConfigV3ApplyRequestCountForTest() == 1, "persist-fail path should send one apply");
    Require(proxy.GetConfigV3PersistRequestCountForTest() == 1, "persist-fail path should attempt persist");
    const auto result = proxy.GetLastApplyConfigResult();
    Require(result.status == App::ApplyConfigStatus::LiveApplied, "apply ok/persist fail should still report live applied");
    Require(result.liveApplied, "apply ok/persist fail should report liveApplied");
    Require(result.persistAttempted && !result.persisted, "persist failure should be visible");
    Require(result.unpersistedLiveChanges, "persist failure should leave unpersisted live changes");
    const auto state = proxy.GetConfigDraftPathState("service.auto_mode");
    Require(state.dirty, "apply ok/persist fail should keep dirty key");
    Require(state.applyState == App::ConfigDraftApplyState::LiveApplied, "apply ok/persist fail should keep live-applied key state");
    Require(state.persistState == App::ConfigDraftPersistState::Unpersisted, "apply ok/persist fail should mark key unpersisted");
}

void TestAppliedUnpersistedSameValueCommitStillRetriesPersist() {
    App::ServiceProxy proxy;
    ApplyCatalog(proxy);
    ApplySnapshotWithBaseline(proxy, 10, 42);
    proxy.SetConfigDraftValue("service.auto_mode", false);
    proxy.SetConfigV3IpcTestResponses(
        true,
        MakeApplyConfigPatchV3Response(Ipc::ConfigV3MutationStatus::Ok, 1, 1, 0),
        MakePersistConfigV3FailureResponse());

    Require(proxy.ApplyConfigStoreGlobally(), "initial live apply should succeed even when persist fails");
    Require(proxy.GetConfigV3ApplyRequestCountForTest() == 1, "initial live apply should send one patch");
    Require(proxy.GetConfigV3PersistRequestCountForTest() == 1, "initial live apply should attempt persist");
    auto state = proxy.GetConfigDraftPathState("service.auto_mode");
    Require(state.dirty, "persist failure should leave applied key dirty");
    Require(state.applyState == App::ConfigDraftApplyState::LiveApplied,
            "persist failure should preserve live-applied state");
    Require(state.persistState == App::ConfigDraftPersistState::Unpersisted,
            "persist failure should mark key unpersisted");

    const auto appliedSnapshotBytes = MakeSnapshotBytes(10, 43, false);
    Require(proxy.ApplyConfigV3SnapshotBytesForTest(appliedSnapshotBytes.data(), appliedSnapshotBytes.size()),
            "snapshot refresh should record the applied value");
    proxy.SetConfigDraftValue("service.auto_mode", false);
    state = proxy.GetConfigDraftPathState("service.auto_mode");
    Require(state.dirty, "same-value recommit should keep applied key dirty for persist retry");
    Require(state.applyState == App::ConfigDraftApplyState::LiveApplied,
            "same-value recommit should preserve live-applied state");
    Require(state.persistState == App::ConfigDraftPersistState::Unpersisted,
            "same-value recommit should preserve unpersisted state");

    proxy.SetConfigV3IpcTestResponses(
        true,
        std::vector<Ipc::IpcResponse>{},
        MakePersistConfigV3Response(1, 0));

    Require(proxy.ApplyConfigStoreGlobally(), "same-value persist retry should succeed without a second patch apply");
    Require(proxy.GetConfigV3ApplyRequestCountForTest() == 0,
            "same-value persist retry should not send another patch apply");
    Require(proxy.GetConfigV3PersistRequestCountForTest() == 1,
            "same-value persist retry should still send PersistConfigV3");
    const auto result = proxy.GetLastApplyConfigResult();
    Require(result.status == App::ApplyConfigStatus::LiveApplied,
            "same-value persist retry should retain live-applied outcome");
    Require(result.liveApplied, "same-value persist retry should preserve liveApplied status");
    Require(result.persistAttempted && result.persisted,
            "same-value persist retry should report successful persist");
    Require(!result.unpersistedLiveChanges,
            "same-value persist retry success should clear unpersisted live change flag");
    state = proxy.GetConfigDraftPathState("service.auto_mode");
    Require(!state.dirty, "same-value persist retry success should clear dirty state");
    Require(state.applyState == App::ConfigDraftApplyState::LiveApplied,
            "same-value persist retry success should retain live-applied key state");
    Require(state.persistState == App::ConfigDraftPersistState::Persisted,
            "same-value persist retry success should mark key persisted");
}

void TestConnectedRestartRequiredApplyUsesConfigPatchV3() {
    App::ServiceProxy proxy;
    ApplyCatalog(proxy);
    ApplySnapshotWithBaseline(proxy, 10, 32);
    proxy.SetConfigDraftValue("service.mode", std::string("touch_only"));
    proxy.SetConfigV3IpcTestResponses(
        true,
        MakeApplyConfigPatchV3Response(Ipc::ConfigV3MutationStatus::Ok, 1, 0, 1),
        MakePersistConfigV3Response(1, 0));

    Require(proxy.ApplyConfigStoreGlobally(), "restart-required v3 apply should succeed");
    Require(proxy.GetConfigV3ApplyRequestCountForTest() == 1, "restart-required apply should send one v3 patch request");
    const auto request = proxy.GetLastConfigV3ApplyRequestForTest();
    const auto patch = Config::deserializePatch(request.bytes, request.payloadBytes);
    Require(patch.entries.size() == 1, "restart-required v3 patch should contain one dirty entry");
    Require(patch.entries.front().keyId == Config::ConfigKeyId::SvcMode, "restart-required v3 patch should include service.mode");
    const auto result = proxy.GetLastApplyConfigResult();
    Require(result.status == App::ApplyConfigStatus::RestartRequired, "restart-required v3 apply should report RestartRequired");
    Require(!result.liveApplied, "restart-required-only patch should not report liveApplied");
    Require(result.restartRequired, "restart-required v3 apply should set restartRequired");
    Require(result.persistAttempted && result.persisted, "restart-required v3 apply should persist staged value");
    const auto state = proxy.GetConfigDraftPathState("service.mode");
    Require(!state.dirty, "persisted restart-required key should clear dirty state");
    Require(state.applyState == App::ConfigDraftApplyState::StagedRestartRequired,
            "restart-required key should be staged, not live-applied");
    Require(state.persistState == App::ConfigDraftPersistState::Persisted,
            "restart-required key should be marked persisted after persist success");
}

void TestConnectedRejectedApplyResultIsPropagated() {
    App::ServiceProxy proxy;
    ApplyCatalog(proxy);
    ApplySnapshotWithBaseline(proxy, 10, 34);
    proxy.SetConfigDraftValue("service.auto_mode", false);
    proxy.SetConfigV3IpcTestResponses(
        true,
        MakeApplyConfigPatchV3Response(
            Ipc::ConfigV3MutationStatus::Rejected,
            0,
            0,
            0,
            1,
            Config::ConfigKeyId::SvcAutoMode),
        MakePersistConfigV3Response(0, 0));

    Require(!proxy.ApplyConfigStoreGlobally(), "rejected v3 apply result should fail the app apply");
    Require(proxy.GetConfigV3ApplyRequestCountForTest() == 1, "rejected v3 apply should send one patch request");
    Require(proxy.GetConfigV3PersistRequestCountForTest() == 0, "rejected v3 apply should not persist");
    const auto result = proxy.GetLastApplyConfigResult();
    Require(result.status == App::ApplyConfigStatus::LiveApplyFailed, "rejected v3 result should map to LiveApplyFailed");
    Require(!result.liveApplied && !result.restartRequired, "rejected v3 result should not report applied state");
    const auto state = proxy.GetConfigDraftPathState("service.auto_mode");
    Require(state.dirty, "rejected key should remain dirty");
    Require(state.applyState == App::ConfigDraftApplyState::Failed, "rejected key should keep failed state");
    Require(state.failedKeyId == Config::ConfigKeyId::SvcAutoMode, "rejected key state should identify failed key");
    Require(!state.errorMessage.empty(), "rejected key should keep an error message");
}

void TestVersionMismatchRetryRejectsInvalidWireVersion() {
    App::ServiceProxy proxy;
    ApplyCatalog(proxy);
    ApplySnapshotWithBaseline(proxy, 10, 35);
    proxy.SetConfigDraftValue("service.auto_mode", false);
    proxy.SetConfigV3IpcTestResponses(
        true,
        std::vector<Ipc::IpcResponse>{
            MakeApplyConfigPatchV3Response(Ipc::ConfigV3MutationStatus::VersionMismatch, 0, 0, 0),
            MakeInvalidApplyConfigPatchV3WireVersionResponse(),
        },
        MakePersistConfigV3Response(0, 0),
        MakeSnapshotBytes(10, 36));

    Require(!proxy.ApplyConfigStoreGlobally(), "retry response with invalid wireVersion should fail");
    Require(proxy.GetConfigV3ApplyRequestCountForTest() == 2, "version mismatch should retry once after snapshot refresh");
    Require(proxy.GetConfigV3PersistRequestCountForTest() == 0, "invalid retry response should not persist");
    const auto versions = proxy.GetConfigV3BaselineVersionsForTest();
    Require(versions.snapshotVersion == 36, "version mismatch path should refresh snapshot baseline before retry");
    const auto result = proxy.GetLastApplyConfigResult();
    Require(result.status == App::ApplyConfigStatus::LiveApplyFailed, "invalid retry wireVersion should map to LiveApplyFailed");
}

void TestVersionMismatchRebaseUsesRefreshedBaseline() {
    App::ServiceProxy proxy;
    ApplyCatalog(proxy);
    ApplySnapshotWithBaseline(proxy, 10, 37);
    proxy.SetConfigDraftValue("service.auto_mode", false);
    proxy.SetConfigV3IpcTestResponses(
        true,
        std::vector<Ipc::IpcResponse>{
            MakeApplyConfigPatchV3Response(Ipc::ConfigV3MutationStatus::VersionMismatch, 0, 0, 0),
            MakeApplyConfigPatchV3Response(Ipc::ConfigV3MutationStatus::Ok, 1, 1, 0),
        },
        MakePersistConfigV3Response(1, 0),
        MakeSnapshotBytes(10, 38));

    Require(proxy.ApplyConfigStoreGlobally(), "version mismatch should refresh snapshot and retry dirty draft");
    Require(proxy.GetConfigV3ApplyRequestCountForTest() == 2, "version mismatch should send retry patch");
    Require(proxy.GetConfigV3PersistRequestCountForTest() == 1, "successful retry should persist");
    const auto versions = proxy.GetConfigV3BaselineVersionsForTest();
    Require(versions.snapshotVersion == 38, "retry should use refreshed snapshot baseline");
    const auto request = proxy.GetLastConfigV3ApplyRequestForTest();
    Require(request.baseSchemaVersion == 10, "retry request should use refreshed schema baseline");
    Require(request.baseSnapshotVersion == 38, "retry request should use refreshed snapshot baseline");
    const auto patch = Config::deserializePatch(request.bytes, request.payloadBytes);
    Require(patch.entries.size() == 1, "rebased retry patch should contain current dirty draft entry");
    Require(patch.entries.front().keyId == Config::ConfigKeyId::SvcAutoMode,
            "rebased retry patch should target service.auto_mode");
    Require(patch.entries.front().stringValue == "false",
            "rebased retry patch should keep current dirty draft value");
    Require(!proxy.GetConfigDraftPathState("service.auto_mode").dirty,
            "successful retry persist should clear dirty state");
}

void TestVersionMismatchRebaseToPersistOnlyRetriesPersist() {
    App::ServiceProxy proxy;
    ApplyCatalog(proxy);
    ApplySnapshotWithBaseline(proxy, 10, 39);
    proxy.SetConfigDraftValue("service.auto_mode", false);
    proxy.SetConfigV3IpcTestResponses(
        true,
        std::vector<Ipc::IpcResponse>{
            MakeApplyConfigPatchV3Response(Ipc::ConfigV3MutationStatus::Ok, 1, 1, 0),
        },
        MakePersistConfigV3FailureResponse());

    Require(proxy.ApplyConfigStoreGlobally(), "initial live apply should succeed even when persist fails");
    Require(proxy.GetConfigV3ApplyRequestCountForTest() == 1, "initial live apply should send one patch");
    Require(proxy.GetConfigV3PersistRequestCountForTest() == 1, "initial live apply should attempt persist");
    auto state = proxy.GetConfigDraftPathState("service.auto_mode");
    Require(state.dirty, "persist failure should leave applied key dirty");
    Require(state.applyState == App::ConfigDraftApplyState::LiveApplied,
            "persist failure should preserve live-applied state");
    Require(state.persistState == App::ConfigDraftPersistState::Unpersisted,
            "persist failure should mark key unpersisted");

    proxy.SetConfigV3IpcTestResponses(
        true,
        std::vector<Ipc::IpcResponse>{
            MakeApplyConfigPatchV3Response(Ipc::ConfigV3MutationStatus::VersionMismatch, 0, 0, 0),
        },
        MakePersistConfigV3Response(1, 0),
        MakeSnapshotBytes(10, 40, false));

    Require(proxy.ApplyConfigStoreGlobally(), "persist-only rebase should persist without treating old VersionMismatch as rejection");
    Require(proxy.GetConfigV3ApplyRequestCountForTest() == 1,
            "persist-only rebase should not send a second patch apply after snapshot refresh");
    Require(proxy.GetConfigV3PersistRequestCountForTest() == 1,
            "persist-only rebase should still send PersistConfigV3");
    const auto result = proxy.GetLastApplyConfigResult();
    Require(result.status == App::ApplyConfigStatus::LiveApplied,
            "persist-only retry should retain live-applied outcome");
    Require(result.liveApplied, "persist-only retry should preserve liveApplied status");
    Require(result.persistAttempted && result.persisted,
            "persist-only retry should report successful persist");
    Require(!result.unpersistedLiveChanges,
            "persist-only retry success should clear unpersisted live change flag");
    state = proxy.GetConfigDraftPathState("service.auto_mode");
    Require(!state.dirty, "persist-only retry success should clear dirty state");
    Require(state.applyState == App::ConfigDraftApplyState::LiveApplied,
            "persist-only retry success should retain live-applied key state");
    Require(state.persistState == App::ConfigDraftPersistState::Persisted,
            "persist-only retry success should mark key persisted");
}

void TestNonUserOverridePersistPolicyRemainsUnpersisted() {
    App::ServiceProxy proxy;
    ApplyCatalogWithPathPersistPolicy(proxy, "service.auto_mode", Config::ConfigPersistPolicy::RuntimeOnly);
    ApplySnapshotWithBaseline(proxy, 10, 41);
    proxy.SetConfigDraftValue("service.auto_mode", false);
    proxy.SetConfigV3IpcTestResponses(
        true,
        MakeApplyConfigPatchV3Response(Ipc::ConfigV3MutationStatus::Ok, 1, 1, 0),
        MakePersistConfigV3Response(0, 1));

    Require(proxy.ApplyConfigStoreGlobally(), "runtime-only live apply should succeed");
    Require(proxy.GetConfigV3ApplyRequestCountForTest() == 1, "runtime-only live apply should send one patch");
    Require(proxy.GetConfigV3PersistRequestCountForTest() == 1, "runtime-only live apply should still attempt global persist");
    const auto result = proxy.GetLastApplyConfigResult();
    Require(result.status == App::ApplyConfigStatus::LiveApplied,
            "runtime-only live apply should report live applied");
    Require(result.persistAttempted && !result.persisted,
            "runtime-only path should not make the global result look fully persisted");
    Require(result.unpersistedLiveChanges,
            "runtime-only path should keep unpersisted live change flag set");
    const auto state = proxy.GetConfigDraftPathState("service.auto_mode");
    Require(state.dirty, "runtime-only path should remain dirty after persist success response");
    Require(state.applyState == App::ConfigDraftApplyState::LiveApplied,
            "runtime-only path should still be live applied");
    Require(state.persistState == App::ConfigDraftPersistState::Unpersisted,
            "runtime-only path should not be marked persisted");
    Require(state.persistStatus == Ipc::IpcStatusCode::Ok,
            "runtime-only skipped persist should retain the successful PersistConfigV3 IPC status");
}

void TestLocalFallbackInitializesSchemaAndDefaultStore() {
    App::ServiceProxy proxy;
    Require(!proxy.GetConfigSchemaSnapshot().entries.empty(), "local fallback schema should initialize without Service connection");
    Require(proxy.GetConfigDraftStore().has("service.mode"), "local fallback default draft should contain service.mode");
    Require(proxy.GetConfigDraftStore().has("touch.signal_cond.baseline_no_finger_max_step"),
            "local fallback default draft should contain touch catalog key");
    const auto versions = proxy.GetConfigV3BaselineVersionsForTest();
    Require(versions.catalogSchemaVersion == 0, "local fallback should not claim a v3 catalog baseline");
    Require(versions.snapshotVersion == 0, "local fallback should not claim a v3 snapshot baseline");
}

void TestV3SnapshotPayloadWritesConfigStore() {
    App::ServiceProxy proxy;
    ApplyCatalog(proxy);

    Config::ConfigV3SnapshotPayload snapshot{};
    snapshot.schemaVersion = 10;
    snapshot.snapshotVersion = 21;
    snapshot.entries.push_back({Config::ConfigKeyId::SvcMode, int32_t{1}});
    snapshot.entries.push_back({Config::ConfigKeyId::SvcPenButtonRoute, std::string("win32_only")});
    snapshot.entries.push_back({Config::ConfigKeyId::SvcAutoMode, false});
    const auto bytes = Config::serializeConfigV3Snapshot(snapshot);

    Require(proxy.ApplyConfigV3SnapshotBytesForTest(bytes.data(), bytes.size()), "v3 snapshot payload should apply");
    Require(proxy.GetConfigDraftStore().getOr<std::string>("service.mode", "") == "touch_only",
            "v3 snapshot should write numeric enum as string value to ConfigStore");
    Require(proxy.GetConfigDraftStore().getOr<std::string>("service.pen_button_route", "") == "win32_only",
            "v3 snapshot should write string enum value to ConfigStore");
    Require(proxy.GetConfigDraftStore().getOr<bool>("service.auto_mode", true) == false,
            "v3 snapshot should write bool value to ConfigStore");
    const auto versions = proxy.GetConfigV3BaselineVersionsForTest();
    Require(versions.snapshotSchemaVersion == 10, "v3 snapshot schemaVersion should be recorded");
    Require(versions.snapshotVersion == 21, "v3 snapshotVersion should be recorded");
}

void TestV3SnapshotPreservesDirtyPath() {
    App::ServiceProxy proxy;
    ApplyCatalog(proxy);
    proxy.SetConfigDraftValue("service.mode", std::string("full"));

    Config::ConfigV3SnapshotPayload snapshot{};
    snapshot.schemaVersion = 10;
    snapshot.snapshotVersion = 22;
    snapshot.entries.push_back({Config::ConfigKeyId::SvcMode, std::string("touch_only")});
    const auto bytes = Config::serializeConfigV3Snapshot(snapshot);

    Require(proxy.ApplyConfigV3SnapshotBytesForTest(bytes.data(), bytes.size()), "dirty v3 snapshot should still succeed");
    Require(proxy.GetConfigDraftStore().getOr<std::string>("service.mode", "") == "full",
            "v3 snapshot should not overwrite dirty path");
    Require(proxy.GetServiceConfigSnapshotStoreForTest().getOr<std::string>("service.mode", "") == "touch_only",
            "v3 snapshot should still update service snapshot cache for dirty path");
    Require(proxy.IsSrvModeFull(), "draft desired mode should remain dirty full value");
    Require(!proxy.IsSrvActiveModeFull(), "active mode mirror should come from service snapshot, not dirty draft");
    const auto state = proxy.GetConfigDraftPathState("service.mode");
    Require(state.dirty, "dirty draft should remain dirty after snapshot refresh");
    Require(state.hasServiceSnapshot, "dirty draft state should record service snapshot availability");
}

void TestV3SnapshotSkipsUnknownKeyId() {
    App::ServiceProxy proxy;
    ApplyCatalog(proxy);

    Config::ConfigV3SnapshotPayload snapshot{};
    snapshot.schemaVersion = 10;
    snapshot.snapshotVersion = 23;
    snapshot.entries.push_back({static_cast<Config::ConfigKeyId>(0x0200), int32_t{0}});
    snapshot.entries.push_back({Config::ConfigKeyId::SvcAutoMode, false});
    const auto bytes = Config::serializeConfigV3Snapshot(snapshot);

    Require(proxy.ApplyConfigV3SnapshotBytesForTest(bytes.data(), bytes.size()), "unknown keyId should be skipped safely");
    Require(proxy.GetConfigDraftStore().getOr<bool>("service.auto_mode", true) == false,
            "known key after unknown keyId should still apply");
}

void TestV3SnapshotSkipsUnmappedNumericEnum() {
    App::ServiceProxy proxy;
    ApplyCatalog(proxy);

    Config::ConfigV3SnapshotPayload snapshot{};
    snapshot.schemaVersion = 10;
    snapshot.snapshotVersion = 24;
    snapshot.entries.push_back({Config::ConfigKeyId::SvcMode, int32_t{999}});
    snapshot.entries.push_back({Config::ConfigKeyId::SvcAutoMode, false});
    const auto bytes = Config::serializeConfigV3Snapshot(snapshot);

    Require(proxy.ApplyConfigV3SnapshotBytesForTest(bytes.data(), bytes.size()), "unmapped enum snapshot should not fail payload apply");
    Require(proxy.GetConfigDraftStore().getOr<std::string>("service.mode", "") == "full",
            "unmapped numeric enum should not overwrite existing string enum value");
    Require(proxy.GetConfigDraftStore().getOr<bool>("service.auto_mode", true) == false,
            "valid entry after unmapped numeric enum should still apply");
}

void TestInvalidV3PayloadDoesNotClearSchemaOrStore() {
    App::ServiceProxy proxy;
    const auto beforeEntries = proxy.GetConfigSchemaSnapshot().entries.size();
    const std::vector<uint8_t> invalid{0, 1, 2, 3};

    Require(!proxy.ApplyConfigV3CatalogBytesForTest(invalid.data(), invalid.size()),
            "invalid v3 catalog should fail");
    Require(proxy.GetConfigSchemaSnapshot().entries.size() == beforeEntries,
            "failed v3 catalog should not clear schema");
    const auto beforeVersions = proxy.GetConfigV3BaselineVersionsForTest();
    Require(!proxy.ApplyConfigV3SnapshotBytesForTest(invalid.data(), invalid.size()),
            "invalid v3 snapshot should fail");
    Require(proxy.GetConfigDraftStore().getOr<std::string>("service.mode", "") == "full",
            "failed v3 snapshot should not clear store");
    const auto afterVersions = proxy.GetConfigV3BaselineVersionsForTest();
    Require(afterVersions.snapshotSchemaVersion == beforeVersions.snapshotSchemaVersion,
            "failed v3 snapshot should not change recorded schemaVersion");
    Require(afterVersions.snapshotVersion == beforeVersions.snapshotVersion,
            "failed v3 snapshot should not change recorded snapshotVersion");
}

} // namespace

int main() {
    try {
        TestSchemaCoversServiceTouchAndStylusCatalogKeys();
        TestDirtyPathKeyIdsRemainStable();
        TestDefaultYamlContainsCatalogBackedKeys();
        TestV3CatalogPayloadAppliesSchemaAndDefaults();
        TestLocalFallbackInitializesSchemaAndDefaultStore();
        TestV3SnapshotPayloadWritesConfigStore();
        TestV3SnapshotPreservesDirtyPath();
        TestV3SnapshotSkipsUnknownKeyId();
        TestV3SnapshotSkipsUnmappedNumericEnum();
        TestInvalidV3PayloadDoesNotClearSchemaOrStore();
        TestConnectedApplyUsesConfigPatchV3();
        TestConnectedApplyOkPersistFailKeepsDirty();
        TestAppliedUnpersistedSameValueCommitStillRetriesPersist();
        TestConnectedRestartRequiredApplyUsesConfigPatchV3();
        TestConnectedRejectedApplyResultIsPropagated();
        TestVersionMismatchRetryRejectsInvalidWireVersion();
        TestVersionMismatchRebaseUsesRefreshedBaseline();
        TestVersionMismatchRebaseToPersistOnlyRetriesPersist();
        TestNonUserOverridePersistPolicyRemainsUnpersisted();
        std::cout << "[TEST] ServiceProxy catalog schema tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
