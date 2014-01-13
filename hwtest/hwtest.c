#include <sys/types.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <libgen.h>
#include <ctype.h>
#include <time.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>      /*标准输入输出定义*/
#include <stdlib.h>     /*标准函数库定义*/
#include <unistd.h>     /*Unix标准函数定义*/
#include <sys/types.h>  /**/
#include <sys/stat.h>   /**/
#include <fcntl.h>      /*文件控制定义*/
#include <termios.h>    /*PPSIX终端控制定义*/
#include <errno.h>

#include <linux/rtc.h>

#include "ds18b20.h"
#include "at91gpio.h"
#include "at91flash.h"

/*********************************/
static int DS18B20_test(void)
{
	int fd = -1;
	int i;
	int value;
	unsigned char buffer[2];
	at91_ds18b20_ctl_s ds18b20_args;
	ds18b20_args.which_port = at91_ds18b20_port_PA;
	ds18b20_args.which_pin = at91_ds18b20_pin_4;
	memset(buffer,0x0,2);
	fd = open("/dev/ds18b20", O_RDWR);
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
/*********************************/
#define READRTCTIME 0
#define SETRTCTIME 1
static int RTC_test(void)
{
	int fd = -1;
	int i;
	int retval;
    struct rtc_time rtc_tm;
    memset(&rtc_tm,0x0,sizeof(rtc_tm));
    fd = open("/dev/rtc0", O_RDWR);
    if (-1 == fd)
    {	
    	perror("can't open /dev/rtc0\n");  			
    	return -1;
    }
    else  		
    {		
    	printf("open ok!\n");  		
    }
	#if READRTCTIME > 0
    retval = ioctl(fd, RTC_RD_TIME, &rtc_tm);
    if (retval == -1) 
    {		
		perror("RTC_RD_TIME ioctl");		
		return -1;	
    }
    printf("%s(): %4d-%02d-%02d %02d:%02d:%02d weekday:%02d\n", __func__,
		1900 + rtc_tm.tm_year, rtc_tm.tm_mon + 1, rtc_tm.tm_mday,
		rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec,rtc_tm.tm_wday);
	#endif

	#if SETRTCTIME > 0
    rtc_tm.tm_year = 2012 - 1900;
    rtc_tm.tm_mon = 3 - 1;
    rtc_tm.tm_mday = 12;
    rtc_tm.tm_hour = 10;
    rtc_tm.tm_min = 10;
    rtc_tm.tm_sec = 10;
	rtc_tm.tm_wday = 2;
	rtc_tm.tm_isdst = 0;
    retval = ioctl(fd, RTC_SET_TIME, &rtc_tm);
    if (retval == -1) 
    {		
		perror("RTC_SET_TIME ioctl");		
		return -1;
    }
	for(i = 0;i < 10; i++)
	{
		sleep(5);
		retval = ioctl(fd, RTC_RD_TIME, &rtc_tm);
	    if (retval == -1) 
	    {		
			perror("RTC_RD_TIME ioctl");		
			return -1;
	    }
	    printf("%s(): %4d-%02d-%02d %02d:%02d:%02d weekday:%02d\n", __func__,
			1900 + rtc_tm.tm_year, rtc_tm.tm_mon + 1, rtc_tm.tm_mday,
			rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec,rtc_tm.tm_wday);
	}
	
    #endif
    close(fd);
}
/*********************************/
static int ADC_test(void)
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
	printf("-------------------------------------------\n");
	printf("adc channel:0 	<----------------> 	AD_VIN6\n");
	printf("adc channel:1 	<----------------> 	AD_VIN7\n");
	printf("adc channel:2 	<----------------> 	AD_VIN8\n");
	printf("adc channel:3 	<----------------> 	AD_V01\n");
	printf("adc channel:4 	<----------------> 	AD_V02\n");
	printf("adc channel:5 	<----------------> 	AD_Iout\n");
	printf("adc channel:6 	<----------------> 	AD_VIN1\n");
	printf("adc channel:7 	<----------------> 	AD_VIN2\n");
	printf("adc channel:8 	<----------------> 	AD_VIN3\n");
	printf("adc channel:9 	<----------------> 	AD_VIN4\n");
	printf("adc channel:10 	<----------------> 	AD_VIN5\n");
	printf("-------------------------------------------\n");
	for(i = 0; i < 11; i++)
	{
		printf("adc channel:%d data:%f V\n",i,(float)adcdatabuf[i]/1024.0*3.0);
	}
	close(fd);
}
/*********************************/
static int NAND_FLASH_test(void)
{
	int fd = -1;
	int i;
	int offset = 0;
	int len = 0;
	int value = 0;
	char *flashdata;
	
	offset = 0;
	len = 100;
	value = 0;
	flashdata = (char *)malloc(len * sizeof(char));
	if(flashdata == NULL)
	{
		printf("malloc error!!!\n");
		return -1;
	}
	memset(flashdata, value, len);
	printf("======date write to flash======\n");
	for(i = 0; i < len; i++)//这里只打印出10个数据
	{
		printf("flash data[%d]:0x%x\n", i, flashdata[i]);
	}
	//打开设备文件
	fd = open("/dev/at91flash", O_RDWR);//O_RDONLY,O_RDWR
	if (-1 == fd)  		
	{  			
		perror("can't open /dev/at91flash\n");  			
		return -1;  		
	}  		
	else  		
	{  			
		//printf("open ok!\n");  		
	}
	flash_data uarg;
	uarg.pbuf = flashdata;
	uarg.len = len;
	uarg.offset = offset;//mtd2(usrdata)为8M, 偏移地址为:0~(8388608 - 1)
	//write
	ioctl(fd, AT91FLASH_WRITE, &uarg);
	//read
	printf("\n========read after write========\n");
	memset(flashdata, 0xff, len);
	ioctl(fd, AT91FLASH_READ, &uarg);
	for(i = 0; i < len; i++)//这里只打印出10个数据
	{
		printf("flash data[%d]:0x%x\n", i, flashdata[i]);
	}
	free(flashdata);
	close(fd);
}
/*********************************/
#define tty "/dev/ttyS1"

