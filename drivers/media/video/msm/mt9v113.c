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
#include "mt9v113.h"
/******************************************************************************
Debug Switch
******************************************************************************/
#ifdef CDBG
#undef CDBG
#endif
#ifdef CDBG_HIGH
#undef CDBG_HIGH
#endif

//#define MT9V113_VERBOSE_DGB

#ifdef MT9V113_VERBOSE_DGB
#define CDBG(fmt, args...)        pr_debug(fmt, ##args)
#define CDBG_HIGH(fmt, args...)   pr_debug(fmt, ##args)
#else
#define CDBG(fmt, args...)        do { } while (0)
#define CDBG_HIGH(fmt, args...)   printk(fmt, ##args)
#endif

/******************************************************************************
Local Definition
******************************************************************************/
#define MT9V113_I2C_MAX_TIMES 10

static int mt9v113_pwdn_gpio;
static int mt9v113_reset_gpio;
static int mt9v113_vcm_pwd_gpio;

static int MT9V113_CSI_CONFIG = 0;

struct mt9v113_work {
  struct work_struct work;
};
static struct mt9v113_work *mt9v113_sensorw;
static struct i2c_client    *mt9v113_client;
static DECLARE_WAIT_QUEUE_HEAD(mt9v113_wait_queue);
DEFINE_MUTEX(mt9v113_mutex);
static u8 mt9v113_i2c_buf[4] = {0};
static u8 mt9v113_counter = 0;
static int16_t mt9v113_effect = CAMERA_EFFECT_OFF;
static int is_autoflash = 0;
static int mt9v113_effect_value = 0;

struct __mt9v113_ctrl
{
  const struct msm_camera_sensor_info *sensordata;
  int sensormode;
  uint fps_divider;
  uint pict_fps_divider;
  u16 curr_step_pos;
  u16 curr_lens_pos;
  u16 init_curr_lens_pos;
  u16 my_reg_gain;
  u16 my_reg_line_count;
  enum mt9v113_resolution_t prev_res;
  enum mt9v113_resolution_t pict_res;
  enum mt9v113_resolution_t curr_res;
  enum mt9v113_test_mode_t set_test;
};
static struct __mt9v113_ctrl *mt9v113_ctrl;
static int afinit = 1;

extern struct rw_semaphore leds_list_lock;
extern struct list_head leds_list;
extern int lcd_camera_power_onoff(int on);

/******************************************************************************
Local Function Declare
******************************************************************************/
static int mt9v113_i2c_txdata(u16 saddr,u8 *txdata,int length);
static int mt9v113_i2c_write_word(unsigned short saddr,
                                  unsigned int waddr,
                                  unsigned int wdata);
static int mt9v113_i2c_write_byte(unsigned short saddr,
                                  unsigned int waddr,
                                  unsigned int wdata);
static int mt9v113_i2c_rxdata(unsigned short saddr,
                              unsigned char *rxdata,
                              int length);
static int32_t mt9v113_i2c_read_word(unsigned short saddr,
                                     unsigned int raddr,
                                     unsigned int *rdata);
static int32_t MT9V113_WriteRegsTbl(PMT9V113_WREG pTb,int32_t len);
static void mt9v113_stream_switch(int bOn);
static int mt9v113_set_flash_light(enum led_brightness brightness);
static int mt9v113_sensor_setting(int update_type, int rt);
static int mt9v113_video_config(int mode);
static int mt9v113_snapshot_config(int mode);
static int mt9v113_raw_snapshot_config(int mode);
static int mt9v113_sensor_open_init(const struct msm_camera_sensor_info *data);
static int mt9v113_sensor_release(void);
static int mt9v113_i2c_remove(struct i2c_client *client);
static int mt9v113_init_client(struct i2c_client *client);
static int mt9v113_set_sensor_mode(int mode,int res);
static long mt9v113_set_effect(int mode, int effect);
static void mt9v113_power_off(void);
static int mt9v113_set_brightness(int8_t brightness);
static int mt9v113_set_contrast(int contrast);
static long mt9v113_set_exposure_mode(int mode);
static int mt9v113_set_wb_oem(uint8_t param);
static long mt9v113_set_antibanding(int antibanding);
static int32_t mt9v113_lens_shading_enable(uint8_t is_enable);
static int mt9v113_set_saturation(int saturation);
static int mt9v113_set_sharpness(int sharpness);
static int mt9v113_set_touchaec(uint32_t x,uint32_t y);
static int mt9v113_sensor_start_af(void);
static int mt9v113_set_exposure_compensation(int compensation);
static int mt9v113_set_iso(int8_t iso_type);
static int mt9v113_sensor_config(void __user *argp);
static void mt9v113_power_on(void);
static void mt9v113_power_reset(void);
static int mt9v113_read_model_id(const struct msm_camera_sensor_info *data);
static int mt9v113_probe_init_gpio(const struct msm_camera_sensor_info *data);
static void mt9v113_probe_free_gpio(const struct msm_camera_sensor_info *data);
static int mt9v113_sensor_probe(const struct msm_camera_sensor_info *info,
                                struct msm_sensor_ctrl *s);
static int mt9v113_i2c_probe(struct i2c_client *client,
                             const struct i2c_device_id *id);
static int __mt9v113_probe(struct platform_device *pdev);
static int __init mt9v113_init(void);
/******************************************************************************

******************************************************************************/
static int mt9v113_i2c_txdata(u16 saddr,u8 *txdata,int length)
{
  struct i2c_msg msg[] = {
    {
      .addr  = saddr,
      .flags = 0,
      .len = length,
      .buf = txdata,
    },
  };
  if (i2c_transfer(mt9v113_client->adapter, msg, 1) < 0)
    return -EIO;
  else
    return 0;
}

