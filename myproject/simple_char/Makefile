# SPDX-License-Identifier: GPL-2.0

 
ifeq ($(INDEPEND_BUILD),)
obj-$(CONFIG_SIMPLE_CHAR) := simple_char.o
else # make INDEPEND_BUILD=1 
obj-m := simple_char.o
KERNELDIR ?= /lib/modules/`uname -r`/build
.PHONY:modules
moudules:
	make -C $(KERNELDIR) M=$(PWD) modules
.PHONY:clean
clean:
	make -C $(KERNELDIR) M=$(PWD) clean
	rm -f *.ko
endif