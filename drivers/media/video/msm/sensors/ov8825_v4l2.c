/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

//zxj ++
#include "../actuators/msm_actuator.h"
//zxj --
#include "msm_sensor.h"
#include "msm.h"
#define SENSOR_NAME "ov8825"
#define PLATFORM_DRIVER_NAME "msm_camera_ov8825"
#define ov8825_obj ov8825_##obj

#ifdef CDBG
#undef CDBG
#endif
#ifdef CDBG_HIGH
#undef CDBG_HIGH
#endif

#define OV8825_DGB

#ifdef OV8825_DGB
#define CDBG(fmt, args...) printk(fmt, ##args)
#define CDBG_HIGH(fmt, args...) printk(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#define CDBG_HIGH(fmt, args...) printk(fmt, ##args)
#endif

/* TO DO - Currently ov5647 typical values are used
 * Need to get the exact values */
#define OV8825_RG_RATIO_TYPICAL_VALUE 64 /* R/G of typical camera module */
#define OV8825_BG_RATIO_TYPICAL_VALUE 105 /* B/G of typical camera module */

#define return_if_val_fail(p, c) if(!(p)){printk("##### %s line%d: PROGRAM_INVALID\n",__func__,__LINE__); return c;}
//zxj ++
#if 0
#define OV8825_AF_MSB 0x3619
#define OV8825_AF_LSB 0x3618

uint16_t ov8825_damping_threshold = 10;
int8_t S3_to_0 = 0x01;
#endif
//zxj --

//extern int lcd_camera_power_onoff(int on);
extern int camera_power_onoff(int on);

//add by lzj
static int32_t vfe_clk = 266667000;
// add end

DEFINE_MUTEX(ov8825_mut);
static struct msm_sensor_ctrl_t ov8825_s_ctrl;

struct otp_struct {
	uint8_t customer_id;
	uint8_t module_integrator_id;
	uint8_t lens_id;
	uint8_t rg_ratio;
	uint8_t bg_ratio;
	uint8_t user_data[5];
} st_ov8825_otp;

static struct msm_camera_i2c_reg_conf ov8825_start_settings[] = {
	{0x0100, 0x01},
};

static struct msm_camera_i2c_reg_conf ov8825_stop_settings[] = {
	{0x0100, 0x00},
};

static struct msm_camera_i2c_reg_conf ov8825_groupon_settings[] = {
	{0x3208, 0x00},
};

static struct msm_camera_i2c_reg_conf ov8825_groupoff_settings[] = {
	{0x3208, 0x10},
	{0x3208, 0xA0},
};

static struct msm_camera_i2c_reg_conf ov8825_prev_settings[] = {
	{0x3003, 0xce}, /*PLL_CTRL0*/
	{0x3004, 0xd4}, /*PLL_CTRL1*/
	{0x3005, 0x00}, /*PLL_CTRL2*/
	{0x3006, 0x10}, /*PLL_CTRL3*/
	{0x3007, 0x3b}, /*PLL_CTRL4*/
	{0x3011, 0x01}, /*MIPI_Lane_4_Lane*/
	{0x3012, 0x80}, /*SC_PLL CTRL_S0*/
	{0x3013, 0x39}, /*SC_PLL CTRL_S1*/
	{0x3104, 0x20}, /*SCCB_PLL*/
	{0x3106, 0x15}, /*SRB_CTRL*/
	{0x3501, 0x4e}, /*AEC_HIGH*/
	{0x3502, 0xa0}, /*AEC_LOW*/
	{0x350b, 0x1f}, /*AGC*/
	{0x3600, 0x06}, /*ANACTRL0*/
	{0x3601, 0x34}, /*ANACTRL1*/
	{0x3700, 0x20}, /*SENCTROL0 Sensor control*/
	{0x3702, 0x50}, /*SENCTROL2 Sensor control*/
	{0x3703, 0xcc}, /*SENCTROL3 Sensor control*/
	{0x3704, 0x19}, /*SENCTROL4 Sensor control*/
	{0x3705, 0x14}, /*SENCTROL5 Sensor control*/
	{0x3706, 0x4b}, /*SENCTROL6 Sensor control*/
	{0x3707, 0x63}, /*SENCTROL7 Sensor control*/
	{0x3708, 0x84}, /*SENCTROL8 Sensor control*/
	{0x3709, 0x40}, /*SENCTROL9 Sensor control*/
	{0x370a, 0x12}, /*SENCTROLA Sensor control*/
	{0x370e, 0x00}, /*SENCTROLE Sensor control*/
	{0x3711, 0x0f}, /*SENCTROL11 Sensor control*/
	{0x3712, 0x9c}, /*SENCTROL12 Sensor control*/
	{0x3724, 0x01}, /*Reserved*/
	{0x3725, 0x92}, /*Reserved*/
	{0x3726, 0x01}, /*Reserved*/
	{0x3727, 0xa9}, /*Reserved*/
	{0x3800, 0x00}, /*HS(HREF start High)*/
	{0x3801, 0x00}, /*HS(HREF start Low)*/
	{0x3802, 0x00}, /*VS(Vertical start High)*/
	{0x3803, 0x00}, /*VS(Vertical start Low)*/
	{0x3804, 0x0c}, /*HW = 3295*/
	{0x3805, 0xdf}, /*HW*/
	{0x3806, 0x09}, /*VH = 2459*/
	{0x3807, 0x9b}, /*VH*/
	{0x3808, 0x06}, /*ISPHO = 1632*/
	{0x3809, 0x60}, /*ISPHO*/
	{0x380a, 0x04}, /*ISPVO = 1224*/
	{0x380b, 0xc8}, /*ISPVO*/
	{0x380c, 0x0d}, /*HTS = 3516*/
	{0x380d, 0xbc}, /*HTS*/
	{0x380e, 0x04}, /*VTS = 1264*/
	{0x380f, 0xf0}, /*VTS*/
	{0x3810, 0x00}, /*HOFF = 8*/
	{0x3811, 0x08}, /*HOFF*/
	{0x3812, 0x00}, /*VOFF = 4*/
	{0x3813, 0x04}, /*VOFF*/
	{0x3814, 0x31}, /*X INC*/
	{0x3815, 0x31}, /*Y INC*/
	{0x3820, 0x81}, /*Timing Reg20:Vflip*/
	{0x3821, 0x17}, /*Timing Reg21:Hmirror*/
	{0x3f00, 0x00}, /*PSRAM Ctrl0*/
	{0x3f01, 0xfc}, /*PSRAM Ctrl1*/
	{0x3f05, 0x10}, /*PSRAM Ctrl5*/
	{0x4600, 0x04}, /*VFIFO Ctrl0*/
	{0x4601, 0x00}, /*VFIFO Read ST High*/
	{0x4602, 0x30}, /*VFIFO Read ST Low*/
	{0x4837, 0x28}, /*MIPI PCLK PERIOD*/
	{0x5068, 0x00}, /*HSCALE_CTRL*/
	{0x506a, 0x00}, /*VSCALE_CTRL*/
	{0x5c00, 0x80}, /*PBLC CTRL00*/
	{0x5c01, 0x00}, /*PBLC CTRL01*/
	{0x5c02, 0x00}, /*PBLC CTRL02*/
	{0x5c03, 0x00}, /*PBLC CTRL03*/
	{0x5c04, 0x00}, /*PBLC CTRL04*/
	{0x5c08, 0x10}, /*PBLC CTRL08*/
	{0x6900, 0x61}, /*CADC CTRL00*/
};

