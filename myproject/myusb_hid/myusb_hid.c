#include <linux/init.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/hid.h>

static int myusb_hid_probe(struct usb_interface *intf,
			   const struct usb_device_id *id)
{
	mylog("match hid interface\n");
	return 0;
}

static void myusb_hid_disconnect(struct usb_interface *intf)
{
	mylog("called when remove module\n");
}

// match interface class is HID
static struct usb_device_id myusb_hid_ids[] = {
	{ .match_flags = USB_DEVICE_ID_MATCH_INT_CLASS,
	  .bInterfaceClass = USB_INTERFACE_CLASS_HID },
	{}
};
// interface driver
static struct usb_driver myusb_hid_driver = {
	.name = "myusb_hid",
	.probe = myusb_hid_probe,
	.disconnect = myusb_hid_disconnect,
	.id_table = myusb_hid_ids,
};
static int __init myusb_hid_init(void)
{
	printk("myusb_hid init \n");
	printk("KBUILD_MODNAME:%s,THIS_MODULE:%llu\n", KBUILD_MODNAME,
	       (uint64_t)THIS_MODULE);
	usb_register(&myusb_hid_driver);
	return 0;
}

static void __exit myusb_hid_exit(void)
{
	usb_deregister(&myusb_hid_driver);
	printk("myusb_hid exit \n");
}

module_init(myusb_hid_init);
module_exit(myusb_hid_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("myusb_hid");