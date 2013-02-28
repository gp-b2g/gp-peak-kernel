/* Copyright (c) 2010-2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define DEBUG

#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/leds.h>
#include <linux/slab.h>
#include <media/msm_camera.h>
#include <mach/gpio.h>
#include <mach/camera.h>
#include "ov5640.h"

/* Debug switch */
#ifdef CDBG
#undef CDBG
#endif
#ifdef CDBG_HIGH
#undef CDBG_HIGH
#endif

//#define OV5640_VERBOSE_DGB

#ifdef OV5640_VERBOSE_DGB
#define CDBG(fmt, args...) pr_debug(fmt, ##args)
#define CDBG_HIGH(fmt, args...) pr_debug(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#define CDBG_HIGH(fmt, args...) pr_debug(fmt, ##args)
#endif


#define FALSE 0
#define TRUE 1

#define AVG_LUM_THRESH 5

static int ov5640_pwdn_gpio;
static int ov5640_reset_gpio;
static int ov5640_driver_pwdn_gpio;


static int OV5640_CSI_CONFIG = 0;

struct ov5640_work {
	struct work_struct work;
};
static struct ov5640_work *ov5640_sensorw;
static struct i2c_client    *ov5640_client;
static DECLARE_WAIT_QUEUE_HEAD(ov5640_wait_queue);
DEFINE_MUTEX(ov5640_mutex);
static u8 ov5640_i2c_buf[4];
static u8 ov5640_counter = 0;
static int16_t ov5640_effect = CAMERA_EFFECT_OFF;
static int is_autoflash = 0;
static int is_autoflash_working = 0;

static int effect_value = 0;
static unsigned int SAT_U = 0x40;
static unsigned int SAT_V = 0x40;

struct __ov5640_ctrl
{
	const struct msm_camera_sensor_info *sensordata;
	int sensormode;
	uint fps_divider; /* init to 1 * 0x00000400 */
	uint pict_fps_divider; /* init to 1 * 0x00000400 */
	u16 curr_step_pos;
	u16 curr_lens_pos;
	u16 init_curr_lens_pos;
	u16 my_reg_gain;
	u16 my_reg_line_count;
	enum msm_s_resolution prev_res;
	enum msm_s_resolution pict_res;
	enum msm_s_resolution curr_res;
	enum msm_s_test_mode  set_test;
};
static struct __ov5640_ctrl *ov5640_ctrl;
static int afinit = 1;

extern struct rw_semaphore leds_list_lock;
extern struct list_head leds_list;

//const awb gain for RGB
static int reg3400val = 0x07;
static int reg3401val = 0x30;
static int reg3402val = 0x04;
static int reg3403val = 0x00;
static int reg3404val = 0x05;
static int reg3405val = 0x50;

static int current_avg_luminance = AVG_LUM_THRESH +1;
static int is_awb_enabled = 1;  //default auto
static int is_effect_auto = 1;  //default auto

static int ov5640_i2c_remove(struct i2c_client *client);
static int ov5640_i2c_probe(struct i2c_client *client,const struct i2c_device_id *id);

static int ov5640_i2c_txdata(u16 saddr,u8 *txdata,int length)
{
#if 1

	struct i2c_msg msg[] = {
		{
			.addr  = saddr,
			.flags = 0,
			.len = length,
			.buf = txdata,
		},
	};

	if (i2c_transfer(ov5640_client->adapter, msg, 1) < 0)
		return -EIO;
	else
		return 0;
#endif
//    return 0;
}

static int ov5640_i2c_write(unsigned short saddr, unsigned int waddr,
    unsigned short bdata,u8 trytimes)
{
	int rc = -EIO;
	ov5640_counter = 0;
	ov5640_i2c_buf[0] = (waddr & 0xFF00)>>8;
	ov5640_i2c_buf[1] = (waddr & 0x00FF);
	ov5640_i2c_buf[2] = (bdata & 0x00FF);

    CDBG("ov5640 write:saddr:0x%x, waddr:0x%x, data:0x%x\r\n",saddr,waddr,bdata);
	while ((ov5640_counter < trytimes) &&(rc != 0))
	{
		rc = ov5640_i2c_txdata(saddr, ov5640_i2c_buf, 3);

		if (rc < 0)
		{
			ov5640_counter++;
			CDBG_HIGH("***--CAMERA-- i2c_write_w failed,i2c addr=0x%x, command addr = 0x%x, val = 0x%x, s=%d, rc=%d!\n",saddr,waddr, bdata,ov5640_counter,rc);
			msleep(4);
		}
	}
	return rc;
}

static int ov5640_i2c_rxdata(unsigned short saddr,
    unsigned char *rxdata, int length)
{
	struct i2c_msg msgs[] = {
		{
			.addr   = saddr,
			.flags = 0,
			.len   = 2,
			.buf   = rxdata,
		},
		{
			.addr   = saddr,
			.flags = I2C_M_RD,
			.len   = length,
			.buf   = rxdata,
		},
	};

	if (i2c_transfer(ov5640_client->adapter, msgs, 2) < 0) {
		CDBG_HIGH("--CAMERA-- ov5640_i2c_rxdata failed!\n");
		return -EIO;
	}

	return 0;
}

static int32_t ov5640_i2c_read_byte(unsigned short   saddr,
    unsigned int raddr, unsigned int *rdata)
{
	int rc = 0;
	unsigned char buf[2];
	//CDBG("+ov5640_i2c_read_byte\n");
	memset(buf, 0, sizeof(buf));

	buf[0] = (raddr & 0xFF00)>>8;
	buf[1] = (raddr & 0x00FF);

	rc = ov5640_i2c_rxdata(saddr, buf, 1);
	if (rc < 0) {
		CDBG_HIGH("--CAMERA-- ov5640_i2c_read_byte failed!\n");
		return rc;
	}

	*rdata = buf[0];
    CDBG("ov5640 read:saddr:0x%x, raddr:0x%x, rdata:0x%x\r\n", saddr,raddr,*rdata);
	//CDBG("-ov5640_i2c_read_byte\n");
	return rc;
}


static int32_t OV5640_WritePRegs(POV5640_WREG pTb,int32_t len)
{
	int32_t i, ret = 0;
	uint32_t regv;
	for (i = 0;i < len; i++)
	{
		if (0 == pTb[i].mask) {
			ov5640_i2c_write(ov5640_client->addr,pTb[i].addr,pTb[i].data,10);
		} else {
			ov5640_i2c_read_byte(ov5640_client->addr,pTb[i].addr, &regv);
			regv &= pTb[i].mask;
			regv |= (pTb[i].data & (~pTb[i].mask));
			ov5640_i2c_write(ov5640_client->addr, pTb[i].addr, regv, 10);
		}
	}
	return ret;
}

static void camera_sw_power_onoff(int v)
{
	if (v == 0) {
		CDBG("--CAMERA-- camera_sw_power_onoff: off\n");
		ov5640_i2c_write(ov5640_client->addr, 0x4202, 0x0f, 10); //stream off
	}
	else {
		CDBG("--CAMERA-- camera_sw_power_onoff: on\n");
		ov5640_i2c_write(ov5640_client->addr, 0x4202, 0x00, 10); //stream on
	}
}

static void ov5640_power_off(void)
{
	CDBG("--CAMERA-- %s ... (Start...)\n",__func__);
    CDBG("--CAMERA--set gpio %d to 1\n",ov5640_pwdn_gpio);
	gpio_set_value(ov5640_pwdn_gpio, 1);
	CDBG("--CAMERA-- %s ... (End...)\n",__func__);
}

static void ov5640_power_on(void)
{
	CDBG("--CAMERA-- %s ... (Start...)\n",__func__);
    CDBG("--CAMERA--set gpio %d to 0\n",ov5640_pwdn_gpio);
	gpio_set_value(ov5640_pwdn_gpio, 0);
	CDBG("--CAMERA-- %s ... (End...)\n",__func__);
}

static void ov5640_power_reset(void)
{
	CDBG("--CAMERA-- %s ... (Start...)\n",__func__);
    CDBG("Generate a pulse on gpio %d\n",ov5640_reset_gpio);

	gpio_set_value(ov5640_reset_gpio, 1);   //reset camera reset pin
	mdelay(20);
	gpio_set_value(ov5640_reset_gpio, 0);
	mdelay(20);
	gpio_set_value(ov5640_reset_gpio, 1);
	mdelay(20);

	CDBG("--CAMERA-- %s ... (End...)\n",__func__);
}

static int ov5640_probe_readID(const struct msm_camera_sensor_info *data)
{
	int rc = 0;
	u32 device_id_high = 0;
	u32 device_id_low = 0;

	CDBG("--CAMERA-- %s (Start...)\n",__func__);
	CDBG("--CAMERA-- %s sensor poweron,begin to read ID!\n",__func__);

	//0x300A ,sensor ID register
	rc = ov5640_i2c_read_byte(ov5640_client->addr, 0x300A, &device_id_high);

	if (rc < 0)
	{
		CDBG_HIGH("--CAMERA-- %s ok , readI2C failed, rc = 0x%x\r\n", __func__, rc);
		return rc;
	}
	CDBG("--CAMERA-- %s  readID high byte, data = 0x%x\r\n", __func__, device_id_high);

	//0x300B ,sensor ID register
	rc = ov5640_i2c_read_byte(ov5640_client->addr, 0x300B, &device_id_low);
	if (rc < 0)
	{
		CDBG_HIGH("--CAMERA-- %s ok , readI2C failed,rc = 0x%x\r\n", __func__, rc);
		return rc;
	}

	CDBG("--CAMERA-- %s  readID low byte, data = 0x%x\r\n", __func__, device_id_low);
	CDBG_HIGH("--CAMERA-- %s return ID :0x%x\n", __func__, (device_id_high<<8)+device_id_low);

	//0x5640, ov5640 chip id
	if((device_id_high<<8)+device_id_low != OV5640_SENSOR_ID)
	{
		CDBG_HIGH("--CAMERA-- %s ok , device id error, should be 0x%x\r\n",
		__func__, OV5640_SENSOR_ID);
		return -EINVAL;
	}
	else
	{
		CDBG_HIGH("--CAMERA-- %s ok , device id=0x%x\n",__func__,OV5640_SENSOR_ID);
		return 0;
	}
}

#if 0
static void dump_af_status(void)
{
	int tmp = 0;

	ov5640_i2c_read_byte(ov5640_client->addr, 0x3000, &tmp);
	CDBG("    %s 0x3000 = %x\n", __func__, tmp);
	ov5640_i2c_read_byte(ov5640_client->addr, 0x3001, &tmp);
	CDBG("    %s 0x3001 = %x\n", __func__, tmp);
	ov5640_i2c_read_byte(ov5640_client->addr, 0x3004, &tmp);
	CDBG("    %s 0x3004 = %x\n", __func__, tmp);
	ov5640_i2c_read_byte(ov5640_client->addr, 0x3005, &tmp);
	CDBG("    %s 0x3005 = %x\n", __func__, tmp);
	ov5640_i2c_read_byte(ov5640_client->addr, OV5640_CMD_FW_STATUS, &tmp);
	CDBG("    %s af_st = %x\n\n", __func__, tmp);
}
#endif

