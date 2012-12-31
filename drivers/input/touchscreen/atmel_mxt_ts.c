/*
 * Atmel maXTouch Touchscreen driver
 *
 * Copyright (C) 2010 Samsung Electronics Co.Ltd
 * Author: Joonyoung Shim <jy0922.shim@samsung.com>
 * Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/i2c.h>
#include <linux/i2c/atmel_mxt_ts.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/regulator/consumer.h>
#include <linux/string.h>

#if defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
/* Early-suspend level */
#define MXT_SUSPEND_LEVEL 1
#endif

//#define ATMEL_I2C_TEST
#ifdef ATMEL_I2C_TEST
struct i2c_client *this_client;
struct delayed_work     pen_event_work;
struct workqueue_struct *ts_workqueue;
#define DELAY_TIME    1000
#endif

#define DEFAULT_X_RESOLUTION 540
#define DEFAULT_Y_RESOLUTION  960

#define PRINTK_TAG  "maxtouch->%s_%d:"
//#define __MXT_DEBUG__
#ifdef __MXT_DEBUG__
#define dbg_printk(fmt,args...)   \
 printk(KERN_DEBUG PRINTK_TAG fmt,__FUNCTION__,__LINE__,##args)
#else
#define dbg_printk(fmt,args...)
#endif

#define err_printk(fmt,args...)   \
printk(KERN_ERR PRINTK_TAG "!!Error!!"fmt,__FUNCTION__,__LINE__,##args)


#define info_printk(fmt,args...)   \
    printk(KERN_INFO PRINTK_TAG fmt,__FUNCTION__,__LINE__,##args)


/* Family ID */
#define MXT224_ID	0x80
#define MXT224E_ID	0x81
#define MXT1386_ID	0xA0

/* Version */
#define MXT_VER_20		20
#define MXT_VER_21		21
#define MXT_VER_22		22

/* I2C slave address pairs */
struct mxt_address_pair {
	int bootloader;
	int application;
};
int i2c_retries = 0;
static const struct mxt_address_pair mxt_slave_addresses[] = {
	{ 0x24, 0x4a },
	{ 0x25, 0x4b },
	{ 0x25, 0x4b },
	{ 0x26, 0x4c },
	{ 0x27, 0x4d },
	{ 0x34, 0x5a },
	{ 0x35, 0x5b },
	{ 0 },
};

#define CONFIG_VER_OFFSET 6
#define CONFIG_TP_TYPE    5
#define __UPDATE_CFG__

static const u8 gp_config_data[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00,
	0x1E, 0x0C, 0x32, 0x0A, 0x00, 0x05, 0x05, 0x00,
	0x00, 0x00, 0x01, 0x00, 0x00, 0x83, 0x00, 0x00,
	0x11, 0x0A, 0x01, 0x10, 0x32, 0x03, 0x03, 0x00,
	0x01, 0x02, 0x21, 0x05, 0x0A, 0x0A, 0x0A, 0xbf,
	0x03, 0x1b, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x0A, 0x0C, 0x83, 0x00, 0x0A,
	0x01, 0x01, 0x01, 0x01, 0x2D, 0x03, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x00,
	0x00, 0x06, 0x0B, 0x10, 0x13, 0x15, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x03, 0x00, 0xEC, 0x2C, 0x4C, 0x1D, 0xF8,
	0x2A, 0x58, 0x1B, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x03, 0x20, 0x20, 0x0A
};

static const u8 c8680_config_data[] = {
	0x00, 0x00,0x00, 0x00, 0x00, 0x04, 0x01, 0x00,
	0x1E, 0x0C, 0x32, 0x0A, 0x00, 0x05, 0x05, 0x00,
	0x00, 0x00, 0x01, 0x00, 0x00, 0x83, 0x00, 0x00,
	0x11, 0x0A, 0x01, 0x10, 0x32, 0x03, 0x03, 0x00,
	0x01, 0x02, 0x21, 0x05, 0x0A, 0x0A, 0x0A, 0xBF,
	0x03, 0x1B, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x0A, 0x0C, 0x83, 0x00, 0x0A,
	0x04, 0x01, 0x01, 0x01, 0x2D, 0x03, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x00,
	0x00, 0x06, 0x0B, 0x10, 0x13, 0x15, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x03, 0x00, 0xEC, 0x2C, 0x4C, 0x1D, 0xF8,
	0x2A, 0x58, 0x1B, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x03, 0x20, 0x20, 0x0A
};

static char * driver_ver [] = {
 "add version info"
};

static ssize_t atmel_driver_show(struct device *dev,
                       struct device_attribute *attr,
                       char *buf)
 {
        int i ;
        int ret = 0;
        for(i=0;i< ARRAY_SIZE(driver_ver);i++){
          ret += snprintf(buf+ret,PAGE_SIZE,"ver%2d:%s\n",i,driver_ver[i]);
        }
    return ret ;
 }

enum mxt_device_state { INIT, APPMODE, BOOTLOADER, ACTIVE, DEEPSLEEP};

/* Firmware */
#define MXT_FW_NAME		"maxtouch.fw"

/* Firmware frame size including frame data and CRC */
#define MXT_SINGLE_FW_MAX_FRAME_SIZE	278
#define MXT_CHIPSET_FW_MAX_FRAME_SIZE	534

/* Registers */
#define MXT_FAMILY_ID		0x00
#define MXT_VARIANT_ID		0x01
#define MXT_VERSION		0x02
#define MXT_BUILD		0x03
#define MXT_MATRIX_X_SIZE	0x04
#define MXT_MATRIX_Y_SIZE	0x05
#define MXT_OBJECT_NUM		0x06
#define MXT_OBJECT_START	0x07

#define MXT_OBJECT_SIZE		6

/* Object types */
#define MXT_DEBUG_DIAGNOSTIC_T37	37
#define MXT_SPT_MESSAGECOUNT_T44	44
#define MXT_GEN_MESSAGE_T5		5
#define MXT_GEN_COMMAND_T6		6
#define MXT_SPT_USERDATA_T38		38
#define MXT_GEN_POWER_T7		7
#define MXT_GEN_ACQUIRE_T8		8
#define MXT_TOUCH_MULTI_T9		9
#define MXT_TOUCH_KEYARRAY_T15		15
#define MXT_SPT_COMMSCONFIG_T18		18
#define MXT_SPT_GPIOPWM_T19		19
#define MXT_TOUCH_PROXIMITY_T23		23
#define MXT_SPT_SELFTEST_T25		25
#define MXT_PROCI_GRIP_T40		40
#define MXT_PROCI_TOUCHSUPPRESSION_T42	42
#define MXT_SPT_CTECONFIG_T46		46
#define MXT_PROCI_STYLUS_T47		47
#define MXT_PROCI_ADAPTIVETHRESHOLD_T55	55
#define MXT_PROCI_SLIMSENSOR_T56	56
#define MXT_PROCI_EXTRATOUCHSCREENDATA_T57	57
#define MXT_PROCI_TIMER_T61	61
#define MXT_PROCI_MAXCHARGER_T62	62
#define MXT_GEN_DATASOURCE_T53		53
#define MXT_TOUCH_PROXKEY_T52		52
#define MXT_PROCI_GRIPFACE_T20		20
#define MXT_PROCG_NOISE_T22		22
#define MXT_PROCI_ONETOUCH_T24		24
#define MXT_PROCI_TWOTOUCH_T27		27
#define MXT_PROCI_PALM_T41		41
#define MXT_PROCI_SHIELDLESS_T56	56
#define MXT_PROCG_NOISESUPPRESSION_T48	48
#define MXT_SPT_CTECONFIG_T28		28
#define MXT_SPT_DIGITIZER_T43		43

/* MXT_GEN_COMMAND_T6 field */
#define MXT_COMMAND_RESET	0
#define MXT_COMMAND_BACKUPNV	1
#define MXT_COMMAND_CALIBRATE	2
#define MXT_COMMAND_REPORTALL	3
#define MXT_COMMAND_DIAGNOSTIC	5

/* MXT_GEN_COMMAND_T6 message field */
#define MXT_COMMAND_MSG_CALIBRATE	(1 << 4)

/* MXT_GEN_POWER_T7 field */
#define MXT_POWER_IDLEACQINT	0
#define MXT_POWER_ACTVACQINT	1
#define MXT_POWER_ACTV2IDLETO	2

/* MXT_GEN_ACQUIRE_T8 field */
#define MXT_ACQUIRE_CHRGTIME	0
#define MXT_ACQUIRE_TCHDRIFT	2
#define MXT_ACQUIRE_DRIFTST	3
#define MXT_ACQUIRE_TCHAUTOCAL	4
#define MXT_ACQUIRE_SYNC	5
#define MXT_ACQUIRE_ATCHCALST	6
#define MXT_ACQUIRE_ATCHCALSTHR	7

/* MXT_TOUCH_MULT_T9 field */
#define MXT_TOUCH_CTRL		0
#define MXT_TOUCH_XORIGIN	1
#define MXT_TOUCH_YORIGIN	2
#define MXT_TOUCH_XSIZE		3
#define MXT_TOUCH_YSIZE		4
#define MXT_TOUCH_BLEN		6
#define MXT_TOUCH_TCHTHR	7
#define MXT_TOUCH_TCHDI		8
#define MXT_TOUCH_ORIENT	9
#define MXT_TOUCH_MOVHYSTI	11
#define MXT_TOUCH_MOVHYSTN	12
#define MXT_TOUCH_NUMTOUCH	14
#define MXT_TOUCH_MRGHYST	15
#define MXT_TOUCH_MRGTHR	16
#define MXT_TOUCH_AMPHYST	17
#define MXT_TOUCH_XRANGE_LSB	18
#define MXT_TOUCH_XRANGE_MSB	19
#define MXT_TOUCH_YRANGE_LSB	20
#define MXT_TOUCH_YRANGE_MSB	21
#define MXT_TOUCH_XLOCLIP	22
#define MXT_TOUCH_XHICLIP	23
#define MXT_TOUCH_YLOCLIP	24
#define MXT_TOUCH_YHICLIP	25
#define MXT_TOUCH_XEDGECTRL	26
#define MXT_TOUCH_XEDGEDIST	27
#define MXT_TOUCH_YEDGECTRL	28
#define MXT_TOUCH_YEDGEDIST	29
#define MXT_TOUCH_JUMPLIMIT	30
#define MXT_TOUCH_USER_DATA_T38 38
/* MXT_PROCI_GRIPFACE_T20 field */
#define MXT_GRIPFACE_CTRL	0
#define MXT_GRIPFACE_XLOGRIP	1
#define MXT_GRIPFACE_XHIGRIP	2
#define MXT_GRIPFACE_YLOGRIP	3
#define MXT_GRIPFACE_YHIGRIP	4
#define MXT_GRIPFACE_MAXTCHS	5
#define MXT_GRIPFACE_SZTHR1	7
#define MXT_GRIPFACE_SZTHR2	8
#define MXT_GRIPFACE_SHPTHR1	9
#define MXT_GRIPFACE_SHPTHR2	10
#define MXT_GRIPFACE_SUPEXTTO	11

/* MXT_PROCI_NOISE field */
#define MXT_NOISE_CTRL		0
#define MXT_NOISE_OUTFLEN	1
#define MXT_NOISE_GCAFUL_LSB	3
#define MXT_NOISE_GCAFUL_MSB	4
#define MXT_NOISE_GCAFLL_LSB	5
#define MXT_NOISE_GCAFLL_MSB	6
#define MXT_NOISE_ACTVGCAFVALID	7
#define MXT_NOISE_NOISETHR	8
#define MXT_NOISE_FREQHOPSCALE	10
#define MXT_NOISE_FREQ0		11
#define MXT_NOISE_FREQ1		12
#define MXT_NOISE_FREQ2		13
#define MXT_NOISE_FREQ3		14
#define MXT_NOISE_FREQ4		15
#define MXT_NOISE_IDLEGCAFVALID	16

/* MXT_SPT_COMMSCONFIG_T18 */
#define MXT_COMMS_CTRL		0
#define MXT_COMMS_CMD		1

/* MXT_SPT_CTECONFIG_T28 field */
#define MXT_CTE_CTRL		0
#define MXT_CTE_CMD		1
#define MXT_CTE_MODE		2
#define MXT_CTE_IDLEGCAFDEPTH	3
#define MXT_CTE_ACTVGCAFDEPTH	4
#define MXT_CTE_VOLTAGE		5

/* MXT_PROCI_TOUCHSUPPRESSION_T42 message field */
#define MXT_TCHSUP_MSG_ACTIVE	(1 << 0)

#define MXT_VOLTAGE_DEFAULT	2700000
#define MXT_VOLTAGE_STEP	10000

/* Analog voltage @2.7 V */
#define MXT_VTG_MIN_UV		2700000
#define MXT_VTG_MAX_UV		3300000
#define MXT_ACTIVE_LOAD_UA	15000
#define MXT_LPM_LOAD_UA		10
/* Digital voltage @1.8 V */
#define MXT_VTG_DIG_MIN_UV	1800000
#define MXT_VTG_DIG_MAX_UV	1800000
#define MXT_ACTIVE_LOAD_DIG_UA	10000
#define MXT_LPM_LOAD_DIG_UA	10

#define MXT_I2C_VTG_MIN_UV	1800000
#define MXT_I2C_VTG_MAX_UV	1800000
#define MXT_I2C_LOAD_UA		10000
#define MXT_I2C_LPM_LOAD_UA	10

/* Define for MXT_GEN_COMMAND_T6 */
#define MXT_BOOT_VALUE		0xa5
#define MXT_BACKUP_VALUE	0x55
#define MXT_BACKUP_TIME		25	/* msec */
#define MXT224_RESET_TIME	65	/* msec */
#define MXT224E_RESET_TIME	150	/* msec */
#define MXT1386_RESET_TIME	250	/* msec */
#define MXT_RESET_TIME		250	/* msec */
#define MXT_RESET_NOCHGREAD	400	/* msec */

#define MXT_FWRESET_TIME	1000	/* msec */

#define MXT_WAKE_TIME		25

/* Command to unlock bootloader */
#define MXT_UNLOCK_CMD_MSB	0xaa
#define MXT_UNLOCK_CMD_LSB	0xdc

/* Bootloader mode status */
#define MXT_WAITING_BOOTLOAD_CMD	0xc0	/* valid 7 6 bit only */
#define MXT_WAITING_FRAME_DATA	0x80	/* valid 7 6 bit only */
#define MXT_FRAME_CRC_CHECK	0x02
#define MXT_FRAME_CRC_FAIL	0x03
#define MXT_FRAME_CRC_PASS	0x04
#define MXT_APP_CRC_FAIL	0x40	/* valid 7 8 bit only */
#define MXT_BOOT_STATUS_MASK	0x3f
#define MXT_BOOT_EXTENDED_ID	(1 << 5)
#define MXT_BOOT_ID_MASK	0x1f

/* Touch status */
#define MXT_SUPPRESS		(1 << 1)
#define MXT_AMP			(1 << 2)
#define MXT_VECTOR		(1 << 3)
#define MXT_MOVE		(1 << 4)
#define MXT_RELEASE		(1 << 5)
#define MXT_PRESS		(1 << 6)
#define MXT_DETECT		(1 << 7)

/* Touch orient bits */
#define MXT_XY_SWITCH		(1 << 0)
#define MXT_X_INVERT		(1 << 1)
#define MXT_Y_INVERT		(1 << 2)

/* Touchscreen absolute values */
#define MXT_MAX_AREA		0xff
#define MXT_MAX_PRESSURE    0xff
#define MXT_MAX_FINGER		10

#define T7_DATA_SIZE		3
#define MXT_MAX_RW_TRIES	3
#define MXT_BLOCK_SIZE		256
#define MXT_CFG_VERSION_LEN	3
#define MXT_CFG_VERSION_EQUAL	0
#define MXT_CFG_VERSION_LESS	1
#define MXT_CFG_VERSION_GREATER	2

#define MXT_DEBUGFS_DIR		"atmel_mxt_ts"
#define MXT_DEBUGFS_FILE	"object"

struct mxt_info {
	u8 family_id;
	u8 variant_id;
	u8 version;
	u8 build;
	u8 matrix_xsize;
	u8 matrix_ysize;
	u8 object_num;
};

struct mxt_object {
	u8 type;
	u16 start_address;
	u8 size;
	u8 instances;
	u8 num_report_ids;

	/* to map object and message */
	u8 max_reportid;
    u8 min_reportid;
};

struct mxt_message {
	u8 reportid;
	u8 message[7];
	u8 checksum;
};

struct mxt_finger {
	int status;
	int x;
	int y;
	int area;
	int pressure;
};

/* Each client has this additional data */
struct mxt_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
    struct hrtimer timer ;
    int tp_type ;
	const struct mxt_platform_data *pdata;
	const struct mxt_config_info *config_info;
	struct workqueue_struct *atmel_wq;
	struct work_struct work;
	enum mxt_device_state state;
	struct mxt_object *object_table;
    u16 mem_size;
    struct device dev ;
    u8 status  ;//active or deepsleep
	struct mxt_info info;
	struct mxt_finger finger[MXT_MAX_FINGER];
	unsigned int irq;
	struct regulator *vcc_ana;
	struct regulator *vcc_dig;
	struct regulator *vcc_i2c;
#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif
	u8 t6_reportid;
	u8 t7_data[T7_DATA_SIZE];
	u16 t7_start_addr;
	u32 keyarray_old;
	u32 keyarray_new;
	u8 t9_max_reportid;
	u8 t9_min_reportid;
	u8 t15_max_reportid;
	u8 t15_min_reportid;
	u8 t42_reportid;
       u8 t56_reportid;
	u8 t62_reportid;
	u8 cfg_version[MXT_CFG_VERSION_LEN];
	int cfg_version_idx;
	int t38_start_addr;
	bool update_cfg;
	const char *fw_name;
    u8 max_reportid;
};

static struct dentry *debug_base;

#ifdef __UPDATE_CFG__
static bool mxt_object_writable(unsigned int type)
{
	switch (type) {
	//case MXT_DEBUG_DIAGNOSTIC_T37:
       //case MXT_SPT_MESSAGECOUNT_T44:
       //case MXT_GEN_MESSAGE_T5:
       //case MXT_GEN_COMMAND_T6:
       case MXT_SPT_USERDATA_T38:
       case MXT_GEN_POWER_T7:
       case MXT_GEN_ACQUIRE_T8	:
       case MXT_TOUCH_MULTI_T9:
       case MXT_TOUCH_KEYARRAY_T15:
       case MXT_SPT_COMMSCONFIG_T18:
       case MXT_SPT_GPIOPWM_T19:
       case MXT_TOUCH_PROXIMITY_T23	:
       case MXT_PROCI_ONETOUCH_T24:
       case MXT_PROCI_TWOTOUCH_T27:
       case MXT_SPT_SELFTEST_T25:
       case MXT_PROCI_GRIPFACE_T20:
       case MXT_PROCG_NOISE_T22:
       case MXT_SPT_CTECONFIG_T46:
		return true;
	default:
		return false;
	}
}
#endif

static void mxt_dump_message(struct device *dev,
				  struct mxt_message *message)
{
    #if 0
    printk("id:0x%x", message->reportid);
	printk("    msg1:0x%x", message->message[0]);
	printk("    msg2:0x%x", message->message[1]);
	printk("    msg3:0x%x", message->message[2]);
	printk("    msg4:0x%x", message->message[3]);
	printk("    msg5:0x%x", message->message[4]);
 	printk("    msg6:0x%x", message->message[5]);
    printk("    msg7:0x%x", message->message[6]);
	printk("    checksum:0x%x\n", message->checksum);
    #endif
}

static int mxt_switch_to_bootloader_address(struct mxt_data *data)
{
	int i;
	struct i2c_client *client = data->client;

	if (data->state == BOOTLOADER) {
		dev_err(&client->dev, "Already in BOOTLOADER state\n");
		return -EINVAL;
	}

	for (i = 0; mxt_slave_addresses[i].application != 0;  i++) {
		if (mxt_slave_addresses[i].application == client->addr) {
			dev_info(&client->dev, "Changing to bootloader address: "
				"%02x -> %02x",
				client->addr,
				mxt_slave_addresses[i].bootloader);

			client->addr = mxt_slave_addresses[i].bootloader;
			data->state = BOOTLOADER;
			return 0;
		}
	}

	dev_err(&client->dev, "Address 0x%02x not found in address table",
								client->addr);
	return -EINVAL;
}

static int mxt_switch_to_appmode_address(struct mxt_data *data)
{
	int i;
	struct i2c_client *client = data->client;

	if (data->state == APPMODE) {
		dev_err(&client->dev, "Already in APPMODE state\n");
		return -EINVAL;
	}

	for (i = 0; mxt_slave_addresses[i].application != 0;  i++) {
		if (mxt_slave_addresses[i].bootloader == client->addr) {
			dev_info(&client->dev,
				"Changing to application mode address: "
							"0x%02x -> 0x%02x",
				client->addr,
				mxt_slave_addresses[i].application);

			client->addr = mxt_slave_addresses[i].application;
			data->state = APPMODE;
			return 0;
		}
	}

	dev_err(&client->dev, "Address 0x%02x not found in address table",
								client->addr);
	return -EINVAL;
}

static int mxt_get_bootloader_version(struct i2c_client *client, u8 val)
{
	u8 buf[3];

	if (val | MXT_BOOT_EXTENDED_ID)	{
		dev_dbg(&client->dev,
				"Retrieving extended mode ID information");

		if (i2c_master_recv(client, &buf[0], 3) != 3) {
			dev_err(&client->dev, "%s: i2c recv failed\n",
								__func__);
			return -EIO;
		}

		dev_info(&client->dev, "Bootloader ID:%d Version:%d",
			buf[1], buf[2]);

		return buf[0];
	} else {
		dev_info(&client->dev, "Bootloader ID:%d",
			val & MXT_BOOT_ID_MASK);

		return val;
	}
}

static int mxt_get_bootloader_id(struct i2c_client *client)
{
	u8 val;
	u8 buf[3];

	if (i2c_master_recv(client, &val, 1) != 1) {
		dev_err(&client->dev, "%s: i2c recv failed\n", __func__);
		return -EIO;
	}

	if (val | MXT_BOOT_EXTENDED_ID)	{
		if (i2c_master_recv(client, &buf[0], 3) != 3) {
			dev_err(&client->dev, "%s: i2c recv failed\n",
								__func__);
			return -EIO;
		}
		return buf[1];
	} else {
		dev_info(&client->dev, "Bootloader ID:%d",
			val & MXT_BOOT_ID_MASK);

		return val & MXT_BOOT_ID_MASK;
	}
}

static int mxt_check_bootloader(struct i2c_client *client,
				unsigned int state)
{
	u8 val;

recheck:
	if (i2c_master_recv(client, &val, 1) != 1) {
		dev_err(&client->dev, "%s: i2c recv failed\n", __func__);
		return -EIO;
	}

	switch (state) {
	case MXT_WAITING_BOOTLOAD_CMD:
		val = mxt_get_bootloader_version(client, val);
		val &= ~MXT_BOOT_STATUS_MASK;
		break;
	case MXT_WAITING_FRAME_DATA:
	case MXT_APP_CRC_FAIL:
		val &= ~MXT_BOOT_STATUS_MASK;
		break;
	case MXT_FRAME_CRC_PASS:
		if (val == MXT_FRAME_CRC_CHECK)
			goto recheck;
		if (val == MXT_FRAME_CRC_FAIL) {
			dev_err(&client->dev, "Bootloader CRC fail\n");
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	if (val != state) {
		dev_err(&client->dev, "Invalid bootloader mode state %X\n",
			val);
		return -EINVAL;
	}

	return 0;
}

static int mxt_unlock_bootloader(struct i2c_client *client)
{
	u8 buf[2];

	buf[0] = MXT_UNLOCK_CMD_LSB;
	buf[1] = MXT_UNLOCK_CMD_MSB;

	if (i2c_master_send(client, buf, 2) != 2) {
		dev_err(&client->dev, "%s: i2c send failed\n", __func__);
		return -EIO;
	}

	return 0;
}

static int mxt_fw_write(struct i2c_client *client,
			const u8 *data, unsigned int frame_size)
{
	if (i2c_master_send(client, data, frame_size) != frame_size) {
		dev_err(&client->dev, "%s: i2c send failed\n", __func__);
		return -EIO;
	}

	return 0;
}

static int __mxt_read_reg(struct i2c_client *client,
			       u16 reg, u16 len, void *val)
{
	struct i2c_msg xfer[2];
	u8 buf[2];
	int i = 0;

	buf[0] = reg & 0xff;
	buf[1] = (reg >> 8) & 0xff;

	/* Write register */
	xfer[0].addr = client->addr;
	xfer[0].flags = 0;
	xfer[0].len = 2;
	xfer[0].buf = buf;

	/* Read data */
	xfer[1].addr = client->addr;
	xfer[1].flags = I2C_M_RD;
	xfer[1].len = len;
	xfer[1].buf = val;

	do {
		if (i2c_transfer(client->adapter, xfer, 2) == 2)
			return 0;
		msleep(MXT_WAKE_TIME);
	} while (++i < MXT_MAX_RW_TRIES);

	i2c_retries++;
	if(i2c_retries<2){
		client->addr = 0x4a;
		return __mxt_read_reg(client, reg, len, val);
	}else{
		dev_err(&client->dev, ": %s: i2c transfer failed\n", __func__);
	}
		
	
	return -EIO;
}

static int mxt_read_reg(struct i2c_client *client, u16 reg, u8 *val)
{
	return __mxt_read_reg(client, reg, 1, val);
}

static int __mxt_write_reg(struct i2c_client *client,
		    u16 addr, u16 length, u8 *value)
{
	u8 buf[MXT_BLOCK_SIZE + 2];
	int i, tries = 0;

	if (length > MXT_BLOCK_SIZE)
		return -EINVAL;

	buf[0] = addr & 0xff;
	buf[1] = (addr >> 8) & 0xff;
	for (i = 0; i < length; i++)
		buf[i + 2] = *value++;

	do {
		if (i2c_master_send(client, buf, length + 2) == (length + 2))
			return 0;
		msleep(MXT_WAKE_TIME);
	} while (++tries < MXT_MAX_RW_TRIES);

	dev_err(&client->dev, "%s: i2c send failed\n", __func__);
	return -EIO;
}

static int mxt_write_reg(struct i2c_client *client, u16 reg, u8 val)
{
	return __mxt_write_reg(client, reg, 1, &val);
}

static int mxt_read_object_table(struct i2c_client *client,
				      u16 reg, u8 *object_buf)
{
	return __mxt_read_reg(client, reg, MXT_OBJECT_SIZE,
				   object_buf);
}

static struct mxt_object *
mxt_get_object(struct mxt_data *data, u8 type)
{
	struct mxt_object *object;
	int i;

	for (i = 0; i < data->info.object_num; i++) {
		object = data->object_table + i;
		if (object->type == type)
			return object;
	}

	dev_err(&data->client->dev, "Invalid object type\n");
	return NULL;
}

static int mxt_read_message(struct mxt_data *data,
				 struct mxt_message *message)
{
	struct mxt_object *object;
	u16 reg;

	object = mxt_get_object(data, MXT_GEN_MESSAGE_T5);
	if (!object)
		return -EINVAL;

	reg = object->start_address;
	return __mxt_read_reg(data->client, reg,
			sizeof(struct mxt_message), message);
}

/* 0 : success */
static int mxt_read_object(struct mxt_data *data,
				u8 type, u8 offset, u8 *val)
{
	struct mxt_object *object;
	u16 reg;

	object = mxt_get_object(data, type);
	if (!object)
		return -EINVAL;

	reg = object->start_address;
	return __mxt_read_reg(data->client, reg + offset, 1, val);
}

//return 0 success
static int mxt_write_object(struct mxt_data *data,
				 u8 type, u8 offset, u8 val)
{
	struct mxt_object *object;
	u16 reg;

	object = mxt_get_object(data, type);
	if (!object)
		return -EINVAL;

	reg = object->start_address;
	return mxt_write_reg(data->client, reg + offset, val);
}

static void mxt_input_report(struct mxt_data *data, int single_id)
{
	struct mxt_finger *finger = data->finger;
	struct input_dev *input_dev = data->input_dev;
	//int status = finger[single_id].status;
	int finger_num = 0;
	int id;

	for (id = 0; id < MXT_MAX_FINGER; id++) {
		if (!finger[id].status)
			continue;

		input_mt_slot(input_dev, id);
		/* Firmware reports min/max values when the touch is
		 * outside screen area. Send a release event in
		 * such cases to avoid unwanted touches.
		 */
		if (finger[id].x < data->pdata->panel_minx ||
				finger[id].x > data->pdata->panel_maxx ||
				finger[id].y < data->pdata->panel_miny ||
				finger[id].y > data->pdata->panel_maxy) {
            dbg_printk("x,y is out of ranger x[%d],y[%d]\n",
               finger[id].x,finger[id].y);
            finger[id].status = MXT_RELEASE;
		}

		input_mt_report_slot_state(input_dev, MT_TOOL_FINGER,
				finger[id].status != MXT_RELEASE);

		if (finger[id].status != MXT_RELEASE) {
            //dbg_printk("report key id[%d]\n",id);
			finger_num++;
			input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR,
					finger[id].area);
			input_report_abs(input_dev, ABS_MT_POSITION_X,
					finger[id].x);
			input_report_abs(input_dev, ABS_MT_POSITION_Y,
					finger[id].y);
			input_report_abs(input_dev, ABS_MT_PRESSURE,
					 finger[id].pressure);
		} else {
		    //dbg_printk("staus set to 0\n");
			finger[id].status = 0;
		}
	}


    //dbg_printk(" all key report\n");
    input_report_key(input_dev, BTN_TOUCH, finger_num > 0);
	input_sync(input_dev);
}

static void mxt_cleanup_finger(struct mxt_data *data)
{
	struct mxt_finger *finger = data->finger;
	int id = 0;

	for (id = 0; id < MXT_MAX_FINGER; id++) {
		if ((finger[id].status != 0) && (finger[id].status != MXT_RELEASE)) {
			finger[id].status = MXT_RELEASE;
		}
	}
    mxt_input_report(data, id);
}

static void mxt_handle_calibration(struct mxt_data *data,
				      struct mxt_message *message)
{
	struct device *dev = &data->client->dev;
	u8 status = message->message[0];

	dev_warn(dev, "%s, T6 status: 0x%x.\n", __func__, status);
	if (status & MXT_COMMAND_MSG_CALIBRATE)
		mxt_cleanup_finger(data);
}

static void mxt_handle_touchsuppression(struct mxt_data *data,
				      struct mxt_message *message)
{
	struct device *dev = &data->client->dev;
	u8 status = message->message[0];

	dev_warn(dev, "%s, T42 status: 0x%x.\n", __func__, status);
	if (status & MXT_TCHSUP_MSG_ACTIVE)
		mxt_cleanup_finger(data);
}

static void mxt_handle_noisesuppression(struct mxt_data *data,
				      struct mxt_message *message)
{
	struct device *dev = &data->client->dev;

	dev_warn(dev, "T62 status    : 0x%x\n", message->message[0]);
	dev_warn(dev, "T62 ADCs per x: 0x%x\n", message->message[1]);
	dev_warn(dev, "T62 cur freq  : 0x%x\n", message->message[2]);
	dev_warn(dev, "T62 state     : 0x%x\n", message->message[3]);
}

static void mxt_input_touchevent(struct mxt_data *data,
				      struct mxt_message *message, int id)
{
	struct mxt_finger *finger = data->finger;
//	struct device *dev = &data->client->dev;
	u8 status = message->message[0];
   // int tmp ;
	int x;
	int y;
	int area;
	int pressure;

    dbg_printk("finger id[%d]: status :",id) ;
    if(status &MXT_RELEASE)
        dbg_printk("RELEASE\n");
    else if(status &MXT_MOVE)
        dbg_printk("MOVE\n");
    else if(status &MXT_DETECT)
        dbg_printk("DETECH\n") ;
    else if(status &MXT_PRESS)
        dbg_printk("PRESS\n");
    else if(status &MXT_SUPPRESS)
        dbg_printk("SUPPRESS\n") ;
    else if(status &MXT_AMP)
        dbg_printk("AMP\n");
    else if(status &MXT_VECTOR)
        dbg_printk("VECTOR\n");
    else {
        dbg_printk("status is undefine\n");
    }
	/* Check the touch is present on the screen */
	if (status & MXT_RELEASE) {

		finger[id].status = MXT_RELEASE;
		//mxt_input_report(data, id);
		return;
	}

	x = (message->message[1] << 4) | ((message->message[3] >> 4) & 0xf);
	y = (message->message[2] << 4) | ((message->message[3] & 0xf));

    if (data->pdata->panel_maxx < 1024)
		x = x >> 2;
	if (data->pdata->panel_maxy < 1024)
		y = y >> 2;

	area = message->message[4];
	pressure = message->message[5];

    dbg_printk("x[%d],y[%d]\n",x,y);

	finger[id].status = status & MXT_MOVE ?
				MXT_MOVE : MXT_PRESS;
	finger[id].x = x;
	finger[id].y = y;
	finger[id].area = area;
	finger[id].pressure = pressure;


}

#ifndef __KERNEL_KEY__
#define _m_x  55
#define _y_p  1008
#define _ho_x 200
#define _b_x  360
#define _s_x  500

static inline int is_key(struct mxt_data * data,int code)
{
    switch(code){
        case 1 :
        {
            if(data->tp_type==1)
                return KEY_HOME ;
            if(data->tp_type==4)
                return KEY_MENU ;
        }
        case 2 :
            return KEY_HOME ;
        case 4 :
            return KEY_BACK ;
        case 8:
            return KEY_SEARCH ;
            break ;
        default :
            return 0 ;
    }
    return 0 ;
}

static int atmel_report_virtual_key(struct mxt_data * data,int code,int value)
{
    int x ;

    int status ;
    int key ;
    key = is_key(data,code) ;
    if(!key)return 0 ;

  if(!value){
        info_printk("virtual key release\n");
        status = MXT_RELEASE ;
    } else{
        status = MXT_DETECT ;
     }

    switch(key)
    {
        case KEY_MENU:
            x = _m_x ;
            dbg_printk("pressure menu button\n");
            break ;
        case KEY_HOME :
            dbg_printk("pressure home button\n");
            x = _ho_x ;
            break ;
        case KEY_BACK :
            x = _b_x  ;
            dbg_printk("pressure back button\n");
         break ;
         case KEY_SEARCH:
           dbg_printk("pressure search button\n");
            x = _s_x ;
            break ;
         default:
            info_printk("no key value to report \n");
            return 0 ;
    }

    dbg_printk("report key [%d]\n",key);
    input_mt_slot(data->input_dev,0) ;
    input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER,
                    status != MXT_RELEASE);
    if(status == MXT_DETECT){
     input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR,MXT_MAX_AREA/2);
     input_report_abs(data->input_dev, ABS_MT_POSITION_X,x);
     input_report_abs(data->input_dev, ABS_MT_POSITION_Y,_y_p);
     input_report_abs(data->input_dev, ABS_MT_PRESSURE,MXT_MAX_PRESSURE/2);
    }

    input_report_key(data->input_dev,BTN_TOUCH,status!=MXT_RELEASE);
    //input_mt_sync(data->input_dev) ;
    input_sync(data->input_dev) ;
    return 0 ;
}
#endif

static void mxt_handle_key_array(struct mxt_data *data,
				struct mxt_message *message)
{
	u32 keys_changed;

    info_printk("new array[%d]\n",data->keyarray_new);
	data->keyarray_new = message->message[1] |
				(message->message[2] << 8) |
				(message->message[3] << 16);


	keys_changed = data->keyarray_old ^ data->keyarray_new;

	if (!keys_changed) {
		info_printk(": no keys changed\n");
		return;
	}

    #ifdef __KERNEL_KEY__
	input_report_key(data->input_dev, is_key(data,keys_changed),data->keyarray_new&(keys_changed),);
	input_sync(data->input_dev);
    #else
    atmel_report_virtual_key(data,keys_changed,data->keyarray_new & (keys_changed));
    #endif


	data->keyarray_old = data->keyarray_new;
}

static irqreturn_t mxt_irq_handler(int irq, void *dev_id)
{
	struct mxt_data *data = dev_id;
    //info_printk("irq handler\n");
	disable_irq_nosync(data->irq);
	queue_work(data->atmel_wq, &data->work);
	return IRQ_HANDLED;
}

static void mxt_reset_delay(struct mxt_data *data)
{
	struct mxt_info *info = &data->info;

	switch (info->family_id) {
	case MXT224_ID:
		msleep(MXT224_RESET_TIME);
		break;
	case MXT224E_ID:
		msleep(MXT224E_RESET_TIME);
		break;
	case MXT1386_ID:
		msleep(MXT1386_RESET_TIME);
		break;
	default:
		msleep(MXT_RESET_TIME);
	}
}

#ifdef __UPDATE_CFG__
static int mxt_backup_nv(struct mxt_data *data)
{
    int error;
	u8 command_register;
	int timeout_counter = 0;

	/* Backup to memory */
	mxt_write_object(data, MXT_GEN_COMMAND_T6,
			MXT_COMMAND_BACKUPNV,
			MXT_BACKUP_VALUE);
	msleep(MXT_BACKUP_TIME);

	do {
		error = mxt_read_object(data, MXT_GEN_COMMAND_T6,
					MXT_COMMAND_BACKUPNV,
					&command_register);
		if (error)
			return error;

		usleep_range(1000, 2000);
            //when process backup command the bit is clear
	} while ((command_register != 0) && (++timeout_counter <= 100));

	if (timeout_counter > 100) {
		dev_err(&data->client->dev, "No response after backup!\n");
		return -EIO;
	}

	/* Soft reset */
	mxt_write_object(data, MXT_GEN_COMMAND_T6, MXT_COMMAND_RESET, 1);

	mxt_reset_delay(data);

	return 0;
}

/*
    check config file
*/
static int mxt_check_reg_init(struct mxt_data *data)
{
	struct mxt_object *object;
    u8 cfg_ver ,chip_ver ;
    int i, j, config_offset;
    int index = 0,size;
    u8 type = 0 ;
    const u8 * cfg_data ;
    int ret = 0 ;

    ret = mxt_read_object(data,MXT_TOUCH_USER_DATA_T38,CONFIG_TP_TYPE,&type) ;
    if(ret){
        err_printk("read tp type error\n");
        return -EIO ;
    }
    if( type==1 || type==0){
        cfg_data = gp_config_data;
        size     = sizeof(gp_config_data) ;
        data->tp_type = 1 ;
        info_printk("select one icon tp config data\n");
    }else if(type == 4 ){
        cfg_data = c8680_config_data ;
        size     = sizeof(c8680_config_data);
        data->tp_type = 4 ;
        info_printk("select four icon tp config data\n");
    }else {
        err_printk("not tp config data \n");
        data->tp_type = 0 ;
        return -EIO ;
    }

    info_printk("select %d icon tp config data\n",type);
    cfg_ver = cfg_data[CONFIG_VER_OFFSET] ;
    mxt_read_object(data,MXT_TOUCH_USER_DATA_T38,CONFIG_VER_OFFSET,&chip_ver) ;
    if(cfg_ver <= chip_ver){
        info_printk("No need to upgarde cfg cfg_ver[%d],chip_ver[%d]\n",
            cfg_ver,chip_ver);
        return 0 ;
    }else {
        info_printk("Need to upgarde cfg cfg_ver[%d],chip_ver[%d]\n",
            cfg_ver,chip_ver);
    }

	for (i = 0; i < data->info.object_num; i++) {
		object = data->object_table + i;

		if (!mxt_object_writable(object->type))
			continue;

     dbg_printk(": object->type = %d, object->size = %d\n", object->type, object->size);

		for (j = 0; j < object->size; j++) {
			config_offset = index + j;
      dbg_printk(": config_offset = %d, data= %02x\n",
                   config_offset, cfg_data[config_offset]);
			if (config_offset > size) {
 				info_printk(": Not enough config data!\n");
 				return -EINVAL;

			}
			mxt_write_object(data, object->type, j,
					 cfg_data[config_offset]);
		}
		index += object->size;
	}

        //if tp type not confirm,we dont backup config file
	if (type) {
           info_printk("backup config file to ic \n");
	   mxt_backup_nv(data) ;
	}
    return 0 ;
}
#endif

static int mxt_make_highchg(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	struct mxt_message message;
	int count = 10;
	int error;

	/* Read dummy message to make high CHG pin */
	do {
		error = mxt_read_message(data, &message);
		if (error)
			return error;
	} while (message.reportid != 0xff && --count);

	if (!count) {
		dev_err(dev, "CHG pin isn't cleared\n");
		return -EBUSY;
	}
    mxt_dump_message(dev,&message) ;
	return 0;
}

#ifdef ATMEL_I2C_TEST
static void atmel_i2c_test(struct work_struct *work)
{
    int error;
    u8 val = 0;

    printk(": this_client->addr = 0x%x\n", this_client->addr);
    //this_client->addr = 0x4a;
    error = mxt_read_reg(this_client, MXT_FAMILY_ID, &val);
    if (error){
        printk(": MXT_FAMILY_ID = %d\n", val);
        printk(": Read MXT_FAMILY_ID failed\n");
    }
    else
        printk(": MXT_FAMILY_ID = %d\n", val);
    queue_delayed_work(ts_workqueue, &pen_event_work, msecs_to_jiffies(DELAY_TIME));
}
#endif

static int mxt_get_info(struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	struct mxt_info *info = &data->info;
	int error;
	u8 val;

	error = mxt_read_reg(client, MXT_FAMILY_ID, &val);
	if (error){
              printk(": Read MXT_FAMILY_ID failed\n");
#ifdef ATMEL_I2C_TEST
	INIT_DELAYED_WORK(&pen_event_work, atmel_i2c_test);
	ts_workqueue = create_workqueue("atmel_i2c_test");
	queue_delayed_work(ts_workqueue, &pen_event_work, msecs_to_jiffies(DELAY_TIME));
#endif

		return error;
       }
	info->family_id = val;
       printk(": MXT_FAMILY_ID = %d\n", val);

	error = mxt_read_reg(client, MXT_VARIANT_ID, &val);
	if (error)
		return error;
	info->variant_id = val;

	error = mxt_read_reg(client, MXT_VERSION, &val);
	if (error)
		return error;
	info->version = val;

	error = mxt_read_reg(client, MXT_BUILD, &val);
	if (error)
		return error;
	info->build = val;

	error = mxt_read_reg(client, MXT_OBJECT_NUM, &val);
	if (error)
		return error;
	info->object_num = val;

	return 0;
}

static int mxt_get_object_table(struct mxt_data *data)
{
	int error;
	int i;
	u16 reg;
	u16 end_address;
	u8 reportid = 0;
	u8 buf[MXT_OBJECT_SIZE];
	data->mem_size = 0;

	for (i = 0; i < data->info.object_num; i++) {
		struct mxt_object *object = data->object_table + i;

		reg = MXT_OBJECT_START + MXT_OBJECT_SIZE * i;
		error = mxt_read_object_table(data->client, reg, buf);
		if (error){
                     printk(": mxt_read_object_table error.......\n");
			return error;
              }

		object->type = buf[0];
		object->start_address = (buf[2] << 8) | buf[1];
		object->size = buf[3] + 1;
		object->instances = buf[4] + 1;
		object->num_report_ids = buf[5];

		if (object->num_report_ids) {
			reportid += object->num_report_ids * object->instances;
			object->max_reportid = reportid;
			object->min_reportid = object->max_reportid -
				object->instances * object->num_report_ids + 1;
		}

		end_address = object->start_address
			+ object->size * object->instances - 1;

		if (end_address >= data->mem_size)
			data->mem_size = end_address + 1;

		dbg_printk(": T%u, start:%u size:%u instances:%u "
			"min_reportid:%u max_reportid:%u\n",
			object->type, object->start_address, object->size,
			object->instances,
			object->min_reportid, object->max_reportid);
	}

	/* Store maximum reportid */
	data->max_reportid = reportid;

	return 0;
}

static int compare_versions(const u8 *v1, const u8 *v2)
{
	int i;

	if (!v1 || !v2)
		return -EINVAL;

	/* The major version number stays the same across different versions for
	 * a particular controller on a target. The minor and sub-minor version
	 * numbers indicate which version is newer.
	 */
	printk("v1[0] = %d, v1[0] = %d\n", (int)v1[0], (int)v2[0]);
	if (v1[0] != v2[0])
		return -EINVAL;

	for (i = 1; i < MXT_CFG_VERSION_LEN; i++) {
		if (v1[i] > v2[i])
			return MXT_CFG_VERSION_LESS;	/* v2 is older */

		if (v1[i] < v2[i])
			return MXT_CFG_VERSION_GREATER;	/* v2 is newer */
	}

	return MXT_CFG_VERSION_EQUAL;	/* v1 and v2 are equal */
}

static void mxt_check_config_version(struct mxt_data *data,
			const struct mxt_config_info *cfg_info,
			bool match_major,
			const u8 **cfg_version_found,
			bool *found_cfg_major_match)
{
	const u8 *cfg_version;
	int result = -EINVAL;

	cfg_version = cfg_info->config + data->cfg_version_idx;

	if (*cfg_version_found)
		result = compare_versions(*cfg_version_found, cfg_version);

       data->config_info = cfg_info;
 	   data->update_cfg = true;

       info_printk(":data->config_info->configlength = %d\n",
                            data->config_info->config_length);
}

/* If the controller's config version has a non-zero major number, call this
 * function with match_major = true to look for the latest config present in
 * the pdata based on matching family id, variant id, f/w version, build, and
 * config major number. If the controller is programmed with wrong config data
 * previously, call this function with match_major = false to look for latest
 * config based on based on matching family id, variant id, f/w version and
 * build only.
 */
static int mxt_search_config_array(struct mxt_data *data, bool match_major)
{
	const struct mxt_platform_data *pdata = data->pdata;
	const struct mxt_config_info *cfg_info;
	const struct mxt_info *info = &data->info;
	const u8 *cfg_version_found;
	bool found_cfg_major_match = false;
	int i;

	cfg_version_found = match_major ? data->cfg_version : NULL;

	for (i = 0; i < pdata->config_array_size; i++) {

		cfg_info = &pdata->config_array[i];

		if (!cfg_info->config || !cfg_info->config_length)
			continue;

		if (info->family_id == cfg_info->family_id &&
			info->variant_id == cfg_info->variant_id &&
			info->version == cfg_info->version &&
			info->build == cfg_info->build) {
            info_printk("version is macth\n");
			mxt_check_config_version(data, cfg_info, match_major,
				&cfg_version_found, &found_cfg_major_match);
		}
	}

	if (data->config_info || found_cfg_major_match)
		return 0;

	data->config_info = NULL;
	data->fw_name = NULL;

	return -EINVAL;
}

static int mxt_get_config(struct mxt_data *data)
{
	const struct mxt_platform_data *pdata = data->pdata;
	struct device *dev = &data->client->dev;
	struct mxt_object *object;
	int error;

	if (!pdata->config_array || !pdata->config_array_size) {
		err_printk(": No cfg data provided by platform data\n");
		return 0;
	}

	/* Get current config version */
 	object = mxt_get_object(data, MXT_SPT_USERDATA_T38);
 	if (!object) {
 		dev_err(dev, ": Unable to obtain USERDATA object\n");
 		return -EINVAL;
 	}

 	error = __mxt_read_reg(data->client, object->start_address,
 				sizeof(data->cfg_version), data->cfg_version);
 	if (error) {
 		dev_err(dev, ": Unable to read config version\n");
 		return error;
 	}
 	info_printk("Current config version on the controller is %d.%d.%d\n",
 			data->cfg_version[0], data->cfg_version[1],
 			data->cfg_version[2]);

	/* It is possible that the config data on the controller is not
	 * versioned and the version number returns 0. In this case,
	 * find a match without the config version checking.
	 */
 	error = mxt_search_config_array(data,
 				data->cfg_version[0] != 0 ? true : false);
 	if (error) {
 		/* If a match wasn't found for a non-zero config version,
 		 * it means the controller has the wrong config data. Search
 		 * for a best match based on controller and firmware version,
 		 * but not config version.
 		 */
 		if (data->cfg_version[0])
 			error = mxt_search_config_array(data, false);
 		if (error) {
 			dev_err(dev,
 				": Unable to find matching config in pdata\n");
 			return error;
 		}
 	}

	return 0;
}




static int mxt_save_objects(struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	struct mxt_object *t6_object;
	struct mxt_object *t7_object;
	struct mxt_object *t9_object;
	struct mxt_object *t15_object;
	int error;

	t6_object = mxt_get_object(data, MXT_GEN_COMMAND_T6);
	if (!t6_object) {
		dev_err(&client->dev, ": Failed to get T6 object\n");
		return -EINVAL;
	}
	data->t6_reportid = t6_object->max_reportid;
	if ( t6_object->num_report_ids > 1)
		dev_err(&client->dev, ": T6 has %d report id\n", t6_object->num_report_ids);
	/* Store T7 and T9 locally, used in suspend/resume operations */
	t7_object = mxt_get_object(data, MXT_GEN_POWER_T7);
	if (!t7_object) {
		dev_err(&client->dev, ": Failed to get T7 object\n");
		return -EINVAL;
	}

	data->t7_start_addr = t7_object->start_address;
	error = __mxt_read_reg(client, data->t7_start_addr,
				T7_DATA_SIZE, data->t7_data);
	if (error < 0) {
		dev_err(&client->dev,
			": Failed to save current power state\n");
		return error;
	}

    info_printk("t70[%d],t71[%d],t72[%d]\n",data->t7_data[0],
                              data->t7_data[1],data->t7_data[2]);


	/* Store T9, T15's min and max report ids */
	t9_object = mxt_get_object(data, MXT_TOUCH_MULTI_T9);
	if (!t9_object) {
		dev_err(&client->dev, ": Failed to get T9 object\n");
		return -EINVAL;
	}
	data->t9_max_reportid = t9_object->max_reportid;
	data->t9_min_reportid = t9_object->max_reportid -
					t9_object->num_report_ids + 1;

	if (data->pdata->key_codes) {
		t15_object = mxt_get_object(data, MXT_TOUCH_KEYARRAY_T15);
		if (!t15_object)
			dev_dbg(&client->dev, ": T15 object is not available\n");
		else {
			data->t15_max_reportid = t15_object->max_reportid;
			data->t15_min_reportid = t15_object->max_reportid -
						t15_object->num_report_ids + 1;
		}
	}

	return 0;
}

static int mxt_initialize(struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	struct mxt_info *info = &data->info;
	int error;
	u8 val;

	error = mxt_get_info(data);
	if (error) {
              err_printk(": mxt_initialize faile  ----> can not connect to Atmel TP\n");
		/* Try bootloader mode */
		error = mxt_switch_to_bootloader_address(data);
		if (error)
			return error;

		error = mxt_check_bootloader(client, MXT_APP_CRC_FAIL);
		if (error)
			return error;

		dev_err(&client->dev, "Application CRC failure\n");
		data->state = BOOTLOADER;

		return 0;
	}

	info_printk(
			": Family ID: %d Variant ID: %d Version: %d.%d "
			"Build: 0x%02X Object Num: %d\n",
			info->family_id, info->variant_id,
			info->version >> 4, info->version & 0xf,
			info->build, info->object_num);

	data->state = APPMODE;

	data->object_table = kcalloc(info->object_num,
				     sizeof(struct mxt_object),
				     GFP_KERNEL);
	if (!data->object_table) {
		dev_err(&client->dev, ": Failed to allocate memory\n");
		return -ENOMEM;
	}

	/* Get object table information */
	error = mxt_get_object_table(data);
	if (error)
		goto free_object_table;

	/* Get config data from platform data */
	error = mxt_get_config(data);
	if (error)
		err_printk(": Config info not found.\n");

#if defined(__UPDATE_CFG__)
        error = mxt_check_reg_init(data);
		if (error) {
			dev_err(&client->dev,
				": Failed to check reg init value\n");
			goto free_object_table;
		}
#endif

	error = mxt_save_objects(data);
	if (error)
		goto free_object_table;

	/* Update matrix size at info struct */
	error = mxt_read_reg(client, MXT_MATRIX_X_SIZE, &val);
	info->matrix_xsize = val;

	error = mxt_read_reg(client, MXT_MATRIX_Y_SIZE, &val);
	if (error)
	info->matrix_ysize = val;

    dev_info(&client->dev,
			": Matrix X Size: %d Matrix Y Size: %d\n",
			info->matrix_xsize, info->matrix_ysize);

	return 0;

free_object_table:
	kfree(data->object_table);
	return error;
}

static u8 one_object_type = 0 ;
static ssize_t mxt_one_object_show(struct device *dev,
                        struct device_attribute *attr, char *buf)
 {
    struct mxt_data * data = dev_get_drvdata(dev) ;
    struct mxt_object * object ;
    int count =0 ;
    int j  ;
    u8 val, error;
    object = mxt_get_object(data,one_object_type) ;
    for (j = 0; j < object->size/* + 1*/; j++) {
			error = mxt_read_object(data,
						object->type, j, &val);
			if (error)
				return error;

			count += snprintf(buf + count, PAGE_SIZE - count,
					"\t[%2d]: %02x\n", j, val);
			if (count >= PAGE_SIZE)
				return PAGE_SIZE - 1;
		}
    return count ;
 }

static ssize_t mxt_one_object_store(struct device *dev,struct device_attribute *attr,
                                const char *buf,size_t count)
{
    info_printk("object[%s]\n",buf) ;
    one_object_type = (u8)simple_strtoul(buf,NULL,10) ;
   return count ;
}

static ssize_t mxt_device_status_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
    struct mxt_data * data = dev_get_drvdata(dev) ;
    int count ;
    if(data->status==ACTIVE)
        count =  snprintf(buf,PAGE_SIZE,"%s\n","active") ;
    else if(data->status==DEEPSLEEP)
        count =  snprintf(buf,PAGE_SIZE,"%s\n","deepsleep");
    else
       count  =  snprintf(buf,PAGE_SIZE,"%s\n","none") ;

    return count ;
}

