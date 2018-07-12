#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/platform_data/at24.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/delay.h>

struct at24_data {
	struct at24_platform_data chip;
	struct cdev cdev;

	unsigned int num_addresses;
	struct i2c_client *client[];
};

static struct class *at24_class;

#define AT24_SIZE_BYTELEN 5
#define AT24_SIZE_FLAGS 8

#define AT24_BITMASK(x) (BIT(x) - 1)

/* create non-zero magic value for given eeprom parameters */
#define AT24_DEVICE_MAGIC(_len, _flags) 		\
	((1 << AT24_SIZE_FLAGS | (_flags)) 		\
	    << AT24_SIZE_BYTELEN | ilog2(_len))

static struct i2c_device_id at24_ids[] = {
	{"24c02", AT24_DEVICE_MAGIC(2048/8, 0)},
	{ /*END OF LIST*/ },
};

static int at24_open(struct inode *inode, struct file *filp)
{
	struct at24_data *at24;

	at24 = container_of(inode->i_cdev, struct at24_data, cdev);
	filp->private_data = at24; /* for other methods */

//	printk("open %s\n", at24->client[0]->name);
	return 0;
}


static ssize_t at24_read(struct file *filp, char __user *buf,
				size_t count, loff_t *f_pos)
{
	struct at24_data *at24 = filp->private_data;
	int page_size = at24->chip.page_size;
	unsigned char *tmp;
	ssize_t ret = 0;
	int i = 0;
	int j;
	int err_count = 0;

	if (*f_pos >= at24->chip.byte_len)
		return 0;

	tmp = kmalloc(page_size, GFP_KERNEL);
	if (!tmp) {
		return -EFAULT;
	}

	for (i = 0; i < count;) {
		unsigned addr = *f_pos + i;
		unsigned len = (int)(addr + page_size) / page_size * page_size - addr;
		if (len + i > count)
			len = count - i;
retry:
		len = i2c_smbus_read_i2c_block_data_or_emulated(at24->client[0], addr, len, tmp);
		if (len <= 0) {
			if (err_count++ < 5) {
				msleep(1);
				goto retry;
			} else {
				ret = i;
				goto out;
			}
		}
		ret = copy_to_user(buf + i, tmp, len);
		if (ret != 0) {
			printk("cpoy to user failed.ret:%d, len:%d\n", ret, len);
			ret = -EFAULT;
			goto out;
		}

		printk("read:addr:%d,len:%d:\n",addr, len);
		for (j = 0; j < len; j++)
			printk("%c,", tmp[j]);
		printk("\n");

		i += len;
		err_count = 0;
	}
	ret = i;

out:
	if (ret > 0)
		*f_pos += ret;
	kfree(tmp);
	return ret;
}

#if 0
static ssize_t at24_write(struct file *filp, const char __user *buf,
					size_t count, loff_t *f_pos)
{
	struct at24_data *at24 = filp->private_data;
	int page_size = at24->chip.page_size;
	unsigned char *tmp;
	ssize_t ret = 0;
	int i = 0;
	int j;
	int err_count = 0;

	if (*f_pos >= at24->chip.byte_len)
		return 0;

	tmp = kmalloc(page_size, GFP_KERNEL);
	if (!tmp) {
		return -EFAULT;
	}

	for (i = 0; i < count;) {
		unsigned addr = *f_pos + i;
		unsigned len = (int)(addr + page_size) / page_size * page_size - addr;
		if (len + i > count)
			len = count - i;

		ret = copy_from_user(tmp, buf + i, len);
		if (ret != 0) {
			printk("cpoy from user failed.ret:%d, len:%d\n", ret, len);
			ret = -EFAULT;
			goto out;
		}

		printk("write:addr:%d,len:%d:\n",addr, len);
		for (j = 0; j < len; j++)
			printk("%c,", tmp[j]);
		printk("\n");

retry:
		ret = i2c_smbus_write_i2c_block_data(at24->client[0], addr, len, tmp);
		if (ret < 0) {
			if (err_count++ < 5) {
				msleep(1);
				goto retry;
			} else {
				printk("i2c write error\n");
				goto out;
			}
		}

		i += len;
	}
	ret = i;

out:
	if (ret > 0)
		*f_pos += ret;
	kfree(tmp);
	return ret;
}
#else
static ssize_t at24_write(struct file *filp, const char __user *buf,
					size_t count, loff_t *f_pos)
{
	struct at24_data *at24 = filp->private_data;
	unsigned char tmp;
	ssize_t ret = 0;
	int i = 0;
	int err_count = 0;

	for (i = 0; i < count;) {
		ret = copy_from_user(&tmp, buf + i, 1);
		if (ret != 0) {
			printk("cpoy from user failed.ret:%d, len:%d\n", ret, 1);
			ret = -EFAULT;
			goto out;
		}
retry:
		ret = i2c_smbus_write_byte_data(at24->client[0], *f_pos + i, tmp);
		if (ret < 0) { /* i2c 可能会失败 所以要延时后重试 */
			if (err_count++ < 5) {
				msleep(1);
				goto retry;
			}
			else {
				printk("i2c write error\n");
				goto out;
			}
		}
		err_count = 0;
		i++;
	}
	*f_pos += i;
	ret = i;

out:
	return ret;

}
#endif