static struct msm_camera_i2c_reg_conf ov8825_snap_settings[] = {
	{0x3003, 0xce}, /*PLL_CTRL0*/
	{0x3004, 0xd8}, /*PLL_CTRL1*/
	{0x3005, 0x00}, /*PLL_CTRL2*/
	{0x3006, 0x10}, /*PLL_CTRL3*/
	{0x3007, 0x3b}, /*PLL_CTRL4*/
	{0x3011, 0x01}, /*MIPI_Lane_4_Lane*/
	{0x3012, 0x81}, /*SC_PLL CTRL_S0*/
	{0x3013, 0x39}, /*SC_PLL CTRL_S1*/
	{0x3104, 0x20}, /*SCCB_PLL*/
	{0x3106, 0x11}, /*SRB_CTRL*/
	{0x3501, 0x9a}, /*AEC_HIGH*/
	{0x3502, 0xa0}, /*AEC_LOW*/
	{0x350b, 0x1f}, /*AGC*/
	{0x3600, 0x07}, /*ANACTRL0*/
	{0x3601, 0x33}, /*ANACTRL1*/
	{0x3700, 0x10}, /*SENCTROL0 Sensor control*/
	{0x3702, 0x28}, /*SENCTROL2 Sensor control*/
	{0x3703, 0x6c}, /*SENCTROL3 Sensor control*/
	{0x3704, 0x8d}, /*SENCTROL4 Sensor control*/
	{0x3705, 0x0a}, /*SENCTROL5 Sensor control*/
	{0x3706, 0x27}, /*SENCTROL6 Sensor control*/
	{0x3707, 0x63}, /*SENCTROL7 Sensor control*/
	{0x3708, 0x40}, /*SENCTROL8 Sensor control*/
	{0x3709, 0x20}, /*SENCTROL9 Sensor control*/
	{0x370a, 0x12}, /*SENCTROLA Sensor control*/
	{0x370e, 0x00}, /*SENCTROLE Sensor control*/
	{0x3711, 0x07}, /*SENCTROL11 Sensor control*/
	{0x3712, 0x4e}, /*SENCTROL12 Sensor control*/
	{0x3724, 0x00}, /*Reserved*/
	{0x3725, 0xd4}, /*Reserved*/
	{0x3726, 0x00}, /*Reserved*/
	{0x3727, 0xe1}, /*Reserved*/
	{0x3800, 0x00}, /*HS(HREF start High)*/
	{0x3801, 0x00}, /*HS(HREF start Low)*/
	{0x3802, 0x00}, /*VS(Vertical start Hgh)*/
	{0x3803, 0x00}, /*VS(Vertical start Low)*/
	{0x3804, 0x0c}, /*HW = 3295*/
	{0x3805, 0xdf}, /*HW*/
	{0x3806, 0x09}, /*VH = 2459*/
	{0x3807, 0x9b}, /*VH*/
	{0x3808, 0x0c}, /*ISPHO = 1632*/
	{0x3809, 0xc0}, /*ISPHO*/
	{0x380a, 0x09}, /*ISPVO = 1224*/
	{0x380b, 0x90}, /*ISPVO*/
	{0x380c, 0x0e}, /*HTS = 3516*/
	{0x380d, 0x00}, /*HTS*/
	{0x380e, 0x09}, /*VTS = 1264*/
	{0x380f, 0xb0}, /*VTS*/
	{0x3810, 0x00}, /*HOFF = 8*/
	{0x3811, 0x10}, /*HOFF*/
	{0x3812, 0x00}, /*VOFF = 4*/
	{0x3813, 0x06}, /*VOFF*/
	{0x3814, 0x11}, /*X INC*/
	{0x3815, 0x11}, /*Y INC*/
	{0x3820, 0x80}, /*Timing Reg20:Vflip*/
	{0x3821, 0x16}, /*Timing Reg21:Hmirror*/
	{0x3f00, 0x02}, /*PSRAM Ctrl0*/
	{0x3f01, 0xfc}, /*PSRAM Ctrl1*/
	{0x3f05, 0x10}, /*PSRAM Ctrl5*/
	{0x4600, 0x04}, /*VFIFO Ctrl0*/
	{0x4601, 0x00}, /*VFIFO Read ST High*/
	{0x4602, 0x78}, /*VFIFO Read ST Low*/
	{0x4837, 0x28}, /*MIPI PCLK PERIOD*/
	{0x5068, 0x00}, /*HSCALE_CTRL*/
	{0x506a, 0x00}, /*VSCALE_CTRL*/
	{0x5c00, 0x80}, /*PBLC CTRL00*/
	{0x5c01, 0x00}, /*PBLC CTRL01*/
	{0x5c02, 0x00}, /*PBLC CTRL02*/
	{0x5c03, 0x00}, /*PBLC CTRL03*/
	{0x5c04, 0x00}, /*PBLC CTRL04*/
	{0x5c08, 0x10}, /*PBLC CTRL08*/
	{0x6900, 0x61}, /*CADC CTRL00*/
};


static struct msm_camera_i2c_reg_conf ov8825_reset_settings[] = {
	{0x0103, 0x01},
};