static int ov5640_af_setting(void)
{
	int rc = 0;
	int lens = sizeof(ov5640_afinit_tbl)/sizeof(ov5640_afinit_tbl[0]);

	CDBG("--CAMERA-- ov5640_af_setting\n");

	ov5640_i2c_write(ov5640_client->addr,0x3000,0x20,10);

	rc = ov5640_i2c_txdata(ov5640_client->addr,ov5640_afinit_tbl,lens);
	//rc = OV5640Core_WritePREG(ov5640_afinit_tbl);

	if (rc < 0)
	{
		CDBG_HIGH("--CAMERA-- AF_init failed\n");
		return rc;
	}

	ov5640_i2c_write(ov5640_client->addr,OV5640_CMD_MAIN,0x00,10);
	ov5640_i2c_write(ov5640_client->addr,OV5640_CMD_ACK,0x00,10);
	ov5640_i2c_write(ov5640_client->addr,OV5640_CMD_PARA0,0x00,10);
	ov5640_i2c_write(ov5640_client->addr,OV5640_CMD_PARA1,0x00,10);
	ov5640_i2c_write(ov5640_client->addr,OV5640_CMD_PARA2,0x00,10);
	ov5640_i2c_write(ov5640_client->addr,OV5640_CMD_PARA3,0x00,10);
	ov5640_i2c_write(ov5640_client->addr,OV5640_CMD_PARA4,0x00,10);
	ov5640_i2c_write(ov5640_client->addr,OV5640_CMD_FW_STATUS,0x7f,10);
	ov5640_i2c_write(ov5640_client->addr,0x3000,0x00,10);

	return rc;
}

static int ov5640_set_flash_light(enum led_brightness brightness)
{
#if 0
	struct led_classdev *led_cdev;

	CDBG("--CAMERA-- ov5640_set_flash_light brightness = %d\n", brightness);

	down_read(&leds_list_lock);
	list_for_each_entry(led_cdev, &leds_list, node) {
		if (!strncmp(led_cdev->name, "flashlight", 10)) {
			break;
		}
	}
	up_read(&leds_list_lock);

	if (led_cdev) {
		led_brightness_set(led_cdev, brightness);
	} else {
		CDBG_HIGH("--CAMERA-- get flashlight device failed\n");
		return -1;
	}
#endif
	return 0;
}

#if 0
static int ov5640_get_preview_exposure_gain(void)
{
	int rc = 0;
	unsigned int ret_l,ret_m,ret_h;

	ov5640_i2c_write(ov5640_client->addr,0x3503, 0x07,10);

	//get preview exp & gain
	ret_h = ret_m = ret_l = 0;
	ov5640_preview_exposure = 0;
	ov5640_i2c_read_byte(ov5640_client->addr,0x3500, &ret_h);
	ov5640_i2c_read_byte(ov5640_client->addr,0x3501, &ret_m);
	ov5640_i2c_read_byte(ov5640_client->addr,0x3502, &ret_l);
	ov5640_preview_exposure = (ret_h << 12) + (ret_m << 4) + (ret_l >> 4);
	CDBG("preview_exposure=%d\n", ov5640_preview_exposure);

	ret_h = ret_m = ret_l = 0;
	ov5640_preview_maxlines = 0;
	ov5640_i2c_read_byte(ov5640_client->addr,0x380e, &ret_h);
	ov5640_i2c_read_byte(ov5640_client->addr,0x380f, &ret_l);
	ov5640_preview_maxlines = (ret_h << 8) + ret_l;
	CDBG("Preview_Maxlines=%d\n", ov5640_preview_maxlines);

	//Read back AGC Gain for preview
	ov5640_gain = 0;
	ov5640_i2c_read_byte(ov5640_client->addr,0x350b, &ov5640_gain);
	CDBG("Gain,0x350b=0x%x\n", ov5640_gain);

	return rc;
}

static int ov5640_set_capture_exposure_gain(void)
{
	int rc = 0;
	//calculate capture exp & gain

	unsigned char ExposureLow,ExposureMid,ExposureHigh;
	unsigned int ret_l,ret_m,ret_h,Lines_10ms;
	unsigned short ulCapture_Exposure,iCapture_Gain;
	unsigned int ulCapture_Exposure_Gain,Capture_MaxLines;

	ret_h = ret_m = ret_l = 0;
	ov5640_i2c_read_byte(ov5640_client->addr,0x380e, &ret_h);
	ov5640_i2c_read_byte(ov5640_client->addr,0x380f, &ret_l);
	Capture_MaxLines = (ret_h << 8) + ret_l;
	CDBG("Capture_MaxLines=%d\n", Capture_MaxLines);

	if(m_60Hz == TRUE)
	{
		Lines_10ms = Capture_Framerate * Capture_MaxLines/12000;
	}
	else
	{
		Lines_10ms = Capture_Framerate * Capture_MaxLines/10000;
	}

	if (ov5640_preview_maxlines == 0)
	{
		ov5640_preview_maxlines = 1;
	}

	ulCapture_Exposure = (ov5640_preview_exposure*(Capture_Framerate)*(Capture_MaxLines))
	                	/(((ov5640_preview_maxlines)*(g_Preview_FrameRate)));

	iCapture_Gain = ov5640_gain;

	ulCapture_Exposure_Gain = ulCapture_Exposure * iCapture_Gain;

	if(ulCapture_Exposure_Gain < Capture_MaxLines*16)
	{
		ulCapture_Exposure = ulCapture_Exposure_Gain/16;

		if (ulCapture_Exposure > Lines_10ms)
		{
			ulCapture_Exposure /= Lines_10ms;
			ulCapture_Exposure *= Lines_10ms;
		}
	}
	else
	{
		ulCapture_Exposure = Capture_MaxLines;
	}

	if(ulCapture_Exposure == 0)
	{
		ulCapture_Exposure = 1;
	}

	iCapture_Gain = (ulCapture_Exposure_Gain * 2 / ulCapture_Exposure + 1) / 2;

	ExposureLow = ((unsigned char)ulCapture_Exposure) << 4;

	ExposureMid = (unsigned char)(ulCapture_Exposure >> 4) & 0xff;

	ExposureHigh = (unsigned char)(ulCapture_Exposure >> 12);


	// set capture exp & gain

	ov5640_i2c_write(ov5640_client->addr,0x350b, iCapture_Gain, 10);
	ov5640_i2c_write(ov5640_client->addr,0x3502, ExposureLow, 10);
	ov5640_i2c_write(ov5640_client->addr,0x3501, ExposureMid, 10);
	ov5640_i2c_write(ov5640_client->addr,0x3500, ExposureHigh, 10);

	CDBG("iCapture_Gain=%d\n", iCapture_Gain);
	CDBG("ExposureLow=%d\n", ExposureLow);
	CDBG("ExposureMid=%d\n", ExposureMid);
	CDBG("ExposureHigh=%d\n", ExposureHigh);

	msleep(250);

	return rc;
}
#endif

/********** Exposure optimization **********/
#define	AE_TARGET	0x34
static int XVCLK = 2400;    // real clock/10000
static int current_target = AE_TARGET;
static int preview_sysclk, preview_HTS;
static int AE_high, AE_low;

static int OV5640_read_i2c(unsigned int raddr)
{
	unsigned int data;
	ov5640_i2c_read_byte(ov5640_client->addr,raddr, &data);
	return (int)data;
}
static int OV5640_write_i2c(unsigned int waddr, unsigned int bdata)
{
	return ov5640_i2c_write(ov5640_client->addr, waddr, (unsigned short)bdata, 10);
}

int OV5640_get_sysclk(void)
{
	// calculate sysclk
	int temp1, temp2;
	int Multiplier, PreDiv, VCO, SysDiv, Pll_rdiv, sclk_rdiv, sysclk;
	int Bit_div2x = 4;//default
	int sclk_rdiv_map[] = {1, 2, 4, 8};

	temp1 = OV5640_read_i2c(0x3034);
	temp2 = temp1 & 0x0f;
	if (temp2 == 8 || temp2 == 10) {
		Bit_div2x = temp2 / 2;
	}

	temp1 = OV5640_read_i2c(0x3035);
	SysDiv = temp1>>4;
	if(SysDiv == 0) {
		SysDiv = 16;
	}

	temp1 = OV5640_read_i2c(0x3036);
	Multiplier = temp1;

	temp1 = OV5640_read_i2c(0x3037);
	PreDiv = temp1 & 0x0f;
	Pll_rdiv = ((temp1 >> 4) & 0x01) + 1;

	temp1 = OV5640_read_i2c(0x3108);
	temp2 = temp1 & 0x03;
	sclk_rdiv = sclk_rdiv_map[temp2];

	VCO = XVCLK * Multiplier / PreDiv;

	sysclk = VCO / SysDiv / Pll_rdiv * 2 / Bit_div2x / sclk_rdiv;

	CDBG("--CAMERA-- current sysclk is = %d\n", sysclk);

	return sysclk;
}


int OV5640_get_HTS(void)
{
	// read HTS from register settings
	int HTS;

	HTS = OV5640_read_i2c(0x380c);
	HTS = (HTS<<8) + OV5640_read_i2c(0x380d);
	CDBG("--CAMERA-- get_HTS = %d\n", HTS);

	return HTS;
}

int OV5640_get_VTS(void)
{
	// read VTS from register settings
	int VTS;

	VTS = OV5640_read_i2c(0x380e);
	VTS = (VTS<<8) + OV5640_read_i2c(0x380f);
	CDBG("--CAMERA-- get_VTS = %d\n", VTS);

	return VTS;
}

int OV5640_set_VTS(int VTS)
{
	// write VTS to registers
	int temp;

	CDBG("--CAMERA-- set_VTS = %d\n", VTS);
	temp = VTS & 0xff;
	OV5640_write_i2c(0x380f, temp);

	temp = VTS>>8;
	OV5640_write_i2c(0x380e, temp);

	return 0;
}


int OV5640_get_shutter(void)
{
	// read shutter, in number of line period
	int shutter;

	shutter = (OV5640_read_i2c(0x03500) & 0x0f);
	shutter = (shutter<<8) + OV5640_read_i2c(0x3501);
	shutter = (shutter<<4) + (OV5640_read_i2c(0x3502)>>4);

	CDBG("--CAMERA-- get_shutter: = %d\n", shutter);

	return shutter;
}

