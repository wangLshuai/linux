#include <linux/init.h>
#include <linux/module.h>
#include <linux/hid.h>
#include <linux/proc_fs.h>

static struct hid_device *my_hid;
static struct proc_dir_entry *hid_proc;
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
// KEY_A  00 00 04 00 00 00 00 00
// KEY_Z  00 00 1d 00 00 00 00 00
// KEY_LEFTSHIFT 02 00 00 00 00 00 00 00
// release 00 00 00 00 00 00 00 00
static char report[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
// Force Feedback device init
static int my_hid_ff_init(struct hid_device *hdev)
{
	printk("%s called\n", __func__);
	return 0;
}

static int my_hid_start(struct hid_device *hdev)
{
	printk("%s called when add hid\n", __func__);
	return 0;
}
static void my_hid_stop(struct hid_device *hdev)
{
	printk("%s called when destory hid\n", __func__);
}

static int my_hid_open(struct hid_device *hdev)
{
	printk("called when open input event device file\n");
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
static struct hid_ll_driver my_hid_ll_driver = {
	.start = my_hid_start,
	.stop = my_hid_stop,
	.parse = my_hid_parse,
	.open = my_hid_open,
	.close = my_hid_close,
	.raw_request = my_hid_raw_request,
};

static ssize_t hid_test_read(struct file *file, char __user *buf, size_t count,
			     loff_t *pos)
{
	char *s = "please input a-z or A-Z\n";
	return simple_read_from_buffer(buf, count, pos, s, strlen(s));
}

static ssize_t hid_test_write(struct file *file, const char __user *buf,
			      size_t count, loff_t *pos)
{
	ssize_t ret;
	int i;
	char key_buf[64] = { 0 };
	ret = simple_write_to_buffer(key_buf, sizeof(key_buf), pos, buf, count);

	for (i = 0; i < ret; i++) {
		if (key_buf[i] >= 'A' && key_buf[i] <= 'Z') {
			report[3] = key_buf[i] - 'A' + 0x04;
			report[0] = 0x02;
		}
		if (key_buf[i] >= 'a' && key_buf[i] <= 'z') {
			report[3] = key_buf[i] - 'a' + 0x04;
		}
		hid_input_report(my_hid, HID_INPUT_REPORT, report,
				 sizeof(report), 0);
		report[0] = 0x00;
	}
	report[3] = 0x00;
	hid_input_report(my_hid, HID_INPUT_REPORT, report, sizeof(report), 0);
	*pos = 0;
	return ret;
}
static struct file_operations fops = { .read = hid_test_read,
				       .write = hid_test_write };
static int __init hid_test_init(void)
{
	int ret;
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
	ret = hid_add_device(my_hid);
	if (ret) {
		hid_destroy_device(my_hid);
		return ret;
	}

	hid_proc = proc_create("hid_test", 0, NULL, &fops);
	if (IS_ERR(hid_proc)) {
		return PTR_ERR(hid_proc);
	}
	return 0;
}

static void __exit hid_test_exit(void)
{
	hid_destroy_device(my_hid);
	proc_remove(hid_proc);
	printk("hid_test_exit\n");
}

module_init(hid_test_init);
module_exit(hid_test_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("hid_test");