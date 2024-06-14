
## hub 初始化
hub设备初始化时初始化了workqueue hub_event()和注册了中断事务urb回调函数hub_irq()。
```c
static int hub_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
    //初始化workqueue,被调度的函数是hub_event
	INIT_WORK(&hub->events, hub_event);

	if (hub_configure(hub, &desc->endpoint[0].desc) >= 0)
		return 0;

	hub_disconnect(intf);
	return -ENODEV;
}

static int hub_configure(struct usb_hub *hub,
	struct usb_endpoint_descriptor *endpoint)
{

    // 填充回调函数hub_irq
	usb_fill_int_urb(hub->urb, hdev, pipe, *hub->buffer, maxp, hub_irq,
		hub, endpoint->bInterval);


	hub_activate(hub, HUB_INIT);
	return 0;

fail:
	dev_err(hub_dev, "config failed, %s (err %d)\n",
			message, ret);
	/* hub_disconnect() frees urb and descriptor */
	return ret;
}
```
一个usb设备，比如usb hid,插入hub时，hub接口的usb信号线电平被改变，hub接收到插入事件。
usb主控制器hcd(host control device)，比如xhci会定时遍历usb总线下的所有设备，当它要求usb hub input时，hcd得到一个input，触发hcd的中断,匹配地址和urb,然后延时调用tasklet,在softirq里调用hub驱动注册的回调函数hub_irq().

[    5.054512] Call Trace:
[    5.054989]  <IRQ>
[    5.054989]  dump_stack+0x50/0x70
[    5.054989]  hub_irq.cold+0x42/0xa7
[    5.054989]  __usb_hcd_giveback_urb+0x77/0xe0
[    5.054989]  usb_giveback_urb_bh+0x90/0xe0
[    5.054989]  tasklet_action_common.isra.0+0x4a/0x110
[    5.054989]  __do_softirq+0x10f/0x31c
[    5.054989]  irq_exit+0x93/0xb0
[    5.054989]  do_IRQ+0x82/0x140
[    5.054989]  common_interrupt+0xf/0xf
[    5.054989]  </IRQ>

```c
static void hub_irq(struct urb *urb)
{

	switch (status) {
	case -ENOENT:		/* synchronous unlink */
	case -ECONNRESET:	/* async unlink */
	case -ESHUTDOWN:	/* hardware going away */
		return;

	default:		/* presumably an error */
		/* Cause a hub reset after 10 consecutive errors */
		dev_dbg(hub->intfdev, "transfer --> %d\n", status);
		if ((++hub->nerrors < 10) || hub->error)
			goto resubmit;
		hub->error = status;
		/* FALL THROUGH */

	/* let hub_wq handle things */
	case 0:			/* we got data:  port status changed */
		bits = 0;
		for (i = 0; i < urb->actual_length; ++i)
			bits |= ((unsigned long) ((*hub->buffer)[i]))
					<< (i*8);
		hub->event_bits[0] = bits;
		break;
	}

	hub->nerrors = 0;

	/* Something happened, let hub_wq figure it out */
	kick_hub_wq(hub);

resubmit:
	hub_resubmit_irq_urb(hub);
}

void usb_kick_hub_wq(struct usb_device *hdev)
{
	struct usb_hub *hub = usb_hub_to_struct_hub(hdev);

	if (hub)
		kick_hub_wq(hub);
}

static void kick_hub_wq(struct usb_hub *hub)
{
	struct usb_interface *intf;

	if (hub->disconnected || work_pending(&hub->events))
		return;


	INIT_WORK(&hub->events, hub_event);
    // 此时在软中断上下文中，延时调度workqueue hub->events,实际上是在一个内核线程里调用hub_event函数
	if (queue_work(hub_wq, &hub->events))
		return;

	/* the work has already been scheduled */
	usb_autopm_put_interface_async(intf);
	kref_put(&hub->kref, hub_release);
}
```

hub_event函数处理插入事件，最终创建usb-hid设备

调用栈

hid_generic_probe(struct hid_device * hdev, const struct hid_device_id * id) (/home/wang/program/linux/drivers/hid/hid-generic.c:61)
hid_device_probe(struct device * dev) (/home/wang/program/linux/drivers/hid/hid-core.c:2212)
really_probe(struct device * dev, struct device_driver * drv) (/home/wang/program/linux/drivers/base/dd.c:548)
driver_probe_device(struct device_driver * drv, struct device * dev) (/home/wang/program/linux/drivers/base/dd.c:721)
bus_for_each_drv(struct bus_type * bus, struct device_driver * start, void * data, int (*)(struct device_driver *, void *) fn) (/home/wang/program/linux/drivers/base/bus.c:430)
__device_attach(struct device * dev, bool allow_async) (/home/wang/program/linux/drivers/base/dd.c:894)
device_initial_probe(struct device * dev) (/home/wang/program/linux/drivers/base/dd.c:941)
bus_probe_device(struct device * dev) (/home/wang/program/linux/drivers/base/bus.c:490)
device_add(struct device * dev) (/home/wang/program/linux/drivers/base/core.c:2202)
hid_add_device() (/home/wang/program/linux/drivers/hid/hid-core.c:2368)
hid_add_device(struct hid_device * hdev) (/home/wang/program/linux/drivers/hid/hid-core.c:2317)
usbhid_probe(struct usb_interface * intf, const struct usb_device_id * id) (/home/wang/program/linux/drivers/hid/usbhid/hid-core.c:1389)
usb_probe_interface(struct device * dev) (/home/wang/program/linux/drivers/usb/core/driver.c:361)
really_probe(struct device * dev, struct device_driver * drv) (/home/wang/program/linux/drivers/base/dd.c:552)
driver_probe_device(struct device_driver * drv, struct device * dev) (/home/wang/program/linux/drivers/base/dd.c:721)
bus_for_each_drv(struct bus_type * bus, struct device_driver * start, void * data, int (*)(struct device_driver *, void *) fn) (/home/wang/program/linux/drivers/base/bus.c:430)
__device_attach(struct device * dev, bool allow_async) (/home/wang/program/linux/drivers/base/dd.c:894)
device_initial_probe(struct device * dev) (/home/wang/program/linux/drivers/base/dd.c:941)
bus_probe_device(struct device * dev) (/home/wang/program/linux/drivers/base/bus.c:490)
device_add(struct device * dev) (/home/wang/program/linux/drivers/base/core.c:2202)
usb_set_configuration(struct usb_device * dev, int configuration) (/home/wang/program/linux/drivers/usb/core/message.c:2026)
generic_probe(struct usb_device * udev) (/home/wang/program/linux/drivers/usb/core/generic.c:210)
really_probe(struct device * dev, struct device_driver * drv) (/home/wang/program/linux/drivers/base/dd.c:552)
driver_probe_device(struct device_driver * drv, struct device * dev) (/home/wang/program/linux/drivers/base/dd.c:721)
bus_for_each_drv(struct bus_type * bus, struct device_driver * start, void * data, int (*)(struct device_driver *, void *) fn) (/home/wang/program/linux/drivers/base/bus.c:430)
__device_attach(struct device * dev, bool allow_async) (/home/wang/program/linux/drivers/base/dd.c:894)
device_initial_probe(struct device * dev) (/home/wang/program/linux/drivers/base/dd.c:941)
bus_probe_device(struct device * dev) (/home/wang/program/linux/drivers/base/bus.c:490)
device_add(struct device * dev) (/home/wang/program/linux/drivers/base/core.c:2202)
usb_new_device(struct usb_device * udev) (/home/wang/program/linux/drivers/usb/core/hub.c:2536)
hub_port_connect(u16 portstatus) (/home/wang/program/linux/drivers/usb/core/hub.c:5101)
hub_port_connect_change() (/home/wang/program/linux/drivers/usb/core/hub.c:5216)
port_event() (/home/wang/program/linux/drivers/usb/core/hub.c:5362)
hub_event(struct work_struct * work) (/home/wang/program/linux/drivers/usb/core/hub.c:5442)
process_one_work(struct worker * worker, struct work_struct * work) (/home/wang/program/linux/kernel/workqueue.c:2269)
worker_thread(void * __worker) (/home/wang/program/linux/kernel/workqueue.c:2415)
kthread(void * _create) (/home/wang/program/linux/kernel/kthread.c:255)
ret_from_fork() (/home/wang/program/linux/arch/x86/entry/entry_64.S:352)
[Unknown/Just-In-Time compiled code] (Unknown Source:0)
```c
static void hub_event(struct work_struct *work)
{
	struct usb_device *hdev;
	struct usb_interface *intf;
	struct usb_hub *hub;
	struct device *hub_dev;
	u16 hubstatus;
	u16 hubchange;
	int i, ret;

	// work参数是一个hub的events内部成员，根据地址偏移得到有事件的hub
	hub = container_of(work, struct usb_hub, events);


	/* deal with port status changes */
	for (i = 1; i <= hdev->maxchild; i++) {
		struct usb_port *port_dev = hub->ports[i - 1];

		if (test_bit(i, hub->event_bits)
				|| test_bit(i, hub->change_bits)
				|| test_bit(i, hub->wakeup_bits)) {
			/*
			 * The get_noresume and barrier ensure that if
			 * the port was in the process of resuming, we
			 * flush that work and keep the port active for
			 * the duration of the port_event().  However,
			 * if the port is runtime pm suspended
			 * (powered-off), we leave it in that state, run
			 * an abbreviated port_event(), and move on.
			 */
			pm_runtime_get_noresume(&port_dev->dev);
			pm_runtime_barrier(&port_dev->dev);
			usb_lock_port(port_dev);
			// 处理端口事件
			port_event(hub, i);
			usb_unlock_port(port_dev);
			pm_runtime_put_sync(&port_dev->dev);
		}
	}

	

}

```

