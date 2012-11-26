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
#define SENSOR_NAME "mt9v113"
#define PLATFORM_DRIVER_NAME "msm_camera_mt9v113"
#define mt9v113_obj mt9v113_##obj
#define MSB                             1
#define LSB                             0

#ifdef CDBG
#undef CDBG
#endif
#ifdef CDBG_HIGH
#undef CDBG_HIGH
#endif

#define MT9V113_VERBOSE_DGB

#ifdef MT9V113_VERBOSE_DGB
#define CDBG(fmt, args...) printk(fmt, ##args)
#define CDBG_HIGH(fmt, args...) printk(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#define CDBG_HIGH(fmt, args...) printk(fmt, ##args)
#endif
extern int lcd_camera_power_onoff(int on);

DEFINE_MUTEX(mt9v113_mut);

static int effect_value = CAMERA_EFFECT_OFF;


static struct msm_camera_i2c_reg_conf mt9v113_start_settings[] = {
	{0x3400, 0x7a3c},  /* streaming on */
};

static struct msm_camera_i2c_reg_conf mt9v113_stop_settings[] = {
	{0x3400, 0x7a36},  /* streaming off*/
};

static struct msm_sensor_ctrl_t mt9v113_s_ctrl;

/*init settings for preview, do not contains delay*/
static struct msm_camera_i2c_reg_conf mt9v113_init_standby_control1[] = {
	{0x0018, 0x4028}, // STANDBY_CONTROL
};
static struct msm_camera_i2c_reg_conf mt9v113_init_reset_misc_control1[] = {
	{0x001A, 0x0011}, // RESET_AND_MISC_CONTROL
};
static struct msm_camera_i2c_reg_conf mt9v113_init_reset_misc_control2[] = {
	{0x001A, 0x0018}, // RESET_AND_MISC_CONTROL
};
static struct msm_camera_i2c_reg_conf mt9v113_init_standby_control2[] = {
		// 24MHz / 26MHz
	//MCLK 24.00; fout:224.00, M:28, N:2, P:0, VCO:448.00, PCLK:14.00, fps:30.01
	{0x0014, 0x2145}, // PLL_CONTROL
	{0x0010, 0x0534}, //PLL Dividers=540
	{0x0012, 0x0000}, //PLL P Dividers=0
	{0x0014, 0x244B}, //PLL control: TEST_BYPASS on=9291

};
static struct msm_camera_i2c_reg_conf mt9v113_init_clk_pll1[] = {
		// Allow PLL to lock
	{0x0014, 0x304B}, //PLL control: PLL_ENABLE on=12363

};
static struct msm_camera_i2c_reg_conf mt9v113_init_clk_pll_enable[] = {
	{0x0014, 0xB04A}, // PLL_CONTROL
	{0x0018, 0x402C}, // STANDBY_CONTROL
};
static struct msm_camera_i2c_reg_conf mt9v113_init_clk_pll_ctl[] = {
	{0x321C, 0x0000}, // OFIFO_CONTROL_STATUS
	{0x3400, 0x7A34}, // MIPI_CONTROL
	{0x098C, 0x02F0}, // MCU_ADDRESS
	{0x0990, 0x0000}, // MCU_DATA_0
	{0x098C, 0x02F2}, // MCU_ADDRESS
	{0x0990, 0x0210}, // MCU_DATA_0
	{0x098C, 0x02F4}, // MCU_ADDRESS
	{0x0990, 0x001A}, // MCU_DATA_0
	{0x098C, 0x2145}, // MCU_ADDRESS
	{0x0990, 0x02F4}, // MCU_DATA_0
	{0x098C, 0xA134}, // MCU_ADDRESS
	{0x0990, 0x0001}, // MCU_DATA_0

};
static struct msm_camera_i2c_reg_conf mt9v113_init_output[] = {
	{0x098C, 0x2703}, //Output Width (A)
	{0x0990, 0x0280}, //=640
	{0x098C, 0x2705}, //Output Height (A)
	{0x0990, 0x01E0}, //=480
	{0x098C, 0x2707}, //Output Width (B)
	{0x0990, 0x0280}, //=640
	{0x098C, 0x2709}, //Output Height (B)
	{0x0990, 0x01E0}, //=480
	{0x098C, 0x270D}, //Row Start (A)
	{0x0990, 0x0000}, //=0
	{0x098C, 0x270F}, //Column Start (A)
	{0x0990, 0x0000}, //=0
	{0x098C, 0x2711}, //Row End (A)
	{0x0990, 0x01E7}, //=487
	{0x098C, 0x2713}, //Column End (A)
	{0x0990, 0x0287}, //=647
	{0x098C, 0x2715}, //Row Speed (A)
	{0x0990, 0x0001}, //=1
	{0x098C, 0x2717}, //Read Mode (A)
	{0x0990, 0x0026}, //=38
	{0x098C, 0x2719}, //sensor_fine_correction (A)
	{0x0990, 0x001A}, //=26
	{0x098C, 0x271B}, //sensor_fine_IT_min (A)
	{0x0990, 0x006B}, //=107
	{0x098C, 0x271D}, //sensor_fine_IT_max_margin (A)
	{0x0990, 0x006B}, //=107
	{0x098C, 0x271F}, //Frame Lines (A)
	{0x0990, 0x01FF}, //=554
	{0x098C, 0x2721}, //Line Length (A)
	{0x0990, 0x0350}, //=842
	{0x098C, 0x2723}, //Row Start (B)
	{0x0990, 0x0000}, //=0
	{0x098C, 0x2725}, //Column Start (B)
	{0x0990, 0x0000}, //=0
	{0x098C, 0x2727}, //Row End (B)
	{0x0990, 0x01E7}, //=487
	{0x098C, 0x2729}, //Column End (B)
	{0x0990, 0x0287}, //=647
	{0x098C, 0x272B}, //Row Speed (B)
	{0x0990, 0x0001}, //=1
	{0x098C, 0x272D}, //Read Mode (B)
	{0x0990, 0x0026}, //=38
	{0x098C, 0x272F}, //sensor_fine_correction (B)
	{0x0990, 0x001A}, //=26
	{0x098C, 0x2731}, //sensor_fine_IT_min (B)
	{0x0990, 0x006B}, //=107
	{0x098C, 0x2733}, //sensor_fine_IT_max_margin (B)
	{0x0990, 0x006B}, //=107
	{0x098C, 0x2735}, //Frame Lines (B)
	{0x0990, 0x01FF}, //=1135
	{0x098C, 0x2737}, //Line Length (B)
	{0x0990, 0x0350}, //=842
	{0x098C, 0x2739}, //Crop_X0 (A)
	{0x0990, 0x0000}, //=0
	{0x098C, 0x273B}, //Crop_X1 (A)
	{0x0990, 0x027F}, //=639
	{0x098C, 0x273D}, //Crop_Y0 (A)
	{0x0990, 0x0000}, //=0
	{0x098C, 0x273F}, //Crop_Y1 (A)
	{0x0990, 0x01DF}, //=479
	{0x098C, 0x2747}, //Crop_X0 (B)
	{0x0990, 0x0000}, //=0
	{0x098C, 0x2749}, //Crop_X1 (B)
	{0x0990, 0x027F}, //=639
	{0x098C, 0x274B}, //Crop_Y0 (B)
	{0x0990, 0x0000}, //=0
	{0x098C, 0x274D}, //Crop_Y1 (B)
	{0x0990, 0x01DF}, //=479
	{0x098C, 0x222D}, //R9 Step
	{0x0990, 0x0080}, //=139
	{0x098C, 0xA408}, //search_f1_50
	{0x0990, 0x001F}, //=33
	{0x098C, 0xA409}, //search_f2_50
	{0x0990, 0x0021}, //=35
	{0x098C, 0xA40A}, //search_f1_60
	{0x0990, 0x0025}, //=40
	{0x098C, 0xA40B}, //search_f2_60
	{0x0990, 0x0027}, //=42
	{0x098C, 0x2411}, //R9_Step_60 (A)
	{0x0990, 0x0080}, //=139
	{0x098C, 0x2413}, //R9_Step_50 (A)
	{0x0990, 0x0099}, //=166
	{0x098C, 0x2415}, //R9_Step_60 (B)
	{0x0990, 0x0080}, //=139
	{0x098C, 0x2417}, //R9_Step_50 (B)
	{0x0990, 0x0099}, //=166
	{0x098C, 0xA404}, //FD Mode
	{0x0990, 0x0010}, //=16
	{0x098C, 0xA40D}, //Stat_min
	{0x0990, 0x0002}, //=2
	{0x098C, 0xA40E}, //Stat_max
	{0x0990, 0x0003}, //=3
	{0x098C, 0xA410}, //Min_amplitude
	{0x0990, 0x000A}, //=10
};

