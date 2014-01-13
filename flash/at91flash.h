#ifndef FLASH_H
#define FLASH_H
typedef struct _flash_data
{
	int offset;
	int len;
	char *pbuf;
}flash_data;

#define AT91FLASH            't'
#define AT91FLASH_READ		    _IOWR(AT91FLASH, 1, flash_data) 
#define AT91FLASH_WRITE		    _IOWR(AT91FLASH, 2, flash_data)
#define AT91FLASH_LSEEK		    _IOWR(AT91FLASH, 3, flash_data)
#endif