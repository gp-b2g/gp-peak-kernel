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

#include <linux/i2c.h>
#include <linux/i2c/sx150x.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <asm/mach-types.h>
#include <mach/msm_iomap.h>
#include <mach/board.h>
#include <mach/irqs-7xxx.h>
#include "devices-msm7x2xa.h"
#include "board-msm7627a.h"
#include <mach/vreg.h>

#define FLASH_NOW 13
#define FLASH_EN 32
/*zhuangxiaojian, C8680, enable flash controled by gpio {*/
#if defined(CONFIG_OV8825) || defined(CONFIG_S5K3H2Y) || defined(CONFIG_S5K3H2Y_SUNNY)
//#else
#include <linux/leds-sgm3141.h>
#endif
/*} zhuangxiaojian, C8680, enable flash controled by gpio*/

#ifdef CONFIG_FB_MSM_LCDC_RENESAS_R61581_HVGA	//just use one config parameter for TinBoost, other is for C8680	
	#define GPIO_SKU3_CAM_5MP_SHDN_N   		23         /* PWDN */
	#define GPIO_SKU3_CAM_5MP_CAMIF_RESET   	89    /* (board_is(EVT))?123:121 RESET */
	#define GPIO_SKU1_CAM_VGA_SHDN    		26
	#define GPIO_SKU1_CAM_VGA_RESET_N 		29
#else
	#define GPIO_SKU3_CAM_5MP_SHDN_N   		5         /* PWDN */
	#define GPIO_SKU3_CAM_5MP_CAMIF_RESET   	6    /* (board_is(EVT))?123:121 RESET */
	#define GPIO_SKU1_CAM_VGA_SHDN    		18

#ifdef CONFIG_CELLON_PRJ_C8681
	#define GPIO_SKU1_CAM_VGA_RESET_N 		109
#else	
	#define GPIO_SKU1_CAM_VGA_RESET_N 		29
	#define GPIO_CAMERA_ID						116
#if defined(CONFIG_S5K3H2Y) || defined(CONFIG_S5K3H2Y_SUNNY)
	#define GPIO_S5K3H2Y_AF_ENABLE	7
#endif
#endif	
#endif

#define GPIO_NOT_CONFIGURED -1

unsigned camera_id ;

#ifdef CONFIG_MSM_CAMERA_V4L2

