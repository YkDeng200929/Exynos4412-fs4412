#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>


void mpu6050_write_register(int fd,unsigned char regaddr,unsigned char regval)
{
	int ret;
	struct i2c_msg message;
	struct i2c_rdwr_ioctl_data data;
	unsigned int text_buf[] = {regaddr, regval};
	message.addr  = 0x68;
	message.flags = 0x0;
	message.len   = sizeof(text_buf)/sizeof(text_buf[0]);
	message.buf   = text_buf;
	data.msgs	  = &message;
	data.nmsgs	  = 1;
	ret = ioctl(fd, I2C_RDWR , &data);
	if (!ret){
		printf("ioctl error\n");
		return ret;
	}
	return ;
}

unsigned char mpu6050_red_register(int fd, unsigned char regaddr)
{	
	int ret;
	struct i2c_msg message[2];
	unsigned char read_buff[1];
	unsigned char text_buff[] = {regaddr};
	struct i2c_rdwr_ioctl_data data;
	
	message[0].addr = 0x68;
	message[0].len  = sizeof(text_buff)/sizeof(text_buff[0]);
	message[0].flags= 0x0;
	message[0].buf	= text_buff;

	message[1].addr = 0x68;
	message[1].len  = sizeof(read_buff)/sizeof(read_buff[0]);
	message[1].flags= 0x1;
	message[1].buf	= read_buff;

	data.msgs  = message;
	data.nmsgs = sizeof(message)/sizeof(message[0]);
		
	ret = ioctl(fd, I2C_RDWR , &data);
	if (!ret){
		printf("ioctl error\n");
		return ret;
	}
	return read_buff[0];
}

int main()
{
	unsigned char buf[2];
	short temp;
	
	int fd = open("/dev/i2c-5", O_RDWR);
	if (fd < 0){
		perror("Fail to open\n");
		return -1;
	}

	while (1){
		buf[0] = mpu6050_red_register(fd, 0x42);
		buf[1] = mpu6050_red_register(fd, 0x41);

		temp = *((short *)buf);
		temp = temp / 340 + 36;
		printf("%d c\n",temp);
		sleep(1);
	}

	return 0;
}
