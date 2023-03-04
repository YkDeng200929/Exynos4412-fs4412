#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h> // open firmware
#include <linux/interrupt.h>

struct tasklet_struct key_tasklet;

// 下半部
static void key_interrupt_handle(unsigned long priv)
{
	printk("Ringing!!!\n");
	return ;
}


// 上半部
static irqreturn_t key_interrupt(int irq, void *dev_id)
{
	printk("You press a key\n");
	tasklet_schedule(&key_tasklet);
	return IRQ_HANDLED;
}

static int key_temp_probe(struct platform_device *pdev)
{
	int err;
	struct resource* res;
	
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	printk("IRQ: %d\n", (int)res->start);

	tasklet_init(&key_tasklet, key_interrupt_handle, 0);
	
	err = devm_request_irq(&pdev->dev, 
							res->start, 
							key_interrupt, 
							res->flags & IRQF_TRIGGER_MASK, 
							"key-interrupt", 
							NULL);
	if (err) {
		printk("Fail to IRQ request irq %d\n", res->start);
		return err;
	}
	return 0;
}

static int key_temp_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id key_temp_match[] = {
	{ .compatible = "fs4412-key" },
	{},
};

static struct platform_driver key_temp_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "key-temp",
		.of_match_table = of_match_ptr(key_temp_match),
	},
	.probe = key_temp_probe,
	.remove = key_temp_remove,
};

module_platform_driver(key_temp_driver);

MODULE_AUTHOR("Martin Persson <martin.persson@stericsson.com>");
MODULE_DESCRIPTION("ABX500 temperature driver");
MODULE_LICENSE("GPL");

