// SPDX-License-Identifier: GPL-2.0
/*
 * himax-thp — Himax HX83121A 的 SPI 传输驱动（Linux 版的 SPBTESTTOOL）。
 *
 * 只做硬件相关的事：总线原语、帧中断、复位脚、屏幕电源联动，不解算。芯片协议、初始化时序、读帧策略与触点解算
 * 都在用户态（EGoTouchService 的 HimaxCore + TsaCore），用户态经 /dev/himax_thp 访问：
 *
 *   ioctl BUS_READ / BUS_WRITE   所绑通道上的一次总线读写（功能码与总线头由驱动填）
 *   read / poll                  帧中断里读好、排在队列里的帧（带时间戳与序号）
 *   ioctl 其余                   通道绑定、读帧开关、复位脚、面板电源状态
 *
 * Master 与 Slave 是两颗芯片，各在一路 SPI 上、各有一根 INT（本机 ACPI 的 \_SB.THPA / THPB）：Master 是
 * spi6 上的 himax,hx83121a-ts，Slave 是 spi20 上的 himax,hx83121a-ts-slave，Master 的 himax,cascade
 * 指向 Slave。两者都由本驱动绑定，Master probe 时把 Slave 接成第二路（device link 保证 Slave 解绑前 Master
 * 先解绑），对用户态仍是一个 /dev/himax_thp。复位脚、mode 脚与面板联动只在 Master 上。DT 见
 * bindings/himax,hx83121a-ts.yaml。
 *
 * 帧在各路自己的中断线程里读：打开了读帧的那一路发 [读功能码, 0x30, 0x00] 后连读整帧，去掉 3 字节总线头后
 * 放进该路的帧队列，用户态 read 取走。中断触发方式与 SPI 模式都按 DT。
 *
 * 芯片要在 SPI 模式下才按帧拉 INT：它在复位后的 FW reload 时采样 mode 脚（本机 gpio174），低 = SPI、
 * 高 = I2C-HID。本驱动 probe 时把它拉低，之后的复位都由用户态做。
 *
 * ABI 见 himax_thp.h。
 */

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio/driver.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kref.h>
#include <linux/ktime.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

#include <drm/drm_panel.h>

#include "himax_thp.h"

#define HIMAX_THP_NUM_CH		2

/* SPI 功能码：每次传输的首字节 */
#define HIMAX_THP_OP_WRITE_MASTER	0xf2
#define HIMAX_THP_OP_READ_MASTER	0xf3
#define HIMAX_THP_OP_WRITE_SLAVE	0xf4
#define HIMAX_THP_OP_READ_SLAVE		0xf5

/* 总线头：读是 [功能码, cmd, dummy]，写是 [功能码, cmd] */
#define HIMAX_THP_READ_HDR		3
#define HIMAX_THP_WRITE_HDR		2

/*
 * 读帧的总线命令：事件栈寄存器。Windows 帧的 7 字节头 = 3 字节总线头 + 4 字节数据头，与 linux-gaokun
 * 驱动读事件栈时 heatmap 落在数据第 4 字节一致。
 */
#define HIMAX_THP_CMD_EVENT_STACK	0x30

/* 每帧的数据长度（不含总线头），与 Common/include/FrameLayout.h 的 kMasterFrameSize / kSlaveFrameSize 减 3 一致 */
#define HIMAX_THP_MASTER_FRAME_LEN	5060
#define HIMAX_THP_SLAVE_FRAME_LEN	336

/*
 * 芯片在帧准备好时拉低 INT，主机读完帧才放开，放开要一点时间。Master 的帧中断按 DT 是低电平触发：中断线程读完
 * 立即返回的话 INT 还低着，马上又触发一次，读到的要么是空的事件栈（全零），要么是芯片正在写的下一帧（错位）。
 * 所以读完帧后等 INT 回到高电平再返回，最多等这么久（帧间隔约 8 ms）。Slave 按 overlay 也是低电平触发。
 */
#define HIMAX_THP_INT_RELEASE_US	2000
#define HIMAX_THP_INT_POLL_US		50

struct himax_thp;

struct himax_thp_frame {
	struct himax_thp_frame_hdr hdr;
	u8 *data;			/* frame_len 字节，指向所属通道的 qbuf */
};

struct himax_thp_chan {
	struct himax_thp *ts;
	struct spi_device *spi;		/* 这一路的 SPI 设备；没有级联 Slave 时 Slave 这一路为 NULL，probe 后不变 */
	int irq;			/* 这一路的帧中断，spi 为 NULL 时无效 */
	bool irq_enabled;
	u64 irq_ns;			/* 硬中断里记下的中断时刻，中断线程读 */
	struct gpio_desc *int_gpio;	/* 帧中断那根脚，只读电平；找不到时为 NULL */
	struct gpio_device *int_gdev;	/* int_gpio 所在的 GPIO 控制器，持着引用 */
	u8 op_write;
	u8 op_read;
	u32 frame_len;			/* 每帧的数据长度，不含总线头 */
	bool int_enabled;		/* 用户态打开了这一路的读帧 */

