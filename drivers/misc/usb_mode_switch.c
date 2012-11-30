#include <linux/module.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>


struct work_struct work ;
static struct workqueue_struct *usb_wq;

//change the USB composite to diag interface
int cellon_show_diag(void)
{
	struct file *fp;
	loff_t pos = 0;
	fp = filp_open("/sys/class/android_usb/android0/enable", O_WRONLY, 0);
	if (1 == IS_ERR(fp)) 
	{
		printk("%s: enable file open failed\n", __FUNCTION__);
		return -1;
	}
	fp->f_op->write(fp, "0", 1, &pos);
	filp_close(fp, current->files);
	fp = NULL;

	fp = filp_open("/sys/class/android_usb/android0/idProduct", O_WRONLY, 0);
	if (1 == IS_ERR(fp)) 
	{
		printk("%s: idProduct file open failed\n", __FUNCTION__);
		return -1;
	}
	fp->f_op->write(fp, "9025", 4, &pos);
	filp_close(fp, current->files);
	fp = NULL;

	fp = filp_open("/sys/class/android_usb/android0/idVendor", O_WRONLY, 0);
	if (1 == IS_ERR(fp)) 
	{
		printk("%s: idVendor file open failed\n", __FUNCTION__);
		return -1;
	}
	fp->f_op->write(fp, "05C6", 4, &pos);
	filp_close(fp, current->files);
	fp = NULL;

	fp = filp_open("/sys/class/android_usb/android0/f_diag/clients", O_WRONLY, 0);
	if (1 == IS_ERR(fp)) 
	{
		printk("%s: clients file open failed\n", __FUNCTION__);
		return -1;
	}
	fp->f_op->write(fp, "diag", 4, &pos);
	filp_close(fp, current->files);
	fp = NULL;

	fp = filp_open("/sys/class/android_usb/android0/f_serial/transports", O_WRONLY, 0);
	if (1 == IS_ERR(fp)) 
	{
		printk("%s: transports file open failed\n", __FUNCTION__);
		return -1;
	}
	fp->f_op->write(fp, "smd,tty", 7, &pos);
	filp_close(fp, current->files);
	fp = NULL;

	fp = filp_open("/sys/class/android_usb/android0/functions", O_WRONLY, 0);
	if (1 == IS_ERR(fp)) 
	{
		printk("%s: functions file open failed\n", __FUNCTION__);
		return -1;
	}
	fp->f_op->write(fp, "diag,adb,serial,serial,rmnet_smd", 32, &pos);
	filp_close(fp, current->files);
	fp = NULL;
	
	fp = filp_open("/sys/class/android_usb/android0/enable", O_WRONLY, 0);
	if (1 == IS_ERR(fp)) 
	{
		printk("%s: enable1 file open failed\n", __FUNCTION__);
		return -1;
	}
	fp->f_op->write(fp, "1", 1, &pos);
	filp_close(fp, current->files);
	fp = NULL;
	
	printk("%s: mode swith end\n", __FUNCTION__);

	return 1;
}

static void mode_switch_work(struct work_struct *work)
{
	int open_file_flag = 0;
	
	open_file_flag = cellon_show_diag();
	if (1 != open_file_flag)
		printk("%s: mode swith failed\n", __FUNCTION__);	
}

void cellon_usb_mode_switch(void)
{
	queue_work(usb_wq, &work);
}

static int __init usb_mode_switch_init(void) 
{
	usb_wq = create_singlethread_workqueue("usb_wq");
    if (!usb_wq)
        return -ENOMEM;

	INIT_WORK(&work, mode_switch_work);
	
	return 0;
}

static void __exit usb_mode_switch_exit(void)
{
	if (usb_wq)
        destroy_workqueue(usb_wq);
}

module_init(usb_mode_switch_init);
module_exit(usb_mode_switch_exit);

MODULE_DESCRIPTION("cellon Driver");
MODULE_LICENSE("GPL");


