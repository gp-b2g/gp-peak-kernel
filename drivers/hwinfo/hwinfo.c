
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/device.h>  
#include <asm/uaccess.h>  

#include "hwinfo.h"

static int cellon_hwinfo_major = 0;
static int cellon_hwinfo_minor = 0;

static struct class* cellon_hwinfo_class = NULL;  
static struct cellon_hwinfo_dev* pcellon_hwinfo_dev = NULL;  

static int cellon_hwinfo_open(struct inode* inode, struct file* filp);  
static int cellon_hwinfo_release(struct inode* inode, struct file* filp);  
static ssize_t cellon_hwinfo_read(struct file* filp, char __user *buf, size_t count, loff_t* f_pos);  
static ssize_t cellon_hwinfo_write(struct file* filp, const char __user *buf, size_t count, loff_t* f_pos);  

static struct file_operations cellon_hwinfo_fops = {  
	.owner = THIS_MODULE,
	.open = cellon_hwinfo_open,
	.release = cellon_hwinfo_release,
	.read = cellon_hwinfo_read,
	.write = cellon_hwinfo_write,
};
 
static ssize_t cellon_hwinfo_show(struct device* dev, struct device_attribute* attr,  char* buf);  
static ssize_t cellon_hwinfo_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);  
 
static DEVICE_ATTR(lcdinfo, S_IRUGO | S_IWUSR, cellon_hwinfo_show, cellon_hwinfo_store); 

static int cellon_hwinfo_open(struct inode* inode, struct file* filp) 
{  
	struct cellon_hwinfo_dev* dev;
  
	dev = container_of(inode->i_cdev, struct cellon_hwinfo_dev, dev);  
	filp->private_data = dev;
  
	return 0;  
}  

static int cellon_hwinfo_release(struct inode* inode, struct file* filp) 
{  
	return 0;  
}  


static char cellon_hw_project_name[2][20] = 
{
	"Project:  C8680",
	"Project:  C8681",
};

#if 0
static char	cellon_hw_info[10][10] = 
{
	"NT35565",	// 	1. LCD
	"S2202",	//	2. TP
	"OV8825",	//	3. REAR CAMERA
	"OV2675",	//	4. FRONT CAMERA
	"BMA22X2",	//	5. G-SENSOR
	"BMC050",	//	6. E-COMPASS
	"LTR558",	//	7. L/P SENSOR
	"AR6005",	//	8. WIFI
	"WCN2243",	//	9. BT
	"WCN2243",	//	10. FM
};
#endif

static char cellon_lcd_info[7][50] = 
{
	"LCD:  TIANMA, IC: HX8369",
	"LCD:  TRULY, IC: OTM9608A",
	"LCD:  BOE, IC: NT35510",
	"LCD:  CHIMEI, IC: OTM8009",
	"LCD:  BYD, IC: NT35516",
	"LCD:  SHARP, IC: NT35565",
	"LCD:  UNKOWN",
};

static char cellon_TP_info[2][50] = 
{
	"TP:  Ofilm, IC: S2202",
	"TP:  TRULY, IC: HX8526A",
};


static char cellon_Rear_Camera_info[20] = 
{
	"Rear Camera:  ",
};

static char cellon_Front_Camera_info[20] = 
{
	"Front Camera:  ",
};

extern char CTP_Panel_manufacturer;
extern char Lcd_Panel_manufacturer;
extern char cellon_project_name;
extern char cellon_msm_sensor_name[2][20];
static ssize_t cellon_hwinfo_read(struct file* filp, char __user *buf, size_t count, loff_t* f_pos) 
{  
	int num;
	ssize_t err = 0;  
	struct cellon_hwinfo_dev* dev = filp->private_data;
	char hwinfo_buf[512];
	

	if(down_interruptible(&(dev->sem)))
	{  
	      return -ERESTARTSYS;  
	}       

	memset(buf, 0, count);
	memset(hwinfo_buf, 0, 512);
	
	//project name
	num = cellon_project_name;
	strcpy(hwinfo_buf, &cellon_hw_project_name[num][0]);
	strcat(hwinfo_buf, "\n");

	//LCD
	num = Lcd_Panel_manufacturer;	
	if(num > 6)
		num = 6;
	strcat(hwinfo_buf, &cellon_lcd_info[num][0]);
	strcat(hwinfo_buf, "\n");

	//TP
	num = CTP_Panel_manufacturer;
	strcat(hwinfo_buf, &cellon_TP_info[num][0]);
	strcat(hwinfo_buf, "\n");


	//Rear Camera
	num = cellon_project_name;
	strcat(hwinfo_buf, &cellon_Rear_Camera_info[0]);
	strcat(hwinfo_buf, &cellon_msm_sensor_name[0][0]);
	strcat(hwinfo_buf, "\n");

	//Front Camera
	num = cellon_project_name;
	strcat(hwinfo_buf, &cellon_Front_Camera_info[0]);
	strcat(hwinfo_buf, &cellon_msm_sensor_name[1][0]);
	strcat(hwinfo_buf, "\n");

	if(copy_to_user(buf, hwinfo_buf, strlen(hwinfo_buf)))
	{
		err = -EFAULT;
		goto out;
	}	

	err = sizeof(dev->lcdinfo);

out:
	up(&(dev->sem));
	return err;  
}  


