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
#define SENSOR_NAME "s5k3h2y"
#define PLATFORM_DRIVER_NAME "msm_camera_s5k3h2y"
#define s5k3h2y_obj s5k3h2y_##obj

/* TO DO - Currently ov5647 typical values are used
 * Need to get the exact values */
#define S5K3H2Y_RG_RATIO_TYPICAL_VALUE 64 /* R/G of typical camera module */
#define S5K3H2Y_BG_RATIO_TYPICAL_VALUE 105 /* B/G of typical camera module */

#ifdef CDBG
#undef CDBG
#endif
#ifdef CDBG_HIGH
#undef CDBG_HIGH
#endif

#define S5K3H2Y_VERBOSE_DGB

#ifdef S5K3H2Y_VERBOSE_DGB
#define CDBG(fmt, args...) printk(fmt, ##args)
#define CDBG_HIGH(fmt, args...) printk(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#define CDBG_HIGH(fmt, args...) printk(fmt, ##args)
#endif

//add by lzj
#define S5K3H2Y_EEP_I2C_ADDR			0xA<<3 	// GT24C16 : 1010XXX | PAGE_ADDR(0~7)
#define S5K3H2Y_EEP_PAGE_SIZE			256		// GT24C16
//static uint16_t Cal_eeprom_checksum;
//add end
#define MSB                             1
#define LSB                             0
//zxj ++
#ifdef ORIGINAL_VERSION
#else
#define S5K3H2Y_AF_MSB 0x3619
#define S5K3H2Y_AF_LSB 0x3618

uint16_t s5k3h2y_damping_threshold = 10;

int8_t S3_to_0 = 0x01;

#endif
//zxj --

//extern int lcd_camera_power_onoff(int on);
extern int camera_power_onoff(int on);

//add by lzj
static int32_t vfe_clk = 266667000;
// add end

DEFINE_MUTEX(s5k3h2y_mut);
static struct msm_sensor_ctrl_t s5k3h2y_s_ctrl;

struct otp_struct {
	uint8_t customer_id;
	uint8_t module_integrator_id;
	uint8_t lens_id;
	uint8_t rg_ratio;
	uint8_t bg_ratio;
	uint8_t user_data[5];
} st_s5k3h2y_otp;

static struct msm_camera_i2c_reg_conf s5k3h2y_start_settings[] = {
	{0x0100, 0x01},
};

static struct msm_camera_i2c_reg_conf s5k3h2y_stop_settings[] = {
	{0x0100, 0x00},
};

static struct msm_camera_i2c_reg_conf s5k3h2y_groupon_settings[]={
	{0x0104, 0x01},
};

static struct msm_camera_i2c_reg_conf s5k3h2y_groupoff_settings[]={
	{0x0104, 0x00},
};

static struct msm_camera_i2c_reg_conf s5k3h2y_prev_settings[] = {
// 1640*1232  30fps
//	Readout	subsampling	                                                 
{0x0344,0x00	},//X addr start 0d
{0x0345,0x00 },
{0x0346,0x00	},//Y addr start 0d
{0x0347,0x00 },
{0x0348,0x0C	},//X addr end 3277d
{0x0349,0xCD },
{0x034A,0x09	},//Y addr end 2463d
{0x034B,0x9F },
{0x0381,0x01	},//x_even_inc = 1
{0x0383,0x03	},//x_odd_inc = 3
{0x0385,0x01	},//y_even_inc = 1
{0x0387,0x03	},//y_odd_inc = 3
{0x0401,0x00	},//Derating_en  = 0 (disable)
{0x0405,0x10 },
{0x0700,0x05	},//fifo_water_mark_pixels = 1328
{0x0701,0x30 },
{0x034C,0x06	},//x_output_size = 1640
{0x034D,0x60 },
{0x034E,0x04	},//y_output_size = 1232
{0x034F,0xD0 },
		
{0x0200,0x02	},//fine integration time
{0x0201,0x50 },
//{0x0202,0x04	},//Coarse integration time
//{0x0203,0xD8 },
//{0x0204,0x00	},//Analog gain
//{0x0205,0x20 },
{0x0342,0x0D	},//Line_length_pck 3470d
{0x0343,0x8E },
{0x0340,0x04	},//Frame_length_lines 1248d
{0x0341,0xE0 },
		
//Manufacture Setting		
{0x300E,0x2D	},//Hbinnning[2] : 1b enale / 0b disable
{0x31A3,0x40	},//Vbinning enable[6] : 1b enale / 0b disable
{0x301A,0x77	},//"In case of using the Vt_Pix_Clk more than 137Mhz, 0xA7h should be adopted! "
{0x3053,0xCF },
};

