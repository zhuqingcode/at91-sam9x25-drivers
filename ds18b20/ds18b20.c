#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/version.h>      /* for linux version */
#include <linux/types.h>        /* size_t */
#include <linux/fs.h>           /* everything... */
#include <linux/errno.h>        /* error codes */
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/cdev.h>
#include <linux/input.h>
#include <linux/miscdevice.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <linux/slab.h> //zhuqing
//#include <linux/delay.h>
#include <asm-generic/delay.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 26)
//warning build for Linux-2.6.27 or above
#include <mach/hardware.h>
#include <mach/at91_pio.h>
#include <mach/gpio.h>

#else 
//warning build for Linux-2.6.26 or earlier
#include <asm/arch/gpio.h>
#include <asm/arch/hardware.h>
#endif
#include "ds18b20.h"
//#define DS18B20_PIN   AT91_PIN_PA4
#define DEV_NAME "ds18b20"
#define DS18B20_HIGH 1
#define DS18B20_LOW  0
unsigned char tmpdata[9];
unsigned char lastdata[2];
unsigned char supply;
int DS18B20_PIN;

#define READ_ROM 0x33
#define MATCH_ROM 0x55
#define SKIP_ROM 0xcc
#define SEARCH_ROM 0xf0
#define ALARM_SEARCH 0xec
#define CONVERT_T 0x44
#define READ_SCRATCHPAD 0xbe
#define READ_SUPPLY 0xb4

unsigned char CrcTable [256]={
0,  94, 188,  226,  97,  63,  221,  131,  194,  156,  126,  32,  163,  253,  31,  65,
157,  195,  33,  127,  252,  162,  64,  30,  95,  1,  227,  189,  62,  96,  130,  220,
35,  125,  159,  193,  66,  28,  254,  160,  225,  191,  93,  3,  128,  222,  60,  98,
190,  224,  2,  92,  223,  129,  99,  61,  124,  34,  192,  158,  29,  67,  161,  255,
70,  24,  250,  164,  39,  121,  155,  197,  132,  218,  56,  102,  229,  187,  89,  7,
219,  133, 103,  57,  186,  228,  6,  88,  25,  71,  165,  251,  120,  38,  196,  154,
101,  59, 217,  135,  4,  90,  184,  230,  167,  249,  27,  69,  198,  152,  122,  36,
248,  166, 68,  26,  153,  199,  37,  123,  58,  100,  134,  216,  91,  5,  231,  185,
140,  210, 48,  110,  237,  179,  81,  15,  78,  16,  242,  172,  47,  113,  147,  205,
17,  79,  173,  243,  112,  46,  204,  146,  211,  141,  111,  49,  178,  236,  14,  80,
175,  241, 19,  77,  206,  144,  114,  44,  109,  51,  209,  143,  12,  82,  176,  238,
50,  108,  142,  208,  83,  13,  239,  177,  240,  174,  76,  18,  145,  207,  45,  115,
202,  148, 118,  40,  171,  245,  23,  73,  8,  86,  180,  234,  105,  55,  213, 139,
87,  9,  235,  181,  54,  104,  138,  212,  149,  203,  41,  119,  244,  170,  72,  22,
233,  183,  85,  11,  136,  214,  52,  106,  43,  117,  151,  201,  74,  20,  246,  168,
116,  42,  200,  150,  21,  75,  169,  247,  182,  232,  10,  84,  215,  137,  107,  53};

static void ds18b20_write_byte (unsigned char byte)
{
	unsigned char i;
    at91_set_gpio_output(DS18B20_PIN, 0);

	for (i = 0; i < 8; i++)
	{
		at91_set_gpio_value(DS18B20_PIN, DS18B20_LOW);
		udelay(1);
		if(byte & 0x01)
		{
			at91_set_gpio_value(DS18B20_PIN, DS18B20_HIGH);
		}
		udelay(65);
		at91_set_gpio_value(DS18B20_PIN, DS18B20_HIGH);
		udelay(1);//zhuqing 2012.3.27
		byte >>= 1;
	}
}

