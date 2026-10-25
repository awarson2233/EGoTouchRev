/*
 * @Author: Detach0-0 detach0-0@outlook.com
 * @Date: 2025-12-01 19:25:55
 * @LastEditors: Detach0-0 detach0-0@outlook.com
 * @LastEditTime: 2025-12-29 13:13:56
 * @FilePath: \EGoTouchRev-vsc\HimaxChipCore\source\HimaxHal.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "himax/HimaxProtocol.h"
#include "himax/HimaxByteUtils.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <array>
#include <cstring>

namespace Himax {

    ChipResult<> HimaxProtocol::burst_enable(HalDevice *dev, bool isEnable) {
        if (!dev || !dev->IsValid()) return std::unexpected(ChipError::CommunicationError);

        uint8_t tmp_data[4]{};
        tmp_data[0] = 0x31;

        if (auto res = dev->WriteBus(0x13, nullptr, tmp_data, 1); !res) {
            return res;
        }

        tmp_data[0] = (0x12 | isEnable);

        if (auto res = dev->WriteBus(0x0D, nullptr, tmp_data, 1); !res) {
            return res;
        }
        return {};
    }

    namespace {

    ChipResult<> ConfigureBurstForLength(HalDevice* dev, uint32_t len) {
        return HimaxProtocol::burst_enable(dev, len > 4);
    }

    } // anonymous namespace

    ChipResult<> HimaxProtocol::register_read(HalDevice *dev, const uint32_t addr, uint8_t *buffer, uint32_t len) {
        if (!dev || !dev->IsValid()) return std::unexpected(ChipError::CommunicationError);

        uint8_t tmp_data[4];
        detail::ParseAddressLittleEndian(addr, tmp_data, 4);

        if (len > 256) {
            return std::unexpected(ChipError::InvalidOperation);
        }

        if (auto res = ConfigureBurstForLength(dev, len); !res) return res;

        if (auto res = dev->WriteBus(0x00, tmp_data, nullptr, 0); !res) {
            return res;
        }

        tmp_data[0] = 0;
        if (auto res = dev->WriteBus(0x0C, nullptr, tmp_data, 1); !res) {
            return res;
        }

        if (auto res = dev->ReadBus(0x08, buffer, len); !res) {
            return res;
        }
        
        return {};
    }

    ChipResult<> HimaxProtocol::register_write(HalDevice *dev, const uint32_t addr, const uint8_t *data, uint32_t len) {
        if (!dev || !dev->IsValid()) return std::unexpected(ChipError::CommunicationError);
        uint8_t tmp_data[4];
        detail::ParseAddressLittleEndian(addr, tmp_data, 4);

        if (auto res = ConfigureBurstForLength(dev, len); !res) return res;
        return dev->WriteBus(0x00, tmp_data, data, len);
    }

    void HimaxProtocol::build_command_packet(uint8_t cmd_id, uint8_t cmd_val, uint8_t *packet) {
        if (!packet) {
            return;
        }

        // 1. 初始化 16 字节缓冲区 (对应 DLL 中的局部变量布局)
        std::array<uint8_t, 16> tmp_data{};

        // 2. 设置包含“虚拟包头”的初始值，用于计算 CheckSum
        tmp_data[0] = 0xA8;     // local_60[0]
        tmp_data[1] = 0x8A;     // local_60[1]
        tmp_data[2] = cmd_id;  // local_60[2]
        tmp_data[3] = 0x00;     // local_60[3]
        tmp_data[4] = cmd_val;  // local_60[4]
        
        // 3. 计算 CheckSum (iVar13)
        // 逻辑：将 16 字节看作 8 个 uint16 (小端序) 进行累加
        uint32_t checksum_sum = 0;
        for (int i = 0; i < 16; i += 2) {
            // 组合低字节和高字节为 16位 整数
            uint16_t word = static_cast<uint16_t>(tmp_data[i]) |
                            (static_cast<uint16_t>(tmp_data[i + 1]) << 8);
            checksum_sum += word;
        }

        // 4. 计算补码 (Two's Complement)
        // 逻辑：0x10000 - Sum，并截断为 16 位
        uint16_t final_checksum = static_cast<uint16_t>((0x10000 - checksum_sum) & 0xFFFF);

        // 5. 填入校验和 (位置必须在 16 字节包内，通常是最后两个字节)
        tmp_data[14] = final_checksum & 0xFF;         // Low Byte
        tmp_data[15] = (final_checksum >> 8) & 0xFF;  // High Byte

        // 6. 关键步骤：发送前清除包头
        tmp_data[0] = 0x00;
        tmp_data[1] = 0x00;

        std::memcpy(packet, tmp_data.data(), 16);    
    }

    ChipResult<> HimaxProtocol::write_and_verify(HalDevice* dev, const uint32_t addr, const uint8_t* data, uint32_t len, uint32_t verify_len) {
        if (!dev || !dev->IsValid()) return std::unexpected(ChipError::CommunicationError);

        std::vector<uint8_t> write_buf(data, data + len);
        std::vector<uint8_t> read_buf(len, 0);

        if (auto res = register_write(dev, addr, data, len); !res) {
            return res;
        }

        if (auto res = register_read(dev, addr, read_buf.data(), len); !res) {
            return res;
        }

        uint32_t cmp_len = verify_len;
        if (cmp_len == 0) {
            cmp_len = (len >= 2) ? 2u : len;
        }
        if (cmp_len > len) {
            cmp_len = len;
        }

        if (std::equal(write_buf.begin(), write_buf.begin() + cmp_len, read_buf.begin())) {
            return {};
        }

        return std::unexpected(ChipError::VerificationFailed);
    }

    /**
    * @brief 设置 Safe Mode 接口状态
    * 
    * @param dev 设备指针
    * @param state 目标状态
    *              - true: 进入 Safe Mode (写入 {0x27, 0x95} 到 0x31)
    *              - false: 退出 Safe Mode (写入 {0x00, 0x00} 到 0x31)
    * @return bool 操作是否成功
    */
    ChipResult<> HimaxProtocol::safeModeSetRaw(HalDevice* dev, const bool state) {
        if (!dev || !dev->IsValid()) return std::unexpected(ChipError::CommunicationError);
        uint8_t tmp_data[2]{};
        const uint8_t adr_i2c_psw_lb = 0x31;

        if (state == 1) {
            tmp_data[0] = 0x27;
            tmp_data[1] = 0x95;
            if (auto res = dev->WriteBus(adr_i2c_psw_lb, nullptr, tmp_data, 2); !res) return res;
            if (auto res = dev->ReadBus(adr_i2c_psw_lb, tmp_data, 2); !res) return res;

            if (tmp_data[0] == 0x27 && tmp_data[1] == 0x95) return {};
            return std::unexpected(ChipError::VerificationFailed);
        } else {
            if (auto res = dev->WriteBus(adr_i2c_psw_lb, nullptr, tmp_data, 2); !res) return res;
            if (auto res = dev->ReadBus(adr_i2c_psw_lb, tmp_data, 2); !res) return res;

            if (tmp_data[0] == 0x00 && tmp_data[1] == 0x00) return {};
            return std::unexpected(ChipError::VerificationFailed);
        }
    }

    ChipResult<> HimaxProtocol::send_command(HalDevice* dev, uint8_t cmd_id, uint8_t cmd_val, uint8_t& current_slot) {
        if (!dev || !dev->IsValid()) return std::unexpected(ChipError::CommunicationError);

        constexpr uint32_t BASE_ADDR = 0x10007550;
        const uint32_t addr = BASE_ADDR + static_cast<uint32_t>(current_slot) * 0x10;
        

        // 构建命令包
        std::array<uint8_t, 16> packet{};
        build_command_packet(cmd_id, cmd_val, packet.data());

        // 写入命令包到对应 slot 地址
        if (auto res = register_write(dev, addr, packet.data(), 16); !res) {
            return res;
        }

        // 写入触发标记 (0xA8, 0x8A, cmd_id, 0x00)
        std::array<uint8_t, 4> trigger = {0xA8, 0x8A, cmd_id, 0x00};
        if (auto res = register_write(dev, addr, trigger.data(), 4); !res) {
            return res;
        }

        // 读取确认
        std::array<uint8_t, 16> read_buf{};
        if (auto res = register_read(dev, addr, read_buf.data(), 16); !res) {
            return res;
        }

        // 更新 slot (循环使用 0-4)
        current_slot = (current_slot + 1) % 5;
        return {};
    }
}