/******************************************************************************

******************************************************************************/
static int mt9v113_i2c_write_word(unsigned short saddr,
                                  unsigned int waddr,
                                  unsigned int wdata)
{
  int rc = -EIO;
  mt9v113_counter = 0;
  mt9v113_i2c_buf[0] = (waddr & 0xFF00)>>8;
  mt9v113_i2c_buf[1] = (waddr & 0x00FF);
  mt9v113_i2c_buf[2] = (wdata & 0xFF00)>>8;
  mt9v113_i2c_buf[3] = (wdata & 0x00FF);
  CDBG("Mt9v113 W_Word: saddr:0x%x, waddr:0x%x, wdata:0x%x\r\n", saddr, waddr,wdata);
  while ((mt9v113_counter < MT9V113_I2C_MAX_TIMES) &&(rc != 0))
  {
    rc = mt9v113_i2c_txdata(saddr, mt9v113_i2c_buf, 4);
    if (rc < 0)
    {
      mt9v113_counter++;
      CDBG("mt9v113_i2c_txdata failed,Reg(0x%04x) = 0x%04x, rc=%d!\n",
           waddr, wdata,rc);
    }
  }
  return rc;
}

/******************************************************************************

******************************************************************************/
static int mt9v113_i2c_write_byte(unsigned short saddr,
                                  unsigned int waddr,
                                  unsigned int wdata)
{
  int rc = -EIO;
  mt9v113_counter = 0;
  mt9v113_i2c_buf[0] = (waddr & 0xFF00)>>8;
  mt9v113_i2c_buf[1] = (waddr & 0x00FF);
  mt9v113_i2c_buf[2] = (wdata & 0x00FF);

  CDBG("Mt9v113 W_Byte: saddr:0x%x, waddr:0x%x, wdata:0x%x\r\n", saddr, waddr,wdata);
  while ((mt9v113_counter < MT9V113_I2C_MAX_TIMES) &&(rc != 0))
  {
    rc = mt9v113_i2c_txdata(saddr, mt9v113_i2c_buf, 3);
    if (rc < 0)
    {
      mt9v113_counter++;
      CDBG("mt9v113_i2c_txdata failed,Reg(0x%04x) = 0x%02x, rc=%d!\n",
            waddr,wdata,rc);
    }
  }
  return rc;
}

/******************************************************************************

******************************************************************************/
static int mt9v113_i2c_rxdata(unsigned short saddr,
                              unsigned char *rxdata,
                              int length)
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

    if (i2c_transfer(mt9v113_client->adapter, msgs, 2) < 0) {
        CDBG("mt9v113_i2c_rxdata failed!\n");
        return -EIO;
    }

    return 0;
}

/******************************************************************************

******************************************************************************/
static int32_t mt9v113_i2c_read_word(unsigned short saddr,
                                     unsigned int raddr,
                                     unsigned int *rdata)
{
    int32_t rc = 0;
    unsigned char buf[4] = {0};

    if (!rdata)
        return -EIO;

    buf[0] = (raddr & 0xFF00)>>8;
    buf[1] = (raddr & 0x00FF);

    rc = mt9v113_i2c_rxdata(saddr, buf, 2);
    if (rc < 0) {
        CDBG("mt9v113_i2c_read_word failed!\n");
        return rc;
    }

    *rdata = buf[0] << 8 | buf[1];
    CDBG("Mt9v113 read: saddr:0x%x, raddr:0x%x, rdata:0x%x\r\n", saddr, raddr,*rdata);
    return rc;
}

/******************************************************************************

******************************************************************************/
static int32_t MT9V113_WriteRegsTbl(PMT9V113_WREG pTb,int32_t len)
{
  int32_t i, ret = 0;
  for (i = 0;i < len; i++)
  {
    if (0 == pTb[i].data_format){
      /*Reg Data Format:16Bit = 0*/
      if(0 == pTb[i].delay_ms){
        mt9v113_i2c_write_word(mt9v113_client->addr,pTb[i].addr,pTb[i].data);
      }else{
        mt9v113_i2c_write_word(mt9v113_client->addr,pTb[i].addr,pTb[i].data);
        msleep(pTb[i].delay_ms);
      }
    } else {
      /*Reg Data Format:8Bit = 1*/
      if(0 == pTb[i].delay_ms){
        mt9v113_i2c_write_byte(mt9v113_client->addr,pTb[i].addr,pTb[i].data);
      }else{
        mt9v113_i2c_write_byte(mt9v113_client->addr,pTb[i].addr,pTb[i].data);
        msleep(pTb[i].delay_ms);
      }
    }
  }
  return ret;
}

/******************************************************************************

******************************************************************************/
static void mt9v113_stream_switch(int bOn)
{
  if (bOn == 0) {
    CDBG_HIGH("mt9v113_stream_switch: off\n");
    mt9v113_i2c_write_word(mt9v113_client->addr,0x3400,0x7a26);
  }
  else {
    CDBG_HIGH("mt9v113_stream_switch: on\n");
    mt9v113_i2c_write_word(mt9v113_client->addr,0x3400,0x7a2c);
  }
}

/******************************************************************************

******************************************************************************/

static int mt9v113_set_flash_light(enum led_brightness brightness)
{
  CDBG("%s brightness = %d\n",__func__ , brightness);
  #if 0
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
    CDBG("get flashlight device failed\n");
    return -1;
  }
  #endif
  return 0;
}
/******************************************************************************

******************************************************************************/
static int mt9v113_sensor_setting(int update_type, int rt)
{
  int rc = -EINVAL;
  struct msm_camera_csi_params mt9v113_csi_params;
  CDBG_HIGH("update_type = %d, rt = %d\n", update_type, rt);
  if (update_type == S_REG_INIT) {
    rc = MT9V113_WriteSettingTbl(mt9v113_init_tbl);
    if (rc < 0) {
      CDBG_HIGH("sensor init setting failed\n");
      return rc;
    }
    CDBG("AF_init: afinit = %d\n", afinit);
    if (afinit == 1) {
      rc = MT9V113_WriteSettingTbl(mt9v113_afinit_tbl);
      if (rc < 0) {
        CDBG_HIGH("mt9v113 af setting failed\n");
        return rc;
      }
      afinit = 0;
    }
    /* reset fps_divider */
    mt9v113_ctrl->fps_divider = 1 * 0x0400;
    rc = 0;
  } else if (update_type == S_UPDATE_PERIODIC) {
    mt9v113_stream_switch(0); //stream off
    msleep(10);
    if(!MT9V113_CSI_CONFIG) {
      mt9v113_csi_params.lane_cnt = 1;
      mt9v113_csi_params.data_format = CSI_8BIT;
      mt9v113_csi_params.lane_assign = 0xe4;
      mt9v113_csi_params.dpcm_scheme = 0;
      mt9v113_csi_params.settle_cnt = 30;
      rc = msm_camio_csi_config(&mt9v113_csi_params);
      MT9V113_CSI_CONFIG = 1;
    }
    //turn off flash when preview
    mt9v113_set_flash_light(LED_OFF);
    if (rt == S_QTR_SIZE) {
      /* preview setting */
      rc = MT9V113_WriteSettingTbl(mt9v113_preview_tbl_30fps);
      CDBG("%s, mt9v113_preview_tbl_30fps is set\n",__func__);
    } else if (rt == S_FULL_SIZE) {
      CDBG("snapshot in, is_autoflash - %d\n", is_autoflash);
      if (is_autoflash == 1) {
        mt9v113_set_flash_light(LED_FULL);
      }
      rc = MT9V113_WriteSettingTbl(mt9v113_capture_tbl);
      CDBG("%s, mt9v113_capture_tbl is set\n",__func__);
    }
    msleep(10);
    mt9v113_stream_switch(1); //stream on
  }
  return rc;
}

