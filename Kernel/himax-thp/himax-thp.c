// SPDX-License-Identifier: GPL-2.0
/*
 * himax-thp — Himax HX83121A 的 SPI 传输驱动（Linux 版的 SPBTESTTOOL）。
 *
 * 只做硬件相关的事：复位脚、帧中断、屏幕电源联动和 SPI 收发，不解算。芯片协议、初始化与触点解算
 * 都在用户态（EGoTouchService 的 HimaxCore + TsaCore），用户态经 /dev/himax_thp 访问：
 *
 *   ioctl XFER       一次全双工传输（Master F2/F3、Slave F4/F5）
 *   read / poll      帧中断里读好的最新一帧
 *   ioctl 其余       通道绑定、帧长、读帧开关、复位脚、面板电源状态
 *
 * Master 与 Slave 是两颗芯片，各在一路 SPI 上、各有一根 INT（本机 ACPI 的 \_SB.THPA / THPB）：Master 是
 * spi6 上的 himax,hx83121a-ts，Slave 是 spi20 上的 egotouch,hx83121a-thp-slave（主线 DT 没有，
 * 由 thp-slave-dt.ko 的 overlay 补上）。两者都由本驱动绑定，Slave 挂到 Master 的实例上作为第二路，
 * 对用户态仍是一个 /dev/himax_thp。复位脚与面板联动只在 Master 上。
 *
 * 帧在各路自己的中断线程里读：打开了读帧的那一路发 [功能码, 0x30, 0x00] 后连读整帧，原样（含 3 字节总线头）
 * 放进该路的帧槽，用户态 read 取走。中断触发方式与 SPI 模式都按 DT。
 *
 * 芯片要在 SPI 模式下才按帧拉 INT：它在复位后的 FW reload 时采样 gpio174，低 = SPI、高 = I2C-HID。这根脚由
 * DT 配成低电平（Master 节点 ts0_default 里的 mode-n-pins），本驱动不管它。
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
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
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

/*
 * 读帧的总线命令：事件栈寄存器。Windows 帧的 7 字节头 = 3 字节总线头 + 4 字节数据头，与 linux-gaokun
 * 驱动读事件栈时 heatmap 落在数据第 4 字节一致，据此推断 SPBTESTTOOL 的 GET_FRAME 也是读它
 * （待实机用用户态的帧校验确认）。
 */
#define HIMAX_THP_CMD_EVENT_STACK	0x30

/* 默认帧长，与 Common/include/FrameLayout.h 的 kMasterFrameSize / kSlaveFrameSize 一致 */
#define HIMAX_THP_MASTER_FRAME_LEN	5063
#define HIMAX_THP_SLAVE_FRAME_LEN	339

/*
 * 芯片在帧准备好时拉低 INT，主机读完帧才放开，放开要一点时间。Master 的帧中断按 DT 是低电平触发：中断线程读完
 * 立即返回的话 INT 还低着，马上又触发一次，读到的要么是空的事件栈（全零），要么是芯片正在写的下一帧（错位）。
 * 所以读完帧后等 INT 回到高电平再返回，最多等这么久（帧间隔约 8 ms）。Slave 按 overlay 是下降沿触发，等一下也无妨。
 */
#define HIMAX_THP_INT_RELEASE_US	2000
#define HIMAX_THP_INT_POLL_US		50

struct himax_thp;

struct himax_thp_chan {
	struct himax_thp *ts;
	struct spi_device *spi;		/* 这一路的 SPI 设备；Slave 还没接上时为 NULL。改它要持两把锁 */
	int irq;			/* 这一路的帧中断，spi 为 NULL 时无效 */
	bool irq_enabled;
	struct gpio_desc *int_gpio;	/* 帧中断那根脚，只读电平；找不到时为 NULL */
	struct gpio_device *int_gdev;	/* int_gpio 所在的 GPIO 控制器，持着引用 */
	u8 op_write;
	u8 op_read;
	u32 frame_len;			/* 中断里每帧读多少字节 */
	bool int_enabled;		/* 用户态打开了这一路的读帧 */
	u8 *frame;			/* 最新一帧 */
	u32 frame_bytes;		/* frame 里有效的字节数 */
	u64 seq;			/* 每读到一帧 +1 */
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

