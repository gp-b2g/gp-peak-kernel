/*
 *  stk31xx_int.c - Linux kernel modules for proximity/ambient light sensor
 *  (Intrrupt Mode)
 *
 *  Copyright (C) 2011 Patrick Chang / SenseTek <patrick_chang@sitronix.com.tw>
 *  Copyright (C) 2012 Lex Hsieh     / SenseTek <lex_hsieh@sitronix.com.tw>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/poll.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/completion.h>
#include <linux/kthread.h>
#include <linux/errno.h>
#include <linux/wakelock.h>
#include <linux/interrupt.h>

#define CONFIG_STK_PS_ENGINEER_TUNING
//#define CONFIG_STK_SYSFS_DBG

#include "linux/stk31xx.h"
#include "linux/stk_defines.h"
#include "linux/stk_lk_defs.h"
#if (defined(STK_MANUAL_CT_CALI) || defined(STK_MANUAL_GREYCARD_CALI))
#include   <linux/fs.h>   
#include  <asm/uaccess.h> 
#endif	/* #ifdef STK_MANUAL_CT_CALI */
#ifdef SITRONIX_PERMISSION_THREAD
#include <linux/fcntl.h>
#include <linux/syscalls.h>
#endif 	//	#ifdef SITRONIX_PERMISSION_THREAD

#define ADDITIONAL_GPIO_CFG 1

/* // Additional GPIO CFG Header */
#if ADDITIONAL_GPIO_CFG
#include <linux/gpio.h>
#define EINT_GPIO 136
#endif

#define STKALS_DRV_NAME	"stk_als"
#define STKPS_DRV_NAME "stk_ps"
#define DEVICE_NAME		"stk-oss"
#define DRIVER_VERSION  STK_DRIVER_VER
#define LightSensorDevName "stk_als"
#define ProximitySensorDevName "stk_ps"

#define DEFAULT_ALS_DELAY 20
#define ALS_ODR_DELAY (1000/10)
#define PS_ODR_DELAY (1000/50)

#define stk_writew i2c_smbus_write_word_data
#define stk_readw i2c_smbus_read_word_data

#define STK_LOCK0 mutex_unlock(&stkps_io_lock)
#define STK_LOCK1 mutex_lock(&stkps_io_lock)
static int32_t pdata_val;
static int32_t cdata_val;
static bool away = 0;

#if( !defined(CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD) && defined(CONFIG_STK31XX_INT_MODE))
static uint32_t lux_threshold_table[] =
{
	3,
	10,
	40,
	65,
	145,
	300,
	550,
	930,
	1250,
	1700,
};

#define LUX_THD_TABLE_SIZE (sizeof(lux_threshold_table)/sizeof(uint32_t)+1)
static uint16_t code_threshold_table[LUX_THD_TABLE_SIZE+1];
#endif 	//#if( !defined(CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD) && defined(CONFIG_STK31XX_INT_MODE))

static int32_t init_all_setting(struct stk31xx_platform_data* plat_data);
static int32_t enable_ps(uint8_t enable);
static int32_t enable_als(uint8_t enable);
static int32_t software_reset(void);

static int32_t set_als_it(uint8_t it);
static int32_t set_als_gain(uint8_t gain);
static int32_t set_ps_it(uint8_t it);
static int32_t set_ps_slp(uint8_t slp);
static int32_t set_ps_led_driving_current(uint8_t irdr);
static int32_t set_ps_gc(uint8_t gc);

static int32_t set_ps_thd_l(uint8_t thd_l);
static int32_t set_ps_thd_h(uint8_t thd_h);

static int32_t set_als_thd_l(uint16_t thd_l);
static int32_t set_als_thd_h(uint16_t thd_h);

static int32_t reset_int_flag(uint8_t org_status,uint8_t disable_flag);
static int32_t enable_ps_int(uint8_t enable);
static int32_t enable_als_int(uint8_t enable);
#if (defined(STK_MANUAL_CT_CALI) || defined(STK_MANUAL_GREYCARD_CALI) || defined(STK_AUTO_CT_CALI_SATU) || defined(STK_AUTO_CT_CALI_NO_SATU))
static int32_t set_ps_cali(void);
#endif
static struct mutex stkps_io_lock;
static struct stkps31xx_data* pStkPsData = NULL;
static struct wake_lock proximity_sensor_wakelock;
static uint8_t ps_code_low_thd;
static uint8_t ps_code_high_thd;
static int32_t als_transmittance;

static int32_t psensor_is_good = -1;

inline uint32_t alscode2lux(uint32_t alscode)
{
   alscode += ((alscode<<7)+(alscode<<3)+(alscode>>1));     // 137.5
      //x1       //   x128         x8            x0.5
    alscode<<=3; // x 8 (software extend to 19 bits)
    // Gain & IT setting ==> x8
    // ==> i.e. code x 8800
    // Org : 1 code = 0.88 Lux
    // 8800 code = 0.88 lux --> this means it must be * 1/10000

    alscode/=als_transmittance;

    //printk("%s: alscode fixed = %d\n", __func__, alscode);
#if 1
    if (alscode > 220)
           alscode = 220 + ((alscode - 220) / 20);
    else if (alscode >= 20 && alscode < 100)
           alscode += ((100 - alscode) / 2);
#endif
    return alscode;
}

inline uint32_t lux2alscode(uint32_t lux)
{
    lux*=als_transmittance;
    lux/=1100;
    if (unlikely(lux>=(1<<16)))
        lux = (1<<16) -1;
    return lux;

}

#ifndef CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD
static void init_code_threshold_table(void)
{
    uint32_t i,j;
    uint32_t alscode;

    code_threshold_table[0] = 0;
    //INFO("alscode[0]=%d\n",0);
    for (i=1,j=0;i<LUX_THD_TABLE_SIZE;i++,j++)
    {
        alscode = lux2alscode(lux_threshold_table[j]);
        //INFO("alscode[%d]=%d\n",i,alscode);
        code_threshold_table[i] = (uint16_t)(alscode);
    }
    code_threshold_table[i] = 0xffff;
    //INFO("alscode[%d]=%d\n",i,alscode);
}

static uint32_t get_lux_interval_index(uint16_t alscode)
{
    uint32_t i;
    for (i=1;i<=LUX_THD_TABLE_SIZE;i++)
    {
        if ((alscode>=code_threshold_table[i-1])&&(alscode<code_threshold_table[i]))
        {
            return i;
        }
    }
    return LUX_THD_TABLE_SIZE;
}
#else
inline void set_als_new_thd_by_reading(uint16_t alscode)
{
    int32_t high_thd,low_thd;
    high_thd = alscode + lux2alscode(CONFIG_STK_ALS_CHANGE_THRESHOLD);
    low_thd = alscode - lux2alscode(CONFIG_STK_ALS_CHANGE_THRESHOLD);
    if (high_thd >= (1<<16))
        high_thd = (1<<16) -1;
    if (low_thd <0)
        low_thd = 0;
    //printk("%s: alscode %d high_thd %d low_thd %d\n", __func__, alscode, high_thd, low_thd);
    set_als_thd_h((uint16_t)high_thd);
    set_als_thd_l((uint16_t)low_thd);
}
#endif // CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD

static int32_t init_all_setting(struct stk31xx_platform_data* plat_data)
{
    if (software_reset()<0)
    {
        ERR("STK PS : error --> device not found\n");
        return 0;
    }
	pStkPsData->ps_cmd_reg = plat_data->ps_cmd;
	pStkPsData->als_cmd_reg = plat_data->als_cmd;
	pStkPsData->ps_gc_reg = plat_data->ps_gain;
	
    enable_ps(0);
    enable_als(0);
    set_ps_slp((plat_data->ps_cmd & STK_PS_CMD_SLP_MASK) >> STK_PS_CMD_SLP_SHIFT);
    set_ps_gc(plat_data->ps_gain);
    set_ps_it((plat_data->ps_cmd & STK_PS_CMD_IT_MASK) >> STK_PS_CMD_IT_SHIFT);
    set_ps_led_driving_current((plat_data->ps_cmd & STK_PS_CMD_DR_MASK) >> STK_PS_CMD_DR_SHIFT);
    set_als_gain((plat_data->als_cmd & STK_ALS_CMD_GAIN_MASK) >> STK_ALS_CMD_GAIN_SHIFT); 
    set_als_it((plat_data->als_cmd & STK_ALS_CMD_IT_MASK) >> STK_ALS_CMD_IT_SHIFT); 
    set_ps_thd_h(plat_data->ps_high_thd);
    set_ps_thd_l(plat_data->ps_low_thd);

    enable_ps_int(1);
    enable_als_int(1);
#if (defined(STK_MANUAL_CT_CALI) || defined(STK_MANUAL_GREYCARD_CALI) || defined(STK_AUTO_CT_CALI_SATU) || defined(STK_AUTO_CT_CALI_NO_SATU))
	pStkPsData->ps_cali_done = false;
#endif	
#if (defined(STK_MANUAL_CT_CALI) || defined(STK_MANUAL_GREYCARD_CALI))		
	pStkPsData->first_boot = true;	
#endif
#if (defined(STK_AUTO_CT_CALI_SATU) || defined(STK_AUTO_CT_CALI_NO_SATU))	
	set_ps_cali();
#endif	
    return 1;
}

static int32_t software_reset(void)
{
    // software reset and check stk 83xx is valid
    int32_t r;
    uint8_t w_reg;
    uint8_t org_reg;

    r = i2c_smbus_read_byte_data(pStkPsData->client,STK_PS_GC_REG);
    if (r<0)
    {
        ERR("STK PS software reset: read i2c error\n");
        return r;
    }
    org_reg = (uint8_t)(r&0xf0);
    w_reg = ~((uint8_t)(r&0xff));
    r = i2c_smbus_write_byte_data(pStkPsData->client,STK_PS_GC_REG,w_reg);
    if (r<0)
    {
        ERR("STK PS software reset: write i2c error\n");
        return r;
    }
    r = i2c_smbus_read_byte_data(pStkPsData->client,STK_PS_GC_REG);
    if (w_reg!=(uint8_t)(r&0xff))
    {
        ERR("STK PS software reset: read-back value is not  the same\n");
        return -1;
    }
    r = i2c_smbus_write_byte_data(pStkPsData->client,STK_PS_SOFTWARE_RESET_REG,0);
    msleep(5);
    if (r<0)
    {
        ERR("STK PS software reset: read error after reset\n");
        return r;
    }
    return 0;
}

