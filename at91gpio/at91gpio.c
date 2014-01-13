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
#include "at91gpio.h"
#define LED       		AT91_PIN_PB18
#define LIQUIDLEVEL     AT91_PIN_PA15

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
		
		//case at91_gpio_port_PE:
		//{
		//	port_pin = (PIN_BASE + 0x80 + which_pin);
		//	break;
		//}
		
		default:
		{
			printk(KERN_INFO "has no such port!\n");
			return -1;
		}
	}
	return port_pin;
}
static int gpio_init(int port_pin, int inout)
{
    //at91_set_GPIO_periph(LED, 0); /* pullup disable */
    //gpio_direction_output(LED, 1); /* output *///并设置为高电平

	//at91_set_gpio_input(LIQUIDLEVEL, 0); /* input */ 

	switch(inout)
	{
		case at91_gpio_in:
		{
			at91_set_gpio_input(port_pin, 0); /* input */  
			break;
		}

		case at91_gpio_out:
		{
			at91_set_gpio_output(port_pin, 0);
			break;
		}

		default:
		{
			printk(KERN_INFO "gpio has no such status!\n");
			return -1;
		}
	}
	return 0;
	
}

static int at91gpio_open(struct inode *inode ,struct file *file)
{
    	return 0;
}

static int at91gpio_release(struct inode *inode ,struct file *file)
{
    	return 0;
}

static long at91gpio_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int port_pin;
	at91_gpio_ctl_s at91_gpio_ctl_arg;
	
	if(copy_from_user(&at91_gpio_ctl_arg, (at91_gpio_ctl_s *)arg, sizeof(at91_gpio_ctl_s)) != 0 )
	{
		printk(KERN_INFO "error in copy_from_user!\n");
		return -EFAULT;
	}
	//printk(KERN_INFO "******cmd****** : %d\n",cmd);
	port_pin = get_port_pin(at91_gpio_ctl_arg.which_port, at91_gpio_ctl_arg.which_pin);
	if(-1 == port_pin)
	{
		printk(KERN_INFO "get port and pin error!\n");
		return -1;
	}
	switch(cmd)
	{
		
		case AT91GPIO_WRITE:
		{
			//printk(KERN_INFO "*******at91_gpio_write******\n");
			at91_set_gpio_value(port_pin, at91_gpio_ctl_arg.value);
			break;
		}
		case AT91GPIO_READ:
		{
			at91_gpio_ctl_arg.value = at91_get_gpio_value(port_pin);
			//printk(KERN_INFO "====================\n");
			//printk(KERN_INFO "gpio value : %d\n",at91_gpio_ctl_arg.value);
			//printk(KERN_INFO "====================\n");
			if(copy_to_user((at91_gpio_ctl_s *)arg, &at91_gpio_ctl_arg, sizeof(at91_gpio_ctl_s)) != 0)
			{
				return -EFAULT;
			}
			break;
		}
		case AT91GPIO_CTL:
		{
			//printk(KERN_INFO "*******at91_gpio_ctl******\n");
			gpio_init(port_pin,at91_gpio_ctl_arg.inout);
			break;
		}
		default:break;	
	}
	return 0;

} 

static struct file_operations at91gpio_fops =
{ 
	.owner = THIS_MODULE, 
	.open  = at91gpio_open,
	.release = at91gpio_release,
	.unlocked_ioctl = at91gpio_ioctl,
}; 



static struct miscdevice at91gpio_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "at91gpio",
	.fops = &at91gpio_fops,
};
   
static int __init  at91gpio_init(void)
{ 
    	int ret; 
    	ret = misc_register(&at91gpio_dev);
		
    	if (ret) 
    	{ 
        	printk("can't register major number\n"); 
        	return ret; 
    	}

		//gpio_init();
		printk("at91gpio driver v1.0\n");
    	return 0; 
} 

static void __exit at91gpio_exit(void) 
{ 
	    misc_deregister(&at91gpio_dev);
}


module_init(at91gpio_init);
module_exit(at91gpio_exit);
/* input param with insmod. */

//MODULE_INFO(build, UTS_VERSION);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhuqing@talentinfo.com.cn");