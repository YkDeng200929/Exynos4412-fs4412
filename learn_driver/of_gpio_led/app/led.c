#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "led.h"

int main(int argc, const char *argv[])
{
	int i = 0;
	int fd;
	char dev_name[] = "/dev/fs4412-led-gpio.2";

	fd = open(dev_name,O_RDWR);
	if(fd < 0){
		fprintf(stderr,"Fail to open : %s\n",dev_name);
	}


	while(1)
	{
		ioctl(fd,DEV_LED_ON);
		usleep(500 * 1000);
		ioctl(fd,DEV_LED_OFF);
		usleep(500 * 1000);
	}

	return 0;
}
