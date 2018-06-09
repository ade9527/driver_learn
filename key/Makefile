KERNELDIR ?=../../kernel/linux-4.6

obj-m := key_drv.o
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

.PHONY: clean
clean:
	rm -rf *.o *.ko *.mod* *.order *.symvers
