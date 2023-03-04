#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h> // open firmware
#include "led_char.h"

MODULE_LICENSE("GPL v2");

// �����豸����

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
	// ����һ��probe����һ���豸
	struct led_device *pled = NULL;
	struct led_device_info led_info;

	printk("led_probe\n");
	
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	// ��Ϊ�豸��dts�ļ��еĽڵ���Ϊfs4412-led���Դ�ӡ�����Ĳ���fs4412_led
	printk("Device name: %s\n", res->name);
	
	if (!res){
		
		printk("Fail to get resource\n");
		return -ENODEV;
	}
	
	printk("resource: 0x%x\n", (uint32_t)res->start);

	// ע���ַ��豸
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
	// Ϊ���ⱻ����,���Խ�pled����platform_device�ṹ����,ע��ʱȡ��
	pled->reg_phy_addr = res->start;
	pled->reg_phy_size = resource_size(res);
	pled->level = led_info.level;	
	pled->pin = led_info.pin;
	pled->bits = led_info.bits;
	// �ټ���ģʽλ
	pled->mode = led_info.mode;
	
	platform_set_drvdata(pdev, pled);

	return 0;
}

static int led_remove(struct platform_device *pdev)
{
	// ȡ��platform_device�д�ŵ�pled, Ȼ�����char_driver�е�ȡ���豸ע�ắ����ע��
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