int OV5640_set_shutter(int shutter)
{
	// write shutter, in number of line period
	int temp;

	CDBG("--CAMERA-- set_shutter: = %d\n", shutter);
	shutter = shutter & 0xffff;

	temp = shutter & 0x0f;
	temp = temp<<4;
	OV5640_write_i2c(0x3502, temp);

	temp = shutter & 0xfff;
	temp = temp>>4;
	OV5640_write_i2c(0x3501, temp);

	temp = shutter>>12;
	OV5640_write_i2c(0x3500, temp);

	return 0;
}

int OV5640_get_gain16(void)
{
	// read gain, 16 = 1x
	int gain16;

	gain16 = OV5640_read_i2c(0x350a) & 0x03;
	gain16 = (gain16<<8) + OV5640_read_i2c(0x350b);

	CDBG("--CAMERA-- get_gain16: = %d\n", gain16);

	return gain16;
}

int OV5640_set_gain16(int gain16)
{
	// write gain, 16 = 1x
	int temp;

	CDBG("--CAMERA-- set_gain16: = %d\n", gain16);
	gain16 = gain16 & 0x3ff;

	temp = gain16 & 0xff;
	OV5640_write_i2c(0x350b, temp);

	temp = gain16>>8;
	OV5640_write_i2c(0x350a, temp);

	return 0;
}

//For exposure conversion
void OV5640_set_bandingfilter(void)
{
	int preview_VTS;
	int band_step60, max_band60, band_step50, max_band50;

	// read preview PCLK
	preview_sysclk = OV5640_get_sysclk();

	// read preview HTS
	preview_HTS = OV5640_get_HTS();

	// read preview VTS
	preview_VTS = OV5640_get_VTS();

	// calculate banding filter
	// 60Hz
	band_step60 = preview_sysclk * 100/preview_HTS * 100/120;
	OV5640_write_i2c(0x3a0a, (band_step60 >> 8));
	OV5640_write_i2c(0x3a0b, (band_step60 & 0xff));

	max_band60 = (int)((preview_VTS-4)/band_step60);
	OV5640_write_i2c(0x3a0d, max_band60);

	// 50Hz
	band_step50 = preview_sysclk * 100/preview_HTS;
	OV5640_write_i2c(0x3a08, (band_step50 >> 8));
	OV5640_write_i2c(0x3a09, (band_step50 & 0xff));

	max_band50 = (int)((preview_VTS-4)/band_step50);
	OV5640_write_i2c(0x3a0e, max_band50);
}


int OV5640_set_AE_target(int target)
{
	// stable in high
	int fast_high, fast_low;

	CDBG("--CAMERA-- set_AE_target: = %d\n", target);

	AE_low = target * 23 / 25;    // 0.92
	AE_high = target * 27 / 25;    // 1.08

	fast_high = AE_high<<1;
	if(fast_high>255)
		fast_high = 255;

	fast_low = AE_low>>1;

	OV5640_write_i2c(0x3a0f, AE_high);
	OV5640_write_i2c(0x3a10, AE_low);
	OV5640_write_i2c(0x3a1b, AE_high);
	OV5640_write_i2c(0x3a1e, AE_low);
	OV5640_write_i2c(0x3a11, fast_high);
	OV5640_write_i2c(0x3a1f, fast_low);

	return 0;
}

void OV5640_set_EV(int ev)
{
	CDBG("--CAMERA-- set_EV: = %d\n", ev);

	switch (ev)
	{
		case 0://+5
			current_target = AE_TARGET * 16/5;
			OV5640_set_AE_target(current_target);
			break;

		case 1://+4
			current_target = AE_TARGET * 5/2;
			OV5640_set_AE_target(current_target);
			break;

		case 2://+3
			current_target = AE_TARGET * 2;
			OV5640_set_AE_target(current_target);
			break;

		case 3://+2
			current_target = AE_TARGET * 8/5;
			OV5640_set_AE_target(current_target);
			break;

		case 4://+1
			current_target = AE_TARGET * 5/4;
			OV5640_set_AE_target(current_target);
			break;

		case 5://+0
			current_target = AE_TARGET;
			OV5640_set_AE_target(current_target);
			break;

		case 6://-1
			current_target = AE_TARGET * 4/5;
			OV5640_set_AE_target(current_target);
			break;

		case 7://-2
			current_target = AE_TARGET * 5/8;
			OV5640_set_AE_target(current_target);
			break;

		case 8://-3
			current_target = AE_TARGET / 2;
			OV5640_set_AE_target(current_target);
			break;

		case 9://-4
			current_target = AE_TARGET * 2/5;
			OV5640_set_AE_target(current_target);
			break;

		case 10://-5
			current_target = AE_TARGET * 5/16;
			OV5640_set_AE_target(current_target);
			break;

		default:
			break;
	}
}


int OV5640_get_light_frequency(void)
{
	// get banding filter value
	int temp, temp1, light_frequency;

	temp = OV5640_read_i2c(0x3c01);

	if (temp & 0x80) {
		// manual
		temp1 = OV5640_read_i2c(0x3c00);
		if (temp1 & 0x04) {
		// 50Hz
		light_frequency = 50;
		CDBG("--CAMERA-- get_light_frequency: 50Hz Manual\n");
		}
		else {
			// 60Hz
			light_frequency = 60;
			CDBG("--CAMERA-- get_light_frequency: 60Hz Manual\n");
		}
	}
	else
	{
		// auto
		temp1 = OV5640_read_i2c(0x3c0c);
		if (temp1 & 0x01) {
			// 50Hz
			light_frequency = 50;
			CDBG("--CAMERA-- get_light_frequency: 50Hz Auto\n");
		}
		else {
			// 60Hz
			light_frequency = 60;
			CDBG("--CAMERA-- get_light_frequency: 60Hz Auto\n");
		}
	}
	return light_frequency;
}

static int  preview_shutter, preview_gain16;
int OV5640_capture(void)
{
	int rc;
	int tmp;
	long exposure_adj;

	//int preview_shutter, preview_gain16, average;
	int average;
	int capture_shutter, capture_gain16;
	int capture_sysclk, capture_HTS, capture_VTS;
	int light_frequency, capture_bandingfilter, capture_max_band;
	long capture_gain16_shutter;

	CDBG("--CAMERA-- ======================================\n");

	CDBG("--CAMERA-- snapshot in, is_autoflash - %d\n", is_autoflash);

	OV5640_write_i2c(0x3503, 0x3); //AE manual mode

	//AE below
	// read preview shutter
	preview_shutter = OV5640_get_shutter();

	// read preview gain
	preview_gain16 = OV5640_get_gain16();

	// get average
	average = OV5640_read_i2c(0x56a1);
	CDBG("OV5640_capture.. 0x56a1 =%d \n", average);

#if 0
	if (is_autoflash == 1) {
		ov5640_i2c_read_byte(ov5640_client->addr, 0x350b, &tmp);
		CDBG("--CAMERA-- GAIN VALUE : %x\n", tmp);
		if ((tmp & 0x80) != 0) {
			ov5640_set_flash_light(LED_FULL);
		}
	}
#endif

	rc = OV5640Core_WritePREG(ov5640_capture_tbl);

	mdelay(3);

	//if awb mode is auto, check the average luminace
	if(is_awb_enabled){
		CDBG("%s: is_awb_enabled =%d, current_avg_luminance =%d, is_effect_auto=%d \n",
			__func__, is_awb_enabled, current_avg_luminance, is_effect_auto);

		if(current_avg_luminance <= AVG_LUM_THRESH){
			tmp = OV5640_read_i2c(0x3406);
			tmp = tmp | 0x01;
			OV5640_write_i2c(0x3406, tmp);

			OV5640_write_i2c(0x3400, reg3400val);
			OV5640_write_i2c(0x3401, reg3401val);
			OV5640_write_i2c(0x3402, reg3402val);
			OV5640_write_i2c(0x3403, reg3403val);
			OV5640_write_i2c(0x3404, reg3404val);
			OV5640_write_i2c(0x3405, reg3405val);

			CDBG("%s: reg3400val =%d, reg3401val=%d, reg3402val=%d, reg3403val=%d, reg3404val=%d,reg3405val=%d \n",
				__func__,reg3400val,reg3401val, reg3402val,reg3403val,reg3404val,reg3405val);

			if(is_effect_auto){
				OV5640_write_i2c(0x5583, 0x28);
				OV5640_write_i2c(0x5584, 0x28);
				tmp = OV5640_read_i2c(0x5588);
				tmp = tmp | 0x40;
				OV5640_write_i2c(0x5588, tmp);
				CDBG("OV5640_capture. effect is auto \n");
			}
		}
	}

	// read capture VTS
	capture_HTS = OV5640_get_HTS();
	capture_VTS = OV5640_get_VTS();
	capture_sysclk = OV5640_get_sysclk();

	// calculate capture banding filter
	light_frequency = OV5640_get_light_frequency();
	if (light_frequency == 60) {
		// 60Hz
		capture_bandingfilter = capture_sysclk * 100 / capture_HTS * 100 / 120;
	}
	else {
		// 50Hz
		capture_bandingfilter = capture_sysclk * 100 / capture_HTS;
	}
	capture_max_band = (int)((capture_VTS - 4)/capture_bandingfilter);

	// calculate capture shutter/gain16
	if (average > AE_low && average < AE_high) {
		// in stable range
		capture_gain16_shutter = preview_gain16 * preview_shutter * capture_sysclk/preview_sysclk * preview_HTS/capture_HTS * AE_TARGET / average;
	}
	else {
		capture_gain16_shutter = preview_gain16 * preview_shutter * capture_sysclk/preview_sysclk * preview_HTS/capture_HTS;
	}

	if(is_autoflash_working)
	{
		//auto flash working, re-calibrate exposure
		//*0.8
		exposure_adj = capture_gain16_shutter * 4;
		capture_gain16_shutter = exposure_adj / 5;
	}

	// gain to shutter
	if(capture_gain16_shutter < (capture_bandingfilter * 16)) {
		// shutter < 1/100
		capture_shutter = capture_gain16_shutter/16;
		if(capture_shutter <1)
			capture_shutter = 1;

		capture_gain16 = capture_gain16_shutter/capture_shutter;
		if(capture_gain16 < 16)
			capture_gain16 = 16;
	}
	else {
		if(capture_gain16_shutter > (capture_bandingfilter*capture_max_band*16)) {
			// exposure reach max
			capture_shutter = capture_bandingfilter*capture_max_band;
			capture_gain16 = capture_gain16_shutter / capture_shutter;
		}
		else {
			// 1/100 < capture_shutter =< max, capture_shutter = n/100
			capture_shutter = ((int) (capture_gain16_shutter/16/capture_bandingfilter)) * capture_bandingfilter;
			capture_gain16 = capture_gain16_shutter / capture_shutter;
		}
	}

	// write capture gain
	OV5640_set_gain16(capture_gain16);

	// write capture shutter
	if (capture_shutter > (capture_VTS - 4)) {
		capture_VTS = capture_shutter + 4;
		OV5640_set_VTS(capture_VTS);
	}
	OV5640_set_shutter(capture_shutter);

	//OV5640_set_EV(8);

	return rc;
}

