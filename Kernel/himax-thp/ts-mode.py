#!/usr/bin/env python3
# 设置 TDDI 的主机接口选择脚 gpio174（开发用，需要 root）。芯片在 FW reload（gpio99 复位后自动触发）时采样它：
#   低 = SPI 模式（THP 原始数据，himax-spi / himax-thp 用的）  高 = I2C-HID 模式（UEFI 用的）
# UEFI 把它留在高电平，DT 也没配，所以开机后芯片在 I2C-HID 模式：SPI 寄存器能读写，但从不出帧、不拉 INT。
# 只需要在下一次复位前是对的；设完即释放，TLMM 会保持输出电平。
#   ts-mode.py spi | i2c
import fcntl, glob, struct, sys

TS_MODE_PIN = 174
TLMM_LABEL = b'f100000.pinctrl'

GPIO_GET_CHIPINFO_IOCTL = 0x8044B401          # _IOR(0xB4, 0x01, struct gpiochip_info)，68 字节
GPIO_V2_GET_LINE_IOCTL = 0xC250B407           # _IOWR(0xB4, 0x07, struct gpio_v2_line_request)，592 字节
GPIO_V2_LINE_FLAG_OUTPUT = 1 << 3
GPIO_V2_LINE_ATTR_ID_OUTPUT_VALUES = 2


def tlmm_chip():
    for path in sorted(glob.glob('/dev/gpiochip*')):
        with open(path, 'rb') as f:
            info = fcntl.ioctl(f, GPIO_GET_CHIPINFO_IOCTL, bytes(68))
        if info[32:64].rstrip(b'\0') == TLMM_LABEL:
            return path
    sys.exit('TLMM gpiochip not found')


def set_pin(level):
    # struct gpio_v2_line_request：offsets[64]、consumer[32]、config（flags、num_attrs、padding[5]、attrs[10]）、
    # num_lines、event_buffer_size、padding[5]、fd
    offsets = struct.pack('<64I', TS_MODE_PIN, *([0] * 63))
    consumer = b'ts-mode'.ljust(32, b'\0')
    attr = struct.pack('<IIQQ', GPIO_V2_LINE_ATTR_ID_OUTPUT_VALUES, 0, level & 1, 1)
    config = struct.pack('<QI5I', GPIO_V2_LINE_FLAG_OUTPUT, 1, *([0] * 5)) + attr + bytes(24 * 9)
    req = bytearray(offsets + consumer + config + struct.pack('<II5Ii', 1, 0, *([0] * 5), 0))
    assert len(req) == 592
    with open(tlmm_chip(), 'rb') as f:
        fcntl.ioctl(f, GPIO_V2_GET_LINE_IOCTL, req)
    fd = struct.unpack_from('<i', req, 588)[0]
    import os
    os.close(fd)


if __name__ == '__main__':
    mode = sys.argv[1] if len(sys.argv) > 1 else ''
    if mode not in ('spi', 'i2c'):
        sys.exit('usage: ts-mode.py spi | i2c')
    set_pin(0 if mode == 'spi' else 1)
    print(f'gpio{TS_MODE_PIN} -> {"low (SPI)" if mode == "spi" else "high (I2C-HID)"}; takes effect at the next TS reset / FW reload')
