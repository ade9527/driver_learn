#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <linux/io.h>

#include <asm/div64.h>

#include <asm/mach/map.h>
#include <mach/regs-lcd.h>
#include <mach/regs-gpio.h>
#include <mach/fb.h>


/*#include "lcd_2440fb.h"
 */

struct lcd_regs {
	unsigned long lcdcon1;
	unsigned long lcdcon2;
	unsigned long lcdcon3;
	unsigned long lcdcon4;
	unsigned long lcdcon5;
	unsigned long lcdsaddr1;
	unsigned long lcdsaddr2;
	unsigned long lcdsaddr3;
	unsigned long redlut;
	unsigned long greenlut;
	unsigned long bluelut;
	unsigned long reserved[8];
	unsigned long dithnode;
	unsigned long tpal;;
	unsigned long lcdintpnd;
	unsigned long lcdsrcpnd;
	unsigned long lcdintmsk;
	unsigned long tconsel;
};

static struct fb_info *s3c_bfinfo;
static volatile unsigned long *gpbcon;
static volatile unsigned long *gpbdat;
static volatile unsigned long *gpccon;
static volatile unsigned long *gpdcon;
static volatile unsigned long *gpgcon;
static volatile struct lcd_regs* lcd_regs = NULL;
struct resource *lcd_mem;
static u32 pseudo_palette[16];
static struct clk *lcd_clk = NULL;

static inline unsigned int chan_to_field(unsigned int chan, struct fb_bitfield *bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf->length;
	return chan << bf->offset;
}

static int s3c_lcdfb_setcolreg(unsigned int regno, unsigned int red,
				unsigned int green, unsigned int blue, unsigned int transp,
				struct fb_info *info)
{
	unsigned int val;

	if (regno > 16)
		return 1;

	val  = chan_to_field(red, &info->var.red);
	val |= chan_to_field(green, &info->var.green);
	val |= chan_to_field(blue, &info->var.blue);
	pseudo_palette[regno] = val;
	return 0;
}

#if 0
static int s3c_lcd_set_par(struct fb_info *info)
{
	int val;
	/* clkval [17:8]: VCLK = HCLK / [(CLKVAL+1) x 2]
	 *				  100ns[10MHz] = 100MHz / [(CLKVAL+1) x 2]
	 *				  clkval = 10 / 2 - 1;
	 *				  clkval = 4;
	 * pnrmode [6:5] 0b11 TFT LCD
	 * bppmode [4:1] 0b1100 16bpp for TFT
	 * envid [0]: lcd video output and the logic enable/disable.
	 *				0 disable, 1 enable
	 * */
	lcd_regs->lcdcon1 = (4 << 8) | (3<<5) | (0xc<<1); // 暂时不使能lcd

	/*
	 * vbpd [31:24] : vsync信号后多少时间后发第一个数据
	 *				T0 - T1 -T2 = 4
	 *				vbpd = 4 - 1;
	 * VLINEVAL [23:14] these bits determine the vertical size of LCD panel.
	 *					vlineval = 320 - 1;
	 * vfpd [13:6] : 发出最后一行数据后多长时间开始发vsync
	 *				T2 - T5 = 2;
	 *				vfpd = 2 - 1;
	 * vspw [5:0] : vsync 宽度 T1
	 *				vspw = 1 - 1;
	 **/
	lcd_regs->lcdcon2 = (3 << 24) | (319 << 14) | (1 << 6) | (0 << 0);

	/*
	 * LCDCON3:
	 * HBPD [25:19] HSYNC 之后多少个vclk 开始发送第一个像素数据
	 *				hbpd = T6 - T7 - T8 - 1 = 273 - 5 - 251 - 1;
	 *				HBPD = 16;
	 * HOZVAL [18:8] 每行像素个数 T11 = 240
	 *				HOZVAL = 240 - 1;
	 * HFPD [7:0] 每行结束到hsync 的vclk数 T8 - T11
	 *				HFPD = 251 - 240 - 1 = 10;
	 **/
	lcd_regs->lcdcon3 = (16 << 19) | (239 << 8) | (10 << 0);

	/*
	 * HSPW [7:0] hsync 宽度 T7 = 5
	 *				HSPW = 4;
	 **/
	lcd_regs->lcdcon4 = (4 << 0);

	/*
	 * 信号极性
	 * bit[11] this bit determines the format of 16 bpp output video data.
	 *			1 = 5:6:5 format
	 * bit[10]  0 = 下降沿取值
	 * bit[9] 1 = hsycn 低电平有效
	 * bit[8]  1 = vsync 信号反转 低电平有效
	 * bit[6] 0 = VDEN 不用反转
	 * bit[3] 0 = pwren 输出0
	 * bit[1] 0 = BSWAP disable
	 * bit[0] 1 = hwswp 开 2440 p413
	 **/
	lcd_regs->lcdcon5 = (1<<11) | (1<<9) | (1<<8) | (1<<0);

	lcd_regs->lcdsaddr1 = (s3c_bfinfo->fix.smem_start >> 1);
	lcd_regs->lcdsaddr2 = (((s3c_bfinfo->fix.smem_start +
						s3c_bfinfo->fix.smem_len) >> 1));
	lcd_regs->lcdsaddr3 = (240*2/2) & 0x3ff;
	lcd_regs->tpal = 0;
//	lcd_regs->tconsel = ((0xCE6) & ~7) | 1<<4;

	/* enable lcd controller */
	lcd_regs->lcdcon1 |= (1<<0);
	lcd_regs->lcdcon5 |= (1<<3);

	val = readl(&lcd_regs->lcdsaddr3);
	printk("lcd drv: &lcdsaddr3:%p,con1:%x, val:%x\n",&lcd_regs->lcdsaddr3,(int)lcd_regs->lcdcon1, val);
	
	printk("lcd drv: screen_base:%p\n,smem_start:%p\n",s3c_bfinfo->screen_base , (void *)s3c_bfinfo->fix.smem_start);

	return 0;
}
#endif

