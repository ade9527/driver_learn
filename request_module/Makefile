KERNELDIR ?=../../kernel/linux-4.6

obj-m := req_drv.o
PWD := $(shell pwd)

module := $(patsubst %.o,%.ko,$(obj-m))
all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

.PHONY: clean install all
clean:
	rm -rf *.o *.ko *.mod* *.order *.symvers

install:
	cp $(module) ~/nfs/2440_rootfs/root/modules/
