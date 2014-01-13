//#include <stdio.h>
//#include <sys/ioctl.h>
//#include <sys/time.h>
//#include <sys/types.h>
//#include <fcntl.h>
//#include <unistd.h>
//#include <stdlib.h>
//#include <errno.h>
//#include <string.h>

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

//#include "mtd/mtd-user.h"
//#include "mtd/jffs2-user.h"
#include "at91flash.h"

#define DATALEN 2048  //0x00020000 128KB，写入的时候必须是erasesize的整数倍，在这里erasesize为128KB

//static struct jffs2_unknown_node cleanmarker;
#if 0
int flash_erase_write(int fd, char *buffer, int len)
{
	/******************************************************************/
	mtd_info_t meminfo;
	int clmpos = 0, clmlen = 8;
	erase_info_t erase;
	int isNAND, bbtest = 1;
	if (ioctl(fd, MEMGETINFO, &meminfo) != 0) {
		fprintf(stderr, "unable to get MTD device info\n");
		return -1;
	}
	#if 0
	printf("++++++++++++++++++++++++++++++++++++\n");
	printf("meminfo.type 		: %d\n", meminfo.type);
	printf("meminfo.flags 		: %d\n", meminfo.flags);
	printf("meminfo.size 		: %d\n", meminfo.size);
	printf("meminfo.erasesize	: %d\n", meminfo.erasesize);
	printf("meminfo.oobblock   	: %d\n", meminfo.oobblock);
	printf("meminfo.oobsize 	: %d\n", meminfo.oobsize);
	printf("meminfo.ecctype 	: %d\n", meminfo.ecctype);
	printf("meminfo.eccsize 	: %d\n", meminfo.eccsize);
	printf("++++++++++++++++++++++++++++++++++++\n");
	#endif

	erase.length = meminfo.erasesize;
	isNAND = meminfo.type == MTD_NANDFLASH ? 1 : 0;

	for (erase.start = 0; erase.start < meminfo.size; erase.start += meminfo.erasesize) {
		if (bbtest) {
			loff_t offset = erase.start;
			int ret = ioctl(fd, MEMGETBADBLOCK, &offset);
			if (ret > 0) {
				if (1)
					printf ("\nSkipping bad block at 0x%08x\n", erase.start);
				continue;
			} 
		}

		
		#if 0
		if (1) {
			printf("\rErasing %d Kibyte @ %x -- %2llu %% complete.",
			     meminfo.erasesize / 1024, erase.start,
			     (unsigned long long) 
			     erase.start * 100 / meminfo.size);
		}
		
		fflush(stdout);
		#endif
		if (ioctl(fd, MEMERASE, &erase) != 0) {
			fprintf(stderr, "MTD Erase failure: %s\n");
			continue;
		}
		
				
		/* write cleanmarker */
/*		if (isNAND) {
			struct mtd_oob_buf oob;
			oob.ptr = (unsigned char *) &cleanmarker;
			oob.start = erase.start + clmpos;
			oob.length = clmlen;
			if (ioctl (fd, MEMWRITEOOB, &oob) != 0) {
				fprintf(stderr, "MTD writeoob failure:\n");
				continue;
			}
		}
*/
	}

	/******************************************************************/

}
#endif
int main(int argc, char *argv[])
{
	int fd = -1;
	int i;
	int offset = 0;
	int len = 0;
	int value = 0;
	unsigned char *flashdata;
	if(argc < 4)
	{
		printf( "==========how to use==========\n"
				"./flash_demo offset len value\n"
				"offset : 0 ~ 125829119(120MB)\n"
				"len    : 1 ~ 131072\n"
				"value  : 0 ~ 255\n"
				"==============================\n"
				);
		return -1;
	}
	offset = atoi(argv[1]);
	
	if(offset > 125829119)
	{
		printf("offset error!!! offset : 0 ~ 125829119(120MB)\n");
		return -1;
	}
	len = atoi(argv[2]);
	
	if(len > 131072 || len < 1)
	{
		printf("len error!!! len    : 1 ~ 131072\n");
		return -1;
	}
	value = atoi(argv[3]);
	
	if(value > 255)
	{
		printf("value error!!! value	: 0 ~ 255\n");
		return -1;
	}
	printf("============args you input============\n");
	printf("offset 	: %d\n", offset);
	printf("len 	: %d\n", len);
	printf("value 	: %d\n", value);
	printf("======================================\n");
	
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
	/*
	//赋值
	for(i = 0; i < sizeof(flashdata); i++)
	{
		flashdata[i] = i;
	}
	*/
	flash_data uarg;
	//memset(&uarg, 0x0, sizeof(flash_data));
	uarg.pbuf = flashdata;
	uarg.len = len;
	uarg.offset = offset;//mtd2(usrdata)为8M, 偏移地址为:0~(8388608 - 1)
	//retval = lseek(fd,131070,SEEK_SET);//把文件指针置为距离文件首131070个字节处
	//printf("========read before write========\n");
	//read(fd,flashdata,10);
	/*
	//read
	ioctl(fd, AT91FLASH_READ, &uarg);
	for(i = 0; i < len; i++)//这里只打印出10个数据
	{
		printf("flash data:0x%x\n",flashdata[i]);
	}
	*/
	/*
	for(i = 0; i < sizeof(flashdata); i++)
	{
		flashdata[i] = i;
	}
	*/
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
	//flash_erase_write(fd, NULL, 0);
	#if 0
	retval = lseek(fd,131071,SEEK_SET);//把文件指针置为距离文件首131070个字节处
	retval = write(fd,flashdata,10);
	retval = lseek(fd,131070,SEEK_SET);
	read(fd,flashdata,10);
	printf("\n========read after write========\n");
	for(i = 0; i < 10; i++)//这里读出10个字节的数据只是为了比较写进去与否
	{
		printf("flash data:0x%x\n",flashdata[i]);
	}
	#endif
	free(flashdata);
	close(fd);
	fprintf(stderr, "\n\n\t\t\t *** Test complete ***\n");
}

