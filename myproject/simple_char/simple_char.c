#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kfifo.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/poll.h>

#define NAME "simple_char"
struct simple_char {
	char name[64];
	struct miscdevice misc;
	wait_queue_head_t read_queue;
	wait_queue_head_t write_queue;
	DECLARE_KFIFO(fifo, char, 64);
	struct fasync_struct *fasync;
};

#define SIMPLE_CHAR_DEVICE_NUMS 8

static struct simple_char simple_char_devs[SIMPLE_CHAR_DEVICE_NUMS];
static int simple_char_open(struct inode *inode, struct file *file)
{
	int i;
	int major = MAJOR(inode->i_rdev);
	int minor = MINOR(inode->i_rdev);

	printk("%s: major=%d,minor=%d\n", NAME, major, minor);

	for (i = 0; i < SIMPLE_CHAR_DEVICE_NUMS; i++) {
		if (minor ==
		    MINOR(simple_char_devs[i].misc.this_device->devt)) {
			file->private_data = &simple_char_devs[i];
		}
	}
	return 0;
}
static int simple_char_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t simple_char_read(struct file *file, char __user *buf,
				size_t count, loff_t *pos)
{
	struct simple_char *simple_char_dev = file->private_data;
	int ret = 0;
	if ((file->f_flags & O_NONBLOCK) &&
	    kfifo_is_empty(&(simple_char_dev->fifo))) {
		return -EAGAIN;
	}

	if ((ret = wait_event_interruptible(
		     simple_char_dev->read_queue,
		     !kfifo_is_empty(&simple_char_dev->fifo))))
		return ret;

	int actual_len;
	ret = kfifo_to_user(&simple_char_dev->fifo, buf, count, &actual_len);
	if (ret)
		return -EIO;

	if (!kfifo_is_full(&simple_char_dev->fifo)) {
		wake_up_interruptible(&simple_char_dev->write_queue);
	}

	return actual_len;
}
static ssize_t simple_char_write(struct file *file, const char __user *buf,
				 size_t count, loff_t *pos)
{
	struct simple_char *simple_char_dev = file->private_data;
	int ret = 0;
	if ((file->f_flags & O_NONBLOCK) &&
	    kfifo_is_full(&simple_char_dev->fifo)) {
		return -EAGAIN;
	}
	if ((ret = wait_event_interruptible(
		     simple_char_dev->write_queue,
		     !kfifo_is_full(&simple_char_dev->fifo))))
		return ret;
	ssize_t free_space = kfifo_size(&simple_char_dev->fifo) -
			     kfifo_len(&simple_char_dev->fifo);
	count = count > free_space ? free_space : count;
	int actual_len;
	ret = kfifo_from_user(&simple_char_dev->fifo, buf, count, &actual_len);
	if (ret)
		return -EIO;

	if (!kfifo_is_empty(&simple_char_dev->fifo)) {
		wake_up_interruptible(&simple_char_dev->read_queue);
		kill_fasync(&simple_char_dev->fasync, SIGIO, POLL_IN);
	}
	return actual_len;
}
static unsigned int simple_char_poll(struct file *file, poll_table *wait)
{
	int mask = 0;
	struct simple_char *simple_char_dev = file->private_data;
	printk("%s:minor:%d\n", __func__,
	       MINOR(simple_char_dev->misc.this_device->devt));
	poll_wait(file, &simple_char_dev->read_queue, wait);
	poll_wait(file, &simple_char_dev->write_queue, wait);

	if (!kfifo_is_empty(&simple_char_dev->fifo))
		mask |= POLLIN | POLLRDNORM;
	if (!kfifo_is_full(&simple_char_dev->fifo))
		mask |= POLLOUT | POLLWRNORM;
	printk("%s:return mask:%x\n", __func__, mask);
	return mask;
}

static int simple_char_fasync(int fd, struct file *file, int on)
{
	printk("%s\n", __func__);
	struct simple_char *simple_char_dev = file->private_data;
	return fasync_helper(fd, file, on, &simple_char_dev->fasync);
}
static const struct file_operations simple_char_drv_fops = {
	.open = simple_char_open,
	.release = simple_char_release,
	.read = simple_char_read,
	.write = simple_char_write,
	.poll = simple_char_poll,
	.fasync = simple_char_fasync,
};

static int __init multiplexing_init(void)
{
	int ret, i;
	printk("simple char that support multiplexing init \n");

	for (i = 0; i < SIMPLE_CHAR_DEVICE_NUMS; i++) {
		init_waitqueue_head(&simple_char_devs[i].read_queue);
		init_waitqueue_head(&simple_char_devs[i].write_queue);
		INIT_KFIFO(simple_char_devs[i].fifo);
		printk("fifo is full:%d\n",
		       kfifo_is_full(&simple_char_devs[i].fifo));
		printk("fifo size:%d\n", kfifo_size(&simple_char_devs[i].fifo));
		simple_char_devs[i].misc.fops = &simple_char_drv_fops;
		snprintf(simple_char_devs[i].name, 64, "%s%d", NAME, i);
		simple_char_devs[i].misc.minor = MISC_DYNAMIC_MINOR;
		simple_char_devs[i].misc.name = simple_char_devs[i].name;
		ret = misc_register(&simple_char_devs[i].misc);

		if (ret) {
			printk("failed register misc device\n");
			return ret;
		}
		printk("succeded regiser char device:%s minor:%d\n",
		       simple_char_devs[i].name,
		       MINOR(simple_char_devs[i].misc.this_device->devt));
	}

	return 0;
}

static void __exit multiplexing_exit(void)
{
	int i;
	printk("multiplexing exit\n");
	for (i = 0; i < SIMPLE_CHAR_DEVICE_NUMS; i++)
		misc_deregister(&simple_char_devs[i].misc);
}

module_init(multiplexing_init);
module_exit(multiplexing_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("multiplexing io");