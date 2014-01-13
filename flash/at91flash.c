#include <asm/div64.h>
#include <linux/err.h>
#include <linux/mtd/mtd.h>
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
#include <linux/semaphore.h>
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

#include "at91flash.h"
#define MAX_KMALLOC_SIZE 0x20000

#define PRINT_PREF KERN_INFO "at91flash: "

static int dev;

module_param(dev, int, S_IRUGO);
MODULE_PARM_DESC(dev, "MTD device number to use");

static struct mtd_info *mtd;
static unsigned char *bbt;
static int ebcnt;
static int pgsize;
static int pgcnt;
static int is_block_bad(int ebnum)
{
	loff_t addr = ebnum * mtd->erasesize;
	int ret;

	ret = mtd->block_isbad(mtd, addr);
	//if (ret)
		//printk(PRINT_PREF "block %d is bad\n", ebnum);
	return ret;
}

static int scan_for_bad_eraseblocks(void)
{
	int i, bad = 0;

	bbt = kzalloc(ebcnt, GFP_KERNEL);
	if (!bbt) {
		printk(PRINT_PREF "error: cannot allocate memory\n");
		return -ENOMEM;
	}

	/* NOR flash does not implement block_isbad */
	if (mtd->block_isbad == NULL)
		return 0;

	printk(PRINT_PREF "scanning for bad eraseblocks\n");
	for (i = 0; i < ebcnt; ++i) {
		bbt[i] = is_block_bad(i) ? 1 : 0;
		if (bbt[i])
			bad += 1;
		cond_resched();
	}
	printk(PRINT_PREF "scanned %d eraseblocks, %d are bad\n", i, bad);
	return 0;
}

static int erase_eraseblock(int ebnum)
{
	int err;
	struct erase_info ei;
	loff_t addr = ebnum * mtd->erasesize;

	memset(&ei, 0, sizeof(struct erase_info));
	ei.mtd  = mtd;
	ei.addr = addr;
	ei.len  = mtd->erasesize;

	err = mtd->erase(mtd, &ei);
	if (err) {
		printk(PRINT_PREF "error %d while erasing EB %d\n", err, ebnum);
		return err;
	}

	if (ei.state == MTD_ERASE_FAILED) {
		printk(PRINT_PREF "some erase error occurred at EB %d\n",
		       ebnum);
		return -EIO;
	}

	return 0;
}

static int at91flash_open(struct inode *inode ,struct file *file)
{
		return 0;
}

static int at91flash_release(struct inode *inode ,struct file *file)
{
    	return 0;
}

static loff_t at91flash_lseek (struct file *file, loff_t offset, int orig)
{
	switch (orig) {
	case SEEK_SET:
		break;
	case SEEK_CUR:
		offset += file->f_pos;
		break;
	case SEEK_END:
		offset += mtd->size;
		break;
	default:
		return -EINVAL;
	}
	if (offset >= 0 && offset <= mtd->size)
	{
		return file->f_pos = offset;
	}
	return -EINVAL;
}

