/*
 * Touch Screen driver for Sitronix I2C Interface.
 *   Copyright (c) 2010 CT Chen <ct_chen@sitronix.com.tw>
 *
 * Please refer to Sitronix I2C Interface Protocol for the protocol specification.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/timer.h>
#include <linux/gpio.h>

#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <mach/gpio.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/slab.h>

#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/string.h>
#include <asm/unistd.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include "sitronix_ts.h"

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#define TEST 0

struct sitronix_ts_data {
	uint16_t addr;
	struct i2c_client *client;
	struct input_dev *input_dev;
	int use_irq;
	struct hrtimer timer;
	struct work_struct  work;
	int (*power)(int on);
	int (*get_int_status)(void);
	void (*reset_ic)(void);
};

struct sitronix_ts_priv {
	struct i2c_client *client;
	struct input_dev *input;
	struct work_struct work;
	struct mutex mutex;
	struct hrtimer timer;
	#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
	#endif
	int irq;
	bool isp_enabled;
	bool autotune_result;
	bool always_update;
	char I2C_Offset;
	char I2C_Length;
	char I2C_RepeatTime;
	int  fw_revision;
	int	 struct_version;
};

#define	MAX_CHANNELS			32
#define CONFIG_PARAM_MAX_CHANNELS	32
#define MUTUAL_CALIBRATION_BASE_SIZE	256

#define FLAG_GPIO    122
#define INT_GPIO     82
#define RESET_GPIO	 30
#define ST_KEY_SEARCH 	217
static struct input_dev *simulate_key_input = NULL;

//#define USE_POLL
//#define DEBUG
#ifdef DEBUG
#define TP_DEBUG(format, args...)   printk(KERN_INFO "TP_:%s( )_%d_: " format, \
	__FUNCTION__ , __LINE__, ## args);
#define DBG() printk("[%s]:%d => \n",__FUNCTION__,__LINE__)
#else
#define TP_DEBUG(format, args...);
#define DBG()
#endif

struct config_param_v0{
	u8 	reserve1[3];		
	u8	x_chs;				// Number of X channels.
	u8	y_chs;				// Number of Y channels.								
	u16	x_res;				// X resolution. [6:5]
	u16	y_res;				// Y resolution. [8:7]
	u8	ch_map[MAX_CHANNELS];		// Channle mapping table.

	u8	data_threshold_shift_x;		// Data threshold shift. [41]
	u8	data_threshold_offset_x;	// Data threshold offset.[42]

	u8	pt_threshold_shift;		// Point threshold shift. [43]
	u8	pt_threshold_offset;		// Point threshold offset. [44]
	u8      reserve2[5];
	
	u8 	cnt[MAX_CHANNELS];
	u8	offset[MAX_CHANNELS];
	u16	baseline[MAX_CHANNELS];	
	
	u8	k_chs;
	u8 	reserve4[11];
	
	u8	wake_threshold_shift;		
	u8	data_threshold_shift_y;		// Data threshold shift.  [191]
	u8	data_threshold_offset_y;	// Data threshold offset. [192]
	u8	data_threshold_shift_k;		// Data threshold shift.  [193]
	u8	data_threshold_offset_k;	// Data threshold offset. [194] 
	
	u8	peak_threshold_shift_x;		// Data threshold shift. 	[195]
	u8	peak_threshold_offset_x;	// Data threshold offset.	[196]
	u8	peak_threshold_shift_y;		// Data threshold shift.	[197]
	u8	peak_threshold_offset_y;	// Data threshold offset.	[198]
	
	u8	mutual_threshold_shift;		// Data threshold shift.	[199]
	u8	mutual_threshold_offset;	// Data threshold offset.	[200]
	
	//Filter
	u8	filter_rate;		// [201]
	u16	filter_range_1;	// [202]
	u16	filter_range_2;	// [203]
	
	u8	reserve5[299];
} __attribute__ ((packed)); 


struct config_param_v1{
	u8	reserve1[3];
	u8	x_chs;			           	 //BCB:x_chs, Number of X channels.
	u8	y_chs;			       	//BCB:y_chs, Number of Y channels.
	u8	k_chs;				       //BCB:k_chs, Number of Key channels.
	u16	x_res;				       //BCB:x_res,x report resolution
	u16	y_res;			            	//BCB:y_res,y report resolution
	u8	ch_map[MAX_CHANNELS];		//BCB:ch_map,Channle mapping table.
	
	u8	reserve2[15];
	
	u8	data_threshold_shift_x;		    //BCB:Daimond_data_threshold_shift_X,Data threshold shift.
	u8	data_threshold_offset_x;	    //BCB:Daimond_data_threshold_offset_X,Data threshold offset.
	u8	pt_threshold_shift;		        //BCB:pt_threshold_shift_XY,Point threshold shift.
	u8	pt_threshold_offset;		    //BCB:pt_threshold_offset_XY,Point threshold offset.
	u16	pt_threshold;			        //BCB:pt_threshold,Point threshold.
	u8	reserve3;
	u8	data_threshold_shift_y;		    //BCB:data_threshold_shift_Y,Data threshold shift.
	u8	data_threshold_offset_y;	    //BCB:data_threshold_offset_Y,Data threshold offset.
	u8	data_threshold_shift_k;		    //BCB:pt_threshold_shift_K,Data threshold shift.
	u8	data_threshold_offset_k;	    //BCB:pt_threshold_offset_K,Data threshold offset.
	u8	peak_threshold_shift_x;		    //BCB:Daimond_peak_shift_X,Data threshold shift.
	u8	peak_threshold_offset_x;	    //BCB:Daimond_peak_offset_X,Data threshold offset.
	u8	peak_threshold_shift_y;		    //BCB:Daimond_peak_shift_Y,Data threshold shift.
	u8	peak_threshold_offset_y;	    //BCB:Daimond_peak_offset_Y,Data threshold offset.
	u8	mutual_threshold_shift;		    //BCB:Daimond_mutual_threshold_shift,Data threshold shift.
	u8	mutual_threshold_offset;	//BCB:Daimond_mutual_threshold_offset,Data threshold offset.
	u8	filter_rate;                    	//BCB:filter_rate,
	u16	filter_range_1;                 	//BCB:filter_range1,
	u16	filter_range_2;	              //BCB:filter_range2,
	//
	u8  Bar_X_RAW;                      	//BCB:Bar_X_RAW,
	u8  Bar_X_Raw_2_Peak;             	//BCB:Bar_X_Raw_2_Peak,
	u8  Bar_X_Delta;                    	//BCB:Bar_X_Delta,
	u16 Bar_Y_Delta_2_Peak;           	//BCB:Bar_Y_Delta_2_Peak,
	u8  Border_Offset_X;                	//BCB:Border_Offset_X,
	u8  Border_Offset_Y;                	//BCB:Border_Offset_Y,
	
	u8	reserve[42];
	u8	cnt[CONFIG_PARAM_MAX_CHANNELS];
	u8	offset[CONFIG_PARAM_MAX_CHANNELS];
	u16	baseline[CONFIG_PARAM_MAX_CHANNELS];
	u8	mutual_baseline[MUTUAL_CALIBRATION_BASE_SIZE];		//used to store the mutual calibration baseline in no touch
} __attribute__ ((packed)); 

#define SITRONIX_MT_ENABLED

#define STX_TS_MAX_RES_SHIFT	(11)
#define STX_TS_MAX_VAL		((1 << STX_TS_MAX_RES_SHIFT) - 1)

#define ST1232_FLASH_SIZE	0x3C00
#define ST1232_ISP_PAGE_SIZE	0x200
#define ST1232_ROM_PARAM_ADR 0x3E00

// ISP command.
#define STX_ISP_ERASE		0x80
#define STX_ISP_SEND_DATA	0x81
#define STX_ISP_WRITE_FLASH	0x82
#define STX_ISP_READ_FLASH	0x83
#define STX_ISP_RESET		0x84
#define STX_ISP_READY		0x8F

typedef struct {
	u8	y_h		: 3,
		reserved	: 1,
		x_h		: 3,
		valid		: 1;
	u8	x_l;
	u8	y_l;
} xy_data_t;

typedef struct {
	u8	fingers		: 3,
		gesture		: 5;
	u8	keys;
	xy_data_t	xy_data[2];
} stx_report_data_t;

static struct sitronix_ts_data *sitronix_ts_gpts = NULL;
static u8 isp_page_buf[ST1232_ISP_PAGE_SIZE];

int      sitronix_release(struct inode *, struct file *);
int      sitronix_open(struct inode *, struct file *);
ssize_t  sitronix_write(struct file *file, const char *buf, size_t count, loff_t *ppos);
ssize_t  sitronix_read(struct file *file, char *buf, size_t count, loff_t *ppos);
long	 sitronix_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static struct cdev sitronix_cdev;
static struct class *sitronix_class;
static int sitronix_major = 0;

static uint8_t sitronix_ts_gJNIDataBuffer[SITRONIX_JNI_BUFFER_SIZE] = {0};

static int sitronix_ts_gAutoTuneTableOffset = 0;
static int sitronix_ts_gAutoTuneTableSize = 0;
static uint8_t sitronix_ts_gAutoTuneTableData[SITRONIX_AUTOTUNE_TABLE_BUFFER_SIZE] = {0};
static int sitronix_ts_gAutoTuneTableFlashAddr = 0;
static int sitronix_ts_gAutoTuneTableFlashSize = 0;

static int sitronix_ts_gAutoTuneCodeOffset = 0;
static int sitronix_ts_gAutoTuneCodeSize = 0;
static uint8_t sitronix_ts_gAutoTuneCodeData[SITRONIX_AUTOTUNE_CODE_BUFFER_SIZE] = {0};
static int sitronix_ts_gAutoTuneCodeFlashAddr = 0;
static int sitronix_ts_gAutoTuneCodeFlashSize = 0;

static int sitronix_ts_gTouchParamFileOffset = 0;
static int sitronix_ts_gTouchParamFileSize = 0;
static uint8_t sitronix_ts_gTouchParamFileData[SITRONIX_TOUCH_PARAM_BUFFER_SIZE] = {0};
static int sitronix_ts_gTouchParamFileFlashAddr = 0;
static int sitronix_ts_gTouchParamFileFlashSize = 0;

static int sitronix_ts_gTouchFwFileOffset = 0;
static int sitronix_ts_gTouchFwFileSize = 0;
static uint8_t sitronix_ts_gTouchFwFileData[SITRONIX_TOUCH_FW_BUFFER_SIZE] = {0};
static int sitronix_ts_gTouchFwFileFlashAddr = 0;
static int sitronix_ts_gTouchFwFileFlashSize = 0;

static uint8_t sitronix_ts_Isp_key[] = {0,'S',0,'T',0,'X',0,'_',0,'F',0,'W',0,'U',0,'P'};

int  sitronix_open(struct inode *inode, struct file *filp)
{
	return 0;
}
EXPORT_SYMBOL(sitronix_open);

int  sitronix_release(struct inode *inode, struct file *filp)
{
	return 0;
}
EXPORT_SYMBOL(sitronix_release);

ssize_t  sitronix_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	return -EROFS;
}
EXPORT_SYMBOL(sitronix_write);

ssize_t  sitronix_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	return 0;
}
EXPORT_SYMBOL(sitronix_read);

static bool sitronix_ts_checksum(int size, uint8_t checksum, uint8_t *Buf)
{
	int i = 0;
	uint8_t temp = 0;
	temp = Buf[0];
	for(i = 1; i < size ; i++){
		//UpgradeMsg("[%d] %02x^%02x = %02x\n", i, temp , sitronix_ts_gTouchFwFileData[i], temp^ sitronix_ts_gTouchFwFileData[i]);
		temp = temp ^ Buf[i];
	}
	UpgradeMsg("checksum = %02x\n", temp);
	return (checksum == temp)? true:false;	
}

static int sitronix_ts_IspModeEnable(struct sitronix_ts_data *ts)
{
	int ret = 0;
	struct i2c_msg msg[2];
	uint8_t TransmitData[2];
	unsigned char *pReceivedData = NULL;
	int i = 0;

	UpgradeMsg("%s\n", __FUNCTION__);
	pReceivedData = (unsigned char *)kmalloc(1, GFP_ATOMIC);
	if(pReceivedData == NULL)
		return -ENOMEM;

	for(i = 0; i < sizeof(sitronix_ts_Isp_key); i+=2){
		msg[0].addr = ts->client->addr;
		msg[0].flags = 0;
		msg[0].len = 2;
		msg[0].buf = &TransmitData[0];
		TransmitData[0] = sitronix_ts_Isp_key[i];
		TransmitData[1] = sitronix_ts_Isp_key[i+1];

		ret = i2c_transfer(ts->client->adapter, msg, 1);
		if (ret < 0){
			UpgradeMsg("Enable ISP mode error (%d)\n", ret);
			return ret;
		}
	}
	mdelay(SITRONIX_TS_CHANGE_MODE_DELAY);

	return 0;
}

static int sitronix_ts_IspReset(struct sitronix_ts_data *ts)
{
	int ret = 0;
	struct i2c_msg msg[1];
	uint8_t TransmitData[8];

	UpgradeMsg("%s\n", __FUNCTION__);
	msg[0].addr = ts->client->addr;
	msg[0].flags = 0;
	msg[0].len = ISP_PACKET_SIZE;
	msg[0].buf = TransmitData;
	memset(TransmitData, 0, ISP_PACKET_SIZE);
	TransmitData[0] = ISP_CMD_RESET;

	ret = i2c_transfer(ts->client->adapter, msg, 1);
	if (ret < 0){
		UpgradeMsg("Send ISP_RESET packet error (%d)\n", ret);
		return ret;
	}
	
	mdelay(SITRONIX_TS_CHANGE_MODE_DELAY);
	mdelay(SITRONIX_TS_CHANGE_MODE_DELAY);

	msg[0].addr = ts->client->addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = TransmitData;
	TransmitData[0] = I2C_REG_DEV_CTRL_ADDR;
	TransmitData[1] = I2C_DEV_CTRL_RESET;

	ret = i2c_transfer(ts->client->adapter, msg, 1);
	if (ret < 0){
		UpgradeMsg("Reset error (%d)\n", ret);
		return ret;
	}
	mdelay(SITRONIX_TS_CHANGE_MODE_DELAY);

	return 0;
}

static int sitronix_ts_IspPageErase(struct sitronix_ts_data *ts, uint8_t PageNumber)
{
	int ret = 0;
	struct i2c_msg msg[1];
	uint8_t TransmitData[8], ReceivedData[8];

	UpgradeMsg("%s\n", __FUNCTION__);
	msg[0].addr = ts->client->addr;
	msg[0].flags = 0;
	msg[0].len = ISP_PACKET_SIZE;
	msg[0].buf = TransmitData;
	memset(TransmitData, 0, ISP_PACKET_SIZE);
	TransmitData[0] = ISP_CMD_ERASE;
	TransmitData[2] = PageNumber;
	ret = i2c_transfer(ts->client->adapter, msg, 1);
	if (ret < 0){
		UpgradeMsg("Send ISP_ERASE packete error (%d)\n", ret);
		return ret;
	}

	msg[0].addr = ts->client->addr;
	msg[0].flags = I2C_M_RD;
	msg[0].len = ISP_PACKET_SIZE;
	msg[0].buf = ReceivedData;

	ret = i2c_transfer(ts->client->adapter, msg, 1);
	if (ret < 0){
		UpgradeMsg("Read packet error (%d)\n", ret);
		return ret;
	}else{
		if(ReceivedData[0] != ISP_CMD_READY){
			UpgradeMsg("Read ISP_READY packet error (%d)\n", ret);
			return -EBUSY; 
		}
	}

	return 0;
}
static int sitronix_ts_IspPageRead(struct sitronix_ts_data *ts, uint8_t *buf, uint8_t PageNumber)
{
	int ret = 0;
	struct i2c_msg msg[1];
	uint8_t TransmitData[8];
	int16_t ReadNumByte = 0;

	UpgradeMsg("%s, Page# = %d\n", __FUNCTION__, PageNumber);
	msg[0].addr = ts->client->addr;
	msg[0].flags = 0;
	msg[0].len = ISP_PACKET_SIZE;
	msg[0].buf = TransmitData;
	memset(TransmitData, 0, ISP_PACKET_SIZE);
	TransmitData[0] = ISP_CMD_READ_FLASH;
	TransmitData[2] = PageNumber;
	ret = i2c_transfer(ts->client->adapter, msg, 1);
	if (ret < 0){
		UpgradeMsg("Send ISP_READ_FLASH packete error (%d)\n", ret);
		return ret;
	}

	msg[0].addr = ts->client->addr;
	msg[0].flags = I2C_M_RD;
	msg[0].len = ISP_PACKET_SIZE;
	while(ReadNumByte < DEV_FLAHS_PAGE_SIZE){
		msg[0].buf = &buf[ReadNumByte];

		ret = i2c_transfer(ts->client->adapter, msg, 1);
		if (ret < 0){
			UpgradeMsg("Read ISP page data error (%d)\n", ret);
			return ret;
		}
		if(ret == 0)
			break;
		ReadNumByte += ISP_PACKET_SIZE;
		//UpgradeMsg("ReadNumByte = %d\n", ReadNumByte);
	}
	return ReadNumByte;
}
static int sitronix_ts_FlashRead(struct sitronix_ts_data *ts, uint8_t *buf, int Offset, int NumByte)
{
	int ret = 0;
	int16_t ReadNumByte = 0, StartPage, PageOffset, ReadSize;
	uint8_t temp[DEV_FLAHS_PAGE_SIZE];
	
	UpgradeMsg("%s, Offset = %d, Size = %d\n", __FUNCTION__, Offset, NumByte);
	if(NumByte == 0)
		return 0;
	if(Offset >= DEV_FLASH_SIZE)
		return -EINVAL;
	
	if((Offset + NumByte) > DEV_FLASH_SIZE)
		NumByte = DEV_FLASH_SIZE - Offset;

	StartPage = Offset / DEV_FLAHS_PAGE_SIZE;
	PageOffset = Offset % DEV_FLAHS_PAGE_SIZE;

	while(NumByte > 0){
		ret = sitronix_ts_IspPageRead(ts, temp, StartPage++);
		if(ret < 0)
			return ret;
		ReadSize = DEV_FLAHS_PAGE_SIZE - PageOffset;
		if(NumByte < ReadSize)
			ReadSize = NumByte;
		memcpy(buf, &temp[PageOffset], ReadSize);
		buf += ReadSize;
		ReadNumByte += ReadSize;
		NumByte -= ReadSize;
		PageOffset = 0;
	}
	return ReadNumByte;
}

static int sitronix_ts_IspCalChecksum(uint8_t *buf, int DataSize)
{
	short int checksum = 0;
	int i = 0;
	UpgradeMsg("%s\n", __FUNCTION__);
	for(i = 0; i < DataSize; i++)
		checksum += buf[i];
	UpgradeMsg("checksum = %04x\n", checksum);
	return checksum;
}

static int sitronix_ts_IspPageWrite(struct sitronix_ts_data *ts, uint8_t *buf, uint8_t PageNumber)
{
	int ret = 0;
	struct i2c_msg msg[1];
	uint8_t TransmitData[8], ReceivedData[8];
	int16_t WriteNumByte = 0, WriteLength = 0 ;
	unsigned short checksum;
	int RetryCount = 0;
	
	UpgradeMsg("%s, Page# = %d\n", __FUNCTION__, PageNumber);
	msg[0].addr = ts->client->addr;
	msg[0].len = ISP_PACKET_SIZE;

	while(RetryCount++ < 4){
		WriteNumByte = 0;
		memset(TransmitData, 0, ISP_PACKET_SIZE);

		checksum = sitronix_ts_IspCalChecksum(buf, DEV_FLAHS_PAGE_SIZE);
		TransmitData[0] = ISP_CMD_WRITE_FLASH;
		TransmitData[2] = PageNumber;
		TransmitData[4] = (uint8_t)(checksum & 0xff);
		TransmitData[5] = (uint8_t)(checksum >> 8);

		msg[0].flags = 0;
		msg[0].buf = TransmitData;
		ret = i2c_transfer(ts->client->adapter, msg, 1);
		if (ret < 0){
			UpgradeMsg("Send ISP_WRITE_FLASH packete error (%d)\n", ret);
			return ret;
		}

		TransmitData[0] = ISP_CMD_SEND_DATA;
		while(WriteNumByte < DEV_FLAHS_PAGE_SIZE){
			WriteLength = DEV_FLAHS_PAGE_SIZE - WriteNumByte;
			if(WriteLength > 7)
				WriteLength = 7;
			memcpy(&TransmitData[1], &buf[WriteNumByte], WriteLength);
			ret = i2c_transfer(ts->client->adapter, msg, 1);
			if (ret < 0){
				UpgradeMsg("Send ISP_SEND_DATA packet error (%d)\n", ret);
				return ret;
			}
			WriteNumByte += WriteLength;
		}

		msg[0].flags = I2C_M_RD;
		msg[0].buf = ReceivedData;

		ret = i2c_transfer(ts->client->adapter, msg, 1);
		if (ret < 0){
			UpgradeMsg("Read packet error (%d)\n", ret);
			return ret;
		}else{
			if(ReceivedData[0] != ISP_CMD_READY){
				UpgradeMsg("Read ISP_READY packet fail\n");
				return -EBUSY;
			}
			if((ReceivedData[2] & 0x10) != 0){
				UpgradeMsg("error occurs and retry to write flash (retry %d times) \n", RetryCount);
				if(RetryCount < 4){
					if(sitronix_ts_IspPageErase(ts, PageNumber) < 0){
						UpgradeMsg("fail to erase flash ");
						return -EFAULT;
					}
					continue;
				}else{
					UpgradeMsg("Error occurs during write page data into flash (0x%x)", ReceivedData[2]);
					return -EFAULT;
				}
			}
		}
		break;
	}
	return WriteNumByte;
}
static int sitronix_ts_FlashWrite(struct sitronix_ts_data *ts, uint8_t *buf, int Offset, int NumByte)
{
	int16_t WriteNumByte = 0, StartPage, PageOffset, WriteLength;
	uint8_t temp[DEV_FLAHS_PAGE_SIZE];
	
	UpgradeMsg("%s, offset = %d, Size = %d\n", __FUNCTION__, Offset, NumByte);
	if(NumByte == 0)
		return 0;
	if(Offset >= DEV_FLASH_SIZE)
		return -EINVAL;
	if((Offset + NumByte) > DEV_FLASH_SIZE)
		NumByte = DEV_FLASH_SIZE - Offset;
	StartPage = Offset / DEV_FLAHS_PAGE_SIZE;
	PageOffset = Offset % DEV_FLAHS_PAGE_SIZE;
	while(NumByte > 0){
		if((PageOffset != 0) || (NumByte < DEV_FLAHS_PAGE_SIZE)){
			if(sitronix_ts_IspPageRead(ts, temp, StartPage) < 0)
				return -EFAULT;
		}
		WriteLength = DEV_FLAHS_PAGE_SIZE - PageOffset;
		if(NumByte < WriteLength)
			WriteLength = NumByte;
		memcpy(&temp[PageOffset], buf, WriteLength);
		if((sitronix_ts_IspPageWrite(ts, temp, StartPage++)) < DEV_FLAHS_PAGE_SIZE)
			return -EFAULT;
		NumByte -= WriteLength;
		buf += WriteLength;
		WriteNumByte += WriteLength;
		PageOffset = 0;
	}
	return WriteNumByte;
}


static int sitronix_ts_IspGetStatus(struct sitronix_ts_data *ts, uint8_t *status)
{
	int retval = 0;
	struct i2c_msg msg[2];
	uint8_t TransmitData[1], ReceivedData[1];

	UpgradeMsg("%s\n", __FUNCTION__);

	msg[0].addr = ts->client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = TransmitData;
	TransmitData[0] = I2C_ATPAGE_STATUS_ADDR;

	msg[1].addr = ts->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = ReceivedData;
	retval = i2c_transfer(ts->client->adapter, msg, 2);
	if (retval < 0){
		UpgradeMsg("Read I2C_ATPAGE_STATUS_ADDR error (%d)\n", retval);
		return retval;
	}
	*status = ReceivedData[0];
	return 0;
}

static bool sitronix_ts_IspAutoTuneProcess(struct sitronix_ts_data *ts)
{
	int ret = 0;
	struct i2c_msg msg[2];
	uint8_t TransmitData[2], ReceivedData[10];
	uint8_t status = 0;
	int count = 0;

	UpgradeMsg("%s\n", __FUNCTION__);
	mdelay(100);

	msg[0].addr = ts->client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = TransmitData;
	TransmitData[0] = I2C_ATPAGE_DEVICE_CTL_ADDR;

	msg[1].addr = ts->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = ReceivedData;
	ret = i2c_transfer(ts->client->adapter, msg, 2);
	if (ret < 0){
		UpgradeMsg("Read I2C_ATPAGE_DEVICE_CTL_ADDR error (%d)\n", ret);
		return ret;
	}
	ReceivedData[0] |= DEVICE_CTL_AUTO_TUNE;
	
	msg[0].len = 2;
	TransmitData[0] = I2C_ATPAGE_DEVICE_CTL_ADDR;
	TransmitData[1] = ReceivedData[0];
	ret = i2c_transfer(ts->client->adapter, msg, 1);
	if (ret < 0){
		UpgradeMsg("write Auto Tune bit error (%d)\n", ret);
		return ret;
	}
	mdelay(200);
		
	do{
		ret = sitronix_ts_IspGetStatus(ts, &status);
		count++;	
	}while(status && count < 50000);
	if(count == 50000){
		UpgradeMsg("AutoTune timeout\n");
		ret = -EFAULT;	
	}else{
		UpgradeMsg("AutoTune Finish\n");
	}
	return 0;
}


static int sitronix_ts_fw_upgrade_with_auto_tune(struct sitronix_ts_data *ts)
{
	int retval = 0;
	UpgradeMsg("%s\n", __FUNCTION__);
	if((retval = sitronix_ts_IspModeEnable(ts)) < 0)
		return retval;
	UpgradeMsg("Write AutoTune Code\n");
	if((retval = sitronix_ts_FlashWrite(ts, sitronix_ts_gAutoTuneCodeData, sitronix_ts_gAutoTuneCodeFlashAddr, sitronix_ts_gAutoTuneCodeFlashSize)) < 0)
		return retval;
	UpgradeMsg("Write AutoTune Table\n");
	if((retval = sitronix_ts_FlashWrite(ts, sitronix_ts_gAutoTuneTableData, sitronix_ts_gAutoTuneTableFlashAddr, sitronix_ts_gAutoTuneTableFlashSize)) < 0)
		return retval;
	if((retval = sitronix_ts_IspReset(ts)) < 0)
		return retval;
	
	UpgradeMsg("AutoTune process\n");
	if((retval = sitronix_ts_IspAutoTuneProcess(ts)) < 0)
		return retval;

	if((retval = sitronix_ts_IspModeEnable(ts)) < 0)
		return retval;
	UpgradeMsg("Write Touch FW\n");
	if((retval = sitronix_ts_FlashWrite(ts, sitronix_ts_gTouchFwFileData, sitronix_ts_gTouchFwFileFlashAddr, sitronix_ts_gTouchFwFileFlashSize)) < 0)
		return retval;
	if((retval = sitronix_ts_IspReset(ts)) < 0)
		return retval;
	
	return retval;
}

static bool sitronix_ts_getAlgParamOffsetInFlash(struct sitronix_ts_data *ts, int16_t *AlgParam1Offset, int16_t *AlgParam2Offset)
{
	uint8_t temp[2];

	UpgradeMsg("%s\n", __FUNCTION__);
	if(sitronix_ts_FlashRead(ts, temp, ISP_PANEL_PARAM_ADDR + ISP_ALG_PARAM_PTR_OFFSET, 2) < 0)	
		return false;
	*AlgParam1Offset = (temp[1] << 8) | temp[0];
	*AlgParam2Offset = *AlgParam1Offset + ISP_ALG_PARAM2_OFFSET;
	return true;
}
static bool sitronix_ts_upgrade_touch_param(struct sitronix_ts_data *ts)
{
	int16_t AlgParam1Offset, AlgParam2Offset;
	UpgradeMsg("%s\n", __FUNCTION__);
	if(sitronix_ts_getAlgParamOffsetInFlash(ts, &AlgParam1Offset, &AlgParam2Offset)){
		if(sitronix_ts_FlashWrite(ts, sitronix_ts_gTouchParamFileData, AlgParam1Offset, ISP_ALG_PARAM_SIZE >> 1) < 0){
			return false;
		}
		if(sitronix_ts_FlashWrite(ts, &sitronix_ts_gTouchParamFileData[ISP_ALG_PARAM_SIZE >> 1], AlgParam2Offset, ISP_ALG_PARAM_SIZE >> 1) < 0){
			return false;
		}
	}
	return true;
}
static int sitronix_ts_upgrade_touch_fw_and_parameter(struct sitronix_ts_data *ts)
{
	int ret = 0;
	UpgradeMsg("%s\n", __FUNCTION__);

	ret = sitronix_ts_IspModeEnable(ts);
	if(ret < 0)
		return ret;
	sitronix_ts_upgrade_touch_param(ts);

	ret = sitronix_ts_FlashWrite(ts, sitronix_ts_gTouchFwFileData, sitronix_ts_gTouchFwFileFlashAddr, sitronix_ts_gTouchFwFileFlashSize);
	if(ret < 0)
		return ret;
	ret = sitronix_ts_IspReset(ts);
	if(ret < 0)
		return ret;
	return ret;
}
static int sitronix_ts_upgrade_touch_fw(struct sitronix_ts_data *ts)
{
	int ret = 0;
	UpgradeMsg("%s\n", __FUNCTION__);

	ret = sitronix_ts_IspModeEnable(ts);
	if(ret < 0)
		return ret;
	ret = sitronix_ts_FlashWrite(ts, sitronix_ts_gTouchFwFileData, sitronix_ts_gTouchFwFileFlashAddr, sitronix_ts_gTouchFwFileFlashSize);
	if(ret < 0)
		return ret;
	ret = sitronix_ts_IspReset(ts);
	if(ret < 0)
		return ret;
	return ret;
}

static int sitronix_get_fw_revision(struct sitronix_ts_data *ts, uint8_t *pRevision);
long	 sitronix_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int err = 0;
	int retval = 0;
	int buf[2];

	if (_IOC_TYPE(cmd) != SMT_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > SMT_IOC_MAXNR) return -ENOTTY;
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE,(void __user *)arg,\
				 _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ,(void __user *)arg,\
				  _IOC_SIZE(cmd));
	if (err) return -EFAULT;

	switch(cmd) {
		case IOCTL_SMT_GET_DRIVER_REVISION:
			printk("IOCTL_SMT_GET_DRIVER_REVISION\n");
			sitronix_ts_gJNIDataBuffer[0] = SITRONIX_TOUCH_DRIVER_VERSION;
			if(copy_to_user((uint8_t __user *)arg, &sitronix_ts_gJNIDataBuffer[0], 1)){
				printk("fail to get driver version\n");
				retval = -EFAULT;
			}
			break;
		case IOCTL_SMT_GET_FW_REVISION:
			printk("IOCTL_SMT_GET_FW_REVISION\n");
			retval = sitronix_get_fw_revision(sitronix_ts_gpts, &sitronix_ts_gJNIDataBuffer[0]);
			if(retval == 0){
#if TEST
                ((uint32_t *)sitronix_ts_gJNIDataBuffer)[0] = 6;
		        UpgradeMsg("fw revision = %d (%x %x %x %x)\n", (sitronix_ts_gJNIDataBuffer[0] << 24) |  (sitronix_ts_gJNIDataBuffer[1] << 16) | (sitronix_ts_gJNIDataBuffer[2] << 8) | sitronix_ts_gJNIDataBuffer[3], sitronix_ts_gJNIDataBuffer[0], sitronix_ts_gJNIDataBuffer[1], sitronix_ts_gJNIDataBuffer[2], sitronix_ts_gJNIDataBuffer[3]);
#endif
				if(copy_to_user((uint8_t __user *)arg, &sitronix_ts_gJNIDataBuffer[0], 4))
					retval = -EFAULT;
			}else{
				printk("fail to get fw revision\n");
				retval = -EFAULT;
			}
			break;
		case IOCTL_SMT_WRITE_AUTO_TUNE_TABLE_OFFSET:
			printk("IOCTL_SMT_WRITE_AUTO_TUNE_TABLE_OFFSET\n");
			if(copy_from_user(&buf[0], (int __user *)arg, sizeof(int)))
				retval = -EFAULT;
			else{
				printk("Auto Tune Table offset = %d\n", buf[0]);
				if(buf[0] < 0){
					printk("wrong Auto Tune Table offset\n");
					retval = -EINVAL;
				}else
					sitronix_ts_gAutoTuneTableOffset = buf[0];
			}
			break;
		case IOCTL_SMT_WRITE_AUTO_TUNE_TABLE_SIZE:
			printk("IOCTL_SMT_WRITE_AUTO_TUNE_TABLE_SIZE\n");
			if(copy_from_user(&buf[0], (int __user *)arg, sizeof(int)))
				retval = -EFAULT;
			else{
				printk("Auto Tune Table size = %d\n", buf[0]);
				if((buf[0] <= 0) || (buf[0] > SITRONIX_JNI_BUFFER_SIZE)){
					printk("wrong Auto Tune Table size\n");
					retval = -EINVAL;
				}else
					sitronix_ts_gAutoTuneTableSize = buf[0];
			}
			break;
		case IOCTL_SMT_WRITE_AUTO_TUNE_TABLE_DATA:
			printk("IOCTL_SMT_WRITE_AUTO_TUNE_TABLE_DATA\n");

			if((sitronix_ts_gAutoTuneTableSize > 0)&&(sitronix_ts_gAutoTuneTableOffset >= 0)){
				if(copy_from_user(&sitronix_ts_gJNIDataBuffer[0], (uint8_t __user *)arg, sitronix_ts_gAutoTuneTableSize))
					retval = -EFAULT;
				else{
					memcpy(&sitronix_ts_gAutoTuneTableData[sitronix_ts_gAutoTuneTableOffset], &sitronix_ts_gJNIDataBuffer[0], sitronix_ts_gAutoTuneTableSize);
					//for(i = sitronix_ts_gAutoTuneTableOffset; i < (sitronix_ts_gAutoTuneTableOffset)+sitronix_ts_gAutoTuneTableSize; i++)
					//	printk("[%d] = %02x \n", i, sitronix_ts_gAutoTuneTableData[i]);
				}
			}else
				retval = -EINVAL;
			break;
		case IOCTL_SMT_WRITE_AUTO_TUNE_CODE_OFFSET:
			printk("IOCTL_SMT_WRITE_AUTO_TUNE_CODE_OFFSET\n");
			if(copy_from_user(&buf[0], (int __user *)arg, sizeof(int)))
				retval = -EFAULT;
			else{
				printk("Auto Tune Code offset = %d\n", buf[0]);
				if(buf[0] < 0){
					printk("wrong Auto Tune Code offset\n");
					retval = -EINVAL;
				}else
					sitronix_ts_gAutoTuneCodeOffset = buf[0];
			}
			break;
		case IOCTL_SMT_WRITE_AUTO_TUNE_CODE_SIZE:
			printk("IOCTL_SMT_WRITE_AUTO_TUNE_CODE_SIZE\n");
			if(copy_from_user(&buf[0], (int __user *)arg, sizeof(int)))
				retval = -EFAULT;
			else{
				printk("Auto Tune Code size = %d\n", buf[0]);
				if((buf[0] <= 0) || (buf[0] > SITRONIX_JNI_BUFFER_SIZE)){
					printk("wrong Auto Tune Code size\n");
					retval = -EINVAL;
				}else
					sitronix_ts_gAutoTuneCodeSize = buf[0];
			}
			break;
		case IOCTL_SMT_WRITE_AUTO_TUNE_CODE_DATA:
			printk("IOCTL_SMT_WRITE_AUTO_TUNE_CODE_DATA\n");

			if((sitronix_ts_gAutoTuneCodeSize > 0)&&(sitronix_ts_gAutoTuneCodeOffset >= 0)){
				if(copy_from_user(&sitronix_ts_gJNIDataBuffer[0], (uint8_t __user *)arg, sitronix_ts_gAutoTuneCodeSize))
					retval = -EFAULT;
				else{
					memcpy(&sitronix_ts_gAutoTuneCodeData[sitronix_ts_gAutoTuneCodeOffset], &sitronix_ts_gJNIDataBuffer[0], sitronix_ts_gAutoTuneCodeSize);
					//for(i = sitronix_ts_gAutoTuneCodeOffset; i < (sitronix_ts_gAutoTuneCodeOffset)+sitronix_ts_gAutoTuneCodeSize; i++)
					//	printk("[%d] = %02x \n", i, sitronix_ts_gAutoTuneCodeData[i]);
				}
			}else
				retval = -EINVAL;
				
			break;
		case IOCTL_SMT_WRITE_TOUCH_PARAM_FILE_OFFSET:
			printk("IOCTL_SMT_WRITE_TOUCH_PARAM_FILE_OFFSET\n");
			if(copy_from_user(&buf[0], (int __user *)arg, sizeof(int)))
				retval = -EFAULT;
			else{
				printk("Touch param file offset = %d\n", buf[0]);
				if(buf[0] < 0){
					printk("wrong Touch Parameter file offset\n");
					retval = -EINVAL;
				}else
					sitronix_ts_gTouchParamFileOffset = buf[0];
			}
			break;
		case IOCTL_SMT_WRITE_TOUCH_PARAM_FILE_SIZE:
			printk("IOCTL_SMT_WRITE_TOUCH_PARAM_FILE_SIZE\n");
			if(copy_from_user(&buf[0], (int __user *)arg, sizeof(int)))
				retval = -EFAULT;
			else{
				printk("Touch param file size = %d\n", buf[0]);
				if((buf[0] <= 0) || (buf[0] > SITRONIX_JNI_BUFFER_SIZE)){
					printk("wrong touch param file size\n");
					retval = -EINVAL;
				}else
					sitronix_ts_gTouchParamFileSize = buf[0];
			}
			break;
		case IOCTL_SMT_WRITE_TOUCH_PARAM_FILE_DATA:
			printk("IOCTL_SMT_WRITE_TOUCH_PARAM_FILE_DATA\n");
			if((sitronix_ts_gTouchParamFileSize > 0)&&(sitronix_ts_gTouchParamFileOffset >= 0)){
				if(copy_from_user(&sitronix_ts_gJNIDataBuffer[0], (uint8_t __user *)arg, sitronix_ts_gTouchParamFileSize))
					retval = -EFAULT;
				else{
					memcpy(&sitronix_ts_gTouchParamFileData[sitronix_ts_gTouchParamFileOffset], &sitronix_ts_gJNIDataBuffer[0], sitronix_ts_gTouchParamFileSize);
					//for(i = sitronix_ts_gTouchParamFileOffset; i < (sitronix_ts_gTouchParamFileOffset)+sitronix_ts_gTouchParamFileSize; i++)
					//	printk("[%d] = %02x \n", i, sitronix_ts_gTouchParamFileData[i]);
				}
			}else
				retval = -EINVAL;
			break;
		case IOCTL_SMT_WRITE_TOUCH_FW_FILE_OFFSET:
			printk("IOCTL_SMT_WRITE_TOUCH_FW_FILE_OFFSET\n");
			if(copy_from_user(&buf[0], (int __user *)arg, sizeof(int)))
				retval = -EFAULT;
			else{
				printk("Touch fw file offset = %d\n", buf[0]);
				if(buf[0] < 0){
					printk("wrong Touch fw file offset\n");
					retval = -EINVAL;
				}else
					sitronix_ts_gTouchFwFileOffset = buf[0];
			}
			break;
		case IOCTL_SMT_WRITE_TOUCH_FW_FILE_SIZE:
			printk("IOCTL_SMT_WRITE_TOUCH_FW_FILE_SIZE\n");
			if(copy_from_user(&buf[0], (int __user *)arg, sizeof(int)))
				retval = -EFAULT;
			else{
				printk("Touch fw file size = %d\n", buf[0]);
				if((buf[0] <= 0) || (buf[0] > SITRONIX_JNI_BUFFER_SIZE)){
					printk("wrong Touch fw file size\n");
					retval = -EINVAL;
				}else
					sitronix_ts_gTouchFwFileSize = buf[0];
			}
			break;
		case IOCTL_SMT_WRITE_TOUCH_FW_FILE_DATA:
			printk("IOCTL_SMT_WRITE_TOUCH_FW_FILE_DATA\n");
			if((sitronix_ts_gTouchFwFileSize > 0)&&(sitronix_ts_gTouchFwFileOffset >= 0)){
				if(copy_from_user(&sitronix_ts_gJNIDataBuffer[0], (uint8_t __user *)arg, sitronix_ts_gTouchFwFileSize))
					retval = -EFAULT;
				else{
					memcpy(&sitronix_ts_gTouchFwFileData[sitronix_ts_gTouchFwFileOffset], &sitronix_ts_gJNIDataBuffer[0], sitronix_ts_gTouchFwFileSize);
					//for(i = sitronix_ts_gTouchFwFileOffset; i < (sitronix_ts_gTouchFwFileOffset)+sitronix_ts_gTouchFwFileSize; i++)
					//	printk("[%d] = %02x \n", i, sitronix_ts_gTouchFwFileData[i]);
				}
			}else
				retval = -EINVAL;
			break;
		case IOCTL_SMT_FW_UPGRADE_WITH_AUTO_TUNE:
			printk("IOCTL_SMT_FW_UPGRADE_WITH_AUTO_TUNE\n");
			if(copy_from_user(&sitronix_ts_gJNIDataBuffer[0], (uint8_t __user *)arg, 15))
				retval = -EFAULT;
			else{
				sitronix_ts_gAutoTuneCodeFlashSize = (sitronix_ts_gJNIDataBuffer[0] << 8) | sitronix_ts_gJNIDataBuffer[1];
				sitronix_ts_gAutoTuneCodeFlashAddr = (sitronix_ts_gJNIDataBuffer[3] << 8) | sitronix_ts_gJNIDataBuffer[4];
				sitronix_ts_gAutoTuneTableFlashSize = (sitronix_ts_gJNIDataBuffer[5] << 8) | sitronix_ts_gJNIDataBuffer[6];
				sitronix_ts_gAutoTuneTableFlashAddr = (sitronix_ts_gJNIDataBuffer[8] << 8) | sitronix_ts_gJNIDataBuffer[9];
				sitronix_ts_gTouchFwFileFlashSize= (sitronix_ts_gJNIDataBuffer[10] << 8) | sitronix_ts_gJNIDataBuffer[11];
				sitronix_ts_gTouchFwFileFlashAddr = (sitronix_ts_gJNIDataBuffer[13] << 8) | sitronix_ts_gJNIDataBuffer[14];
				printk("auto tune code size = %d, checksum = %02x, addr = 0x%x \n", sitronix_ts_gAutoTuneCodeFlashSize, sitronix_ts_gJNIDataBuffer[2], sitronix_ts_gAutoTuneCodeFlashAddr);
				printk("auto tune table size = %d, checksum = %02x, addr = 0x%x \n", sitronix_ts_gAutoTuneTableFlashSize, sitronix_ts_gJNIDataBuffer[7], sitronix_ts_gAutoTuneTableFlashAddr);
				printk("touch fw file size = %d, checksum = %02x, addr = 0x%x \n", sitronix_ts_gTouchFwFileFlashSize, sitronix_ts_gJNIDataBuffer[12], sitronix_ts_gTouchFwFileFlashAddr);
				if(sitronix_ts_checksum(sitronix_ts_gAutoTuneCodeFlashSize, sitronix_ts_gJNIDataBuffer[2], sitronix_ts_gAutoTuneCodeData)&&sitronix_ts_checksum(sitronix_ts_gAutoTuneTableFlashSize, sitronix_ts_gJNIDataBuffer[7], sitronix_ts_gAutoTuneTableData)&&sitronix_ts_checksum(sitronix_ts_gTouchFwFileFlashSize, sitronix_ts_gJNIDataBuffer[12], sitronix_ts_gTouchFwFileData)){
					retval = sitronix_ts_fw_upgrade_with_auto_tune(sitronix_ts_gpts);
				}else
					retval = EINVAL;
			}
			break;
		case IOCTL_SMT_UPGRADE_TOUCH_FW_AND_PARAM:
			printk("IOCTL_SMT_UPGRADE_TOUCH_FW_AND_PARAM\n");
			if(copy_from_user(&sitronix_ts_gJNIDataBuffer[0], (uint8_t __user *)arg, 10))
				retval = -EFAULT;
			else{
				sitronix_ts_gTouchParamFileFlashSize = (sitronix_ts_gJNIDataBuffer[0] << 8) | sitronix_ts_gJNIDataBuffer[1];
				sitronix_ts_gTouchParamFileFlashAddr = (sitronix_ts_gJNIDataBuffer[3] << 8) | sitronix_ts_gJNIDataBuffer[4];
				sitronix_ts_gTouchFwFileFlashSize= (sitronix_ts_gJNIDataBuffer[5] << 8) | sitronix_ts_gJNIDataBuffer[6];
				sitronix_ts_gTouchFwFileFlashAddr = (sitronix_ts_gJNIDataBuffer[8] << 8) | sitronix_ts_gJNIDataBuffer[9];
				printk("touch param file size = %d, checksum = %02x, addr = 0x%x \n", sitronix_ts_gTouchParamFileFlashSize, sitronix_ts_gJNIDataBuffer[2], sitronix_ts_gTouchParamFileFlashAddr);
				printk("touch fw file size = %d, checksum = %02x, addr = 0x%x \n", sitronix_ts_gTouchFwFileFlashSize, sitronix_ts_gJNIDataBuffer[7], sitronix_ts_gTouchFwFileFlashAddr);
				if(sitronix_ts_checksum(sitronix_ts_gTouchParamFileFlashSize, sitronix_ts_gJNIDataBuffer[2], sitronix_ts_gTouchParamFileData) &&
					sitronix_ts_checksum(sitronix_ts_gTouchFwFileFlashSize, sitronix_ts_gJNIDataBuffer[7], sitronix_ts_gTouchFwFileData))
					retval = sitronix_ts_upgrade_touch_fw_and_parameter(sitronix_ts_gpts);
				else
					retval = -EINVAL;
			}
			break;
		case IOCTL_SMT_UPGRADE_TOUCH_FW:
			printk("IOCTL_SMT_UPGRADE_TOUCH_FW\n");
			if(copy_from_user(&sitronix_ts_gJNIDataBuffer[0], (uint8_t __user *)arg, 5))
				retval = -EFAULT;
			else{
				sitronix_ts_gTouchFwFileFlashSize= (sitronix_ts_gJNIDataBuffer[0] << 8) | sitronix_ts_gJNIDataBuffer[1];
				sitronix_ts_gTouchFwFileFlashAddr = (sitronix_ts_gJNIDataBuffer[3] << 8) | sitronix_ts_gJNIDataBuffer[4];
				printk("touch fw file size = %d, checksum = %02x, addr = 0x%x \n", sitronix_ts_gTouchFwFileFlashSize, sitronix_ts_gJNIDataBuffer[2], sitronix_ts_gTouchFwFileFlashAddr);
				if(sitronix_ts_checksum(sitronix_ts_gTouchFwFileFlashSize, sitronix_ts_gJNIDataBuffer[2], sitronix_ts_gTouchFwFileData))
					retval = sitronix_ts_upgrade_touch_fw(sitronix_ts_gpts);
				else
					retval = -EINVAL;
			}
			break;
		case IOCTL_SMT_DO_AUTOTUNE:
			retval = sitronix_ts_IspAutoTuneProcess(sitronix_ts_gpts);
			break;
		default:
			retval = -ENOTTY;
	}

	return retval;
}
EXPORT_SYMBOL(sitronix_ioctl);
static int sitronix_get_fw_revision(struct sitronix_ts_data *ts, uint8_t *pRevision)
{
	int ret = 0;
	struct i2c_msg msg[2];
	uint8_t start_reg;

	msg[0].addr = ts->client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &start_reg;
	start_reg = FIRMWARE_REVISION_3;

	msg[1].addr = ts->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 4;
	msg[1].buf = pRevision;
	ret = i2c_transfer(ts->client->adapter, msg, 2);
	if (ret < 0){
		UpgradeMsg("read fw revision error (%d)\n", ret);
	}else{
		UpgradeMsg("fw revision = %d (%x %x %x %x)\n", (pRevision[0] << 24) |  (pRevision[1] << 16) | (pRevision[2] << 8) | pRevision[3], pRevision[0], pRevision[1], pRevision[2], pRevision[3]);
	}
	return 0;
}

static int sitronix_ts_get_fw_version(struct i2c_client *client, u32 *ver)
{
	char buf[1];
	int ret;

	buf[0] = 0x0;	//Set Reg. address to 0x0 for reading FW version.
	if ((ret = i2c_master_send(client, buf, 1)) != 1)
		return -EIO;

	//Read 1 byte FW version from Reg. 0x0 set previously.
	if ((ret = i2c_master_recv(client, buf, 1)) != 1)
		return -EIO;

	*ver = (u32) buf[0];

	buf[0] = 0x10;	//Set Reg. address back to 0x10 for coordinates.
	if ((ret = i2c_master_send(client, buf, 1)) != 1)
		return -EIO;

	return 0;
}

static int sitronix_ts_get_fw_revision(struct i2c_client *client, u32 *rev)
{
	char buf[4];
	int ret;

	buf[0] = 0xC;	//Set Reg. address to 0x0 for reading FW version.
	if ((ret = i2c_master_send(client, buf, 1)) != 1)
		return -EIO;

	//Read 1 byte FW version from Reg. 0x0 set previously.
	if ((ret = i2c_master_recv(client, buf, 4)) != 4)
		return -EIO;

	*rev = ((u32)buf[3]);
	*rev |= (((u32)buf[2]) << 8);
	*rev |= (((u32)buf[1]) << 16);
	*rev |= (((u32)buf[0]) << 24);

	buf[0] = 0x10;	//Set Reg. address back to 0x10 for coordinates.
	if ((ret = i2c_master_send(client, buf, 1)) != 1)
		return -EIO;

	return 0;

}

static int sitronix_ts_set_autotune_en(struct i2c_client *client)
{
	char buf[2], device_ctrl;
	int ret;
	
	printk("AutoTune enable\n");
	buf[0] = 0x02;	// Set Reg. address to 0x02 for reading Device Control
	if((ret = i2c_master_send(client, buf, 1)) != 1)
		return -EIO;

	// Read 1 byte Device Control from Reg. 0x02
	if((ret = i2c_master_recv(client, &device_ctrl, 1)) != 1)
		return -EIO;
		
	device_ctrl |= 0x80;
	
	buf[0] = 0x2;	//Set Reg. address to 0x4 for reading XY resolution.
	buf[1] = device_ctrl;
	if ((ret = i2c_master_send(client, buf, 2)) != 2)
		return -EIO;

	mdelay(100);

	buf[0] = 0x10;	//Set Reg. address back to 0x10 for coordinates.
	if ((ret = i2c_master_send(client, buf, 1)) != 1)
		return -EIO;
	
	return 0;
}

static int sitronix_ts_get_status(struct i2c_client *client)
{
	char buf[2], status;
	int ret;
	buf[0] = 0x01;	// Set Reg. address to 0x01 for reading Device Status
	if((ret = i2c_master_send(client, buf, 1)) != 1)
		return -EIO;
	
	// Read 1 byte Device Control from Reg 0x01
	if((ret = i2c_master_recv(client, &status, 1)) != 1)
		return -EIO;

	buf[0] = 0x10;	//Set Reg. address back to 0x10 for coordinates.
	if ((ret = i2c_master_send(client, buf, 1)) != 1)
		return -EIO;

	printk(".");
	return status;
	
}

static int sitronix_ts_get_resolution(struct i2c_client *client, u16 *x_res, u16 *y_res)
{
	char buf[3];
	int ret;

	buf[0] = 0x4;	//Set Reg. address to 0x4 for reading XY resolution.
	if ((ret = i2c_master_send(client, buf, 1)) != 1)
		return -EIO;

	//Read 3 byte XY resolution from Reg. 0x4 set previously.
	if ((ret = i2c_master_recv(client, buf, 3)) != 3)
		return -EIO;

	*x_res = ((buf[0] & 0xF0) << 4) | buf[1];
	*y_res = ((buf[0] & 0x0F) << 8) | buf[2];

	buf[0] = 0x10;	//Set Reg. address back to 0x10 for coordinates.
	if ((ret = i2c_master_send(client, buf, 1)) != 1)
		return -EIO;

	return 0;

}

#if 0
static int sitronix_ts_set_resolution(struct i2c_client *client, u16 x_res, u16 y_res)
{
	char buf[4];
	int ret = 0;

	buf[0] = 0x4;	//Set Reg. address to 0x4 for reading XY resolution.
	buf[1] = ((u8)((x_res & 0x0700) >> 4)) | ((u8)((y_res & 0x0700) >> 8));
	buf[2] = ((u8)(x_res & 0xFF));
	buf[3] = ((u8)(y_res & 0xFF));
	if ((ret = i2c_master_send(client, buf, 4)) != 4)
		return -EIO;

	buf[0] = 0x10;	//Set Reg. address back to 0x10 for coordinates.
	if ((ret = i2c_master_send(client, buf, 1)) != 1)
		return -EIO;

	return 0;
}
#endif

struct point {
	int x;
	int y;
	int valid;
};

#define FLG_FINGER1_KEY_DOWN    (1<<0)
#define FLG_FINGER2_KEY_DOWN    (1<<1)
#define FLG_KEY_DOWN            (FLG_FINGER1_KEY_DOWN | FLG_FINGER2_KEY_DOWN)
int flags;

/* 
 * report_key - report the key if the point is in key area 
 *
 * return:
 *       true: the key is report     false: not a key
 */
