#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define MPU6050_GET_TEMP 0x1122

int main(void)
{
	int fd = open("/dev/mpu6050-dev", O_RDWR);
	int ret;
	short temp;

	if (fd < 0) {
		perror("Fail to open");
		return fd;
	}

	while (1) {
		ret = ioctl(fd,MPU6050_GET_TEMP,&temp);
		if (ret < 0){
			perror("ioctl fail\n");
			break;
		}
		temp = temp / 340 + 36;
		printf("%d c\n", temp);
		sleep(1);
	}

	close(fd);

	return 0;
}
