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
 */


/*
[SENSOR]
Sensor Model:
Camera Module:
Lens Model:
Driver IC:      = MT9V113
PV Size         = 640 x 480
Cap Size        = 640 x 480
Output Format   = YUYV
MCLK Speed      = 24M
PV DVP_PCLK     =
Cap DVP_PCLK    =
PV Frame Rate   = 30fps
Cap Frame Rate  = 7.5fps
I2C Slave ID    =
I2C Mode        = 16Addr, 16Data
*/

#ifndef CAMSENSOR_MT9V113
#define CAMSENSOR_MT9V113
#define MT9V113_WriteSettingTbl(pTbl)     MT9V113_WriteRegsTbl(pTbl,sizeof(pTbl)/sizeof(pTbl[0]))

enum mt9v113_test_mode_t {
	TEST_OFF,
	TEST_1,
	TEST_2,
	TEST_3
};

enum mt9v113_resolution_t {
  QTR_SIZE,
  FULL_SIZE,
  HFR_60FPS,
  HFR_90FPS,
  HFR_120FPS,
  INVALID_SIZE
};
/******************************************************************************
MT9V113_WREG, *PMT9V113_WREG Definition
******************************************************************************/
typedef struct {
	unsigned int addr;          /*Reg Addr :16Bit*/
	unsigned int data;          /*Reg Data :16Bit or 8Bit*/
    unsigned int data_format;   /*Reg Data Format:16Bit = 0,8Bit = 1*/
	unsigned int delay_ms;      /*Operation NOP time(ms)*/
} MT9V113_WREG, *PMT9V113_WREG;

