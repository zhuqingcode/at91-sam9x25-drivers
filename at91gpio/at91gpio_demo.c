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
	//���豸�ļ�
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
	//��PA0  ���ó����
	ioctl(fd, AT91GPIO_CTL, &at91_gpio_ctl_arg);
	//��PA0дΪ0
	ioctl(fd, AT91GPIO_WRITE, &at91_gpio_ctl_arg);
	//��PA0  ���ó�����
	at91_gpio_ctl_arg.inout = at91_gpio_in;
	if(ioctl(fd, AT91GPIO_CTL, &at91_gpio_ctl_arg))
	{
		perror("ioctl()");
	}
	//����PA0��ֵ
	if(ioctl(fd, AT91GPIO_READ, &at91_gpio_ctl_arg))
	{
		perror("ioctl()");
	}
	//�õ�PA0  ������ֵ
	value = at91_gpio_ctl_arg.value;
	printf("gpio value @ demo: %d\n",value);
	//�ر��豸�ļ�
	close(fd);
	fprintf(stderr, "\t\t\t *** Test complete ***\n");
}
