# TODO ToG: Add license header
KERNEL_MODULE_NAME := marian/snd-marian

DBG_LVL_ERROR	:= 1
DBG_LVL_WARN	:= 2
DBG_LVL_INFO	:= 3
DBG_LVL_DEBUG	:= 4

DBG_LEVEL := $(DBG_LVL_WARN)

ifneq ($(KERNELRELEASE),)
obj-m := marian/
else
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules \
        EXTRA_CFLAGS="-DDBG_LEVEL=$(DBG_LEVEL)"

install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install \
        EXTRA_CFLAGS="-DDBG_LEVEL=$(DBG_LEVEL)"

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

endif
