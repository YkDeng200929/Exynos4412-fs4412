ifeq ($(KERNELRELEASE),)

X86_KERNELPATH = /lib/modules/$(shell uname -r)/build
ARM_KERNELPATH = /home/linux/new_fs4412/kernel/linux-3.14/
COMPILEPATH = $(shell pwd)
FUN = led_test.c

x86-modules:
	make -C $(X86_KERNELPATH) M=$(COMPILEPATH) modules
#	gcc $(FUN) -o x86-output

arm-modules:
	make -C $(ARM_KERNELPATH) M=$(COMPILEPATH) modules ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi-
	#arm-none-linux-gnueabi-gcc $(FUN) -o arm-output
	cp ./led_device.ko /home/linux/new_fs4412/rootfs/
	#cp ./arm-output /home/linux/new_fs4412/rootfs/

clean:
	make -C $(X86_KERNELPATH) M=$(COMPILEPATH) clean

else
	obj-m = led_device.o

endif