/******************************************************************************

******************************************************************************/
static int mt9v113_video_config(int mode)
{
  int rc = 0;
  CDBG("%s\n",__func__);
  if ((rc = mt9v113_sensor_setting(S_UPDATE_PERIODIC,
    mt9v113_ctrl->prev_res)) < 0) {
    CDBG_HIGH("mt9v113_video_config failed\n");
    return rc;
  }
  mt9v113_ctrl->curr_res = mt9v113_ctrl->prev_res;
  mt9v113_ctrl->sensormode = mode;
  return rc;
}

/******************************************************************************

******************************************************************************/
static int mt9v113_snapshot_config(int mode)
{
  int rc = 0;
  CDBG("%s\n",__func__);
  if (mt9v113_ctrl->curr_res != mt9v113_ctrl->pict_res) {
    if ((rc = mt9v113_sensor_setting(S_UPDATE_PERIODIC,
      mt9v113_ctrl->pict_res)) < 0) {
      CDBG_HIGH("mt9v113_snapshot_config failed\n");
      return rc;
    }
  }
  mt9v113_ctrl->curr_res = mt9v113_ctrl->pict_res;
  mt9v113_ctrl->sensormode = mode;
  return rc;
}

/******************************************************************************

******************************************************************************/
static int mt9v113_raw_snapshot_config(int mode)
{
  int rc = 0;
  CDBG("%s\n",__func__);
  if (mt9v113_ctrl->curr_res != mt9v113_ctrl->pict_res) {
    if ((rc = mt9v113_sensor_setting(S_UPDATE_PERIODIC,
        mt9v113_ctrl->pict_res)) < 0) {
      CDBG_HIGH("mt9v113_raw_snapshot_config failed\n");
      return rc;
    }
  }
  mt9v113_ctrl->curr_res = mt9v113_ctrl->pict_res;
  mt9v113_ctrl->sensormode = mode;
  return rc;
}

/******************************************************************************

******************************************************************************/
static int mt9v113_sensor_open_init(const struct msm_camera_sensor_info *data)
{
  int rc = -ENOMEM;
  CDBG_HIGH("%s\n",__func__);
  mt9v113_ctrl = kzalloc(sizeof(struct __mt9v113_ctrl), GFP_KERNEL);
  if (!mt9v113_ctrl)
  {
    CDBG_HIGH("kzalloc mt9v113_ctrl error\n");
    return rc;
  }
  mt9v113_ctrl->fps_divider = 1 * 0x00000400;
  mt9v113_ctrl->pict_fps_divider = 1 * 0x00000400;
  mt9v113_ctrl->set_test = S_TEST_OFF;
  mt9v113_ctrl->prev_res = S_QTR_SIZE;
  mt9v113_ctrl->pict_res = S_FULL_SIZE;
  if (data)
    mt9v113_ctrl->sensordata = data;
  lcd_camera_power_onoff(1);
  mt9v113_power_off();
  CDBG_HIGH("%s: msm_camio_clk_rate_set\n",__func__);
  msm_camio_clk_rate_set(24000000);
  msleep(5);
  mt9v113_power_on();
  msleep(5);
  mt9v113_power_reset();
  msleep(5);
  rc = mt9v113_sensor_setting(S_REG_INIT, mt9v113_ctrl->prev_res);
  if (rc < 0)
  {
    CDBG_HIGH("%s :mt9v113_sensor_setting failed. rc = %d\n",__func__,rc);
    kfree(mt9v113_ctrl);
    mt9v113_ctrl = NULL;
    mt9v113_power_off();
    lcd_camera_power_onoff(0);
    return rc;
  }
  MT9V113_CSI_CONFIG = 0;
  CDBG_HIGH("%s: re_init_sensor ok\n",__func__);
  return rc;
}

/******************************************************************************

******************************************************************************/
static int mt9v113_sensor_release(void)
{
  CDBG_HIGH("%s\n",__func__);
  mutex_lock(&mt9v113_mutex);
  mt9v113_power_off();
  lcd_camera_power_onoff(0);
  if(NULL != mt9v113_ctrl)
  {
    kfree(mt9v113_ctrl);
    mt9v113_ctrl = NULL;
  }
  MT9V113_CSI_CONFIG = 0;
  mutex_unlock(&mt9v113_mutex);
  return 0;
}

/******************************************************************************

******************************************************************************/

static const struct i2c_device_id mt9v113_i2c_id[] = {
  {"mt9v113", 0},{}
};

/******************************************************************************

******************************************************************************/
static int mt9v113_i2c_remove(struct i2c_client *client)
{
  if(NULL != mt9v113_sensorw)
  {
    kfree(mt9v113_sensorw);
    mt9v113_sensorw = NULL;
  }
  mt9v113_client = NULL;
  return 0;
}

/******************************************************************************

******************************************************************************/
static int mt9v113_init_client(struct i2c_client *client)
{
  init_waitqueue_head(&mt9v113_wait_queue);
  return 0;
}

