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
#include <linux/gpio.h>
#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_NT35510_boe.h"

static struct msm_panel_common_pdata *mipi_nt35510_boe_pdata;
static struct dsi_buf nt35510_boe_tx_buf;
static struct dsi_buf nt35510_boe_rx_buf;
spinlock_t nt35510_boe_spin_lock;

/* common setting */
static char exit_sleep[2] = {0x11, 0x00};
static char display_on[2] = {0x29, 0x00};
static char display_off[2] = {0x28, 0x00};
static char enter_sleep[2] = {0x10, 0x00};
static char write_ram[2] = {0x2c, 0x00}; /* write ram */

static struct dsi_cmd_desc nt35510_boe_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(enter_sleep), enter_sleep}
};

static char cmd0[] = {
	0xF0, 0x55, 0xAA, 0x52,
	0x08, 0x01,
};
static char cmd1[] = {
	0xB0, 0x09,
};
static char cmd2[] = {
	0xB6, 0x44,
};
static char cmd3[] = {
	0xB1, 0x09,
};
static char cmd4[] = {
	0xB7, 0x34,
};
static char cmd5[] = {
	0xC2, 0x01,
};
static char cmd6[] = {
	0xB3, 0x05,
};
static char cmd7[] = {
	0xB3, 0x05,
};
static char cmd8[] = {
	0xB9, 0x24,
};
static char cmd9[] = {
	0xB5, 0x0B, 0x0B, 0x0B,
};
static char cmd10[] = {
	0xBA, 0x34,
};
static char cmd11[] = {
	0xBC, 0x00, 0x09, 0x00,
};
static char cmd12[] = {
	0xBD, 0x00, 0x90,
};
static char cmd13[] = {
	0xD1, 0x00, 0x01, 0x00,
	0x40, 0x00, 0x70, 0x00,
	0x99, 0x00, 0xb2, 0x00,
	0xda, 0x01, 0x00, 0x01,
	0x32, 0x01, 0x53, 0x01,
	0x8a, 0x01, 0xc1, 0x02,
	0x11, 0x02, 0x52, 0x02,
	0x53, 0x02, 0x8f, 0x02,
	0xcb, 0x02, 0xf6, 0x03,
	0x22, 0x03, 0x53, 0x03,
	0x79, 0x03, 0x9c, 0x03,
	0xc5, 0x03, 0xd8, 0x03,
	0xed, 0x03, 0xf8, 0x03,
	0xfb,

};
static char cmd14[] = {
	0xD2, 0x00, 0x01, 0x00,
	0x40, 0x00, 0x70, 0x00,
	0x99, 0x00, 0xb2, 0x00,
	0xda, 0x01, 0x00, 0x01,
	0x32, 0x01, 0x53, 0x01,
	0x8a, 0x01, 0xc1, 0x02,
	0x11, 0x02, 0x52, 0x02,
	0x53, 0x02, 0x8f, 0x02,
	0xcb, 0x02, 0xf6, 0x03,
	0x22, 0x03, 0x53, 0x03,
	0x79, 0x03, 0x9c, 0x03,
	0xc5, 0x03, 0xd8, 0x03,
	0xed, 0x03, 0xf8, 0x03,
	0xfb,
};
static char cmd15[] = {
	0xD3, 0x00, 0x01, 0x00,
	0x40, 0x00, 0x70, 0x00,
	0x99, 0x00, 0xb2, 0x00,
	0xda, 0x01, 0x00, 0x01,
	0x32, 0x01, 0x53, 0x01,
	0x8a, 0x01, 0xc1, 0x02,
	0x11, 0x02, 0x52, 0x02,
	0x53, 0x02, 0x8f, 0x02,
	0xcb, 0x02, 0xf6, 0x03,
	0x22, 0x03, 0x53, 0x03,
	0x79, 0x03, 0x9c, 0x03,
	0xc5, 0x03, 0xd8, 0x03,
	0xed, 0x03, 0xf8, 0x03,
	0xfb,
};
static char cmd16[] = {
	0xD4, 0x00, 0x01, 0x00,
	0x40, 0x00, 0x70, 0x00,
	0x99, 0x00, 0xb2, 0x00,
	0xda, 0x01, 0x00, 0x01,
	0x32, 0x01, 0x53, 0x01,
	0x8a, 0x01, 0xc1, 0x02,
	0x11, 0x02, 0x52, 0x02,
	0x53, 0x02, 0x8f, 0x02,
	0xcb, 0x02, 0xf6, 0x03,
	0x22, 0x03, 0x53, 0x03,
	0x79, 0x03, 0x9c, 0x03,
	0xc5, 0x03, 0xd8, 0x03,
	0xed, 0x03, 0xf8, 0x03,
	0xfb,
};
static char cmd17[] = {
	0xD5, 0x00, 0x01, 0x00,
	0x40, 0x00, 0x70, 0x00,
	0x99, 0x00, 0xb2, 0x00,
	0xda, 0x01, 0x00, 0x01,
	0x32, 0x01, 0x53, 0x01,
	0x8a, 0x01, 0xc1, 0x02,
	0x11, 0x02, 0x52, 0x02,
	0x53, 0x02, 0x8f, 0x02,
	0xcb, 0x02, 0xf6, 0x03,
	0x22, 0x03, 0x53, 0x03,
	0x79, 0x03, 0x9c, 0x03,
	0xc5, 0x03, 0xd8, 0x03,
	0xed, 0x03, 0xf8, 0x03,
	0xfb,
};
static char cmd18[] = {
	0xD6, 0x00, 0x01, 0x00,
	0x40, 0x00, 0x70, 0x00,
	0x99, 0x00, 0xb2, 0x00,
	0xda, 0x01, 0x00, 0x01,
	0x32, 0x01, 0x53, 0x01,
	0x8a, 0x01, 0xc1, 0x02,
	0x11, 0x02, 0x52, 0x02,
	0x53, 0x02, 0x8f, 0x02,
	0xcb, 0x02, 0xf6, 0x03,
	0x22, 0x03, 0x53, 0x03,
	0x79, 0x03, 0x9c, 0x03,
	0xc5, 0x03, 0xd8, 0x03,
	0xed, 0x03, 0xf8, 0x03,
	0xfb,
};