static ssize_t mxt_message_show(struct device *dev,
              struct device_attribute *attr, char *buf)
 {
    struct mxt_data * data = dev_get_drvdata(dev) ;
    disable_irq(data->irq) ;
    queue_work(data->atmel_wq,&data->work) ;
    return 0 ;
 }

static ssize_t mxt_object_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	struct mxt_object *object;
	int count = 0;
	int i, j;
	int error;
	u8 val;

	for (i = 0; i < data->info.object_num; i++) {
		object = data->object_table + i;

		count += snprintf(buf + count, PAGE_SIZE - count,
				"Object[%d] (Type %d)\n",
				i + 1, object->type);
		if (count >= PAGE_SIZE)
			return PAGE_SIZE - 1;

		for (j = 0; j < object->size + 1; j++) {
			error = mxt_read_object(data,
						object->type, j, &val);
			if (error)
				return error;

			count += snprintf(buf + count, PAGE_SIZE - count,
					"\t[%2d]: %02x (%d)\n", j, val, val);
			if (count >= PAGE_SIZE)
				return PAGE_SIZE - 1;
		}

		count += snprintf(buf + count, PAGE_SIZE - count, "\n");
		if (count >= PAGE_SIZE)
			return PAGE_SIZE - 1;
	}

	return count;
}

