ifeq ($(KERNELRELEASE),)

#KERNEL_DIR ?=/lib/modules/$(shell uname -r)/build 
KERNEL_DIR = /home/linux/fs4412/kernel/linux-3.14/ 
PWD :=$(shell pwd)

modules:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules 

.PHONY:modules clean 
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean

install:
	cp  adc_driver.ko /home/linux/fs4412/fs/rootfs
else 
	obj-m := adc_driver.o 
endif