static inline int report_key(struct point *point, int which_finger) 
{
	int key;
	static int last_key[2];

	/* key area is in x [30 : 300] and y [520 : 576]  */
	if ((point[which_finger].y > 520) && (point[which_finger].y <= 576)) {
		if((point[which_finger].x >= 30) && (point[which_finger].x <= 75)) 
			key = KEY_MENU;
		
		else if((point[which_finger].x >= 105) && (point[which_finger].x <= 150))
			key = KEY_HOME;
		
		else if((point[which_finger].x >= 180) && (point[which_finger].x <= 225))
			key = KEY_BACK;
		
		else if((point[which_finger].x >= 255) && (point[which_finger].x <= 300))
			key = ST_KEY_SEARCH;
		else
			goto release_key;
		
		/* if the key is already report, skip it */
		if (last_key[which_finger] == key)
			return true;

		/* last key is not release, but a new key come in, release the last key */
		if (unlikely(last_key[which_finger])) {
			if(flags & FLG_KEY_DOWN) {
				TP_DEBUG("Last key release %s.\n", (last_key[which_finger] == KEY_MENU)? "MENU" : 
							    ((last_key[which_finger] == KEY_HOME) ? "HOME" : 
							    ((last_key[which_finger] == KEY_BACK) ? "BACK" : "SEARCH")));
				input_report_key(simulate_key_input, last_key[which_finger], 0);
				input_sync(simulate_key_input);

				last_key[which_finger] = 0;
				flags &= ~((which_finger == 0) ? FLG_FINGER1_KEY_DOWN : FLG_FINGER2_KEY_DOWN) ;
			}
		}

		/* report a new key  */
		TP_DEBUG("Key down %s.\n", (key == KEY_MENU)? "MENU" : 
					 ((key == KEY_HOME) ? "HOME" : 
					 ((key == KEY_BACK) ? "BACK" : "SEARCH")));
		input_report_key(simulate_key_input, key, 1);
		input_sync(simulate_key_input);
		flags |= ((which_finger == 0) ? FLG_FINGER1_KEY_DOWN : FLG_FINGER2_KEY_DOWN) ;
		last_key[which_finger] = key;		
		return true;
	}

release_key:
	if(flags & (which_finger == 0 ? FLG_FINGER1_KEY_DOWN : FLG_FINGER2_KEY_DOWN)) {
		TP_DEBUG("Key release %s.\n", (last_key[which_finger] == KEY_MENU)? "MENU" : 
					    ((last_key[which_finger] == KEY_HOME) ? "HOME" : 
					    ((last_key[which_finger] == KEY_BACK) ? "BACK" : "SEARCH")));
		input_report_key(simulate_key_input, last_key[which_finger], 0);
		input_sync(simulate_key_input);
		last_key[which_finger] = 0;
		flags &= ~((which_finger == 0) ? FLG_FINGER1_KEY_DOWN : FLG_FINGER2_KEY_DOWN) ;

		if(!point[which_finger].valid)
			return true;
	}
	return false;
}

