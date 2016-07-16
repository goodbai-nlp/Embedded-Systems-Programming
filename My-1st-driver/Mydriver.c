/* ----------------------------------------------- DRIVER gmem --------------------------------------------------
 
 Basic driver example to show skelton methods for several file operations.
 
 ----------------------------------------------------------------------------------------------------------------*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/mutex.h>

#include <linux/init.h>
#include <linux/moduleparam.h>

#define DEVICE_NAME1  "bus_in"  // device name to be created and registered
#define DEVICE_NAME2  "bus_out1"  // device name to be created and registered
#define DEVICE_NAME3  "bus_out2"  // device name to be created and registered
#define DEVICE_NAME4  "bus_out3"  // device name to be created and registered
 
#define QUEUELEN 11

typedef struct msg{
    int seqnum,source,destination;
    char **str;
}MSG;
/* per device structure */
struct gmem_dev {
	struct cdev cdev;               /* The cdev structure */
	char name[20];                  /* Name of device*/
	// char in_string[256];			 buffer for the input string 
	MSG * buff;
	int qfront,qrear;
	// int current_write_pointer;
	struct mutex lck;
	MSG m;
} *gmem_devp;

static dev_t gmem_dev_number1;      /* Allotted device number */
static dev_t gmem_dev_number2;      /* Allotted device number */
static dev_t gmem_dev_number3;      /* Allotted device number */
static dev_t gmem_dev_number4;      /* Allotted device number */

struct class *gmem_dev_class1;          /* Tie with the device model */
struct class *gmem_dev_class2;          /* Tie with the device model */
struct class *gmem_dev_class3;          /* Tie with the device model */
struct class *gmem_dev_class4;          /* Tie with the device model */

static struct device *gmem_dev_device1;
static struct device *gmem_dev_device2;
static struct device *gmem_dev_device3;
static struct device *gmem_dev_device4;

struct gmem_dev *gmem_devp1,*gmem_devp2,*gmem_devp3,*gmem_devp4;
static char *user_name = "Dear JackBai";

module_param(user_name,charp,0000);	//to get parameter from load.sh script to greet the user

/*
* Open gmem driver
*/
int gmem_driver_open(struct inode *inode, struct file *file)
{
	struct gmem_dev *gmem_devp;
//	printk("\nopening\n");

	/* Get the per-device structure that contains this cdev */
	gmem_devp = container_of(inode->i_cdev, struct gmem_dev, cdev);

	/* Easy access to cmos_devp from rest of the entry points */
	file->private_data = gmem_devp;
	printk("\n%s is openning \n", gmem_devp->name);
	return 0;
}

/*
 * Release gmem driver
 */
int gmem_driver_release(struct inode *inode, struct file *file)
{
	struct gmem_dev *gmem_devp = file->private_data;
	
	printk("\n%s is closing\n", gmem_devp->name);
	
	return 0;
}
/*
 * Write to gmem driver
 */
ssize_t gmem_driver_write(struct file *file, MSG *buf,
           size_t count, loff_t *ppos){
	struct gmem_dev *gmem_devp = file->private_data;
	int tmp;
	//while (count) {	
	if(!copy_from_user(&(gmem_devp->m), buf, sizeof(MSG))){  
    	printk("copy msg from user\n");
    	printk("seqnum %d sender: %d destination %d %d\n",(gmem_devp->m).seqnum,(gmem_devp->m).source,(gmem_devp->m).destination,(gmem_devp->m).str);
    	mutex_lock(&(gmem_devp)->lck);
    	tmp = (gmem_devp->qrear+1)%(QUEUELEN);
	    if(gmem_devp->qfront == tmp){
	        printk("queue is full\n");
	        mutex_unlock(&(gmem_devp->lck));
	        return -1;
	    }
	    gmem_devp->buff[gmem_devp->qrear] = gmem_devp->m;
	    gmem_devp->qrear = (gmem_devp->qrear+1)%(QUEUELEN);
	    mutex_unlock(&(gmem_devp->lck));
		return 1;
    }
    printk("copy_from_user failed\n");
	return -1;
}
/*
 * Read to gmem driver
 */
