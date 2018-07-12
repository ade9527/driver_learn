# mtd

## drivers/mtd/nand/s3c2410.c

```
s3c24xx_nand_probe()
    s3c2410_nand_inithw
        s3c2410_nand_setrate /* 根据platform data 的数据初始化nand ctler */
	s3c2410_nand_init_chip
	nand_scan_ident
		nand_get_flash_type
	s3c2410_nand_add_partition

```
