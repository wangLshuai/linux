#include <linux/init.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/hid.h>

#ifndef mylog
#define mylog(fmt, ...)                                                        \
	printk("mylog----------------in func %s() file:%s:%d " fmt, __func__,  \
	       __FILE__, __LINE__, ##__VA_ARGS__)
#endif
bool find_hid_desc(char *buf, int len,
		   struct usb_descriptor_header **desc_head_pp)
{
	struct usb_descriptor_header *desc_head =
		(struct usb_descriptor_header *)buf;
	while (len > 0) {
		if (desc_head->bDescriptorType == HID_DT_HID) {
			*desc_head_pp = desc_head;
			return true;
		}
		len -= desc_head->bLength;
		desc_head = ((void *)desc_head + desc_head->bLength);
	}

	return false;
}

int usb_get_class_descriptor(struct usb_device *dev, int intf_num, int type,
			     char *buf, int len)
{
	int retry, result;
	memset(buf, 0, len);

	retry = 4;

	do {
		result = usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
					 USB_REQ_GET_DESCRIPTOR,
					 USB_RECIP_INTERFACE | USB_DIR_IN,
					 type << 8, intf_num, buf, len,
					 USB_CTRL_GET_TIMEOUT);
		retry--;
	} while (retry && result < len);

	return result;
}
void show_report_descriptor(struct usb_interface *intf)
{
	int num_alt, i, len, result;
	struct usb_descriptor_header *desc_head;
	struct hid_descriptor *hdesc;
	num_alt = intf->num_altsetting;
	printk("interface alter num:%d\n", num_alt);
	for (i = 0; i < num_alt; i++) {
		//usb_get_extra_descriptor(HID_DT_HID)
		if (find_hid_desc(intf->altsetting[i].extra,
				  intf->altsetting[i].extralen, &desc_head))
			break;
	}

	printk("is HID_DT_HID:%d\n", desc_head->bDescriptorType == HID_DT_HID);
	hdesc = (struct hid_descriptor *)desc_head;
	for (i = 0; i < hdesc->bNumDescriptors; i++) {
		if (hdesc->desc[i].bDescriptorType == HID_DT_REPORT) {
			len = le16_to_cpu(hdesc->desc[i].wDescriptorLength);
			char *buf = kmalloc(len, GFP_KERNEL);
			if (buf == NULL) {
				pr_err("malloc failed");
				return;
			}
			printk("find report descriptor,len:%d", len);
			result = usb_get_class_descriptor(
				interface_to_usbdev(intf),
				intf->cur_altsetting->desc.bInterfaceNumber,
				HID_DT_REPORT, buf, len);
			if (result == len) {
				printk("report descriptor:\n");
				print_hex_dump(KERN_INFO, "", DUMP_PREFIX_NONE,
					       16, 1, buf, len, false);
			}
			kfree(buf);

			return;
		}
	}
}
static int myusb_hid_probe(struct usb_interface *intf,
			   const struct usb_device_id *id)
{
	struct usb_device *usb_d;
	char buf[128];
	mylog("match hid interface\n");

	usb_d = interface_to_usbdev(intf);
	usb_string(usb_d, usb_d->descriptor.iManufacturer, buf, sizeof(buf));
	printk("Manufacturer:%s", buf);
	usb_string(usb_d, usb_d->descriptor.iProduct, buf, sizeof(buf));
	printk("Product:%s\n", buf);
	usb_string(usb_d, usb_d->descriptor.iSerialNumber, buf, sizeof(buf));
	printk("SerialNumber:%s\n", buf);

	show_report_descriptor(intf);

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