## 初始化过程
* 1,hub_port_connect函数分配usb_device,并分配地址

```c
static void hub_port_connect(struct usb_hub *hub, int port1, u16 portstatus,
		u16 portchange)
{

	for (i = 0; i < SET_CONFIG_TRIES; i++) {

		/* reallocate for each attempt, since references
		 * to the previous one can escape in various ways
		 */
		// 分配一个usb_device
		udev = usb_alloc_dev(hdev, hdev->bus, port1);
		if (!udev) {
			dev_err(&port_dev->dev,
					"couldn't allocate usb_device\n");
			goto done;
		}


usb_get_configuration
		/* reset (non-USB 3.0 devices) and get descriptor */
		usb_lock_port(port_dev);
		// 调用hub_set_address()==>>xhci_setup_device.分配一个未用的地址并保存在udev->devaddr成员。
		status = hub_port_init(hub, udev, port1, i);
		usb_unlock_port(port_dev);
		if (status < 0)
			goto loop;


		/* Run it through the hoops (find a driver, etc) */
		if (!status) {
			// 添加并设置设备
			status = usb_new_device(udev);
			if (status) {
				mutex_lock(&usb_port_peer_mutex);
				spin_lock_irq(&device_state_lock);
				port_dev->child = NULL;
				spin_unlock_irq(&device_state_lock);
				mutex_unlock(&usb_port_peer_mutex);
			} else {
				if (hcd->usb_phy && !hdev->parent)
					usb_phy_notify_connect(hcd->usb_phy,
							udev->speed);
			}
		}

		if (status)
			goto loop_disable;


}

``
* 2,usb_new_device()调用usb_enumerate_device()枚举设备 得到设备描述符，调用device_add()注册一个usb device到总线.

```c
int usb_new_device(struct usb_device *udev)
{

	// 枚举设备
	err = usb_enumerate_device(udev);	/* Read descriptors */
	if (err < 0)
		goto fail;
	dev_dbg(&udev->dev, "udev %d, busnum %d, minor = %d\n",
			udev->devnum, udev->bus->busnum,
			(((udev->bus->busnum-1) * 128) + (udev->devnum-1)));
	/* export the usbdev device-node for libusb */
	udev->dev.devt = MKDEV(USB_DEVICE_MAJOR,
			(((udev->bus->busnum-1) * 128) + (udev->devnum-1)));

	/* Tell the world! */

	/* check whether the hub or firmware marks this port as non-removable */
	if (udev->parent)
		set_usb_port_removable(udev);

	/* Register the device.  The device driver is responsible
	 * for configuring the device and invoking the add-device
	 * notifier chain (used by usbfs and possibly others).
	 */ 
	// 添加usb_device到usb_bus的list中
	err = device_add(&udev->dev);
	if (err) {
		dev_err(&udev->dev, "can't device_add, error %d\n", err);
		goto fail;
	}

	/* Create link files between child device and usb port device. */
	if (udev->parent) {
		struct usb_hub *hub = usb_hub_to_struct_hub(udev->parent);
		int port1 = udev->portnum;
		struct usb_port	*port_dev = hub->ports[port1 - 1];

		err = sysfs_create_link(&udev->dev.kobj,
				&port_dev->dev.kobj, "port");
		if (err)
			goto fail;

		err = sysfs_create_link(&port_dev->dev.kobj,
				&udev->dev.kobj, "device");
		if (err) {
			sysfs_remove_link(&udev->dev.kobj, "port");
			goto fail;
		}

		if (!test_and_set_bit(port1, hub->child_usage_bits))
			pm_runtime_get_sync(&port_dev->dev);
	}

	(void) usb_create_ep_devs(&udev->dev, &udev->ep0, udev);
	usb_mark_last_busy(udev);
	pm_runtime_put_sync_autosuspend(&udev->dev);
	return err;

}