static int ov5640_sensor_setting(int update_type, int rt)
{
	int rc = -EINVAL;
	int tmp = 0;
	struct msm_camera_csi_params ov5640_csi_params;

	CDBG("--CAMERA-- update_type = %d, rt = %d\n", update_type, rt);

	//Prepare clk and data stop
	rc = ov5640_i2c_write(ov5640_client->addr, 0x4800, 0x24, 10);
	msleep(10);

	camera_sw_power_onoff(0); //standby
	if (update_type == S_REG_INIT) {
		rc = ov5640_i2c_write(ov5640_client->addr, 0x3103, 0x11, 10);
		rc = ov5640_i2c_write(ov5640_client->addr, 0x3008, 0x82, 10);
		msleep(5);

		//set sensor init setting
		CDBG("--CAMERA-- set sensor init setting\n");
		rc = OV5640Core_WritePREG(ov5640_init_tbl);
		if (rc < 0) {
			CDBG_HIGH("--CAMERA-- sensor init setting failed\n");
			return rc;
		}

		//set image quality setting
		CDBG("--CAMERA-- set image quality setting\n");
		rc = OV5640Core_WritePREG(ov5640_init_iq_tbl);
		if (rc < 0) {
			CDBG_HIGH("--CAMERA-- sensor init_iq setting failed\n");
			return rc;;
		}

		rc =ov5640_i2c_read_byte(ov5640_client->addr, 0x4740, &tmp);
		CDBG("--CAMERA-- init 0x4740 value=0x%x\n", tmp);

		if (tmp != 0x21) {
			rc = ov5640_i2c_write(ov5640_client->addr, 0x4740, 0x21, 10);
			msleep(10);
			rc =ov5640_i2c_read_byte(ov5640_client->addr, 0x4740, &tmp);
			CDBG("--CAMERA-- WG 0x4740 value=0x%x\n", tmp);
		}

		CDBG("--CAMERA-- AF_init: afinit = %d\n", afinit);
		if (afinit == 1) {
			rc = ov5640_af_setting();
			if (rc < 0) {
				CDBG_HIGH("ov5640_af_setting failed\n");
				return rc;
			}
			afinit = 0;
		}

		/* reset fps_divider */
		ov5640_ctrl->fps_divider = 1 * 0x0400;
	} else if (update_type == S_UPDATE_PERIODIC) {
		//wait for clk/data really stop
		if ((rt == FULL_SIZE) || (OV5640_CSI_CONFIG == 0) ) {
			msleep(66);
		}
		else{
			msleep(266);
		}
		if(!OV5640_CSI_CONFIG) {
			ov5640_csi_params.lane_cnt = 2;
			ov5640_csi_params.data_format = CSI_8BIT;
			ov5640_csi_params.lane_assign = 0xe4;
			ov5640_csi_params.dpcm_scheme = 0;
			ov5640_csi_params.settle_cnt = 0x6;

			CDBG("--CAMERA-- msm_camio_csi_config\n");
			rc = msm_camio_csi_config(&ov5640_csi_params);
			CDBG("--CAMERA-- msm_camio_csi_config out\n");

			msleep(20);

			OV5640_CSI_CONFIG = 1;
			CDBG("--CAMERA-- msm_camio_csi_config 1\n");

		}

        CDBG("--CAMERA-- ov5640_sensor_setting 2\n");
        msleep(20);

		if (rt != FULL_SIZE)
		{
            CDBG("--CAMERA-- ov5640_sensor_setting 3\n");
			//turn off flash when preview
			ov5640_set_flash_light(LED_OFF);
			is_autoflash_working = 0;
		}

        CDBG("--CAMERA-- ov5640_sensor_setting 4\n");
		if (rt == QTR_SIZE) {
			/* preview setting */
			rc = OV5640Core_WritePREG(ov5640_preview_tbl_30fps);

			//Test Begin:restore preview shutter
			OV5640_set_gain16(preview_gain16);
			OV5640_set_shutter(preview_shutter);
			//Test End
			//mdelay(30);

			// calculate banding filter
			OV5640_set_bandingfilter();

			// set ae_target
			OV5640_set_AE_target(current_target);
			OV5640_write_i2c(0x3503, 0x0); //AE auto mode
		} else if (rt == FULL_SIZE) {
			rc = OV5640_capture();
			/* Delay to get Snapshot Working */
			//msleep(260);

		} else if (rt == HFR_60FPS) {
			rc = OV5640Core_WritePREG(ov5640_preview_tbl_60fps);
		} else if (rt == HFR_90FPS) {
			rc = OV5640Core_WritePREG(ov5640_preview_tbl_90fps);
		}

		//if awb mode is auto, check status of 0x3406
		if((rt != FULL_SIZE) && is_awb_enabled){
			CDBG("--CAMERA-- set auto awb mode. \n");
			tmp = OV5640_read_i2c(0x3406);
			tmp = tmp & 0xfe;
			OV5640_write_i2c(0x3406, tmp);
			current_avg_luminance = AVG_LUM_THRESH +1; //set default avg lum larger than threshold
		}
		//open stream
		camera_sw_power_onoff(1); //on
		msleep(10);
		rc = ov5640_i2c_write(ov5640_client->addr, 0x4800, 0x04, 10);
	}

	return rc;
}

static int ov5640_video_config(int mode)
{
	int rc = 0;

	CDBG("--CAMERA-- ov5640_video_config\n");
	/* change sensor resolution if needed */
	if ((rc = ov5640_sensor_setting(S_UPDATE_PERIODIC, ov5640_ctrl->prev_res)) < 0) {
		CDBG_HIGH("--CAMERA-- ov5640_video_config failed\n");
		return rc;
	}

	ov5640_ctrl->curr_res = ov5640_ctrl->prev_res;
	ov5640_ctrl->sensormode = mode;
	return rc;
}

static int ov5640_snapshot_config(int mode)
{
	int rc = 0;

	CDBG("--CAMERA-- ov5640_snapshot_config\n");
	/*change sensor resolution if needed */
	if (ov5640_ctrl->curr_res != ov5640_ctrl->pict_res) {
		if ((rc = ov5640_sensor_setting(S_UPDATE_PERIODIC, ov5640_ctrl->pict_res)) < 0) {
			CDBG_HIGH("--CAMERA-- ov5640_snapshot_config failed\n");
			return rc;
		}
	}

	ov5640_ctrl->curr_res = ov5640_ctrl->pict_res;
	ov5640_ctrl->sensormode = mode;
	return rc;
}

static int ov5640_raw_snapshot_config(int mode)
{
	int rc = 0;

	CDBG("--CAMERA-- ov5640_raw_snapshot_config\n");
	/* change sensor resolution if needed */
	if (ov5640_ctrl->curr_res != ov5640_ctrl->pict_res) {
		if ((rc = ov5640_sensor_setting(S_UPDATE_PERIODIC, ov5640_ctrl->pict_res)) < 0) {
			CDBG_HIGH("--CAMERA-- ov5640_raw_snapshot_config failed\n");
			return rc;
		}
	}

	ov5640_ctrl->curr_res = ov5640_ctrl->pict_res;
	ov5640_ctrl->sensormode = mode;
	return rc;
}

static int ov5640_set_sensor_mode(int mode,
  int res)
{
	int rc = 0;

	switch (mode) {
		case SENSOR_PREVIEW_MODE:
		case SENSOR_HFR_60FPS_MODE:
		case SENSOR_HFR_90FPS_MODE:
			ov5640_ctrl->prev_res = res;
			rc = ov5640_video_config(mode);
			break;
		case SENSOR_SNAPSHOT_MODE:
			ov5640_ctrl->pict_res = res;
			rc = ov5640_snapshot_config(mode);
			break;
		case SENSOR_RAW_SNAPSHOT_MODE:
			ov5640_ctrl->pict_res = res;
			rc = ov5640_raw_snapshot_config(mode);
			break;
		default:
			rc = -EINVAL;
			break;
	}

	return rc;
}

extern int lcd_camera_power_onoff(int on);

static int ov5640_sensor_open_init(const struct msm_camera_sensor_info *data)
{
	int rc = -ENOMEM;
	CDBG("--CAMERA-- %s\n",__func__);

	ov5640_ctrl = kzalloc(sizeof(struct __ov5640_ctrl), GFP_KERNEL);
	if (!ov5640_ctrl)
	{
		CDBG_HIGH("--CAMERA-- kzalloc ov5640_ctrl error !!\n");
		return rc;
	}
	ov5640_ctrl->fps_divider = 1 * 0x00000400;
	ov5640_ctrl->pict_fps_divider = 1 * 0x00000400;
	ov5640_ctrl->set_test = S_TEST_OFF;
	ov5640_ctrl->prev_res = QTR_SIZE;
	ov5640_ctrl->pict_res = FULL_SIZE;

	if (data)
		ov5640_ctrl->sensordata = data;

	lcd_camera_power_onoff(1); //turn on LDO for PVT

	ov5640_power_off();

	CDBG("--CAMERA-- %s: msm_camio_clk_rate_set\n", __func__);

	msm_camio_clk_rate_set(24000000);
	msleep(5);

	ov5640_power_on();
	msleep(5);

	ov5640_power_reset();
	msleep(5);

	CDBG("--CAMERA-- %s: init sequence\n", __func__);

	rc = ov5640_sensor_setting(S_REG_INIT, ov5640_ctrl->prev_res);
	if (rc < 0)
	{
		CDBG_HIGH("--CAMERA-- %s : ov5640_setting failed. rc = %d\n",__func__,rc);
		kfree(ov5640_ctrl);
		ov5640_ctrl = NULL;

		ov5640_power_off();

		lcd_camera_power_onoff(0); //turn off LDO for PVT

		return rc;
	}

	OV5640_CSI_CONFIG = 0;

	CDBG("--CAMERA--re_init_sensor ok!!\n");
	return rc;
}

static int ov5640_sensor_release(void)
{
	CDBG("--CAMERA--ov5640_sensor_release!!\n");

	mutex_lock(&ov5640_mutex);

	ov5640_power_off();

	lcd_camera_power_onoff(0);

	if(NULL != ov5640_ctrl)
	{
		kfree(ov5640_ctrl);
		ov5640_ctrl = NULL;
	}
	OV5640_CSI_CONFIG = 0;

	mutex_unlock(&ov5640_mutex);
	return 0;
}

static const struct i2c_device_id ov5640_i2c_id[] = {
	{"ov5640", 0},{}
};

static int ov5640_i2c_remove(struct i2c_client *client)
{
	if(NULL != ov5640_sensorw)
	{
		kfree(ov5640_sensorw);
		ov5640_sensorw = NULL;
	}
	ov5640_client = NULL;
	return 0;
}

