#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");


static struct class *my_key_class;
static struct device *my_key_dev;

static int major = 0;

static volatile unsigned long *gpfcon = NULL;
static volatile unsigned long *gpfdat = NULL;
static volatile unsigned long *gpgcon = NULL;
static volatile unsigned long *gpgdat = NULL;

static int my_key_open(struct inode *inode, struct file *file)
{
	/* 配置 gpf0, 2 为输入引脚 */
	*gpfcon &= ~((0x3 << 0) | (0x3 << (2*2)));

	/* 配置 gpg 3, 11 为输入引脚 */
	*gpgcon &= ~((0x3 << (2*2)) | (0x3 << (2*11)));

	printk("In my_key driver: open my_key\n");
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
	int val = 0;
	unsigned char key_val[4] = {0};
	int ret;
	
	if (count != sizeof(key_val))
		return -EINVAL;

	val = *gpfdat;
	key_val[0] = (val & (0x1 << 0)) ? 0 : 1;
	key_val[1] = (val & (0x1 << 2)) ? 0 : 1;

	val = *gpgdat;
	key_val[2] = (val & (0x1 << 3)) ? 0 : 1;
	key_val[3] = (val & (0x1 << 11)) ? 0 : 1;

	ret = copy_to_user(buf, key_val, sizeof(key_val));

	printk("In my_key drv:read my_key val:0x%x\n", val);
	return count;
}

static struct file_operations my_key_fops = {
	.owner  = THIS_MODULE,
	.open   = my_key_open,
	.write  = my_key_write,
	.read   = my_key_read,
};

static int __init my_key_init(void)
{
	major = register_chrdev(0, "my_key_drv", &my_key_fops);

	my_key_class = class_create(THIS_MODULE, "my_key_class");
	if (IS_ERR(my_key_class))
		return PTR_ERR(my_key_class);
	
	my_key_dev = device_create(my_key_class, NULL, MKDEV(major, 0), NULL, "my_key");
	if (unlikely(IS_ERR(my_key_dev)))
		return PTR_ERR(my_key_dev);

	gpfcon = ioremap(0x56000050, 16);
	gpfdat = gpfcon + 1;

	gpgcon = ioremap(0x56000060, 16);
	gpgdat = gpgcon + 1;

	printk("In my_key driver: my_key init major:%d\n", major);
	return 0;
}

static void __exit my_key_exit(void)
{
	iounmap(gpfcon);
	//device_unregister(my_key_dev);
	device_destroy(my_key_class, MKDEV(major, 0));
	class_destroy(my_key_class);

	unregister_chrdev(major, "my_key_drv");
	printk("In my_key driver: my_key exit\n");
}

module_init(my_key_init);
module_exit(my_key_exit);
