#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <scsi/scsi.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_tcq.h>
#include <scsi/scsi_cmnd.h>

// 以下是字符设备驱动框架

// 设备号
#define MAJOR_ID 	255
#define MINOR_ID 	3

#define LED1_ON 	0X11
#define LED1_OFF  	0x22
#define LED2_ON		0x33
#define LED2_OFF 	0x44
#define LED3_ON		0x55
#define LED3_OFF	0x66
#define LED4_ON		0x77
#define LED4_OFF	0x88

#define LED_PHY_ADDR  		0x11000c40    	// led1
#define LED_PHY_ADDRII 		0x11000c20   	// led2
#define LED_PHY_ADDRIII		0x114001E0	  	// led3_1 && 2

#define LED_ADDR_SIZE 		8

#define LED_CON 0
#define LED_DAT 4

MODULE_LICENSE("GPL v2");

// struct file_operation 是 cdev的成员
struct led_device {
	void __iomem *led1_reg;
	void __iomem *led2_reg;
	void __iomem *led3_reg;
	void __iomem *led4_reg;
	dev_t dev_id;
	struct cdev led_cdev;	
	struct device *dev;
};

struct led_device *pled = NULL;

static struct class *led_class;

//@ padding: 偏移量
void led_control_on(void __iomem *reg, int padding)
{
	int lighter_on = readl(reg + LED_DAT);
	lighter_on |= (0x1 << padding);
	writel(lighter_on, reg + LED_DAT);
}

void led_control_off(void __iomem *reg, int padding)
{
	int lighter_off = readl(reg + LED_DAT);
	lighter_off &= ~(0x1 << padding);
	writel(lighter_off, reg + LED_DAT);
}

void led_init(void __iomem *reg, int padding)
{
	int led = readl(reg);
	led &= ~(0xf << padding);
	led |=  (0x1 << padding);
	writel(led, reg);
}

static int led_device_open(struct inode *inode, struct file *file)
{
	// 将物理地址与虚拟地址映射
	pled->led1_reg = ioremap(LED_PHY_ADDR, LED_ADDR_SIZE);
	pled->led2_reg = ioremap(LED_PHY_ADDRII, LED_ADDR_SIZE);
	pled->led3_reg = ioremap(LED_PHY_ADDRIII, LED_ADDR_SIZE);
	pled->led4_reg = ioremap(LED_PHY_ADDRIII, LED_ADDR_SIZE);

	if (!pled->led1_reg) {
		printk("Fail to ioremap\n");
		return -EIO;
	}
	printk("led device open\n");
	
	led_init(pled->led1_reg, 28);
	led_init(pled->led2_reg, 0);
	led_init(pled->led3_reg, 16);
	led_init(pled->led4_reg, 20);
	
	return 0;
}

int led_device_release(struct inode *inode, struct file *file)
{
	printk("led device release\n");
	return 0;
}

static long led_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
		case LED1_ON: {
			printk("Led1 on\n");
			led_control_on(pled->led1_reg, 7);
			break;
		}
		case LED1_OFF: {
			printk("Led1 off\n");
			led_control_off(pled->led1_reg, 7);
			break;
		}
		case LED2_ON: {
			printk("Led2 on\n");
			led_control_on(pled->led2_reg, 0);
			break;
		}
		case LED2_OFF: {
			printk("Led2 off\n");
			led_control_off(pled->led2_reg, 0);
			break;
		}
		case LED3_ON: {
			printk("Led3 on\n");
			led_control_on(pled->led3_reg, 4);
			break;
		}
		case LED3_OFF: {
			printk("Led3 off\n");
			led_control_off(pled->led3_reg, 4);
			break;
		}
		case LED4_ON: {
			printk("Led4 on\n");
			led_control_on(pled->led4_reg, 5);
			break;
		}
		case LED4_OFF: {
			printk("Led4 off\n");
			led_control_off(pled->led4_reg, 5);
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

static int led_driver_init(void)
{
	int err;

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

static void led_driver_exit(void)
{
	device_destroy(led_class, pled->dev_id);
	class_destroy(led_class);
	cdev_del(&pled->led_cdev);
	unregister_chrdev_region(pled->dev_id, 1);
	kfree(pled);

	return ;
}

module_init(led_driver_init);
module_exit(led_driver_exit);
