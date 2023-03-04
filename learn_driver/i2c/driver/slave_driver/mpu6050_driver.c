#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include "mpu6050.h"

MODULE_LICENSE("GPL v2");

struct mpu6050_device{
	dev_t devno;
	struct cdev cdev;
	struct class *cls;
	struct device *dev;
	struct i2c_client *client;
};

int  mpu6050_write_register(struct mpu6050_device *mpu,unsigned char regaddr,unsigned char regval)
{
	int ret;
	struct i2c_msg msg;
	struct i2c_client *client = mpu->client;
	unsigned char tx_buf[] = {regaddr,regval};

	msg.addr  = 0x68;
	msg.flags = 0x0;
	msg.len   = sizeof(tx_buf)/sizeof(tx_buf[0]);
	msg.buf   = tx_buf;

	ret = i2c_transfer(client->adapter,&msg,1);
	if(ret < 0){
		printk("Fail to i2c transfer\n");
		return ret;
	}

	return 0;
}

unsigned char mpu6050_red_register(struct mpu6050_device *mpu,unsigned char regaddr)
{
	int ret;
	struct i2c_msg msg[2];
	unsigned char rx_buf[1];
	unsigned char tx_buf[] = {regaddr};
	struct i2c_client *client = mpu->client;

	msg[0].addr  = 0x68;
	msg[0].flags = 0x0;
	msg[0].len   = sizeof(tx_buf)/sizeof(tx_buf[0]);
	msg[0].buf   = tx_buf;

	msg[1].addr  = 0x68;
	msg[1].flags = 0x1;
	msg[1].len   = sizeof(rx_buf)/sizeof(rx_buf[0]);
	msg[1].buf   = rx_buf;
	
	ret = i2c_transfer(client->adapter,msg,sizeof(msg)/sizeof(msg[0]));
	if(ret < 0){
		printk("Fail to i2c transfer\n");
		return ret;
	}

	return rx_buf[0];
}

/* This function handles open for the character device */
static int mpu6050_chrdev_open(struct inode *inode, struct file *file)
{
	struct mpu6050_device *mpu = container_of(inode->i_cdev,struct mpu6050_device,cdev);
	file->private_data = mpu;
	mpu6050_write_register(mpu,0X6B,0);
	
	return 0;
}

static long mpu6050_chrdev_ioctl(struct file *file, unsigned int cmd,unsigned long arg)
{
	int ret;
	unsigned char temp[2];
	struct mpu6050_device *mpu = file->private_data;
	
	switch(cmd){
		case MPU6050_GET_TEMP:
			temp[0] =  mpu6050_red_register(mpu,0x42);
			if(temp[0] < 0){
				return temp[0];
			}
			
			temp[1] =  mpu6050_red_register(mpu,0x41);
			if(temp[1] < 0){
				return temp[1];
			}

			ret = copy_to_user((void __user *)arg,temp,sizeof(temp));
			if(ret){
				printk("Fail to copy to user\n");
				return -EFAULT;
			}
			break;

		default:
			printk("unKnown cmd!\n");
			break;
	}
	
	return 0;
}

static int mpu6050_chrdev_release(struct inode *inode, struct file *file)
{
	printk("led chrdev release success!\n");

	return 0;
}


/* File operations struct for character device */
static const struct file_operations mpu6050_fops = {
	.owner		= THIS_MODULE,
	.open		= mpu6050_chrdev_open,
	.unlocked_ioctl	= mpu6050_chrdev_ioctl,
	.release	= mpu6050_chrdev_release,
};


int register_mpu6050_chrdev(struct mpu6050_device *mpu)
{
	int retval;
	
	printk("led driver init\n");

	//mpu->cdev.ops = &led_fops;
	cdev_init(&mpu->cdev,&mpu6050_fops);

	//注册一个设备号
	retval = alloc_chrdev_region(&mpu->devno,0,1,"mpu6050");
	if(retval < 0){
		printk("Fail to alloc chrdev region\n");
		goto err_alloc_chrdev_region;
	}
	
	//添加字符设备
	retval = cdev_add(&mpu->cdev,mpu->devno,1);
	if(retval){
		printk("Fail to cdev add\n");
		goto err_cdev_add;
	}

	//创建类
	mpu->cls = class_create(THIS_MODULE, "mpu6050");
	if (IS_ERR(mpu->cls)){
		printk("Fail to class create\n");
		retval = PTR_ERR(mpu->cls);
		goto err_class_create;
	}

	//创建设备
	mpu->dev = device_create(mpu->cls,NULL,mpu->devno,NULL,"mpu6050_device");
	if (IS_ERR(mpu->dev)) {
		printk("Fail to device create\n");
		retval = PTR_ERR(mpu->dev);
		goto err_device_create;
	}

	return 0;
	
err_device_create:
	class_destroy(mpu->cls);
	
err_class_create:
	cdev_del(&mpu->cdev);

err_cdev_add:
	unregister_chrdev_region(mpu->devno,1);

err_alloc_chrdev_region:
	return retval;	
}


static int mpu6050_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
	int ret;
	struct mpu6050_device *mpu;
	
	printk("mpu6050 probe success\n");
	mpu = devm_kmalloc(&client->dev,sizeof(*mpu),GFP_KERNEL);
	if(!mpu){
		printk("Fail to kmalloc\n");
		return -ENOMEM;
	}
	i2c_set_clientdata(client,mpu);

	mpu->client = client;

	ret = register_mpu6050_chrdev(mpu);
	if(ret < 0){
		printk("Fail to register mpu6050 chrdev\n");
		return ret;
	}
	
	return 0;
}


static int  mpu6050_remove(struct i2c_client *client)
{
	printk("mpu6050 remove success\n");
	return 0;
}

static struct i2c_device_id  mpu6050_id[] = {
	{ "mpu6050", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, mpu6050_id);

#ifdef CONFIG_OF
static struct of_device_id dt_match[] = {
	{ .compatible = "mpu6050", },
	{},
};
MODULE_DEVICE_TABLE(of,dt_match);
#endif /* CONFIG_OF */



static struct i2c_driver mpu6050_driver = {
	.driver = {
		.name	= "mpu6050",
		.owner	= THIS_MODULE,
		.of_match_table	= of_match_ptr(dt_match),
	},
	.probe		= mpu6050_probe,
	.remove		= mpu6050_remove,
	.id_table	= mpu6050_id,
};

module_i2c_driver(mpu6050_driver);