static struct msm_camera_i2c_reg_conf mt9v113_init_tunning_settings1[] = {
	// Lens Shading
	//85% LSC
	//{0x3210, 0x09B0}, // COLOR_PIPELINE_CONTROL
	{0x364E, 0x0210}, // P_GR_P0Q0
	{0x3650, 0x0CCA}, // P_GR_P0Q1
	{0x3652, 0x2D12}, // P_GR_P0Q2
	{0x3654, 0xCD0C}, // P_GR_P0Q3
	{0x3656, 0xC632}, // P_GR_P0Q4
	{0x3658, 0x00D0}, // P_RD_P0Q0
	{0x365A, 0x60AA}, // P_RD_P0Q1
	{0x365C, 0x7272}, // P_RD_P0Q2
	{0x365E, 0xFE09}, // P_RD_P0Q3
	{0x3660, 0xAD72}, // P_RD_P0Q4
	{0x3662, 0x0170}, // P_BL_P0Q0
	{0x3664, 0x5DCB}, // P_BL_P0Q1
	{0x3666, 0x27D2}, // P_BL_P0Q2
	{0x3668, 0xCE4D}, // P_BL_P0Q3
	{0x366A, 0x9DB3}, // P_BL_P0Q4
	{0x366C, 0x0150}, // P_GB_P0Q0
	{0x366E, 0xB809}, // P_GB_P0Q1
	{0x3670, 0x30B2}, // P_GB_P0Q2
	{0x3672, 0x82AD}, // P_GB_P0Q3
	{0x3674, 0xC1D2}, // P_GB_P0Q4
	{0x3676, 0xC4CD}, // P_GR_P1Q0
	{0x3678, 0xBE47}, // P_GR_P1Q1
	{0x367A, 0x5E4F}, // P_GR_P1Q2
	{0x367C, 0x9F10}, // P_GR_P1Q3
	{0x367E, 0xEC30}, // P_GR_P1Q4
	{0x3680, 0x914D}, // P_RD_P1Q0
	{0x3682, 0x846A}, // P_RD_P1Q1
	{0x3684, 0x66AB}, // P_RD_P1Q2
	{0x3686, 0x20D0}, // P_RD_P1Q3
	{0x3688, 0x1714}, // P_RD_P1Q4
	{0x368A, 0x99AC}, // P_BL_P1Q0
	{0x368C, 0x5CED}, // P_BL_P1Q1
	{0x368E, 0x00B1}, // P_BL_P1Q2
	{0x3690, 0x716C}, // P_BL_P1Q3
	{0x3692, 0x9594}, // P_BL_P1Q4
	{0x3694, 0xA22D}, // P_GB_P1Q0
	{0x3696, 0xB88A}, // P_GB_P1Q1
	{0x3698, 0x02B0}, // P_GB_P1Q2
	{0x369A, 0xC38F}, // P_GB_P1Q3
	{0x369C, 0x2B30}, // P_GB_P1Q4
	{0x369E, 0x6B32}, // P_GR_P2Q0
	{0x36A0, 0x128C}, // P_GR_P2Q1
	{0x36A2, 0x2574}, // P_GR_P2Q2
	{0x36A4, 0xD1B3}, // P_GR_P2Q3
	{0x36A6, 0xC2B8}, // P_GR_P2Q4
	{0x36A8, 0x1893}, // P_RD_P2Q0
	{0x36AA, 0x8DB0}, // P_RD_P2Q1
	{0x36AC, 0x2134}, // P_RD_P2Q2
	{0x36AE, 0x9014}, // P_RD_P2Q3
	{0x36B0, 0xFC57}, // P_RD_P2Q4
	{0x36B2, 0x2DB2}, // P_BL_P2Q0
	{0x36B4, 0x8FB1}, // P_BL_P2Q1
	{0x36B6, 0x9832}, // P_BL_P2Q2
	{0x36B8, 0x1CD4}, // P_BL_P2Q3
	{0x36BA, 0xE437}, // P_BL_P2Q4
	{0x36BC, 0x5992}, // P_GB_P2Q0
	{0x36BE, 0x99AF}, // P_GB_P2Q1
	{0x36C0, 0x0F54}, // P_GB_P2Q2
	{0x36C2, 0x9A52}, // P_GB_P2Q3
	{0x36C4, 0xB358}, // P_GB_P2Q4
	{0x36C6, 0xC3EE}, // P_GR_P3Q0
	{0x36C8, 0xC3F1}, // P_GR_P3Q1
	{0x36CA, 0x94D2}, // P_GR_P3Q2
	{0x36CC, 0x4175}, // P_GR_P3Q3
	{0x36CE, 0x4EB7}, // P_GR_P3Q4
	{0x36D0, 0xF310}, // P_RD_P3Q0
	{0x36D2, 0x0C51}, // P_RD_P3Q1
	{0x36D4, 0x6C75}, // P_RD_P3Q2
	{0x36D6, 0xDA96}, // P_RD_P3Q3
	{0x36D8, 0x8FF9}, // P_RD_P3Q4
	{0x36DA, 0x9C10}, // P_BL_P3Q0
	{0x36DC, 0x99B2}, // P_BL_P3Q1
	{0x36DE, 0xFCD4}, // P_BL_P3Q2
	{0x36E0, 0x6CD5}, // P_BL_P3Q3
	{0x36E2, 0x7E98}, // P_BL_P3Q4
	{0x36E4, 0xE64E}, // P_GB_P3Q0
	{0x36E6, 0x8E72}, // P_GB_P3Q1
	{0x36E8, 0x38D2}, // P_GB_P3Q2
	{0x36EA, 0x4935}, // P_GB_P3Q3
	{0x36EC, 0xBCB6}, // P_GB_P3Q4
	{0x36EE, 0xB5F3}, // P_GR_P4Q0
	{0x36F0, 0xBAD4}, // P_GR_P4Q1
	{0x36F2, 0x8E39}, // P_GR_P4Q2
	{0x36F4, 0x1FF8}, // P_GR_P4Q3
	{0x36F6, 0x1D3C}, // P_GR_P4Q4
	{0x36F8, 0x8CB3}, // P_RD_P4Q0
	{0x36FA, 0x8834}, // P_RD_P4Q1
	{0x36FC, 0xBF58}, // P_RD_P4Q2
	{0x36FE, 0x4239}, // P_RD_P4Q3
	{0x3700, 0x19F9}, // P_RD_P4Q4
	{0x3702, 0x770D}, // P_BL_P4Q0
	{0x3704, 0x7234}, // P_BL_P4Q1
	{0x3706, 0xCB98}, // P_BL_P4Q2
	{0x3708, 0x84B9}, // P_BL_P4Q3
	{0x370A, 0x33FC}, // P_BL_P4Q4
	{0x370C, 0xA1D2}, // P_GB_P4Q0
	{0x370E, 0xAE33}, // P_GB_P4Q1
	{0x3710, 0x8E79}, // P_GB_P4Q2
	{0x3712, 0x4AB8}, // P_GB_P4Q3
	{0x3714, 0x2D1C}, // P_GB_P4Q4
	{0x3644, 0x013C}, // POLY_ORIGIN_C
	{0x3642, 0x00E8}, // POLY_ORIGIN_R
	{0x3210, 0x09B8}, // COLOR_PIPELINE_CONTROL
	// Char Setting and fine-tuning
	{0x098C, 0xAB1F}, // MCU_ADDRESS [HG_LLMODE]
	{0x0990, 0x00C7}, // MCU_DATA_0
	{0x098C, 0xAB31}, // MCU_ADDRESS [HG_NR_STOP_G]
	{0x0990, 0x001E}, // MCU_DATA_0
	{0x098C, 0x274F}, // MCU_ADDRESS [RESERVED_MODE_4F]
	{0x0990, 0x0004}, // MCU_DATA_0
	{0x098C, 0x2741}, // MCU_ADDRESS [RESERVED_MODE_41]
	{0x0990, 0x0004}, // MCU_DATA_0
	{0x098C, 0xAB20}, // MCU_ADDRESS [HG_LL_SAT1]
	{0x0990, 0x0054}, // MCU_DATA_0
	{0x098C, 0xAB21}, // MCU_ADDRESS [HG_LL_INTERPTHRESH1]
	{0x0990, 0x0046}, // MCU_DATA_0
	{0x098C, 0xAB22}, // MCU_ADDRESS [HG_LL_APCORR1]
	{0x0990, 0x0002}, // MCU_DATA_0
	{0x098C, 0xAB24}, // MCU_ADDRESS [HG_LL_SAT2]
	{0x0990, 0x0005}, // MCU_DATA_0
	{0x098C, 0x2B28}, // MCU_ADDRESS [HG_LL_BRIGHTNESSSTART]
	{0x0990, 0x170C}, // MCU_DATA_0
	{0x098C, 0x2B2A}, // MCU_ADDRESS [HG_LL_BRIGHTNESSSTOP]
	{0x0990, 0x3E80}, // MCU_DATA_0
	//{0x3210, 0x09B8}, // COLOR_PIPELINE_CONTROL
	{0x098C, 0x2306}, // MCU_ADDRESS [AWB_CCM_L_0]
	{0x0990, 0x0315}, // MCU_DATA_0
	{0x098C, 0x2308}, // MCU_ADDRESS [AWB_CCM_L_1]
	{0x0990, 0xFDDC}, // MCU_DATA_0
	{0x098C, 0x230A}, // MCU_ADDRESS [AWB_CCM_L_2]
	{0x0990, 0x003A}, // MCU_DATA_0
	{0x098C, 0x230C}, // MCU_ADDRESS [AWB_CCM_L_3]
	{0x0990, 0xFF58}, // MCU_DATA_0
	{0x098C, 0x230E}, // MCU_ADDRESS [AWB_CCM_L_4]
	{0x0990, 0x02B7}, // MCU_DATA_0
	{0x098C, 0x2310}, // MCU_ADDRESS [AWB_CCM_L_5]
	{0x0990, 0xFF31}, // MCU_DATA_0
	{0x098C, 0x2312}, // MCU_ADDRESS [AWB_CCM_L_6]
	{0x0990, 0xFF4C}, // MCU_DATA_0
	{0x098C, 0x2314}, // MCU_ADDRESS [AWB_CCM_L_7]
	{0x0990, 0xFE4C}, // MCU_DATA_0
	{0x098C, 0x2316}, // MCU_ADDRESS [AWB_CCM_L_8]
	{0x0990, 0x039E}, // MCU_DATA_0
	{0x098C, 0x2318}, // MCU_ADDRESS [AWB_CCM_L_9]
	{0x0990, 0x001C}, // MCU_DATA_0
	{0x098C, 0x231A}, // MCU_ADDRESS [AWB_CCM_L_10]
	{0x0990, 0x0039}, // MCU_DATA_0
	{0x098C, 0x231C}, // MCU_ADDRESS [AWB_CCM_RL_0]
	{0x0990, 0x007F}, // MCU_DATA_0
	{0x098C, 0x231E}, // MCU_ADDRESS [AWB_CCM_RL_1]
	{0x0990, 0xFF77}, // MCU_DATA_0
	{0x098C, 0x2320}, // MCU_ADDRESS [AWB_CCM_RL_2]
	{0x0990, 0x000A}, // MCU_DATA_0
	{0x098C, 0x2322}, // MCU_ADDRESS [AWB_CCM_RL_3]
	{0x0990, 0x0020}, // MCU_DATA_0
	{0x098C, 0x2324}, // MCU_ADDRESS [AWB_CCM_RL_4]
	{0x0990, 0x001B}, // MCU_DATA_0
	{0x098C, 0x2326}, // MCU_ADDRESS [AWB_CCM_RL_5]
	{0x0990, 0xFFC6}, // MCU_DATA_0
	{0x098C, 0x2328}, // MCU_ADDRESS [AWB_CCM_RL_6]
	{0x0990, 0x0086}, // MCU_DATA_0
	{0x098C, 0x232A}, // MCU_ADDRESS [AWB_CCM_RL_7]
	{0x0990, 0x00B5}, // MCU_DATA_0
	{0x098C, 0x232C}, // MCU_ADDRESS [AWB_CCM_RL_8]
	{0x0990, 0xFEC3}, // MCU_DATA_0
	{0x098C, 0x232E}, // MCU_ADDRESS [AWB_CCM_RL_9]
	{0x0990, 0x0001}, // MCU_DATA_0
	{0x098C, 0x2330}, // MCU_ADDRESS [AWB_CCM_RL_10]
	{0x0990, 0xFFEF}, // MCU_DATA_0
	{0x098C, 0xA348}, // MCU_ADDRESS [AWB_GAIN_BUFFER_SPEED]
	{0x0990, 0x0008}, // MCU_DATA_0
	{0x098C, 0xA349}, // MCU_ADDRESS [AWB_JUMP_DIVISOR]
	{0x0990, 0x0002}, // MCU_DATA_0
	{0x098C, 0xA34A}, // MCU_ADDRESS [AWB_GAIN_MIN]
	{0x0990, 0x0090}, // MCU_DATA_0
	{0x098C, 0xA34B}, // MCU_ADDRESS [AWB_GAIN_MAX]
	{0x0990, 0x00FF}, // MCU_DATA_0
	{0x098C, 0xA34C}, // MCU_ADDRESS [AWB_GAINMIN_B]
	{0x0990, 0x0075}, // MCU_DATA_0
	{0x098C, 0xA34D}, // MCU_ADDRESS [AWB_GAINMAX_B]
	{0x0990, 0x00EF}, // MCU_DATA_0
	{0x098C, 0xA351}, // MCU_ADDRESS [AWB_CCM_POSITION_MIN]
	{0x0990, 0x0000}, // MCU_DATA_0
	{0x098C, 0xA352}, // MCU_ADDRESS [AWB_CCM_POSITION_MAX]
	{0x0990, 0x007F}, // MCU_DATA_0
	{0x098C, 0xA354}, // MCU_ADDRESS [AWB_SATURATION]
	{0x0990, 0x0043}, // MCU_DATA_0
	{0x098C, 0xA355}, // MCU_ADDRESS [AWB_MODE]
	{0x0990, 0x0001}, // MCU_DATA_0
	{0x098C, 0xA35D}, // MCU_ADDRESS [AWB_STEADY_BGAIN_OUT_MIN]
	{0x0990, 0x0078}, // MCU_DATA_0
	{0x098C, 0xA35E}, // MCU_ADDRESS [AWB_STEADY_BGAIN_OUT_MAX]
	{0x0990, 0x0086}, // MCU_DATA_0
	{0x098C, 0xA35F}, // MCU_ADDRESS [AWB_STEADY_BGAIN_IN_MIN]
	{0x0990, 0x007E}, // MCU_DATA_0
	{0x098C, 0xA360}, // MCU_ADDRESS [AWB_STEADY_BGAIN_IN_MAX]
	{0x0990, 0x0082}, // MCU_DATA_0
	{0x098C, 0x2361}, // MCU_ADDRESS [AWB_CNT_PXL_TH]
	{0x0990, 0x0040}, // MCU_DATA_0
	{0x098C, 0xA363}, // MCU_ADDRESS [AWB_TG_MIN0]
	{0x0990, 0x00D2}, // MCU_DATA_0
	{0x098C, 0xA364}, // MCU_ADDRESS [AWB_TG_MAX0]
	{0x0990, 0x00F6}, // MCU_DATA_0
	{0x098C, 0xA302}, // MCU_ADDRESS [AWB_WINDOW_POS]
	{0x0990, 0x0000}, // MCU_DATA_0
	{0x098C, 0xA303}, // MCU_ADDRESS [AWB_WINDOW_SIZE]
	{0x0990, 0x00EF}, // MCU_DATA_0
	{0x098C, 0xAB20}, // MCU_ADDRESS [HG_LL_SAT1]
	{0x0990, 0x0024}, // MCU_DATA_0
	// AE Setting Frame rate 7.5fps ~ 30fps
//	{0x098C, 0xA20C}, // MCU_ADDRESS
//	{0x0990, 0x0004}, // AE_MAX_INDEX
//	{0x098C, 0xA215}, // MCU_ADDRESS
//	{0x0990, 0x0004}, // AE_INDEX_TH23
	{0x098C, 0xA20D}, // MCU_ADDRESS [AE_MIN_VIRTGAIN]
	{0x0990, 0x0030}, // MCU_DATA_0
	{0x098C, 0xA20E}, // MCU_ADDRESS [AE_MAX_VIRTGAIN]
	{0x0990, 0x0080}, // MCU_DATA_0
	// Sharpness
	{0x098C, 0xAB22}, // MCU_ADDRESS [HG_LL_APCORR1]
	{0x0990, 0x0001}, // MCU_DATA_0
	// AE Gate
	{0x098C, 0xA207}, // MCU_ADDRESS [AE_GATE]
	{0x0990, 0x0006}, // MCU_DATA_0
	// AE Target
	{0x098C, 0xA24F}, // MCU_ADDRESS [AE_BASETARGET]
	{0x0990, 0x0031}, // MCU_DATA_0
	{0x098C, 0xAB3C}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
	{0x0990, 0x0000}, // MCU_DATA_0
	{0x098C, 0xAB3D}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
	{0x0990, 0x0007}, // MCU_DATA_0
	{0x098C, 0xAB3E}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
	{0x0990, 0x0016}, // MCU_DATA_0
	{0x098C, 0xAB3F}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
	{0x0990, 0x0039}, // MCU_DATA_0
	{0x098C, 0xAB40}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
	{0x0990, 0x005F}, // MCU_DATA_0
	{0x098C, 0xAB41}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
	{0x0990, 0x007A}, // MCU_DATA_0
	{0x098C, 0xAB42}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
	{0x0990, 0x008F}, // MCU_DATA_0
	{0x098C, 0xAB43}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
	{0x0990, 0x00A1}, // MCU_DATA_0
	{0x098C, 0xAB44}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
	{0x0990, 0x00AF}, // MCU_DATA_0
	{0x098C, 0xAB45}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
	{0x0990, 0x00BB}, // MCU_DATA_0
	{0x098C, 0xAB46}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
	{0x0990, 0x00C6}, // MCU_DATA_0
	{0x098C, 0xAB47}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
	{0x0990, 0x00CF}, // MCU_DATA_0
	{0x098C, 0xAB48}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
	{0x0990, 0x00D8}, // MCU_DATA_0
	{0x098C, 0xAB49}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
	{0x0990, 0x00E0}, // MCU_DATA_0
	{0x098C, 0xAB4A}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
	{0x0990, 0x00E7}, // MCU_DATA_0
	{0x098C, 0xAB4B}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
	{0x0990, 0x00EE}, // MCU_DATA_0
	{0x098C, 0xAB4C}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
	{0x0990, 0x00F4}, // MCU_DATA_0
	{0x098C, 0xAB4D}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
	{0x0990, 0x00FA}, // MCU_DATA_0
	{0x098C, 0xAB4E}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
	{0x0990, 0x00FF}, // MCU_DATA_0
	// saturation
	{0x098C, 0xAB20}, // MCU_ADDRESS [HG_LL_SAT1]
	{0x0990, 0x0045}, // MCU_DATA_0
	{0x098C, 0xAB24}, // MCU_ADDRESS [HG_LL_SAT2]
	{0x0990, 0x0034}, // MCU_DATA_0
//	{0x098C, 0xA20C}, // MCU_ADDRESS
//	{0x0990, 0x0018}, // AE_MAX_INDEX
//	{0x098C, 0xA215}, // MCU_ADDRESS
//	{0x0990, 0x0008}, // AE_INDEX_TH23
//	{0x098C, 0xA20C}, // MCU_ADDRESS
//	{0x0990, 0x0018}, // AE_MAX_INDEX
	{0x098C, 0xA215}, // MCU_ADDRESS
	{0x0990, 0x0006}, // AE_INDEX_TH23
	{0x098C, 0xA34A}, // MCU_ADDRESS
	{0x0990, 0x007A}, // MCU_DATA_0