int tty_init(int fd, int nSpeed, int nBits, char nEvent,int nStop)
{
	struct termios newtio,oldtio;
	if(tcgetattr(fd,&oldtio)!=0)//保存以前端口设置
	{
		//perror("set Serial 1");
		return -1;
	}
  	bzero(&newtio,sizeof(newtio));
	newtio.c_cflag |= CLOCAL|CREAD;
	newtio.c_cflag &= ~CSIZE;
	newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); /*Input*/
  	newtio.c_oflag &= ~OPOST; /*Output*/

	switch(nBits)
	{
		case 7:
				newtio.c_cflag|=CS7;//7个数据位
				break;
 		case 8:
				newtio.c_cflag|=CS8;//8个数据位
				break;
		default:
				break;
	}

  	switch(nEvent)//设置奇偶检验位
	{
		case 'O':
				newtio.c_cflag |= PARENB;
				newtio.c_cflag |= PARODD;
				newtio.c_iflag |= (INPCK|ISTRIP);
				break;
		case 'E':
				newtio.c_iflag |= (INPCK|ISTRIP);
				newtio.c_cflag |= PARENB;
				newtio.c_cflag &= ~PARODD;
				break;
		case 'N':
				newtio.c_cflag &= ~PARENB;
				break;
		default:
				break;
	}

  	switch(nSpeed)//设置波特率
  	{
  		case 2400:
				cfsetispeed(&newtio,B2400);
				cfsetospeed(&newtio,B2400);
				break;
		case 4800:
				cfsetispeed(&newtio,B4800);
				cfsetospeed(&newtio,B4800);
				break;
		case 9600:
				cfsetispeed(&newtio,B9600);
  				cfsetospeed(&newtio,B9600);
				break;
		case 115200:
				cfsetispeed(&newtio,B115200);
				cfsetospeed(&newtio,B115200);
				break;
		case 460800:
				cfsetispeed(&newtio,B460800);
				cfsetospeed(&newtio,B460800);
	  			break;
		  	default:
		  		cfsetispeed(&newtio,B9600);
		  		cfsetospeed(&newtio,B9600);
		  		break;   
	 }
	 if(nStop==1)//设置停止位
	 {
	 	newtio.c_cflag&=~CSTOPB;
	 }
	 else if(nStop==2)
	 {
	 	newtio.c_cflag |= CSTOPB;
	 }

	 newtio.c_cc[VTIME] = 1;//设置读取每个字符的等待时间
	 newtio.c_cc[VMIN]  = 0;//指定读取字符的最少数目
	 tcflush(fd,TCIFLUSH);
	 
	 if((tcsetattr(fd,TCSANOW,&newtio))!=0)//激活以上配置
	 {
	 	//perror("com set error");
	  	return -1;
	 }
	 return 0;

}

int tty_write(int fd,char * pwritebuf,int datalen)
{
	int nwrite = 0;
	if(fd <= 0 || pwritebuf == NULL)
	{
		printf("uart ctl  failed:%d\n",fd);
		return 1;
	}
	nwrite = write(fd,pwritebuf,datalen);
	return nwrite;
}