static int strtobyte(const char *data, u8 *value)
{
	char str[3];

	str[0] = data[0];
	str[1] = data[1];
	str[2] = '\0';

	return kstrtou8(str, 16, value);
}

static int mxt_load_fw(struct device *dev, const char *fn)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	const struct firmware *fw = NULL;
	unsigned int frame_size;
	unsigned int retry = 0;
	unsigned int pos = 0;
	int ret, i, max_frame_size;
	u8 *frame;

	switch (data->info.family_id) {
	case MXT224_ID:
	case MXT224E_ID:
		max_frame_size = MXT_SINGLE_FW_MAX_FRAME_SIZE;
		break;
	case MXT1386_ID:
		max_frame_size = MXT_CHIPSET_FW_MAX_FRAME_SIZE;
		break;
	default:
		return -EINVAL;
	}

	frame = kmalloc(max_frame_size, GFP_KERNEL);
	if (!frame) {
		dev_err(dev, "Unable to allocate memory for frame data\n");
		return -ENOMEM;
	}

	ret = request_firmware(&fw, fn, dev);
	if (ret < 0) {
		dev_err(dev, "Unable to open firmware %s\n", fn);
		goto free_frame;
	}

	if (data->state != BOOTLOADER) {
		/* Change to the bootloader mode */
		mxt_write_object(data, MXT_GEN_COMMAND_T6,
				MXT_COMMAND_RESET, MXT_BOOT_VALUE);
		mxt_reset_delay(data);

		ret = mxt_switch_to_bootloader_address(data);
		if (ret)
			goto release_firmware;
	}

	ret = mxt_check_bootloader(client, MXT_WAITING_BOOTLOAD_CMD);
	if (ret) {
		/* Bootloader may still be unlocked from previous update
		 * attempt */
		ret = mxt_check_bootloader(client,
			MXT_WAITING_FRAME_DATA);

		if (ret)
			goto return_to_app_mode;
	} else {
		dev_info(dev, "Unlocking bootloader\n");
		/* Unlock bootloader */
		mxt_unlock_bootloader(client);
	}

	while (pos < fw->size) {
		ret = mxt_check_bootloader(client,
						MXT_WAITING_FRAME_DATA);
		if (ret)
			goto release_firmware;

		/* Get frame length MSB */
		ret = strtobyte(fw->data + pos, frame);
		if (ret)
			goto release_firmware;

		/* Get frame length LSB */
		ret = strtobyte(fw->data + pos + 2, frame + 1);
		if (ret)
			goto release_firmware;

		frame_size = ((*frame << 8) | *(frame + 1));

		/* We should add 2 at frame size as the the firmware data is not
		 * included the CRC bytes.
		 */
		frame_size += 2;

		if (frame_size > max_frame_size) {
			dev_err(dev, "Invalid frame size - %d\n", frame_size);
			ret = -EINVAL;
			goto release_firmware;
		}

		/* Convert frame data and CRC from hex to binary */
		for (i = 2; i < frame_size; i++) {
			ret = strtobyte(fw->data + pos + i * 2, frame + i);
			if (ret)
				goto release_firmware;
		}

		/* Write one frame to device */
		mxt_fw_write(client, frame, frame_size);

		ret = mxt_check_bootloader(client,
						MXT_FRAME_CRC_PASS);
		if (ret) {
			retry++;

			/* Back off by 20ms per retry */
			msleep(retry * 20);

			if (retry > 20)
				goto release_firmware;
		} else {
			retry = 0;
			pos += frame_size * 2;
			dev_dbg(dev, "Updated %d/%zd bytes\n", pos, fw->size);
		}
	}

