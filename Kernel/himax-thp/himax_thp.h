/* SPDX-License-Identifier: (GPL-2.0 WITH Linux-syscall-note) OR MIT */
/*
 * himax_thp.h — himax-thp 字符设备的用户态 ABI，内核驱动与用户态守护进程共用。
 *
 * 对应 Windows 上 SPBTESTTOOL 驱动的 IOCTL（见 Port/win32/TouchBusWin32.cpp）：
 *   FULL_DUPLEX / WriteFile          -> HIMAX_THP_IOC_XFER
 *   GET_FRAME + SET_TIMEOUT/SET_BLOCK -> read() + poll()（O_NONBLOCK 即非阻塞）
 *   INT_OPEN / INT_CLOSE             -> HIMAX_THP_IOC_INT_ENABLE
 *   SET_RESET                        -> HIMAX_THP_IOC_SET_RESET
 *   READ_ACPI（面板电源）             -> HIMAX_THP_IOC_PANEL_STATE，变化时 poll 报 POLLPRI
 *
 * 每个打开的文件绑定一个通道（默认 Master）。Master 与 Slave 在同一根片选上，靠每次传输首字节的
 * 功能码区分：Master F2 写 / F3 读，Slave F4 写 / F5 读；XFER 的首字节必须是所绑通道的功能码。
 */
#ifndef _UAPI_HIMAX_THP_H
#define _UAPI_HIMAX_THP_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define HIMAX_THP_DEVICE_NAME		"himax_thp"

#define HIMAX_THP_CH_MASTER		0
#define HIMAX_THP_CH_SLAVE		1

/* 单次 XFER 与单帧的上限（字节）；Master 帧 5063、Slave 帧 339 */
#define HIMAX_THP_MAX_XFER		8192

struct himax_thp_xfer {
	__u64 tx_buf;		/* 用户态地址，len 字节，首字节是功能码 */
	__u64 rx_buf;		/* 用户态地址，len 字节；0 表示只写（对应 WriteFile） */
	__u32 len;
	__u32 reserved;		/* 置 0 */
};

#define HIMAX_THP_IOC_MAGIC		'H'

/* 绑定本文件的通道（HIMAX_THP_CH_*） */
#define HIMAX_THP_IOC_SET_CHANNEL	_IOW(HIMAX_THP_IOC_MAGIC, 0x00, __u32)
/* 设置所绑通道每帧读多少字节（含 3 字节总线头），中断里按这个长度读帧 */
#define HIMAX_THP_IOC_SET_FRAME_LEN	_IOW(HIMAX_THP_IOC_MAGIC, 0x01, __u32)
/* 一次全双工传输 */
#define HIMAX_THP_IOC_XFER		_IOW(HIMAX_THP_IOC_MAGIC, 0x02, struct himax_thp_xfer)
/*
 * 1：所绑通道开始在帧中断里读帧，并立刻读一帧（同 SpbThpTool 的 INT_OPEN，取走芯片已经备好的帧，这一路才会
 * 继续出 INT）；0：停止。面板断电时驱动把两路都关掉，上电后要重新打开
 */
#define HIMAX_THP_IOC_INT_ENABLE	_IOW(HIMAX_THP_IOC_MAGIC, 0x03, __u32)
/* 复位脚物理电平：1 拉高（释放复位），0 拉低（复位） */
#define HIMAX_THP_IOC_SET_RESET		_IOW(HIMAX_THP_IOC_MAGIC, 0x04, __u32)
/* 面板电源状态字，bit0 = 屏幕已上电；读过之后本文件的 POLLPRI 清掉 */
#define HIMAX_THP_IOC_PANEL_STATE	_IOR(HIMAX_THP_IOC_MAGIC, 0x05, __u32)

#define HIMAX_THP_PANEL_ON		(1u << 0)

#endif /* _UAPI_HIMAX_THP_H */