static struct msm_camera_i2c_reg_conf s5k3h2y_snap_settings[] = {
// 3264*2448  15fps	
//	Readout	Full	                                                 
//{0x0344,0x00},          
//{0x0345,0x08},         // x_addr_start (8d)   
//{0x0346,0x01},
//{0x0347,0x3a},		// y_addr_start (314d)
//{0x0348,0x0C},
{0x0349,0xCf	},	// x_addr_end (3279d)
//{0x034A,0x08},
//{0x034B,0x65},		// y_addr_end (2149d)

{0x0381,0x01	},//x_even_inc = 1
{0x0383,0x01	},//x_odd_inc = 1
{0x0385,0x01	},//y_even_inc = 1
{0x0387,0x01	},//y_odd_inc = 1
{0x0401,0x00	},//Derating_en  = 0 (disable)

{0x0405,0x1B	},	// scaling ratio 27/16
{0x0700,0x05	},	
{0x0701,0xce }, 	// fifo size 1486d
{0x034c,0x0c },     //x_output_size 3264d
{0x034d,0xc0 },
{0x034e,0x09 },    //y_output_size 2448d
{0x034f,0x90 },

{0x0200,0x02	},//fine integration time
{0x0201,0x50},
//{0x0202,0x04	},//Coarse integration time
//{0x0203,0xE7},
//{0x0204,0x00	},//Analog gain
//{0x0205,0x20},
{0x0342,0x0D	},//Line_length_pck 3470d
{0x0343,0x8E},
{0x0340,0x09	},//Frame_length_lines 2480d
{0x0341,0xB0},

//Manufacture Setting    
{0x300E,0x29	},//Hbinnning[2] : 1b enale / 0b disable
{0x31A3,0x00	},//Vbinning enable[6] : 1b enale / 0b disable
{0x301A,0x77	},//"In case of using the Vt_Pix_Clk more than 137Mhz, 0xA7h should be adopted! "
{0x3053,0xCF	},     
};


static struct msm_camera_i2c_reg_conf s5k3h2y_reset_settings[] = {
	{0x0103, 0x01},
};

