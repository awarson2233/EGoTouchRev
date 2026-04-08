/*
 * @Author: Detach0-0 detach0-0@outlook.com
 * @Date: 2025-12-01 19:25:55
 * @LastEditors: Detach0-0 detach0-0@outlook.com
 * @LastEditTime: 2025-12-29 13:13:56
 * @FilePath: \EGoTouchRev-vsc\HimaxChipCore\source\HimaxHal.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "himax/HimaxProtocol.h"
#include "Device.h"
#include <cstddef>
#include <cstdint>
#include <array>
#include <errhandlingapi.h>
#include <handleapi.h>
#include <ioapiset.h>
#include <minwindef.h>
#include <synchapi.h>
#include <windows.h>
#include <winscard.h>


constexpr uint8_t OP_WRITE_MASTER = 0xF2;
constexpr uint8_t OP_READ_MASTER  = 0xF3;
constexpr uint8_t OP_WRITE_SLAVE  = 0xF4;
constexpr uint8_t OP_READ_SLAVE   = 0xF5;

const DWORD SPI_IOCTL_INT_OPEN    = 0x4001c00; // 打开中断/初始化
const DWORD SPI_IOCTL_INT_CLOSE   = 0x4001c04; // 关闭中断
const DWORD SPI_IOCTL_WRITEREAD   = 0x4001c10; // [不常用] 偶见于特定初始化流
const DWORD SPI_IOCTL_WAIT_INT    = 0x4001c20; // 等待中断触发
const DWORD SPI_IOCTL_FULL_DUPLEX = 0x4001c24; // [核心] BusRead / 全双工读
const DWORD SPI_IOCTL_GET_FRAME   = 0x4001c28; // 获取帧数据
const DWORD SPI_IOCTL_SET_TIMEOUT = 0x4001c2c; // 设置超时
const DWORD SPI_IOCTL_SET_BLOCK   = 0x4001c30; // 设置阻塞模式
const DWORD SPI_IOCTL_SET_RESET   = 0x4001c34; // 复位设备
const DWORD SPI_IOCTL_READ_ACPI   = 0x4001c38; // 读取 ACPI 配置

namespace {

    /**
     * @brief 执行同步 I/O 操作（封装 Overlapped 异步操作为同步）
     * @param handle 文件句柄
     * @param mutex 互斥锁
     * @param lastError 错误码输出
     * @param ioFunc I/O 函数 lambda
     * @param expectedLen 预期长度
     * @param checkLen 是否检查长度
     * @param outBytes 实际传输字节数输出
     * @return bool 是否成功
     */
    template<typename Func>
    ChipResult<> PerformSyncIo(HANDLE handle, DWORD& lastError, Func ioFunc, ChipError errType, DWORD expectedLen = 0, bool checkLen = false, DWORD* outBytes = NULL) {
        if (handle == INVALID_HANDLE_VALUE) {
            lastError = ERROR_INVALID_HANDLE;
            return std::unexpected(ChipError::CommunicationError);
        }
        OVERLAPPED ov = {0};
        ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!ov.hEvent) {
            lastError = GetLastError();
            return std::unexpected(ChipError::InternalError);
        }

        DWORD bytesTransferred = 0;
        BOOL res = ioFunc(&ov, &bytesTransferred);

        if (!res) {
            DWORD err = GetLastError();
            if (err == ERROR_IO_PENDING) {
                if (WaitForSingleObject(ov.hEvent, INFINITE) == WAIT_OBJECT_0) {
                    res = GetOverlappedResult(handle, &ov, &bytesTransferred, TRUE);
                    if (!res) lastError = GetLastError();
                } else {
                    lastError = GetLastError();
                }
            } else {
                lastError = err;
            }
        }
        CloseHandle(ov.hEvent);

        if (!res) return std::unexpected(errType);
        if (checkLen && bytesTransferred != expectedLen) {
            lastError = ERROR_WRITE_FAULT;
            return std::unexpected(ChipError::CommunicationError);
        }
        
        if (outBytes) *outBytes = bytesTransferred;
        lastError = 0;
        return {};
    }
}

namespace Himax {