/******************************************************************************
case CFG_SET_MODE:                  // 0
******************************************************************************/
static int mt9v113_set_sensor_mode(int mode,int res)
{
  int rc = 0;
  CDBG("%s mode = %d\n",__func__ , mode);
  switch (mode) {
    case SENSOR_PREVIEW_MODE:
    case SENSOR_HFR_60FPS_MODE:
    case SENSOR_HFR_90FPS_MODE:
      {
        mt9v113_ctrl->prev_res = res;
        rc = mt9v113_video_config(mode);
      }
      break;
    case SENSOR_SNAPSHOT_MODE:
      {
        mt9v113_ctrl->pict_res = res;
        rc = mt9v113_snapshot_config(mode);
      }
      break;
    case SENSOR_RAW_SNAPSHOT_MODE:
      {
        mt9v113_ctrl->pict_res = res;
        rc = mt9v113_raw_snapshot_config(mode);
      }
      break;
    default:
      {
        rc = -EINVAL;
      }
      break;
  }
  return rc;
}

/******************************************************************************
case CFG_SET_EFFECT:                // 1
******************************************************************************/
static long mt9v113_set_effect(int mode, int effect)
{
  int rc = 0;
  CDBG("%s mode = %d,effect = %d\n",__func__ ,mode,effect);
  switch (mode)
  {
    case SENSOR_PREVIEW_MODE:
    case SENSOR_HFR_60FPS_MODE:
    case SENSOR_HFR_90FPS_MODE:
      {
        ;
      }
      break;
    case SENSOR_SNAPSHOT_MODE:
      {
        ;
      }
      break;
    default:
      {
        CDBG("%s Default(Not Support)\n",__func__);
      }
      break;
  }
  mt9v113_effect_value = effect;
  switch (effect)
  {
    case CAMERA_EFFECT_OFF:        //0
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_effect_normal_tbl);
      }
      break;
    case CAMERA_EFFECT_MONO:       //1
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_effect_mono_tbl);
      }
      break;
    case CAMERA_EFFECT_NEGATIVE:   //2
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_effect_negative_tbl);
      }
      break;
    case CAMERA_EFFECT_SOLARIZE:   //3
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_effect_solarize_tbl);
      }
      break;
    case CAMERA_EFFECT_SEPIA:       //4
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_effect_sepia_tbl);
      }
      break;
    case CAMERA_EFFECT_POSTERIZE:   //5
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_effect_posterize_tbl);
      }
      break;
    case CAMERA_EFFECT_WHITEBOARD:  //6
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_effect_whiteboard_tbl);
      }
      break;
    case CAMERA_EFFECT_BLACKBOARD:  //7
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_effect_blackboard_tbl);
      }
      break;
    case CAMERA_EFFECT_AQUA:        //8
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_effect_aqua_tbl);
      }
      break;
    case CAMERA_EFFECT_BW:         //10
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_effect_bw_tbl);
      }
      break;
    case CAMERA_EFFECT_BLUISH:     //11
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_effect_bluish_tbl);
      }
      break;
    case CAMERA_EFFECT_REDDISH:    //13
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_effect_reddish_tbl);
      }
      break;
    case CAMERA_EFFECT_GREENISH:   //14
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_effect_greenish_tbl);
      }
      break;
    default:
      {
        CDBG("%s ...Default(Not Support)\n",__func__);
      }
      break;
  }
  mt9v113_effect = effect;
  return rc;
}

/******************************************************************************
case CFG_PWR_DOWN:                  // 4
******************************************************************************/
static void mt9v113_power_off(void)
{
  CDBG("%s\n",__func__);
  gpio_set_value(mt9v113_pwdn_gpio, 1);
}

/******************************************************************************
case CFG_SET_BRIGHTNESS:            //12
******************************************************************************/
static int mt9v113_set_brightness(int8_t brightness)
{
  int rc = 0;
  CDBG("%s brightness = %d\n",__func__ , brightness);
  switch (brightness){
    case CAMERA_BRIGHTNESS_LV0:
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_brightness_lv0_tbl);
      }
      break;
    case CAMERA_BRIGHTNESS_LV1:
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_brightness_lv1_tbl);
      }
      break;
    case CAMERA_BRIGHTNESS_LV2:
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_brightness_lv2_tbl);
      }
      break;
    case CAMERA_BRIGHTNESS_LV3:
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_brightness_lv3_tbl);
      }
      break;
    case CAMERA_BRIGHTNESS_LV4:
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_brightness_default_lv4_tbl);
      }
      break;
    case CAMERA_BRIGHTNESS_LV5:
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_brightness_lv5_tbl);
      }
      break;
    case CAMERA_BRIGHTNESS_LV6:
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_brightness_lv6_tbl);
      }
      break;
    case CAMERA_BRIGHTNESS_LV7:
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_brightness_lv7_tbl);
      }
      break;
    case CAMERA_BRIGHTNESS_LV8:
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_brightness_lv8_tbl);
      }
      break;
    default:
      {
        CDBG("CAMERA_BRIGHTNESS_ERROR COMMAND\n");
      }
      break;
  }
  return rc;
}

/******************************************************************************
case CFG_SET_CONTRAST:              //13
******************************************************************************/
static int mt9v113_set_contrast(int contrast)
{
  int rc = 0;
  CDBG("%s contrast = %d\n",__func__ , contrast);
  if (mt9v113_effect_value == CAMERA_EFFECT_OFF)
  {
    switch (contrast)
    {
      case CAMERA_CONTRAST_LV0:
        {
          rc = MT9V113_WriteSettingTbl(mt9v113_contrast_lv0_tbl);
        }
        break;
      case CAMERA_CONTRAST_LV1:
        {
          rc = MT9V113_WriteSettingTbl(mt9v113_contrast_lv1_tbl);
        }
        break;
      case CAMERA_CONTRAST_LV2:
        {
          rc = MT9V113_WriteSettingTbl(mt9v113_contrast_lv2_tbl);
        }
        break;
      case CAMERA_CONTRAST_LV3:
        {
          rc = MT9V113_WriteSettingTbl(mt9v113_contrast_lv3_tbl);
        }
        break;
      case CAMERA_CONTRAST_LV4:
        {
          rc = MT9V113_WriteSettingTbl(mt9v113_contrast_default_lv4_tbl);
        }
        break;
      case CAMERA_CONTRAST_LV5:
        {
          rc = MT9V113_WriteSettingTbl(mt9v113_contrast_lv5_tbl);
        }
        break;
      case CAMERA_CONTRAST_LV6:
        {
          rc = MT9V113_WriteSettingTbl(mt9v113_contrast_lv6_tbl);
        }
        break;
      case CAMERA_CONTRAST_LV7:
        {
          rc = MT9V113_WriteSettingTbl(mt9v113_contrast_lv7_tbl);
        }
        break;
      case CAMERA_CONTRAST_LV8:
        {
          rc = MT9V113_WriteSettingTbl(mt9v113_contrast_lv8_tbl);
        }
        break;
      default:
        {
          CDBG("CAMERA_CONTRAST_ERROR COMMAND\n");
        }
        break;
    }
  }
  return rc;
}