	/* 状态：帧槽、序号、开关、面板电源 */
	struct mutex state_lock;
	wait_queue_head_t wait;
	struct himax_thp_chan ch[HIMAX_THP_NUM_CH];
	bool panel_on;
	u64 panel_seq;			/* 面板电源每变化一次 +1 */
	bool gone;			/* 设备已解绑，所有文件操作返回 -ENODEV */
};

struct himax_thp_file {
	struct himax_thp *ts;
	u32 channel;
	u64 seen_seq;			/* 本文件最后取走的帧序号 */
	u64 seen_panel_seq;		/* 本文件最后读过的面板状态序号 */
	u8 *bounce;			/* read 时帧槽到用户态的中转 */
};

/*
 * Master 与 Slave 是两个 spi 设备，probe 先后不定：两边各自登记，后到的那个负责接上。
 * 本机只有一套触摸，用全局变量就够了。
 */
static DEFINE_MUTEX(himax_thp_pair_lock);
static struct himax_thp *himax_thp_master;	/* 已经 probe 好的 Master 实例 */
static struct spi_device *himax_thp_slave_spi;	/* 已经绑定的 Slave 设备 */

static void himax_thp_release_ts(struct kref *ref)
{
	struct himax_thp *ts = container_of(ref, struct himax_thp, ref);
	int i;

	for (i = 0; i < HIMAX_THP_NUM_CH; i++) {
		kfree(ts->ch[i].frame);
		if (ts->ch[i].int_gdev)
			gpio_device_put(ts->ch[i].int_gdev);
	}
	kfree(ts->tx);
	kfree(ts->rx);
	spi_dev_put(ts->spi);
	kfree(ts);
}