void report_point(struct sitronix_ts_priv *priv)
{
	int ret;
	int i, point_count;
	char buf[8] = {0};					
	stx_report_data_t *pdata = (stx_report_data_t *) buf;   /* use buf as stx_report_data_t type*/
	struct point point[2] = {{0,0},{0,0}};

	mutex_lock(&priv->mutex);
	/* get the firmware data  */
	buf[0] = 0x10;          
	if ((ret = i2c_master_send(priv->client, buf, 1)) != 1) {
		dev_err(&priv->client->dev, "Unable to Send 0x10 to I2C!\n");
		goto out;
	}
	
	if ((ret = i2c_master_recv(priv->client, buf, sizeof(buf))) != sizeof(buf)) {
		dev_err(&priv->client->dev, "Unable to read XY data from I2C!\n");
		goto out;
	}

	for(i = 0, point_count = 0; i < 2; ++i) {		
		if(!pdata->xy_data[i].valid && !(flags & (i ? FLG_FINGER2_KEY_DOWN : FLG_FINGER1_KEY_DOWN))) {
			continue;
		}

		point[i].x = pdata->xy_data[i].x_h << 8 | pdata->xy_data[i].x_l;
		point[i].y = pdata->xy_data[i].y_h << 8 | pdata->xy_data[i].y_l;
		point[i].valid = pdata->xy_data[i].valid;

		if(report_key(point, i)) {
			continue;
		}

		point_count++;

		if(point[i].y <= 520) {
			input_report_abs(priv->input, ABS_MT_TRACKING_ID, i);
			input_report_abs(priv->input, ABS_MT_TOUCH_MAJOR, 1);
			input_report_abs(priv->input, ABS_MT_POSITION_X, point[i].x);
			input_report_abs(priv->input, ABS_MT_POSITION_Y, point[i].y);
			input_mt_sync(priv->input);

			TP_DEBUG("Rep: point[%d] {%d, %d}.\n", i, point[i].x, point[i].y);
		}			
	}

	input_report_key(priv->input, BTN_TOUCH, !!point_count);	

	if(point_count == 0)
		input_mt_sync(priv->input);
	
	input_sync(priv->input);
	
	TP_DEBUG("\n");
out:
	mutex_unlock(&priv->mutex);
}

