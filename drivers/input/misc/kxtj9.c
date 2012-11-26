/* drivers/input/misc/kxtj9.c - KXTJ9 accelerometer driver
 *
 * Copyright (C) 2012 Kionix, Inc.
 * Written by Kuching Tan <kuchingtan@kionix.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input/kxtj9.h>
#include <linux/version.h>

#define NAME			"kxtik"
#define G_MAX			8000
/* OUTPUT REGISTERS */
#define XOUT_L			0x06
#define WHO_AM_I		0x0F
/* CONTROL REGISTERS */
#define INT_REL			0x1A
#define CTRL_REG1		0x1B
#define INT_CTRL1		0x1E
#define DATA_CTRL		0x21
/* CONTROL REGISTER 1 BITS */
#define PC1_OFF			0x7F
#define PC1_ON			(1 << 7)
/* INPUT_ABS CONSTANTS */
#define FUZZ			3
#define FLAT			3
/* RESUME STATE INDICES */
#define RES_DATA_CTRL		0
#define RES_CTRL_REG1		1
#define RES_INT_CTRL1		2
#define RESUME_ENTRIES		3

/*
 * The following table lists the maximum appropriate poll interval for each
 * available output data rate.
 */
static const struct {
	unsigned int cutoff;
	u8 mask;
} kxtj9_odr_table[] = {
	{ 3,	ODR800F },
	{ 5,	ODR400F },
	{ 10,	ODR200F },
	{ 20,	ODR100F },
	{ 40,	ODR50F  },
	{ 80,	ODR25F  },
	{ 0,	ODR12_5F},
};

struct kxtj9_data {
	struct i2c_client *client;
	struct kxtj9_platform_data pdata;
	struct input_dev *input_dev;
	struct delayed_work poll_work;
	struct workqueue_struct *workqueue;
	unsigned int poll_interval;
	unsigned int poll_delay;
	u8 shift;
	u8 ctrl_reg1;
	u8 data_ctrl;
};

static int kxtj9_i2c_read(struct kxtj9_data *tj9, u8 addr, u8 *data, int len)
{
	struct i2c_msg msgs[] = {
		{
			.addr = tj9->client->addr,
			.flags = tj9->client->flags,
			.len = 1,
			.buf = &addr,
		},
		{
			.addr = tj9->client->addr,
			.flags = tj9->client->flags | I2C_M_RD,
			.len = len,
			.buf = data,
		},
	};

	return i2c_transfer(tj9->client->adapter, msgs, 2);
}

static void kxtj9_report_acceleration_data(struct kxtj9_data *tj9)
{
	s16 acc_data[3]; /* Data bytes from hardware xL, xH, yL, yH, zL, zH */
	s16 x, y, z;
	int err;
	struct input_dev *input_dev = tj9->input_dev;

	mutex_lock(&input_dev->mutex);
	err = kxtj9_i2c_read(tj9, XOUT_L, (u8 *)acc_data, 6);
	mutex_unlock(&input_dev->mutex);
	if (err < 0)
		dev_err(&tj9->client->dev, "accelerometer data read failed\n");

	x = ((s16) le16_to_cpu(acc_data[tj9->pdata.axis_map_x])) >> tj9->shift;
	y = ((s16) le16_to_cpu(acc_data[tj9->pdata.axis_map_y])) >> tj9->shift;
	z = ((s16) le16_to_cpu(acc_data[tj9->pdata.axis_map_z])) >> tj9->shift;

#if 0
	pr_info("Accel: x = %d, y = %d, z = %d\n", \
		 tj9->pdata.negate_x ? -x : x,  \
		 tj9->pdata.negate_y ? -y : y,  \
		 tj9->pdata.negate_z ? -z : z);
#endif

	input_report_abs(tj9->input_dev, ABS_X, tj9->pdata.negate_x ? -x : x);
	input_report_abs(tj9->input_dev, ABS_Y, tj9->pdata.negate_y ? -y : y);
	input_report_abs(tj9->input_dev, ABS_Z, tj9->pdata.negate_z ? -z : z);
	input_sync(tj9->input_dev);
}

static void kxtj9_poll_work(struct work_struct *work)
{
	struct kxtj9_data *tj9 = container_of((struct delayed_work *)work,	struct kxtj9_data, poll_work);

	queue_delayed_work(tj9->workqueue, &tj9->poll_work, tj9->poll_delay);

	kxtj9_report_acceleration_data(tj9);
}