	/* 帧队列：从 head 起的 count 帧，满了丢掉新读到的帧（同 SpbThpTool）。持 state_lock 访问 */
	u8 *qbuf;
	struct himax_thp_frame queue[HIMAX_THP_FRAME_QUEUE];
	unsigned int head;
	unsigned int count;
	u32 seq;			/* 下一帧的序号 */
	u32 dropped;			/* 累计丢掉的帧数 */
};

struct himax_thp {
	struct spi_device *spi;		/* Master 的 SPI 设备，复位脚、面板联动、misc 都挂在它上面 */
	struct gpio_desc *reset;
	struct miscdevice misc;
	struct drm_panel_follower follower;
	bool has_follower;
	struct kref ref;		/* probe 持有一份，每个打开的文件各一份 */

	/* 总线：SPI 传输与收发缓冲。先拿 bus_lock 再拿 state_lock，反过来不行 */
	struct mutex bus_lock;
	u8 *tx;
	u8 *rx;

	/* 状态：帧队列、开关、面板电源 */
	struct mutex state_lock;
	wait_queue_head_t wait;
	struct himax_thp_chan ch[HIMAX_THP_NUM_CH];
	u32 panel_state;		/* HIMAX_THP_PANEL_* */
	u64 panel_seq;			/* 面板状态每变化一次 +1 */
	bool gone;			/* 设备已解绑，所有文件操作返回 -ENODEV */
};

struct himax_thp_file {
	struct himax_thp *ts;
	u32 channel;
	u64 seen_panel_seq;		/* 本文件最后读过的面板状态序号 */
	u8 *bounce;			/* read 时帧队列到用户态的中转：帧头 + 帧数据 */
};

static void himax_thp_release_ts(struct kref *ref)
{
	struct himax_thp *ts = container_of(ref, struct himax_thp, ref);
	int i;

	for (i = 0; i < HIMAX_THP_NUM_CH; i++) {
		kfree(ts->ch[i].qbuf);
		if (ts->ch[i].int_gdev)
			gpio_device_put(ts->ch[i].int_gdev);
	}
	/* Slave 这一路持着 himax_thp_get_slave 拿到的引用；Master 这一路借用 ts->spi 的 */
	if (ts->ch[HIMAX_THP_CH_SLAVE].spi)
		spi_dev_put(ts->ch[HIMAX_THP_CH_SLAVE].spi);
	kfree(ts->tx);
	kfree(ts->rx);
	spi_dev_put(ts->spi);
	kfree(ts);
}

static bool himax_thp_panel_on(struct himax_thp *ts)
{
	return ts->panel_state & HIMAX_THP_PANEL_ON;
}

/* 每一路的帧中断开关跟着「面板上电、这一路接上了且在读帧」走。持 state_lock 调用 */
static void himax_thp_update_irq_locked(struct himax_thp *ts)
{
	int i;

	for (i = 0; i < HIMAX_THP_NUM_CH; i++) {
		struct himax_thp_chan *ch = &ts->ch[i];
		bool want = himax_thp_panel_on(ts) && !ts->gone && ch->spi && ch->int_enabled;

		if (want == ch->irq_enabled)
			continue;
		if (want)
			enable_irq(ch->irq);
		else
			/* 中断线程自己也要拿 state_lock，这里不能等它跑完 */
			disable_irq_nosync(ch->irq);
		ch->irq_enabled = want;
	}
}

/* -------------------------------------------------------------------------- */
/* 总线原语 */

/* 总线读：发 [读功能码, cmd, 0x00]，数据落在 ts->rx + HIMAX_THP_READ_HDR 起的 len 字节。持 bus_lock 调用 */
static int himax_spi_read(struct himax_thp_chan *ch, u8 cmd, u32 len)
{
	struct himax_thp *ts = ch->ts;
	struct spi_transfer xfer = {
		.tx_buf = ts->tx,
		.rx_buf = ts->rx,
		.len = HIMAX_THP_READ_HDR + len,
	};

	memset(ts->tx, 0, xfer.len);
	ts->tx[0] = ch->op_read;
	ts->tx[1] = cmd;
	return spi_sync_transfer(ch->spi, &xfer, 1);
}