static struct msm_camera_i2c_reg_conf ov8825_recommend_settings[] = {
	{0x3000, 0x16},
	{0x3001, 0x00},
	{0x3002, 0x6c},
	{0x300d, 0x00},
	{0x301f, 0x09},
	{0x3010, 0x00},
	{0x3018, 0x00},
	{0x3300, 0x00},
	{0x3500, 0x00},
	{0x3503, 0x07},
	{0x3509, 0x00},
	{0x3602, 0x42},
	{0x3603, 0x5c},
	{0x3604, 0x98},
	{0x3605, 0xf5},
	{0x3609, 0xb4},
	{0x360a, 0x7c},
	{0x360b, 0xc9},
	{0x360c, 0x0b},
	{0x3612, 0x00},
	{0x3613, 0x02},
	{0x3614, 0x0f},
	{0x3615, 0x00},
	{0x3616, 0x03},
	{0x3617, 0xa1},
	{0x3618, 0x00},
	{0x3619, 0x00},
	{0x361a, 0xB0},
	{0x361b, 0x04},
	{0x361c, 0x07},
	{0x3701, 0x44},
	{0x370b, 0x01},
	{0x370c, 0x50},
	{0x370d, 0x00},
	{0x3816, 0x02},
	{0x3817, 0x40},
	{0x3818, 0x00},
	{0x3819, 0x40},
	{0x3b1f, 0x00},
	{0x3d00, 0x00},
	{0x3d01, 0x00},
	{0x3d02, 0x00},
	{0x3d03, 0x00},
	{0x3d04, 0x00},
	{0x3d05, 0x00},
	{0x3d06, 0x00},
	{0x3d07, 0x00},
	{0x3d08, 0x00},
	{0x3d09, 0x00},
	{0x3d0a, 0x00},
	{0x3d0b, 0x00},
	{0x3d0c, 0x00},
	{0x3d0d, 0x00},
	{0x3d0e, 0x00},
	{0x3d0f, 0x00},
	{0x3d10, 0x00},
	{0x3d11, 0x00},
	{0x3d12, 0x00},
	{0x3d13, 0x00},
	{0x3d14, 0x00},
	{0x3d15, 0x00},
	{0x3d16, 0x00},
	{0x3d17, 0x00},
	{0x3d18, 0x00},
	{0x3d19, 0x00},
	{0x3d1a, 0x00},
	{0x3d1b, 0x00},
	{0x3d1c, 0x00},
	{0x3d1d, 0x00},
	{0x3d1e, 0x00},
	{0x3d1f, 0x00},
	{0x3d80, 0x00},
	{0x3d81, 0x00},
	{0x3d84, 0x00},
	{0x3f06, 0x00},
	{0x3f07, 0x00},
	{0x4000, 0x29},
	{0x4001, 0x02},
	{0x4002, 0x45},
	{0x4003, 0x08},
	{0x4004, 0x04},
	{0x4005, 0x18},
	{0x4300, 0xff},
	{0x4303, 0x00},
	{0x4304, 0x08},
	{0x4307, 0x00},
	{0x4800, 0x04},
	{0x4801, 0x0f},
	{0x4843, 0x02},
	{0x5000, 0x06},
	{0x5001, 0x00},
	{0x5002, 0x00},
	{0x501f, 0x00},
	{0x5780, 0xfc},
	{0x5c05, 0x00},
	{0x5c06, 0x00},
	{0x5c07, 0x80},
	{0x6700, 0x05},
	{0x6701, 0x19},
	{0x6702, 0xfd},
	{0x6703, 0xd7},
	{0x6704, 0xff},
	{0x6705, 0xff},
	{0x6800, 0x10},
	{0x6801, 0x02},
	{0x6802, 0x90},
	{0x6803, 0x10},
	{0x6804, 0x59},
	{0x6901, 0x04},
	{0x5800, 0x0f},
	{0x5801, 0x0d},
	{0x5802, 0x09},
	{0x5803, 0x0a},
	{0x5804, 0x0d},
	{0x5805, 0x14},
	{0x5806, 0x0a},
	{0x5807, 0x04},
	{0x5808, 0x03},
	{0x5809, 0x03},
	{0x580a, 0x05},
	{0x580b, 0x0a},
	{0x580c, 0x05},
	{0x580d, 0x02},
	{0x580e, 0x00},
	{0x580f, 0x00},
	{0x5810, 0x03},
	{0x5811, 0x05},
	{0x5812, 0x09},
	{0x5813, 0x03},
	{0x5814, 0x01},
	{0x5815, 0x01},
	{0x5816, 0x04},
	{0x5817, 0x09},
	{0x5818, 0x09},
	{0x5819, 0x08},
	{0x581a, 0x06},
	{0x581b, 0x06},
	{0x581c, 0x08},
	{0x581d, 0x06},
	{0x581e, 0x33},
	{0x581f, 0x11},
	{0x5820, 0x0e},
	{0x5821, 0x0f},
	{0x5822, 0x11},
	{0x5823, 0x3f},
	{0x5824, 0x08},
	{0x5825, 0x46},
	{0x5826, 0x46},
	{0x5827, 0x46},
	{0x5828, 0x46},
	{0x5829, 0x46},
	{0x582a, 0x42},
	{0x582b, 0x42},
	{0x582c, 0x44},
	{0x582d, 0x46},
	{0x582e, 0x46},
	{0x582f, 0x60},
	{0x5830, 0x62},
	{0x5831, 0x42},
	{0x5832, 0x46},
	{0x5833, 0x46},
	{0x5834, 0x44},
	{0x5835, 0x44},
	{0x5836, 0x44},
	{0x5837, 0x48},
	{0x5838, 0x28},
	{0x5839, 0x46},
	{0x583a, 0x48},
	{0x583b, 0x68},
	{0x583c, 0x28},
	{0x583d, 0xae},
	{0x5842, 0x00},
	{0x5843, 0xef},
	{0x5844, 0x01},
	{0x5845, 0x3f},
	{0x5846, 0x01},
	{0x5847, 0x3f},
	{0x5848, 0x00},
	{0x5849, 0xd5},
	{0x3503, 0x07},
	{0x3500, 0x00},
	{0x3501, 0x27},
	{0x3502, 0x00},
	{0x350b, 0xff},
	{0x3400, 0x04},
	{0x3401, 0x00},
	{0x3402, 0x04},
	{0x3403, 0x00},
	{0x3404, 0x04},
	{0x3405, 0x00},
	{0x3406, 0x01},
	{0x5001, 0x01},
	{0x5000, 0x86},/* enable lens compensation and dpc */
	/* LENC setting 70% */
	{0x5800, 0x21},
	{0x5801, 0x10},
	{0x5802, 0x09},
	{0x5803, 0x0a},
	{0x5804, 0x0f},
	{0x5805, 0x23},
	{0x5806, 0x08},
	{0x5807, 0x04},
	{0x5808, 0x04},
	{0x5809, 0x04},
	{0x580a, 0x04},
	{0x580b, 0x0a},
	{0x580c, 0x04},
	{0x580d, 0x02},
	{0x580e, 0x00},
	{0x580f, 0x00},
	{0x5810, 0x03},
	{0x5811, 0x06},
	{0x5812, 0x05},
	{0x5813, 0x02},
	{0x5814, 0x00},
	{0x5815, 0x00},
	{0x5816, 0x03},
	{0x5817, 0x06},
	{0x5818, 0x09},
	{0x5819, 0x05},
	{0x581a, 0x04},
	{0x581b, 0x04},
	{0x581c, 0x05},
	{0x581d, 0x0a},
	{0x581e, 0x24},
	{0x581f, 0x11},
	{0x5820, 0x0a},
	{0x5821, 0x0a},
	{0x5822, 0x10},
	{0x5823, 0x27},
	{0x5824, 0x2a},
	{0x5825, 0x58},
	{0x5826, 0x28},
	{0x5827, 0x28},
	{0x5828, 0x28},
	{0x5829, 0x28},
	{0x582a, 0x46},
	{0x582b, 0x44},
	{0x582c, 0x46},
	{0x582d, 0x46},
	{0x582e, 0x28},
	{0x582f, 0x62},
	{0x5830, 0x60},
	{0x5831, 0x42},
	{0x5832, 0x28},
	{0x5833, 0x48},
	{0x5834, 0x46},
	{0x5835, 0x46},
	{0x5836, 0x26},
	{0x5837, 0x46},
	{0x5838, 0x28},
	{0x5839, 0x48},
	{0x583a, 0x28},
	{0x583b, 0x28},
	{0x583c, 0x26},
	{0x583d, 0x9d},
};

static struct v4l2_subdev_info ov8825_subdev_info[] = {
	{
		.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt    = 1,
		.order    = 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_camera_i2c_conf_array ov8825_init_conf[] = {
	{&ov8825_reset_settings[0],
	ARRAY_SIZE(ov8825_reset_settings), 50, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov8825_recommend_settings[0],
	ARRAY_SIZE(ov8825_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA}
};

static struct msm_camera_i2c_conf_array ov8825_confs[] = {
	{&ov8825_snap_settings[0],
	ARRAY_SIZE(ov8825_snap_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov8825_prev_settings[0],
	ARRAY_SIZE(ov8825_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_sensor_output_info_t ov8825_dimensions[] = {
	{
		.x_output = 0xCC0,
		.y_output = 0x990,
		.line_length_pclk = 0xE00,
		.frame_length_lines = 0x9B0,
		.vt_pixel_clk = 133400000,
		.op_pixel_clk = 176000000,
		.binning_factor = 1,
	},
	{
		.x_output = 0x660,
		.y_output = 0x4C8,
		.line_length_pclk = 0x6DE,
		.frame_length_lines = 0x505,
		.vt_pixel_clk = 66700000,
		.op_pixel_clk = 88000000,
		.binning_factor = 2,
	},
};

static struct msm_camera_csi_params ov8825_csi_params = {
	.data_format = CSI_10BIT,
	.lane_cnt    = 2,
	.lane_assign = 0xe4,
	.dpcm_scheme = 0,
	.settle_cnt  = 14,
};

static struct msm_camera_csi_params *ov8825_csi_params_array[] = {
	&ov8825_csi_params,
	&ov8825_csi_params,
};

static struct msm_sensor_output_reg_addr_t ov8825_reg_addr = {
	.x_output = 0x3808,
	.y_output = 0x380a,
	.line_length_pclk = 0x380c,
	.frame_length_lines = 0x380e,
};

static struct msm_sensor_id_info_t ov8825_id_info = {
	.sensor_id_reg_addr = 0x300A,
	.sensor_id = 0x8825,
};

static struct msm_sensor_exp_gain_info_t ov8825_exp_gain_info = {
	.coarse_int_time_addr = 0x3501,
	.global_gain_addr = 0x350A,
	.vert_offset = 6,
};

/********************************************
 * index: index of otp group. (0, 1, 2)
 * return value:
 *     0, group index is empty
 *     1, group index has invalid data
 *     2, group index has valid data
 **********************************************/
uint16_t ov8825_check_otp_wb(struct msm_sensor_ctrl_t *s_ctrl, uint16_t index)
{
	uint16_t temp, i;
	uint16_t address;

	/* clear otp buffer */

	/* select otp bank 0 */
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3d84, 0x08,
			MSM_CAMERA_I2C_BYTE_DATA);

	/* load otp into buffer */
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3d81, 0x01,
			MSM_CAMERA_I2C_BYTE_DATA);

	/* read from group [index] */
	address = 0x3d05 + index * 9;
	msm_camera_i2c_read(s_ctrl->sensor_i2c_client, address, &temp,
			MSM_CAMERA_I2C_BYTE_DATA);

	/* disable otp read */
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3d81, 0x00,
			MSM_CAMERA_I2C_BYTE_DATA);

	/* clear otp buffer */
	for (i = 0; i < 32; i++) {
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, (0x3d00+i),
				0x00, MSM_CAMERA_I2C_BYTE_DATA);
	}

	if (!temp)
		return 0;
	else if ((!(temp & 0x80)) && (temp & 0x7f))
		return 2;
	else
		return 1;
}

void ov8825_read_otp_wb(struct msm_sensor_ctrl_t *s_ctrl,
		uint16_t index, struct otp_struct *potp)
{
	uint16_t temp, i;
	uint16_t address;

	/* select otp bank 0 */
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3d84, 0x08,
			MSM_CAMERA_I2C_BYTE_DATA);

	/* load otp data into buffer */
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3d81, 0x01,
			MSM_CAMERA_I2C_BYTE_DATA);