static ssize_t cellon_hwinfo_write(struct file* filp, const char __user *buf, size_t count, loff_t* f_pos)
{  
	struct cellon_hwinfo_dev* dev = filp->private_data;  
	ssize_t err = 0;          

	if(down_interruptible(&(dev->sem)))
	{  
		return -ERESTARTSYS;
	}          

	if(copy_from_user(&(dev->lcdinfo), buf, count)) 
	{  
		err = -EFAULT;
		goto out;  
	}  

	err = sizeof(dev->lcdinfo);  

out:  
	up(&(dev->sem));  
	return err;  
}


static ssize_t __cellon_hwinfo_get_lcdinfo(struct cellon_hwinfo_dev* dev, char* buf) 
{  
	int lcdinfo = 0;

	if(down_interruptible(&(dev->sem))) 
	{                  
		return -ERESTARTSYS;          
	}          

	lcdinfo = dev->lcdinfo;
	up(&(dev->sem));

	return snprintf(buf, PAGE_SIZE, "Truly");
}  


static ssize_t __cellon_hwinfo_set_lcdinfo(struct cellon_hwinfo_dev* dev, const char* buf, size_t count) 
{  
	int lcdinfo = 0;

	lcdinfo = simple_strtol(buf, NULL, 10);

	if(down_interruptible(&(dev->sem)))
	{                  
		return -ERESTARTSYS;          
	}          

	dev->lcdinfo = lcdinfo;          
	up(&(dev->sem));  

	return count;  
}  


static ssize_t cellon_hwinfo_show(struct device* dev, struct device_attribute* attr, char* buf) 
{  
	struct cellon_hwinfo_dev* hdev = (struct cellon_hwinfo_dev*)dev_get_drvdata(dev);          

	return __cellon_hwinfo_get_lcdinfo(hdev, buf);
}  

static ssize_t cellon_hwinfo_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{   
	struct cellon_hwinfo_dev* hdev = (struct cellon_hwinfo_dev*)dev_get_drvdata(dev);    

	return __cellon_hwinfo_set_lcdinfo(hdev, buf, count);
}


static ssize_t cellon_hwinfo_proc_read(char* page, char** start, off_t off, int count, int* eof, void* data)
{  
	if(off > 0) 
	{  
		*eof = 1;  
		return 0;
	}  

	 return __cellon_hwinfo_get_lcdinfo(pcellon_hwinfo_dev, page);
}  

static ssize_t cellon_hwinfo_proc_write(struct file* filp, const char __user *buff, unsigned long len, void* data)
{  
	int err = 0;  
	char* page = NULL;  

	if(len > PAGE_SIZE) 
	{  
		printk(KERN_ALERT"The buff is too large: %lu.\n", len);
		return -EFAULT;  
	}  

	page = (char*)__get_free_page(GFP_KERNEL);  
	if(!page) 
	{                  
		printk(KERN_ALERT"Failed to alloc page.\n");  
		return -ENOMEM;  
	}          

	if(copy_from_user(page, buff, len))
	{  
		printk(KERN_ALERT"Failed to copy buff from user.\n");                  
		err = -EFAULT;  
		goto out;  
	}  

	err = __cellon_hwinfo_set_lcdinfo(pcellon_hwinfo_dev, page, len);  
 
out:  
	free_page((unsigned long)page);  
	return err;  
}  
 

static void cellon_hwinfo_create_proc(void) 
{  
	struct proc_dir_entry* entry;
 
	entry = create_proc_entry(CELLON_HWINFO_DEVICE_PROC_NAME, 0, NULL);  
	if(entry) 
	{  
		entry->read_proc = cellon_hwinfo_proc_read;  
		entry->write_proc = cellon_hwinfo_proc_write;  
	}  
}  