/******************************************************************************
case CFG_SET_EXPOSURE_MODE:         //15
******************************************************************************/
static long mt9v113_set_exposure_mode(int mode)
{
  long rc = 0;
  CDBG("%s ...mode = %d\n",__func__ , mode);
  switch (mode)
  {
    case CAMERA_SETAE_AVERAGE:
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_ae_average_tbl);
      }
      break;
    case CAMERA_SETAE_CENWEIGHT:
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_ae_centerweight_tbl);
      }
      break;
    default:
      {
        CDBG("ERROR COMMAND OR NOT SUPPORT\n");
      }
      break;
  }
  return rc;
}

/******************************************************************************
case CFG_SET_WB:                    //16
******************************************************************************/
static int mt9v113_set_wb_oem(uint8_t param)
{
  int rc = 0;
  CDBG("%s param = %d\n", __func__, param);
  switch(param){
    case CAMERA_WB_AUTO:              //1
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_wb_Auto);
      }
      break;
    case CAMERA_WB_CUSTOM:            //2
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_wb_custom);
      }
      break;
    case CAMERA_WB_INCANDESCENT:      //3
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_wb_inc);
      }
      break;
    case CAMERA_WB_FLUORESCENT:       //4
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_wb_fluorescent);
      }
      break;
    case CAMERA_WB_DAYLIGHT:          //5
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_wb_daylight);
      }
      break;
    case CAMERA_WB_CLOUDY_DAYLIGHT:   //6
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_wb_cloudy);
      }
      break;
    case CAMERA_WB_TWILIGHT:          //7
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_wb_twilight);
      }
      break;
    case CAMERA_WB_SHADE:            //8
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_wb_shade);
      }
      break;
    default:
      break;
  }
  return rc;
}

/******************************************************************************
case CFG_SET_ANTIBANDING:           //17
******************************************************************************/
static long mt9v113_set_antibanding(int antibanding)
{
  long rc = 0;
  CDBG("%s antibanding = %d\n", __func__, antibanding);
  switch (antibanding)
  {
    case CAMERA_ANTIBANDING_OFF:
      {
        ;
      }
      break;
    case CAMERA_ANTIBANDING_60HZ:
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_antibanding_60z_tbl);
      }
      break;
    case CAMERA_ANTIBANDING_50HZ:
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_antibanding_50z_tbl);
      }
      break;
    case CAMERA_ANTIBANDING_AUTO:
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_antibanding_auto_tbl);
      }
      break;
    default:
      {
        CDBG("CAMERA_ANTIBANDING_ERROR COMMAND\n");
      }
      break;
  }
  return rc;
}

/******************************************************************************
case CFG_SET_LENS_SHADING:          //20
******************************************************************************/
static int32_t mt9v113_lens_shading_enable(uint8_t is_enable)
{
  int32_t rc = 0;
  if(is_enable)
  {
    CDBG("%s: enable\n",__func__);
    rc = MT9V113_WriteSettingTbl(mt9v113_lens_shading_on_tbl);
  }
  else
  {
    CDBG("%s: disable\n",__func__);
    rc = MT9V113_WriteSettingTbl(mt9v113_lens_shading_off_tbl);
  }
  return rc;
}

/******************************************************************************
case CFG_SET_SATURATION:            //32
******************************************************************************/
static int mt9v113_set_saturation(int saturation)
{
  long rc = 0;
  CDBG("%s saturation = %d\n",__func__ , saturation);
  if (mt9v113_effect_value == CAMERA_EFFECT_OFF)
  {
    switch (saturation){
      case CAMERA_SATURATION_LV0:    //0
        {
          rc = MT9V113_WriteSettingTbl(mt9v113_saturation_lv0_tbl);
        }
        break;
      case CAMERA_SATURATION_LV1:    //1
        {
          rc = MT9V113_WriteSettingTbl(mt9v113_saturation_lv1_tbl);
        }
        break;
      case CAMERA_SATURATION_LV2:    //2
        {
          rc = MT9V113_WriteSettingTbl(mt9v113_saturation_lv2_tbl);
        }
        break;
      case CAMERA_SATURATION_LV3:    //3
        {
          rc = MT9V113_WriteSettingTbl(mt9v113_saturation_lv3_tbl);
        }
        break;
      case CAMERA_SATURATION_LV4:    //4
        {
          rc = MT9V113_WriteSettingTbl(mt9v113_saturation_default_lv4_tbl);
        }
        break;
      case CAMERA_SATURATION_LV5:    //5
        {
          rc = MT9V113_WriteSettingTbl(mt9v113_saturation_lv5_tbl);
        }
        break;
      case CAMERA_SATURATION_LV6:    //6
        {
          rc = MT9V113_WriteSettingTbl(mt9v113_saturation_lv6_tbl);
        }
        break;
      case CAMERA_SATURATION_LV7:    //7
        {
          rc = MT9V113_WriteSettingTbl(mt9v113_saturation_lv7_tbl);
        }
        break;
      case CAMERA_SATURATION_LV8:    //8
        {
          rc = MT9V113_WriteSettingTbl(mt9v113_saturation_lv8_tbl);
        }
        break;
      default:
        {
          CDBG("CAMERA_SATURATION_ERROR COMMAND\n");
        }
        break;
    }
  }
  return rc;
}