static int32_t enable_ps_int(uint8_t enable)
{
    pStkPsData->ps_cmd_reg &= (~STK_PS_CMD_INT_MASK);
    pStkPsData->ps_cmd_reg |= STK_PS_CMD_INT(enable);
    return i2c_smbus_write_byte_data(pStkPsData->client,STK_PS_CMD_REG,pStkPsData->ps_cmd_reg);
}

static int32_t enable_als_int(uint8_t enable)
{
    pStkPsData->als_cmd_reg &= (~STK_ALS_CMD_INT_MASK);
    pStkPsData->als_cmd_reg |= STK_ALS_CMD_INT(enable);
    return i2c_smbus_write_byte_data(pStkPsData->client,STK_ALS_CMD_REG,pStkPsData->als_cmd_reg);
}

static int32_t set_als_it(uint8_t it)
{
    pStkPsData->als_cmd_reg &= (~STK_ALS_CMD_IT_MASK);
    pStkPsData->als_cmd_reg |= (STK_ALS_CMD_IT_MASK & STK_ALS_CMD_IT(it));
    return i2c_smbus_write_byte_data(pStkPsData->client,STK_ALS_CMD_REG,pStkPsData->als_cmd_reg);

}

static int32_t set_als_gain(uint8_t gain)
{
	/*if(gain >= 2)
	{
		INFO("STK PS : als_gain = %d\n", gain);		
	}	*/
    pStkPsData->als_cmd_reg &= (~STK_ALS_CMD_GAIN_MASK);
    pStkPsData->als_cmd_reg |= (STK_ALS_CMD_GAIN_MASK & STK_ALS_CMD_GAIN(gain));
    return i2c_smbus_write_byte_data(pStkPsData->client,STK_ALS_CMD_REG,pStkPsData->als_cmd_reg);
}

static int32_t set_ps_it(uint8_t it)
{
    pStkPsData->ps_cmd_reg &= (~STK_PS_CMD_IT_MASK);
    pStkPsData->ps_cmd_reg |= (STK_PS_CMD_IT_MASK & STK_PS_CMD_IT(it));
    return i2c_smbus_write_byte_data(pStkPsData->client,STK_PS_CMD_REG,pStkPsData->ps_cmd_reg);
}

static int32_t set_ps_slp(uint8_t slp)
{
    pStkPsData->ps_cmd_reg &= (~STK_PS_CMD_SLP_MASK);
    pStkPsData->ps_cmd_reg |= (STK_PS_CMD_SLP_MASK & STK_PS_CMD_SLP(slp));
    return i2c_smbus_write_byte_data(pStkPsData->client,STK_PS_CMD_REG,pStkPsData->ps_cmd_reg);

}

static int32_t set_ps_led_driving_current(uint8_t irdr)
{
    pStkPsData->ps_cmd_reg &= (~STK_PS_CMD_DR_MASK);
    pStkPsData->ps_cmd_reg |= (STK_PS_CMD_DR_MASK & STK_PS_CMD_DR(irdr));
    return i2c_smbus_write_byte_data(pStkPsData->client,STK_PS_CMD_REG,pStkPsData->ps_cmd_reg);
}

static int32_t set_ps_gc(uint8_t gc)
{
    int32_t retval;

    retval = i2c_smbus_read_byte_data(pStkPsData->client,STK_PS_GC_REG);
    if (retval<0)
        return retval;
    pStkPsData->ps_gc_reg = (uint8_t)retval;
    pStkPsData->ps_gc_reg &= (~STK_PS_GC_GAIN_MASK);
    pStkPsData->ps_gc_reg |= (STK_PS_GC_GAIN(gc)&STK_PS_GC_GAIN_MASK);

    return i2c_smbus_write_byte_data(pStkPsData->client,STK_PS_GC_REG,pStkPsData->ps_gc_reg);
}

static int32_t set_ps_thd_l(uint8_t thd_l)
{
    ps_code_low_thd = thd_l;
	return i2c_smbus_write_byte_data(pStkPsData->client,STK_PS_THD_L_REG,thd_l);
}
static int32_t set_ps_thd_h(uint8_t thd_h)
{
    ps_code_high_thd = thd_h;
    return i2c_smbus_write_byte_data(pStkPsData->client,STK_PS_THD_H_REG,thd_h);
}

static int32_t set_als_thd_l(uint16_t thd_l)
{
    uint8_t temp;
    uint8_t* pSrc = (uint8_t*)&thd_l;
    temp = *pSrc;
    *pSrc = *(pSrc+1);
    *(pSrc+1) = temp;
    return i2c_smbus_write_word_data(pStkPsData->client,STK_ALS_THD_L1_REG,thd_l);
}

static int32_t set_als_thd_h(uint16_t thd_h)
{
    uint8_t temp;
    uint8_t* pSrc = (uint8_t*)&thd_h;
    temp = *pSrc;
    *pSrc = *(pSrc+1);
    *(pSrc+1) = temp;
    return i2c_smbus_write_word_data(pStkPsData->client,STK_ALS_THD_H1_REG,thd_h);
}

inline int32_t get_status_reg(void)
{
    return i2c_smbus_read_byte_data(pStkPsData->client,STK_PS_STATUS_REG);
}

static int32_t reset_int_flag(uint8_t org_status,uint8_t disable_flag)
{
	uint8_t val;
	
	org_status &= (STK_PS_STATUS_PS_INT_FLAG_MASK | STK_PS_STATUS_ALS_INT_FLAG_MASK);
    val = (uint8_t)(org_status&(~disable_flag));
    return i2c_smbus_write_byte_data(pStkPsData->client,STK_PS_STATUS_REG,val);
}

static inline int32_t get_als_reading(void)
{
    int32_t word_data, tmp_word_data;
	// make sure MSB and LSB are the same data set
	tmp_word_data = i2c_smbus_read_word_data(pStkPsData->client, STK_ALS_DT1_REG);
	if(tmp_word_data < 0)
	{
		ERR("STK PS :%s fail, err=0x%x", __func__, tmp_word_data);
		return tmp_word_data;	   
	}
	else
	{
		word_data = ((tmp_word_data & 0xFF00) >> 8) | ((tmp_word_data & 0x00FF) << 8) ;
		cdata_val = word_data;
		//INFO("%s: word_data=0x%4x\n", __func__, word_data);
		return word_data;
	}
}

static inline int32_t get_ps_reading(void)
{
	int32_t ps;
	ps = i2c_smbus_read_byte_data(pStkPsData->client,STK_PS_READING_REG);
    if(ps < 0)
	{
		ERR("STK PS :%s fail, err=0x%x", __func__, ps);
		return -EINVAL;
	}
	pdata_val = ps;
	return ps;
}

#if (defined(STK_MANUAL_CT_CALI) || defined(STK_MANUAL_GREYCARD_CALI))
#ifdef SITRONIX_PERMISSION_THREAD
SYSCALL_DEFINE3(fchmodat, int, dfd, const char __user *, filename, mode_t, mode);
static struct task_struct * SitronixPermissionThread = NULL;
static int sitronix_ps_delay_permission_thread_start = 5000;

static int sitronix_ts_permission_thread(void *data)
{
	int ret = 0;
	int retry = 0;
	mm_segment_t fs = get_fs();
	set_fs(KERNEL_DS);
	
	//INFO("%s start\n", __func__);
	do{
		INFO("delay %d ms\n", sitronix_ps_delay_permission_thread_start);
		msleep(sitronix_ps_delay_permission_thread_start);
		ret = sys_fchmodat(AT_FDCWD, "/sys/devices/platform/stk-oss/ps_cali" , 0664);
		if(ret < 0)
		printk("fail to execute sys_fchmodat, ret = %d\n", ret);
		if(retry++ > 10)
		break;
	}while(ret == -ENOENT);
	set_fs(fs);
	//INFO("%s exit\n", __func__);
	return 0;
}
#endif // SITRONIX_PERMISSION_THREAD

static int32_t get_ps_cali_file(char * r_buf, int8_t buf_size)
{
	struct file  *cali_file;
	mm_segment_t fs;	
	ssize_t ret;
	
    cali_file = filp_open(STK_CALI_FILE, O_RDWR,0);
    if(IS_ERR(cali_file))
	{
        ERR("STK PS %s: filp_open error!", __func__);
        return -ENOENT;
	}
	else
	{
		fs = get_fs();
		set_fs(get_ds());
		ret = cali_file->f_op->read(cali_file,r_buf,sizeof(r_buf),&cali_file->f_pos);
		if(ret < 0)
		{
			ERR("STK PS %s: read error, ret=%d", __func__, ret);
			filp_close(cali_file,NULL);
			return -EIO;
		}		
		set_fs(fs);
    }
	
    filp_close(cali_file,NULL);	
	return 0;	
}