	//AE frame rate 15-30fps
	{0x098C, 0xA20C}, 	// MCU_ADDRESS [AE_MAX_INDEX]
	{0x0990, 0x0006}, 	// MCU_DATA_0
	//{0x098C, 0xA103}, // MCU_ADDRESS
	//{0x0990, 0x0006}, // MCU_DATA_0
	{0x0018, 0x0028}, // STANDBY_CONTROL
};

static struct msm_camera_i2c_reg_conf mt9v113_init_tunning_settings2[] = {
	{0x098C, 0xA103}, // MCU_ADDRESS
	{0x0990, 0x0005}, // MCU_DATA_0
};


static struct msm_camera_i2c_reg_conf mt9v113_prev_settings[] = {
};

static struct msm_camera_i2c_reg_conf mt9v113_snap_settings[] = {
};


static struct v4l2_subdev_info mt9v113_subdev_info[] = {
	{
		.code   = V4L2_MBUS_FMT_YUYV8_2X8,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt    = 1,
		.order    = 0,
	},
	/* more can be supported, to be added later */
};

/* add delay information to the init settings */
static struct msm_camera_i2c_conf_array mt9v113_init_conf[] = {
	{&mt9v113_init_standby_control1[0],
	ARRAY_SIZE(mt9v113_init_standby_control1), 1, MSM_CAMERA_I2C_WORD_DATA},
	{&mt9v113_init_reset_misc_control1[0],
	ARRAY_SIZE(mt9v113_init_reset_misc_control1), 5, MSM_CAMERA_I2C_WORD_DATA},
	{&mt9v113_init_reset_misc_control2[0],
	ARRAY_SIZE(mt9v113_init_reset_misc_control2), 5, MSM_CAMERA_I2C_WORD_DATA},
	{&mt9v113_init_standby_control2[0],
	ARRAY_SIZE(mt9v113_init_standby_control2), 1, MSM_CAMERA_I2C_WORD_DATA},
	{&mt9v113_init_clk_pll1[0],
	ARRAY_SIZE(mt9v113_init_clk_pll1), 50, MSM_CAMERA_I2C_WORD_DATA},
	{&mt9v113_init_clk_pll_enable[0],
	ARRAY_SIZE(mt9v113_init_clk_pll_enable), 10, MSM_CAMERA_I2C_WORD_DATA},
	{&mt9v113_init_clk_pll_ctl[0],
	ARRAY_SIZE(mt9v113_init_clk_pll_ctl), 1, MSM_CAMERA_I2C_WORD_DATA},
	{&mt9v113_init_output[0],
	ARRAY_SIZE(mt9v113_init_output), 0, MSM_CAMERA_I2C_WORD_DATA},
	{&mt9v113_init_tunning_settings1[0],
	ARRAY_SIZE(mt9v113_init_tunning_settings1), 100, MSM_CAMERA_I2C_WORD_DATA},
	{&mt9v113_init_tunning_settings2[0],
	ARRAY_SIZE(mt9v113_init_tunning_settings2), 200, MSM_CAMERA_I2C_WORD_DATA},
};

static struct msm_camera_i2c_conf_array mt9v113_confs[] = {
	{&mt9v113_snap_settings[0],
	ARRAY_SIZE(mt9v113_snap_settings), 0, MSM_CAMERA_I2C_WORD_DATA},
	{&mt9v113_prev_settings[0],
	ARRAY_SIZE(mt9v113_prev_settings), 0, MSM_CAMERA_I2C_WORD_DATA},
};

static struct msm_sensor_output_info_t mt9v113_dimensions[] = {
	{
		.x_output = 0x280,
		.y_output = 0x1E0,
		.line_length_pclk = 0x287,
		.frame_length_lines = 0x1E7,
		.vt_pixel_clk = 14000000,
		.op_pixel_clk = 14000000,
		.binning_factor = 1,
	},
	{
		.x_output = 0x280,
		.y_output = 0x1E0,
		.line_length_pclk = 0x287,
		.frame_length_lines = 0x1E7,
		.vt_pixel_clk = 14000000,
		.op_pixel_clk = 14000000,
		.binning_factor = 1,
	},
};

static struct msm_camera_i2c_reg_conf mt9v113_saturation[][4] = {
	{//LEVEL -5 /* SATURATION LEVEL0*/
	{0x098C, 0xAB20}, // MCU_ADDRESS [HG_LL_SAT1]
	{0x0990, 0x0000}, // MCU_DATA_0
	{0x098C, 0xAB24}, // MCU_ADDRESS [HG_LL_SAT2]
	{0x0990, 0x0000}, // MCU_DATA_0
	},
	{//LEVEL -4 /* SATURATION LEVEL1*/
	{0x098C, 0xAB20}, // MCU_ADDRESS [HG_LL_SAT1]
	{0x0990, 0x0010}, // MCU_DATA_0
	{0x098C, 0xAB24}, // MCU_ADDRESS [HG_LL_SAT2]
	{0x0990, 0x0010}, // MCU_DATA_0
	},
	{//LEVEL -3 /* SATURATION LEVEL2*/
	{0x098C, 0xAB20}, // MCU_ADDRESS [HG_LL_SAT1]
	{0x0990, 0x001B}, // MCU_DATA_0
	{0x098C, 0xAB24}, // MCU_ADDRESS [HG_LL_SAT2]
	{0x0990, 0x001B}, // MCU_DATA_0
	},
	{//LEVEL -2 /* SATURATION LEVEL3*/
	{0x098C, 0xAB20}, // MCU_ADDRESS [HG_LL_SAT1]
	{0x0990, 0x0027}, // MCU_DATA_0
	{0x098C, 0xAB24}, // MCU_ADDRESS [HG_LL_SAT2]
	{0x0990, 0x0027}, // MCU_DATA_0
	},
	{//LEVEL -1 /* SATURATION LEVEL4*/
	{0x098C, 0xAB20}, // MCU_ADDRESS [HG_LL_SAT1]
	{0x0990, 0x0030}, // MCU_DATA_0
	{0x098C, 0xAB24}, // MCU_ADDRESS [HG_LL_SAT2]
	{0x0990, 0x0030}, // MCU_DATA_0
	},
	{//LEVEL 0 /* SATURATION LEVEL5*/
	{0x098C, 0xAB20}, // MCU_ADDRESS [HG_LL_SAT1]
	{0x0990, 0x003F}, // MCU_DATA_0
	{0x098C, 0xAB24}, // MCU_ADDRESS [HG_LL_SAT2]
	{0x0990, 0x003F}, // MCU_DATA_0
	},
	{//LEVEL +1 /* SATURATION LEVEL6*/
	{0x098C, 0xAB20}, // MCU_ADDRESS [HG_LL_SAT1]
	{0x0990, 0x004F}, // MCU_DATA_0
	{0x098C, 0xAB24}, // MCU_ADDRESS [HG_LL_SAT2]
	{0x0990, 0x004F}, // MCU_DATA_0
	},
	{//LEVEL +2 /* SATURATION LEVEL7*/
	{0x098C, 0xAB20}, // MCU_ADDRESS [HG_LL_SAT1]
	{0x0990, 0x005F}, // MCU_DATA_0
	{0x098C, 0xAB24}, // MCU_ADDRESS [HG_LL_SAT2]
	{0x0990, 0x005F}, // MCU_DATA_0
	},
	{//LEVEL +3 /* SATURATION LEVEL8*/
	{0x098C, 0xAB20}, // MCU_ADDRESS [HG_LL_SAT1]
	{0x0990, 0x006F}, // MCU_DATA_0
	{0x098C, 0xAB24}, // MCU_ADDRESS [HG_LL_SAT2]
	{0x0990, 0x006F}, // MCU_DATA_0
	},
	{//LEVEL +4 /* SATURATION LEVEL9*/
	{0x098C, 0xAB20}, // MCU_ADDRESS [HG_LL_SAT1]
	{0x0990, 0x0080}, // MCU_DATA_0
	{0x098C, 0xAB24}, // MCU_ADDRESS [HG_LL_SAT2]
	{0x0990, 0x0080}, // MCU_DATA_0
	},
	{//LEVEL +5 /* SATURATION LEVEL10*/
	{0x098C, 0xAB20}, // MCU_ADDRESS [HG_LL_SAT1]
	{0x0990, 0x009F}, // MCU_DATA_0
	{0x098C, 0xAB24}, // MCU_ADDRESS [HG_LL_SAT2]
	{0x0990, 0x009F}, // MCU_DATA_0
	},
};
static struct msm_camera_i2c_conf_array mt9v113_saturation_confs[][1] = {
	{{mt9v113_saturation[0], ARRAY_SIZE(mt9v113_saturation[0]), 0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_saturation[1], ARRAY_SIZE(mt9v113_saturation[1]), 0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_saturation[2], ARRAY_SIZE(mt9v113_saturation[2]), 0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_saturation[3], ARRAY_SIZE(mt9v113_saturation[3]), 0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_saturation[4], ARRAY_SIZE(mt9v113_saturation[4]), 0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_saturation[5], ARRAY_SIZE(mt9v113_saturation[5]), 0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_saturation[6], ARRAY_SIZE(mt9v113_saturation[6]), 0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_saturation[7], ARRAY_SIZE(mt9v113_saturation[7]), 0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_saturation[8], ARRAY_SIZE(mt9v113_saturation[8]), 0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_saturation[9], ARRAY_SIZE(mt9v113_saturation[9]), 0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_saturation[10],ARRAY_SIZE(mt9v113_saturation[10]),0, MSM_CAMERA_I2C_WORD_DATA},},
};

static int mt9v113_saturation_enum_map[] = {
	MSM_V4L2_SATURATION_L0,
	MSM_V4L2_SATURATION_L1,
	MSM_V4L2_SATURATION_L2,
	MSM_V4L2_SATURATION_L3,
	MSM_V4L2_SATURATION_L4,
	MSM_V4L2_SATURATION_L5,/* default */
	MSM_V4L2_SATURATION_L6,
	MSM_V4L2_SATURATION_L7,
	MSM_V4L2_SATURATION_L8,
	MSM_V4L2_SATURATION_L9,
	MSM_V4L2_SATURATION_L10,
};

static struct msm_camera_i2c_enum_conf_array mt9v113_saturation_enum_confs = {
	.conf = &mt9v113_saturation_confs[0][0],
	.conf_enum = mt9v113_saturation_enum_map,
	.num_enum = ARRAY_SIZE(mt9v113_saturation_enum_map),
	.num_index = ARRAY_SIZE(mt9v113_saturation_confs),
	.num_conf = ARRAY_SIZE(mt9v113_saturation_confs[0]),
	.data_type = MSM_CAMERA_I2C_WORD_DATA,
};