ssize_t gmem_driver_read(struct file *file, MSG *buf,size_t count,loff_t *ppos){
	printk("=========================\n");
	int bytes_read = 0;
	struct gmem_dev *gm = file->private_data;
	MSG *mm =(MSG*)kmalloc(sizeof(MSG),GFP_KERNEL);
	// (*mm)->str=NULL;
	/*
	 * If we're at the end of the message, 
	 * return 0 signifying end of file 
	 */
	mutex_lock(&(gm->lck));
	if(gm->qfront == gm->qrear){
        printk("queue is empty!\n");
        mutex_unlock(&(gm->lck));
        return -1;
    }
    *mm = gm->buff[gm->qfront];
    printk("top element address %d\n",(gm->buff[gm->qfront]).str);
    gm->qfront = (gm->qfront +1)%(QUEUELEN);
    if(!copy_to_user(buf,mm,24)){
    	printk("%d\n",sizeof(*mm));
    	printk("%d->%d:\n",buf,mm);
		printk("copy to user\n");
		printk("copy: seqnum %d sender: %d destination %d %d\n",(*buf).seqnum,(*buf).source,(*buf).destination,(*buf).str);
		mutex_unlock(&(gm->lck));
		return 1;
	}
	mutex_unlock(&(gm->lck));
	printk("copy failed\n");
	return -1;
}

/* File operations structure. Defined in linux/fs.h */
static struct file_operations gmem_fops = {
    .owner		= THIS_MODULE,           /* Owner */
    .open		= gmem_driver_open,        /* Open method */
    .release	= gmem_driver_release,     /* Release method */
    .write		= gmem_driver_write,       /* Write method */
    .read		= gmem_driver_read,        /* Read method */
};

/*
 * Driver Initialization
 */