/******************************************************************************
case CFG_SET_SHARPNESS:             //33
******************************************************************************/
static int mt9v113_set_sharpness(int sharpness)
{
  int rc = 0;
  CDBG("%s sharpness = %d\n",__func__ , sharpness);
  if (mt9v113_effect_value == CAMERA_EFFECT_OFF)
  {
    switch(sharpness){
      case CAMERA_SHARPNESS_LV0:    //0
        {
          rc = MT9V113_WriteSettingTbl(mt9v113_sharpness_lv0_tbl);
        }
        break;
      case CAMERA_SHARPNESS_LV1:    //5
        {
          rc = MT9V113_WriteSettingTbl(mt9v113_sharpness_lv1_tbl);
        }
        break;
      case CAMERA_SHARPNESS_LV2:    //10
        {
          rc = MT9V113_WriteSettingTbl(mt9v113_sharpness_default_lv2_tbl);
        }
        break;
      case CAMERA_SHARPNESS_LV3:    //15
        {
          rc = MT9V113_WriteSettingTbl(mt9v113_sharpness_lv3_tbl);
        }
        break;
      case CAMERA_SHARPNESS_LV4:    //20
        {
          rc = MT9V113_WriteSettingTbl(mt9v113_sharpness_lv4_tbl);
        }
        break;
      case CAMERA_SHARPNESS_LV5:    //25
        {
          rc = MT9V113_WriteSettingTbl(mt9v113_sharpness_lv5_tbl);
        }
        break;
      case CAMERA_SHARPNESS_LV6:    //30
        {
          rc = MT9V113_WriteSettingTbl(mt9v113_sharpness_lv6_tbl);
        }
        break;
      case CAMERA_SHARPNESS_LV7:    //35
        {
          rc = MT9V113_WriteSettingTbl(mt9v113_sharpness_lv7_tbl);
        }
        break;
      case CAMERA_SHARPNESS_LV8:    //40
        {
          rc = MT9V113_WriteSettingTbl(mt9v113_sharpness_lv8_tbl);
        }
        break;
      default:
        {
          CDBG("CAMERA_SHARPNESS_ERROR COMMAND\n");
        }
        break;
    }
  }
  return rc;
}

/******************************************************************************
case CFG_SET_TOUCHAEC:              //34
******************************************************************************/
static int mt9v113_set_touchaec(uint32_t x,uint32_t y)
{
  int rc = 0;
  return rc;
}

/******************************************************************************
case CFG_SET_AUTO_FOCUS:            //35
******************************************************************************/
static int mt9v113_sensor_start_af(void)
{
  int rc = 0;
  return rc;
}

/******************************************************************************
case CFG_SET_EXPOSURE_COMPENSATION: //37
******************************************************************************/
static int mt9v113_set_exposure_compensation(int compensation)
{
  long rc = 0;
  CDBG("%s compensation = %d\n",__func__ , compensation);
  switch(compensation)
  {
    case CAMERA_EXPOSURE_COMPENSATION_LV0:  // 12
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_exposure_compensation_lv0_tbl);
      }
      break;
    case CAMERA_EXPOSURE_COMPENSATION_LV1:  //  6
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_exposure_compensation_lv1_tbl);
      }
      break;
    case CAMERA_EXPOSURE_COMPENSATION_LV2:  //  0
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_exposure_compensation_lv2_default_tbl);
      }
      break;
    case CAMERA_EXPOSURE_COMPENSATION_LV3:  // -6
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_exposure_compensation_lv3_tbl);
      }
      break;
    case CAMERA_EXPOSURE_COMPENSATION_LV4:  //-12
      {
        rc = MT9V113_WriteSettingTbl(mt9v113_exposure_compensation_lv4_tbl);
      }
      break;
    default:
      {
        CDBG("ERROR CAMERA_EXPOSURE_COMPENSATION\n");
      }
      break;
  }
  return rc;
}

/******************************************************************************
case CFG_SET_ISO:                   //38
******************************************************************************/
static int mt9v113_set_iso(int8_t iso_type)
{
  long rc = 0;
  CDBG("%s iso_type = %d\n",__func__ , iso_type);
  switch(iso_type){
  case CAMERA_ISO_TYPE_AUTO:        // 0
    {
      rc = MT9V113_WriteSettingTbl(mt9v113_iso_type_auto);
    }
    break;
  case CAMEAR_ISO_TYPE_HJR:         // 1
    {
      rc = MT9V113_WriteSettingTbl(mt9v113_iso_type_auto);
    }
    break;
  case CAMEAR_ISO_TYPE_100:         // 2
    {
      rc = MT9V113_WriteSettingTbl(mt9v113_iso_type_100);
    }
    break;
  case CAMERA_ISO_TYPE_200:         // 3
    {
      rc = MT9V113_WriteSettingTbl(mt9v113_iso_type_200);
    }
    break;
  case CAMERA_ISO_TYPE_400:         // 4
    {
      rc = MT9V113_WriteSettingTbl(mt9v113_iso_type_400);
    }
    break;
  case CAMEAR_ISO_TYPE_800:         // 5
    {
      rc = MT9V113_WriteSettingTbl(mt9v113_iso_type_800);
    }
    break;
  case CAMERA_ISO_TYPE_1600:        // 6
    {
      rc = MT9V113_WriteSettingTbl(mt9v113_iso_type_1600);
    }
    break;
  default:
    {
      CDBG("ERROR ISO TYPE\n");
    }
    break;
  }
  return rc;
}