static void sitronix_ts_work(struct work_struct *work)
{
	struct sitronix_ts_priv *priv =
		container_of(work, struct sitronix_ts_priv, work);
	
	//report_ponit(priv);
	report_point(priv);
	enable_irq(priv->irq); 
}

#ifndef USE_POLL
static irqreturn_t sitronix_ts_isr(int irq, void *dev_data)
{
	struct sitronix_ts_priv *priv = dev_data;
	
//    printk(KERN_DEBUG "sitronix: isr.\n");
	disable_irq_nosync(priv->irq);
	schedule_work(&priv->work);

	return IRQ_HANDLED;
}

#else
static enum hrtimer_restart sitronix_ts_timer_func(struct hrtimer *timer)
{
	struct sitronix_ts_priv *priv =
		 container_of(timer, struct sitronix_ts_priv, timer);
	
	TP_DEBUG("use polling.\n");
	schedule_work(&priv->work);
	hrtimer_start(&priv->timer, ktime_set(0, 400000000), HRTIMER_MODE_REL);//400ms
	
	return HRTIMER_NORESTART;
}
#endif

static int sitronix_ts_open(struct input_dev *dev)
{
	struct sitronix_ts_priv *priv = input_get_drvdata(dev);

	enable_irq(priv->irq);

	return 0;
}

static void sitronix_ts_close(struct input_dev *dev)
{
	struct sitronix_ts_priv *priv = input_get_drvdata(dev);

	disable_irq(priv->irq);
	//cancel_work_sync(&priv->work);
}

static int st1232_isp_erase(struct i2c_client *client, u8 page_num)
{
	u8 data[8];

	data[0] = STX_ISP_ERASE;
	data[1] = 0x0;
	data[2] = page_num;

	if (i2c_master_send(client, data, sizeof(data)) != sizeof(data)) {
		dev_err(&client->dev, "%s(%u): ISP erase page(%u) failed!\n",
						__FUNCTION__, __LINE__, (unsigned int)page_num);
		return -EIO;
	}

	if (i2c_master_recv(client, data, sizeof(data)) != sizeof(data) || data[0] != STX_ISP_READY) {
		dev_err(&client->dev, "%s(%u): ISP read READY failed!\n", __FUNCTION__, __LINE__);
		return -EIO;
	}

	return 0;
}

static int st1232_isp_reset(struct i2c_client *client)
{
	u8 data[8];

	data[0] = STX_ISP_RESET;

	if (i2c_master_send(client, data, sizeof(data)) != sizeof(data)) {
		dev_err(&client->dev, "%s(%u): ISP reset chip failed!\n", __FUNCTION__, __LINE__);
		return -EIO;
	}

	mdelay(200);
	//printk("*************************************** ISP reset ok! ***********************************************\n");
	return 0;
}

static int st1232_jump_to_isp(struct i2c_client *client)
{
	int i;
	u8 signature[] = "STX_FWUP";
	u8 buf[2];

	for (i = 0; i < strlen(signature); i++) {
		buf[0] = 0x0;
		buf[1] = signature[i];
		if (i2c_master_send(client, buf, 2) != 2) {
			dev_err(&client->dev, "%s(%u): Unable to write ISP STX_FWUP!\n", __FUNCTION__, __LINE__);
			return -EIO;
		}
		msleep(100);
	}

	//printk("*************************************** Jump to ISP ok! ***********************************************\n");

	return 0;
}

static int st1232_isp_read_page(struct i2c_client *client, char *page_buf, u8 page_num)
{
	u8 data[8];
	u32 rlen;

	memset(data, 0, sizeof(data));
	memset(page_buf, 0, ST1232_ISP_PAGE_SIZE);
	data[0] = STX_ISP_READ_FLASH;
	data[2] = page_num;
	if (i2c_master_send(client, data, sizeof(data)) != sizeof(data)) {
		dev_err(&client->dev, "%s(%u): ISP read flash failed!\n", __FUNCTION__, __LINE__);
		return -EIO;
	}
	rlen = 0;
	while (rlen < ST1232_ISP_PAGE_SIZE) {
		if (i2c_master_recv(client, (page_buf+rlen), sizeof(data)) != sizeof(data)) {
			dev_err(&client->dev, "%s(%u): ISP read data failed!\n", __FUNCTION__, __LINE__);
			return -EIO;
		}
		rlen += 8;
	}


	return ST1232_ISP_PAGE_SIZE;
}

static u16 st1232_isp_cksum(char *page_buf)
{
	u16 cksum = 0;
	int i;
	for (i = 0; i < ST1232_ISP_PAGE_SIZE; i++) {
		cksum += (u16) page_buf[i];
	}
	return cksum;
}

static int st1232_isp_write_page(struct i2c_client *client, char *page_buf, u8 page_num)
{
	u8 data[8];
	int wlen;
	u32 len;
	u16 cksum;

	if (st1232_isp_erase(client, page_num) < 0) {
		return -EIO;
	}

	cksum = st1232_isp_cksum(page_buf);

	data[0] = STX_ISP_WRITE_FLASH;
	data[2] = page_num;
	data[4] = (cksum & 0xFF);
	data[5] = ((cksum & 0xFF) >> 8);

	if (i2c_master_send(client, data, sizeof(data)) != sizeof(data)) {
		dev_err(&client->dev, "%s(%u): ISP write page failed!\n", __FUNCTION__, __LINE__);
		return -EIO;
	}

	data[0] = STX_ISP_SEND_DATA;
	wlen = ST1232_ISP_PAGE_SIZE;
	len = 0;
	while (wlen>0) {
		len = (wlen < 7) ? wlen : 7;
		memcpy(&data[1], page_buf, len);

		if (i2c_master_send(client, data, sizeof(data)) != sizeof(data)) {
			dev_err(&client->dev, "%s(%u): ISP send data failed!\n", __FUNCTION__, __LINE__);
			return -EIO;
		}

		wlen -= 7;
		page_buf += 7;
	}

	if (i2c_master_recv(client, data, sizeof(data)) != sizeof(data) || data[0] != STX_ISP_READY) {
		dev_err(&client->dev, "%s(%u): ISP read READY failed!\n", __FUNCTION__, __LINE__);
		return -EIO;
	}

	return ST1232_ISP_PAGE_SIZE;
}

