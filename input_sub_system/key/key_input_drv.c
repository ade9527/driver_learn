#include <linux/input.h>
#include <linux/module.h>
#include <linux/init.h>

#include <asm/irq.h>
#include <asm/io.h>

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/gpio.h>
#include <linux/timer.h>
#include <mach/regs-gpio.h>
#include <mach/gpio-samsung.h>


#if defined(CONFIG_KEY_DEBUG)
#define key_debug(fmt...) printk(fmt);
#else
#define key_debug(fmt...)
#endif

static struct input_dev *key_input;
static struct timer_list key_timer;

struct pin_desc {
	int irq;
	char *name;
	unsigned int pin;
	unsigned int key_val; //key_code
};

static struct pin_desc pins_desc[] = {
	{IRQ_EINT0, "s2", S3C2410_GPF(0), KEY_L},
	{IRQ_EINT2, "s3", S3C2410_GPF(2), KEY_S},
	{IRQ_EINT11, "s4", S3C2410_GPG(3), KEY_ENTER},
	{IRQ_EINT19, "s5", S3C2410_GPG(11), KEY_LEFTSHIFT},
};

static struct pin_desc *irq_pd = NULL;

static irqreturn_t key_irq_handler(int irq, void *dev_id)
{
	irq_pd = (struct pin_desc*)dev_id;
	mod_timer(&key_timer, jiffies+HZ/100);
	return IRQ_HANDLED;
}

static void kery_timer_function(unsigned long data)
{
	struct pin_desc *pindesc_p = irq_pd;
	unsigned int pinval;

	if (!pindesc_p)
		return;
	
	pinval = !gpio_get_value(pindesc_p->pin);

	/* 上报事件 */
//	input_event(key_input, EV_KEY, pindesc_p->key_val, pinval);
	input_report_key(key_input, pindesc_p->key_val, pinval);
	input_sync(key_input);
}

static int __init my_key_init(void)
{
	int ret = 0;
	int i;

	/* 1.分配一个 struct input_dev */
	key_input = input_allocate_device();
	if (!key_input) {
		printk(KERN_ERR "key_input_drv.c: Not enough memory\n");
		return -ENOMEM;
	}

	/* 2.设置input_dev */
	__set_bit(EV_KEY, key_input->evbit); /* set 能产生哪一类类事件 */
	__set_bit(EV_REP, key_input->evbit); /* set 能产生重复事件 */

	/* 设置4个按键为'l', 's', 'enter', 'leftshit' */
	__set_bit(KEY_L, key_input->keybit);
	__set_bit(KEY_S, key_input->keybit);
	__set_bit(KEY_ENTER, key_input->keybit);
	__set_bit(KEY_LEFTSHIFT, key_input->keybit);

	/* 3.注册 */
	ret = input_register_device(key_input);
	if (ret) {
		printk(KERN_ERR "key_input.c: Failed to register input device\n");
		goto err_free_input_dev;
	}

	/* 4.硬件相关操作 */
	
	/* 初始化定时器 */
	init_timer(&key_timer);
	key_timer.function = &kery_timer_function;
	key_timer.expires = 0;
	add_timer(&key_timer);

	/* irq 初始化*/
	for(i = 0; i < sizeof(pins_desc)/sizeof(struct pin_desc); i++) {
		ret = request_irq(pins_desc[i].irq, key_irq_handler, IRQ_TYPE_EDGE_BOTH, 
							pins_desc[i].name, &pins_desc[i]);
		if (ret) {
			printk(KERN_ERR "key_input_drv.c: Can't allocate irq %d\n",
					pins_desc[i].irq);
			goto err_free_irq;
		}
	}

	return 0;

err_free_irq:
	for(i--; i >= 0; i--) {
		free_irq(pins_desc[i].irq, &pins_desc[i]);
	}
	del_timer(&key_timer);
	input_unregister_device(key_input);
err_free_input_dev:
	input_free_device(key_input);
	return ret;
}

static void __exit my_key_exit(void)
{
	int i;
	
	for(i = 0; i < sizeof(pins_desc)/sizeof(struct pin_desc); i++) {
		free_irq(pins_desc[i].irq, &pins_desc[i]);
	}
	del_timer(&key_timer);
	input_unregister_device(key_input);
	input_free_device(key_input);

	printk("In my_key driver: my_key exit\n");



}

module_init(my_key_init);
module_exit(my_key_exit);
MODULE_LICENSE("GPL");
