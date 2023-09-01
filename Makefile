# TODO ToG: Add license header
KERNEL_MODULE_NAME := marian/snd-marian

ifneq ($(KERNELRELEASE),)
obj-m := marian/
else
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

endif
