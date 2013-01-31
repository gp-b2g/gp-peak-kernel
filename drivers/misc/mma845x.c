/*
 *  mma845x.c - Linux kernel modules for 3-Axis Orientation/Motion
 *  Detection Sensor MMA8451/MMA8452/MMA8453
 *
 *  Copyright (C) 2010-2011 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/pm.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/input.h>

#define SENSORS_MMA_POSITION	4

#define MMA845X_I2C_ADDR	0x1D
#define MMA8451_ID			0x1A
#define MMA8452_ID			0x2A
#define MMA8453_ID			0x3A

#define MAX_DELAY			500
#define MIN_DELAY			8
#define MODE_CHANGE_DELAY_MS	100

#define MMA845X_BUF_SIZE		7
/* register enum for mma845x registers */
enum {
	MMA845X_STATUS 		= 0x00,
	MMA845X_OUT_X_MSB 	= 0x01,
	MMA845X_OUT_X_LSB 	= 0x02,
	MMA845X_OUT_Y_MSB		= 0x03,
	MMA845X_OUT_Y_LSB		= 0x04,
	MMA845X_OUT_Z_MSB		= 0x05,
	MMA845X_OUT_Z_LSB		= 0x06,

	MMA845X_F_SETUP 		= 0x09,
	MMA845X_TRIG_CFG		= 0x0A,
	MMA845X_SYSMOD		= 0x0B,
	MMA845X_INT_SOURCE	= 0x0C,
	MMA845X_WHO_AM_I		= 0x0D,
	MMA845X_XYZ_DATA_CFG	= 0x0E,
	MMA845X_HP_FILTER_CUTOFF	= 0x0F,

	MMA845X_PL_STATUS		= 0x10,
	MMA845X_PL_CFG		= 0x11,
	MMA845X_PL_COUNT		= 0x12,
	MMA845X_PL_BF_ZCOMP	= 0x13,
	MMA845X_P_L_THS_REG	= 0x14,

	MMA845X_FF_MT_CFG		= 0x15,
	MMA845X_FF_MT_SRC		= 0x16,
	MMA845X_FF_MT_THS		= 0x17,
	MMA845X_FF_MT_COUNT	= 0x18,

	MMA845X_TRANSIENT_CFG = 0x1D,
	MMA845X_TRANSIENT_SRC	= 0x1E,
	MMA845X_TRANSIENT_THS	= 0x1F,
	MMA845X_TRANSIENT_COUNT	= 0x20,

	MMA845X_PULSE_CFG		= 0x21,
	MMA845X_PULSE_SRC		= 0x22,
	MMA845X_PULSE_THSX	= 0x23,
	MMA845X_PULSE_THSY	= 0x24,
	MMA845X_PULSE_THSZ	= 0x25,
	MMA845X_PULSE_TMLT	= 0x26,
	MMA845X_PULSE_LTCY	= 0x27,
	MMA845X_PULSE_WIND	= 0x28,

	MMA845X_ASLP_COUNT	= 0x29,
	MMA845X_CTRL_REG1		= 0x2A,
	MMA845X_CTRL_REG2		= 0x2B,
	MMA845X_CTRL_REG3		= 0x2C,
	MMA845X_CTRL_REG4		= 0x2D,
	MMA845X_CTRL_REG5		= 0x2E,

	MMA845X_OFF_X			= 0x2F,
	MMA845X_OFF_Y			= 0x30,
	MMA845X_OFF_Z			= 0x31,

	MMA845X_REG_END,
};

/* The sensitivity is represented in counts/g. In 2g mode the
sensitivity is 1024 counts/g. In 4g mode the sensitivity is 512
counts/g and in 8g mode the sensitivity is 256 counts/g.
 */
enum {
	MODE_2G = 0,
	MODE_4G,
	MODE_8G,
};