static struct msm_camera_i2c_reg_conf s5k3h2y_recommend_settings[] = {
//{0x0100,0x00},//stream off
//{0x0103,0x01},//sw rstn
//{0x0101,0x00},//Mirror off
	
{0x3065,0x35},	
{0x310E,0x00},	
{0x3098,0xAB},
{0x30C7,0x0A},	
{0x309A,0x01},	
{0x310D,0xC6},	
{0x30c3,0x40},	
{0x30BB,0x02},	
{0x30BC,0x38	},//According to MCLK, these registers should be changed!
{0x30BD,0x40	},//
{0x3110,0x70	},//
{0x3111,0x80	},//
{0x3112,0x7B	},//
{0x3113,0xC0	},//
{0x30C7,0x1A	},//

//	Manufacture Setting	
{0x3000,0x08	},
{0x3001,0x05	},
{0x3002,0x0D	},
{0x3003,0x21	},
{0x3004,0x62	},
{0x3005,0x0B	},
{0x3006,0x6D	},
{0x3007,0x02	},
{0x3008,0x62	},
{0x3009,0x62	},
{0x300A,0x41	},
{0x300B,0x10	},
{0x300C,0x21	},
{0x300D,0x04	},
{0x307E,0x03	},
{0x307F,0xA5	},
{0x3080,0x04 },	
{0x3081,0x29	},
{0x3082,0x03	},
{0x3083,0x21	},
{0x3011,0x5F	},
{0x3156,0xE2	},
{0x3027,0xBE },
{0x300f,0x02	},
{0x3010,0x10	},
{0x3017,0x74	},
{0x3018,0x00	},
{0x3020,0x02	},
{0x3021,0x00},
{0x3023,0x80	},
{0x3024,0x08	},
{0x3025,0x08	},
{0x301C,0xD4	},
{0x315D,0x00	},
{0x3054,0x00	},
{0x3055,0x35	},
{0x3062,0x04	},
{0x3063,0x38	},
{0x31A4,0x04	},
{0x3016,0x54	},
{0x3157,0x02	},
{0x3158,0x00	},
{0x315B,0x02	},
{0x315C,0x00	},
{0x301B,0x05	},
{0x3028,0x41	},
{0x302A,0x10	},
{0x3060,0x00	},
{0x302D,0x19 },
{0x302B,0x05	},
{0x3072,0x13	},
{0x3073,0x21	},
{0x3074,0x82	},
{0x3075,0x20	},
{0x3076,0xA2	},
{0x3077,0x02	},
{0x3078,0x91	},
{0x3079,0x91	},
{0x307A,0x61	},
{0x307B,0x28	},
{0x307C,0x31	},
{0x304E,0x40},
{0x304F,0x01},
{0x3050,0x00},
{0x3088,0x01},
{0x3089,0x00},
{0x3210,0x01},
{0x3211,0x00},
{0x308E,0x01},
{0x308F,0x8F},
{0x3064,0x03},
{0x31A7,0x0F},

//Mode Setting PLL EXCLK 24Mhz, vt_pix_clk_freq_mhz=129.6Mhz,op_sys_clk_freq_mhz=648Mhz
{0x0305,0x04	},//pre_pll_clk_div = 4
{0x0306,0x00	},//pll_multiplier 
{0x0307,0x6C	},//pll_multiplier  = 108
{0x0303,0x01	},//vt_sys_clk_div = 1
{0x0301,0x05	},//vt_pix_clk_div = 5
{0x030B,0x01	},//op_sys_clk_div = 1
{0x0309,0x05	},//op_pix_clk_div = 5
{0x30CC,0xB0	},//DPHY_band_ctrl 640¢¦690MHz
{0x31A1,0x58	},//"DBR_CLK = PLL_CLK / DIV_DBR(0x31A1[3:0]) = 648Mhz / 8 = 81Mhz[7:4] must be same as vt_pix_clk_div (0x0301)"
                            
//	Readout	subsampling	                                                 
{0x0344,0x00	},//X addr start 0d
{0x0345,0x00 },
{0x0346,0x00	},//Y addr start 0d
{0x0347,0x00 },
{0x0348,0x0C	},//X addr end 3277d
{0x0349,0xCD },
{0x034A,0x09	},//Y addr end 2463d
{0x034B,0x9F },
{0x0381,0x01	},//x_even_inc = 1
{0x0383,0x03	},//x_odd_inc = 3
{0x0385,0x01	},//y_even_inc = 1
{0x0387,0x03	},//y_odd_inc = 3
{0x0401,0x00	},//Derating_en  = 0 (disable)
{0x0405,0x10 },
{0x0700,0x05	},//fifo_water_mark_pixels = 1328
{0x0701,0x30 },
{0x034C,0x06	},//x_output_size = 1640
{0x034D,0x60 },
{0x034E,0x04	},//y_output_size = 1232
{0x034F,0xD0 },
		
{0x0200,0x02	},//fine integration time
{0x0201,0x50 },
{0x0202,0x04	},//Coarse integration time
{0x0203,0xD8 },
{0x0204,0x00	},//Analog gain
{0x0205,0x20 },
{0x0342,0x0D	},//Line_length_pck 3470d
{0x0343,0x8E },
{0x0340,0x04	},//Frame_length_lines 1248d
{0x0341,0xE0 },
		
//Manufacture Setting		
{0x300E,0x2D	},//Hbinnning[2] : 1b enale / 0b disable
{0x31A3,0x40	},//Vbinning enable[6] : 1b enale / 0b disable
{0x301A,0x77	},//"In case of using the Vt_Pix_Clk more than 137Mhz, 0xA7h should be adopted! "
{0x3053,0xCF },
     
//{0x0100,0x01 },// streaming on
};

