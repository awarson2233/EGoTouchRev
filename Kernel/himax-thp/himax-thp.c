// SPDX-License-Identifier: GPL-2.0
/*
 * himax-thp - SPI transport for the Himax HX83121A in host processing mode
 *
 * The driver handles the hardware only: bus primitives, frame interrupts, the
 * reset line and panel power. Chip protocol, initialization, the frame
 * reading policy and touch processing live in a userspace daemon, which uses
 * /dev/himax_thp:
 *
 *   ioctl BUS_READ / BUS_WRITE   one bus transfer on the bound channel, the
 *                                driver adds the opcode and bus header
 *   read / poll                  frames read from the frame interrupt and
 *                                queued with timestamp and sequence number
 *   other ioctls                 channel binding, frame reading on/off, reset
 *                                pulse, panel power state
 *
 * A channel is bound to one file at a time, and closing that file stops its
 * frame reading, so a daemon that dies leaves no channel reading frames.
 *
 * Master and slave (primary and secondary in the binding) are two controllers,
 * each on its own SPI bus with its own interrupt line (ACPI \_SB.THPA / THPB on
 * Gaokun3). Both use the himax,hx83121a-ts compatible and are bound by this
 * driver; the master's himax,cascade points at the slave. The master probe
 * attaches the slave as the second channel, behind a device link that unbinds
 * the master before the slave, and userspace still sees one /dev/himax_thp.
 * The reset line, the interface select line and the panel follower belong to
 * the master.
 *
 * Each channel reads its frames in its own interrupt thread: when frame
 * reading is enabled it sends [read opcode, 0x30, 0x00], reads the whole
 * frame, strips the 3 byte bus header and queues the frame for read(). The
 * interrupt trigger and SPI mode come from DT; the frame interrupt is falling
 * edge, see the comment after the frame lengths.
 *
 * The controller raises frame interrupts only in SPI mode, which it selects by
 * sampling the interface select line (GPIO174 on Gaokun3) when its firmware
 * reloads after a reset: low selects SPI, high I2C-HID. The driver drives the
 * line low at probe; all resets after that are requested by userspace, which
 * passes the pulse width to the RESET ioctl.
 *
 * The ABI is in include/uapi/linux/himax_thp.h.
 */

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/gpio/consumer.h>
#include <linux/himax_thp.h>
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

#define HIMAX_THP_NUM_CH		2

/* SPI opcodes: the first byte of every transfer */
#define HIMAX_THP_OP_WRITE_MASTER	0xf2
#define HIMAX_THP_OP_READ_MASTER	0xf3
#define HIMAX_THP_OP_WRITE_SLAVE	0xf4
#define HIMAX_THP_OP_READ_SLAVE		0xf5

/* Bus header: [opcode, cmd, dummy] for reads, [opcode, cmd] for writes */
#define HIMAX_THP_READ_HDR		3
#define HIMAX_THP_WRITE_HDR		2

/* Bus command that reads a frame: the event stack register */
#define HIMAX_THP_CMD_EVENT_STACK	0x30

/* Frame data length, not counting the bus header */
#define HIMAX_THP_MASTER_FRAME_LEN	5060
#define HIMAX_THP_SLAVE_FRAME_LEN	336

/*
 * The controller pulls its interrupt line low when a frame is ready and
 * releases it some time after the host has read the frame; it does not
 * prepare the next frame before that. So the frame interrupt has to be
 * falling edge: one edge, one read per frame. With a low level trigger the
 * line is often still low when the thread returns, the interrupt fires again
 * and reads either an empty event stack (all zero) or the next frame while
 * the controller is still writing it, and a failed read becomes an interrupt
 * storm.
 *
 * With an edge trigger a channel whose frame read failed keeps its line low
 * and raises no further edge, so it stalls. Userspace times out, enables the
 * channel again and the read that comes with it collects the frame, which
 * restarts the channel, as on Windows.
 */

struct himax_thp;