/* 总线写：发 [写功能码, cmd, 数据]，数据要先放在 ts->tx + HIMAX_THP_WRITE_HDR 起的 len 字节。持 bus_lock 调用 */
static int himax_spi_write(struct himax_thp_chan *ch, u8 cmd, u32 len)
{
	struct himax_thp *ts = ch->ts;
	struct spi_transfer xfer = {
		.tx_buf = ts->tx,
		.len = HIMAX_THP_WRITE_HDR + len,
	};

	ts->tx[0] = ch->op_write;
	ts->tx[1] = cmd;
	return spi_sync_transfer(ch->spi, &xfer, 1);
}

/* -------------------------------------------------------------------------- */
/* 帧队列 */

/*
 * 持 state_lock 调用。队列满时丢掉这一帧、留着排在前面的，与 SpbThpTool 的读完成回调一致（它在
 * REACH THP_LIST_MAX_FRAMES 时直接返回，不入队）：用户态的读帧策略是照着这个行为写的。
 */
static void himax_thp_enqueue_locked(struct himax_thp_chan *ch, const u8 *data, u64 ns)
{
	struct himax_thp_frame *f;

	if (ch->count == HIMAX_THP_FRAME_QUEUE) {
		/* 丢掉的帧也占一个序号，用户态从序号的跳变就能看出丢帧 */
		ch->seq++;
		ch->dropped++;
		dev_dbg_ratelimited(&ch->spi->dev, "frame queue full, dropped frame (%u so far)\n",
				    ch->dropped);
		return;
	}
	f = &ch->queue[(ch->head + ch->count) % HIMAX_THP_FRAME_QUEUE];
	memcpy(f->data, data, ch->frame_len);
	f->hdr = (struct himax_thp_frame_hdr) {
		.timestamp_ns = ns,
		.seq = ch->seq++,
		.len = ch->frame_len,
		.dropped = ch->dropped,
	};
	ch->count++;
}

/* 持 state_lock 调用 */
static void himax_thp_flush_locked(struct himax_thp_chan *ch)
{
	ch->head = 0;
	ch->count = 0;
}

/* -------------------------------------------------------------------------- */
/* 帧中断 */

static void himax_thp_read_frame(struct himax_thp_chan *ch, u64 ns)
{
	struct himax_thp *ts = ch->ts;
	bool want;
	int ret;

	/* 先拿 bus_lock：读帧是一次总线传输，与用户态的总线读写互斥 */
	mutex_lock(&ts->bus_lock);
	mutex_lock(&ts->state_lock);
	want = ch->spi && ch->int_enabled && himax_thp_panel_on(ts) && !ts->gone;
	mutex_unlock(&ts->state_lock);
	if (!want)
		goto out;

	ret = himax_spi_read(ch, HIMAX_THP_CMD_EVENT_STACK, ch->frame_len);
	if (ret) {
		dev_err_ratelimited(&ch->spi->dev, "frame read (op %#x) failed: %d\n",
				    ch->op_read, ret);
	} else {
		mutex_lock(&ts->state_lock);
		himax_thp_enqueue_locked(ch, ts->rx + HIMAX_THP_READ_HDR, ns);
		mutex_unlock(&ts->state_lock);
	}
out:
	mutex_unlock(&ts->bus_lock);
}

/* 等芯片放开 INT（回到高电平），见 HIMAX_THP_INT_RELEASE_US */
static void himax_thp_wait_int_release(struct himax_thp_chan *ch)
{
	unsigned int waited;

	if (!ch->int_gpio)
		return;
	for (waited = 0; waited < HIMAX_THP_INT_RELEASE_US; waited += HIMAX_THP_INT_POLL_US) {
		if (gpiod_get_raw_value_cansleep(ch->int_gpio))
			return;
		usleep_range(HIMAX_THP_INT_POLL_US, HIMAX_THP_INT_POLL_US + 20);
	}
}

/* 硬中断只记下时刻：线程被调度晚了也不影响帧的时间戳 */
static irqreturn_t himax_thp_irq(int irq, void *data)
{
	struct himax_thp_chan *ch = data;

	WRITE_ONCE(ch->irq_ns, ktime_get_ns());
	return IRQ_WAKE_THREAD;
}

/* 每一路一个中断线程，只读自己那一路 */
static irqreturn_t himax_thp_irq_thread(int irq, void *data)
{
	struct himax_thp_chan *ch = data;

	himax_thp_read_frame(ch, READ_ONCE(ch->irq_ns));
	himax_thp_wait_int_release(ch);
	wake_up_interruptible(&ch->ts->wait);

	return IRQ_HANDLED;
}

/* -------------------------------------------------------------------------- */
/* 屏幕电源联动：触摸与显示是同一颗芯片（TDDI），面板没上电时芯片的触摸部分也不工作 */

