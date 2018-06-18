#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb/input.h>
#include <linux/hid.h>

MODULE_AUTHOR("ade");
//MODULE_DESCRIPTION();
MODULE_LICENSE("GPL");


static struct input_dev *uk_idev;
static char *usb_buf;
static dma_addr_t usb_buf_phys;
static int len;
static struct urb *uk_urb;


static void usbmouse_as_key_irq(struct urb *urb)
{
	static char pre_val = 0;
	/* USB鼠标数据含义
	 * data[0]: bit0-左键, 1-按下, 0-松开
	 *          bit1-右键, 1-按下, 0-松开
	 *          bit2-中键, 1-按下, 0-松开
	 *
     */
	if ((pre_val ^ usb_buf[0]) & 0x01) {
		input_event(uk_idev, EV_KEY, KEY_L, (usb_buf[0] & 0x01)? 1 : 0);
	}

	if ((pre_val ^ usb_buf[0]) & 0x02) {
		input_event(uk_idev, EV_KEY, KEY_S, (usb_buf[0] & 0x02)? 1 : 0);
	}

	if ((pre_val ^ usb_buf[0]) & 0x04) {
		input_event(uk_idev, EV_KEY, KEY_ENTER, (usb_buf[0] & 0x04)? 1 : 0);
	}

	if (pre_val ^ usb_buf[0]) {
		input_sync(uk_idev);
	}
	pre_val = usb_buf[0];


#if 0
	int i;
	static int cnt = 0;
	printk("data cnt %d: ", ++cnt);
	for (i = 0; i < len; i++) {
		printk("%02x ", usb_buf[i]);
	}
	printk("\n");
#endif
	usb_submit_urb(uk_urb, GFP_KERNEL);
}

static int usbmouse_as_key_probe(struct usb_interface *intf,
						const struct usb_device_id *id)
{
	struct usb_device *usb_dev = interface_to_usbdev(intf);
	struct usb_host_interface *interface;
	struct usb_endpoint_descriptor *endpoint;
	int ret;
	int pipe;

	interface = intf->cur_altsetting;
	endpoint = &interface->endpoint[0].desc;

	uk_idev = input_allocate_device();
	if (!uk_idev) {
		return -ENOMEM;
	}
	
	__set_bit(EV_KEY, uk_idev->evbit);
	__set_bit(EV_REP, uk_idev->evbit);

	__set_bit(KEY_L, uk_idev->keybit);
	__set_bit(KEY_S, uk_idev->keybit);
	__set_bit(KEY_ENTER, uk_idev->keybit);

	/* 3.注册 */
	ret = input_register_device(uk_idev);
	if (ret) {
		printk(KERN_ERR "usbmouse_as_key.c: Failed to register input device\n");
		goto err_free_input_dev;
	}

	/* usb 相关的操作 */
	/* 数据传输3要素:源, 目的, 长度 */

	/* 源:usb设备的某个节点 */
	pipe = usb_rcvintpipe(usb_dev, endpoint->bEndpointAddress);
	len = endpoint->wMaxPacketSize;
	usb_buf = usb_alloc_coherent(usb_dev, len, GFP_ATOMIC, &usb_buf_phys);


	/* urb : usb requset block */
	uk_urb = usb_alloc_urb(0, GFP_KERNEL);

	usb_fill_int_urb(uk_urb, usb_dev, pipe, usb_buf, len, usbmouse_as_key_irq, NULL,
						endpoint->bInterval);
	uk_urb->transfer_dma = usb_buf_phys;
	uk_urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	/* 使用 urb */
	usb_submit_urb(uk_urb, GFP_KERNEL);

	printk("usbmouse_as_key: found a mouse device\n");


	return 0;
err_free_input_dev:
	input_free_device(uk_idev);
	return ret;
}

static void usbmouse_as_key_disconnect(struct usb_interface *intf)
{
	struct usb_device *usb_dev = interface_to_usbdev(intf);

	usb_kill_urb(uk_urb);
	usb_free_urb(uk_urb);

	usb_free_coherent(usb_dev, len, usb_buf,usb_buf_phys);
	input_unregister_device(uk_idev);
	input_free_device(uk_idev);
}

static struct usb_device_id usb_mouse_id_table [] = {
	{ USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID, USB_INTERFACE_SUBCLASS_BOOT,
		USB_INTERFACE_PROTOCOL_MOUSE) },
	{ }	/* Terminating entry */
};

MODULE_DEVICE_TABLE (usb, usb_mouse_id_table);

static struct usb_driver usbmouse_as_key_driver = {
	.name		= "usbmouse_as_key",
	.probe		= usbmouse_as_key_probe,
	.disconnect	= usbmouse_as_key_disconnect,
	.id_table	= usb_mouse_id_table,
};

module_usb_driver(usbmouse_as_key_driver);
