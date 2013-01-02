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
#include <linux/device.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <asm/mach-types.h>
#include <linux/i2c.h>
#include <devices-msm7x2xa.h>

#ifdef CONFIG_AVAGO_APDS990X
#include <linux/input/apds990x.h>
#endif

#ifdef CONFIG_MPU_SENSORS_MPU3050
#include <linux/mpu.h>
#endif

#ifdef  CONFIG_INPUT_LTR502
#include <linux/input/ltr502.h>
#endif

#ifdef  CONFIG_INPUT_LTR558
#include <linux/input/ltr558.h>
#endif

#ifdef CONFIG_SENSORS_AK8975
#include <linux/akm8975.h>
#endif

#ifdef CONFIG_SENSORS_AK8963
#include <linux/akm8963.h>
#endif
#ifdef CONFIG_GSENSOR_LIS3DH
#include <linux/lis3dh.h>
#endif

#ifdef CONFIG_AVAGO_APDS990X
#ifndef APDS990X_IRQ_GPIO
#define APDS990X_IRQ_GPIO 17
#endif

#ifndef APDS990x_PS_DETECTION_THRESHOLD
#define APDS990x_PS_DETECTION_THRESHOLD         600
#endif

#ifndef APDS990x_PS_HSYTERESIS_THRESHOLD
#define APDS990x_PS_HSYTERESIS_THRESHOLD        500
#endif

#ifndef APDS990x_ALS_THRESHOLD_HSYTERESIS
#define APDS990x_ALS_THRESHOLD_HSYTERESIS       20
#endif

#if defined(CONFIG_INPUT_KXTJ9)
#include <linux/input/kxtj9.h>
#endif

#if defined(CONFIG_I2C) && defined(CONFIG_INPUT_PS31XX)
#include <linux/stk31xx.h>
#endif

static struct msm_gpio apds990x_cfg_data[] = {
        {GPIO_CFG(APDS990X_IRQ_GPIO, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_6MA),"apds990x_irq"},
};

static struct apds990x_platform_data apds990x_platformdata = {
        .irq            = MSM_GPIO_TO_INT(APDS990X_IRQ_GPIO),
        .ps_det_thld    = APDS990x_PS_DETECTION_THRESHOLD,
        .ps_hsyt_thld   = APDS990x_PS_HSYTERESIS_THRESHOLD,
        .als_hsyt_thld  = APDS990x_ALS_THRESHOLD_HSYTERESIS,
};

static struct i2c_board_info i2c_info_apds990x = {
        I2C_BOARD_INFO("apds990x", 0x39),
        .platform_data = &apds990x_platformdata,
};

static int apds990x_setup(void)
{
        int retval = 0;

        retval = msm_gpios_request_enable(apds990x_cfg_data, sizeof(apds990x_cfg_data)/sizeof(struct msm_gpio));
        if(retval) {
                printk(KERN_ERR "%s: Failed to obtain L/P sensor interrupt. Code: %d.", __func__, retval);
        }

        i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
                        &i2c_info_apds990x, 1);

        return retval;
}
#endif

#ifdef CONFIG_MPU_SENSORS_MPU3050

#define GPIO_ACC_INT 28
#define GPIO_GYRO_INT 27

/* gyro x and z axis invert for EVB*/
static struct mpu_platform_data mpu3050_data = {
	.int_config  = 0x10,
	.orientation = { -1, 0, 0,
			0, 1, 0,
			0, 0, -1 },
};

/* accel x and z axis invert for EVB */
static struct ext_slave_platform_data inv_mpu_bma250_data = {
	.bus         = EXT_SLAVE_BUS_SECONDARY,
	.orientation = { -1, 0, 0,
			0, 1, 0,
			0, 0, -1 },
};
/* compass  */
static struct ext_slave_platform_data inv_mpu_mmc328xms_data = {
	.bus         = EXT_SLAVE_BUS_PRIMARY,
	.orientation = { -1, 0, 0,
			0, 1, 0,
			0, 0, 1 },
};

/* gyro x and z axis invert for EVT*/
static struct mpu_platform_data mpu3050_data_qrd5 = {
	.int_config  = 0x10,
	.orientation = { 1, 0, 0,
			0, 1, 0,
			0, 0, 1 },
};

