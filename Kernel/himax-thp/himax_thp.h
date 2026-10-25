/* SPDX-License-Identifier: (GPL-2.0 WITH Linux-syscall-note) OR MIT */
/*
 * himax_thp.h — himax-thp 字符设备的用户态 ABI，内核驱动与用户态守护进程共用。
 *
 * 驱动只提供总线原语和取帧，芯片的初始化、AFE 命令、读帧策略都在用户态。与 Windows 上 SPBTESTTOOL 驱动的对应：
 *   FULL_DUPLEX / WriteFile          -> HIMAX_THP_IOC_BUS_READ / HIMAX_THP_IOC_BUS_WRITE
 *   GET_FRAME + SET_TIMEOUT/SET_BLOCK -> read() + poll()（O_NONBLOCK 即非阻塞）
 *   INT_OPEN / INT_CLOSE             -> HIMAX_THP_IOC_INT_ENABLE
 *   SET_RESET                        -> HIMAX_THP_IOC_SET_RESET
 *   READ_ACPI（面板电源）             -> HIMAX_THP_IOC_PANEL_STATE，变化时 poll 报 POLLPRI
 *
 * 每个打开的文件绑定一个通道（默认 Master）。Master 与 Slave 是两颗芯片，各在一路 SPI 上；总线功能码
 * （Master F2 写 / F3 读，Slave F4 写 / F5 读）和 3 字节总线头由驱动按所绑通道填，用户态只给命令字节和数据。
 */
#ifndef _UAPI_HIMAX_THP_H
#define _UAPI_HIMAX_THP_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define HIMAX_THP_DEVICE_NAME		"himax_thp"

#define HIMAX_THP_CH_MASTER		0
#define HIMAX_THP_CH_SLAVE		1

/* 一次总线读写的数据上限（字节，不含总线头） */
#define HIMAX_THP_BUS_MAX		8192

/*
 * 一次总线操作。
 *   BUS_READ：发 [读功能码, cmd, 0x00]，把随后的 len 字节收进 buf（len >= 1）
 *   BUS_WRITE：发 [写功能码, cmd, buf 的 len 字节]（len 可以为 0）
 */
struct himax_thp_bus {
	__u64 buf;		/* 用户态地址 */
	__u32 len;
	__u8 cmd;
	__u8 reserved[3];	/* 置 0 */
};

/*
 * read() 每次取走所绑通道最早的一帧：先是这个帧头，紧跟 len 字节帧数据（不含总线头，从芯片的 5A A5 起）。
 * 缓冲区放不下整帧时返回 -EINVAL。每一路最多排 HIMAX_THP_FRAME_QUEUE 帧，满了丢掉新读到的帧（同 SpbThpTool），
 * 计入 dropped。
 */
struct himax_thp_frame_hdr {
	__u64 timestamp_ns;	/* CLOCK_MONOTONIC：这一帧的中断到达时刻 */
	__u32 seq;		/* 这一路读到的帧序号，每帧 +1 */
	__u32 len;		/* 帧数据字节数：Master 5060，Slave 336 */
	__u32 dropped;		/* 这一路累计因队列满丢掉的帧数 */
	__u32 reserved;
};

#define HIMAX_THP_FRAME_QUEUE		2

#define HIMAX_THP_IOC_MAGIC		'H'

/* 绑定本文件的通道（HIMAX_THP_CH_*） */
#define HIMAX_THP_IOC_SET_CHANNEL	_IOW(HIMAX_THP_IOC_MAGIC, 0x00, __u32)
/* 所绑通道上的总线读写，见 struct himax_thp_bus */
#define HIMAX_THP_IOC_BUS_READ		_IOW(HIMAX_THP_IOC_MAGIC, 0x01, struct himax_thp_bus)
#define HIMAX_THP_IOC_BUS_WRITE		_IOW(HIMAX_THP_IOC_MAGIC, 0x02, struct himax_thp_bus)
/*
 * 1：所绑通道开始在帧中断里读帧，并立刻读一帧（同 SpbThpTool 的 INT_OPEN，取走芯片已经备好的帧，这一路才会
 * 继续出 INT）；0：停止，并丢掉队列里还没取走的帧。面板断电时驱动把两路都关掉，上电后要重新打开
 */
#define HIMAX_THP_IOC_INT_ENABLE	_IOW(HIMAX_THP_IOC_MAGIC, 0x03, __u32)
/* 复位脚物理电平：1 拉高（释放复位），0 拉低（复位）。只改电平，脉冲宽度由调用方控制 */
#define HIMAX_THP_IOC_SET_RESET		_IOW(HIMAX_THP_IOC_MAGIC, 0x04, __u32)
/* 面板电源状态字（HIMAX_THP_PANEL_*）；读过之后本文件的 POLLPRI 清掉 */
#define HIMAX_THP_IOC_PANEL_STATE	_IOR(HIMAX_THP_IOC_MAGIC, 0x05, __u32)

/* 面板已上电（prepared）：TDDI 的触摸部分随之上电 */
#define HIMAX_THP_PANEL_ON		(1u << 0)
/* 面板已点亮（enabled）：显示链路已经开始出图 */
#define HIMAX_THP_PANEL_ENABLED		(1u << 1)

#endif /* _UAPI_HIMAX_THP_H */