static struct fb_ops s3c_lcdfb_ops = {
	.owner		= THIS_MODULE,
	.fb_setcolreg	= s3c_lcdfb_setcolreg,
//	.fb_set_par	    = s3c_lcd_set_par,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit
};


static int __init mylcd_init(void)
{
	int ret = 0;
	/* 分配一个fb_info */
	s3c_bfinfo = framebuffer_alloc(0, NULL);
	memset(s3c_bfinfo, 0, sizeof(*s3c_bfinfo));

	/* set fb_info 固定参数 */
	strcpy(s3c_bfinfo->fix.id, "mylcd");
	s3c_bfinfo->fix.smem_len = 240*320*16/8;
	s3c_bfinfo->fix.type = FB_TYPE_PACKED_PIXELS;
	s3c_bfinfo->fix.visual = FB_VISUAL_TRUECOLOR;
	s3c_bfinfo->fix.line_length = 240 * 2;

	/* 设置可变参数 */
	s3c_bfinfo->var.xres	= 240;
	s3c_bfinfo->var.yres	= 320;
	s3c_bfinfo->var.width	= 240;
	s3c_bfinfo->var.height	= 320;
	s3c_bfinfo->var.xres_virtual = 240;
	s3c_bfinfo->var.yres_virtual = 320;
	s3c_bfinfo->var.bits_per_pixel = 16;
	/* RGB:565 */
	s3c_bfinfo->var.red.offset = 11;
	s3c_bfinfo->var.red.length = 5;
	s3c_bfinfo->var.green.offset = 5;
	s3c_bfinfo->var.green.length = 6;
	s3c_bfinfo->var.blue.offset = 0;
	s3c_bfinfo->var.blue.length = 5;

	s3c_bfinfo->var.activate = FB_ACTIVATE_NOW;
	
	/* 设置操作参数 */
	s3c_bfinfo->fbops		= &s3c_lcdfb_ops;

	s3c_bfinfo->screen_size = 240*320*2;
	s3c_bfinfo->pseudo_palette = pseudo_palette;


	/* 硬件相关的set */
	gpbcon = ioremap(0x56000010, 8);
	gpbdat = gpbcon + 1;
	gpccon = ioremap(0x56000020, 16);
	gpdcon = ioremap(0x56000030, 16);
	gpgcon = ioremap(0x56000060, 4);

	printk("lcd drv:ioremap %p %p %p %p\n", gpbcon, gpccon, gpdcon, gpgcon);
	*gpccon = 0xaaaaaaaa;
	*(gpccon+2) = 0xffff;
	*gpdcon = 0xaaaaaaaa;
	*(gpdcon+2) = 0xffff;

	*gpbcon &= ~(0x3);
	*gpbcon |= 1;
	*gpbdat &= ~1; /* 先关闭背光 */

	*gpgcon |= (3 << (4*2)); /* gpg4 lcd_power 引脚 */
	printk("gpccon:%x\n", (int)*gpccon);

	lcd_clk = clk_get(NULL, "lcd");
	if (IS_ERR(lcd_clk)) {
		printk("lcd drv: get lcd clk error\n");
		ret = -EBUSY;
		goto out;
	}
	clk_prepare_enable(lcd_clk);

	lcd_mem = request_mem_region(0x4d000000, sizeof(struct lcd_regs), "mys3c_lcd");
	if (lcd_mem == NULL) {
		printk("request mem  lcd  error\n");
		return -ENXIO;
	}

	lcd_regs = ioremap(0x4D000000, sizeof(struct lcd_regs));

	printk("lcd drv: lcd regs:%p, tpal:%p\n", lcd_regs, &lcd_regs->tpal);
#if 1
	/* clkval [17:8]: VCLK = HCLK / [(CLKVAL+1) x 2]
	 *				  100ns[10MHz] = 100MHz / [(CLKVAL+1) x 2]
	 *				  clkval = 10 / 2 - 1;
	 *				  clkval = 4;
	 * pnrmode [6:5] 0b11 TFT LCD
	 * bppmode [4:1] 0b1100 16bpp for TFT
	 * envid [0]: lcd video output and the logic enable/disable.
	 *				0 disable, 1 enable
	 * */
	lcd_regs->lcdcon1 = (4 << 8) | (3<<5) | (0xc<<1); // 暂时不使能lcd

	/*
	 * vbpd [31:24] : vsync信号后多少时间后发第一个数据
	 *				T0 - T1 -T2 = 4
	 *				vbpd = 4 - 1;
	 * VLINEVAL [23:14] these bits determine the vertical size of LCD panel.
	 *					vlineval = 320 - 1;
	 * vfpd [13:6] : 发出最后一行数据后多长时间开始发vsync
	 *				T2 - T5 = 2;
	 *				vfpd = 2 - 1;
	 * vspw [5:0] : vsync 宽度 T1
	 *				vspw = 1 - 1;
	 **/
	lcd_regs->lcdcon2 = (7 << 24) | (319 << 14) | (1 << 6) | (0 << 0);

	/*
	 * LCDCON3:
	 * HBPD [25:19] HSYNC 之后多少个vclk 开始发送第一个像素数据
	 *				hbpd = T6 - T7 - T8 - 1 = 273 - 5 - 251 - 1;
	 *				HBPD = 16;
	 * HOZVAL [18:8] 每行像素个数 T11 = 240
	 *				HOZVAL = 240 - 1;
	 * HFPD [7:0] 每行结束到hsync 的vclk数 T8 - T11
	 *				HFPD = 251 - 240 - 1 = 10;
	 **/
	lcd_regs->lcdcon3 = (7 << 19) | (239 << 8) | (10 << 0);

	/*
	 * HSPW [7:0] hsync 宽度 T7 = 5
	 *				HSPW = 4;
	 **/
	lcd_regs->lcdcon4 = (4 << 0);

	/*
	 * 信号极性
	 * bit[11] this bit determines the format of 16 bpp output video data.
	 *			1 = 5:6:5 format
	 * bit[10]  0 = 下降沿取值
	 * bit[9] 1 = hsycn 低电平有效
	 * bit[8]  1 = vsync 信号反转 低电平有效
	 * bit[6] 0 = VDEN 不用反转
	 * bit[3] 0 = pwren 输出0
	 * bit[1] 0 = BSWAP disable
	 * bit[0] 1 = hwswp 开 2440 p413
	 **/
	lcd_regs->lcdcon5 = (1<<11) | (1<<9) | (1<<8) | (1<<0);
#endif
	/* 分配显存 */
	s3c_bfinfo->screen_base = dma_alloc_wc(NULL, PAGE_ALIGN(s3c_bfinfo->fix.smem_len),
							(dma_addr_t *)&s3c_bfinfo->fix.smem_start, GFP_KERNEL);
	if (!s3c_bfinfo->screen_base) {
		printk("2440 lcd drv:alloc screen mem error\n");
		return -ENOMEM;
	}
	memset(s3c_bfinfo->screen_base, 0x0, PAGE_ALIGN(s3c_bfinfo->fix.smem_len));
#if 1
	lcd_regs->lcdsaddr1 = (s3c_bfinfo->fix.smem_start >> 1) & ~(3 << 30);
	lcd_regs->lcdsaddr2 = (((s3c_bfinfo->fix.smem_start +
						s3c_bfinfo->fix.smem_len) >> 1)) & 0x1fffff;
	lcd_regs->lcdsaddr3 = (240*2/2) & 0x3ff;
	lcd_regs->tpal = 0;
//	lcd_regs->tconsel = ((0xCE6) & ~7) | 1<<4;

	/* enable lcd controller */
	lcd_regs->lcdcon1 |= (1<<0);
	lcd_regs->lcdcon5 |= (1<<3);

	
	printk("lcd drv: screen_base:%p,smem_start:%p\n",s3c_bfinfo->screen_base , (void *)s3c_bfinfo->fix.smem_start);
#endif
	*gpbdat |= 1; // enable led;
	printk("lcd drv enable led\n");

	/* 注册 */
	register_framebuffer(s3c_bfinfo);

	return 0;
out:
	return ret;
}

static void __exit mylcd_exit(void)
{
	unregister_framebuffer(s3c_bfinfo);
	*gpbdat &= ~(1);
	dma_free_wc(NULL, PAGE_ALIGN(s3c_bfinfo->fix.smem_len),
				s3c_bfinfo->screen_base, s3c_bfinfo->fix.smem_start);
	iounmap(lcd_regs);
	release_mem_region(lcd_mem->start, resource_size(lcd_mem));
	iounmap(gpbcon);
	iounmap(gpccon);
	iounmap(gpdcon);
	iounmap(gpgcon);
	framebuffer_release(s3c_bfinfo);
}

module_init(mylcd_init);
module_exit(mylcd_exit);

MODULE_AUTHOR("Ade <ade.kernel@gmail.com>");
MODULE_DESCRIPTION("Framebuffer driver for the s3c2440");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:s3c2440-lcd");