struct himax_thp_frame {
	struct himax_thp_frame_hdr hdr;
	u8 *data;			/* frame_len bytes in the channel's qbuf */
};

struct himax_thp_chan {
	struct himax_thp *ts;
	struct spi_device *spi;		/* slave: NULL without a cascade; fixed after probe */
	int irq;			/* frame interrupt, only valid with spi */
	bool irq_enabled;
	u64 irq_ns;			/* interrupt time, written by the hard irq handler */
	u8 op_write;
	u8 op_read;
	u32 frame_len;			/* frame data length, not counting the bus header */
	struct himax_thp_file *owner;	/* the file bound to this channel, under state_lock */
	bool int_enabled;		/* the owner enabled frame reading on this channel */

	/*
	 * Frame queue: count frames starting at head. When full, newly read
	 * frames are dropped, as SPBTESTTOOL does. Protected by state_lock.
	 */
	u8 *qbuf;
	struct himax_thp_frame queue[HIMAX_THP_FRAME_QUEUE];
	unsigned int head;
	unsigned int count;
	u32 seq;			/* sequence number of the next frame */
	u32 dropped;			/* frames dropped so far */
};

struct himax_thp {
	struct spi_device *spi;		/* master SPI device, owns reset, panel follower and misc */
	struct gpio_desc *reset;
	struct miscdevice misc;
	struct drm_panel_follower follower;
	bool has_follower;
	struct kref ref;		/* one for probe, one per open file */

	/* Bus: SPI transfers and their buffers. Take bus_lock before state_lock */
	struct mutex bus_lock;
	u8 *tx;
	u8 *rx;

	/* State: frame queues, switches, panel power */
	struct mutex state_lock;
	wait_queue_head_t wait;
	struct himax_thp_chan ch[HIMAX_THP_NUM_CH];
	u32 panel_state;		/* HIMAX_THP_PANEL_* */
	u64 panel_seq;			/* incremented on every panel state change */
	bool gone;			/* device unbound, file operations return -ENODEV */
};

struct himax_thp_file {
	struct himax_thp *ts;
	struct himax_thp_chan *ch;	/* bound channel, NULL before SET_CHANNEL */
	u64 seen_panel_seq;		/* last panel state sequence read by this file */
	struct mutex read_lock;		/* serializes read() on this file, which owns bounce */
	u8 *bounce;			/* frame header and data on their way to userspace */
};

static void himax_thp_release_ts(struct kref *ref)
{
	struct himax_thp *ts = container_of(ref, struct himax_thp, ref);
	int i;

	for (i = 0; i < HIMAX_THP_NUM_CH; i++)
		kfree(ts->ch[i].qbuf);
	/* The slave holds the reference from himax_thp_get_slave(), the master borrows ts->spi */
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

/*
 * A channel's frame interrupt is enabled while the panel is powered and the
 * channel is attached and reading frames. Called with state_lock held.
 */
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
			/* The interrupt thread takes state_lock too, so don't wait for it here */
			disable_irq_nosync(ch->irq);
		ch->irq_enabled = want;
	}
}

/* -------------------------------------------------------------------------- */
/* Bus primitives */

/*
 * Bus read: sends [read opcode, cmd, 0x00]; the len data bytes land at
 * ts->rx + HIMAX_THP_READ_HDR. Called with bus_lock held.
 */
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

/*
 * Bus write: sends [write opcode, cmd, data]; the len data bytes must already
 * be at ts->tx + HIMAX_THP_WRITE_HDR. Called with bus_lock held.
 */
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
/* Frame queue */

/*
 * Called with state_lock held. A full queue drops the new frame and keeps the
 * queued ones, like the SPBTESTTOOL read completion, which returns without
 * queueing when it reaches THP_LIST_MAX_FRAMES; the userspace frame reading
 * policy relies on this.
 */
