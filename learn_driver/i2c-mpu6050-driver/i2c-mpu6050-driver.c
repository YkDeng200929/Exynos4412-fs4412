#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/kobject.h>
#include <linux/irq.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#define MPU6050_MAJOR 226
#define MPU6050_GET_TEMP 0x1122

static struct class *mpu6050_class;
static struct i2c_client *mpu6050_client = NULL;

int mpu6050_write_register(unsigned char regaddr,unsigned char regval)
{
	int ret;
	struct i2c_msg msg;
	struct i2c_adapter *adapter = mpu6050_client->adapter;
	unsigned char tx_buf[] = {regaddr,regval};

	msg.addr  = mpu6050_client->addr;
	msg.flags = 0x0;
	msg.len   = sizeof(tx_buf)/sizeof(tx_buf[0]);
	msg.buf   = tx_buf;

	ret = i2c_transfer(adapter, &msg, 1);
	if(ret < 0){
		printk("i2c_transfer\n");
		return ret;
	}

	return 0;
}

int mpu6050_read_register(unsigned char regaddr, uint8_t *regval)
{
	int ret;
	struct i2c_msg msg[2];
	unsigned char rx_buf[1];
	unsigned char tx_buf[] = {regaddr};
	struct i2c_adapter *adapter = mpu6050_client->adapter;


	msg[0].addr  = mpu6050_client->addr;
	msg[0].flags = 0x0;
	msg[0].len   = sizeof(tx_buf)/sizeof(tx_buf[0]);
	msg[0].buf   = tx_buf;

	msg[1].addr  = mpu6050_client->addr;
	msg[1].flags = 0x1;
	msg[1].len   = sizeof(rx_buf)/sizeof(rx_buf[0]);
	msg[1].buf   = rx_buf;

	ret = i2c_transfer(adapter, msg, 2);
	if(ret < 0){
		printk("Fail to i2c_transfer\n");
		return ret;
	}
	printk("kernel: %d\n", rx_buf[0]);
	*regval = rx_buf[0];

	return 0;
}

static long mpu6050_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	uint8_t buf[2];
	
	switch (cmd)
	{
		case MPU6050_GET_TEMP:
			ret = mpu6050_read_register(0x42, &buf[0]);
			if (ret < 0){
				printk("Fail to mpu6050_read_register 0x42\n");
				break;
			}
			ret = mpu6050_read_register(0x41, &buf[1]);
			if (ret < 0){
				printk("Fail to mpu6050_read_register 0x41\n");
				break;
			}
			ret = copy_to_user((void __user *)arg, buf, sizeof(buf));
			break;
	}
	
	
	return ret;
}

static int mpu6050_open(struct inode *inode, struct file *file)
{

	return 0;
}

static int mpu6050_close(struct inode *inode, struct file *filp)
{
	return 0;
}

static const struct file_operations mpu6050_fops = {
	.owner		 = THIS_MODULE,		/* owner */
	.unlocked_ioctl	 = mpu6050_ioctl,	/* ioctl */
	.open		 = mpu6050_open,		/* open */
	.release	 = mpu6050_close,	/* release */
};

static int mpu6050_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;

	struct device *mpu6050_dev;
	struct i2c_adapter *adapter = client->adapter;
	
	printk("address: %#x\n", client->addr);
	ret = __register_chrdev(MPU6050_MAJOR, 0, 1, "mpu6050", &mpu6050_fops);
	
	if (ret < 0) {
		printk("Fail to __register_chrdev\n");
		return ret;
	}

	mpu6050_class = class_create(THIS_MODULE, "mpu6050-dev");
	
	if (IS_ERR(mpu6050_class)) {
		printk("Fail to class_create\n");
		goto error_class_create;
	}

	mpu6050_dev = device_create(mpu6050_class, NULL,
				     MKDEV(MPU6050_MAJOR, 0), NULL,
				     "mpu6050-dev");
	printk("adapter->nr: %d\n", adapter->nr);
	
	if (IS_ERR(mpu6050_dev)) {
		printk("Fail to device_create\n");
		goto error_device_create;
	}
	mpu6050_client = client;

	return 0;

error_device_create:
	class_destroy(mpu6050_class);

error_class_create:
	__unregister_chrdev(MPU6050_MAJOR, 0, 1, "mpu6050");
	return ret;
}

static int mpu6050_remove(struct i2c_client *client)
{
	device_destroy(mpu6050_class, MKDEV(MPU6050_MAJOR, 0));
	class_destroy(mpu6050_class);
	__unregister_chrdev(MPU6050_MAJOR, 0, 1, "mpu6050");
	return 0;
}

static const struct i2c_device_id mpu6050_id[] = {
	{ "mpu6050", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, mpu6050_id);

static const struct of_device_id mpu6050_match_table[] = {
	{ .compatible = "fs4412-mpu6050", },
	{ },
};

static struct i2c_driver mpu6050_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "mpu6050",
		.of_match_table = mpu6050_match_table,
	},
	.probe		= mpu6050_probe,
	.remove		= mpu6050_remove,
	.id_table	= mpu6050_id,
};

module_i2c_driver(mpu6050_driver);

MODULE_LICENSE("GPL v2");


