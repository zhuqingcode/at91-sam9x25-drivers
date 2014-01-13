#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

int main()
{
	int fd = -1;
	int i;
	unsigned short adcdatabuf[11];
	//打开设备文件
	fd = open("/dev/at91adc", O_RDWR);
	if (-1 == fd)  		
	{  			
		perror("can't open /dev/at91adc\n");  			
		return -1;  		
	}  		
	else  		
	{  			
		printf("open ok!\n");  		
	}
	//读出AD值
	read(fd,adcdatabuf,sizeof(adcdatabuf));
	printf("----------------------------------\n");
	printf("adc channel:0 <----------------> AD_VIN6\n");
	printf("adc channel:1 <----------------> AD_VIN7\n");
	printf("adc channel:2 <----------------> AD_VIN8\n");
	printf("adc channel:3 <----------------> AD_V01\n");
	printf("adc channel:4 <----------------> AD_V02\n");
	printf("adc channel:5 <----------------> AD_Iout\n");
	printf("adc channel:6 <----------------> AD_VIN1\n");
	printf("adc channel:7 <----------------> AD_VIN2\n");
	printf("adc channel:8 <----------------> AD_VIN3\n");
	printf("adc channel:9 <----------------> AD_VIN4\n");
	printf("adc channel:10 <----------------> AD_VIN5\n");
	printf("----------------------------------\n");
	for(i = 0; i < 11; i++)
	{
		printf("adc channel:%d data:%f V\n",i,(float)adcdatabuf[i]/1024.0*3.0);
	}
	close(fd);
	fprintf(stderr, "\n\n\t\t\t *** Test complete ***\n");
}