return_to_app_mode:
	mxt_switch_to_appmode_address(data);
release_firmware:
	release_firmware(fw);
free_frame:
	kfree(frame);

	return ret;
}

static const char *
mxt_search_fw_name(struct mxt_data *data, u8 bootldr_id)
{
	const struct mxt_platform_data *pdata = data->pdata;
	const struct mxt_config_info *cfg_info;
	const char *fw_name = NULL;
	int i;

	for (i = 0; i < pdata->config_array_size; i++) {
		cfg_info = &pdata->config_array[i];
		if (bootldr_id == cfg_info->bootldr_id && cfg_info->fw_name) {
			data->config_info = cfg_info;
			data->info.family_id = cfg_info->family_id;
			fw_name = cfg_info->fw_name;
		}
	}

	return fw_name;
}

static ssize_t mxt_update_fw_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	int error;
	const char *fw_name;
	u8 bootldr_id;
	u8 cfg_version[MXT_CFG_VERSION_LEN] = {0};

	/* If fw_name is set, then the existing firmware has an upgrade */
	if (!data->fw_name) {
		/*
		 * If the device boots up in the bootloader mode, check if
		 * there is a firmware to upgrade.
		 */
		if (data->state == BOOTLOADER) {
			bootldr_id = mxt_get_bootloader_id(data->client);
			if (bootldr_id <= 0) {
				dev_err(dev,
					"Unable to retrieve bootloader id\n");
				return -EINVAL;
			}
			fw_name = mxt_search_fw_name(data, bootldr_id);
			if (fw_name == NULL) {
				dev_err(dev,
				"Unable to find fw from bootloader id\n");
				return -EINVAL;
			}
		} else {
			/* In APPMODE, if the f/w name does not exist, quit */
			dev_err(dev,
			"Firmware name not specified in platform data\n");
			return -EINVAL;
		}
	} else {
		fw_name = data->fw_name;
	}

	dev_info(dev, "Upgrading the firmware file to %s\n", fw_name);

	disable_irq(data->irq);

	error = mxt_load_fw(dev, fw_name);
	if (error) {
		dev_err(dev, "The firmware update failed(%d)\n", error);
		count = error;
	} else {
		dev_info(dev, "The firmware update succeeded\n");

		/* Wait for reset */
		msleep(MXT_FWRESET_TIME);

		data->state = INIT;
		kfree(data->object_table);
		data->object_table = NULL;
		data->cfg_version_idx = 0;
		data->update_cfg = false;

		error = __mxt_write_reg(data->client, data->t38_start_addr,
				sizeof(cfg_version), cfg_version);
		if (error)
			dev_err(dev,
			"Unable to zero out config version after fw upgrade\n");

		mxt_initialize(data);
	}

	if (data->state == APPMODE) {
		enable_irq(data->irq);

		error = mxt_make_highchg(data);
		if (error)
			return error;
	}

	return count;
}

