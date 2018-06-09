KERNELDIR ?=../../kernel/linux-4.6

obj-m := lcd_2440fb.o
PWD := $(shell pwd)

module := $(patsubst %.o,%.ko,$(obj-m))
all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

.PHONY: clean install all
clean:
	rm -rf *.o *.ko *.mod* *.order *.symvers

install:
	sudo cp $(module) ~/nfs/2440_rootfs/root/modules/