static int st1232_isp_read_flash(struct i2c_client *client, char *buf, loff_t off, size_t count)
{
	u32 page_num, page_off;
	u32 len = 0, rlen = 0;

	page_num = off / ST1232_ISP_PAGE_SIZE;
	page_off = off % ST1232_ISP_PAGE_SIZE;

	if (page_off) {

		if ((len = st1232_isp_read_page(client, isp_page_buf, page_num)) < 0)
			return len;

		len -= page_off;

		len = (count > len ? len : count);

		memcpy(buf, (isp_page_buf + page_off), len);
		buf += len;
		count -= len;
		rlen += len;
		page_num++;
	}

	while (count) {
		if ((len = st1232_isp_read_page(client, isp_page_buf, page_num)) < 0)
			return len;

		len = (count > len ? len : count);

		memcpy(buf, isp_page_buf, len);

		buf += len;
		count -= len;
		rlen += len;
		page_num++;
	}

	return rlen;
}

static ssize_t st1232_flash_read(struct file *filp,struct kobject *kobj, struct bin_attribute *attr,
				  char *buf, loff_t off, size_t count)
{
	struct i2c_client *client = kobj_to_i2c_client(kobj);
	int rc;

	dev_dbg(&client->dev, "%s(%u): buf=%p, off=%lli, count=%zi)\n", __FUNCTION__, __LINE__, buf, off, count);

	if (off >= ST1232_FLASH_SIZE)
		return 0;

	if (off + count > ST1232_FLASH_SIZE)
		count = ST1232_FLASH_SIZE - off;

	rc = st1232_isp_read_flash(client, buf, off, count);

	if (rc < 0)
		return -EIO;

	return rc;
}

static int st1232_isp_write_flash(struct i2c_client *client, char *buf, loff_t off, size_t count)
{
	u8 page_num, page_off;
	u32 len, wlen = 0;
	int rc;

	page_num = off / ST1232_ISP_PAGE_SIZE;
	page_off = off % ST1232_ISP_PAGE_SIZE;

	if (page_off) {

		// Start RMW.
		// Read page.
		if ((rc = st1232_isp_read_page(client, isp_page_buf, page_num)) < 0)
			return rc;

		len = ST1232_ISP_PAGE_SIZE - page_off;
		// Modify data.
		memcpy((isp_page_buf+page_off), buf, len);

		// Write back page.
		st1232_isp_write_page(client, isp_page_buf, page_num);

		buf += len;
		count -= len;
		wlen += len;
	}

	while (count) {
		if (count >= ST1232_ISP_PAGE_SIZE) {
			len = ST1232_ISP_PAGE_SIZE;
			memcpy(isp_page_buf, buf, len);

			if ((rc = st1232_isp_write_page(client, isp_page_buf, page_num)) < 0)
				return rc;

			buf += len;
			count -= len;
			wlen += len;
		} else {
			// Start RMW.
			// Read page.
			if ((rc = st1232_isp_read_page(client, isp_page_buf, page_num)) < 0)
				return rc;

			len = count;
			// Modify data.
			memcpy(isp_page_buf, buf, len);

			// Write back page.
			if ((rc = st1232_isp_write_page(client, isp_page_buf, page_num)) < 0)
				return rc;

			buf += len;
			count -= len;
			wlen += len;
		}
		page_num++;
	}

	return wlen;
}

static ssize_t st1232_flash_write(struct file *filp, struct kobject *kobj, struct bin_attribute *attr,
				   char *buf, loff_t off, size_t count)
{
	struct i2c_client *client = kobj_to_i2c_client(kobj);
	int rc;

	dev_dbg(&client->dev, "%s(%u): buf=%p, off=%lli, count=%zi)\n", __FUNCTION__, __LINE__, buf, off, count);

	if (off >= ST1232_FLASH_SIZE)
		return -ENOSPC;

	if (off + count > ST1232_FLASH_SIZE)
		count = ST1232_FLASH_SIZE - off;

	rc = st1232_isp_write_flash(client, buf, off, count);

	if (rc < 0)
		return -EIO;

	return rc;
}

static struct bin_attribute st1232_flash_bin_attr = {
	.attr = {
		.name = "flash",
		.mode = S_IRUGO | S_IWUSR,
	},
	.size = ST1232_FLASH_SIZE,
	.read = st1232_flash_read,
	.write = st1232_flash_write,
};

static ssize_t st1232_panel_config_read(struct file *filp, struct kobject *kobj, struct bin_attribute *attr,
				  char *buf, loff_t off, size_t count)
{
	return count;
}


static ssize_t st1232_panel_config_write(struct file *filp, struct kobject *kobj, struct bin_attribute *attr,
				   char *buf, loff_t off, size_t count)
{
	struct i2c_client *client = kobj_to_i2c_client(kobj);
	int i;
	u8 page_num;
	u32 len;

	page_num = ST1232_ROM_PARAM_ADR / ST1232_ISP_PAGE_SIZE;
	dev_dbg(&client->dev, "%s(%u): buf=%p, off=%lli, count=%zi)\n", __FUNCTION__, __LINE__, buf, off, count);
	
	printk("input panel config\n");
	for(i = 0 ; i < 32 ; i++) {
		printk("%02x %02x %02x %02x %02x %02x %02x %02x ", buf[i*16], buf[i*16+1], buf[i*16+2], buf[i*16+3], buf[i*16+4], buf[i*16+5], buf[i*16+6], buf[i*16+7]);
		printk("%02x %02x %02x %02x %02x %02x %02x %02x\n", buf[i*16+8], buf[i*16+9], buf[i*16+10], buf[i*16+11], buf[i*16+12], buf[i*16+13], buf[i*16+14], buf[i*16+15]);
	}
	
	//Write back
	if ((len = st1232_isp_write_page(client, buf, page_num)) < 0)
				return -EIO;
	return count;
}

static struct bin_attribute st1232_panel_bin_attr = {
	.attr = {
		.name = "panel_config",
		.mode = S_IRUGO | S_IWUSR,
	},
	.size = ST1232_ISP_PAGE_SIZE,
	.read = st1232_panel_config_read,
	.write = st1232_panel_config_write,
};

static ssize_t sitronix_ts_isp_ctrl_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sitronix_ts_priv *priv = i2c_get_clientdata(client);
	
	return sprintf(buf, "%d\n", priv->isp_enabled);
}

static ssize_t sitronix_ts_isp_ctrl_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sitronix_ts_priv *priv = i2c_get_clientdata(client);
	int enabled = 0;

	sscanf(buf, "%x", &enabled);
	if (priv->isp_enabled && !enabled) {
		//ISP Reset.
		priv->isp_enabled = false;
		st1232_isp_reset(client);
	} else if (!priv->isp_enabled && enabled) {
		//Jump to ISP.
		priv->isp_enabled = true;
		st1232_jump_to_isp(client);
	}

	return count;
}

static DEVICE_ATTR(isp_ctrl, 0644, sitronix_ts_isp_ctrl_show, sitronix_ts_isp_ctrl_store);

static ssize_t sitronix_ts_revision_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	u32 rev;
	int err;
	
	if ((err = sitronix_ts_get_fw_revision(client, &rev))) {
		dev_err(&client->dev, "Unable to get FW revision!\n");
		return 0;
	}

	return sprintf(buf, "%u\n", rev);
}

static DEVICE_ATTR(revision, 0644, sitronix_ts_revision_show, NULL);

static ssize_t sitronix_ts_struct_version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sitronix_ts_priv *priv = i2c_get_clientdata(client);
	u8 page_num;
	u32 len;
	
	page_num = ST1232_ROM_PARAM_ADR / ST1232_ISP_PAGE_SIZE;
	
	//Jump to ISP.
	if(!priv->isp_enabled) {
		priv->isp_enabled = true;
		st1232_jump_to_isp(client);
	}
	
	//Read data from ROM
	if ((len = st1232_isp_read_page(client, isp_page_buf, page_num)) < 0)
			return -EIO;

	//ISP Reset.
	priv->isp_enabled = false;
	st1232_isp_reset(client);

	return sprintf(buf, "%u\n", isp_page_buf[1]);
}

static DEVICE_ATTR(struct_version, 0644, sitronix_ts_struct_version_show, NULL);

/*
 * sitronix data threshold show & store
 */
static ssize_t sitronix_ts_data_threshold_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sitronix_ts_priv *priv = i2c_get_clientdata(client);
	u8 x_shift, y_shift, k_shift;
	u8 x_offset, y_offset, k_offset;
	u8 page_num;
	u32 len;

	struct config_param_v0 *pROM_v0;
	struct config_param_v1 *pROM_v1;
	
	page_num = ST1232_ROM_PARAM_ADR / ST1232_ISP_PAGE_SIZE;
	
	//Jump to ISP.
	if(!priv->isp_enabled) {
		priv->isp_enabled = true;
		st1232_jump_to_isp(client);
	}
	
	//Read data from ROM
	if ((len = st1232_isp_read_page(client, isp_page_buf, page_num)) < 0)
			return -EIO;

	if(priv->struct_version == 0x00) {
		pROM_v0 = (struct config_param_v0 *)(&isp_page_buf);
		x_shift = pROM_v0->data_threshold_shift_x;
		x_offset = pROM_v0->data_threshold_offset_x;
		y_shift = pROM_v0->data_threshold_shift_y;
		y_offset = pROM_v0->data_threshold_offset_y;
		k_shift = pROM_v0->data_threshold_shift_k;
		k_offset = pROM_v0->data_threshold_offset_k;
	} else {
		pROM_v1 = (struct config_param_v1 *)(&isp_page_buf);
		x_shift = pROM_v1->data_threshold_shift_x;
		x_offset = pROM_v1->data_threshold_offset_x;
		y_shift = pROM_v1->data_threshold_shift_y;
		y_offset = pROM_v1->data_threshold_offset_y;
		k_shift = pROM_v1->data_threshold_shift_k;
		k_offset = pROM_v1->data_threshold_offset_k;
	}
	//ISP Reset.
	priv->isp_enabled = false;
	st1232_isp_reset(client);
	
	return sprintf(buf, "%u %u %u %u %u %u\n", x_shift, x_offset, y_shift, y_offset, k_shift, k_offset);
}

static ssize_t sitronix_ts_data_threshold_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sitronix_ts_priv *priv = i2c_get_clientdata(client);
	u32 x_shift, y_shift, k_shift;
	u32 x_offset, y_offset, k_offset;
	u8 page_num;
	int len;
	struct config_param_v0 *pROM_v0;
	struct config_param_v1 *pROM_v1;
	
	page_num = ST1232_ROM_PARAM_ADR / ST1232_ISP_PAGE_SIZE;
	sscanf(buf, "%u %u %u %u %u %u", &x_shift, &x_offset, &y_shift, &y_offset, &k_shift, &k_offset);

	//Jump to ISP.
	if(!priv->isp_enabled) {
		priv->isp_enabled = true;
		st1232_jump_to_isp(client);
	}
	
	//Read data from ROM
	if ((len = st1232_isp_read_page(client, isp_page_buf, page_num)) < 0)
			return -EIO;

	if(priv->struct_version == 0x00) {
		pROM_v0 = (struct config_param_v0 *)(&isp_page_buf);
		pROM_v0->data_threshold_shift_x = x_shift&0xFF;
		pROM_v0->data_threshold_offset_x = x_offset&0xFF;
		pROM_v0->data_threshold_shift_y = y_shift&0xFF;
		pROM_v0->data_threshold_offset_y = y_offset&0xFF;
		pROM_v0->data_threshold_shift_k = k_shift&0xFF;
		pROM_v0->data_threshold_offset_k = k_offset&0xFF;
	} else {
		pROM_v1 = (struct config_param_v1 *)(&isp_page_buf);
		pROM_v1->data_threshold_shift_x = x_shift&0xFF;
		pROM_v1->data_threshold_offset_x = x_offset&0xFF;
		pROM_v1->data_threshold_shift_y = y_shift&0xFF;
		pROM_v1->data_threshold_offset_y = y_offset&0xFF;
		pROM_v1->data_threshold_shift_k = k_shift&0xFF;
		pROM_v1->data_threshold_offset_k = k_offset&0xFF;
	}
	//Write back
	if ((len = st1232_isp_write_page(client, isp_page_buf, page_num)) < 0)
				return -EIO;
	
	//ISP Reset.
	priv->isp_enabled = false;
	st1232_isp_reset(client);

	return count;
}

static DEVICE_ATTR(data_threshold, 0644, sitronix_ts_data_threshold_show, sitronix_ts_data_threshold_store);

/*
 * sitronix point threshold show & store
 */
static ssize_t sitronix_ts_point_threshold_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sitronix_ts_priv *priv = i2c_get_clientdata(client);
	u8 shift, offset;
	u8 page_num;
	int len;
	struct config_param_v0 *pROM_v0;
	struct config_param_v1 *pROM_v1;
	
	page_num = ST1232_ROM_PARAM_ADR / ST1232_ISP_PAGE_SIZE;
	
	//Jump to ISP.
	if(!priv->isp_enabled) {
		priv->isp_enabled = true;
		st1232_jump_to_isp(client);
	}
	
	//Read data from ROM
	if ((len = st1232_isp_read_page(client, isp_page_buf, page_num)) < 0)
			return -EIO;

	if(priv->struct_version == 0x00) {
		pROM_v0 = (struct config_param_v0 *)(&isp_page_buf);
		shift = pROM_v0->pt_threshold_shift;
		offset = pROM_v0->pt_threshold_offset;
	} else {
		pROM_v1 = (struct config_param_v1 *)(&isp_page_buf);
		shift = pROM_v1->pt_threshold_shift;
		offset = pROM_v1->pt_threshold_offset;
	}
	
	//ISP Reset.
	priv->isp_enabled = false;
	st1232_isp_reset(client);
	
	return sprintf(buf, "%u %u\n", shift, offset);
}

static ssize_t sitronix_ts_point_threshold_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sitronix_ts_priv *priv = i2c_get_clientdata(client);
	u32 shift, offset;
	u8 page_num;
	int len;
	struct config_param_v0 *pROM;
	
	page_num = ST1232_ROM_PARAM_ADR / ST1232_ISP_PAGE_SIZE;
	sscanf(buf, "%u %u", &shift, &offset);
	
	//Jump to ISP.
	if(!priv->isp_enabled) {
		priv->isp_enabled = true;
		st1232_jump_to_isp(client);
	}
	
	//Read data from ROM
	if ((len = st1232_isp_read_page(client, isp_page_buf, page_num)) < 0)
			return -EIO;
	pROM = (struct config_param_v0 *)(&isp_page_buf);
	pROM->pt_threshold_shift = shift&0xFF;
	pROM->pt_threshold_offset = offset&0xFF;
	
	//Write back
	if ((len = st1232_isp_write_page(client, isp_page_buf, page_num)) < 0)
				return -EIO;
	
	//ISP Reset.
	priv->isp_enabled = false;
	st1232_isp_reset(client);
	return count;
}

static DEVICE_ATTR(point_threshold, 0644, sitronix_ts_point_threshold_show, sitronix_ts_point_threshold_store);

/*
 * sitronix peak threshold show & store
 */