static struct v4l2_subdev_info s5k3h2y_subdev_info[] = {
	{
		.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt    = 1,
		.order    = 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_camera_i2c_conf_array s5k3h2y_init_conf[] = {
	{&s5k3h2y_reset_settings[0],
	ARRAY_SIZE(s5k3h2y_reset_settings), 50, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2y_recommend_settings[0],
	ARRAY_SIZE(s5k3h2y_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA}
};

static struct msm_camera_i2c_conf_array s5k3h2y_confs[] = {
	{&s5k3h2y_snap_settings[0],
	ARRAY_SIZE(s5k3h2y_snap_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2y_prev_settings[0],
	ARRAY_SIZE(s5k3h2y_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_sensor_output_info_t s5k3h2y_dimensions[] = {
#if 0
{
		.x_output = 0xCC0,
		.y_output = 0x990,
		.line_length_pclk = 0xD8E,
		.frame_length_lines = 0x9B0,
		.vt_pixel_clk = 129600000,
		.op_pixel_clk = 648000000,
		.binning_factor = 1,
	},
#endif
	{
		.x_output = 0xCC0,
		.y_output = 0x990,
		.line_length_pclk = 0xD8E,
		.frame_length_lines = 0x9B0,
		.vt_pixel_clk = 129084000,
		.op_pixel_clk = 176000000,
		.binning_factor = 1,
	},
	{
		.x_output = 0x660,
		.y_output = 0x4D0,
		.line_length_pclk = 0xD8E,
		.frame_length_lines = 0x4E0,
		.vt_pixel_clk = 129600000,
		.op_pixel_clk = 648000000,
		.binning_factor = 1,
	},
};

static struct msm_camera_csi_params s5k3h2y_csi_params = {
	.data_format = CSI_10BIT,
	.lane_cnt    = 2,
	.lane_assign = 0xe4,
	.dpcm_scheme = 0,
	.settle_cnt  = 14,
};

static struct msm_camera_csi_params *s5k3h2y_csi_params_array[] = {
	&s5k3h2y_csi_params,
	&s5k3h2y_csi_params,
};

static struct msm_sensor_output_reg_addr_t s5k3h2y_reg_addr = {
	.x_output = 0x034c,
	.y_output = 0x034e,
	.line_length_pclk = 0x0342,
	.frame_length_lines = 0x0340,
};

static struct msm_sensor_id_info_t s5k3h2y_id_info = {
	.sensor_id_reg_addr = 0x0000,
	.sensor_id = 0x382B,
};

static struct msm_sensor_exp_gain_info_t s5k3h2y_exp_gain_info = {
	.coarse_int_time_addr = 0x0202,
	.global_gain_addr = 0x0204,
	.vert_offset = 8,
};


//add by lzj
static int s5k3h2y_i2c_rxburst(struct msm_sensor_ctrl_t *s_ctrl,unsigned short saddr, unsigned char *rxdata,
	int length)
{
	struct i2c_msg msgs[] = {
		{
			.addr  = saddr,
			.flags = I2C_M_RD,
			.len   = length,
			.buf   = rxdata,
		},
	};

	if (i2c_transfer(s_ctrl->msm_sensor_client->adapter, msgs, 2) < 0) {
		CDBG("s5k3h2y_i2c_rxdata failed!\n");
		return -EIO;
	}

	return 0;
}

static void s5k3h2y_get_calibration_data_from_eeprom(struct msm_sensor_ctrl_t *s_ctrl,uint16_t idx, char *data)
{
       #if 1
	int32_t rc = 0, cnt = 0, i = 0;
	unsigned char buf[S5K3H2Y_EEP_PAGE_SIZE];
	unsigned char pageAddr = idx*3;
	//unsigned short raddr = 0, saddr = 0;
	CDBG("#### lzj : %s idx : %d \n",__func__,idx);

	if (s_ctrl==NULL) CDBG("#### lzj : %s s_ctrl null \n",__func__);
	if (data==NULL) CDBG("#### lzj : %s data null \n",__func__);

	
	memset(data, 0, MAX_CAL_DATA_PACKET_LEN);

	// Cal data use 5pages(0~4) of eeprom /start address, page unit? 0page ff?, Memory Map check
	while(pageAddr < 5 && cnt < 3)
	{
		memset(buf, 0, sizeof(buf));
		rc = s5k3h2y_i2c_rxburst(s_ctrl,S5K3H2Y_EEP_I2C_ADDR | pageAddr, 
			buf, S5K3H2Y_EEP_PAGE_SIZE);
		//CDBG("s5k4e5y1_i2c_rxburst] rc = %d", rc);
		memcpy(data+cnt*S5K3H2Y_EEP_PAGE_SIZE, buf, S5K3H2Y_EEP_PAGE_SIZE);
		cnt++;
		pageAddr++;
	}

	// for debug
	CDBG("%s: \n", __func__);
	for (i = 0; i < cnt*S5K3H2Y_EEP_PAGE_SIZE; i++)
		CDBG("index = %d, %x ", i, *(data+i));
       #else
	int32_t rc = 0, i;

	Cal_eeprom_checksum = 0;
	
        for (i = 0; i < MAX_CAL_DATA_PACKET_LEN; i++){
                *(data+i) = (char)(i&0xFF);
                Cal_eeprom_checksum += *(data+i);

		  CDBG("%s 1 : idx = %x, *(data+%d) = %x, checksum = %d\n", __func__, idx, i,
			(char)(i&0xFF), Cal_eeprom_checksum);
                }

        if (idx==1){
                *(data+1117-MAX_CAL_DATA_PACKET_LEN) = (char)(Cal_eeprom_checksum&0xFF);
		CDBG("%s : data+1117-MAX_CAL_DATA_PACKET_LEN = %d\n", __func__, (char)(Cal_eeprom_checksum&0xFF));
        	}	
        for (i = 0; i < MAX_CAL_DATA_PACKET_LEN; i++)
		CDBG("%x ", *(data+i));
	#endif

}



//add end

inline uint8_t s5k3h2y_sa8q_byte(uint16_t word, uint8_t offset)
{
	return word >> (offset * BITS_PER_BYTE);
}


static int32_t s5k3h2y_write_exp_gain(struct msm_sensor_ctrl_t *s_ctrl,
		uint16_t gain, uint32_t line)
{
	/* Begin - MCNEX */
	/* remove red line noise in low light environment */
	/* This code comes from msm8260 */
	/* So, needs debuging based on msm8960 */
#if 0
	uint16_t max_legal_gain = 0x0200;
	int32_t rc = 0;
	static uint32_t fl_lines;
	//gain = 0x80;
	//CDBG("%s gain : %d  , line : %d ",__func__, gain, line);
	if (gain > max_legal_gain) {
		CDBG("Max legal gain Line:%d\n", __LINE__);
		gain = max_legal_gain;
	}
	CDBG("%s gain : %d  , line : %d ",__func__, gain, line);
	/* Analogue Gain */
	//msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0104, 
	//	0x01, MSM_CAMERA_I2C_BYTE_DATA);
	//s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0204, 
		(gain&0xFF00)>>8, MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0205, 
		gain&0xFF, MSM_CAMERA_I2C_BYTE_DATA);
	//CDBG("org fl_lines(0x%x), gain(0x%x) @LINE:%d \n", s_ctrl->curr_frame_length_lines, gain, __LINE__);

	if (line > (s_ctrl->curr_frame_length_lines - 4)) {
		fl_lines = line+4;
		s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0340, 
			(fl_lines&0xFF00)>>8, MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0341, 
			fl_lines&0xFF, MSM_CAMERA_I2C_BYTE_DATA);

		/* Coarse Integration Time */
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0202, 
			(line&0xFF00)>>8, MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0203, 
			line&0xFF, MSM_CAMERA_I2C_BYTE_DATA);
		s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
		//CDBG("fl_lines(0x%x), line(0x%x) @LINE:%d \n", fl_lines, line, __LINE__);
		
	} else if (line < (fl_lines - 4)) {
		fl_lines = line+4;
		if (fl_lines < s_ctrl->curr_frame_length_lines)
			fl_lines = s_ctrl->curr_frame_length_lines;

		s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0340, 
			(fl_lines&0xFF00)>>8, MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0341, 
			fl_lines&0xFF, MSM_CAMERA_I2C_BYTE_DATA);

		/* Coarse Integration Time */
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0202, 
			(line&0xFF00)>>8, MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0203, 
			line&0xFF, MSM_CAMERA_I2C_BYTE_DATA);
		s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
		//CDBG("fl_lines(0x%x), line(0x%x) @LINE:%d \n", fl_lines, line, __LINE__);

	} else {
		fl_lines = line+4;
		s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0340, 
			(fl_lines&0xFF00)>>8, MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0341, 
			fl_lines&0xFF, MSM_CAMERA_I2C_BYTE_DATA);

		/* Coarse Integration Time */
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0202, 
			(line&0xFF00)>>8, MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0203, 
			line&0xFF, MSM_CAMERA_I2C_BYTE_DATA);
		s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
		//CDBG("fl_lines(0x%x), line(0x%x) @LINE:%d \n", fl_lines, line, __LINE__);
	}