static int kxtj9_update_g_range(struct kxtj9_data *tj9, u8 new_g_range)
{
	switch (new_g_range) {
	case KXTJ9_G_2G:
		tj9->shift = 4;
		break;
	case KXTJ9_G_4G:
		tj9->shift = 3;
		break;
	case KXTJ9_G_8G:
		tj9->shift = 2;
		break;
	default:
		return -EINVAL;
	}

	tj9->ctrl_reg1 &= 0xe7;
	tj9->ctrl_reg1 |= new_g_range;

	return 0;
}

static int kxtj9_update_odr(struct kxtj9_data *tj9, unsigned int poll_interval)
{
	int err;
	int i;

	/* Use the lowest ODR that can support the requested poll interval */
	for (i = 0; i < ARRAY_SIZE(kxtj9_odr_table); i++) {
		tj9->data_ctrl = kxtj9_odr_table[i].mask;
		if (poll_interval < kxtj9_odr_table[i].cutoff)
			break;
	}

	err = i2c_smbus_write_byte_data(tj9->client, CTRL_REG1, 0);
	if (err < 0)
		return err;

	err = i2c_smbus_write_byte_data(tj9->client, DATA_CTRL, tj9->data_ctrl);
	if (err < 0)
		return err;

	err = i2c_smbus_write_byte_data(tj9->client, CTRL_REG1, tj9->ctrl_reg1);
	if (err < 0)
		return err;

	return 0;
}

static int kxtj9_device_power_on(struct kxtj9_data *tj9)
{
	if (tj9->pdata.power_on)
		return tj9->pdata.power_on();

	return 0;
}

static void kxtj9_device_power_off(struct kxtj9_data *tj9)
{
	int err;

	tj9->ctrl_reg1 &= PC1_OFF;
	err = i2c_smbus_write_byte_data(tj9->client, CTRL_REG1, tj9->ctrl_reg1);
	if (err < 0)
		dev_err(&tj9->client->dev, "soft power off failed\n");

	if (tj9->pdata.power_off)
		tj9->pdata.power_off();
}

static int kxtj9_enable(struct kxtj9_data *tj9)
{
	int err;

	err = kxtj9_device_power_on(tj9);
	if (err < 0)
		return err;

	/* ensure that PC1 is cleared before updating control registers */
	err = i2c_smbus_write_byte_data(tj9->client, CTRL_REG1, 0);
	if (err < 0)
		return err;

	err = kxtj9_update_g_range(tj9, tj9->pdata.g_range);
	if (err < 0)
		return err;

	/* turn on outputs */
	tj9->ctrl_reg1 |= PC1_ON;
	err = i2c_smbus_write_byte_data(tj9->client, CTRL_REG1, tj9->ctrl_reg1);
	if (err < 0)
		return err;

	err = kxtj9_update_odr(tj9, tj9->poll_interval);
	if (err < 0)
		return err;

	queue_delayed_work(tj9->workqueue, &tj9->poll_work, 0);

	return 0;
}

static void kxtj9_disable(struct kxtj9_data *tj9)
{
	cancel_delayed_work_sync(&tj9->poll_work);

	kxtj9_device_power_off(tj9);
}

static int kxtj9_input_open(struct input_dev *input)
{
	struct kxtj9_data *tj9 = input_get_drvdata(input);

	return kxtj9_enable(tj9);
}

static void kxtj9_input_close(struct input_dev *dev)
{
	struct kxtj9_data *tj9 = input_get_drvdata(dev);

	kxtj9_disable(tj9);
}

static void __devinit kxtj9_init_input_device(struct kxtj9_data *tj9,
					      struct input_dev *input_dev)
{
	__set_bit(EV_ABS, input_dev->evbit);
	input_set_abs_params(input_dev, ABS_X, -G_MAX, G_MAX, FUZZ, FLAT);
	input_set_abs_params(input_dev, ABS_Y, -G_MAX, G_MAX, FUZZ, FLAT);
	input_set_abs_params(input_dev, ABS_Z, -G_MAX, G_MAX, FUZZ, FLAT);

	input_dev->name = "kxtik";
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &tj9->client->dev;
}

