#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

void mpu6050_write_register(int fd,unsigned char regaddr,unsigned char regval)
{
	int ret;
	struct i2c_msg msg;
	struct i2c_rdwr_ioctl_data  ioctl_data;
	unsigned char tx_buf[] = {regaddr,regval};

	msg.addr  = 0x68;
	msg.flags = 0x0;
	msg.len   = sizeof(tx_buf)/sizeof(tx_buf[0]);
	msg.buf   = tx_buf;

	ioctl_data.msgs  = &msg;
	ioctl_data.nmsgs = 1;
	ret = ioctl(fd,I2C_RDWR,&ioctl_data);
	if(ret < 0){
		perror("Fail to ioctl");
		return ;
	}

	return;
}

unsigned char mpu6050_red_register(int fd,unsigned char regaddr)
{
	int ret;
	struct i2c_msg msg[2];
	unsigned char rx_buf[1];
	unsigned char tx_buf[] = {regaddr};
	struct i2c_rdwr_ioctl_data  ioctl_data;

	msg[0].addr  = 0x68;
	msg[0].flags = 0x0;
	msg[0].len   = sizeof(tx_buf)/sizeof(tx_buf[0]);
	msg[0].buf   = tx_buf;

	msg[1].addr  = 0x68;
	msg[1].flags = 0x1;
	msg[1].len   = sizeof(rx_buf)/sizeof(rx_buf[0]);
	msg[1].buf   = rx_buf;

	ioctl_data.msgs  = msg;
	ioctl_data.nmsgs = sizeof(msg)/sizeof(msg[0]);
	ret = ioctl(fd,I2C_RDWR,&ioctl_data);
	if(ret < 0){
		perror("Fail to ioctl");
		return ;
	}

	return rx_buf[0];
}

int main(void)
{
	int fd;
	short temp;
	unsigned char buf[2];

	fd = open("/dev/i2c-5",O_RDWR);
	if(fd < 0){
		perror("Fail to open!\n");
		return -1;
	}
		
		
	mpu6050_write_register(fd,0X6B,0);
		
	while(1){	
		buf[0] =  mpu6050_red_register(fd,0x42);
		buf[1] =  mpu6050_red_register(fd,0x41);

		temp = *((short *)buf);
		temp = temp / 340 + 36;
		printf("%d c\n",temp);
		sleep(1);
	}
	
	return 0;
}

