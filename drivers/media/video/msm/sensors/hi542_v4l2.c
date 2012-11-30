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

#include "msm_sensor.h"
#include "msm.h"

#define SENSOR_NAME "hi542"
#define PLATFORM_DRIVER_NAME "msm_camera_hi542"
#define hi542_obj hi542_##obj

DEFINE_MUTEX(hi542_mut);
static int32_t vfe_clk_rate = 192000000;

static struct msm_sensor_ctrl_t hi542_s_ctrl;

static struct msm_camera_i2c_reg_conf hi542_start_settings[] = {
	{0x0001,0x00},
};

static struct msm_camera_i2c_reg_conf hi542_stop_settings[] = {
	{0x0001,0x01},
};

static struct msm_camera_i2c_reg_conf hi542_groupon_settings[] = {
	{0x0104,0x01},
};

static struct msm_camera_i2c_reg_conf hi542_groupoff_settings[] = {
	{0x0104,0x00},
};

static struct msm_camera_i2c_reg_conf hi542_prev_settings[] = {
	{0x0010,0x41},
	{0x0034,0x03},
	{0x0035,0xd4},
	{0x0500,0x19},
	{0x0630,0x05},
	{0x0631,0x18},
	{0x0632,0x03},
	{0x0633,0xd4},
};

static struct msm_camera_i2c_reg_conf hi542_snap_settings[] = {
	{0x0010,0x40},
	{0x0034,0x07},
	{0x0035,0xA8},
	{0x0500,0x11},
	{0x0630,0x0A},
	{0x0631,0x30},
	{0x0632,0x07},
	{0x0633,0xA8},
};

static struct msm_camera_i2c_reg_conf hi542_recommend_settings[] = {