static int __devinit kxtj9_setup_input_device(struct kxtj9_data *tj9)
{
	struct input_dev *input_dev;
	int err;

	input_dev = input_allocate_device();
	if (!input_dev) {
		dev_err(&tj9->client->dev, "input device allocate failed\n");
		return -ENOMEM;
	}

	tj9->input_dev = input_dev;

	input_dev->open = kxtj9_input_open;
	input_dev->close = kxtj9_input_close;
	input_set_drvdata(input_dev, tj9);

	kxtj9_init_input_device(tj9, input_dev);

	err = input_register_device(tj9->input_dev);
	if (err) {
		dev_err(&tj9->client->dev,
			"unable to register input polled device %s: %d\n",
			tj9->input_dev->name, err);
		input_free_device(tj9->input_dev);
		return err;
	}

	return 0;
}

/*
 * When IRQ mode is selected, we need to provide an interface to allow the user
 * to change the output data rate of the part.  For consistency, we are using
 * the set_poll method, which accepts a poll interval in milliseconds, and then
 * calls update_odr() while passing this value as an argument.  In IRQ mode, the
 * data outputs will not be read AT the requested poll interval, rather, the
 * lowest ODR that can support the requested interval.  The client application
 * will be responsible for retrieving data from the input node at the desired
 * interval.
 */

/* Returns currently selected poll interval (in ms) */
static ssize_t kxtj9_get_poll(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct kxtj9_data *tj9 = i2c_get_clientdata(client);

	return sprintf(buf, "%d\n", tj9->poll_interval);
}

/* Allow users to select a new poll interval (in ms) */
static ssize_t kxtj9_set_poll(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct kxtj9_data *tj9 = i2c_get_clientdata(client);
	struct input_dev *input_dev = tj9->input_dev;
	unsigned int interval;

	#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35))
	int error;
	error = kstrtouint(buf, 10, &interval);
	if (error < 0)
		return error;
	#else
	interval = (unsigned int)simple_strtoul(buf, NULL, 10);
	#endif

	/* Lock the device to prevent races with open/close (and itself) */
	mutex_lock(&input_dev->mutex);

	/*
	 * Set current interval to the greater of the minimum interval or
	 * the requested interval
	 */
	tj9->poll_interval = max(interval, tj9->pdata.min_interval);
	tj9->poll_delay = msecs_to_jiffies(tj9->poll_interval);

	kxtj9_update_odr(tj9, tj9->poll_interval);

	mutex_unlock(&input_dev->mutex);

	return count;
}

/* Allow users to enable/disable the device */
static ssize_t kxtj9_set_enable(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct kxtj9_data *tj9 = i2c_get_clientdata(client);
	struct input_dev *input_dev = tj9->input_dev;
	unsigned int enable;

	#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35))
	int error;
	error = kstrtouint(buf, 10, &enable);
	if (error < 0)
		return error;
	#else
	enable = (unsigned int)simple_strtoul(buf, NULL, 10);
	#endif

	/* Lock the device to prevent races with open/close (and itself) */
	mutex_lock(&input_dev->mutex);

	if(enable)
		kxtj9_enable(tj9);
	else
		kxtj9_disable(tj9);

	mutex_unlock(&input_dev->mutex);

	return count;
}

static DEVICE_ATTR(poll, S_IRUGO|S_IWUSR, kxtj9_get_poll, kxtj9_set_poll);
static DEVICE_ATTR(enable,S_IRUGO | S_IWUGO, NULL, kxtj9_set_enable);

static struct attribute *kxtj9_attributes[] = {
	&dev_attr_poll.attr,
	&dev_attr_enable.attr,
	NULL
};

static struct attribute_group kxtj9_attribute_group = {
	.attrs = kxtj9_attributes
};

static int __devinit kxtj9_verify(struct kxtj9_data *tj9)
{
	int retval;

	retval = kxtj9_device_power_on(tj9);
	if (retval < 0)
		return retval;

	retval = i2c_smbus_read_byte_data(tj9->client, WHO_AM_I);
	if (retval < 0) {
		dev_err(&tj9->client->dev, "read err int source\n");
		goto out;
	}

	retval = retval != 0x05 ? -EIO : 0;

out:
	kxtj9_device_power_off(tj9);
	return retval;
}

