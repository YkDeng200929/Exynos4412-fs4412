#ifndef _LED_CHAR_HEAD_H
#define _LED_CHAR_HEAD_H

#include <linux/fs.h>
#include <linux/cdev.h>

struct led_device {
	uint32_t reg_phy_addr;
	uint32_t reg_phy_size;
	uint32_t pin;
	uint32_t mode;
	uint32_t level;
	uint32_t bits;
	dev_t dev_id;
	void __iomem *led1_reg;
	struct cdev led_cdev;	
	struct device *dev;

};

extern int led_char_driver_register(struct led_device **ppled);
extern void led_char_driver_unregister(struct led_device *pled);


#endif
