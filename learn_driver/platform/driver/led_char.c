#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <asm/io.h>
#include "led_char.h"

// 以下是字符设备驱动框架

// 设备号
#define MAJOR_ID 	255
#define MINOR_ID 	3

#define LED1_ON 	0X11
#define LED1_OFF  	0x22

#define LED_CON 0
#define LED_DAT 4


// struct file_operation 是 cdev的成员

static struct class *led_class;
struct led_device *pled;

void led_init(struct led_device *pled)
{
	int pin = pled->pin;
	int mode = pled->mode;
	int bits  = pled->bits;
	int mask  = (1 << bits) - 1;
	
	int led = readl(pled->led1_reg + LED_CON);
	led &= ~(mask << (pin * bits));
	led |=  (mode << (pin * bits));
	writel(led, pled->led1_reg + LED_CON);

	return ;
}

//@ padding: 偏移量
void led_control_on(struct led_device *pled)
{
	int pin = pled->pin;
	int level = pled->level;
	
	int lighter_on = readl(pled->led1_reg + LED_DAT);
	printk("on: 0x%x\n", lighter_on);
	lighter_on &= ~(1 << pin);
	lighter_on |= (level << pin);
	writel(lighter_on, pled->led1_reg + LED_DAT);

	return ;
}

void led_control_off(struct led_device *pled)
{
	int level = pled->level;
	int pin = pled->pin;

	int lighter_off = readl(pled->led1_reg + LED_DAT);
	printk("off: 0x%x\n", lighter_off);

	printk("Lighter_off reg:  0x%x\n", lighter_off);
	lighter_off &= ~(1 << pin);	
	printk("Lighter_off &= pin:  0x%x\n", lighter_off);
	lighter_off |= (!level << pin);
	writel(lighter_off, pled->led1_reg + LED_DAT);

	return;
}

static int led_device_open(struct inode *inode, struct file *file)
{
	struct led_device *pled = container_of(inode->i_cdev, struct led_device, led_cdev);
	// 将物理地址与虚拟地址映射
	printk("reg_phy_addr: 0x%x reg_phy_size: %d\n", pled->reg_phy_addr, pled->reg_phy_size);

	file->private_data = pled;
	pled->led1_reg = ioremap(pled->reg_phy_addr, pled->reg_phy_size);
	printk("reg1: 0x%x\n", pled->led1_reg);

	if (!pled->led1_reg) {
		printk("Fail to ioremap\n");
		return -EIO;
	}
	printk("led device open\n");
	
	led_init(pled);
	printk("Init ok\n");
	
	return 0;
}

int led_device_release(struct inode *inode, struct file *file)
{
	printk("led device release\n");
	return 0;
}

static long led_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct led_device *pled = (struct led_device *)file->private_data;

	switch (cmd) {
		case LED1_ON: {
			printk("Led1 on\n");
			led_control_on(pled);
			break;
		}
		case LED1_OFF: {
			printk("Led1 off\n");
			led_control_off(pled);
			break;
		}
	}

	return 0;
}

static const struct file_operations led_deivce = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= led_ioctl,
	.open		= led_device_open,
	.release	= led_device_release,
};

int led_char_driver_register(struct led_device **ppled)
{
	int err;
	struct led_device *pled = NULL;

	pled = kmalloc(sizeof(*pled), GFP_KERNEL);
	if (!pled) {
		printk("Fail to Kmalloc\n");
		err = -ENOMEM;
		goto err_kmalloc;
	}
	cdev_init(&pled->led_cdev, &led_deivce);

	// 宏函数生成设备号
	pled->dev_id = MKDEV(MAJOR_ID, MINOR_ID);

	// 注册设备号
	err = register_chrdev_region(pled->dev_id, 1, "led_id");
	
	if (err) {
		printk(KERN_ERR "CUSE: failed to register chrdev region\n");
		goto err_register_chrdev_region;
	}

	// 通过设备号添加字符设备
	err = cdev_add(&pled->led_cdev, pled->dev_id, 1);
	if (err) {
		printk("failed to cdev_add\n");
		goto err_cdev_add;
	}

	// 创建类
	led_class = class_create(THIS_MODULE, "led_class");
	if (IS_ERR(led_class)) {
		printk("Fail to create class\n");
		goto err_class_create;
	}

	// 自动创建设备文件
	pled->dev = device_create(led_class, NULL, pled->dev_id, NULL, "led_device");
	if (IS_ERR(pled->dev)) {
		printk("Fail to device_create\n");
		goto err_device_create;
	}
	*ppled = pled;

	return 0;

// 错误跳转处理, 避免重复编写
err_device_create:
	device_destroy(led_class, pled->dev_id);
	
err_class_create:
	class_destroy(led_class);
	
err_cdev_add:
	cdev_del(&pled->led_cdev);

err_register_chrdev_region:
	unregister_chrdev_region(pled->dev_id, 1);
	kfree(pled);

err_kmalloc:	
	return err;
}

void led_char_driver_unregister(struct led_device *pled)
{
	device_destroy(led_class, pled->dev_id);
	class_destroy(led_class);
	cdev_del(&pled->led_cdev);
	unregister_chrdev_region(pled->dev_id, 1);
	kfree(pled);

	return ;
}

