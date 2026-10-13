#pragma once
// ══════════════════════════════════════════════════════════════════════
// TsaOutputMapper — 把 TSA_Rpt* 逐点翻成业务侧的 TouchContact。
//
// 在场 / 抬起的判定照搬 ApDaemon 的 TsaFrame::ObInput：TSA_RptIsTouchDown 非零
// 即在场，否则 TSA_RptIsLiftOff 非零即抬起，两者都为零的点不上报。TSACore 只给
// 「在场」而不区分按下与移动，并且会整帧或逐点地把触点从输出里拿掉（不带抬起
// 事件），所以这里按 ID 记账，保证每个 ID 对 HID 侧都是 Down → Move… → Up。
//
// 坐标：驱动把整帧倒序后才交给 TSACore，TSACore 报的坐标因此相对面板原始
// 行列旋转了 180°。业务侧（VhfReporter、诊断界面）沿用原始行列方向的显示
// 像素，这里把两个轴都翻回去。
// ══════════════════════════════════════════════════════════════════════

#include <array>
#include <cstdint>

#include "SolverTypes.h"
#include "TsaCoreApi.h"
#include "TsaHostShim.h"

namespace Solvers::Tsa {

class TsaOutputMapper {
public:
    void Reset() noexcept { m_activeIds = 0; }

    /// 读取本帧 TSACore 输出，写入 contacts。返回写入的点数（含抬起）。
    template <typename ContactList>
    int Collect(ContactList& contacts) noexcept {
        contacts.clear();
        uint32_t activeIds = 0;
        uint32_t liftedIds = 0;
        int written = 0;
        const uint32_t count = TSA_RptTouchNum();
        for (uint32_t i = 0; i < count && i < kMaxContacts; ++i) {
            const auto index = static_cast<uint8_t>(i);
            const bool present = TSA_RptIsTouchDown(index) != 0;
            if (!present && TSA_RptIsLiftOff(index) == 0) continue;

            const int tsaId = TSA_RptTouchID(index);
            if (tsaId < 0 || tsaId >= kMaxIds) continue;
            const uint32_t idBit = 1u << tsaId;
            const bool wasActive = (m_activeIds & idBit) != 0;
            // 从未按下过的点（按下防抖期间就离开）也会带抬起事件，HID 侧不需要
            if (!present && !wasActive) continue;

            TouchContact contact{};
            // VHF 只上报 id > 0 的点，TSACore 的 ID 从 0 起
            contact.id = tsaId + 1;
            contact.x = Unrotate(TSA_RptTouchXPos(index), TsaHostShim::kXRes);
            contact.y = Unrotate(TSA_RptTouchYPos(index), TsaHostShim::kYRes);
            contact.signalSum = static_cast<int>(TSA_RptTouchPressure(index));
            contact.isReported = true;
            if (present) {
                contact.reportEvent = wasActive ? TouchReportMove : TouchReportDown;
                contact.state = wasActive ? TouchStateMove : TouchStateDown;
                activeIds |= idBit;
                m_last[static_cast<size_t>(tsaId)] = {contact.x, contact.y};
            } else {
                contact.reportEvent = TouchReportUp;
                contact.state = TouchStateUp;
                liftedIds |= idBit;
            }
            if (contacts.try_push_back(contact)) ++written;
        }

        // 上一帧在场、本帧既不在场也没给抬起的 ID：在最后位置补一个抬起
        uint32_t vanished = m_activeIds & ~(activeIds | liftedIds);
        for (int id = 0; vanished != 0; ++id, vanished >>= 1) {
            if ((vanished & 1u) == 0) continue;
            TouchContact contact{};
            contact.id = id + 1;
            contact.x = m_last[static_cast<size_t>(id)].x;
            contact.y = m_last[static_cast<size_t>(id)].y;
            contact.isReported = true;
            contact.reportEvent = TouchReportUp;
            contact.state = TouchStateUp;
            if (contacts.try_push_back(contact)) ++written;
        }
        m_activeIds = activeIds;
        return written;
    }

    [[nodiscard]] bool HasActiveContacts() const noexcept { return m_activeIds != 0; }

private:
    static constexpr uint32_t kMaxContacts = 10;  // ApDaemon 同样截到 10 点
    static constexpr int kMaxIds = 32;

    struct Pos {
        float x = 0.0f;
        float y = 0.0f;
    };

    static float Unrotate(uint64_t scaled, uint16_t res) noexcept {
        const int span = static_cast<int>(res) * TsaHostShim::kResScale;
        const int v = span - 1 - static_cast<int>(scaled & 0xffffu);
        return static_cast<float>(v < 0 ? 0 : v) / static_cast<float>(TsaHostShim::kResScale);
    }

    uint32_t m_activeIds = 0;
    std::array<Pos, kMaxIds> m_last{};
};

}  // namespace Solvers::Tsa
