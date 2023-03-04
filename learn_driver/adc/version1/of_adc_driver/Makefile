ifeq ($(KERNELRELEASE),)

#KERNEL_DIR ?=/lib/modules/$(shell uname -r)/build 
KERNEL_DIR = /home/linux/fs4412/kernel/linux-3.14/ 
ARM_KERNELPATH = /home/linux/new_fs4412/kernel/linux-3.14/
PWD :=$(shell pwd)
COMPILEPATH = $(shell pwd)

modules:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules 

.PHONY:modules clean 

arm-modules:
	make -C $(ARM_KERNELPATH) M=$(COMPILEPATH) modules ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi-
#	arm-none-linux-gnueabi-gcc $(FUN) -o arm-output
	cp ./adc_driver.ko /home/linux/new_fs4412/rootfs/

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean

install:
	cp  adc_driver.ko /home/linux/new_fs4412/rootfs/
else 
	obj-m := adc_driver.o 
endif
