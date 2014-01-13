/******************************************************************************
| File name : at91adc.c |
| Description : Source code for the EDAQ ADC driver. This driver supports two |
| : adc channesl with minor numbers 0 and 1. When the module is |
| : is inserted to kernel, TC0 module is setup to generate interr-|
| : upt every 1 msec. Both ADC channels are sampled in the ISR |
| : and the samples are copied into a ring buffer. Sampling rate |
| : can be changed by changing SAMPLE_INTERVAL_MS. In response to |
| : the read from user space, latest requested number of samples |
| : are returned as an unsigned short array. Channel 0/1 data is |
| : returned depending on the minor number of the device opened. |
| History : |
| ### Date Author Comments |
| 001 30-Jul-08 S. Thalore Initial creation |
| Copyright 2008 | 
| This program is free software: you can redistribute it and/or modify |
| it under the terms of the GNU General Public License as published by |
| the Free Software Foundation, either version 3 of the License, or |
| (at your option) any later version. |
| |
| This program is distributed in the hope that it will be useful, |
| but WITHOUT ANY WARRANTY; without even the implied warranty of |
| MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the |
| GNU General Public License for more details. |
| |
| You should have received a copy of the GNU General Public License |
| along with this program. If not, see <http://www.gnu.org/licenses/>. |
******************************************************************************/
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
#include <linux/irqreturn.h>
#include <linux/clk.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <linux/slab.h> //zhuqing
#include <linux/timer.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 26)
//warning build for Linux-2.6.27 or above
#include <mach/hardware.h>
#include <mach/at91_pio.h>
#include <mach/gpio.h>
#include <mach/at91_tc.h>
#include <mach/at91_adc.h>
//#include <mach/at91sam9x5.h>
#else 
//warning build for Linux-2.6.26 or earlier
#include <asm/arch/gpio.h>
#include <asm/arch/hardware.h>
#endif


// Set the period between reads (max allowed value 1000)
#define SAMPLE_INTERVAL_MS 5


// Assume client always reads data at 1 second intervals; so the client will read:
#define READ_SAMPLES 1000/SAMPLE_INTERVAL_MS
// Allocate to be at least 5 x bigger so that ISR will never catch up with reading.
#define MAX_ADCSAMPLES 16*1024//5* READ_SAMPLES


//int at91adc_devno;
//struct cdev at91adc_cdev;
struct file_operations at91adc_fops;

//unsigned short  *at91adc_pbuf0, *at91adc_pbuf1, at91adc_appidx;
unsigned short  *at91adc_pbuf0,*at91adc_pbuf1,*at91adc_pbuf2,*at91adc_pbuf3,
*at91adc_pbuf4, *at91adc_pbuf5,*at91adc_pbuf6,*at91adc_pbuf7,*at91adc_pbuf8,
*at91adc_pbuf9,*at91adc_pbuf10,*at91adc_pbuf11,at91adc_appidx = 0;