static void himax_thp_set_panel(struct himax_thp *ts, u32 set, u32 clear)
{
	bool off;
	int i;

	mutex_lock(&ts->state_lock);
	ts->panel_state = (ts->panel_state & ~clear) | set;
	ts->panel_seq++;
	off = !himax_thp_panel_on(ts);
	if (off) {
		/* 断电后芯片状态全丢，上电后由用户态重新初始化、再打开读帧 */
		for (i = 0; i < HIMAX_THP_NUM_CH; i++) {
			ts->ch[i].int_enabled = false;
			himax_thp_flush_locked(&ts->ch[i]);
		}
	}
	himax_thp_update_irq_locked(ts);
	mutex_unlock(&ts->state_lock);

	if (off) {
		/* 两路的中断都在 remove 里先摘掉 follower 再释放，这里的中断号一直有效 */
		for (i = 0; i < HIMAX_THP_NUM_CH; i++)
			if (ts->ch[i].spi)
				synchronize_irq(ts->ch[i].irq);
	}
	wake_up_interruptible(&ts->wait);
}

static int himax_thp_panel_prepared(struct drm_panel_follower *follower)
{
	struct himax_thp *ts = container_of(follower, struct himax_thp, follower);

	himax_thp_set_panel(ts, HIMAX_THP_PANEL_ON, 0);
	return 0;
}

static int himax_thp_panel_enabled(struct drm_panel_follower *follower)
{
	struct himax_thp *ts = container_of(follower, struct himax_thp, follower);

	himax_thp_set_panel(ts, HIMAX_THP_PANEL_ENABLED, 0);
	return 0;
}

static int himax_thp_panel_disabling(struct drm_panel_follower *follower)
{
	struct himax_thp *ts = container_of(follower, struct himax_thp, follower);

	himax_thp_set_panel(ts, 0, HIMAX_THP_PANEL_ENABLED);
	return 0;
}

static int himax_thp_panel_unpreparing(struct drm_panel_follower *follower)
{
	struct himax_thp *ts = container_of(follower, struct himax_thp, follower);

	himax_thp_set_panel(ts, 0, HIMAX_THP_PANEL_ON | HIMAX_THP_PANEL_ENABLED);
	return 0;
}

static const struct drm_panel_follower_funcs himax_thp_follower_funcs = {
	.panel_prepared = himax_thp_panel_prepared,
	.panel_enabled = himax_thp_panel_enabled,
	.panel_disabling = himax_thp_panel_disabling,
	.panel_unpreparing = himax_thp_panel_unpreparing,
};

/* -------------------------------------------------------------------------- */
/* 文件操作 */

static int himax_thp_open(struct inode *inode, struct file *file)
{
	struct himax_thp *ts = container_of(file->private_data, struct himax_thp, misc);
	struct himax_thp_file *f;

	f = kzalloc(sizeof(*f), GFP_KERNEL);
	if (!f)
		return -ENOMEM;
	f->bounce = kmalloc(sizeof(struct himax_thp_frame_hdr) + HIMAX_THP_BUS_MAX, GFP_KERNEL);
	if (!f->bounce) {
		kfree(f);
		return -ENOMEM;
	}

	/* misc 核心持着 misc_mtx 调进来，remove 里的 misc_deregister 也要它：这里拿到的 ts 一定还活着 */
	mutex_lock(&ts->state_lock);
	if (ts->gone) {
		mutex_unlock(&ts->state_lock);
		kfree(f->bounce);
		kfree(f);
		return -ENODEV;
	}
	kref_get(&ts->ref);
	f->ts = ts;
	f->channel = HIMAX_THP_CH_MASTER;
	f->seen_panel_seq = ts->panel_seq;
	mutex_unlock(&ts->state_lock);

	file->private_data = f;
	return stream_open(inode, file);
}

static int himax_thp_release(struct inode *inode, struct file *file)
{
	struct himax_thp_file *f = file->private_data;

	kref_put(&f->ts->ref, himax_thp_release_ts);
	kfree(f->bounce);
	kfree(f);
	return 0;
}

static bool himax_thp_frame_ready(struct himax_thp_file *f)
{
	struct himax_thp *ts = f->ts;

	return READ_ONCE(ts->gone) || READ_ONCE(ts->ch[f->channel].count);
}

