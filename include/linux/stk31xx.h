/*
 *
 * $Id: stk31xx.h,v 1.0 2011/02/26 11:12:08 jsgood Exp $
 *
 * Copyright (C) 2011 Patrick Chang <patrick_chang@sitronix.com.tw>
 * Copyright (C) 2012 Lex Hsieh     <lex_hsieh@sitronix.com.tw> 
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 */
#ifndef __STK_I2C_PS31XX
#define __STK_I2C_PS31XX

#ifdef CONFIG_HAS_EARLYSUSPEND
	#include <linux/earlysuspend.h>
#endif

/* Driver Settings */
#define CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD
#ifdef CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD
#define CONFIG_STK_ALS_CHANGE_THRESHOLD	1	/* The threshold to trigger ALS interrupt, unit: lux */	
#endif	/* #ifdef CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD */

//#define CONFIG_STK_SYSFS_DBG
#define CONFIG_STK_PS_ENGINEER_TUNING	/* Mount nodes on SYSFS to tune proximity sensor's parameter for debug purpose */ 

/* STK calibration */
//#define STK_MANUAL_CT_CALI
//#define STK_MANUAL_GREYCARD_CALI
#define STK_AUTO_CT_CALI_SATU
//#define STK_AUTO_CT_CALI_NO_SATU


/* Define Register Map */
#define STK_ALS_CMD_REG 0x01
#define STK_ALS_DT1_REG 0x02
#define STK_ALS_DT2_REG 0x03

#define STK_ALS_THD_H1_REG 0x04
#define STK_ALS_THD_H0_REG 0x05
#define STK_ALS_THD_L1_REG 0x06
#define STK_ALS_THD_L0_REG 0x07

#define STK_PS_STATUS_REG 0x08
#define STK_PS_CMD_REG 0x09
#define STK_PS_READING_REG 0x0A
#define STK_PS_THD_H_REG 0x0B
#define STK_PS_THD_L_REG 0x0C

#define STK_PS_SOFTWARE_RESET_REG 0x80
#define STK_PS_GC_REG 0x82

/* Define ALS CMD */

#define STK_ALS_CMD_GAIN_SHIFT  6
#define STK_ALS_CMD_IT_SHIFT    2
#define STK_ALS_CMD_SD_SHIFT    0
#define STK_ALS_CMD_INT_SHIFT   1

#define STK_ALS_CMD_GAIN(x) ((x)<<STK_ALS_CMD_GAIN_SHIFT)
#define STK_ALS_CMD_IT(x) ((x)<<STK_ALS_CMD_IT_SHIFT)
#define STK_ALS_CMD_INT(x) ((x)<<STK_ALS_CMD_INT_SHIFT)
#define STK_ALS_CMD_SD(x) ((x)<<STK_ALS_CMD_SD_SHIFT)

#define STK_ALS_CMD_GAIN_MASK 0xC0
#define STK_ALS_CMD_IT_MASK 0x0C
#define STK_ALS_CMD_INT_MASK 0x02
#define STK_ALS_CMD_SD_MASK 0x1

/* Define PS Status */
#define STK_PS_STATUS_ID_SHIFT  6

#define STK_PS_STATUS_ID(x) ((x)<<STK_PS_STATUS_ID_SHIFT)

#define STK_PS_STATUS_ID_MASK  0xC0
#define STK_PS_STATUS_PS_INT_FLAG_MASK 0x20
#define STK_PS_STATUS_ALS_INT_FLAG_MASK 0x10

#define STK_PS31xx_ID 0x00


/* Define PS CMD */

#define STK_PS_CMD_INTCTRL_SHIFT 7
#define STK_PS_CMD_SLP_SHIFT    5
#define STK_PS_CMD_DR_SHIFT     4
#define STK_PS_CMD_IT_SHIFT     2
#define STK_PS_CMD_INT_SHIFT    1
#define STK_PS_CMD_SD_SHIFT     0

#define STK_PS_CMD_INTCTRL(x) ((x)<<STK_PS_CMD_INTCTRL_SHIFT)
#define STK_PS_CMD_SLP(x) ((x)<<STK_PS_CMD_SLP_SHIFT)
#define STK_PS_CMD_DR(x) ((x)<<STK_PS_CMD_DR_SHIFT)
#define STK_PS_CMD_IT(x) ((x)<<STK_PS_CMD_IT_SHIFT)
#define STK_PS_CMD_INT(x) ((x)<<STK_PS_CMD_INT_SHIFT)
#define STK_PS_CMD_SD(x) ((x)<<STK_PS_CMD_SD_SHIFT)