void __iomem *at91tc1_base;
void __iomem *at91adc_base;
struct clk *at91adc_clk;
struct clk *at91tc1_clk;
#if 0
//----------------------------
int FullSample = 0;
#define SAMPLE_TIME 1024
unsigned short at91adc_ch6_buf[SAMPLE_TIME];
int sample_num = 0;
struct timer_list adc_timer;
#define TIMER_DELAY HZ/100
void adc_timer_handler(void)
{
	//printk("%s\n",__FUNCTION__);
	iowrite32(AT91_ADC_START, (at91adc_base + AT91_ADC_CR));//start to convert
	udelay(10);
	at91adc_ch6_buf[sample_num] = ioread32(at91adc_base + AT91_ADC_CHR(6));
	sample_num++;
	if(sample_num >=  SAMPLE_TIME)
	{
		sample_num = 0;
		FullSample = 1;
	}
	// add timer
	//add_timer(&adc_timer) ;
	int ret = mod_timer(&adc_timer, jiffies + TIMER_DELAY);
	//printk("mod_timer retval:%d\n", ret);
	//printk("jiffies : %lu\n",jiffies);
	return;
}	
void adc_timer_setup(void)
{
	/*** Initialize the timer structure***/
	printk("TIMER_DELAY : %d\n",HZ/100);
	init_timer(&adc_timer) ;
	adc_timer.function = adc_timer_handler ;
	adc_timer.expires = jiffies + TIMER_DELAY; // HZ = 100 
	add_timer(&adc_timer);
	/***Initialisation ends***/
	return;
}
void sample(void)
{
	while(1)
	{
		iowrite32(AT91_ADC_START, (at91adc_base + AT91_ADC_CR));//start to convert
		udelay(100);
		at91adc_ch6_buf[sample_num] = ioread32(at91adc_base + AT91_ADC_CHR(6));
		sample_num++;
		if(sample_num >=  SAMPLE_TIME)
		{
			sample_num = 0;
			FullSample = 1;
		}
	}
}
#endif
//----------------------------
/*****************************************************************************************
| Timer counter 0 ISR: Sample both ADC channels and copy the data to module ring buffer. |
*****************************************************************************************/
static irqreturn_t at91tc0_isr (int irq, void *dev_id)
{ 
	int status;
	int i;
	//struct timeval time;
	static int timecount = 0; 
	// Read TC0 status register to reset RC compare status.
	status = ioread32(at91tc1_base + AT91_TC_SR);

	//timecount++;
	//if (timecount >= SAMPLE_INTERVAL_MS)
	//{
	//	timecount = 0;
		//do_gettimeofday(&time);
		//printk(KERN_INFO "Time %u:%u\n",time.tv_sec, time.tv_usec);

		// Trigger the ADC (this will be done using TIOA automatically eventually).
		iowrite32(AT91_ADC_START, (at91adc_base + AT91_ADC_CR));

		// Wait for conversion to be complete.
		//while ((ioread32(at91adc_base + AT91_ADC_SR) & AT91_ADC_DRDY) == 0) cpu_relax();
		for(i=0;i<10;i++)//make sure adc convert ok!
		{
			cpu_relax();
		}

		// Copy converted data to module ring buffer.
		at91adc_pbuf0[at91adc_appidx] = ioread32(at91adc_base + AT91_ADC_CHR(0)) & 0x3ff;
		at91adc_pbuf1[at91adc_appidx] = ioread32(at91adc_base + AT91_ADC_CHR(1)) & 0x3ff;
		at91adc_pbuf2[at91adc_appidx] = ioread32(at91adc_base + AT91_ADC_CHR(2)) & 0x3ff;
		at91adc_pbuf3[at91adc_appidx] = ioread32(at91adc_base + AT91_ADC_CHR(3)) & 0x3ff;
		at91adc_pbuf4[at91adc_appidx] = ioread32(at91adc_base + AT91_ADC_CHR(4)) & 0x3ff;
		//at91adc_pbuf5[at91adc_appidx] = ioread32(at91adc_base + AT91_ADC_CHR(6)) & 0x3ff;
		at91adc_pbuf6[at91adc_appidx] = ioread32(at91adc_base + AT91_ADC_CHR(6)) & 0x3ff;
		at91adc_pbuf7[at91adc_appidx] = ioread32(at91adc_base + AT91_ADC_CHR(7)) & 0x3ff;
		at91adc_pbuf8[at91adc_appidx] = ioread32(at91adc_base + AT91_ADC_CHR(8)) & 0x3ff;
		at91adc_pbuf9[at91adc_appidx] = ioread32(at91adc_base + AT91_ADC_CHR(9)) & 0x3ff;
		at91adc_pbuf10[at91adc_appidx] = ioread32(at91adc_base + AT91_ADC_CHR(10)) & 0x3ff;
		at91adc_pbuf11[at91adc_appidx] = ioread32(at91adc_base + AT91_ADC_CHR(11)) & 0x3ff;
		
		//printk("data: %d\n", at91adc_pbuf6[at91adc_appidx]);
		//at91adc_pbuf1[at91adc_appidx] = ioread32(at91adc_base + AT91_ADC_CHR(1));

		// Increment the ring buffer index and check for wrap around.
		at91adc_appidx += 1;
		if (at91adc_appidx >= MAX_ADCSAMPLES) at91adc_appidx = 0;
	//}

return IRQ_HANDLED;
}

/*****************************************************************************************
| Module open: |
*****************************************************************************************/
static int at91adc_open (struct inode *inode, struct file *filp) 
{ 
	return 0; 
} 

/*****************************************************************************************
| Module close: |
*****************************************************************************************/
static int at91adc_release (struct inode *inode, struct file *filp) 
{ 
	return 0; 
} 

