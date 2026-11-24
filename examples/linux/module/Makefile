# Builds the module against a configured and built kernel tree.
# Usage: make KDIR=<kernel tree> ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
KDIR ?= /lib/modules/$(shell uname -r)/build

obj-m += sfw_counter.o
ccflags-y += -g # debug info for addr2line and faddr2line

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