static DEVICE_ATTR(object, 0444, mxt_object_show, NULL);
static DEVICE_ATTR(message, 0444, mxt_message_show, NULL);
static DEVICE_ATTR(status, 0444, mxt_device_status_show,NULL);
static DEVICE_ATTR(update_fw, 0664, NULL, mxt_update_fw_store);
static DEVICE_ATTR(one,0664,mxt_one_object_show,mxt_one_object_store) ;
static DEVICE_ATTR(ver,0664,atmel_driver_show,NULL) ;

static struct attribute *mxt_attrs[] = {
	&dev_attr_object.attr,
	&dev_attr_update_fw.attr,
	&dev_attr_status.attr,
    &dev_attr_one.attr,
    &dev_attr_message.attr,
    &dev_attr_ver.attr,
	NULL
};

static const struct attribute_group mxt_attr_group = {
	.attrs = mxt_attrs,
};

static int mxt_start(struct mxt_data *data)
{
	int error;

	/* restore the old power state values and reenable touch */

    info_printk("device start\n");
    //dump_stack();
    if(data->t7_data[0]==0){
        info_printk("idle internal 0 \n");
        data->t7_data[0] = 32 ;
    }

    if(data->t7_data[1]==0){
        info_printk("active internal 0 \n");
        data->t7_data[1] = 16 ;
    }
    if(data->t7_data[2]==0){
        info_printk("active to idle internal 0 \n");
        data->t7_data[2] = 50;
    }
    error = __mxt_write_reg(data->client, data->t7_start_addr,
				T7_DATA_SIZE, data->t7_data);
	if (error < 0) {
		dev_err(&data->client->dev,
			"failed to restore old power state\n");
        return error;
	}
    data->status = ACTIVE  ;
	return 0;
}