static int32_t set_ps_cali_file(char * w_buf, int8_t buf_size)
{
	struct file  *cali_file;
	char r_buf[STK_CALI_FILE_SIZE] = {0};	
	mm_segment_t fs;	
	ssize_t ret;
	int8_t i;

	
    cali_file = filp_open(STK_CALI_FILE, O_CREAT | O_RDWR,0666);
	
    if(IS_ERR(cali_file))
	{
        ERR("STK PS %s: filp_open error!", __func__);
        return -ENOENT;
	}
	else
	{
		fs = get_fs();
		set_fs(get_ds());
		
		ret = cali_file->f_op->write(cali_file,w_buf,sizeof(w_buf),&cali_file->f_pos);
		if(ret != sizeof(w_buf))
		{
			ERR("STK PS %s: write error!", __func__);
			filp_close(cali_file,NULL);
			return -EIO;
		}
		cali_file->f_pos=0x00;
		ret = cali_file->f_op->read(cali_file,r_buf,sizeof(r_buf),&cali_file->f_pos);
		if(ret < 0)
		{
			ERR("STK PS %s: read error!", __func__);
			filp_close(cali_file,NULL);
			return -EIO;
		}		
		set_fs(fs);
		
		//INFO("STK PS %s: read ret=%d!", __func__, ret);
		for(i=0;i<STK_CALI_FILE_SIZE;i++)
		{
			if(r_buf[i] != w_buf[i])
			{
				ERR("STK PS %s: read back error!", __func__);
				filp_close(cali_file,NULL);
				return -EIO;
			}
		}
    }
    filp_close(cali_file,NULL);	
	//INFO("STK PS %s successfully\n", __func__);
	return 0;
}

static int32_t get_ps_cali_thd(int16_t *ps_thdh, int16_t *ps_thdl)
{
	char r_buf[STK_CALI_FILE_SIZE] = {0};
	int32_t ret;
	
	if ((ret = get_ps_cali_file(r_buf, STK_CALI_FILE_SIZE)) < 0)
	{
		return ret;
	}
	else
	{
		if(r_buf[0] == STK_CALI_VER0 && r_buf[1] == STK_CALI_VER1)
		{
			*ps_thdh = r_buf[2];
			*ps_thdl = r_buf[3];
			pStkPsData->ps_cali_done = true;
		}
		else
		{
			ERR("STK PS %s: cali version number error! r_buf=0x%x,0x%x,0x%x,0x%x\n", __func__, r_buf[0], r_buf[1], r_buf[2], r_buf[3]);						
			return -EINVAL;
		}
	}
	
	return 0;
}

static void set_ps_thd_to_file(int16_t ps_thd_data[])
{
	char w_buf[STK_CALI_FILE_SIZE] = {0};	
	
	w_buf[0] = STK_CALI_VER0;
	w_buf[1] = STK_CALI_VER1;
	w_buf[2] = ps_thd_data[STK_HIGH_THD];			
	w_buf[3] = ps_thd_data[STK_LOW_THD];			
	set_ps_cali_file(w_buf, STK_CALI_FILE_SIZE);			
}
#endif	/*	#if (defined(STK_MANUAL_CT_CALI) || defined(STK_MANUAL_GREYCARD_CALI))	*/

#if (defined(STK_MANUAL_CT_CALI) || defined(STK_AUTO_CT_CALI_NO_SATU) || defined(STK_AUTO_CT_CALI_SATU))
static int32_t calc_ct_cali_thd(int16_t ps_stat_data[], int16_t ps_thd_data[])
{
	ps_thd_data[STK_LOW_THD] = ps_stat_data[STK_DATA_MAX] + STK_THD_L_ABOVE_CT;
	ps_thd_data[STK_HIGH_THD] = ps_stat_data[STK_DATA_MAX] + STK_THD_H_ABOVE_CT;	
	//printk("STK PS : PS_threshold_high=0x%x\n PS_threshold_low=0x%x\n Max crosstalk=0x%x\n", ps_thd_data[STK_HIGH_THD], ps_thd_data[STK_LOW_THD], ps_stat_data[STK_DATA_MAX]);				
	if(ps_thd_data[STK_HIGH_THD] > STK_THD_MAX || ps_thd_data[STK_LOW_THD] > STK_THD_MAX) 
	{
		ERR("STK PS %s: threshold is too large!\n", __func__);
		return -11;
	}	
	STK_LOCK(1);
	pStkPsData->ps_cali_done = true;
	STK_LOCK(0);
	return 0;
}
#endif 	/*	#if (defined(STK_MANUAL_CT_CALI) || defined(STK_AUTO_CT_CALI_NO_SATU) || defined(STK_AUTO_CT_CALI_SATU))	*/

#ifdef STK_MANUAL_GREYCARD_CALI
static int32_t calc_greycard_cali_thd(int16_t ps_stat_data[], int16_t ps_thd_data[])
{
	ps_thd_data[STK_LOW_THD] = ps_stat_data[STK_DATA_MIN] - STK_DIFF_GREY_N_THD_L;
	ps_thd_data[STK_HIGH_THD] = ps_stat_data[STK_DATA_MIN] - STK_DIFF_GREY_N_THD_H;	
	
	//printk("STK PS : PS_threshold_high=0x%x\n PS_threshold_low=0x%x\n min ps reading=0x%x\n", ps_thd_data[STK_HIGH_THD], ps_thd_data[STK_LOW_THD], ps_stat_data[STK_DATA_MIN]);					
	if(ps_thd_data[STK_HIGH_THD] > STK_THD_MAX || ps_thd_data[STK_LOW_THD] > STK_THD_MAX) 
	{
		ERR("STK PS %s: threshold is too large\n ", __func__);
		return -11;
	}
	STK_LOCK(1);
	pStkPsData->ps_cali_done = true;
	STK_LOCK(0);		
	return 0;
}


static int32_t judge_grey_card_cali(int16_t ps_stat_data[])
{	
	int16_t diff_ps_data_ct = 0;
	
	if(ps_stat_data[STK_DATA_MIN] > STK_MIN_GREY_PS_DATA)
	{
		diff_ps_data_ct = ps_stat_data[STK_DATA_MAX] - ps_stat_data[STK_DATA_MIN];
		if (diff_ps_data_ct > STK_MAX_PS_DIFF_AUTO)		
		{
			ERR("STK PS : Min PS data:%d, Max - Min:%d\n", ps_stat_data[STK_DATA_MIN], diff_ps_data_ct);	
			ERR("STK PS : The difference between Max PS code and Min PS code is too large\n");
			return -1;
		}
	}
	else
	{
		ERR("STK PS : Min PS data:%d\n", ps_stat_data[STK_DATA_MIN]);	
		ERR("STK PS : PS reading is too small\n");
		return -2;				
	}
	
	return 0;
}
#endif	/* #ifdef STK_MANUAL_GREYCARD_CALI */

#if defined(STK_AUTO_CT_CALI_NO_SATU)
static int32_t judge_ct_cali_nosatu(int16_t ps_stat_data[])
{
	int16_t diff_ps_data_ct = 0;
	
	if(ps_stat_data[STK_DATA_MIN] < ps_code_low_thd)
	{
		diff_ps_data_ct = ps_stat_data[STK_DATA_MAX] - ps_stat_data[STK_DATA_MIN];
		if (diff_ps_data_ct > STK_MAX_PS_DIFF_AUTO)		
		{
			ERR("STK PS : Max PS data of crosstalk:%d, Max_crosstalk - Min_crosstalk:%d\n", ps_stat_data[STK_DATA_MAX], diff_ps_data_ct);	
			ERR("STK PS : The difference between Max PS code and Min PS code is too large\n");
			return -1;
		}
	}
	else
	{
		ERR("STK PS : Min PS data of crosstalk:%d is larger than ps_code_low_thd:%d\n", ps_stat_data[STK_DATA_MIN], ps_code_low_thd);	
		ERR("STK PS : PS crosstalk is too large\n");
		return -2;				
	}
	
	return 0;	
}
#endif	/*	#if defined(STK_AUTO_CT_CALI_NO_SATU)	*/

#if (defined(STK_MANUAL_CT_CALI) || defined(STK_AUTO_CT_CALI_SATU))
static int32_t judge_ct_cali_satu(int16_t ps_stat_data[])
{
	int16_t diff_ps_data_ct = 0;
	
	if(ps_stat_data[STK_DATA_MAX] < STK_MAX_PS_CROSSTALK)
	{
		diff_ps_data_ct = ps_stat_data[STK_DATA_MAX] - ps_stat_data[STK_DATA_MIN];
		if (diff_ps_data_ct > STK_MAX_PS_DIFF_AUTO)		
		{
			ERR("STK PS : Max PS data of crosstalk:%d, Max_ct - Min_ct:%d\n", ps_stat_data[STK_DATA_MAX], diff_ps_data_ct);	
			ERR("STK PS : The difference between Max PS code and Min PS code is too large\n");
			return -1;
		}
	}
	else
	{
		ERR("STK PS : Max PS data of crosstalk:%d\n", ps_stat_data[STK_DATA_MAX]);	
		ERR("STK PS : PS crosstalk is too large\n");
		return -2;				
	}
	
	return 0;
}
#endif	/*	#if (defined(STK_MANUAL_CT_CALI) || defined(STK_AUTO_CT_CALI_SATU))	*/

#if (defined(STK_MANUAL_CT_CALI) || defined(STK_MANUAL_GREYCARD_CALI) || defined(STK_AUTO_CT_CALI_NO_SATU) || defined(STK_AUTO_CT_CALI_SATU))
static void set_ps_thd_to_driver(int16_t ps_thd_data[])
{
	STK_LOCK(1);
	set_ps_thd_h(ps_thd_data[STK_HIGH_THD]);
	set_ps_thd_l(ps_thd_data[STK_LOW_THD]);
	STK_LOCK(0);	
}

