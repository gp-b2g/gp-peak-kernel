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
#define DEBUG

#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/bootmem.h>
#include <linux/regulator/consumer.h>
#include <asm/mach-types.h>
#include <asm/io.h>
#include <mach/msm_bus_board.h>
#include <mach/msm_memtypes.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/gpiomux.h>
#include <mach/socinfo.h>
#include <mach/rpc_pmapp.h>
#include "devices.h"
#include "board-msm7627a.h"
#include <mach/msm_iomap.h>
#include <mach/msm_smsm.h>
#ifdef CONFIG_FB_MSM_TRIPLE_BUFFER
#define MSM_FB_SIZE		0x4BF000
#define MSM7x25A_MSM_FB_SIZE    0x1C2000
#define MSM8x25_MSM_FB_SIZE	0x5FA000
#else
#define MSM_FB_SIZE		0x32A000
#define MSM7x25A_MSM_FB_SIZE	0x12C000
#define MSM8x25_MSM_FB_SIZE	0x3FC000
#endif

#define GPIO_QRD3_LCD_BRDG_RESET_N	85
#define GPIO_QRD3_LCD_BACKLIGHT_EN	96

#define GPIO_SKUA_LCD_BRDG_RESET_N 129
#define GPIO_SKUA_LCD_BACKLIGHT_EN 28
#define GPIO_SKUA_LCD_ID 11

#define machine_is_msm8225_cellon() (1)

#define GPIO_C8681_LCD_RESET_N	 4
#define GPIO_C8681_LCD_MODESEL	113
#define GPIO_C8681_LCD_ID0 129
#define GPIO_C8681_LCD_ID1 124

#define GPIO_C8680_LCD_RESET_N	 85
#define GPIO_C8680_LCD_BACKLIGHT_EN 96
#define GPIO_C8680_LCD_ID 118

/* LK splash flag, 0 - off, 1 - command, 2 - video */
static int cont_splash_enabled = 0;
static int __init lk_splash_setup(char *str)
{
	cont_splash_enabled = simple_strtol(str, NULL, 0);
	printk("cont_splash_enabled = %d\n", cont_splash_enabled);
	return 1;
}
__setup("splash=", lk_splash_setup);

/*
 * Reserve enough v4l2 space for a double buffered full screen
 * res image (864x480x1.5x2)
 */
#define MSM_V4L2_VIDEO_OVERLAY_BUF_SIZE 1244160

static unsigned fb_size = MSM_FB_SIZE;
static int __init fb_size_setup(char *p)
{
	fb_size = memparse(p, NULL);
	return 0;
}

early_param("fb_size", fb_size_setup);

/* Common EXT power control for QRD */
static struct regulator *reg_ext_2v8;
static struct regulator *reg_ext_1v8;