static ssize_t himax_thp_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	struct himax_thp_file *f = file->private_data;
	struct himax_thp *ts = f->ts;
	struct himax_thp_chan *ch;
	struct himax_thp_frame *fr;
	size_t n;
	int ret;

	for (;;) {
		mutex_lock(&ts->state_lock);
		if (ts->gone) {
			mutex_unlock(&ts->state_lock);
			return -ENODEV;
		}
		ch = &ts->ch[f->channel];
		if (count < sizeof(fr->hdr) + ch->frame_len) {
			mutex_unlock(&ts->state_lock);
			return -EINVAL;
		}
		if (ch->count)
			break;		/* 持着 state_lock 往下取 */
		mutex_unlock(&ts->state_lock);

		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;
		ret = wait_event_interruptible(ts->wait, himax_thp_frame_ready(f));
		if (ret)
			return ret;
	}

	fr = &ch->queue[ch->head];
	n = sizeof(fr->hdr) + fr->hdr.len;
	memcpy(f->bounce, &fr->hdr, sizeof(fr->hdr));
	memcpy(f->bounce + sizeof(fr->hdr), fr->data, fr->hdr.len);
	ch->head = (ch->head + 1) % HIMAX_THP_FRAME_QUEUE;
	ch->count--;
	mutex_unlock(&ts->state_lock);

	if (copy_to_user(buf, f->bounce, n))
		return -EFAULT;
	return n;
}

static __poll_t himax_thp_poll(struct file *file, poll_table *wait)
{
	struct himax_thp_file *f = file->private_data;
	struct himax_thp *ts = f->ts;
	__poll_t mask = 0;

	poll_wait(file, &ts->wait, wait);

	mutex_lock(&ts->state_lock);
	if (ts->gone)
		mask |= EPOLLHUP | EPOLLERR;
	if (ts->ch[f->channel].count)
		mask |= EPOLLIN | EPOLLRDNORM;
	if (ts->panel_seq != f->seen_panel_seq)
		mask |= EPOLLPRI;
	mutex_unlock(&ts->state_lock);

	return mask;
}

static long himax_thp_ioctl_bus(struct himax_thp_file *f, bool read, void __user *argp)
{
	struct himax_thp *ts = f->ts;
	struct himax_thp_chan *ch = &ts->ch[f->channel];
	struct himax_thp_bus b;
	void __user *ubuf;
	long ret;

	if (copy_from_user(&b, argp, sizeof(b)))
		return -EFAULT;
	if (b.len > HIMAX_THP_BUS_MAX || (read && !b.len) ||
	    memchr_inv(b.reserved, 0, sizeof(b.reserved)))
		return -EINVAL;
	ubuf = u64_to_user_ptr(b.buf);

	mutex_lock(&ts->bus_lock);
	/* 没有级联 Slave 时这一路没有总线 */
	if (READ_ONCE(ts->gone) || !ch->spi) {
		ret = -ENODEV;
		goto out;
	}
	if (read) {
		ret = himax_spi_read(ch, b.cmd, b.len);
		if (!ret && copy_to_user(ubuf, ts->rx + HIMAX_THP_READ_HDR, b.len))
			ret = -EFAULT;
	} else {
		if (copy_from_user(ts->tx + HIMAX_THP_WRITE_HDR, ubuf, b.len))
			ret = -EFAULT;
		else
			ret = himax_spi_write(ch, b.cmd, b.len);
	}
out:
	mutex_unlock(&ts->bus_lock);
	return ret;
}

static long himax_thp_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct himax_thp_file *f = file->private_data;
	struct himax_thp *ts = f->ts;
	void __user *argp = (void __user *)arg;
	u32 val;
	long ret = 0;

	if (cmd == HIMAX_THP_IOC_BUS_READ || cmd == HIMAX_THP_IOC_BUS_WRITE)
		return himax_thp_ioctl_bus(f, cmd == HIMAX_THP_IOC_BUS_READ, argp);

	if (cmd == HIMAX_THP_IOC_PANEL_STATE) {
		mutex_lock(&ts->state_lock);
		val = ts->panel_state;
		f->seen_panel_seq = ts->panel_seq;
		mutex_unlock(&ts->state_lock);
		return put_user(val, (u32 __user *)argp);
	}

	if (_IOC_DIR(cmd) != _IOC_WRITE || _IOC_SIZE(cmd) != sizeof(u32))
		return -ENOTTY;
	if (get_user(val, (u32 __user *)argp))
		return -EFAULT;

	switch (cmd) {
	case HIMAX_THP_IOC_SET_CHANNEL:
		if (val >= HIMAX_THP_NUM_CH)
			return -EINVAL;
		f->channel = val;
		break;

	case HIMAX_THP_IOC_INT_ENABLE:
		mutex_lock(&ts->state_lock);
		if (ts->gone) {
			ret = -ENODEV;
		} else {
			ts->ch[f->channel].int_enabled = !!val;
			if (!val)
				himax_thp_flush_locked(&ts->ch[f->channel]);
			himax_thp_update_irq_locked(ts);
		}
		mutex_unlock(&ts->state_lock);
		/*
		 * 与 SpbThpTool 的 INT_OPEN 一致：打开时立刻读一帧（和中断里读帧同一条路）。芯片把一帧交出去之前不会再
		 * 拉下一次 INT，打开前就已经备好的那一帧没人取的话这一路会一直停着；用户态遇到某一路超时就重新打开它，
		 * 靠的也是这一次读。
		 */
		if (!ret && val) {
			himax_thp_read_frame(&ts->ch[f->channel], ktime_get_ns());
			wake_up_interruptible(&ts->wait);
		}
		break;

	case HIMAX_THP_IOC_SET_RESET:
		if (!ts->reset)
			return -ENODEV;
		/* DT 里复位脚是低有效：逻辑 1（复位）= 物理低。ABI 给的是物理电平 */
		mutex_lock(&ts->bus_lock);
		if (READ_ONCE(ts->gone))
			ret = -ENODEV;
		else
			gpiod_set_value_cansleep(ts->reset, !val);
		mutex_unlock(&ts->bus_lock);
		break;

	default:
		return -ENOTTY;
	}

	return ret;
}