```

在usb_enumerate_device()函数中调用usb_get_configuration得到配置描述符,配置描述符集。
解析配置描述符，接口描述符，端点描述符和extra描述符，保存在usb_device中

```c
int usb_get_configuration(struct usb_device *dev)
{
	struct device *ddev = &dev->dev;
	// 设备描述符在之前的hub_port_init得到
	int ncfg = dev->descriptor.bNumConfigurations;
	int result = -ENOMEM;
	unsigned int cfgno, length;
	unsigned char *bigbuffer;
	struct usb_config_descriptor *desc;

	if (ncfg > USB_MAXCONFIG) {
		dev_warn(ddev, "too many configurations: %d, "
		    "using maximum allowed: %d\n", ncfg, USB_MAXCONFIG);
		dev->descriptor.bNumConfigurations = ncfg = USB_MAXCONFIG;
	}

	if (ncfg < 1) {
		dev_err(ddev, "no configurations\n");
		return -EINVAL;
	}

	length = ncfg * sizeof(struct usb_host_config);

	dev->config = kzalloc(length, GFP_KERNEL);
	if (!dev->config)
		goto err2;

	length = ncfg * sizeof(char *);
	// ncfg个配置描述符集的指针数组的空间
	dev->rawdescriptors = kzalloc(length, GFP_KERNEL);
	if (!dev->rawdescriptors)
		goto err2;
	
	// 一个配置描述符缓存
	desc = kmalloc(USB_DT_CONFIG_SIZE, GFP_KERNEL);
	if (!desc)
		goto err2;

	for (cfgno = 0; cfgno < ncfg; cfgno++) {
		/* We grab just the first descriptor so we know how long
		 * the whole configuration is */
		 // 读取低cfgno个配置描述符
		result = usb_get_descriptor(dev, USB_DT_CONFIG, cfgno,
		    desc, USB_DT_CONFIG_SIZE);
		if (result < 0) {
			dev_err(ddev, "unable to read config index %d "
			    "descriptor/%s: %d\n", cfgno, "start", result);
			if (result != -EPIPE)
				goto err;
			dev_err(ddev, "chopping to %d config(s)\n", cfgno);
			dev->descriptor.bNumConfigurations = cfgno;
			break;
		} else if (result < 4) {
			dev_err(ddev, "config index %d descriptor too short "
			    "(expected %i, got %i)\n", cfgno,
			    USB_DT_CONFIG_SIZE, result);
			result = -EINVAL;
			goto err;
		}
		length = max((int) le16_to_cpu(desc->wTotalLength),
		    USB_DT_CONFIG_SIZE);

		/* Now that we know the length, get the whole thing */
		// 分配第cfgno个个配置描述符集的空间
		bigbuffer = kmalloc(length, GFP_KERNEL);
		if (!bigbuffer) {
			result = -ENOMEM;
			goto err;
		}

		if (dev->quirks & USB_QUIRK_DELAY_INIT)
			msleep(200);
		// 读取配置描述符集
		result = usb_get_descriptor(dev, USB_DT_CONFIG, cfgno,
		    bigbuffer, length);
		if (result < 0) {
			dev_err(ddev, "unable to read config index %d "
			    "descriptor/%s\n", cfgno, "all");
			kfree(bigbuffer);
			goto err;
		}
		if (result < length) {
			dev_warn(ddev, "config index %d descriptor too short "
			    "(expected %i, got %i)\n", cfgno, length, result);
			length = result;
		}
	// 配置描述符集的指针保存在rawdescriptors数组里
		dev->rawdescriptors[cfgno] = bigbuffer;

		// 解析配置描述符集中的接口描述符和端点描述符，保存在dev的成员变量config的desc变量里。
		// 他会调用usb_parse_interface()函数，解析interface descriptor
		// usb_parse_interface()函数会调用usb_parse_endpoint()函数，解析endpoint descriptor
		result = usb_parse_configuration(dev, cfgno,
		    &dev->config[cfgno], bigbuffer, length);
		if (result < 0) {
			++cfgno;
			goto err;
		}
	}

err:
	kfree(desc);
	dev->descriptor.bNumConfigurations = cfgno;
err2:
	if (result == -ENOMEM)
		dev_err(ddev, "out of memory\n");
	return result;
}
```


* 3，device_add添加设备会匹配generic usb_device driver。prob初始化usb设备,一个usb插槽插入的就是一个usb设备，所有的usb设备都匹配。

调用栈
usb_device_match(struct device * dev, struct device_driver * drv) (/home/wang/program/linux/drivers/usb/core/driver.c:796)
driver_match_device() (/home/wang/program/linux/drivers/base/base.h:129)
__device_attach_driver(struct device_driver * drv, void * _data) (/home/wang/program/linux/drivers/base/dd.c:808)
bus_for_each_drv(struct bus_type * bus, struct device_driver * start, void * data, int (*)(struct device_driver *, void *) fn) (/home/wang/program/linux/drivers/base/bus.c:430)
__device_attach(struct device * dev, bool allow_async) (/home/wang/program/linux/drivers/base/dd.c:894)
device_initial_probe(struct device * dev) (/home/wang/program/linux/drivers/base/dd.c:941)
bus_probe_device(struct device * dev) (/home/wang/program/linux/drivers/base/bus.c:490)
device_add(struct device * dev) (/home/wang/program/linux/drivers/base/core.c:2202)
usb_new_device(struct usb_device * udev) (/home/wang/program/linux/drivers/usb/core/hub.c:2536)


```c
static int usb_device_match(struct device *dev, struct device_driver *drv)
{
	/* devices and interfaces are handled separately */
	// 是否是usb_device,usb总线上device list有两种设备，interface和device.device->type == usb_device_type
	if (is_usb_device(dev)) {

		/* interface drivers never match devices */
		// 是不是device驱动，如果不是，返回0.不匹配
		if (!is_usb_device_driver(drv))
			return 0;

		/* TODO: Add real matching code */
		return 1;

	} else if (is_usb_interface(dev)) {  // interface device->type == usb_if_device_type
		struct usb_interface *intf;
		struct usb_driver *usb_drv;
		const struct usb_device_id *id;

		/* device drivers never match interfaces */
		if (is_usb_device_driver(drv))
			return 0;

		intf = to_usb_interface(dev);
		usb_drv = to_usb_driver(drv);

		// 如果是interface 就要匹配id,因为有很多种interface driver。
		id = usb_match_id(intf, usb_drv->id_table);
		if (id)
			return 1;

		id = usb_match_dynamic_id(intf, usb_drv);
		if (id)
			return 1;
	}

	return 0;
}

```

* 4, 一个usb 设备插入后，初始化并注册到总线上的是一个usb device.
所以匹配到usb_device驱动，调用usb_probe去继续初始化设备。得到配置描述符，选择一个配置描述符，usb可以有多个配置描述符，每一个配置描述符可以认为是一种功能集合，最终只能选择一个功能集合。

```c
// drivers/usb/core/generic.c
static int generic_probe(struct usb_device *udev)
{
	int err, c;

	/* Choose and set the configuration.  This registers the interfaces
	 * with the driver core and lets interface drivers bind to them.
	 */
	if (udev->authorized == 0)
		dev_err(&udev->dev, "Device is not authorized for usage\n");
	else {
		// 多个配置描述符，选择一个，返回为配置描述符号
		c = usb_choose_configuration(udev);
		if (c >= 0) {
			// 为接口描述符集中的每个接口创建interface device.
			err = usb_set_configuration(udev, c);
			if (err && err != -ENODEV) {
				dev_err(&udev->dev, "can't set config #%d, error %d\n",
					c, err);
				/* This need not be fatal.  The user can try to
				 * set other configurations. */
			}
		}
	}
	/* USB device state == configured ... usable */
	usb_notify_add_device(udev);

	return 0;
}

```

```c