int __init gmem_driver_init(void)
{
	int ret1,ret2,ret3,ret4;
	int time_since_boot;

	/* Request dynamic allocation of a device major number */
	if (alloc_chrdev_region(&gmem_dev_number1, 0, 1, DEVICE_NAME1) < 0) {
			printk(KERN_DEBUG "Can't register device\n"); return -1;
	}
	if (alloc_chrdev_region(&gmem_dev_number2, 0, 1, DEVICE_NAME2) < 0) {
			printk(KERN_DEBUG "Can't register device\n"); return -1;
	}
	if (alloc_chrdev_region(&gmem_dev_number3, 0, 1, DEVICE_NAME3) < 0) {
			printk(KERN_DEBUG "Can't register device\n"); return -1;
	}
	if (alloc_chrdev_region(&gmem_dev_number4, 0, 1, DEVICE_NAME4) < 0) {
			printk(KERN_DEBUG "Can't register device\n"); return -1;
	}

	/* Populate sysfs entries */
	gmem_dev_class1 = class_create(THIS_MODULE, DEVICE_NAME1);
	gmem_dev_class2 = class_create(THIS_MODULE, DEVICE_NAME2);
	gmem_dev_class3 = class_create(THIS_MODULE, DEVICE_NAME3);
	gmem_dev_class4 = class_create(THIS_MODULE, DEVICE_NAME4);

	/* Allocate memory for the per-device structure */
	gmem_devp1 = kmalloc(sizeof(struct gmem_dev), GFP_KERNEL);
	gmem_devp2 = kmalloc(sizeof(struct gmem_dev), GFP_KERNEL);
	gmem_devp3 = kmalloc(sizeof(struct gmem_dev), GFP_KERNEL);
	gmem_devp4 = kmalloc(sizeof(struct gmem_dev), GFP_KERNEL);

	if ((!gmem_devp1)||(!gmem_devp2)||(!gmem_devp3)||(!gmem_devp4)){
		printk("Bad Kmalloc\n"); return -ENOMEM;
	}

	/* Request I/O region */
	sprintf(gmem_devp1->name, DEVICE_NAME1);
	sprintf(gmem_devp2->name, DEVICE_NAME2);
	sprintf(gmem_devp3->name, DEVICE_NAME3);
	sprintf(gmem_devp4->name, DEVICE_NAME4);

	/* Connect the file operations with the cdev */
	cdev_init(&gmem_devp1->cdev, &gmem_fops);
	gmem_devp1->cdev.owner = THIS_MODULE;
	cdev_init(&gmem_devp2->cdev, &gmem_fops);
	gmem_devp2->cdev.owner = THIS_MODULE;
	cdev_init(&gmem_devp3->cdev, &gmem_fops);
	gmem_devp3->cdev.owner = THIS_MODULE;
	cdev_init(&gmem_devp4->cdev, &gmem_fops);
	gmem_devp4->cdev.owner = THIS_MODULE;
	/* Connect the major/minor number to the cdev */
	ret1 = cdev_add(&gmem_devp1->cdev, (gmem_dev_number1), 1);
	ret2 = cdev_add(&gmem_devp2->cdev, (gmem_dev_number2), 1);
	ret3 = cdev_add(&gmem_devp3->cdev, (gmem_dev_number3), 1);
	ret4 = cdev_add(&gmem_devp4->cdev, (gmem_dev_number4), 1);
	if (ret1||ret2||ret3||ret4) {
		printk("Bad cdev\n");
		return 0;
	}

	/* Send uevents to udev, so it'll create /dev nodes */
	gmem_dev_device1 = device_create(gmem_dev_class1, NULL, MKDEV(MAJOR(gmem_dev_number1), 0), NULL, DEVICE_NAME1);		
	gmem_dev_device2 = device_create(gmem_dev_class2, NULL, MKDEV(MAJOR(gmem_dev_number2), 0), NULL, DEVICE_NAME2);		
	gmem_dev_device3 = device_create(gmem_dev_class3, NULL, MKDEV(MAJOR(gmem_dev_number3), 0), NULL, DEVICE_NAME3);		
	gmem_dev_device4 = device_create(gmem_dev_class4, NULL, MKDEV(MAJOR(gmem_dev_number4), 0), NULL, DEVICE_NAME4);		




	// device_create_file(gmem_dev_device, &dev_attr_xxx);

	// memset(gmem_devp->in_string, 0, 256);
	mutex_init(&(gmem_devp1->lck));
	gmem_devp1->buff = (MSG *)kmalloc(sizeof(MSG)*QUEUELEN,GFP_KERNEL);
	//gmem_devp1->m->str = 
	gmem_devp1->qrear = gmem_devp1->qfront=0;
	mutex_init(&(gmem_devp2->lck));
	gmem_devp2->buff = (MSG *)kmalloc(sizeof(MSG)*QUEUELEN,GFP_KERNEL);
	gmem_devp2->qrear = gmem_devp2->qfront=0;

	mutex_init(&(gmem_devp3->lck));
	gmem_devp3->buff = (MSG *)kmalloc(sizeof(MSG)*QUEUELEN,GFP_KERNEL);
	gmem_devp3->qrear = gmem_devp3->qfront=0;

	mutex_init(&(gmem_devp4->lck));
	gmem_devp4->buff = (MSG *)kmalloc(sizeof(MSG)*QUEUELEN,GFP_KERNEL);
	gmem_devp4->qrear = gmem_devp4->qfront=0;
	time_since_boot=(jiffies-INITIAL_JIFFIES)/HZ;//since on some systems jiffies is a very huge uninitialized value at boot and saved.
	// sprintf(gmem_devp->in_string, "Hi %s, this machine has been on for %d seconds", user_name, time_since_boot);
	
	// gmem_devp->current_write_pointer = 0;

	printk("gmem driver initialized.\n");// '%s'\n",gmem_devp->in_string);
	return 0;
}
/* Driver Exit */
void __exit gmem_driver_exit(void)
{
	// device_remove_file(gmem_dev_device, &dev_attr_xxx);
	/* Release the major number */
	unregister_chrdev_region((gmem_dev_number1), 1);
	unregister_chrdev_region((gmem_dev_number2), 1);
	unregister_chrdev_region((gmem_dev_number3), 1);
	unregister_chrdev_region((gmem_dev_number4), 1);
	/* Destroy device */
	device_destroy (gmem_dev_class1, MKDEV(MAJOR(gmem_dev_number1), 0));
	device_destroy (gmem_dev_class2, MKDEV(MAJOR(gmem_dev_number2), 0));
	device_destroy (gmem_dev_class3, MKDEV(MAJOR(gmem_dev_number3), 0));
	device_destroy (gmem_dev_class4, MKDEV(MAJOR(gmem_dev_number4), 0));

	cdev_del(&gmem_devp1->cdev);
	kfree(gmem_devp1);
	cdev_del(&gmem_devp2->cdev);
	kfree(gmem_devp2);
	cdev_del(&gmem_devp3->cdev);
	kfree(gmem_devp3);
	cdev_del(&gmem_devp4->cdev);
	kfree(gmem_devp4);
	/* Destroy driver_class */
	class_destroy(gmem_dev_class1);
	class_destroy(gmem_dev_class2);
	class_destroy(gmem_dev_class3);
	class_destroy(gmem_dev_class4);
	printk("gmem driver removed.\n");
}

module_init(gmem_driver_init);
module_exit(gmem_driver_exit);
MODULE_LICENSE("GPL v2");