static int mxt_stop(struct mxt_data *data)
{
	int error;
	u8 t7_data[T7_DATA_SIZE] = {0};
    info_printk("\n");
	error = __mxt_write_reg(data->client, data->t7_start_addr,
				T7_DATA_SIZE, t7_data);
	if (error < 0) {
		dev_err(&data->client->dev,
			"failed to configure deep sleep mode\n");
        return error;
	}

    data->status = DEEPSLEEP ;
	return 0;
}

static int mxt_input_open(struct input_dev *dev)
{
	struct mxt_data *data = input_get_drvdata(dev);
	int error;

	if (data->state == APPMODE) {
		error = mxt_start(data);
		if (error < 0) {
			dev_err(&data->client->dev, "mxt_start failed in input_open\n");

            return error;
		}
	}

	return 0;
}

static void mxt_input_close(struct input_dev *dev)
{
	struct mxt_data *data = input_get_drvdata(dev);
	int error;

	if (data->state == APPMODE) {
		error = mxt_stop(data);
		if (error < 0)
			dev_err(&data->client->dev, "mxt_stop failed in input_close\n");
	}
}

static int mxt_reset(struct mxt_data *data)
{
	int error;

	/* Re-Calibrate the touch to avoid some error*/
	error = mxt_write_object(data, MXT_GEN_COMMAND_T6,
			MXT_COMMAND_CALIBRATE,
			0x01);
	if (error < 0) {
		err_printk("write fail(%d)\n",error);
	}

	mxt_reset_delay(data);

	return error;
}

