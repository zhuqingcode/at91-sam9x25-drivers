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
#include <linux/delay.h>
#include <linux/slab.h> //zhuqing
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
#include "at91mc323.h"
#define MC323_RST       		AT91_PIN_PA2
#define MC323_POWERON     		AT91_PIN_PA3
#define MC323_WAKEUP     		AT91_PIN_PA8
#define MC323_RING     			AT91_PIN_PA7

#define HIGH 0      //这里为0是由于硬件上做了三极管的装换
#define LOW 1

static int at91mc323_ctl_gpio_init(void)
{
	at91_set_gpio_output(MC323_RST, 0);
	at91_set_gpio_output(MC323_POWERON, 0);
	at91_set_gpio_output(MC323_WAKEUP, 0);
	at91_set_gpio_output(MC323_RING, 0);
	return 0;
}

static int at91mc323_reset(void)
{
	at91_set_gpio_value(MC323_RST, HIGH);
	at91_set_gpio_value(MC323_RST, LOW);
	mdelay(20);
	at91_set_gpio_value(MC323_RST, HIGH);
	return 0;
}

static int at91mc323_poweron(void)
{
	at91_set_gpio_value(MC323_POWERON, HIGH);
	at91_set_gpio_value(MC323_POWERON, LOW);
	mdelay(200);
	at91_set_gpio_value(MC323_POWERON, HIGH);
	return 0;
}

static int at91mc323_poweroff(void)
{
	return 0;
}

static int at91mc323_open(struct inode *inode ,struct file *file)
{
	return 0;
}

static int at91mc323_release(struct inode *inode ,struct file *file)
{
	return 0;
}

static long at91mc323_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	
	switch(cmd)
	{
		
		case at91_mc323_poweron:
		{
			at91mc323_poweron();
			break;
		}
		case at91_mc323_poweroff:
		{
			at91mc323_poweroff();
			break;
		}
		case at91_mc323_reset:
		{
			at91mc323_reset();
			break;
		}
		default:break;	
	}
	return 0;

} 

static struct file_operations at91mc323_fops =
{ 
	.owner = THIS_MODULE, 
	.open  = at91mc323_open,
	.release = at91mc323_release,
	.unlocked_ioctl = at91mc323_ioctl,
}; 



static struct miscdevice at91mc323_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "at91mc323",
	.fops = &at91mc323_fops,
};
   
static int __init  at91mc323_init(void)
{ 
    	int ret; 
    	ret = misc_register(&at91mc323_dev);
		
    	if (ret) 
    	{ 
        	printk("can't register major number\n"); 
        	return ret; 
    	}

		at91mc323_ctl_gpio_init();
		at91mc323_poweron();
		printk("at91mc323 driver v1.0\n");
    	return 0; 
} 

static void __exit at91mc323_exit(void) 
{ 
	    misc_deregister(&at91mc323_dev);
}


module_init(at91mc323_init);
module_exit(at91mc323_exit);
/* input param with insmod. */

//MODULE_INFO(build, UTS_VERSION);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhuqing@talentinfo.com.cn");