/* accel x and z axis invert for EVT */
static struct ext_slave_platform_data inv_mpu_bma250_data_qrd5 = {
	.bus         = EXT_SLAVE_BUS_SECONDARY,
	.orientation = { 1, 0, 0,
			0, 1, 0,
			0, 0, 1 },
};
/* compass for EVT  */
static struct ext_slave_platform_data inv_mpu_mmc328xms_data_qrd5 = {
	.bus         = EXT_SLAVE_BUS_PRIMARY,
	.orientation = { 1, 0, 0,
			0, 1, 0,
			0, 0, -1 },
};

static struct i2c_board_info __initdata mpu3050_boardinfo[] = {
	{
		I2C_BOARD_INFO("mpu3050", 0x68),
		.irq = MSM_GPIO_TO_INT(GPIO_GYRO_INT),
		.platform_data = &mpu3050_data,
	},
	{
		I2C_BOARD_INFO("bma250", 0x18),
		//.irq = MSM_GPIO_TO_INT(GPIO_ACC_INT),
		.platform_data = &inv_mpu_bma250_data,
	},
	{
		I2C_BOARD_INFO("mmc328xms", 0x30),
		//.irq = (IH_GPIO_BASE + COMPASS_IRQ_GPIO),
		.platform_data = &inv_mpu_mmc328xms_data,
	},
};

static struct i2c_board_info __initdata mpu3050_boardinfo_qrd5[] = {
	{
		I2C_BOARD_INFO("mpu3050", 0x68),
		.irq = MSM_GPIO_TO_INT(GPIO_GYRO_INT),
		.platform_data = &mpu3050_data_qrd5,
	},
	{
		I2C_BOARD_INFO("bma250", 0x18),
		//.irq = MSM_GPIO_TO_INT(GPIO_ACC_INT),
		.platform_data = &inv_mpu_bma250_data_qrd5,
	},
	{
		I2C_BOARD_INFO("mmc328xms", 0x30),
		//.irq = (IH_GPIO_BASE + COMPASS_IRQ_GPIO),
		.platform_data = &inv_mpu_mmc328xms_data_qrd5,
	},
};

static struct msm_gpio mpu3050_gpio_cfg_data[] = {
	{ GPIO_CFG(GPIO_GYRO_INT, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_6MA),
		"mpu3050_gyroint" },
	{ GPIO_CFG(GPIO_ACC_INT, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_6MA),
		"mpu3050_accint" },
};

static int mpu3050_gpio_setup(void) {
	int ret = 0;
	ret = msm_gpios_request_enable(mpu3050_gpio_cfg_data,
				 sizeof(mpu3050_gpio_cfg_data)/sizeof(struct msm_gpio));
	if( ret<0 )
		printk(KERN_ERR "Failed to obtain mpu3050 int GPIO!\n");
	else
		printk("mpu3050 int GPIO request!\n");
	if(machine_is_msm8625_qrd5() || machine_is_msm7x27a_qrd5a() ) {
		if (ARRAY_SIZE(mpu3050_boardinfo_qrd5))
			i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
						mpu3050_boardinfo_qrd5,
						ARRAY_SIZE(mpu3050_boardinfo_qrd5));
	} else {
		if (ARRAY_SIZE(mpu3050_boardinfo))
			i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
						mpu3050_boardinfo,
						ARRAY_SIZE(mpu3050_boardinfo));
	}
	printk("i2c_register_board_info for MPU3050\n");

	return ret;
}
#endif

#ifdef CONFIG_MT9E013
static struct i2c_board_info tps61310_i2c_info[] __initdata = {
	/*zhuangxiaojian, 2012-08-30, C8681, add led flash for camera module {*/
	{
		I2C_BOARD_INFO("tps61310", 0x33),
	},
	/*} zhuangxiaojian, 2012-08-30, C8681, add led flash for camera module*/
};
#endif

#ifdef CONFIG_BOSCH_BMA250
static struct i2c_board_info bma250_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("bma250", 0x18),
	},
};
#endif

