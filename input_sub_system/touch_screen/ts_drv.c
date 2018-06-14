#include <linux/module.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/timer.h>
//#include <poll.h>

#define CONFIG_TS_DRV_DEBUG

#define TS_IS_UP(reg) ( \
			(reg->adcdat0 & (1 << 15) && (reg->adcdat1 & (1 << 15))))

#ifdef CONFIG_TS_DRV_DEBUG
	#define ts_debug(x...) printk(KERN_ERR x);
#else
	#define ts_debug(x...)
#endif


struct s3c_ts_regs {
	unsigned long adccon;
	unsigned long adctsc;
	unsigned long adcdly;
	unsigned long adcdat0;
	unsigned long adcdat1;
	unsigned long adcupdn;
};


static struct input_dev *s3c_ts_dev;
static volatile struct s3c_ts_regs *s3c_ts_regs = NULL;
static struct timer_list ts_timer;

static void enter_wait_pen_down_mode(void)
{
	s3c_ts_regs->adctsc = 0xd3;
}

static void enter_wait_pen_up_mode(void)
{
	s3c_ts_regs->adctsc = 0x1d3;
}

static void enter_measure_xy_mode(void)
{
	s3c_ts_regs->adctsc = (1<<2);
}

static void start_adc(void)
{
	s3c_ts_regs->adccon |= (1 << 0);
}

static irqreturn_t pen_down_up_irq_handler(int irq, void *dev_id)
{
	int adcdat0, adcdat1;
	adcdat0 = (int)s3c_ts_regs->adcdat0;
	adcdat1 = (int)s3c_ts_regs->adcdat1;

	if (TS_IS_UP(s3c_ts_regs)) {
		//ts_debug("pen up\n");
		input_report_abs(s3c_ts_dev, ABS_PRESSURE, 0);
		input_report_key(s3c_ts_dev, BTN_TOUCH, 0);
		input_sync(s3c_ts_dev);
		enter_wait_pen_down_mode();
	} else {
		enter_measure_xy_mode();
		start_adc();
	}
	return IRQ_HANDLED;
}

static irqreturn_t adc_irq(int irq, void *dev_id)
{
	static int cnt = 0;
	static int x[4], y[4];
	int adcdat0, adcdat1;

	/* 优化2, 如果adc完成时 触摸屏已经松开则丢弃结果 */
	adcdat0 = (int)s3c_ts_regs->adcdat0;
	adcdat1 = (int)s3c_ts_regs->adcdat1;
	
	if ((adcdat0 & (1<15)) && (adcdat1 & (1 << 15))) {
		/* 已经松开 */
		//ts_debug("pen uped\n");
		input_report_abs(s3c_ts_dev, ABS_PRESSURE, 0);
		input_report_key(s3c_ts_dev, BTN_TOUCH, 0);
		input_sync(s3c_ts_dev);
		goto out;
	}

	/* 优化3: 多次测量求平均值 */

	x[cnt] = adcdat0 & 0x3ff;
	y[cnt] = adcdat1 & 0x3ff;
	cnt++;

	if (cnt != 4) {
		enter_measure_xy_mode();
		start_adc();
		return IRQ_HANDLED;
	}

	/* 优化4: 加软件滤波 */
	/* 这里没做 */
	adcdat0 = (x[0] + x[1] + x[2] +x[3]) / 4;
	adcdat1 = (y[0] + y[1] + y[2] +y[3]) / 4;
	//ts_debug("ts_drv: adc_irq x = %d, y = %d\n", adcdat0, adcdat1);
	/* 上报事件 */
	input_report_abs(s3c_ts_dev, ABS_X, adcdat0);
	input_report_abs(s3c_ts_dev, ABS_Y, adcdat1);
	input_report_abs(s3c_ts_dev, ABS_PRESSURE, 1);
	input_report_key(s3c_ts_dev, BTN_TOUCH, 1);
	input_sync(s3c_ts_dev);

	mod_timer(&ts_timer, jiffies + HZ/10); // 100ms

out:
	cnt = 0;
	enter_wait_pen_up_mode();
	return IRQ_HANDLED;
}