#if defined(CONFIG_OV8825) || defined(CONFIG_OV2675) || defined(CONFIG_MT9E013) || defined(CONFIG_MT9V113) || defined(CONFIG_S5K3H2Y) || defined(CONFIG_S5K3H2Y_SUNNY) || defined(CONFIG_HI542)
static uint32_t camera_off_gpio_table[] = {
	GPIO_CFG(15, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
};

static uint32_t camera_on_gpio_table[] = {
	GPIO_CFG(15, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
};
#endif


#ifdef CONFIG_WEBCAM_OV7692_QRD
static struct msm_camera_gpio_conf gpio_conf_ov7692 = {
	.camera_off_table = camera_off_gpio_table,
	.camera_on_table = camera_on_gpio_table,
	.gpio_no_mux = 1,
};
#endif

#ifdef CONFIG_OV2675
static struct msm_camera_gpio_conf gpio_conf_ov2675= {
	.camera_off_table = camera_off_gpio_table,
	.camera_on_table = camera_on_gpio_table,
	.gpio_no_mux = 1,
};
#endif


#ifdef CONFIG_MT9V113
static struct msm_camera_gpio_conf gpio_conf_mt9v113= {
	.camera_off_table = camera_off_gpio_table,
	.camera_on_table = camera_on_gpio_table,
	.gpio_no_mux = 1,
};
#endif




#if defined(CONFIG_OV8825) || defined(CONFIG_OV2675) || defined(CONFIG_MT9E013) || defined(CONFIG_MT9V113) || defined(CONFIG_S5K3H2Y) || defined(CONFIG_S5K3H2Y_SUNNY) || defined(CONFIG_HI542)
static struct camera_vreg_t msm_cam_vreg[] = {
	{"msme1", REG_LDO, 1800000, 1800000, 0},
	{"gp2", REG_LDO, 2850000, 2850000, 0},
	{"usb2", REG_LDO, 1800000, 1800000, 0},
};
#endif

//static struct msm_camera_sensor_info msm_camera_sensor_s5k4e1_data;

struct msm_camera_device_platform_data msm_camera_device_data_csi1 = {
	.csid_core = 1,
	.is_csic = 1,
};

struct msm_camera_device_platform_data msm_camera_device_data_csi0 = {
	.csid_core = 0,
	.is_csic = 1,
};



#ifdef CONFIG_WEBCAM_OV7692_QRD
static struct msm_camera_sensor_platform_info sensor_board_info_ov7692 = {
	.mount_angle = 90,
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
	.gpio_conf = &gpio_conf_ov7692,
};

static struct msm_camera_sensor_flash_data flash_ov7692 = {
	.flash_type     = MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_info msm_camera_sensor_ov7692_data = {
	.sensor_name	    = "ov7692",
	.sensor_reset_enable    = 0,
	.pmic_gpio_enable  = 1,
	.sensor_reset	   = GPIO_NOT_CONFIGURED,
	.sensor_pwd	     = GPIO_NOT_CONFIGURED,
	.pdata			= &msm_camera_device_data_csi0,
	.flash_data	     = &flash_ov7692,
	.sensor_platform_info   = &sensor_board_info_ov7692,
	.csi_if		 = 1,
	.camera_type = FRONT_CAMERA_2D,
	.sensor_type = YUV_SENSOR,
};
#endif

// add by lzj 
#ifdef CONFIG_S5K3H2Y

static struct i2c_board_info msm_act_main_cam_i2c_info = {
        I2C_BOARD_INFO("msm_actuator", 0x11),
};

static struct msm_actuator_info msm_act_main_cam_6_info = {
        .board_info     = &msm_act_main_cam_i2c_info,
        .cam_name   = MSM_ACTUATOR_MAIN_CAM_6,
        .bus_id         = MSM_GSBI0_QUP_I2C_BUS_ID,
        .vcm_pwd        = GPIO_NOT_CONFIGURED,
        .vcm_enable     = 0,
};

static struct msm_camera_gpio_conf gpio_conf_s5k3h2y = {
	.camera_off_table = camera_off_gpio_table,
	.camera_on_table = camera_on_gpio_table,
	.gpio_no_mux = 1,
};

static struct msm_camera_sensor_platform_info sensor_board_info_s5k3h2y = {
	.mount_angle	= 90,
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
	.gpio_conf = &gpio_conf_s5k3h2y,
};

#ifdef ORIGINAL_VERSION
static struct msm_camera_sensor_flash_data flash_s5k3h2y = {
	.flash_type     = MSM_CAMERA_FLASH_NONE,
};

#else
static struct msm_camera_sensor_flash_src msm_flash_src_s5k3h2y = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_LED1,
#if 0
	._fsrc.gpio_src.off = SGM3141_FLASH_OFF,
	._fsrc.gpio_src.low = SGM3141_FLASH_LOW,
	._fsrc.gpio_src.high = SGM3141_FLASH_HIGH,
	._fsrc.gpio_src.set_flash_type = &sgm3141_set_flash_type,
#else
	._fsrc.ext_driver_src.led_en = FLASH_NOW,
	._fsrc.ext_driver_src.led_flash_en = FLASH_EN,
#endif
};

static struct msm_camera_sensor_flash_data flash_s5k3h2y = {
	.flash_type     = MSM_CAMERA_FLASH_LED,
	.flash_src	= &msm_flash_src_s5k3h2y,
};

#endif

static struct msm_camera_sensor_info msm_camera_sensor_s5k3h2y_data = {
	.sensor_name    = "s5k3h2y",
	.sensor_reset_enable    = 1,
	.pmic_gpio_enable = 1,
	.sensor_reset           = GPIO_SKU3_CAM_5MP_CAMIF_RESET,
	.sensor_pwd     = GPIO_SKU3_CAM_5MP_SHDN_N,
	.pdata  = &msm_camera_device_data_csi1,
	.flash_data     = &flash_s5k3h2y,
	.sensor_platform_info = &sensor_board_info_s5k3h2y,
	.csi_if = 1,
	.camera_type = BACK_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
	.actuator_info = &msm_act_main_cam_6_info,
};

#endif
// add by lzj 






#ifdef CONFIG_S5K3H2Y_SUNNY

static struct i2c_board_info msm_act_s5k3h2y_sunny_i2c_info = {
        I2C_BOARD_INFO("msm_actuator", 0x11),
};

 struct msm_actuator_info msm_act_s5k3h2y_sunny_6_info = {
        .board_info     = &msm_act_s5k3h2y_sunny_i2c_info,
        .cam_name   = MSM_ACTUATOR_MAIN_CAM_6,
        .bus_id         = MSM_GSBI0_QUP_I2C_BUS_ID,
        .vcm_pwd        = GPIO_NOT_CONFIGURED,
        .vcm_enable     = 0,
};

static struct msm_camera_gpio_conf gpio_conf_s5k3h2y_sunny = {
	.camera_off_table = camera_off_gpio_table,
	.camera_on_table = camera_on_gpio_table,
	.gpio_no_mux = 1,
};

static struct msm_camera_sensor_platform_info sensor_board_info_s5k3h2y_sunny = {
	.mount_angle	= 90,
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
	.gpio_conf = &gpio_conf_s5k3h2y_sunny,
};

#ifdef ORIGINAL_VERSION
static struct msm_camera_sensor_flash_data flash_s5k3h2y_sunny = {
	.flash_type     = MSM_CAMERA_FLASH_NONE,
};

#else
static struct msm_camera_sensor_flash_src msm_flash_src_s5k3h2y_sunny = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_GPIO,
	._fsrc.gpio_src.off = SGM3141_FLASH_OFF,
	._fsrc.gpio_src.low = SGM3141_FLASH_LOW,
	._fsrc.gpio_src.high = SGM3141_FLASH_HIGH,
	._fsrc.gpio_src.set_flash_type = &sgm3141_set_flash_type,
};

static struct msm_camera_sensor_flash_data flash_s5k3h2y_sunny = {
	.flash_type     = MSM_CAMERA_FLASH_LED,
	.flash_src	= &msm_flash_src_s5k3h2y_sunny,
};

#endif

static struct msm_camera_sensor_info msm_camera_sensor_s5k3h2y_sunny_data = {
	.sensor_name    = "s5k3h2y_sunny",
	.sensor_reset_enable    = 1,
	.pmic_gpio_enable = 1,
	.sensor_reset           = GPIO_SKU3_CAM_5MP_CAMIF_RESET,
	.sensor_pwd     = GPIO_SKU3_CAM_5MP_SHDN_N,
	.pdata  = &msm_camera_device_data_csi1,
	.flash_data     = &flash_s5k3h2y_sunny,
	.sensor_platform_info = &sensor_board_info_s5k3h2y_sunny,
	.csi_if = 1,
	.camera_type = BACK_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
	.actuator_info = &msm_act_s5k3h2y_sunny_6_info,
};

#endif
// add by lzj 






#ifdef CONFIG_OV8825
static struct msm_camera_gpio_conf gpio_conf_ov8825 = {
	.camera_off_table = camera_off_gpio_table,
	.camera_on_table = camera_on_gpio_table,
	.gpio_no_mux = 1,
};

/*zhuangxiaojian, C8680, enable flash controlled by gpio {*/
#ifdef ORIGINAL_VERSION
static struct msm_camera_sensor_flash_data flash_ov8825 = {
	.flash_type     = MSM_CAMERA_FLASH_NONE,
};
#else
static struct msm_camera_sensor_flash_src msm_flash_src_ov8825 = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_GPIO,
	._fsrc.gpio_src.off = SGM3141_FLASH_OFF,
	._fsrc.gpio_src.low = SGM3141_FLASH_LOW,
	._fsrc.gpio_src.high = SGM3141_FLASH_HIGH,
	._fsrc.gpio_src.set_flash_type = &sgm3141_set_flash_type,
};