static ssize_t at91flash_read(struct file *file,char __user *buf,size_t count, loff_t *ppos)
{
	/**********************************************/
	size_t readlen = 0;
	size_t readtmplen = 0;
	int ret, err = 0;
	loff_t addr;
	char *readbuf;
	int ebnum;
	int blockoffset;
	uint64_t tmp;

	while(1)
	{
		tmp = *ppos;
		do_div(tmp, mtd->erasesize);
		ebnum = tmp;
		
		if(ebnum >= ebcnt)
		{
			printk(KERN_INFO"bad blocks!!!\n");
			return -EFAULT;
		}
		
		bbt[ebnum] = is_block_bad(ebnum) ? 1 : 0;
		
		if (bbt[ebnum])//bad block???
		{
			*ppos = (ebnum + 1) * mtd->erasesize;//jump to the start of next block!
			if (*ppos >= mtd->size)
			{
				printk(KERN_INFO"end of file!!!\n");
				return -EFAULT;
			}
		}
		else
		{
			break;
		}
	}
	
	addr = *ppos;//read from this address!!!
	if (*ppos + count > mtd->size)
	{
		count = mtd->size - *ppos;
		printk(KERN_INFO"reach the end of file!!!\n");
	}
		
	if (!count)
		return 0;
	
	if (count > MAX_KMALLOC_SIZE)
			count = MAX_KMALLOC_SIZE;
	readbuf = kmalloc(count, GFP_KERNEL);
	if (!readbuf)
		return -ENOMEM;
	/*********************/
	if(*ppos + count > (ebnum + 1) * mtd->erasesize)//block jump
	{
		//read the first block
		ret = mtd->read(mtd, addr, (ebnum + 1) * mtd->erasesize - *ppos, &readlen, readbuf);
		if (ret == -EUCLEAN)
			ret = 0;
		if (ret || readlen != ((ebnum + 1) * mtd->erasesize - *ppos)) 
		{
			printk(PRINT_PREF "error: read failed at %#llx\n",
			       (long long)addr);
		}
		if(readlen != 0)
		{
			*ppos += readlen;
			readtmplen = readlen;
			if (copy_to_user(buf, readbuf, readlen)) 
			{
				kfree(readbuf);
				return -EFAULT;
			}
		}
		//find the second good block
		while(1)
		{
			tmp = *ppos;
			do_div(tmp, mtd->erasesize);
			ebnum = tmp;
			if(ebnum >= ebcnt)
			{
				printk(KERN_INFO"bad blocks!!!\n");
				kfree(readbuf);
				return readlen;
			}

			bbt[ebnum] = is_block_bad(ebnum) ? 1 : 0;
			
			if (bbt[ebnum])//bad block???
			{
				*ppos = (ebnum + 1) * mtd->erasesize;//jump to the start of next block!
				if (*ppos >= mtd->size)
				{
					printk(KERN_INFO"end of file!!!\n");
					kfree(readbuf);
					return -EFAULT;
				}
			}
			else
			{
				break;
			}
		}
		//read the second block
		addr = *ppos;
		ret = mtd->read(mtd, addr, count - readtmplen, &readlen, readbuf);
		if (ret == -EUCLEAN)
			ret = 0;
		if (ret || readlen != count - readtmplen) 
		{
			printk(PRINT_PREF "error: read failed at %#llx\n",
			       (long long)addr);
		}
		if(readlen != 0)
		{
			*ppos += readlen;
			if (copy_to_user(buf + readtmplen, readbuf, readlen)) 
			{
				kfree(readbuf);
				return -EFAULT;
			}
		}
		kfree(readbuf);
		return readlen;
	}
	/*********************/
	else if(*ppos + count <= (ebnum + 1) * mtd->erasesize)
	{
		ret = mtd->read(mtd, addr, count, &readlen, readbuf);
		if (ret == -EUCLEAN)
			ret = 0;
		if (ret || readlen != count) 
		{
			printk(PRINT_PREF "error: read failed at %#llx\n",
			       (long long)addr);
		}
		if(readlen != 0)
		{
			*ppos += readlen;
			if (copy_to_user(buf, readbuf, readlen)) 
			{
				kfree(readbuf);
				return -EFAULT;
			}
		}
		kfree(readbuf);
		return readlen;
	}
	else
	{
		printk(KERN_INFO"read too much!!!\n");
		kfree(readbuf);
		return -EFAULT;
	}
	
	/**********************************************/
}

