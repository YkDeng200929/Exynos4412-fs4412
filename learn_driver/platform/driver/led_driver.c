#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h> // open firmware
#include "led_char.h"

MODULE_LICENSE("GPL v2");

// 解析设备属性

struct led_device_info
{
	uint32_t pin;
	uint32_t bits;
	uint32_t mode;
	uint32_t level;
};

int parse_device_property(struct platform_device *pdev,
						  struct led_device_info *info)
{
	int ret;

	struct device_node *np = pdev->dev.of_node;
	ret  = of_property_read_u32(np, "pin", &info->pin);
	ret += of_property_read_u32(np, "bits", &info->bits);
	ret += of_property_read_u32(np, "mode", &info->mode);
	ret += of_property_read_u32(np, "level", &info->level);

	if (ret){
		printk("Fail to of_property_read_u32\n");
		return ret;
	}	
	printk("Pin: %d\n", info->pin);
	printk("mode: 0x%x\n", info->bits);
	printk("bits: %d\n", info->mode);
	printk("level: %d\n", info->level);

	return 0;
}

static int led_probe(struct platform_device *pdev)
{
	int err;
	struct resource* res;
	// 调用一次probe创建一个设备
	struct led_device *pled = NULL;
	struct led_device_info led_info;

	printk("led_probe\n");
	
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	// 因为设备树dts文件中的节点名为fs4412-led所以打印出来的不是fs4412_led
	printk("Device name: %s\n", res->name);
	
	if (!res){
		
		printk("Fail to get resource\n");
		return -ENODEV;
	}
	
	printk("resource: 0x%x\n", (uint32_t)res->start);

	// 注册字符设备
	err = parse_device_property(pdev, &led_info);
	if (err){
		printk("Fail to parse_device_property\n");
		return err;
	}
	err = led_char_driver_register(&pled);
		if (err){
		printk("Fail to led_char_driver_register\n");
		return err;
	}
	// 为避免被覆盖,所以将pled存在platform_device结构体中,注销时取出
	pled->reg_phy_addr = res->start;
	pled->reg_phy_size = resource_size(res);
	pled->level = led_info.level;	
	pled->pin = led_info.pin;
	pled->bits = led_info.bits;
	// 少加了模式位
	pled->mode = led_info.mode;
	
	platform_set_drvdata(pdev, pled);

	return 0;
}

static int led_remove(struct platform_device *pdev)
{
	// 取出platform_device中存放的pled, 然后调用char_driver中的取消设备注册函数来注销
	struct led_device *pled = platform_get_drvdata(pdev);

	printk("led_remove\n");
	led_char_driver_unregister(pled);

	return 0;
}

static const struct of_device_id led_type[] = {
	{ .compatible = "fs4412_led", },
	{},
};
MODULE_DEVICE_TABLE(of, led_type);

struct platform_driver  led_driver = {
	.probe		= led_probe,
	.remove		= led_remove,
	.driver = {
		.name  = "led",
		.owner = THIS_MODULE,
		.of_match_table = led_type,
	}
};

int led_driver_init(void)
{
	return platform_driver_register(&led_driver);
}
void led_driver_exit(void)
{
	platform_driver_unregister(&led_driver);
	return ;
}

module_init(led_driver_init);
module_exit(led_driver_exit);