//Contrast Settings
//Contrast +5 Level 10
static struct msm_camera_i2c_reg_conf mt9v113_contrast_plus_5_settings[] = {
	{-1, -1},
};

//Contrast +4 Level 9
static struct msm_camera_i2c_reg_conf mt9v113_contrast_plus_4_settings[] = {
	{-1, -1},
};

//Contrast +3 Level 8
static struct msm_camera_i2c_reg_conf mt9v113_contrast_plus_3_settings[] = {
	{0x098C, 0xAB3C}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
	{0x0990, 0x0000}, //MCU_DATA_0
	{0x098C, 0xAB3D}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
	{0x0990, 0x0002}, //MCU_DATA_0
	{0x098C, 0xAB3E}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
	{0x0990, 0x0007}, //MCU_DATA_0
	{0x098C, 0xAB3F}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
	{0x0990, 0x0016}, //MCU_DATA_0
	{0x098C, 0xAB40}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
	{0x0990, 0x0044}, //MCU_DATA_0
	{0x098C, 0xAB41}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
	{0x0990, 0x0069}, //MCU_DATA_0
	{0x098C, 0xAB42}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
	{0x0990, 0x0083}, //MCU_DATA_0
	{0x098C, 0xAB43}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
	{0x0990, 0x0098}, //MCU_DATA_0
	{0x098C, 0xAB44}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
	{0x0990, 0x00A8}, //MCU_DATA_0
	{0x098C, 0xAB45}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
	{0x0990, 0x00B6}, //MCU_DATA_0
	{0x098C, 0xAB46}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
	{0x0990, 0x00C2}, //MCU_DATA_0
	{0x098C, 0xAB47}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
	{0x0990, 0x00CC}, //MCU_DATA_0
	{0x098C, 0xAB48}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
	{0x0990, 0x00D5}, //MCU_DATA_0
	{0x098C, 0xAB49}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
	{0x0990, 0x00DE}, //MCU_DATA_0
	{0x098C, 0xAB4A}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
	{0x0990, 0x00E6}, //MCU_DATA_0
	{0x098C, 0xAB4B}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
	{0x0990, 0x00ED}, //MCU_DATA_0
	{0x098C, 0xAB4C}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
	{0x0990, 0x00F3}, //MCU_DATA_0
	{0x098C, 0xAB4D}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
	{0x0990, 0x00F9}, //MCU_DATA_0
	{0x098C, 0xAB4E}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
	{0x0990, 0x00FF}, //MCU_DATA_0
	{0x098C, 0xA103}, //MCU_ADDRESS [SEQ_CMD]
//woody test : aptina FAE suggest
	{0x0990, 0x0005}, //MCU_DATA_0
};
//Contrast +2 Level 7
static struct msm_camera_i2c_reg_conf mt9v113_contrast_plus_2_settings[] = {
	{0x098C, 0xAB3C}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
	{0x0990, 0x0000}, //MCU_DATA_0
	{0x098C, 0xAB3D}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
	{0x0990, 0x0003}, //MCU_DATA_0
	{0x098C, 0xAB3E}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
	{0x0990, 0x0009}, //MCU_DATA_0
	{0x098C, 0xAB3F}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
	{0x0990, 0x001E}, //MCU_DATA_0
	{0x098C, 0xAB40}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
	{0x0990, 0x0050}, //MCU_DATA_0
	{0x098C, 0xAB41}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
	{0x0990, 0x006F}, //MCU_DATA_0
	{0x098C, 0xAB42}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
	{0x0990, 0x0087}, //MCU_DATA_0
	{0x098C, 0xAB43}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
	{0x0990, 0x009B}, //MCU_DATA_0
	{0x098C, 0xAB44}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
	{0x0990, 0x00AA}, //MCU_DATA_0
	{0x098C, 0xAB45}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
	{0x0990, 0x00B8}, //MCU_DATA_0
	{0x098C, 0xAB46}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
	{0x0990, 0x00C3}, //MCU_DATA_0
	{0x098C, 0xAB47}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
	{0x0990, 0x00CD}, //MCU_DATA_0
	{0x098C, 0xAB48}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
	{0x0990, 0x00D6}, //MCU_DATA_0
	{0x098C, 0xAB49}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
	{0x0990, 0x00DF}, //MCU_DATA_0
	{0x098C, 0xAB4A}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
	{0x0990, 0x00E6}, //MCU_DATA_0
	{0x098C, 0xAB4B}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
	{0x0990, 0x00ED}, //MCU_DATA_0
	{0x098C, 0xAB4C}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
	{0x0990, 0x00F3}, //MCU_DATA_0
	{0x098C, 0xAB4D}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
	{0x0990, 0x00F9}, //MCU_DATA_0
	{0x098C, 0xAB4E}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
	{0x0990, 0x00FF}, //MCU_DATA_0
	{0x098C, 0xA103}, //MCU_ADDRESS [SEQ_CMD]
//woody test : aptina FAE suggest
	{0x0990, 0x0005}, //MCU_DATA_0
};

//Contrast +1 Level 6
static struct msm_camera_i2c_reg_conf mt9v113_contrast_plus_1_settings[] = {
	{0x098C, 0xAB3C}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
	{0x0990, 0x0000}, //MCU_DATA_0
	{0x098C, 0xAB3D}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
	{0x0990, 0x0004}, //MCU_DATA_0
	{0x098C, 0xAB3E}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
	{0x0990, 0x000D}, //MCU_DATA_0
	{0x098C, 0xAB3F}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
	{0x0990, 0x0029}, //MCU_DATA_0
	{0x098C, 0xAB40}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
	{0x0990, 0x0058}, //MCU_DATA_0
	{0x098C, 0xAB41}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
	{0x0990, 0x0075}, //MCU_DATA_0
	{0x098C, 0xAB42}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
	{0x0990, 0x008C}, //MCU_DATA_0
	{0x098C, 0xAB43}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
	{0x0990, 0x009E}, //MCU_DATA_0
	{0x098C, 0xAB44}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
	{0x0990, 0x00AD}, //MCU_DATA_0
	{0x098C, 0xAB45}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
	{0x0990, 0x00BA}, //MCU_DATA_0
	{0x098C, 0xAB46}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
	{0x0990, 0x00C5}, //MCU_DATA_0
	{0x098C, 0xAB47}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
	{0x0990, 0x00CE}, //MCU_DATA_0
	{0x098C, 0xAB48}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
	{0x0990, 0x00D7}, //MCU_DATA_0
	{0x098C, 0xAB49}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
	{0x0990, 0x00DF}, //MCU_DATA_0
	{0x098C, 0xAB4A}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
	{0x0990, 0x00E7}, //MCU_DATA_0
	{0x098C, 0xAB4B}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
	{0x0990, 0x00ED}, //MCU_DATA_0
	{0x098C, 0xAB4C}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
	{0x0990, 0x00F4}, //MCU_DATA_0
	{0x098C, 0xAB4D}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
	{0x0990, 0x00F9}, //MCU_DATA_0
	{0x098C, 0xAB4E}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
	{0x0990, 0x00FF}, //MCU_DATA_0
	{0x098C, 0xA103}, //MCU_ADDRESS [SEQ_CMD]
//woody test : aptina FAE suggest
	{0x0990, 0x0005}, //MCU_DATA_0
};


//Contrast 0 Level 5
static struct msm_camera_i2c_reg_conf mt9v113_contrast_zero_settings[] = {
	{0x098C, 0xAB3C}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
	{0x0990, 0x0000}, //MCU_DATA_0
	{0x098C, 0xAB3D}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
	{0x0990, 0x000D}, //MCU_DATA_0
	{0x098C, 0xAB3E}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
	{0x0990, 0x0024}, //MCU_DATA_0
	{0x098C, 0xAB3F}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
	{0x0990, 0x0042}, //MCU_DATA_0
	{0x098C, 0xAB40}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
	{0x0990, 0x0064}, //MCU_DATA_0
	{0x098C, 0xAB41}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
	{0x0990, 0x007E}, //MCU_DATA_0
	{0x098C, 0xAB42}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
	{0x0990, 0x0092}, //MCU_DATA_0
	{0x098C, 0xAB43}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
	{0x0990, 0x00A3}, //MCU_DATA_0
	{0x098C, 0xAB44}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
	{0x0990, 0x00B1}, //MCU_DATA_0
	{0x098C, 0xAB45}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
	{0x0990, 0x00BC}, //MCU_DATA_0
	{0x098C, 0xAB46}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
	{0x0990, 0x00C7}, //MCU_DATA_0
	{0x098C, 0xAB47}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
	{0x0990, 0x00D0}, //MCU_DATA_0
	{0x098C, 0xAB48}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
	{0x0990, 0x00D9}, //MCU_DATA_0
	{0x098C, 0xAB49}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
	{0x0990, 0x00E0}, //MCU_DATA_0
	{0x098C, 0xAB4A}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
	{0x0990, 0x00E7}, //MCU_DATA_0
	{0x098C, 0xAB4B}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
	{0x0990, 0x00EE}, //MCU_DATA_0
	{0x098C, 0xAB4C}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
	{0x0990, 0x00F4}, //MCU_DATA_0
	{0x098C, 0xAB4D}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
	{0x0990, 0x00FA}, //MCU_DATA_0
	{0x098C, 0xAB4E}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
	{0x0990, 0x00FF}, //MCU_DATA_0
	{0x098C, 0xA103}, //MCU_ADDRESS [SEQ_CMD]
//woody test : aptina FAE suggest
	{0x0990, 0x0005}, //MCU_DATA_0
};

//Contrast -1 Level 4
static struct msm_camera_i2c_reg_conf mt9v113_contrast_minus_1_settings[] = {
	{0x098C, 0xAB3C}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
	{0x0990, 0x0000}, //MCU_DATA_0
	{0x098C, 0xAB3D}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
	{0x0990, 0x0017}, //MCU_DATA_0
	{0x098C, 0xAB3E}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
	{0x0990, 0x002E}, //MCU_DATA_0
	{0x098C, 0xAB3F}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
	{0x0990, 0x0047}, //MCU_DATA_0
	{0x098C, 0xAB40}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
	{0x0990, 0x0067}, //MCU_DATA_0
	{0x098C, 0xAB41}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
	{0x0990, 0x0080}, //MCU_DATA_0
	{0x098C, 0xAB42}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
	{0x0990, 0x0094}, //MCU_DATA_0
	{0x098C, 0xAB43}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
	{0x0990, 0x00A4}, //MCU_DATA_0
	{0x098C, 0xAB44}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
	{0x0990, 0x00B2}, //MCU_DATA_0
	{0x098C, 0xAB45}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
	{0x0990, 0x00BD}, //MCU_DATA_0
	{0x098C, 0xAB46}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
	{0x0990, 0x00C7}, //MCU_DATA_0
	{0x098C, 0xAB47}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
	{0x0990, 0x00D1}, //MCU_DATA_0
	{0x098C, 0xAB48}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
	{0x0990, 0x00D9}, //MCU_DATA_0
	{0x098C, 0xAB49}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
	{0x0990, 0x00E1}, //MCU_DATA_0
	{0x098C, 0xAB4A}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
	{0x0990, 0x00E8}, //MCU_DATA_0
	{0x098C, 0xAB4B}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
	{0x0990, 0x00EE}, //MCU_DATA_0
	{0x098C, 0xAB4C}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
	{0x0990, 0x00F4}, //MCU_DATA_0
	{0x098C, 0xAB4D}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
	{0x0990, 0x00FA}, //MCU_DATA_0
	{0x098C, 0xAB4E}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
	{0x0990, 0x00FF}, //MCU_DATA_0
	{0x098C, 0xA103}, //MCU_ADDRESS [SEQ_CMD]
//woody test : aptina FAE suggest
	{0x0990, 0x0005}, //MCU_DATA_0
};

//Contrast -2 Level 3
static struct msm_camera_i2c_reg_conf mt9v113_contrast_minus_2_settings[] = {
	{0x098C, 0xAB3C}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
	{0x0990, 0x0000}, //MCU_DATA_0
	{0x098C, 0xAB3D}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
	{0x0990, 0x0025}, //MCU_DATA_0
	{0x098C, 0xAB3E}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
	{0x0990, 0x0035}, //MCU_DATA_0
	{0x098C, 0xAB3F}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
	{0x0990, 0x004B}, //MCU_DATA_0
	{0x098C, 0xAB40}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
	{0x0990, 0x006A}, //MCU_DATA_0
	{0x098C, 0xAB41}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
	{0x0990, 0x0082}, //MCU_DATA_0
	{0x098C, 0xAB42}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
	{0x0990, 0x0096}, //MCU_DATA_0
	{0x098C, 0xAB43}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
	{0x0990, 0x00A5}, //MCU_DATA_0
	{0x098C, 0xAB44}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
	{0x0990, 0x00B3}, //MCU_DATA_0
	{0x098C, 0xAB45}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
	{0x0990, 0x00BE}, //MCU_DATA_0
	{0x098C, 0xAB46}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
	{0x0990, 0x00C8}, //MCU_DATA_0
	{0x098C, 0xAB47}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
	{0x0990, 0x00D1}, //MCU_DATA_0
	{0x098C, 0xAB48}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
	{0x0990, 0x00D9}, //MCU_DATA_0
	{0x098C, 0xAB49}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
	{0x0990, 0x00E1}, //MCU_DATA_0
	{0x098C, 0xAB4A}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
	{0x0990, 0x00E8}, //MCU_DATA_0
	{0x098C, 0xAB4B}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
	{0x0990, 0x00EE}, //MCU_DATA_0
	{0x098C, 0xAB4C}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
	{0x0990, 0x00F4}, //MCU_DATA_0
	{0x098C, 0xAB4D}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
	{0x0990, 0x00FA}, //MCU_DATA_0
	{0x098C, 0xAB4E}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
	{0x0990, 0x00FF}, //MCU_DATA_0
	{0x098C, 0xA103}, //MCU_ADDRESS [SEQ_CMD]
//woody test : aptina FAE suggest
	{0x0990, 0x0005}, //MCU_DATA_0
};