static struct msm_camera_sensor_flash_data flash_ov8825 = {
	.flash_type     = MSM_CAMERA_FLASH_LED,
	.flash_src	= &msm_flash_src_ov8825,
};
#endif
/*} zhuangxiaojian, C8680, enable flash controlled by gpio*/

static struct msm_camera_sensor_platform_info sensor_board_info_ov8825 = {
	.mount_angle  = 90,
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
	.gpio_conf = &gpio_conf_ov8825,
};

/*zhuangxiaojian, C8680, 2012-06-28, enable AF for camera ov8825 {*/
static struct i2c_board_info ov8825_vcm_i2c_info = {
	I2C_BOARD_INFO("ov8825_vcm", 0x36),
};

struct msm_actuator_info ov8825_af_info = {
	.board_info = &ov8825_vcm_i2c_info,
	.vcm_enable = 1,
	.cam_name = MSM_ACTUATOR_MAIN_CAM_3,
};
/*} zhuangxiaojian, C8680, 2012-06-28, enable AF for camera ov8825*/

static struct msm_camera_sensor_info msm_camera_sensor_ov8825_data = {
	.sensor_name    = "ov8825",
	.sensor_reset_enable    = 1,
	.pmic_gpio_enable = 1,
	.sensor_reset           = GPIO_SKU3_CAM_5MP_CAMIF_RESET,
	.sensor_pwd     = GPIO_SKU3_CAM_5MP_SHDN_N,
	.pdata  = &msm_camera_device_data_csi1,
	.flash_data     = &flash_ov8825,
	.sensor_platform_info = &sensor_board_info_ov8825,
	.csi_if = 1,
	.camera_type = BACK_CAMERA_2D,
	/*zhuangxiaojian, C8680, 2012-06-28, enable AF for camera ov8825 {*/
	.sensor_type = BAYER_SENSOR,
	.actuator_info = &ov8825_af_info,
	/*} zhuangxiaojian, C8680, 2012-06-28, enable AF for camera ov8825*/
};

#endif

/*zhuangxiaojian, C8681, 2012-07-27, add mt9e013 sensor {*/
#ifdef CONFIG_MT9E013
static struct msm_camera_gpio_conf gpio_conf_mt9e013 = {
	.camera_off_table = camera_off_gpio_table,
	.camera_on_table = camera_on_gpio_table,
	.gpio_no_mux = 1,
};

#if 1
static struct msm_camera_sensor_flash_src msm_flash_src_mt9e013 = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_EXT,	
};

static struct msm_camera_sensor_flash_data flash_mt9e013 = {
	.flash_type             = MSM_CAMERA_FLASH_LED,
	.flash_src              = &msm_flash_src_mt9e013,
};
#else
static struct msm_camera_sensor_flash_data flash_mt9e013 = {
	.flash_type     = MSM_CAMERA_FLASH_NONE,
};
#endif
static struct msm_camera_sensor_platform_info sensor_board_info_mt9e013 = {
	.mount_angle	= 270,
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
	.gpio_conf = &gpio_conf_mt9e013,
};

//static struct i2c_board_info mt9e013_vcm_i2c_info = {
//	I2C_BOARD_INFO("mt9e013_vcm", 0x36),
//};

//struct msm_actuator_info mt9e013_af_info = {
//	.board_info = &mt9e013_vcm_i2c_info,
//	.vcm_enable = 1,
//	.cam_name = MSM_ACTUATOR_MAIN_CAM_5,
//};
//struct msm_camera_device_platform_data mt9e013_platform_data = {
//	.ioclk.vfe_clk_rate = 192000000,
//};

static struct msm_camera_sensor_info msm_camera_sensor_mt9e013_data = {
	.sensor_name    = "mt9e013",
	.sensor_reset_enable = 1,
	.pmic_gpio_enable    = 1,
	.sensor_reset           = GPIO_SKU3_CAM_5MP_CAMIF_RESET,
	.sensor_pwd     = GPIO_SKU3_CAM_5MP_SHDN_N,
	.pdata                  = &msm_camera_device_data_csi1,
	.flash_data             = &flash_mt9e013,
	.sensor_platform_info   = &sensor_board_info_mt9e013,
	.csi_if                 = 1,
	.camera_type = BACK_CAMERA_2D,
//	.pdata = &mt9e013_platform_data,
//	.sensor_type = BAYER_SENSOR,
//	.actuator_info = &mt9e013_af_info,
};
#endif
/*} zhuangxiaojian, C8681, 2012-07-27, add mt9e013 sensor*/

