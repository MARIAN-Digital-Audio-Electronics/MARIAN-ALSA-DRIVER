# MARIAN PCIe soundcards ALSA driver
#
# Author: Tobias Groß <theguy@audio-fpga.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details at:
# http://www.gnu.org/licenses/gpl-2.0.html

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
        EXTRA_CFLAGS="-DDBG_LEVEL=$(DBG_LEVEL) -include $(PWD)/snd_printk_compat.h"

install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install \
        EXTRA_CFLAGS="-DDBG_LEVEL=$(DBG_LEVEL) -include $(PWD)/snd_printk_compat.h"

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

endif
