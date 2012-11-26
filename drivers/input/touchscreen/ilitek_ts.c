/*
 Copyright (c) 2010 by ilitek Technology.
	All rights reserved.

	ilitek I2C touch screen driver for Android platform
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/timer.h>
#include <linux/gpio.h>

#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <mach/gpio.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/slab.h>

#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/string.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

struct ilitek_ts_priv {
	struct i2c_client *client;
	struct input_dev *input;
	struct work_struct work;
	struct mutex mutex;
	struct hrtimer timer;
	#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
	#endif
	int irq;
//	bool isp_enabled;
//	bool autotune_result;
//	bool always_update;
//	char I2C_Offset;
//	char I2C_Length;
//	char I2C_RepeatTime;
//	int  fw_revision;
//	int	 struct_version;
};

#define	MAX_CHANNELS			32
#define CONFIG_PARAM_MAX_CHANNELS	32
#define MUTUAL_CALIBRATION_BASE_SIZE	256

#define FLAG_GPIO    122
#define INT_GPIO     82
#define RESET_GPIO	 30
#define ST_KEY_SEARCH 	217
static struct input_dev *simulate_key_input = NULL;

//#define USE_POLL
//#define DEBUG

#ifdef DEBUG
#define TP_DEBUG(format, args...)   printk(KERN_INFO "TP_:%s( )_%d_: " format, \
	__FUNCTION__ , __LINE__, ## args);
#define DBG() printk("[%s]:%d => \n",__FUNCTION__,__LINE__)
#else
#define TP_DEBUG(format, args...);
#define DBG()
#endif

typedef struct
{
	u8 status1  :  1,
	   status2  :  1,
	   no_status  :  6;
}valid_status;

static int ilitek_ts_get_fw_version(struct i2c_client *client, u32 *ver)
{
	char buf[4];
	int ret;

	buf[0] = 0x40;	//Set Reg. address to 0x40 for reading FW version.
	if ((ret = i2c_master_send(client, buf, 1)) != 1)
		return -EIO;
	mdelay(10);
	//Read 1 byte FW version from Reg. 0x40 set previously.
	if ((ret = i2c_master_recv(client, buf, 4)) != 4)
		return -EIO;

	*ver = ((u32)buf[3]);
	*ver |= (((u32)buf[2]) << 8);
	*ver |= (((u32)buf[1]) << 16);
	*ver |= (((u32)buf[0]) << 24);

	buf[0] = 0x10;	//Set Reg. address back to 0x10 for coordinates.
	if ((ret = i2c_master_send(client, buf, 1)) != 1)
		return -EIO;

	return 0;

}

static int ilitek_ts_get_resolution(struct i2c_client *client, u32 *rev)
{
	char buf[6];
	int ret;

	buf[0] = 0x20;	//Set Reg. address to 0x20 for reading resolution.
	if ((ret = i2c_master_send(client, buf, 1)) != 1)
		return -EIO;
	mdelay(10);
	//Read 1 byte FW version from Reg. 0x20 set previously.
	if ((ret = i2c_master_recv(client, buf, 6)) != 6)
		return -EIO;

	*rev = ((u32)buf[3]);
	*rev |= (((u32)buf[2]) << 8);
	*rev |= (((u32)buf[1]) << 16);
	*rev |= (((u32)buf[0]) << 24);

	buf[0] = 0x10;	//Set Reg. address back to 0x10 for coordinates.
	if ((ret = i2c_master_send(client, buf, 1)) != 1)
		return -EIO;

	return 0;

}

static int in_key_area(unsigned short x0, unsigned short y0) 
{

	if ((x0 >= 15) && (x0 <= 65) && (y0 >= 500) && (y0  <= 540)) {
		return KEY_MENU;
	} 
	else if ((x0 >= 95) && (x0 <= 145) && (y0 >= 500) && (y0  <= 540)) {
		return KEY_HOME;
	}
	else if ((x0 >= 175) && (x0 <= 225) && (y0 >= 500) && (y0  <= 540)) {
		return KEY_BACK;
	}
	else if ((x0 >= 255) && (x0 <= 305) && (y0 >= 500) && (y0  <= 540)) {
		return ST_KEY_SEARCH;
	}
	else{
		return 0;
	}
}

void ilitek_report_ponit(struct ilitek_ts_priv *priv)
{
	char buf[9];
	valid_status *pdata;
	int ret;
	static unsigned short key_value = 0;
	static unsigned short pre_key = 0;

	int x[2] = {(int) -1, (int) -1 };
	int y[2] = {(int) -1, (int) -1} ;
	static int xx[2] = { (int) -1, (int) -1 };
	static int yy[2] = { (int) -1, (int) -1 };
	static bool finger[2] = { 0, 0 };
	static bool flag = false;
	static bool key_down = false;
	struct i2c_client *client = NULL;
	mutex_lock(&priv->mutex);
	client = priv->client;
	
	buf[0] = 0x10;
	if ((ret = i2c_master_send(priv->client, buf, 1)) != 1) {
		dev_err(&priv->client->dev, "Unable to Send 0x10 to I2C!\n");
		goto out;
	}
	
	if ((ret = i2c_master_recv(priv->client, buf, sizeof(buf))) != sizeof(buf)) {
		dev_err(&priv->client->dev, "Unable to read XY data from I2C!\n");
		goto out;
	}

	pdata = (valid_status*)&buf[0];

	x[0] = (((int)buf[2]) << 8) + buf[1];
	y[0] = (((int)buf[4]) << 8) + buf[3];

	x[1] = (((int)buf[6]) << 8) + buf[5];
	y[1] = (((int)buf[8]) << 8) + buf[7];

	if((!pdata->status1 )&& (!pdata->status2))
	{ //no finger
		if (finger[0]) {
				input_report_abs(priv->input, ABS_MT_TRACKING_ID, 1);
				input_report_abs(priv->input, ABS_MT_TOUCH_MAJOR, 0);
				input_report_abs(priv->input, ABS_MT_WIDTH_MAJOR, 0);
				input_report_abs(priv->input, ABS_MT_POSITION_X, xx[0]);
				input_report_abs(priv->input, ABS_MT_POSITION_Y, yy[0]);
				input_mt_sync(priv->input);
				finger[0] = 0;
				flag = 1;
				TP_DEBUG("no finger: 111111--x[0] = %d, y[0] = %d\n", xx[0], yy[0]);
		}
		if (finger[1]) {
			input_report_abs(priv->input, ABS_MT_TRACKING_ID, 2);
			input_report_abs(priv->input, ABS_MT_TOUCH_MAJOR, 0);
			input_report_abs(priv->input, ABS_MT_WIDTH_MAJOR, 0);
			input_report_abs(priv->input, ABS_MT_POSITION_X, xx[1]);
			input_report_abs(priv->input, ABS_MT_POSITION_Y, yy[1]);
			input_mt_sync(priv->input);
			finger[1] = 0;
			flag = 1;
			TP_DEBUG("no finger: 22222--x[1] = %d, y[1] = %d\n", xx[1], yy[1]);
		}
		
		if(key_down) {
			input_report_key(simulate_key_input, pre_key, 0);
			key_down = false;
			input_sync(simulate_key_input);
			TP_DEBUG("Key UP = %d, \n", pre_key);
			key_value = 0;
			pre_key = 0;
		}
		
		if(flag == 0) 
			TP_DEBUG("Only no more data~~\n");
					
		if (flag) {
			input_sync(priv->input);
			flag = 0;
		}

		goto out;
	}	
	
	if (pdata->status1) 
	{

		/* If new key or screen ,Realse pre_key*/
		key_value = in_key_area(x[0], y[0]);
		if (( key_value != pre_key) && key_down){ 
			key_down = false;
			input_report_key(simulate_key_input, pre_key, 0);
			input_sync(simulate_key_input);
			TP_DEBUG("Key Up = %d, \n", pre_key);
			pre_key = 0;
			//goto out;
		}	
		
		if(key_value != 0) {
			 /* if touch key_area, release screen*/
			if(finger[0]) {           
				input_report_abs(priv->input, ABS_MT_TRACKING_ID, 0);
				input_report_abs(priv->input, ABS_MT_TOUCH_MAJOR, 0);
				input_report_abs(priv->input, ABS_MT_POSITION_X, xx[0]);
				input_report_abs(priv->input, ABS_MT_POSITION_Y, yy[0]);
				//input_report_key(priv->input, BTN_TOUCH, 0);		//Finger Up.
				input_mt_sync(priv->input);
				finger[0] = 0;
				TP_DEBUG("X1 = %d   Y1 = %d   UP   ...\n", xx[0],  yy[0]);
			}

			/* Only report a key once */
			if((pre_key != key_value)) {
				input_report_key(simulate_key_input, key_value, 1);
				key_down = true;
				pre_key = key_value;
				TP_DEBUG("Key Down = %d, \n", key_value);
			}
		}
		
		if( y[0] <= 480) { 
			input_report_abs(priv->input, ABS_MT_TRACKING_ID, 0);
			input_report_abs(priv->input, ABS_MT_TOUCH_MAJOR, 1);
			input_report_abs(priv->input, ABS_MT_POSITION_X, x[0]);
			input_report_abs(priv->input, ABS_MT_POSITION_Y, y[0]);
			//input_report_key(priv->input, BTN_TOUCH, 1);		//Finger Down.
			input_mt_sync(priv->input);
			TP_DEBUG("X1 = %d   Y1 = %d   Down ...\n", x[0], y[0]);
			xx[0] = x[0];
			yy[0] = y[0];
			finger[0] = 1;
		}
	} else {

		if(key_down) {
			input_report_key(simulate_key_input, pre_key, 0);
			key_down = false;
			input_sync(simulate_key_input);
			TP_DEBUG("Key UP = %d, \n", pre_key);
			key_value = 0;
			pre_key = 0;
		}
		if(finger[0]) {
			input_report_abs(priv->input, ABS_MT_TRACKING_ID, 0);
			input_report_abs(priv->input, ABS_MT_TOUCH_MAJOR, 0);
			input_report_abs(priv->input, ABS_MT_POSITION_X, xx[0]);
			input_report_abs(priv->input, ABS_MT_POSITION_Y, yy[0]);
			//input_report_key(priv->input, BTN_TOUCH, 0);		//Finger Up.
			input_mt_sync(priv->input);
			finger[0] = 0;
			TP_DEBUG("X1 = %d   Y1 = %d   UP   ...\n", xx[0],  yy[0]);
		}
		
	}
		
	if (pdata->status2) {
		
		/*enable the second finger that touch the key area */
		if(finger[0]) {
		/* If new key or screen ,Realse pre_key*/
			key_value = in_key_area(x[1], y[1]);
			if (( key_value != pre_key) && key_down){ 
				key_down = false;
				input_report_key(simulate_key_input, pre_key, 0);
				input_sync(simulate_key_input);
				TP_DEBUG("Key Up = %d, \n", pre_key);
				pre_key = 0;
				//goto out;
			}	
		
			if(key_value != 0) {
				 /* if touch key_area, release screen*/
				if(finger[0]) {           
					input_report_abs(priv->input, ABS_MT_TRACKING_ID, 0);
					input_report_abs(priv->input, ABS_MT_TOUCH_MAJOR, 0);
					input_report_abs(priv->input, ABS_MT_POSITION_X, xx[0]);
					input_report_abs(priv->input, ABS_MT_POSITION_Y, yy[0]);
					//input_report_key(priv->input, BTN_TOUCH, 0);		//Finger Up.
					input_mt_sync(priv->input);
					finger[0] = 0;
					TP_DEBUG("X1 = %d   Y1 = %d   UP   ...\n", xx[0],  yy[0]);
				}

				/* Only report a key once */
				if((pre_key != key_value)) {
					input_report_key(simulate_key_input, key_value, 1);
					key_down = true;
					pre_key = key_value;
					TP_DEBUG("Key Down = %d, \n", key_value);
				}
			}
		}
		
		if( y[1] <= 480) {
			input_report_abs(priv->input, ABS_MT_TRACKING_ID, 1);
			input_report_abs(priv->input, ABS_MT_TOUCH_MAJOR, 1);
			input_report_abs(priv->input, ABS_MT_POSITION_X, x[1]);
			input_report_abs(priv->input, ABS_MT_POSITION_Y, y[1]);
			//input_report_key(priv->input, BTN_TOUCH, 1);		//Finger Down.
			input_mt_sync(priv->input);
			xx[1] = x[1];
			yy[1] = y[1];
			finger[1] = 1;
			TP_DEBUG("X2 = %d   Y2 = %d   Down ...\n", x[1], y[1]);
		}
	} else {

		/*enable the second finger that touch the key area */
		if(finger[0]) {
			if(key_down) {
				input_report_key(simulate_key_input, pre_key, 0);
				key_down = false;
				input_sync(simulate_key_input);
				TP_DEBUG("Key UP = %d, \n", pre_key);
				key_value = 0;
				pre_key = 0;
			}
		}
		
		if(finger[1]) {
			input_report_abs(priv->input, ABS_MT_TRACKING_ID, 1);
			input_report_abs(priv->input, ABS_MT_TOUCH_MAJOR, 0);
			input_report_abs(priv->input, ABS_MT_POSITION_X, xx[1]);
			input_report_abs(priv->input, ABS_MT_POSITION_Y, yy[1]);
			//input_report_key(priv->input, BTN_TOUCH, 0);		//Finger Up.
			input_mt_sync(priv->input);
			finger[1] = 0;
			TP_DEBUG("X2 = %d   Y2 = %d   UP   ...\n", xx[1],  yy[1]);
		}
	}

	input_sync(priv->input);