	{0x0011, 0x90},   
	{0x0020, 0x00},
	{0x0021, 0x00},
	{0x0022, 0x00},
	{0x0023, 0x00},
	{0x0038, 0x02},
	{0x0039, 0x2C},
	{0x003C, 0x00},
	{0x003D, 0x0C},
	{0x003E, 0x00},
	{0x003F, 0x0C},
	{0x0040, 0x00},
	{0x0041, 0x35},
	{0x0042, 0x00},
	{0x0043, 0x14},
	{0x0045, 0x07},
	{0x0046, 0x01},
	{0x0047, 0xD0},
	{0x0050, 0x00},
	{0x0052, 0x10},
	{0x0053, 0x10},
	{0x0054, 0x10},
	{0x0055, 0x08},
	{0x0056, 0x80},
	{0x0057, 0x08},
	{0x0058, 0x08},
	{0x0059, 0x08},
	{0x005A, 0x08},
	{0x005B, 0x02},
	{0x0070, 0x00},
	{0x0081, 0x01},
	{0x0082, 0x23},
	{0x0083, 0x00},
	{0x0085, 0x00},
	{0x0086, 0x00},
	{0x008C, 0x02},
	{0x00A0, 0x0f},
	{0x00A1, 0x00},
	{0x00A2, 0x00},
	{0x00A3, 0x00},
	{0x00A4, 0xFF},
	{0x00A5, 0x00},
	{0x00A6, 0x00},
	{0x00A8, 0x7F},
	{0x00A9, 0x7F},
	{0x00AA, 0x7F},
	{0x00B4, 0x00},
	{0x00B5, 0x00},
	{0x00B6, 0x02},
	{0x00B7, 0x01},
	{0x00D4, 0x00},
	{0x00D5, 0xaa},
	{0x00D6, 0x01},
	{0x00D7, 0xc9},
	{0x00D8, 0x05},
	{0x00D9, 0x59},
	{0x00DA, 0x00},
	{0x00DB, 0xb0},
	{0x00DC, 0x01},
	{0x00DD, 0xc9},
	{0x011C, 0x00},  //exp max for fix fps                                                           //00 //1F
	{0x011D, 0x7a},  //0x99/fix 3fps //0x7a/fix 4fps //0x4c/fix 5fps //0x63/fix 6fps //0x51/fix 7fps //99 //FF
	{0x011E, 0x12},  //0x94/fix 3fps //0x12/fix 4fps //0x4b/fix 5fps //0x75/fix 6fps //0x78/fix 7fps //94 //FF
	{0x011F, 0x00},  //0x56/fix 3fps //0x00/fix 4fps //0x40/fix 5fps //0x36/fix 6fps //0x24/fix 7fps //56 //FF
	{0x012A, 0xFF},
	{0x012B, 0x00},
	{0x0129, 0x22}, //AG //40
	{0x0210, 0x00},
	{0x0212, 0x00},
	{0x0213, 0x00},
	{0x0216, 0x00},
	{0x0219, 0x33},
	{0x021B, 0x55},
	{0x021C, 0x85},
	{0x021D, 0xFF},
	{0x021E, 0x01},
	{0x021F, 0x00},
	{0x0220, 0x02},
	{0x0221, 0x00},
	{0x0222, 0xA0},
	{0x0227, 0x0A},
	{0x0228, 0x5C},
	{0x0229, 0x2d},
	{0x022A, 0x04},
	{0x022C, 0x01},
	{0x022D, 0x23},
	{0x0237, 0x00},
	{0x0238, 0x00},
	{0x0239, 0xA5},
	{0x023A, 0x20},
	{0x023B, 0x00},
	{0x023C, 0x22},
	{0x023F, 0x80},
	{0x0240, 0x04},
	{0x0241, 0x07},
	{0x0242, 0x00},
	{0x0243, 0x01},
	{0x0244, 0x80},
	{0x0245, 0xE0},
	{0x0246, 0x00},
	{0x0247, 0x00},
	{0x024A, 0x00},
	{0x024B, 0x14},
	{0x024D, 0x00},
	{0x024E, 0x03},
	{0x024F, 0x00},
	{0x0250, 0x53},
	{0x0251, 0x00},
	{0x0252, 0x07},
	{0x0253, 0x00},
	{0x0254, 0x4F},
	{0x0255, 0x00},
	{0x0256, 0x07},
	{0x0257, 0x00},
	{0x0258, 0x4F},
	{0x0259, 0x0C},
	{0x025A, 0x0C},
	{0x025B, 0x0C},
	{0x026C, 0x00},
	{0x026D, 0x09},
	{0x026E, 0x00},
	{0x026F, 0x4B},
	{0x0270, 0x00},
	{0x0271, 0x09},
	{0x0272, 0x00},
	{0x0273, 0x4B},
	{0x0274, 0x00},
	{0x0275, 0x09},
	{0x0276, 0x00},
	{0x0277, 0x4B},
	{0x0278, 0x00},
	{0x0279, 0x01},
	{0x027A, 0x00},
	{0x027B, 0x55},
	{0x027C, 0x00},
	{0x027D, 0x00},
	{0x027E, 0x05},
	{0x027F, 0x5E},
	{0x0280, 0x00},
	{0x0281, 0x03},
	{0x0282, 0x00},
	{0x0283, 0x45},
	{0x0284, 0x00},
	{0x0285, 0x03},
	{0x0286, 0x00},
	{0x0287, 0x45},
	{0x0288, 0x05},
	{0x0289, 0x5c},
	{0x028A, 0x05},
	{0x028B, 0x60},
	{0x02A0, 0x01},
	{0x02A1, 0xe0},
	{0x02A2, 0x02},
	{0x02A3, 0x22},
	{0x02A4, 0x05},
	{0x02A5, 0x5C},
	{0x02A6, 0x05},
	{0x02A7, 0x60},
	{0x02A8, 0x05},
	{0x02A9, 0x5C},
	{0x02AA, 0x05},
	{0x02AB, 0x60},
	{0x02D2, 0x0F},
	{0x02DB, 0x00},
	{0x02DC, 0x00},
	{0x02DD, 0x00},
	{0x02DE, 0x0C},
	{0x02DF, 0x00},
	{0x02E0, 0x04},
	{0x02E1, 0x00},
	{0x02E2, 0x00},
	{0x02E3, 0x00},
	{0x02E4, 0x0F},
	{0x02F0, 0x05},
	{0x02F1, 0x05},
	{0x0310, 0x00},
	{0x0311, 0x01},
	{0x0312, 0x05},
	{0x0313, 0x5A},
	{0x0314, 0x00},
	{0x0315, 0x01},
	{0x0316, 0x05},
	{0x0317, 0x5A},
	{0x0318, 0x00},
	{0x0319, 0x05},
	{0x031A, 0x00},
	{0x031B, 0x2F},
	{0x031C, 0x00},
	{0x031D, 0x05},
	{0x031E, 0x00},
	{0x031F, 0x2F},
	{0x0320, 0x00},
	{0x0321, 0xAB},
	{0x0322, 0x02},
	{0x0323, 0x55},
	{0x0324, 0x00},
	{0x0325, 0xAB},
	{0x0326, 0x02},
	{0x0327, 0x55},
	{0x0328, 0x00},
	{0x0329, 0x01},
	{0x032A, 0x00},
	{0x032B, 0x10},
	{0x032C, 0x00},
	{0x032D, 0x01},
	{0x032E, 0x00},
	{0x032F, 0x10},
	{0x0330, 0x00},
	{0x0331, 0x02},
	{0x0332, 0x00},
	{0x0333, 0x2e},
	{0x0334, 0x00},
	{0x0335, 0x02},
	{0x0336, 0x00},
	{0x0337, 0x2e},
	{0x0358, 0x00},
	{0x0359, 0x46},
	{0x035A, 0x05},
	{0x035B, 0x59},
	{0x035C, 0x00},
	{0x035D, 0x46},
	{0x035E, 0x05},
	{0x035F, 0x59},
	{0x0360, 0x00},
	{0x0361, 0x46},
	{0x0362, 0x00},
	{0x0363, 0xa4},
	{0x0364, 0x00},
	{0x0365, 0x46},
	{0x0366, 0x00},
	{0x0367, 0xa4},
	{0x0368, 0x00},
	{0x0369, 0x46},
	{0x036A, 0x00},
	{0x036B, 0xa6},
	{0x036C, 0x00},
	{0x036D, 0x46},
	{0x036E, 0x00},
	{0x036F, 0xa6},
	{0x0370, 0x00},
	{0x0371, 0xb0},
	{0x0372, 0x05},
	{0x0373, 0x59},
	{0x0374, 0x00},
	{0x0375, 0xb0},
	{0x0376, 0x05},
	{0x0377, 0x59},
	{0x0378, 0x00},
	{0x0379, 0x45},
	{0x037A, 0x00},
	{0x037B, 0xAA},
	{0x037C, 0x00},
	{0x037D, 0x99},
	{0x037E, 0x01},
	{0x037F, 0xAE},
	{0x0380, 0x01},
	{0x0381, 0xB1},
	{0x0382, 0x02},
	{0x0383, 0x56},
	{0x0384, 0x05},
	{0x0385, 0x6D},
	{0x0386, 0x00},
	{0x0387, 0xDC},
	{0x03A0, 0x05},
	{0x03A1, 0x5E},
	{0x03A2, 0x05},
	{0x03A3, 0x62},
	{0x03A4, 0x01},
	{0x03A5, 0xc9},
	{0x03A6, 0x01},
	{0x03A7, 0x27},
	{0x03A8, 0x05},
	{0x03A9, 0x59},
	{0x03AA, 0x02},
	{0x03AB, 0x55},
	{0x03AC, 0x01},
	{0x03AD, 0xc5},
	{0x03AE, 0x01},
	{0x03AF, 0x27},
	{0x03B0, 0x05},
	{0x03B1, 0x55},
	{0x03B2, 0x02},
	{0x03B3, 0x55},
	{0x03B4, 0x00},
	{0x03B5, 0x0A},
	{0x03D3, 0x08},
	{0x03D5, 0x44},
	{0x03D6, 0x51},
	{0x03D7, 0x56},
	{0x03D8, 0x44},
	{0x03D9, 0x06},
	{0x0580, 0x01},
	{0x0581, 0x00},
	{0x0582, 0x80},
	{0x0583, 0x00},
	{0x0584, 0x80},
	{0x0585, 0x00},
	{0x0586, 0x80},
	{0x0587, 0x00},
	{0x0588, 0x80},
	{0x0589, 0x00},
	{0x058A, 0x80},
	{0x05C2, 0x00},
	{0x05C3, 0x00},
	{0x0080, 0xC7},
	{0x0119, 0x05},  //EXPTIME_MIN_H //00 //08
	{0x011A, 0xaf},  //EXPTIME_MIN_M //15 //af
	{0x011B, 0xff},  //EXPTIME_MIN_L //C0 //ff
	{0x0115, 0x00},
	{0x0116, 0x2A},
	{0x0117, 0x4C},
	{0x0118, 0x20},
	{0x0223, 0xED},
	{0x0224, 0xE4},
	{0x0225, 0x09},
	{0x0226, 0x36},
	{0x023E, 0x80},
	{0x05B0, 0x00},
	{0x03D2, 0xAD},
	{0x0616, 0x00},
	{0x0616, 0x01},
	{0x03D0, 0xE9}, //E9 //82
	{0x03D1, 0x75}, //75 //75
	{0x03D2, 0xAC}, //AC //AC
	//{0x03D3, 0x00}, //   
	{0x0800, 0x07},
	{0x0801, 0x08},
	{0x0802, 0x02},
	{0x0012, 0x00},
	{0x0013, 0x00},
	{0x0024, 0x07},
	{0x0025, 0xA8},
	{0x0026, 0x0A},
	{0x0027, 0x30},
	{0x0030, 0x00},
	{0x0031, 0x03},
	{0x0032, 0x07},
	{0x0033, 0xAC},
	{0x003A, 0x00},
	{0x003B, 0x2E},
	{0x004A, 0x03},
	{0x004B, 0xD4},
	{0x004C, 0x05},
	{0x004D, 0x18},
	{0x0C98, 0x05},
	{0x0C99, 0x5E},
	{0x0C9A, 0x05},
	{0x0C9B, 0x62},
	{0x05A0, 0x01},
	{0x0084, 0x30},
	{0x008D, 0xFF},
	{0x0090, 0x02},
	{0x00A7, 0x80},
	{0x021A, 0x15},
	{0x022B, 0xb0},
	{0x0232, 0x37},
	{0x0010, 0x41},
	{0x0740, 0x1A},
	{0x0742, 0x1A},
	{0x0743, 0x1A},
	{0x0744, 0x1A},
	{0x0745, 0x04},
	{0x0746, 0x32},
	{0x0747, 0x05},
	{0x0748, 0x01},
	{0x0749, 0x90},
	{0x074A, 0x1A},
	{0x074B, 0xB1},
	{0x0500, 0x19},
	{0x0510, 0x10},
	{0x0217, 0x44},
	{0x0218, 0x00},
	{0x02ac, 0x00},
	{0x02ad, 0x00},
	{0x02ae, 0x00},
	{0x02af, 0x00},
	{0x02b0, 0x00},
	{0x02b1, 0x00},
	{0x02b2, 0x00},
	{0x02b3, 0x00},
	{0x02b4, 0x60},
	{0x02b5, 0x21},
	{0x02b6, 0x66},
	{0x02b7, 0x8a},
	{0x02c0, 0x36},
	{0x02c1, 0x36},
	{0x02c2, 0x36},
	{0x02c3, 0x36},
	{0x02c4, 0xE4},
	{0x02c5, 0xE4},
	{0x02c6, 0xE4},
	{0x02c7, 0xdb},
	{0x061A, 0x01},
	{0x061B, 0x03}, //03
	{0x061C, 0x00}, //00
	{0x061D, 0x00},
	{0x061E, 0x00},
	{0x061F, 0x03},
	{0x0613, 0x01},
	{0x0615, 0x01},
	{0x0616, 0x01},
	{0x0617, 0x00},
	{0x0619, 0x00}, //01
	{0x0008, 0x0F},
	{0x0663, 0x05},
	{0x0660, 0x03},
	