enum {
	MMA_STANDBY = 0,
	MMA_ACTIVED,
};
struct mma845x_data_axis{
	short x;
	short y;
	short z;
};
struct mma845x_data{
	struct i2c_client * client;
	struct workqueue_struct *mma845x_wq;
	struct input_dev *input_dev;
	struct hrtimer timer;
	struct work_struct  work;
	unsigned int delay; 
	struct mutex data_lock;
	int active;
	int position;
	u8 chip_id;
	int mode;
};
static char * mma845x_names[] ={
   "mma8451",
   "mma8452",
   "mma8453",
};
#if 0
static int mma845x_position_setting[8][3][3] =
{
   {{ 0, -1,  0}, { 1,  0,	0}, {0, 0,	1}},
   {{-1,  0,  0}, { 0, -1,	0}, {0, 0,	1}},
   {{ 0,  1,  0}, {-1,  0,	0}, {0, 0,	1}},
   {{ 1,  0,  0}, { 0,  1,	0}, {0, 0,	1}},
   
   {{ 0, -1,  0}, {-1,  0,	0}, {0, 0,  -1}},
   {{-1,  0,  0}, { 0,  1,	0}, {0, 0,  -1}},
   {{ 0,  1,  0}, { 1,  0,	0}, {0, 0,  -1}},
   {{ 1,  0,  0}, { 0, -1,	0}, {0, 0,  -1}},
};

static int mma845x_data_convert(struct mma845x_data* pdata,struct mma845x_data_axis *axis_data)
{
   short rawdata[3],data[3];
   int i,j;
   int position = pdata->position ;
   if(position < 0 || position > 7 )
   		position = 0;
   rawdata [0] = axis_data->x ; rawdata [1] = axis_data->y ; rawdata [2] = axis_data->z ;  
   for(i = 0; i < 3 ; i++)
   {
   	data[i] = 0;
   	for(j = 0; j < 3; j++)
		data[i] += rawdata[j] * mma845x_position_setting[position][i][j];
   }
   axis_data->x = data[0];
   axis_data->y = data[1];
   axis_data->z = data[2];
   return 0;
}
#endif
static char * mma845x_id2name(u8 id){
	int index = 0;
	if(id == MMA8451_ID)
		index = 0;
	else if(id == MMA8452_ID)
		index = 1;
	else if(id == MMA8453_ID)
		index = 2;
	return mma845x_names[index];
}
static int mma845x_device_init(struct i2c_client *client)
{
	int result;
	struct mma845x_data *pdata = i2c_get_clientdata(client);
	result = i2c_smbus_write_byte_data(client, MMA845X_CTRL_REG1, 0);
	if (result < 0)
		goto out;

	result = i2c_smbus_write_byte_data(client, MMA845X_XYZ_DATA_CFG,
					   pdata->mode);
	if (result < 0)
		goto out;
	pdata->active = MMA_STANDBY;
	msleep(MODE_CHANGE_DELAY_MS);
	return 0;
out:
	dev_err(&client->dev, "error when init mma845x:(%d)", result);
	return result;
}
static int mma845x_device_stop(struct i2c_client *client)
{
	u8 val;
	val = i2c_smbus_read_byte_data(client, MMA845X_CTRL_REG1);
	i2c_smbus_write_byte_data(client, MMA845X_CTRL_REG1,val & 0xfe);
	return 0;
}

static int mma845x_read_data(struct i2c_client *client,struct mma845x_data_axis *data)
{
	u8 tmp_data[MMA845X_BUF_SIZE];
	int ret;

	ret = i2c_smbus_read_i2c_block_data(client,
					    MMA845X_OUT_X_MSB, 7, tmp_data);
	if (ret < MMA845X_BUF_SIZE) {
		dev_err(&client->dev, "i2c block read failed\n");
		return -EIO;
	}
	data->x = ((tmp_data[0] << 8) & 0xff00) | tmp_data[1];
	data->y = ((tmp_data[2] << 8) & 0xff00) | tmp_data[3];
	data->z = ((tmp_data[4] << 8) & 0xff00) | tmp_data[5];

	//printk(KERN_EMERG "data.x[%d] data.y[%d] data.z[%d]\n", data->x, data->y, data->z);
	return 0;
}

