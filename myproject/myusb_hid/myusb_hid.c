#include <linux/init.h>
#include <linux/module.h>

static int __init myusb_hid_init(void)
{
	printk("myusb_hid init \n");
	return 0;
}

static void __exit myusb_hid_exit(void)
{
	printk("myusb_hid exit \n");
}

module_init(myusb_hid_init);
module_exit(myusb_hid_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("myusb_hid");