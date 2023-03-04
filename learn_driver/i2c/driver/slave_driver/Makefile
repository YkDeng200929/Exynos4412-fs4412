ifeq ($(KERNELRELEASE),)

#KERNEL_BUILD=/lib/modules/$(shell uname -r)/build
KERNEL_BUILD=/home/linux/fs4412/kernel/linux-3.14
MODULE_PATH=$(shell pwd)

module:
	$(MAKE) -C $(KERNEL_BUILD) M=$(MODULE_PATH) modules

clean:
	$(MAKE) -C $(KERNEL_BUILD) M=$(MODULE_PATH) clean

install:
	cp mpu6050_driver.ko /home/linux/fs4412/share/rootfs

else
	obj-m = mpu6050_driver.o
endif