static ssize_t sitronix_ts_peak_threshold_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sitronix_ts_priv *priv = i2c_get_clientdata(client);
	u8 x_shift, y_shift;
	u8 x_offset, y_offset;
	u8 page_num;
	int len;
	struct config_param_v0 *pROM_v0;
	struct config_param_v1 *pROM_v1;
	
	page_num = ST1232_ROM_PARAM_ADR / ST1232_ISP_PAGE_SIZE;
	
	//Jump to ISP.
	if(!priv->isp_enabled) {
		priv->isp_enabled = true;
		st1232_jump_to_isp(client);
	}
	
	//Read data from ROM
	if ((len = st1232_isp_read_page(client, isp_page_buf, page_num)) < 0)
			return -EIO;

	if(priv->struct_version == 0x00)
	{
		pROM_v0 = (struct config_param_v0 *)(&isp_page_buf);
		x_shift = pROM_v0->peak_threshold_shift_x;
		x_offset = pROM_v0->peak_threshold_offset_x;
		y_shift = pROM_v0->peak_threshold_shift_y;
		y_offset = pROM_v0->peak_threshold_offset_y;
	} else {
		pROM_v1 = (struct config_param_v1 *)(&isp_page_buf);
		x_shift = pROM_v1->peak_threshold_shift_x;
		x_offset = pROM_v1->peak_threshold_offset_x;
		y_shift = pROM_v1->peak_threshold_shift_y;
		y_offset = pROM_v1->peak_threshold_offset_y;
	}
	
	//ISP Reset.
	priv->isp_enabled = false;
	st1232_isp_reset(client);
	
	return sprintf(buf, "%u %u %u %u\n", x_shift, x_offset, y_shift, y_offset);
}

static ssize_t sitronix_ts_peak_threshold_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sitronix_ts_priv *priv = i2c_get_clientdata(client);
	u32 x_shift, y_shift;
	u32 x_offset, y_offset;
	u8 page_num;
	int len;
	struct config_param_v0 *pROM_v0;
	struct config_param_v1 *pROM_v1;
	
	page_num = ST1232_ROM_PARAM_ADR / ST1232_ISP_PAGE_SIZE;
	sscanf(buf, "%u %u %u %u", &x_shift, &x_offset, &y_shift, &y_offset);
	
	//Jump to ISP.
	if(!priv->isp_enabled) {
		priv->isp_enabled = true;
		st1232_jump_to_isp(client);
	}
	
	//Read data from ROM
	if ((len = st1232_isp_read_page(client, isp_page_buf, page_num)) < 0)
			return -EIO;

	if(priv->struct_version == 0x00) {
		pROM_v0 = (struct config_param_v0 *)(&isp_page_buf);
		pROM_v0->peak_threshold_shift_x = x_shift&0xFF;
		pROM_v0->peak_threshold_offset_x = x_offset&0xFF;
		pROM_v0->peak_threshold_shift_y = y_shift&0xFF;
		pROM_v0->peak_threshold_offset_y = y_offset&0xFF;
	} else {
		pROM_v1 = (struct config_param_v1 *)(&isp_page_buf);
		pROM_v1->peak_threshold_shift_x = x_shift&0xFF;
		pROM_v1->peak_threshold_offset_x = x_offset&0xFF;
		pROM_v1->peak_threshold_shift_y = y_shift&0xFF;
		pROM_v1->peak_threshold_offset_y = y_offset&0xFF;
	}
	
	//Write back
	if ((len = st1232_isp_write_page(client, isp_page_buf, page_num)) < 0)
				return -EIO;
	
	//ISP Reset.
	priv->isp_enabled = false;
	st1232_isp_reset(client);
	return count;
}

static DEVICE_ATTR(peak_threshold, 0644, sitronix_ts_peak_threshold_show, sitronix_ts_peak_threshold_store);

/*
 * sitronix mutual threshold show & store
 */
static ssize_t sitronix_ts_mutual_threshold_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sitronix_ts_priv *priv = i2c_get_clientdata(client);
	u8 shift, offset;
	u8 page_num;
	int len;
	struct config_param_v0 *pROM_v0;
	struct config_param_v1 *pROM_v1;
	
	page_num = ST1232_ROM_PARAM_ADR / ST1232_ISP_PAGE_SIZE;
	
	//Jump to ISP.
	if(!priv->isp_enabled) {
		priv->isp_enabled = true;
		st1232_jump_to_isp(client);
	}
	
	//Read data from ROM
	if ((len = st1232_isp_read_page(client, isp_page_buf, page_num)) < 0)
			return -EIO;

	if(priv->struct_version == 0x00)
	{
		pROM_v0 = (struct config_param_v0 *)(&isp_page_buf);
		shift = pROM_v0->mutual_threshold_shift;
		offset = pROM_v0->mutual_threshold_offset;
	} else {
		pROM_v1 = (struct config_param_v1 *)(&isp_page_buf);
		shift = pROM_v1->mutual_threshold_shift;
		offset = pROM_v1->mutual_threshold_offset;
	}
	
	//ISP Reset.
	priv->isp_enabled = false;
	st1232_isp_reset(client);
	
	return sprintf(buf, "%u %u\n", shift, offset);
}

static ssize_t sitronix_ts_mutual_threshold_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sitronix_ts_priv *priv = i2c_get_clientdata(client);
	u32 shift, offset;
	u8 page_num;
	int len;
	struct config_param_v0 *pROM_v0;
	struct config_param_v1 *pROM_v1;
	
	page_num = ST1232_ROM_PARAM_ADR / ST1232_ISP_PAGE_SIZE;
	sscanf(buf, "%u %u", &shift, &offset);
	
	//Jump to ISP.
	if(!priv->isp_enabled) {
		priv->isp_enabled = true;
		st1232_jump_to_isp(client);
	}
	
	//Read data from ROM
	if ((len = st1232_isp_read_page(client, isp_page_buf, page_num)) < 0)
			return -EIO;

	if(priv->struct_version == 0x00) {
		pROM_v0 = (struct config_param_v0 *)(&isp_page_buf);
		pROM_v0->mutual_threshold_shift = shift&0xFF;
		pROM_v0->mutual_threshold_offset = offset&0xFF;
	} else {
		pROM_v1 = (struct config_param_v1 *)(&isp_page_buf);
		pROM_v1->mutual_threshold_shift = shift&0xFF;
		pROM_v1->mutual_threshold_offset = offset&0xFF;
	}
	
	//Write back
	if ((len = st1232_isp_write_page(client, isp_page_buf, page_num)) < 0)
				return -EIO;
	
	//ISP Reset.
	priv->isp_enabled = false;
	st1232_isp_reset(client);
	return count;
}

static DEVICE_ATTR(mutual_threshold, 0644, sitronix_ts_mutual_threshold_show, sitronix_ts_mutual_threshold_store);

/*
 * sitronix range filter show & store
 */
static ssize_t sitronix_ts_range_filter_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sitronix_ts_priv *priv = i2c_get_clientdata(client);
	u8 rate;
	u16 range1,range2;
	u8 page_num;
	int len;
	struct config_param_v0 *pROM_v0;
	struct config_param_v1 *pROM_v1;
	
	page_num = ST1232_ROM_PARAM_ADR / ST1232_ISP_PAGE_SIZE;
	
	//Jump to ISP.
	if(!priv->isp_enabled) {
		priv->isp_enabled = true;
		st1232_jump_to_isp(client);
	}
	
	//Read data from ROM
	if ((len = st1232_isp_read_page(client, isp_page_buf, page_num)) < 0)
			return -EIO;

	if(priv->struct_version == 0x00) {
		pROM_v0 = (struct config_param_v0 *)(&isp_page_buf);
		range1 = ((pROM_v0->filter_range_1 & 0xFF)<<8) | (pROM_v0->filter_range_1>>8);
		range2 = ((pROM_v0->filter_range_2 & 0xFF)<<8) | (pROM_v0->filter_range_2>>8);
		rate = pROM_v0->filter_rate;
	} else {
		pROM_v1 = (struct config_param_v1 *)(&isp_page_buf);
		range1 = ((pROM_v1->filter_range_1 & 0xFF)<<8) | (pROM_v1->filter_range_1>>8);
		range2 = ((pROM_v1->filter_range_2 & 0xFF)<<8) | (pROM_v1->filter_range_2>>8);
		rate = pROM_v1->filter_rate;
	}
	
	//ISP Reset.
	priv->isp_enabled = false;
	st1232_isp_reset(client);

	return sprintf(buf, "%u %u %u\n", range1, range2, rate);
}

static ssize_t sitronix_ts_range_filter_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sitronix_ts_priv *priv = i2c_get_clientdata(client);
	u32 range1,range2,rate;
	u8 page_num;
	int len;
	struct config_param_v0 *pROM_v0;
	struct config_param_v1 *pROM_v1;
	
	page_num = ST1232_ROM_PARAM_ADR / ST1232_ISP_PAGE_SIZE;
	sscanf(buf, "%u %u %u", &range1, &range2, &rate);
	
	//Jump to ISP.
	if(!priv->isp_enabled) {
		priv->isp_enabled = true;
		st1232_jump_to_isp(client);
	}
	
	//Read data from ROM
	if ((len = st1232_isp_read_page(client, isp_page_buf, page_num)) < 0)
			return -EIO;

	if(priv->struct_version == 0x00) {
		pROM_v0 = (struct config_param_v0 *)(&isp_page_buf);
		pROM_v0->filter_range_1 = ((range1&0xFF)<<8) | ((range1>>8)&0xFF);
		pROM_v0->filter_range_2 = ((range2&0xFF)<<8) | ((range2>>8)&0xFF);
		pROM_v0->filter_rate = rate&0xFF;
	} else {
		pROM_v1 = (struct config_param_v1 *)(&isp_page_buf);
		pROM_v1->filter_range_1 = ((range1&0xFF)<<8) | ((range1>>8)&0xFF);
		pROM_v1->filter_range_2 = ((range2&0xFF)<<8) | ((range2>>8)&0xFF);
		pROM_v1->filter_rate = rate&0xFF;
	}
	
	//Write back
	if ((len = st1232_isp_write_page(client, isp_page_buf, page_num)) < 0)
				return -EIO;
	
	//ISP Reset.
	priv->isp_enabled = false;
	st1232_isp_reset(client);
	return count;
}

static DEVICE_ATTR(range_filter, 0644 , sitronix_ts_range_filter_show, sitronix_ts_range_filter_store);

/*
 * sitronix barX filter show & store
 */
static ssize_t sitronix_ts_barX_filter_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sitronix_ts_priv *priv = i2c_get_clientdata(client);
	u8 RAW , RAW_2Peak , Delta;
	u8 page_num;
	int len;
	struct config_param_v1 *pROM_v1;
	if(priv->struct_version != 0x00) {
		page_num = ST1232_ROM_PARAM_ADR / ST1232_ISP_PAGE_SIZE;
	
		//Jump to ISP.
		if(!priv->isp_enabled) {
			priv->isp_enabled = true;
			st1232_jump_to_isp(client);
		}
	
		//Read data from ROM
		if ((len = st1232_isp_read_page(client, isp_page_buf, page_num)) < 0)
			return -EIO;

		pROM_v1 = (struct config_param_v1 *)(&isp_page_buf);
		RAW = pROM_v1->Bar_X_RAW;
		RAW_2Peak = pROM_v1->Bar_X_Raw_2_Peak;
		Delta = pROM_v1->Bar_X_Delta;
	
		//ISP Reset.
		priv->isp_enabled = false;
		st1232_isp_reset(client);

		return sprintf(buf, "%u %u %u\n", RAW, RAW_2Peak, Delta);
	} else {
		return sprintf(buf, "Not Support!!\n");
	}
}

static ssize_t sitronix_ts_barX_filter_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sitronix_ts_priv *priv = i2c_get_clientdata(client);
	u32 RAW , RAW_2Peak , Delta;
	u8 page_num;
	int len;
	struct config_param_v1 *pROM_v1;
	
	if(priv->struct_version != 0x00) {
		page_num = ST1232_ROM_PARAM_ADR / ST1232_ISP_PAGE_SIZE;
		sscanf(buf, "%u %u %u", &RAW, &RAW_2Peak, &Delta);
	
		//Jump to ISP.
		if(!priv->isp_enabled) {
			priv->isp_enabled = true;
			st1232_jump_to_isp(client);
		}
	
		//Read data from ROM
		if ((len = st1232_isp_read_page(client, isp_page_buf, page_num)) < 0)
			return -EIO;

		pROM_v1 = (struct config_param_v1 *)(&isp_page_buf);
		pROM_v1->Bar_X_RAW = RAW&0xFF;
		pROM_v1->Bar_X_Raw_2_Peak = RAW_2Peak&0xFF;
		pROM_v1->Bar_X_Delta = Delta&0xFF;
	
	
		//Write back
		if ((len = st1232_isp_write_page(client, isp_page_buf, page_num)) < 0)
				return -EIO;
	
		//ISP Reset.
		priv->isp_enabled = false;
		st1232_isp_reset(client);
	}		
	return count;
}

static DEVICE_ATTR(barX_filter, 0644 , sitronix_ts_barX_filter_show, sitronix_ts_barX_filter_store);


/*
 * sitronix barY filter show & store
 */
static ssize_t sitronix_ts_barY_filter_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sitronix_ts_priv *priv = i2c_get_clientdata(client);
	u16 RAW_2Peak;
	u8  shift , offset;
	u8 page_num;
	int len;
	struct config_param_v1 *pROM_v1;
	
	if(priv->struct_version != 0x00) {
		page_num = ST1232_ROM_PARAM_ADR / ST1232_ISP_PAGE_SIZE;
	
		//Jump to ISP.
		if(!priv->isp_enabled) {
			priv->isp_enabled = true;
			st1232_jump_to_isp(client);
		}
	
		//Read data from ROM
		if ((len = st1232_isp_read_page(client, isp_page_buf, page_num)) < 0)
				return -EIO;

		pROM_v1 = (struct config_param_v1 *)(&isp_page_buf);
		RAW_2Peak =( (pROM_v1->Bar_Y_Delta_2_Peak & 0xFF)<<8) | (pROM_v1->Bar_Y_Delta_2_Peak>>8);
		shift = pROM_v1->peak_threshold_shift_y;
		offset = pROM_v1->peak_threshold_offset_y;
	
		//ISP Reset.
		priv->isp_enabled = false;
		st1232_isp_reset(client);
	
		return sprintf(buf, "%u %u %u\n", RAW_2Peak, shift, offset);
	} else {
		return sprintf(buf, "Not Support!!\n");
	}
}

static ssize_t sitronix_ts_barY_filter_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sitronix_ts_priv *priv = i2c_get_clientdata(client);
	u32 RAW_2Peak , shift , offset;
	u8 page_num;
	int len;
	struct config_param_v1 *pROM_v1;
	
	if(priv->struct_version != 0x00) {
		page_num = ST1232_ROM_PARAM_ADR / ST1232_ISP_PAGE_SIZE;
		sscanf(buf, "%u %u %u", &RAW_2Peak, &shift, &offset);
	
		//Jump to ISP.
		if(!priv->isp_enabled) {
			priv->isp_enabled = true;
			st1232_jump_to_isp(client);
		}
	
		//Read data from ROM
		if ((len = st1232_isp_read_page(client, isp_page_buf, page_num)) < 0)
				return -EIO;

		pROM_v1 = (struct config_param_v1 *)(&isp_page_buf);
		pROM_v1->Bar_Y_Delta_2_Peak = ((RAW_2Peak&0xFF)<<8) | ((RAW_2Peak>>8)&0xFF);
		pROM_v1->peak_threshold_shift_y = shift&0xFF;
		pROM_v1->peak_threshold_offset_y = offset&0xFF;
	
	
		//Write back
		if ((len = st1232_isp_write_page(client, isp_page_buf, page_num)) < 0)
					return -EIO;
	
		//ISP Reset.
		priv->isp_enabled = false;
		st1232_isp_reset(client);
	}
	return count;
}

static DEVICE_ATTR(barY_filter, 0644 , sitronix_ts_barY_filter_show, sitronix_ts_barY_filter_store);
/*
 * sitronix resolution show & store
 */