static const struct file_operations himax_thp_fops = {
	.owner = THIS_MODULE,
	.open = himax_thp_open,
	.release = himax_thp_release,
	.read = himax_thp_read,
	.poll = himax_thp_poll,
	.unlocked_ioctl = himax_thp_ioctl,
	.compat_ioctl = compat_ptr_ioctl,
};

/* -------------------------------------------------------------------------- */
/* probe / remove */

/*
 * DT 只给了帧中断（interrupts-extended = <&tlmm 175 ...>），没有单独的 int-gpios。中断控制器报不了
 * 电平（msm 的 irqchip 没有 irq_get_irqchip_state），所以从中断的引用反查出那根 GPIO，只用来读电平。
 */
static void himax_thp_find_int_gpio(struct himax_thp_chan *ch, struct device *dev)
{
	struct of_phandle_args args;
	struct gpio_device *gdev;
	struct gpio_desc *desc;

	if (of_parse_phandle_with_args(dev->of_node, "interrupts-extended",
				       "#interrupt-cells", 0, &args))
		goto none;
	gdev = gpio_device_find_by_fwnode(of_fwnode_handle(args.np));
	of_node_put(args.np);
	if (!gdev)
		goto none;
	desc = gpio_device_get_desc(gdev, args.args[0]);
	if (IS_ERR(desc)) {
		gpio_device_put(gdev);
		goto none;
	}
	ch->int_gdev = gdev;
	ch->int_gpio = desc;
	return;
none:
	dev_warn(dev, "cannot read the INT line, frames may be read twice per interrupt\n");
}

static int himax_thp_init_chan(struct himax_thp *ts, struct himax_thp_chan *ch,
			       u8 op_write, u8 op_read, u32 frame_len)
{
	int i;

	ch->ts = ts;
	ch->op_write = op_write;
	ch->op_read = op_read;
	ch->frame_len = frame_len;
	ch->qbuf = kmalloc_array(HIMAX_THP_FRAME_QUEUE, frame_len, GFP_KERNEL);
	if (!ch->qbuf)
		return -ENOMEM;
	for (i = 0; i < HIMAX_THP_FRAME_QUEUE; i++)
		ch->queue[i].data = ch->qbuf + i * frame_len;
	return 0;
}

/* 申请某一路的帧中断，触发方式按 DT；先不打开，跟着读帧开关走（himax_thp_update_irq_locked） */
static int himax_thp_request_irq(struct himax_thp_chan *ch, const char *name)
{
	return request_threaded_irq(ch->irq, himax_thp_irq, himax_thp_irq_thread,
				    IRQF_ONESHOT | IRQF_NO_AUTOEN, name, ch);
}

/*
 * 找到 himax,cascade 指向的 Slave，接成第二路。Slave 要先由本驱动绑定（它的 pinctrl 随绑定生效）；还没有的话
 * 推迟 probe，等它绑定后驱动核心会重试。device link 让 Slave 解绑前先解绑 Master，所以 Master 在的时候这一路的
 * spi 一直有效。没有 himax,cascade 时只有 Master 一路，用户态在 Slave 那一路拿到 -ENODEV。
 */
static int himax_thp_get_slave(struct himax_thp *ts)
{
	struct device *dev = &ts->spi->dev;
	struct himax_thp_chan *ch = &ts->ch[HIMAX_THP_CH_SLAVE];
	struct device_node *np;
	struct device *sdev;
	int ret;

	np = of_parse_phandle(dev->of_node, "himax,cascade", 0);
	if (!np)
		return 0;
	sdev = bus_find_device_by_of_node(&spi_bus_type, np);
	of_node_put(np);
	if (!sdev)
		return dev_err_probe(dev, -EPROBE_DEFER, "cascaded slave not registered yet\n");
	if (READ_ONCE(sdev->driver) != dev->driver) {
		ret = dev_err_probe(dev, -EPROBE_DEFER, "cascaded slave not bound yet\n");
		goto err_put;
	}
	if (!device_link_add(dev, sdev, DL_FLAG_AUTOREMOVE_CONSUMER)) {
		ret = dev_err_probe(dev, -EINVAL, "cannot link to the cascaded slave\n");
		goto err_put;
	}

	ch->spi = to_spi_device(sdev);
	ch->irq = ch->spi->irq;
	himax_thp_find_int_gpio(ch, sdev);
	return 0;

err_put:
	put_device(sdev);
	return ret;
}