int usb_set_configuration(struct usb_device *dev, int configuration)
{
	int i, ret;
	struct usb_host_config *cp = NULL;
	struct usb_interface **new_interfaces = NULL;
	struct usb_hcd *hcd = bus_to_hcd(dev->bus);
	int n, nintf;

	if (dev->authorized == 0 || configuration == -1)
		configuration = 0;
	else {
		for (i = 0; i < dev->descriptor.bNumConfigurations; i++) {
			if (dev->config[i].desc.bConfigurationValue ==
					configuration) {
				cp = &dev->config[i];
				break;
			}
		}
	}
	if ((!cp && configuration != 0))
		return -EINVAL;

	/* The USB spec says configuration 0 means unconfigured.
	 * But if a device includes a configuration numbered 0,
	 * we will accept it as a correctly configured state.
	 * Use -1 if you really want to unconfigure the device.
	 */
	if (cp && configuration == 0)
		dev_warn(&dev->dev, "config 0 descriptor??\n");

	/* Allocate memory for new interfaces before doing anything else,
	 * so that if we run out then nothing will have changed. */
	n = nintf = 0;
	if (cp) {
		nintf = cp->desc.bNumInterfaces;
		// 接口指针数组，每个成员是一个接口指针
		new_interfaces = kmalloc_array(nintf, sizeof(*new_interfaces),
					       GFP_NOIO);
		if (!new_interfaces)
			return -ENOMEM;

		for (; n < nintf; ++n) {
			// 为每个interface分配位置，保存地址在new_interfaces数组里
			new_interfaces[n] = kzalloc(
					sizeof(struct usb_interface),
					GFP_NOIO);
			if (!new_interfaces[n]) {
				ret = -ENOMEM;
free_interfaces:
				while (--n >= 0)
					kfree(new_interfaces[n]);
				kfree(new_interfaces);
				return ret;
			}
		}

		i = dev->bus_mA - usb_get_max_power(dev, cp);
		if (i < 0)
			dev_warn(&dev->dev, "new config #%d exceeds power "
					"limit by %dmA\n",
					configuration, -i);
	}

	/* Wake up the device so we can send it the Set-Config request */
	ret = usb_autoresume_device(dev);
	if (ret)
		goto free_interfaces;

	/* if it's already configured, clear out old state first.
	 * getting rid of old interfaces means unbinding their drivers.
	 */
	if (dev->state != USB_STATE_ADDRESS)
		usb_disable_device(dev, 1);	/* Skip ep0 */

	/* Get rid of pending async Set-Config requests for this device */
	cancel_async_set_config(dev);

	/* Make sure we have bandwidth (and available HCD resources) for this
	 * configuration.  Remove endpoints from the schedule if we're dropping
	 * this configuration to set configuration 0.  After this point, the
	 * host controller will not allow submissions to dropped endpoints.  If
	 * this call fails, the device state is unchanged.
	 */
	mutex_lock(hcd->bandwidth_mutex);
	/* Disable LPM, and re-enable it once the new configuration is
	 * installed, so that the xHCI driver can recalculate the U1/U2
	 * timeouts.
	 */
	if (dev->actconfig && usb_disable_lpm(dev)) {
		dev_err(&dev->dev, "%s Failed to disable LPM\n", __func__);
		mutex_unlock(hcd->bandwidth_mutex);
		ret = -ENOMEM;
		goto free_interfaces;
	}
	ret = usb_hcd_alloc_bandwidth(dev, cp, NULL, NULL);
	if (ret < 0) {
		if (dev->actconfig)
			usb_enable_lpm(dev);
		mutex_unlock(hcd->bandwidth_mutex);
		usb_autosuspend_device(dev);
		goto free_interfaces;
	}

	/*
	 * Initialize the new interface structures and the
	 * hc/hcd/usbcore interface/endpoint state.
	 */
	for (i = 0; i < nintf; ++i) {
		struct usb_interface_cache *intfc;
		struct usb_interface *intf;
		struct usb_host_interface *alt;
		u8 ifnum;

		cp->interface[i] = intf = new_interfaces[i];
		intfc = cp->intf_cache[i];
		intf->altsetting = intfc->altsetting;
		intf->num_altsetting = intfc->num_altsetting;
		intf->authorized = !!HCD_INTF_AUTHORIZED(hcd);
		kref_get(&intfc->ref);

		alt = usb_altnum_to_altsetting(intf, 0);

		/* No altsetting 0?  We'll assume the first altsetting.
		 * We could use a GetInterface call, but if a device is
		 * so non-compliant that it doesn't have altsetting 0
		 * then I wouldn't trust its reply anyway.
		 */
		if (!alt)
			alt = &intf->altsetting[0];

		ifnum = alt->desc.bInterfaceNumber;
		intf->intf_assoc = find_iad(dev, cp, ifnum);
		intf->cur_altsetting = alt;
		usb_enable_interface(dev, intf, true);
		intf->dev.parent = &dev->dev;
		if (usb_of_has_combined_node(dev)) {
			device_set_of_node_from_dev(&intf->dev, &dev->dev);
		} else {
			intf->dev.of_node = usb_of_get_interface_node(dev,
					configuration, ifnum);
		}
		intf->dev.driver = NULL;
		intf->dev.bus = &usb_bus_type;
		intf->dev.type = &usb_if_device_type;
		intf->dev.groups = usb_interface_groups;
		/*
		 * Please refer to usb_alloc_dev() to see why we set
		 * dma_mask and dma_pfn_offset.
		 */
		intf->dev.dma_mask = dev->dev.dma_mask;
		intf->dev.dma_pfn_offset = dev->dev.dma_pfn_offset;
		INIT_WORK(&intf->reset_ws, __usb_queue_reset_device);
		intf->minor = -1;
		device_initialize(&intf->dev);
		pm_runtime_no_callbacks(&intf->dev);
		dev_set_name(&intf->dev, "%d-%s:%d.%d", dev->bus->busnum,
				dev->devpath, configuration, ifnum);
		usb_get_dev(dev);
	}
	kfree(new_interfaces);

	// 用usb控制事务设置usb设备工作的配置描述符集
	ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
			      USB_REQ_SET_CONFIGURATION, 0, configuration, 0,
			      NULL, 0, USB_CTRL_SET_TIMEOUT);
	if (ret < 0 && cp) {
		/*
		 * All the old state is gone, so what else can we do?
		 * The device is probably useless now anyway.
		 */
		usb_hcd_alloc_bandwidth(dev, NULL, NULL, NULL);
		for (i = 0; i < nintf; ++i) {
			usb_disable_interface(dev, cp->interface[i], true);
			put_device(&cp->interface[i]->dev);
			cp->interface[i] = NULL;
		}
		cp = NULL;
	}

	dev->actconfig = cp;
	mutex_unlock(hcd->bandwidth_mutex);

	if (!cp) {
		usb_set_device_state(dev, USB_STATE_ADDRESS);

		/* Leave LPM disabled while the device is unconfigured. */
		usb_autosuspend_device(dev);
		return ret;
	}
	usb_set_device_state(dev, USB_STATE_CONFIGURED);

	if (cp->string == NULL &&
			!(dev->quirks & USB_QUIRK_CONFIG_INTF_STRINGS))
		cp->string = usb_cache_string(dev, cp->desc.iConfiguration);

	/* Now that the interfaces are installed, re-enable LPM. */
	usb_unlocked_enable_lpm(dev);
	/* Enable LTM if it was turned off by usb_disable_device. */
	usb_enable_ltm(dev);

	/* Now that all the interfaces are set up, register them
	 * to trigger binding of drivers to interfaces.  probe()
	 * routines may install different altsettings and may
	 * claim() any interfaces not yet bound.  Many class drivers
	 * need that: CDC, audio, video, etc.
	 */
	for (i = 0; i < nintf; ++i) {
		struct usb_interface *intf = cp->interface[i];

		// 添加本配置描述符集下的所有interface 到bus总线
		ret = device_add(&intf->dev);
		if (ret != 0) {
			dev_err(&dev->dev, "device_add(%s) --> %d\n",
				dev_name(&intf->dev), ret);
			continue;
		}
		create_intf_ep_devs(intf);
	}

	usb_autosuspend_device(dev);
	return 0;
}

