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
#define LED2_ON 	0x33
#define LED2_OFF 	0x44
#define LED3_ON 	0x55
#define LED3_OFF 	0x66
#define LED4_ON 	0x77
#define LED4_OFF 	0x88

int main(void)
{
	int fd = open("/dev/led_id", O_RDWR);

	if (fd < 0) {
		perror("Fail to open");
		return fd;
	}

	while (1) {
		ioctl(fd, LED1_ON);
		sleep(1);
		ioctl(fd, LED2_ON);
		sleep(1);
		ioctl(fd, LED3_ON);
		sleep(1);
		ioctl(fd, LED4_ON);
		sleep(1);

		ioctl(fd, LED4_OFF);
		sleep(1);
		ioctl(fd, LED3_OFF);
		sleep(1);
		ioctl(fd, LED2_OFF);
		sleep(1);
		ioctl(fd, LED1_OFF);
		sleep(1);
	}

	close(fd);

	return 0;
}