#ifdef CONFIG_HI542
static struct msm_camera_gpio_conf gpio_conf_hi542 = {
	.camera_off_table = camera_off_gpio_table,
	.camera_on_table = camera_on_gpio_table,
	.gpio_no_mux = 1,
};

#if 1
static struct msm_camera_sensor_flash_src msm_flash_src_hi542 = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_EXT,	
};

static struct msm_camera_sensor_flash_data flash_hi542 = {
	.flash_type             = MSM_CAMERA_FLASH_LED,
	.flash_src              = &msm_flash_src_hi542,
};
#else
static struct msm_camera_sensor_flash_data flash_hi542 = {
	.flash_type     = MSM_CAMERA_FLASH_NONE,
};
#endif
static struct msm_camera_sensor_platform_info sensor_board_info_hi542 = {
	.mount_angle	= 90,
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
	.gpio_conf = &gpio_conf_hi542,
};

static struct msm_camera_sensor_info msm_camera_sensor_hi542_data = {
	.sensor_name    = "hi542",
	.sensor_reset_enable = 1,
	.pmic_gpio_enable    = 1,
	.sensor_reset           = GPIO_SKU3_CAM_5MP_CAMIF_RESET,
	.sensor_pwd     = GPIO_SKU3_CAM_5MP_SHDN_N,
	.pdata                  = &msm_camera_device_data_csi1,
	.flash_data             = &flash_hi542,
	.sensor_platform_info   = &sensor_board_info_hi542,
	.csi_if                 = 1,
	.camera_type = BACK_CAMERA_2D,
};
#endif

#ifdef CONFIG_OV2675
static struct msm_camera_sensor_platform_info sensor_board_info_ov2675 = {
	.mount_angle = 270,
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
	.gpio_conf = &gpio_conf_ov2675,
};