/* Slave 自己只检查一下帧中断：它的总线与中断都由 himax,cascade 指向它的 Master 接管 */
static int himax_thp_slave_probe(struct spi_device *spi)
{
	if (spi->irq <= 0)
		return dev_err_probe(&spi->dev, -EINVAL, "no frame interrupt\n");
	return 0;
}

static int himax_thp_master_probe(struct spi_device *spi)
{
	struct device *dev = &spi->dev;
	struct himax_thp_chan *mch, *sch;
	struct gpio_desc *mode;
	struct himax_thp *ts;
	int ret;

	if (spi->irq <= 0)
		return dev_err_probe(dev, -EINVAL, "no frame interrupt\n");

	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (!ts)
		return -ENOMEM;
	kref_init(&ts->ref);
	mutex_init(&ts->bus_lock);
	mutex_init(&ts->state_lock);
	init_waitqueue_head(&ts->wait);
	ts->spi = spi_dev_get(spi);

	/* kmalloc 的内存可以直接做 DMA，栈上的不行 */
	ts->tx = kmalloc(HIMAX_THP_READ_HDR + HIMAX_THP_BUS_MAX, GFP_KERNEL);
	ts->rx = kmalloc(HIMAX_THP_READ_HDR + HIMAX_THP_BUS_MAX, GFP_KERNEL);
	if (!ts->tx || !ts->rx) {
		ret = -ENOMEM;
		goto err_put;
	}
	ret = himax_thp_init_chan(ts, &ts->ch[HIMAX_THP_CH_MASTER], HIMAX_THP_OP_WRITE_MASTER,
				  HIMAX_THP_OP_READ_MASTER, HIMAX_THP_MASTER_FRAME_LEN);
	if (!ret)
		ret = himax_thp_init_chan(ts, &ts->ch[HIMAX_THP_CH_SLAVE], HIMAX_THP_OP_WRITE_SLAVE,
					  HIMAX_THP_OP_READ_SLAVE, HIMAX_THP_SLAVE_FRAME_LEN);
	if (ret)
		goto err_put;
	mch = &ts->ch[HIMAX_THP_CH_MASTER];
	sch = &ts->ch[HIMAX_THP_CH_SLAVE];
	/* Master 这一路的 spi 借用 ts->spi 的引用，不另外 get */
	mch->spi = spi;
	mch->irq = spi->irq;

	ret = himax_thp_get_slave(ts);
	if (ret)
		goto err_put;

	/* 逻辑低 = 不复位：接管时不打断正在运行的芯片，复位时序由用户态的 Init 决定 */
	ts->reset = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ts->reset)) {
		ret = dev_err_probe(dev, PTR_ERR(ts->reset), "reset gpio\n");
		goto err_put;
	}
	/* 拉低 = SPI 模式，下一次复位后的 FW reload 采样它；旧 DT 用 pinctrl 拉低、没有这个属性 */
	mode = devm_gpiod_get_optional(dev, "mode", GPIOD_OUT_LOW);
	if (IS_ERR(mode)) {
		ret = dev_err_probe(dev, PTR_ERR(mode), "mode gpio\n");
		goto err_put;
	}

	himax_thp_find_int_gpio(mch, dev);

	ret = himax_thp_request_irq(mch, "himax-thp");
	if (ret) {
		dev_err_probe(dev, ret, "request irq %d\n", mch->irq);
		goto err_put;
	}
	if (sch->spi) {
		ret = himax_thp_request_irq(sch, "himax-thp-slave");
		if (ret) {
			dev_err_probe(dev, ret, "request slave irq %d\n", sch->irq);
			goto err_irq;
		}
	}

	if (drm_is_panel_follower(dev)) {
		/* 面板已经上电的话，这里会直接回调 panel_prepared（点亮了的话还有 panel_enabled） */
		ts->follower.funcs = &himax_thp_follower_funcs;
		ret = drm_panel_add_follower(dev, &ts->follower);
		if (ret) {
			dev_err_probe(dev, ret, "panel follower\n");
			goto err_slave_irq;
		}
		ts->has_follower = true;
	} else {
		dev_warn(dev, "no panel phandle, assuming panel always powered\n");
		ts->panel_state = HIMAX_THP_PANEL_ON | HIMAX_THP_PANEL_ENABLED;
	}

	ts->misc.minor = MISC_DYNAMIC_MINOR;
	ts->misc.name = HIMAX_THP_DEVICE_NAME;
	ts->misc.fops = &himax_thp_fops;
	ts->misc.parent = dev;
	ret = misc_register(&ts->misc);
	if (ret) {
		dev_err_probe(dev, ret, "misc register\n");
		goto err_follower;
	}

	spi_set_drvdata(spi, ts);
	dev_info(dev, "ready: /dev/%s, irq %d trigger %#x (INT level %s), panel %#x\n",
		 HIMAX_THP_DEVICE_NAME, mch->irq, irq_get_trigger_type(mch->irq),
		 mch->int_gpio ? "readable" : "unknown", ts->panel_state);
	if (sch->spi)
		dev_info(dev, "cascaded slave %s: irq %d trigger %#x (INT level %s)\n",
			 dev_name(&sch->spi->dev), sch->irq, irq_get_trigger_type(sch->irq),
			 sch->int_gpio ? "readable" : "unknown");
	else
		dev_info(dev, "no cascaded slave\n");
	return 0;