static int32_t get_several_ps_data(int16_t ps_stat_data[])
{
	int32_t ps_data;
	int8_t data_count = 0;	
	uint16_t sample_time_ps = 0;		
	int32_t ave_ps_int32 = 0;
	ps_stat_data[STK_DATA_MAX] = 0;
	ps_stat_data[STK_DATA_MIN] = 9999;
	ps_stat_data[STK_DATA_AVE] = 0;
	
	STK_LOCK(1);	
	enable_ps(1);	
	sample_time_ps = pStkPsData->ps_cmd_reg;
	STK_LOCK(0);
	sample_time_ps&=STK_PS_CMD_SLP_MASK;
	sample_time_ps>>=STK_PS_CMD_SLP_SHIFT;	
	sample_time_ps = cali_sample_time_table[sample_time_ps];
	
	while(data_count < STK_CALI_SAMPLE_NO)
	{
		msleep(sample_time_ps);	
		STK_LOCK(1);
		ps_data = get_ps_reading();
		STK_LOCK(0);
		//INFO("STK PS %s: ps_cyc %d ps_data %d=%d\n", __func__, ps_cyc, data_count, ps_data);
		if(ps_data < 0)
		{
			STK_LOCK(1);
			enable_ps(0);
			STK_LOCK(0);
			return ps_data;
		}
		
		ave_ps_int32 +=  ps_data;			
		if(ps_data > ps_stat_data[STK_DATA_MAX])
			ps_stat_data[STK_DATA_MAX] = ps_data;
		if(ps_data < ps_stat_data[STK_DATA_MIN])
			ps_stat_data[STK_DATA_MIN] = ps_data;						
		data_count++;	
	}	
	ave_ps_int32 /= STK_CALI_SAMPLE_NO;	
	ps_stat_data[STK_DATA_AVE] = (int16_t)ave_ps_int32;
	//INFO("STK PS %s: ave_ps_int32 =%d\n", __func__,  ave_ps_int32);
	STK_LOCK(1);
	enable_ps(0);	/* force disable after cali to make sure interrupt is correct*/
	STK_LOCK(0);	
	return 0;
}
	
static int32_t set_ps_cali(void)
{
	int16_t ps_statistic_data[3] = {0};
	int32_t ret = 0;
	int16_t ps_thd_data[2] = {150, 140};
	
	ret = get_several_ps_data(ps_statistic_data);
	if(ret < 0)
	{
		ERR("STK PS %s: get_several_ps_data failed, ret=%d\n", __func__, ret);
		ERR("STK PS %s: calibration fail\n", __func__);
		return ret;
	}		
#ifdef STK_MANUAL_CT_CALI	
	ret = judge_ct_cali_satu(ps_statistic_data);
	if(ret == 0)
	{		
		ret = calc_ct_cali_thd(ps_statistic_data, ps_thd_data);
		if(ret < 0)
		{
			ERR("STK PS %s: calibration fail, errno=%d\n", __func__, ret);
			return ret;
		}
		set_ps_thd_to_driver(ps_thd_data);	
		set_ps_thd_to_file(ps_thd_data);			
		//printk("STK PS : PS calibration was done successfully\n");			
	}
	else
		ERR("STK PS %s: calibration fail, errno=%d\n", __func__, ret);
#elif defined(STK_MANUAL_GREYCARD_CALI)
	ret = judge_grey_card_cali(ps_statistic_data);
	if(ret == 0)
	{
		ret = calc_greycard_cali_thd(ps_statistic_data, ps_thd_data);
		if(ret < 0)
		{
			ERR("STK PS %s: calibration fail, errno=%d\n", __func__, ret);
			return ret;
		}		
		
		set_ps_thd_to_driver(ps_thd_data);	
		set_ps_thd_to_file(ps_thd_data);
		//printk("STK PS : PS calibration was done successfully\n");			
	}
	else
		ERR("STK PS %s: calibration fail, errno=%d\n", __func__, ret);	
#elif defined(STK_AUTO_CT_CALI_SATU)
	ret = judge_ct_cali_satu(ps_statistic_data);
	if(ret == 0)
	{		
		ret = calc_ct_cali_thd(ps_statistic_data, ps_thd_data);
		if(ret < 0)
		{
			ERR("STK PS %s: calibration fail, errno=%d\n", __func__, ret);
			return ret;
		}
		set_ps_thd_to_driver(ps_thd_data);				
		//printk("STK PS : PS calibration was done successfully, min = %d avg = %d max = %d\n", ps_statistic_data[0], ps_statistic_data[1], ps_statistic_data[2]);			
	}
	else if (ret == -2)
	{
		ps_thd_data[STK_HIGH_THD] =  STK_MAX_PS_CROSSTALK - STK_THD_H_BELOW_MAX_CT;
		ps_thd_data[STK_LOW_THD] = STK_MAX_PS_CROSSTALK	- STK_THD_L_BELOW_MAX_CT;	
		set_ps_thd_to_driver(ps_thd_data);	
	}
	else
		ERR("STK PS %s: calibration fail, errno=%d\n", __func__, ret);	
#elif defined(STK_AUTO_CT_CALI_NO_SATU)
	if(judge_ct_cali_nosatu(ps_statistic_data) == 0)
	{		
		ret = calc_ct_cali_thd(ps_statistic_data, ps_thd_data);
		if(ret < 0)
		{
			//ERR("STK PS %s: calibration fail, errno=%d\n", __func__, ret);
			return ret;
		}
		set_ps_thd_to_driver(ps_thd_data);				
		//printk("STK PS : PS calibration was done successfully\n");			
	}
	//else
		//ERR("STK PS %s: calibration fail, errno=%d\n", __func__, ret);	
#endif
	return ret;
}
#endif	/* #if (defined(STK_MANUAL_CT_CALI) || defined(STK_MANUAL_GREYCARD_CALI) || defined(STK_AUTO_CT_CALI_NO_SATU) || defined(STK_AUTO_CT_CALI_SATU)) */

inline void als_report_event(struct input_dev* dev,int32_t report_value)
{
    pStkPsData->als_lux_last = report_value;
	input_report_abs(dev, ABS_MISC, report_value);
	input_sync(dev);
	//INFO("STK PS : als input event %d lux\n",report_value);
}

inline void ps_report_event(struct input_dev* dev,int32_t report_value)
{
	/* Sometimes, when receiving a call on Firefox OS report_value is -1
	   when you take it, report_value must be 1 to prevent blank screen issues.
	*/
	if(report_value < 0)
		report_value = 1;

    pStkPsData->ps_distance_last = report_value;
	input_report_abs(dev, ABS_DISTANCE, report_value);
	input_sync(dev);
	wake_lock_timeout(&proximity_sensor_wakelock, 2*HZ);
	//INFO("STK PS : ps input event %d cm\n",report_value);
}

static int32_t enable_ps(uint8_t enable)
{
    int32_t ret;
    int32_t reading;
#if (defined(STK_MANUAL_CT_CALI) || defined(STK_MANUAL_GREYCARD_CALI))
	int16_t ps_thd_h = -1, ps_thd_l = -1;
	int16_t thd_h_auto = 0, thd_l_auto = 0;
	
	if(pStkPsData->first_boot == true)
	{		
		pStkPsData->first_boot = false;
		ret = get_ps_cali_thd(&ps_thd_h, &ps_thd_l);		
		if(ret == 0)
		{
			thd_l_auto = ps_thd_l;
			thd_h_auto = ps_thd_h;	
			//INFO("STK PS %s : read PS calibration data from file, thd_l_auto=%d, thd_h_auto=%d\n", 
			//	__func__, thd_l_auto, thd_h_auto);			
			if(thd_h_auto <= STK_THD_MAX && thd_l_auto <= STK_THD_MAX)
			{
				set_ps_thd_h(thd_h_auto);
				set_ps_thd_l(thd_l_auto);		
			}
			else
				ERR("STK PS %s: threshold is too large!\n", __func__);
		}			
	}
#endif	//	#if (defined(STK_MANUAL_CT_CALI) || defined(STK_MANUAL_GREYCARD_CALI))			
	
    pStkPsData->ps_cmd_reg &= (~STK_PS_CMD_SD_MASK);
    pStkPsData->ps_cmd_reg |= STK_PS_CMD_SD(enable?0:1);
    ret = i2c_smbus_write_byte_data(pStkPsData->client,STK_PS_CMD_REG,pStkPsData->ps_cmd_reg);

    if (enable) {
	msleep(100);
        reading = get_ps_reading();
        //INFO("%s : ps code = %d ps_code_high_thd = %d\n",__func__,reading, ps_code_high_thd);
        if(reading < 0)
        {
    	ERR("stk_oss_work:get_ps_reading fail, ret=%d", reading);
    	return reading; 			
        }
    
		ps_report_event(pStkPsData->ps_input_dev, 1);
        if (reading>=ps_code_high_thd)
        {
            ps_report_event(pStkPsData->ps_input_dev,0);
        }
        else if (reading<ps_code_high_thd)
        {
            ps_report_event(pStkPsData->ps_input_dev,1);
        }
    }
    return ret;

}
static int32_t enable_als(uint8_t enable)
{
    int32_t ret;
    if (enable)
        enable_als_int(0);
    pStkPsData->als_cmd_reg &= (~STK_ALS_CMD_SD_MASK);
    pStkPsData->als_cmd_reg |= STK_ALS_CMD_SD(enable?0:1);
    ret = i2c_smbus_write_byte_data(pStkPsData->client,STK_ALS_CMD_REG,pStkPsData->als_cmd_reg);

    if (enable)
    {		
        set_als_thd_h(0x0000);
        set_als_thd_l(0xFFFF);
        enable_als_int(1);
    }
    return ret;
}

#ifdef CONFIG_STK_SYSFS_DBG
// For Debug
static ssize_t help_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
     return sprintf(buf, "Usage : cat xxxx\nor echo val > xxxx\
     \nWhere xxxx = ps_code : RO (0~255)\nals_code : RO (0~65535)\nlux : RW (0~by your setting)\ndistance : RW (0 or 1)\
     \nals_enable : RW (0~1)\nps_enable : RW(0~1)\nals_transmittance : RW (1~10000)\
     \nps_sleep_time : RW (0~3)\nps_led_driving_current : RW(0~1)\nps_integral_time(0~3)\nps_gain_setting : RW(0~3)\n");

}

static ssize_t driver_version_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
    return sprintf(buf,"%s\n",STK_DRIVER_VER);
}

static ssize_t als_code_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
    int32_t reading;
    STK_LOCK(1);
    reading = get_als_reading();
    STK_LOCK(0);
    return sprintf(buf, "%d\n", reading);
}

static ssize_t ps_code_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
    int32_t reading;
    STK_LOCK(1);
    reading = get_ps_reading();
    STK_LOCK(0);
    return sprintf(buf, "%d\n", reading);
}
#endif //CONFIG_STK_SYSFS_DBG