	/* read otp data from 0x3d00 - 0x3d1f*/
	address = 0x3d05 + index * 9;

	msm_camera_i2c_read(s_ctrl->sensor_i2c_client, address, &temp,
			MSM_CAMERA_I2C_BYTE_DATA);

	potp->module_integrator_id = temp;
	potp->customer_id = temp & 0x7f;

	msm_camera_i2c_read(s_ctrl->sensor_i2c_client, (address+1), &temp,
			MSM_CAMERA_I2C_BYTE_DATA);
	potp->lens_id = temp;

	msm_camera_i2c_read(s_ctrl->sensor_i2c_client, (address+2), &temp,
			MSM_CAMERA_I2C_BYTE_DATA);
	potp->rg_ratio = temp;

	msm_camera_i2c_read(s_ctrl->sensor_i2c_client, (address+3), &temp,
			MSM_CAMERA_I2C_BYTE_DATA);
	potp->bg_ratio = temp;

	msm_camera_i2c_read(s_ctrl->sensor_i2c_client, (address+4), &temp,
			MSM_CAMERA_I2C_BYTE_DATA);
	potp->user_data[0] = temp;

	msm_camera_i2c_read(s_ctrl->sensor_i2c_client, (address+5), &temp,
			MSM_CAMERA_I2C_BYTE_DATA);
	potp->user_data[1] = temp;

	msm_camera_i2c_read(s_ctrl->sensor_i2c_client, (address+6), &temp,
			MSM_CAMERA_I2C_BYTE_DATA);
	potp->user_data[2] = temp;

	msm_camera_i2c_read(s_ctrl->sensor_i2c_client, (address+7), &temp,
			MSM_CAMERA_I2C_BYTE_DATA);
	potp->user_data[3] = temp;

	msm_camera_i2c_read(s_ctrl->sensor_i2c_client, (address+8), &temp,
			MSM_CAMERA_I2C_BYTE_DATA);
	potp->user_data[4] = temp;

	CDBG("%s customer_id  = 0x%02x\r\n", __func__, potp->customer_id);
	CDBG("%s lens_id      = 0x%02x\r\n", __func__, potp->lens_id);
	CDBG("%s rg_ratio     = 0x%02x\r\n", __func__, potp->rg_ratio);
	CDBG("%s bg_ratio     = 0x%02x\r\n", __func__, potp->bg_ratio);
	CDBG("%s user_data[0] = 0x%02x\r\n", __func__, potp->user_data[0]);
	CDBG("%s user_data[1] = 0x%02x\r\n", __func__, potp->user_data[1]);
	CDBG("%s user_data[2] = 0x%02x\r\n", __func__, potp->user_data[2]);
	CDBG("%s user_data[3] = 0x%02x\r\n", __func__, potp->user_data[3]);
	CDBG("%s user_data[4] = 0x%02x\r\n", __func__, potp->user_data[4]);

	/* disable otp read */
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3d81, 0x00,
			MSM_CAMERA_I2C_BYTE_DATA);

	/* clear otp buffer */
	for (i = 0; i < 32; i++)
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, (0x3d00+i),
				0x00, MSM_CAMERA_I2C_BYTE_DATA);
}

/**********************************************
 * r_gain, sensor red gain of AWB, 0x400 =1
 * g_gain, sensor green gain of AWB, 0x400 =1
 * b_gain, sensor blue gain of AWB, 0x400 =1
 ***********************************************/
void ov8825_update_awb_gain(struct msm_sensor_ctrl_t *s_ctrl,
		uint16_t r_gain, uint16_t g_gain, uint16_t b_gain)
{
	CDBG("%s r_gain = 0x%04x\r\n", __func__, r_gain);
	CDBG("%s g_gain = 0x%04x\r\n", __func__, g_gain);
	CDBG("%s b_gain = 0x%04x\r\n", __func__, b_gain);
	if (r_gain > 0x400) {
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x5186,
				(r_gain>>8), MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x5187,
				(r_gain&0xff), MSM_CAMERA_I2C_BYTE_DATA);
	}
	if (g_gain > 0x400) {
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x5188,
				(g_gain>>8), MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x5189,
				(g_gain&0xff), MSM_CAMERA_I2C_BYTE_DATA);
	}
	if (b_gain > 0x400) {
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x518a,
				(b_gain>>8), MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x518b,
				(b_gain&0xff), MSM_CAMERA_I2C_BYTE_DATA);
	}
}

/**************************************************
 * call this function after OV8825 initialization
 * return value:
 *     0, update success
 *     1, no OTP
 ***************************************************/
uint16_t ov8825_update_otp(struct msm_sensor_ctrl_t *s_ctrl)
{
	uint16_t i;
	uint16_t otp_index;
	uint16_t temp;
	uint16_t r_gain, g_gain, b_gain, g_gain_r, g_gain_b;

	/* R/G and B/G of current camera module is read out from sensor OTP */
	/* check first OTP with valid data */
	for (i = 0; i < 3; i++) {
		temp = ov8825_check_otp_wb(s_ctrl, i);
		if (temp == 2) {
			otp_index = i;
			break;
		}
	}
	if (i == 3) {
		/* no valid wb OTP data */
		CDBG("no valid wb OTP data\r\n");
		return 1;
	}
	ov8825_read_otp_wb(s_ctrl, otp_index, &st_ov8825_otp);
	/* calculate g_gain */
	/* 0x400 = 1x gain */
	if (st_ov8825_otp.bg_ratio < OV8825_BG_RATIO_TYPICAL_VALUE) {
		if (st_ov8825_otp.rg_ratio < OV8825_RG_RATIO_TYPICAL_VALUE) {
			g_gain = 0x400;
			b_gain = 0x400 *
				OV8825_BG_RATIO_TYPICAL_VALUE /
				st_ov8825_otp.bg_ratio;
			r_gain = 0x400 *
				OV8825_RG_RATIO_TYPICAL_VALUE /
				st_ov8825_otp.rg_ratio;
		} else {
			r_gain = 0x400;
			g_gain = 0x400 *
				st_ov8825_otp.rg_ratio /
				OV8825_RG_RATIO_TYPICAL_VALUE;
			b_gain = g_gain *
				OV8825_BG_RATIO_TYPICAL_VALUE /
				st_ov8825_otp.bg_ratio;
		}
	} else {
		if (st_ov8825_otp.rg_ratio < OV8825_RG_RATIO_TYPICAL_VALUE) {
			b_gain = 0x400;
			g_gain = 0x400 *
				st_ov8825_otp.bg_ratio /
				OV8825_BG_RATIO_TYPICAL_VALUE;
			r_gain = g_gain *
				OV8825_RG_RATIO_TYPICAL_VALUE /
				st_ov8825_otp.rg_ratio;
		} else {
			g_gain_b = 0x400 *
				st_ov8825_otp.bg_ratio /
				OV8825_BG_RATIO_TYPICAL_VALUE;
			g_gain_r = 0x400 *
				st_ov8825_otp.rg_ratio /
				OV8825_RG_RATIO_TYPICAL_VALUE;
			if (g_gain_b > g_gain_r) {
				b_gain = 0x400;
				g_gain = g_gain_b;
				r_gain = g_gain *
					OV8825_RG_RATIO_TYPICAL_VALUE /
					st_ov8825_otp.rg_ratio;
			} else {
				r_gain = 0x400;
				g_gain = g_gain_r;
				b_gain = g_gain *
					OV8825_BG_RATIO_TYPICAL_VALUE /
					st_ov8825_otp.bg_ratio;
			}
		}
	}
	ov8825_update_awb_gain(s_ctrl, r_gain, g_gain, b_gain);
	return 0;
}