    /**
     * @brief 构造函数，打开设备句柄并初始化
     * @param path 设备路径
     * @param type 设备类型 (Master/Slave/Interrupt)
     */
    HalDevice::HalDevice(const wchar_t* path, DeviceType type) : m_handle(INVALID_HANDLE_VALUE), m_lastError(0), m_type(type) {
        m_xfer_buffer.reserve(0x4000 + 32);

        m_handle = CreateFileW(
            path, 
            GENERIC_ALL, 
            FILE_SHARE_READ | FILE_SHARE_WRITE, 
            NULL, 
            OPEN_EXISTING, 
            FILE_FLAG_OVERLAPPED, 
            NULL
        );
        if (m_handle == INVALID_HANDLE_VALUE) {
            m_lastError = GetLastError();
        }

        m_ov.hEvent = CreateEvent(nullptr, true, false, nullptr);
        if (m_ov.hEvent == nullptr) {
            m_lastError = GetLastError();
            if (m_handle != INVALID_HANDLE_VALUE) {
                CloseHandle(m_handle);
                m_handle = INVALID_HANDLE_VALUE;
            }
            return;
        }
        if (type == DeviceType::Slave) {
            m_readOp = OP_READ_SLAVE;
            m_writeOp = OP_WRITE_SLAVE;
        } else {
            m_readOp = OP_READ_MASTER;
            m_writeOp = OP_WRITE_MASTER;
        }
    }

    /**
     * @brief 析构函数，关闭句柄
     */
    HalDevice::~HalDevice() {
        if (m_ov.hEvent) {
            CloseHandle(m_ov.hEvent);
            m_ov.hEvent = nullptr;
        }
        
        if (IsValid()) {
            CloseHandle(m_handle);
            m_handle = INVALID_HANDLE_VALUE;
        }
    }

    /**
     * @brief 检查设备句柄是否有效
     * @return bool 是否有效
     */
    bool HalDevice::IsValid() const {return m_handle != INVALID_HANDLE_VALUE;}

    /**
     * @brief 执行 DeviceIoControl 操作
     * @param code 控制码
     * @param in 输入缓冲区
     * @param inLen 输入长度
     * @param out 输出缓冲区
     * @param outLen 输出长度
     * @param retLen 实际返回长度
     * @return bool 是否成功
     */
    ChipResult<> HalDevice::Ioctl(DWORD code, const void* in, uint32_t inLen, void* out, uint32_t outLen, uint32_t* retLen) {
        ChipResult<> result = std::unexpected(ChipError::InternalError);
        for (int cnt = 0; cnt < 10; cnt++) {
            result = PerformSyncIo(m_handle, m_lastError, [&](OVERLAPPED* pov, LPDWORD pBytes) {
                return DeviceIoControl(m_handle, code, (LPVOID)in, inLen, out, outLen, pBytes, pov);
            }, ChipError::CommunicationError, 0, false, (DWORD*)retLen);
            if (result) break;
            // If the I/O was cancelled via CancelIoEx, do not retry.
            if (m_lastError == ERROR_OPERATION_ABORTED) break;
        }

        return result;
    }

    /**
     * @brief 从设备读取原始数据
     * @param buffer 缓冲区
     * @param len 读取长度
     * @return bool 是否成功
     */
    ChipResult<> HalDevice::Read(void* buffer, uint32_t len) {
        return PerformSyncIo(m_handle, m_lastError, [&](OVERLAPPED* pov, LPDWORD pBytes){
            return ReadFile(m_handle, buffer, len, pBytes, pov);
        }, ChipError::CommunicationError);
    }

    /**
     * @brief 等待设备中断触发
     * @return bool 是否成功触发
     */
    ChipResult<> HalDevice::WaitInterrupt() {
        if (!IsValid() || !m_ov.hEvent) { 
            m_lastError = ERROR_INVALID_HANDLE;
            return std::unexpected(ChipError::CommunicationError); 
        }

        ResetEvent(m_ov.hEvent);
        
        DWORD bytesReturned = 0;
        BOOL res = DeviceIoControl(
            m_handle,
            SPI_IOCTL_WAIT_INT,
            nullptr, 0,
            nullptr, 0,
            &bytesReturned, &m_ov
        );

        if (!res && GetLastError() != ERROR_IO_PENDING) {
            m_lastError = GetLastError();
            return std::unexpected(ChipError::CommunicationError);
        }

        DWORD waitResult = WaitForSingleObject(m_ov.hEvent, 200);

        if (waitResult != WAIT_OBJECT_0) {
            m_lastError = (waitResult == WAIT_FAILED) ? GetLastError() : ERROR_GEN_FAILURE;
            return std::unexpected(ChipError::Timeout);
        }

        BOOL ok = GetOverlappedResult(m_handle, &m_ov, &bytesReturned, FALSE);
        if (!ok) {
            m_lastError = GetLastError();
            return std::unexpected(ChipError::CommunicationError);
        }

        ResetEvent(m_ov.hEvent);
        m_lastError = 0;
        return {};
    }