static ssize_t lux_range_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
    return sprintf(buf, "%d\n", alscode2lux((1<<16) -1));//full code

}

static ssize_t dist_res_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
    return sprintf(buf, "1\n"); // means 1 cm in Android
}
static ssize_t lux_res_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
    return sprintf(buf, "1\n"); // means 1 lux in Android
}
static ssize_t distance_range_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
    return sprintf(buf, "%d\n",1);
}

static ssize_t ps_enable_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
    int32_t enable, ret;   
    STK_LOCK(1);
    enable = (pStkPsData->ps_cmd_reg & STK_PS_CMD_SD_MASK)?0:1;
    STK_LOCK(0);

    ret = i2c_smbus_read_byte_data(pStkPsData->client,STK_PS_CMD_REG);
    ret &= STK_PS_CMD_SD_MASK;
	ret = !ret;
	
	if(enable == ret)
		return sprintf(buf, "%d\n", ret);
	else
	{
		ERR("STK PS: %s: driver and sensor mismatch! driver_enable=0x%x, sensor_enable=%x\n", __func__, enable, ret);
		return sprintf(buf, "%d\n", ret);
	}
}

static ssize_t ps_enable_store(struct kobject *kobj,
                               struct kobj_attribute *attr,
                               const char *buf, size_t len)
{
    uint32_t value = simple_strtoul(buf, NULL, 10);
    //INFO("STK PS31xx Driver : Enable PS : %d\n",value);
    STK_LOCK(1);
    enable_ps(value);
    STK_LOCK(0);
    return len;
}

static ssize_t als_enable_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
    int32_t enable, ret;
    STK_LOCK(1);
    enable = (pStkPsData->als_cmd_reg & STK_ALS_CMD_SD_MASK)?0:1;
    STK_LOCK(0);
    ret = i2c_smbus_read_byte_data(pStkPsData->client,STK_ALS_CMD_REG);
    ret &= STK_ALS_CMD_SD_MASK;
	ret = !ret;
	
	if(enable == ret)
		return sprintf(buf, "%d\n", ret);
	else
	{
		ERR("STK PS: %s: driver and sensor mismatch! driver_enable=0x%x, sensor_enable=%x\n", __func__, enable, ret);
		return sprintf(buf, "%d\n", ret);
	}
}

static ssize_t als_enable_store(struct kobject *kobj,
                                struct kobj_attribute *attr,
                                const char *buf, size_t len)
{
    uint32_t value = simple_strtoul(buf, NULL, 10);
    //INFO("STK PS31xx Driver : Enable ALS : %d\n",value);
    STK_LOCK(1);
    enable_als(value);
    STK_LOCK(0);
    return len;
}

static ssize_t lux_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
    int32_t als_reading;
    STK_LOCK(1);
    als_reading = get_als_reading();
    STK_LOCK(0);
    return sprintf(buf, "%d lux\n", alscode2lux(als_reading));
}


static int32_t cdata_read_show(struct kobject *kobj, struct kobj_attribute * attr, char * buf)
{
	return sprintf(buf,"%d\n",cdata_val);
}

static int32_t pdata_read_show(struct kobject *kobj, struct kobj_attribute * attr, char * buf)
{
	return sprintf(buf,"%d\n",pdata_val);
}

static ssize_t lux_store(struct kobject *kobj,
                                struct kobj_attribute *attr,
                                const char *buf, size_t len)
{
    unsigned long value = simple_strtoul(buf, NULL, 16);
    STK_LOCK(1);
    als_report_event(pStkPsData->als_input_dev,value);
    STK_LOCK(0);
    return len;
}

static ssize_t distance_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
    int32_t dist=1;
    STK_LOCK(1);
		if (get_ps_reading()>=ps_code_high_thd)
        {
            ps_report_event(pStkPsData->ps_input_dev,0);
            dist=0;
        }
        else
        {
            ps_report_event(pStkPsData->ps_input_dev,1);
            dist=1;
        }
    STK_LOCK(0);
    return sprintf(buf, "%d\n", dist);
}

static ssize_t distance_store(struct kobject *kobj,
                                struct kobj_attribute *attr,
                                const char *buf, size_t len)
{
    unsigned long value = simple_strtoul(buf, NULL, 16);
    STK_LOCK(1);
    ps_report_event(pStkPsData->ps_input_dev,value);
    STK_LOCK(0);
    return len;
}

#if (defined(STK_MANUAL_CT_CALI) || defined(STK_MANUAL_GREYCARD_CALI))
static ssize_t ps_cali_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
	int cali;	
	int16_t ps_thd_h = 150, ps_thd_l = 140;
	int16_t	thd_auto[2] = {150, 140};
	int32_t ret;
	 
	ret = get_ps_cali_thd(&ps_thd_h, &ps_thd_l);		
	if(ret == 0)
	{		
		thd_auto[STK_LOW_THD] = ps_thd_l;
		thd_auto[STK_HIGH_THD] = ps_thd_h;	
		//INFO("STK PS %s : read PS calibration data from file, thd_auto[STK_LOW_THD]=%d, thd_auto[STK_HIGH_THD]=%d\n", 
		//__func__, thd_auto[STK_LOW_THD], thd_auto[STK_HIGH_THD]);			
	
		if(thd_auto[STK_HIGH_THD] <= STK_THD_MAX && thd_auto[STK_LOW_THD] <= STK_THD_MAX)		
			set_ps_thd_to_driver(thd_auto);			
		else		
			ERR("STK PS %s: threshold is too large!\n", __func__);			
	}
	else
		set_ps_cali();
		
	cali = pStkPsData->ps_cali_done?1:0;
	return sprintf(buf, "%d\n",cali);
}

static ssize_t ps_cali_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t len)
{
    unsigned long value;
	int ret;	
	if((ret = strict_strtoul(buf, 16, &value)) < 0)
	{
		ERR("STK PS %s:strict_strtoul failed, ret=0x%x", __func__, ret);
		return ret;	
	}
	
	if(value == 1)
		set_ps_cali();
	
	return len;
}

#endif	/* #if (defined(STK_MANUAL_CT_CALI) || defined(STK_MANUAL_GREYCARD_CALI)) */

#ifdef CONFIG_STK_SYSFS_DBG
/* Only for debug */
static struct kobj_attribute help_attribute = (struct kobj_attribute)__ATTR_RO(help);
static struct kobj_attribute driver_version_attribute = (struct kobj_attribute)__ATTR_RO(driver_version);
static struct kobj_attribute als_code_attribute = (struct kobj_attribute)__ATTR_RO(als_code);
static struct kobj_attribute ps_code_attribute = (struct kobj_attribute)__ATTR_RO(ps_code);
#endif //CONFIG_STK_SYSFS_DBG
static struct kobj_attribute lux_range_attribute = (struct kobj_attribute)__ATTR_RO(lux_range);
static struct kobj_attribute lux_attribute = (struct kobj_attribute)__ATTR_RW(lux);
static struct kobj_attribute distance_attribute = (struct kobj_attribute)__ATTR_RW(distance);
static struct kobj_attribute ps_enable_attribute = (struct kobj_attribute)__ATTR_RW(ps_enable);
static struct kobj_attribute als_enable_attribute = (struct kobj_attribute)__ATTR_RW(als_enable);
static struct kobj_attribute ps_dist_res_attribute = (struct kobj_attribute)__ATTR_RO(dist_res);
static struct kobj_attribute lux_res_attribute = (struct kobj_attribute)__ATTR_RO(lux_res);
static struct kobj_attribute ps_distance_range_attribute = (struct kobj_attribute)__ATTR_RO(distance_range);
static struct kobj_attribute pdata_read_attribute = (struct kobj_attribute)__ATTR_RO(pdata_read);
static struct kobj_attribute cdata_read_attribute = (struct kobj_attribute)__ATTR_RO(cdata_read);
#if (defined(STK_MANUAL_CT_CALI) || defined(STK_MANUAL_GREYCARD_CALI))
static struct kobj_attribute dev_ps_cali_attribute = (struct kobj_attribute)__ATTR_RW(ps_cali);
#endif	/*#if (defined(STK_MANUAL_CT_CALI) || defined(STK_MANUAL_GREYCARD_CALI)) */
#ifdef CONFIG_STK_SYSFS_DBG

static ssize_t als_transmittance_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
    int32_t transmittance;
    STK_LOCK(1);
    transmittance = als_transmittance;
    STK_LOCK(0);
    return sprintf(buf, "%d\n", transmittance);
}


static ssize_t als_transmittance_store(struct kobject *kobj,
                                       struct kobj_attribute *attr,
                                       const char *buf, size_t len)
{
    unsigned long value = simple_strtoul(buf, NULL, 10);
    STK_LOCK(1);
    als_transmittance = value;
    STK_LOCK(0);
    return len;
}



static struct kobj_attribute als_transmittance_attribute = (struct kobj_attribute)__ATTR_RW(als_transmittance);



#ifdef CONFIG_STK_PS_ENGINEER_TUNING

static ssize_t ps_sleep_time_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
    int32_t value;
    STK_LOCK(1);
    value = pStkPsData->ps_cmd_reg;
    STK_LOCK(0);
    value&=STK_PS_CMD_SLP_MASK;
    value>>=STK_PS_CMD_SLP_SHIFT;
    return sprintf(buf, "0x%x\n", value);
}

static ssize_t ps_sleep_time_store(struct kobject *kobj,
                                struct kobj_attribute *attr,
                                const char *buf, size_t len)
{
    unsigned long value = simple_strtoul(buf, NULL, 10);
    STK_LOCK(1);
    set_ps_slp(value);
    STK_LOCK(0);
    return len;
}

static ssize_t ps_led_driving_current_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
    int32_t value;
    STK_LOCK(1);
    value = pStkPsData->ps_cmd_reg;
    STK_LOCK(0);
    value&=STK_PS_CMD_DR_MASK;
    value>>=STK_PS_CMD_DR_SHIFT;
    return sprintf(buf, "0x%x\n", value);
}