static void himax_thp_enqueue_locked(struct himax_thp_chan *ch, const u8 *data, u64 ns)
{
	struct himax_thp_frame *f;

	if (ch->count == HIMAX_THP_FRAME_QUEUE) {
		/* A dropped frame still takes a sequence number, so userspace sees the gap */
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

/* Called with state_lock held */
static void himax_thp_flush_locked(struct himax_thp_chan *ch)
{
	ch->head = 0;
	ch->count = 0;
}

/* -------------------------------------------------------------------------- */
/* Frame interrupt */

static void himax_thp_read_frame(struct himax_thp_chan *ch, u64 ns)
{
	struct himax_thp *ts = ch->ts;
	bool want;
	int ret;

	/* bus_lock first: a frame read is a bus transfer, exclusive with userspace transfers */
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

/* The hard irq handler only takes the time, so a late thread does not skew the timestamp */
static irqreturn_t himax_thp_irq(int irq, void *data)
{
	struct himax_thp_chan *ch = data;

	WRITE_ONCE(ch->irq_ns, ktime_get_ns());
	return IRQ_WAKE_THREAD;
}

/* One interrupt thread per channel, reading only its own channel */
static irqreturn_t himax_thp_irq_thread(int irq, void *data)
{
	struct himax_thp_chan *ch = data;

	himax_thp_read_frame(ch, READ_ONCE(ch->irq_ns));
	wake_up_interruptible(&ch->ts->wait);

	return IRQ_HANDLED;
}

/* -------------------------------------------------------------------------- */
/* Panel power: touch and display are one TDDI chip, its touch part is off with the panel */

static void himax_thp_set_panel(struct himax_thp *ts, u32 set, u32 clear)
{
	bool off;
	int i;

	mutex_lock(&ts->state_lock);
	ts->panel_state = (ts->panel_state & ~clear) | set;
	ts->panel_seq++;
	off = !himax_thp_panel_on(ts);
	if (off) {
		/* The chip loses its state; userspace initializes it again after power up */
		for (i = 0; i < HIMAX_THP_NUM_CH; i++) {
			ts->ch[i].int_enabled = false;
			himax_thp_flush_locked(&ts->ch[i]);
		}
	}
	himax_thp_update_irq_locked(ts);
	mutex_unlock(&ts->state_lock);

	if (off) {
		/* remove() drops the follower before freeing the irqs, so they are valid here */
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
/* File operations */

static int himax_thp_open(struct inode *inode, struct file *file)
{
	struct himax_thp *ts = container_of(file->private_data, struct himax_thp, misc);
	struct himax_thp_file *f;

	f = kzalloc_obj(*f);
	if (!f)
		return -ENOMEM;
	f->bounce = kmalloc(sizeof(struct himax_thp_frame_hdr) + HIMAX_THP_BUS_MAX, GFP_KERNEL);
	if (!f->bounce) {
		kfree(f);
		return -ENOMEM;
	}

	/* Called under misc_mtx, which misc_deregister() in remove() also takes: ts is alive */
	mutex_lock(&ts->state_lock);
	if (ts->gone) {
		mutex_unlock(&ts->state_lock);
		kfree(f->bounce);
		kfree(f);
		return -ENODEV;
	}
	kref_get(&ts->ref);
	f->ts = ts;
	f->seen_panel_seq = ts->panel_seq;
	mutex_unlock(&ts->state_lock);
	mutex_init(&f->read_lock);

	file->private_data = f;
	return stream_open(inode, file);
}

/* Release the channel of f and stop its frame reading. Called with state_lock held */
static void himax_thp_unbind_locked(struct himax_thp_file *f)
{
	struct himax_thp_chan *ch = f->ch;

	if (!ch)
		return;
	ch->owner = NULL;
	ch->int_enabled = false;
	himax_thp_flush_locked(ch);
	himax_thp_update_irq_locked(f->ts);
	f->ch = NULL;
}

static int himax_thp_release(struct inode *inode, struct file *file)
{
	struct himax_thp_file *f = file->private_data;
	struct himax_thp *ts = f->ts;

	/* A daemon that exits or crashes leaves no channel reading frames behind */
	mutex_lock(&ts->state_lock);
	himax_thp_unbind_locked(f);
	mutex_unlock(&ts->state_lock);

	kref_put(&ts->ref, himax_thp_release_ts);
	mutex_destroy(&f->read_lock);
	kfree(f->bounce);
	kfree(f);
	return 0;
}

static bool himax_thp_frame_ready(struct himax_thp_file *f)
{
	struct himax_thp_chan *ch = READ_ONCE(f->ch);

	return READ_ONCE(f->ts->gone) || !ch || READ_ONCE(ch->count);
}

/* Called with f->read_lock held */
static ssize_t himax_thp_dequeue(struct file *file, char __user *buf, size_t count)
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
		ch = f->ch;
		if (!ch) {
			mutex_unlock(&ts->state_lock);
			return -EINVAL;
		}
		if (count < sizeof(fr->hdr) + ch->frame_len) {
			mutex_unlock(&ts->state_lock);
			return -EINVAL;
		}
		if (ch->count)
			break;		/* dequeue below with state_lock held */
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

static ssize_t himax_thp_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	struct himax_thp_file *f = file->private_data;
	ssize_t ret;

	if (mutex_lock_interruptible(&f->read_lock))
		return -ERESTARTSYS;
	ret = himax_thp_dequeue(file, buf, count);
	mutex_unlock(&f->read_lock);
	return ret;
}

static __poll_t himax_thp_poll(struct file *file, poll_table *wait)
{
	struct himax_thp_file *f = file->private_data;
	struct himax_thp *ts = f->ts;
	__poll_t mask = 0;

	poll_wait(file, &ts->wait, wait);

	mutex_lock(&ts->state_lock);
	if (ts->gone || !f->ch)
		mask |= EPOLLERR;
	if (ts->gone)
		mask |= EPOLLHUP;
	if (f->ch && f->ch->count)
		mask |= EPOLLIN | EPOLLRDNORM;
	if (ts->panel_seq != f->seen_panel_seq)
		mask |= EPOLLPRI;
	mutex_unlock(&ts->state_lock);

	return mask;
}

static long himax_thp_ioctl_bus(struct himax_thp_file *f, bool read, void __user *argp)
{
	struct himax_thp *ts = f->ts;
	struct himax_thp_chan *ch;
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
	mutex_lock(&ts->state_lock);
	ch = f->ch;
	ret = ts->gone ? -ENODEV : !ch ? -EINVAL : 0;
	mutex_unlock(&ts->state_lock);
	if (ret)
		goto out;
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
	struct himax_thp_chan *ch;
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
		ch = &ts->ch[val];
		mutex_lock(&ts->state_lock);
		if (ts->gone)
			ret = -ENODEV;
		else if (!ch->spi)
			ret = -ENODEV;		/* no cascaded slave */
		else if (ch->owner && ch->owner != f)
			ret = -EBUSY;
		if (!ret && ch->owner != f) {
			himax_thp_unbind_locked(f);
			ch->owner = f;
			WRITE_ONCE(f->ch, ch);
		}
		mutex_unlock(&ts->state_lock);
		break;

	case HIMAX_THP_IOC_INT_ENABLE:
		mutex_lock(&ts->state_lock);
		ch = f->ch;
		if (ts->gone) {
			ret = -ENODEV;
		} else if (!ch) {
			ret = -EINVAL;
		} else {
			ch->int_enabled = !!val;
			if (!val)
				himax_thp_flush_locked(ch);
			himax_thp_update_irq_locked(ts);
		}
		mutex_unlock(&ts->state_lock);
		/*
		 * Like SPBTESTTOOL's INT_OPEN, read one frame right away, on the
		 * same path as the interrupt. The controller raises no new
		 * interrupt before it has handed over its frame, so a frame that
		 * was ready before the channel was enabled would stall it;
		 * userspace also relies on this read when it reopens a channel
		 * that timed out.
		 */
		if (!ret && val) {
			himax_thp_read_frame(ch, ktime_get_ns());
			wake_up_interruptible(&ts->wait);
		}
		break;

	case HIMAX_THP_IOC_RESET:
		if (!ts->reset)
			return -ENODEV;
		if (val > HIMAX_THP_RESET_MAX_MS)
			return -EINVAL;
		/* Under bus_lock: no transfer runs while the controllers are in reset */
		mutex_lock(&ts->bus_lock);
		if (READ_ONCE(ts->gone)) {
			ret = -ENODEV;
		} else {
			gpiod_set_value_cansleep(ts->reset, 1);
			msleep(max_t(u32, val, HIMAX_THP_RESET_MIN_MS));
			gpiod_set_value_cansleep(ts->reset, 0);
		}
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

/*
 * Request a channel's frame interrupt with the trigger from DT. It stays
 * disabled until frame reading is enabled, see himax_thp_update_irq_locked().
 */
static int himax_thp_request_irq(struct himax_thp_chan *ch, const char *name)
{
	if (irq_get_trigger_type(ch->irq) & IRQ_TYPE_LEVEL_MASK)
		dev_warn(&ch->spi->dev, "frame interrupt is level triggered, frames may be read twice; use IRQ_TYPE_EDGE_FALLING\n");
	return request_threaded_irq(ch->irq, himax_thp_irq, himax_thp_irq_thread,
				    IRQF_ONESHOT | IRQF_NO_AUTOEN, name, ch);
}

/*
 * Attach the slave that himax,cascade points at as the second channel. The
 * slave has to be bound by this driver first, which applies its pinctrl state;
 * until then defer, and the driver core retries once it binds. The device link
 * unbinds the master before the slave, so the slave's spi device stays valid
 * while the master is bound. Without himax,cascade there is only the master
 * channel and userspace gets -ENODEV on the slave one.
 */
static int himax_thp_get_slave(struct himax_thp *ts)
{
	struct device *dev = &ts->spi->dev;
	struct himax_thp_chan *ch = &ts->ch[HIMAX_THP_CH_SLAVE];
	struct device_link *link = NULL;
	struct device_node *np;
	struct device *sdev;
	bool bound;
	int ret;

	np = of_parse_phandle(dev->of_node, "himax,cascade", 0);
	if (!np)
		return 0;
	sdev = bus_find_device_by_of_node(&spi_bus_type, np);
	of_node_put(np);
	if (!sdev)
		return dev_err_probe(dev, -EPROBE_DEFER, "cascaded slave not registered yet\n");

	/*
	 * Hold the slave's device lock so that it cannot unbind between the
	 * check and the link. Only try it: unbinding the slave takes its lock
	 * and then ours to unbind this consumer, and we already hold ours.
	 */
	if (!device_trylock(sdev)) {
		ret = dev_err_probe(dev, -EPROBE_DEFER, "cascaded slave busy\n");
		goto err_put;
	}
	bound = sdev->driver == dev->driver;
	if (bound)
		link = device_link_add(dev, sdev, DL_FLAG_AUTOREMOVE_CONSUMER);
	device_unlock(sdev);

	if (!bound) {
		ret = dev_err_probe(dev, -EPROBE_DEFER, "cascaded slave not bound yet\n");
		goto err_put;
	}
	if (!link) {
		ret = dev_err_probe(dev, -EINVAL, "cannot link to the cascaded slave\n");
		goto err_put;
	}

	ch->spi = to_spi_device(sdev);
	ch->irq = ch->spi->irq;
	return 0;

err_put:
	put_device(sdev);
	return ret;
}

/*
 * The SPI core maps the interrupt from DT before probe. If this boot already
 * mapped the same line with another trigger (for example from the I2C-HID
 * node), the mapping fails and spi->irq is 0; only a reboot changes the
 * trigger.
 */
static int himax_thp_check_irq(struct spi_device *spi)
{
	if (spi->irq <= 0)
		return dev_err_probe(&spi->dev, -EINVAL,
				     "no frame interrupt (trigger type changed since it was first mapped? reboot)\n");
	return 0;
}

/*
 * The slave binds so that the driver core applies its pinctrl state and the
 * device link orders it before its master; it checks its frame interrupt and
 * has no state of its own. The master drives both: the two controllers share
 * the reset line and the panel, and userspace initializes them as one cascade
 * through one device node.
 */
static int himax_thp_slave_probe(struct spi_device *spi)
{
	return himax_thp_check_irq(spi);
}

static int himax_thp_master_probe(struct spi_device *spi)
{
	struct device *dev = &spi->dev;
	struct himax_thp_chan *mch, *sch;
	struct gpio_desc *mode;
	struct himax_thp *ts;
	int ret;

	ret = himax_thp_check_irq(spi);
	if (ret)
		return ret;

	ts = kzalloc_obj(*ts);
	if (!ts)
		return -ENOMEM;
	kref_init(&ts->ref);
	mutex_init(&ts->bus_lock);
	mutex_init(&ts->state_lock);
	init_waitqueue_head(&ts->wait);
	ts->spi = spi_dev_get(spi);

	/* kmalloc memory is DMA safe, the stack is not */
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
	/* The master channel borrows the reference of ts->spi */
	mch->spi = spi;
	mch->irq = spi->irq;

	ret = himax_thp_get_slave(ts);
	if (ret)
		goto err_put;

	/* Logical low, out of reset: don't disturb a running chip, userspace times the reset */
	ts->reset = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ts->reset)) {
		ret = dev_err_probe(dev, PTR_ERR(ts->reset), "reset gpio\n");
		goto err_put;
	}
	/* Low selects SPI, sampled when the firmware reloads after the next reset */
	mode = devm_gpiod_get_optional(dev, "mode", GPIOD_OUT_LOW);
	if (IS_ERR(mode)) {
		ret = dev_err_probe(dev, PTR_ERR(mode), "mode gpio\n");
		goto err_put;
	}

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
		/* A powered panel calls panel_prepared (and panel_enabled) right away */
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
	dev_info(dev, "ready: /dev/%s, irq %d trigger %#x, panel %#x\n",
		 HIMAX_THP_DEVICE_NAME, mch->irq, irq_get_trigger_type(mch->irq), ts->panel_state);
	if (sch->spi)
		dev_info(dev, "cascaded slave %s: irq %d trigger %#x\n",
			 dev_name(&sch->spi->dev), sch->irq, irq_get_trigger_type(sch->irq));
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

	/* No new opens; open files keep their reference and get -ENODEV from now on */
	misc_deregister(&ts->misc);

	/* Take both locks: no transfer is in flight, and none will touch SPI afterwards */
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

/*
 * Both controllers are the same part with the same compatible; DT topology
 * gives the role. A node that another node's himax,cascade points at is the
 * slave, any other node is a master.
 */
static bool himax_thp_is_slave(struct spi_device *spi)
{
	struct device_node *np, *target;

	for_each_node_with_property(np, "himax,cascade") {
		target = of_parse_phandle(np, "himax,cascade", 0);
		of_node_put(target);
		if (np != spi->dev.of_node && target == spi->dev.of_node) {
			of_node_put(np);
			return true;
		}
	}
	return false;
}

static int himax_thp_probe(struct spi_device *spi)
{
	return himax_thp_is_slave(spi) ? himax_thp_slave_probe(spi) : himax_thp_master_probe(spi);
}

static void himax_thp_remove(struct spi_device *spi)
{
	/*
	 * Tell the roles apart by the drvdata set in probe rather than by DT,
	 * whose himax,cascade may already be gone when an overlay is removed.
	 * The slave has no drvdata and no state of its own; the device link
	 * unbinds its master first.
	 */
	if (spi_get_drvdata(spi))
		himax_thp_master_remove(spi);
}

static const struct of_device_id himax_thp_of_match[] = {
	{ .compatible = "himax,hx83121a-ts" },
	{ }
};
MODULE_DEVICE_TABLE(of, himax_thp_of_match);

static const struct spi_device_id himax_thp_spi_ids[] = {
	{ "hx83121a-ts" },
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