#ifdef CONFIG_GSENSOR_BMA2X2
static struct i2c_board_info bma2x2_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("bma2x2", 0x18),
	},
};
#endif
#ifdef CONFIG_GSENSOR_MC32X0
static struct i2c_board_info mc32x0_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("mc32x0", 0x4c),
	},
};
#endif
#ifdef CONFIG_GSENSOR_LIS3DH
static struct lis3dh_acc_platform_data  lis3dh_pdata = {
    .poll_interval = 20,
		.min_interval = 10,
		.g_range = 0x10, // 4G
		.axis_map_x = 0 ,
		.axis_map_y = 1,
		.axis_map_z = 2,
		.negate_x = 0,
		.negate_y = 0,
		.negate_z = 1,
		.gpio_int1 = -1,
		.gpio_int2 = -1,
};
static struct i2c_board_info lis3dh_acc_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("lis3dh_acc", 0x19),
		.platform_data =  &lis3dh_pdata,
	},
};
#endif

#if defined(CONFIG_I2C) && defined(CONFIG_INPUT_LTR502)

static struct ltr502_platform_data ltr502_pdata = {
	.int_gpio = -1,
};

static struct i2c_board_info ltr502_light_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("ltr502", 0x1c),
		.platform_data =  &ltr502_pdata,
	},
};

static struct msm_gpio ltr502_light_gpio_cfg_data[] = {
	{GPIO_CFG(-1, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_6MA), "ltr502_light_int"},
};

static int ltr502_light_gpio_setup(void) {
	int ret = 0;
	ltr502_pdata.int_gpio = 17;
	ltr502_light_gpio_cfg_data[0].gpio_cfg =
                                GPIO_CFG(ltr502_pdata.int_gpio, 0,
                                GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_6MA);
	ret = msm_gpios_request_enable(ltr502_light_gpio_cfg_data, 1);
	if(ret < 0)
		printk(KERN_ERR "%s: Failed to obtain acc int GPIO %d. Code: %d\n",
				__func__, ltr502_pdata.int_gpio, ret);

	return ret;
}
#endif

#if defined(CONFIG_I2C) && defined(CONFIG_INPUT_LTR558)

#define LTR558_INT        17

static struct ltr558_platform_data ltr558_pdata = {
	.intr = LTR558_INT,
};

static struct i2c_board_info ltr558_light_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("ltr558", 0x23),
		.platform_data =  &ltr558_pdata,
		.irq           = MSM_GPIO_TO_INT(LTR558_INT),
	},
};
#endif

#if defined(CONFIG_I2C) && defined(CONFIG_INPUT_PS31XX)
#define PS31XX_INT         17
static struct stk31xx_platform_data stk31xx_data = {
	.als_cmd = 0x4c,
	.ps_cmd =  0x21,
	.ps_gain = 0x0D,
	.transmittance = 800,
	.ps_high_thd   = 120,
	.ps_low_thd    = 100,
	.int_pin       = PS31XX_INT
};

static struct i2c_board_info ps31xx_light_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("stk_ps", 0x48),
		.platform_data = &stk31xx_data,
		.irq           = MSM_GPIO_TO_INT(PS31XX_INT),
	},
};

#endif

#if defined(CONFIG_I2C) && defined(CONFIG_INPUT_TMD27713)
#include <linux/tmd27713.h>
#define TMD27713_IRQ         17

static struct tmd27713_platform_data tmd27713_data = {
	.pdrive = 0x03,
	.ppcount = 0x08,
	.setup_resources = NULL,
	.release_resources = NULL,
};