//Contrast -3 Level 2
static struct msm_camera_i2c_reg_conf mt9v113_contrast_minus_3_settings[] = {
	{0x098C, 0xAB3C}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
	{0x0990, 0x0000}, //MCU_DATA_0
	{0x098C, 0xAB3D}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
	{0x0990, 0x002A}, //MCU_DATA_0
	{0x098C, 0xAB3E}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
	{0x0990, 0x0039}, //MCU_DATA_0
	{0x098C, 0xAB3F}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
	{0x0990, 0x004D}, //MCU_DATA_0
	{0x098C, 0xAB40}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
	{0x0990, 0x006B}, //MCU_DATA_0
	{0x098C, 0xAB41}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
	{0x0990, 0x0083}, //MCU_DATA_0
	{0x098C, 0xAB42}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
	{0x0990, 0x0096}, //MCU_DATA_0
	{0x098C, 0xAB43}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
	{0x0990, 0x00A6}, //MCU_DATA_0
	{0x098C, 0xAB44}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
	{0x0990, 0x00B3}, //MCU_DATA_0
	{0x098C, 0xAB45}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
	{0x0990, 0x00BE}, //MCU_DATA_0
	{0x098C, 0xAB46}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
	{0x0990, 0x00C8}, //MCU_DATA_0
	{0x098C, 0xAB47}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
	{0x0990, 0x00D1}, //MCU_DATA_0
	{0x098C, 0xAB48}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
	{0x0990, 0x00DA}, //MCU_DATA_0
	{0x098C, 0xAB49}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
	{0x0990, 0x00E1}, //MCU_DATA_0
	{0x098C, 0xAB4A}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
	{0x0990, 0x00E8}, //MCU_DATA_0
	{0x098C, 0xAB4B}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
	{0x0990, 0x00EE}, //MCU_DATA_0
	{0x098C, 0xAB4C}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
	{0x0990, 0x00F4}, //MCU_DATA_0
	{0x098C, 0xAB4D}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
	{0x0990, 0x00FA}, //MCU_DATA_0
	{0x098C, 0xAB4E}, //MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
	{0x0990, 0x00FF}, //MCU_DATA_0
	{0x098C, 0xA103}, //MCU_ADDRESS [SEQ_CMD]
//woody test : aptina FAE suggest
	{0x0990, 0x0005}, //MCU_DATA_0
};

//Contrast -4 Level 1
static struct msm_camera_i2c_reg_conf mt9v113_contrast_minus_4_settings[] = {
	{-1, -1},
};
//Contrast -5 Level 0
static struct msm_camera_i2c_reg_conf mt9v113_contrast_minus_5_settings[] = {
	{-1, -1},
};

static struct msm_camera_i2c_conf_array mt9v113_contrast_confs[][1] = {
	{{&mt9v113_contrast_minus_5_settings[0], ARRAY_SIZE(mt9v113_contrast_minus_5_settings),
		5, MSM_CAMERA_I2C_WORD_DATA},},
	{{&mt9v113_contrast_minus_4_settings[0], ARRAY_SIZE(mt9v113_contrast_minus_4_settings),
		5, MSM_CAMERA_I2C_WORD_DATA},},
	{{&mt9v113_contrast_minus_3_settings[0], ARRAY_SIZE(mt9v113_contrast_minus_3_settings),
		5, MSM_CAMERA_I2C_WORD_DATA},},
	{{&mt9v113_contrast_minus_2_settings[0], ARRAY_SIZE(mt9v113_contrast_minus_2_settings),
		5, MSM_CAMERA_I2C_WORD_DATA},},
	{{&mt9v113_contrast_minus_1_settings[0], ARRAY_SIZE(mt9v113_contrast_minus_1_settings),
		5, MSM_CAMERA_I2C_WORD_DATA},},
	{{&mt9v113_contrast_zero_settings[0], ARRAY_SIZE(mt9v113_contrast_zero_settings),
		5, MSM_CAMERA_I2C_WORD_DATA},},
	{{&mt9v113_contrast_plus_1_settings[0], ARRAY_SIZE(mt9v113_contrast_plus_1_settings),
		5, MSM_CAMERA_I2C_WORD_DATA},},
	{{&mt9v113_contrast_plus_2_settings[0], ARRAY_SIZE(mt9v113_contrast_plus_2_settings),
		5, MSM_CAMERA_I2C_WORD_DATA},},
	{{&mt9v113_contrast_plus_3_settings[0], ARRAY_SIZE(mt9v113_contrast_plus_3_settings),
		5, MSM_CAMERA_I2C_WORD_DATA},},
	{{&mt9v113_contrast_plus_4_settings[0], ARRAY_SIZE(mt9v113_contrast_plus_4_settings),
		5, MSM_CAMERA_I2C_WORD_DATA},},
	{{&mt9v113_contrast_plus_5_settings[0], ARRAY_SIZE(mt9v113_contrast_plus_5_settings),
		5, MSM_CAMERA_I2C_WORD_DATA},},
};


static int mt9v113_contrast_enum_map[] = {
	MSM_V4L2_CONTRAST_L0,
	MSM_V4L2_CONTRAST_L1,
	MSM_V4L2_CONTRAST_L2,
	MSM_V4L2_CONTRAST_L3,
	MSM_V4L2_CONTRAST_L4,
	MSM_V4L2_CONTRAST_L5, /* default value */
	MSM_V4L2_CONTRAST_L6,
	MSM_V4L2_CONTRAST_L7,
	MSM_V4L2_CONTRAST_L8,
	MSM_V4L2_CONTRAST_L9,
	MSM_V4L2_CONTRAST_L10,
};

static struct msm_camera_i2c_enum_conf_array mt9v113_contrast_enum_confs = {
	.conf = &mt9v113_contrast_confs[0][0],
	.conf_enum = mt9v113_contrast_enum_map,
	.num_enum = ARRAY_SIZE(mt9v113_contrast_enum_map),
	.num_index = ARRAY_SIZE(mt9v113_contrast_confs),
	.num_conf = ARRAY_SIZE(mt9v113_contrast_confs[0]),
	.data_type = MSM_CAMERA_I2C_WORD_DATA,
};


static struct msm_camera_i2c_reg_conf mt9v113_sharpness[][2] = {
	{
	//SHARPNESS 0
	{0x098C, 0xAB22},// MCU_ADDRESS [HG_LL_APCORR1]
	{0x0990, 0x0000},// MCU_DATA_0
	},
	{
	//SHARPNESS 1
	{0x098C, 0xAB22},// MCU_ADDRESS [HG_LL_APCORR1]
	{0x0990, 0x0001},// MCU_DATA_0
	},
	{
	//SHARPNESS 2
	{0x098C, 0xAB22},// MCU_ADDRESS [HG_LL_APCORR1]
	{0x0990, 0x0002},// MCU_DATA_0
	},
	{
	//SHARPNESS 3
	{0x098C, 0xAB22},// MCU_ADDRESS [HG_LL_APCORR1]
	{0x0990, 0x0003},// MCU_DATA_0
	},
	{
	//SHARPNESS 4
	{0x098C, 0xAB22},// MCU_ADDRESS [HG_LL_APCORR1]
	{0x0990, 0x0004},// MCU_DATA_0
	},
	{
	//SHARPNESS 5
	{0x098C, 0xAB22},// MCU_ADDRESS [HG_LL_APCORR1]
	{0x0990, 0x0005},// MCU_DATA_0
	},
};

static struct msm_camera_i2c_conf_array mt9v113_sharpness_confs[][1] = {
	{{mt9v113_sharpness[0], ARRAY_SIZE(mt9v113_sharpness[0]), 0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_sharpness[1], ARRAY_SIZE(mt9v113_sharpness[1]), 0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_sharpness[2], ARRAY_SIZE(mt9v113_sharpness[2]), 0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_sharpness[3], ARRAY_SIZE(mt9v113_sharpness[3]), 0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_sharpness[4], ARRAY_SIZE(mt9v113_sharpness[4]), 0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_sharpness[5], ARRAY_SIZE(mt9v113_sharpness[5]), 0, MSM_CAMERA_I2C_WORD_DATA},},
};

static int mt9v113_sharpness_enum_map[] = {
	MSM_V4L2_SHARPNESS_L0,
	MSM_V4L2_SHARPNESS_L1,
	MSM_V4L2_SHARPNESS_L2, /* default value */
	MSM_V4L2_SHARPNESS_L3,
	MSM_V4L2_SHARPNESS_L4,
	MSM_V4L2_SHARPNESS_L5,
};

static struct msm_camera_i2c_enum_conf_array mt9v113_sharpness_enum_confs = {
	.conf = &mt9v113_sharpness_confs[0][0],
	.conf_enum = mt9v113_sharpness_enum_map,
	.num_enum = ARRAY_SIZE(mt9v113_sharpness_enum_map),
	.num_index = ARRAY_SIZE(mt9v113_sharpness_confs),
	.num_conf = ARRAY_SIZE(mt9v113_sharpness_confs[0]),
	.data_type = MSM_CAMERA_I2C_WORD_DATA,
};

static struct msm_camera_i2c_reg_conf mt9v113_exposure[][4] = {
	{/*EXPOSURECOMPENSATIONN2*/
	{0x098C, 0xA244}, // MCU_ADDRESS [AE_BASETARGET]
	{0x0990, 0x0008}, // MCU_DATA_0
	{0x098C, 0xA24F}, // MCU_ADDRESS [AE_BASETARGET]
	{0x0990, 0x0018}, // MCU_DATA_0

	},
	{/*EXPOSURECOMPENSATIONN1*/
	{0x098C, 0xA244}, // MCU_ADDRESS [AE_BASETARGET]
	{0x0990, 0x0008}, // MCU_DATA_0
	{0x098C, 0xA24F}, // MCU_ADDRESS [AE_BASETARGET]
	{0x0990, 0x0020}, // MCU_DATA_0
	},
	{/*EXPOSURECOMPENSATIOND*/
	{0x098C, 0xA244}, // MCU_ADDRESS [AE_BASETARGET]
	{0x0990, 0x0008}, // MCU_DATA_0
	{0x098C, 0xA24F}, // MCU_ADDRESS [AE_BASETARGET]
	{0x0990, 0x0031}, // MCU_DATA_0
	},
	{/*EXPOSURECOMPENSATIONP1*/
	{0x098C, 0xA244}, // MCU_ADDRESS [AE_BASETARGET]
	{0x0990, 0x0008}, // MCU_DATA_0
	{0x098C, 0xA24F}, // MCU_ADDRESS [AE_BASETARGET]
	{0x0990, 0x005B}, // MCU_DATA_0
	},
	{/*EXPOSURECOMPENSATIONP2*/
	{0x098C, 0xA244}, // MCU_ADDRESS [AE_BASETARGET]
	{0x0990, 0x0008}, // MCU_DATA_0
	{0x098C, 0xA24F}, // MCU_ADDRESS [AE_BASETARGET]
	{0x0990, 0x007B}, // MCU_DATA_0
	},
};

static struct msm_camera_i2c_conf_array mt9v113_exposure_confs[][1] = {
	{{mt9v113_exposure[0], ARRAY_SIZE(mt9v113_exposure[0]), 0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_exposure[1], ARRAY_SIZE(mt9v113_exposure[1]), 0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_exposure[2], ARRAY_SIZE(mt9v113_exposure[2]), 0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_exposure[3], ARRAY_SIZE(mt9v113_exposure[3]), 0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_exposure[4], ARRAY_SIZE(mt9v113_exposure[4]), 0, MSM_CAMERA_I2C_WORD_DATA},},
};

static int mt9v113_exposure_enum_map[] = {
	MSM_V4L2_EXPOSURE_N2,
	MSM_V4L2_EXPOSURE_N1,
	MSM_V4L2_EXPOSURE_D, /* default vaule */
	MSM_V4L2_EXPOSURE_P1,
	MSM_V4L2_EXPOSURE_P2,
};

static struct msm_camera_i2c_enum_conf_array mt9v113_exposure_enum_confs = {
	.conf = &mt9v113_exposure_confs[0][0],
	.conf_enum = mt9v113_exposure_enum_map,
	.num_enum = ARRAY_SIZE(mt9v113_exposure_enum_map),
	.num_index = ARRAY_SIZE(mt9v113_exposure_confs),
	.num_conf = ARRAY_SIZE(mt9v113_exposure_confs[0]),
	.data_type = MSM_CAMERA_I2C_WORD_DATA,
};