static ssize_t sitronix_ts_resolution_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sitronix_ts_priv *priv = i2c_get_clientdata(client);
	u16 x_res, y_res;
	u8 page_num;
	int len;
	struct config_param_v0 *pROM_v0;
	struct config_param_v1 *pROM_v1;
	
	page_num = ST1232_ROM_PARAM_ADR / ST1232_ISP_PAGE_SIZE;
	
	//Jump to ISP.
	if(!priv->isp_enabled) {
		priv->isp_enabled = true;
		st1232_jump_to_isp(client);
	}
	
	//Read data from ROM
	if ((len = st1232_isp_read_page(client, isp_page_buf, page_num)) < 0)
			return -EIO;

	if(priv->struct_version == 0x00) {
		pROM_v0 = (struct config_param_v0 *)(&isp_page_buf);
		x_res = ((pROM_v0->x_res&0xFF)<<8) | (pROM_v0->x_res>>8);
		y_res = ((pROM_v0->y_res&0xFF)<<8) | (pROM_v0->y_res>>8);
	} else {
		pROM_v1 = (struct config_param_v1 *)(&isp_page_buf);
		x_res = ((pROM_v1->x_res&0xFF)<<8) | (pROM_v1->x_res>>8);
		y_res = ((pROM_v1->y_res&0xFF)<<8) | (pROM_v1->y_res>>8);
	}
	
	//ISP Reset.
	priv->isp_enabled = false;
	st1232_isp_reset(client);
	return sprintf(buf, "%u %u\n", x_res, y_res);
}

static ssize_t sitronix_ts_resolution_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sitronix_ts_priv *priv = i2c_get_clientdata(client);
	u32 x_res, y_res;
	u8 page_num;
	int len;
	struct config_param_v0 *pROM_v0;
	struct config_param_v1 *pROM_v1;
	
	page_num = ST1232_ROM_PARAM_ADR / ST1232_ISP_PAGE_SIZE;
	sscanf(buf, "%u%u", &x_res, &y_res);

	//Jump to ISP.
	if(!priv->isp_enabled) {
		priv->isp_enabled = true;
		st1232_jump_to_isp(client);
	}
	
	//Read data from ROM
	if ((len = st1232_isp_read_page(client, isp_page_buf, page_num)) < 0)
			return -EIO;

	if(priv->struct_version == 0x00) {
		pROM_v0 = (struct config_param_v0 *)(&isp_page_buf);
		pROM_v0->x_res = ((x_res&0xFF)<<8) | ((x_res>>8)&0xFF);
		pROM_v0->y_res = ((y_res&0xFF)<<8) | ((y_res>>8)&0xFF);
	} else {
		pROM_v1 = (struct config_param_v1 *)(&isp_page_buf);
		pROM_v1->x_res = ((x_res&0xFF)<<8) | ((x_res>>8)&0xFF);
		pROM_v1->y_res = ((y_res&0xFF)<<8) | ((y_res>>8)&0xFF);
	}
	
	//Write back
	if ((len = st1232_isp_write_page(client, isp_page_buf, page_num)) < 0)
				return -EIO;
	
	//ISP Reset.
	priv->isp_enabled = false;
	st1232_isp_reset(client);
	return count;
}

static DEVICE_ATTR(resolution, 0644 , sitronix_ts_resolution_show, sitronix_ts_resolution_store);

static ssize_t sitronix_ts_channel_num_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sitronix_ts_priv *priv = i2c_get_clientdata(client);
	u8 page_num;
	u8 x_chs , y_chs , k_chs;
	struct config_param_v0 *pROM_v0;
	struct config_param_v1 *pROM_v1;
	
	page_num = ST1232_ROM_PARAM_ADR / ST1232_ISP_PAGE_SIZE;
	
	//Jump to ISP.
	if(!priv->isp_enabled) {
		priv->isp_enabled = true;
		st1232_jump_to_isp(client);
	}
	
	if(priv->struct_version == 0x00) {
		pROM_v0 = (struct config_param_v0 *)(&isp_page_buf);
		x_chs = pROM_v0->x_chs;
		y_chs = pROM_v0->y_chs;
		k_chs = pROM_v0->k_chs;
	} else {
		pROM_v1 = (struct config_param_v1 *)(&isp_page_buf);
		x_chs = pROM_v1->x_chs;
		y_chs = pROM_v1->y_chs;
		k_chs = pROM_v1->k_chs;
	}
	
	//ISP Reset.
	priv->isp_enabled = false;
	st1232_isp_reset(client);
	return sprintf(buf, "%u %u %u\n", x_chs, y_chs, k_chs);
}
static DEVICE_ATTR(channel_num, 0644, sitronix_ts_channel_num_show, NULL);

static ssize_t sitronix_ts_para_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sitronix_ts_priv *priv = i2c_get_clientdata(client);
	u8 page_num;
	int len , i;
	
	page_num = ST1232_ROM_PARAM_ADR / ST1232_ISP_PAGE_SIZE;
	
	//Jump to ISP.
	if(!priv->isp_enabled) {
		priv->isp_enabled = true;
		st1232_jump_to_isp(client);
	}
	
	//Read data from ROM
	if ((len = st1232_isp_read_page(client, isp_page_buf, page_num)) < 0)
			return -EIO;
	
	for(i = 0 ; i < 32 ; i++) {
		printk("%02x %02x %02x %02x %02x %02x %02x %02x ",isp_page_buf[16*i], isp_page_buf[16*i+1], isp_page_buf[16*i+2], isp_page_buf[16*i+3], isp_page_buf[16*i+4], isp_page_buf[16*i+5], isp_page_buf[16*i+6], isp_page_buf[16*i+7]);
		printk("%02x %02x %02x %02x %02x %02x %02x %02x\n",isp_page_buf[16*i+8], isp_page_buf[16*i+9], isp_page_buf[16*i+10], isp_page_buf[16*i+11], isp_page_buf[16*i+12], isp_page_buf[16*i+13], isp_page_buf[16*i+14], isp_page_buf[16*i+15]);
	}
	
	//ISP Reset.
	priv->isp_enabled = false;
	st1232_isp_reset(client);
	return 0;
}
static DEVICE_ATTR(para, 0644 , sitronix_ts_para_show, NULL);


static int sitronix_ts_autotune_result_check(struct device *dev, int count_low, int count_high, int offset_low, int offset_high, int base_low, int base_high)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sitronix_ts_priv *priv = i2c_get_clientdata(client);
	u8 page_num;
	int check_num,len, i, baseline;
	struct config_param_v0 *pROM_v0;
	struct config_param_v1 *pROM_v1;
	priv->autotune_result = true;

	page_num = ST1232_ROM_PARAM_ADR / ST1232_ISP_PAGE_SIZE;

	//Jump to ISP.
	if(!priv->isp_enabled) {
		priv->isp_enabled = true;
		st1232_jump_to_isp(client);
	}
	
	//Read data from ROM
	if ((len = st1232_isp_read_page(client, isp_page_buf, page_num)) < 0)
			return -EIO;
	if(priv->struct_version == 0x00) {
		pROM_v0 = (struct config_param_v0 *)(&isp_page_buf);
	
		check_num = pROM_v0->x_chs + pROM_v0->y_chs + pROM_v0->k_chs;
	
		for(i = 0 ; i < check_num ; i++) {
			//Check Cnt
			if(i < pROM_v0->x_chs) {
				if(((pROM_v0->cnt[i] & 0x1F) < count_low) || ((pROM_v0->cnt[i] & 0x1F) > count_high)) {
					printk("invalid X[%d]_p1 = 0x%02X\n", i , pROM_v0->cnt[i] & 0x1F);
					priv->autotune_result = false;
				}
			} else if(i < pROM_v0->x_chs + pROM_v0->y_chs) {
				if(((pROM_v0->cnt[i] & 0x1F) < count_low) || ((pROM_v0->cnt[i] & 0x1F) > count_high)) {
					printk("invalid Y[%d]_p1 = 0x%02X\n", i-pROM_v0->x_chs , pROM_v0->cnt[i] & 0x1F);			
					priv->autotune_result = false;
				}
			} else {
				if(((pROM_v0->cnt[i] & 0x1F) < 0x06) || ((pROM_v0->cnt[i] & 0x1F) > 0x1E)) {
					printk("invalid K[%d]_p1 = 0x%02X\n", i-pROM_v0->x_chs-pROM_v0->y_chs , pROM_v0->cnt[i] & 0x1F);
					priv->autotune_result = false;
				}
			}
			//Check Offset
			if((pROM_v0->offset[i] < offset_low) || (pROM_v0->offset[i] > offset_high)) {
				if(i < pROM_v0->x_chs) {
					printk("invalid X[%d]_p2 = 0x%02X\n", i , pROM_v0->offset[i]);
					priv->autotune_result = false;
				} else if(i < pROM_v0->x_chs + pROM_v0->y_chs) {
					printk("invalid Y[%d]_p2 = 0x%02X\n", i-pROM_v0->x_chs , pROM_v0->offset[i]);
					priv->autotune_result = false;
				} else {
					printk("invalid K[%d]_p2 = 0x%02X\n", i-pROM_v0->x_chs-pROM_v0->y_chs , pROM_v0->offset[i]);
					priv->autotune_result = false;
				}
			}
			//Check Baseline 
			baseline = ((pROM_v0->baseline[i] & 0xFF) << 8) | ((pROM_v0->baseline[i] & 0xFF00) >> 8);
			if((baseline < base_low) || (baseline > base_high)) {
				if(i < pROM_v0->x_chs) {
					printk("invalid X[%d]_p3 = 0x%02X\n", i , baseline);
					priv->autotune_result = false;
				} else if(i < pROM_v0->x_chs + pROM_v0->y_chs) {
					printk("invalid Y[%d]_p3 = 0x%02X\n", i-pROM_v0->x_chs , baseline);
					priv->autotune_result = false;
				} else {
					printk("invalid K[%d]_p3 = 0x%02X\n", i-pROM_v0->x_chs-pROM_v0->y_chs , baseline);
					priv->autotune_result = false;
				}
			}
		}
		printk("Channel num : %u\n" , check_num);
	} else {
		pROM_v1 = (struct config_param_v1 *)(&isp_page_buf);
	
		check_num = pROM_v1->x_chs + pROM_v1->y_chs + pROM_v1->k_chs;
		for(i = 0 ; i < check_num ; i++) {
			//Check Cnt
			if(i < pROM_v1->x_chs) {
				if(((pROM_v1->cnt[i] & 0x1F) < count_low) || ((pROM_v1->cnt[i] & 0x1F) > count_high)) {
					printk("invalid X[%d]_p1 = 0x%02X\n", i , pROM_v1->cnt[i] & 0x1F);
					priv->autotune_result = false;
				}
			} else if(i < pROM_v1->x_chs + pROM_v1->y_chs) {
				if(((pROM_v1->cnt[i] & 0x1F) < count_low) || ((pROM_v1->cnt[i] & 0x1F) > count_high)) {
					printk("invalid Y[%d]_p1 = 0x%02X\n", i-pROM_v1->x_chs , pROM_v1->cnt[i] & 0x1F);			
					priv->autotune_result = false;
				}
			} else {
				if(((pROM_v1->cnt[i] & 0x1F) < 0x06) || ((pROM_v1->cnt[i] & 0x1F) > 0x1E)) {
					printk("invalid K[%d]_p1 = 0x%02X\n", i-pROM_v1->x_chs-pROM_v1->y_chs , pROM_v1->cnt[i] & 0x1F);
					priv->autotune_result = false;
				}
			}
			//Check Offset
			if((pROM_v1->offset[i] < offset_low) || (pROM_v1->offset[i] > offset_high)) {
				if(i < pROM_v1->x_chs) {
					printk("invalid X[%d]_p2 = 0x%02X\n", i , pROM_v1->offset[i]);
					priv->autotune_result = false;
				} else if(i < pROM_v1->x_chs + pROM_v1->y_chs) {
					printk("invalid Y[%d]_p2 = 0x%02X\n", i-pROM_v1->x_chs , pROM_v1->offset[i]);
					priv->autotune_result = false;
				} else {
					printk("invalid K[%d]_p2 = 0x%02X\n", i-pROM_v1->x_chs-pROM_v1->y_chs , pROM_v1->offset[i]);
					priv->autotune_result = false;
				}
			}
			//Check Baseline 
			baseline = ((pROM_v1->baseline[i] & 0xFF) << 8) | ((pROM_v1->baseline[i] & 0xFF00) >> 8);
			if((baseline < base_low) || (baseline > base_high)) {
				if(i < pROM_v1->x_chs) {
					printk("invalid X[%d]_p3 = 0x%02X\n", i , baseline);
					priv->autotune_result = false;
				} else if(i < pROM_v1->x_chs + pROM_v1->y_chs) {
					printk("invalid Y[%d]_p3 = 0x%02X\n", i-pROM_v1->x_chs , baseline);
					priv->autotune_result = false;
				} else {
					printk("invalid K[%d]_p3 = 0x%02X\n", i-pROM_v1->x_chs-pROM_v1->y_chs , baseline);
					priv->autotune_result = false;
				}
			}
		}
		printk("Channel num : %u\n" , check_num);
	}	
	
	if(priv->always_update) {
		return priv->always_update;
	} else {
		return priv->autotune_result;
	}
}

static ssize_t sitronix_ts_autotune_en_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sitronix_ts_priv *priv = i2c_get_clientdata(client);
	char status;
	int count_low, count_high, offset_low, offset_high, base_low, base_high;
	int update = 0;	
	int enabled = 0;
	int wait = 0;
	
	priv->always_update = false;
	sscanf(buf, "%x %x 0x%02x 0x%02x 0x%02x 0x%02x %u %u",&enabled, &update, &count_low, &count_high, &offset_low, &offset_high, &base_low, &base_high);

	printk("Check parameter : count[%02X:%02X]\toffset[%02X:%02X]\tbaseline[%u:%u]\n", count_low, count_high, offset_low, offset_high, base_low, base_high);
	
	if(enabled) {
		priv->autotune_result = false;
		priv->always_update = update;
		sitronix_ts_set_autotune_en(client);
		msleep(200);
		status = sitronix_ts_get_status(client);
		while(status && (count < 50000)) {
			status = sitronix_ts_get_status(client);
			wait++;
			if(wait % 16 == 15)
				printk("\n");
		}
		if(wait <= 10000) {
			printk("\nAutoTune Done!\n");
			sitronix_ts_autotune_result_check(dev, count_low, count_high, offset_low, offset_high, base_low, base_high);
		} else {
			printk("\nAutoTune Timeout!\n");
		}
	}
	return count;
}

static DEVICE_ATTR(autotune_en, 0644, NULL, sitronix_ts_autotune_en_store);

static ssize_t sitronix_ts_autotune_result_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sitronix_ts_priv *priv = i2c_get_clientdata(client);
	
	if(priv->always_update) {
		return sprintf(buf, "%u\n", priv->always_update);
	} else {
		return sprintf(buf, "%u\n", priv->autotune_result);
	}
}

static DEVICE_ATTR(autotune_result, 0644, sitronix_ts_autotune_result_show, NULL);

static ssize_t sitronix_ts_debug_check_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sitronix_ts_priv *priv = i2c_get_clientdata(client);
	char Point_buf[256];
	int i, j, ret;

	for(i = 0 ; i < priv->I2C_RepeatTime ; i++) {
		Point_buf[0] = priv->I2C_Offset;	//Set Reg. address to 0x10 for reading point report.
		if ((ret = i2c_master_send(client, Point_buf, 1)) != 1) {
			printk("I2C Send Data Fail\n");
			return 0;
		}

		//Read 8 byte point data from Reg. 0x10 set previously.
		if ((ret = i2c_master_recv(client, Point_buf, priv->I2C_Length)) != priv->I2C_Length) {
			printk("I2C Read Data Fail\n");
			return 0;
		}
		
		printk("===================debug report===================\n");
		for(j = 0 ; j < priv->I2C_Length ; j++) {
			printk("Buf[%d] : 0x%02X\n",j, Point_buf[j]);
		}		
		msleep(100);
	}
	return 0;	
}

static ssize_t sitronix_ts_debug_check_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sitronix_ts_priv *priv = i2c_get_clientdata(client);
	int Offset , Length , RepeatTime;

	sscanf(buf, "%u%u%u", &Offset, &Length, &RepeatTime);
	priv->I2C_Offset = Offset&0xFF;
	priv->I2C_Length = Length&0xFF;
	priv->I2C_RepeatTime = RepeatTime&0xFF;

	return count;
}
static DEVICE_ATTR(debug_check, 0644, sitronix_ts_debug_check_show, sitronix_ts_debug_check_store);

