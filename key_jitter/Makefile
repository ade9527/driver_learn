KERNELDIR ?=../../kernel/linux-4.6

#drv-file := $(wildcard *_drv.c)
obj-m := key_jitter_drv.o#$(patsubst %.c,%.o,$(drv-file))
PWD := $(shell pwd)

module := $(patsubst %.o,%.ko,$(obj-m))
all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

.PHONY: clean install all
clean:
	rm -rf *.o *.ko *.mod* *.order *.symvers

install: all
	sudo cp $(module) ~/nfs/2440_rootfs/root/modules/
