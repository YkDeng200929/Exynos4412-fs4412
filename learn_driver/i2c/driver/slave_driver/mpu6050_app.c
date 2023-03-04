#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "mpu6050.h"

int main(int argc, const char *argv[])
{
	int fd;
	int ret;
	short temp;

	fd = open("/dev/mpu6050_device",O_RDWR);
	if(fd < 0){
		perror("Fail to open");
		return -1;
	}

	while(1){		
		ret = ioctl(fd,MPU6050_GET_TEMP,(unsigned long)&temp);
		if(ret < 0){
			perror("Fail to ioctl");
			return -1;
		}

		temp = temp / 340 + 36;
		printf("%d\n",temp);
		sleep(1);
	}
	
	return 0;
}
