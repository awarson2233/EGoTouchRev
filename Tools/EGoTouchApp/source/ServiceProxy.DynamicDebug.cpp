#include "ServiceProxyInternal.h"
#include <algorithm>
#include <chrono>
#include <cstring>

namespace App {

std::vector<DynamicDebugField> ServiceProxy::GetDynamicDebugFields() const {
    std::lock_guard<std::mutex> lk(m_dynamicDebugMutex);
    return m_dynamicDebugFields;
}

bool ServiceProxy::GetDynamicDebugValue(uint16_t fieldId, DynamicDebugValue& out) const {
    std::lock_guard<std::mutex> lk(m_dynamicDebugMutex);
    auto it = m_dynamicDebugValues.find(fieldId);
    if (it == m_dynamicDebugValues.end()) return false;
    out = it->second;
    return true;
}

void ServiceProxy::ClearDynamicDebugState() {
    {
        std::lock_guard<std::mutex> lk(m_dynamicDebugMutex);
        m_dynamicDebugFields.clear();
        m_dynamicDebugValues.clear();
        m_lastDvrDynamicSchema = DvrDynamicDebugSchema{};
    }
    m_dynamicSchemaVersion.store(0);
    m_dynamicSchemaHash.store(0);
    if (m_dvrDynamicDebugBuffer) {
        m_dvrDynamicDebugBuffer->Clear();
    }
}

bool ServiceProxy::RefreshDynamicDebugSchema() {
    if (!m_client.IsConnected()) {
        ClearDynamicDebugState();
        return false;
    }

    auto fail = [this]() {
        ClearDynamicDebugState();
        return false;
    };

    std::vector<DynamicDebugField> all;
    uint16_t offset = 0;
    uint16_t totalFields = 0;
    uint16_t schemaVersion = 0;
    uint32_t schemaHash = 0;
    bool haveSchemaHeader = false;

    while (true) {
        Ipc::DebugSchemaRequest reqSchema{};
        reqSchema.offset = offset;
        reqSchema.limit = 0;

        Ipc::IpcRequest req{};
        req.command = Ipc::IpcCommand::GetDebugSchema;
        req.paramLen = static_cast<uint16_t>(sizeof(reqSchema));
        std::memcpy(req.param, &reqSchema, sizeof(reqSchema));

        const auto resp = m_client.Send(req);
        if (!resp.success || resp.dataLen < sizeof(Ipc::DebugSchemaResponseHeader)) {
            return fail();
        }

        Ipc::DebugSchemaResponseHeader hdr{};
        std::memcpy(&hdr, resp.data, sizeof(hdr));
        if (hdr.recordSize != sizeof(Ipc::DebugFieldSchemaWire)) {
            return fail();
        }

        if (!haveSchemaHeader) {
            if (hdr.totalFields > Ipc::kDebugSnapshotMaxValues) {
                return fail();
            }
            schemaVersion = hdr.schemaVersion;
            schemaHash = hdr.schemaHash;
            totalFields = hdr.totalFields;
            haveSchemaHeader = true;
        } else if (schemaVersion != hdr.schemaVersion || schemaHash != hdr.schemaHash || totalFields != hdr.totalFields) {
            return fail();
        }

        size_t cursor = sizeof(Ipc::DebugSchemaResponseHeader);
        for (uint16_t i = 0; i < hdr.returnedFields; ++i) {
            if (cursor + sizeof(Ipc::DebugFieldSchemaWire) > resp.dataLen) {
                return fail();
            }
            Ipc::DebugFieldSchemaWire w{};
            std::memcpy(&w, resp.data + cursor, sizeof(w));
            cursor += sizeof(w);

            DynamicDebugField f;
            f.fieldId = w.fieldId;
            f.valueType = static_cast<Ipc::DebugValueType>(w.valueType);
            f.sourceKind = static_cast<Ipc::DebugSourceKind>(w.sourceKind);
            f.sourceIndex = w.sourceIndex;
            f.uiOrder = w.uiOrder;
            f.dvrTarget = static_cast<Ipc::DebugDvrTarget>(w.dvrTarget);
            f.dvrPositionMode = static_cast<Ipc::DebugDvrPositionMode>(w.dvrPositionMode);
            f.dvrIndex = w.dvrIndex;
            f.key = w.key;
            f.displayName = w.displayName;
            f.unit = w.unit;
            f.uiGroup = w.uiGroup;
            f.dvrColumnName = w.dvrColumnName;
            f.dvrAnchor = w.dvrAnchor;
            all.push_back(std::move(f));
        }

        offset = static_cast<uint16_t>(all.size());
        if (offset >= totalFields || hdr.returnedFields == 0) {
            break;
        }
    }

    {
        std::lock_guard<std::mutex> lk(m_dynamicDebugMutex);
        m_dynamicDebugFields = all;
        m_dynamicDebugValues.clear();
        m_lastDvrDynamicSchema.fields = std::move(all);
        m_lastDvrDynamicSchema.schemaVersion = schemaVersion;
        m_lastDvrDynamicSchema.schemaHash = schemaHash;
    }
    m_dynamicSchemaVersion.store(schemaVersion);
    m_dynamicSchemaHash.store(schemaHash);
    if (m_dvrDynamicDebugBuffer) {
        m_dvrDynamicDebugBuffer->Clear();
    }
    return true;
}

bool ServiceProxy::RefreshDynamicDebugSnapshot(uint64_t* outFrameTimestamp) {
    if (outFrameTimestamp) {
        *outFrameTimestamp = 0;
    }
    if (!m_client.IsConnected()) {
        ClearDynamicDebugState();
        return false;
    }

    uint16_t schemaVersion = m_dynamicSchemaVersion.load();
    if (schemaVersion == 0) {
        const auto now = std::chrono::steady_clock::now();
        bool shouldRetry = false;
        {
            std::lock_guard<std::mutex> lk(m_dynamicDebugMutex);
            if (m_nextDynamicDebugSchemaRetry.time_since_epoch().count() == 0 || now >= m_nextDynamicDebugSchemaRetry) {
                m_nextDynamicDebugSchemaRetry = now + std::chrono::seconds(2);
                shouldRetry = true;
            }
        }
        if (!shouldRetry || !RefreshDynamicDebugSchema()) {
            return false;
        }
        schemaVersion = m_dynamicSchemaVersion.load();
        if (schemaVersion == 0) {
            return false;
        }
    }

    Ipc::IpcRequest req{};
    req.command = Ipc::IpcCommand::GetDebugSnapshot;
    const auto resp = m_client.Send(req);
    if (!resp.success || resp.dataLen < Ipc::kDebugSnapshotLegacyHeaderSize) {
        {
            std::lock_guard<std::mutex> lk(m_dynamicDebugMutex);
            m_dynamicDebugValues.clear();
        }
        return false;
    }

    Ipc::DebugSnapshotHeader hdr{};
    std::memcpy(&hdr, resp.data, sizeof(hdr));
    if (hdr.schemaVersion != schemaVersion ||
        hdr.recordSize != sizeof(Ipc::DebugSnapshotValueWire) ||
        hdr.fieldCount > Ipc::kDebugSnapshotMaxValues) {
        ClearDynamicDebugState();
        return false;
    }

    const size_t expectedLen = Ipc::kDebugSnapshotLegacyHeaderSize +
        static_cast<size_t>(hdr.fieldCount) * sizeof(Ipc::DebugSnapshotValueWire);
    if (resp.dataLen < expectedLen) {
        ClearDynamicDebugState();
        return false;
    }

    uint64_t frameTimestamp = 0;
    bool hasFrameTimestamp = false;
    if (resp.dataLen >= expectedLen + sizeof(Ipc::DebugSnapshotMetadataWire)) {
        Ipc::DebugSnapshotMetadataWire meta{};
        std::memcpy(&meta, resp.data + expectedLen, sizeof(meta));
        const uint32_t knownFlags = Ipc::kDebugSnapshotHasFrameTimestamp;
        if (meta.magic == Ipc::kDebugSnapshotMetadataMagic &&
            meta.wireSize == sizeof(Ipc::DebugSnapshotMetadataWire) &&
            meta.version == Ipc::kDebugSnapshotMetadataVersion &&
            (meta.frameIdentityFlags & ~knownFlags) == 0 &&
            (meta.frameIdentityFlags & Ipc::kDebugSnapshotHasFrameTimestamp) != 0) {
            frameTimestamp = meta.frameTimestamp;
            hasFrameTimestamp = true;
        }
    }

    bool schemaMismatch = false;
    {
        std::lock_guard<std::mutex> lk(m_dynamicDebugMutex);
        if (schemaVersion != m_dynamicSchemaVersion.load()) {
            return false;
        }
        if (hdr.fieldCount != m_dynamicDebugFields.size() || m_dynamicDebugFields.empty()) {
            schemaMismatch = true;
        }

        m_dynamicDebugValues.clear();
        if (!schemaMismatch) {
            m_dynamicDebugValues.reserve(m_dynamicDebugFields.size());
            size_t cursor = Ipc::kDebugSnapshotLegacyHeaderSize;
            for (size_t i = 0; i < m_dynamicDebugFields.size(); ++i) {
                Ipc::DebugSnapshotValueWire wire{};
                std::memcpy(&wire, resp.data + cursor, sizeof(wire));
                cursor += sizeof(wire);

                if (wire.fieldId != m_dynamicDebugFields[i].fieldId ||
                    static_cast<Ipc::DebugValueType>(wire.valueType) != m_dynamicDebugFields[i].valueType) {
                    schemaMismatch = true;
                    break;
                }

                DynamicDebugValue value{};
                value.valueType = static_cast<Ipc::DebugValueType>(wire.valueType);
                value.valid = (wire.flags & 0x1u) != 0;
                value.rawValue = wire.rawValue;
                m_dynamicDebugValues.emplace(wire.fieldId, value);
            }
        }
    }

    if (schemaMismatch) {
        ClearDynamicDebugState();
        return false;
    }
    if (outFrameTimestamp && hasFrameTimestamp) {
        *outFrameTimestamp = frameTimestamp;
    }
    return true;
}

DvrDynamicDebugSchema ServiceProxy::CaptureDynamicDebugSchema() const {
    std::lock_guard<std::mutex> lk(m_dynamicDebugMutex);
    DvrDynamicDebugSchema schema;
    schema.fields = m_dynamicDebugFields;
    schema.schemaVersion = m_dynamicSchemaVersion.load();
    schema.schemaHash = m_dynamicSchemaHash.load();
    return schema;
}

DvrDynamicDebugFrame ServiceProxy::CaptureDynamicDebugFrame() const {
    std::lock_guard<std::mutex> lk(m_dynamicDebugMutex);
    DvrDynamicDebugFrame frame;
    frame.samples.reserve(m_dynamicDebugFields.size());
    for (const auto& field : m_dynamicDebugFields) {
        App::DvrDynamicDebugSample sample;
        sample.fieldId = field.fieldId;
        auto it = m_dynamicDebugValues.find(field.fieldId);
        if (it != m_dynamicDebugValues.end()) {
            sample.value = it->second;
        } else {
            sample.value.valueType = field.valueType;
        }
        frame.samples.push_back(std::move(sample));
    }
    return frame;
}

Dvr::DvrDynamicDebugFrameSlot ServiceProxy::CaptureDynamicDebugFrameSlot(uint64_t dvrSeq) const {
    std::lock_guard<std::mutex> lk(m_dynamicDebugMutex);
    Dvr::DvrDynamicDebugFrameSlot slot{};
    slot.dvrSeq = dvrSeq;
    const size_t sampleCount = std::min(m_dynamicDebugFields.size(), static_cast<size_t>(Dvr::kMaxDynamicDebugSamples));
    slot.sampleCount = static_cast<uint16_t>(sampleCount);
    for (size_t i = 0; i < sampleCount; ++i) {
        const auto& field = m_dynamicDebugFields[i];
        auto& sample = slot.samples[i];
        sample.fieldId = field.fieldId;
        sample.valueType = static_cast<uint8_t>(field.valueType);
        auto it = m_dynamicDebugValues.find(field.fieldId);
        if (it != m_dynamicDebugValues.end()) {
            sample.valid = it->second.valid ? 1 : 0;
            sample.rawValue = it->second.rawValue;
        }
    }
    return slot;
}


} // namespace App