	//msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0104, 
	//	0x00, MSM_CAMERA_I2C_BYTE_DATA);	
	//s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	return rc;
#else
#if 0
	uint32_t fl_lines, offset;
	uint8_t int_time[3];
	fl_lines =
		(s_ctrl->curr_frame_length_lines * s_ctrl->fps_divider) / Q10;
	offset = s_ctrl->sensor_exp_gain_info->vert_offset;
	if (line > (fl_lines - offset))
		fl_lines = line + offset;

	/* Begin - MCNEX */
	/* temporarily hard-code exposure gain */
	/* This code will be changed when camera tuning is performed */
#if 1
	gain |= 0x20;
#endif
	/* End - MCNEX */

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
#else
	/* jason.lee ,2012-10-23 , C8680, fix camera full size could stop when facing to brightness environment in preview mode { */
	uint16_t max_legal_gain = 0x0200;
	int32_t rc = 0;
	static uint32_t fl_lines, offset;
	

	offset = s_ctrl->sensor_exp_gain_info->vert_offset;
	if (gain > max_legal_gain) {
		CDBG("Max legal gain Line:%d\n", __LINE__);
		gain = max_legal_gain;
	}

	/* Analogue Gain */
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr,
		s5k3h2y_sa8q_byte(gain, MSB),
		MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr + 1,
		s5k3h2y_sa8q_byte(gain, LSB),
		MSM_CAMERA_I2C_BYTE_DATA);
	
	fl_lines = s_ctrl->curr_frame_length_lines;
	fl_lines = (fl_lines * s_ctrl->fps_divider) / Q10;
	if (line > (fl_lines - offset)) {
		fl_lines = line + offset;
		s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_output_reg_addr->frame_length_lines,
			s5k3h2y_sa8q_byte(fl_lines, MSB),
			MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_output_reg_addr->frame_length_lines + 1,
			s5k3h2y_sa8q_byte(fl_lines, LSB),
			MSM_CAMERA_I2C_BYTE_DATA);
		/* Coarse Integration Time */
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->coarse_int_time_addr,
			s5k3h2y_sa8q_byte(line, MSB),
			MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 1,
			s5k3h2y_sa8q_byte(line, LSB),
			MSM_CAMERA_I2C_BYTE_DATA);
		s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	} else if (line < (fl_lines - offset)) {
		//fl_lines = line + offset;
		if (fl_lines < s_ctrl->curr_frame_length_lines)
			fl_lines = s_ctrl->curr_frame_length_lines;

		s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
		/* Coarse Integration Time */
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->coarse_int_time_addr,
			s5k3h2y_sa8q_byte(line, MSB),
			MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 1,
			s5k3h2y_sa8q_byte(line, LSB),
			MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_output_reg_addr->frame_length_lines,
			s5k3h2y_sa8q_byte(fl_lines, MSB),
			MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_output_reg_addr->frame_length_lines + 1,
			s5k3h2y_sa8q_byte(fl_lines, LSB),
			MSM_CAMERA_I2C_BYTE_DATA);
		s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	} else {
		//fl_lines = line+8;
		s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
		/* Coarse Integration Time */
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->coarse_int_time_addr,
			s5k3h2y_sa8q_byte(line, MSB),
			MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 1,
			s5k3h2y_sa8q_byte(line, LSB),
			MSM_CAMERA_I2C_BYTE_DATA);
		s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	}
  // msleep(200);
	return rc;

	/* } jason.lee ,2012-10-23 , C8680, fix camera full size could stop when facing to brightness environment in preview mode*/