static ssize_t ps_led_driving_current_store(struct kobject *kobj,
                                struct kobj_attribute *attr,
                                const char *buf, size_t len)
{
    unsigned long value = simple_strtoul(buf, NULL, 10);
    STK_LOCK(1);
    set_ps_led_driving_current(value);
    STK_LOCK(0);
    return len;
}

static ssize_t ps_integral_time_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
    int32_t value;
    STK_LOCK(1);
    value = pStkPsData->ps_cmd_reg;
    STK_LOCK(0);
    value&=STK_PS_CMD_IT_MASK;
    value>>=STK_PS_CMD_IT_SHIFT;
    return sprintf(buf, "0x%x\n", value);
}

static ssize_t ps_integral_time_store(struct kobject *kobj,
                                struct kobj_attribute *attr,
                                const char *buf, size_t len)
{
    unsigned long value = simple_strtoul(buf, NULL, 10);
    STK_LOCK(1);
    set_ps_it((uint8_t)value);
    STK_LOCK(0);
    return len;
}

static ssize_t ps_gain_setting_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
    int32_t gc_reg;
    STK_LOCK(1);
    gc_reg = pStkPsData->ps_gc_reg;
    STK_LOCK(0);
    return sprintf(buf, "0x%x\n", gc_reg);
}

static ssize_t ps_gain_setting_store(struct kobject *kobj,
                                struct kobj_attribute *attr,
                                const char *buf, size_t len)
{
    unsigned long value = simple_strtoul(buf, NULL, 10);
    STK_LOCK(1);
    set_ps_gc((uint8_t)value);
    STK_LOCK(0);
    return len;
}

static ssize_t ps_code_thd_l_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
    int32_t ps_thd_l_reg;
    STK_LOCK(1);
    ps_thd_l_reg = i2c_smbus_read_byte_data(pStkPsData->client,STK_PS_THD_L_REG);
    if(ps_thd_l_reg < 0)
	{
		ERR("STK PS :%s fail, err=0x%x", __func__, ps_thd_l_reg);	
		return -EINVAL;
	}
    STK_LOCK(0);
    return sprintf(buf, "%d\n", ps_thd_l_reg);
}

static ssize_t ps_code_thd_l_store(struct kobject *kobj,
                                struct kobj_attribute *attr,
                                const char *buf, size_t len)
{
    unsigned long value = simple_strtoul(buf, NULL, 10);
    STK_LOCK(1);
    set_ps_thd_l(value);
    STK_LOCK(0);
    return len;
}

static ssize_t ps_code_thd_h_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
    int32_t ps_thd_h_reg;
    STK_LOCK(1);
    ps_thd_h_reg = i2c_smbus_read_byte_data(pStkPsData->client,STK_PS_THD_H_REG);
    if(ps_thd_h_reg < 0)
	{
		ERR("STK PS :%s fail, err=0x%x", __func__, ps_thd_h_reg);		
		return -EINVAL;		
	}
    STK_LOCK(0);
    return sprintf(buf, "%d\n", ps_thd_h_reg);
}

static ssize_t ps_code_thd_h_store(struct kobject *kobj,
                                struct kobj_attribute *attr,
                                const char *buf, size_t len)
{
    unsigned long value = simple_strtoul(buf, NULL, 10);
    STK_LOCK(1);
    set_ps_thd_h(value);
    STK_LOCK(0);
    return len;
}

static ssize_t als_lux_thd_l_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
    int32_t als_thd_l0_reg,als_thd_l1_reg;
    STK_LOCK(1);
    als_thd_l0_reg = i2c_smbus_read_byte_data(pStkPsData->client,STK_ALS_THD_L0_REG);
    als_thd_l1_reg = i2c_smbus_read_byte_data(pStkPsData->client,STK_ALS_THD_L1_REG);
    STK_LOCK(0);
    if(als_thd_l0_reg < 0)
	{
		ERR("STK PS :%s fail, err=0x%x", __func__, als_thd_l0_reg);			
		return -EINVAL;
	}
	if(als_thd_l1_reg < 0)
	{
		ERR("STK PS :%s fail, err=0x%x", __func__, als_thd_l1_reg);				
		return -EINVAL;
	}
    als_thd_l0_reg|=(als_thd_l1_reg<<8);

    return sprintf(buf, "%d\n", alscode2lux(als_thd_l0_reg));
}

static ssize_t als_lux_thd_l_store(struct kobject *kobj,
                                struct kobj_attribute *attr,
                                const char *buf, size_t len)
{
    unsigned long value = simple_strtoul(buf, NULL, 20);
    value = lux2alscode(value);
    STK_LOCK(1);
    set_als_thd_l(value);
    STK_LOCK(0);
    return len;
}

static ssize_t als_lux_thd_h_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
    int32_t als_thd_h0_reg,als_thd_h1_reg;
    STK_LOCK(1);
    als_thd_h0_reg = i2c_smbus_read_byte_data(pStkPsData->client,STK_ALS_THD_H0_REG);
    als_thd_h1_reg = i2c_smbus_read_byte_data(pStkPsData->client,STK_ALS_THD_H1_REG);
    STK_LOCK(0);
    if(als_thd_h0_reg < 0)
	{
		ERR("STK PS :%s fail, err=0x%x", __func__, als_thd_h0_reg);			
		return -EINVAL;
	}
	if(als_thd_h1_reg < 0)
	{
		ERR("STK PS :%s fail, err=0x%x", __func__, als_thd_h1_reg);				
		return -EINVAL;
	}	
    als_thd_h0_reg|=(als_thd_h1_reg<<8);

    return sprintf(buf, "%d\n", alscode2lux(als_thd_h0_reg));
}

static ssize_t als_lux_thd_h_store(struct kobject *kobj,
                                struct kobj_attribute *attr,
                                const char *buf, size_t len)
{
    unsigned long value = simple_strtoul(buf, NULL, 20);
    value = lux2alscode(value);
    STK_LOCK(1);
    set_als_thd_h(value);
    STK_LOCK(0);
    return len;
}

static ssize_t all_reg_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
    int32_t ps_reg[13];
	uint8_t cnt;
    STK_LOCK(1);
	for(cnt=0;cnt<12;cnt++)
	{
		ps_reg[cnt] = i2c_smbus_read_byte_data(pStkPsData->client, (cnt+1));
		if(ps_reg[cnt] < 0)
		{
			STK_LOCK(0);
			ERR("all_reg_show:i2c_smbus_read_byte_data fail, ret=%d", ps_reg[cnt]);	
			return -EINVAL;
		}		
	}
	ps_reg[12] = i2c_smbus_read_byte_data(pStkPsData->client, STK_PS_GC_REG);
	if(ps_reg[12] < 0)
	{
		STK_LOCK(0);
		ERR("all_reg_show:i2c_smbus_read_byte_data fail, ret=%d", ps_reg[12]);	
		return -EINVAL;
	}
	//INFO("reg[0x82]=0x%2X\n", ps_reg[12]);	
    STK_LOCK(0);

    return sprintf(buf, "%2X %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X %2x\n", ps_reg[0], ps_reg[1], ps_reg[2], ps_reg[3], 
		ps_reg[4], ps_reg[5], ps_reg[6], ps_reg[7], ps_reg[8], ps_reg[9], ps_reg[10], ps_reg[11], ps_reg[12]);
}

static ssize_t recv_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
	return 0;
}

static ssize_t recv_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t len)
{
    unsigned long value;
	int ret;
	int32_t recv_data;	
	if((ret = strict_strtoul(buf, 16, &value)) < 0)
	{
		ERR("STK PS %s:strict_strtoul failed, ret=0x%x", __func__, ret);
		return ret;	
	}
	recv_data = i2c_smbus_read_byte_data(pStkPsData->client,value);
	//printk("STK PS: reg 0x%x=0x%x\n", (int)value, recv_data);
	return len;
}

static ssize_t send_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
	return 0;
}


static ssize_t send_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t len)
{
	int addr, cmd;
	u8 addr_u8, cmd_u8;
	s32 ret, i;
	char *token[10];
	
	for (i = 0; i < 2; i++)
		token[i] = strsep((char **)&buf, " ");
	if((ret = strict_strtoul(token[0], 16, (unsigned long *)&(addr))) < 0)
	{
		ERR("STK PS %s:strict_strtoul failed, ret=0x%x", __func__, ret);
		return ret;	
	}
	if((ret = strict_strtoul(token[1], 16, (unsigned long *)&(cmd))) < 0)
	{
		ERR("STK PS %s:strict_strtoul failed, ret=0x%x", __func__, ret);
		return ret;	
	}
	//INFO("STK PS: write reg 0x%x=0x%x\n", addr, cmd);		
	/*
		if(2 != sscanf(buf, "%2x %2x", &addr, &cmd))
		{
		ERR("STK PS %s: unknown format\n", __func__);
		return 0;
		}
	 */
	addr_u8 = (u8) addr;
	cmd_u8 = (u8) cmd;
	STK_LOCK(1);
	ret = i2c_smbus_write_byte_data(pStkPsData->client,addr_u8,cmd_u8);
	STK_LOCK(0);
	if (0 != ret)
	{	
		ERR("STK PS %s: i2c_smbus_write_byte_data fail\n", __func__);
		return ret;
	}
	
	return len;
}

static struct kobj_attribute ps_sleep_time_attribute = (struct kobj_attribute)__ATTR_RW(ps_sleep_time);
static struct kobj_attribute ps_led_driving_current_attribute = (struct kobj_attribute)__ATTR_RW(ps_led_driving_current);
static struct kobj_attribute ps_integral_time_attribute = (struct kobj_attribute)__ATTR_RW(ps_integral_time);
static struct kobj_attribute ps_gain_setting_attribute = (struct kobj_attribute)__ATTR_RW(ps_gain_setting);