static int ov5640_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&ov5640_wait_queue);
	return 0;
}

static long ov5640_set_effect(int mode, int effect)
{
	int rc = 0;
	CDBG("--CAMERA-- %s ...(Start)\n",__func__);

	switch (mode)
	{
		case SENSOR_PREVIEW_MODE:
		case SENSOR_HFR_60FPS_MODE:
		case SENSOR_HFR_90FPS_MODE:
		//Context A Special Effects /
		CDBG("--CAMERA-- %s ...SENSOR_PREVIEW_MODE\n",__func__);
		break;

		case SENSOR_SNAPSHOT_MODE:
		// Context B Special Effects /
		CDBG("--CAMERA-- %s ...SENSOR_SNAPSHOT_MODE\n",__func__);
		break;

		default:
		break;
	}

	effect_value = effect;
	camera_sw_power_onoff(0); //standby
	msleep(33);
	switch (effect)
	{
		case CAMERA_EFFECT_OFF: {
			CDBG("--CAMERA-- %s ...CAMERA_EFFECT_OFF\n",__func__);
			rc = OV5640Core_WritePREG(ov5640_effect_normal_tbl);
			ov5640_i2c_write(ov5640_client->addr, 0x5583, SAT_U, 10); //for recover saturation level when change special effect
			ov5640_i2c_write(ov5640_client->addr, 0x5584, SAT_V, 10); //for recover saturation level when change special effect
			is_effect_auto = 1;
			break;
		}

		case CAMERA_EFFECT_MONO: {
			CDBG("--CAMERA-- %s ...CAMERA_EFFECT_MONO\n",__func__);
			rc = OV5640Core_WritePREG(ov5640_effect_mono_tbl);
			is_effect_auto = 0;
			break;
		}

		case CAMERA_EFFECT_BW: {
			CDBG("--CAMERA-- %s ...CAMERA_EFFECT_BW\n",__func__);
			rc = OV5640Core_WritePREG(ov5640_effect_bw_tbl);
			is_effect_auto = 0;
			break;
		}
		case CAMERA_EFFECT_BLUISH: {
			CDBG("--CAMERA-- %s ...CAMERA_EFFECT_BLUISH\n",__func__);
			rc = OV5640Core_WritePREG(ov5640_effect_bluish_tbl);
			is_effect_auto = 0;
			break;
		}
		case CAMERA_EFFECT_SOLARIZE: {
			CDBG("--CAMERA-- %s ...CAMERA_EFFECT_NEGATIVE\n",__func__);
			rc = OV5640Core_WritePREG(ov5640_effect_solarize_tbl);
			is_effect_auto = 0;
			break;
		}
		case CAMERA_EFFECT_SEPIA: {
			CDBG("--CAMERA-- %s ...CAMERA_EFFECT_SEPIA\n",__func__);
			rc = OV5640Core_WritePREG(ov5640_effect_sepia_tbl);
			is_effect_auto = 0;
			break;
		}
		case CAMERA_EFFECT_REDDISH: {
			CDBG("--CAMERA-- %s ...CAMERA_EFFECT_REDDISH\n",__func__);
			rc = OV5640Core_WritePREG(ov5640_effect_reddish_tbl);
			is_effect_auto = 0;
			break;
		}
		case CAMERA_EFFECT_GREENISH: {
			CDBG("--CAMERA-- %s ...CAMERA_EFFECT_GREENISH\n",__func__);
			rc = OV5640Core_WritePREG(ov5640_effect_greenish_tbl);
			is_effect_auto = 0;
			break;
		}
		case CAMERA_EFFECT_NEGATIVE: {
			CDBG("--CAMERA-- %s ...CAMERA_EFFECT_NEGATIVE\n",__func__);
			rc = OV5640Core_WritePREG(ov5640_effect_negative_tbl);
			is_effect_auto = 0;
			break;
		}
		default: {
			CDBG_HIGH("--CAMERA-- %s ...Default(Not Support)\n",__func__);
		}
	}
	ov5640_effect = effect;
	camera_sw_power_onoff(1); //resume
	//Refresh Sequencer /
	CDBG("--CAMERA-- %s ...(End)\n",__func__);
	return rc;
}

static int ov5640_set_brightness(int8_t brightness)
{
	int rc = 0;
	CDBG("--CAMERA-- %s ...(Start)\n",__func__);
	CDBG("--CAMERA-- %s ...brightness = %d\n",__func__ , brightness);

	switch (brightness)
	{
		case CAMERA_BRIGHTNESS_LV0:
			CDBG("--CAMERA--CAMERA_BRIGHTNESS_LV0\n");
			rc = OV5640Core_WritePREG(ov5640_brightness_lv0_tbl);
			break;

		case CAMERA_BRIGHTNESS_LV1:
			CDBG("--CAMERA--CAMERA_BRIGHTNESS_LV1\n");
			rc = OV5640Core_WritePREG(ov5640_brightness_lv1_tbl);
			break;

		case CAMERA_BRIGHTNESS_LV2:
			CDBG("--CAMERA--CAMERA_BRIGHTNESS_LV2\n");
			rc = OV5640Core_WritePREG(ov5640_brightness_lv2_tbl);
			break;

		case CAMERA_BRIGHTNESS_LV3:
			CDBG("--CAMERA--CAMERA_BRIGHTNESS_LV3\n");
			rc = OV5640Core_WritePREG(ov5640_brightness_lv3_tbl);
			break;

		case CAMERA_BRIGHTNESS_LV4:
			CDBG("--CAMERA--CAMERA_BRIGHTNESS_LV4\n");
			rc = OV5640Core_WritePREG(ov5640_brightness_default_lv4_tbl);
			break;

		case CAMERA_BRIGHTNESS_LV5:
			CDBG("--CAMERA--CAMERA_BRIGHTNESS_LV5\n");
			rc = OV5640Core_WritePREG(ov5640_brightness_lv5_tbl);
			break;

		case CAMERA_BRIGHTNESS_LV6:
			CDBG("--CAMERA--CAMERA_BRIGHTNESS_LV6\n");
			rc = OV5640Core_WritePREG(ov5640_brightness_lv6_tbl);
			break;

		case CAMERA_BRIGHTNESS_LV7:
			CDBG("--CAMERA--CAMERA_BRIGHTNESS_LV7\n");
			rc = OV5640Core_WritePREG(ov5640_brightness_lv7_tbl);
			break;

		case CAMERA_BRIGHTNESS_LV8:
			CDBG("--CAMERA--CAMERA_BRIGHTNESS_LV8\n");
			rc = OV5640Core_WritePREG(ov5640_brightness_lv8_tbl);
			break;

		default:
			CDBG_HIGH("--CAMERA--CAMERA_BRIGHTNESS_ERROR COMMAND\n");
			break;
	}
	CDBG("--CAMERA-- %s ...(End)\n",__func__);
	return rc;
}

static int ov5640_set_contrast(int contrast)
{
	int rc = 0;
	CDBG("--CAMERA-- %s ...(Start)\n",__func__);
	CDBG("--CAMERA-- %s ...contrast = %d\n",__func__ , contrast);

	if (effect_value == CAMERA_EFFECT_OFF)
	{
		switch (contrast)
		{
			case CAMERA_CONTRAST_LV0:
				CDBG("--CAMERA--CAMERA_CONTRAST_LV0\n");
				rc = OV5640Core_WritePREG(ov5640_contrast_lv0_tbl);
				break;
			case CAMERA_CONTRAST_LV1:
				CDBG("--CAMERA--CAMERA_CONTRAST_LV1\n");
				rc = OV5640Core_WritePREG(ov5640_contrast_lv1_tbl);
				break;
			case CAMERA_CONTRAST_LV2:
				CDBG("--CAMERA--CAMERA_CONTRAST_LV2\n");
				rc = OV5640Core_WritePREG(ov5640_contrast_lv2_tbl);
				break;
			case CAMERA_CONTRAST_LV3:
				CDBG("--CAMERA--CAMERA_CONTRAST_LV3\n");
				rc = OV5640Core_WritePREG(ov5640_contrast_lv3_tbl);
				break;
			case CAMERA_CONTRAST_LV4:
				CDBG("--CAMERA--CAMERA_CONTRAST_LV4\n");
				rc = OV5640Core_WritePREG(ov5640_contrast_lv4_tbl);
				break;
			case CAMERA_CONTRAST_LV5:
				CDBG("--CAMERA--CAMERA_CONTRAST_LV5\n");
				rc = OV5640Core_WritePREG(ov5640_contrast_default_lv5_tbl);
				break;
			case CAMERA_CONTRAST_LV6:
				CDBG("--CAMERA--CAMERA_CONTRAST_LV6\n");
				rc = OV5640Core_WritePREG(ov5640_contrast_lv6_tbl);
				break;
			case CAMERA_CONTRAST_LV7:
				CDBG("--CAMERA--CAMERA_CONTRAST_LV7\n");
				rc = OV5640Core_WritePREG(ov5640_contrast_lv7_tbl);
				break;
			case CAMERA_CONTRAST_LV8:
				CDBG("--CAMERA--CAMERA_CONTRAST_LV8\n");
				rc = OV5640Core_WritePREG(ov5640_contrast_lv8_tbl);
				break;
			case CAMERA_CONTRAST_LV9:
				CDBG("--CAMERA--CAMERA_CONTRAST_LV9\n");
				rc = OV5640Core_WritePREG(ov5640_contrast_lv9_tbl);
				break;
			case CAMERA_CONTRAST_LV10:
				CDBG("--CAMERA--CAMERA_CONTRAST_LV10\n");
				rc = OV5640Core_WritePREG(ov5640_contrast_lv10_tbl);
				break;
			default:
				CDBG_HIGH("--CAMERA--CAMERA_CONTRAST_ERROR COMMAND\n");
				break;
		}
	}
	CDBG("--CAMERA-- %s ...(End)\n",__func__);
	return rc;
}

