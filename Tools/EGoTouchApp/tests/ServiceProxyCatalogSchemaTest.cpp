#include "ConfigUIRenderer.h"
#include "ServiceProxyInternal.h"
#include "config/ConfigKeyMap.h"
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

void RequireDraftState(const App::ServiceProxy& proxy,
                       std::string_view path,
                       App::ConfigDraftApplyState applyState,
                       App::ConfigDraftPersistState persistState,
                       bool dirty) {
    const auto state = proxy.GetConfigDraftPathState(path);
    Require(state.dirty == dirty, "draft dirty state mismatch");
    Require(state.applyState == applyState, "draft apply state mismatch");
    Require(state.persistState == persistState, "draft persist state mismatch");
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

void TestConfigUiRendererBadges() {
    Require(std::string(App::ConfigScopeBadge(Config::ConfigScope::ServicePolicy)) == "Service",
            "service scope badge mismatch");
    Require(std::string(App::ConfigScopeBadge(Config::ConfigScope::TouchPipeline)) == "Touch",
            "touch scope badge mismatch");
    Require(std::string(App::ConfigApplyTimingBadge(Config::ConfigApplyTiming::RestartRequired)) == "Restart",
            "restart timing badge mismatch");
    Require(std::string(App::ConfigPersistPolicyBadge(Config::ConfigPersistPolicy::UserOverride)) == "UserOverride",
            "user override persist badge mismatch");
    Require(std::string(App::ConfigApplyStateBadge(App::ConfigUIApplyState::StagedRestartRequired)) == "StagedRestartRequired",
            "staged apply state badge mismatch");
    Require(std::string(App::ConfigPersistStateBadge(App::ConfigUIPersistState::Unpersisted)) == "Unpersisted",
            "unpersisted state badge mismatch");
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
    Require(proxy.GetConfigV3PersistRequestCountForTest() == 0, "connected apply should not persist session-only changes");
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
    Require(!result.persistAttempted && !result.persisted, "live v3 apply should remain session-only");
    const auto state = proxy.GetConfigDraftPathState("service.auto_mode");
    Require(!state.dirty, "session-only live apply should clear dirty state");
    Require(state.applyState == App::ConfigDraftApplyState::LiveApplied, "session-only live apply should retain live-applied key state");
    Require(state.persistState == App::ConfigDraftPersistState::NotAttempted, "session-only live apply should not mark key persisted");
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
    Require(!result.persistAttempted && !result.persisted, "restart-required v3 apply should remain session-only");
    const auto state = proxy.GetConfigDraftPathState("service.mode");
    Require(!state.dirty, "session-only restart-required key should clear dirty state");
    Require(state.applyState == App::ConfigDraftApplyState::StagedRestartRequired,
            "restart-required key should be staged, not live-applied");
    Require(state.persistState == App::ConfigDraftPersistState::NotAttempted,
            "restart-required key should not be marked persisted");
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
    Require(proxy.GetConfigV3PersistRequestCountForTest() == 0, "successful retry should not persist session-only changes");
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

void TestLocalFallbackInitializesSchemaAndDefaultStore() {
    App::ServiceProxy proxy;
    Require(!proxy.GetConfigSchemaSnapshot().entries.empty(), "local fallback schema should initialize without Service connection");
    Require(FindEntry(proxy.GetConfigSchemaSnapshot(), "service.mode") != nullptr,
            "local fallback schema should include service.mode");
    Require(proxy.GetConfigDraftStore().has("service.mode"), "local fallback default draft should contain service.mode");
    Require(proxy.GetConfigDraftStore().has("service.auto_mode"), "local fallback default draft should contain service.auto_mode");
    Require(proxy.GetConfigDraftStore().has("touch.signal_cond.baseline_no_finger_max_step"),
            "local fallback default draft should contain touch catalog key");
    const auto versions = proxy.GetConfigV3BaselineVersionsForTest();
    Require(versions.catalogSchemaVersion == 0, "local fallback should not claim a v3 catalog baseline");
    Require(versions.snapshotVersion == 0, "local fallback should not claim a v3 snapshot baseline");
}

void TestOfflineLiveApplyRequiresServiceConnection() {
    App::ServiceProxy proxy;
    proxy.SetConfigDraftValue("service.auto_mode", false);

    Require(!proxy.IsConfigAdjustmentAllowed(), "offline config adjustment should be disabled");
    Require(proxy.GetConfigDraftStore().getOr<bool>("service.auto_mode", false),
            "offline SetConfigDraftValue should not overwrite fallback draft");
    Require(!proxy.ApplyConfigStoreGlobally(), "offline live-applicable change should require Service config sync");
    const auto result = proxy.GetLastApplyConfigResult();
    Require(result.status == App::ApplyConfigStatus::LiveApplyFailed, "offline live-applicable change should report LiveApplyFailed");
    Require(!result.liveApplied, "offline live-applicable change should not set liveApplied");
    Require(!result.restartRequired, "offline live-applicable change should not require restart");
    Require(!result.persistAttempted && !result.persisted, "offline path should not call Service persist");
    RequireDraftState(proxy,
                      "service.auto_mode",
                      App::ConfigDraftApplyState::Clean,
                      App::ConfigDraftPersistState::NotAttempted,
                      false);
}

void TestOfflineRestartRequiredChangeRequiresServiceConnection() {
    App::ServiceProxy proxy;
    proxy.SetConfigDraftValue("service.mode", std::string("touch_only"));

    Require(!proxy.IsConfigAdjustmentAllowed(), "offline config adjustment should be disabled");
    Require(proxy.GetConfigDraftStore().getOr<std::string>("service.mode", "") == "full",
            "offline SetConfigDraftValue should not overwrite fallback service.mode");
    Require(!proxy.ApplyConfigStoreGlobally(), "offline restart-required change should require Service config sync");
    const auto result = proxy.GetLastApplyConfigResult();
    Require(result.status == App::ApplyConfigStatus::LiveApplyFailed, "offline restart-required change should report LiveApplyFailed");
    Require(!result.liveApplied, "restart-required-only change should not report liveApplied");
    Require(!result.restartRequired, "offline restart-required change should not be staged locally");
    RequireDraftState(proxy,
                      "service.mode",
                      App::ConfigDraftApplyState::Clean,
                      App::ConfigDraftPersistState::NotAttempted,
                      false);
}

void TestCatalogNotReadyBlocksSnapshotApplyAndConnectedPatch() {
    App::ServiceProxy proxy;
    proxy.SetConfigDraftValue("service.auto_mode", false);
    proxy.SetConfigV3IpcTestResponses(
        true,
        std::vector<Ipc::IpcResponse>{
            MakeApplyConfigPatchV3Response(Ipc::ConfigV3MutationStatus::Ok, 1, 1, 0),
        },
        MakePersistConfigV3Response(1, 0),
        MakeSnapshotBytes(10, 55));

    Require(!proxy.IsConfigAdjustmentAllowed(), "catalog-not-ready path should keep config adjustment disabled");
    Require(!proxy.ApplyConfigStoreGlobally(), "connected apply should fail while catalog is not ready");
    Require(proxy.GetConfigV3ApplyRequestCountForTest() == 0,
            "catalog-not-ready path should not send a v3 patch even when snapshot bytes are available");
    Require(proxy.GetConfigV3PersistRequestCountForTest() == 0,
            "catalog-not-ready path should not persist");
    Require(!proxy.GetServiceConfigSnapshotStoreForTest().has("service.auto_mode"),
            "catalog-not-ready path should not apply fetched snapshot bytes");
    const auto versions = proxy.GetConfigV3BaselineVersionsForTest();
    Require(versions.snapshotVersion == 0,
            "catalog-not-ready path should not establish a snapshot baseline from fetched bytes");
    const auto result = proxy.GetLastApplyConfigResult();
    Require(result.status == App::ApplyConfigStatus::LiveApplyFailed,
            "catalog-not-ready connected apply should report LiveApplyFailed");
    RequireDraftState(proxy,
                      "service.auto_mode",
                      App::ConfigDraftApplyState::Clean,
                      App::ConfigDraftPersistState::NotAttempted,
                      false);
}

void TestCatalogFailureAfterBaselineBlocksPatchPersist() {
    App::ServiceProxy proxy;
    ApplyCatalog(proxy);
    ApplySnapshotWithBaseline(proxy, 10, 56);
    proxy.SetConfigDraftValue("service.auto_mode", false);

    const std::vector<uint8_t> invalidCatalog{0, 1, 2, 3};
    Require(!proxy.ApplyConfigV3CatalogBytesForTest(invalidCatalog.data(), invalidCatalog.size()),
            "invalid catalog should mark the v3 catalog not ready");
    proxy.SetConfigV3IpcTestResponses(
        true,
        std::vector<Ipc::IpcResponse>{
            MakeApplyConfigPatchV3Response(Ipc::ConfigV3MutationStatus::Ok, 1, 1, 0),
        },
        MakePersistConfigV3Response(1, 0),
        MakeSnapshotBytes(10, 57));

    Require(!proxy.ApplyConfigStoreGlobally(),
            "connected apply should fail after catalog failure even with an old snapshot baseline");
    Require(proxy.GetConfigV3ApplyRequestCountForTest() == 0,
            "catalog failure after baseline should not use the old baseline for patch apply");
    Require(proxy.GetConfigV3PersistRequestCountForTest() == 0,
            "catalog failure after baseline should not persist");
    const auto versions = proxy.GetConfigV3BaselineVersionsForTest();
    Require(versions.snapshotVersion == 56,
            "catalog failure after baseline should not refresh or replace the old snapshot baseline");
    const auto result = proxy.GetLastApplyConfigResult();
    Require(result.status == App::ApplyConfigStatus::LiveApplyFailed,
            "catalog failure after baseline should report LiveApplyFailed");
    RequireDraftState(proxy,
                      "service.auto_mode",
                      App::ConfigDraftApplyState::Pending,
                      App::ConfigDraftPersistState::NotAttempted,
                      true);
}

void TestConnectedV3SnapshotFetchFailureBlocksEditingBeforeDirtyDraft() {
    App::ServiceProxy proxy;
    ApplyCatalog(proxy);
    proxy.SetConfigDraftValue("service.auto_mode", false);
    proxy.SetConfigV3IpcTestResponses(true, Ipc::IpcResponse{}, Ipc::IpcResponse{});

    Require(!proxy.IsConfigAdjustmentAllowed(), "snapshot-missing config adjustment should stay disabled");
    Require(!proxy.ApplyConfigStoreGlobally(), "connected apply without v3 snapshot baseline should fail fast");
    Require(proxy.GetConfigV3ApplyRequestCountForTest() == 0,
            "connected baseline failure should not send a v3 patch with unknown base version");
    Require(proxy.GetConfigV3PersistRequestCountForTest() == 0,
            "connected baseline failure should not persist");
    const auto result = proxy.GetLastApplyConfigResult();
    Require(result.status == App::ApplyConfigStatus::LiveApplyFailed,
            "connected v3 snapshot fetch failure should report LiveApplyFailed");
    Require(!result.liveApplied && !result.restartRequired,
            "connected v3 snapshot fetch failure should not report applied state");
    const auto state = proxy.GetConfigDraftPathState("service.auto_mode");
    Require(!state.dirty, "snapshot-missing SetConfigDraftValue should not create dirty draft");
    Require(state.applyState == App::ConfigDraftApplyState::Clean,
            "snapshot-missing draft state should remain clean");
    Require(state.persistState == App::ConfigDraftPersistState::NotAttempted,
            "connected v3 snapshot fetch failure should not mark persist attempted");
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
    Config::ConfigV3SnapshotPayload baseline{};
    baseline.schemaVersion = 10;
    baseline.snapshotVersion = 20;
    baseline.entries.push_back({Config::ConfigKeyId::SvcMode, std::string("touch_only")});
    const auto baselineBytes = Config::serializeConfigV3Snapshot(baseline);
    Require(proxy.ApplyConfigV3SnapshotBytesForTest(baselineBytes.data(), baselineBytes.size()), "baseline v3 snapshot should apply");
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

void TestV3SnapshotOverwriteClearsDirtyDraft() {
    App::ServiceProxy proxy;
    ApplyCatalog(proxy);
    Config::ConfigV3SnapshotPayload baseline{};
    baseline.schemaVersion = 10;
    baseline.snapshotVersion = 25;
    baseline.entries.push_back({Config::ConfigKeyId::SvcMode, std::string("touch_only")});
    const auto baselineBytes = Config::serializeConfigV3Snapshot(baseline);
    Require(proxy.ApplyConfigV3SnapshotBytesForTest(baselineBytes.data(), baselineBytes.size()), "baseline v3 snapshot should apply");
    proxy.SetConfigDraftValue("service.mode", std::string("full"));
    Require(proxy.GetConfigDraftPathState("service.mode").dirty, "service.mode should be dirty before forced sync");

    Config::ConfigV3SnapshotPayload syncSnapshot{};
    syncSnapshot.schemaVersion = 10;
    syncSnapshot.snapshotVersion = 26;
    syncSnapshot.entries.push_back({Config::ConfigKeyId::SvcMode, std::string("touch_only")});
    const auto syncBytes = Config::serializeConfigV3Snapshot(syncSnapshot);
    Require(proxy.ApplyConfigV3SnapshotBytesForTest(syncBytes.data(), syncBytes.size(), true), "forced v3 snapshot should apply");
    Require(proxy.GetConfigDraftStore().getOr<std::string>("service.mode", "") == "touch_only",
            "forced v3 snapshot should overwrite dirty draft with Service value");
    Require(!proxy.GetConfigDraftPathState("service.mode").dirty,
            "forced v3 snapshot should clear dirty state");
    Require(!proxy.IsSrvModeFull(), "forced v3 snapshot should sync desired mode mirror from Service value");
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

    ApplyCatalog(proxy);
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

void TestPublicSettersNoOpUntilConfigSyncReady() {
    App::ServiceProxy proxy;

    proxy.SetSrvStylusVhfEnabled(false);
    proxy.SetSrvAutoMode(false);
    proxy.SetPenButtonMode(PenButtonMode::NativeBarrel);
    proxy.SetPenButtonRoute(PenButtonRoute::Win32Only);
    proxy.SetConfigDraftValue("service.mode", std::string("touch_only"));

    Require(!proxy.IsConfigAdjustmentAllowed(), "public setters should not enable editing before config sync");
    Require(proxy.IsSrvStylusVhfEnabled(), "stylus VHF setter should be ignored before config sync");
    Require(proxy.IsSrvAutoMode(), "auto mode setter should be ignored before config sync");
    Require(proxy.GetPenButtonMode() == PenButtonMode::OemCustom,
            "pen button mode setter should be ignored before config sync");
    Require(proxy.GetPenButtonRoute() == PenButtonRoute::VhfOnly,
            "pen button route setter should be ignored before config sync");
    Require(proxy.GetConfigDraftStore().getOr<std::string>("service.mode", "") == "full",
            "raw draft setter should be ignored before config sync");
    RequireDraftState(proxy,
                      "service.mode",
                      App::ConfigDraftApplyState::Clean,
                      App::ConfigDraftPersistState::NotAttempted,
                      false);
}

void TestCatalogAndSnapshotReadyUsesServiceSnapshotDraft() {
    App::ServiceProxy proxy;
    ApplyCatalog(proxy);

    Config::ConfigV3SnapshotPayload snapshot{};
    snapshot.schemaVersion = 10;
    snapshot.snapshotVersion = 61;
    snapshot.entries.push_back({Config::ConfigKeyId::SvcMode, std::string("touch_only")});
    snapshot.entries.push_back({Config::ConfigKeyId::SvcAutoMode, false});
    snapshot.entries.push_back({Config::ConfigKeyId::SvcPenButtonRoute, std::string("win32_only")});
    const auto bytes = Config::serializeConfigV3Snapshot(snapshot);

    Require(proxy.ApplyConfigV3SnapshotBytesForTest(bytes.data(), bytes.size(), true),
            "catalog and snapshot should complete config sync");
    Require(proxy.GetConfigServiceSyncState() == App::ConfigServiceSyncState::Ready,
            "test sync helper should mark Ready only after snapshot success");
    Require(proxy.GetConfigDraftStore().getOr<std::string>("service.mode", "") == "touch_only",
            "editable draft should come from Service snapshot");
    Require(proxy.GetConfigDraftStore().getOr<bool>("service.auto_mode", true) == false,
            "editable draft bool should come from Service snapshot");
    Require(proxy.GetConfigDraftStore().getOr<std::string>("service.pen_button_route", "") == "win32_only",
            "editable draft enum should come from Service snapshot");
}

void TestSnapshotSchemaMismatchBlocksReadyAndPreservesDraft() {
    App::ServiceProxy proxy;
    ApplyCatalog(proxy);
    const auto beforeMode = proxy.GetConfigDraftStore().getOr<std::string>("service.mode", "");
    const auto beforeVersions = proxy.GetConfigV3BaselineVersionsForTest();

    Config::ConfigV3SnapshotPayload snapshot{};
    snapshot.schemaVersion = 999;
    snapshot.snapshotVersion = 62;
    snapshot.entries.push_back({Config::ConfigKeyId::SvcMode, std::string("touch_only")});
    const auto bytes = Config::serializeConfigV3Snapshot(snapshot);

    Require(!proxy.ApplyConfigV3SnapshotBytesForTest(bytes.data(), bytes.size(), true),
            "schema-mismatched snapshot should fail");
    Require(proxy.GetConfigServiceSyncState() != App::ConfigServiceSyncState::Ready,
            "schema-mismatched snapshot should not mark config sync Ready");
    Require(!proxy.IsConfigAdjustmentAllowed(),
            "schema-mismatched snapshot should keep editing disabled");
    Require(proxy.GetConfigDraftStore().getOr<std::string>("service.mode", "") == beforeMode,
            "schema-mismatched snapshot should not overwrite editable draft");
    const auto afterVersions = proxy.GetConfigV3BaselineVersionsForTest();
    Require(afterVersions.snapshotSchemaVersion == beforeVersions.snapshotSchemaVersion,
            "schema-mismatched snapshot should not update snapshot schemaVersion");
    Require(afterVersions.snapshotVersion == beforeVersions.snapshotVersion,
            "schema-mismatched snapshot should not update snapshotVersion");
}

void TestForcedSnapshotOverwriteClearsDirtyBaselinePathAndApplyState() {
    App::ServiceProxy proxy;
    ApplyCatalog(proxy);
    Config::ConfigV3SnapshotPayload baseline{};
    baseline.schemaVersion = 10;
    baseline.snapshotVersion = 63;
    baseline.entries.push_back({Config::ConfigKeyId::SvcMode, std::string("touch_only")});
    const auto baselineBytes = Config::serializeConfigV3Snapshot(baseline);
    Require(proxy.ApplyConfigV3SnapshotBytesForTest(baselineBytes.data(), baselineBytes.size()),
            "baseline v3 snapshot should apply");
    proxy.SetConfigDraftValue("service.mode", std::string("full"));
    const auto beforeState = proxy.GetConfigDraftPathState("service.mode");
    Require(beforeState.dirty, "precondition dirty edit should exist before forced sync");
    Require(beforeState.hasDirtyBaseline, "precondition dirty edit should keep a dirty baseline");
    Require(beforeState.applyState == App::ConfigDraftApplyState::Pending,
            "precondition dirty edit should be pending before forced sync");

    Config::ConfigV3SnapshotPayload snapshot{};
    snapshot.schemaVersion = 10;
    snapshot.snapshotVersion = 64;
    snapshot.entries.push_back({Config::ConfigKeyId::SvcMode, std::string("touch_only")});
    const auto bytes = Config::serializeConfigV3Snapshot(snapshot);

    Require(proxy.ApplyConfigV3SnapshotBytesForTest(bytes.data(), bytes.size(), true),
            "forced snapshot overwrite should succeed");
    Require(proxy.GetConfigDraftStore().getOr<std::string>("service.mode", "") == "touch_only",
            "forced snapshot should replace draft with Service value");
    RequireDraftState(proxy,
                      "service.mode",
                      App::ConfigDraftApplyState::Clean,
                      App::ConfigDraftPersistState::NotAttempted,
                      false);
    const auto afterState = proxy.GetConfigDraftPathState("service.mode");
    Require(!afterState.hasDirtyBaseline,
            "forced snapshot should clear path dirty baseline state");
    const auto result = proxy.GetLastApplyConfigResult();
    Require(result.status == App::ApplyConfigStatus::NotAttempted,
            "forced snapshot should clear last apply status");
    Require(!proxy.HasUnpersistedLiveConfigChanges(),
            "forced snapshot should clear unpersisted live config flag");
}

void TestNoChangeApplyClearsPendingDirtyAndReturnsNoChanges() {
    App::ServiceProxy proxy;
    ApplyCatalog(proxy);
    ApplySnapshotWithBaseline(proxy, 10, 65);

    proxy.SetConfigDraftValue("service.auto_mode", false);
    Require(proxy.GetConfigDraftPathState("service.auto_mode").dirty,
            "precondition dirty edit should be pending");
    proxy.SetConfigDraftValue("service.auto_mode", true);
    Require(proxy.GetConfigDraftPathState("service.auto_mode").dirty == false,
            "returning to Service snapshot should clear dirty immediately");

    Require(proxy.ApplyConfigStoreGlobally(), "no-change apply should succeed");
    const auto result = proxy.GetLastApplyConfigResult();
    Require(result.status == App::ApplyConfigStatus::NoChanges,
            "no-change apply should return NoChanges status");
    Require(proxy.GetConfigV3ApplyRequestCountForTest() == 0,
            "no-change apply should not send v3 patch");
    RequireDraftState(proxy,
                      "service.auto_mode",
                      App::ConfigDraftApplyState::Clean,
                      App::ConfigDraftPersistState::NotAttempted,
                      false);
}

} // namespace

int main() {
    try {
        TestSchemaCoversServiceTouchAndStylusCatalogKeys();
        TestDirtyPathKeyIdsRemainStable();
        TestConfigUiRendererBadges();
        TestV3CatalogPayloadAppliesSchemaAndDefaults();
        TestLocalFallbackInitializesSchemaAndDefaultStore();
        TestOfflineLiveApplyRequiresServiceConnection();
        TestOfflineRestartRequiredChangeRequiresServiceConnection();
        TestCatalogNotReadyBlocksSnapshotApplyAndConnectedPatch();
        TestCatalogFailureAfterBaselineBlocksPatchPersist();
        TestConnectedV3SnapshotFetchFailureBlocksEditingBeforeDirtyDraft();
        TestV3SnapshotPayloadWritesConfigStore();
        TestV3SnapshotPreservesDirtyPath();
        TestV3SnapshotOverwriteClearsDirtyDraft();
        TestV3SnapshotSkipsUnknownKeyId();
        TestV3SnapshotSkipsUnmappedNumericEnum();
        TestInvalidV3PayloadDoesNotClearSchemaOrStore();
        TestPublicSettersNoOpUntilConfigSyncReady();
        TestCatalogAndSnapshotReadyUsesServiceSnapshotDraft();
        TestSnapshotSchemaMismatchBlocksReadyAndPreservesDraft();
        TestForcedSnapshotOverwriteClearsDirtyBaselinePathAndApplyState();
        TestNoChangeApplyClearsPendingDirtyAndReturnsNoChanges();
        TestConnectedApplyUsesConfigPatchV3();
        TestConnectedRestartRequiredApplyUsesConfigPatchV3();
        TestConnectedRejectedApplyResultIsPropagated();
        TestVersionMismatchRetryRejectsInvalidWireVersion();
        TestVersionMismatchRebaseUsesRefreshedBaseline();
        std::cout << "[TEST] ServiceProxy catalog schema tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
