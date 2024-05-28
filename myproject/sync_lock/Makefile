# SPDX-License-Identifier: GPL-2.0

KBUILD_CFLAGS += -O0
ifeq ($(INDEPEND_BUILD),)
obj-$(CONFIG_SYNC_LOCK) := sync_lock.o
else # make INDEPEND_BUILD=1 
obj-m := sync_lock.o
KERNELDIR ?= /lib/modules/`uname -r`/build
.PHONY:modules
moudules:
	make -C $(KERNELDIR) M=$(PWD) modules
.PHONY:clean
clean:
	make -C $(KERNELDIR) M=$(PWD) clean
	rm -f *.ko
endif