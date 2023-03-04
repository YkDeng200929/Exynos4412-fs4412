#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define LED1_ON 	0X11
#define LED1_OFF  	0x22

int main(void)
{
	int fd = open("/dev/led_device", O_RDWR);

	if (fd < 0) {
		perror("Fail to open");
		return fd;
	}

	while (1) {
		ioctl(fd, LED1_ON);
		sleep(1);
		ioctl(fd, LED1_OFF);
		sleep(1);
	}

	close(fd);

	return 0;
}