/******************************************************************************
Initial Setting Table
******************************************************************************/
MT9V113_WREG mt9v113_init_tbl[] =
{
    //Reset the register at the beginning
    {0x0018, 0x4028,0,20}, // STANDBY_CONTROL
    {0x001A, 0x0011,0,10}, // RESET_AND_MISC_CONTROL
    {0x001A, 0x0010,0,10}, // RESET_AND_MISC_CONTROL
    {0x0018, 0x4028,0,50}, // STANDBY_CONTROL
    {0x098C, 0x02F0,0,0}, // MCU_ADDRESS
    {0x0990, 0x0000,0,0}, // MCU_DATA_0
    {0x098C, 0x02F2,0,0}, // MCU_ADDRESS
    {0x0990, 0x0210,0,0}, // MCU_DATA_0
    {0x098C, 0x02F4,0,0}, // MCU_ADDRESS
    {0x0990, 0x001A,0,0}, // MCU_DATA_0
    {0x098C, 0x2145,0,0}, // MCU_ADDRESS
    {0x0990, 0x02F4,0,0}, // MCU_DATA_0
    {0x098C, 0xA134,0,0}, // MCU_ADDRESS
    {0x0990, 0x0001,0,0}, // MCU_DATA_0
    {0x31E0, 0x0001,0,0}, // PIX_DEF_ID
    {0x001A, 0x0010,0,0}, // RESET_AND_MISC_CONTROL
    {0x3400, 0x7A2C,0,0}, // MIPI_CONTROL
    {0x321C, 0x8003,0,0}, // OFIFO_CONTROL_STATUS
    {0x001E, 0x0777,0,0}, // PAD_SLEW
    {0x0016, 0x42DF,0,0}, // CLOCKS_CONTROL
    // 24MHz / 26MHz
    {0x0014, 0xB04B,0,0}, // PLL_CONTROL
    {0x0014, 0xB049,0,0}, // PLL_CONTROL
    {0x0010, 0x021C,0,0}, //PLL Dividers=540
    {0x0012, 0x0000,0,0}, //PLL P Dividers=0
    {0x0014, 0x244B,0,1}, //PLL control: TEST_BYPASS on=9291
    // Allow PLL to lock
    {0x0014, 0x304B,0,10}, //PLL control: PLL_ENABLE on=12363
    {0x0014, 0xB04A,0,1}, // PLL_CONTROL
    {0x098C, 0x2703,0,0}, //Output Width (A)
    {0x0990, 0x0280,0,0}, //=640
    {0x098C, 0x2705,0,0}, //Output Height (A)
    {0x0990, 0x01E0,0,0}, //=480
    {0x098C, 0x2707,0,0}, //Output Width (B)
    {0x0990, 0x0280,0,0}, //=640
    {0x098C, 0x2709,0,0}, //Output Height (B)
    {0x0990, 0x01E0,0,0}, //=480
    {0x098C, 0x270D,0,0}, //Row Start (A)
    {0x0990, 0x0000,0,0}, //=0
    {0x098C, 0x270F,0,0}, //Column Start (A)
    {0x0990, 0x0000,0,0}, //=0
    {0x098C, 0x2711,0,0}, //Row End (A)
    {0x0990, 0x01E7,0,0}, //=487
    {0x098C, 0x2713,0,0}, //Column End (A)
    {0x0990, 0x0287,0,0}, //=647
    {0x098C, 0x2715,0,0}, //Row Speed (A)
    {0x0990, 0x0001,0,0}, //=1
    {0x098C, 0x2717,0,0}, //Read Mode (A)
    {0x0990, 0x0026,0,0}, //=38
    {0x098C, 0x2719,0,0}, //sensor_fine_correction (A)
    {0x0990, 0x001A,0,0}, //=26
    {0x098C, 0x271B,0,0}, //sensor_fine_IT_min (A)
    {0x0990, 0x006B,0,0}, //=107
    {0x098C, 0x271D,0,0}, //sensor_fine_IT_max_margin (A)
    {0x0990, 0x006B,0,0}, //=107
    {0x098C, 0x271F,0,0}, //Frame Lines (A)
    {0x0990, 0x022A,0,0}, //=554
    {0x098C, 0x2721,0,0}, //Line Length (A)
    {0x0990, 0x034A,0,0}, //=842
    {0x098C, 0x2723,0,0}, //Row Start (B)
    {0x0990, 0x0000,0,0}, //=0
    {0x098C, 0x2725,0,0}, //Column Start (B)
    {0x0990, 0x0000,0,0}, //=0
    {0x098C, 0x2727,0,0}, //Row End (B)
    {0x0990, 0x01E7,0,0}, //=487
    {0x098C, 0x2729,0,0}, //Column End (B)
    {0x0990, 0x287 ,0,0}, //=647
    {0x098C, 0x272B,0,0}, //Row Speed (B)
    {0x0990, 0x0001,0,0}, //=1
    {0x098C, 0x272D,0,0}, //Read Mode (B)
    {0x0990, 0x0026,0,0}, //=38
    {0x098C, 0x272F,0,0}, //sensor_fine_correction (B)
    {0x0990, 0x001A,0,0}, //=26
    {0x098C, 0x2731,0,0}, //sensor_fine_IT_min (B)
    {0x0990, 0x006B,0,0}, //=107
    {0x098C, 0x2733,0,0}, //sensor_fine_IT_max_margin (B)
    {0x0990, 0x006B,0,0}, //=107
    {0x098C, 0x2735,0,0}, //Frame Lines (B)
    {0x0990, 0x046F,0,0}, //=1135
    {0x098C, 0x2737,0,0}, //Line Length (B)
    {0x0990, 0x034A,0,0}, //=842
    {0x098C, 0x2739,0,0}, //Crop_X0 (A)
    {0x0990, 0x0000,0,0}, //=0
    {0x098C, 0x273B,0,0}, //Crop_X1 (A)
    {0x0990, 0x027F,0,0}, //=639
    {0x098C, 0x273D,0,0}, //Crop_Y0 (A)
    {0x0990, 0x0000,0,0}, //=0
    {0x098C, 0x273F,0,0}, //Crop_Y1 (A)
    {0x0990, 0x01DF,0,0}, //=479
    {0x098C, 0x2747,0,0}, //Crop_X0 (B)
    {0x0990, 0x0000,0,0}, //=0
    {0x098C, 0x2749,0,0}, //Crop_X1 (B)
    {0x0990, 0x027F,0,0}, //=639
    {0x098C, 0x274B,0,0}, //Crop_Y0 (B)
    {0x0990, 0x0000,0,0}, //=0
    {0x098C, 0x274D,0,0}, //Crop_Y1 (B)
    {0x0990, 0x01DF,0,0}, //=479
    {0x098C, 0x222D,0,0}, //R9 Step
    {0x0990, 0x008B,0,0}, //=139
    {0x098C, 0xA408,0,0}, //search_f1_50
    {0x0990, 0x0021,0,0}, //=33
    {0x098C, 0xA409,0,0}, //search_f2_50
    {0x0990, 0x0023,0,0}, //=35
    {0x098C, 0xA40A,0,0}, //search_f1_60
    {0x0990, 0x0028,0,0}, //=40
    {0x098C, 0xA40B,0,0}, //search_f2_60
    {0x0990, 0x002A,0,0}, //=42
    {0x098C, 0x2411,0,0}, //R9_Step_60 (A)
    {0x0990, 0x008B,0,0}, //=139
    {0x098C, 0x2413,0,0}, //R9_Step_50 (A)
    {0x0990, 0x00A6,0,0}, //=166
    {0x098C, 0x2415,0,0}, //R9_Step_60 (B)
    {0x0990, 0x008B,0,0}, //=139
    {0x098C, 0x2417,0,0}, //R9_Step_50 (B)
    {0x0990, 0x00A6,0,0}, //=166
    {0x098C, 0xA404,0,0}, //FD Mode
    {0x0990, 0x0010,0,0}, //=16
    {0x098C, 0xA40D,0,0}, //Stat_min
    {0x0990, 0x0002,0,0}, //=2
    {0x098C, 0xA40E,0,0}, //Stat_max
    {0x0990, 0x0003,0,0}, //=3
    {0x098C, 0xA410,0,0}, //Min_amplitude
    {0x0990, 0x000A,0,0}, //=10
    //REG=0x098C, 0xA20C // MCU_ADDRESS [AE_MAX_INDEX]
    //REG=0x0990, 0x0004 // MCU_DATA_0
    // Lens Shading
    //85% LSC
    {0x3210, 0x09B0,0,0}, // COLOR_PIPELINE_CONTROL
    {0x364E, 0x0210,0,0}, // P_GR_P0Q0
    {0x3650, 0x0CCA,0,0}, // P_GR_P0Q1
    {0x3652, 0x2D12,0,0}, // P_GR_P0Q2
    {0x3654, 0xCD0C,0,0}, // P_GR_P0Q3
    {0x3656, 0xC632,0,0}, // P_GR_P0Q4
    {0x3658, 0x00D0,0,0}, // P_RD_P0Q0
    {0x365A, 0x60AA,0,0}, // P_RD_P0Q1
    {0x365C, 0x7272,0,0}, // P_RD_P0Q2
    {0x365E, 0xFE09,0,0}, // P_RD_P0Q3
    {0x3660, 0xAD72,0,0}, // P_RD_P0Q4
    {0x3662, 0x0170,0,0}, // P_BL_P0Q0
    {0x3664, 0x5DCB,0,0}, // P_BL_P0Q1
    {0x3666, 0x27D2,0,0}, // P_BL_P0Q2
    {0x3668, 0xCE4D,0,0}, // P_BL_P0Q3
    {0x366A, 0x9DB3,0,0}, // P_BL_P0Q4
    {0x366C, 0x0150,0,0}, // P_GB_P0Q0
    {0x366E, 0xB809,0,0}, // P_GB_P0Q1
    {0x3670, 0x30B2,0,0}, // P_GB_P0Q2
    {0x3672, 0x82AD,0,0}, // P_GB_P0Q3
    {0x3674, 0xC1D2,0,0}, // P_GB_P0Q4
    {0x3676, 0xC4CD,0,0}, // P_GR_P1Q0
    {0x3678, 0xBE47,0,0}, // P_GR_P1Q1
    {0x367A, 0x5E4F,0,0}, // P_GR_P1Q2
    {0x367C, 0x9F10,0,0}, // P_GR_P1Q3
    {0x367E, 0xEC30,0,0}, // P_GR_P1Q4
    {0x3680, 0x914D,0,0}, // P_RD_P1Q0
    {0x3682, 0x846A,0,0}, // P_RD_P1Q1
    {0x3684, 0x66AB,0,0}, // P_RD_P1Q2
    {0x3686, 0x20D0,0,0}, // P_RD_P1Q3
    {0x3688, 0x1714,0,0}, // P_RD_P1Q4
    {0x368A, 0x99AC,0,0}, // P_BL_P1Q0
    {0x368C, 0x5CED,0,0}, // P_BL_P1Q1
    {0x368E, 0x00B1,0,0}, // P_BL_P1Q2
    {0x3690, 0x716C,0,0}, // P_BL_P1Q3
    {0x3692, 0x9594,0,0}, // P_BL_P1Q4
    {0x3694, 0xA22D,0,0}, // P_GB_P1Q0
    {0x3696, 0xB88A,0,0}, // P_GB_P1Q1
    {0x3698, 0x02B0,0,0}, // P_GB_P1Q2
    {0x369A, 0xC38F,0,0}, // P_GB_P1Q3
    {0x369C, 0x2B30,0,0}, // P_GB_P1Q4
    {0x369E, 0x6B32,0,0}, // P_GR_P2Q0
    {0x36A0, 0x128C,0,0}, // P_GR_P2Q1
    {0x36A2, 0x2574,0,0}, // P_GR_P2Q2
    {0x36A4, 0xD1B3,0,0}, // P_GR_P2Q3
    {0x36A6, 0xC2B8,0,0}, // P_GR_P2Q4
    {0x36A8, 0x1893,0,0}, // P_RD_P2Q0
    {0x36AA, 0x8DB0,0,0}, // P_RD_P2Q1
    {0x36AC, 0x2134,0,0}, // P_RD_P2Q2
    {0x36AE, 0x9014,0,0}, // P_RD_P2Q3
    {0x36B0, 0xFC57,0,0}, // P_RD_P2Q4
    {0x36B2, 0x2DB2,0,0}, // P_BL_P2Q0
    {0x36B4, 0x8FB1,0,0}, // P_BL_P2Q1
    {0x36B6, 0x9832,0,0}, // P_BL_P2Q2
    {0x36B8, 0x1CD4,0,0}, // P_BL_P2Q3
    {0x36BA, 0xE437,0,0}, // P_BL_P2Q4
    {0x36BC, 0x5992,0,0}, // P_GB_P2Q0
    {0x36BE, 0x99AF,0,0}, // P_GB_P2Q1
    {0x36C0, 0x0F54,0,0}, // P_GB_P2Q2
    {0x36C2, 0x9A52,0,0}, // P_GB_P2Q3
    {0x36C4, 0xB358,0,0}, // P_GB_P2Q4
    {0x36C6, 0xC3EE,0,0}, // P_GR_P3Q0
    {0x36C8, 0xC3F1,0,0}, // P_GR_P3Q1
    {0x36CA, 0x94D2,0,0}, // P_GR_P3Q2
    {0x36CC, 0x4175,0,0}, // P_GR_P3Q3
    {0x36CE, 0x4EB7,0,0}, // P_GR_P3Q4
    {0x36D0, 0xF310,0,0}, // P_RD_P3Q0
    {0x36D2, 0x0C51,0,0}, // P_RD_P3Q1
    {0x36D4, 0x6C75,0,0}, // P_RD_P3Q2
    {0x36D6, 0xDA96,0,0}, // P_RD_P3Q3
    {0x36D8, 0x8FF9,0,0}, // P_RD_P3Q4
    {0x36DA, 0x9C10,0,0}, // P_BL_P3Q0
    {0x36DC, 0x99B2,0,0}, // P_BL_P3Q1
    {0x36DE, 0xFCD4,0,0}, // P_BL_P3Q2
    {0x36E0, 0x6CD5,0,0}, // P_BL_P3Q3
    {0x36E2, 0x7E98,0,0}, // P_BL_P3Q4
    {0x36E4, 0xE64E,0,0}, // P_GB_P3Q0
    {0x36E6, 0x8E72,0,0}, // P_GB_P3Q1
    {0x36E8, 0x38D2,0,0}, // P_GB_P3Q2
    {0x36EA, 0x4935,0,0}, // P_GB_P3Q3
    {0x36EC, 0xBCB6,0,0}, // P_GB_P3Q4
    {0x36EE, 0xB5F3,0,0}, // P_GR_P4Q0
    {0x36F0, 0xBAD4,0,0}, // P_GR_P4Q1
    {0x36F2, 0x8E39,0,0}, // P_GR_P4Q2
    {0x36F4, 0x1FF8,0,0}, // P_GR_P4Q3
    {0x36F6, 0x1D3C,0,0}, // P_GR_P4Q4
    {0x36F8, 0x8CB3,0,0}, // P_RD_P4Q0
    {0x36FA, 0x8834,0,0}, // P_RD_P4Q1
    {0x36FC, 0xBF58,0,0}, // P_RD_P4Q2
    {0x36FE, 0x4239,0,0}, // P_RD_P4Q3
    {0x3700, 0x19F9,0,0}, // P_RD_P4Q4
    {0x3702, 0x770D,0,0}, // P_BL_P4Q0
    {0x3704, 0x7234,0,0}, // P_BL_P4Q1
    {0x3706, 0xCB98,0,0}, // P_BL_P4Q2
    {0x3708, 0x84B9,0,0}, // P_BL_P4Q3
    {0x370A, 0x33FC,0,0}, // P_BL_P4Q4
    {0x370C, 0xA1D2,0,0}, // P_GB_P4Q0
    {0x370E, 0xAE33,0,0}, // P_GB_P4Q1
    {0x3710, 0x8E79,0,0}, // P_GB_P4Q2
    {0x3712, 0x4AB8,0,0}, // P_GB_P4Q3
    {0x3714, 0x2D1C,0,0}, // P_GB_P4Q4
    {0x3644, 0x013C,0,0}, // POLY_ORIGIN_C
    {0x3642, 0x00E8,0,0}, // POLY_ORIGIN_R
    {0x3210, 0x09B8,0,0}, // COLOR_PIPELINE_CONTROL
    // Char Setting and fine-tuning
    {0x098C, 0xAB1F,0,0}, // MCU_ADDRESS [HG_LLMODE]
    {0x0990, 0x00C7,0,0}, // MCU_DATA_0
    {0x098C, 0xAB31,0,0}, // MCU_ADDRESS [HG_NR_STOP_G]
    {0x0990, 0x001E,0,0}, // MCU_DATA_0
    {0x098C, 0x274F,0,0}, // MCU_ADDRESS [RESERVED_MODE_4F]
    {0x0990, 0x0004,0,0}, // MCU_DATA_0
    {0x098C, 0x2741,0,0}, // MCU_ADDRESS [RESERVED_MODE_41]
    {0x0990, 0x0004,0,0}, // MCU_DATA_0
    {0x098C, 0xAB20,0,0}, // MCU_ADDRESS [HG_LL_SAT1]
    {0x0990, 0x0054,0,0}, // MCU_DATA_0
    {0x098C, 0xAB21,0,0}, // MCU_ADDRESS [HG_LL_INTERPTHRESH1]
    {0x0990, 0x0046,0,0}, // MCU_DATA_0
    {0x098C, 0xAB22,0,0}, // MCU_ADDRESS [HG_LL_APCORR1]
    {0x0990, 0x0002,0,0}, // MCU_DATA_0
    {0x098C, 0xAB24,0,0}, // MCU_ADDRESS [HG_LL_SAT2]
    {0x0990, 0x0005,0,0}, // MCU_DATA_0
    {0x098C, 0x2B28,0,0}, // MCU_ADDRESS [HG_LL_BRIGHTNESSSTART]
    {0x0990, 0x170C,0,0}, // MCU_DATA_0
    {0x098C, 0x2B2A,0,0}, // MCU_ADDRESS [HG_LL_BRIGHTNESSSTOP]
    {0x0990, 0x3E80,0,0}, // MCU_DATA_0
    {0x3210, 0x09B8,0,0}, // COLOR_PIPELINE_CONTROL
    {0x098C, 0x2306,0,0}, // MCU_ADDRESS [AWB_CCM_L_0]
    {0x0990, 0x0315,0,0}, // MCU_DATA_0
    {0x098C, 0x2308,0,0}, // MCU_ADDRESS [AWB_CCM_L_1]
    {0x0990, 0xFDDC,0,0}, // MCU_DATA_0
    {0x098C, 0x230A,0,0}, // MCU_ADDRESS [AWB_CCM_L_2]
    {0x0990, 0x003A,0,0}, // MCU_DATA_0
    {0x098C, 0x230C,0,0}, // MCU_ADDRESS [AWB_CCM_L_3]
    {0x0990, 0xFF58,0,0}, // MCU_DATA_0
    {0x098C, 0x230E,0,0}, // MCU_ADDRESS [AWB_CCM_L_4]
    {0x0990, 0x02B7,0,0}, // MCU_DATA_0
    {0x098C, 0x2310,0,0}, // MCU_ADDRESS [AWB_CCM_L_5]
    {0x0990, 0xFF31,0,0}, // MCU_DATA_0
    {0x098C, 0x2312,0,0}, // MCU_ADDRESS [AWB_CCM_L_6]
    {0x0990, 0xFF4C,0,0}, // MCU_DATA_0
    {0x098C, 0x2314,0,0}, // MCU_ADDRESS [AWB_CCM_L_7]
    {0x0990, 0xFE4C,0,0}, // MCU_DATA_0
    {0x098C, 0x2316,0,0}, // MCU_ADDRESS [AWB_CCM_L_8]
    {0x0990, 0x039E,0,0}, // MCU_DATA_0
    {0x098C, 0x2318,0,0}, // MCU_ADDRESS [AWB_CCM_L_9]
    {0x0990, 0x001C,0,0}, // MCU_DATA_0
    {0x098C, 0x231A,0,0}, // MCU_ADDRESS [AWB_CCM_L_10]
    {0x0990, 0x0039,0,0}, // MCU_DATA_0
    {0x098C, 0x231C,0,0}, // MCU_ADDRESS [AWB_CCM_RL_0]
    {0x0990, 0x007F,0,0}, // MCU_DATA_0
    {0x098C, 0x231E,0,0}, // MCU_ADDRESS [AWB_CCM_RL_1]
    {0x0990, 0xFF77,0,0}, // MCU_DATA_0
    {0x098C, 0x2320,0,0}, // MCU_ADDRESS [AWB_CCM_RL_2]
    {0x0990, 0x000A,0,0}, // MCU_DATA_0
    {0x098C, 0x2322,0,0}, // MCU_ADDRESS [AWB_CCM_RL_3]
    {0x0990, 0x0020,0,0}, // MCU_DATA_0
    {0x098C, 0x2324,0,0}, // MCU_ADDRESS [AWB_CCM_RL_4]
    {0x0990, 0x001B,0,0}, // MCU_DATA_0
    {0x098C, 0x2326,0,0}, // MCU_ADDRESS [AWB_CCM_RL_5]
    {0x0990, 0xFFC6,0,0}, // MCU_DATA_0
    {0x098C, 0x2328,0,0}, // MCU_ADDRESS [AWB_CCM_RL_6]
    {0x0990, 0x0086,0,0}, // MCU_DATA_0
    {0x098C, 0x232A,0,0}, // MCU_ADDRESS [AWB_CCM_RL_7]
    {0x0990, 0x00B5,0,0}, // MCU_DATA_0
    {0x098C, 0x232C,0,0}, // MCU_ADDRESS [AWB_CCM_RL_8]
    {0x0990, 0xFEC3,0,0}, // MCU_DATA_0
    {0x098C, 0x232E,0,0}, // MCU_ADDRESS [AWB_CCM_RL_9]
    {0x0990, 0x0001,0,0}, // MCU_DATA_0
    {0x098C, 0x2330,0,0}, // MCU_ADDRESS [AWB_CCM_RL_10]
    {0x0990, 0xFFEF,0,0}, // MCU_DATA_0
    {0x098C, 0xA348,0,0}, // MCU_ADDRESS [AWB_GAIN_BUFFER_SPEED]
    {0x0990, 0x0008,0,0}, // MCU_DATA_0
    {0x098C, 0xA349,0,0}, // MCU_ADDRESS [AWB_JUMP_DIVISOR]
    {0x0990, 0x0002,0,0}, // MCU_DATA_0
    {0x098C, 0xA34A,0,0}, // MCU_ADDRESS [AWB_GAIN_MIN]
    {0x0990, 0x0090,0,0}, // MCU_DATA_0
    {0x098C, 0xA34B,0,0}, // MCU_ADDRESS [AWB_GAIN_MAX]
    {0x0990, 0x00FF,0,0}, // MCU_DATA_0
    {0x098C, 0xA34C,0,0}, // MCU_ADDRESS [AWB_GAINMIN_B]
    {0x0990, 0x0075,0,0}, // MCU_DATA_0
    {0x098C, 0xA34D,0,0}, // MCU_ADDRESS [AWB_GAINMAX_B]
    {0x0990, 0x00EF,0,0}, // MCU_DATA_0
    {0x098C, 0xA351,0,0}, // MCU_ADDRESS [AWB_CCM_POSITION_MIN]
    {0x0990, 0x0000,0,0}, // MCU_DATA_0
    {0x098C, 0xA352,0,0}, // MCU_ADDRESS [AWB_CCM_POSITION_MAX]
    {0x0990, 0x007F,0,0}, // MCU_DATA_0
    {0x098C, 0xA354,0,0}, // MCU_ADDRESS [AWB_SATURATION]
    {0x0990, 0x0043,0,0}, // MCU_DATA_0
    {0x098C, 0xA355,0,0}, // MCU_ADDRESS [AWB_MODE]
    {0x0990, 0x0001,0,0}, // MCU_DATA_0
    {0x098C, 0xA35D,0,0}, // MCU_ADDRESS [AWB_STEADY_BGAIN_OUT_MIN]
    {0x0990, 0x0078,0,0}, // MCU_DATA_0
    {0x098C, 0xA35E,0,0}, // MCU_ADDRESS [AWB_STEADY_BGAIN_OUT_MAX]
    {0x0990, 0x0086,0,0}, // MCU_DATA_0
    {0x098C, 0xA35F,0,0}, // MCU_ADDRESS [AWB_STEADY_BGAIN_IN_MIN]
    {0x0990, 0x007E,0,0}, // MCU_DATA_0
    {0x098C, 0xA360,0,0}, // MCU_ADDRESS [AWB_STEADY_BGAIN_IN_MAX]
    {0x0990, 0x0082,0,0}, // MCU_DATA_0
    {0x098C, 0x2361,0,0}, // MCU_ADDRESS [AWB_CNT_PXL_TH]
    {0x0990, 0x0040,0,0}, // MCU_DATA_0
    {0x098C, 0xA363,0,0}, // MCU_ADDRESS [AWB_TG_MIN0]
    {0x0990, 0x00D2,0,0}, // MCU_DATA_0
    {0x098C, 0xA364,0,0}, // MCU_ADDRESS [AWB_TG_MAX0]
    {0x0990, 0x00F6,0,0}, // MCU_DATA_0
    {0x098C, 0xA302,0,0}, // MCU_ADDRESS [AWB_WINDOW_POS]
    {0x0990, 0x0000,0,0}, // MCU_DATA_0
    {0x098C, 0xA303,0,0}, // MCU_ADDRESS [AWB_WINDOW_SIZE]
    {0x0990, 0x00EF,0,0}, // MCU_DATA_0
    {0x098C, 0xAB20,0,0}, // MCU_ADDRESS [HG_LL_SAT1]
    {0x0990, 0x0024,0,0}, // MCU_DATA_0
    // AE Setting Frame rate 7.5fps ~ 30fps
    {0x098C, 0xA20C,0,0}, // MCU_ADDRESS
    {0x0990, 0x0004,0,0}, // AE_MAX_INDEX
    {0x098C, 0xA215,0,0}, // MCU_ADDRESS
    {0x0990, 0x0004,0,0}, // AE_INDEX_TH23
    {0x098C, 0xA20D,0,0}, // MCU_ADDRESS [AE_MIN_VIRTGAIN]
    {0x0990, 0x0030,0,0}, // MCU_DATA_0
    {0x098C, 0xA20E,0,0}, // MCU_ADDRESS [AE_MAX_VIRTGAIN]
    {0x0990, 0x0080,0,0}, // MCU_DATA_0
    // Sharpness
    {0x098C, 0xAB22,0,0}, // MCU_ADDRESS [HG_LL_APCORR1]
    {0x0990, 0x0001,0,0}, // MCU_DATA_0
    // AE Gate
    {0x098C, 0xA207,0,0}, // MCU_ADDRESS [AE_GATE]
    {0x0990, 0x0006,0,0}, // MCU_DATA_0
    // AE Target
    {0x098C, 0xA24F,0,0}, // MCU_ADDRESS [AE_BASETARGET]
    {0x0990, 0x0044,0,0}, // MCU_DATA_0
    {0x098C, 0xAB3C,0,0}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
    {0x0990, 0x0000,0,0}, // MCU_DATA_0
    {0x098C, 0xAB3D,0,0}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
    {0x0990, 0x0007,0,0}, // MCU_DATA_0
    {0x098C, 0xAB3E,0,0}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
    {0x0990, 0x0016,0,0}, // MCU_DATA_0
    {0x098C, 0xAB3F,0,0}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
    {0x0990, 0x0039,0,0}, // MCU_DATA_0
    {0x098C, 0xAB40,0,0}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
    {0x0990, 0x005F,0,0}, // MCU_DATA_0
    {0x098C, 0xAB41,0,0}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
    {0x0990, 0x007A,0,0}, // MCU_DATA_0
    {0x098C, 0xAB42,0,0}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
    {0x0990, 0x008F,0,0}, // MCU_DATA_0
    {0x098C, 0xAB43,0,0}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
    {0x0990, 0x00A1,0,0}, // MCU_DATA_0
    {0x098C, 0xAB44,0,0}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
    {0x0990, 0x00AF,0,0}, // MCU_DATA_0
    {0x098C, 0xAB45,0,0}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
    {0x0990, 0x00BB,0,0}, // MCU_DATA_0
    {0x098C, 0xAB46,0,0}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
    {0x0990, 0x00C6,0,0}, // MCU_DATA_0
    {0x098C, 0xAB47,0,0}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
    {0x0990, 0x00CF,0,0}, // MCU_DATA_0
    {0x098C, 0xAB48,0,0}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
    {0x0990, 0x00D8,0,0}, // MCU_DATA_0
    {0x098C, 0xAB49,0,0}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
    {0x0990, 0x00E0,0,0}, // MCU_DATA_0
    {0x098C, 0xAB4A,0,0}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
    {0x0990, 0x00E7,0,0}, // MCU_DATA_0
    {0x098C, 0xAB4B,0,0}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
    {0x0990, 0x00EE,0,0}, // MCU_DATA_0
    {0x098C, 0xAB4C,0,0}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
    {0x0990, 0x00F4,0,0}, // MCU_DATA_0
    {0x098C, 0xAB4D,0,0}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
    {0x0990, 0x00FA,0,0}, // MCU_DATA_0
    {0x098C, 0xAB4E,0,0}, // MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
    {0x0990, 0x00FF,0,0}, // MCU_DATA_0
    // saturation
    {0x098C, 0xAB20,0,0}, // MCU_ADDRESS [HG_LL_SAT1]
    {0x0990, 0x0045,0,0}, // MCU_DATA_0
    {0x098C, 0xAB24,0,0}, // MCU_ADDRESS [HG_LL_SAT2]
    {0x0990, 0x0034,0,0}, // MCU_DATA_0
    {0x098C, 0xA20C,0,0}, // MCU_ADDRESS
    {0x0990, 0x0018,0,0}, // AE_MAX_INDEX
    {0x098C, 0xA215,0,0}, // MCU_ADDRESS
    {0x0990, 0x0008,0,0}, // AE_INDEX_TH23
    {0x098C, 0xA20C,0,0}, // MCU_ADDRESS
    {0x0990, 0x0018,0,0}, // AE_MAX_INDEX
    {0x098C, 0xA215,0,0}, // MCU_ADDRESS
    {0x0990, 0x0018,0,0}, // AE_INDEX_TH23
    {0x098C, 0xA34A,0,0}, // MCU_ADDRESS
    {0x0990, 0x007A,0,0}, // MCU_DATA_0
    {0x098C, 0xA103,0,0}, // MCU_ADDRESS
    {0x0990, 0x0006,0,200}, // MCU_DATA_0
    {0x098C, 0xA103,0,0}, // MCU_ADDRESS
    {0x0990, 0x0005,0,100}, // MCU_DATA_0
#if 0
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //Comments for Timing
    ; This file was generated by: MT9V113 (SOC0380) Register Wizard
    ; Version: 4.1.11.28409 Build Date: 07/12/2011
    ;
    ; [PLL PARAMETERS]
    ;
    ; Bypass PLL: Unchecked
    ; Input Frequency: 24.000
    ; Use Min Freq.: Unchecked
    ; Target PLL Frequency: 28.000
    ; Target VCO Frequency: Unspecified
    ; "M" Value: Unspecified
    ; "N" Value: Unspecified
    ;
    ; Target PLL Frequency: 28 MHz
    ; Input Clock Frequency: 24 MHz
    ;
    ; Actual PLL Frequency: 28 MHz
    ;
    ; M=28
    ; N=2
    ; Fpdf=8 MHz
    ; Fvco=448 MHz
    ;
    ; [CONTEXT A PARAMETERS]
    ;
    ; Requested Frames Per Second: 27.500
    ; Output Columns: 640
    ; Output Rows: 480
    ; Allow Skipping: Unchecked
    ; Use Context B Line Time: Unchecked
    ; Low Power: Unchecked
    ; Blanking Computation: HB Min then VB
    ;
    ; Max Frame Time: 36.3636 msec
    ; Max Frame Clocks: 509090.9 clocks (14 MHz)
    ; Pixel Clock: divided by 1
    ; Skip Mode: 1x cols, 1x rows, Bin Mode: No
    ; Horiz clks: 648 active + 194 blank=842 total
    ; Vert rows: 488 active + 116 blank=604 total
    ; Extra Delay: 522 clocks
    ;
    ; Actual Frame Clocks: 509090 clocks
    ; Row Time: 60.143 usec / 842 clocks
    ; Frame time: 36.363571 msec
    ; Frames per Sec: 27.500 fps
    ;
    ; 50Hz Flicker Period: 166.27 lines
    ; 60Hz Flicker Period: 138.56 lines
    ;
    ; [CONTEXT B PARAMETERS]
    ;
    ; Requested Frames Per Second: 14.645
    ; Output Columns: 640
    ; Output Rows: 480
    ; Allow Skipping: Unchecked
    ; Use Context A Line Time: Unchecked
    ; Low Power: Unchecked
    ; Blanking Computation: HB Min then VB
    ;
    ; Max Frame Time: 68.2827 msec
    ; Max Frame Clocks: 955957.6 clocks (14 MHz)
    ; Pixel Clock: divided by 1
    ; Skip Mode: 1x cols, 1x rows, Bin Mode: No
    ; Horiz clks: 648 active + 194 blank=842 total
    ; Vert rows: 488 active + 647 blank=1135 total
    ; Extra Delay: 287 clocks
    ;
    ; Actual Frame Clocks: 955957 clocks
    ; Row Time: 60.143 usec / 842 clocks
    ; Frame time: 68.282643 msec
    ; Frames per Sec: 14.645 fps
    ;
    ; 50Hz Flicker Period: 166.27 lines
    ; 60Hz Flicker Period: 138.56 lines
    ;
    ;

    [MIPI Enable]
    REG=0x3400, 0x7A2C // MIPI_CONTROL

    [Low light frame rate]
    REG=0x098C, 0xA20C // MCU_ADDRESS
    REG=0x0990, 0x18 // AE_MAX_INDEX
    REG=0x098C, 0xA215 // MCU_ADDRESS
    REG=0x0990, 0x18 // AE_INDEX_TH23
    REG=0x098C, 0xA103 // MCU_ADDRESS
    REG=0x0990, 0x0005 // MCU_DATA_0


    [30fps]
    REG=0x098C, 0xA20C // MCU_ADDRESS
    REG=0x0990, 0x04 // AE_MAX_INDEX
    REG=0x098C, 0xA215 // MCU_ADDRESS
    REG=0x0990, 0x04 // AE_INDEX_TH23
    REG=0x098C, 0xA103 // MCU_ADDRESS
    REG=0x0990, 0x0005 // MCU_DATA_0
#endif
    {0x098C, 0xA244,0,0},
    {0x0990, 0x00BB,0,0},
    {0x3400, 0x7a26,0,0},
};