static char cmd19[] = {
	0xF0, 0x55, 0xAA, 0x52,
	0x08, 0x00,
};

static char cmd20[] = {
	0xBC, 0x00, 0x00, 0x00,
};

static char cmd21[] = {
	0xB6, 0x0a,
};
static char cmd22[] = {
	0xB7, 0x00, 0x00,
};
static char cmd23[] = {
	0xB8, 0x01, 0x05, 0x05,
	0x05,
};
static char cmd24[] = {
	0xBa, 0x01,
};

static char cmd25[] = {
	0xb1, 0xec, 0x06,
};

static char cmd26[] = {
	0xcc, 0x03, 0x00, 0x00,
};

static char cmd27[] = {
	0x3a, 0x77,
};
static char cmd28[] = {
	0x35, 0x00,
};
static char cmd29[] = {
	0x36, 0x00,
};
static char display_bringtness[] = {
	0x51, 0x80,
};
static char crtl_display[2] = {
	0x53, 0x2c,
};
static char cabc[2] = {
	0x55, 0x02,
};

static struct dsi_cmd_desc nt35510_boe_cmd_display_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd0), cmd0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd1), cmd1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd2), cmd2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd3), cmd3},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd4), cmd4},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd5), cmd5},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd6), cmd6},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd7), cmd7},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd8), cmd8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd9), cmd9},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd10), cmd10},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd11), cmd11},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd12), cmd12},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd13), cmd13},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd14), cmd14},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd15), cmd15},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd16), cmd16},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd17), cmd17},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd18), cmd18},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd19), cmd19},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd20), cmd20},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd21), cmd21},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd22), cmd22},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd23), cmd23},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd24), cmd24},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd25), cmd25},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd26), cmd26},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd27), cmd27},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd28), cmd28},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd29), cmd29},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_bringtness), display_bringtness},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(crtl_display), crtl_display},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cabc), cabc},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,	sizeof(write_ram), write_ram},
};

static int mipi_nt35510_boe_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	pr_debug("%s E\n", __func__);
	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;

	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi_dsi_cmds_tx(mfd, &nt35510_boe_tx_buf, nt35510_boe_cmd_display_on_cmds,
		ARRAY_SIZE(nt35510_boe_cmd_display_on_cmds));
	pr_debug("%s X\n", __func__);

	return 0;
}

static int mipi_nt35510_boe_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi_dsi_cmds_tx(mfd, &nt35510_boe_tx_buf, nt35510_boe_display_off_cmds,
			ARRAY_SIZE(nt35510_boe_display_off_cmds));

	return 0;
}

static int __devinit mipi_nt35510_boe_lcd_probe(struct platform_device *pdev)
{
	pr_debug("%s\n", __func__);

	if (pdev->id == 0) {
		mipi_nt35510_boe_pdata = pdev->dev.platform_data;
		return 0;
	}

	spin_lock_init(&nt35510_boe_spin_lock);
	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_nt35510_boe_lcd_probe,
	.driver = {
		.name   = "mipi_nt35510_boe",
	},
};

#define MAX_BL_LEVEL 32
#define LCD_BL_EN 0

extern int pmic_gpio_direction_output(unsigned gpio);
extern int pmic_gpio_set_value(unsigned gpio, int value);

static void mipi_nt35510_boe_set_backlight(struct msm_fb_data_type *mfd)
{
	int level = mfd->bl_level;
	int max = mfd->panel_info.bl_max;
	int min = mfd->panel_info.bl_min;

	pr_debug("%s, level = %d\n", __func__, level);
	if (level < min)
		level = min;
	if (level > max)
		level = max;

	pmic_gpio_direction_output(LCD_BL_EN);
	if (level == 0) {
		pmic_gpio_set_value(LCD_BL_EN, 0);
		return;
	} else {
		pmic_gpio_set_value(LCD_BL_EN, 1);
	}

	return;
}

static struct msm_fb_panel_data nt35510_boe_panel_data = {
	.on	= mipi_nt35510_boe_lcd_on,
	.off = mipi_nt35510_boe_lcd_off,
	.set_backlight = mipi_nt35510_boe_set_backlight,
};

static int ch_used[3];

static int mipi_nt35510_boe_lcd_init(void)
{
	mipi_dsi_buf_alloc(&nt35510_boe_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&nt35510_boe_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}
int mipi_nt35510_boe_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	ret = mipi_nt35510_boe_lcd_init();
	if (ret) {
		pr_err("mipi_nt35510_boe_lcd_init() failed with ret %u\n", ret);
		return ret;
	}

	pdev = platform_device_alloc("mipi_nt35510_boe", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	nt35510_boe_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &nt35510_boe_panel_data,
				sizeof(nt35510_boe_panel_data));
	if (ret) {
		pr_debug("%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		pr_debug("%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}