```


* 5,device_add注册usb interface到总线list,遍历所有的驱动最终匹配usb-hid驱动.

```c
// drivers/hid/usbhid/hid-core.c
// 比较interface的class属性，interface->calss == USB_INTERFACE_CLASS_HID就匹配
static const struct usb_device_id hid_usb_ids[] = {
	{ .match_flags = USB_DEVICE_ID_MATCH_INT_CLASS,
		.bInterfaceClass = USB_INTERFACE_CLASS_HID },
	{ }						/* Terminating entry */
};

MODULE_DEVICE_TABLE (usb, hid_usb_ids);

static struct usb_driver hid_driver = {
	.name =		"usbhid",
	.probe =	usbhid_probe,
	.disconnect =	usbhid_disconnect,
#ifdef CONFIG_PM
	.suspend =	hid_suspend,
	.resume =	hid_resume,
	.reset_resume =	hid_reset_resume,
#endif
	.pre_reset =	hid_pre_reset,
	.post_reset =	hid_post_reset,
	.id_table =	hid_usb_ids,
	.supports_autosuspend = 1,
};

```

匹配后调用usbhid_probe函数建立并初始化usbhid设备。
```c
static int usbhid_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_host_interface *interface = intf->cur_altsetting;
	struct usb_device *dev = interface_to_usbdev(intf);
	struct usbhid_device *usbhid;
	struct hid_device *hid;
	unsigned int n, has_in = 0;
	size_t len;
	int ret;

	dbg_hid("HID probe called for ifnum %d\n",
			intf->altsetting->desc.bInterfaceNumber);

	for (n = 0; n < interface->desc.bNumEndpoints; n++)
		if (usb_endpoint_is_int_in(&interface->endpoint[n].desc))
			has_in++;
	if (!has_in) {
		hid_err(intf, "couldn't find an input interrupt endpoint\n");
		return -ENODEV;
	}
	
	hid = hid_allocate_device();
	if (IS_ERR(hid))
		return PTR_ERR(hid);

	usb_set_intfdata(intf, hid);
	// hid low level driver，hid框架操作hid时会调用这些回到函数
	hid->ll_driver = &usb_hid_driver;
	hid->ff_init = hid_pidff_init;
#ifdef CONFIG_USB_HIDDEV
	hid->hiddev_connect = hiddev_connect;
	hid->hiddev_disconnect = hiddev_disconnect;
	hid->hiddev_hid_event = hiddev_hid_event;
	hid->hiddev_report_event = hiddev_report_event;
#endif
	hid->dev.parent = &intf->dev;
	hid->bus = BUS_USB;
	hid->vendor = le16_to_cpu(dev->descriptor.idVendor);
	hid->product = le16_to_cpu(dev->descriptor.idProduct);
	hid->version = le16_to_cpu(dev->descriptor.bcdDevice);
	hid->name[0] = 0;
	if (intf->cur_altsetting->desc.bInterfaceProtocol ==
			USB_INTERFACE_PROTOCOL_MOUSE)
		hid->type = HID_TYPE_USBMOUSE;
	else if (intf->cur_altsetting->desc.bInterfaceProtocol == 0)
		hid->type = HID_TYPE_USBNONE;

	if (dev->manufacturer)
		strlcpy(hid->name, dev->manufacturer, sizeof(hid->name));

	if (dev->product) {
		if (dev->manufacturer)
			strlcat(hid->name, " ", sizeof(hid->name));
		strlcat(hid->name, dev->product, sizeof(hid->name));
	}

	if (!strlen(hid->name))
		snprintf(hid->name, sizeof(hid->name), "HID %04x:%04x",
			 le16_to_cpu(dev->descriptor.idVendor),
			 le16_to_cpu(dev->descriptor.idProduct));

	usb_make_path(dev, hid->phys, sizeof(hid->phys));
	strlcat(hid->phys, "/input", sizeof(hid->phys));
	len = strlen(hid->phys);
	if (len < sizeof(hid->phys) - 1)
		snprintf(hid->phys + len, sizeof(hid->phys) - len,
			 "%d", intf->altsetting[0].desc.bInterfaceNumber);

	if (usb_string(dev, dev->descriptor.iSerialNumber, hid->uniq, 64) <= 0)
		hid->uniq[0] = 0;

	// 保存usbhid的上下文
	usbhid = kzalloc(sizeof(*usbhid), GFP_KERNEL);
	if (usbhid == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	hid->driver_data = usbhid;
	usbhid->hid = hid;
	usbhid->intf = intf;
	usbhid->ifnum = interface->desc.bInterfaceNumber;

	init_waitqueue_head(&usbhid->wait);
	INIT_WORK(&usbhid->reset_work, hid_reset);
	timer_setup(&usbhid->io_retry, hid_retry_timeout, 0);
	spin_lock_init(&usbhid->lock);

	// hid添加到hid总线上去，他会匹配驱动并调用probe
	ret = hid_add_device(hid);
	if (ret) {
		if (ret != -ENODEV)
			hid_err(intf, "can't add hid device: %d\n", ret);
		goto err_free;
	}

	return 0;
err_free:
	kfree(usbhid);
err:
	hid_destroy_device(hid);
	return ret;
}

```

* 6, 和usb匹配的过程类似。区别是hid_bus设置了probe函数，real_probe函数使用bus->probe而不是drviver->probe。bus->probe hid_device_probe过滤了一些忽略的驱动。最终调用hid_generic_probe


```c
static int hid_generic_probe(struct hid_device *hdev,
			     const struct hid_device_id *id)
{
	int ret;

	hdev->quirks |= HID_QUIRK_INPUT_PER_APP;

	ret = hid_parse(hdev);
	if (ret)
		return ret;

	// 调用ll_driver的start usbhid_start
	return hid_hw_start(hdev, HID_CONNECT_DEFAULT);
}

static const struct hid_device_id hid_table[] = {
	{ HID_DEVICE(HID_BUS_ANY, HID_GROUP_ANY, HID_ANY_ID, HID_ANY_ID) },
	{ }
};
MODULE_DEVICE_TABLE(hid, hid_table);

static struct hid_driver hid_generic = {
	.name = "hid-generic",
	.id_table = hid_table,
	.match = hid_generic_match,
	.probe = hid_generic_probe,
};