static ssize_t at91flash_write(struct file *file, char __user *buf, size_t count,loff_t *ppos)
{
	int err = 0;
	int ret;
	size_t written = 0;
	char *kbuf = NULL;
	loff_t readaddr;
	int readlen;
	loff_t addr;;
	uint64_t tmp;
	int ebnum;
	int blockoffset;
	if (*ppos + count > mtd->size)
	{
		count = mtd->size - *ppos;
		printk(KERN_INFO"reach the end of file!!!\n");
	}
	else if(count > mtd->erasesize)
	{
		printk(KERN_INFO"you can not write > mtd->erasesize!!!\n");
		return -EFAULT;
	}
	
		
	if (!count)
	{
		printk(KERN_INFO"you can not write 0 byte!!!\n");
		return 0;
	}
		
	
	
	while(1)//find first good block!!!
	{
		tmp = *ppos;
		blockoffset = do_div(tmp, mtd->erasesize);
		ebnum = tmp;
		if(ebnum >= ebcnt)
		{
			printk(KERN_INFO"bad blocks!!!\n");
			return -EFAULT;
		}

		bbt[ebnum] = is_block_bad(ebnum) ? 1 : 0;
		
		if (bbt[ebnum])//bad block???
		{
			*ppos = (ebnum + 1) * mtd->erasesize;//jump to the start of next block!
			if (*ppos >= mtd->size)
			{
				printk(KERN_INFO"end of file!!!\n");
				return -EFAULT;
			}
			
		}
		else
		{
			break;
		}
	}

	kbuf = kmalloc(mtd->erasesize, GFP_KERNEL);
	if (!kbuf)
		return -ENOMEM;
	
	addr = *ppos;//read from this address!!!
	if(!addr)//first block
	{	
		if (copy_from_user(kbuf, buf, count)) 
		{
			kfree(kbuf);
			return -EFAULT;
		}
		goto erase_action;
	}
	//firstly, read
	readaddr = ebnum * mtd->erasesize;//start of block
	ret = mtd->read(mtd, readaddr, mtd->erasesize, &readlen, kbuf);//read the original data,the hole block
	if (ret == -EUCLEAN)
		ret = 0;
	if (ret || readlen != mtd->erasesize) 
	{
		printk(PRINT_PREF "error: read failed at %#llx\n",
		       (long long)readaddr);
		kfree(kbuf);
		return -EFAULT;
	}
	//secondly, data assembly
	if(*ppos + count <= (ebnum + 1) * mtd->erasesize)//·ÀÖ¹blockÌøÔ¾
	{
		if (copy_from_user(kbuf + blockoffset, buf, count)) 
		{
			kfree(kbuf);
			return -EFAULT;
		}
	}
	else if ((*ppos + count > (ebnum + 1) * mtd->erasesize) && (*ppos + count <= (ebnum + 2) * mtd->erasesize))
	{
		//copy data
		if (copy_from_user(kbuf + blockoffset, buf, mtd->erasesize - blockoffset)) 
		{
			kfree(kbuf);
			return -EFAULT;
		}
		//first block
		//erase
		err = erase_eraseblock(ebnum);
		if(err)
		{
			kfree(kbuf);
			return -EFAULT;
		}
		//write
		addr = ebnum * mtd->erasesize;
		//cond_resched();
		err = mtd->write(mtd, addr, mtd->erasesize, &written, kbuf);
		if (err || written != mtd->erasesize)
			printk(PRINT_PREF "error: write failed at %#llx\n",
			       (long long)addr);
		*ppos += mtd->erasesize - blockoffset;
		//second block
		while(1)//find next good block!!!
		{
			tmp = *ppos;
			do_div(tmp, mtd->erasesize);
			ebnum = tmp;
			if(ebnum >= ebcnt)
			{
				printk(KERN_INFO"bad blocks!!!\n");
				kfree(kbuf);
				return -EFAULT;
			}

			bbt[ebnum] = is_block_bad(ebnum) ? 1 : 0;
			
			if (bbt[ebnum])//bad block???
			{
				*ppos = (ebnum + 1) * mtd->erasesize;//jump to the start of next block!
				if (*ppos >= mtd->size)
				{
					printk(KERN_INFO"end of file!!!\n");
					kfree(kbuf);
					return -EFAULT;
				}
			}
			else
			{
				break;
			}
		}
		//read second block
		readaddr = ebnum * mtd->erasesize;
		ret = mtd->read(mtd, readaddr, mtd->erasesize, &readlen, kbuf);//read the original data,the hole block
		if (ret == -EUCLEAN)
		ret = 0;
		if (ret || readlen != mtd->erasesize) 
		{
			printk(PRINT_PREF "error: read failed at %#llx\n",
			       (long long)readaddr);
			kfree(kbuf);
			return -EFAULT;
		}
		//copy data
		if (copy_from_user(kbuf, buf + (mtd->erasesize - blockoffset), count - (mtd->erasesize - blockoffset))) 
		{
			printk(KERN_INFO"copy data error!!!\n");
			kfree(kbuf);
			return -EFAULT;
		}
		//erase
		err = erase_eraseblock(ebnum);
		if(err)
		{
			kfree(kbuf);
			return -EFAULT;
		}
		//write
		addr = ebnum * mtd->erasesize;
		
		//cond_resched();
		err = mtd->write(mtd, addr, mtd->erasesize, &written, kbuf);
		if (err || written != mtd->erasesize)
			printk(PRINT_PREF "error: write failed at %#llx\n",
			       (long long)addr);
		*ppos += count - (mtd->erasesize - blockoffset);
		kfree(kbuf);
		return count;
	}
	else
	{
		printk(KERN_INFO"you write too much data!!!\n");
		kfree(kbuf);
		return -EFAULT;
	}
	//finally, erase
erase_action :

	err = erase_eraseblock(ebnum);
	if(err)
	{
		kfree(kbuf);
		return -EFAULT;
	}
	
	//write
	addr = ebnum * mtd->erasesize;
	//cond_resched();
	err = mtd->write(mtd, addr, mtd->erasesize, &written, kbuf);
	if (err || written != mtd->erasesize)
		printk(PRINT_PREF "error: write failed at %#llx\n",
		       (long long)addr);
	*ppos += written;
	kfree(kbuf);
	return written;
}