static struct msm_camera_i2c_reg_conf mt9v113_iso[][8] = {
	{
	//ISO Auto
	{0x098C, 0xA20D}, // MCU_ADDRESS [AE_MIN_VIRTGAIN]
	{0x0990, 0x0030}, // MCU_DATA_0
	{0x098C, 0xA20E}, // MCU_ADDRESS [AE_MAX_VIRTGAIN]
	{0x0990, 0x0080}, // MCU_DATA_0
	{0x098C, 0xA216}, // MCU_ADDRESS [AE_MAXGAIN23]
	{0x0990, 0x0078}, // MCU_DATA_0
	{0x098C, 0xA103}, // MCU_ADDRESS [SEQ_CMD]
	{0x0990, 0x0005}, // MCU_DATA_0
	},
	{
	/*ISO_DEBLUR*/
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1},
	},
	{
	//ISO 100
	{0x098C, 0xA20D}, // MCU_ADDRESS [AE_MIN_VIRTGAIN]
	{0x0990, 0x0030}, // MCU_DATA_0
	{0x098C, 0xA20E}, // MCU_ADDRESS [AE_MAX_VIRTGAIN]
	{0x0990, 0x0030}, // MCU_DATA_0
	{0x098C, 0xA216}, // MCU_ADDRESS [AE_MAXGAIN23]
	{0x0990, 0x0030}, // MCU_DATA_0
	{0x098C, 0xA103}, // MCU_ADDRESS [SEQ_CMD]
	{0x0990, 0x0005}, // MCU_DATA_0
	},
	{
	//ISO 200
	{0x098C, 0xA20D}, // MCU_ADDRESS [AE_MIN_VIRTGAIN]
	{0x0990, 0x0040}, // MCU_DATA_0
	{0x098C, 0xA20E}, // MCU_ADDRESS [AE_MAX_VIRTGAIN]
	{0x0990, 0x0040}, // MCU_DATA_0
	{0x098C, 0xA216}, // MCU_ADDRESS [AE_MAXGAIN23]
	{0x0990, 0x0040}, // MCU_DATA_0
	{0x098C, 0xA103}, // MCU_ADDRESS [SEQ_CMD]
	{0x0990, 0x0005}, // MCU_DATA_0
	},
	{
	//ISO 400
	{0x098C, 0xA20D}, // MCU_ADDRESS [AE_MIN_VIRTGAIN]
	{0x0990, 0x0050}, // MCU_DATA_0
	{0x098C, 0xA20E}, // MCU_ADDRESS [AE_MAX_VIRTGAIN]
	{0x0990, 0x0050}, // MCU_DATA_0
	{0x098C, 0xA216}, // MCU_ADDRESS [AE_MAXGAIN23]
	{0x0990, 0x0050}, // MCU_DATA_0
	{0x098C, 0xA103}, // MCU_ADDRESS [SEQ_CMD]
	{0x0990, 0x0005}, // MCU_DATA_0
	},
	{
	//ISO 800
	{0x098C, 0xA20D}, // MCU_ADDRESS [AE_MIN_VIRTGAIN]
	{0x0990, 0x0060}, // MCU_DATA_0
	{0x098C, 0xA20E}, // MCU_ADDRESS [AE_MAX_VIRTGAIN]
	{0x0990, 0x0060}, // MCU_DATA_0
	{0x098C, 0xA216}, // MCU_ADDRESS [AE_MAXGAIN23]
	{0x0990, 0x0060}, // MCU_DATA_0
	{0x098C, 0xA103}, // MCU_ADDRESS [SEQ_CMD]
	{0x0990, 0x0005}, // MCU_DATA_0
	},
	{
	//ISO 1600
	{0x098C, 0xA20D}, // MCU_ADDRESS [AE_MIN_VIRTGAIN]
	{0x0990, 0x0080}, // MCU_DATA_0
	{0x098C, 0xA20E}, // MCU_ADDRESS [AE_MAX_VIRTGAIN]
	{0x0990, 0x0080}, // MCU_DATA_0
	{0x098C, 0xA216}, // MCU_ADDRESS [AE_MAXGAIN23]
	{0x0990, 0x0080}, // MCU_DATA_0
	{0x098C, 0xA103}, // MCU_ADDRESS [SEQ_CMD]
	{0x0990, 0x0005}, // MCU_DATA_0
	},
};

static struct msm_camera_i2c_conf_array mt9v113_iso_confs[][1] = {
	{{mt9v113_iso[0], ARRAY_SIZE(mt9v113_iso[0]),  0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_iso[1], ARRAY_SIZE(mt9v113_iso[1]),  0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_iso[2], ARRAY_SIZE(mt9v113_iso[2]),  0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_iso[3], ARRAY_SIZE(mt9v113_iso[3]),  0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_iso[4], ARRAY_SIZE(mt9v113_iso[4]),  0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_iso[5], ARRAY_SIZE(mt9v113_iso[5]),  0, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_iso[5], ARRAY_SIZE(mt9v113_iso[5]),  0, MSM_CAMERA_I2C_WORD_DATA},},
};

static int mt9v113_iso_enum_map[] = {
	MSM_V4L2_ISO_AUTO , /* default vaule */
	MSM_V4L2_ISO_DEBLUR,
	MSM_V4L2_ISO_100,
	MSM_V4L2_ISO_200,
	MSM_V4L2_ISO_400,
	MSM_V4L2_ISO_800,
	MSM_V4L2_ISO_1600,
};


static struct msm_camera_i2c_enum_conf_array mt9v113_iso_enum_confs = {
	.conf = &mt9v113_iso_confs[0][0],
	.conf_enum = mt9v113_iso_enum_map,
	.num_enum = ARRAY_SIZE(mt9v113_iso_enum_map),
	.num_index = ARRAY_SIZE(mt9v113_iso_confs),
	.num_conf = ARRAY_SIZE(mt9v113_iso_confs[0]),
	.data_type = MSM_CAMERA_I2C_WORD_DATA,
};

//Effect Settings
static struct msm_camera_i2c_reg_conf mt9v113_no_effect[] = {
	{0x098C, 0x2759}, //MCU_ADDRESS [MODE_SPEC_EFFECTS_A]
	{0x0990, 0x6440}, //MCU_DATA_0
	{0x098C, 0x275B}, //MCU_ADDRESS [MODE_SPEC_EFFECTS_B]
	{0x0990, 0x6440}, //MCU_DATA_0
	{0x098C, 0xA103}, //MCU_ADDRESS [SEQ_CMD]
//woody test : aptina FAE suggest
	{0x0990, 0x0005}, //MCU_DATA_0
};

static struct msm_camera_i2c_conf_array mt9v113_no_effect_confs[] = {
	{&mt9v113_no_effect[0], ARRAY_SIZE(mt9v113_no_effect), 30, MSM_CAMERA_I2C_WORD_DATA},
};

static struct msm_camera_i2c_reg_conf mt9v113_special_effect[][6] = {
	{/*for special effect OFF*/
	{0x098C, 0x2759}, //MCU_ADDRESS [MODE_SPEC_EFFECTS_A]
	{0x0990, 0x6440}, //MCU_DATA_0
	{0x098C, 0x275B}, //MCU_ADDRESS [MODE_SPEC_EFFECTS_B]
	{0x0990, 0x6440}, //MCU_DATA_0
	{0x098C, 0xA103}, //MCU_ADDRESS [SEQ_CMD]
	//woody test : aptina FAE suggest
	{0x0990, 0x0005}, //MCU_DATA_0
	},
	{/*for special effect MONO*/
	{0x098C, 0x2759}, //MCU_ADDRESS [MODE_SPEC_EFFECTS_A]
	{0x0990, 0x6441}, //MCU_DATA_0
	{0x098C, 0x275B}, //MCU_ADDRESS [MODE_SPEC_EFFECTS_B]
	{0x0990, 0x6441}, //MCU_DATA_0
	{0x098C, 0xA103}, //MCU_ADDRESS [SEQ_CMD]
	//woody test : aptina FAE suggest
	{0x0990, 0x0005}, //MCU_DATA_0
	},
	{/*for special efefct Negative*/
	{0x098C, 0x2759}, // MCU_ADDRESS [MODE_SPEC_EFFECTS_A]
	{0x0990, 0x6443}, // MCU_DATA_0
	{0x098C, 0x275B}, // MCU_ADDRESS [MODE_SPEC_EFFECTS_B]
	{0x0990, 0x6443}, // MCU_DATA_0
	{0x098C, 0xA103}, // MCU_ADDRESS [SEQ_CMD]
	//woody test : aptina FAE suggest
	{0x0990, 0x0005}, // MCU_DATA_0
	},
	{
	//solarize
	{0x098C, 0x2759}, // MCU_ADDRESS [MODE_SPEC_EFFECTS_A]
	{0x0990, 0x6444}, // MCU_DATA_0
	{0x098C, 0x275B}, // MCU_ADDRESS [MODE_SPEC_EFFECTS_B]
	{0x0990, 0x6444}, // MCU_DATA_0
	{0x098C, 0xA103}, // MCU_ADDRESS [SEQ_CMD]
	{0x0990, 0x0005}, // MCU_DATA_0
	},
	{/*for sepia*/
	{0x098C, 0x2759}, //MCU_ADDRESS [MODE_SPEC_EFFECTS_A]
	{0x0990, 0x6442}, //MCU_DATA_0
	{0x098C, 0x275B}, //MCU_ADDRESS [MODE_SPEC_EFFECTS_B]
	{0x0990, 0x6442}, //MCU_DATA_0
	{0x098C, 0xA103}, //MCU_ADDRESS [SEQ_CMD]
//woody test : aptina FAE suggest
	{0x0990, 0x0005}, //MCU_DATA_0
	},
	{/* Posteraize, SOC380 solarize2 */
	{0x098C, 0x2759}, // MCU_ADDRESS [MODE_SPEC_EFFECTS_A]
	{0x0990, 0x6445}, // MCU_DATA_0
	{0x098C, 0x275B}, // MCU_ADDRESS [MODE_SPEC_EFFECTS_B]
	{0x0990, 0x6445}, // MCU_DATA_0
	{0x098C, 0xA103}, // MCU_ADDRESS [SEQ_CMD]
	{0x0990, 0x0005}, // MCU_DATA_0
	},
	{/* White board not supported*/
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1}
	},
	{/*Blackboard not supported*/
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1}
	},
	{/*Aqua not supported*/
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1}
	},
	{/*Emboss not supported */
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1}
	},
	{/*sketch not supported*/
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1}
	},
	{/*Neon not supported*/
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1}
	},
	{/*MAX value*/
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1}
	},
};

static struct msm_camera_i2c_conf_array mt9v113_special_effect_confs[][1] = {
	{{mt9v113_special_effect[0],  ARRAY_SIZE(mt9v113_special_effect[0]),
		5, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_special_effect[1],  ARRAY_SIZE(mt9v113_special_effect[1]),
		5, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_special_effect[2],  ARRAY_SIZE(mt9v113_special_effect[2]),
		5, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_special_effect[3],  ARRAY_SIZE(mt9v113_special_effect[3]),
		5, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_special_effect[4],  ARRAY_SIZE(mt9v113_special_effect[4]),
		5, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_special_effect[5],  ARRAY_SIZE(mt9v113_special_effect[5]),
		5, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_special_effect[6],  ARRAY_SIZE(mt9v113_special_effect[6]),
		5, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_special_effect[7],  ARRAY_SIZE(mt9v113_special_effect[7]),
		5, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_special_effect[8],  ARRAY_SIZE(mt9v113_special_effect[8]),
		5, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_special_effect[9],  ARRAY_SIZE(mt9v113_special_effect[9]),
		5, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_special_effect[10], ARRAY_SIZE(mt9v113_special_effect[10]),
		5, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_special_effect[11], ARRAY_SIZE(mt9v113_special_effect[11]),
		5, MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_special_effect[12], ARRAY_SIZE(mt9v113_special_effect[12]),
		5, MSM_CAMERA_I2C_WORD_DATA},},
};

static int mt9v113_special_effect_enum_map[] = {
	MSM_V4L2_EFFECT_OFF,
	MSM_V4L2_EFFECT_MONO,
	MSM_V4L2_EFFECT_NEGATIVE,
	MSM_V4L2_EFFECT_SOLARIZE,
	MSM_V4L2_EFFECT_SEPIA,
	MSM_V4L2_EFFECT_POSTERAIZE,
	MSM_V4L2_EFFECT_WHITEBOARD,
	MSM_V4L2_EFFECT_BLACKBOARD,
	MSM_V4L2_EFFECT_AQUA,
	MSM_V4L2_EFFECT_EMBOSS,
	MSM_V4L2_EFFECT_SKETCH,
	MSM_V4L2_EFFECT_NEON,
	MSM_V4L2_EFFECT_MAX,
};

