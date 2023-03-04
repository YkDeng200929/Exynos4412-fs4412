#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#define LED_PHY_ADDR  		0x11000c40    	// led1
#define LED_PHY_ADDRII 		0x11000c20   	// led2
#define LED_PHY_ADDRIII		0x114001E0	  	// led3_1 && 2

#define LED_ADDR_SIZE 		8

#define LED_CON 0
#define LED_DAT 4


MODULE_LICENSE("GPL v2");


struct resource led_resource [] = {
	[0] = {
		.start =  LED_PHY_ADDR + LED_CON,
		.end   =  LED_PHY_ADDR + LED_ADDR_SIZE - 1,
		.flags =  IORESOURCE_MEM, // 指明资源类型
	},
};// 该结构体会被driver下的Bobe函数调用, 此时driver可以通过设备来获取不同设备的寄存器地址

void led_release(struct device* dev)
{
	return ;
}


struct platform_device  led_device = {
		.name	= "fs4412_led",
		.dev	= {
			.release	= led_release,
			},
		.resource = led_resource,
		.num_resources = ARRAY_SIZE(led_resource),
};

static int led_device_init(void)
{
	return platform_device_register(&led_device);
}
static void led_device_exit(void)
{
	platform_device_unregister(&led_device);
}

module_init(led_device_init);
module_exit(led_device_exit);