    /**
     * @brief 通过总线读取数据
     * @param cmd 命令码
     * @param data 接收缓冲区
     * @param len 读取长度
     * @return bool 是否成功
     */
    ChipResult<> HalDevice::ReadBus(uint8_t cmd, uint8_t* data, uint32_t len) {
        size_t total_size = data_offset + len;

        m_xfer_buffer.clear();

        m_xfer_buffer.push_back(m_readOp);
        m_xfer_buffer.push_back(cmd);
        m_xfer_buffer.push_back(0x00);      //Dummy Byte
        
        m_xfer_buffer.resize(total_size, 0);

        uint32_t retLen = 0;
        auto res = Ioctl(SPI_IOCTL_FULL_DUPLEX, 
                            m_xfer_buffer.data(), m_xfer_buffer.size(), 
                            m_xfer_buffer.data(), m_xfer_buffer.size(), 
                            &retLen);
        
        if (!res) return std::unexpected(ChipError::CommunicationError);

        if (retLen < total_size) {
            return std::unexpected(ChipError::CommunicationError);
        }

        memcpy(data, m_xfer_buffer.data() + data_offset, len);
        
        return {};
    }

    /**
     * @brief 通过总线写入数据
     * @param cmd 命令码
     * @param addr 地址 (可选)
     * @param data 数据缓冲区
     * @param len 数据长度
     * @return bool 是否成功
     */
    ChipResult<> HalDevice::WriteBus(const uint8_t cmd, const uint8_t* addr, const uint8_t* data, const uint32_t len) {
        m_xfer_buffer.clear();

        m_xfer_buffer.clear();
        m_xfer_buffer.push_back(m_writeOp);
        m_xfer_buffer.push_back(cmd);

        if (addr != NULL) {
            m_xfer_buffer.insert(m_xfer_buffer.end(), addr, addr + 4);
        }
        if (data != NULL) {
            m_xfer_buffer.insert(m_xfer_buffer.end(), data, data + len);
        }


        return PerformSyncIo(m_handle, m_lastError, [&](OVERLAPPED* pov, LPDWORD pBytes){
            return WriteFile(m_handle, m_xfer_buffer.data(), (DWORD)m_xfer_buffer.size(), NULL, pov);
        }, ChipError::CommunicationError);
    }

    /**
     * @brief 读取 ACPI 配置数据
     * @param data 接收缓冲区
     * @param len 读取长度
     * @return bool 是否成功
     */
    ChipResult<> HalDevice::ReadAcpi(uint8_t* data, uint32_t len) {
        m_xfer_buffer.assign(len, 0);
        
        uint32_t retLen = 0;

        auto res = Ioctl(SPI_IOCTL_READ_ACPI, nullptr, 0, m_xfer_buffer.data(), len, &retLen);
        if (!res) return std::unexpected(ChipError::CommunicationError);

        if (retLen < len) {
            return std::unexpected(ChipError::CommunicationError);
        }
        
        memcpy(data, m_xfer_buffer.data(), len);
        return {};
    }

    /**
     * @brief 获取一帧完整的触摸数据
     * @param buffer 接收缓冲区
     * @param outLen 缓冲区长度
     * @param retLen 实际返回长度
     * @return bool 是否成功
     */
    ChipResult<> HalDevice::GetFrame(void* buffer, uint32_t outLen, uint32_t *retLen){
        return Ioctl(SPI_IOCTL_GET_FRAME, NULL, 0, buffer, outLen, retLen);
    }