static struct msm_camera_i2c_enum_conf_array
		 mt9v113_special_effect_enum_confs = {
	.conf = &mt9v113_special_effect_confs[0][0],
	.conf_enum = mt9v113_special_effect_enum_map,
	.num_enum = ARRAY_SIZE(mt9v113_special_effect_enum_map),
	.num_index = ARRAY_SIZE(mt9v113_special_effect_confs),
	.num_conf = ARRAY_SIZE(mt9v113_special_effect_confs[0]),
	.data_type = MSM_CAMERA_I2C_WORD_DATA,
};
static struct msm_camera_i2c_reg_conf mt9v113_antibanding[][2] = {
	{//60Hz
	{0x098C, 0xA404}, // MCU_ADDRESS [FD_MODE]
	{0x0990, 0x00D1}, // MCU_DATA_0
	},
	{//50Hz
	{0x098C, 0xA404}, // MCU_ADDRESS [FD_MODE]
	{0x0990, 0x00B1}, // MCU_DATA_0};
	},
	{//Auto
	{0x098C, 0xA404}, // MCU_ADDRESS [FD_MODE]
	{0x0990, 0x0012}, // MCU_DATA_0};
	},
};

static struct msm_camera_i2c_conf_array mt9v113_antibanding_confs[][1] = {
	{{mt9v113_antibanding[0], ARRAY_SIZE(mt9v113_antibanding[0]),  0,
		MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_antibanding[1], ARRAY_SIZE(mt9v113_antibanding[1]),  0,
		MSM_CAMERA_I2C_WORD_DATA},},
	{{mt9v113_antibanding[2], ARRAY_SIZE(mt9v113_antibanding[2]),  0,
		MSM_CAMERA_I2C_WORD_DATA},},
};

static int mt9v113_antibanding_enum_map[] = {
	MSM_V4L2_POWER_LINE_60HZ,
	MSM_V4L2_POWER_LINE_50HZ,
	MSM_V4L2_POWER_LINE_AUTO,
};


static struct msm_camera_i2c_enum_conf_array mt9v113_antibanding_enum_confs = {
	.conf = &mt9v113_antibanding_confs[0][0],
	.conf_enum = mt9v113_antibanding_enum_map,
	.num_enum = ARRAY_SIZE(mt9v113_antibanding_enum_map),
	.num_index = ARRAY_SIZE(mt9v113_antibanding_confs),
	.num_conf = ARRAY_SIZE(mt9v113_antibanding_confs[0]),
	.data_type = MSM_CAMERA_I2C_WORD_DATA,
};

static struct msm_camera_i2c_reg_conf mt9v113_wb_off[] ={
	{-1, -1},
};

static struct msm_camera_i2c_reg_conf mt9v113_wb_auto[] ={
	{0x098C, 0xA34A}, //MCU_ADDRESS [AWB_GAIN_MIN]
	{0x0990, 0x0090}, //MCU_DATA_0
	{0x098C, 0xA34B}, //MCU_ADDRESS [AWB_GAIN_MAX]
	{0x0990, 0x00FF}, //MCU_DATA_0
	{0x098C, 0xA34C}, //MCU_ADDRESS [AWB_GAINMIN_B]
	{0x0990, 0x0075}, //MCU_DATA_0
	{0x098C, 0xA34D}, //MCU_ADDRESS [AWB_GAINMAX_B]
	{0x0990, 0x00EF}, //MCU_DATA_0
	{0x098C, 0xA351}, //MCU_ADDRESS [AWB_CCM_POSITION_MIN]
	{0x0990, 0x0000}, //MCU_DATA_0
	{0x098C, 0xA352}, //MCU_ADDRESS [AWB_CCM_POSITION_MAX]
	{0x0990, 0x007F}, //MCU_DATA_0
	{0x098C, 0xA103}, //MCU_ADDRESS [SEQ_CMD]
//woody test : aptina FAE suggest
	{0x0990, 0x0005}, //MCU_DATA_0
};

//Sunny
static struct msm_camera_i2c_reg_conf mt9v113_wb_sunny[]= {
	{0x098C, 0xA34A}, //MCU_ADDRESS [AWB_GAIN_MIN]
	{0x0990, 0x00CA}, //MCU_DATA_0
	{0x098C, 0xA34B}, //MCU_ADDRESS [AWB_GAIN_MAX]
	{0x0990, 0x00CA}, //MCU_DATA_0
	{0x098C, 0xA34C}, //MCU_ADDRESS [AWB_GAINMIN_B]
	{0x0990, 0x007B}, //MCU_DATA_0
	{0x098C, 0xA34D}, //MCU_ADDRESS [AWB_GAINMAX_B]
	{0x0990, 0x007B}, //MCU_DATA_0
	{0x098C, 0xA351}, //MCU_ADDRESS [AWB_CCM_POSITION_MIN]
	{0x0990, 0x007F}, //MCU_DATA_0
	{0x098C, 0xA352}, //MCU_ADDRESS [AWB_CCM_POSITION_MAX]
	{0x0990, 0x007F}, //MCU_DATA_0
	{0x098C, 0xA103}, //MCU_ADDRESS [SEQ_CMD]
//woody test : aptina FAE suggest
	{0x0990, 0x0005}, //MCU_DATA_0
};

//Cloudy
static struct msm_camera_i2c_reg_conf mt9v113_wb_cloudy[] = {
	{0x098C, 0xA34A}, //MCU_ADDRESS [AWB_GAIN_MIN]
	{0x0990, 0x00D0}, //MCU_DATA_0
	{0x098C, 0xA34B}, //MCU_ADDRESS [AWB_GAIN_MAX]
	{0x0990, 0x00D0}, //MCU_DATA_0
	{0x098C, 0xA34C}, //MCU_ADDRESS [AWB_GAINMIN_B]
	{0x0990, 0x0070}, //MCU_DATA_0
	{0x098C, 0xA34D}, //MCU_ADDRESS [AWB_GAINMAX_B]
	{0x0990, 0x0070}, //MCU_DATA_0
	{0x098C, 0xA351}, //MCU_ADDRESS [AWB_CCM_POSITION_MIN]
	{0x0990, 0x007F}, //MCU_DATA_0
	{0x098C, 0xA352}, //MCU_ADDRESS [AWB_CCM_POSITION_MAX]
	{0x0990, 0x007F}, //MCU_DATA_0
	{0x098C, 0xA103}, //MCU_ADDRESS [SEQ_CMD]
//woody test : aptina FAE suggest
	{0x0990, 0x0005}, //MCU_DATA_0
};

//Fluorescent
static struct msm_camera_i2c_reg_conf mt9v113_wb_fluorescent[] = {
	{0x098C, 0xA34A}, //MCU_ADDRESS [AWB_GAIN_MIN]
	{0x0990, 0x00A6}, //MCU_DATA_0
	{0x098C, 0xA34B}, //MCU_ADDRESS [AWB_GAIN_MAX]
	{0x0990, 0x00A6}, //MCU_DATA_0
	{0x098C, 0xA34C}, //MCU_ADDRESS [AWB_GAINMIN_B]
	{0x0990, 0x0080}, //MCU_DATA_0
	{0x098C, 0xA34D}, //MCU_ADDRESS [AWB_GAINMAX_B]
	{0x0990, 0x0080}, //MCU_DATA_0
	{0x098C, 0xA351}, //MCU_ADDRESS [AWB_CCM_POSITION_MIN]
	{0x0990, 0x0035}, //MCU_DATA_0
	{0x098C, 0xA352}, //MCU_ADDRESS [AWB_CCM_POSITION_MAX]
	{0x0990, 0x0035}, //MCU_DATA_0
	{0x098C, 0xA103}, //MCU_ADDRESS [SEQ_CMD]
//woody test : aptina FAE suggest
	{0x0990, 0x0005}, //MCU_DATA_0
};

//Incandescent
static struct msm_camera_i2c_reg_conf mt9v113_wb_incandescent[] = {
	{0x098C, 0xA34A}, //MCU_ADDRESS [AWB_GAIN_MIN]
	{0x0990, 0x008D}, //MCU_DATA_0
	{0x098C, 0xA34B}, //MCU_ADDRESS [AWB_GAIN_MAX]
	{0x0990, 0x008D}, //MCU_DATA_0
	{0x098C, 0xA34C}, //MCU_ADDRESS [AWB_GAINMIN_B]
	{0x0990, 0x0088}, //MCU_DATA_0
	{0x098C, 0xA34D}, //MCU_ADDRESS [AWB_GAINMAX_B]
	{0x0990, 0x0088}, //MCU_DATA_0
	{0x098C, 0xA351}, //MCU_ADDRESS [AWB_CCM_POSITION_MIN]
	{0x0990, 0x0000}, //MCU_DATA_0
	{0x098C, 0xA352}, //MCU_ADDRESS [AWB_CCM_POSITION_MAX]
	{0x0990, 0x0000}, //MCU_DATA_0
	{0x098C, 0xA103}, //MCU_ADDRESS [SEQ_CMD]
//woody test : aptina FAE suggest
	{0x0990, 0x0005}, //MCU_DATA_0
};

//custom
static struct msm_camera_i2c_reg_conf mt9v113_wb_custom[] = {
	{0x098C, 0xA34A}, //MCU_ADDRESS [AWB_GAIN_MIN]
	{0x0990, 0x00A6}, //MCU_DATA_0
	{0x098C, 0xA34B}, //MCU_ADDRESS [AWB_GAIN_MAX]
	{0x0990, 0x00A6}, //MCU_DATA_0
	{0x098C, 0xA34C}, //MCU_ADDRESS [AWB_GAINMIN_B]
	{0x0990, 0x0080}, //MCU_DATA_0
	{0x098C, 0xA34D}, //MCU_ADDRESS [AWB_GAINMAX_B]
	{0x0990, 0x0080}, //MCU_DATA_0
	{0x098C, 0xA351}, //MCU_ADDRESS [AWB_CCM_POSITION_MIN]
	{0x0990, 0x0035}, //MCU_DATA_0
	{0x098C, 0xA352}, //MCU_ADDRESS [AWB_CCM_POSITION_MAX]
	{0x0990, 0x0035}, //MCU_DATA_0
	{0x098C, 0xA103}, //MCU_ADDRESS [SEQ_CMD]
//woody test : aptina FAE suggest
	{0x0990, 0x0005}, //MCU_DATA_0
};

static struct msm_camera_i2c_conf_array mt9v113_wb_oem_confs[][1] = {
	{{&mt9v113_wb_off[0], ARRAY_SIZE(mt9v113_wb_off), 5,
		MSM_CAMERA_I2C_WORD_DATA},},
	{{&mt9v113_wb_auto[0], ARRAY_SIZE(mt9v113_wb_auto), 5,
		MSM_CAMERA_I2C_WORD_DATA},},
	{{&mt9v113_wb_custom[0], ARRAY_SIZE(mt9v113_wb_custom), 5,
		MSM_CAMERA_I2C_WORD_DATA},},
	{{&mt9v113_wb_incandescent[0], ARRAY_SIZE(mt9v113_wb_incandescent), 5,
		MSM_CAMERA_I2C_WORD_DATA},},
	{{&mt9v113_wb_fluorescent[0], ARRAY_SIZE(mt9v113_wb_fluorescent), 5,
		MSM_CAMERA_I2C_WORD_DATA},},
	{{&mt9v113_wb_sunny[0], ARRAY_SIZE(mt9v113_wb_sunny), 5,
		MSM_CAMERA_I2C_WORD_DATA},},
	{{&mt9v113_wb_cloudy[0], ARRAY_SIZE(mt9v113_wb_cloudy), 5,
		MSM_CAMERA_I2C_WORD_DATA},},
};

static int mt9v113_wb_oem_enum_map[] = {
	MSM_V4L2_WB_OFF,
	MSM_V4L2_WB_AUTO ,
	MSM_V4L2_WB_CUSTOM,
	MSM_V4L2_WB_INCANDESCENT,
	MSM_V4L2_WB_FLUORESCENT,
	MSM_V4L2_WB_DAYLIGHT,
	MSM_V4L2_WB_CLOUDY_DAYLIGHT,
};

static struct msm_camera_i2c_enum_conf_array mt9v113_wb_oem_enum_confs = {
	.conf = &mt9v113_wb_oem_confs[0][0],
	.conf_enum = mt9v113_wb_oem_enum_map,
	.num_enum = ARRAY_SIZE(mt9v113_wb_oem_enum_map),
	.num_index = ARRAY_SIZE(mt9v113_wb_oem_confs),
	.num_conf = ARRAY_SIZE(mt9v113_wb_oem_confs[0]),
	.data_type = MSM_CAMERA_I2C_WORD_DATA,
};


int mt9v113_saturation_msm_sensor_s_ctrl_by_enum(
		struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;
	CDBG("--CAMERA-- %s IN, value:%d\n", __func__, value);
	if (effect_value == CAMERA_EFFECT_OFF) {
		rc = msm_sensor_write_enum_conf_array(
			s_ctrl->sensor_i2c_client,
			ctrl_info->enum_cfg_settings, value);
	}
	CDBG("--CAMERA-- %s ...(End)\n", __func__);
	return rc;
}