static int32_t ov8825_write_exp_gain(struct msm_sensor_ctrl_t *s_ctrl,
		uint16_t gain, uint32_t line)
{
	uint32_t fl_lines, offset;
	uint8_t int_time[3];

	fl_lines =
		(s_ctrl->curr_frame_length_lines * s_ctrl->fps_divider) / Q10;
	offset = s_ctrl->sensor_exp_gain_info->vert_offset;
	if (line > (fl_lines - offset))
		fl_lines = line + offset;
	CDBG("ov8825_write_exp_gain: %d %d %d\n", fl_lines, gain, line);
	s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->frame_length_lines, fl_lines,
		MSM_CAMERA_I2C_WORD_DATA);
	int_time[0] = line >> 12;
	int_time[1] = line >> 4;
	int_time[2] = line << 4;
	msm_camera_i2c_write_seq(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr-1,
		&int_time[0], 3);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr, gain,
		MSM_CAMERA_I2C_WORD_DATA);
	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	return 0;
}

static const struct i2c_device_id ov8825_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&ov8825_s_ctrl},
	{ }
};

int32_t ov8825_sensor_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int32_t rc = 0;
	//struct msm_sensor_ctrl_t *s_ctrl;
	if (camera_id !=1) return -EFAULT;
	
	CDBG("\n in ov8825_sensor_i2c_probe\n");
	rc = msm_sensor_i2c_probe(client, id);
	if (client->dev.platform_data == NULL) {
		pr_err("%s: NULL sensor data\n", __func__);
		return -EFAULT;
	}
	//s_ctrl = client->dev.platform_data;
	//if (s_ctrl->sensordata->pmic_gpio_enable)
	//	lcd_camera_power_onoff(0);
	//	camera_power_onoff(0);
	return rc;
}

int32_t ov8825_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct msm_camera_sensor_info *info = s_ctrl->sensordata;
	CDBG("%s IN\r\n", __func__);

	CDBG("%s, sensor_pwd:%d, sensor_reset:%d\r\n",__func__, info->sensor_pwd, info->sensor_reset);

	gpio_direction_output(info->sensor_pwd, 0);
	gpio_direction_output(info->sensor_reset, 0);
	usleep_range(5000, 6000);

	if (info->pmic_gpio_enable) {
	//	info->pmic_gpio_enable = 0;
		//zxj ++
		#ifdef ORIGINAL_VERSION
		lcd_camera_power_onoff(1);
		#else
		camera_power_onoff(1);
		#endif
		//zxj --
	}
	//gpio_direction_output(info->sensor_pwd, 0);
	//gpio_direction_output(info->sensor_reset, 0);
	//usleep_range(10000, 11000);
	rc = msm_sensor_power_up(s_ctrl);
	if (rc < 0) {
		CDBG("%s: msm_sensor_power_up failed\n", __func__);
		return rc;
	}
	/* turn on ldo and vreg */
	gpio_direction_output(info->sensor_pwd, 1);
	msleep(20);
	gpio_direction_output(info->sensor_reset, 1);
	msleep(40);
	return rc;
}

int32_t ov8825_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	struct msm_camera_sensor_info *info = s_ctrl->sensordata;
	int rc = 0;
	CDBG("%s IN\r\n", __func__);

	//Stop stream first
	s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);
	msleep(40);

	gpio_direction_output(info->sensor_pwd, 0);
	usleep_range(5000, 5100);
	msm_sensor_power_down(s_ctrl);
	msleep(40);
	if (info->pmic_gpio_enable){
		camera_power_onoff(0);
	}
	return rc;
}

static struct i2c_driver ov8825_i2c_driver = {
	.id_table = ov8825_i2c_id,
	.probe  = ov8825_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client ov8825_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};



static int __init msm_sensor_init_module(void)
{
	return i2c_add_driver(&ov8825_i2c_driver);
}

static struct v4l2_subdev_core_ops ov8825_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops ov8825_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops ov8825_subdev_ops = {
	.core = &ov8825_subdev_core_ops,
	.video  = &ov8825_subdev_video_ops,
};

static int is_first_preview = 1;

int32_t ov8825_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res)
{
	int32_t rc = 0;
	static int csi_config;
	//zxj, 2012-08-15
	static unsigned short af_reg_l;
	static unsigned short af_reg_h;
	int af_step_pos;
	CDBG("8825 sensor setting in, update_type:0x%x, res:0x%x\r\n",update_type, res);

	if(update_type == MSM_SENSOR_UPDATE_PERIODIC) {
		msm_camera_i2c_read(s_ctrl->sensor_i2c_client, 0x3618, &af_reg_l,
			MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_read(s_ctrl->sensor_i2c_client, 0x3619, &af_reg_h,
			MSM_CAMERA_I2C_BYTE_DATA);
		CDBG("AF_tuning data 3618 is 0x%x, 3619 is 0x%x\r\n", af_reg_l, af_reg_h);

		//set to zero to avoid lens crash sound
		for(af_step_pos = af_reg_h&0x3f; af_step_pos > 0; af_step_pos-=8) {
			msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3618, 0x9,
				MSM_CAMERA_I2C_BYTE_DATA);
			msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3619, af_step_pos&0xff,
				MSM_CAMERA_I2C_BYTE_DATA);
			msleep(2);
		}
	}
	//zxj,2012-08-15
	s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);
	msleep(30);
	if (update_type == MSM_SENSOR_REG_INIT) {
		CDBG("Register INIT\n");
		s_ctrl->curr_csi_params = NULL;
		msm_sensor_enable_debugfs(s_ctrl);
		msm_sensor_write_init_settings(s_ctrl);
		CDBG("Update OTP\n");
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x100, 0x1,
				MSM_CAMERA_I2C_BYTE_DATA);
		msleep(66);
		ov8825_update_otp(s_ctrl);
		usleep_range(10000, 11000);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x100, 0x0,
		  MSM_CAMERA_I2C_BYTE_DATA);
		csi_config = 0;
		is_first_preview = 1;
	} else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
		CDBG("PERIODIC : %d\n", res);
		msm_sensor_write_conf_array(
			s_ctrl->sensor_i2c_client,
			s_ctrl->msm_sensor_reg->mode_settings, res);
		msleep(30);
		if (!csi_config) {
			s_ctrl->curr_csic_params = s_ctrl->csic_params[res];
			CDBG("CSI config in progress\n");
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_CSIC_CFG,
				s_ctrl->curr_csic_params);
			CDBG("CSI config is done\n");
			mb();
			msleep(30);
			csi_config = 1;
		}
		//add by lzj
		//v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
		//	NOTIFY_PCLK_CHANGE,
		//	&s_ctrl->sensordata->pdata->ioclk.vfe_clk_rate);

		if (res == MSM_SENSOR_RES_4)
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
					NOTIFY_PCLK_CHANGE,
					&vfe_clk);
		// add end 

		s_ctrl->func_tbl->sensor_start_stream(s_ctrl);
		//zxj,2012-08-15
		af_reg_l = af_reg_l & 0xf0;
		af_reg_l = af_reg_l | 0x0e;
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3618, af_reg_l,
			MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3619, af_reg_h,
			MSM_CAMERA_I2C_BYTE_DATA);
		CDBG("AF_tuning write data 3618 is 0x%x, 3619 is 0x%x\r\n", af_reg_l, af_reg_h);

		if (!is_first_preview) {
			if (res == 1) {
				msleep(200);
			}
		} else {
			is_first_preview = 0;	
		}
		//zxj,2012-08-15
		msleep(50);
	}
	return rc;
}