err_follower:
	if (ts->has_follower)
		drm_panel_remove_follower(&ts->follower);
err_slave_irq:
	if (sch->spi)
		free_irq(sch->irq, sch);
err_irq:
	free_irq(mch->irq, mch);
err_put:
	kref_put(&ts->ref, himax_thp_release_ts);
	return ret;
}

static void himax_thp_master_remove(struct spi_device *spi)
{
	struct himax_thp *ts = spi_get_drvdata(spi);
	struct himax_thp_chan *mch = &ts->ch[HIMAX_THP_CH_MASTER];
	struct himax_thp_chan *sch = &ts->ch[HIMAX_THP_CH_SLAVE];

	/* 不再接受新的打开；已经打开的文件各自持着引用，之后的操作都返回 -ENODEV */
	misc_deregister(&ts->misc);

	/* 两把锁都拿：保证没有进行中的传输，之后谁也不会再碰 SPI */
	mutex_lock(&ts->bus_lock);
	mutex_lock(&ts->state_lock);
	ts->gone = true;
	himax_thp_update_irq_locked(ts);
	mutex_unlock(&ts->state_lock);
	mutex_unlock(&ts->bus_lock);
	wake_up_interruptible(&ts->wait);

	if (ts->has_follower)
		drm_panel_remove_follower(&ts->follower);
	if (sch->spi)
		free_irq(sch->irq, sch);
	free_irq(mch->irq, mch);

	kref_put(&ts->ref, himax_thp_release_ts);
}

enum himax_thp_role {
	HIMAX_THP_ROLE_MASTER,
	HIMAX_THP_ROLE_SLAVE,
};

static bool himax_thp_is_slave(struct spi_device *spi)
{
	return (uintptr_t)spi_get_device_match_data(spi) == HIMAX_THP_ROLE_SLAVE;
}

static int himax_thp_probe(struct spi_device *spi)
{
	return himax_thp_is_slave(spi) ? himax_thp_slave_probe(spi) : himax_thp_master_probe(spi);
}

static void himax_thp_remove(struct spi_device *spi)
{
	/* Slave 没有自己的状态；device link 保证它解绑前 Master 已经解绑 */
	if (!himax_thp_is_slave(spi))
		himax_thp_master_remove(spi);
}

static const struct of_device_id himax_thp_of_match[] = {
	{ .compatible = "himax,hx83121a-ts", .data = (void *)HIMAX_THP_ROLE_MASTER },
	{ .compatible = "himax,hx83121a-ts-slave", .data = (void *)HIMAX_THP_ROLE_SLAVE },
	{ }
};
MODULE_DEVICE_TABLE(of, himax_thp_of_match);

static const struct spi_device_id himax_thp_spi_ids[] = {
	{ "hx83121a-ts", HIMAX_THP_ROLE_MASTER },
	{ "hx83121a-ts-slave", HIMAX_THP_ROLE_SLAVE },
	{ }
};
MODULE_DEVICE_TABLE(spi, himax_thp_spi_ids);

static struct spi_driver himax_thp_driver = {
	.driver = {
		.name = "himax-thp",
		.of_match_table = himax_thp_of_match,
	},
	.probe = himax_thp_probe,
	.remove = himax_thp_remove,
	.id_table = himax_thp_spi_ids,
};
module_spi_driver(himax_thp_driver);

MODULE_DESCRIPTION("Himax HX83121A SPI transport for a userspace touch daemon");
MODULE_LICENSE("GPL");