#endif


#endif
	/* End - MCNEX */
}


static const struct i2c_device_id s5k3h2y_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&s5k3h2y_s_ctrl},
	{ }
};


static void s5k3h2y_regulator_onoff(bool on)
{
	int rc = 0;
	static struct regulator *reg_1v2;

	if (on)
	{
		if (!reg_1v2) {
			reg_1v2 = regulator_get(NULL, "ldo05");
			if (IS_ERR(reg_1v2)) {
				pr_err("##### '%s' regulator not found, rc=%ld\n",
					"ext_1v2", IS_ERR(reg_1v2));
				reg_1v2 = NULL;
			}
			rc = regulator_set_voltage(reg_1v2, 1200000, 1200000);
			if (rc < 0) {
				printk("######%s line%d: rc == %d",__func__,__LINE__,rc);
				
			}
		}

		rc = regulator_enable(reg_1v2);
		if (rc) {
			pr_err("##### '%s' regulator enable failed, rc=%d\n",
				"reg_1v2",rc);
		}
	}else{
		rc = regulator_disable(reg_1v2);
		if (rc) {
			pr_err("##### '%s' regulator enable failed, rc=%d\n",
				"reg_1v2", rc);
		}
	}



}


int32_t s5k3h2y_sensor_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int32_t rc = 0;
	struct msm_sensor_ctrl_t *s_ctrl;

	if (camera_id !=0) return -EFAULT;
	CDBG("\n in s5k3h2y_sensor_i2c_probe\n");
	
	rc = msm_sensor_i2c_probe(client, id);
	if (client->dev.platform_data == NULL) {
		pr_err("%s: NULL sensor data\n", __func__);
		return -EFAULT;
	}
	s_ctrl = client->dev.platform_data;
	if (s_ctrl->sensordata->pmic_gpio_enable)
	//	lcd_camera_power_onoff(0);
		camera_power_onoff(0);
	CDBG("######### %s end\n",__func__);
	return rc;
}