static struct attribute *sitronix_ts_attrs_v0[] = {
	&dev_attr_isp_ctrl.attr,
	&dev_attr_revision.attr,
	&dev_attr_struct_version.attr,
	&dev_attr_data_threshold.attr,
	&dev_attr_point_threshold.attr,
	&dev_attr_peak_threshold.attr,
	&dev_attr_mutual_threshold.attr,
	&dev_attr_range_filter.attr,
	&dev_attr_barX_filter.attr,
	&dev_attr_barY_filter.attr,
	&dev_attr_resolution.attr,
	&dev_attr_channel_num.attr,
	&dev_attr_para.attr,
	&dev_attr_autotune_en.attr,
	&dev_attr_autotune_result.attr,
	&dev_attr_debug_check.attr,
	NULL,
};



static struct attribute_group sitronix_ts_attr_group_v0 = {
	.name = "sitronix_ts_attrs",
	.attrs = sitronix_ts_attrs_v0,
};

static int sitronix_ts_create_sysfs_entry(struct i2c_client *client)
{
	//struct sitronix_ts_priv *priv = i2c_get_clientdata(client);
	int err;

	err = sysfs_create_group(&(client->dev.kobj), &sitronix_ts_attr_group_v0);
	if (err) {
		dev_dbg(&client->dev, "%s(%u): sysfs_create_group() failed!\n", __FUNCTION__, __LINE__);
	}
	
	err = sysfs_create_bin_file(&client->dev.kobj, &st1232_flash_bin_attr);
	if (err) {
		sysfs_remove_group(&(client->dev.kobj), &sitronix_ts_attr_group_v0);
		dev_dbg(&client->dev, "%s(%u): sysfs_create_bin_file() failed!\n", __FUNCTION__, __LINE__);
	}
	err = sysfs_create_bin_file(&client->dev.kobj, &st1232_panel_bin_attr);
	if (err) {
		sysfs_remove_group(&(client->dev.kobj), &sitronix_ts_attr_group_v0);
		sysfs_remove_bin_file(&(client->dev.kobj), &st1232_flash_bin_attr);
		dev_dbg(&client->dev, "%s(%u): sysfs_create_bin_file() failed!\n", __FUNCTION__, __LINE__);
	}
	return err;
}

static void sitronix_ts_destroy_sysfs_entry(struct i2c_client *client)
{
	//struct sitronix_ts_priv *priv = i2c_get_clientdata(client);
	sysfs_remove_bin_file(&client->dev.kobj, &st1232_panel_bin_attr);
	sysfs_remove_bin_file(&client->dev.kobj, &st1232_flash_bin_attr);
	sysfs_remove_group(&(client->dev.kobj), &sitronix_ts_attr_group_v0);

	return;
}

#if 0
static int sitronix_ts_get_struct_version(struct i2c_client *client, u8 *rev)
{
	struct sitronix_ts_priv *priv = i2c_get_clientdata(client);
	u8 page_num;
	u32 len;
	
	
	page_num = ST1232_ROM_PARAM_ADR / ST1232_ISP_PAGE_SIZE;
	
	//Jump to ISP.
	if(!priv->isp_enabled) {
		priv->isp_enabled = true;
		st1232_jump_to_isp(client);
	}
	
	//Read data from ROM
	if ((len = st1232_isp_read_page(client, isp_page_buf, page_num)) < 0)
			return -EIO;

	*rev = (u8)isp_page_buf[1];

	//ISP Reset.
	priv->isp_enabled = false;
	st1232_isp_reset(client);

	return 0;
}

static int sitronix_ts_internal_update(struct i2c_client *client)
{
	struct sitronix_ts_priv *priv = i2c_get_clientdata(client);
	int err = 1;
	int count, off;
	char buf[512];
	struct file *file = NULL;

	//Jump to ISP.
	printk("Jump to ISP\n");
	if(!priv->isp_enabled) {
		priv->isp_enabled = true;
		st1232_jump_to_isp(client);
	}
	
	// Update FW
	mm_segment_t old_fs = get_fs();
	set_fs(KERNEL_DS);
	//file = filp_open("/media/mmcblk0p1/WinTek/touch_panel_T20_m0_QFN48.bin" , O_RDWR , 0644);
	file = filp_open("/media/mmcblk0p1/touch_screen_uart_isp.bin" , O_RDWR , 0644);
	if(IS_ERR(file))
	{	
		printk("open file fail\n");
		//ISP Reset.
		priv->isp_enabled = false;
		st1232_isp_reset(client);
		return -1;
	}
	
	off = 0;
	
	while((count = file->f_op->read(file, (char *)buf , 512 , &file->f_pos)) > 0) {
		err = st1232_isp_write_flash(client, buf, off, count);
		if (err < 0) {
			printk("update fw fail\n");
			//ISP Reset.
			priv->isp_enabled = false;
			st1232_isp_reset(client);
			return err;
		}
		printk("Update FW : offset %d , length %d\n",off , count);
		off += count;
	}
	filp_close(file, NULL);
	set_fs(old_fs);
	printk("Update FW Finish\n");
	//ISP Reset.
	priv->isp_enabled = false;
	st1232_isp_reset(client);
	printk("ISP Reset\n");

	return err;

}
#endif

void TP_Reset_sleep(void)
{
	gpio_set_value(RESET_GPIO, 1);	
	usleep(20);
	gpio_set_value(RESET_GPIO, 0);
	msleep(10);
	gpio_set_value(RESET_GPIO, 1);
	//udelay(100);
	msleep(200);
}

void TP_Reset_delay(void)
{
	gpio_set_value(RESET_GPIO, 1);	
	udelay(20);
	gpio_set_value(RESET_GPIO, 0);
	mdelay(10);
	gpio_set_value(RESET_GPIO, 1);
	//udelay(100);
	mdelay(200);
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void sitronix_ts_early_suspend(struct early_suspend *h)
{
	disable_irq(MSM_GPIO_TO_INT(INT_GPIO));
	
	TP_DEBUG("early_suspend done.\n");
}

static void sitronix_ts_late_resume(struct early_suspend *h)
{
	enable_irq(MSM_GPIO_TO_INT(INT_GPIO));
	TP_DEBUG("late_resume done.\n");
}
#endif

static int __devinit sitronix_ts_probe(struct i2c_client *client, const struct i2c_device_id *idp)
{
	struct sitronix_ts_priv *priv;
	struct input_dev *input;
	int err = -ENOMEM;
	u16 x_res = 0, y_res = 0;
	u32 ver, rev;
	int rc;
//	u8 struct_ver;

	printk("sit TP cellon\n");

	rc = gpio_tlmm_config(GPIO_CFG(INT_GPIO, 0,
				GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
				GPIO_CFG_8MA), GPIO_CFG_ENABLE);
	if (rc) {
		pr_err("%s: gpio_tlmm_config for %d failed\n",
			__func__, INT_GPIO);
		goto err0;
	}
	gpio_set_value(INT_GPIO, 1);
	gpio_request(INT_GPIO, "interrupt");
	gpio_request(RESET_GPIO, "reset");
	gpio_direction_input(INT_GPIO);
	gpio_direction_output(RESET_GPIO, 1);
	
	mdelay(10);
	
	TP_Reset_delay();

	sitronix_ts_gpts = kzalloc(sizeof(struct sitronix_ts_data), GFP_KERNEL);
	sitronix_ts_gpts->client = client;
	priv = (struct sitronix_ts_priv *)kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dev_err(&client->dev, "Failed to allocate driver data\n");
		goto err0;
	}
	
	mutex_init(&priv->mutex);
	
	INIT_WORK(&priv->work, sitronix_ts_work);
	input = input_allocate_device();
	
	if (!input) {
		dev_err(&client->dev, "Failed to allocate input device.\n");
		goto err1;
	}

	if ((err = sitronix_ts_get_fw_version(client, &ver))) {
		dev_err(&client->dev, "Unable to get FW version!\n");
		goto err1;
	} else {
		printk("%s(%u): FW version=%X\n", __FUNCTION__, __LINE__, ver);
	}
	
	if ((err = sitronix_ts_get_resolution(client, &x_res, &y_res))) {
		dev_err(&client->dev, "Unable to get resolution!\n");
		goto err1;
	}else {
		TP_DEBUG(": Resolution=  %d x%d \n",  x_res, y_res);
	}
	
	input->name = client->name;
	input->phys = "I2C";
	input->id.bustype = BUS_I2C;
	input->dev.parent = &client->dev;
	input->open = sitronix_ts_open;
	input->close = sitronix_ts_close;

	set_bit(EV_ABS, input->evbit);
	//set_bit(EV_SYN, input->evbit);
	set_bit(BTN_TOUCH, input->keybit);

	input_set_abs_params(input, ABS_MT_TRACKING_ID, 0, 2, 0, 0);
	input_set_abs_params(input, ABS_MT_TOUCH_MAJOR, 0, 2, 0, 0);
	input_set_abs_params(input, ABS_MT_POSITION_X, 0, 320, 0, 0);
	input_set_abs_params(input, ABS_MT_POSITION_Y, 0, 520, 0, 0);

	err = input_register_device(input);
	if (err)
		goto err1; 

	//create simulate key input
	simulate_key_input = input_allocate_device();
	if (simulate_key_input == NULL) {
		printk("%s: not enough memory for input device\n", __func__);
		err = -ENOMEM;
		goto err1;
	}
	simulate_key_input->name = "7x27a_kp";
	__set_bit(EV_KEY, simulate_key_input->evbit);
	__set_bit(KEY_BACK, simulate_key_input->keybit);
	__set_bit(KEY_MENU, simulate_key_input->keybit);
	__set_bit(KEY_HOME, simulate_key_input->keybit);
	__set_bit(ST_KEY_SEARCH, simulate_key_input->keybit);
	err = input_register_device(simulate_key_input);
	if (err != 0) {
		printk("%s: failed to register input device\n", __func__);
		goto err1;
	}
	
	priv->client = client;
	priv->input = input;
	//priv->irq = client->irq;
	priv->isp_enabled = false;
	priv->autotune_result = false;
	priv->always_update = false;

	i2c_set_clientdata(client, priv);
	input_set_drvdata(input, priv);
	
	if ((err = sitronix_ts_get_fw_revision(client, &rev))) {
		dev_err(&client->dev, "Unable to get FW revision!\n");
		goto err1;
	} else {
		dev_dbg(&client->dev, "%s(%u): FW revision=%X\n", __FUNCTION__, __LINE__, rev);
		priv->fw_revision = rev;
	}
	/*
	if((err = sitronix_ts_get_struct_version(client, &struct_ver))){
		dev_err(&client->dev, "Unable to get struct version!\n");
	}else{
		dev_dbg(&client->dev, "%s(%u): struct version=%X\n", __FUNCTION__, __LINE__, struct_ver);
		priv->struct_version = struct_ver;
	}*/
	priv->struct_version = 1;
	
	// ToDo
	// Check Version
	// Check Power 
	//sitronix_ts_internal_update(client);

	#ifndef USE_POLL
	priv->irq = MSM_GPIO_TO_INT(INT_GPIO); /* MSM_GPIO_TO_INT */
	pr_info("sitronix_ts_ gpio_irq = [%d]\n", priv->irq);
	err = request_irq(priv->irq, sitronix_ts_isr, IRQF_TRIGGER_LOW,
			  client->name, priv);
	if (err) {
		dev_err(&client->dev, "Unable to request touchscreen IRQ.\n");
		goto err2;
	}
	#else
	hrtimer_init(&priv->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	priv->timer.function = sitronix_ts_timer_func;
	hrtimer_start(&priv->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	#endif
	
	/* Disable irq. The irq will be enabled once the input device is opened. */
	disable_irq(priv->irq);
	device_init_wakeup(&client->dev, 0);

	err = sitronix_ts_create_sysfs_entry(client);
	if (err) {
		TP_DEBUG("ERROR!\n");
		goto err2;
	}

	#ifdef CONFIG_HAS_EARLYSUSPEND
	priv->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	priv->early_suspend.suspend = sitronix_ts_early_suspend;
	priv->early_suspend.resume = sitronix_ts_late_resume;
	register_early_suspend(&priv->early_suspend);
	#endif 

	pr_info("sitronix_ts driver is on##############################\n");
	return 0;

err2:
	input_unregister_device(input);
	input = NULL; /* so we dont try to free it below */
err1:
	input_free_device(input);
	i2c_set_clientdata(client, NULL);
	kfree(priv);
err0:
	return err;
}

static int __devexit sitronix_ts_remove(struct i2c_client *client)
{
	struct sitronix_ts_priv *priv = i2c_get_clientdata(client);

	sitronix_ts_destroy_sysfs_entry(client);
	free_irq(priv->irq, priv);
	input_unregister_device(priv->input);
	i2c_set_clientdata(client, NULL);
	kfree(priv);

	return 0;
}

static int sitronix_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{

	char buf[2];
	int ret;
	
	
	buf[0] = 0x2;	
	buf[1] = 0x2;
	if ((ret = i2c_master_send(client, buf, 2)) != 2)
		return ret;

	TP_DEBUG("suspend done.\n");
	mdelay(10);
	return 0;
}

static int sitronix_ts_resume(struct i2c_client *client)
{
#if 0
    TP_Reset_sleep();
#else
    	char buf[2];
	int ret;

    	buf[0] = 0x2;
	buf[1] = 0x0;
	if ((ret = i2c_master_send(client, buf, 2)) != 2)
		return ret;
#endif
	TP_DEBUG("resume done.\n");
	return 0;
}

void sitronix_ts_shutdown(struct i2c_client *client)
{
	char buf[2];
	int ret;
	
	disable_irq(MSM_GPIO_TO_INT(INT_GPIO));
	mdelay(10);
	
	buf[0] = 0x2;	
	buf[1] = 0x2;
	if ((ret = i2c_master_send(client, buf, 2)) != 2){
		printk("sitronix_ts_shutdown error\n");
	}

	printk("sitronix_ts_shutdown \n");
	mdelay(10);
}

static const struct i2c_device_id sitronix_ts_id[] = {
	{ "sitronix_ts", 0x54 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sitronix_ts_id);

static struct i2c_driver sitronix_ts_driver = {
	.driver = {
		.name = "sitronix_ts",
	},
	.probe = sitronix_ts_probe,
	.remove = __devexit_p(sitronix_ts_remove),
	.suspend = sitronix_ts_suspend,
	.resume = sitronix_ts_resume,
	.shutdown = sitronix_ts_shutdown,
	.id_table = sitronix_ts_id,
};

static struct file_operations nc_fops = {
	.owner =        THIS_MODULE,
	.write		= sitronix_write,
	.read		= sitronix_read,
	.open		= sitronix_open,
	.unlocked_ioctl = sitronix_ioctl,
	.release	= sitronix_release,
};

static int __init sitronix_ts_init(void)
{
	int result;
	int err = 0;
	dev_t devno;
		
			devno = MKDEV(sitronix_major, 0);
			result  = alloc_chrdev_region(&devno, 0, 1, "sitronixMT");
			if(result < 0){
				printk("fail to allocate chrdev (%d) \n", result);
				return 0;
			}
			sitronix_major = MAJOR(devno);
		        cdev_init(&sitronix_cdev, &nc_fops);
			sitronix_cdev.owner = THIS_MODULE;
			sitronix_cdev.ops = &nc_fops;
		        err =  cdev_add(&sitronix_cdev, devno, 1);
			if(err){
				printk("fail to add cdev (%d) \n", err);
				return 0;
			}

			sitronix_class = class_create(THIS_MODULE, "sitronixMT");
			if (IS_ERR(sitronix_class)) {
				result = PTR_ERR(sitronix_class);
				unregister_chrdev(sitronix_major, "sitronixMT");
				printk("fail to create class (%d) \n", result);
				return result;
			}
			device_create(sitronix_class, NULL, MKDEV(sitronix_major, 0), NULL, "sitronixMT");
			return i2c_add_driver(&sitronix_ts_driver);
		
}

static void __exit sitronix_ts_exit(void)
{
	dev_t dev_id = MKDEV(sitronix_major, 0);
	
	i2c_del_driver(&sitronix_ts_driver);
	
	cdev_del(&sitronix_cdev); 

	device_destroy(sitronix_class, dev_id); //delete device node under /dev
	class_destroy(sitronix_class); //delete class created by us
	unregister_chrdev_region(dev_id, 1);
}

MODULE_DESCRIPTION("Sitronix Touch Screen Driver");
MODULE_AUTHOR("CT Chen <ct_chen@sitronix.com.tw>");
MODULE_LICENSE("GPL");

module_init(sitronix_ts_init);
module_exit(sitronix_ts_exit);

