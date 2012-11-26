
#ifndef _CELLON_HWINFO_H_
#define _CELLON_HWINFO_H_  
 
#include <linux/cdev.h>  
#include <linux/semaphore.h>  

#define CELLON_HWINFO_DEVICE_NODE_NAME  "cellonhw"
#define CELLON_HWINFO_DEVICE_FILE_NAME  "cellonhw"
#define CELLON_HWINFO_DEVICE_PROC_NAME  "cellonhw"
#define CELLON_HWINFO_DEVICE_CLASS_NAME "cellonhw"

struct cellon_hwinfo_dev {
 	int val;
 	int lcdinfo;
	int hwid;
	char modulename[64];
	char drivername[64];
	struct semaphore sem;
	struct cdev dev;
};
   
#endif

