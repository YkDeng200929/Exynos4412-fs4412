#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

#include "led.h"

MODULE_LICENSE("GPL");

struct led_device
{
	dev_t devno;
	int   gpio;
	int   trigger_level;
	struct cdev cdev;
	struct class *cls;
	struct device *device;
};

int exynos_led_open(struct inode *inode, struct file *file)
{
	struct led_device *pled = container_of(inode->i_cdev,struct led_device,cdev);
	printk("open success");
	file->private_data = pled;

	return 0;
}

int exynos_led_release(struct inode *inode, struct file *file)
{
	struct led_device *pled = container_of(inode->i_cdev,struct led_device,cdev);
	
	//关闭led灯
	gpio_direction_output(pled->gpio,!pled->trigger_level);
	
	return 0;
}

long exynos_led_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret;
	int level;
	struct led_device *pled =file->private_data;
		
	switch(cmd)
	{
		case DEV_LED_ON:
			level = pled->trigger_level;
			break;

		case DEV_LED_OFF:
			level = !(pled->trigger_level);
			break;

		default:
			return -EINVAL;
	}

	printk("gpio : %d,level : %d\n",pled->gpio,level);
	ret = gpio_direction_output(pled->gpio,level);
	if (ret) {
		printk("Setting LED GPIO direction failed!\n");
		return ret;
	}

	return 0;
}

struct file_operations exynos_led_fops = {
	.owner   = THIS_MODULE,
	.open    = exynos_led_open,
	.release = exynos_led_release,
	.unlocked_ioctl = exynos_led_ioctl,
};

int register_led_device(struct platform_device *pdev)
{
	int ret;
	struct led_device *pled = platform_get_drvdata(pdev);

	cdev_init(&pled->cdev,&exynos_led_fops);

	//动态申请设备号
	ret = alloc_chrdev_region(&pled->devno,0, 1,pdev->name);
	if(ret < 0){
		printk("Cannot alloc dev num");
		return ret;
	}

	//添加字符设备
	ret = cdev_add(&pled->cdev,pled->devno, 1);
	if(ret < 0){
		printk("Failed to add cdev");
		goto err_cdev_add;
	}

	//创建led类
	pled->cls = class_create(THIS_MODULE, "fs-led");
	if(IS_ERR(pled->cls)){
		printk("Failed to class_create\n");
		ret = PTR_ERR(pled->cls);
		goto err_class_create;
	}

	
	//导出设备信息到用户空间,由udev来创建设备节点
	pled->device = device_create(pled->cls,NULL,pled->devno,NULL,pdev->name);
	if(IS_ERR(pled->device)){
			printk("Failed to device_create");
			ret = PTR_ERR(pled->device);
			goto err_device_create;
	}

	return 0;

err_device_create:
	class_destroy(pled->cls);

err_class_create:
	cdev_del(&pled->cdev);

err_cdev_add:
	unregister_chrdev_region(pled->devno,1);

	return ret;
}

static int  exynos_led_probe(struct platform_device *pdev)
{
	int ret;
	struct led_device *pled;
	struct device_node *led_node = pdev->dev.of_node;
	
	printk("match\n");

	printk("pdev->name : %s\n",pdev->name);

	//动态分配led_device内存
	pled = devm_kzalloc(&pdev->dev,sizeof(struct led_device),GFP_KERNEL);
	if (!pled){
		printk("Fail to malloc memory for pled\n");
		return -ENOMEM;
	}
	platform_set_drvdata(pdev, pled);

	//获取led灯对应的gpio口
	/*
	 *@led_node   : 设备树的节点对应的结构体(struct device_node)地址
	 *@led-gpio   : gpio的属性名称
	 *@0          : 这个设备的第一个gpio 
	 *@flags      : 获取设备树的gpio描述的标志
	<p align="justify"> *return      : 返回的是gpio的编号
	 */
	pled->gpio = of_get_named_gpio_flags(led_node,"led-gpio",0,(enum of_gpio_flags *)&pled->trigger_level);
	printk("os gpio number:%d\n",pled->gpio);
	//判断是否是有效的gpio口
	if(!gpio_is_valid(pled->gpio)){
		printk("invalid led-gpios:%d\n",pled->gpio);
		return -ENODEV;
	}

	//请求占用gpio
	ret = devm_gpio_request(&pdev->dev,pled->gpio,"led-gpio");
	if (ret){
		printk("Failed GPIO request for GPIO %d: %d\n",pled->gpio, ret);
		return ret;
	}

	//注册led_device到系统
	ret = register_led_device(pdev);
	if(ret < 0){
		printk("Fail to register led device!\n");
		return ret;
	}
		
	return 0;
}


static int exynos_led_remove(struct platform_device *pdev)
{
	struct led_device *pled = platform_get_drvdata(pdev);
	
	printk("led_remove: device remove\n");
		
	device_destroy(pled->cls,pled->devno);
	class_destroy(pled->cls);
	cdev_del(&pled->cdev);
	unregister_chrdev_region(pled->devno,1);

	return 0;
}


#if defined(CONFIG_OF)
static const struct of_device_id exynos_led_dt_ids[] = {
	{ .compatible = "fs4412-led-gpio" },
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, exynos_led_dt_ids);
#endif

static struct platform_driver exynos_led_driver = {
	.probe		= exynos_led_probe,
	.remove     = exynos_led_remove,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "exynos-led",
		.of_match_table = of_match_ptr(exynos_led_dt_ids),
	},
};

int __init led_init(void)
{
	int ret;
	
	printk("Register led driver to platfrom bus!\n");

	ret = platform_driver_register(&exynos_led_driver);
	
	return ret;	
}



void __exit led_exit(void)
{
	printk("Remove led driver to platfrom bus!\n");
	
	platform_driver_unregister(&exynos_led_driver);
	return;
}

module_init(led_init);
module_exit(led_exit);