static struct msm_camera_sensor_flash_data flash_ov2675 = {
	.flash_type     = MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_info msm_camera_sensor_ov2675_data = {
	.sensor_name	    = "ov2675",
	.sensor_reset_enable    = 1,
	.pmic_gpio_enable  = 0,
	.sensor_reset	   = GPIO_SKU1_CAM_VGA_RESET_N,
	.sensor_pwd	     = GPIO_SKU1_CAM_VGA_SHDN,
	.pdata			= &msm_camera_device_data_csi0,
	.flash_data	     = &flash_ov2675,
	.sensor_platform_info   = &sensor_board_info_ov2675,
	.csi_if		 = 1,
	.camera_type = FRONT_CAMERA_2D,
	.sensor_type = YUV_SENSOR,
};
#endif


#ifdef CONFIG_MT9V113
static struct msm_camera_sensor_platform_info sensor_board_info_mt9v113 = {
	.mount_angle = 90,
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
	.gpio_conf = &gpio_conf_mt9v113,
};

static struct msm_camera_sensor_flash_data flash_mt9v113 = {
	.flash_type     = MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9v113_data = {
	.sensor_name	    = "mt9v113",
	.sensor_reset_enable    = 1,
	.pmic_gpio_enable  = 0,
	.sensor_reset	   = GPIO_SKU1_CAM_VGA_RESET_N,
	.sensor_pwd	     = GPIO_SKU1_CAM_VGA_SHDN,
	.pdata			= &msm_camera_device_data_csi0,
	.flash_data	     = &flash_mt9v113,
	.sensor_platform_info   = &sensor_board_info_mt9v113,
	.csi_if		 = 1,
	.camera_type = FRONT_CAMERA_2D,
	.sensor_type = YUV_SENSOR,
};
#endif

static struct platform_device msm_camera_server = {
	.name = "msm_cam_server",
	.id = 0,
};

static void __init msm7x27a_init_cam(void)
{

#ifdef CONFIG_OV8825
	sensor_board_info_ov8825.cam_vreg = NULL;
	sensor_board_info_ov8825.num_vreg = 0;
#endif
#ifdef CONFIG_MT9E013
	sensor_board_info_mt9e013.cam_vreg = NULL;
	sensor_board_info_mt9e013.num_vreg = 0;
#endif

#ifdef CONFIG_HI542
	sensor_board_info_hi542.cam_vreg = NULL;
	sensor_board_info_hi542.num_vreg = 0;
#endif

#ifdef CONFIG_OV2675
	sensor_board_info_ov2675.cam_vreg = NULL;
	sensor_board_info_ov2675.num_vreg = 0;
#endif	

#ifdef CONFIG_MT9V113
	sensor_board_info_mt9v113.cam_vreg = NULL;
	sensor_board_info_mt9v113.num_vreg = 0;
#endif	

#ifdef CONFIG_S5K3H2Y
	sensor_board_info_s5k3h2y.cam_vreg = NULL;
	sensor_board_info_s5k3h2y.num_vreg = 0;
#endif	

#ifdef CONFIG_S5K3H2Y_SUNNY
	sensor_board_info_s5k3h2y_sunny.cam_vreg = NULL;
	sensor_board_info_s5k3h2y_sunny.num_vreg = 0;
#endif	
	platform_device_register(&msm_camera_server);
	
	if (1) {
		platform_device_register(&msm8625_device_csic0);
		platform_device_register(&msm8625_device_csic1);
	} else {
		platform_device_register(&msm7x27a_device_csic0);
		platform_device_register(&msm7x27a_device_csic1);
	}
	if (1)
		*(int *) msm7x27a_device_clkctl.dev.platform_data = 1;
		
	platform_device_register(&msm7x27a_device_clkctl);
	platform_device_register(&msm7x27a_device_vfe);
}

static struct i2c_board_info i2c_camera_devices[] = {

#ifdef CONFIG_WEBCAM_OV7692_QRD
	{
		I2C_BOARD_INFO("ov7692", 0x78),
		.platform_data = &msm_camera_sensor_ov7692_data,
	},
#endif

#ifdef CONFIG_OV8825
	{
		I2C_BOARD_INFO("ov8825", 0x6C >> 3),
		.platform_data = &msm_camera_sensor_ov8825_data,

	},
#endif

#ifdef CONFIG_MT9E013
	{
		I2C_BOARD_INFO("mt9e013", 0x6C >> 2),
		.platform_data = &msm_camera_sensor_mt9e013_data,
	},
#endif

#ifdef CONFIG_HI542
	{
		I2C_BOARD_INFO("hi542", 0x40),
		.platform_data = &msm_camera_sensor_hi542_data,
	},
#endif

#ifdef CONFIG_OV2675
	{
		I2C_BOARD_INFO("ov2675", 0x60),
		.platform_data = &msm_camera_sensor_ov2675_data,

	},
#endif

#ifdef CONFIG_MT9V113
	{
		I2C_BOARD_INFO("mt9v113", 0x7a),
		.platform_data = &msm_camera_sensor_mt9v113_data,

	},
#endif

#ifdef CONFIG_S5K3H2Y
	{
		I2C_BOARD_INFO("s5k3h2y", 0x6C),
		.platform_data = &msm_camera_sensor_s5k3h2y_data,

	},
#endif

#ifdef CONFIG_S5K3H2Y_SUNNY
	{
		I2C_BOARD_INFO("s5k3h2y_sunny", 0x20),
		.platform_data = &msm_camera_sensor_s5k3h2y_sunny_data,

	},
#endif
};



#else //none-v4l2 below

#endif




static void evb_camera_gpio_cfg(void)
{
	int rc = 0;
#ifdef CONFIG_MT9E013
	printk("##### %s line%d: CONFIG_MT9E013\n",__func__,__LINE__);
	rc = gpio_request(GPIO_SKU3_CAM_5MP_SHDN_N, "mt9e013");
	if (rc < 0)
		pr_err("##### %s: gpio_request GPIO_SKU3_CAM_5MP_SHDN_N failed!",
			 __func__);

	pr_debug("gpio_tlmm_config %d\r\n", GPIO_SKU3_CAM_5MP_SHDN_N);
	rc = gpio_tlmm_config(GPIO_CFG(GPIO_SKU3_CAM_5MP_SHDN_N, 0,
		GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
		GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("##### %s:unable to enable Powr Dwn gpio for main camera!\n",
			 __func__);
		gpio_free(GPIO_SKU3_CAM_5MP_SHDN_N);
	}

	gpio_direction_output(GPIO_SKU3_CAM_5MP_SHDN_N, 1);

	rc = gpio_request(GPIO_SKU3_CAM_5MP_CAMIF_RESET, "mt9e013");
	if (rc < 0)
		pr_err("##### %s: gpio_request GPIO_SKU3_CAM_5MP_CAMIF_RESET failed!",
			 __func__);

	pr_debug("##### gpio_tlmm_config %d\r\n", GPIO_SKU3_CAM_5MP_CAMIF_RESET);
	rc = gpio_tlmm_config(GPIO_CFG(GPIO_SKU3_CAM_5MP_CAMIF_RESET, 0,
		GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
		GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("##### %s: unable to enable reset gpio for main camera!\n",
			 __func__);
		gpio_free(GPIO_SKU3_CAM_5MP_CAMIF_RESET);
	}

	gpio_direction_output(GPIO_SKU3_CAM_5MP_CAMIF_RESET, 1);
#endif

#ifdef CONFIG_HI542
	printk("##### %s line%d: CONFIG_HI542\n",__func__,__LINE__);
	rc = gpio_request(GPIO_SKU3_CAM_5MP_SHDN_N, "hi542");
	if (rc < 0)
		pr_err("##### %s: gpio_request GPIO_SKU3_CAM_5MP_SHDN_N failed!",
			 __func__);

	pr_debug("gpio_tlmm_config %d\r\n", GPIO_SKU3_CAM_5MP_SHDN_N);
	rc = gpio_tlmm_config(GPIO_CFG(GPIO_SKU3_CAM_5MP_SHDN_N, 0,
		GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
		GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("##### %s:unable to enable Powr Dwn gpio for main camera!\n",
			 __func__);
		gpio_free(GPIO_SKU3_CAM_5MP_SHDN_N);
	}

	gpio_direction_output(GPIO_SKU3_CAM_5MP_SHDN_N, 1);

	rc = gpio_request(GPIO_SKU3_CAM_5MP_CAMIF_RESET, "hi542");
	if (rc < 0)
		pr_err("##### %s: gpio_request GPIO_SKU3_CAM_5MP_CAMIF_RESET failed!",
			 __func__);

	pr_debug("##### gpio_tlmm_config %d\r\n", GPIO_SKU3_CAM_5MP_CAMIF_RESET);
	rc = gpio_tlmm_config(GPIO_CFG(GPIO_SKU3_CAM_5MP_CAMIF_RESET, 0,
		GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
		GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("##### %s: unable to enable reset gpio for main camera!\n",
			 __func__);
		gpio_free(GPIO_SKU3_CAM_5MP_CAMIF_RESET);
	}

	gpio_direction_output(GPIO_SKU3_CAM_5MP_CAMIF_RESET, 1);
#endif

	if (camera_id==1){
		
#ifdef CONFIG_OV8825
	rc = gpio_request(GPIO_SKU3_CAM_5MP_SHDN_N, "ov8825");
	if (rc < 0)
		pr_err("%s: gpio_request GPIO_SKU3_CAM_5MP_SHDN_N failed!",
			 __func__);

	pr_debug("gpio_tlmm_config %d\r\n", GPIO_SKU3_CAM_5MP_SHDN_N);
	rc = gpio_tlmm_config(GPIO_CFG(GPIO_SKU3_CAM_5MP_SHDN_N, 0,
		GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
		GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s:unable to enable Powr Dwn gpio for main camera!\n",
			 __func__);
		gpio_free(GPIO_SKU3_CAM_5MP_SHDN_N);
	}

	gpio_direction_output(GPIO_SKU3_CAM_5MP_SHDN_N, 1);

	rc = gpio_request(GPIO_SKU3_CAM_5MP_CAMIF_RESET, "ov8825");
	if (rc < 0)
		pr_err("%s: gpio_request GPIO_SKU3_CAM_5MP_CAMIF_RESET failed!",
			 __func__);

	pr_debug("gpio_tlmm_config %d\r\n", GPIO_SKU3_CAM_5MP_CAMIF_RESET);
	rc = gpio_tlmm_config(GPIO_CFG(GPIO_SKU3_CAM_5MP_CAMIF_RESET, 0,
		GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
		GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s: unable to enable reset gpio for main camera!\n",
			 __func__);
		gpio_free(GPIO_SKU3_CAM_5MP_CAMIF_RESET);
	}

	gpio_direction_output(GPIO_SKU3_CAM_5MP_CAMIF_RESET, 1);
#endif

	}else if (camera_id == 0){ 
	
	
//add by lzj
#ifdef CONFIG_S5K3H2Y
	rc = gpio_request(GPIO_SKU3_CAM_5MP_SHDN_N, "s5k3h2y");
	if (rc < 0)
		pr_err("%s: gpio_request GPIO_SKU3_CAM_5MP_SHDN_N failed!",
			 __func__);

	pr_debug("gpio_tlmm_config %d\r\n", GPIO_SKU3_CAM_5MP_SHDN_N);
	rc = gpio_tlmm_config(GPIO_CFG(GPIO_SKU3_CAM_5MP_SHDN_N, 0,
		GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
		GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s:unable to enable Powr Dwn gpio for main camera!\n",
			 __func__);
		gpio_free(GPIO_SKU3_CAM_5MP_SHDN_N);
	}

	gpio_direction_output(GPIO_SKU3_CAM_5MP_SHDN_N, 1);

	rc = gpio_request(GPIO_SKU3_CAM_5MP_CAMIF_RESET, "s5k3h2y");
	if (rc < 0)
		pr_err("%s: gpio_request GPIO_SKU3_CAM_5MP_CAMIF_RESET failed!",
			 __func__);

	pr_debug("gpio_tlmm_config %d\r\n", GPIO_SKU3_CAM_5MP_CAMIF_RESET);
	rc = gpio_tlmm_config(GPIO_CFG(GPIO_SKU3_CAM_5MP_CAMIF_RESET, 0,
		GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
		GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s: unable to enable reset gpio for main camera!\n",
			 __func__);
		gpio_free(GPIO_SKU3_CAM_5MP_CAMIF_RESET);
	}

	gpio_direction_output(GPIO_SKU3_CAM_5MP_CAMIF_RESET, 1);


	rc = gpio_request(GPIO_S5K3H2Y_AF_ENABLE, "s5k3h2y");
	if (rc < 0)
		pr_err("%s: gpio_request GPIO_SKU3_CAM_5MP_SHDN_N failed!",
			 __func__);

	pr_debug("gpio_tlmm_config %d\r\n", GPIO_S5K3H2Y_AF_ENABLE);
	rc = gpio_tlmm_config(GPIO_CFG(GPIO_S5K3H2Y_AF_ENABLE, 0,
		GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
		GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s:unable to enable Powr Dwn gpio for main camera!\n",
			 __func__);
		gpio_free(GPIO_S5K3H2Y_AF_ENABLE);
	}

	gpio_direction_output(GPIO_S5K3H2Y_AF_ENABLE, 1);

	
#endif
	}else if (camera_id == 3){ 
	
	
//add by lzj
#ifdef CONFIG_S5K3H2Y
	rc = gpio_request(GPIO_SKU3_CAM_5MP_SHDN_N, "s5k3h2y_sunny");
	if (rc < 0)
		pr_err("%s: gpio_request GPIO_SKU3_CAM_5MP_SHDN_N failed!",
			 __func__);

	pr_debug("gpio_tlmm_config %d\r\n", GPIO_SKU3_CAM_5MP_SHDN_N);
	rc = gpio_tlmm_config(GPIO_CFG(GPIO_SKU3_CAM_5MP_SHDN_N, 0,
		GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
		GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s:unable to enable Powr Dwn gpio for main camera!\n",
			 __func__);
		gpio_free(GPIO_SKU3_CAM_5MP_SHDN_N);
	}

	gpio_direction_output(GPIO_SKU3_CAM_5MP_SHDN_N, 1);

	rc = gpio_request(GPIO_SKU3_CAM_5MP_CAMIF_RESET, "s5k3h2y_sunny");
	if (rc < 0)
		pr_err("%s: gpio_request GPIO_SKU3_CAM_5MP_CAMIF_RESET failed!",
			 __func__);

	pr_debug("gpio_tlmm_config %d\r\n", GPIO_SKU3_CAM_5MP_CAMIF_RESET);
	rc = gpio_tlmm_config(GPIO_CFG(GPIO_SKU3_CAM_5MP_CAMIF_RESET, 0,
		GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
		GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s: unable to enable reset gpio for main camera!\n",
			 __func__);
		gpio_free(GPIO_SKU3_CAM_5MP_CAMIF_RESET);
	}

	gpio_direction_output(GPIO_SKU3_CAM_5MP_CAMIF_RESET, 1);


	rc = gpio_request(GPIO_S5K3H2Y_AF_ENABLE, "s5k3h2y_sunny");
	if (rc < 0)
		pr_err("%s: gpio_request GPIO_SKU3_CAM_5MP_SHDN_N failed!",
			 __func__);

	pr_debug("gpio_tlmm_config %d\r\n", GPIO_S5K3H2Y_AF_ENABLE);
	rc = gpio_tlmm_config(GPIO_CFG(GPIO_S5K3H2Y_AF_ENABLE, 0,
		GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
		GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s:unable to enable Powr Dwn gpio for main camera!\n",
			 __func__);
		gpio_free(GPIO_S5K3H2Y_AF_ENABLE);
	}

	gpio_direction_output(GPIO_S5K3H2Y_AF_ENABLE, 1);

	
#endif
	}
//add end


#ifdef CONFIG_OV2675
	rc = gpio_request(GPIO_SKU1_CAM_VGA_SHDN, "ov2675");
	if (rc < 0)
		pr_err("%s: gpio_request---GPIO_SKU1_CAM_VGA_SHDN failed!",
			 __func__);

	rc = gpio_tlmm_config(GPIO_CFG(GPIO_SKU1_CAM_VGA_SHDN, 0,
		GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP,
		GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s:unable to enable Powr Dwn gpio for frnt camera!\n",
			 __func__);
		gpio_free(GPIO_SKU1_CAM_VGA_SHDN);
	}

	gpio_direction_output(GPIO_SKU1_CAM_VGA_SHDN, 1);

	rc = gpio_request(GPIO_SKU1_CAM_VGA_RESET_N, "ov2675");
	if (rc < 0)
		pr_err("%s: gpio_request---GPIO_SKU1_CAM_VGA_RESET_N failed!",
			 __func__);

	rc = gpio_tlmm_config(GPIO_CFG(GPIO_SKU1_CAM_VGA_RESET_N, 0,
		GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP,
		GPIO_CFG_2MA), GPIO_CFG_ENABLE);

	if (rc < 0) {
		pr_err("%s: unable to enable reset gpio for front camera!\n",
			 __func__);
		gpio_free(GPIO_SKU1_CAM_VGA_RESET_N);
	}
	gpio_direction_output(GPIO_SKU1_CAM_VGA_RESET_N, 1);
#endif


#ifdef CONFIG_MT9V113
	rc = gpio_request(GPIO_SKU1_CAM_VGA_SHDN, "mt9v113");
	if (rc < 0)
		pr_err("%s: gpio_request---GPIO_SKU1_CAM_VGA_SHDN failed!",
			 __func__);

	rc = gpio_tlmm_config(GPIO_CFG(GPIO_SKU1_CAM_VGA_SHDN, 0,
		GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP,
		GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s:unable to enable Powr Dwn gpio for frnt camera!\n",
			 __func__);
		gpio_free(GPIO_SKU1_CAM_VGA_SHDN);
	}

	gpio_direction_output(GPIO_SKU1_CAM_VGA_SHDN, 1);

	rc = gpio_request(GPIO_SKU1_CAM_VGA_RESET_N, "mt9v113");
	if (rc < 0)
		pr_err("%s: gpio_request---GPIO_SKU1_CAM_VGA_RESET_N failed!",
			 __func__);

	rc = gpio_tlmm_config(GPIO_CFG(GPIO_SKU1_CAM_VGA_RESET_N, 0,
		GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP,
		GPIO_CFG_2MA), GPIO_CFG_ENABLE);

	if (rc < 0) {
		pr_err("%s: unable to enable reset gpio for front camera!\n",
			 __func__);
		gpio_free(GPIO_SKU1_CAM_VGA_RESET_N);
	}
	gpio_direction_output(GPIO_SKU1_CAM_VGA_RESET_N, 1);
#endif
}



/* TODO: sensor specific? */
static struct regulator *reg_ext_2v8;
static struct regulator *reg_ext_1v8;
static struct regulator *reg_2v8;

int lcd_camera_power_onoff(int on)
{
	int rc = 0;

	printk("%s: on = %d\n", __func__, on);

	/* ext_2v8 and ext_1v8 control */
	if (!reg_ext_2v8) {
		reg_ext_2v8 = regulator_get(NULL, "ext_2v8");
		if (IS_ERR(reg_ext_2v8)) {
			pr_err("'%s' regulator not found, rc=%ld\n",
				"ext_2v8", IS_ERR(reg_ext_2v8));
			reg_ext_2v8 = NULL;
			return -ENODEV;
		}
	}

	if (!reg_ext_1v8) {
		reg_ext_1v8 = regulator_get(NULL, "ext_1v8");
		if (IS_ERR(reg_ext_1v8)) {
			pr_err("'%s' regulator not found, rc=%ld\n",
				"ext_1v8", IS_ERR(reg_ext_1v8));
			reg_ext_1v8 = NULL;
			return -ENODEV;
		}
	}

	if (on) {
		rc = regulator_enable(reg_ext_2v8);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"reg_ext_2v8", rc);
			return rc;
		}

		rc = regulator_enable(reg_ext_1v8);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"reg_ext_1v8", rc);
			return rc;
		}

		pr_debug("%s(on): success\n", __func__);
	} else {
		rc = regulator_disable(reg_ext_2v8);
		if (rc)
			pr_warning("'%s' regulator disable failed, rc=%d\n",
				"reg_ext_2v8", rc);

		rc = regulator_disable(reg_ext_1v8);
		if (rc)
			pr_warning("'%s' regulator disable failed, rc=%d\n",
				"reg_ext_1v8", rc);

		pr_debug("%s(off): success\n", __func__);
	}

	return rc;
}
EXPORT_SYMBOL(lcd_camera_power_onoff);

/*zhuangxiaojian, 2012-08-01, c8680, add for camera power supply {*/
#ifdef ORIGINAL_VERSION
#else
int camera_power_onoff(int on)
{
	int rc = 0;

	printk("##### %s: on = %d\n", __func__, on);

	if (!reg_2v8) {
		reg_2v8 = regulator_get(NULL, "mmc");
		if (IS_ERR(reg_2v8)) {
			pr_err("##### '%s' regulator not found, rc=%ld\n",
				"ext_2v8", IS_ERR(reg_2v8));
			reg_2v8 = NULL;
			return -ENODEV;
		}
		rc = regulator_set_voltage(reg_2v8, 2850000, 2850000);
		if (rc < 0) {
			printk("######%s line%d: rc == %d",__func__,__LINE__,rc);
			
		}
	}

	if (on) {
		rc = regulator_enable(reg_2v8);
		if (rc) {
			pr_err("##### '%s' regulator enable failed, rc=%d\n",
				"reg_2v8", rc);
			return rc;
		}

	} else {
		rc = regulator_disable(reg_2v8);
		if (rc)
			pr_warning("##### '%s' regulator disable failed, rc=%d\n",
				"reg_2v8", rc);

		pr_debug("%s(off): success\n", __func__);
	}
	printk("##### %s: on = %d end \n", __func__, on);
	return rc;
}

EXPORT_SYMBOL(camera_power_onoff);
#endif
/*} zhuangxiaojian, 2012-08-01, c8680, add for camera power supply*/

void camera_af_software_powerdown(struct i2c_client *client)
{
	int rc = 0;
	unsigned char txdata[] = {0x80, 0x0};
	struct i2c_msg msg[] = {
		{
			.addr = 0x18>>1,
			.flags = 0,
			.len = 2,
			.buf = txdata,
		 },
	};
	printk("camera_af_software_powerdown IN...\n");
	/* send software powerdown cmd to AF motor, avoid current leak */
	if (machine_is_msm8625_skua()){
		printk("SKUA AF Motor software powerdown, write i2c saddr:0x18, waddr:0x80, wdata:0x00\n");
		rc = i2c_transfer(client->adapter, msg, 1);
		if (rc < 0)
			printk("AF software powerdown faild\n");
		return;
	}
}
EXPORT_SYMBOL(camera_af_software_powerdown);


// add by lzj 
// function  :  get camera id to distinguish OV8825 or S5K3H2Y
// return     :  1 (OV8825) 
//			 0 (S5K3H2Y)
int get_camera_id(void)
{
#if defined(CONFIG_OV8825) || defined(CONFIG_S5K3H2Y) || defined(CONFIG_S5K3H2Y_SUNNY) 
	int rc;
	rc = gpio_request(GPIO_CAMERA_ID,"camera_id");
	if (rc < 0)
		pr_err("%s: gpio_request---116 failed!",
			 __func__);

	rc = gpio_tlmm_config(GPIO_CFG(GPIO_CAMERA_ID, 0,
		GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
		GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s:unable to enable Powr Dwn gpio for frnt camera!\n",
			 __func__);
		gpio_free(GPIO_CAMERA_ID);
	}

	camera_id = gpio_get_value(GPIO_CAMERA_ID);

#ifdef CONFIG_S5K3H2Y_SUNNY
	if (camera_id == 1) camera_id = 3;
#endif
	
#else
	camera_id = 2;
#endif
	printk("\n ######### %s : 116 : %d \n",__func__,camera_id);

	return camera_id;

}
// add end

EXPORT_SYMBOL(camera_id);


void __init msm7627a_camera_init(void)
{
#ifndef CONFIG_MSM_CAMERA_V4L2
	int rc;
#endif

	/* LCD and camera power (VREG & LDO) init */
	get_camera_id();
	evb_camera_gpio_cfg();

		

#if defined(CONFIG_MSM_CAMERA_V4L2)
	msm7x27a_init_cam();
#endif


	printk("i2c_register_board_info\n");
	i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
				i2c_camera_devices,
				ARRAY_SIZE(i2c_camera_devices));
				


}