    /**
     * @brief 设置 I/O 超时时间
     * @param milisecond 毫秒数
     * @return bool 是否成功
     */
    ChipResult<> HalDevice::SetTimeOut(uint32_t milisecond) {
        return Ioctl(SPI_IOCTL_SET_TIMEOUT, &milisecond, sizeof(milisecond), NULL, 0, NULL);
    }

    /**
     * @brief 设置阻塞或非阻塞模式
     * @param state true 为阻塞, false 为非阻塞
     * @return bool 是否成功
     */
    ChipResult<> HalDevice::SetBlock(bool state) {
        m_xfer_buffer.clear();

        m_xfer_buffer.clear();
        m_xfer_buffer.push_back(uint8_t(state));
        m_xfer_buffer.resize(4, 0);

        return Ioctl(SPI_IOCTL_SET_BLOCK, m_xfer_buffer.data(), 4, NULL, 0, NULL);          
    }

    /**
     * @brief 设置设备复位状态
     * @param state true 为拉高复位, false 为拉低复位
     * @return bool 是否成功
     */
    ChipResult<> HalDevice::SetReset(bool state) {
        uint32_t val = state ? 1 : 0;
        return Ioctl(SPI_IOCTL_SET_RESET, &val, sizeof(val), NULL, 0, NULL);  
    }

    /**
     * @brief 打开中断监听
     * @return bool 是否成功
     */
    ChipResult<> HalDevice::IntOpen(void) {
        return Ioctl(SPI_IOCTL_INT_OPEN, NULL, 0, NULL, 0, NULL);
    }

    /**
     * @brief 关闭中断监听
     * @return bool 是否成功
     */
    ChipResult<> HalDevice::IntClose(void) {
        return Ioctl(SPI_IOCTL_INT_CLOSE, NULL, 0, NULL, 0, NULL);
    }

    /**
     * @brief 获取最后一次发生的错误码
     * @return DWORD Windows 错误码
     */
    DWORD HalDevice::GetError() {
        return m_lastError;
    }

    bool HalDevice::IsTimeoutError() const {
        return m_lastError == WAIT_TIMEOUT ||
               m_lastError == ERROR_TIMEOUT ||
               m_lastError == ERROR_SEM_TIMEOUT;
    }

    void HalDevice::CancelPendingIo() {
        if (m_handle != INVALID_HANDLE_VALUE) {
            CancelIoEx(m_handle, NULL);
        }
    }

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

    /**
    * @brief 将地址值按指定长度解析为小端序字节数组
    * @param addr 地址值
    * @param cmd 目标缓冲区
    * @param len 目标长度 (1, 2, 或 4)
    */
    void himax_parse_assign_cmd(uint32_t addr, uint8_t *cmd, int len) {
        switch (len) {
        case 1:
            cmd[0] = static_cast<uint8_t>(addr & 0xFF);
            break;
        case 2:
            cmd[0] = static_cast<uint8_t>(addr & 0xFF);
            cmd[1] = static_cast<uint8_t>((addr >> 8) & 0xFF);
            break;
        case 4:
            cmd[0] = static_cast<uint8_t>(addr & 0xFF);
            cmd[1] = static_cast<uint8_t>((addr >> 8) & 0xFF);
            cmd[2] = static_cast<uint8_t>((addr >> 16) & 0xFF);
            cmd[3] = static_cast<uint8_t>((addr >> 24) & 0xFF);
            break;
        default:
            break;
        }
    }

    ChipResult<> HimaxProtocol::register_read(HalDevice *dev, const uint32_t addr, uint8_t *buffer, uint32_t len) {
        if (!dev || !dev->IsValid()) return std::unexpected(ChipError::CommunicationError);

        uint8_t tmp_data[4];
        himax_parse_assign_cmd(addr, tmp_data, 4);

        if (len > 256) {
            return std::unexpected(ChipError::InvalidOperation);
        }
        if (len > 4) {
            if (auto res = burst_enable(dev, true); !res) return res;
        } else {
            if (auto res = burst_enable(dev, false); !res) return res;
        }

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
        himax_parse_assign_cmd(addr, tmp_data, 4);

        if (len > 4) {
            if (auto res = burst_enable(dev, 1); !res) return res;
        } else {
            if (auto res = burst_enable(dev, 0); !res) return res;
        }
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