static void s3c_ts_timer_function(unsigned long data)
{
	if (TS_IS_UP(s3c_ts_regs)) {
		input_report_abs(s3c_ts_dev, ABS_PRESSURE, 0);
		input_report_key(s3c_ts_dev, BTN_TOUCH, 0);
		input_sync(s3c_ts_dev);
		enter_wait_pen_down_mode();
	} else {
		enter_measure_xy_mode();
		start_adc();
	}
}

static int __init my_ts_init(void)
{
	int ret = 0;
	struct clk *adc_clk;

	/* input subsystem init*/
	s3c_ts_dev = input_allocate_device();
	if (!s3c_ts_dev) {
		printk(KERN_ERR "ts_drv.c:Not enough memory\n");
		return -ENOMEM;
	}
	__set_bit(EV_KEY, s3c_ts_dev->evbit);
	__set_bit(EV_ABS, s3c_ts_dev->evbit);

	__set_bit(BTN_TOUCH, s3c_ts_dev->keybit);
	input_set_abs_params(s3c_ts_dev, ABS_X, 0, 0X3FF, 0, 0);
	input_set_abs_params(s3c_ts_dev, ABS_Y, 0, 0X3FF, 0, 0);
	input_set_abs_params(s3c_ts_dev, ABS_PRESSURE, 0, 1, 0, 0);

	ret = input_register_device(s3c_ts_dev);
	if (ret) {
		printk(KERN_ERR "ts_drv.c: Failed to register input device\n");
		goto err_free_input_dev;
	}

	/* 硬件相关初始化 */
	adc_clk = clk_get(NULL, "adc");
	if (IS_ERR(adc_clk)) {
		printk("ts_drv: get adc clk error\n");
		ret = -EBUSY;
		goto err_input_unregister;
	}
	clk_prepare_enable(adc_clk);
//	clk_enable(adc_clk);

	/* ADC/TS 寄存器设置 */
	s3c_ts_regs = ioremap(0x58000000, sizeof(struct s3c_ts_regs));
	if (!s3c_ts_regs) {
		printk(KERN_ERR "ts_drv: Failed to adc ioremap\n");
		ret = -ENXIO;
		goto err_free_clk;
	}

	/*
	 * PRSCEN [14]   : A/D converter prescaler enable
	 * PRSCVL [13:6] : A/D converter prescaler value
	 *				 = 49, ADCclk = PCLK/(49+1) = 1MHz
	 * 
	 * STDBM [2]	 : = 0 Normal operation mode
	 * ENABLE_START [0] : A/D conversion starts by enable.
	 *					= 0; 先设为0;
	 **/
	s3c_ts_regs->adccon = (1<<14) | (49<<6);

	/*
	 * adc值不稳定优化:
	 * 1. ADCDLY设置最大值, 使得adc读取时电压稳定.
	 **/
	s3c_ts_regs->adcdly = 0xffff;

	ret = request_irq(IRQ_TC, pen_down_up_irq_handler, 0, "s3c2440_ts_pen", NULL);
	if (ret) {
		printk(KERN_ERR "ts_drv.c: Cannot get TC interrupt\n");
		goto err_iomap;
	}

	ret = request_irq(IRQ_ADC, adc_irq, 0, "s3c2440_ts_adc", NULL);
	if (ret) {
		printk(KERN_ERR "ts_drv.c: Cannot get ADC interrupt\n");
		goto err_irq_tc;
	}

	init_timer(&ts_timer);
	ts_timer.function = s3c_ts_timer_function;
	add_timer(&ts_timer);

	enter_wait_pen_down_mode();

	return 0;
err_irq_tc:
	free_irq(IRQ_TC, NULL);
err_iomap:
	iounmap(s3c_ts_regs);
err_free_clk:
	clk_disable(adc_clk);
	clk_put(adc_clk);
err_input_unregister:
	input_unregister_device(s3c_ts_dev);
err_free_input_dev:
	input_free_device(s3c_ts_dev);
	return ret;
}

static void __exit my_ts_exit(void)
{
	free_irq(IRQ_ADC, NULL);
	free_irq(IRQ_TC, NULL);
	iounmap(s3c_ts_regs);
	input_unregister_device(s3c_ts_dev);
	input_free_device(s3c_ts_dev);
}

module_init(my_ts_init);
module_exit(my_ts_exit);
MODULE_LICENSE("GPL");
