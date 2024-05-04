# SPDX-License-Identifier: GPL-2.0

ifeq ($(INDEPEND_BUILD),)
obj-$(CONFIG_TEST_DRIVER) += test.o
else # make INDEPEND_BUILD=1
obj-m = test.o
KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
.PHONY:modules
modules:
	make -C $(KERNEL_DIR) M=$(PWD) modules
clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean
	rm -f *.ko
endif