#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include "head.h"

int main(int argc, const char *argv[])
{
	int fd;
	int n;
	int adc_data;
	
	fd = open("/dev/exynos4412-adc",O_RDWR | O_NONBLOCK);
	if(fd < 0){
		perror("Fail to open");
		exit(EXIT_FAILURE);
	}
	
	//初始化
	ioctl(fd,ADC_DEVICE_INIT);

	while(1)
	{
		//使能ADC转换
		ioctl(fd,ADC_DEVICE_START);
		//读取ADC的值
		n = read(fd,&adc_data,sizeof(adc_data));
		//printf("n = %d\n",n);
		if(n < 0){
			continue;
		}
		
		printf("adc_data : %d\n",adc_data);
		printf("%fv\n",1.8 * ((float)adc_data/ 0xfff));
		sleep(1);
	}

	close(fd);
	return 0;
}