static void cellon_hwinfo_remove_proc(void)
{  
	remove_proc_entry(CELLON_HWINFO_DEVICE_PROC_NAME, NULL);  
}



static int  __cellon_hwinfo_setup_dev(struct cellon_hwinfo_dev* dev)
{  
	int err;  
	dev_t devno = MKDEV(cellon_hwinfo_major, cellon_hwinfo_minor);  

	memset(dev, 0, sizeof(struct cellon_hwinfo_dev));  

	cdev_init(&(dev->dev), &cellon_hwinfo_fops);
	dev->dev.owner = THIS_MODULE;
	dev->dev.ops = &cellon_hwinfo_fops;

	err = cdev_add(&(dev->dev),devno, 1);  
	if(err)
	{  
		return err;  
	}     

	sema_init(&(dev->sem), 1);
	dev->lcdinfo = 1;

	return 0;  
}  


static int __init cellon_hwinfo_init(void)
{   
	int err = -1;  
	dev_t dev = 0;  
	struct device* temp = NULL;  

	printk(KERN_ALERT"Initializing cellon hwinfo device.\n");      

	err = alloc_chrdev_region(&dev, 0, 1, CELLON_HWINFO_DEVICE_NODE_NAME);
	if(err < 0) 
	{  
		printk(KERN_ALERT"Failed to alloc char dev region.\n");  
		goto fail;  
	}  

	cellon_hwinfo_major = MAJOR(dev);  
	cellon_hwinfo_minor = MINOR(dev);          

	pcellon_hwinfo_dev = kmalloc(sizeof(struct cellon_hwinfo_dev), GFP_KERNEL);  
	if(!pcellon_hwinfo_dev)
	{  
		err = -ENOMEM;  
		printk(KERN_ALERT"Failed to alloc cellon hwinfo dev.\n");  
		goto unregister;
	}          

	err = __cellon_hwinfo_setup_dev(pcellon_hwinfo_dev);
	if(err)
	{
		printk(KERN_ALERT"Failed to setup dev: %d.\n", err);  
		goto cleanup;
	}          

	cellon_hwinfo_class = class_create(THIS_MODULE, CELLON_HWINFO_DEVICE_CLASS_NAME);  
	if(IS_ERR(cellon_hwinfo_class)) 
	{  
		err = PTR_ERR(cellon_hwinfo_class);  
		printk(KERN_ALERT"Failed to create cellon class.\n");  
		goto destroy_cdev;  
	}          

	temp = device_create(cellon_hwinfo_class, NULL, dev, "%s", CELLON_HWINFO_DEVICE_FILE_NAME);
	if(IS_ERR(temp)) 
	{  
	 	err = PTR_ERR(temp);
		printk(KERN_ALERT"Failed to create cellon device.");  
		goto destroy_class;  
	}          


	err = device_create_file(temp, &dev_attr_lcdinfo);  
	if(err < 0) 
	{  
		printk(KERN_ALERT"Failed to create attribute lcdinfo.");        
		goto destroy_device;  
	}  

	dev_set_drvdata(temp, pcellon_hwinfo_dev);

	cellon_hwinfo_create_proc();

	printk(KERN_ALERT"Succedded to initialize cellon device.\n");  
	return 0;  
 
destroy_device:
	device_destroy(cellon_hwinfo_class, dev);

destroy_class:
	class_destroy(cellon_hwinfo_class);
 
destroy_cdev:  
	cdev_del(&(pcellon_hwinfo_dev->dev));  

cleanup:  
	 kfree(pcellon_hwinfo_dev);

unregister:  
	 unregister_chrdev_region(MKDEV(cellon_hwinfo_major, cellon_hwinfo_minor), 1);  

fail:  
	return err;  
}  


static void __exit cellon_hwinfo_exit(void) 
{  
	dev_t devno = MKDEV(cellon_hwinfo_major, cellon_hwinfo_minor);  

	printk(KERN_ALERT"Destroy cellon device.\n");

	cellon_hwinfo_remove_proc();

	if(cellon_hwinfo_class) 
	{
		device_destroy(cellon_hwinfo_class, MKDEV(cellon_hwinfo_major, cellon_hwinfo_minor));  
		class_destroy(cellon_hwinfo_class);
	}          

	if(pcellon_hwinfo_dev) 
	{
		cdev_del(&(pcellon_hwinfo_dev->dev));  
		kfree(pcellon_hwinfo_dev);
	}          

	unregister_chrdev_region(devno, 1);
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("cellon hwinfo Driver");

module_init(cellon_hwinfo_init);
module_exit(cellon_hwinfo_exit);