static unsigned char ds18b20_read_byte (void)
{
	unsigned char i = 0;
	unsigned char byte = 0;
	
	for (i = 0; i < 8; i++)
	{
		byte >>= 1;
		at91_set_gpio_output(DS18B20_PIN, 0);
		///////////////////////////////
		at91_set_gpio_value(DS18B20_PIN, DS18B20_HIGH);
		udelay(2);
		///////////////////////////////
		at91_set_gpio_value(DS18B20_PIN, DS18B20_LOW);
		udelay(3);
		at91_set_gpio_value(DS18B20_PIN, DS18B20_HIGH);
		at91_set_gpio_input(DS18B20_PIN, 0); /* input */
		udelay(5);

		if (at91_get_gpio_value(DS18B20_PIN))
		{
			byte |= 0x80;
		}
		udelay(60);
		//gpio_direction_output(DS18B20_PIN, 1); /* output */
		//at91_set_gpio_value(DS18B20_PIN, DS18B20_HIGH);
	}
	return byte;
}

static int ds18b20_reset(void)
{
    at91_set_gpio_output(DS18B20_PIN, 0);
	
	at91_set_gpio_value(DS18B20_PIN, DS18B20_HIGH);
	udelay(100);

	at91_set_gpio_value(DS18B20_PIN, DS18B20_LOW);
	udelay(600);
	
	at91_set_gpio_value(DS18B20_PIN, DS18B20_HIGH);
	udelay(100);

	at91_set_gpio_input(DS18B20_PIN, 0);
		
	if(at91_get_gpio_value(DS18B20_PIN))
	{ 
		printk("ds18b20 reset failed!\n"); 
		return 1;
	}
	else
	{
		//printk("ds18b20 reset success!\n"); 
		return 0;
	}
	
}

static void ds18b20_proc(void)
{
	int i;
	//while(ds18b20_reset());
	ds18b20_reset();
	udelay(100);
	ds18b20_write_byte(READ_ROM);
	//while(ds18b20_reset());
	ds18b20_reset();
	udelay(100);
	ds18b20_write_byte(SKIP_ROM);
	ds18b20_write_byte(CONVERT_T);
	udelay(100);
	//while(ds18b20_reset());
	ds18b20_reset();
	udelay(100);
	ds18b20_write_byte(SKIP_ROM);
	ds18b20_write_byte(READ_SCRATCHPAD);
	/*
	tmpdata[1] = ds18b20_read_byte();//temp low 8 bit
	tmpdata[0] = ds18b20_read_byte();//temp high 8 bit
	*/ 
	for(i = 0;i < 9;i++)
	{
		tmpdata[i] = ds18b20_read_byte();
	}
	/*
	while(ds18b20_reset());
	udelay(100);
	ds18b20_write_byte(READ_SUPPLY);
	supply = ds18b20_read_byte();
	*/
}

static int get_port_pin(int which_port, int which_pin)
{
	int port_pin;
	switch(which_port)
	{
		case at91_gpio_port_PA:
		{
			port_pin = (PIN_BASE + 0x00 + which_pin);
			break;
		}
		
		case at91_gpio_port_PB:
		{
			port_pin = (PIN_BASE + 0x20 + which_pin);
			break;
		}
		
		case at91_gpio_port_PC:
		{
			port_pin = (PIN_BASE + 0x40 + which_pin);
			break;
		}
		
		case at91_gpio_port_PD:
		{
			port_pin = (PIN_BASE + 0x60 + which_pin);
			break;
		}
		
		default:
		{
			printk(KERN_INFO "has no such port!\n");
			return -1;
		}
	}
	return port_pin;
}

static int ds18b20_open(struct inode *inode ,struct file *file)
{
	//printk("ds18b20_open ok\n");
	return 0;
}

static int ds18b20_release(struct inode *inode ,struct file *file)
{
	//printk("ds18b20_release ok\n");
	return 0;
}