/******************************************************************************

******************************************************************************/
static int mt9v113_sensor_config(void __user *argp)
{
  struct sensor_cfg_data cdata;
  long rc = 0;
  if (copy_from_user(&cdata, (void *)argp, sizeof(struct sensor_cfg_data)))
    return -EFAULT;
  CDBG("%s %d\n",__func__,cdata.cfgtype);
  mutex_lock(&mt9v113_mutex);
  switch (cdata.cfgtype) {
    case CFG_SET_MODE:                  // 0
      {
        rc =mt9v113_set_sensor_mode(cdata.mode, cdata.rs);
      }
      break;
    case CFG_SET_EFFECT:                // 1
      {
        rc = mt9v113_set_effect(cdata.mode, cdata.cfg.effect);
      }
      break;
    case CFG_START:                     // 2
      {
        rc = -EINVAL;
      }
      break;
    case CFG_PWR_UP:                    // 3
      {
        rc = -EINVAL;
      }
      break;
    case CFG_PWR_DOWN:                  // 4
      {
        mt9v113_power_off();
      }
      break;
    case CFG_WRITE_EXPOSURE_GAIN:       // 5
      {
        rc = -EINVAL;
      }
      break;
    case CFG_SET_DEFAULT_FOCUS:         // 6
      {
        rc = -EINVAL;
      }
      break;
    case CFG_MOVE_FOCUS:                // 7
      {
        rc = -EINVAL;
      }
      break;
    case CFG_REGISTER_TO_REAL_GAIN:     // 8
      {
        rc = -EINVAL;
      }
      break;
    case CFG_REAL_TO_REGISTER_GAIN:     // 9
      {
        rc = -EINVAL;
      }
      break;
    case CFG_SET_FPS:                   //10
      {
        rc = -EINVAL;
      }
      break;
    case CFG_SET_PICT_FPS:              //11
      {
        rc = -EINVAL;
      }
      break;
    case CFG_SET_BRIGHTNESS:            //12
      {
        rc = mt9v113_set_brightness(cdata.cfg.brightness);
      }
      break;
    case CFG_SET_CONTRAST:              //13
      {
        rc = mt9v113_set_contrast(cdata.cfg.contrast);
      }
      break;
    case CFG_SET_ZOOM:                  //14
      {
        rc = -EINVAL;
      }
      break;
    case CFG_SET_EXPOSURE_MODE:         //15
      {
        rc = mt9v113_set_exposure_mode(cdata.cfg.ae_mode);
      }
      break;
    case CFG_SET_WB:                    //16
      {
        mt9v113_set_wb_oem(cdata.cfg.wb_val);
      }
      break;
    case CFG_SET_ANTIBANDING:           //17
      {
        rc = mt9v113_set_antibanding(cdata.cfg.antibanding);
      }
      break;
    case CFG_SET_EXP_GAIN:              //18
      {
        rc = -EINVAL;
      }
      break;
    case CFG_SET_PICT_EXP_GAIN:         //19
      {
        rc = -EINVAL;
      }
      break;
    case CFG_SET_LENS_SHADING:          //20
      {
        rc = mt9v113_lens_shading_enable(cdata.cfg.lens_shading);
      }
      break;
    case CFG_GET_PICT_FPS:              //21
      {
        rc = -EINVAL;
      }
      break;
    case CFG_GET_PREV_L_PF:             //22
      {
        rc = -EINVAL;
      }
      break;
    case CFG_GET_PREV_P_PL:             //23
      {
        rc = -EINVAL;
      }
      break;
    case CFG_GET_PICT_L_PF:             //24
      {
        rc = -EINVAL;
      }
      break;
    case CFG_GET_PICT_P_PL:             //25
      {
        rc = -EINVAL;
      }
      break;
    case CFG_GET_AF_MAX_STEPS:          //26
      {
        rc = -EINVAL;
      }
      break;
    case CFG_GET_PICT_MAX_EXP_LC:       //27
      {
        rc = -EINVAL;
      }
      break;
    case CFG_SEND_WB_INFO:              //28
      {
        rc = -EINVAL;
      }
      break;
    case CFG_SENSOR_INIT:               //29
      {
        rc = -EINVAL;
      }
      break;
    case CFG_GET_3D_CALI_DATA:          //30
      {
        rc = -EINVAL;
      }
      break;
    case CFG_GET_CALIB_DATA:            //31
      {
        rc = -EINVAL;
      }
      break;
    case CFG_SET_SATURATION:            //32
      {
        rc = mt9v113_set_saturation(cdata.cfg.saturation);
      }
      break;
    case CFG_SET_SHARPNESS:             //33
      {
        rc = mt9v113_set_sharpness(cdata.cfg.sharpness);
      }
      break;
    case CFG_SET_TOUCHAEC:              //34
      {
        rc = mt9v113_set_touchaec(cdata.cfg.aec_cord.x, cdata.cfg.aec_cord.y);
      }
      break;
    case CFG_SET_AUTO_FOCUS:            //35
      {
        rc = mt9v113_sensor_start_af();
      }
      break;
    case CFG_SET_AUTOFLASH:             //36
      {
        is_autoflash = cdata.cfg.is_autoflash;
      }
      break;
    case CFG_SET_EXPOSURE_COMPENSATION: //37
      {
        rc = mt9v113_set_exposure_compensation(cdata.cfg.exp_compensation);
      }
      break;
    case CFG_SET_ISO:                   //38
      {
        rc = mt9v113_set_iso(cdata.cfg.iso_type);
      }
      break;
    case CFG_SET_AUTO_LED_START:        //39
      {
        rc = -EINVAL;
      }
      break;
    default:
      CDBG("%s: Command=%d (Not Implement)!!\n",__func__,cdata.cfgtype);
      break;
  }
  mutex_unlock(&mt9v113_mutex);
  return rc;
}

/******************************************************************************

******************************************************************************/
static struct i2c_driver mt9v113_i2c_driver = {
    .id_table = mt9v113_i2c_id,
    .probe  = mt9v113_i2c_probe,
    .remove = mt9v113_i2c_remove,
    .driver = {
        .name = "mt9v113",
    },
};

/******************************************************************************

******************************************************************************/
static void mt9v113_power_on(void)
{
  CDBG_HIGH("%s\n",__func__);
  gpio_set_value(mt9v113_pwdn_gpio, 0);
}

/******************************************************************************

******************************************************************************/
static void mt9v113_power_reset(void)
{
  CDBG_HIGH("%s\n",__func__);
  gpio_set_value(mt9v113_reset_gpio, 1);
  mdelay(1);
  gpio_set_value(mt9v113_reset_gpio, 0);
  mdelay(1);
  gpio_set_value(mt9v113_reset_gpio, 1);
  mdelay(1);
}

/******************************************************************************

******************************************************************************/
static int mt9v113_read_model_id(const struct msm_camera_sensor_info *data)
{
  int rc = 0;
  uint model_id = 0;
  CDBG_HIGH("%s\n",__func__);
  rc = mt9v113_i2c_read_word(mt9v113_client->addr,0x0000, &model_id);
  if ((rc < 0)||(model_id != 0x2280)){
    CDBG_HIGH("mt9v113_read_model_id model_id(0x%04x) mismatch!\n",model_id);
    return -ENODEV;
  }
  CDBG_HIGH("mt9v113_read_model_id model_id(0x%04x) match success!\n",model_id);
  return rc;
}