```

```c
static int usbhid_start(struct hid_device *hid)
{
	struct usb_interface *intf = to_usb_interface(hid->dev.parent);
	struct usb_host_interface *interface = intf->cur_altsetting;
	struct usb_device *dev = interface_to_usbdev(intf);
	struct usbhid_device *usbhid = hid->driver_data;
	unsigned int n, insize = 0;
	int ret;


		ret = -ENOMEM;
		if (usb_endpoint_dir_in(endpoint)) {
			if (usbhid->urbin)
				continue;
			if (!(usbhid->urbin = usb_alloc_urb(0, GFP_KERNEL)))
				goto fail;
			pipe = usb_rcvintpipe(dev, endpoint->bEndpointAddress);
			// 填充了urb 中断传输完成回调函数hid_irq_in
			usb_fill_int_urb(usbhid->urbin, dev, pipe, usbhid->inbuf, insize,
					 hid_irq_in, hid, interval);
			usbhid->urbin->transfer_dma = usbhid->inbuf_dma;
			usbhid->urbin->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
		} else {
			if (usbhid->urbout)
				continue;
			if (!(usbhid->urbout = usb_alloc_urb(0, GFP_KERNEL)))
				goto fail;
			pipe = usb_sndintpipe(dev, endpoint->bEndpointAddress);
			usb_fill_int_urb(usbhid->urbout, dev, pipe, usbhid->outbuf, 0,
					 hid_irq_out, hid, interval);
			usbhid->urbout->transfer_dma = usbhid->outbuf_dma;
			usbhid->urbout->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
		}
	}

	
	return ret;
}
```

* 7,当/dev/input设备被用户层程序，比如Xorg打开时，由于hid设备在初始化时hidinput_allocate为input设备注册了open函数hidinput_open。所以最终将调用hidinput_open==>>hid_hw_open==>>usbhid_open==>>hid_start_in

```c

static int hid_start_in(struct hid_device *hid)
{
	unsigned long flags;
	int rc = 0;
	struct usbhid_device *usbhid = hid->driver_data;

	spin_lock_irqsave(&usbhid->lock, flags);
	if (test_bit(HID_IN_POLLING, &usbhid->iofl) &&
	    !test_bit(HID_DISCONNECTED, &usbhid->iofl) &&
	    !test_bit(HID_SUSPENDED, &usbhid->iofl) &&
	    !test_and_set_bit(HID_IN_RUNNING, &usbhid->iofl)) {
		// 提交给hcd urb,这样hcd的中断函数将在hid事件来到是调用hid_irq_in函数
		rc = usb_submit_urb(usbhid->urbin, GFP_ATOMIC);
		if (rc != 0) {
			clear_bit(HID_IN_RUNNING, &usbhid->iofl);
			if (rc == -ENOSPC)
				set_bit(HID_NO_BANDWIDTH, &usbhid->iofl);
		} else {
			clear_bit(HID_NO_BANDWIDTH, &usbhid->iofl);
		}
	}
	spin_unlock_irqrestore(&usbhid->lock, flags);
	return rc;
}


```

## 事件处理

hcd 硬件定时轮询总线设备,读取到hid interface,如果有usb 中断事务输入,则调用注册的urb回调函数hid_irq_in，这是在硬件中断上下文中。

```c

static void hid_irq_in(struct urb *urb)
{
	struct hid_device	*hid = urb->context;
	struct usbhid_device	*usbhid = hid->driver_data;
	int			status;

	switch (urb->status) {
	case 0:			/* success */
		usbhid->retry_delay = 0;
		if (!test_bit(HID_OPENED, &usbhid->iofl))
			break;
		usbhid_mark_busy(usbhid);
		if (!test_bit(HID_RESUME_RUNNING, &usbhid->iofl)) {
			mylog("hid raw data\n");
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_NONE, 16, 1,
		       urb->transfer_buffer, urb->actual_length, true);
			// 解析report,转换为input格式
			hid_input_report(urb->context, HID_INPUT_REPORT,
					 urb->transfer_buffer,
					 urb->actual_length, 1);
			/*
			 * autosuspend refused while keys are pressed
			 * because most keyboards don't wake up when
			 * a key is released
			 */
			if (hid_check_keys_pressed(hid))
				set_bit(HID_KEYS_PRESSED, &usbhid->iofl);
			else
				clear_bit(HID_KEYS_PRESSED, &usbhid->iofl);
		}
		break;
	case -EPIPE:		/* stall */
		usbhid_mark_busy(usbhid);
		clear_bit(HID_IN_RUNNING, &usbhid->iofl);
		set_bit(HID_CLEAR_HALT, &usbhid->iofl);
		schedule_work(&usbhid->reset_work);
		return;
	case -ECONNRESET:	/* unlink */
	case -ENOENT:
	case -ESHUTDOWN:	/* unplug */
		clear_bit(HID_IN_RUNNING, &usbhid->iofl);
		return;
	case -EILSEQ:		/* protocol error or unplug */
	case -EPROTO:		/* protocol error or unplug */
	case -ETIME:		/* protocol error or unplug */
	case -ETIMEDOUT:	/* Should never happen, but... */
		usbhid_mark_busy(usbhid);
		clear_bit(HID_IN_RUNNING, &usbhid->iofl);
		hid_io_error(hid);
		return;
	default:		/* error */
		hid_warn(urb->dev, "input irq status %d received\n",
			 urb->status);
	}

	status = usb_submit_urb(urb, GFP_ATOMIC);
	if (status) {
		clear_bit(HID_IN_RUNNING, &usbhid->iofl);
		if (status != -EPERM) {
			hid_err(hid, "can't resubmit intr, %s-%s/input%d, status %d\n",
				hid_to_usb_dev(hid)->bus->bus_name,
				hid_to_usb_dev(hid)->devpath,
				usbhid->ifnum, status);
			hid_io_error(hid);
		}
	}
}