static void mma845x_work_func(struct work_struct *work)
{
	struct mma845x_data *pdata = container_of(work, struct mma845x_data, work);
	struct mma845x_data_axis data;
	
	//mutex_lock(&pdata->data_lock);
	
	if (mma845x_read_data(pdata->client,&data) != 0)
		goto out;
	//mma845x_data_convert(pdata,&data);
	input_report_abs(pdata->input_dev, ABS_X, data.x);
	input_report_abs(pdata->input_dev, ABS_Y, data.y);
	input_report_abs(pdata->input_dev, ABS_Z, data.z);
	input_sync(pdata->input_dev);

out:
	//mutex_unlock(&pdata->data_lock);
	return;
}
static enum hrtimer_restart mma845x_timer_func(struct hrtimer *timer)
{
	struct mma845x_data *pdata = container_of(timer, struct mma845x_data, timer);

	queue_work(pdata->mma845x_wq, &pdata->work);
	hrtimer_start(&pdata->timer, ktime_set(0, pdata->delay*1000000), HRTIMER_MODE_REL);

	return HRTIMER_NORESTART;
}

static ssize_t mma845x_enable_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct input_dev *input_dev = to_input_dev(dev);
	struct mma845x_data *pdata = input_get_drvdata(input_dev);
	struct i2c_client *client = pdata->client;

	u8 val;
	int enable;
	
	mutex_lock(&pdata->data_lock);
	val = i2c_smbus_read_byte_data(client, MMA845X_CTRL_REG1);  
	if((val & 0x01) && pdata->active == MMA_ACTIVED)
		enable = 1;
	else
		enable = 0;
	mutex_unlock(&pdata->data_lock);
	
	return sprintf(buf, "%d\n", enable);
}

static ssize_t mma845x_enable_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct input_dev *input_dev = to_input_dev(dev);
	struct mma845x_data *pdata = input_get_drvdata(input_dev);
	struct i2c_client *client = pdata->client;
	
	int ret;
	unsigned long enable;
	u8 val = 0;
	
	enable = simple_strtoul(buf, NULL, 10);    
	mutex_lock(&pdata->data_lock);
	enable = (enable > 0) ? 1 : 0;
	if(enable && pdata->active == MMA_STANDBY){  
		val = i2c_smbus_read_byte_data(client,MMA845X_CTRL_REG1);
		ret = i2c_smbus_write_byte_data(client, MMA845X_CTRL_REG1, val|0x01);  
		if(!ret){
			pdata->active = MMA_ACTIVED;
			hrtimer_start(&pdata->timer, ktime_set(0, pdata->delay*1000000), HRTIMER_MODE_REL);
			printk("mma enable setting active \n");
	    	}
	}else if(enable == 0  && pdata->active == MMA_ACTIVED){
		val = i2c_smbus_read_byte_data(client,MMA845X_CTRL_REG1);
		ret = i2c_smbus_write_byte_data(client, MMA845X_CTRL_REG1,val & 0xFE);
		if(!ret){
			hrtimer_cancel(&pdata->timer);
			pdata->active= MMA_STANDBY;
			printk("mma enable setting inactive \n");
		}
	}
	mutex_unlock(&pdata->data_lock);
	
	return count;
}

static ssize_t mma845x_delay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct input_dev *input_dev = to_input_dev(dev);
	struct mma845x_data *pdata = input_get_drvdata(input_dev);
	int delay;

	mutex_lock(&pdata->data_lock);
	delay = (int) pdata->delay;
	mutex_unlock(&pdata->data_lock);

	return sprintf(buf, "%d\n", pdata->delay);

}

static ssize_t mma845x_delay_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long delay;
	struct input_dev *input_dev = to_input_dev(dev);
	struct mma845x_data *pdata = input_get_drvdata(input_dev);

	delay = simple_strtoul(buf, NULL, 10);   
	if (delay > MAX_DELAY)
		delay = MAX_DELAY;
	if (delay < MIN_DELAY)
		delay = MIN_DELAY;

	mutex_lock(&pdata->data_lock);
	pdata->delay = (unsigned int) delay;
	mutex_unlock(&pdata->data_lock);
	
	return count;
}

static ssize_t mma845x_position_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct input_dev *input_dev = to_input_dev(dev);
	struct mma845x_data *pdata = input_get_drvdata(input_dev);
	int position = 0;
	
	mutex_lock(&pdata->data_lock);
	position = pdata->position ;
	mutex_unlock(&pdata->data_lock);
	
	return sprintf(buf, "%d\n", position);
}

