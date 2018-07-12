#include <linux/module.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/in.h>
#include <linux/jiffies.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/gfp.h>
#include <linux/io.h>
#include <asm/irq.h>
#include <linux/ip.h>

static struct net_device *vnet_dev;



static void emulator_rx_packet(struct sk_buff *skb,
						  struct net_device *dev)
{
	unsigned char *type;
	struct iphdr *ih;
	__be32 *saddr, *daddr, tmp;
	unsigned char tmp_dev_addr[ETH_ALEN];
	struct ethhdr *ethhdr;
	struct sk_buff *rx_skb;

	/* 对调 '源/目的'的mac地址 */
	ethhdr = (struct ethhdr *)skb->data;
	memcpy(tmp_dev_addr, ethhdr->h_dest, ETH_ALEN);
	memcpy(ethhdr->h_dest, ethhdr->h_source, ETH_ALEN);
	memcpy(ethhdr->h_source, tmp_dev_addr, ETH_ALEN);

	/* 对调 ip地址 */
	ih = (struct iphdr *)(skb->data + sizeof(struct ethhdr));
	saddr = &ih->saddr;
	daddr = &ih->daddr;

	tmp = *saddr;
	*saddr = *daddr;
	*daddr = tmp;

	type = skb->data + sizeof(struct ethhdr) + sizeof(struct iphdr);
	// 修改数据类型, 原来的0x8表示ping
	*type = 0; /* 0 表示reply(答复) */

	ih->check = 0; /* and rebuild the checksum (ip needs it) */
	ih->check = ip_fast_csum((unsigned char *)ih, ih->ihl);

	// 构造一个sk_buff
	rx_skb = dev_alloc_skb(skb->len + 2);
	skb_reserve(rx_skb, 2); /* align IP on 16B boundary */
	memcpy(skb_put(rx_skb, skb->len), skb->data, skb->len);

	/* Write metadata, and then pass to the receive level */
	rx_skb->dev = dev;
	rx_skb->protocol = eth_type_trans(rx_skb, dev);
	rx_skb->ip_summed = CHECKSUM_UNNECESSARY; /* don't check it */

	dev->stats.rx_packets++;
	dev->stats.rx_bytes += skb->len;

	netif_rx(rx_skb);
}


static netdev_tx_t vnet_send_packet(struct sk_buff *skb,
						  struct net_device *dev)
{
	static int cnt = 0;
	unsigned char *type;

	/* 对于真是的网卡, 把skb里的数据通过网卡发送出去 */

	netif_stop_queue(dev); /* 停止该网卡的队列 */


	type = skb->data + sizeof(struct ethhdr) + sizeof(struct iphdr);

	printk("skb type = %d, cnt = %d\n", (int)*type, ++cnt);
	if (*type == 0x8) {
		/* 构造一个假的sk_buff, 上报 */
		emulator_rx_packet(skb, dev);
	}

	dev_kfree_skb(skb); /* 释放skb*/
	netif_wake_queue(dev); /* 数据全部发送出去后,唤醒网卡的队列 */

	/* 更新统计信息 */
	dev->stats.tx_packets++;
	dev->stats.tx_bytes += skb->len;

	return NETDEV_TX_OK;
}

static struct net_device_ops vnetdev_ops = {
	.ndo_start_xmit = vnet_send_packet,
	
};

static int __init vnet_init(void)
{
	vnet_dev = alloc_netdev(0, "vnet%d", NET_NAME_UNKNOWN, ether_setup);
	if (!vnet_dev) {
		printk("vnet.c: fail for alloc netdev\n");
		return -ENOMEM;
	}
	vnet_dev->netdev_ops = &vnetdev_ops;

	vnet_dev->flags    |= IFF_NOARP;
//	vnet_dev->features |= NETIF_F_NO_CSUM;

	register_netdev(vnet_dev);
	return 0;
}

static void __exit vnet_exit(void)
{
	unregister_netdev(vnet_dev);
}

module_init(vnet_init);
module_exit(vnet_exit);
MODULE_LICENSE("GPL");

