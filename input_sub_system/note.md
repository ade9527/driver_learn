# input子系统说明
1.`drivers/input/input.c`文件提供了input子系统的架构模块(deviec的注册函数,handler的注册函数,minor分配函数等...)
```

```
struct input_dev
```
struct input_dev {
	const char *name;
	const char *phys;
	const char *uniq;
	struct input_id id;

	unsigned long propbit[BITS_TO_LONGS(INPUT_PROP_CNT)];

	unsigned long evbit[BITS_TO_LONGS(EV_CNT)]; // 表示能产生哪类事件
	unsigned long keybit[BITS_TO_LONGS(KEY_CNT)]; // 表示产生哪些按键
	unsigned long relbit[BITS_TO_LONGS(REL_CNT)]; // 表示能产生哪些鼠标事件
	unsigned long absbit[BITS_TO_LONGS(ABS_CNT)]; // 坐标
	unsigned long mscbit[BITS_TO_LONGS(MSC_CNT)];
	unsigned long ledbit[BITS_TO_LONGS(LED_CNT)];
	unsigned long sndbit[BITS_TO_LONGS(SND_CNT)];
	unsigned long ffbit[BITS_TO_LONGS(FF_CNT)];
	unsigned long swbit[BITS_TO_LONGS(SW_CNT)];
	
	....
};
```

初始化函数:
```

```

> BITMAP_LAST_WORD_MASK(nbits);
这个宏计算 nbits 个bit的mask(从低位算),还有一个从高位算起的版本`BITMAP_FIRST_WORD_MASK`

```
#define BITMAP_FIRST_WORD_MASK(start) (~0UL << ((start) & (BITS_PER_LONG - 1)))
#define BITMAP_LAST_WORD_MASK(nbits) (~0UL >> (-(nbits) & (BITS_PER_LONG - 1)))
```