```

最终hidinput_hid_event函数调用input_event填充事件，打开/dev/input/event*的程序将收到input事件

调用栈显示这在硬中断上下文中
hidinput_hid_event(struct hid_device * hid, struct hid_field * field, const struct hid_usage * usage, __s32 value) (/home/wang/program/linux/drivers/hid/hid-input.c:1383)
hid_process_event(struct hid_device * hid, struct hid_field * field, struct hid_usage * usage, __s32 value, int interrupt) (/home/wang/program/linux/drivers/hid/hid-core.c:1463)
hid_input_field() (/home/wang/program/linux/drivers/hid/hid-core.c:1507)
hid_report_raw_event(struct hid_device * hid, int type, u8 * data, u32 size, int interrupt) (/home/wang/program/linux/drivers/hid/hid-core.c:1714)
hid_input_report(struct hid_device * hid, int type, u8 * data, u32 size, int interrupt) (/home/wang/program/linux/drivers/hid/hid-core.c:1781)
hid_irq_in(struct urb * urb) (/home/wang/program/linux/drivers/hid/usbhid/hid-core.c:287)
__usb_hcd_giveback_urb(struct urb * urb) (/home/wang/program/linux/drivers/usb/core/hcd.c:1656)
usb_hcd_giveback_urb(struct usb_hcd * hcd, struct urb * urb, int status) (/home/wang/program/linux/drivers/usb/core/hcd.c:1721)
xhci_giveback_urb_in_irq(struct xhci_hcd * xhci, int status) (/home/wang/program/linux/drivers/usb/host/xhci-ring.c:656)
xhci_td_cleanup(struct xhci_hcd * xhci, struct xhci_td * td, struct xhci_ring * ep_ring, int * status) (/home/wang/program/linux/drivers/usb/host/xhci-ring.c:1937)
finish_td(struct xhci_hcd * xhci, struct xhci_td * td, struct xhci_transfer_event * event, struct xhci_virt_ep * ep, int * status) (/home/wang/program/linux/drivers/usb/host/xhci-ring.c:1987)
process_bulk_intr_td() (/home/wang/program/linux/drivers/usb/host/xhci-ring.c:2296)
handle_tx_event() (/home/wang/program/linux/drivers/usb/host/xhci-ring.c:2631)
xhci_handle_event() (/home/wang/program/linux/drivers/usb/host/xhci-ring.c:2708)
xhci_irq(struct usb_hcd * hcd) (/home/wang/program/linux/drivers/usb/host/xhci-ring.c:2810)
__handle_irq_event_percpu(struct irq_desc * desc, unsigned int * flags) (/home/wang/program/linux/kernel/irq/handle.c:149)
handle_irq_event_percpu(struct irq_desc * desc) (/home/wang/program/linux/kernel/irq/handle.c:189)
handle_irq_event(struct irq_desc * desc) (/home/wang/program/linux/kernel/irq/handle.c:206)
handle_edge_irq(struct irq_desc * desc) (/home/wang/program/linux/kernel/irq/chip.c:830)
generic_handle_irq_desc() (/home/wang/program/linux/include/linux/irqdesc.h:156)
do_IRQ(struct pt_regs * regs) (/home/wang/program/linux/arch/x86/kernel/irq.c:252)
common_interrupt() (/home/wang/program/linux/arch/x86/entry/entry_64.S:607)
[Unknown/Just-In-Time compiled code] (Unknown Source:0)
fixed_percpu_data (Unknown Source:0)
[Unknown/Just-In-Time compiled code] (Unknown Source:0)



鼠标hid report数据和input的log。report有四个字节，从0到3编号。

input_event函数参数,当type是EV_REL时表述是鼠标光标移动事件
code
#define REL_X			0x00
#define REL_Y			0x01
input value是移动量。
所以report第1个字节是有符号char值，值是X方向上的变化。
report第2个字节是Y方向上的光标变化。


/hid/hid-input.c:1383 mosue 1 34
[   48.717751] mylog----------------in func hid_irq_in() file:drivers/hid/usbhid/hid-core.c:284 hid raw data
[   48.718332] 00 f8 27 00                                      ..'.
[   48.718560] mylog----------------in func hidinput_hid_event() file:drivers/hid/hid-input.c:1383 mosue 0 -8
[   48.718575] mylog----------------in func hidinput_hid_event() file:drivers/hid/hid-input.c:1383 mosue 1 39
[   48.734472] mylog----------------in func hid_irq_in() file:drivers/hid/usbhid/hid-core.c:284 hid raw data
[   48.735103] 00 f6 24 00                                      ..$.
[   48.735342] mylog----------------in func hidinput_hid_event() file:drivers/hid/hid-input.c:1383 mosue 0 -10
[   48.735358] mylog----------------in func hidinput_hid_event() file:drivers/hid/hid-input.c:1383 mosue 1 36
[   48.751090] mylog----------------in func hid_irq_in() file:drivers/hid/usbhid/hid-core.c:284 hid raw data
[   48.751606] 00 f7 17 00                                      ....
[   48.751831] mylog----------------in func hidinput_hid_event() file:drivers/hid/hid-input.c:1383 mosue 0 -9
[   48.751846] mylog----------------in func hidinput_hid_event() file:drivers/hid/hid-input.c:1383 mosue 1 23
[   48.767793] mylog----------------in func hid_irq_in() file:drivers/hid/usbhid/hid-core.c:284 hid raw data
[   48.768388] 00 fa 0c 00                                      ....
[   48.768610] mylog----------------in func hidinput_hid_event() file:drivers/hid/hid-input.c:1383 mosue 0 -6
[   48.768625] mylog----------------in func hidinput_hid_event() file:drivers/hid/hid-input.c:1383 mosue 1 12
[   48.784446] mylog----------------in func hid_irq_in() file:drivers/hid/usbhid/hid-core.c:284 hid raw data
[   48.785246] 00 ff 01 00                                      ....
[   48.785397] mylog----------------in func hidinput_hid_event() file:drivers/hid/hid-input.c:1383 mosue 0 -1
[   48.785397] mylog----------------in func hidinput_hid_event() file:drivers/hid/hid-input.c:1383 mosue 1 1




## 报告解析过程

usbhid设备初始化时分配了hid设备，设置了ll_driver。注册hid设备到hid总线是，hid框架会调用注册的

调用栈
usbhid_parse(struct hid_device * hid) (/home/wang/program/linux/drivers/hid/usbhid/hid-core.c:975)
hid_add_device(struct hid_device * hdev) (/home/wang/program/linux/drivers/hid/hid-core.c:2344)
usbhid_probe(struct usb_interface * intf, const struct usb_device_id * id) (/home/wang/program/linux/drivers/hid/usbhid/hid-core.c:1389)
usb_probe_interface(struct device * dev) (/home/wang/program/linux/drivers/usb/core/driver.c:361)
really_probe(struct device * dev, struct device_driver * drv) (/home/wang/program/linux/drivers/base/dd.c:552)
driver_probe_device(struct device_driver * drv, struct device * dev) (/home/wang/program/linux/drivers/base/dd.c:721)
bus_for_each_drv(struct bus_type * bus, struct device_driver * start, void * data, int (*)(struct device_driver *, void *) fn) (/home/wang/program/linux/drivers/base/bus.c:430)
__device_attach(struct device * dev, bool allow_async) (/home/wang/program/linux/drivers/base/dd.c:894)
device_initial_probe(struct device * dev) (/home/wang/program/linux/drivers/base/dd.c:941)
bus_probe_device(struct device * dev) (/home/wang/program/linux/drivers/base/bus.c:490)
device_add(struct device * dev) (/home/wang/program/linux/drivers/base/core.c:2202)
usb_set_configuration(struct usb_device * dev, int configuration) (/home/wang/program/linux/drivers/usb/core/message.c:2026)
generic_probe(struct usb_device * udev) (/home/wang/program/linux/drivers/usb/core/generic.c:210)
really_probe(struct device * dev, struct device_driver * drv) (/home/wang/program/linux/drivers/base/dd.c:552)
driver_probe_device(struct device_driver * drv, struct device * dev) (/home/wang/program/linux/drivers/base/dd.c:721)
bus_for_each_drv(struct bus_type * bus, struct device_driver * start, void * data, int (*)(struct device_driver *, void *) fn) (/home/wang/program/linux/drivers/base/bus.c:430)
__device_attach(struct device * dev, bool allow_async) (/home/wang/program/linux/drivers/base/dd.c:894)
device_initial_probe(struct device * dev) (/home/wang/program/linux/drivers/base/dd.c:941)
bus_probe_device(struct device * dev) (/home/wang/program/linux/drivers/base/bus.c:490)
device_add(struct device * dev) (/home/wang/program/linux/drivers/base/core.c:2202)
usb_new_device(struct usb_device * udev) (/home/wang/program/linux/drivers/usb/core/hub.c:2536)
hub_port_connect(u16 portstatus) (/home/wang/program/linux/drivers/usb/core/hub.c:5101)
hub_port_connect_change() (/home/wang/program/linux/drivers/usb/core/hub.c:5216)
port_event() (/home/wang/program/linux/drivers/usb/core/hub.c:5362)
hub_event(struct work_struct * work) (/home/wang/program/linux/drivers/usb/core/hub.c:5442)
process_one_work(struct worker * worker, struct work_struct * work) (/home/wang/program/linux/kernel/workqueue.c:2269)
worker_thread(void * __worker) (/home/wang/program/linux/kernel/workqueue.c:2415)
kthread(void * _create) (/home/wang/program/linux/kernel/kthread.c:255)
ret_from_fork() (/home/wang/program/linux/arch/x86/entry/entry_64.S:352)
[Unknown/Just-In-Time compiled code] (Unknown Source:0)
```c
struct hid_ll_driver usb_hid_driver = {
	.parse = usbhid_parse,
	.start = usbhid_start,
	.stop = usbhid_stop,
	.open = usbhid_open,
	.close = usbhid_close,
	.power = usbhid_power,
	.request = usbhid_request,
	.wait = usbhid_wait_io,
	.raw_request = usbhid_raw_request,
	.output_report = usbhid_output_report,
	.idle = usbhid_idle,
};