static struct msm_gpio taos_gpio_int_config_data[] = {
	{ GPIO_CFG(TMD27713_IRQ, 0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "taos_irq" },
};

static void taos_init_irq(void){
	int ret = 0;
	ret = msm_gpios_request_enable(taos_gpio_int_config_data, 1);
	if (ret < 0) {
		pr_err("%s: gpio enable failed: %d\n", __func__, ret);
		return;
	}
	printk("%s\n", __func__);
	return;
}

static struct i2c_board_info tmd27713_gpio_i2c_board_info[] __initdata = {
	{
		I2C_BOARD_INFO("tmd27713", 0x39),
		.irq           = MSM_GPIO_TO_INT(TMD27713_IRQ),
		.platform_data = &tmd27713_data,
	},
};
#endif

#ifdef CONFIG_SENSORS_AK8975

static struct msm_gpio akm_gpio_cfg_data[] = {
	{
		GPIO_CFG(-1, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_6MA),
		"akm_int"
	},
};

static int akm_gpio_setup(void) {
	int ret = 0;
	akm_gpio_cfg_data[0].gpio_cfg =
				GPIO_CFG(18, 0,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_6MA);
	ret = msm_gpios_request_enable(akm_gpio_cfg_data,
				 sizeof(akm_gpio_cfg_data)/sizeof(struct msm_gpio));
	return ret;
}

static struct akm8975_platform_data akm_platform_data_8975 = {
		.gpio_DRDY = -1,
};

static struct i2c_board_info akm8975_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("akm8975", 0x0e),
		.platform_data =  &akm_platform_data_8975,
		.flags = I2C_CLIENT_WAKE,
		.irq = -1,//MSM_GPIO_TO_INT(GPIO_COMPASS_DRDY_INDEX),
	},
};
#endif

#ifdef CONFIG_SENSORS_AK8963
#ifdef CONFIG_CELLON_PRJ_C8681
#define AKM_GPIO_DRDY       26 
#else
#define AKM_GPIO_DRDY       27
#endif
#define AKM_GPIO_RST       0
#define AKM_LAYOUT           3
#define AKM_OUTBIT           1
static struct akm8963_platform_data akm_platform_data_8963 = {
	.gpio_DRDY        = AKM_GPIO_DRDY,
//	.gpio_RST         = AKM_GPIO_RST,        
	.layout           = AKM_LAYOUT,
	.outbit           = AKM_OUTBIT,
};
static int akm_gpio_setup(void) {
	int rc;

	rc = gpio_tlmm_config(GPIO_CFG(AKM_GPIO_DRDY, 0,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_6MA), GPIO_CFG_ENABLE);
	if (rc) {
		pr_err("%s: gpio_tlmm_config for %d failed\n",
			__func__, AKM_GPIO_DRDY);
		return rc;
	}

	rc = gpio_request(AKM_GPIO_DRDY, "akm_init");
	return rc;
}

#ifdef CONFIG_CELLON_PRJ_C8681
static struct i2c_board_info akm8963_i2c_info[] __initdata = {
	{	I2C_BOARD_INFO("akm8963", 0x0E),
		.flags 		= I2C_CLIENT_WAKE,
		.platform_data  = &akm_platform_data_8963,
		.irq  		= MSM_GPIO_TO_INT(AKM_GPIO_DRDY),
	},
};
#else
static struct i2c_board_info akm8963_i2c_info[] __initdata = {
	{	I2C_BOARD_INFO("akm8963", 0x0D),
		.flags 		= I2C_CLIENT_WAKE,
		.platform_data  = &akm_platform_data_8963,
		.irq  		= MSM_GPIO_TO_INT(AKM_GPIO_DRDY),
	},
};
#endif
#endif

#if defined(CONFIG_INPUT_KXTJ9)
#define KXTJ9_DEVICE_MAP	7
#define KXTJ9_MAP_X		((KXTJ9_DEVICE_MAP-1)%2)
#define KXTJ9_MAP_Y		(KXTJ9_DEVICE_MAP%2)
#define KXTJ9_NEG_X		(((KXTJ9_DEVICE_MAP+1)/2)%2)
#define KXTJ9_NEG_Y		(((KXTJ9_DEVICE_MAP+5)/4)%2)
#define KXTJ9_NEG_Z		((KXTJ9_DEVICE_MAP-1)/4)

struct kxtj9_platform_data kxtj9_pdata = {
	.min_interval 	= 5,
	.poll_interval 	= 200,

	.axis_map_x 	= KXTJ9_MAP_X,
	.axis_map_y 	= KXTJ9_MAP_Y,
	.axis_map_z 	= 2,

	.negate_x 		= KXTJ9_NEG_X,
	.negate_y 		= KXTJ9_NEG_Y,
	.negate_z 		= KXTJ9_NEG_Z,

	.res_12bit 		= RES_12BIT,
	.g_range  		= KXTJ9_G_2G,
};

