/*
 * @Author: Detach0-0 detach0-0@outlook.com
 * @Date: 2025-12-01 19:26:00
 * @LastEditors: Detach0-0 detach0-0@outlook.com
 * @LastEditTime: 2026-01-02 19:23:43
 * @FilePath: \EGoTouchRev-vsc\HimaxChipCore\header\HimaxProtocol.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
// HimaxHal.h
#pragma once
#include "common/DeviceError.h"
#include "Port/TouchBus.h"

#include <vector>

namespace Himax {
    // 协议层沿用的名字：芯片的总线通道就是 port 层的 ITouchBus，各平台的实现在 Port/<平台>/ 下
    using DeviceType = Port::TouchChannel;
    using HalDevice = Port::ITouchBus;

    namespace HimaxProtocol {

        ChipResult<> burst_enable(HalDevice *dev, bool isEnable);
        ChipResult<> register_read(HalDevice *dev, const uint32_t addr, uint8_t* buffer, uint32_t len);
        ChipResult<> register_write(HalDevice *dev, const uint32_t addr, const uint8_t* buffer, uint32_t len);

        void build_command_packet(uint8_t cmd_id, uint8_t cmd_val, uint8_t* packet);
        ChipResult<> write_and_verify(HalDevice* dev, const uint32_t addr, const uint8_t* data, uint32_t len, uint32_t verify_len = 0);
        ChipResult<> safeModeSetRaw(HalDevice* dev, const bool state);
        
        /**
         * @brief 向芯片发送命令包
         * @param dev 目标设备
         * @param cmd_id 命令 ID
         * @param cmd_val 命令值
         * @param current_slot 当前 slot（会被更新）
         * @return ChipResult 是否成功
         */
        ChipResult<> send_command(HalDevice* dev, uint8_t cmd_id, uint8_t cmd_val, uint8_t& current_slot);
    }
}