/*****************************************************************************************
| Module read: Return last READ_SAMPLES samples from ADC chan 0 or 1 depending on the |
| minor number. |
*****************************************************************************************/
static ssize_t at91adc_read (struct file *filp, char  __user *buf, size_t bufsize, loff_t *f_pos)
{
	int i,j;
	unsigned short at91adc_buf[11];
	memset(at91adc_buf,0x0,sizeof(at91adc_buf));
	unsigned int data_add0 = 0;
	unsigned int data_add1 = 0;
	unsigned int data_add2 = 0;
	unsigned int data_add3 = 0;
	unsigned int data_add4 = 0;
	//unsigned int data_add5 = 0;
	unsigned int data_add6 = 0;
	unsigned int data_add7 = 0;
	unsigned int data_add8 = 0;
	unsigned int data_add9 = 0;
	unsigned int data_add10 = 0;
	unsigned int data_add11 = 0;
	//iowrite32(AT91_ADC_START, (at91adc_base + AT91_ADC_CR));//start to convert
	//udelay(10);
	//printk(KERN_INFO"reading adc channel data...\n");
	#if 0
	for(i = 0;i < 11;i++)
	{
		if(i >= 5)
		{
			if(5 == i)
			{
				int j;
				unsigned int data_add = 0;
				for(j=0;j<MAX_ADCSAMPLES;j++)
				{
					data_add += at91adc_pbuf6[j];
				}
				//at91adc_buf[i] = data_add/MAX_ADCSAMPLES;
				at91adc_buf[i] = data_add>>10;
				at91adc_buf[i] &= 0x3ff;//10 bit
			}
			else
			{
				at91adc_buf[i] = ioread32(at91adc_base + AT91_ADC_CHR(i + 1));//AD5 不用
				at91adc_buf[i] &= 0x3ff;//10 bit
			}
			
		}
		else
		{
			at91adc_buf[i] = ioread32(at91adc_base + AT91_ADC_CHR(i));
			at91adc_buf[i] &= 0x3ff;//10 bit
		}
		
		
	}
	#endif
	for(j=0;j<MAX_ADCSAMPLES;j++)
	{
		data_add0 += at91adc_pbuf0[j];
		data_add1 += at91adc_pbuf1[j];
		data_add2 += at91adc_pbuf2[j];
		data_add3 += at91adc_pbuf3[j];
		data_add4 += at91adc_pbuf4[j];
		//data_add5 += at91adc_pbuf5[j];
		data_add6 += at91adc_pbuf6[j];
		data_add7 += at91adc_pbuf7[j];
		data_add8 += at91adc_pbuf8[j];
		data_add9 += at91adc_pbuf9[j];
		data_add10 += at91adc_pbuf10[j];
		data_add11 += at91adc_pbuf11[j];
	}
	at91adc_buf[0] = data_add0>>14;
	at91adc_buf[0] &= 0x3ff;//10 bit

	at91adc_buf[1] = data_add1>>14;
	at91adc_buf[1] &= 0x3ff;//10 bit

	at91adc_buf[2] = data_add2>>14;
	at91adc_buf[2] &= 0x3ff;//10 bit

	at91adc_buf[3] = data_add3>>14;
	at91adc_buf[3] &= 0x3ff;//10 bit

	at91adc_buf[4] = data_add4>>14;
	at91adc_buf[4] &= 0x3ff;//10 bit

	at91adc_buf[5] = data_add6>>14;
	at91adc_buf[5] &= 0x3ff;//10 bit

	at91adc_buf[6] = data_add7>>14;
	at91adc_buf[6] &= 0x3ff;//10 bit

	at91adc_buf[7] = data_add8>>14;
	at91adc_buf[7] &= 0x3ff;//10 bit

	at91adc_buf[8] = data_add9>>14;
	at91adc_buf[8] &= 0x3ff;//10 bit

	at91adc_buf[9] = data_add10>>14;
	at91adc_buf[9] &= 0x3ff;//10 bit

	at91adc_buf[10] = data_add11>>14;
	at91adc_buf[10] &= 0x3ff;//10 bit

	if (copy_to_user(buf, at91adc_buf, sizeof(at91adc_buf)) != 0) 
	{
		return -EFAULT;
	}
	
	return 0;
	
	
	
}