/******************************************************************************

******************************************************************************/
static int mt9v113_probe_init_gpio(const struct msm_camera_sensor_info *data)
{
  int rc = 0;
  CDBG_HIGH("%s\n",__func__);
  mt9v113_pwdn_gpio = data->sensor_pwd;
  mt9v113_reset_gpio = data->sensor_reset;
  mt9v113_vcm_pwd_gpio = data->vcm_pwd ;
#if 0
  gpio_free(mt9v113_pwdn_gpio);
  gpio_free(mt9v113_reset_gpio);

  if (data->sensor_reset_enable) {
    rc = gpio_request(data->sensor_reset, "mt9v113");
    if (0 == rc) {
      rc = gpio_tlmm_config(GPIO_CFG(data->sensor_reset, 0,GPIO_CFG_OUTPUT,
                            GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
      if (rc < 0) {
        gpio_free(data->sensor_reset);
        CDBG("mt9v113 gpio_tlmm_config(%d) fail",data->sensor_reset);
        return rc;
      }
    }else{
      CDBG("mt9v113 gpio_request(%d) fail",data->sensor_reset);
      return rc;
    }
  }

  rc = gpio_request(data->sensor_pwd, "mt9v113");
  if (0 == rc) {
    rc = gpio_tlmm_config(GPIO_CFG(data->sensor_pwd, 0,GPIO_CFG_OUTPUT,
                          GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
    if (rc < 0) {
      gpio_free(data->sensor_reset);
      gpio_free(data->sensor_pwd);
      CDBG("mt9v113 gpio_tlmm_config(%d) fail",data->sensor_pwd);
      return rc;
    }
  }else{
    gpio_free(data->sensor_reset);
    CDBG("mt9v113 gpio_request(%d) fail",data->sensor_pwd);
    return rc;
  }
  if (data->vcm_enable)
  {
    gpio_direction_output(data->vcm_pwd, 1);
  }
  gpio_direction_output(data->sensor_reset, 1);
  gpio_direction_output(data->sensor_pwd, 1);
#endif
  return rc;
}

/******************************************************************************

******************************************************************************/
static void mt9v113_probe_free_gpio(const struct msm_camera_sensor_info *data)
{
#if 0
  gpio_free(mt9v113_pwdn_gpio);
  gpio_free(mt9v113_reset_gpio);
  if (data->vcm_enable)
  {
    gpio_free(mt9v113_vcm_pwd_gpio);
    mt9v113_vcm_pwd_gpio = 0xFF ;
  }
  mt9v113_pwdn_gpio = 0xFF;
  mt9v113_reset_gpio = 0xFF;
#endif
}
/******************************************************************************

******************************************************************************/
static int mt9v113_sensor_probe(const struct msm_camera_sensor_info *info,
                                struct msm_sensor_ctrl *s)
{
  int rc = -ENOTSUPP;
  CDBG_HIGH("%s\n",__func__);
  rc = i2c_add_driver(&mt9v113_i2c_driver);
  if ((rc < 0 ) || (mt9v113_client == NULL))
  {
    CDBG_HIGH("i2c_add_driver failed\n");
    if(NULL != mt9v113_sensorw)
    {
      kfree(mt9v113_sensorw);
      mt9v113_sensorw = NULL;
    }
    mt9v113_client = NULL;
    return rc;
  }
  lcd_camera_power_onoff(1);
  rc = mt9v113_probe_init_gpio(info);
  if (rc < 0) {
    CDBG_HIGH("%s,mt9v113_probe_init_gpio failed\n",__func__);
    i2c_del_driver(&mt9v113_i2c_driver);
    return rc;
  }
  mt9v113_power_off();
  msm_camio_clk_rate_set(24000000);
  msleep(5);
  mt9v113_power_on();
  mt9v113_power_reset();
  rc = mt9v113_read_model_id(info);
  if (rc < 0)
  {
    CDBG_HIGH("mt9v113_read_model_id failed\n");
    CDBG_HIGH("%s,unregister\n",__func__);
    i2c_del_driver(&mt9v113_i2c_driver);
    mt9v113_power_off();
    lcd_camera_power_onoff(0);
    mt9v113_probe_free_gpio(info);
    return rc;
  }
  s->s_init = mt9v113_sensor_open_init;
  s->s_release = mt9v113_sensor_release;
  s->s_config  = mt9v113_sensor_config;
  s->s_camera_type = FRONT_CAMERA_2D;
  s->s_mount_angle = info->sensor_platform_info->mount_angle;
  mt9v113_power_off();
  lcd_camera_power_onoff(0);
  return rc;
}
/******************************************************************************

******************************************************************************/
static int mt9v113_i2c_probe(struct i2c_client *client,
                             const struct i2c_device_id *id)
{
  CDBG_HIGH("%s\n",__func__);
  if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
  {
    CDBG_HIGH("i2c_check_functionality failed\n");
    return -ENOMEM;
  }

  mt9v113_sensorw = kzalloc(sizeof(struct mt9v113_work), GFP_KERNEL);
  if (!mt9v113_sensorw)
  {
    CDBG_HIGH("kzalloc failed\n");
    return -ENOMEM;
  }
  i2c_set_clientdata(client, mt9v113_sensorw);
  mt9v113_init_client(client);
  mt9v113_client = client;
  mt9v113_client->addr = client->addr;
  CDBG_HIGH("%s:mt9v113_client->addr = 0x%02x\n",__func__,mt9v113_client->addr);
  return 0;
}
/******************************************************************************

******************************************************************************/
static int __mt9v113_probe(struct platform_device *pdev)
{
  return msm_camera_drv_start(pdev, mt9v113_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
  .probe = __mt9v113_probe,
  .driver = {
    .name = "msm_camera_mt9v113",
    .owner = THIS_MODULE,
  },
};

static int __init mt9v113_init(void)
{
  mt9v113_i2c_buf[0]=0x5A;
  return platform_driver_register(&msm_camera_driver);
}

module_init(mt9v113_init);

MODULE_DESCRIPTION("MT9V113 YUV MIPI sensor driver");
MODULE_LICENSE("GPL v2");