int tty_read(int fd,char * preadbuf,int datalen)
{
	int nread = 0;
#if 1	
	fd_set rfds;
	int  nselect = 0;
	struct	timeval  tmout;
	tmout.tv_sec = 6;   
	tmout.tv_usec = 0;   
	if(fd <= 0 || preadbuf == NULL)
	{
		printf("uart ctl  failed:%d\n",fd);
		return 1;
	}
	FD_ZERO(&rfds);
	FD_SET(fd,&rfds);
	nselect = select(fd+1,&rfds,NULL,NULL,&tmout);
	if(nselect < 0)
	{
		printf("uart Read date select failed,errno:0x%x\n",errno);
		return errno;
	}
	else if(nselect == 0)
	{
		//printf("select() timeout!\n");
	   	return -1;
	}
	else
	{
		if(FD_ISSET(fd,&rfds))
		{
		  	nread = read(fd, preadbuf, datalen);
			if( nread < 0)
			{
				printf("uart read error \n");
				return -1;
			}
			else
			{
				return nread;
			}
		}
	}
#else
	nread = read(fd, preadbuf, datalen);
	if( nread < 0)
	{
		printf("uart read error \n");
		return -1;
	}
	else
	{
		printf(preadbuf);
		printf("\n");
		return nread;
	}	
#endif
	
}

int tty_open(char *dev)
{
	int fd = -1;
	fd = open(dev, O_RDWR);  //|O_NONBLOCK		
	if (-1 == fd)  		
	{  			
		//perror("Can't open tty port");
		return -1;
	}  		
	else  		
	{  			
		return fd;
	}
}
int tty_close(int fd)
{
	if(fd)
	{
		close(fd);
		fd = 0;
	}
	return 0;
}
void tty_usleep(unsigned int usleeptime)
{
	usleep(usleeptime);
}


static int CDMA_test(void)
{
	int fd = -1;
	int nread = 0;
	int nwrite = 0;
	char buff[512];
	char AT[]="AT\r";
	fd = tty_open(tty);
	if(-1 == fd)
	{
		return -1;
	}
	
	if(0 == tty_init(fd, 115200, 8, 'N', 1))
	{
		
	}
	else
	{
		tty_close(fd);
		return -1;
	}
	nwrite = tty_write(fd, AT, strlen(AT));
	if(nwrite)
	{
		printf("=========writedata==========\n");
		printf("%s\n",AT);
		printf("============================\n");
		usleep(100*1000);
	}

	memset(buff, 0x0, sizeof(buff));

	nread = tty_read(fd, buff, 512);
	if (nread > 0)
	{
		buff[nread] = '\0';
		printf("==========readdata==========\n");
		printf("%s\n",buff);
		printf("============================\n");
	}
	tty_close(fd);
}
/*********************************/
static int GPIO_test(void)
{
	int fd = -1;
	int i;
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
	at91_gpio_ctl_arg.which_port = at91_gpio_port_PB;
	at91_gpio_ctl_arg.which_pin = at91_gpio_pin_0;
	at91_gpio_ctl_arg.inout = at91_gpio_out;
	at91_gpio_ctl_arg.value = 1;
	//把PA0  配置成输出
	ioctl(fd, AT91GPIO_CTL, &at91_gpio_ctl_arg);
	printf("testing...please wait ...\n");
	for(i = 0;i < 20; i++)
	{
		//把PA0写为0
		ioctl(fd, AT91GPIO_WRITE, &at91_gpio_ctl_arg);
		if(at91_gpio_ctl_arg.value == 1)
		{
			at91_gpio_ctl_arg.value = 0;
		}
		else if(at91_gpio_ctl_arg.value == 0)
		{
			at91_gpio_ctl_arg.value = 1;
		}
		sleep(1);
	}
	
	//关闭设备文件
	close(fd);
}
/*********************************/
int main(int argc, char *argv[])
{
	char ch;	
	char ping[1024] =   "\n=====================================\n"
						"1  --> <DS18B20> 		test\n"
						"2  --> <RTC> 			test\n"
						"3  --> <ADC> 			test\n"
						"4  --> <NAND FLASH> 		test\n"
						"5  --> <CDMA> 			test\n"
						"6  --> <GPIO> 			test\n"
						"q  --> <quit>              \n"
						"please enter order:\n";
	printf("%s",ping);	
	while((ch = getchar())!= 'q')
	{				
		if('\n' == ch)
		{			
			continue;		
		}				
		switch(ch)		
		{			
			case '1':
			{				
				DS18B20_test();				
				break;			
			}						
			case '2':			
			{				
				RTC_test();	
				break;			
			}						
			case '3':		
			{				
				ADC_test();				
				break;			
			}						
			case '4':	
			{				
				NAND_FLASH_test(); 				
				break;			
			}			
			case '5':			
			{				
				CDMA_test(); 				
				break;			
			}	
			case '6':			
			{				
				GPIO_test(); 				
				break;			
			}	
			default:				
				printf("no order@@@@@!\n");
		}				
		printf("%s",ping);	
	}	

		return 1;
}