static int __devinit kxtj9_probe(struct i2c_client *client,
				 const struct i2c_device_id *id)
{
	const struct kxtj9_platform_data *pdata = client->dev.platform_data;
	struct kxtj9_data *tj9;
	int err;

	if (!i2c_check_functionality(client->adapter,
				I2C_FUNC_I2C | I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev, "client is not i2c capable\n");
		return -ENXIO;
	}

	if (!pdata) {
		dev_err(&client->dev, "platform data is NULL; exiting\n");
		return -EINVAL;
	}

	tj9 = kzalloc(sizeof(*tj9), GFP_KERNEL);
	if (!tj9) {
		dev_err(&client->dev,
			"failed to allocate memory for module data\n");
		return -ENOMEM;
	}

	tj9->client = client;
	tj9->pdata = *pdata;

	if (pdata->init) {
		err = pdata->init();
		if (err < 0)
			goto err_free_mem;
	}

	err = kxtj9_verify(tj9);
	if (err < 0) {
		dev_err(&client->dev, "device not recognized\n");
		goto err_pdata_exit;
	}

	i2c_set_clientdata(client, tj9);

	err = kxtj9_setup_input_device(tj9);
	if (err)
		goto err_pdata_exit;

	tj9->ctrl_reg1 = tj9->pdata.res_12bit | tj9->pdata.g_range;
	tj9->poll_interval = tj9->pdata.poll_interval;
	tj9->poll_delay = msecs_to_jiffies(tj9->poll_interval);

	tj9->workqueue = create_workqueue("kxtj9 Workqueue");
	INIT_DELAYED_WORK(&tj9->poll_work, kxtj9_poll_work);

	err = sysfs_create_group(&client->dev.kobj, &kxtj9_attribute_group);
	if (err) {
		dev_err(&client->dev, "sysfs create failed: %d\n", err);
		goto err_destroy_input;
	}

	return 0;

err_destroy_input:
	input_unregister_device(tj9->input_dev);
	destroy_workqueue(tj9->workqueue);
err_pdata_exit:
	if (tj9->pdata.exit)
		tj9->pdata.exit();
err_free_mem:
	kfree(tj9);
	return err;
}

static int __devexit kxtj9_remove(struct i2c_client *client)
{
	struct kxtj9_data *tj9 = i2c_get_clientdata(client);

	sysfs_remove_group(&client->dev.kobj, &kxtj9_attribute_group);
	input_unregister_device(tj9->input_dev);
	destroy_workqueue(tj9->workqueue);

	if (tj9->pdata.exit)
		tj9->pdata.exit();

	kfree(tj9);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int kxtj9_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct kxtj9_data *tj9 = i2c_get_clientdata(client);
	struct input_dev *input_dev = tj9->input_dev;

	mutex_lock(&input_dev->mutex);

	if (input_dev->users)
		kxtj9_disable(tj9);

	mutex_unlock(&input_dev->mutex);
	return 0;
}

static int kxtj9_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct kxtj9_data *tj9 = i2c_get_clientdata(client);
	struct input_dev *input_dev = tj9->input_dev;
	int retval = 0;

	mutex_lock(&input_dev->mutex);

	if (input_dev->users)
		kxtj9_enable(tj9);

	mutex_unlock(&input_dev->mutex);
	return retval;
}
#endif

static SIMPLE_DEV_PM_OPS(kxtj9_pm_ops, kxtj9_suspend, kxtj9_resume);

static const struct i2c_device_id kxtj9_id[] = {
	{ NAME, 0 },
	{ },
};

MODULE_DEVICE_TABLE(i2c, kxtj9_id);

static struct i2c_driver kxtj9_driver = {
	.driver = {
		.name	= NAME,
		.owner	= THIS_MODULE,
		.pm	= &kxtj9_pm_ops,
	},
	.probe		= kxtj9_probe,
	.remove		= __devexit_p(kxtj9_remove),
	.id_table	= kxtj9_id,
};

static int __init kxtj9_init(void)
{
	return i2c_add_driver(&kxtj9_driver);
}
module_init(kxtj9_init);

static void __exit kxtj9_exit(void)
{
	i2c_del_driver(&kxtj9_driver);
}
module_exit(kxtj9_exit);

MODULE_DESCRIPTION("KXTJ9 accelerometer driver");
MODULE_AUTHOR("Kuching Tan <kuchingtan@kionix.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.6.1");
