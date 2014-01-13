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
	//���豸�ļ�
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
	
	//����mc323
	ioctl(fd, at91_mc323_reset);
	//�ر��豸�ļ�
	close(fd);
	fprintf(stderr, "\t\t\t *** Test complete ***\n");
}