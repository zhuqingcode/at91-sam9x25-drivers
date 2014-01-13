#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "at91gpio.h"
int main()
{
	int fd = -1;
	int value;
	at91_gpio_ctl_s at91_gpio_ctl_arg;
	//打开设备文件
	fd = open("/dev/at91gpio", O_RDWR);
	if (-1 == fd)  		
	{  			
		perror("can't open /dev/at91gpio\n");  			
		return -1;  		
	}  		
	else  		
	{  			
		//printf("open ok!\n");  	
		
	}
	at91_gpio_ctl_arg.which_port = at91_gpio_port_PA;
	at91_gpio_ctl_arg.which_pin = at91_gpio_pin_15;
	at91_gpio_ctl_arg.inout = at91_gpio_out;
	at91_gpio_ctl_arg.value = 1;
	//把PA0  配置成输出
	ioctl(fd, AT91GPIO_CTL, &at91_gpio_ctl_arg);
	//把PA0写为0
	ioctl(fd, AT91GPIO_WRITE, &at91_gpio_ctl_arg);
	//把PA0  配置成输入
	at91_gpio_ctl_arg.inout = at91_gpio_in;
	if(ioctl(fd, AT91GPIO_CTL, &at91_gpio_ctl_arg))
	{
		perror("ioctl()");
	}
	//读入PA0的值
	if(ioctl(fd, AT91GPIO_READ, &at91_gpio_ctl_arg))
	{
		perror("ioctl()");
	}
	//得到PA0  的输入值
	value = at91_gpio_ctl_arg.value;
	printf("gpio value @ demo: %d\n",value);
	//关闭设备文件
	close(fd);
	fprintf(stderr, "\t\t\t *** Test complete ***\n");
}
