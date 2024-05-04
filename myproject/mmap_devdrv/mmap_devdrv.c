#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>

#define DEVICE_NAME "mmap_dev"
#define BUF_SIZE 4 * PAGE_SIZE
#define MMAP_DEV_CMD_GET_BUFSIZE 0

struct mmap_dev_t {
	struct miscdevice misc;
	char *buf;
};

static struct mmap_dev_t mmap_dev = {
	.misc = {
	.name = DEVICE_NAME,
	.minor = MISC_DYNAMIC_MINOR,
	},
	.buf = NULL,
};

static int mmap_dev_open(struct inode *inode, struct file *file)
{
	int major = MAJOR(inode->i_rdev);
	int minjor = MINOR(inode->i_rdev);
	printk("major:%d,minjor:%d\n", major, minjor);
	printk("mmap_dev_open\n");
	return 0;
}
static int mmap_dev_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t mmap_dev_read(struct file *file, char __user *buf, size_t count,
			     loff_t *ppos)
{
	int nbytes = simple_read_from_buffer(buf, count, ppos, mmap_dev.buf,
					     BUF_SIZE);
	printk("mmap_dev_read %d bytes, *ppos:%lld\n", nbytes, *ppos);
	return nbytes;
}

static ssize_t mmap_dev_write(struct file *file, const char __user *buf,
			      size_t count, loff_t *ppos)
{
	int nbytes = simple_write_to_buffer(mmap_dev.buf, BUF_SIZE, ppos, buf,
					    count);
	printk("mmap_dev_write %d bytes, *ppos:%lld\n", nbytes, *ppos);
	return nbytes;
}

static int mmap_dev_mmap(struct file *file, struct vm_area_struct *vma)
{
	printk("mmap_dev_mmap\n");
	unsigned long pfn;
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
	unsigned long len;

	len = vma->vm_end - vma->vm_start;
	printk("vma->vm_pgoff:%lu,pageoffset:%lu\n", vma->vm_pgoff, offset);
	if (offset > BUF_SIZE) {
		return -EINVAL;
	}

	if (len > (BUF_SIZE - offset)) {
		return -EINVAL;
	}

	pfn = virt_to_phys(mmap_dev.buf + offset) >> PAGE_SHIFT;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	if (remap_pfn_range(vma, vma->vm_start, pfn, len, vma->vm_page_prot)) {
		printk("remap_pfn_range failed\n");
		return -EAGAIN;
	}
	printk("%s end\n", __func__);
	return 0;
}
static long mmap_dev_ioctl(struct file *file, unsigned int ctl,
			   unsigned long arg)
{
	printk("%s start\n", __func__);
	void __user *args = (void __user *)arg;
	size_t bufsize = BUF_SIZE;
	switch (ctl) {
	default:
		return -EINVAL;
	case MMAP_DEV_CMD_GET_BUFSIZE: {
		if (copy_to_user(args, &bufsize, sizeof(bufsize)))
			return -EFAULT;
		return 0;
	}
	}
}
static const struct file_operations fops = {
	.read = mmap_dev_read,
	.write = mmap_dev_write,
	.mmap = mmap_dev_mmap,
	.open = mmap_dev_open,
	.release = mmap_dev_release,
	.unlocked_ioctl = mmap_dev_ioctl,
	.owner = THIS_MODULE,
};

int __init mmap_devdrv_init(void)
{
	int ret;
	mmap_dev.misc.fops = &fops;
	mmap_dev.buf = kmalloc(BUF_SIZE, GFP_KERNEL);
	if (mmap_dev.buf == NULL) {
		pr_err("vmalloc memory failed\n");
		return -ENOMEM;
	}
	ret = misc_register(&mmap_dev.misc);
	if (ret) {
		pr_err("misc_register failed:%d\n", ret);
		vfree(mmap_dev.buf);
		return ret;
	}

	return 0;
}

void __exit mmap_devdrv_exit(void)
{
	kfree(mmap_dev.buf);
	misc_deregister(&mmap_dev.misc);
	return;
}

module_init(mmap_devdrv_init);
module_exit(mmap_devdrv_exit);
MODULE_AUTHOR("wangshuai");
MODULE_LICENSE("GPL");