#ifdef CONFIG_PM
static int mxt_suspend(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);
	struct input_dev *input_dev = data->input_dev;
	int error;

	disable_irq(data->client->irq);
	if (cancel_work_sync(&data->work))
		enable_irq(data->client->irq);

	mutex_lock(&input_dev->mutex);

	if (input_dev->users) {
		error = mxt_stop(data);
		if (error < 0) {
			dev_err(dev, "mxt_stop failed in suspend\n");
			mutex_unlock(&input_dev->mutex);
			return error;
		}

	}

	mutex_unlock(&input_dev->mutex);

	mxt_cleanup_finger(data);

	return 0;
}

static int mxt_resume(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);
	struct input_dev *input_dev = data->input_dev;
	int error;

	mxt_reset(data);

	mxt_reset_delay(data);

	mutex_lock(&input_dev->mutex);

	if (input_dev->users) {
		error = mxt_start(data);
		if (error < 0) {
			dev_err(dev, "mxt_start failed in resume\n");
			mutex_unlock(&input_dev->mutex);
			return error;
		}
	}

	mutex_unlock(&input_dev->mutex);

	enable_irq(data->client->irq);

	return 0;
}

#if defined(CONFIG_HAS_EARLYSUSPEND)
static void mxt_early_suspend(struct early_suspend *h)
{
	struct mxt_data *data = container_of(h, struct mxt_data, early_suspend);

	mxt_suspend(&data->client->dev);
}

static void mxt_late_resume(struct early_suspend *h)
{
	struct mxt_data *data = container_of(h, struct mxt_data, early_suspend);

	mxt_resume(&data->client->dev);
}
#endif

static const struct dev_pm_ops mxt_pm_ops = {
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= mxt_suspend,
	.resume		= mxt_resume,
#endif
};
#endif

static int mxt_debugfs_object_show(struct seq_file *m, void *v)
{
	struct mxt_data *data = m->private;
	struct mxt_object *object;
	struct device *dev = &data->client->dev;
	int i, j, k;
	int error;
	int obj_size;
	u8 val;

	for (i = 0; i < data->info.object_num; i++) {
		object = data->object_table + i;
		obj_size = object->size + 1;

		seq_printf(m, "Object[%d] (Type %d)\n", i + 1, object->type);

		for (j = 0; j < object->instances + 1; j++) {
			seq_printf(m, "[Instance %d]\n", j);

			for (k = 0; k < obj_size; k++) {
				error = mxt_read_object(data, object->type,
							j * obj_size + k, &val);
				if (error) {
					dev_err(dev,
						"Failed to read object %d "
						"instance %d at offset %d\n",
						object->type, j, k);
					return error;
				}

				seq_printf(m, "Byte %d: 0x%02x (%d)\n",
						k, val, val);
			}
		}
	}

	return 0;
}

static int mxt_debugfs_object_open(struct inode *inode, struct file *file)
{
	return single_open(file, mxt_debugfs_object_show, inode->i_private);
}

static const struct file_operations mxt_object_fops = {
	.owner		= THIS_MODULE,
	.open		= mxt_debugfs_object_open,
	.read		= seq_read,
	.release	= single_release,
};

static void __devinit mxt_debugfs_init(struct mxt_data *data)
{
	debug_base = debugfs_create_dir(MXT_DEBUGFS_DIR, NULL);
	if (IS_ERR_OR_NULL(debug_base))
		pr_err("atmel_mxt_ts: Failed to create debugfs dir\n");
	if (IS_ERR_OR_NULL(debugfs_create_file(MXT_DEBUGFS_FILE,
					       0444,
					       debug_base,
					       data,
					       &mxt_object_fops))) {
		pr_err("atmel_mxt_ts: Failed to create object file\n");
		debugfs_remove_recursive(debug_base);
	}
}