static int ov5640_set_sharpness(int sharpness)
{
	int rc = 0;
	CDBG("--CAMERA-- %s ...(Start)\n",__func__);
	CDBG("--CAMERA-- %s ...sharpness = %d\n",__func__ , sharpness);

	if (effect_value == CAMERA_EFFECT_OFF)
	{
		switch(sharpness)
		{
			case CAMERA_SHARPNESS_LV0:
				CDBG("--CAMERA--CAMERA_SHARPNESS_LV0\n");
				rc = OV5640Core_WritePREG(ov5640_sharpness_lv0_tbl);
				break;
			case CAMERA_SHARPNESS_LV1:
				CDBG("--CAMERA--CAMERA_SHARPNESS_LV1\n");
				rc = OV5640Core_WritePREG(ov5640_sharpness_lv1_tbl);
				break;
			case CAMERA_SHARPNESS_LV2:
				CDBG("--CAMERA--CAMERA_SHARPNESS_LV2\n");
				rc = OV5640Core_WritePREG(ov5640_sharpness_default_lv2_tbl);
				break;
			case CAMERA_SHARPNESS_LV3:
				CDBG("--CAMERA--CAMERA_SHARPNESS_LV3\n");
				rc = OV5640Core_WritePREG(ov5640_sharpness_lv3_tbl);
				break;
			case CAMERA_SHARPNESS_LV4:
				CDBG("--CAMERA--CAMERA_SHARPNESS_LV4\n");
				rc = OV5640Core_WritePREG(ov5640_sharpness_lv4_tbl);
				break;
			case CAMERA_SHARPNESS_LV5:
				CDBG("--CAMERA--CAMERA_SHARPNESS_LV5\n");
				rc = OV5640Core_WritePREG(ov5640_sharpness_lv5_tbl);
				break;
			case CAMERA_SHARPNESS_LV6:
				CDBG("--CAMERA--CAMERA_SHARPNESS_LV6\n");
				rc = OV5640Core_WritePREG(ov5640_sharpness_lv6_tbl);
				break;
			case CAMERA_SHARPNESS_LV7:
				CDBG("--CAMERA--CAMERA_SHARPNESS_LV7\n");
				rc = OV5640Core_WritePREG(ov5640_sharpness_lv7_tbl);
				break;
			case CAMERA_SHARPNESS_LV8:
				CDBG("--CAMERA--CAMERA_SHARPNESS_LV8\n");
				rc = OV5640Core_WritePREG(ov5640_sharpness_lv8_tbl);
				break;
			default:
				CDBG_HIGH("--CAMERA--CAMERA_SHARPNESS_ERROR COMMAND\n");
				break;
		}
	}

	CDBG("--CAMERA-- %s ...(End)\n",__func__);
	return rc;
}

static int ov5640_set_saturation(int saturation)
{
	long rc = 0;
	//int i = 0;
	CDBG("--CAMERA-- %s ...(Start)\n",__func__);
	CDBG("--CAMERA-- %s ...saturation = %d\n",__func__ , saturation);

	if (effect_value == CAMERA_EFFECT_OFF)
	{
		switch (saturation)
		{
			case CAMERA_SATURATION_LV0:
				CDBG("--CAMERA--CAMERA_SATURATION_LV0\n");
				rc = OV5640Core_WritePREG(ov5640_saturation_lv0_tbl);
				break;
			case CAMERA_SATURATION_LV1:
				CDBG("--CAMERA--CAMERA_SATURATION_LV1\n");
				rc = OV5640Core_WritePREG(ov5640_saturation_lv1_tbl);
				break;
			case CAMERA_SATURATION_LV2:
				CDBG("--CAMERA--CAMERA_SATURATION_LV2\n");
				rc = OV5640Core_WritePREG(ov5640_saturation_lv2_tbl);
				break;
			case CAMERA_SATURATION_LV3:
				CDBG("--CAMERA--CAMERA_SATURATION_LV3\n");
				rc = OV5640Core_WritePREG(ov5640_saturation_lv3_tbl);
				break;
			case CAMERA_SATURATION_LV4:
				CDBG("--CAMERA--CAMERA_SATURATION_LV4\n");
				rc = OV5640Core_WritePREG(ov5640_saturation_lv4_tbl);
				break;
			case CAMERA_SATURATION_LV5:
				CDBG("--CAMERA--CAMERA_SATURATION_LV5\n");
				rc = OV5640Core_WritePREG(ov5640_saturation_default_lv5_tbl);
				break;
			case CAMERA_SATURATION_LV6:
				CDBG("--CAMERA--CAMERA_SATURATION_LV6\n");
				rc = OV5640Core_WritePREG(ov5640_saturation_lv6_tbl);
				break;
			case CAMERA_SATURATION_LV7:
				CDBG("--CAMERA--CAMERA_SATURATION_LV7\n");
				rc = OV5640Core_WritePREG(ov5640_saturation_lv7_tbl);
				break;
			case CAMERA_SATURATION_LV8:
				CDBG("--CAMERA--CAMERA_SATURATION_LV8\n");
				rc = OV5640Core_WritePREG(ov5640_saturation_lv8_tbl);
				break;
			case CAMERA_SATURATION_LV9:
				CDBG("--CAMERA--CAMERA_SATURATION_LV9\n");
				rc = OV5640Core_WritePREG(ov5640_saturation_lv9_tbl);
				break;
			case CAMERA_SATURATION_LV10:
				CDBG("--CAMERA--CAMERA_SATURATION_LV10\n");
				rc = OV5640Core_WritePREG(ov5640_saturation_lv10_tbl);
				break;
			default:
				CDBG_HIGH("--CAMERA--CAMERA_SATURATION_ERROR COMMAND\n");
				break;
		}
	}

	//for recover saturation level when change special effect
	switch (saturation)
	{
		case CAMERA_SATURATION_LV0:
			CDBG("--CAMERA--CAMERA_SATURATION_LV0\n");
			SAT_U = 0x00;
			SAT_V = 0x00;
			break;
		case CAMERA_SATURATION_LV1:
			CDBG("--CAMERA--CAMERA_SATURATION_LV1\n");
			SAT_U = 0x06;
			SAT_V = 0x06;
			break;
		case CAMERA_SATURATION_LV2:
			CDBG("--CAMERA--CAMERA_SATURATION_LV2\n");
			SAT_U = 0x10;
			SAT_V = 0x10;
			break;
		case CAMERA_SATURATION_LV3:
			CDBG("--CAMERA--CAMERA_SATURATION_LV3\n");
			SAT_U = 0x20;
			SAT_V = 0x20;
			break;
		case CAMERA_SATURATION_LV4:
			CDBG("--CAMERA--CAMERA_SATURATION_LV4\n");
			SAT_U = 0x30;
			SAT_V = 0x30;
			break;
		case CAMERA_SATURATION_LV5:
			CDBG("--CAMERA--CAMERA_SATURATION_LV5\n");
			SAT_U = 0x40;
			SAT_V = 0x40;
			break;
		case CAMERA_SATURATION_LV6:
			CDBG("--CAMERA--CAMERA_SATURATION_LV6\n");
			SAT_U = 0x50;
			SAT_V = 0x50;
			break;
		case CAMERA_SATURATION_LV7:
			CDBG("--CAMERA--CAMERA_SATURATION_LV7\n");
			SAT_U = 0x60;
			SAT_V = 0x60;
			break;
		case CAMERA_SATURATION_LV8:
			CDBG("--CAMERA--CAMERA_SATURATION_LV8\n");
			SAT_U = 0x70;
			SAT_V = 0x70;
			break;
		case CAMERA_SATURATION_LV9:
			CDBG("--CAMERA--CAMERA_SATURATION_LV9\n");
			SAT_U = 0x80;
			SAT_V = 0x80;
			break;
		case CAMERA_SATURATION_LV10:
			CDBG("--CAMERA--CAMERA_SATURATION_LV10\n");
			SAT_U = 0x88;
			SAT_V = 0x88;
			break;
		default:
			CDBG_HIGH("--CAMERA--CAMERA_SATURATION_ERROR COMMAND\n");
			break;
	}

	CDBG("--CAMERA-- %s ...(End)\n",__func__);
	return rc;
}

static long ov5640_set_antibanding(int antibanding)
{
	long rc = 0;
	//int i = 0;
	CDBG("--CAMERA-- %s ...(Start)\n", __func__);
	CDBG("--CAMERA-- %s ...antibanding = %d\n", __func__, antibanding);

	switch (antibanding)
	{
		case CAMERA_ANTIBANDING_OFF:
			CDBG("--CAMERA--CAMERA_ANTIBANDING_OFF\n");
			break;

		case CAMERA_ANTIBANDING_60HZ:
			CDBG("--CAMERA--CAMERA_ANTIBANDING_60HZ\n");
			rc = OV5640Core_WritePREG(ov5640_antibanding_60z_tbl);
			break;

		case CAMERA_ANTIBANDING_50HZ:
			CDBG("--CAMERA--CAMERA_ANTIBANDING_50HZ\n");
			rc = OV5640Core_WritePREG(ov5640_antibanding_50z_tbl);
			break;

		case CAMERA_ANTIBANDING_AUTO:
			CDBG("--CAMERA--CAMERA_ANTIBANDING_AUTO\n");
			rc = OV5640Core_WritePREG(ov5640_antibanding_auto_tbl);
			break;

		default:
			CDBG_HIGH("--CAMERA--CAMERA_ANTIBANDING_ERROR COMMAND\n");
			break;
	}
	CDBG("--CAMERA-- %s ...(End)\n",__func__);
	return rc;
}

static long ov5640_set_exposure_mode(int mode)
{
	long rc = 0;
	//int i =0;
	CDBG("--CAMERA-- %s ...(Start)\n",__func__);
	CDBG("--CAMERA-- %s ...mode = %d\n",__func__ , mode);
#if 0
	switch (mode)
	{
		case CAMERA_SETAE_AVERAGE:
			CDBG("--CAMERA--CAMERA_SETAE_AVERAGE\n");
			rc = OV5640Core_WritePREG(ov5640_ae_average_tbl);
			break;
		case CAMERA_SETAE_CENWEIGHT:
			CDBG("--CAMERA--CAMERA_SETAE_CENWEIGHT\n");
			rc = OV5640Core_WritePREG(ov5640_ae_centerweight_tbl);
			break;
		default:
			CDBG("--CAMERA--ERROR COMMAND OR NOT SUPPORT\n");
			break;
	}
#endif
	CDBG("--CAMERA-- %s ...(End)\n",__func__);
	return rc;
}

static int32_t ov5640_lens_shading_enable(uint8_t is_enable)
{
	int32_t rc = 0;
	CDBG("--CAMERA--%s: ...(Start). enable = %d\n", __func__, is_enable);

	if(is_enable)
	{
		CDBG("--CAMERA--%s: enable~!!\n", __func__);
		rc = OV5640Core_WritePREG(ov5640_lens_shading_on_tbl);
	}
	else
	{
		CDBG("--CAMERA--%s: disable~!!\n", __func__);
		rc = OV5640Core_WritePREG(ov5640_lens_shading_off_tbl);
	}
	CDBG("--CAMERA--%s: ...(End). rc = %d\n", __func__, rc);
	return rc;
}

