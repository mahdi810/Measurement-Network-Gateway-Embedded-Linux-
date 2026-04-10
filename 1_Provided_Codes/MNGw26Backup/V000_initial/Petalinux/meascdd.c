/*  meascdd.c - the ultimate character device driver for the PMB1# board.

* Copyright (C) 2022 K. Mueller
*

*/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/interrupt.h>

#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/fs.h>			// Header for the Linux file system support
#include <linux/uaccess.h>		// Required for the copy to user function
#include <asm/io.h>				// Required ioread / iowrite

#include "measdev.h"


/* Standard module information, edit as appropriate */
MODULE_LICENSE("GPL");
MODULE_AUTHOR
    ("kmueller");
MODULE_DESCRIPTION
    ("meascdd - MeasDemo character device driver");

#define DRIVER_NAME "meascdd"

#define DEVICE_NAME "meascdd"	///< The device will appear at /dev/meascdd using this value
#define CLASS_NAME  "meas"		///< The device class -- this is a character device driver

#define MDSUCCESS  0

/* command line parameters to your module. (unused)
unsigned myint = 0xdeadbeef;
char *mystr = "default";

module_param(myint, int, S_IRUGO);
module_param(mystr, charp, S_IRUGO);
*/

static int    majorNumber;                  ///< Stores the device number -- determined automatically
static int    numberOpens = 0;              ///< Counts the number of times the device is opened
static struct class*  meascddClass  = NULL; ///< The device-driver class struct pointer
static struct device* meascddDevice = NULL; ///< The device-driver device struct pointer
static volatile void *Meas_Base_Address = NULL;

// MEAScdd IO data
static unsigned int Reg_Num = 0;
static  MeasObj_struct TF_Obj_Snd, TF_Obj_Rcv;
static volatile unsigned int Led_Output;

// The prototype functions for the character driver -- must come before the struct definition
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops =
{
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release
};

struct meascdd_local {
	int irq;
	unsigned long mem_start;
	unsigned long mem_end;
	void __iomem *base_addr;
};

static irqreturn_t meascdd_irq(int irq, void *lp)
{
	void * reg_addr;

	reg_addr = (void *)((unsigned int)Meas_Base_Address + 0x0C);
	iowrite32(MBINT_ENABLE | MBINT_ACKN | TIMER100ms, reg_addr);
	iowrite32(Led_Output, Meas_Base_Address);
	Led_Output ^= 0x080;
	iowrite32(MBINT_ENABLE | TIMER100ms, reg_addr);
	return IRQ_HANDLED;
}

static int meascdd_probe(struct platform_device *pdev)
{
	struct resource *r_irq; /* Interrupt resources */
	struct resource *r_mem; /* IO mem resources */
	struct device *dev = &pdev->dev;
	struct meascdd_local *lp = NULL;

	int rc = 0;

	Led_Output = 1;
	dev_info(dev, "Device Tree Probing\n");
	/* Get iospace for the device */
	r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!r_mem) {
		dev_err(dev, "invalid address\n");
		return -ENODEV;
	}
	lp = (struct meascdd_local *) kmalloc(sizeof(struct meascdd_local), GFP_KERNEL);
	if (!lp) {
		dev_err(dev, "Cound not allocate meascdd device\n");
		return -ENOMEM;
	}
	dev_set_drvdata(dev, lp);
	lp->mem_start = r_mem->start;
	lp->mem_end = r_mem->end;

	if (!request_mem_region(lp->mem_start,
				lp->mem_end - lp->mem_start + 1,
				DRIVER_NAME)) {
		dev_err(dev, "Couldn't lock memory region at %p\n",
			(void *)lp->mem_start);
		rc = -EBUSY;
		goto error1;
	}

	lp->base_addr = ioremap(lp->mem_start, lp->mem_end - lp->mem_start + 1);
	if (!lp->base_addr) {
		dev_err(dev, "meascdd: Could not allocate iomem\n");
		rc = -EIO;
		goto error2;
	}

	/* save remapped base address for later use for file read/write */
	Meas_Base_Address = lp->base_addr;
	
	/* Get IRQ for the device */
	r_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!r_irq) {
		dev_info(dev, "no IRQ found\n");
		dev_info(dev, "meascdd at 0x%08x mapped to 0x%08x\n",
			(unsigned int __force)lp->mem_start,
			(unsigned int __force)lp->base_addr);
		return 0;
	}
	lp->irq = r_irq->start;
	rc = request_irq(lp->irq, &meascdd_irq, 0, DRIVER_NAME, lp);
	if (rc) {
		dev_err(dev, "testmodule: Could not allocate interrupt %d.\n",
			lp->irq);
		goto error3;
	}

	dev_info(dev,"meascdd at 0x%08x mapped to 0x%08x, irq=%d\n",
		(unsigned int __force)lp->mem_start,
		(unsigned int __force)lp->base_addr,
		lp->irq);

	// create device node and class for user space access
	// Try to dynamically allocate a major number for the device -- more difficult but worth it
	majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
	if (majorNumber < 0) {
		printk(KERN_ALERT "MEAScdd failed to register a major number\n");
		return majorNumber;
	}
	printk(KERN_INFO "MEAScdd: registered correctly with major number %d\n", majorNumber);
	
	// Register the device class
	meascddClass = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(meascddClass)) {			// Check for error and clean up if there is
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT "Failed to register device class\n");
		return PTR_ERR(meascddClass);	// Correct way to return an error on a pointer
	}
	printk(KERN_INFO "MEAScdd: device class registered correctly\n");
	
	// Register the device driver
	meascddDevice = device_create(meascddClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
	if (IS_ERR(meascddDevice)) {			// Clean up if there is an error
		class_destroy(meascddClass);		// Repeated code but the alternative is goto statements
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT "Failed to create the device\n");
		return PTR_ERR(meascddDevice);
	}

	return 0;