static int usbhid_parse(struct hid_device *hid)
{
	struct usb_interface *intf = to_usb_interface(hid->dev.parent);
	struct usb_host_interface *interface = intf->cur_altsetting;
	struct usb_device *dev = interface_to_usbdev (intf);
	struct hid_descriptor *hdesc;
	u32 quirks = 0;
	unsigned int rsize = 0;
	char *rdesc;
	int ret, n;
	int num_descriptors;
	size_t offset = offsetof(struct hid_descriptor, desc);

	quirks = hid_lookup_quirk(hid);

	if (quirks & HID_QUIRK_IGNORE)
		return -ENODEV;

	/* Many keyboards and mice don't like to be polled for reports,
	 * so we will always set the HID_QUIRK_NOGET flag for them. */
	if (interface->desc.bInterfaceSubClass == USB_INTERFACE_SUBCLASS_BOOT) {
		if (interface->desc.bInterfaceProtocol == USB_INTERFACE_PROTOCOL_KEYBOARD ||
			interface->desc.bInterfaceProtocol == USB_INTERFACE_PROTOCOL_MOUSE)
				quirks |= HID_QUIRK_NOGET;
	}

	// 判断扩展描述符里是否有hid描述符
	if (usb_get_extra_descriptor(interface, HID_DT_HID, &hdesc) &&
	    (!interface->desc.bNumEndpoints ||
	     usb_get_extra_descriptor(&interface->endpoint[0], HID_DT_HID, &hdesc))) {
		dbg_hid("class descriptor not present\n");
		return -ENODEV;
	}

	if (hdesc->bLength < sizeof(struct hid_descriptor)) {
		dbg_hid("hid descriptor is too short\n");
		return -EINVAL;
	}

	hid->version = le16_to_cpu(hdesc->bcdHID);
	hid->country = hdesc->bCountryCode;

	num_descriptors = min_t(int, hdesc->bNumDescriptors,
	       (hdesc->bLength - offset) / sizeof(struct hid_class_descriptor));

	// 遍历hid描述符找到报告描述符
	for (n = 0; n < num_descriptors; n++)
		if (hdesc->desc[n].bDescriptorType == HID_DT_REPORT)
			rsize = le16_to_cpu(hdesc->desc[n].wDescriptorLength);

	if (!rsize || rsize > HID_MAX_DESCRIPTOR_SIZE) {
		dbg_hid("weird size of report descriptor (%u)\n", rsize);
		return -EINVAL;
	}

	// 分配存储报告描述符的空间
	rdesc = kmalloc(rsize, GFP_KERNEL);
	if (!rdesc)
		return -ENOMEM;

	hid_set_idle(dev, interface->desc.bInterfaceNumber, 0, 0);

	// 得到报告描述符
	ret = hid_get_class_descriptor(dev, interface->desc.bInterfaceNumber,
			HID_DT_REPORT, rdesc, rsize);
	if (ret < 0) {
		dbg_hid("reading report descriptor failed\n");
		kfree(rdesc);
		goto err;
	}

	print_hex_dump(KERN_INFO, "hid report descriptor", DUMP_PREFIX_NONE, 2, 1,
		       rdesc, rsize, true);
	ret = hid_parse_report(hid, rdesc, rsize);
	kfree(rdesc);
	if (ret) {
		dbg_hid("parsing report descriptor failed\n");
		goto err;
	}

	hid->quirks |= quirks;

	return 0;
err:
	return ret;
}

```

报告描述符打印出结果。qemu虚拟机中只添加了一个usb-mouse鼠标。这是一个mouse报告描述符
[    1.733128] hid report descriptor05 01 09 02 a1 01 09 01 a1 00 05 09 19 01 29 05  ..............).
[    1.733344] hid report descriptor15 00 25 01 95 05 75 01 81 02 95 01 75 03 81 01  ..%...u.....u...
[    1.733517] hid report descriptor05 01 09 30 09 31 09 38 15 81 25 7f 75 08 95 03  ...0.1.8..%.u...
[    1.733688] hid report descriptor81 06 c0 c0  

使用在线解析工具 https://www.usbzh.com/tool/usb.html

0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
0x09, 0x02,        // Usage (Mouse)
0xA1, 0x01,        // Collection (Application)
0x09, 0x01,        //   Usage (Pointer)
0xA1, 0x00,        //   Collection (Physical)
0x05, 0x09,        //     Usage Page (Button)
0x19, 0x01,        //     Usage Minimum (0x01)
0x29, 0x05,        //     Usage Maximum (0x05)
0x15, 0x00,        //     Logical Minimum (0)
0x25, 0x01,        //     Logical Maximum (1)
0x95, 0x05,        //     Report Count (5)
0x75, 0x01,        //     Report Size (1)
0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x95, 0x01,        //     Report Count (1)
0x75, 0x03,        //     Report Size (3)
0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
0x09, 0x30,        //     Usage (X)
0x09, 0x31,        //     Usage (Y)
0x09, 0x38,        //     Usage (Wheel)
0x15, 0x81,        //     Logical Minimum (-127)
0x25, 0x7F,        //     Logical Maximum (127)
0x75, 0x08,        //     Report Size (8)
0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
0xC0,              //   End Collection
0xC0,              // End Collection

// 48 bytes

报告描述符显示此报告描述符用于鼠标，可以表示button 使用5位，最大值为5,最小值是1，填充3位，按钮总共使用一个字节，实验发现4个字节report数据，第0个字节是button。鼠标左键按下第一字节为1,右键为2,中键为4.

然后用于X,和Y，最后是滚轮，都占用8位，最大值是127,最小是-127.实验发现向下滚页，report最后一个字节是0xff,-1