static ssize_t mma845x_position_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct input_dev *input_dev = to_input_dev(dev);
	struct mma845x_data *pdata = input_get_drvdata(input_dev);
	int  position;
	
	position = simple_strtoul(buf, NULL, 10);    
	mutex_lock(&pdata->data_lock);
	pdata->position = position;
	mutex_unlock(&pdata->data_lock);
	
	return count;
}

static ssize_t mma845x_mode_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct input_dev *input_dev = to_input_dev(dev);
	struct mma845x_data *pdata = input_get_drvdata(input_dev);
	int mode = 0;
	
	mutex_lock(&pdata->data_lock);
	mode = pdata->mode ;
	mutex_unlock(&pdata->data_lock);
	
	return sprintf(buf, "%d\n", mode);
}
#if 0
static ssize_t mma845x_mode_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct input_dev *input_dev = to_input_dev(dev);
	struct mma845x_data *pdata = input_get_drvdata(input_dev);
	int  mode;
	
	mode = simple_strtoul(buf, NULL, 10);    
	mutex_lock(&pdata->data_lock);
	pdata->mode = mode;
	i2c_smbus_write_byte_data(pdata->client, MMA845X_XYZ_DATA_CFG,
					   pdata->mode);
	mutex_unlock(&pdata->data_lock);
	
	return count;
}
#endif
static DEVICE_ATTR(enable_device, S_IRUGO|S_IWUSR|S_IWGRP,
		   mma845x_enable_show, mma845x_enable_store);
static DEVICE_ATTR(pollrate_ms, S_IRUGO|S_IWUSR|S_IWGRP,
		   mma845x_delay_show, mma845x_delay_store);
static DEVICE_ATTR(position, S_IRUGO|S_IWUSR|S_IWGRP,
		   mma845x_position_show, mma845x_position_store);
static DEVICE_ATTR(mode, S_IRUGO|S_IWUSR|S_IWGRP,
		   mma845x_mode_show, NULL);

static struct attribute *mma845x_attributes[] = {
	&dev_attr_enable_device.attr,
	&dev_attr_pollrate_ms.attr,
	&dev_attr_position.attr,
	&dev_attr_mode.attr,
	NULL
};

static const struct attribute_group mma845x_attr_group = {
	.attrs = mma845x_attributes,
};

