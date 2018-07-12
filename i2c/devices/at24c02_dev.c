#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/err.h>

static struct i2c_board_info at24cxx_info = {
	I2C_BOARD_INFO("24c02", 0x40),
};

static struct i2c_client *at24_client;

static int __init at24cxx_device_init(void)
{
	struct i2c_adapter *adap;

	adap = i2c_get_adapter(0);
	if (!adap) {
		printk("i2c get adapter failed\n");
		return -ENODEV;
	}

	at24_client = i2c_new_device(adap,&at24cxx_info);
	i2c_put_adapter(adap);
	if (!at24_client) {
		printk("i2c new device failed\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit at24cxx_device_exit(void)
{
	i2c_unregister_device(at24_client);
}

module_init(at24cxx_device_init);
module_exit(at24cxx_device_exit);
MODULE_LICENSE("GPL");