static void atmel_ts_work_func(struct work_struct *work)
{
	struct mxt_data *data = container_of(work, struct mxt_data, work);
	struct mxt_message message;
	struct device *dev = &data->client->dev;
	int id = -1;
	u8 reportid;
    u8 object = 0 ;
    //info_printk("work report\n");
	if (data->state != APPMODE) {
		dev_err(dev, "Ignoring IRQ - not in APPMODE state\n");
		goto end;
	}

	do {

        if (mxt_read_message(data, &message)) {
			dev_err(dev, ": Failed to read message\n");
			goto end;
		}
		reportid = message.reportid ;


		if (!reportid) {
			dev_dbg(dev, ": Report id 0 is reserved\n");
			continue;
		}

		/* check whether report id is part of T9 or T15 */
		id = reportid - data->t9_min_reportid;

        dbg_printk("read message id[%d]\n",id);

		if (reportid >= data->t9_min_reportid &&
					reportid <= data->t9_max_reportid){
            object = 1;
            mxt_input_touchevent(data, &message, id);
		}
		else if (reportid >= data->t15_min_reportid &&
					reportid <= data->t15_max_reportid)
			mxt_handle_key_array(data, &message);
		else if (reportid == data->t6_reportid)
			mxt_handle_calibration(data, &message);
		else if (reportid == data->t42_reportid)
			mxt_handle_touchsuppression(data, &message);
		else if (reportid == data->t62_reportid)
			mxt_handle_noisesuppression(data, &message);
		else
			mxt_dump_message(dev, &message);
        //in mode 0 ;
	} while ((reportid != 0xff) || !gpio_get_value(data->pdata->gpio_irq));
end:
    if(object)
    mxt_input_report(data, id);

	enable_irq(data->client->irq);

}

#define CALI_SIZE 64
#define CAL_VAL  20

//0:calibration success   1:calibration
static int cali_tp(struct mxt_data * data )
{
    int ret = 0 ;
    int touch_sum = 0 ;
    int antitouch_sum = 0 ;
    int i ;
    u8 tmp ;

    return 0 ;
    ret = mxt_write_object(data,MXT_GEN_COMMAND_T6,MXT_COMMAND_DIAGNOSTIC,0x3F) ;
    if(ret){
        err_printk("write COMMAND T6 error\n");
        return 1 ;
    }

    //read touch number
    for(i=0;i< CALI_SIZE;i++)
    {
        ret = mxt_read_object(data,MXT_DEBUG_DIAGNOSTIC_T37,i,&tmp);
        if(ret){
           err_printk("read T37 error\n");
           return 1 ;
        }
        if(i<CALI_SIZE/2)
        {
           while(tmp){
           touch_sum ++ ;
           tmp = tmp&(tmp-1) ;
           }
        }else {
           while(tmp){
           antitouch_sum ++ ;
           tmp = tmp&(tmp-1) ;
           }
        }


    }

    if(touch_sum &&(antitouch_sum==0)){
        // calibration may be good
        //todo close calibration
        return 0 ;
    }else if(touch_sum+CAL_VAL<antitouch_sum){

        return 1 ;
   }

    return 1 ;
}

static enum hrtimer_restart timer_func(struct hrtimer *timer)
{
    static int count = 0 ;
    struct mxt_data *ts = container_of(timer, struct mxt_data, timer);
    info_printk("timer [%d]\n",count) ;

    if(count++>10){
        count = 0 ;
        return HRTIMER_NORESTART ;
    }

    if(cali_tp(ts))
     hrtimer_start(&ts->timer, ktime_set(1,0), HRTIMER_MODE_REL);

    return HRTIMER_NORESTART;
}


static int __devinit mxt_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	const struct mxt_platform_data *pdata = client->dev.platform_data;
	struct mxt_data *data;
	struct input_dev *input_dev;
	int error, i;

	if (!pdata){
        err_printk("no platform data \n");
        return -EINVAL;
	}

    info_printk("mxt_probe \n");

	data = kzalloc(sizeof(struct mxt_data), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!data || !input_dev) {
		dev_err(&client->dev, "Failed to allocate memory\n");
		error = -ENOMEM;
		goto err_free_mem;
	}

	data->state = INIT;
	input_dev->name = ATMEL_MXT154_NAME;
	input_dev->id.bustype = BUS_I2C;
	//input_dev->dev.parent = &client->dev;
	input_dev->open  = mxt_input_open;
	input_dev->close = mxt_input_close;

#ifdef ATMEL_I2C_TEST
       this_client = client;
#endif
	data->client = client;
	data->input_dev = input_dev;
	data->pdata = pdata;
	data->irq = client->irq;

	__set_bit(EV_ABS, input_dev->evbit);
	__set_bit(EV_KEY, input_dev->evbit);
    __set_bit(EV_SYN,input_dev->evbit) ;
	__set_bit(BTN_TOUCH, input_dev->keybit);

	/*
	For multi touch
    disp_maxx = 540 ;
    disp_maxy = 960 ;
   */
    dbg_printk("display x[%d],display y[%d]\n",pdata->disp_maxx,pdata->disp_maxy);

    input_mt_init_slots(input_dev, MXT_MAX_FINGER);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR,
			     0, MXT_MAX_AREA, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X,
			pdata->disp_minx, pdata->disp_maxx, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y,
			pdata->disp_miny, pdata->disp_maxy, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE,
			     0, MXT_MAX_PRESSURE, 0, 0);

	/* set key array supported keys */
	if (pdata->key_codes) {
		for (i = 0; i < MXT_KEYARRAY_MAX_KEYS; i++) {
			if (pdata->key_codes[i])
				input_set_capability(input_dev, EV_KEY,
							pdata->key_codes[i]);
		}
	}

	input_set_drvdata(input_dev, data);
	i2c_set_clientdata(client, data);

	if (pdata->gpio_init){
		error = pdata->gpio_init();
        if(error< 0){
            err_printk("request gpio error\n");
            goto err_free_mem ;
        }
	}


	mxt_reset_delay(data);

	error = mxt_initialize(data);
	if (error){
     err_printk("mxt initialize error \n");
        if(pdata->gpio_uninit)
            pdata->gpio_uninit() ;
        goto err_gpio_rst_req ;
	}
	data->atmel_wq = create_singlethread_workqueue("atmel_wq");
	if (!data->atmel_wq) {
		err_printk(": fail to create work queue for atmel CTP!\n");
		error = -ENOMEM;
		goto err_free_object;
	}


    INIT_WORK(&data->work, atmel_ts_work_func);

    error = request_irq(gpio_to_irq(pdata->gpio_irq), mxt_irq_handler,
            IRQF_DISABLED|IRQF_TRIGGER_LOW, client->dev.driver->name, data);
    if (error) {
       dev_err(&client->dev, ": Failed to register interrupt\n");
       goto err_free_wq;
    }

	if (data->state == APPMODE) {
		error = mxt_make_highchg(data);
		if (error) {
			dev_err(&client->dev, ": Failed to make high CHG\n");
			goto err_free_irq;
		}
	}

	error = input_register_device(input_dev);
	if (error){
        err_printk("register input devices error \n");
        goto err_free_irq;
	}

    data->dev.init_name = ATMEL_MXT154_NAME  ;
    dev_set_drvdata(&data->dev,data);
    error = device_register(&data->dev) ;
    error = sysfs_create_group(&data->dev.kobj, &mxt_attr_group);
	if (error)
		goto err_unregister_device;


#if defined(CONFIG_HAS_EARLYSUSPEND)
	data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN +
						MXT_SUSPEND_LEVEL;
	data->early_suspend.suspend = mxt_early_suspend;
	data->early_suspend.resume = mxt_late_resume;
	register_early_suspend(&data->early_suspend);
#endif

	mxt_debugfs_init(data);

    mxt_read_object(data,MXT_TOUCH_MULTI_T9,MXT_TOUCH_XRANGE_LSB,(u8*)&i);
    error = i ;
    mxt_read_object(data,MXT_TOUCH_MULTI_T9,MXT_TOUCH_XRANGE_MSB,(u8*)&i);
    error = error + (i<<8) ;
        info_printk("x resolution[%d]\n",error);

    mxt_read_object(data,MXT_TOUCH_MULTI_T9,MXT_TOUCH_YRANGE_LSB,(u8*)&i);
    error = i ;
    mxt_read_object(data,MXT_TOUCH_MULTI_T9,MXT_TOUCH_YRANGE_MSB,(u8*)&i);
    error = error + (i<<8) ;
    info_printk("y resolution[%d]\n",error);

    error = mxt_read_object(data,MXT_TOUCH_MULTI_T9,0,(u8*)&i);
    if(error){
        err_printk("read T9 error\n");
    }else {
        info_printk("T9[0x%x]\n",i);
    }

    hrtimer_init(&data->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    data->timer.function = timer_func;
    hrtimer_start(&data->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
    info_printk("max touch  probe success\n");
    return 0;

err_unregister_device:
	input_unregister_device(input_dev);
	input_dev = NULL;
err_free_irq:
	free_irq(client->irq, data);
err_free_wq:
	destroy_workqueue(data->atmel_wq);
err_free_object:
	kfree(data->object_table);
err_gpio_rst_req:

err_free_mem:
	input_free_device(input_dev);
	kfree(data);
	return error;
}

static int __devexit mxt_remove(struct i2c_client *client)
{
	struct mxt_data *data = i2c_get_clientdata(client);

	sysfs_remove_group(&client->dev.kobj, &mxt_attr_group);

	destroy_workqueue(data->atmel_wq);
	input_unregister_device(data->input_dev);
#if defined(CONFIG_HAS_EARLYSUSPEND)
	unregister_early_suspend(&data->early_suspend);
#endif

	kfree(data->object_table);
	kfree(data);

    if(data->pdata->gpio_uninit)
       data->pdata->gpio_uninit() ;

	debugfs_remove_recursive(debug_base);

	return 0;
}

static const struct i2c_device_id mxt_id[] = {
	{ATMEL_MXT154_NAME, 0 },

};
MODULE_DEVICE_TABLE(i2c, mxt_id);

static struct i2c_driver mxt_driver = {
	.driver = {
		.name	= ATMEL_MXT154_NAME,
		.owner	= THIS_MODULE,
#ifdef CONFIG_PM
		.pm	= &mxt_pm_ops,
#endif
	},
	.probe		= mxt_probe,
	.remove		= __devexit_p(mxt_remove),
	.id_table	= mxt_id,
};

static int __init mxt_init(void)
{
	return i2c_add_driver(&mxt_driver);
}

static void __exit mxt_exit(void)
{
	i2c_del_driver(&mxt_driver);
}

module_init(mxt_init);
module_exit(mxt_exit);

/* Module information */
MODULE_AUTHOR("Joonyoung Shim <jy0922.shim@samsung.com>");
MODULE_DESCRIPTION("Atmel maXTouch Touchscreen driver");
MODULE_LICENSE("GPL");
