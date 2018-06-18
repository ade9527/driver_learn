#include <linux/major.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/bitops.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/genhd.h>
#include <linux/hdreg.h>

#define RAMBLOCK_SIZE (0x400*0x400)

static struct gendisk *ramblock_disk;
static struct request_queue *ramblock_queue;
static DEFINE_SPINLOCK(ramblock_lock);
static int major;
static unsigned char *ramblock_buf;

static void do_ramblock_request(struct request_queue* q)
{
	struct request *req;
	static int rcnt = 0, wcnt = 0;

	req = blk_fetch_request(q);
	while(req) {
		unsigned long offset = blk_rq_pos(req) << 9;
		unsigned long len = blk_rq_cur_bytes(req);
		void *buffer = bio_data(req->bio);

		printk("access: start:%lu, len:%lu, buffer:%p\n", offset, len, buffer);
		if (len + offset > RAMBLOCK_SIZE) {
			printk("bad access: block=%llu, count=%u\n",
					(unsigned long long)blk_rq_pos(req),
					blk_rq_cur_sectors(req));
			goto done;
		}
		
		if (rq_data_dir(req) == READ) {
			printk("do_ramblock_request read %d\n", ++rcnt);
			memcpy(buffer, ramblock_buf + offset, len);
		} else {
			memcpy(ramblock_buf + offset, buffer, len);
			printk("do_ramblock_request write %d\n", ++wcnt);
		}
done:
		if (!__blk_end_request_cur(req, 0)) {
			req = blk_fetch_request(q);
		} else {
		}
	}
}

static int ramblock_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
	printk("ramblock: getgeo\n");

	geo->heads = 2;
	geo->cylinders = 32;
	geo->sectors = RAMBLOCK_SIZE / 2 /32 /512;
	return 0;
}

static struct block_device_operations ramblock_fops = {
	.owner = THIS_MODULE,
	.getgeo = ramblock_getgeo,
};

static int __init ramblock_init(void)
{
	int ret = 0;

	/* 1.分配一个gendisk struct */
	major = register_blkdev(0, "ramblock");
	if (major <= 0) {
		printk("ramblock.c: get major failed\n");
		return major;
	}
	
	ramblock_disk = alloc_disk(16);
	if (!ramblock_disk) {
		printk("ramblock.c: alloc_disk err\n");
		ret = -ENOMEM;
		goto out_disk;
	}


	/* 2.设置*/
	/* 2.1.分配设置队列:提供读写能力 */
	ramblock_queue = blk_init_queue(do_ramblock_request, &ramblock_lock);
	if (!ramblock_queue) {
		printk("ramblock.c: init queue err\n");
		ret = -ENOMEM;
		goto out_queue;
	}
	ramblock_disk->queue = ramblock_queue;

	/* 2.2 设置其它属性,如容量 */
	ramblock_disk->major = major;
	ramblock_disk->first_minor = 0;
	sprintf(ramblock_disk->disk_name, "ramblock");
	ramblock_disk->fops = &ramblock_fops;
	set_capacity(ramblock_disk, RAMBLOCK_SIZE/512);

	/* 分配一块内存 */
	ramblock_buf = kzalloc(RAMBLOCK_SIZE, GFP_KERNEL);
	if (!ramblock_buf) {
		printk("ramblock.c: alloc buf err\n");
		ret = -ENOMEM;
		goto out_buf;
	}

	/* 3.注册 */
	add_disk(ramblock_disk);

	return 0;
out_buf:
	blk_cleanup_queue(ramblock_queue);
out_queue:
	put_disk(ramblock_disk);
out_disk:
	unregister_blkdev(major, "ramblock");
	return ret;
}

static void __exit ramblock_exit(void)
{
	del_gendisk(ramblock_disk);
	blk_cleanup_queue(ramblock_queue);
	put_disk(ramblock_disk);
	unregister_blkdev(major, "ramblock");
	kfree(ramblock_buf);
}


module_init(ramblock_init);
module_exit(ramblock_exit);
MODULE_LICENSE("GPL");
