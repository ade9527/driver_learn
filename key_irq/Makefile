KERNELDIR ?=~/workspaces/test/s3c2440/kernel/linux-4.6

#drv-file := $(wildcard *_drv.c)
obj-m := key_irq_drv.o#$(patsubst %.c,%.o,$(drv-file))
PWD := $(shell pwd)

module := $(patsubst %.o,%.ko,$(obj-m))
drv-file := $(patsubst %.o,%.c,$(obj-m))


module: ${drv-file}
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	rm -rf *.o  *.mod* *.order *.symvers
	rm -rf .*.ko* .tmp* .*.o.*

.PHONY: clean install all
clean:
	rm -rf *.ko

install: all
	sudo cp $(module) ~/nfs/2440_rootfs/root/modules/