	{0x0042, 0x00},
	{0x0043, 0x20},
	
	{0x0034, 0x03},
	{0x0035, 0xD4},
	
	{0x0010, 0x41},
	{0x0011, 0x04},
	{0x0500, 0x19},
	
	{0x0630, 0x05},
	{0x0631, 0x18},
	{0x0632, 0x03},
	{0x0633, 0xD4},
	{0x0001, 0x00},

};

static struct v4l2_subdev_info hi542_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
};

static struct msm_camera_i2c_conf_array hi542_init_conf[] = {
	{&hi542_recommend_settings[0],
	ARRAY_SIZE(hi542_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA}
};

static struct msm_camera_i2c_conf_array hi542_confs[] = {
	{&hi542_snap_settings[0],
	ARRAY_SIZE(hi542_snap_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi542_prev_settings[0],
	ARRAY_SIZE(hi542_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_sensor_output_info_t hi542_dimensions[] = {
	{  /* For SNAPSHOT */
		.x_output = 0xA30,  
		.y_output = 0x7A8,  
		.line_length_pclk = 2791,
		.frame_length_lines = 1995,
		.vt_pixel_clk = 84000000,
		.op_pixel_clk = 84000000,
		.binning_factor = 0x0,
	},
	{/* For PREVIEW */
		.x_output = 0x518,//1304
		.y_output = 0x3D4, //980
		.line_length_pclk = 2791,
		.frame_length_lines = 1003,
		.vt_pixel_clk = 84000000,
		.op_pixel_clk = 84000000,
		.binning_factor = 0x0,
	},
};

static struct msm_camera_csi_params hi542_csi_params = {
	.data_format = CSI_10BIT,
	.lane_cnt    = 2,
	.lane_assign = 0xe4,
	.dpcm_scheme = 0,
	.settle_cnt  = 0x21,
};

static struct msm_camera_csi_params *hi542_csi_params_array[] = {
	&hi542_csi_params,
	&hi542_csi_params,
};

static struct msm_sensor_output_reg_addr_t hi542_reg_addr = {
	.x_output = 0x0630,
	.y_output = 0x0632,
	.line_length_pclk = 0x0641,
	.frame_length_lines = 0x0643,
};

static struct msm_sensor_id_info_t hi542_id_info = {
	.sensor_id_reg_addr = 0x0004,
	.sensor_id = 0xb1,
};

static struct msm_sensor_exp_gain_info_t hi542_exp_gain_info = {
	.coarse_int_time_addr = 0x0115,
	.global_gain_addr = 0x0129,
	.vert_offset = 0,
};

static int32_t hi542_write_exp_gain(struct msm_sensor_ctrl_t *s_ctrl,
		uint16_t gain, uint32_t line)
{
	int rc = 0;
	unsigned int  max_legal_gain = 0x00, mask = 0xFF;
	unsigned int i, pixels_line, fixed_value, min_line = 4;
	unsigned short values_1[] = { 0, 0, 0, 0, 0};
	unsigned short values_2[] = { 0, 0, 0, 0, 0};
	printk("hi542_write_exp_gain,gain=%d, line=%d\n",gain,line);
	if(gain < max_legal_gain){
		gain = max_legal_gain;
	}

	if(line < min_line){
		line = min_line;
	}

	pixels_line = line * 2791;
	fixed_value = pixels_line + 5582;

	for(i = 1; i < 5; i++){
		values_1[i] = mask & pixels_line;
		values_2[i] = mask & fixed_value;
	pixels_line >>= 8;
	fixed_value >>= 8;
	}

	values_1[0] = gain;
	s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
/* hi542 fixed time update */
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
	s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 11,
	values_2[4],MSM_CAMERA_I2C_BYTE_DATA);
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
	s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 12,
	values_2[3],MSM_CAMERA_I2C_BYTE_DATA);
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
	s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 13,
	values_2[2],MSM_CAMERA_I2C_BYTE_DATA);
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
	s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 14,
	values_2[1],MSM_CAMERA_I2C_BYTE_DATA);
	
/* hi542 max time update */
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
	s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 7,
	values_1[4],MSM_CAMERA_I2C_BYTE_DATA);
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
	s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 8,
	values_1[3],MSM_CAMERA_I2C_BYTE_DATA);
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
	s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 9,
	values_1[2],MSM_CAMERA_I2C_BYTE_DATA);
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
	s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 10,
	values_1[1],MSM_CAMERA_I2C_BYTE_DATA);
	