static struct i2c_board_info accel_kxtj9_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("kxtik", 0x0F),
		.platform_data = &kxtj9_pdata,
	},
};
#endif // CONFIG_INPUT_KXTJ9

void __init msm7627a_sensor_init(void)
{
#ifdef CONFIG_AVAGO_APDS990X
	if (machine_is_msm8625_skua() || machine_is_msm7627a_evb() || machine_is_msm8625_evb() || machine_is_msm8625_qrd5() || machine_is_msm7x27a_qrd5a() ) {
		apds990x_setup();
	}
#endif

#ifdef CONFIG_MPU_SENSORS_MPU3050
	if (machine_is_msm7627a_evb() || machine_is_msm8625_evb() || machine_is_msm8625_qrd5() || machine_is_msm7x27a_qrd5a()) {
		mpu3050_gpio_setup();
	}
#endif

#ifdef CONFIG_BOSCH_BMA250
	if (machine_is_msm8625_qrd7()) {
		pr_info("i2c_register_board_info BMA250 ACC\n");
		i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
					bma250_i2c_info,
					ARRAY_SIZE(bma250_i2c_info));
	}
#endif

#ifdef CONFIG_MT9E013
		i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
					tps61310_i2c_info,
					ARRAY_SIZE(tps61310_i2c_info));
#endif

#ifdef CONFIG_GSENSOR_BMA2X2
		pr_info("i2c_register_board_info BMA2X2 ACC\n");
		i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
					bma2x2_i2c_info,
					ARRAY_SIZE(bma2x2_i2c_info));
#endif 
#ifdef CONFIG_GSENSOR_MC32X0
		i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
					mc32x0_i2c_info,
					ARRAY_SIZE(mc32x0_i2c_info));
#endif
#ifdef CONFIG_GSENSOR_LIS3DH
		pr_info("i2c_register_board_info lis3dh_acc ACC\n");
		i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
					lis3dh_acc_i2c_info,
					ARRAY_SIZE(lis3dh_acc_i2c_info));
#endif 

#ifdef CONFIG_INPUT_LTR502
	if (machine_is_msm8625_qrd7()) {
		pr_info("i2c_register_board_info LTR502\n");
		ltr502_light_gpio_setup();
		i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
				ltr502_light_i2c_info,
				ARRAY_SIZE(ltr502_light_i2c_info));
	}
#endif

#ifdef CONFIG_INPUT_LTR558
	pr_info("i2c_register_board_info LTR558\n");
	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			ltr558_light_i2c_info,
			ARRAY_SIZE(ltr558_light_i2c_info));
#endif

#if defined(CONFIG_INPUT_PS31XX)
	printk("i2c_register_board_info PS31XX\n");
	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			ps31xx_light_i2c_info,
			ARRAY_SIZE(ps31xx_light_i2c_info));
#endif

#ifdef CONFIG_INPUT_TMD27713
	taos_init_irq();
	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID, 
			tmd27713_gpio_i2c_board_info,
			ARRAY_SIZE(tmd27713_gpio_i2c_board_info));
#endif

#ifdef CONFIG_SENSORS_AK8975
	if (machine_is_msm8625_qrd7()) {
		pr_info("i2c_register_board_info AKM8975\n");
		akm_gpio_setup();
		akm_platform_data_8975.gpio_DRDY = 18;
		akm8975_i2c_info[0].irq = gpio_to_irq(akm_platform_data_8975.gpio_DRDY);
		i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
				akm8975_i2c_info,
				ARRAY_SIZE(akm8975_i2c_info));
	}
#endif

#ifdef CONFIG_SENSORS_AK8963
		pr_info("i2c_register_board_info AKM8963\n");
		akm_gpio_setup();
		i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
				akm8963_i2c_info,
				ARRAY_SIZE(akm8963_i2c_info));
#endif

#ifdef CONFIG_INPUT_KXTJ9
	if(machine_is_msm8625_skua()) {
	pr_info("i2c_register_board_info KXTJ9\n");
		i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			accel_kxtj9_i2c_info,
			ARRAY_SIZE(accel_kxtj9_i2c_info));
	}
#endif
}