/******************************************************************************
Preview Setting Table 30Fps
******************************************************************************/
MT9V113_WREG mt9v113_preview_tbl_30fps[] =
{

};

/******************************************************************************
Preview Setting Table 60Fps
******************************************************************************/
MT9V113_WREG mt9v113_preview_tbl_60fps[] =
{

};

/******************************************************************************
Preview Setting Table 90Fps
******************************************************************************/
MT9V113_WREG mt9v113_preview_tbl_90fps[] =
{

};

/******************************************************************************
Capture Setting Table
******************************************************************************/
MT9V113_WREG mt9v113_capture_tbl[]=
{

};

/******************************************************************************
Contrast Setting
******************************************************************************/
MT9V113_WREG mt9v113_contrast_lv0_tbl[] =
{
//Contrast -4
  {0x098C,0xAB3C,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
  {0x0990,0x0000,0,0},//MCU_DATA_0
  {0x098C,0xAB3D,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
  {0x0990,0x006F,0,0},//MCU_DATA_0
  {0x098C,0xAB3E,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
  {0x0990,0x0080,0,0},//MCU_DATA_0
  {0x098C,0xAB3F,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
  {0x0990,0x0092,0,0},//MCU_DATA_0
  {0x098C,0xAB40,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
  {0x0990,0x00A8,0,0},//MCU_DATA_0
  {0x098C,0xAB41,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
  {0x0990,0x00B6,0,0},//MCU_DATA_0
  {0x098C,0xAB42,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
  {0x0990,0x00C1,0,0},//MCU_DATA_0
  {0x098C,0xAB43,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
  {0x0990,0x00CA,0,0},//MCU_DATA_0
  {0x098C,0xAB44,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
  {0x0990,0x00D2,0,0},//MCU_DATA_0
  {0x098C,0xAB45,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
  {0x0990,0x00D8,0,0},//MCU_DATA_0
  {0x098C,0xAB46,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
  {0x0990,0x00DE,0,0},//MCU_DATA_0
  {0x098C,0xAB47,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
  {0x0990,0x00E3,0,0},//MCU_DATA_0
  {0x098C,0xAB48,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
  {0x0990,0x00E8,0,0},//MCU_DATA_0
  {0x098C,0xAB49,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
  {0x0990,0x00ED,0,0},//MCU_DATA_0
  {0x098C,0xAB4A,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
  {0x0990,0x00F1,0,0},//MCU_DATA_0
  {0x098C,0xAB4B,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
  {0x0990,0x00F5,0,0},//MCU_DATA_0
  {0x098C,0xAB4C,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
  {0x0990,0x00F8,0,0},//MCU_DATA_0
  {0x098C,0xAB4D,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
  {0x0990,0x00FC,0,0},//MCU_DATA_0
  {0x098C,0xAB4E,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
  {0x0990,0x00FF,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_contrast_lv1_tbl[] =
{
//Contrast -3
  {0x098C,0xAB3C,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
  {0x0990,0x0000,0,0},//MCU_DATA_0
  {0x098C,0xAB3D,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
  {0x0990,0x0038,0,0},//MCU_DATA_0
  {0x098C,0xAB3E,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
  {0x0990,0x0060,0,0},//MCU_DATA_0
  {0x098C,0xAB3F,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
  {0x0990,0x0079,0,0},//MCU_DATA_0
  {0x098C,0xAB40,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
  {0x0990,0x0094,0,0},//MCU_DATA_0
  {0x098C,0xAB41,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
  {0x0990,0x00A6,0,0},//MCU_DATA_0
  {0x098C,0xAB42,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
  {0x0990,0x00B3,0,0},//MCU_DATA_0
  {0x098C,0xAB43,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
  {0x0990,0x00BD,0,0},//MCU_DATA_0
  {0x098C,0xAB44,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
  {0x0990,0x00C7,0,0},//MCU_DATA_0
  {0x098C,0xAB45,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
  {0x0990,0x00CF,0,0},//MCU_DATA_0
  {0x098C,0xAB46,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
  {0x0990,0x00D6,0,0},//MCU_DATA_0
  {0x098C,0xAB47,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
  {0x0990,0x00DC,0,0},//MCU_DATA_0
  {0x098C,0xAB48,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
  {0x0990,0x00E2,0,0},//MCU_DATA_0
  {0x098C,0xAB49,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
  {0x0990,0x00E8,0,0},//MCU_DATA_0
  {0x098C,0xAB4A,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
  {0x0990,0x00ED,0,0},//MCU_DATA_0
  {0x098C,0xAB4B,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
  {0x0990,0x00F2,0,0},//MCU_DATA_0
  {0x098C,0xAB4C,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
  {0x0990,0x00F7,0,0},//MCU_DATA_0
  {0x098C,0xAB4D,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
  {0x0990,0x00FB,0,0},//MCU_DATA_0
  {0x098C,0xAB4E,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
  {0x0990,0x00FF,0,0},//MCU_DATA_0

};
MT9V113_WREG mt9v113_contrast_lv2_tbl[] =
{
//Contrast -2
  {0x098C,0xAB3C,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
  {0x0990,0x0000,0,0},//MCU_DATA_0
  {0x098C,0xAB3D,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
  {0x0990,0x002C,0,0},//MCU_DATA_0
  {0x098C,0xAB3E,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
  {0x0990,0x004F,0,0},//MCU_DATA_0
  {0x098C,0xAB3F,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
  {0x0990,0x0069,0,0},//MCU_DATA_0
  {0x098C,0xAB40,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
  {0x0990,0x0085,0,0},//MCU_DATA_0
  {0x098C,0xAB41,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
  {0x0990,0x0098,0,0},//MCU_DATA_0
  {0x098C,0xAB42,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
  {0x0990,0x00A6,0,0},//MCU_DATA_0
  {0x098C,0xAB43,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
  {0x0990,0x00B2,0,0},//MCU_DATA_0
  {0x098C,0xAB44,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
  {0x0990,0x00BD,0,0},//MCU_DATA_0
  {0x098C,0xAB45,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
  {0x0990,0x00C6,0,0},//MCU_DATA_0
  {0x098C,0xAB46,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
  {0x0990,0x00CE,0,0},//MCU_DATA_0
  {0x098C,0xAB47,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
  {0x0990,0x00D6,0,0},//MCU_DATA_0
  {0x098C,0xAB48,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
  {0x0990,0x00DD,0,0},//MCU_DATA_0
  {0x098C,0xAB49,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
  {0x0990,0x00E4,0,0},//MCU_DATA_0
  {0x098C,0xAB4A,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
  {0x0990,0x00EA,0,0},//MCU_DATA_0
  {0x098C,0xAB4B,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
  {0x0990,0x00EF,0,0},//MCU_DATA_0
  {0x098C,0xAB4C,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
  {0x0990,0x00F5,0,0},//MCU_DATA_0
  {0x098C,0xAB4D,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
  {0x0990,0x00FA,0,0},//MCU_DATA_0
  {0x098C,0xAB4E,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
  {0x0990,0x00FF,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_contrast_lv3_tbl[] =
{
//Contrast -1
  {0x098C,0xAB3C,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
  {0x0990,0x0000,0,0},//MCU_DATA_0
  {0x098C,0xAB3D,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
  {0x0990,0x001B,0,0},//MCU_DATA_0
  {0x098C,0xAB3E,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
  {0x0990,0x0035,0,0},//MCU_DATA_0
  {0x098C,0xAB3F,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
  {0x0990,0x004E,0,0},//MCU_DATA_0
  {0x098C,0xAB40,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
  {0x0990,0x006B,0,0},//MCU_DATA_0
  {0x098C,0xAB41,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
  {0x0990,0x0080,0,0},//MCU_DATA_0
  {0x098C,0xAB42,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
  {0x0990,0x0090,0,0},//MCU_DATA_0
  {0x098C,0xAB43,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
  {0x0990,0x009E,0,0},//MCU_DATA_0
  {0x098C,0xAB44,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
  {0x0990,0x00AB,0,0},//MCU_DATA_0
  {0x098C,0xAB45,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
  {0x0990,0x00B6,0,0},//MCU_DATA_0
  {0x098C,0xAB46,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
  {0x0990,0x00C0,0,0},//MCU_DATA_0
  {0x098C,0xAB47,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
  {0x0990,0x00CA,0,0},//MCU_DATA_0
  {0x098C,0xAB48,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
  {0x0990,0x00D3,0,0},//MCU_DATA_0
  {0x098C,0xAB49,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
  {0x0990,0x00DB,0,0},//MCU_DATA_0
  {0x098C,0xAB4A,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
  {0x0990,0x00E3,0,0},//MCU_DATA_0
  {0x098C,0xAB4B,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
  {0x0990,0x00EA,0,0},//MCU_DATA_0
  {0x098C,0xAB4C,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
  {0x0990,0x00F2,0,0},//MCU_DATA_0
  {0x098C,0xAB4D,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
  {0x0990,0x00F8,0,0},//MCU_DATA_0
  {0x098C,0xAB4E,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
  {0x0990,0x00FF,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_contrast_default_lv4_tbl[] =
{
//Contrast (Default)
  {0x098C,0xAB3C,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
  {0x0990,0x0000,0,0},//MCU_DATA_0
  {0x098C,0xAB3D,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
  {0x0990,0x0007,0,0},//MCU_DATA_0
  {0x098C,0xAB3E,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
  {0x0990,0x0016,0,0},//MCU_DATA_0
  {0x098C,0xAB3F,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
  {0x0990,0x0039,0,0},//MCU_DATA_0
  {0x098C,0xAB40,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
  {0x0990,0x005F,0,0},//MCU_DATA_0
  {0x098C,0xAB41,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
  {0x0990,0x007A,0,0},//MCU_DATA_0
  {0x098C,0xAB42,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
  {0x0990,0x008F,0,0},//MCU_DATA_0
  {0x098C,0xAB43,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
  {0x0990,0x00A1,0,0},//MCU_DATA_0
  {0x098C,0xAB44,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
  {0x0990,0x00AF,0,0},//MCU_DATA_0
  {0x098C,0xAB45,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
  {0x0990,0x00BB,0,0},//MCU_DATA_0
  {0x098C,0xAB46,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
  {0x0990,0x00C6,0,0},//MCU_DATA_0
  {0x098C,0xAB47,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
  {0x0990,0x00CF,0,0},//MCU_DATA_0
  {0x098C,0xAB48,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
  {0x0990,0x00D8,0,0},//MCU_DATA_0
  {0x098C,0xAB49,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
  {0x0990,0x00E0,0,0},//MCU_DATA_0
  {0x098C,0xAB4A,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
  {0x0990,0x00E7,0,0},//MCU_DATA_0
  {0x098C,0xAB4B,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
  {0x0990,0x00EE,0,0},//MCU_DATA_0
  {0x098C,0xAB4C,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
  {0x0990,0x00F4,0,0},//MCU_DATA_0
  {0x098C,0xAB4D,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
  {0x0990,0x00FA,0,0},//MCU_DATA_0
  {0x098C,0xAB4E,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
  {0x0990,0x00FF,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_contrast_lv5_tbl[] =
{
//Contrast +1
  {0x098C,0xAB3C,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
  {0x0990,0x0000,0,0},//MCU_DATA_0
  {0x098C,0xAB3D,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
  {0x0990,0x0005,0,0},//MCU_DATA_0
  {0x098C,0xAB3E,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
  {0x0990,0x000F,0,0},//MCU_DATA_0
  {0x098C,0xAB3F,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
  {0x0990,0x0027,0,0},//MCU_DATA_0
  {0x098C,0xAB40,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
  {0x0990,0x0048,0,0},//MCU_DATA_0
  {0x098C,0xAB41,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
  {0x0990,0x0062,0,0},//MCU_DATA_0
  {0x098C,0xAB42,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
  {0x0990,0x0078,0,0},//MCU_DATA_0
  {0x098C,0xAB43,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
  {0x0990,0x008C,0,0},//MCU_DATA_0
  {0x098C,0xAB44,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
  {0x0990,0x009D,0,0},//MCU_DATA_0
  {0x098C,0xAB45,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
  {0x0990,0x00AB,0,0},//MCU_DATA_0
  {0x098C,0xAB46,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
  {0x0990,0x00B8,0,0},//MCU_DATA_0
  {0x098C,0xAB47,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
  {0x0990,0x00C4,0,0},//MCU_DATA_0
  {0x098C,0xAB48,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
  {0x0990,0x00CF,0,0},//MCU_DATA_0
  {0x098C,0xAB49,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
  {0x0990,0x00D8,0,0},//MCU_DATA_0
  {0x098C,0xAB4A,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
  {0x0990,0x00E1,0,0},//MCU_DATA_0
  {0x098C,0xAB4B,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
  {0x0990,0x00E9,0,0},//MCU_DATA_0
  {0x098C,0xAB4C,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
  {0x0990,0x00F1,0,0},//MCU_DATA_0
  {0x098C,0xAB4D,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
  {0x0990,0x00F8,0,0},//MCU_DATA_0
  {0x098C,0xAB4E,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
  {0x0990,0x00FF,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_contrast_lv6_tbl[] =
{
//Contrast +2
  {0x098C,0xAB3C,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
  {0x0990,0x0000,0,0},//MCU_DATA_0
  {0x098C,0xAB3D,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
  {0x0990,0x0003,0,0},//MCU_DATA_0
  {0x098C,0xAB3E,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
  {0x0990,0x000A,0,0},//MCU_DATA_0
  {0x098C,0xAB3F,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
  {0x0990,0x001B,0,0},//MCU_DATA_0
  {0x098C,0xAB40,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
  {0x0990,0x0038,0,0},//MCU_DATA_0
  {0x098C,0xAB41,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
  {0x0990,0x004F,0,0},//MCU_DATA_0
  {0x098C,0xAB42,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
  {0x0990,0x0064,0,0},//MCU_DATA_0
  {0x098C,0xAB43,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
  {0x0990,0x0078,0,0},//MCU_DATA_0
  {0x098C,0xAB44,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
  {0x0990,0x008B,0,0},//MCU_DATA_0
  {0x098C,0xAB45,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
  {0x0990,0x009C,0,0},//MCU_DATA_0
  {0x098C,0xAB46,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
  {0x0990,0x00AB,0,0},//MCU_DATA_0
  {0x098C,0xAB47,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
  {0x0990,0x00B9,0,0},//MCU_DATA_0
  {0x098C,0xAB48,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
  {0x0990,0x00C5,0,0},//MCU_DATA_0
  {0x098C,0xAB49,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
  {0x0990,0x00D1,0,0},//MCU_DATA_0
  {0x098C,0xAB4A,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
  {0x0990,0x00DB,0,0},//MCU_DATA_0
  {0x098C,0xAB4B,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
  {0x0990,0x00E5,0,0},//MCU_DATA_0
  {0x098C,0xAB4C,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
  {0x0990,0x00EE,0,0},//MCU_DATA_0
  {0x098C,0xAB4D,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
  {0x0990,0x00F7,0,0},//MCU_DATA_0
  {0x098C,0xAB4E,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
  {0x0990,0x00FF,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_contrast_lv7_tbl[] =
{
//Contrast +3
  {0x098C,0xAB3C,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
  {0x0990,0x0000,0,0},//MCU_DATA_0
  {0x098C,0xAB3D,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
  {0x0990,0x0002,0,0},//MCU_DATA_0
  {0x098C,0xAB3E,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
  {0x0990,0x0007,0,0},//MCU_DATA_0
  {0x098C,0xAB3F,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
  {0x0990,0x0015,0,0},//MCU_DATA_0
  {0x098C,0xAB40,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
  {0x0990,0x002D,0,0},//MCU_DATA_0
  {0x098C,0xAB41,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
  {0x0990,0x0043,0,0},//MCU_DATA_0
  {0x098C,0xAB42,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
  {0x0990,0x0058,0,0},//MCU_DATA_0
  {0x098C,0xAB43,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
  {0x0990,0x006D,0,0},//MCU_DATA_0
  {0x098C,0xAB44,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
  {0x0990,0x0083,0,0},//MCU_DATA_0
  {0x098C,0xAB45,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
  {0x0990,0x0097,0,0},//MCU_DATA_0
  {0x098C,0xAB46,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
  {0x0990,0x00A8,0,0},//MCU_DATA_0
  {0x098C,0xAB47,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
  {0x0990,0x00B7,0,0},//MCU_DATA_0
  {0x098C,0xAB48,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
  {0x0990,0x00C5,0,0},//MCU_DATA_0
  {0x098C,0xAB49,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
  {0x0990,0x00D1,0,0},//MCU_DATA_0
  {0x098C,0xAB4A,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
  {0x0990,0x00DC,0,0},//MCU_DATA_0
  {0x098C,0xAB4B,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
  {0x0990,0x00E5,0,0},//MCU_DATA_0
  {0x098C,0xAB4C,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
  {0x0990,0x00EF,0,0},//MCU_DATA_0
  {0x098C,0xAB4D,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
  {0x0990,0x00F7,0,0},//MCU_DATA_0
  {0x098C,0xAB4E,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
  {0x0990,0x00FF,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_contrast_lv8_tbl[] =
{
//Contrast +4
  {0x098C,0xAB3C,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
  {0x0990,0x0000,0,0},//MCU_DATA_0
  {0x098C,0xAB3D,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
  {0x0990,0x0002,0,0},//MCU_DATA_0
  {0x098C,0xAB3E,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
  {0x0990,0x0004,0,0},//MCU_DATA_0
  {0x098C,0xAB3F,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
  {0x0990,0x000E,0,0},//MCU_DATA_0
  {0x098C,0xAB40,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
  {0x0990,0x0023,0,0},//MCU_DATA_0
  {0x098C,0xAB41,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
  {0x0990,0x0038,0,0},//MCU_DATA_0
  {0x098C,0xAB42,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
  {0x0990,0x004C,0,0},//MCU_DATA_0
  {0x098C,0xAB43,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
  {0x0990,0x0061,0,0},//MCU_DATA_0
  {0x098C,0xAB44,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
  {0x0990,0x0078,0,0},//MCU_DATA_0
  {0x098C,0xAB45,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
  {0x0990,0x008E,0,0},//MCU_DATA_0
  {0x098C,0xAB46,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
  {0x0990,0x00A2,0,0},//MCU_DATA_0
  {0x098C,0xAB47,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
  {0x0990,0x00B3,0,0},//MCU_DATA_0
  {0x098C,0xAB48,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
  {0x0990,0x00C2,0,0},//MCU_DATA_0
  {0x098C,0xAB49,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
  {0x0990,0x00CF,0,0},//MCU_DATA_0
  {0x098C,0xAB4A,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
  {0x0990,0x00DA,0,0},//MCU_DATA_0
  {0x098C,0xAB4B,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
  {0x0990,0x00E5,0,0},//MCU_DATA_0
  {0x098C,0xAB4C,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
  {0x0990,0x00EE,0,0},//MCU_DATA_0
  {0x098C,0xAB4D,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
  {0x0990,0x00F7,0,0},//MCU_DATA_0
  {0x098C,0xAB4E,0,0},//MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
  {0x0990,0x00FF,0,0},//MCU_DATA_0
};

/******************************************************************************
Sharpness Setting
******************************************************************************/
MT9V113_WREG mt9v113_sharpness_lv0_tbl[] =
{
//Sharpness 0
};
MT9V113_WREG mt9v113_sharpness_lv1_tbl[] =
{
//Sharpness 1
  {0x098C,0xAB22,0,0},//MCU_ADDRESS [HG_LL_APCORR1]
  {0x0990,0x0000,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_sharpness_default_lv2_tbl[] =
{
//Sharpness_Auto (Default)
  {0x098C,0xAB22,0,0},//MCU_ADDRESS [HG_LL_APCORR1]
  {0x0990,0x0001,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_sharpness_lv3_tbl[] =
{
//Sharpness 3
  {0x098C,0xAB22,0,0},//MCU_ADDRESS [HG_LL_APCORR1]
  {0x0990,0x0002,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_sharpness_lv4_tbl[] =
{
//Sharpness 4
  {0x098C,0xAB22,0,0},//MCU_ADDRESS [HG_LL_APCORR1]
  {0x0990,0x0003,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_sharpness_lv5_tbl[] =
{
//Sharpness 5
  {0x098C,0xAB22,0,0},//MCU_ADDRESS [HG_LL_APCORR1]
  {0x0990,0x0004,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_sharpness_lv6_tbl[] =
{
//Sharpness 6
  {0x098C,0xAB22,0,0},//MCU_ADDRESS [HG_LL_APCORR1]
  {0x0990,0x0005,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_sharpness_lv7_tbl[] =
{
//Sharpness 7
  {0x098C,0xAB22,0,0},//MCU_ADDRESS [HG_LL_APCORR1]
  {0x0990,0x0006,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_sharpness_lv8_tbl[] =
{
//Sharpness 8
  {0x098C,0xAB22,0,0},//MCU_ADDRESS [HG_LL_APCORR1]
  {0x0990,0x0007,0,0},//MCU_DATA_0
};

/******************************************************************************
Saturation Setting
******************************************************************************/
MT9V113_WREG mt9v113_saturation_lv0_tbl[] =
{
//Saturation x0.25
  {0x098C,0xAB20,0,0},//MCU_ADDRESS [HG_LL_SAT1]
  {0x0990,0x0000,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_saturation_lv1_tbl[] =
{
//Saturation x0.5
  {0x098C,0xAB20,0,0},//MCU_ADDRESS [HG_LL_SAT1]
  {0x0990,0x0015,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_saturation_lv2_tbl[] =
{
//Saturation x0.75
  {0x098C,0xAB20,0,0},//MCU_ADDRESS [HG_LL_SAT1]
  {0x0990,0x0025,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_saturation_lv3_tbl[] =
{
//Saturation x0.75
  {0x098C,0xAB20,0,0},//MCU_ADDRESS [HG_LL_SAT1]
  {0x0990,0x0035,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_saturation_default_lv4_tbl[] =
{
//Saturation x1 (Default)
  {0x098C,0xAB20,0,0},//MCU_ADDRESS [HG_LL_SAT1]
  {0x0990,0x0045,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_saturation_lv5_tbl[] =
{
//Saturation x1.25
  {0x098C,0xAB20,0,0},//MCU_ADDRESS [HG_LL_SAT1]
  {0x0990,0x0050,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_saturation_lv6_tbl[] =
{
//Saturation x1.5
  {0x098C,0xAB20,0,0},//MCU_ADDRESS [HG_LL_SAT1]
  {0x0990,0x0060,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_saturation_lv7_tbl[] =
{
//Saturation x1.25
  {0x098C,0xAB20,0,0},//MCU_ADDRESS [HG_LL_SAT1]
  {0x0990,0x0070,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_saturation_lv8_tbl[] =
{
//Saturation x1.5
  {0x098C,0xAB20,0,0},//MCU_ADDRESS [HG_LL_SAT1]
  {0x0990,0x007f,0,0},//MCU_DATA_0
};

/******************************************************************************
Brightness Setting
******************************************************************************/
MT9V113_WREG mt9v113_brightness_lv0_tbl[] =
{
//Brightness -4
};
MT9V113_WREG mt9v113_brightness_lv1_tbl[] =
{
//Brightness -3
};
MT9V113_WREG mt9v113_brightness_lv2_tbl[] =
{
//Brightness -2
};
MT9V113_WREG mt9v113_brightness_lv3_tbl[] =
{
//Brightness -1
};
MT9V113_WREG mt9v113_brightness_default_lv4_tbl[] =
{
//Brightness 0 (Default)
  {0x098C,0xA75D,0,0},//MCU_ADDRESS [MODE_Y_RGB_OFFSET_A]
  {0x0990,0x0000,0,0},//MCU_DATA_0
  {0x337E,0x0000,0,0},//Y_RGB_OFFSET
};
MT9V113_WREG mt9v113_brightness_lv5_tbl[] =
{
//Brightness +1
  {0x098C,0xA75D,0,0},//MCU_ADDRESS [MODE_Y_RGB_OFFSET_A]
  {0x0990,0x0010,0,0},//MCU_DATA_0
  {0x337E,0x1000,0,0},//Y_RGB_OFFSET
};
MT9V113_WREG mt9v113_brightness_lv6_tbl[] =
{
//Brightness +2
  {0x098C,0xA75D,0,0},//MCU_ADDRESS [MODE_Y_RGB_OFFSET_A]
  {0x0990,0x0028,0,0},//MCU_DATA_0
  {0x337E,0x2800,0,0},//Y_RGB_OFFSET
};
MT9V113_WREG mt9v113_brightness_lv7_tbl[] =
{
//Brightness +3
  {0x098C,0xA75D,0,0},//MCU_ADDRESS [MODE_Y_RGB_OFFSET_A]
  {0x0990,0x0040,0,0},//MCU_DATA_0
  {0x337E,0x4000,0,0},//Y_RGB_OFFSET
};
MT9V113_WREG mt9v113_brightness_lv8_tbl[] =
{
//Brightness +4
  {0x098C,0xA75D,0,0},//MCU_ADDRESS [MODE_Y_RGB_OFFSET_A]
  {0x0990,0x0060,0,0},//MCU_DATA_0
  {0x337E,0x6000,0,0},//Y_RGB_OFFSET
};

/******************************************************************************
Exposure Compensation Setting
******************************************************************************/
MT9V113_WREG mt9v113_exposure_compensation_lv0_tbl[]=
{
//Exposure Compensation +1.7EV
  {0x098C,0xA244,0,0},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB,0,0},//MCU_DATA_0
  {0x098C,0xA24F,0,0},//MCU_ADDRESS[AE_BASETARGET]
  {0x0990,0x0080,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_exposure_compensation_lv1_tbl[]=
{
//Exposure Compensation +1.0EV
  {0x098C,0xA244,0,0},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB,0,0},//MCU_DATA_0
  {0x098C,0xA24F,0,0},//MCU_ADDRESS[AE_BASETARGET]
  {0x0990,0x0060,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_exposure_compensation_lv2_default_tbl[]=
{
//Exposure Compensation default
  {0x098C,0xA244,0,0},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB,0,0},//MCU_DATA_0
  {0x098C,0xA24F,0,0},//MCU_ADDRESS[AE_BASETARGET]
  {0x0990,0x0044,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_exposure_compensation_lv3_tbl[]=
{
//Exposure Compensation -1.0EV
  {0x098C,0xA244,0,0},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB,0,0},//MCU_DATA_0
  {0x098C,0xA24F,0,0},//MCU_ADDRESS[AE_BASETARGET]
  {0x0990,0x002f,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_exposure_compensation_lv4_tbl[]=
{
//Exposure Compensation -1.7EV
  {0x098C,0xA244,0,0},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB,0,0},//MCU_DATA_0
  {0x098C,0xA24F,0,0},//MCU_ADDRESS[AE_BASETARGET]
  {0x0990,0x0018,0,0},//MCU_DATA_0
};

/******************************************************************************
ISO TYPE Setting
******************************************************************************/
MT9V113_WREG mt9v113_iso_type_auto[]=
{
//ISO Auto
  {0x098C,0xA20D,0,0},//MCU_ADDRESS[AE_MIN_VIRTGAIN]
  {0x0990,0x0030,0,0},//MCU_DATA_0
  {0x098C,0xA20E,0,0},//MCU_ADDRESS[AE_MAX_VIRTGAIN]
  {0x0990,0x0080,0,0},//MCU_DATA_0
  {0x098C,0x2212,0,0},//MCU_ADDRESS[AE_MAX_DGAIN_AE1]
  {0x0990,0x0080,0,0},//MCU_DATA_0
  {0x098C,0xA103,0,0},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0005,0,100},//MCU_DATA_0
  {0x098C,0xA244,0,0},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_iso_type_100[]=
{
//ISO 100
  {0x098C,0xA20D,0,0},//MCU_ADDRESS[AE_MIN_VIRTGAIN]
  {0x0990,0x001D,0,0},//MCU_DATA_0
  {0x098C,0xA20E,0,0},//MCU_ADDRESS[AE_MAX_VIRTGAIN]
  {0x0990,0x001D,0,0},//MCU_DATA_0
  {0x098C,0x2212,0,0},//MCU_ADDRESS[AE_MAX_DGAIN_AE1]
  {0x0990,0x0080,0,0},//MCU_DATA_0
  {0x098C,0xA103,0,0},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0005,0,100},//MCU_DATA_0
  {0x098C,0xA244,0,0},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_iso_type_200[]=
{
//ISO 200
  {0x098C,0xA20D,0,0},//MCU_ADDRESS[AE_MIN_VIRTGAIN]
  {0x0990,0x0047,0,0},//MCU_DATA_0
  {0x098C,0xA20E,0,0},//MCU_ADDRESS[AE_MAX_VIRTGAIN]
  {0x0990,0x0047,0,0},//MCU_DATA_0
  {0x098C,0x2212,0,0},//MCU_ADDRESS[AE_MAX_DGAIN_AE1]
  {0x0990,0x0080,0,0},//MCU_DATA_0
  {0x098C,0xA103,0,0},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0005,0,100},//MCU_DATA_0
  {0x098C,0xA244,0,0},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_iso_type_400[]=
{
//ISO 400
};
MT9V113_WREG mt9v113_iso_type_800[]=
{
//ISO 800
};
MT9V113_WREG mt9v113_iso_type_1600[]=
{
//ISO 1600
};

/******************************************************************************
Auto Expourse Weight Setting
******************************************************************************/
MT9V113_WREG mt9v113_ae_average_tbl[] =
{
//Whole Image Average
};
MT9V113_WREG mt9v113_ae_centerweight_tbl[] =
{
//Whole Image Center More weight
};

/******************************************************************************
Light Mode Setting
******************************************************************************/
MT9V113_WREG mt9v113_wb_Auto[]=
{
//CAMERA_WB_AUTO                //1
  {0x098C,0xA34A,0,0},//MCU_ADDRESS[AWB_GAIN_MIN]
  {0x0990,0x0090,0,0},//MCU_DATA_0
  {0x098C,0xA34B,0,0},//MCU_ADDRESS[AWB_GAIN_MAX]
  {0x0990,0x00FF,0,0},//MCU_DATA_0
  {0x098C,0xA34C,0,0},//MCU_ADDRESS[AWB_GAINMIN_B]
  {0x0990,0x0075,0,0},//MCU_DATA_0
  {0x098C,0xA34D,0,0},//MCU_ADDRESS[AWB_GAINMAX_B]
  {0x0990,0x00EF,0,0},//MCU_DATA_0
  {0x098C,0xA351,0,0},//MCU_ADDRESS[AWB_CCM_POSITION_MIN]
  {0x0990,0x0000,0,0},//MCU_DATA_0
  {0x098C,0xA352,0,0},//MCU_ADDRESS[AWB_CCM_POSITION_MAX]
  {0x0990,0x007F,0,0},//MCU_DATA_0
  {0x098C,0xA103,0,0},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0005,0,100},//MCU_DATA_0
  {0x098C,0xA244,0,0},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_wb_custom[]=
{
//CAMERA_WB_CUSTOM             //2
};
MT9V113_WREG mt9v113_wb_inc[]=
{
//CAMERA_WB_INCANDESCENT       //3
  {0x098C,0xA34A,0,0},//MCU_ADDRESS[AWB_GAIN_MIN]
  {0x0990,0x0090,0,0},//MCU_DATA_0
  {0x098C,0xA34B,0,0},//MCU_ADDRESS[AWB_GAIN_MAX]
  {0x0990,0x0090,0,0},//MCU_DATA_0
  {0x098C,0xA34C,0,0},//MCU_ADDRESS[AWB_GAINMIN_B]
  {0x0990,0x008B,0,0},//MCU_DATA_0
  {0x098C,0xA34D,0,0},//MCU_ADDRESS[AWB_GAINMAX_B]
  {0x0990,0x008B,0,0},//MCU_DATA_0
  {0x098C,0xA351,0,0},//MCU_ADDRESS[AWB_CCM_POSITION_MIN]
  {0x0990,0x0000,0,0},//MCU_DATA_0
  {0x098C,0xA352,0,0},//MCU_ADDRESS[AWB_CCM_POSITION_MAX]
  {0x0990,0x0000,0,0},//MCU_DATA_0
  {0x098C,0xA34E,0,0},//MCU_ADDRESS[AWB_GAIN_R]
  {0x0990,0x0090,0,0},//MCU_DATA_0
  {0x098C,0xA350,0,0},//MCU_ADDRESS[AWB_GAIN_B]
  {0x0990,0x008B,0,0},//MCU_DATA_0
  {0x098C,0xA353,0,0},//MCU_ADDRESS[AWB_CCM_POSITION]
  {0x0990,0x0000,0,0},//MCU_DATA_0
  {0x098C,0xA103,0,0},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0005,0,100},//MCU_DATA_0
  {0x098C,0xA244,0,0},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_wb_fluorescent[]=
{
//CAMERA_WB_FLUORESCENT       //4
  {0x098C,0xA34A,0,0},//MCU_ADDRESS[AWB_GAIN_MIN]
  {0x0990,0x00C3,0,0},//MCU_DATA_0
  {0x098C,0xA34B,0,0},//MCU_ADDRESS[AWB_GAIN_MAX]
  {0x0990,0x00C3,0,0},//MCU_DATA_0
  {0x098C,0xA34C,0,0},//MCU_ADDRESS[AWB_GAINMIN_B]
  {0x0990,0x0084,0,0},//MCU_DATA_0
  {0x098C,0xA34D,0,0},//MCU_ADDRESS[AWB_GAINMAX_B]
  {0x0990,0x0084,0,0},//MCU_DATA_0
  {0x098C,0xA351,0,0},//MCU_ADDRESS[AWB_CCM_POSITION_MIN]
  {0x0990,0x002F,0,0},//MCU_DATA_0
  {0x098C,0xA352,0,0},//MCU_ADDRESS[AWB_CCM_POSITION_MAX]
  {0x0990,0x002F,0,0},//MCU_DATA_0
  {0x098C,0xA34E,0,0},//MCU_ADDRESS[AWB_GAIN_R]
  {0x0990,0x00C3,0,0},//MCU_DATA_0
  {0x098C,0xA350,0,0},//MCU_ADDRESS[AWB_GAIN_B]
  {0x0990,0x0084,0,0},//MCU_DATA_0
  {0x098C,0xA353,0,0},//MCU_ADDRESS[AWB_CCM_POSITION]
  {0x0990,0x002F,0,0},//MCU_DATA_0
  {0x098C,0xA103,0,0},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0005,0,100},//MCU_DATA_0
  {0x098C,0xA244,0,0},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_wb_daylight[]=
{
//CAMERA_WB_DAYLIGHT          //5
  {0x098C,0xA34A,0,0},//MCU_ADDRESS[AWB_GAIN_MIN]
  {0x0990,0x00E0,0,0},//MCU_DATA_0
  {0x098C,0xA34B,0,0},//MCU_ADDRESS[AWB_GAIN_MAX]
  {0x0990,0x00E0,0,0},//MCU_DATA_0
  {0x098C,0xA34C,0,0},//MCU_ADDRESS[AWB_GAINMIN_B]
  {0x0990,0x007D,0,0},//MCU_DATA_0
  {0x098C,0xA34D,0,0},//MCU_ADDRESS[AWB_GAINMAX_B]
  {0x0990,0x007D,0,0},//MCU_DATA_0
  {0x098C,0xA351,0,0},//MCU_ADDRESS[AWB_CCM_POSITION_MIN]
  {0x0990,0x007F,0,0},//MCU_DATA_0
  {0x098C,0xA352,0,0},//MCU_ADDRESS[AWB_CCM_POSITION_MAX]
  {0x0990,0x007F,0,0},//MCU_DATA_0
  {0x098C,0xA34E,0,0},//MCU_ADDRESS[AWB_GAIN_R]
  {0x0990,0x00E0,0,0},//MCU_DATA_0
  {0x098C,0xA350,0,0},//MCU_ADDRESS[AWB_GAIN_B]
  {0x0990,0x007D,0,0},//MCU_DATA_0
  {0x098C,0xA353,0,0},//MCU_ADDRESS[AWB_CCM_POSITION]
  {0x0990,0x007F,0,0},//MCU_DATA_0
  {0x098C,0xA103,0,0},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0005,0,100},//MCU_DATA_0
  {0x098C,0xA244,0,0},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_wb_cloudy[]=
{
//CAMERA_WB_CLOUDY_DAYLIGHT   //6
  {0x098C,0xA34A,0,0},//MCU_ADDRESS[AWB_GAIN_MIN]
  {0x0990,0x00EA,0,0},//MCU_DATA_0
  {0x098C,0xA34B,0,0},//MCU_ADDRESS[AWB_GAIN_MAX]
  {0x0990,0x00EA,0,0},//MCU_DATA_0
  {0x098C,0xA34C,0,0},//MCU_ADDRESS[AWB_GAINMIN_B]
  {0x0990,0x0072,0,0},//MCU_DATA_0
  {0x098C,0xA34D,0,0},//MCU_ADDRESS[AWB_GAINMAX_B]
  {0x0990,0x0072,0,0},//MCU_DATA_0
  {0x098C,0xA351,0,0},//MCU_ADDRESS[AWB_CCM_POSITION_MIN]
  {0x0990,0x007F,0,0},//MCU_DATA_0
  {0x098C,0xA352,0,0},//MCU_ADDRESS[AWB_CCM_POSITION_MAX]
  {0x0990,0x007F,0,0},//MCU_DATA_0
  {0x098C,0xA34E,0,0},//MCU_ADDRESS[AWB_GAIN_R]
  {0x0990,0x00EA,0,0},//MCU_DATA_0
  {0x098C,0xA350,0,0},//MCU_ADDRESS[AWB_GAIN_B]
  {0x0990,0x0072,0,0},//MCU_DATA_0
  {0x098C,0xA353,0,0},//MCU_ADDRESS[AWB_CCM_POSITION]
  {0x0990,0x007F,0,0},//MCU_DATA_0
  {0x098C,0xA103,0,0},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0005,0,100},//MCU_DATA_0
  {0x098C,0xA244,0,0},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_wb_twilight[]=
{
//CAMERA_WB_TWILIGHT          //7
};
MT9V113_WREG mt9v113_wb_shade[]=
{
//CAMERA_WB_SHADE             //8
};

/******************************************************************************
EFFECT Setting
******************************************************************************/
MT9V113_WREG mt9v113_effect_normal_tbl[] =
{
//CAMERA_EFFECT_OFF           0
  {0x098C,0x2759,0,0},//MCU_ADDRESS[MODE_SPEC_EFFECTS_A]
  {0x0990,0x6440,0,0},//MCU_DATA_0
  {0x098C,0x275B,0,0},//MCU_ADDRESS[MODE_SPEC_EFFECTS_B]
  {0x0990,0x6440,0,0},//MCU_DATA_0
  {0x098C,0xA103,0,0},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0005,0,100},//MCU_DATA_0
  {0x098C,0xA244,0,0},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_effect_mono_tbl[] =
{
//CAMERA_EFFECT_MONO          1
  {0x098C,0x2759,0,0},//MCU_ADDRESS[MODE_SPEC_EFFECTS_A]
  {0x0990,0x6441,0,0},//MCU_DATA_0
  {0x098C,0x275B,0,0},//MCU_ADDRESS[MODE_SPEC_EFFECTS_B]
  {0x0990,0x6441,0,0},//MCU_DATA_0
  {0x098C,0xA103,0,0},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0005,0,100},//MCU_DATA_0
  {0x098C,0xA244,0,0},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_effect_negative_tbl[] =
{
//CAMERA_EFFECT_NEGATIVE      2
  {0x098C,0x2759,0,0},//MCU_ADDRESS[MODE_SPEC_EFFECTS_A]
  {0x0990,0x6443,0,0},//MCU_DATA_0
  {0x098C,0x275B,0,0},//MCU_ADDRESS[MODE_SPEC_EFFECTS_B]
  {0x0990,0x6443,0,0},//MCU_DATA_0
  {0x098C,0xA103,0,0},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0005,0,100},//MCU_DATA_0
  {0x098C,0xA244,0,0},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_effect_solarize_tbl[] =
{
//CAMERA_EFFECT_SOLARIZE      3
  {0x098C,0x2759,0,0},//MCU_ADDRESS[MODE_SPEC_EFFECTS_A]
  {0x0990,0x6444,0,0},//MCU_DATA_0
  {0x098C,0x275B,0,0},//MCU_ADDRESS[MODE_SPEC_EFFECTS_B]
  {0x0990,0x6444,0,0},//MCU_DATA_0
  {0x098C,0xA103,0,0},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0005,0,100},//MCU_DATA_0
  {0x098C,0xA244,0,0},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_effect_sepia_tbl[] =
{
//CAMERA_EFFECT_SEPIA         4
  {0x098C,0x2763,0,0},//MCU_ADDRESS[MODE_COMMONMODESETTINGS_FX_SEPIA_SETTINGS]
  {0x0990,0xB023,0,0},//MCU_DATA_0
  {0x098C,0x2759,0,0},//MCU_ADDRESS[MODE_SPEC_EFFECTS_A]
  {0x0990,0x6442,0,0},//MCU_DATA_0
  {0x098C,0x275B,0,0},//MCU_ADDRESS[MODE_SPEC_EFFECTS_B]
  {0x0990,0x6442,0,0},//MCU_DATA_0
  {0x098C,0xA103,0,0},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0005,0,100},//MCU_DATA_0
  {0x098C,0xA244,0,0},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_effect_posterize_tbl[] =
{
//CAMERA_EFFECT_POSTERIZE     5
};
MT9V113_WREG mt9v113_effect_whiteboard_tbl[] =
{
//CAMERA_EFFECT_WHITEBOARD    6
};
MT9V113_WREG mt9v113_effect_blackboard_tbl[] =
{
//CAMERA_EFFECT_BLACKBOARD    7
};
MT9V113_WREG mt9v113_effect_aqua_tbl[] =
{
//CAMERA_EFFECT_AQUA          8
};
MT9V113_WREG mt9v113_effect_bw_tbl[] =
{
//CAMERA_EFFECT_BW            10
};
MT9V113_WREG mt9v113_effect_bluish_tbl[] =
{
//CAMERA_EFFECT_BLUISH        12
  {0x098C,0x2763,0,0},//MCU_ADDRESS[MODE_COMMONMODESETTINGS_FX_SEPIA_SETTINGS]
  {0x0990,0x4890,0,0},//MCU_DATA_0
  {0x098C,0x2759,0,0},//MCU_ADDRESS[MODE_SPEC_EFFECTS_A]
  {0x0990,0x6442,0,0},//MCU_DATA_0
  {0x098C,0x275B,0,0},//MCU_ADDRESS[MODE_SPEC_EFFECTS_B]
  {0x0990,0x6442,0,0},//MCU_DATA_0
  {0x098C,0xA103,0,0},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0005,0,100},//MCU_DATA_0
  {0x098C,0xA244,0,0},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_effect_reddish_tbl[] =
{
//CAMERA_EFFECT_REDDISH       13
  {0x098C,0x2763,0,0},//MCU_ADDRESS[MODE_COMMONMODESETTINGS_FX_SEPIA_SETTINGS]
  {0x0990,0xE868,0,0},//MCU_DATA_0
  {0x098C,0x2759,0,0},//MCU_ADDRESS[MODE_SPEC_EFFECTS_A]
  {0x0990,0x6442,0,0},//MCU_DATA_0
  {0x098C,0x275B,0,0},//MCU_ADDRESS[MODE_SPEC_EFFECTS_B]
  {0x0990,0x6442,0,0},//MCU_DATA_0
  {0x098C,0xA103,0,0},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0005,0,100},//MCU_DATA_0
  {0x098C,0xA244,0,0},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_effect_greenish_tbl[] =
{
//CAMERA_EFFECT_GREENISH      14
  {0x098C,0x2763,0,0},//MCU_ADDRESS[MODE_COMMONMODESETTINGS_FX_SEPIA_SETTINGS]
  {0x0990,0xC0C0,0,0},//MCU_DATA_0
  {0x098C,0x2759,0,0},//MCU_ADDRESS[MODE_SPEC_EFFECTS_A]
  {0x0990,0x6442,0,0},//MCU_DATA_0
  {0x098C,0x275B,0,0},//MCU_ADDRESS[MODE_SPEC_EFFECTS_B]
  {0x0990,0x6442,0,0},//MCU_DATA_0
  {0x098C,0xA103,0,0},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0005,0,100},//MCU_DATA_0
  {0x098C,0xA244,0,0},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB,0,0},//MCU_DATA_0
};

/******************************************************************************
AntiBanding Setting
******************************************************************************/
MT9V113_WREG mt9v113_antibanding_auto_tbl[] =
{
//Auto-XCLK24MHz
  {0x098C,0xA404,0,0},//MCU_ADDRESS[FD_MODE]
  {0x0990,0x0050,0,0},//MCU_DATA_0
  {0x098C,0xA103,0,0},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0006,0,100},//MCU_DATA_0
  {0x098C,0xA244,0,0},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_antibanding_50z_tbl[] =
{
//Band 50Hz
  {0x098C,0xA404,0,0},//MCU_ADDRESS[FD_MODE]
  {0x0990,0x00F0,0,0},//MCU_DATA_0
  {0x098C,0xA103,0,0},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0006,0,100},//MCU_DATA_0
  {0x098C,0xA244,0,0},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB,0,0},//MCU_DATA_0
};
MT9V113_WREG mt9v113_antibanding_60z_tbl[] =
{
//Band 60Hz
  {0x098C,0xA404,0,0},//MCU_ADDRESS[FD_MODE]
  {0x0990,0x00B0,0,0},//MCU_DATA_0
  {0x098C,0xA103,0,0},//MCU_ADDRESS[SEQ_CMD]
  {0x0990,0x0006,0,100},//MCU_DATA_0
  {0x098C,0xA244,0,0},//MCU_ADDRESS[AE_DRTFEATURECTRL]
  {0x0990,0x00BB,0,0},//MCU_DATA_0
};

/******************************************************************************
Lens_shading Setting
******************************************************************************/
MT9V113_WREG mt9v113_lens_shading_on_tbl[] =
{
//Lens_shading On
  {0x3210,0x09B8,0,0},//COLOR_PIPELINE_CONTROL
};

MT9V113_WREG mt9v113_lens_shading_off_tbl[] =
{
//Lens_shading Off
  {0x3210,0x09B0,0,0},//COLOR_PIPELINE_CONTROL
};
/******************************************************************************
Auto Focus Setting
******************************************************************************/
MT9V113_WREG mt9v113_afinit_tbl[] =
{

};
#endif /* CAMSENSOR_MT9V113 */