static int qrd_lcd_ext_power_control(int on)
{
	int rc = 0;

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

static int cont_splash_done = 0;
static void qrd_lcd_splash_power_vote(int on)
{
	if (on) {
		qrd_lcd_ext_power_control(1);
	} else if (!cont_splash_done) {
		qrd_lcd_ext_power_control(0);
	}

	return;
}

static uint32_t lcdc_truly_gpio_initialized;

#define SKU3_LCDC_GPIO_DISPLAY_RESET	90
#define SKU3_LCDC_GPIO_SPI_MOSI		19
#define SKU3_LCDC_GPIO_SPI_CLK		20
#define SKU3_LCDC_GPIO_SPI_CS0_N	21
#define SKU3_LCDC_LCD_CAMERA_LDO_2V8	35  /*LCD_CAMERA_LDO_2V8*/
#define SKU3_LCDC_LCD_CAMERA_LDO_1V8	34  /*LCD_CAMERA_LDO_1V8*/
#define SKU3_1_LCDC_LCD_CAMERA_LDO_1V8	58  /*LCD_CAMERA_LDO_1V8*/

static uint32_t lcdc_truly_gpio_table[] = {
	19,
	20,
	21,
	89,
	90,
};

static char *lcdc_gpio_name_table[5] = {
	"spi_mosi",
	"spi_clk",
	"spi_cs",
	"gpio_bkl_en",
	"gpio_disp_reset",
};

static int lcdc_truly_gpio_init(void)
{
	int i;
	int rc = 0;

    if (!lcdc_truly_gpio_initialized) {
        for (i = 0; i < ARRAY_SIZE(lcdc_truly_gpio_table); i++) {
            rc = gpio_request(lcdc_truly_gpio_table[i],
                    lcdc_gpio_name_table[i]);
            if (rc < 0) {
                pr_err("Error request gpio %s\n",
                        lcdc_gpio_name_table[i]);
                goto truly_gpio_fail;
            }

            /* Skip control GPIO config if continuous splash */
            rc = gpio_tlmm_config(GPIO_CFG(lcdc_truly_gpio_table[i],
                        0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
                        GPIO_CFG_2MA), GPIO_CFG_ENABLE);
            if (rc < 0) {
                pr_err("Error config lcdc gpio:%d\n",
                        lcdc_truly_gpio_table[i]);
                goto truly_gpio_fail;
            }

            if (!cont_splash_enabled)
                rc = gpio_direction_output(lcdc_truly_gpio_table[i], 0);
            else
                rc = gpio_direction_output(lcdc_truly_gpio_table[i], 1);

            if (rc < 0) {
                pr_err("Error direct lcdc gpio:%d\n",
                        lcdc_truly_gpio_table[i]);
                goto truly_gpio_fail;
            }
        }

		lcdc_truly_gpio_initialized = 1;
	}

	return rc;

truly_gpio_fail:
	for (; i >= 0; i--) {
		pr_err("Freeing GPIO: %d", lcdc_truly_gpio_table[i]);
		gpio_free(lcdc_truly_gpio_table[i]);
	}

	lcdc_truly_gpio_initialized = 0;
	return rc;
}

static int sku3_lcdc_power_save(int on)
{
	int rc = 0;

	pr_debug("%s: on = %d\n", __func__, on);

	rc = qrd_lcd_ext_power_control(on);

        if (on) {
                rc = lcdc_truly_gpio_init();
                if (rc < 0) {
                        pr_err("%s(): Truly GPIO initializations failed",
                                        __func__);
                        return rc;
                }

                if (!cont_splash_enabled || cont_splash_done) {
                        if (lcdc_truly_gpio_initialized) {
                                /*LCD reset*/
                                gpio_set_value(SKU3_LCDC_GPIO_DISPLAY_RESET, 0);
                                msleep(5);
                                gpio_set_value(SKU3_LCDC_GPIO_DISPLAY_RESET, 1);
                        }
                }
        } else {
                /* pull down LCD IO to avoid current leakage */
                gpio_set_value(SKU3_LCDC_GPIO_SPI_MOSI, 0);
                gpio_set_value(SKU3_LCDC_GPIO_SPI_CLK, 0);
                gpio_set_value(SKU3_LCDC_GPIO_SPI_CS0_N, 0);
                gpio_set_value(SKU3_LCDC_GPIO_DISPLAY_RESET, 0);
        }

        /* reduce ref count of ext power */
	if (cont_splash_enabled && !cont_splash_done) {
		qrd_lcd_splash_power_vote(0);
                cont_splash_done = 1;
	}

	return rc;
}

static struct msm_panel_common_pdata lcdc_truly_panel_data = {
	.panel_config_gpio = NULL,
	.gpio_num	  = lcdc_truly_gpio_table,
};

static struct platform_device lcdc_truly_panel_device = {
	.name   = "lcdc_truly_hvga_ips3p2335_pt",
	.id     = 0,
	.dev    = {
		.platform_data = &lcdc_truly_panel_data,
	}
};

static struct regulator_bulk_data regs_lcdc[] = {
	{ .supply = "gp2",   .min_uV = 2850000, .max_uV = 2850000 },
	{ .supply = "msme1", .min_uV = 1800000, .max_uV = 1800000 },
};
static uint32_t lcdc_gpio_initialized;

static void lcdc_toshiba_gpio_init(void)
{
	int rc = 0;
	if (!lcdc_gpio_initialized) {
		if (gpio_request(GPIO_SPI_CLK, "spi_clk")) {
			pr_err("failed to request gpio spi_clk\n");
			return;
		}
		if (gpio_request(GPIO_SPI_CS0_N, "spi_cs")) {
			pr_err("failed to request gpio spi_cs0_N\n");
			goto fail_gpio6;
		}
		if (gpio_request(GPIO_SPI_MOSI, "spi_mosi")) {
			pr_err("failed to request gpio spi_mosi\n");
			goto fail_gpio5;
		}
		if (gpio_request(GPIO_SPI_MISO, "spi_miso")) {
			pr_err("failed to request gpio spi_miso\n");
			goto fail_gpio4;
		}
		if (gpio_request(GPIO_DISPLAY_PWR_EN, "gpio_disp_pwr")) {
			pr_err("failed to request gpio_disp_pwr\n");
			goto fail_gpio3;
		}
		if (gpio_request(GPIO_BACKLIGHT_EN, "gpio_bkl_en")) {
			pr_err("failed to request gpio_bkl_en\n");
			goto fail_gpio2;
		}
		pmapp_disp_backlight_init();

		rc = regulator_bulk_get(NULL, ARRAY_SIZE(regs_lcdc),
					regs_lcdc);
		if (rc) {
			pr_err("%s: could not get regulators: %d\n",
					__func__, rc);
			goto fail_gpio1;
		}

		rc = regulator_bulk_set_voltage(ARRAY_SIZE(regs_lcdc),
				regs_lcdc);
		if (rc) {
			pr_err("%s: could not set voltages: %d\n",
					__func__, rc);
			goto fail_vreg;
		}
		lcdc_gpio_initialized = 1;
	}
	return;
fail_vreg:
	regulator_bulk_free(ARRAY_SIZE(regs_lcdc), regs_lcdc);
fail_gpio1:
	gpio_free(GPIO_BACKLIGHT_EN);
fail_gpio2:
	gpio_free(GPIO_DISPLAY_PWR_EN);
fail_gpio3:
	gpio_free(GPIO_SPI_MISO);
fail_gpio4:
	gpio_free(GPIO_SPI_MOSI);
fail_gpio5:
	gpio_free(GPIO_SPI_CS0_N);
fail_gpio6:
	gpio_free(GPIO_SPI_CLK);
	lcdc_gpio_initialized = 0;
}

static uint32_t lcdc_gpio_table[] = {
	GPIO_SPI_CLK,
	GPIO_SPI_CS0_N,
	GPIO_SPI_MOSI,
	GPIO_DISPLAY_PWR_EN,
	GPIO_BACKLIGHT_EN,
	GPIO_SPI_MISO,
};

static void config_lcdc_gpio_table(uint32_t *table, int len, unsigned enable)
{
	int n;

	if (lcdc_gpio_initialized) {
		/* All are IO Expander GPIOs */
		for (n = 0; n < (len - 1); n++)
			gpio_direction_output(table[n], 1);
	}
}

static void lcdc_toshiba_config_gpios(int enable)
{
	config_lcdc_gpio_table(lcdc_gpio_table,
		ARRAY_SIZE(lcdc_gpio_table), enable);
}

static int msm_fb_lcdc_power_save(int on)
{
	int rc = 0;
	/* Doing the init of the LCDC GPIOs very late as they are from
		an I2C-controlled IO Expander */
	lcdc_toshiba_gpio_init();

	if (lcdc_gpio_initialized) {
		gpio_set_value_cansleep(GPIO_DISPLAY_PWR_EN, on);
		gpio_set_value_cansleep(GPIO_BACKLIGHT_EN, on);

		rc = on ? regulator_bulk_enable(
				ARRAY_SIZE(regs_lcdc), regs_lcdc) :
			  regulator_bulk_disable(
				ARRAY_SIZE(regs_lcdc), regs_lcdc);

		if (rc)
			pr_err("%s: could not %sable regulators: %d\n",
					__func__, on ? "en" : "dis", rc);
	}

	return rc;
}

static int lcdc_toshiba_set_bl(int level)
{
	int ret;

	ret = pmapp_disp_backlight_set_brightness(level);
	if (ret)
		pr_err("%s: can't set lcd backlight!\n", __func__);

	return ret;
}


static int msm_lcdc_power_save(int on)
{
	int rc = 0;
	if (machine_is_msm7627a_qrd3() || machine_is_msm8625_qrd7())
		rc = sku3_lcdc_power_save(on);
	else
		rc = msm_fb_lcdc_power_save(on);

	return rc;
}

static struct lcdc_platform_data lcdc_pdata = {
	.lcdc_gpio_config = NULL,
	.lcdc_power_save   = msm_lcdc_power_save,
};

static int lcd_panel_spi_gpio_num[] = {
		GPIO_SPI_MOSI,  /* spi_sdi */
		GPIO_SPI_MISO,  /* spi_sdoi */
		GPIO_SPI_CLK,   /* spi_clk */
		GPIO_SPI_CS0_N, /* spi_cs  */
};

static struct msm_panel_common_pdata lcdc_toshiba_panel_data = {
	.panel_config_gpio = lcdc_toshiba_config_gpios,
	.pmic_backlight = lcdc_toshiba_set_bl,
	.gpio_num	 = lcd_panel_spi_gpio_num,
};

static struct platform_device lcdc_toshiba_panel_device = {
	.name   = "lcdc_toshiba_fwvga_pt",
	.id     = 0,
	.dev    = {
		.platform_data = &lcdc_toshiba_panel_data,
	}
};

static struct resource msm_fb_resources[] = {
	{
		.flags  = IORESOURCE_DMA,
	}
};

#ifdef CONFIG_MSM_V4L2_VIDEO_OVERLAY_DEVICE
static struct resource msm_v4l2_video_overlay_resources[] = {
	{
		.flags = IORESOURCE_DMA,
	}
};
#endif

#define LCDC_TOSHIBA_FWVGA_PANEL_NAME   "lcdc_toshiba_fwvga_pt"
#define MIPI_CMD_RENESAS_FWVGA_PANEL_NAME       "mipi_cmd_renesas_fwvga"

static int skua_panel_is_himax(void)
{
	static int lcd_gpio_detected = 0;
	static int is_himax = 0; //default is NT35510
	int rc = 0;

	if (!lcd_gpio_detected) {
		rc = gpio_request(GPIO_SKUA_LCD_ID, "skua_lcd_detect");
		if (rc < 0) {
			pr_err("%s: gpio_request skua_lcd_detect failed!",
					__func__);
			goto done;
		}

		rc = gpio_tlmm_config(GPIO_CFG(GPIO_SKUA_LCD_ID, 0,
					GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
					GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		if (rc < 0) {
			pr_err("%s: unable to config skua_lcd_detect\n", __func__);
			gpio_free(GPIO_SKUA_LCD_ID);
			goto done;
		}

		is_himax = gpio_get_value(GPIO_SKUA_LCD_ID);
		printk("%s: is_himax = %d\n", __func__, is_himax);

		gpio_free(GPIO_SKUA_LCD_ID);

		lcd_gpio_detected = true;
	}

done:
	return is_himax;
}

typedef enum {
	MANUFACTURER_TIANMA,
	MANUFACTURER_BYD,
	MANUFACTURER_BOE,
	MANUFACTURER_CHIMEI,
	MANUFACTURER_TRULY,
	MANUFACTURER_SHARP,
	MANUFACTURER_UNKOWN,
} PANEL_MANUFACTURER;

typedef enum {
	C8680,
	C8681,
} CELLON_PROJECT_NAME;

PANEL_MANUFACTURER c8681_panel_manufacturer;
PANEL_MANUFACTURER c8680_panel_manufacturer;

#ifdef CONFIG_CELLON_PRJ_C8681
static int c8681_detect_lcd_panel(void)
{
	int rc = 0;
	int lcd_id0 = 0;
	int lcd_id1 = 0;

	rc = gpio_request(GPIO_C8681_LCD_ID0, "c8681_lcd_id0");
	if (rc < 0) {
		pr_err("%s: gpio_request c8681_lcd_id0 failed!",
				__func__);
		return rc;
	}
	rc = gpio_tlmm_config(GPIO_CFG(GPIO_C8681_LCD_ID0, 0,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s: unable to config c8681_lcd_id0\n", __func__);
		gpio_free(GPIO_C8681_LCD_ID0);
		return rc;
	}
	lcd_id0 = gpio_get_value(GPIO_C8681_LCD_ID0);
	gpio_free(GPIO_C8681_LCD_ID0);

	rc = gpio_request(GPIO_C8681_LCD_ID1, "c8681_lcd_id1");
	if (rc < 0) {
		pr_err("%s: gpio_request c8681_lcd_id0 failed!",
				__func__);
		return rc;
	}
	rc = gpio_tlmm_config(GPIO_CFG(GPIO_C8681_LCD_ID1, 0,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s: unable to config c8681_lcd_id1\n", __func__);
		gpio_free(GPIO_C8681_LCD_ID1);
		return rc;
	}
	lcd_id1 = gpio_get_value(GPIO_C8681_LCD_ID1);
	gpio_free(GPIO_C8681_LCD_ID1);

	if ((lcd_id0 == 1) && (lcd_id1 == 0))
		c8681_panel_manufacturer = MANUFACTURER_TIANMA;
	else if ((lcd_id0 == 0) && (lcd_id1 == 0))
		c8681_panel_manufacturer = MANUFACTURER_CHIMEI;
	else if ((lcd_id0 == 1) && (lcd_id1 == 1))
		c8681_panel_manufacturer = MANUFACTURER_BOE;
	else
		c8681_panel_manufacturer = MANUFACTURER_UNKOWN;

	printk("panel_manufacturer = %d\n", c8681_panel_manufacturer);

	return rc;

}
#else
static int c8680_detect_lcd_panel(void)
{
	int rc = 0;
	int lcd_id = 0;

	rc = gpio_request(GPIO_C8680_LCD_ID, "c8680_lcd_id");
	if (rc < 0) {
		pr_err("%s: gpio_request c8680_lcd_id failed!",
				__func__);
		return rc;
	}
	rc = gpio_tlmm_config(GPIO_CFG(GPIO_C8680_LCD_ID, 0,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s: unable to config c8680_lcd_id\n", __func__);
		gpio_free(GPIO_C8680_LCD_ID);
		return rc;
	}
	lcd_id = gpio_get_value(GPIO_C8680_LCD_ID);
	gpio_free(GPIO_C8680_LCD_ID);

	if (lcd_id == 1)
		c8680_panel_manufacturer = MANUFACTURER_TRULY;
	else if (lcd_id == 0)
		c8680_panel_manufacturer = MANUFACTURER_SHARP;
	else
		c8680_panel_manufacturer = MANUFACTURER_UNKOWN;

	printk("c8680 panel_manufacturer = %d\n", c8680_panel_manufacturer);

	return rc;

}
#endif

char Lcd_Panel_manufacturer;
char cellon_project_name;

static int cellon_auto_detect_lcd_panel(const char *name)
{
	int ret = -ENODEV;

#ifdef CONFIG_CELLON_PRJ_C8681
	if (c8681_panel_manufacturer == MANUFACTURER_TIANMA) {
		if (!strcmp(name, "mipi_video_himax_wvga"))
			ret = 0;
	} else if (c8681_panel_manufacturer == MANUFACTURER_BOE) {
		if (!strcmp(name, "mipi_cmd_nt35510_boe_wvga"))
			ret = 0;
	} else if (c8681_panel_manufacturer == MANUFACTURER_CHIMEI) {
		if (!strcmp(name, "mipi_cmd_otm8009a_wvga"))
			ret = 0;
	}
	
	Lcd_Panel_manufacturer = c8681_panel_manufacturer;
	cellon_project_name = C8681;
#else
	if (c8680_panel_manufacturer == MANUFACTURER_TRULY) {
		if (!strcmp(name, "mipi_cmd_otm9608a_qhd"))
			ret = 0;
	} else if (c8680_panel_manufacturer == MANUFACTURER_SHARP) {
		if (!strcmp(name, "mipi_cmd_nt35565_qhd"))
			ret = 0;
	}

	Lcd_Panel_manufacturer = c8680_panel_manufacturer;
	cellon_project_name = C8680;
	
#endif

	return ret;
}

static int msm_fb_detect_panel(const char *name)
{
	int ret = -ENODEV;

	printk("%s: %d\n", __func__, __LINE__);
	if (machine_is_msm8225_cellon()) {
		ret = cellon_auto_detect_lcd_panel(name);
	} else if (machine_is_msm7x27a_surf() || machine_is_msm7625a_surf() ||
			machine_is_msm8625_surf()) {
		if (!strncmp(name, "lcdc_toshiba_fwvga_pt", 21) ||
				!strncmp(name, "mipi_cmd_renesas_fwvga", 22))
			ret = 0;
	} else if (machine_is_msm7x27a_ffa() || machine_is_msm7625a_ffa()
					|| machine_is_msm8625_ffa()) {
		if (!strncmp(name, "mipi_cmd_renesas_fwvga", 22))
			ret = 0;
	} else if (machine_is_msm7627a_qrd1()) {
		if (!strncmp(name, "mipi_video_truly_wvga", 21))
			ret = 0;
	} else if (machine_is_msm7627a_qrd3() || machine_is_msm8625_qrd7()) {
		if (!strncmp(name, "lcdc_truly_hvga_ips3p2335_pt", 28))
			ret = 0;
	} else if (machine_is_msm7627a_evb() || machine_is_msm8625_evb() || machine_is_msm8625_qrd5() || machine_is_msm7x27a_qrd5a()) {
        if (cont_splash_enabled == 1) {
            if (!strncmp(name, "mipi_cmd_nt35510_wvga", 21))
                ret = 0;
        } else if (cont_splash_enabled == 2) {
            if (!strncmp(name, "mipi_video_nt35510_wvga", 21))
                ret = 0;
        } else {
            if (!strncmp(name, "mipi_cmd_nt35510_wvga", 21))
                ret = 0;
        }
	} else if (machine_is_msm8625_skua()) {
		if (!strncmp(name, "mipi_video_himax_wvga", 21) && skua_panel_is_himax())
			ret = 0;
		else if (!strncmp(name, "mipi_cmd_nt35510_alaska_wvga", 28) && !skua_panel_is_himax())
			ret = 0;
	} else if (machine_is_msm8625_evt()) {
		if (!strncmp(name, "mipi_video_nt35510_wvga", 23))
			ret = 0;
	}

#if !defined(CONFIG_FB_MSM_LCDC_AUTO_DETECT) && \
	!defined(CONFIG_FB_MSM_MIPI_PANEL_AUTO_DETECT) && \
	!defined(CONFIG_FB_MSM_LCDC_MIPI_PANEL_AUTO_DETECT)
		if (machine_is_msm7x27a_surf() ||
			machine_is_msm7625a_surf() ||
			machine_is_msm8625_surf()) {
			if (!strncmp(name, LCDC_TOSHIBA_FWVGA_PANEL_NAME,
				strnlen(LCDC_TOSHIBA_FWVGA_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
				return 0;
		}
#endif

	return ret;
}

static int mipi_truly_set_bl(int on)
{
	gpio_set_value_cansleep(QRD_GPIO_BACKLIGHT_EN, !!on);

	return 1;
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = msm_fb_detect_panel,
};

static struct platform_device msm_fb_device = {
	.name   = "msm_fb",
	.id     = 0,
	.num_resources  = ARRAY_SIZE(msm_fb_resources),
	.resource       = msm_fb_resources,
	.dev    = {
		.platform_data = &msm_fb_pdata,
	}
};

#ifdef CONFIG_MSM_V4L2_VIDEO_OVERLAY_DEVICE
static struct platform_device msm_v4l2_video_overlay_device = {
		.name   = "msm_v4l2_overlay_pd",
		.id     = 0,
		.num_resources  = ARRAY_SIZE(msm_v4l2_video_overlay_resources),
		.resource       = msm_v4l2_video_overlay_resources,
	};
#endif


#ifdef CONFIG_FB_MSM_MIPI_DSI
static int mipi_renesas_set_bl(int level)
{
	int ret;

	ret = pmapp_disp_backlight_set_brightness(level);

	if (ret)
		pr_err("%s: can't set lcd backlight!\n", __func__);

	return ret;
}

static struct msm_panel_common_pdata mipi_renesas_pdata = {
	.pmic_backlight = mipi_renesas_set_bl,
};


static struct platform_device mipi_dsi_renesas_panel_device = {
	.name = "mipi_renesas",
	.id = 0,
	.dev    = {
		.platform_data = &mipi_renesas_pdata,
	}
};
#endif

static struct msm_panel_common_pdata mipi_truly_pdata = {
	.pmic_backlight = mipi_truly_set_bl,
};

static struct platform_device mipi_dsi_truly_panel_device = {
	.name   = "mipi_truly",
	.id     = 0,
	.dev    = {
		.platform_data = &mipi_truly_pdata,
	}
};

/* EVB, SKU5 */
static struct msm_panel_common_pdata mipi_NT35510_pdata = {
	.backlight_level = NULL, //set in tps61161 driver
	.gpio = GPIO_QRD3_LCD_BACKLIGHT_EN,
};

static struct platform_device mipi_dsi_NT35510_panel_device = {
	.name = "mipi_NT35510",
	.id   = 0,
	.dev  = {
		.platform_data = &mipi_NT35510_pdata,
	}
};

static struct platform_device evb_backlight_device = {
	.name = "tps6116_backlight",
	.id   = 0,
	.dev  = {
		.platform_data = &mipi_NT35510_pdata,
	}
};

/* SKUA - NT35510 or Himax */
static struct msm_panel_common_pdata mipi_NT35510_alaska_pdata = {
	.backlight_level = NULL, //set in tps61161 driver
	.gpio = GPIO_SKUA_LCD_BACKLIGHT_EN,
};

static struct platform_device mipi_dsi_NT35510_alaska_panel_device = {
	.name = "mipi_nt35510_alaska",
	.id   = 0,
	.dev  = {
		.platform_data = &mipi_NT35510_alaska_pdata,
	}
};

static struct msm_panel_common_pdata mipi_himax_pdata = {
	.backlight_level = NULL, //set in tps61161 driver
	.gpio = GPIO_SKUA_LCD_BACKLIGHT_EN,
};

static struct platform_device mipi_dsi_himax_panel_device = {
	.name = "mipi_himax",
	.id   = 0,
	.dev  = {
		.platform_data = &mipi_himax_pdata,
	}
};

static struct platform_device skua_backlight_device = {
	.name = "tps6116_backlight",
	.id   = 0,
	.dev  = {
		.platform_data = &mipi_NT35510_alaska_pdata, //Himax or NT35510
	}
};

static struct msm_panel_common_pdata mipi_NT35516_pdata = {
	.pmic_backlight = NULL,
};

static struct platform_device mipi_dsi_NT35516_panel_device = {
	.name   = "mipi_NT35516",
	.id     = 0,
	.dev    = {
		.platform_data = &mipi_NT35516_pdata,
	}
};

static struct msm_panel_common_pdata mipi_NT35565_pdata = {
	.backlight_level = NULL,
};

static struct platform_device mipi_dsi_NT35565_panel_device = {
	.name = "mipi_NT35565",
	.id = 0,
	.dev = {
		.platform_data = &mipi_NT35565_pdata,
	}
};


static struct platform_device *msm_fb_devices[] __initdata = {
	&msm_fb_device,
	&lcdc_toshiba_panel_device,
#ifdef CONFIG_FB_MSM_MIPI_DSI
	&mipi_dsi_renesas_panel_device,
#endif
#ifdef CONFIG_MSM_V4L2_VIDEO_OVERLAY_DEVICE
	&msm_v4l2_video_overlay_device,
#endif
};

static struct platform_device *qrd_fb_devices[] __initdata = {
	&msm_fb_device,
	&mipi_dsi_truly_panel_device,
};

static struct platform_device *qrd3_fb_devices[] __initdata = {
	&msm_fb_device,
	&lcdc_truly_panel_device,
};

static struct platform_device *evb_fb_devices[] __initdata = {
	&msm_fb_device,
	&mipi_dsi_NT35510_panel_device,
	&mipi_dsi_NT35516_panel_device,
	&evb_backlight_device,
};

static struct platform_device *skua_fb_devices[] __initdata = {
	&msm_fb_device,
	&mipi_dsi_NT35510_alaska_panel_device, //NT35510 or Himax
	&skua_backlight_device,
};

static struct platform_device *c8680_fb_devices[] __initdata = {
	&msm_fb_device,
	&mipi_dsi_NT35565_panel_device,
	&mipi_dsi_NT35516_panel_device,
};

void __init msm_msm7627a_allocate_memory_regions(void)
{
	void *addr;
	unsigned long fb_size;

	if (machine_is_msm8225_cellon())
		fb_size = MSM8x25_MSM_FB_SIZE;
	else if (machine_is_msm7625a_surf() || machine_is_msm7625a_ffa())
		fb_size = MSM7x25A_MSM_FB_SIZE;
	else if (machine_is_msm7627a_evb() || machine_is_msm8625_evb()
						|| machine_is_msm8625_evt())
		fb_size = MSM8x25_MSM_FB_SIZE;
	else
		fb_size = MSM_FB_SIZE;

	addr = alloc_bootmem_align(fb_size, 0x1000);
	msm_fb_resources[0].start = __pa(addr);
	msm_fb_resources[0].end = msm_fb_resources[0].start + fb_size - 1;
	pr_info("allocating %lu bytes at %p (%lx physical) for fb\n", fb_size,
						addr, __pa(addr));

#ifdef CONFIG_MSM_V4L2_VIDEO_OVERLAY_DEVICE
	fb_size = MSM_V4L2_VIDEO_OVERLAY_BUF_SIZE;
	addr = alloc_bootmem_align(fb_size, 0x1000);
	msm_v4l2_video_overlay_resources[0].start = __pa(addr);
	msm_v4l2_video_overlay_resources[0].end =
		msm_v4l2_video_overlay_resources[0].start + fb_size - 1;
	pr_debug("allocating %lu bytes at %p (%lx physical) for v4l2\n",
		fb_size, addr, __pa(addr));
#endif

}

static struct msm_panel_common_pdata mdp_pdata = {
	.gpio = 97,
	.mdp_rev = MDP_REV_303,
	.cont_splash_enabled = 0,
};

#define GPIO_LCDC_BRDG_PD	128
#define GPIO_LCDC_BRDG_RESET_N	129
#define GPIO_LCD_DSI_SEL	125
#define LCDC_RESET_PHYS		0x90008014
#ifdef CONFIG_FB_MSM_MIPI_DSI

static  void __iomem *lcdc_reset_ptr;

static unsigned mipi_dsi_gpio[] = {
		GPIO_CFG(GPIO_LCDC_BRDG_RESET_N, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* LCDC_BRDG_RESET_N */
		GPIO_CFG(GPIO_LCDC_BRDG_PD, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* LCDC_BRDG_PD */
};

static unsigned lcd_dsi_sel_gpio[] = {
	GPIO_CFG(GPIO_LCD_DSI_SEL, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP,
			GPIO_CFG_2MA),
};
#endif

enum {
	DSI_SINGLE_LANE = 1,
	DSI_TWO_LANES,
};
#ifdef CONFIG_FB_MSM_MIPI_DSI

static int msm_fb_get_lane_config(void)
{
	/* For MSM7627A SURF/FFA and QRD */
	int rc = DSI_TWO_LANES;
	if (machine_is_msm8225_cellon()) {
		pr_info("DSI_TWO_LANES\n");
	} else if (machine_is_msm7625a_surf() || machine_is_msm7625a_ffa()) {
		rc = DSI_SINGLE_LANE;
		pr_info("DSI_SINGLE_LANES\n");
	} else {
		pr_info("DSI_TWO_LANES\n");
	}
	return rc;
}

static int msm_fb_dsi_client_msm_reset(void)
{
	int rc = 0;

	rc = gpio_request(GPIO_LCDC_BRDG_RESET_N, "lcdc_brdg_reset_n");
	if (rc < 0) {
		pr_err("failed to request lcd brdg reset_n\n");
		return rc;
	}

	rc = gpio_request(GPIO_LCDC_BRDG_PD, "lcdc_brdg_pd");
	if (rc < 0) {
		pr_err("failed to request lcd brdg pd\n");
		return rc;
	}

	rc = gpio_tlmm_config(mipi_dsi_gpio[0], GPIO_CFG_ENABLE);
	if (rc) {
		pr_err("Failed to enable LCDC Bridge reset enable\n");
		goto gpio_error;
	}

	rc = gpio_tlmm_config(mipi_dsi_gpio[1], GPIO_CFG_ENABLE);
	if (rc) {
		pr_err("Failed to enable LCDC Bridge pd enable\n");
		goto gpio_error2;
	}

	rc = gpio_direction_output(GPIO_LCDC_BRDG_RESET_N, 1);
	rc |= gpio_direction_output(GPIO_LCDC_BRDG_PD, 1);
	gpio_set_value_cansleep(GPIO_LCDC_BRDG_PD, 0);

	if (!rc) {
		if (machine_is_msm7x27a_surf() || machine_is_msm7625a_surf()
				|| machine_is_msm8625_surf()) {
			lcdc_reset_ptr = ioremap_nocache(LCDC_RESET_PHYS,
				sizeof(uint32_t));

			if (!lcdc_reset_ptr)
				return 0;
		}
		return rc;
	} else {
		goto gpio_error;
	}

gpio_error2:
	pr_err("Failed GPIO bridge pd\n");
	gpio_free(GPIO_LCDC_BRDG_PD);

gpio_error:
	pr_err("Failed GPIO bridge reset\n");
	gpio_free(GPIO_LCDC_BRDG_RESET_N);
	return rc;
}

static int mipi_truly_sel_mode(int video_mode)
{
	int rc = 0;

	rc = gpio_request(GPIO_LCD_DSI_SEL, "lcd_dsi_sel");
	if (rc < 0)
		goto gpio_error;

	rc = gpio_tlmm_config(lcd_dsi_sel_gpio[0], GPIO_CFG_ENABLE);
	if (rc)
		goto gpio_error;

	rc = gpio_direction_output(GPIO_LCD_DSI_SEL, 1);
	if (!rc) {
		gpio_set_value_cansleep(GPIO_LCD_DSI_SEL, video_mode);
		return rc;
	} else {
		goto gpio_error;
	}

gpio_error:
	pr_err("mipi_truly_sel_mode failed\n");
	gpio_free(GPIO_LCD_DSI_SEL);
	return rc;
}

static int msm_fb_dsi_client_qrd1_reset(void)
{
	int rc = 0;

	rc = gpio_request(GPIO_LCDC_BRDG_RESET_N, "lcdc_brdg_reset_n");
	if (rc < 0) {
		pr_err("failed to request lcd brdg reset_n\n");
		return rc;
	}

	rc = gpio_tlmm_config(mipi_dsi_gpio[0], GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("Failed to enable LCDC Bridge reset enable\n");
		return rc;
	}

	rc = gpio_direction_output(GPIO_LCDC_BRDG_RESET_N, 0);
	if (rc < 0) {
		pr_err("Failed GPIO bridge pd\n");
		gpio_free(GPIO_LCDC_BRDG_RESET_N);
		return rc;
	}

	mipi_truly_sel_mode(1);

	return rc;
}
#endif
#ifdef CONFIG_FB_MSM_MIPI_DSI

static unsigned qrd3_mipi_dsi_gpio[] = {
	GPIO_CFG(GPIO_QRD3_LCD_BRDG_RESET_N, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_NO_PULL,
			GPIO_CFG_2MA), /* GPIO_QRD3_LCD_BRDG_RESET_N */
	GPIO_CFG(GPIO_QRD3_LCD_BACKLIGHT_EN, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_PULL_UP,
			GPIO_CFG_2MA), /* GPIO_QRD3_LCD_BACKLIGHT_EN */
	GPIO_CFG(GPIO_QRD3_LCD_BACKLIGHT_EN, 0, GPIO_CFG_INPUT,
			GPIO_CFG_NO_PULL,
			GPIO_CFG_2MA), /* GPIO_QRD3_LCD_BACKLIGHT_EN */
};

static unsigned skua_mipi_dsi_gpio[] = {
	GPIO_CFG(GPIO_SKUA_LCD_BRDG_RESET_N, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_NO_PULL,
			GPIO_CFG_2MA), /* GPIO_QRD3_LCD_BRDG_RESET_N */
	GPIO_CFG(GPIO_SKUA_LCD_BACKLIGHT_EN, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_PULL_UP,
			GPIO_CFG_2MA), /* GPIO_QRD3_LCD_BACKLIGHT_EN */
};

static int msm_fb_dsi_client_skua_reset(void)
{
	int rc = 0;

	printk("%s\n", __func__);

	rc = gpio_request(GPIO_SKUA_LCD_BRDG_RESET_N, "skua_lcd_brdg_reset_n");
	if (rc < 0) {
		pr_err("Failed to request skua_lcd_brdg_reset_n\n");
		return rc;
	}

	rc = gpio_tlmm_config(skua_mipi_dsi_gpio[0], GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("Failed to enable skua_lcd_brdg_reset_n\n");
		gpio_free(GPIO_QRD3_LCD_BRDG_RESET_N);
		return rc;
	}

	rc = gpio_direction_output(GPIO_SKUA_LCD_BRDG_RESET_N, 0);
	if (rc < 0) {
		pr_err("Failed to direction skua_lcd_brdg_reset_n\n");
		gpio_free(GPIO_QRD3_LCD_BRDG_RESET_N);
		return rc;
	}

	rc = gpio_request(GPIO_SKUA_LCD_BACKLIGHT_EN, "skua_gpio_bkl_en");
	if (rc < 0) {
		pr_err("Failed to request skua_gpio_bkl_en\n");
		return rc;
	}

	rc = gpio_tlmm_config(skua_mipi_dsi_gpio[1], GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("Failed to config skua_gpio_bkl_en\n");
		gpio_free(GPIO_SKUA_LCD_BACKLIGHT_EN);
		return rc;
	}

	rc = gpio_direction_output(GPIO_SKUA_LCD_BACKLIGHT_EN, 0);
	if (rc < 0) {
		pr_err("failed to direction skua_gpio_bkl_en\n");
		gpio_free(GPIO_SKUA_LCD_BACKLIGHT_EN);
		return rc;
	}

	return rc;
}

static int msm_fb_dsi_client_qrd3_reset(void)
{
	int rc = 0;

    pr_debug("%s: cont_splash_enabled = %d\n", __func__, cont_splash_enabled);

    rc = gpio_request(GPIO_QRD3_LCD_BRDG_RESET_N, "qrd3_lcd_brdg_reset_n");
    if (rc < 0) {
        pr_err("Failed to request qrd3_lcd_brdg_reset_n\n");
        return rc;
    }

    rc = gpio_tlmm_config(qrd3_mipi_dsi_gpio[0], GPIO_CFG_ENABLE);
    if (rc < 0) {
        pr_err("Failed to config qrd3_gpio_bkl_en\n");
        goto gpio_fail1;
    }

    if (!cont_splash_enabled)
        rc = gpio_direction_output(GPIO_QRD3_LCD_BRDG_RESET_N, 0);
    else
        rc = gpio_direction_output(GPIO_QRD3_LCD_BRDG_RESET_N, 1);

    if (rc < 0) {
        pr_err("Failed to direction qrd3_gpio_bkl_en\n");
        goto gpio_fail1;
    }

    rc = gpio_request(GPIO_QRD3_LCD_BACKLIGHT_EN,
            "qrd3_gpio_bkl_en");
    if (rc < 0) {
        pr_err("Failed to request qrd3_gpio_bkl_en\n");
        goto gpio_fail1;
    }

    if((machine_is_msm8625_qrd5() && hw_version_is(3, 0)) || (machine_is_msm7x27a_qrd5a() && hw_version_is(3, 0))) {
        rc = gpio_tlmm_config(qrd3_mipi_dsi_gpio[2], GPIO_CFG_ENABLE);
        if (rc < 0) {
            pr_err("Failed to config qrd3_gpio_bkl_en\n");
            goto gpio_fail2;
        }
        rc = gpio_direction_input(GPIO_QRD3_LCD_BACKLIGHT_EN);
        if (rc < 0) {
            pr_err("Failed to direction qrd3_gpio_bkl_en\n");
            goto gpio_fail2;
        }
    } else {
        rc = gpio_tlmm_config(qrd3_mipi_dsi_gpio[1], GPIO_CFG_ENABLE);
        if (rc < 0) {
            pr_err("Failed to config qrd3_gpio_bkl_en\n");
            goto gpio_fail2;
        }

        if (!cont_splash_enabled)
            rc = gpio_direction_output(GPIO_QRD3_LCD_BACKLIGHT_EN, 0);
        else
            rc = gpio_direction_output(GPIO_QRD3_LCD_BACKLIGHT_EN, 1);

        if (rc < 0) {
            pr_err("Failed to direction qrd3_gpio_bkl_en\n");
            goto gpio_fail2;
        }
    }

    return rc;

gpio_fail2:
	gpio_free(GPIO_QRD3_LCD_BACKLIGHT_EN);
gpio_fail1:
	gpio_free(GPIO_QRD3_LCD_BRDG_RESET_N);
	return rc;
}

#ifdef CONFIG_CELLON_PRJ_C8681
static int msm_fb_dsi_client_c8681_reset(void)
{
	int rc = 0;

	printk("%s: %d\n", __func__, __LINE__);

	c8681_detect_lcd_panel();

	return rc;
}
#else
static int msm_fb_dsi_client_c8680_reset(void)
{
	int rc = 0;
	unsigned smem_size;
	unsigned int boot_reason=0;
	int lcd_en=1;
	printk("%s: %d\n", __func__, __LINE__);

	rc = gpio_request(GPIO_C8680_LCD_BACKLIGHT_EN, "gpio_bkl_en");
	if (rc < 0)
		return rc;

	rc = gpio_tlmm_config(GPIO_CFG(GPIO_C8680_LCD_BACKLIGHT_EN, 0,
		GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("failed GPIO_BACKLIGHT_EN tlmm config\n");
		return rc;
	}

	 //add by fengxiaoli to turn off the lcd when power off rtc
	boot_reason= *(unsigned int *)
                 (smem_get_entry(SMEM_POWER_ON_STATUS_INFO, &smem_size));
	printk(KERN_NOTICE "Boot Reason = %d\n",boot_reason);
	if (boot_reason==2)
	     lcd_en=0;
	 //end add
	 rc = gpio_direction_output(GPIO_C8680_LCD_BACKLIGHT_EN, lcd_en);
	if (rc < 0) {
		pr_err("failed to enable backlight\n");
		gpio_free(QRD_GPIO_BACKLIGHT_EN);
		return rc;
	}

	c8680_detect_lcd_panel();

	return rc;
}
#endif

static int msm_fb_dsi_client_cellon_reset(void)
{
	int rc = 0;

#ifdef CONFIG_CELLON_PRJ_C8681
	rc = msm_fb_dsi_client_c8681_reset();
#else
	rc = msm_fb_dsi_client_c8680_reset();
#endif

	return rc;
}

static int msm_fb_dsi_client_reset(void)
{
	int rc = 0;

	if (machine_is_msm8225_cellon())
		rc = msm_fb_dsi_client_cellon_reset();
	else if (machine_is_msm7627a_qrd1())
		rc = msm_fb_dsi_client_qrd1_reset();
	else if (machine_is_msm7627a_evb() || machine_is_msm8625_evb() || 
			machine_is_msm8625_qrd5()|| machine_is_msm7x27a_qrd5a() || 
			machine_is_msm8625_evt())
		rc = msm_fb_dsi_client_qrd3_reset();
	else if (machine_is_msm8625_skua())
		rc = msm_fb_dsi_client_skua_reset();
	else
		rc = msm_fb_dsi_client_msm_reset();

	return rc;
}

static struct regulator_bulk_data regs_dsi[] = {
	{ .supply = "gp2",   .min_uV = 2850000, .max_uV = 2850000 },
	{ .supply = "msme1", .min_uV = 1800000, .max_uV = 1800000 },
};

static int dsi_gpio_initialized;

static int mipi_dsi_panel_msm_power(int on)
{
	int rc = 0;
	uint32_t lcdc_reset_cfg;

	/* I2C-controlled GPIO Expander -init of the GPIOs very late */
	if (unlikely(!dsi_gpio_initialized)) {
		pmapp_disp_backlight_init();

		rc = gpio_request(GPIO_DISPLAY_PWR_EN, "gpio_disp_pwr");
		if (rc < 0) {
			pr_err("failed to request gpio_disp_pwr\n");
			return rc;
		}

		if (machine_is_msm7x27a_surf() || machine_is_msm7625a_surf()
				|| machine_is_msm8625_surf()) {
			rc = gpio_direction_output(GPIO_DISPLAY_PWR_EN, 1);
			if (rc < 0) {
				pr_err("failed to enable display pwr\n");
				goto fail_gpio1;
			}

			rc = gpio_request(GPIO_BACKLIGHT_EN, "gpio_bkl_en");
			if (rc < 0) {
				pr_err("failed to request gpio_bkl_en\n");
				goto fail_gpio1;
			}

			rc = gpio_direction_output(GPIO_BACKLIGHT_EN, 1);
			if (rc < 0) {
				pr_err("failed to enable backlight\n");
				goto fail_gpio2;
			}
		}

		rc = regulator_bulk_get(NULL, ARRAY_SIZE(regs_dsi), regs_dsi);
		if (rc) {
			pr_err("%s: could not get regulators: %d\n",
					__func__, rc);
			goto fail_gpio2;
		}

		rc = regulator_bulk_set_voltage(ARRAY_SIZE(regs_dsi),
						regs_dsi);
		if (rc) {
			pr_err("%s: could not set voltages: %d\n",
					__func__, rc);
			goto fail_vreg;
		}
		if (pmapp_disp_backlight_set_brightness(100))
			pr_err("backlight set brightness failed\n");

		dsi_gpio_initialized = 1;
	}
	if (machine_is_msm7x27a_surf() || machine_is_msm7625a_surf() ||
			machine_is_msm8625_surf()) {
		gpio_set_value_cansleep(GPIO_DISPLAY_PWR_EN, on);
		gpio_set_value_cansleep(GPIO_BACKLIGHT_EN, on);
	} else if (machine_is_msm7x27a_ffa() || machine_is_msm7625a_ffa()
					|| machine_is_msm8625_ffa()) {
		if (on) {
			/* This line drives an active low pin on FFA */
			rc = gpio_direction_output(GPIO_DISPLAY_PWR_EN, !on);
			if (rc < 0)
				pr_err("failed to set direction for "
					"display pwr\n");
		} else {
			gpio_set_value_cansleep(GPIO_DISPLAY_PWR_EN, !on);
			rc = gpio_direction_input(GPIO_DISPLAY_PWR_EN);
			if (rc < 0)
				pr_err("failed to set direction for "
					"display pwr\n");
		}
	}

	if (on) {
		gpio_set_value_cansleep(GPIO_LCDC_BRDG_PD, 0);

		if (machine_is_msm7x27a_surf() ||
				 machine_is_msm7625a_surf() ||
				 machine_is_msm8625_surf()) {
			lcdc_reset_cfg = readl_relaxed(lcdc_reset_ptr);
			rmb();
			lcdc_reset_cfg &= ~1;

			writel_relaxed(lcdc_reset_cfg, lcdc_reset_ptr);
			msleep(20);
			wmb();
			lcdc_reset_cfg |= 1;
			writel_relaxed(lcdc_reset_cfg, lcdc_reset_ptr);
		} else {
			gpio_set_value_cansleep(GPIO_LCDC_BRDG_RESET_N, 0);
			msleep(20);
			gpio_set_value_cansleep(GPIO_LCDC_BRDG_RESET_N, 1);
		}
	} else {
		gpio_set_value_cansleep(GPIO_LCDC_BRDG_PD, 1);
	}

	rc = on ? regulator_bulk_enable(ARRAY_SIZE(regs_dsi), regs_dsi) :
		  regulator_bulk_disable(ARRAY_SIZE(regs_dsi), regs_dsi);

	if (rc)
		pr_err("%s: could not %sable regulators: %d\n",
				__func__, on ? "en" : "dis", rc);

	return rc;
fail_vreg:
	regulator_bulk_free(ARRAY_SIZE(regs_dsi), regs_dsi);
fail_gpio2:
	gpio_free(GPIO_BACKLIGHT_EN);
fail_gpio1:
	gpio_free(GPIO_DISPLAY_PWR_EN);
	dsi_gpio_initialized = 0;
	return rc;
}

static int mipi_dsi_panel_qrd1_power(int on)
{
	int rc = 0;

	if (!dsi_gpio_initialized) {
		rc = gpio_request(QRD_GPIO_BACKLIGHT_EN, "gpio_bkl_en");
		if (rc < 0)
			return rc;

		rc = gpio_tlmm_config(GPIO_CFG(QRD_GPIO_BACKLIGHT_EN, 0,
			GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);
		if (rc < 0) {
			pr_err("failed GPIO_BACKLIGHT_EN tlmm config\n");
			return rc;
		}

		rc = gpio_direction_output(QRD_GPIO_BACKLIGHT_EN, 1);
		if (rc < 0) {
			pr_err("failed to enable backlight\n");
			gpio_free(QRD_GPIO_BACKLIGHT_EN);
			return rc;
		}
		dsi_gpio_initialized = 1;
	}

	gpio_set_value_cansleep(QRD_GPIO_BACKLIGHT_EN, !!on);

	if (on) {
		gpio_set_value_cansleep(GPIO_LCDC_BRDG_RESET_N, 1);
		msleep(20);
		gpio_set_value_cansleep(GPIO_LCDC_BRDG_RESET_N, 0);
		msleep(20);
		gpio_set_value_cansleep(GPIO_LCDC_BRDG_RESET_N, 1);

	}

	return rc;
}

static int mipi_dsi_panel_skua_power(int on)
{
	int rc = 0;

	printk("%s: on = %d\n", __func__, on);

	if (on) {
		gpio_set_value_cansleep(GPIO_SKUA_LCD_BRDG_RESET_N, 0);
		msleep(5);
		gpio_set_value_cansleep(GPIO_SKUA_LCD_BRDG_RESET_N, 1);
	} else {
		gpio_set_value_cansleep(GPIO_SKUA_LCD_BRDG_RESET_N, 0);
	}
	return rc;
}

static int mipi_dsi_panel_qrd3_power(int on)
{
	int rc = 0;

	printk("%s: on = %d\n", __func__, on);

	rc = qrd_lcd_ext_power_control(on);

        if (!cont_splash_enabled || cont_splash_done) {
                if (on) {
                        gpio_set_value_cansleep(GPIO_QRD3_LCD_BRDG_RESET_N, 0);
                        msleep(5);
                        gpio_set_value_cansleep(GPIO_QRD3_LCD_BRDG_RESET_N, 1);
                } else {
                        gpio_set_value_cansleep(GPIO_QRD3_LCD_BRDG_RESET_N, 0);
                }
        }

	/* reduce ref count of ext power */
	if (cont_splash_enabled && !cont_splash_done) {
		qrd_lcd_splash_power_vote(0);
		cont_splash_done = 1;
	}

	return rc;
}

#ifdef CONFIG_CELLON_PRJ_C8681
static int mipi_dsi_panel_c8681_power(int on)
{
	int rc = 0;

	printk("%s: on = %d\n", __func__, on);

	rc = gpio_request(GPIO_C8681_LCD_RESET_N, "c8680_lcdc_reset_n");
	if (rc < 0) {
		pr_err("failed to request c8680 lcd reset_n\n");
		return rc;
	}

	rc = gpio_tlmm_config(GPIO_CFG(GPIO_C8681_LCD_RESET_N, 0,
			GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("Failed to enable LCD reset\n");
		gpio_free(GPIO_LCDC_BRDG_RESET_N);
		return rc;
	}

	rc = gpio_direction_output(GPIO_C8681_LCD_RESET_N, 1);
	if (rc < 0) {
		pr_err("Failed to set reset invalid\n");
		return rc;
	}

	if (on) {
		gpio_set_value_cansleep(GPIO_C8681_LCD_RESET_N, 1);
		udelay(20);
		gpio_set_value_cansleep(GPIO_C8681_LCD_RESET_N, 0);
		udelay(20);
		gpio_set_value_cansleep(GPIO_C8681_LCD_RESET_N, 1);
	}

	gpio_free(GPIO_C8681_LCD_RESET_N);

	rc = gpio_request(GPIO_C8681_LCD_MODESEL, "c8681_lcd_cmd_pin");
	if (rc < 0) {
		pr_err("failed to request c8681 lcd cmd pin\n");
		return rc;
	}

	rc = gpio_tlmm_config(GPIO_CFG(GPIO_C8681_LCD_MODESEL, 0,
			GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("Failed to enable c8681 lcd cmd pin\n");
		gpio_free(GPIO_C8681_LCD_MODESEL);
		return rc;
	}

	rc = gpio_direction_output(GPIO_C8681_LCD_MODESEL, 1);
	if (rc < 0) {
		pr_err("Failed to set reset invalid\n");
		return rc;
	}

	gpio_free(GPIO_C8681_LCD_MODESEL);

	return rc;
}
#else
static int mipi_dsi_panel_c8680_power(int on)
{
	int rc = 0;

	printk("%s: on = %d\n", __func__, on);

	rc = gpio_request(GPIO_C8680_LCD_RESET_N, "c8680_lcdc_reset_n");
	if (rc < 0) {
		pr_err("failed to request c8680 lcd reset_n\n");
		return rc;
	}

	rc = gpio_tlmm_config(GPIO_CFG(GPIO_C8680_LCD_RESET_N, 0,
			GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("Failed to enable LCD reset\n");
		gpio_free(GPIO_LCDC_BRDG_RESET_N);
		return rc;
	}

	rc = gpio_direction_output(GPIO_C8680_LCD_RESET_N, 1);
	if (rc < 0) {
		pr_err("Failed to set reset invalid\n");
		return rc;
	}

	if (on) {
		//both sharp and truly have the same reset timing
		gpio_set_value_cansleep(GPIO_C8680_LCD_RESET_N, 0);
		udelay(50);
		gpio_set_value_cansleep(GPIO_C8680_LCD_RESET_N, 1);
		msleep(20);
	}
	else
	{	
		//must reset IC OTM9608A, otherwise, the phone will waste 3mA when sleep.
		rc = gpio_direction_output(GPIO_C8680_LCD_RESET_N, 0);
		if (rc < 0) {
			pr_err("Failed to set reset invalid\n");
			return rc;
		}
	}

	gpio_free(GPIO_C8680_LCD_RESET_N);

	return rc;
}
#endif

static int mipi_dsi_panel_cellon_power(int on)
{
	int rc = 0;

#ifdef CONFIG_CELLON_PRJ_C8681
	rc = mipi_dsi_panel_c8681_power(on);
#else
	rc = mipi_dsi_panel_c8680_power(on);
#endif

	return rc;
}

static int mipi_dsi_panel_power(int on)
{
	int rc = 0;

	if (machine_is_msm8225_cellon())
		rc = mipi_dsi_panel_cellon_power(on);
	else if (machine_is_msm7627a_qrd1())
		rc = mipi_dsi_panel_qrd1_power(on);
	else if (machine_is_msm7627a_evb() || machine_is_msm8625_evb() || 
			machine_is_msm8625_qrd5() || machine_is_msm7x27a_qrd5a() || 
			machine_is_msm8625_evt())
		rc = mipi_dsi_panel_qrd3_power(on);
	else if(machine_is_msm8625_skua())
		rc = mipi_dsi_panel_skua_power(on);
	else
		rc = mipi_dsi_panel_msm_power(on);
	return rc;
}
#endif
#define MDP_303_VSYNC_GPIO 97

static char mipi_dsi_splash_is_enabled(void) {
    return mdp_pdata.cont_splash_enabled;
}

#ifdef CONFIG_FB_MSM_MIPI_DSI
static struct mipi_dsi_platform_data mipi_dsi_pdata = {
	.vsync_gpio		= MDP_303_VSYNC_GPIO,
	.dsi_power_save		= mipi_dsi_panel_power,
	.dsi_client_reset       = msm_fb_dsi_client_reset,
	.get_lane_config	= msm_fb_get_lane_config,
    .splash_is_enabled  = mipi_dsi_splash_is_enabled,
};
#endif

static char prim_panel_name[PANEL_NAME_MAX_LEN];
static int __init prim_display_setup(char *param)
{
	if (strnlen(param, PANEL_NAME_MAX_LEN))
		strlcpy(prim_panel_name, param, PANEL_NAME_MAX_LEN);
	return 0;
}
early_param("prim_display", prim_display_setup);

void msm7x27a_set_display_params(char *prim_panel)
{
	if (strnlen(prim_panel, PANEL_NAME_MAX_LEN)) {
		strlcpy(msm_fb_pdata.prim_panel_name, prim_panel,
			PANEL_NAME_MAX_LEN);
		pr_debug("msm_fb_pdata.prim_panel_name %s\n",
			msm_fb_pdata.prim_panel_name);
	}
}

void __init msm_fb_add_devices(void)
{
	/* Using continuous splash or not */
	if (machine_is_msm8625_qrd7() || machine_is_msm8625_evb() || machine_is_msm8625_qrd5()) {
            if (cont_splash_enabled) {
                    /* increase ref count of ext power */
                    qrd_lcd_splash_power_vote(1);
                    mdp_pdata.cont_splash_enabled = 1;
                    /* FIXME: Need these flags to indicate backlight driver the initial backlight level */
                    mipi_NT35510_pdata.cont_splash_enabled = 1;
                    mipi_NT35510_alaska_pdata.cont_splash_enabled = 1;
                    mipi_himax_pdata.cont_splash_enabled = 1;
            }
    }

	/* default is NT35510 */
	if (machine_is_msm8625_skua() && skua_panel_is_himax()) {
		skua_fb_devices[1] = &mipi_dsi_himax_panel_device;
		skua_backlight_device.dev.platform_data = &mipi_himax_pdata;
	}

	msm7x27a_set_display_params(prim_panel_name);
	if (machine_is_msm8225_cellon())
		platform_add_devices(c8680_fb_devices,
				ARRAY_SIZE(c8680_fb_devices));
	else if (machine_is_msm7627a_qrd1())
		platform_add_devices(qrd_fb_devices,
				ARRAY_SIZE(qrd_fb_devices));
	else if (machine_is_msm7627a_evb() || machine_is_msm8625_evb() || 
			machine_is_msm8625_qrd5() || machine_is_msm7x27a_qrd5a() || 
			machine_is_msm8625_evt()) {
		platform_add_devices(evb_fb_devices,
				ARRAY_SIZE(evb_fb_devices));
	} else if (machine_is_msm8625_skua())
		platform_add_devices(skua_fb_devices,
				ARRAY_SIZE(skua_fb_devices));
	else if (machine_is_msm7627a_qrd3() || machine_is_msm8625_qrd7()) {
		platform_add_devices(qrd3_fb_devices,
						ARRAY_SIZE(qrd3_fb_devices));
	} else
		platform_add_devices(msm_fb_devices,
				ARRAY_SIZE(msm_fb_devices));

	msm_fb_register_device("mdp", &mdp_pdata);
	if (machine_is_msm7625a_surf() || machine_is_msm7x27a_surf()
		|| machine_is_msm8625_surf() || machine_is_msm7627a_qrd3()
		|| machine_is_msm8625_qrd7())
		msm_fb_register_device("lcdc", &lcdc_pdata);
#ifdef CONFIG_FB_MSM_MIPI_DSI
	msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
#endif
}
