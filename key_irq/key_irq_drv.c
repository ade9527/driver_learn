#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/gpio.h>
#include <mach/regs-gpio.h>
#include <mach/gpio-samsung.h>

MODULE_LICENSE("GPL");


static struct class *my_key_class;
static struct device *my_key_dev;

static int major = 0;

static int ev_press = 0;
static DECLARE_WAIT_QUEUE_HEAD(my_key_waitq);

static struct fasync_struct *my_key_async_q = NULL;

struct pin_desc {
	unsigned int pin;
	unsigned int key_val;
};

static struct pin_desc pins_desc[] = {
	{S3C2410_GPF(0), 0x00},
	{S3C2410_GPF(2), 0x00},
	{S3C2410_GPG(3), 0x00},
	{S3C2410_GPG(11), 0x00},
};


static irqreturn_t buttons_irq(int irq, void *dev_id)
{
	struct pin_desc *pindesc_p = (struct pin_desc *)dev_id;

	/* 按键按下为低电平 所以取反 */
	pindesc_p->key_val = !gpio_get_value(pindesc_p->pin);
	
	ev_press = 1;
	wake_up_interruptible(&my_key_waitq);

	kill_fasync(&my_key_async_q, SIGIO, POLL_IN);
	printk("the irq:%d val:%d\n", irq, pindesc_p->key_val);
	return IRQ_HANDLED;
}

static int my_key_open(struct inode *inode, struct file *file)
{
	int ret;
	ret = request_irq(IRQ_EINT0, buttons_irq, IRQ_TYPE_EDGE_BOTH, "S2", &pins_desc[0]);
	ret = request_irq(IRQ_EINT2, buttons_irq, IRQ_TYPE_EDGE_BOTH, "S3", &pins_desc[1]);
	ret = request_irq(IRQ_EINT11, buttons_irq, IRQ_TYPE_EDGE_BOTH, "S4", &pins_desc[2]);
	ret = request_irq(IRQ_EINT19, buttons_irq, IRQ_TYPE_EDGE_BOTH, "S5", &pins_desc[3]);

	printk("In my_key driver: open my_key\n");
	return 0;
}

static int my_key_close(struct inode *inode, struct file *file)
{
	free_irq(IRQ_EINT0, &pins_desc[0]);
	free_irq(IRQ_EINT2, &pins_desc[1]);
	free_irq(IRQ_EINT11, &pins_desc[2]);
	free_irq(IRQ_EINT19, &pins_desc[3]);

	return 0;
}

static int my_key_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	int val = 0;
	int lenght = sizeof(val) > count ? count: sizeof(val);
	int ret;
	
	ret = copy_from_user(&val, buf, lenght);
	printk("In my_key driver: write my_key val:0x%x\n", val);
	
	return count;
}

static int my_key_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	unsigned int key_val = 0;
	int ret;
	
	key_val = pins_desc[0].key_val | (pins_desc[1].key_val << 1) |
				(pins_desc[2].key_val << 2) | (pins_desc[3].key_val << 3);
	
	if (count != sizeof(key_val))
		return -EINVAL;

//	wait_event_interruptible(my_key_waitq, ev_press);

	ret = copy_to_user(buf, &key_val, sizeof(key_val));
	
	ev_press = 0;
	printk("In my_key drv:read my_key val:0x%x\n", key_val);
	return count;
}

static unsigned int my_key_poll(struct file *fp, poll_table *wait)
{
	unsigned int mask = 0;
	//poll_wait(fp, &my_key_waitq, wait);
	printk("key drv poll\n");

	if (ev_press)
		mask = POLLIN | POLLRDNORM;
	return mask;
}

static int my_key_fasync(int fd, struct file *filp, int on)
{
	printk("key drv fasync\n");
	return fasync_helper(fd, filp, on, &my_key_async_q);
}

static struct file_operations my_key_fops = {
	.owner   = THIS_MODULE,
	.open    = my_key_open,
	.write   = my_key_write,
	.read    = my_key_read,
	.release = my_key_close,
	.poll    = my_key_poll,
	.fasync  = my_key_fasync,
};

static int __init my_key_init(void)
{
	major = register_chrdev(0, "my_key_drv", &my_key_fops);

	my_key_class = class_create(THIS_MODULE, "my_key_class");
	if (IS_ERR(my_key_class))
		return PTR_ERR(my_key_class);
	
	my_key_dev = device_create(my_key_class, NULL, MKDEV(major, 0), NULL, "my_key_irq");
	if (unlikely(IS_ERR(my_key_dev)))
		return PTR_ERR(my_key_dev);


	printk("In my_key driver: my_key init major:%d\n", major);
	return 0;
}

static void __exit my_key_exit(void)
{
	device_unregister(my_key_dev);
	class_destroy(my_key_class);

	unregister_chrdev(major, "my_key_drv");
	printk("In my_key driver: my_key exit\n");
}

module_init(my_key_init);
module_exit(my_key_exit);