struct file_operations at91adc_fops = { 
	.owner = THIS_MODULE, 
	.open = at91adc_open, 
	.read = at91adc_read, 
	.release = at91adc_release, 
}; 
static struct miscdevice at91adc_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "at91adc",
	.fops = &at91adc_fops,
};

/*****************************************************************************************
| Timer counter 0 ISR: Sample both ADC channels and copy the data to module ring buffer. |
*****************************************************************************************/


/*****************************************************************************************
| Module initialization: Allocate device numbers, register device, setup ADC and timer |
| counter registers for 100 msec periodic sampling. |
*****************************************************************************************/
static int __init at91adc_init (void)
{
	int ret; 
	ret = misc_register(&at91adc_dev);
	
	if (ret) 
	{ 
    	printk("can't register major number\n"); 
    	return ret; 
	}

	// Allocate ring buffer memory for storing ADC values for both channels.
	at91adc_pbuf0 = (unsigned short *)kmalloc((MAX_ADCSAMPLES * sizeof(unsigned short)), // Number of bytes
GFP_KERNEL); // Flags
at91adc_pbuf1 = (unsigned short *)kmalloc((MAX_ADCSAMPLES * sizeof(unsigned short)), // Number of bytes
GFP_KERNEL); // Flags
at91adc_pbuf2 = (unsigned short *)kmalloc((MAX_ADCSAMPLES * sizeof(unsigned short)), // Number of bytes
GFP_KERNEL); // Flags
at91adc_pbuf3 = (unsigned short *)kmalloc((MAX_ADCSAMPLES * sizeof(unsigned short)), // Number of bytes
GFP_KERNEL); // Flags
at91adc_pbuf4 = (unsigned short *)kmalloc((MAX_ADCSAMPLES * sizeof(unsigned short)), // Number of bytes
GFP_KERNEL); // Flags
//at91adc_pbuf5 = (unsigned short *)kmalloc((MAX_ADCSAMPLES * sizeof(unsigned short)), // Number of bytes
//GFP_KERNEL); // Flags
at91adc_pbuf6 = (unsigned short *)kmalloc((MAX_ADCSAMPLES * sizeof(unsigned short)), // Number of bytes
GFP_KERNEL); // Flags
at91adc_pbuf7 = (unsigned short *)kmalloc((MAX_ADCSAMPLES * sizeof(unsigned short)), // Number of bytes
GFP_KERNEL); // Flags
at91adc_pbuf8 = (unsigned short *)kmalloc((MAX_ADCSAMPLES * sizeof(unsigned short)), // Number of bytes
GFP_KERNEL); // Flags
at91adc_pbuf9 = (unsigned short *)kmalloc((MAX_ADCSAMPLES * sizeof(unsigned short)), // Number of bytes
GFP_KERNEL); // Flags
at91adc_pbuf10 = (unsigned short *)kmalloc((MAX_ADCSAMPLES * sizeof(unsigned short)), // Number of bytes
GFP_KERNEL); // Flags
at91adc_pbuf11 = (unsigned short *)kmalloc((MAX_ADCSAMPLES * sizeof(unsigned short)), // Number of bytes
GFP_KERNEL); // Flags
/*
at91adc_pbuf1 = (unsigned short *)kmalloc((MAX_ADCSAMPLES * sizeof(unsigned short)), // Number of bytes
GFP_KERNEL); // Flags
*/
if ((at91adc_pbuf0 == NULL) || (at91adc_pbuf1 == NULL)||(at91adc_pbuf2 == NULL) || (at91adc_pbuf3 == NULL)
	||(at91adc_pbuf4 == NULL)// || (at91adc_pbuf5 == NULL)
	||(at91adc_pbuf6 == NULL) || (at91adc_pbuf7== NULL)
	||(at91adc_pbuf8 == NULL) || (at91adc_pbuf9 == NULL)||(at91adc_pbuf10 == NULL) || (at91adc_pbuf11 == NULL)
	)
//if (at91adc_pbuf6 == NULL)
{
	printk(KERN_INFO "at91adc: Memory allocation failed\n");
	ret = -ECANCELED;
	goto exit_3;
}

// Initialize the ring buffer and append index.
at91adc_appidx = 0;
for (ret = 0; ret < MAX_ADCSAMPLES; ret++)
{
	at91adc_pbuf0[ret] = 0;
	at91adc_pbuf1[ret] = 0;
	at91adc_pbuf2[ret] = 0;
	at91adc_pbuf3[ret] = 0;
	at91adc_pbuf4[ret] = 0;
	//at91adc_pbuf5[ret] = 0;
	at91adc_pbuf6[ret] = 0;
	at91adc_pbuf7[ret] = 0;
	at91adc_pbuf8[ret] = 0;
	at91adc_pbuf9[ret] = 0;
	at91adc_pbuf10[ret] = 0;
	at91adc_pbuf11[ret] = 0;
}

	// Initialize ADC. The following two lines set the appropriate PMC bit 
	// for the ADC. Easier than mapping PMC registers and then setting the bit.
	at91adc_clk = clk_get(NULL, // Device pointer - not required.
	"adc_clk"); // Clock name.
	clk_enable(at91adc_clk);

	// Map ADC registers to the current address space.
	at91adc_base = ioremap_nocache(AT91SAM9X5_BASE_ADC, // Physical address
	64); // Number of bytes to be mapped.
	if (at91adc_base == NULL)
	{
		printk(KERN_INFO "at91adc: ADC memory mapping failed\n");
		ret = -EACCES;
		goto exit_4;
	}

	// MUX GPIO pins for ADC (peripheral A) operation 
	at91_set_C_periph(AT91_PIN_PB6, 0);
	at91_set_C_periph(AT91_PIN_PB7, 0);
	at91_set_C_periph(AT91_PIN_PB8, 0);
	at91_set_C_periph(AT91_PIN_PB9, 0);
	at91_set_C_periph(AT91_PIN_PB10, 0);
	at91_set_C_periph(AT91_PIN_PB11, 0);
	at91_set_C_periph(AT91_PIN_PB12, 0);
	at91_set_C_periph(AT91_PIN_PB13, 0);
	at91_set_C_periph(AT91_PIN_PB14, 0);
	at91_set_C_periph(AT91_PIN_PB15, 0);
	//at91_set_C_periph(AT91_PIN_PB16, 0);//不用做AD
	at91_set_C_periph(AT91_PIN_PB17, 0);

	// Reset the ADC
	iowrite32(AT91_ADC_SWRST, (at91adc_base + AT91_ADC_CR));

	// Enable both ADC channels
	//iowrite32((AT91_ADC_CH(1) | AT91_ADC_CH(0)), (at91adc_base + AT91_ADC_CHER));
	iowrite32((AT91_ADC_CH(1) | AT91_ADC_CH(0)
	| AT91_ADC_CH(3) | AT91_ADC_CH(2)
	//| AT91_ADC_CH(5) 
	| AT91_ADC_CH(4)
	| AT91_ADC_CH(7) | AT91_ADC_CH(6)
	| AT91_ADC_CH(9) | AT91_ADC_CH(8)
	| AT91_ADC_CH(11) | AT91_ADC_CH(10)),  (at91adc_base + AT91_ADC_CHER));
	
	// Configure ADC mode register.
	// From table 43-31 in page #775 and page#741 of AT91SAM9260 user manual:
	// Maximum ADC clock frequency = 5MHz = MCK / ((PRESCAL+1) * 2)
	// PRESCAL = ((MCK / 5MHz) / 2) -1 = ((100MHz / 5MHz)/2)-1) = 9
	// Maximum startup time = 15uS = (STARTUP+1)*8/ADC_CLOCK
	// STARTUP = ((15uS*ADC_CLOK)/8)-1 = ((15uS*5MHz)/8)-1 = 9
	// Minimum hold time = 1.2uS = (SHTIM+1)/ADC_CLOCK
	// SHTIM = (1.2uS*ADC_CLOCK)-1 = (1.2uS*5MHz)-1 = 5, Use 9 to ensure 2uS hold time.
	// Enable sleep mode and hardware trigger from TIOA output from TC0.

	//iowrite32((AT91_ADC_SHTIM_(9) | AT91_ADC_STARTUP_(9) | AT91_ADC_PRESCAL_(9) | 
	//AT91_ADC_SLEEP | AT91_ADC_TRGEN), (at91adc_base + AT91_ADC_MR));
	
	iowrite32((AT91_ADC_SHTIM_(9) | AT91_ADC_STARTUP_(9) | AT91_ADC_PRESCAL_(9) | 
	AT91_ADC_SLEEP), (at91adc_base + AT91_ADC_MR));

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Initialize Timer Counter module 0. The following two lines set the appropriate 
// PMC bit for TC0. Easier than mapping PMC registers and then setting the bit.
at91tc1_clk = clk_get(NULL, // Device pointer - not required.
"tcb1_clk"); // Clock name.
clk_enable(at91tc1_clk);

// Map TC0 registers to the current address space.
at91tc1_base = ioremap_nocache(AT91SAM9X5_BASE_TC1, // Physical address
64); // Number of bytes to be mapped.
if (at91tc1_base == NULL)
{
printk(KERN_INFO "at91adc: TC0 memory mapping failed\n");
ret = -EACCES;
goto exit_5;
}

// Configure TC0 in waveform mode, TIMER_CLK1 and to generate interrupt on RC compare.
// Load 50000 to RC so that with TIMER_CLK1 = MCK/2 = 50MHz, the interrupt will be
// generated every 1/50MHz * 50000 = 20nS * 50000 = 1 milli second.
// NOTE: Even though AT91_TC_RC is a 32-bit register, only 16-bits are programmble.

iowrite32(3000, (at91tc1_base + AT91_TC_RC));
iowrite32((AT91_TC_WAVE | AT91_TC_WAVESEL_UP_AUTO), (at91tc1_base + AT91_TC_CMR));
iowrite32(AT91_TC_CPCS, (at91tc1_base + AT91_TC_IER));
iowrite32((AT91_TC_SWTRG | AT91_TC_CLKEN), (at91tc1_base + AT91_TC_CCR));

// Install interrupt for TC0.
ret = request_irq(AT91SAM9X5_ID_TCB, // Interrupt number
at91tc0_isr, // Pointer to the interrupt sub-routine
IRQF_SHARED, // Flags - fast, shared or contributing to entropy pool
"at91adc", // Device name to show as owner in /proc/interrupts
"test"); // Private data for shared interrupts

if (ret != 0)
{
printk(KERN_INFO "at91adc: Timer interrupt request failed\n");
ret = -EBUSY;
goto exit_6;
}

printk(KERN_INFO "at91adc: Loaded module\n""v1.0.0\n");
//printk("HZ:%d\n",HZ);
//--------------------
//adc_timer_setup();
//--------------------
return 0;

exit_6:
iounmap(at91tc1_base); 

exit_5:
clk_disable(at91tc1_clk);
iounmap(at91adc_base);

exit_4:
clk_disable(at91adc_clk);

exit_3:
kfree(at91adc_pbuf0); 
kfree(at91adc_pbuf1);
kfree(at91adc_pbuf2); 
kfree(at91adc_pbuf3); 
kfree(at91adc_pbuf4); 
//kfree(at91adc_pbuf5); 
kfree(at91adc_pbuf6); 
kfree(at91adc_pbuf7); 
kfree(at91adc_pbuf8); 
kfree(at91adc_pbuf9); 
kfree(at91adc_pbuf10); 
kfree(at91adc_pbuf11); 

exit_2:
// Free device number allocated.
//unregister_chrdev_region(at91adc_devno, // allocated device number
misc_deregister(&at91adc_dev);
//2); // number of devices

exit_1:
return ret;
}

static void __exit at91adc_exit (void) 
{
	// Reset PMC bit for ADC and TC0
	clk_disable(at91adc_clk);
	clk_disable(at91tc1_clk);

	// Free TC0 IRQ.
	free_irq(AT91SAM9X5_ID_TCB, // Interrupt number
	"test"); // Private data for shared interrupts

	// Unmap ADC and TC0 register map.
	iounmap(at91adc_base);
	iounmap(at91tc1_base);

	// Free kernel memory allocated
	//kfree(at91adc_pbuf0); 
	//kfree(at91adc_pbuf1);

	// Free device number allocated.
	//unregister_chrdev_region(at91adc_devno, // allocated device number
	misc_deregister(&at91adc_dev);
	//2); // number of devices

	printk(KERN_INFO "at91adc: Unloaded module\n");
	//---------------------
	//del_timer(&adc_timer);
}




module_init(at91adc_init);
module_exit(at91adc_exit);

MODULE_AUTHOR("zhuqing@talentinfo.com.cn");
MODULE_DESCRIPTION("Initialize and read AT91SAM9X5 ADC channels");
MODULE_LICENSE("GPL");

