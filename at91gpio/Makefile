MOUNT_DIR:=/home/Mount/AT91
KERNELDIR := /home/AT91/linux-2.6.39
PWD := $(shell pwd)

obj-m := at91gpio.o
COPYTOMOUNT := cp at91gpio.ko $(MOUNT_DIR)

CROSS_COMPILE := arm-none-linux-gnueabi-

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld

modules:
	$(MAKE) -C $(KERNELDIR) SUBDIRS=$(PWD) ARCH=arm modules
	$(COPYTOMOUNT)
modules_install:
	$(MAKE) -C $(KERNELDIR) SUBDIRS=$(PWD) ARCH=arm modules_install

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.symvers
