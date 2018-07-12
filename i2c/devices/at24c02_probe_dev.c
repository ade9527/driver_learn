#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/err.h>

static const unsigned short at24_addrs[] = {
	0x2c,
	0x50,
	I2C_CLIENT_END,
};

static struct i2c_client *at24_client;

static int __init at24cxx_device_init(void)
{
	struct i2c_adapter *adap;
	struct i2c_board_info i2c_info;

	adap = i2c_get_adapter(0);
	if (!adap) {
		printk("i2c get adapter failed\n");
		return -ENODEV;
	}

	memset(&i2c_info, 0, sizeof(i2c_info));
	strlcpy(i2c_info.type, "24c02", I2C_NAME_SIZE);
	at24_client = i2c_new_probed_device(adap, &i2c_info, at24_addrs, NULL);
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