static int ov5640_set_wb_oem(uint8_t param)
{
	int rc = 0;
	unsigned int tmp2;
	CDBG("--CAMERA--%s runs\r\n",__func__ );
	CDBG("--CAMERA--WB Type:0x%x\r\n", param);
	ov5640_i2c_read_byte(ov5640_client->addr, 0x350b, &tmp2);
	CDBG("--CAMERA-- GAIN VALUE : %x\n", tmp2);

	switch(param)
	{
		case CAMERA_WB_AUTO:
			CDBG("--CAMERA--CAMERA_WB_AUTO\n");
			rc = OV5640Core_WritePREG(ov5640_wb_def);
			is_awb_enabled = 1;
			break;
		case CAMERA_WB_FLUORESCENT:
			CDBG("--CAMERA--CAMERA_WB_FLUORESCENT\n");
			rc = OV5640Core_WritePREG(ov5640_wb_fluorescent);
			is_awb_enabled = 0;
			break;
		case CAMERA_WB_INCANDESCENT:
			CDBG("--CAMERA--CAMERA_WB_INCANDESCENT\n");
			rc = OV5640Core_WritePREG(ov5640_wb_inc);
			is_awb_enabled = 0;
			break;
		case CAMERA_WB_DAYLIGHT:
			CDBG("--CAMERA--CAMERA_WB_DAYLIGHT\n");
			rc = OV5640Core_WritePREG(ov5640_wb_daylight);
			is_awb_enabled = 0;
			break;
		case CAMERA_WB_CLOUDY_DAYLIGHT:
			CDBG("--CAMERA--CAMERA_WB_CLOUDY_DAYLIGHT\n");
			rc = OV5640Core_WritePREG(ov5640_wb_cloudy);
			is_awb_enabled = 0;
			break;
		default:
			break;
	}
	return rc;
}

static int ov5640_set_touchaec(uint32_t x,uint32_t y)
{
	uint8_t aec_arr[8] = {0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11};
	int idx = 0;
	int i;
	CDBG("--CAMERA--%s x: %d ,y: %d\r\n",__func__ ,x,y);
	idx = x /2 + y *2;
	CDBG("--CAMERA--idx: %d\r\n",idx);

	if (x %2 == 0)
	{
		aec_arr[idx] = 0x10 | 0x0a;
	}
	else
	{
		aec_arr[idx] = 0x01 | 0xa0;
	}

	for (i = 0;i < 8; i++)
	{
		CDBG("--CAMERA--write : %x val : %x ", 0x5688 + i, aec_arr[i]);
		ov5640_i2c_write(ov5640_client->addr, 0x5688 + i, aec_arr[i], 10);
	}

	return 1;
}

//QRD
static int ov5640_set_exposure_compensation(int compensation)
{
	long rc = 0;
	//int i = 0;

	CDBG("--CAMERA-- %s ...(Start)\n",__func__);

	CDBG("--CAMERA-- %s ...exposure_compensation = %d\n",__func__ , compensation);
#if 0
	switch(compensation)
	{
		case CAMERA_EXPOSURE_COMPENSATION_LV0:
			CDBG("--CAMERA--CAMERA_EXPOSURE_COMPENSATION_LV0\n");
			rc = OV5640Core_WritePREG(ov5640_exposure_compensation_lv0_tbl);
			break;
		case CAMERA_EXPOSURE_COMPENSATION_LV1:
			CDBG("--CAMERA--CAMERA_EXPOSURE_COMPENSATION_LV1\n");
			rc = OV5640Core_WritePREG(ov5640_exposure_compensation_lv1_tbl);
			break;
		case CAMERA_EXPOSURE_COMPENSATION_LV2:
			CDBG("--CAMERA--CAMERA_EXPOSURE_COMPENSATION_LV2\n");
			rc = OV5640Core_WritePREG(ov5640_exposure_compensation_lv2_default_tbl);
			break;
		case CAMERA_EXPOSURE_COMPENSATION_LV3:
			CDBG("--CAMERA--CAMERA_EXPOSURE_COMPENSATION_LV3\n");
			rc = OV5640Core_WritePREG(ov5640_exposure_compensation_lv3_tbl);
			break;
		case CAMERA_EXPOSURE_COMPENSATION_LV4:
			CDBG("--CAMERA--CAMERA_EXPOSURE_COMPENSATION_LV3\n");
			rc = OV5640Core_WritePREG(ov5640_exposure_compensation_lv4_tbl);
			break;
		default:
			CDBG_HIGH("--CAMERA--ERROR CAMERA_EXPOSURE_COMPENSATION\n");
			break;
	}
#endif
	switch(compensation)
	{
		case CAMERA_EXPOSURE_COMPENSATION_LV0:
			CDBG("--CAMERA--CAMERA_EXPOSURE_COMPENSATION_LV0\n");
			OV5640_set_EV(1);
			break;
		case CAMERA_EXPOSURE_COMPENSATION_LV1:
			CDBG("--CAMERA--CAMERA_EXPOSURE_COMPENSATION_LV1\n");
			OV5640_set_EV(3);
			break;
		case CAMERA_EXPOSURE_COMPENSATION_LV2:
			CDBG("--CAMERA--CAMERA_EXPOSURE_COMPENSATION_LV2\n");
			OV5640_set_EV(5);
			break;
		case CAMERA_EXPOSURE_COMPENSATION_LV3:
			CDBG("--CAMERA--CAMERA_EXPOSURE_COMPENSATION_LV3\n");
			OV5640_set_EV(7);
			break;
		case CAMERA_EXPOSURE_COMPENSATION_LV4:
			CDBG("--CAMERA--CAMERA_EXPOSURE_COMPENSATION_LV3\n");
			OV5640_set_EV(9);
			break;
		default:
			CDBG_HIGH("--CAMERA--ERROR CAMERA_EXPOSURE_COMPENSATION\n");
			OV5640_set_EV(5);
			break;
	}

	CDBG("--CAMERA-- %s ...(End)\n",__func__);

	return rc;
}

static int ov5640_set_iso(int8_t iso_type)
{
	long rc = 0;
	//int i = 0;

	CDBG("--CAMERA-- %s ...(Start)\n",__func__);

	CDBG("--CAMERA-- %s ...iso_type = %d\n",__func__ , iso_type);

	switch(iso_type)
	{
		case CAMERA_ISO_TYPE_AUTO:
			CDBG("--CAMERA--CAMERA_ISO_TYPE_AUTO\n");
			rc = OV5640Core_WritePREG(ov5640_iso_type_auto);
			break;
		case CAMEAR_ISO_TYPE_HJR:
			CDBG("--CAMERA--CAMEAR_ISO_TYPE_HJR\n");
			rc = OV5640Core_WritePREG(ov5640_iso_type_auto);
			break;
		case CAMEAR_ISO_TYPE_100:
			CDBG("--CAMERA--CAMEAR_ISO_TYPE_100\n");
			rc = OV5640Core_WritePREG(ov5640_iso_type_100);
			break;
		case CAMERA_ISO_TYPE_200:
			CDBG("--CAMERA--CAMERA_ISO_TYPE_200\n");
			rc = OV5640Core_WritePREG(ov5640_iso_type_200);
			break;
		case CAMERA_ISO_TYPE_400:
			CDBG("--CAMERA--CAMERA_ISO_TYPE_400\n");
			rc = OV5640Core_WritePREG(ov5640_iso_type_400);
			break;
		case CAMEAR_ISO_TYPE_800:
			CDBG("--CAMERA--CAMEAR_ISO_TYPE_800\n");
			rc = OV5640Core_WritePREG(ov5640_iso_type_800);
			break;
		case CAMERA_ISO_TYPE_1600:
			CDBG("--CAMERA--CAMERA_ISO_TYPE_1600\n");
			rc = OV5640Core_WritePREG(ov5640_iso_type_1600);
			break;
		default:
			CDBG_HIGH("--CAMERA--ERROR ISO TYPE\n");
			break;
	}

	CDBG("--CAMERA-- %s ...(End)\n",__func__);

	return rc;
}

static int ov5640_auto_led_start(void)
{
	unsigned int tmp;
	int rc = 0;
	CDBG("--CAMERA-- %s (Start...)\n",__func__);
	if (is_autoflash == 1) {
		ov5640_i2c_read_byte(ov5640_client->addr, 0x56a1, &tmp);
		CDBG("--CAMERA-- GAIN VALUE : %d\n", tmp);
		if (tmp < 40) {
			ov5640_set_flash_light(LED_FULL);
			is_autoflash_working = 1;
		}
	}
	return rc;
}

static int ov5640_read_average_luminance(void)
{
	int rc = 0;
	CDBG("--CAMERA-- %s (Start...)\n",__func__);

	current_avg_luminance = OV5640_read_i2c(0x56a1);
	CDBG("--CAMERA-- %s current_avg_luminance = %d \n", __func__, current_avg_luminance);

	return rc;
}

static int ov5640_sensor_start_af(void)
{
	int i;
	unsigned int af_st = 0;
	unsigned int af_ack = 0;
	unsigned int tmp = 0;
	int rc = 0;
	CDBG("--CAMERA-- %s (Start...)\n",__func__);

	ov5640_i2c_read_byte(ov5640_client->addr, OV5640_CMD_FW_STATUS,&af_st);
	CDBG("--CAMERA-- %s af_st = %d\n", __func__, af_st);

	ov5640_i2c_write(ov5640_client->addr, OV5640_CMD_ACK, 0x01, 10);
	ov5640_i2c_write(ov5640_client->addr, OV5640_CMD_MAIN, 0x03, 10);

	for (i = 0; i < 50; i++) {
		ov5640_i2c_read_byte(ov5640_client->addr,OV5640_CMD_ACK,&af_ack);
		if (af_ack == 0)
			break;
		msleep(50);
	}
	CDBG("--CAMERA-- %s af_ack = 0x%x\n", __func__, af_ack);

	//    if(af_ack == 0)
	{
	//        mdelay(1000);
		ov5640_i2c_read_byte(ov5640_client->addr, OV5640_CMD_FW_STATUS,&af_st);
		CDBG("--CAMERA-- %s af_st = %d\n", __func__, af_st);

		if (af_st == 0x10)
		{
			CDBG("--CAMERA-- %s AF ok and release AF setting~!!\n", __func__);
		}
		else {
			CDBG_HIGH("--CAMERA-- %s AF not ready!!\n", __func__);
		}
	}

	//  ov5640_i2c_write(ov5640_client->addr,OV5640_CMD_ACK,0x01,10);
	//  ov5640_i2c_write(ov5640_client->addr,OV5640_CMD_MAIN,0x08,10);
	ov5640_i2c_write(ov5640_client->addr,OV5640_CMD_ACK,0x01,10);
	ov5640_i2c_write(ov5640_client->addr,OV5640_CMD_MAIN,0x07,10);

	for (i = 0; i < 70; i++)
	{
		ov5640_i2c_read_byte(ov5640_client->addr, OV5640_CMD_ACK, &af_ack);
		if (af_ack == 0)
			break;
		msleep(25);
	}

	ov5640_i2c_read_byte(ov5640_client->addr, OV5640_CMD_PARA0, &tmp);
	CDBG("--CAMERA--0x3024 = %x \n", tmp);
	rc = ((tmp == 0) ? 1 : 0);

	ov5640_i2c_read_byte(ov5640_client->addr, OV5640_CMD_PARA1, &tmp);
	CDBG("--CAMERA--0x3025 = %x \n", tmp);
	rc = ((tmp == 0) ? 1 : 0);

	ov5640_i2c_read_byte(ov5640_client->addr, OV5640_CMD_PARA2, &tmp);
	CDBG("--CAMERA--0x3026 = %x \n", tmp);
	rc = ((tmp == 0) ? 1 : 0);

	ov5640_i2c_read_byte(ov5640_client->addr, OV5640_CMD_PARA3, &tmp);
	CDBG("--CAMERA--0x3027 = %x \n", tmp);
	rc = ((tmp == 0) ? 1 : 0) ;

	ov5640_i2c_read_byte(ov5640_client->addr, OV5640_CMD_PARA4, &tmp);
	CDBG("--CAMERA--0x3028 = %x \n", tmp);
	rc = ((tmp == 0) ? 1 : 0) ;

	CDBG("--CAMERA-- %s rc = %d(End...)\n", __func__, rc);
	return rc;
}

