#include <stdio.h>
#include <string.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#define DEV_NAME "/dev/rtc0"
#define READRTCTIME 1
#define SETRTCTIME 1

int main(void)
{
	int fd = -1;
	int retval;
    struct rtc_time rtc_tm;
    memset(&rtc_tm,0x0,sizeof(rtc_tm));
    fd = open(DEV_NAME, O_RDWR);
    if (-1 == fd)
    {	
    	perror("can't open /dev/rtc0\n");  			
    	return -1;
    }
    else  		
    {		
    	printf("open ok!\n");  		
    }
	fprintf(stderr, "\n\n\t\t\t *** Test start ***\n");
	#if READRTCTIME > 0
    retval = ioctl(fd, RTC_RD_TIME, &rtc_tm);
    if (retval == -1) 
    {		
		perror("RTC_RD_TIME ioctl");		
		exit(errno);	
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
		exit(errno);	
    }
	sleep(5);
	retval = ioctl(fd, RTC_RD_TIME, &rtc_tm);
    if (retval == -1) 
    {		
		perror("RTC_RD_TIME ioctl");		
		exit(errno);	
    }
    printf("%s(): %4d-%02d-%02d %02d:%02d:%02d weekday:%02d\n", __func__,
		1900 + rtc_tm.tm_year, rtc_tm.tm_mon + 1, rtc_tm.tm_mday,
		rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec,rtc_tm.tm_wday);
    #endif
    close(fd);
	fprintf(stderr, "\n\n\t\t\t *** Test complete ***\n");
 }