/*==================================================
Project: C8681
Description: tps61310 driver
History: zhuangxiaojian 2012-08-31 create
==================================================*/
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/pwm.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <mach/pmic.h>
#include <mach/gpio.h>
#include <linux/err.h>
#include "tps61310.h"

static struct i2c_client *tps61310_client;

static const struct i2c_device_id tps61310_i2c_id[] = {
	{"tps61310", 0},
	{ }
};

static int32_t tps61310_i2c_txdata(unsigned short saddr,
		unsigned char *txdata, int length)
{
	struct i2c_msg msg[] = {
		{
			.addr = saddr,
			.flags = 0,
			.len = length,
			.buf = txdata,
		},
	};
	if (i2c_transfer(tps61310_client->adapter, msg, 1) < 0) {
		printk("tps61310_i2c_txdata faild 0x%x\n", saddr);
		return -EIO;
	}

	return 0;
}

static int tps61310_i2c_rxdata(unsigned short saddr,
    unsigned char *rxdata, int length)
{
	struct i2c_msg msgs[] = {
		{
			.addr   = saddr,
			.flags = 0,
			.len   = 1,
			.buf   = rxdata,
		},
		{
			.addr   = saddr,
			.flags = I2C_M_RD,
			.len   = length,
			.buf   = rxdata,
		},
	};

	if (i2c_transfer(tps61310_client->adapter, msgs, 2) < 0) {
		printk("##### tps61310_i2c_rxdata failed!\n");
		return -EIO;
	}

	return 0;
}

static int32_t tps61310_i2c_read_b(unsigned int raddr, unsigned int *rdata)
{
	int rc = 0;
	unsigned char buf[2];
	
	memset(buf, 0, sizeof(buf));

	buf[0] = raddr;

	rc = tps61310_i2c_rxdata(tps61310_client->addr, buf, 1);
	if (rc < 0) {
		printk("##### tps61310_i2c_read_byte failed!\n");
		return rc;
	}

	*rdata = buf[0];
    printk("tps61310 read: saddr:0x%x, raddr:0x%x, rdata:0x%x\r\n", tps61310_client->addr,raddr,*rdata);
	
	return rc;
}


static int32_t tps61310_i2c_write_b(uint8_t waddr, uint8_t bdata)
{
	int32_t rc = -EFAULT;
	unsigned char buf[2];
	if (!tps61310_client) {
		printk("##### tps61310_client == NULL\n");
		return  -ENOTSUPP;
		}

	memset(buf, 0, sizeof(buf));
	buf[0] = waddr;
	buf[1] = bdata;
	printk("##### %s line%d: tps61310_client->addr == 0x%x\n",__func__,__LINE__,tps61310_client->addr);
	rc = tps61310_i2c_txdata(tps61310_client->addr, buf, 2);
	if (rc < 0) {
		printk("i2c_write_b failed, addr = 0x%x, val = 0x%x!\n",
				waddr, bdata);
		return rc;
	}

	printk("tps61310 write: saddr:0x%x, waddr:0x%x, bdata:0x%x\r\n", tps61310_client->addr,waddr,bdata);
	
	return rc;
}


int tps61310_config(unsigned led_state)
{
	int rc = 0;
	//unsigned int reg_value;
	
	gpio_request(GPIO_CAM_FLASH_NOW, "tps61310_now");
	gpio_request(GPIO_CAM_FLASH_CTRL_EN, "tps61310_en");
	
	switch(led_state) {
		case MSM_LED_STATE_OFF:
			printk("##### %s line%d: MSM_LED_STATE_OFF\n",__func__,__LINE__);
			gpio_direction_output(GPIO_CAM_FLASH_CTRL_EN, 0);
			tps61310_i2c_write_b(0x01, 0x00);
			break;
			
		case MSM_LED_STATE_LOW:
			printk("##### %s line%d: MSM_LED_STATE_LOW\n",__func__,__LINE__);
			gpio_direction_output(GPIO_CAM_FLASH_CTRL_EN, 1);
			gpio_direction_output(GPIO_CAM_FLASH_NOW, 1);
			tps61310_i2c_write_b(0x00, 0x04);
			tps61310_i2c_write_b(0x02, 0x40);
			tps61310_i2c_write_b(0x04, 0x10);
			tps61310_i2c_write_b(0x05, 0x6a);
			break;
			
		case MSM_LED_STATE_HIGH:
			printk("##### %s line%d: MSM_LED_STATE_HIGH\n",__func__,__LINE__);
			gpio_direction_output(GPIO_CAM_FLASH_CTRL_EN, 1);
			gpio_direction_output(GPIO_CAM_FLASH_NOW, 0);
			tps61310_i2c_write_b(0x01, 0x94);
			tps61310_i2c_write_b(0x02, 0x80);
			tps61310_i2c_write_b(0x04, 0x10);
			tps61310_i2c_write_b(0x05, 0x6a);

			break;
		default:
			break;
	}
	return rc;
}