error3:
	free_irq(lp->irq, lp);
error2:
	release_mem_region(lp->mem_start, lp->mem_end - lp->mem_start + 1);
error1:
	kfree(lp);
	dev_set_drvdata(dev, NULL);
	return rc;
}

static int meascdd_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct meascdd_local *lp = dev_get_drvdata(dev);
	free_irq(lp->irq, lp);
	iounmap(lp->base_addr);
	release_mem_region(lp->mem_start, lp->mem_end - lp->mem_start + 1);
	kfree(lp);
	dev_set_drvdata(dev, NULL);
	return 0;
}

/*
** -----------------------------------------------------------------------------
** MEASCDD IO functions
**-----------------------------------------------------------------------------
*/

/** @brief The device open function that is called each time the device is opened
 *  This will only increment the numberOpens counter in this case.
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_open(struct inode *inodep, struct file *filep)
{
	if (numberOpens > 0) {
		printk(KERN_INFO "MEAScdd: Device successfully closed\n");
		return -EBUSY;
	}
	numberOpens++;
	printk(KERN_INFO "MEAScdd: Device has been opened %d time(s)\n", numberOpens);
	return MDSUCCESS;
}


/** @brief This function is called whenever device is being read from user space i.e. data is
 *  being sent from the device to the user. In this case is uses the copy_to_user() function to
 *  send the buffer string to the user and captures any errors.
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 *  @param buffer The pointer to the buffer to which this function writes the data
 *  @param len The length of the b
 *  @param offset The offset if required
 */
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
	int error_count = 0;

	TF_Obj_Snd.rnum = Reg_Num;
	TF_Obj_Snd.rvalue = ioread32(Meas_Base_Address + 4*Reg_Num);
	// copy_to_user has the format ( * to, *from, size) and returns 0 on success
	error_count = copy_to_user(buffer, &TF_Obj_Snd, MEASOBJ_SIZE);
	
	if (error_count == 0) {
		// if true then have success
		// printk(KERN_INFO "MEAScdd: Sent %d bytes to the user\n", (int)MEASOBJ_SIZE);
		return 0;			// clear the position to the start and return 0
	} else {
		printk(KERN_INFO "MEAScdd: Failed to send %d characters to the user\n", error_count);
		return -EFAULT;		// Failed -- return a bad address message (i.e. -14)
	}
}



/** @brief This function is called whenever the device is being written to from user space i.e.
 *  data is sent to the device from the user. The data is copied to the message[] array in this
 *  LKM using the sprintf() function along with the length of the string.
 *  @param filep A pointer to a file object
 *  @param buffer The buffer to that contains the string to write to the device
 *  @param len The length of the array of data that is being passed in the const char buffer
 *  @param offset The offset if required
 */
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset)
{
	void * reg_addr;

	reg_addr = 0;
	if (len == MEASOBJ_SIZE) {
		// memmove(&TF_Obj_Rcv, buffer, len);
		copy_from_user(&TF_Obj_Rcv, buffer, len);
		if (TF_Obj_Rcv.rnum == REGNUM_ID) {
			Reg_Num = TF_Obj_Rcv.rvalue;
		} else {
			if (TF_Obj_Rcv.rnum == 0) {
				Led_Output &= 0x080;
				Led_Output |= TF_Obj_Rcv.rvalue & 0x07f;
			}
			reg_addr = (unsigned int)(Meas_Base_Address + 4*TF_Obj_Rcv.rnum);
			iowrite32(TF_Obj_Rcv.rvalue, reg_addr);
		}
	} else {
		printk(KERN_INFO "MEAScdd: [write] len: %d (exp.: %d)\n", (int)len, (int)MEASOBJ_SIZE);
	}
	return len;
}


/** @brief The device release function that is called whenever the device is closed/released by
 *  the userspace program
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_release(struct inode *inodep, struct file *filep)
{
	numberOpens--;
	printk(KERN_INFO "MEAScdd: Device closed (%d open links)\n", numberOpens);
	return MDSUCCESS;
}

/*
** -----------------------------------------------------------------------------
** MEASCDD IO functions end
**-----------------------------------------------------------------------------
*/


#ifdef CONFIG_OF
static struct of_device_id meascdd_of_match[] = {
	{ .compatible = "xlnx,measdif-1.0", },
	{ /* end of list */ },
};
MODULE_DEVICE_TABLE(of, meascdd_of_match);
#else
# define meascdd_of_match
#endif


static struct platform_driver meascdd_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table	= meascdd_of_match,
	},
	.probe		= meascdd_probe,
	.remove		= meascdd_remove,
};

static int __init meascdd_init(void)
{
	printk("MEAScdd: initialized.\n");
	return platform_driver_register(&meascdd_driver);
}


static void __exit meascdd_exit(void)
{
	platform_driver_unregister(&meascdd_driver);
	printk(KERN_ALERT "MEAScdd: unregistered.\n");
}

module_init(meascdd_init);
module_exit(meascdd_exit);