int32_t s5k3h2y_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct msm_camera_sensor_info *info = NULL;
	CDBG("######### %s begin\n",__func__);
	s5k3h2y_regulator_onoff(true);
	info = s_ctrl->sensordata;
	if (info->pmic_gpio_enable) {
		info->pmic_gpio_enable = 0;
		//zxj ++
		#ifdef ORIGINAL_VERSION
		lcd_camera_power_onoff(1);
		#else
		camera_power_onoff(1);
		#endif
		//zxj --
	}
	gpio_direction_output(info->sensor_pwd, 0);
	gpio_direction_output(info->sensor_reset, 0);
	usleep_range(5000, 5100);
	rc = msm_sensor_power_up(s_ctrl);
	if (rc < 0) {
		CDBG("%s: msm_sensor_power_up failed\n", __func__);
		return rc;
	}
	/* turn on ldo and vreg */
	gpio_direction_output(info->sensor_pwd, 1);
	//msleep(20);
	gpio_direction_output(info->sensor_reset, 1);
	msleep(10);
	CDBG("######### %s end\n",__func__);
	return rc;
}


int32_t s5k3h2y_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	struct msm_camera_sensor_info *info = s_ctrl->sensordata;
	int rc = 0;
	CDBG("%s IN\r\n", __func__);

	//Stop stream first
	s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);
	msleep(10);

	gpio_direction_output(info->sensor_pwd, 0);
	gpio_direction_output(info->sensor_reset, 0);
	
	usleep_range(5000, 5100);
	msm_sensor_power_down(s_ctrl);
	//msleep(5);
	if (info->pmic_gpio_enable){
		camera_power_onoff(0);
	}

	s5k3h2y_regulator_onoff(false);
	
	return rc;
}



