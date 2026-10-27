/* SPDX-License-Identifier: (GPL-2.0 WITH Linux-syscall-note) OR MIT */
/*
 * himax_thp.h - userspace ABI of the himax-thp character device
 *
 * The driver only provides bus primitives and frame capture. Chip
 * initialization, AFE commands and the frame reading policy live in the
 * userspace touch daemon. Mapping to the Windows SPBTESTTOOL driver:
 *
 *   FULL_DUPLEX / WriteFile           -> HIMAX_THP_IOC_BUS_READ / BUS_WRITE
 *   GET_FRAME + SET_TIMEOUT/SET_BLOCK -> read() + poll(), O_NONBLOCK
 *   INT_OPEN / INT_CLOSE              -> HIMAX_THP_IOC_INT_ENABLE
 *   SET_RESET low, wait, SET_RESET high -> HIMAX_THP_IOC_RESET
 *   READ_ACPI (panel power)           -> HIMAX_THP_IOC_PANEL_STATE, POLLPRI
 *
 * Master and slave are two controllers on separate SPI buses. A file binds
 * one of them with SET_CHANNEL before it transfers, enables or reads frames;
 * a channel is bound to at most one file at a time. The driver fills in the
 * bus opcode (master F2 write / F3 read, slave F4 write / F5 read) and the
 * bus header for the bound channel, userspace only passes the command byte
 * and the data. Closing the file stops frame reading on its channel.
 */
#ifndef _UAPI_LINUX_HIMAX_THP_H
#define _UAPI_LINUX_HIMAX_THP_H

#include <linux/const.h>
#include <linux/ioctl.h>
#include <linux/types.h>

#define HIMAX_THP_DEVICE_NAME		"himax_thp"

#define HIMAX_THP_CH_MASTER		0
#define HIMAX_THP_CH_SLAVE		1

/* Data limit of one bus transfer, in bytes, not counting the bus header */
#define HIMAX_THP_BUS_MAX		8192

/*
 * One bus transfer.
 *   BUS_READ:  sends [read opcode, cmd, 0x00] and receives len bytes into
 *              buf (len >= 1)
 *   BUS_WRITE: sends [write opcode, cmd, len bytes of buf] (len may be 0)
 */
struct himax_thp_bus {
	__u64 buf;		/* userspace address */
	__u32 len;
	__u8 cmd;
	__u8 reserved[3];	/* must be zero */
};

/*
 * Each read() returns the oldest frame of the bound channel: this header
 * followed by len bytes of frame data, without the bus header (starting at
 * the controller's 5A A5). A buffer too small for the whole frame gets
 * -EINVAL. Each channel queues up to HIMAX_THP_FRAME_QUEUE frames; when the
 * queue is full a newly read frame is dropped and counted in dropped, as
 * SPBTESTTOOL does.
 */
struct himax_thp_frame_hdr {
	__u64 timestamp_ns;	/* CLOCK_MONOTONIC time of the frame interrupt */
	__u32 seq;		/* frame sequence number of this channel */
	__u32 len;		/* frame data bytes: master 5060, slave 336 */
	__u32 dropped;		/* frames dropped on this channel so far */
	__u32 reserved;
};

#define HIMAX_THP_FRAME_QUEUE		2

#define HIMAX_THP_IOC_MAGIC		0xB9

/*
 * Bind this file to a channel (HIMAX_THP_CH_*), releasing the one it had.
 * -EBUSY: another file has it. -ENODEV: no such controller (a slave without
 * a cascade in DT). Unbound files get -EINVAL from channel operations.
 */
#define HIMAX_THP_IOC_SET_CHANNEL	_IOW(HIMAX_THP_IOC_MAGIC, 0x00, __u32)
/* Bus transfer on the bound channel, see struct himax_thp_bus */
#define HIMAX_THP_IOC_BUS_READ		_IOW(HIMAX_THP_IOC_MAGIC, 0x01, struct himax_thp_bus)
#define HIMAX_THP_IOC_BUS_WRITE		_IOW(HIMAX_THP_IOC_MAGIC, 0x02, struct himax_thp_bus)
/*
 * 1: start reading frames on the bound channel from its frame interrupt, and
 * read one frame right away (like SPBTESTTOOL's INT_OPEN; this collects a
 * frame the controller already holds, without which the channel raises no
 * further interrupt). 0: stop and drop the queued frames. The driver stops
 * both channels when the panel powers off; reopen them after power up.
 */
#define HIMAX_THP_IOC_INT_ENABLE	_IOW(HIMAX_THP_IOC_MAGIC, 0x03, __u32)
/* Panel power state word (HIMAX_THP_PANEL_*); reading it clears POLLPRI for this file */
#define HIMAX_THP_IOC_PANEL_STATE	_IOR(HIMAX_THP_IOC_MAGIC, 0x05, __u32)
/*
 * Reset both controllers: hold the shared reset line asserted for the given
 * milliseconds (at least HIMAX_THP_RESET_MIN_MS, at most HIMAX_THP_RESET_MAX_MS)
 * and release it. Returns once the line is released; waiting for the firmware
 * to reload is up to the caller. No transfer runs during the pulse.
 */
#define HIMAX_THP_IOC_RESET		_IOW(HIMAX_THP_IOC_MAGIC, 0x06, __u32)

#define HIMAX_THP_RESET_MIN_MS		5
#define HIMAX_THP_RESET_MAX_MS		100

/* Panel prepared: the touch part of the TDDI is powered */
#define HIMAX_THP_PANEL_ON		_BITUL(0)
/* Panel enabled: the display pipeline is scanning out */
#define HIMAX_THP_PANEL_ENABLED		_BITUL(1)

#endif /* _UAPI_LINUX_HIMAX_THP_H */