out:
	mutex_unlock(&priv->mutex);
}

static void ilitek_ts_work(struct work_struct *work)
{
	struct ilitek_ts_priv *priv =
		container_of(work, struct ilitek_ts_priv, work);
	
	ilitek_report_ponit(priv);
	enable_irq(priv->irq); 
}

#ifndef USE_POLL
static irqreturn_t ilitek_ts_isr(int irq, void *dev_data)
{
	struct ilitek_ts_priv *priv = dev_data;
	disable_irq_nosync(priv->irq);
	schedule_work(&priv->work);

	return IRQ_HANDLED;
}

#else
static enum hrtimer_restart ilitek_ts_timer_func(struct hrtimer *timer)
{
	struct ilitek_ts_priv *priv =
		 container_of(timer, struct ilitek_ts_priv, timer);
	
	schedule_work(&priv->work);
	hrtimer_start(&priv->timer, ktime_set(0, 400000000), HRTIMER_MODE_REL);//400ms
	
	return HRTIMER_NORESTART;
}
#endif

static int ilitek_ts_open(struct input_dev *dev)
{
	struct ilitek_ts_priv *priv = input_get_drvdata(dev);

	enable_irq(priv->irq);

	return 0;
}

static void ilitek_ts_close(struct input_dev *dev)
{
	struct ilitek_ts_priv *priv = input_get_drvdata(dev);

	disable_irq(priv->irq);
	//cancel_work_sync(&priv->work);
}