/* 每一路的帧中断开关跟着「面板上电、这一路接上了且在读帧」走。持 state_lock 调用 */
static void himax_thp_update_irq_locked(struct himax_thp *ts)
{
	int i;

	for (i = 0; i < HIMAX_THP_NUM_CH; i++) {
		struct himax_thp_chan *ch = &ts->ch[i];
		bool want = ts->panel_on && !ts->gone && ch->spi && ch->int_enabled;

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

/* 在某一路上做一次全双工传输。持 bus_lock 调用，ch->spi 不为 NULL */
static int himax_thp_spi_xfer(struct himax_thp_chan *ch, const u8 *tx, u8 *rx, u32 len)
{
	struct spi_transfer xfer = {
		.tx_buf = tx,
		.rx_buf = rx,
		.len = len,
	};

	return spi_sync_transfer(ch->spi, &xfer, 1);
}

/* -------------------------------------------------------------------------- */
/* 帧中断 */

static void himax_thp_read_frame(struct himax_thp_chan *ch)
{
	struct himax_thp *ts = ch->ts;
	u32 len;
	int ret;

	/* 先拿 bus_lock：Slave 断开时两把锁都持着改 ch->spi，持着 bus_lock 时它不会变 */
	mutex_lock(&ts->bus_lock);
	mutex_lock(&ts->state_lock);
	len = ch->spi && ch->int_enabled && ts->panel_on && !ts->gone ? ch->frame_len : 0;
	mutex_unlock(&ts->state_lock);
	if (!len)
		goto out;

	memset(ts->tx, 0, len);
	ts->tx[0] = ch->op_read;
	ts->tx[1] = HIMAX_THP_CMD_EVENT_STACK;
	ret = himax_thp_spi_xfer(ch, ts->tx, ts->rx, len);
	if (ret) {
		dev_err_ratelimited(&ch->spi->dev, "frame read (op %#x) failed: %d\n",
				    ch->op_read, ret);
	} else {
		mutex_lock(&ts->state_lock);
		memcpy(ch->frame, ts->rx, len);
		ch->frame_bytes = len;
		ch->seq++;
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

/* 每一路一个中断线程，只读自己那一路 */
static irqreturn_t himax_thp_irq_thread(int irq, void *data)
{
	struct himax_thp_chan *ch = data;

	himax_thp_read_frame(ch);
	himax_thp_wait_int_release(ch);
	wake_up_interruptible(&ch->ts->wait);

	return IRQ_HANDLED;
}

/* -------------------------------------------------------------------------- */
/* 屏幕电源联动：触摸与显示是同一颗芯片（TDDI），面板没上电时芯片的触摸部分也不工作 */

static void himax_thp_set_panel(struct himax_thp *ts, bool on)
{
	int i;

	mutex_lock(&ts->state_lock);
	ts->panel_on = on;
	ts->panel_seq++;
	if (!on) {
		/* 断电后芯片状态全丢，上电后由用户态重新初始化、再打开读帧 */
		for (i = 0; i < HIMAX_THP_NUM_CH; i++)
			ts->ch[i].int_enabled = false;
	}
	himax_thp_update_irq_locked(ts);
	mutex_unlock(&ts->state_lock);

	if (!on) {
		/* Slave 的中断号在它断开前一直有效（断开要拿 pair_lock，follower 回调不拿，但 free_irq 自己会等） */
		for (i = 0; i < HIMAX_THP_NUM_CH; i++)
			if (READ_ONCE(ts->ch[i].irq) > 0)
				synchronize_irq(ts->ch[i].irq);
	}
	wake_up_interruptible(&ts->wait);
}

static int himax_thp_panel_prepared(struct drm_panel_follower *follower)
{
	struct himax_thp *ts = container_of(follower, struct himax_thp, follower);

	himax_thp_set_panel(ts, true);
	return 0;
}

static int himax_thp_panel_unpreparing(struct drm_panel_follower *follower)
{
	struct himax_thp *ts = container_of(follower, struct himax_thp, follower);

	himax_thp_set_panel(ts, false);
	return 0;
}

static const struct drm_panel_follower_funcs himax_thp_follower_funcs = {
	.panel_prepared = himax_thp_panel_prepared,
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
	f->bounce = kmalloc(HIMAX_THP_MAX_XFER, GFP_KERNEL);
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
	f->seen_seq = ts->ch[HIMAX_THP_CH_MASTER].seq;
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

	return READ_ONCE(ts->gone) ||
	       READ_ONCE(ts->ch[f->channel].seq) != f->seen_seq;
}

static ssize_t himax_thp_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	struct himax_thp_file *f = file->private_data;
	struct himax_thp *ts = f->ts;
	struct himax_thp_chan *ch;
	size_t n;
	int ret;

	for (;;) {
		mutex_lock(&ts->state_lock);
		if (ts->gone) {
			mutex_unlock(&ts->state_lock);
			return -ENODEV;
		}
		ch = &ts->ch[f->channel];
		if (ch->seq != f->seen_seq)
			break;		/* 持着 state_lock 往下拷 */
		mutex_unlock(&ts->state_lock);

		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;
		ret = wait_event_interruptible(ts->wait, himax_thp_frame_ready(f));
		if (ret)
			return ret;
	}

	n = min_t(size_t, count, ch->frame_bytes);
	memcpy(f->bounce, ch->frame, n);
	f->seen_seq = ch->seq;
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
	if (ts->ch[f->channel].seq != f->seen_seq)
		mask |= EPOLLIN | EPOLLRDNORM;
	if (ts->panel_seq != f->seen_panel_seq)
		mask |= EPOLLPRI;
	mutex_unlock(&ts->state_lock);

	return mask;
}

static long himax_thp_ioctl_xfer(struct himax_thp_file *f, void __user *argp)
{
	struct himax_thp *ts = f->ts;
	struct himax_thp_chan *ch = &ts->ch[f->channel];
	struct himax_thp_xfer x;
	void __user *rx_user;
	long ret;

	if (copy_from_user(&x, argp, sizeof(x)))
		return -EFAULT;
	if (!x.len || x.len > HIMAX_THP_MAX_XFER || x.reserved)
		return -EINVAL;
	rx_user = u64_to_user_ptr(x.rx_buf);

	mutex_lock(&ts->bus_lock);
	/* Slave 没接上（overlay 没打、或还没 probe）时这一路没有总线 */
	if (READ_ONCE(ts->gone) || !ch->spi) {
		ret = -ENODEV;
		goto out;
	}
	if (copy_from_user(ts->tx, u64_to_user_ptr(x.tx_buf), x.len)) {
		ret = -EFAULT;
		goto out;
	}
	/* 只能用所绑通道的功能码，防止一个文件的读写串到另一路 */
	if (ts->tx[0] != ch->op_write && ts->tx[0] != ch->op_read) {
		ret = -EINVAL;
		goto out;
	}

	ret = himax_thp_spi_xfer(ch, ts->tx, x.rx_buf ? ts->rx : NULL, x.len);
	if (!ret && x.rx_buf && copy_to_user(rx_user, ts->rx, x.len))
		ret = -EFAULT;
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

	if (cmd == HIMAX_THP_IOC_XFER)
		return himax_thp_ioctl_xfer(f, argp);

	if (cmd == HIMAX_THP_IOC_PANEL_STATE) {
		mutex_lock(&ts->state_lock);
		val = ts->panel_on ? HIMAX_THP_PANEL_ON : 0;
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
		mutex_lock(&ts->state_lock);
		f->channel = val;
		f->seen_seq = ts->ch[val].seq;
		mutex_unlock(&ts->state_lock);
		break;

	case HIMAX_THP_IOC_SET_FRAME_LEN:
		/* 至少要盖住 3 字节总线头 */
		if (val < 4 || val > HIMAX_THP_MAX_XFER)
			return -EINVAL;
		/* 拿 bus_lock：中断线程按 frame_len 往收发缓冲里读，不能读到一半改长度 */
		mutex_lock(&ts->bus_lock);
		mutex_lock(&ts->state_lock);
		ts->ch[f->channel].frame_len = val;
		mutex_unlock(&ts->state_lock);
		mutex_unlock(&ts->bus_lock);
		break;

	case HIMAX_THP_IOC_INT_ENABLE:
		mutex_lock(&ts->state_lock);
		if (ts->gone) {
			ret = -ENODEV;
		} else {
			ts->ch[f->channel].int_enabled = !!val;
			himax_thp_update_irq_locked(ts);
		}
		mutex_unlock(&ts->state_lock);
		/*
		 * 与 SpbThpTool 的 INT_OPEN 一致：打开时立刻读一帧（和中断里读帧同一条路）。芯片把一帧交出去之前不会再
		 * 拉下一次 INT，打开前就已经备好的那一帧没人取的话这一路会一直停着；用户态遇到某一路超时就重新打开它，
		 * 靠的也是这一次读。
		 */
		if (!ret && val) {
			himax_thp_read_frame(&ts->ch[f->channel]);
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

static void himax_thp_put_int_gpio(struct himax_thp_chan *ch)
{
	if (ch->int_gdev)
		gpio_device_put(ch->int_gdev);
	ch->int_gdev = NULL;
	ch->int_gpio = NULL;
}

static void himax_thp_init_chan(struct himax_thp *ts, struct himax_thp_chan *ch,
				u8 op_write, u8 op_read, u32 frame_len)
{
	ch->ts = ts;
	ch->op_write = op_write;
	ch->op_read = op_read;
	ch->frame_len = frame_len;
}

/* 申请某一路的帧中断，触发方式按 DT；先不打开，跟着读帧开关走（himax_thp_update_irq_locked） */
static int himax_thp_request_irq(struct himax_thp_chan *ch, const char *name)
{
	return request_threaded_irq(ch->irq, NULL, himax_thp_irq_thread,
				    IRQF_ONESHOT | IRQF_NO_AUTOEN, name, ch);
}

/* 把 Slave 设备接到 Master 实例上，成为第二路。持 pair_lock 调用 */
static int himax_thp_attach_slave(struct himax_thp *ts, struct spi_device *spi)
{
	struct himax_thp_chan *ch = &ts->ch[HIMAX_THP_CH_SLAVE];
	int ret;

	if (spi->irq <= 0)
		return dev_err_probe(&spi->dev, -EINVAL, "no frame interrupt\n");

	himax_thp_find_int_gpio(ch, &spi->dev);
	ch->irq = spi->irq;
	ret = himax_thp_request_irq(ch, "himax-thp-slave");
	if (ret) {
		dev_err(&spi->dev, "request irq %d: %d\n", ch->irq, ret);
		ch->irq = 0;
		himax_thp_put_int_gpio(ch);
		return ret;
	}

	mutex_lock(&ts->bus_lock);
	mutex_lock(&ts->state_lock);
	ch->spi = spi_dev_get(spi);
	ch->irq_enabled = false;
	himax_thp_update_irq_locked(ts);
	mutex_unlock(&ts->state_lock);
	mutex_unlock(&ts->bus_lock);

	dev_info(&spi->dev, "slave attached to %s, irq %d trigger %#x (INT level %s)\n",
		 dev_name(&ts->spi->dev), ch->irq, irq_get_trigger_type(ch->irq),
		 ch->int_gpio ? "readable" : "unknown");
	return 0;
}

/* 断开 Slave。持 pair_lock 调用，Slave 必须已经接上 */
static void himax_thp_detach_slave(struct himax_thp *ts)
{
	struct himax_thp_chan *ch = &ts->ch[HIMAX_THP_CH_SLAVE];
	struct spi_device *spi;

	/* 两把锁都拿：没有进行中的传输，之后这一路的 XFER 与读帧都看到 spi == NULL */
	mutex_lock(&ts->bus_lock);
	mutex_lock(&ts->state_lock);
	spi = ch->spi;
	ch->spi = NULL;
	himax_thp_update_irq_locked(ts);
	mutex_unlock(&ts->state_lock);
	mutex_unlock(&ts->bus_lock);
	wake_up_interruptible(&ts->wait);

	free_irq(ch->irq, ch);
	WRITE_ONCE(ch->irq, 0);
	himax_thp_put_int_gpio(ch);
	dev_info(&spi->dev, "slave detached\n");
	spi_dev_put(spi);
}

static int himax_thp_slave_probe(struct spi_device *spi)
{
	int ret = 0;

	mutex_lock(&himax_thp_pair_lock);
	if (himax_thp_slave_spi) {
		ret = dev_err_probe(&spi->dev, -EBUSY, "a slave is already bound\n");
		goto out;
	}
	/* Master 还没来的话先登记，等它 probe 时接上 */
	if (himax_thp_master) {
		ret = himax_thp_attach_slave(himax_thp_master, spi);
		if (ret)
			goto out;
	} else {
		dev_info(&spi->dev, "waiting for the master\n");
	}
	himax_thp_slave_spi = spi;
out:
	mutex_unlock(&himax_thp_pair_lock);
	return ret;
}

static void himax_thp_slave_remove(struct spi_device *spi)
{
	mutex_lock(&himax_thp_pair_lock);
	if (himax_thp_master && himax_thp_master->ch[HIMAX_THP_CH_SLAVE].spi == spi)
		himax_thp_detach_slave(himax_thp_master);
	himax_thp_slave_spi = NULL;
	mutex_unlock(&himax_thp_pair_lock);
}

static int himax_thp_master_probe(struct spi_device *spi)
{
	struct device *dev = &spi->dev;
	struct himax_thp_chan *mch;
	struct himax_thp *ts;
	int i, ret;

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
	ts->tx = kmalloc(HIMAX_THP_MAX_XFER, GFP_KERNEL);
	ts->rx = kmalloc(HIMAX_THP_MAX_XFER, GFP_KERNEL);
	for (i = 0; i < HIMAX_THP_NUM_CH; i++)
		ts->ch[i].frame = kmalloc(HIMAX_THP_MAX_XFER, GFP_KERNEL);
	if (!ts->tx || !ts->rx || !ts->ch[0].frame || !ts->ch[1].frame) {
		ret = -ENOMEM;
		goto err_put;
	}
	himax_thp_init_chan(ts, &ts->ch[HIMAX_THP_CH_MASTER], HIMAX_THP_OP_WRITE_MASTER,
			    HIMAX_THP_OP_READ_MASTER, HIMAX_THP_MASTER_FRAME_LEN);
	himax_thp_init_chan(ts, &ts->ch[HIMAX_THP_CH_SLAVE], HIMAX_THP_OP_WRITE_SLAVE,
			    HIMAX_THP_OP_READ_SLAVE, HIMAX_THP_SLAVE_FRAME_LEN);
	mch = &ts->ch[HIMAX_THP_CH_MASTER];
	/* Master 这一路的 spi 借用 ts->spi 的引用，不另外 get */
	mch->spi = spi;
	mch->irq = spi->irq;

	/* 逻辑低 = 不复位：接管时不打断正在运行的芯片，复位时序由用户态的 Init 决定 */
	ts->reset = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ts->reset)) {
		ret = dev_err_probe(dev, PTR_ERR(ts->reset), "reset gpio\n");
		goto err_put;
	}

	himax_thp_find_int_gpio(mch, dev);

	ret = himax_thp_request_irq(mch, "himax-thp");
	if (ret) {
		dev_err_probe(dev, ret, "request irq %d\n", mch->irq);
		goto err_put;
	}

	if (drm_is_panel_follower(dev)) {
		/* 面板已经上电的话，这里会直接回调 panel_prepared */
		ts->follower.funcs = &himax_thp_follower_funcs;
		ret = drm_panel_add_follower(dev, &ts->follower);
		if (ret) {
			dev_err_probe(dev, ret, "panel follower\n");
			goto err_irq;
		}
		ts->has_follower = true;
	} else {
		dev_warn(dev, "no panel phandle, assuming panel always powered\n");
		ts->panel_on = true;
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
	dev_info(dev, "ready: /dev/%s, irq %d trigger %#x (INT level %s), panel %s\n",
		 HIMAX_THP_DEVICE_NAME, mch->irq, irq_get_trigger_type(mch->irq),
		 mch->int_gpio ? "readable" : "unknown", ts->panel_on ? "on" : "off");

	mutex_lock(&himax_thp_pair_lock);
	himax_thp_master = ts;
	/* Slave 接不上不影响 Master 单独工作，用户态在 Slave 那一路会拿到 -ENODEV */
	if (himax_thp_slave_spi && himax_thp_attach_slave(ts, himax_thp_slave_spi))
		dev_warn(dev, "slave present but could not be attached\n");
	mutex_unlock(&himax_thp_pair_lock);
	return 0;

err_follower:
	if (ts->has_follower)
		drm_panel_remove_follower(&ts->follower);
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

	/* 不再接受新的打开；已经打开的文件各自持着引用，之后的操作都返回 -ENODEV */
	misc_deregister(&ts->misc);

	/* Slave 设备本身留着，下次 Master probe 时再接上 */
	mutex_lock(&himax_thp_pair_lock);
	if (ts->ch[HIMAX_THP_CH_SLAVE].spi)
		himax_thp_detach_slave(ts);
	himax_thp_master = NULL;
	mutex_unlock(&himax_thp_pair_lock);

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
	if (himax_thp_is_slave(spi))
		himax_thp_slave_remove(spi);
	else
		himax_thp_master_remove(spi);
}

static const struct of_device_id himax_thp_of_match[] = {
	{ .compatible = "himax,hx83121a-ts", .data = (void *)HIMAX_THP_ROLE_MASTER },
	{ .compatible = "egotouch,hx83121a-thp-slave", .data = (void *)HIMAX_THP_ROLE_SLAVE },
	{ }
};
MODULE_DEVICE_TABLE(of, himax_thp_of_match);

static const struct spi_device_id himax_thp_spi_ids[] = {
	{ "hx83121a-ts", HIMAX_THP_ROLE_MASTER },
	{ "hx83121a-thp-slave", HIMAX_THP_ROLE_SLAVE },
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
