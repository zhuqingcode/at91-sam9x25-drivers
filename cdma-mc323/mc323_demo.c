#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "at91mc323.h"
int main()
{
	int fd = -1;
	int value;
	//打开设备文件
	fd = open("/dev/at91mc323", O_RDWR);
	if (-1 == fd)  		
	{  			
		perror("can't open /dev/at91mc323\n");  			
		return -1;  		
	}  		
	else  		
	{  			
		//printf("open ok!\n");  	
		
	}
	
	//重启mc323
	ioctl(fd, at91_mc323_reset);
	//关闭设备文件
	close(fd);
	fprintf(stderr, "\t\t\t *** Test complete ***\n");
}