/* hi542 fixed time update */
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
	s_ctrl->sensor_exp_gain_info->global_gain_addr,
	values_1[0],MSM_CAMERA_I2C_BYTE_DATA);
	

/* hi542 fixed time update */
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
	s_ctrl->sensor_exp_gain_info->coarse_int_time_addr,
	values_1[4],MSM_CAMERA_I2C_BYTE_DATA);
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
	s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 1,
	values_1[3],MSM_CAMERA_I2C_BYTE_DATA);
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
	s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 2,
	values_1[2],MSM_CAMERA_I2C_BYTE_DATA);
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
	s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 3,
	values_1[1],MSM_CAMERA_I2C_BYTE_DATA);
	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	return rc;

}

static int32_t hi542_write_exp_snapshot_gain(struct msm_sensor_ctrl_t *s_ctrl,
		uint16_t gain, uint32_t line)
{
	printk("hi542_write_exp_snapshot_gain,gain=%d, line=%d\n",gain,line);
	return hi542_write_exp_gain(s_ctrl, gain, line);
}

static const struct i2c_device_id hi542_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&hi542_s_ctrl},
	{ }
};

static struct i2c_driver hi542_i2c_driver = {
	.id_table = hi542_i2c_id,
	.probe  = msm_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client hi542_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static int __init msm_sensor_init_module(void)
{
	return i2c_add_driver(&hi542_i2c_driver);
}

static struct v4l2_subdev_core_ops hi542_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops hi542_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops hi542_subdev_ops = {
	.core = &hi542_subdev_core_ops,
	.video  = &hi542_subdev_video_ops,
};

int32_t hi542_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res)
{
	int32_t rc = 0;
	static int csi_config;

	if (update_type == MSM_SENSOR_REG_INIT) {
		CDBG("Register INIT\n");
		printk("##### %s line%d: Register INIT\n",__func__,__LINE__);
		s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);
		msleep(66);
		s_ctrl->curr_csi_params = NULL;
		msm_sensor_enable_debugfs(s_ctrl);
		printk("##### %s line%d: begin to write init setting !\n",__func__,__LINE__);
		msm_sensor_write_init_settings(s_ctrl);
		printk("##### %s line%d: init setting end !\n",__func__,__LINE__);
		csi_config = 0;
	} else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
		CDBG("PERIODIC : %d\n", res);
		s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);
		msleep(66);
		msm_sensor_write_conf_array(
			s_ctrl->sensor_i2c_client,
			s_ctrl->msm_sensor_reg->mode_settings, res);
		msleep(100);
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

		//zxj ++
		#ifdef ORIGINAL_VERSION
		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_PCLK_CHANGE,
			&s_ctrl->sensordata->pdata->ioclk.vfe_clk_rate);
		#else
		printk("##### %s line%d: vfe_clk_rate = %d\n",__func__,__LINE__,vfe_clk_rate);
		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_PCLK_CHANGE, &vfe_clk_rate);	
		#endif
		//zxj --
		
		s_ctrl->func_tbl->sensor_start_stream(s_ctrl);
		msleep(100);
	}
	return rc;
}