static unsigned char crc_check(int j)
{
	unsigned char  i,crc_data = 0;
    for(i=0;i < j;i++)
    {
		crc_data = CrcTable[crc_data^tmpdata[i]];
    }
    return (crc_data);
}
static ssize_t ds18b20_read(struct file *file,char __user *buffer,size_t count, loff_t *pos)
{
	//unsigned char check;
	ds18b20_proc();
	//printk(KERN_INFO"tmpMSB:0x%x tmpLSB:0x%x\n", tmpdata[1], tmpdata[0]);
	/*
	printk(KERN_INFO"supply : 0x%x\n", supply);
	check = (tmpdata[0]>>3) & 0x1f ;
	//printk(KERN_INFO"check : 0x%x", check);
	if((check != 0x1f) && (check != 0x0))
	{
		memcpy(tmpdata, lastdata, 2);
		printk(KERN_INFO"11111\n");
		goto copy;
	}
	else if((tmpdata[0] == 0x5) && (tmpdata[1] == 0x50))
	{
		memcpy(tmpdata, lastdata, 2);
		printk(KERN_INFO"22222\n");
		goto copy;
	}
	else if((tmpdata[0] == 0xff) && (tmpdata[1] == 0xff))
	{
		memcpy(tmpdata, lastdata, 2);
		printk(KERN_INFO"33333\n");
		goto copy;
	}
	*/
	if (crc_check(9) == 0)
	{
		unsigned char tmp;
		tmp = tmpdata[0];
		tmpdata[0] = tmpdata[1];
		tmpdata[1] = tmp;
		memcpy(lastdata, tmpdata, 2);
	}
	else
	{
		memcpy(tmpdata, lastdata, 2);
		//printk(KERN_INFO"CRC ERROR!\n");
	}
	/*
	else
	{
		memcpy(lastdata, tmpdata, 2);
		if (copy_to_user(buffer, tmpdata, 2) != 0) 
		{
			return -EFAULT;
			printk("read error!\n");
		}
		return 0;
	}
	*/
	//memcpy(lastdata, tmpdata, 2);
copy:
	if (copy_to_user(buffer, tmpdata, 2) != 0) 
	{
		return -EFAULT;
		printk("read error!\n");
	}
	return 0;
}

static long ds18b20_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int port_pin;
	at91_ds18b20_ctl_s at91_ds18b20_ctl_arg;
	
	switch(cmd)
	{
		case AT91_DS18B20_CTL:
		{
			copy_from_user(&at91_ds18b20_ctl_arg, (at91_ds18b20_ctl_s *)arg, sizeof(at91_ds18b20_ctl_s));
			port_pin = get_port_pin(at91_ds18b20_ctl_arg.which_port, at91_ds18b20_ctl_arg.which_pin);
			if(-1 == port_pin)
			{
				printk(KERN_INFO "get port and pin error!\n");
				return -1;
			}
			else
			{
				DS18B20_PIN = port_pin;
				//at91_set_GPIO_periph(DS18B20_PIN, 1);		
			}
			break;
		}
		default:break;	
	}
	return 0;
}

static struct file_operations ds18b20_fops =
{ 
    	.owner = THIS_MODULE, 
    	.open  = ds18b20_open,
    	.release = ds18b20_release,
    	.read = ds18b20_read,
    	.unlocked_ioctl = ds18b20_ioctl,
}; 

static struct miscdevice ds18b20_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEV_NAME,
	.fops = &ds18b20_fops,
};
   
static int __init  ds18b20_init(void)
{ 
    	int ret; 
    	ret = misc_register(&ds18b20_dev);
		
    	if (ret) 
    	{ 
        	printk("can't register major number\n"); 
        	return ret; 
    	}
		printk("ds18b20 driver v1.0\n");
    	return 0; 
} 

static void __exit ds18b20_exit(void) 
{ 
	    misc_deregister(&ds18b20_dev);
}
module_init(ds18b20_init);
module_exit(ds18b20_exit);
/* input param with insmod. */

//MODULE_INFO(build, UTS_VERSION);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhuqing@talentinfo.com.cn");
