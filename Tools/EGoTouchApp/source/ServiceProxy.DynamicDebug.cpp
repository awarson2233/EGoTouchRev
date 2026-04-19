#include "ServiceProxyInternal.h"
#include <cstring>
#include <unordered_map>

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

bool ServiceProxy::RefreshDynamicDebugSchema() {
    if (!m_client.IsConnected()) return false;

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
            return false;
        }

        Ipc::DebugSchemaResponseHeader hdr{};
        std::memcpy(&hdr, resp.data, sizeof(hdr));
        if (hdr.recordSize != sizeof(Ipc::DebugFieldSchemaWire)) {
            return false;
        }

        if (!haveSchemaHeader) {
            schemaVersion = hdr.schemaVersion;
            schemaHash = hdr.schemaHash;
            totalFields = hdr.totalFields;
            haveSchemaHeader = true;
        } else if (schemaVersion != hdr.schemaVersion || schemaHash != hdr.schemaHash || totalFields != hdr.totalFields) {
            return false;
        }

        size_t cursor = sizeof(Ipc::DebugSchemaResponseHeader);
        for (uint16_t i = 0; i < hdr.returnedFields; ++i) {
            if (cursor + sizeof(Ipc::DebugFieldSchemaWire) > resp.dataLen) {
                return false;
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
        }
        frame.samples.push_back(std::move(sample));
    }
    return frame;
}

bool ServiceProxy::PollDynamicDebugSnapshot() {
    if (!m_client.IsConnected()) return false;

    Ipc::IpcRequest req{};
    req.command = Ipc::IpcCommand::GetDebugSnapshot;
    const auto resp = m_client.Send(req);
    if (!resp.success || resp.dataLen < sizeof(Ipc::DebugSnapshotHeader)) {
        return false;
    }

    Ipc::DebugSnapshotHeader hdr{};
    std::memcpy(&hdr, resp.data, sizeof(hdr));
    if (hdr.recordSize != sizeof(Ipc::DebugSnapshotValueWire)) {
        return false;
    }

    if (m_dynamicSchemaVersion.load() != hdr.schemaVersion) {
        if (!RefreshDynamicDebugSchema()) {
            return false;
        }
    }

    std::unordered_map<uint16_t, DynamicDebugValue> values;
    size_t cursor = sizeof(Ipc::DebugSnapshotHeader);
    for (uint16_t i = 0; i < hdr.fieldCount; ++i) {
        if (cursor + sizeof(Ipc::DebugSnapshotValueWire) > resp.dataLen) {
            break;
        }
        Ipc::DebugSnapshotValueWire v{};
        std::memcpy(&v, resp.data + cursor, sizeof(v));
        cursor += sizeof(v);

        DynamicDebugValue dv;
        dv.valueType = static_cast<Ipc::DebugValueType>(v.valueType);
        dv.valid = (v.flags & 0x1) != 0;
        dv.rawValue = v.rawValue;
        values[v.fieldId] = dv;
    }

    {
        std::lock_guard<std::mutex> lk(m_dynamicDebugMutex);
        m_dynamicDebugValues = std::move(values);
    }
    return true;
}

} // namespace App
