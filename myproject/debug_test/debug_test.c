#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>

static char *str = "hello,world\n";
static char test_attr[100] = "hello,world";
static struct proc_dir_entry *my_proc;
static struct kobject *my_sys;
static struct dentry *debugfs_dir, *debugfs_file, *debugfs_data;
static u64 data;

static ssize_t my_attribute_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, test_attr);
}
static ssize_t my_attribute_store(struct kobject *kobj,
				  struct kobj_attribute *attr, const char *buf,
				  size_t count)
{
	if (count > 99) {
		count = 99;
		test_attr[99] = 0;
	}
	strncpy(test_attr, buf, count);
	return count;
}
static struct kobj_attribute my_attribute = {
	.attr = { .name = "my_attribute", .mode = 0644 },
	.show = my_attribute_show,
	.store = my_attribute_store,

};

static struct attribute *attrs[] = {
	&my_attribute.attr,
	NULL,
};

static struct attribute_group my_group = {
	.name = "attr_group",
	.attrs = attrs,
};

static int debug_test_open(struct inode *inode, struct file *file)
{
	mylog("enter");
	return 0;
}
static ssize_t debug_test_read(struct file *fp, char __user *buf, size_t count,
			       loff_t *p)
{
	size_t slen;
	slen = strlen(str);
	pr_debug("count:%lu\n", count);
	return simple_read_from_buffer(buf, count, p, str, slen);
}
static struct file_operations pdebug_test_operations = {
	.owner = THIS_MODULE,
	.open = debug_test_open,
	.read = debug_test_read,
};

static int __init debug_test_init(void)
{
	int ret = 0;
	printk("debug_test init  ==============\n");
	printk("create my_proc in /proc filesystem\n");
	my_proc = proc_create("my_proc", 0, NULL, &pdebug_test_operations);

	printk("create test in sysfs");
	my_sys = kobject_create_and_add("test", NULL);
	if (!my_sys)
		printk("koject_create_and_add failed\n");
	ret = sysfs_create_group(my_sys, &my_group);

	printk("create test/file test/data in debugfs filesystem\n");
	debugfs_dir = debugfs_create_dir("test", NULL);
	debugfs_file =
		debugfs_create_file("file", S_IRUSR | S_IWUSR, debugfs_dir,
				    NULL, &pdebug_test_operations);
	debugfs_data = debugfs_create_x64("data", S_IRUSR | S_IWUSR,
					  debugfs_dir, &data);
	return 0;
}

static void __exit debug_test_exit(void)
{
	debugfs_remove(debugfs_data);
	debugfs_remove(debugfs_file);
	debugfs_remove(debugfs_data);
	sysfs_remove_group(my_sys, &my_group);
	kobject_del(my_sys);
	proc_remove(my_proc);
}

module_init(debug_test_init);
module_exit(debug_test_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("debug_test io");