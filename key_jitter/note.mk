# 利用定时器消除按键抖动

## 原理
	在按键中断中设置定时器延时10ms,每次抖动产生的中断都重新设置定时器到达时间,这样定时器只工作一次(抖动小于10ms)
> mod_timer(&key_timer, jiffies+HZ/100+1); /* 10ms + 1 防止jiffies到达临近点 */

## 定时器定义及操作函数
1. `struct timer_list`的定义如下
```
struct timer_list {
	/*
	 * All fields that change during normal runtime grouped to the
	 * same cacheline
	 */
	struct hlist_node	entry;
	unsigned long		expires; /* 定时器到达的时间 */
	void			(*function)(unsigned long); /* 到达处理函数 */
	unsigned long		data;
	u32			flags;
	int			slack;

#ifdef CONFIG_TIMER_STATS
	int			start_pid;
	void			*start_site;
	char			start_comm[16];
#endif
#ifdef CONFIG_LOCKDEP
	struct lockdep_map	lockdep_map;
#endif
};

```
2.定时器函数
```
init_timer(struct timer_list *);
add_timer(struct timer_list *);
int mod_timer(struct timer_list *timer, unsigned long expires);
```
