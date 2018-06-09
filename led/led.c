#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");


static struct class *led_class;
static struct device *led_class_dev;

static int major = 0;

static volatile unsigned long *gpfcon = NULL;
static volatile unsigned long *gpfdat = NULL;

static int led_open(struct inode *inode, struct file *file)
{
	printk("In led driver: open led\n");
	return 0;
}

static int led_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	int val = 0;
	int lenght = sizeof(val) > count ? count: sizeof(val);
	int ret;
	
	ret = copy_from_user(&val, buf, lenght);
	printk("In led driver: write led val:0x%x\n", val);
	
	if (val) {
		*gpfdat &= ~(0x07 << 4);
	} else {
		*gpfdat |= 0x07 << 4;
	}
	return count;
}

static struct file_operations led_fops = {
	.owner  = THIS_MODULE,
	.open   = led_open,
	.write  = led_write,
};

static int __init led_init(void)
{
	major = register_chrdev(0, "led_drv", &led_fops);

	led_class = class_create(THIS_MODULE, "led");
	if (IS_ERR(led_class))
		return PTR_ERR(led_class);
	
	led_class_dev = device_create(led_class, NULL, MKDEV(major, 0), NULL, "led");
	if (unlikely(IS_ERR(led_class_dev)))
		return PTR_ERR(led_class_dev);

	gpfcon = ioremap(0x56000050, 16);
	gpfdat = gpfcon + 1;

	*gpfcon &= ~((0x3 << 12) | (0x3 << 10) | (0x3 << 8));
	*gpfcon |= (0x1 << 12) | (0x1 << 10) | (0x1 << 8);

	*gpfdat |= 0x70;

	printk("In led driver: led init major:%d\n", major);
	return 0;
}

static void __exit led_exit(void)
{
	iounmap(gpfcon);
	device_unregister(led_class_dev);
	class_destroy(led_class);

	unregister_chrdev(major, "led_drv");
	printk("In led driver: led exit\n");
}

module_init(led_init);
module_exit(led_exit);