/*zhuangxiaojian, 2012-08-15, c8680, optimize af search mode for ov8825 camera module {*/
#if 0
#ifdef ORIGINAL_VERSION
#else
#if 1
static int32_t ov8825_i2c_write_b(struct msm_camera_i2c_client *client, unsigned short waddr, uint8_t bdata)
{
	int32_t rc = 0;
	if(client == NULL) {
		printk("##### %s: func error !\n",__func__);
		return -1;
	}
	
	rc = msm_camera_i2c_write(client, waddr, bdata, MSM_CAMERA_I2C_BYTE_DATA);

	return rc;
}

static int32_t ov8825_af_i2c_write(uint16_t data)
{
	uint8_t code_val_msb, code_val_lsb;
	uint32_t rc = 0;
	printk("----------- %s: data = %d\n",__func__,data);//zxj
	code_val_msb = data >> 4; 
	code_val_lsb = ((data & 0x000F) << 4) | S3_to_0;
	
	CDBG("code value = %d ,D[9:4] = %d ,D[3:0] = %d\n", data, code_val_msb, code_val_lsb);
	rc = ov8825_i2c_write_b(ov8825_s_ctrl.sensor_i2c_client , OV8825_AF_MSB, code_val_msb);
	if (rc < 0) {
		CDBG("Unable to write code_val_msb = %d\n", code_val_msb);
		return rc;
	}
	rc = ov8825_i2c_write_b(ov8825_s_ctrl.sensor_i2c_client, OV8825_AF_LSB, code_val_lsb);
	if (rc < 0) {
		CDBG("Unable to write code_val_lsb = %d\n", code_val_lsb);
		return rc;
	}

	return rc;
} /* ov8825_af_i2c_write */
#endif

#if 0
int32_t ov8825_af_init(struct msm_sensor_ctrl_t *s_ctrl, struct region_params_t *region_params)
{
	int32_t rc = 0;
	int16_t cur_code = 0;
	int16_t step_index = 0;
	int16_t region_index = 0;
	uint32_t max_code_size = 1;
	uint16_t step_boundary = 0;
	uint16_t code_per_step = 0;	
	uint16_t code_size = s_ctrl->data_size;

	if (copy_from_user(&s_ctrl->region_params,
		(void *)region_params,
		s_ctrl->region_size * sizeof(struct region_params_t)))
		return -EFAULT;
	
	s_ctrl->step_position_table = NULL;
	code_per_step = s_ctrl->region_params[region_index].code_per_step;
	printk("##### %s called\n",__func__);
	
	for (; code_size > 0; code_size--)
		max_code_size *= 2;
	
	s_ctrl->step_position_table = kmalloc(sizeof(uint16_t) * (s_ctrl->total_steps + 1), GFP_KERNEL);
	if (s_ctrl->step_position_table == NULL)
		return -EFAULT;

	cur_code = s_ctrl->initial_code;
	s_ctrl->step_position_table[step_index++] = cur_code;

	for (region_index = 0; region_index < s_ctrl->region_size; region_index++) {
//		code_per_step = s_ctrl->region_params[region_index].code_per_step;
		code_per_step = 18;
		step_boundary = s_ctrl->region_params[region_index].step_bound[MOVE_NEAR];
		
		for (; step_index <= step_boundary; step_index++) {
			cur_code += code_per_step;
			
			if (cur_code < max_code_size)
				s_ctrl->step_position_table[step_index] = cur_code;
			else {
				for (; step_index < s_ctrl->total_steps; step_index++)
					s_ctrl->step_position_table[step_index] = max_code_size;
//				return rc;
			}
		}
	}
	s_ctrl->curr_step_pos = 0;
	s_ctrl->curr_region_index = 0;

	return rc;
}
#else
int32_t ov8825_af_init(struct msm_sensor_ctrl_t *s_ctrl, struct region_params_t *region_params)
{
	int32_t rc = 0;
	int16_t cur_code = 0;
	int16_t step_index = 0;
	int16_t region_index = 0;
	uint32_t max_code_size = 1;
	uint16_t step_boundary = 0;
	uint16_t ov8825_nl_region_code_per_step = 85;	
	uint16_t ov8825_l_region_code_per_step = 18;
	uint16_t ov8825_nl_region_boundary = 3;
	uint16_t code_size = s_ctrl->data_size;

	if (copy_from_user(&s_ctrl->region_params,
		(void *)region_params,
		s_ctrl->region_size * sizeof(struct region_params_t)))
		return -EFAULT;
	
	s_ctrl->step_position_table = NULL;
	
	printk("##### %s called\n",__func__);
	
	for (; code_size > 0; code_size --)
		max_code_size *= 2;
	
	s_ctrl->step_position_table = kmalloc(sizeof(uint16_t) * (s_ctrl->total_steps + 1), GFP_KERNEL);
	if (s_ctrl->step_position_table == NULL)
		return -EFAULT;

	cur_code = s_ctrl->initial_code;
	s_ctrl->step_position_table[step_index++] = cur_code;

	for (region_index = 0; region_index < s_ctrl->region_size; region_index++) {

		step_boundary = s_ctrl->region_params[region_index].step_bound[MOVE_NEAR];
		
		for (step_index = 1; step_index <= step_boundary; step_index++) {
			if (step_index <= ov8825_nl_region_boundary){
				s_ctrl->step_position_table[step_index] = s_ctrl->step_position_table[step_index - 1] 
							+ ov8825_nl_region_code_per_step;
			}else{
				s_ctrl->step_position_table[step_index] = s_ctrl->step_position_table[step_index - 1] 
							+ ov8825_l_region_code_per_step;
			}
			
			if (s_ctrl->step_position_table[step_index] > max_code_size)
				s_ctrl->step_position_table[step_index] = max_code_size;
		}
	}
	
	s_ctrl->curr_step_pos = 0;
	s_ctrl->curr_region_index = 0;

	return rc;
}
#endif


int32_t ov8825_set_default_focus(struct msm_sensor_ctrl_t *s_ctrl,
						struct msm_sensor_af_move_params_t *af_move_params)
{
	int32_t rc = 0;
#if 1
	printk("------------ %s called -------------\n", __func__);

	if (s_ctrl->curr_step_pos != 0)
		rc = s_ctrl->func_tbl->sensor_move_focus(s_ctrl, af_move_params);
#endif
	s_ctrl->curr_step_pos = 0;
	s_ctrl->dest_step_pos = 0;
	printk("------------ %s end -------------\n", __func__);
	return rc;

}

static int32_t DIV_CEIL(int x, int y)
{
    if(x%y >0)
        return x/y + 1;
    else
        return x/y;
}

int32_t ov8825_move_focus(struct msm_sensor_ctrl_t *s_ctrl, 
				struct msm_sensor_af_move_params_t *af_move_params)
{
	int32_t rc = 0;
#if 1
	int16_t next_lens_pos = 0;

