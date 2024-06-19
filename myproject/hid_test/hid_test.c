#include <linux/init.h>
#include <linux/module.h>
#include <linux/hid.h>

struct hid_device *my_hid;
/*
0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
0x09, 0x06,        // Usage (Keyboard)
0xA1, 0x01,        // Collection (Application)
0x75, 0x01,        //   Report Size (1)
0x95, 0x08,        //   Report Count (8)
0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
0x19, 0xE0,        //   Usage Minimum (0xE0)
0x29, 0xE7,        //   Usage Maximum (0xE7)
0x15, 0x00,        //   Logical Minimum (0)
0x25, 0x01,        //   Logical Maximum (1)
0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x95, 0x01,        //   Report Count (1)
0x75, 0x08,        //   Report Size (8)
0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x95, 0x05,        //   Report Count (5)
0x75, 0x01,        //   Report Size (1)
0x05, 0x08,        //   Usage Page (LEDs)
0x19, 0x01,        //   Usage Minimum (Num Lock)
0x29, 0x05,        //   Usage Maximum (Kana)
0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
0x95, 0x01,        //   Report Count (1)
0x75, 0x03,        //   Report Size (3)
0x91, 0x01,        //   Output (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
0x95, 0x06,        //   Report Count (6)
0x75, 0x08,        //   Report Size (8)
0x15, 0x00,        //   Logical Minimum (0)
0x25, 0xFF,        //   Logical Maximum (-1)
0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
0x19, 0x00,        //   Usage Minimum (0x00)
0x29, 0xFF,        //   Usage Maximum (0xFF)
0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
0xC0,              // End Collection

// 63 bytes
*/
static char keyboard_report_descriptor[] = {
	0x05, 0x01, 0x09, 0x06, 0xa1, 0x01, 0x75, 0x01, 0x95, 0x08, 0x05,
	0x07, 0x19, 0xe0, 0x29, 0xe7, 0x15, 0x00, 0x25, 0x01, 0x81, 0x02,
	0x95, 0x01, 0x75, 0x08, 0x81, 0x01, 0x95, 0x05, 0x75, 0x01, 0x05,
	0x08, 0x19, 0x01, 0x29, 0x05, 0x91, 0x02, 0x95, 0x01, 0x75, 0x03,
	0x91, 0x01, 0x95, 0x06, 0x75, 0x08, 0x15, 0x00, 0x25, 0xff, 0x05,
	0x07, 0x19, 0x00, 0x29, 0xff, 0x81, 0x00, 0xc0
};

// Force Feedback device init
static int my_hid_ff_init(struct hid_device *hdev)
{
	printk("%s called\n", __func__);
	return 0;
}

static int my_hid_start(struct hid_device *hdev)
{
	printk("%s called when add hid\n",__func__);
	return 0;
}
static void my_hid_stop(struct hid_device *hdev)
{
	printk("%s called when destory hid\n",__func__);
}

static int my_hid_open(struct hid_device *hdev)
{
	printk("called when open inpput event device file\n");
	return 0;
}
static void my_hid_close(struct hid_device *hdev)
{
	printk("called when close input event device file\n");
}

// int (*power)(struct hid_device *hdev, int level);

// set report descriptor
static int my_hid_parse(struct hid_device *hdev)
{
	int ret;
	printk("%s is called when add hid\n", __func__);
	// dump_stack();
	ret = hid_parse_report(hdev, keyboard_report_descriptor,
			       sizeof(keyboard_report_descriptor));
	if (ret) {
		pr_err("parse report descriptor failed\n");
		return 0;
	}
	return 0;
}

static int my_hid_raw_request(struct hid_device *hdev, unsigned char reportnum,
			      __u8 *buf, size_t len, unsigned char rtype,
			      int reqtype)
{
	BUG_ON(1);
	return 0;
}
struct hid_ll_driver my_hid_ll_driver = {
	.start = my_hid_start,
	.stop = my_hid_stop,
	.parse = my_hid_parse,
	.open = my_hid_open,
	.close = my_hid_close,
	.raw_request = my_hid_raw_request,
};
static int __init hid_test_init(void)
{
	printk("hid_test init \n");
	my_hid = hid_allocate_device();
	if (IS_ERR(my_hid)) {
		pr_err("hid_allocate_device\n");
		return PTR_ERR(my_hid);
	}

	my_hid->bus = BUS_VIRTUAL;
	my_hid->ff_init = my_hid_ff_init;
	my_hid->ll_driver = &my_hid_ll_driver;
	strcpy(my_hid->phys, "virtual/input0");
	hid_add_device(my_hid);

	return 0;
}

static void __exit hid_test_exit(void)
{
	hid_destroy_device(my_hid);
	printk("hid_test_exit\n");
}

module_init(hid_test_init);
module_exit(hid_test_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("hid_test");