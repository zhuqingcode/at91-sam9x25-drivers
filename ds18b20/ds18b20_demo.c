#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

#include "ds18b20.h"
#define DEV_NAME "/dev/ds18b20"

static unsigned char TempX_TAB[16]={0x00,0x01,0x01,0x02,0x03,0x03,0x04,0x04,0x05,0x06,0x06,0x07,0x08,0x08,0x09,0x09};
int temp_calculate(unsigned char temp_high,unsigned char temp_low)
{
	unsigned int Temperature;
	
	if(temp_high & 0x80)
	{
		temp_high=(~temp_high) | 0x08;
		temp_low=~temp_low+1;
		if(temp_low==0)
			temp_high+=1;
	}
	temp_high=(temp_high<<4)+((temp_low&0xf0)>>4);
	temp_low=TempX_TAB[temp_low&0x0f];
	//bit0-bit7为小数位，bit8-bit14为整数位，bit15为正负位
	Temperature=temp_high;
	Temperature=(Temperature<<8) | temp_low;  
	return Temperature;
}
int main(void)
{
	int fd = -1;
	int i;
	int value;
	unsigned char buffer[2];
	at91_ds18b20_ctl_s ds18b20_args;
	ds18b20_args.which_port = at91_gpio_port_PA;
	ds18b20_args.which_pin = at91_gpio_pin_4;
	memset(buffer,0x0,2);
	fd = open(DEV_NAME, O_RDWR);
	if (-1 == fd)  		
	{  			
		perror("can't open /dev/ds18b20\n");  			
		return -1;  		
	}  		
	ioctl(fd,at91_ds18b20_ctl,&ds18b20_args);//选择一个GPIO作为DQ
	for(i = 0; i < 10; i++)
	{
		read(fd,buffer,2);
		printf("buffer[0] : 0x%x      buffer[1] : 0x%x\n",buffer[0],buffer[1]);
		printf("sleep 1s...\n");
		sleep(1);
	}
	close(fd);
}