static struct kobj_attribute ps_code_thd_l_attribute = (struct kobj_attribute)__ATTR_RW(ps_code_thd_l);
static struct kobj_attribute ps_code_thd_h_attribute = (struct kobj_attribute)__ATTR_RW(ps_code_thd_h);

static struct kobj_attribute als_lux_thd_l_attribute = (struct kobj_attribute)__ATTR_RW(als_lux_thd_l);
static struct kobj_attribute als_lux_thd_h_attribute = (struct kobj_attribute)__ATTR_RW(als_lux_thd_h);
static struct kobj_attribute all_reg_attribute = (struct kobj_attribute)__ATTR_RO(all_reg);
static struct kobj_attribute dev_recv_attribute = (struct kobj_attribute)__ATTR_RW(recv);
static struct kobj_attribute dev_send_attribute = (struct kobj_attribute)__ATTR_RW(send);

#endif //CONFIG_STK_PS_ENGINEER_TUNING
#endif //CONFIG_STK_SYSFS_DBG

static struct attribute* sensetek_optical_sensors_attrs [] =
{
    &lux_range_attribute.attr,
    &lux_attribute.attr,
    &distance_attribute.attr,
    &ps_enable_attribute.attr,
    &als_enable_attribute.attr,
    &ps_dist_res_attribute.attr,
    &lux_res_attribute.attr,
    &ps_distance_range_attribute.attr,
    &pdata_read_attribute.attr,
    &cdata_read_attribute.attr,    
#if (defined(STK_MANUAL_CT_CALI) || defined(STK_MANUAL_GREYCARD_CALI))
	&dev_ps_cali_attribute.attr,
#endif	
    NULL,
};

#ifdef CONFIG_STK_SYSFS_DBG

static struct attribute* sensetek_optical_sensors_dbg_attrs [] =
{
    &help_attribute.attr,
    &driver_version_attribute.attr,
    &lux_range_attribute.attr,
    &distance_attribute.attr,
    &ps_code_attribute.attr,
    &als_code_attribute.attr,
    &lux_attribute.attr,
    &ps_enable_attribute.attr,
    &als_enable_attribute.attr,
    &ps_dist_res_attribute.attr,
    &lux_res_attribute.attr,
    &ps_distance_range_attribute.attr,
    &als_transmittance_attribute.attr,
    &pdata_read_attribute.attr,
    &cdata_read_attribute.attr,        
#ifdef CONFIG_STK_PS_ENGINEER_TUNING
    &ps_sleep_time_attribute.attr,
    &ps_led_driving_current_attribute.attr,
    &ps_integral_time_attribute.attr,
    &ps_gain_setting_attribute.attr,
    &ps_code_thd_l_attribute.attr,
    &ps_code_thd_h_attribute.attr,
    &als_lux_thd_l_attribute.attr,
    &als_lux_thd_h_attribute.attr,
    &all_reg_attribute.attr,	
	&dev_recv_attribute.attr,
	&dev_send_attribute.attr,	
#endif //CONFIG_STK_PS_ENGINEER_TUNING
    NULL,
};

// those attributes are only for engineer test/debug
static struct attribute_group sensetek_optics_sensors_attrs_group =
{
    .name = "DBG",
    .attrs = sensetek_optical_sensors_dbg_attrs,
};
#endif //CONFIG_STK_SYSFS_DBG

static struct platform_device *stk_oss_dev = NULL; /* Device structure */

static int stk_sysfs_create_files(struct kobject *kobj,struct attribute** attrs)
{
    int err;
    while(*attrs!=NULL)
    {
        err = sysfs_create_file(kobj,*attrs);
        if (err)
            return err;
        attrs++;
    }
    return 0;
}

static struct workqueue_struct *stk_oss_work_queue = NULL;

static void stk_oss_work(struct work_struct *work)
{

    int32_t ret,reading;
    uint8_t disable_flag = 0;
#ifndef CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD	
	uint32_t nLuxIndex;	
#endif
	
    STK_LOCK(1);
    ret = get_status_reg();

	if(ret < 0)
	{
		STK_LOCK(0);
		ERR("stk_oss_work:get_status_reg fail, ret=%d", ret);
		msleep(30);
		enable_irq(pStkPsData->irq);
		return; 		
	}	
	
    if (ret&STK_PS_STATUS_ALS_INT_FLAG_MASK)
    {
		disable_flag = STK_PS_STATUS_ALS_INT_FLAG_MASK;
        reading = get_als_reading();
		//printk("%s: als_code = %d\n", __func__, reading);
		if(reading < 0)
		{
			STK_LOCK(0);
			ERR("stk_oss_work:get_als_reading fail, ret=%d", reading);
			msleep(30);
			enable_irq(pStkPsData->irq);
			return; 
		}
#ifndef CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD
        nLuxIndex = get_lux_interval_index(reading);
        set_als_thd_h(code_threshold_table[nLuxIndex]);
        set_als_thd_l(code_threshold_table[nLuxIndex-1]);
#else
        set_als_new_thd_by_reading(reading);
#endif //CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD
        als_report_event(pStkPsData->als_input_dev,alscode2lux(reading));
    }
    if (ret&STK_PS_STATUS_PS_INT_FLAG_MASK)
    {

        reading = get_ps_reading();
       // INFO("%s : ps code = %d ps_code_high_thd = %d\n",__func__,reading, ps_code_high_thd);
		if(reading < 0)
		{
			STK_LOCK(0);
			ERR("stk_oss_work:get_ps_reading fail, ret=%d", reading);
			msleep(30);
			enable_irq(pStkPsData->irq);
			return; 			
		}
        if (reading>=ps_code_high_thd)
        {
			disable_flag |= STK_PS_STATUS_PS_INT_FLAG_MASK;
            ps_report_event(pStkPsData->ps_input_dev,0);
            away = 0;
        }
        else if (reading<ps_code_high_thd)
        {
			disable_flag |= STK_PS_STATUS_PS_INT_FLAG_MASK;
            ps_report_event(pStkPsData->ps_input_dev,1);
            away = 1;
	} else
	    msleep(10);
    }

    ret = reset_int_flag(ret,disable_flag);
	if(ret < 0)
	{
		STK_LOCK(0);
		ERR("stk_oss_work:reset_int_flag fail, ret=%d", ret);
		msleep(30);
		enable_irq(pStkPsData->irq);
		return; 		
	}		
	
	msleep(1);
    enable_irq(pStkPsData->irq);

    STK_LOCK(0);
}

static irqreturn_t stk_oss_irq_handler(int irq, void *data)
{
	struct stkps31xx_data *pData = data;
	disable_irq_nosync(irq);
	queue_work(stk_oss_work_queue,&pData->work);
	return IRQ_HANDLED;
}

static int stk31xx_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int32_t enable;
	int err;

	//INFO("%s\n", __func__);
	STK_LOCK(1);
    	enable = (pStkPsData->ps_cmd_reg & STK_PS_CMD_SD_MASK)?0:1;    		
	if(enable)
	{
		if (away) {
			away = 0;
			WARNING("%s: is away\n", __func__);
			STK_LOCK(0);
			return -EAGAIN;
		}

		disable_irq(pStkPsData->irq);	
		err = enable_irq_wake(pStkPsData->irq);	
		if (err)
		{
			WARNING("%s: set_irq_wake(%d,1) failed for (%d)\n",
			__func__, pStkPsData->irq, err);
		}		
	}
	STK_LOCK(0);
	return 0;
}