void tps61310_register_reset(void)
{
	int reg_value = 0;
	
	tps61310_i2c_read_b(0x00, &reg_value);
	reg_value |= (1 << 7);
	tps61310_i2c_write_b(0x00, reg_value);

	usleep(50);
	tps61310_i2c_read_b(0x00, &reg_value);
	reg_value &= ~(1 << 7);
	tps61310_i2c_write_b(0x00, reg_value);
	return;
}

static int tps61310_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int rc = 0;
	int chipid = 0;
	
	printk("tps61310_probe called!\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("i2c_check_functionality failed\n");
		goto probe_failure;
	}

	tps61310_client = client;

	//reset register
	tps61310_register_reset();
	//end
	
	rc = tps61310_i2c_read_b(0x00, &chipid);
	if (rc < 0) 
		goto probe_failure;
	if (chipid != 0x0a) {
		printk("##### chip id error: chipid == 0x%x\n",chipid);
		goto probe_failure;
	}

	gpio_tlmm_config(GPIO_CFG(GPIO_CAM_FLASH_NOW, 0, GPIO_CFG_OUTPUT,
                   GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
    gpio_tlmm_config(GPIO_CFG(GPIO_CAM_FLASH_CTRL_EN, 0, GPIO_CFG_OUTPUT,
    				GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);

	rc = gpio_request(GPIO_CAM_FLASH_NOW, "TPS61310_EN");
	if (!rc) {
		gpio_direction_output(GPIO_CAM_FLASH_NOW, 0);
	}else{
		goto probe_failure;
	}
	rc = gpio_request(GPIO_CAM_FLASH_CTRL_EN, "TPS61310_NOW");
	if (!rc) {
		gpio_direction_output(GPIO_CAM_FLASH_CTRL_EN, 0);
	}else{
		goto gpio_requst_fail;
	}
	
	gpio_set_value_cansleep(GPIO_CAM_FLASH_CTRL_EN, 0);
	
	printk("tps61310_probe successed! rc = %d\n", rc);
	return 0;

gpio_requst_fail:
	gpio_free(GPIO_CAM_FLASH_NOW);
probe_failure:
	pr_err("tps61310_probe failed! rc = %d\n", rc);

	return rc;
}

#if 0
static void tps61310_i2c_remove(void)
{
	gpio_direction_output(GPIO_CAM_FLASH_CTRL_EN, 0);
	gpio_direction_output(GPIO_CAM_FLASH_NOW, 0);

	gpio_free(GPIO_CAM_FLASH_CTRL_EN);
	gpio_free(GPIO_CAM_FLASH_NOW);

	return;
}
#endif

static struct i2c_driver tps61310_i2c_driver = {
	.id_table = tps61310_i2c_id,
	.probe  = tps61310_i2c_probe,
	.remove = __exit_p(tps61310_i2c_remove),
	.driver = {
		.name = "tps61310",
	},
};

static int __init tps61310_init(void)
{
	return i2c_add_driver(&tps61310_i2c_driver);
}

static void __exit tps61310_exit(void)
{
	i2c_del_driver(&tps61310_i2c_driver);
}

module_init(tps61310_init);
module_exit(tps61310_exit);

MODULE_DESCRIPTION("C8680 flash driver");
MODULE_LICENSE("GPL v2");