static struct i2c_driver s5k3h2y_i2c_driver = {
	.id_table = s5k3h2y_i2c_id,
	.probe  = s5k3h2y_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client s5k3h2y_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};



static int __init msm_sensor_init_module(void)
{
	CDBG("######### %s begin\n",__func__);
	return i2c_add_driver(&s5k3h2y_i2c_driver);
}

static struct v4l2_subdev_core_ops s5k3h2y_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops s5k3h2y_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops s5k3h2y_subdev_ops = {
	.core = &s5k3h2y_subdev_core_ops,
	.video  = &s5k3h2y_subdev_video_ops,
};

int32_t s5k3h2y_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res)
{
	int32_t rc = 0;
	static int csi_config;
	CDBG("######### %s begin res = %d update_type=%d \n",__func__,res,update_type);
	s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);
	msleep(30);
	if (update_type == MSM_SENSOR_REG_INIT) {
		CDBG("Register INIT\n");
		s_ctrl->curr_csi_params = NULL;
		msm_sensor_enable_debugfs(s_ctrl);
		msm_sensor_write_init_settings(s_ctrl);
		csi_config = 0;
	} else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
		CDBG("PERIODIC : %d\n", res);
		msm_sensor_write_conf_array(
			s_ctrl->sensor_i2c_client,
			s_ctrl->msm_sensor_reg->mode_settings, res);
		msleep(10);
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
		//msleep(50);
	}
	CDBG("######### %s end\n",__func__);
	return rc;
}


static struct msm_sensor_fn_t s5k3h2y_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain = s5k3h2y_write_exp_gain,
	.sensor_write_snapshot_exp_gain = s5k3h2y_write_exp_gain,
	.sensor_csi_setting = s5k3h2y_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = s5k3h2y_sensor_power_up,
	.sensor_power_down = s5k3h2y_sensor_power_down,
	.get_calibration_data_from_eeprom = s5k3h2y_get_calibration_data_from_eeprom,
	//.sensor_adjust_frame_lines = msm_sensor_adjust_frame_lines,
};

static struct msm_sensor_reg_t s5k3h2y_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = s5k3h2y_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(s5k3h2y_start_settings),
	.stop_stream_conf = s5k3h2y_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(s5k3h2y_stop_settings),
	.group_hold_on_conf = s5k3h2y_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(s5k3h2y_groupon_settings),
	.group_hold_off_conf = s5k3h2y_groupoff_settings,
	.group_hold_off_conf_size =	ARRAY_SIZE(s5k3h2y_groupoff_settings),
	.init_settings = &s5k3h2y_init_conf[0],
	.init_size = ARRAY_SIZE(s5k3h2y_init_conf),
	.mode_settings = &s5k3h2y_confs[0],
	.output_settings = &s5k3h2y_dimensions[0],
	.num_conf = ARRAY_SIZE(s5k3h2y_confs),
};

static struct msm_sensor_ctrl_t s5k3h2y_s_ctrl = {
	.msm_sensor_reg = &s5k3h2y_regs,
	.sensor_i2c_client = &s5k3h2y_sensor_i2c_client,
	.sensor_i2c_addr = 0x6C,
	.sensor_output_reg_addr = &s5k3h2y_reg_addr,
	.sensor_id_info = &s5k3h2y_id_info,
	.sensor_exp_gain_info = &s5k3h2y_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csic_params = &s5k3h2y_csi_params_array[0],
	.msm_sensor_mutex = &s5k3h2y_mut,
	.sensor_i2c_driver = &s5k3h2y_i2c_driver,
	.sensor_v4l2_subdev_info = s5k3h2y_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(s5k3h2y_subdev_info),
	.sensor_v4l2_subdev_ops = &s5k3h2y_subdev_ops,
	.func_tbl = &s5k3h2y_func_tbl,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Omnivison 8MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");