void ilitek_tp_Reset_sleep(void)
{
	gpio_set_value(RESET_GPIO, 1);	
	usleep(20);
	gpio_set_value(RESET_GPIO, 0);
	msleep(20);
	gpio_set_value(RESET_GPIO, 1);
	msleep(200);
}

void ilitek_tp_Reset_delay(void)
{
	gpio_set_value(RESET_GPIO, 1);	
	udelay(20);
	gpio_set_value(RESET_GPIO, 0);
	mdelay(20);
	gpio_set_value(RESET_GPIO, 1);
	mdelay(200);
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void ilitek_ts_early_suspend(struct early_suspend *h)
{
	struct ilitek_ts_priv *priv = container_of(h,
                 struct ilitek_ts_priv, early_suspend);
	char buf[2];
	int ret;
	
	pr_info("ilitek_ts_early_suspend Entering\n");
	disable_irq(priv->irq);
	
	buf[0] = 0x30;	
//	buf[1] = 0x2;
	if ((ret = i2c_master_send(priv->client, buf, 1)) != 1)
		return;

	mdelay(10);
}

static void ilitek_ts_late_resume(struct early_suspend *h)
{
	struct ilitek_ts_priv *priv = container_of(h,
                struct ilitek_ts_priv, early_suspend);
	
	pr_info("ilitek_ts_late_resume Entering\n");

	ilitek_tp_Reset_sleep(); 
	enable_irq(priv->irq);
}
#endif

static int __devinit ilitek_ts_probe(struct i2c_client *client, const struct i2c_device_id *idp)
{
	struct ilitek_ts_priv *priv;
	struct input_dev *input;
	int err = -ENOMEM;
	u32 ver, rev;
	
	gpio_request(INT_GPIO, "interrupt");
	gpio_request(RESET_GPIO, "reset");
	gpio_direction_input(INT_GPIO);
	gpio_direction_output(RESET_GPIO, 1);
	gpio_set_value(INT_GPIO, 1);
	
	mdelay(10);
	
	ilitek_tp_Reset_delay();
	priv = (struct ilitek_ts_priv *)kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dev_err(&client->dev, "Failed to allocate driver data\n");
		goto err0;
	}
	
	mutex_init(&priv->mutex);
	
	INIT_WORK(&priv->work, ilitek_ts_work);
	input = input_allocate_device();
	
	if (!input) {
		dev_err(&client->dev, "Failed to allocate input device.\n");
		goto err1;
	}

	if ((err = ilitek_ts_get_fw_version(client, &ver))) {
		dev_err(&client->dev, "Unable to get FW version!\n");
		goto err1;
	} else {
		printk("%s(%u): FW=40 version=%X\n", __FUNCTION__, __LINE__, ver);
	}

	if ((err = ilitek_ts_get_resolution(client, &rev))) {
		dev_err(&client->dev, "Unable to get resolution!\n");
		goto err1;
	} else {
		printk("%s(%u): Resolution=%X\n", __FUNCTION__, __LINE__, rev);
	//	priv->fw_revision = rev;
	}
	
	input->name = client->name;
	input->phys = "I2C";
	input->id.bustype = BUS_I2C;
	input->dev.parent = &client->dev;
	input->open = ilitek_ts_open;
	input->close = ilitek_ts_close;

	set_bit(EV_ABS, input->evbit);
	//set_bit(EV_SYN, input->evbit);
	//set_bit(BTN_TOUCH, input->keybit);

	input_set_abs_params(input, ABS_MT_TRACKING_ID, 0, 2, 0, 0);
	input_set_abs_params(input, ABS_MT_TOUCH_MAJOR, 0, 2, 0, 0);
	input_set_abs_params(input, ABS_MT_POSITION_X, 0, 320, 0, 0);
	input_set_abs_params(input, ABS_MT_POSITION_Y, 0, 480, 0, 0);

	err = input_register_device(input);
	if (err)
		goto err1; 

	//create simulate key input
	simulate_key_input = input_allocate_device();
	if (simulate_key_input == NULL) {
		printk("%s: not enough memory for input device\n", __func__);
		err = -ENOMEM;
		goto err1;
	}
	simulate_key_input->name = "7x27a_kp";
	__set_bit(EV_KEY, simulate_key_input->evbit);
	__set_bit(KEY_BACK, simulate_key_input->keybit);
	__set_bit(KEY_MENU, simulate_key_input->keybit);
	__set_bit(KEY_HOME, simulate_key_input->keybit);
	__set_bit(ST_KEY_SEARCH, simulate_key_input->keybit);
	err = input_register_device(simulate_key_input);
	if (err != 0) {
		printk("%s: failed to register input device\n", __func__);
		goto err1;
	}
	
	priv->client = client;
	priv->input = input;
	i2c_set_clientdata(client, priv);
	input_set_drvdata(input, priv);

	// ToDo
	// Check Version
	// Check Power 
	//ilitek_ts_internal_update(client);

	#ifndef USE_POLL
	priv->irq = MSM_GPIO_TO_INT(INT_GPIO); /* MSM_GPIO_TO_INT */
	pr_info("ilitek_ts_ gpio_irq = [%d]\n", priv->irq);
	err = request_irq(priv->irq, ilitek_ts_isr, IRQF_TRIGGER_LOW,
			  client->name, priv);
	if (err) {
		dev_err(&client->dev, "Unable to request touchscreen IRQ.\n");
		goto err2;
	}
	#else
	hrtimer_init(&priv->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	priv->timer.function = ilitek_ts_timer_func;
	hrtimer_start(&priv->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	#endif
	
	/* Disable irq. The irq will be enabled once the input device is opened. */
	disable_irq(priv->irq);
	device_init_wakeup(&client->dev, 0);

	#ifdef CONFIG_HAS_EARLYSUSPEND
	priv->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	priv->early_suspend.suspend = ilitek_ts_early_suspend;
	priv->early_suspend.resume = ilitek_ts_late_resume;
	register_early_suspend(&priv->early_suspend);
	#endif 
	pr_info("ilitek_ts driver is on##############################\n");
	return 0;
	
#ifndef USE_POLL
err2:
	input_unregister_device(input);
	input = NULL; /* so we dont try to free it below */
#endif
err1:
	input_free_device(input);
	i2c_set_clientdata(client, NULL);
	kfree(priv);
err0:
	return err;
}

static int __devexit ilitek_ts_remove(struct i2c_client *client)
{
	struct ilitek_ts_priv *priv = i2c_get_clientdata(client);

	free_irq(priv->irq, priv);
	input_unregister_device(priv->input);
	i2c_set_clientdata(client, NULL);
	kfree(priv);

	return 0;
}

static const struct i2c_device_id ilitek_ts_id[] = {
	{ "ilitek_ts", 0x41 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ilitek_ts_id);

static struct i2c_driver ilitek_ts_driver = {
	.driver = {
		.name = "ilitek_ts",
	},
	.probe = ilitek_ts_probe,
	.remove = __devexit_p(ilitek_ts_remove),
	//.suspend = ilitek_ts_suspend,
	//.resume = ilitek_ts_resume,
	.id_table = ilitek_ts_id,
};

static int __init ilitek_ts_init(void)
{
	int ret;
	gpio_request(FLAG_GPIO, "flag_ilitp");
	gpio_direction_input(FLAG_GPIO);
	udelay(10);
	ret = gpio_get_value(FLAG_GPIO);
	printk("flag_gpio ilitek init ret=%d\n", ret);
	if (ret == 0)
	{
		udelay(10);
		ret = gpio_get_value(FLAG_GPIO);
		printk("flag_gpio ilitek init ret=%d\n", ret);
		if (ret == 0)
		{
			gpio_free(FLAG_GPIO);
			return i2c_add_driver(&ilitek_ts_driver);
		}
		else
		{
			gpio_free(FLAG_GPIO);
			return -1;
		} 
	}
	else
	{
		gpio_free(FLAG_GPIO);
		return -1;
	} 
}

static void __exit ilitek_ts_exit(void)
{
	i2c_del_driver(&ilitek_ts_driver);
}

MODULE_DESCRIPTION("ilitek Touch Screen Driver");
MODULE_AUTHOR("jacky.gong");
MODULE_LICENSE("GPL");

module_init(ilitek_ts_init);
module_exit(ilitek_ts_exit);