int mt9v113_contrast_msm_sensor_s_ctrl_by_enum(
		struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;
	CDBG("--CAMERA-- %s IN, value:%d\n", __func__, value);
	if (effect_value == CAMERA_EFFECT_OFF) {
		rc = msm_sensor_write_enum_conf_array(
			s_ctrl->sensor_i2c_client,
			ctrl_info->enum_cfg_settings, value);
	}
	return rc;
}

int mt9v113_sharpness_msm_sensor_s_ctrl_by_enum(
		struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;
	CDBG("--CAMERA-- %s IN, value:%d\n", __func__, value);
	if (effect_value == CAMERA_EFFECT_OFF) {
		rc = msm_sensor_write_enum_conf_array(
			s_ctrl->sensor_i2c_client,
			ctrl_info->enum_cfg_settings, value);
	}
	return rc;
}

int mt9v113_effect_msm_sensor_s_ctrl_by_enum(struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;
	effect_value = value;
	CDBG("--CAMERA-- %s IN, value:%d\n", __func__, value);
	if (effect_value == CAMERA_EFFECT_OFF) {
		rc = msm_sensor_write_conf_array(
			s_ctrl->sensor_i2c_client,
			s_ctrl->msm_sensor_reg->no_effect_settings, 0);
		if (rc < 0) {
			CDBG("write faield\n");
			return rc;
		}
	} else {
		rc = msm_sensor_write_enum_conf_array(
			s_ctrl->sensor_i2c_client,
			ctrl_info->enum_cfg_settings, value);
	}
	return rc;
}

int mt9v113_antibanding_msm_sensor_s_ctrl_by_enum(
		struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;
	CDBG("--CAMERA-- %s IN, value:%d\n", __func__, value);
	rc = msm_sensor_write_enum_conf_array(
		s_ctrl->sensor_i2c_client,
		ctrl_info->enum_cfg_settings, value);
	if (rc < 0) {
		CDBG("write faield\n");
		return rc;
	}
	return rc;
}

int mt9v113_msm_sensor_s_ctrl_by_enum(struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;
	CDBG("--CAMERA-- %s IN, value:%d\n", __func__, value);
	rc = msm_sensor_write_enum_conf_array(
		s_ctrl->sensor_i2c_client,
		ctrl_info->enum_cfg_settings, value);
	if (rc < 0) {
		CDBG("write faield\n");
		return rc;
	}
	return rc;
}

struct msm_sensor_v4l2_ctrl_info_t mt9v113_v4l2_ctrl_info[] = {
	{
		.ctrl_id = V4L2_CID_SATURATION,
		.min = MSM_V4L2_SATURATION_L0,
		.max = MSM_V4L2_SATURATION_L10,
		.step = 1,
		.enum_cfg_settings = &mt9v113_saturation_enum_confs,
		.s_v4l2_ctrl = mt9v113_saturation_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_CONTRAST,
		.min = MSM_V4L2_CONTRAST_L0,
		.max = MSM_V4L2_CONTRAST_L10,
		.step = 1,
		.enum_cfg_settings = &mt9v113_contrast_enum_confs,
		.s_v4l2_ctrl = mt9v113_contrast_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_SHARPNESS,
		.min = MSM_V4L2_SHARPNESS_L0,
		.max = MSM_V4L2_SHARPNESS_L5,
		.step = 1,
		.enum_cfg_settings = &mt9v113_sharpness_enum_confs,
		.s_v4l2_ctrl = mt9v113_sharpness_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_EXPOSURE,
		.min = MSM_V4L2_EXPOSURE_N2,
		.max = MSM_V4L2_EXPOSURE_P2,
		.step = 1,
		.enum_cfg_settings = &mt9v113_exposure_enum_confs,
		.s_v4l2_ctrl = mt9v113_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = MSM_V4L2_PID_ISO,
		.min = MSM_V4L2_ISO_AUTO,
		.max = MSM_V4L2_ISO_1600,
		.step = 1,
		.enum_cfg_settings = &mt9v113_iso_enum_confs,
		.s_v4l2_ctrl = mt9v113_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_SPECIAL_EFFECT,
		.min = MSM_V4L2_EFFECT_OFF,
		.max = MSM_V4L2_EFFECT_NEGATIVE,
		.step = 1,
		.enum_cfg_settings = &mt9v113_special_effect_enum_confs,
		.s_v4l2_ctrl = mt9v113_effect_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_POWER_LINE_FREQUENCY,
		.min = MSM_V4L2_POWER_LINE_60HZ,
		.max = MSM_V4L2_POWER_LINE_AUTO,
		.step = 1,
		.enum_cfg_settings = &mt9v113_antibanding_enum_confs,
		.s_v4l2_ctrl = mt9v113_antibanding_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_WHITE_BALANCE_TEMPERATURE,
		.min = MSM_V4L2_WB_OFF,
		.max = MSM_V4L2_WB_CLOUDY_DAYLIGHT,
		.step = 1,
		.enum_cfg_settings = &mt9v113_wb_oem_enum_confs,
		.s_v4l2_ctrl = mt9v113_msm_sensor_s_ctrl_by_enum,
	},

};

static struct msm_camera_csi_params mt9v113_csi_params = {
	.data_format = CSI_8BIT,
	.lane_cnt    = 1,
	.lane_assign = 0xe4,
	.dpcm_scheme = 0,
	.settle_cnt  = 30,
};

static struct msm_camera_csi_params *mt9v113_csi_params_array[] = {
	&mt9v113_csi_params,
	&mt9v113_csi_params,
};

static struct msm_sensor_output_reg_addr_t mt9v113_reg_addr = {
	.x_output = 0x2703,
	.y_output = 0x2705,
	.line_length_pclk = 0x2713,
	.frame_length_lines = 0x2711,
};

static struct msm_sensor_id_info_t mt9v113_id_info = {
	.sensor_id_reg_addr = 0x000,
	.sensor_id = 0x2280,
};

static struct msm_sensor_exp_gain_info_t mt9v113_exp_gain_info = {
	.coarse_int_time_addr = 0x004f,
	.global_gain_addr = 0x0012,
	.vert_offset = 4,
};


static const struct i2c_device_id mt9v113_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&mt9v113_s_ctrl},
	{ }
};

int32_t mt9v113_sensor_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int32_t rc = 0;
	struct msm_sensor_ctrl_t *s_ctrl;

	CDBG("%s IN\r\n", __func__);

	s_ctrl = (struct msm_sensor_ctrl_t *)(id->driver_data);
	s_ctrl->sensor_i2c_addr = s_ctrl->sensor_i2c_addr;

	rc = msm_sensor_i2c_probe(client, id);

	if (client->dev.platform_data == NULL) {
		CDBG_HIGH("%s: NULL sensor data\n", __func__);
		return -EFAULT;
	}

	usleep_range(5000, 5100);

	return rc;

}

static struct i2c_driver mt9v113_i2c_driver = {
	.id_table = mt9v113_i2c_id,
	.probe  = mt9v113_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client mt9v113_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static int __init msm_sensor_init_module(void)
{
	int rc = 0;
	CDBG("MT9V113\n");

	rc = i2c_add_driver(&mt9v113_i2c_driver);

	return rc;
}

static struct v4l2_subdev_core_ops mt9v113_subdev_core_ops = {
	.s_ctrl = msm_sensor_v4l2_s_ctrl,
	.queryctrl = msm_sensor_v4l2_query_ctrl,
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops mt9v113_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops mt9v113_subdev_ops = {
	.core = &mt9v113_subdev_core_ops,
	.video  = &mt9v113_subdev_video_ops,
};

static int32_t mt9v113_write_exp_gain(struct msm_sensor_ctrl_t *s_ctrl,
		uint16_t gain, uint32_t line)
{
	CDBG("mt9v113_write_exp_gain : Not supported\n");
	return 0;
}

int32_t mt9v113_sensor_set_fps(struct msm_sensor_ctrl_t *s_ctrl,
		struct fps_cfg *fps)
{
	CDBG("mt9v113_sensor_set_fps: Not supported\n");
	return 0;
}

int32_t mt9v113_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct msm_camera_sensor_info *info = NULL;

	CDBG("%s: %d\n", __func__, __LINE__);

	info = s_ctrl->sensordata;

	CDBG("%s, sensor_pwd:%d, sensor_reset:%d\r\n",__func__, info->sensor_pwd, info->sensor_reset);

	gpio_direction_output(info->sensor_pwd, 1);
	if (info->sensor_reset_enable){
		gpio_direction_output(info->sensor_reset, 0);
	}
	usleep_range(10000, 11000);

	/* turn on LDO for PVT */
	if (info->pmic_gpio_enable) {
		lcd_camera_power_onoff(1);
	}

	rc = msm_sensor_power_up(s_ctrl);
	if (rc < 0) {
		CDBG("%s: msm_sensor_power_up failed\n", __func__);
		return rc;
	}

		usleep_range(1000, 1100);
	gpio_direction_output(info->sensor_pwd, 0);
	msleep(20);
	if (info->sensor_reset_enable){
		gpio_direction_output(info->sensor_reset, 1);
	}
	msleep(25);

	return rc;
}

int32_t mt9v113_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct msm_camera_sensor_info *info = NULL;

	CDBG("%s IN\r\n", __func__);

	info = s_ctrl->sensordata;

	msm_sensor_stop_stream(s_ctrl);
	msleep(40);
	gpio_direction_output(info->sensor_pwd, 1);
	usleep_range(5000, 5100);

	rc = msm_sensor_power_down(s_ctrl);
	msleep(40);
	if (s_ctrl->sensordata->pmic_gpio_enable){
		lcd_camera_power_onoff(0);
	}
	return rc;
}

int32_t mt9v113_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res)
{
	int32_t rc = 0;
	static int csi_config;
	if (update_type == MSM_SENSOR_REG_INIT) {
		CDBG("Register INIT\n");
		s_ctrl->curr_csi_params = NULL;
		csi_config = 0;

		//set sensor init&quality setting
		CDBG("--CAMERA-- set sensor init setting\n");
		msm_sensor_write_init_settings(s_ctrl);

	} else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
		CDBG("PERIODIC : %d\n", res);
		if (res == 0)
			return 0;
		if (!csi_config) {
			msm_sensor_stop_stream(s_ctrl);

			msm_sensor_write_conf_array(
				s_ctrl->sensor_i2c_client,
				s_ctrl->msm_sensor_reg->mode_settings, res);
			msleep(30);

			s_ctrl->curr_csic_params = s_ctrl->csic_params[res];
			CDBG("CSI config in progress\n");
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_CSIC_CFG,
				s_ctrl->curr_csic_params);
			CDBG("CSI config is done\n");
			mb();
			msleep(30);

			msm_sensor_start_stream(s_ctrl);
			csi_config = 1;
		}
		msleep(50);
	}
	return rc;
}

static struct msm_sensor_fn_t mt9v113_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = mt9v113_sensor_set_fps,
	.sensor_csi_setting = mt9v113_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = mt9v113_sensor_power_up,
	.sensor_power_down = mt9v113_sensor_power_down,
	.sensor_write_exp_gain = mt9v113_write_exp_gain,
	.sensor_write_snapshot_exp_gain = mt9v113_write_exp_gain,
};

static struct msm_sensor_reg_t mt9v113_regs = {
	.default_data_type = MSM_CAMERA_I2C_WORD_DATA,
	.start_stream_conf = mt9v113_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(mt9v113_start_settings),
	.stop_stream_conf = mt9v113_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(mt9v113_stop_settings),
	.group_hold_on_conf = NULL,
	.group_hold_on_conf_size = 0,
	.group_hold_off_conf = NULL,
	.group_hold_off_conf_size = 0,

	.init_settings = &mt9v113_init_conf[0],
	.init_size = ARRAY_SIZE(mt9v113_init_conf),
	.mode_settings = &mt9v113_confs[0],
	.no_effect_settings = &mt9v113_no_effect_confs[0],
	.output_settings = &mt9v113_dimensions[0],
	.num_conf = ARRAY_SIZE(mt9v113_confs),
};

static struct msm_sensor_ctrl_t mt9v113_s_ctrl = {
	.msm_sensor_reg = &mt9v113_regs,
	.msm_sensor_v4l2_ctrl_info = mt9v113_v4l2_ctrl_info,
	.num_v4l2_ctrl = ARRAY_SIZE(mt9v113_v4l2_ctrl_info),
	.sensor_i2c_client = &mt9v113_sensor_i2c_client,
	.sensor_i2c_addr = 0x7a,
	.sensor_output_reg_addr = &mt9v113_reg_addr,
	.sensor_id_info = &mt9v113_id_info,
	.sensor_exp_gain_info = &mt9v113_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csic_params = &mt9v113_csi_params_array[0],
	.msm_sensor_mutex = &mt9v113_mut,
	.sensor_i2c_driver = &mt9v113_i2c_driver,
	.sensor_v4l2_subdev_info = mt9v113_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(mt9v113_subdev_info),
	.sensor_v4l2_subdev_ops = &mt9v113_subdev_ops,
	.func_tbl = &mt9v113_func_tbl,
	.clk_rate = MSM_SENSOR_MCLK_24HZ,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("APTINA VGA YUV sensor driver");
MODULE_LICENSE("GPL v2");