static int ov5640_sensor_config(void __user *argp)
{
	struct sensor_cfg_data cdata;
	long rc = 0;

	if (copy_from_user(&cdata, (void *)argp, sizeof(struct sensor_cfg_data)))
		return -EFAULT;

	CDBG("--CAMERA-- %s %d\n",__func__,cdata.cfgtype);

	mutex_lock(&ov5640_mutex);
	switch (cdata.cfgtype)
	{
		case CFG_SET_MODE:
			rc =ov5640_set_sensor_mode(cdata.mode, cdata.rs);
			break;
		case CFG_SET_EFFECT:
			CDBG("--CAMERA-- CFG_SET_EFFECT mode=%d, effect = %d !!\n",cdata.mode, cdata.cfg.effect);
			rc = ov5640_set_effect(cdata.mode, cdata.cfg.effect);
			break;
		case CFG_START:
			CDBG("--CAMERA-- CFG_START (Not Support) !!\n");
			// Not Support
			break;
		case CFG_PWR_UP:
			CDBG("--CAMERA-- CFG_PWR_UP (Not Support) !!\n");
			// Not Support
			break;
		case CFG_PWR_DOWN:
			CDBG("--CAMERA-- CFG_PWR_DOWN (Not Support) \n");
			ov5640_power_off();
			break;
		case CFG_SET_DEFAULT_FOCUS:
			CDBG("--CAMERA-- CFG_SET_DEFAULT_FOCUS (Not Implement) !!\n");
			break;
		case CFG_MOVE_FOCUS:
			CDBG("--CAMERA-- CFG_MOVE_FOCUS (Not Implement) !!\n");
			break;
		case CFG_SET_BRIGHTNESS:
			CDBG("--CAMERA-- CFG_SET_BRIGHTNESS  !!\n");
			rc = ov5640_set_brightness(cdata.cfg.brightness);
			break;
		case CFG_SET_CONTRAST:
			CDBG("--CAMERA-- CFG_SET_CONTRAST  !!\n");
			rc = ov5640_set_contrast(cdata.cfg.contrast);
			break;
		case CFG_SET_EXPOSURE_MODE:
			CDBG("--CAMERA-- CFG_SET_EXPOSURE_MODE !!\n");
			rc = ov5640_set_exposure_mode(cdata.cfg.ae_mode);
			break;
		case CFG_SET_ANTIBANDING:
			CDBG("--CAMERA-- CFG_SET_ANTIBANDING antibanding = %d!!\n", cdata.cfg.antibanding);
			rc = ov5640_set_antibanding(cdata.cfg.antibanding);
			break;
		case CFG_SET_LENS_SHADING:
			CDBG("--CAMERA-- CFG_SET_LENS_SHADING !!\n");
			rc = ov5640_lens_shading_enable(cdata.cfg.lens_shading);
			break;
		case CFG_SET_SATURATION:
			CDBG("--CAMERA-- CFG_SET_SATURATION !!\n");
			rc = ov5640_set_saturation(cdata.cfg.saturation);
			break;
		case CFG_SET_SHARPNESS:
			CDBG("--CAMERA-- CFG_SET_SHARPNESS !!\n");
			rc = ov5640_set_sharpness(cdata.cfg.sharpness);
			break;
		case CFG_SET_WB:
			CDBG("--CAMERA-- CFG_SET_WB!!\n");
			ov5640_set_wb_oem(cdata.cfg.wb_val);
			rc = 0;
			break;
		case CFG_SET_TOUCHAEC:
			CDBG("--CAMERA-- CFG_SET_TOUCHAEC!!\n");
			ov5640_set_touchaec(cdata.cfg.aec_cord.x, cdata.cfg.aec_cord.y);
			rc = 0;
			break;
		case CFG_SET_AUTO_FOCUS:
			CDBG("--CAMERA-- CFG_SET_AUTO_FOCUS !\n");
			rc = ov5640_sensor_start_af();
			break;
		case CFG_SET_AUTOFLASH:
			CDBG("--CAMERA-- CFG_SET_AUTOFLASH !\n");
			is_autoflash = cdata.cfg.is_autoflash;
			CDBG("--CAMERA-- is autoflash %d\r\n",is_autoflash);
			rc = 0;
			break;
		case CFG_SET_EXPOSURE_COMPENSATION:
			CDBG("--CAMERA-- CFG_SET_EXPOSURE_COMPENSATION !\n");
			rc = ov5640_set_exposure_compensation(cdata.cfg.exp_compensation);
			break;
		case CFG_SET_ISO:
			CDBG("--CAMERA-- CFG_SET_ISO !\n");
			rc = ov5640_set_iso(cdata.cfg.iso_type);
			break;
		case CFG_SET_AUTO_LED_START:
			CDBG("--CAMERA-- CFG_SET_AUTO_LED_START !\n");
			rc = ov5640_auto_led_start();
			break;
		case CFG_READ_AVG_LUMINANCE:
			CDBG("--CAMERA-- CFG_READ_AVG_LUMINANCE !\n");
			rc = ov5640_read_average_luminance();
			break;
		default:
			CDBG_HIGH("--CAMERA-- %s: Command=%d (Not Implement)!!\n",__func__,cdata.cfgtype);
			rc = -EINVAL;
			break;
	}
	mutex_unlock(&ov5640_mutex);
	return rc;
}

static struct i2c_driver ov5640_i2c_driver = {
	.id_table = ov5640_i2c_id,
	.probe  = ov5640_i2c_probe,
	.remove = ov5640_i2c_remove,
	.driver = {
		.name = "ov5640",
	},
};

static int ov5640_probe_init_gpio(const struct msm_camera_sensor_info *data)
{
	int rc = 0;

	CDBG("--CAMERA-- %s\n",__func__);

	ov5640_pwdn_gpio = data->sensor_pwd;
	ov5640_reset_gpio = data->sensor_reset;
	ov5640_driver_pwdn_gpio = data->vcm_pwd ;


	if (data->vcm_enable)
	{
		gpio_direction_output(data->vcm_pwd, 1);
	}

	gpio_direction_output(data->sensor_reset, 1);
	gpio_direction_output(data->sensor_pwd, 1);

	return rc;

}

static int ov5640_sensor_probe(const struct msm_camera_sensor_info *info,struct msm_sensor_ctrl *s)
{
	int rc = -ENOTSUPP;
	CDBG("--CAMERA-- %s (Start...)\n",__func__);
	rc = i2c_add_driver(&ov5640_i2c_driver);
	CDBG("--CAMERA-- i2c_add_driver ret:0x%x,ov5640_client=0x%x\n",
	rc, (unsigned int)ov5640_client);
	if ((rc < 0 ) || (ov5640_client == NULL))
	{
		CDBG_HIGH("--CAMERA-- i2c_add_driver FAILS!!\n");

		if(NULL != ov5640_sensorw)
		{
			kfree(ov5640_sensorw);
			ov5640_sensorw = NULL;
		}

		ov5640_client = NULL;
		return rc;
	}

	lcd_camera_power_onoff(1);

	rc = ov5640_probe_init_gpio(info);
	if (rc < 0) {
		CDBG_HIGH("--CAMERA-- %s, init gpio failed\n",__func__);
		i2c_del_driver(&ov5640_i2c_driver);
		return rc;
	}

	ov5640_power_off();

	/* SENSOR NEED MCLK TO DO I2C COMMUNICTION, OPEN CLK FIRST*/
	msm_camio_clk_rate_set(24000000);

	msleep(5);

	ov5640_power_on();
	ov5640_power_reset();

	rc = ov5640_probe_readID(info);

	if (rc < 0)
	{
		CDBG_HIGH("--CAMERA--ov5640_probe_readID Fail !!~~~~!!\n");
		CDBG_HIGH("--CAMERA-- %s, unregister\n",__func__);

		i2c_del_driver(&ov5640_i2c_driver);
		ov5640_power_off();
		lcd_camera_power_onoff(0);

		return rc;
	}

	s->s_init = ov5640_sensor_open_init;
	s->s_release = ov5640_sensor_release;
	s->s_config  = ov5640_sensor_config;
	//s->s_AF = ov5640_sensor_set_af;
	//camera_init_flag = true;

	s->s_camera_type = BACK_CAMERA_2D;
	s->s_mount_angle = info->sensor_platform_info->mount_angle;

	ov5640_power_off();
	lcd_camera_power_onoff(0);

	CDBG("--CAMERA-- %s (End...)\n",__func__);
	return rc;
}

static int ov5640_i2c_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
	CDBG("--CAMERA-- %s ... (Start...)\n",__func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
	{
		CDBG_HIGH("--CAMERA--i2c_check_functionality failed\n");
		return -ENOMEM;
	}

	ov5640_sensorw = kzalloc(sizeof(struct ov5640_work), GFP_KERNEL);
	if (!ov5640_sensorw)
	{
		CDBG_HIGH("--CAMERA--kzalloc failed\n");
		return -ENOMEM;
	}
	i2c_set_clientdata(client, ov5640_sensorw);
	ov5640_init_client(client);
	ov5640_client = client;

	CDBG("--CAMERA-- %s ... (End...)\n",__func__);
	return 0;
}

static int __ov5640_probe(struct platform_device *pdev)
{
	return msm_camera_drv_start(pdev, ov5640_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = __ov5640_probe,
	.driver = {
		.name = "msm_camera_ov5640",
		.owner = THIS_MODULE,
	},
};

static int __init ov5640_init(void)
{
	ov5640_i2c_buf[0]=0x5A;
	return platform_driver_register(&msm_camera_driver);
}

module_init(ov5640_init);

MODULE_DESCRIPTION("OV5640 YUV MIPI sensor driver");
MODULE_LICENSE("GPL v2");