extern char cellon_msm_sensor_name[2][20];
int32_t hi542_sensor_match_id(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	uint16_t chipid = 0;
	rc = msm_camera_i2c_read(
			s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_id_info->sensor_id_reg_addr, &chipid,
			MSM_CAMERA_I2C_BYTE_DATA);
	if (rc < 0) {
		pr_err("%s: %s: read id failed\n", __func__,
			s_ctrl->sensordata->sensor_name);
		return rc;
	}

	CDBG("hi542_sensor id: 0x%x\n", chipid);
	printk("sensor_name: %s\n", s_ctrl->sensordata->sensor_name);
	printk("camera_type: %d\n", s_ctrl->sensordata->camera_type);

	if(s_ctrl->sensordata->camera_type == FRONT_CAMERA_2D)
		strcpy(cellon_msm_sensor_name[FRONT_CAMERA_2D], s_ctrl->sensordata->sensor_name);
	if(s_ctrl->sensordata->camera_type == BACK_CAMERA_2D)
		strcpy(cellon_msm_sensor_name[BACK_CAMERA_2D], s_ctrl->sensordata->sensor_name);
	
	if (chipid != s_ctrl->sensor_id_info->sensor_id) {
		pr_err("hi542_sensor_match_id chip id doesnot match\n");
		return -ENODEV;
	}
	return rc;
}