static int stk31xx_resume(struct i2c_client *client)
{
	int32_t enable;
	int err;

	//INFO("%s\n", __func__);
	STK_LOCK(1);
    	enable = (pStkPsData->ps_cmd_reg & STK_PS_CMD_SD_MASK)?0:1;    		
	if(enable)
	{
		err = disable_irq_wake(pStkPsData->irq);	
		enable_irq(pStkPsData->irq);	
		if (err)
		{
			WARNING("%s: disable_irq_wake(%d,1) failed for (%d)\n",
			__func__, pStkPsData->irq, err);
		}		
	}
	STK_LOCK(0);

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void stk31xx_early_suspend(struct early_suspend *h)
{
	int32_t enable;
	//INFO("%s", __func__);

    	STK_LOCK(1);
    	enable = (pStkPsData->als_cmd_reg & STK_ALS_CMD_SD_MASK)?0:1;    		
	if(enable)
	{
		enable_als(0);		
		pStkPsData->als_cmd_reg &= (~STK_ALS_CMD_SD_MASK);	
	}
	STK_LOCK(0);		
	return;
}

static void stk31xx_late_resume(struct early_suspend *h)
{
	int32_t enable;
	
	//INFO("%s", __func__);	
    	STK_LOCK(1);
    	enable = (pStkPsData->als_cmd_reg & STK_ALS_CMD_SD_MASK)?0:1;    		
	if(enable)
	{
		enable_als(1);		
	}
	
	STK_LOCK(0);
	return;
}
#endif	//#ifdef CONFIG_HAS_EARLYSUSPEND

bool ps31xx_sensor_id(void)
{
    // printk("cellon read ps31xx_sensor_id = %d\n", psensor_is_good);
     
     if (psensor_is_good >= 0)
	return true;
     else
        return false;
}
EXPORT_SYMBOL(ps31xx_sensor_id);

static int stk_ps_probe(struct i2c_client *client,
                        const struct i2c_device_id *id)
{
    /*
    printk("STKPS -- %s: I2C is probing (%s)%d\n nDetect = %d\n", __func__,id->name,id->driver_data);
    */
    int err;
    struct stkps31xx_data* ps_data;
	struct stk31xx_platform_data* plat_data;
    //INFO("STK PS : I2C Probing");
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA))
    {
        printk("STKPS -- No Support for I2C_FUNC_SMBUS_BYTE_DATA\n");
        return -ENODEV;
    }
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_WORD_DATA))
    {
        printk("STKPS -- No Support for I2C_FUNC_SMBUS_WORD_DATA\n");
        return -ENODEV;
    }

    if (id->driver_data == 0)
    {
	err = i2c_smbus_read_byte_data(client,STK_PS_STATUS_REG);
	if (err < 0)
	{
		ERR("STK PS %s: read i2c error, err=0x%x\n", __func__, err);
		return err;
	}
	if ((err&STK_PS_STATUS_ID_MASK)!=STK_PS31xx_ID)
	{
		ERR("STK PS %s: invalid ID number", __func__);
		return -EINVAL;
	}			

        ps_data = kzalloc(sizeof(struct stkps31xx_data),GFP_KERNEL);
        ps_data->client = client;
        i2c_set_clientdata(client,ps_data);
	if(client->dev.platform_data != NULL)		
	{
		plat_data = client->dev.platform_data;	
		als_transmittance = plat_data->transmittance;			
	}
	else
	{
		ERR("STK PS %s: Please set stk31xx platform data!\n", __func__);
		return EINVAL;
	}		
        mutex_init(&stkps_io_lock);
        ps_data->als_input_dev = input_allocate_device();
        ps_data->ps_input_dev = input_allocate_device();
        if ((ps_data->als_input_dev==NULL)||(ps_data->ps_input_dev==NULL))
        {
            if (ps_data->als_input_dev==NULL)
                input_free_device(ps_data->ps_input_dev);                
            if (ps_data->ps_input_dev==NULL)
				input_free_device(ps_data->als_input_dev);
            ERR("%s: could not allocate input device\n", __func__);
            mutex_destroy(&stkps_io_lock);
            kfree(ps_data);
            return -ENOMEM;
        }
        ps_data->als_input_dev->name = ALS_NAME;
        ps_data->ps_input_dev->name = PS_NAME;
        set_bit(EV_ABS, ps_data->als_input_dev->evbit);
//        set_bit(EV_ABS, ps_data->ps_input_dev->evbit);
	input_set_capability(ps_data->ps_input_dev, EV_ABS, ABS_DISTANCE);
        input_set_abs_params(ps_data->als_input_dev, ABS_MISC, 0, alscode2lux((1<<16)-1), 0, 0);
        input_set_abs_params(ps_data->ps_input_dev, ABS_DISTANCE, 0,1, 0, 0);
        err = input_register_device(ps_data->als_input_dev);
        if (err<0)
        {
            ERR("STK PS : can not register als input device\n");
            mutex_destroy(&stkps_io_lock);
            input_free_device(ps_data->als_input_dev);
            input_free_device(ps_data->ps_input_dev);
            kfree(ps_data);

            return err;
        }
        //INFO("STK PS : register als input device OK\n");
        err = input_register_device(ps_data->ps_input_dev);
        if (err<0)
        {
            ERR("STK PS : can not register ps input device\n");
            input_unregister_device(ps_data->als_input_dev);
            mutex_destroy(&stkps_io_lock);
            input_free_device(ps_data->als_input_dev);
            input_free_device(ps_data->ps_input_dev);
            kfree(ps_data);
            return err;

        }
        pStkPsData = ps_data;
        ps_data->ps_delay = PS_ODR_DELAY;
        ps_data->als_delay = ALS_ODR_DELAY;
        //printk("STK PS : gpio #=%d, irq=%d\n",plat_data->int_pin, gpio_to_irq(plat_data->int_pin));
		client->irq = gpio_to_irq(plat_data->int_pin);
        if (client->irq<=0)
        {
            ERR("STK PS : fail --> you don't(or forget to) specify a irq number, irq # = %d\n or irq_to_gpio fail\n",client->irq);
            input_unregister_device(ps_data->als_input_dev);
            mutex_destroy(&stkps_io_lock);
            input_free_device(ps_data->als_input_dev);
            input_free_device(ps_data->ps_input_dev);
            kfree(ps_data);
            return -EINVAL;
        }
        stk_oss_work_queue = create_workqueue("stk_oss_wq");
        INIT_WORK(&ps_data->work, stk_oss_work);
        psensor_is_good = enable_als(0);
        enable_ps(0);
#if ADDITIONAL_GPIO_CFG 
	err = gpio_tlmm_config(GPIO_CFG(plat_data->int_pin, 0, GPIO_CFG_INPUT, 
		GPIO_CFG_PULL_UP, GPIO_CFG_8MA), GPIO_CFG_ENABLE);
        err = gpio_request(plat_data->int_pin,"stk-int");         /* Request for gpio pin */
	if (err)
		printk("STK INT gpio request err - %d\n", err);

	gpio_direction_input(plat_data->int_pin);
#endif 
        err = request_irq(client->irq, stk_oss_irq_handler, IRQF_TRIGGER_LOW, DEVICE_NAME, ps_data);
        if (err < 0) {
            WARNING("%s: request_irq(%d) failed for (%d)\n",
                __func__, client->irq, err);
            input_unregister_device(ps_data->als_input_dev);
            mutex_destroy(&stkps_io_lock);
            input_free_device(ps_data->als_input_dev);
            input_free_device(ps_data->ps_input_dev);
            kfree(ps_data);				
            return err;
        }
        pStkPsData->irq = client->irq;

        wake_lock_init(&proximity_sensor_wakelock,WAKE_LOCK_IDLE,"stk_ps_wakelock");

#ifndef CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD
        init_code_threshold_table();
#endif
        if (!init_all_setting(plat_data))
        {
			free_irq(client->irq, NULL);
            input_unregister_device(pStkPsData->als_input_dev);
            input_unregister_device(pStkPsData->ps_input_dev);
            input_free_device(pStkPsData->als_input_dev);
            input_free_device(pStkPsData->ps_input_dev);
            kfree(pStkPsData);
            pStkPsData = NULL;
            return -EINVAL;
        }
#ifdef CONFIG_HAS_EARLYSUSPEND
		ps_data->stk_early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
		ps_data->stk_early_suspend.suspend = stk31xx_early_suspend;
		ps_data->stk_early_suspend.resume = stk31xx_late_resume;
		register_early_suspend(&ps_data->stk_early_suspend);
#endif
        return 0;
    }

    return -EINVAL;
}

static int stk_ps_remove(struct i2c_client *client)
{
	struct stk31xx_platform_data* plat_data;
	plat_data = client->dev.platform_data;

    mutex_destroy(&stkps_io_lock);
    wake_lock_destroy(&proximity_sensor_wakelock);
    if (pStkPsData)
    {
		//disable_irq(pStkPsData->irq);
		free_irq(pStkPsData->irq, NULL);
#if ADDITIONAL_GPIO_CFG // Additional GPIO CFG
		gpio_free(plat_data->int_pin);
#endif // Additional GPIO CFG
        if (stk_oss_work_queue)
            destroy_workqueue(stk_oss_work_queue);
        input_unregister_device(pStkPsData->als_input_dev);
        input_unregister_device(pStkPsData->ps_input_dev);
        input_free_device(pStkPsData->als_input_dev);
        input_free_device(pStkPsData->ps_input_dev);
        kfree(pStkPsData);
        pStkPsData = 0;
    }
    return 0;
}

static const struct i2c_device_id stk_ps_id[] =
{
    { "stk_ps", 0},
    {}
};
MODULE_DEVICE_TABLE(i2c, stk_ps_id);

static struct i2c_driver stk_ps_driver =
{
    .driver = {
        .name = STKPS_DRV_NAME,
    },
    .suspend = stk31xx_suspend,
    .resume = stk31xx_resume,
    .probe = stk_ps_probe,
    .remove = stk_ps_remove,
    .id_table = stk_ps_id,
};

static int __init stk_i2c_ps31xx_init(void)
{

	int ret;
    ret = i2c_add_driver(&stk_ps_driver);
    if (ret)
        return ret;
    if (pStkPsData == NULL)
        return -EINVAL;

    stk_oss_dev = platform_device_alloc(DEVICE_NAME,-1);
    if (!stk_oss_dev)
    {
       i2c_del_driver(&stk_ps_driver);
       return -ENOMEM;
    }
    if (platform_device_add(stk_oss_dev))
    {
       i2c_del_driver(&stk_ps_driver);
       return -ENOMEM;
    }
    ret = stk_sysfs_create_files(&(stk_oss_dev->dev.kobj),sensetek_optical_sensors_attrs);
    if (ret)
    {
        i2c_del_driver(&stk_ps_driver);
        return -ENOMEM;
    }
#ifdef CONFIG_STK_SYSFS_DBG
    ret = sysfs_create_group(&(stk_oss_dev->dev.kobj), &sensetek_optics_sensors_attrs_group);
    if (ret)
    {
        i2c_del_driver(&stk_ps_driver);
        return -ENOMEM;
    }
#endif //CONFIG_STK_SYSFS_DBG
#ifdef SITRONIX_PERMISSION_THREAD
	SitronixPermissionThread = kthread_run(sitronix_ts_permission_thread,"Sitronix","Permissionthread");
	if(IS_ERR(SitronixPermissionThread))
	SitronixPermissionThread = NULL;
#endif // SITRONIX_PERMISSION_THREAD	
	//INFO("STK PS Module initialized.\n");
    return 0;
}

static void __exit stk_i2c_ps31xx_exit(void)
{
    if (stk_oss_dev);
    {
#ifdef CONFIG_STK_SYSFS_DBG
        sysfs_remove_group(&(stk_oss_dev->dev.kobj), &sensetek_optics_sensors_attrs_group);
#endif
    }
    platform_device_put(stk_oss_dev);
    i2c_del_driver(&stk_ps_driver);
    platform_device_unregister(stk_oss_dev);
#ifdef SITRONIX_PERMISSION_THREAD
	if(SitronixPermissionThread)
	SitronixPermissionThread = NULL;
#endif // SITRONIX_PERMISSION_THREAD		
}

MODULE_AUTHOR("Patrick Chang <patrick_chang@sitronix.com.tw>");
MODULE_DESCRIPTION("SenseTek Proximity Sensor driver");
MODULE_LICENSE("GPL");
module_init(stk_i2c_ps31xx_init);
module_exit(stk_i2c_ps31xx_exit);