	int8_t step_dir = af_move_params->sign_dir;
	int16_t dest_step_pos = af_move_params->dest_step_pos;
	uint16_t dest_lens_pos = 0;
	uint16_t curr_lens_pos = 0;
	uint16_t step_boundary = 0;
	uint16_t damping_code_step = 0;
	uint32_t sw_damping_time_wait = 0;

	uint16_t small_step = 0;
	uint32_t target_dist = 0;
//	uint32_t code_dis = 0;
	uint16_t curr_step_pos = s_ctrl->curr_step_pos;
	printk("##### >>>>>>>>>> %s called <<<<<<<<<<<\n",__func__);

//	damping_code_step = af_move_params->ringing_params->damping_step;
//	sw_damping_time_wait= af_move_params->ringing_params->damping_delay;
	
printk("##### %s line%d: damping_code_step == %d\n",__func__,__LINE__,damping_code_step);
printk("##### %s line%d: sw_damping_time_wait == %d\n",__func__,__LINE__,sw_damping_time_wait);
	if (dest_step_pos == curr_step_pos)
		return 0;

	
//	next_lens_pos = curr_lens_pos + (step_dir * damping_code_step);
	step_boundary = s_ctrl->region_params[s_ctrl->curr_region_index].step_bound[0];
#if 0
	target_dis = s_ctrl->region_params[s_ctrl->curr_region_index].code_per_step * num_steps;
	code_dis = s_ctrl->region_params[s_ctrl->curr_region_index].code_per_step;
#endif

	if (dest_step_pos < 0)
		dest_step_pos = 0;
#if 1
	else if (dest_step_pos > step_boundary)
		dest_step_pos = step_boundary;
#else
	else if (dest_step_pos > s_ctrl->total_steps)
		dest_step_pos = s_ctrl->total_steps;
#endif

	
	curr_lens_pos = s_ctrl->step_position_table[curr_step_pos];
	dest_lens_pos = s_ctrl->step_position_table[dest_step_pos];

	printk("##### %s line%d: curr_lens_pos == %d\n",__func__,__LINE__,curr_lens_pos);
	printk("##### %s line%d: dest_lens_pos == %d\n",__func__,__LINE__,dest_lens_pos);
#if 0
	for (next_lens_pos =
		curr_lens_pos + (step_dir * damping_code_step);
		(step_dir * next_lens_pos) <=
			(step_dir * dest_lens_pos);
		next_lens_pos += (step_dir * damping_code_step)) {
#endif
	target_dist = step_dir * (dest_lens_pos - curr_lens_pos);

	if (step_dir < 0 && (target_dist >=
		s_ctrl->step_position_table[ov8825_damping_threshold])) {
		small_step = DIV_CEIL(target_dist, 10);
		sw_damping_time_wait = 10;
	} else {
		small_step = DIV_CEIL(target_dist, 4);
		sw_damping_time_wait = 4;
	}

	for (next_lens_pos =
		curr_lens_pos + (step_dir * small_step);
		(step_dir * next_lens_pos) <=
			(step_dir * dest_lens_pos);
		next_lens_pos += (step_dir * small_step)) {
		printk("##### %s line%d: next_lens_pos == %d\n",__func__,__LINE__,next_lens_pos);

		if (next_lens_pos < s_ctrl->step_position_table[0])
			next_lens_pos = s_ctrl->step_position_table[0];
		else if (next_lens_pos > s_ctrl->step_position_table[step_boundary])
			next_lens_pos = s_ctrl->step_position_table[step_boundary];
			
		if(ov8825_af_i2c_write(next_lens_pos) < 0) {
			printk("############ %s line%d: write af failed !\n",__func__,__LINE__);
			return -EBUSY;
			}
		s_ctrl->curr_lens_pos = next_lens_pos;
		printk("+++++ %s line%d: damping time wait!\n",__func__,__LINE__);
		usleep(sw_damping_time_wait);
	}
	
	if (s_ctrl->curr_lens_pos != dest_lens_pos) {
	printk("##### %s line%d: curr_lens_pos == %d\n",__func__,__LINE__,curr_lens_pos);
	printk("##### %s line%d: dest_lens_pos == %d\n",__func__,__LINE__,dest_lens_pos);
		if(ov8825_af_i2c_write(dest_lens_pos) < 0) {
			printk("############ %s line%d: write af failed !\n",__func__,__LINE__);
			return -EBUSY;
		}
		usleep(sw_damping_time_wait * 50);
	}
//	curr_lens_pos = dest_lens_pos;
//	curr_step_pos = dest_step_pos;
	
	s_ctrl->curr_lens_pos = dest_lens_pos;
	s_ctrl->curr_step_pos = dest_step_pos;
#else
//	int32_t num_steps = af_move_params->num_steps;
	int8_t step_dir = af_move_params->sign_dir;
	int16_t dest_step_pos = af_move_params->dest_step_pos;
	uint16_t dest_lens_pos = 0;
	uint16_t curr_lens_pos = 0;
	uint16_t step_boundary = 0;

	uint16_t curr_step_pos = s_ctrl->curr_step_pos;

	if (dest_step_pos == s_ctrl->curr_step_pos)
		return rc;
printk("##### %s line%d: dest_step_pos == %d\n",__func__,__LINE__,dest_step_pos);
printk("##### %s line%d: curr_step_pos == %d\n",__func__,__LINE__,curr_step_pos);
printk("##### %s line%d: step_dir == %d\n",__func__,__LINE__,step_dir);//zxj
	curr_lens_pos = s_ctrl->step_position_table[s_ctrl->curr_step_pos];
//	step_boundary = s_ctrl->region_params[s_ctrl->curr_region_index].step_bound[step_dir];
	step_boundary = s_ctrl->total_steps;

	dest_lens_pos = s_ctrl->step_position_table[dest_step_pos - 1];
printk("##### %s line%d: step_boundary == %d\n",__func__,__LINE__,step_boundary);
	printk("##### %s line%d: curr_lens_pos == %d\n",__func__,__LINE__,curr_lens_pos);
	printk("##### %s line%d: dest_lens_pos == %d\n",__func__,__LINE__,dest_lens_pos);
	
	printk("##########################################\n");
	while (curr_step_pos != dest_step_pos) {
		printk("+++++++++++++++++++++++++++++++++++++++\n");
		
		if ((dest_step_pos * step_dir) <= (step_boundary * step_dir)) {
			printk("##### %s line%d\n",__func__,__LINE__);//zxj
			if (dest_lens_pos == curr_lens_pos) {
				printk("##### %s line%d: dest_lens_pos == curr_lens_pos\n",__func__,__LINE__);//zxj
				return rc;
			}
			if (ov8825_af_i2c_write(dest_lens_pos) < 0) {
				printk("##### %s line%d: failed to write af\n",__func__,__LINE__);//zxj
				return -EBUSY;
			}
			curr_lens_pos = dest_lens_pos;
		}else{
			dest_step_pos = step_boundary;
			dest_lens_pos = s_ctrl->step_position_table[dest_step_pos];
			printk("##### %s line%d: dest_step_pos == step_boundary == %d\n",__func__,__LINE__,step_boundary);
			printk("##### %s line%d: dest_lens_pos == %d\n",__func__,__LINE__,dest_lens_pos);
			printk("##### %s line%d: curr_lens_pos == %d\n",__func__,__LINE__,curr_lens_pos);//zxj
			if (curr_lens_pos == dest_lens_pos) {
				printk("##### %s line%d: dest_lens_pos == curr_lens_pos\n",__func__,__LINE__);//zxj
				return rc;
			}
			if (ov8825_af_i2c_write(dest_lens_pos) < 0) {
				printk("##### %s line%d: failed to write af\n",__func__,__LINE__);//zxj
				return -EBUSY;
				}
			curr_lens_pos = dest_lens_pos;

			s_ctrl->curr_region_index += step_dir;
		}
		//usleep(af_move_params->sw_damping_time_wait);
		//usleep(5000);
		s_ctrl->curr_step_pos = dest_step_pos;

	}

//	s_ctrl->curr_lens_pos = dest_lens_pos;
//	s_ctrl->curr_step_pos = dest_step_pos;
#endif
	return rc;
}
#endif

#else

DEFINE_MUTEX(ov8825_actuator_mutex);

int32_t ov8825_af_init(struct msm_sensor_ctrl_t *s_ctrl, struct msm_actuator_set_info_t *set_info)
{
	int32_t rc = 0;

	return_if_val_fail(s_ctrl != NULL && set_info != NULL, -1);
	printk("##### %s line%d: E\n",__func__,__LINE__);
	rc = msm_actuator_init(s_ctrl->a_ctrl, set_info);

	if (rc < 0){
		printk("##### %s: init failed !\n",__func__);
		return rc;
	}

	return rc;
}

int32_t ov8825_set_default_focus(struct msm_sensor_ctrl_t *s_ctrl,
						struct msm_actuator_move_params_t* move)
{
	int32_t rc = 0;