static long at91flash_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	flash_data karg;
	if(copy_from_user(&karg, (flash_data *)arg, sizeof(flash_data)) != 0 )
	{
		printk(KERN_INFO "error in copy_from_user!\n");
		return -EFAULT;
	}
	switch(_IOC_NR(cmd))
	{
		
		case _IOC_NR(AT91FLASH_READ):
		{
			//printk(KERN_INFO"...read in...\n");
			at91flash_lseek(file, karg.offset, SEEK_SET);
			at91flash_read(file, karg.pbuf, karg.len, &file->f_pos);
			if(copy_to_user((flash_data *)arg, &karg, sizeof(flash_data)) != 0 )
			{
				printk(KERN_INFO "error in at91flash_ioctl : read!\n");
				return -EFAULT;
			}
			//printk(KERN_INFO"...read out...\n");
			break;
		}
		case _IOC_NR(AT91FLASH_WRITE):
		{
			//printk(KERN_INFO"...write in...\n");
			//printk(KERN_INFO"karg.offset : %d karg.len : %d karg.pbuf : %d\n", karg.offset, karg.len, karg.pbuf[0]);
			at91flash_lseek(file, karg.offset, SEEK_SET);
			at91flash_write(file, karg.pbuf, karg.len, &file->f_pos);
			if(copy_to_user((flash_data *)arg, &karg, sizeof(flash_data)) != 0 )
			{
				printk(KERN_INFO "error in at91flash_ioctl : write!\n");
				return -EFAULT;
			}
			//printk(KERN_INFO"...write out...\n");
			break;
		}
		case _IOC_NR(AT91FLASH_LSEEK):
		{
			
			break;
		}
		default:break;	
	}
	return 0;
}

static struct file_operations at91flash_fops =
{ 
	.owner = THIS_MODULE, 
	.open  = at91flash_open,
	.read  = at91flash_read,
	.write = at91flash_write,
	.release = at91flash_release,
	.llseek = at91flash_lseek,
	.unlocked_ioctl = at91flash_ioctl,
}; 



static struct miscdevice at91flash_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "at91flash",
	.fops = &at91flash_fops,
};

static int __init at91flash_init(void)
{
	int err = 0;
	uint64_t tmp;
	/**********************************************************/
	dev = 2;//means mtd2
	int ret; 
	ret = misc_register(&at91flash_dev);
	
	if (ret) 
	{ 
    	printk("can't register major number\n"); 
    	return ret; 
	}

	printk("at91flash driver v1.0\n");

	mtd = get_mtd_device(NULL, dev);
	if (IS_ERR(mtd)) {
		err = PTR_ERR(mtd);
		printk(PRINT_PREF "error: cannot get MTD device\n");
		misc_deregister(&at91flash_dev);
		return err;
	}

	if (mtd->type != MTD_NANDFLASH) {
		printk(PRINT_PREF "this test requires NAND flash\n");
		goto out;
	}

	tmp = mtd->size;
	do_div(tmp, mtd->erasesize);
	ebcnt = tmp;
	pgcnt = mtd->erasesize / mtd->writesize;
	pgsize = mtd->writesize;
	
	err = scan_for_bad_eraseblocks();
	if (err)
		goto out;
	
	return 0; 
	/**********************************************************/
out:
	kfree(bbt);
	put_mtd_device(mtd);
	misc_deregister(&at91flash_dev);
	return 0; 
}
module_init(at91flash_init);

static void __exit at91flash_exit(void)
{
	misc_deregister(&at91flash_dev);
	put_mtd_device(mtd);
	kfree(bbt);
	return;
}
module_exit(at91flash_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhuqing@talentinfo.com.cn");