static struct file_operations at24_fops = {
	.owner = THIS_MODULE,
	.open  = at24_open,
	.read  = at24_read,
	.write = at24_write,
};

static int
at24_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct at24_data *at24;
	struct at24_platform_data chip = {0};
	unsigned int num_addresses = 0;
	kernel_ulong_t magic = 0;
	int i, err;

	dev_t dev_t;
	struct device *dev_p;

	if (client->dev.platform_data) {
		chip = *(struct at24_platform_data *)client->dev.platform_data;
	} else {
		magic = id->driver_data;
		chip.byte_len = BIT(magic & AT24_BITMASK(AT24_SIZE_BYTELEN));
		magic >>= AT24_SIZE_BYTELEN;
		chip.flags = magic & AT24_BITMASK(AT24_SIZE_FLAGS);
		chip.page_size = 8; /*24c02 的pagesize是8 */
		chip.setup = NULL;
		chip.context = NULL;
	}

	if (!is_power_of_2(chip.byte_len))
		dev_warn(&client->dev,
			"byte_len looks suspicious (no power of 2)!\n");
	if (!chip.page_size) {
		dev_err(&client->dev, "page_size must not be 0!\n");
		return -EINVAL;
	}
	if (!is_power_of_2(chip.page_size))
		dev_warn(&client->dev,
			"page_size looks suspicious (no power of 2)!\n");

	num_addresses = DIV_ROUND_UP(chip.byte_len,256);

	at24 = devm_kzalloc(&client->dev, sizeof(struct at24_data) +
			num_addresses * sizeof(struct i2c_client *), GFP_KERNEL);

	if (!at24)
		return -ENOMEM;

	at24->chip = chip;
	at24->num_addresses = num_addresses;
	at24->client[0] = client;

	for (i = 1; i < num_addresses; i++) {
		at24->client[i] = i2c_new_dummy(client->adapter, client->addr + i);
		if (!at24->client[i]) {
			dev_err(&client->dev, "address 0x%02x unavailable\n",
						client->addr + i);
			err = -EADDRINUSE;
			goto err_clients;
		}
	}

	i2c_set_clientdata(client, at24);

	/* create char dev */
	err = alloc_chrdev_region(&dev_t, 0, 1, "eeprom");
	if (err < 0) {
		dev_err(&client->dev, "alloc chrdev region failed\n");
		goto err_alloc_chrdev;
	}
	cdev_init(&at24->cdev, &at24_fops);
	at24->cdev.owner = THIS_MODULE;
	err = cdev_add(&at24->cdev, dev_t, 1);
	if (err) {
		dev_err(&client->dev, "cdev_add failed\n");
		goto err_cdev_add;
	}
	
	dev_p = device_create(at24_class, &client->dev, dev_t, at24, "%s", client->name);
	if (IS_ERR(dev_p)) {
		dev_err(&client->dev, "device_create failed\n");
		err = PTR_ERR(dev_p);
		goto err_device_create;
	}

	dev_info(&client->dev, "%u byte %s EEPROM\n", chip.byte_len, client->name);
	return 0;

err_device_create:
	cdev_del(&at24->cdev);
err_cdev_add:
	unregister_chrdev_region(dev_t, 1);
err_alloc_chrdev:
err_clients:
	for (i = 1;i < num_addresses; i++) {
		if (at24->client[i])
			i2c_unregister_device(at24->client[i]);
	}

	devm_kfree(&client->dev, at24);
	return err;

}

static int at24_remove(struct i2c_client *client)
{
	int i;
	struct at24_data *at24 = i2c_get_clientdata(client);

	device_destroy(at24_class, at24->cdev.dev);
	cdev_del(&at24->cdev);
	unregister_chrdev_region(at24->cdev.dev, 1);
	for (i = 1;i < at24->num_addresses; i++) {
		if (at24->client[i])
			i2c_unregister_device(at24->client[i]);
	}
	return 0;
}


static struct i2c_driver at24_driver = {
	.driver = {
		.name = "at24cxx",
	},
	.probe  = at24_probe,
	.remove = at24_remove,
	.id_table = at24_ids,
};

static int __init at24_init(void)
{
	at24_class = class_create(THIS_MODULE, "at24_class");
	if (IS_ERR(at24_class))
		return PTR_ERR(at24_class);

	return i2c_add_driver(&at24_driver);
}
static void __exit at24_exit(void)
{
	i2c_del_driver(&at24_driver);
	class_destroy(at24_class);
}

module_init(at24_init);
module_exit(at24_exit);
MODULE_LICENSE("GPL");