	return_if_val_fail(s_ctrl != NULL, -1);
	printk("##### %s: E\n",__func__);
	rc = msm_actuator_set_default_focus(s_ctrl->a_ctrl, move);

	if (rc < 0){
		printk("##### %s: default setting failed !\n",__func__);
		return rc;
	}

	return rc;
}

int32_t ov8825_move_focus(struct msm_sensor_ctrl_t *s_ctrl,
						struct msm_actuator_move_params_t* move)
{
	int32_t rc = 0;

	return_if_val_fail(s_ctrl != NULL, -1);

	rc = msm_actuator_move_focus(s_ctrl->a_ctrl, move);

	if (rc < 0) {
		printk("##### %s: default setting failed !\n",__func__);
		return rc;		
	}

	return rc;
}

/*zhuangxiaojian, 2012-08-18, C8680, add ov8825 i2c write af method {*/
int32_t ov8825_af_i2c_write(struct msm_actuator_ctrl_t *a_ctrl,
	int16_t next_lens_position, uint32_t hw_params)
{
	struct msm_actuator_reg_params_t *write_arr = a_ctrl->reg_tbl;
	uint32_t hw_dword = hw_params;
	uint16_t i2c_byte1 = 0, i2c_byte2 = 0;
	uint16_t value = 0;
	uint32_t size = a_ctrl->reg_tbl_size, i = 0;
	int32_t rc = 0;
	printk("##### %s: IN\n", __func__);
	for (i = 0; i < size; i++) {
		if (write_arr[i].reg_write_type == MSM_ACTUATOR_WRITE_DAC) {
			value = (next_lens_position <<
				write_arr[i].data_shift) |
				((hw_dword & write_arr[i].hw_mask) >>
				write_arr[i].hw_shift);

			if (write_arr[i].reg_addr != 0xFFFF) {
				i2c_byte1 = write_arr[i].reg_addr;
				i2c_byte2 = value;
				if (size != (i+1)) {
					//i2c_byte2 = (i2c_byte2 & 0xFF00) >> 8;
					i2c_byte2 = value & 0xFF;
					CDBG("%s: byte1:0x%x, byte2:0x%x\n",
					__func__, i2c_byte1, i2c_byte2);
					rc = msm_camera_i2c_write(
						ov8825_s_ctrl.sensor_i2c_client,
						i2c_byte1, i2c_byte2,
						a_ctrl->i2c_data_type);
					if (rc < 0) {
						pr_err("%s: i2c write error:%d\n",
							__func__, rc);
						return rc;
					}

					i++;
					i2c_byte1 = write_arr[i].reg_addr;
					//i2c_byte2 = value & 0xFF;
					i2c_byte2 = (value & 0xFF00) >> 8;
				}
			} else {
				i2c_byte1 = (value & 0xFF00) >> 8;
				i2c_byte2 = value & 0xFF;
			}
		} else {
			i2c_byte1 = write_arr[i].reg_addr;
			i2c_byte2 = (hw_dword & write_arr[i].hw_mask) >>
				write_arr[i].hw_shift;
		}
		CDBG("%s: i2c_byte1:0x%x, i2c_byte2:0x%x\n", __func__,
			i2c_byte1, i2c_byte2);
		rc = msm_camera_i2c_write(ov8825_s_ctrl.sensor_i2c_client,
			i2c_byte1, i2c_byte2, a_ctrl->i2c_data_type);
	}
		printk("##### %s: OUT\n", __func__);
	return rc;	
}
/*} zhuangxiaojian, 2012-08-18, C8680, add ov8825 i2c write af method*/
EXPORT_SYMBOL(ov8825_af_i2c_write);

#endif
/*} zhuangxiaojian, 2012-08-15, c8680, optimize af search mode for ov8825 camera module*/

static struct msm_sensor_fn_t ov8825_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain = ov8825_write_exp_gain,
	.sensor_write_snapshot_exp_gain = ov8825_write_exp_gain,
	.sensor_csi_setting = ov8825_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = ov8825_sensor_power_up,
	//.sensor_power_down = msm_sensor_power_down,
	.sensor_power_down = ov8825_sensor_power_down,
	//zxj ++
	#ifdef ORIGINAL_VERSION
	#else
	.sensor_focus_config = msm_sensor_focus_config,
	.sensor_af_init = ov8825_af_init,
	.sensor_set_default_focus = ov8825_set_default_focus,
	.sensor_move_focus = ov8825_move_focus,
	#endif
	//zxj --
};

/*zhuangxiaojian, 2012-08-18, C8680, add for ov8825 camera actuator {*/
#ifdef ORIGINAL_VERSION
#else
//struct msm_actuator_func_tbl func_tbl = {
//	.actuator_i2c_write = ov8825_af_i2c_write,
//};

static struct msm_actuator_ctrl_t ov8825_actuator_t = {
	.curr_step_pos = 0,
	.curr_region_index = 0,
	.actuator_mutex = &ov8825_actuator_mutex,
//	.func_tbl = &func_tbl,
};
#endif
/*} zhuangxiaojian, 2012-08-18, C8680, add for ov8825 camera actuator*/

static struct msm_sensor_reg_t ov8825_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = ov8825_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(ov8825_start_settings),
	.stop_stream_conf = ov8825_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(ov8825_stop_settings),
	.group_hold_on_conf = ov8825_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(ov8825_groupon_settings),
	.group_hold_off_conf = ov8825_groupoff_settings,
	.group_hold_off_conf_size =	ARRAY_SIZE(ov8825_groupoff_settings),
	.init_settings = &ov8825_init_conf[0],
	.init_size = ARRAY_SIZE(ov8825_init_conf),
	.mode_settings = &ov8825_confs[0],
	.output_settings = &ov8825_dimensions[0],
	.num_conf = ARRAY_SIZE(ov8825_confs),
};

static struct msm_sensor_ctrl_t ov8825_s_ctrl = {
	.msm_sensor_reg = &ov8825_regs,
	.sensor_i2c_client = &ov8825_sensor_i2c_client,
	.sensor_i2c_addr = 0x6C,
	.sensor_output_reg_addr = &ov8825_reg_addr,
	.sensor_id_info = &ov8825_id_info,
	.sensor_exp_gain_info = &ov8825_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csic_params = &ov8825_csi_params_array[0],
	.msm_sensor_mutex = &ov8825_mut,
	.sensor_i2c_driver = &ov8825_i2c_driver,
	.sensor_v4l2_subdev_info = ov8825_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(ov8825_subdev_info),
	.sensor_v4l2_subdev_ops = &ov8825_subdev_ops,
	.func_tbl = &ov8825_func_tbl,
	//zxj ++
	.a_ctrl = &ov8825_actuator_t,
	//zxj --
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Omnivison 8MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");