static int __devinit mma845x_probe(struct i2c_client *client,
				   const struct i2c_device_id *id)
{
	int result, chip_id;
	struct mma845x_data *pdata;
	struct i2c_adapter *adapter;

	adapter = to_i2c_adapter(client->dev.parent);
	result = i2c_check_functionality(adapter,
					 I2C_FUNC_SMBUS_BYTE |
					 I2C_FUNC_SMBUS_BYTE_DATA);
	if (!result)
		goto err_out;
	
	/*read MMA845X chip id*/
	chip_id = i2c_smbus_read_byte_data(client, MMA845X_WHO_AM_I);
	if (chip_id != MMA8451_ID && chip_id != MMA8452_ID && chip_id != MMA8453_ID  ) {
		dev_err(&client->dev,
			"read chip ID 0x%x is not equal to 0x%x , 0x%x , 0x%x!\n",
			result, MMA8451_ID, MMA8452_ID, MMA8453_ID);
		result = -EINVAL;
		goto err_out;
	}else{
		printk("mma845x read chip id [%x]\n", chip_id);
	}
	
	pdata = kzalloc(sizeof(struct mma845x_data), GFP_KERNEL);
	if(!pdata){
		result = -ENOMEM;
		dev_err(&client->dev, "alloc data memory error!\n");
		goto err_out;
	}
	
	/* Initialize the MMA845X chip */
	pdata->client = client;
	pdata->chip_id = chip_id;
	pdata->mode = MODE_2G;
	pdata->delay = 100;		// 100ms
	pdata->position = SENSORS_MMA_POSITION;
	mutex_init(&pdata->data_lock);
	i2c_set_clientdata(client,pdata);
	mma845x_device_init(client);

	pdata->input_dev = input_allocate_device();
	if (!pdata->input_dev) {
		printk("input_allocate_device fail\n");
		goto err_alloc_input_device;
	}
	pdata->input_dev->name = "mma845x";
	pdata->input_dev->id.bustype = BUS_I2C;
	pdata->input_dev->uniq = mma845x_id2name(pdata->chip_id);
	set_bit(EV_ABS, pdata->input_dev->evbit);
	input_set_abs_params(pdata->input_dev, ABS_X, -0x7fff, 0x7fff, 0, 0);
	input_set_abs_params(pdata->input_dev, ABS_Y, -0x7fff, 0x7fff, 0, 0);
	input_set_abs_params(pdata->input_dev, ABS_Z, -0x7fff, 0x7fff, 0, 0);
	if (input_register_device(pdata->input_dev)) {
		goto err_register_input_device;
	}
	input_set_drvdata(pdata->input_dev, pdata);

	pdata->mma845x_wq = create_singlethread_workqueue("mma845x_wq");
	if (!pdata->mma845x_wq )
	{
		printk("create_singlethread_workqueue fail\n");
		goto err_create_singlethread_workqueue;
	}
	INIT_WORK(&pdata->work, mma845x_work_func);
	hrtimer_init(&pdata->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	pdata->timer.function = mma845x_timer_func;

	result = sysfs_create_group(&pdata->input_dev->dev.kobj, &mma845x_attr_group);
	if (result) {
		dev_err(&client->dev, "create sysfs failed!\n");
		result = -EINVAL;
		goto err_create_sysfs;
	}

	printk("mma845x device driver probe successfully\n");
	return 0;
err_create_sysfs:
	destroy_workqueue(pdata->mma845x_wq);
err_create_singlethread_workqueue:
	input_unregister_device(pdata->input_dev);
err_register_input_device:
	input_free_device(pdata->input_dev);
err_alloc_input_device:
	kfree(pdata);
err_out:
	printk("mma845x device driver probe fail\n");
	return result;
}
static int __devexit mma845x_remove(struct i2c_client *client)
{
	struct mma845x_data *pdata = i2c_get_clientdata(client);
	
	mma845x_device_stop(client);	
	if(pdata){
		sysfs_remove_group(&pdata->input_dev->dev.kobj, &mma845x_attr_group);
		destroy_workqueue(pdata->mma845x_wq);
		input_unregister_device(pdata->input_dev);
		input_free_device(pdata->input_dev);
		kfree(pdata);
	}
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int mma845x_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct mma845x_data *pdata = i2c_get_clientdata(client);

	if(pdata->active == MMA_ACTIVED)
		hrtimer_cancel(&pdata->timer);
		mma845x_device_stop(client);
	return 0;
}

static int mma845x_resume(struct i2c_client *client)
{
	int val = 0;
	struct mma845x_data *pdata = i2c_get_clientdata(client);

	if(pdata->active == MMA_ACTIVED){
		val = i2c_smbus_read_byte_data(client,MMA845X_CTRL_REG1);
		i2c_smbus_write_byte_data(client, MMA845X_CTRL_REG1, val|0x01);  
		hrtimer_start(&pdata->timer, ktime_set(0, pdata->delay*1000000), HRTIMER_MODE_REL);
	}
	return 0;  
}
#endif

static const struct i2c_device_id mma845x_id[] = {
	{"mma845x", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, mma845x_id);

static struct i2c_driver mma845x_driver = {
	.driver = {
		   .name = "mma845x",
		   .owner = THIS_MODULE,
		   },
	.suspend	= mma845x_suspend,
	.resume	= mma845x_resume,
	.probe	= mma845x_probe,
	.remove	= __devexit_p(mma845x_remove),
	.id_table	= mma845x_id,
};

static int __init mma845x_init(void)
{
	int res;

	res = i2c_add_driver(&mma845x_driver);
	if (res < 0) {
		printk(KERN_INFO "add mma845x i2c driver failed\n");
		return -ENODEV;
	}
	return res;
}

static void __exit mma845x_exit(void)
{
	i2c_del_driver(&mma845x_driver);
}

/*modify by otis, use hrtimer instead of polling mode*/
MODULE_AUTHOR("Cellon, Inc.");
MODULE_DESCRIPTION("MMA845X 3-Axis Orientation/Motion Detection Sensor driver");
MODULE_LICENSE("GPL");

module_init(mma845x_init);
module_exit(mma845x_exit);