#define STK_PS_CMD_INTCTRL_MASK 0x80
#define STK_PS_CMD_SLP_MASK 0x60
#define STK_PS_CMD_DR_MASK 0x10
#define STK_PS_CMD_IT_MASK 0x0C
#define STK_PS_CMD_INT_MASK 0x2
#define STK_PS_CMD_SD_MASK 0x1

/* Define PS GC */
#define STK_PS_GC_GAIN(x) ((x)<<0)
#define STK_PS_GC_GAIN_MASK 0x0f

struct stkps31xx_data {
	struct i2c_client *client;

    uint8_t ps_reading;
    uint16_t als_reading;

	uint8_t ps_gc_reg;
	uint8_t ps_cmd_reg;
	uint8_t als_cmd_reg;

	struct input_dev* als_input_dev;
	struct input_dev* ps_input_dev;
	int32_t ps_distance_last;
	int32_t ps_code_last;
	int32_t als_lux_last;
/* ALS/PS Delay would be USELESS for Interrupt Mode, but provide an unified interface for Android HAL*/
	uint32_t ps_delay;
	uint32_t als_delay;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend stk_early_suspend;
#endif
#ifdef CONFIG_STK31XX_INT_MODE
    struct work_struct work;
    int32_t irq;
#else
    uint8_t bPSThreadRunning;
	uint8_t bALSThreadRunning;
#endif //#ifdef CONFIG_STK31XX_INT_MODE
#if (defined(STK_MANUAL_CT_CALI) || defined(STK_MANUAL_GREYCARD_CALI) || defined(STK_AUTO_CT_CALI_SATU) || defined(STK_AUTO_CT_CALI_NO_SATU))
	bool ps_cali_done;
#endif
#if (defined(STK_MANUAL_CT_CALI) || defined(STK_MANUAL_GREYCARD_CALI))	
	bool first_boot;
#endif	
};

#define ALS_MIN_DELAY   100
#define PS_MIN_DELAY    10

#if (defined(STK_MANUAL_CT_CALI) || defined(STK_MANUAL_GREYCARD_CALI) || defined(STK_AUTO_CT_CALI_SATU) || defined(STK_AUTO_CT_CALI_NO_SATU))
#define STK_CALI_SAMPLE_NO		5
#define STK_THD_MAX				255	
#define STK_MAX_PS_DIFF_AUTO		20

#define STK_HIGH_THD				0
#define STK_LOW_THD				1

#define STK_DATA_MAX			0
#define STK_DATA_AVE				1
#define STK_DATA_MIN				2

const static uint16_t cali_sample_time_table[4] = {20, 40, 100, 300};
#define STK_CALI_VER0			0x48
#define STK_CALI_VER1			0x02
#define STK_CALI_FILE "/data/stk_ps_cali.conf"
#define STK_CALI_FILE_SIZE 5
#endif	/*	#if (defined(STK_MANUAL_CT_CALI) || defined(STK_MANUAL_GREYCARD_CALI) || defined(STK_AUTO_CT_CALI_SATU) || defined(STK_AUTO_CT_CALI_NO_SATU))	*/

#if (defined(STK_MANUAL_CT_CALI) || defined(STK_AUTO_CT_CALI_NO_SATU) || defined(STK_AUTO_CT_CALI_SATU))
#define STK_MAX_PS_CROSSTALK	200
#define STK_THD_H_ABOVE_CT		30
#define STK_THD_L_ABOVE_CT		15
#endif

#if defined(STK_AUTO_CT_CALI_SATU)
#define STK_THD_H_BELOW_MAX_CT		15
#define STK_THD_L_BELOW_MAX_CT		30
#endif

#ifdef STK_MANUAL_GREYCARD_CALI
#define STK_MIN_GREY_PS_DATA			50
#define STK_DIFF_GREY_N_THD_H		15
#define STK_DIFF_GREY_N_THD_L		30
#endif	/*	#ifdef STK_MANUAL_GREYCARD_CALI	*/

#if (defined(STK_MANUAL_CT_CALI) || defined(STK_MANUAL_GREYCARD_CALI))
	#define SITRONIX_PERMISSION_THREAD
#endif

struct stk31xx_platform_data{
	uint8_t als_cmd;
	uint8_t ps_cmd;
	uint8_t ps_gain;
	uint8_t ps_high_thd;
	uint8_t ps_low_thd;
	int32_t transmittance;	
	int 	int_pin;
};
	

#endif // __STK_I2C_PS31XX
