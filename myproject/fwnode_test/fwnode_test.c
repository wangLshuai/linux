#include <linux/init.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/property.h>
#include <linux/platform_device.h>

int usb_device_node(struct usb_device *usb_dev, void *data)
{
	struct fwnode_handle *fwnode;
	printk("usb_device name:%s\n", usb_dev->dev.kobj.name);
	fwnode = dev_fwnode(&usb_dev->dev);
	printk("fwnode_test pointer:%lx\n", (uintptr_t)(fwnode));
	printk("usb device_node:%lx\n", (uintptr_t)usb_dev->dev.of_node);
	return 0;
}
int platform_device_node(struct device *dev, void *data)
{
	struct fwnode_handle *fwnode;
	printk("platform_device name:%s\n", dev->kobj.name);
	fwnode = dev_fwnode(dev);
	printk("fwnode_test pointer:%lx\n", (uintptr_t)(fwnode));
	printk("fwnode_test device_node:%lx\n", (uintptr_t)dev->of_node);
	return 0;
}
static int __init fwnode_test(void)
{
	struct device *dev;
	struct fwnode_handle *fwnode;
	printk("fwnode_test init \n");
	usb_for_each_dev(NULL, usb_device_node);
	bus_for_each_dev(&platform_bus_type, NULL, NULL, platform_device_node);

	dev = bus_find_device_by_name(&platform_bus_type, NULL, "fe200000.gpio");
	if (dev)
	{
		fwnode = dev_fwnode(dev);
		if(fwnode)
		{
			pr_info("test");
		}
	}
	return 0;
}

static void __exit fwnode_test_exit(void)
{
	printk("fwnode_test exit\n");
}

module_init(fwnode_test);
module_exit(fwnode_test_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("fwnode_test");