static struct msm_sensor_fn_t hi542_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain = hi542_write_exp_gain,
	.sensor_write_snapshot_exp_gain = hi542_write_exp_snapshot_gain,
	.sensor_csi_setting = hi542_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = msm_sensor_power_up,
	.sensor_power_down = msm_sensor_power_down,
	.sensor_match_id = hi542_sensor_match_id,
};

static struct msm_sensor_reg_t hi542_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = hi542_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(hi542_start_settings),
	.stop_stream_conf = hi542_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(hi542_stop_settings),
	.group_hold_on_conf = hi542_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(hi542_groupon_settings),
	.group_hold_off_conf = hi542_groupoff_settings,
	.group_hold_off_conf_size =ARRAY_SIZE(hi542_groupoff_settings),
	.init_settings = &hi542_init_conf[0],
	.init_size = ARRAY_SIZE(hi542_init_conf),
	.mode_settings = &hi542_confs[0],
	.output_settings = &hi542_dimensions[0],
	.num_conf = ARRAY_SIZE(hi542_confs),
};

static struct msm_sensor_ctrl_t hi542_s_ctrl = {
	.msm_sensor_reg = &hi542_regs,
	.sensor_i2c_client = &hi542_sensor_i2c_client,
	.sensor_i2c_addr = 0x40,
	.sensor_output_reg_addr = &hi542_reg_addr,
	.sensor_id_info = &hi542_id_info,
	.sensor_exp_gain_info = &hi542_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csic_params = &hi542_csi_params_array[0],
	.msm_sensor_mutex = &hi542_mut,
	.sensor_i2c_driver = &hi542_i2c_driver,
	.sensor_v4l2_subdev_info = hi542_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(hi542_subdev_info),
	.sensor_v4l2_subdev_ops = &hi542_subdev_ops,
	.func_tbl = &hi542_func_tbl,
	.clk_rate = MSM_SENSOR_MCLK_24HZ,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Hynix 5MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");


