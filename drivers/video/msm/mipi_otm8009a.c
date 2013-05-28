/*
 * Copyright (c) 2012, Code Aurora Forum. All rights reserved.
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

#include <mach/gpio.h>
#include <linux/delay.h>
#include <linux/device.h>

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "msm_fb_panel.h"
#include "mipi_otm8009a.h"

static struct msm_panel_common_pdata *mipi_otm8009a_pdata;
static struct dsi_buf otm8009a_tx_buf;
static struct dsi_buf otm8009a_rx_buf;

static char cmd0[] = {
	0x00, 0x00,
};
static char cmd1[] = {
	0xff, 0x80, 0x09, 0x01,
};
static char cmd2[] = {
	0x00, 0x80,
};
static char cmd3[] = {
	0xff, 0x80, 0x09,
};
static char cmd4[]={
    0x11,
};
static char cmd5[] = {
	0x00, 0x80,
};
static char cmd6[] = {
	0xf5, 0x01, 0x18, 0x02,
	0x18, 0x10, 0x18, 0x02,
	0x18, 0x0e, 0x18, 0x0f,
	0x20,
};
static char cmd7[] = {
	0x00, 0x90,
};
static char cmd8[] = {
	0xf5, 0x02, 0x18, 0x08,
	0x18, 0x06, 0x18, 0x0d,
	0x18, 0x0b, 0x18,
};
static char cmd9[] = {
	0x00, 0xa0,
};
static char cmd10[] = {
	0xf5, 0x10, 0x18, 0x01,
	0x18, 0x14, 0x18, 0x14,
	0x18,
};
static char cmd11[] = {
	0x00, 0xb0,
};
static char cmd12[] = {
	0xf5, 0x14, 0x18, 0x12,
	0x18, 0x13, 0x18, 0x11,
	0x18, 0x13, 0x18, 0x00,
	0x00,
};
static char cmd13[] = {
	0x00, 0xb4,
};
static char cmd14[] = {
	0xc0, 0x11,
};
static char cmd15[] = {
	0x00, 0x82,
};
static char cmd16[] = {
	0xc5, 0xa3,
};
static char cmd17[] = {
	0x00, 0x90,
};
static char cmd18[] = {
	0xc5, 0xd6, 0x76,
};
static char cmd19[] = {
	0x00, 0x00,
};
static char cmd20[] = {
	0xd8, 0xa7, 0xa7,
};
static char cmd21[] = {
	0x00, 0x00,
};
static char cmd22[] = {
	0xd9, 0x74,
};
static char cmd23[] = {
	0x00, 0x81,
};
static char cmd24[] = {
	0xc1, 0x66,
};
static char cmd25[] = {
	0x00, 0xa1,
};
static char cmd26[] = {
	0xc1, 0x08,
};
static char cmd27[] = {
	0x00, 0xa3,
};
static char cmd28[] = {
	0xc0, 0x1b,
};
static char cmd29[] = {
	0x00, 0x81,
};
static char cmd30[] = {
	0xc4, 0x83,
};
static char cmd31[] = {
	0x00, 0x92,
};
static char cmd32[] = {
	0xc5, 0x01,
};
static char cmd33[] = {
	0x00, 0xb1,
};
static char cmd34[] = {
	0xc5, 0xa9,
};
static char cmd35[] = {
	0x00, 0x80,
};
static char cmd36[] = {
	0xce, 0x84, 0x03, 0x00,
	0x83, 0x03, 0x00,
};
static char cmd37[] = {
	0x00, 0x90,
};
static char cmd38[] = {
	0xce, 0x33, 0x27, 0x00,
	0x33, 0x28, 0x00,
};
static char cmd39[] = {
	0x00, 0xa0,
};
static char cmd40[] = {
	0xce, 0x38, 0x02, 0x03,
	0x21, 0x00, 0x00, 0x00,
	0x38, 0x01, 0x03, 0x22,
	0x00, 0x00, 0x00,
};
static char cmd41[] = {
	0x00, 0xb0,
};
static char cmd42[] = {
	0xce, 0x38, 0x00, 0x03,
	0x23, 0x00, 0x00, 0x00,
	0x30, 0x00, 0x03, 0x24,
	0x00, 0x00, 0x00,
};
static char cmd43[] = {
	0x00, 0xc0,
};
static char cmd44[] = {
	0xce, 0x30, 0x01, 0x03,
	0x25, 0x00, 0x00, 0x00,
	0x30, 0x02, 0x03, 0x26,
	0x00, 0x00, 0x00,
};
static char cmd45[] = {
	0x00, 0xd0,
};
static char cmd46[] = {
	0xce, 0x30, 0x03, 0x03,
	0x27, 0x00, 0x00, 0x00,
	0x30, 0x04, 0x03, 0x28,
	0x00, 0x00, 0x00,
};
static char cmd47[] = {
	0x00, 0xc7,
};
static char cmd48[] = {
	0xcf, 0x00,
};
static char cmd49[] = {
	0x00, 0xc0,
};
static char cmd50[] = {
	0xcb, 0x00, 0x00, 0x00,
	0x00, 0x54, 0x54, 0x54,
	0x54, 0x00, 0x54, 0x00,
	0x54, 0x00, 0x00, 0x00,
};
static char cmd51[] = {
	0x00, 0xd0,
};
static char cmd52[] = {
	0xcb, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x54, 0x54,
	0x54, 0x54, 0x00, 0x54,
};
static char cmd53[] = {
	0x00, 0xe0,
};
static char cmd54[] = {
	0xcb, 0x00, 0x54, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00,
};
static char cmd55[] = {
	0x00, 0x80,
};
static char cmd56[] = {
	0xcc, 0x00, 0x00, 0x00,
	0x00, 0x0c, 0x0a, 0x10,
	0x0e, 0x00, 0x02,
};
static char cmd57[] = {
	0x00, 0x90,
};
static char cmd58[] = {
	0xcc, 0x00, 0x06, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x0b,
};
static char cmd59[] = {
	0x00, 0xa0,
};
static char cmd60[] = {
	0xcc, 0x09, 0x0f, 0x0d,
	0x00, 0x01, 0x00, 0x05,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
};
static char cmd61[] = {
	0x00, 0xb0,
};
static char cmd62[] = {
	0xcc, 0x00, 0x00, 0x00,
	0x00, 0x0d, 0x0f, 0x09,
	0x0b, 0x00, 0x05,
};
static char cmd63[] = {
	0x00, 0xc0,
};
static char cmd64[] = {
	0xcc, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x0e,
};
static char cmd65[] = {
	0x00, 0x00,
};
static char cmd66[] = {
	0x36, 0x40,
};
static char cmd67[] = {
	0x00, 0xd0,
};
static char cmd68[] = {
	0xcc, 0x10, 0x0a, 0x0c,
	0x00, 0x06, 0x00, 0x02,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
};
static char cmd69[] = {
	0x00, 0x00,
};
static char cmd70[] = {
	0xe1, 0x00, 0x0a, 0x11,
	0x0e, 0x07, 0x0f, 0x0b,
	0x0b, 0x04, 0x07, 0x0d,
	0x09, 0x10, 0x0f, 0x08,
	0x03,
};
static char cmd71[] = {
	0x00, 0x00,
};
static char cmd72[] = {
	0xe2, 0x00, 0x0a, 0x11,
	0x0b, 0x04, 0x07, 0x0d,
	0x09, 0x10, 0x0f, 0x08,
	0x03,
};
static char cmd73[] = {
	0x00, 0x00,
};
static char cmd74[] = {
	0x26, 0x00,
};
static char cmd75[] = {
	0x00, 0x00,
};
static char cmd76[] = {
	0x2b, 0x00, 0x00, 0x03,
	0x56,
};
static char cmd77[] = {
	0x00, 0x00,
};
static char cmd78[] = {
	0x29,
};
static char display_bringtness[2] = {
	0x51, 0x80,
};
static char crtl_display[2] = {
	0x53, 0x2c,
};
static char cabc[2] = {
	0x55, 0x02,
};

static struct dsi_cmd_desc otm8009a_cmd_display_on_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd0), cmd0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd1), cmd1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd2), cmd2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd3), cmd3},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(cmd4), cmd4},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd5), cmd5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd6), cmd6},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd7), cmd7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd8), cmd8},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd9), cmd9},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd10), cmd10},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd11), cmd11},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd12), cmd12},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd13), cmd13},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd14), cmd14},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd15), cmd15},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd16), cmd16},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd17), cmd17},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd18), cmd18},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd19), cmd19},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd20), cmd20},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd21), cmd21},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd22), cmd22},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd23), cmd23},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd24), cmd24},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd25), cmd25},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd26), cmd26},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd27), cmd27},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd28), cmd28},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd29), cmd29},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd30), cmd30},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd31), cmd31},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd32), cmd32},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd33), cmd33},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd34), cmd34},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd35), cmd35},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd36), cmd36},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd37), cmd37},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd38), cmd38},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd39), cmd39},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd40), cmd40},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd41), cmd41},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd42), cmd42},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd43), cmd43},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd44), cmd44},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd45), cmd45},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd46), cmd46},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd47), cmd47},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd48), cmd48},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd49), cmd49},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd50), cmd50},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd51), cmd51},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd52), cmd52},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd53), cmd53},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd54), cmd54},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd55), cmd55},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd56), cmd56},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd57), cmd57},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd58), cmd58},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd59), cmd59},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd60), cmd60},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd61), cmd61},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd62), cmd62},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd63), cmd63},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd64), cmd64},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd65), cmd65},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd66), cmd66},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd67), cmd67},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd68), cmd68},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd69), cmd69},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd70), cmd70},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd71), cmd71},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd72), cmd72},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd73), cmd73},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd74), cmd74},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd75), cmd75},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd76), cmd76},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd77), cmd77},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(cmd78), cmd78},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_bringtness), display_bringtness},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(crtl_display), crtl_display},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cabc), cabc},
};

static char display_off[2] = {0x28, 0x00};
static char enter_sleep[2] = {0x10, 0x00};

static struct dsi_cmd_desc otm8009a_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(enter_sleep), enter_sleep}
};

#define MAX_BL_LEVEL 32
#define LCD_BL_EN 0

extern int pmic_gpio_direction_output(unsigned gpio);
extern int pmic_gpio_set_value(unsigned gpio, int value);

static void mipi_otm8009a_panel_set_backlight(struct msm_fb_data_type *mfd)
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

static int mipi_otm8009a_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	printk("%s: Enter\n", __func__);

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi_dsi_cmds_tx(mfd, &otm8009a_tx_buf,
		otm8009a_cmd_display_on_cmds,
		ARRAY_SIZE(otm8009a_cmd_display_on_cmds));

	printk("%s: Done\n", __func__);
	return 0;
}

static int mipi_otm8009a_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	printk("%s: Enter\n", __func__);
	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi_dsi_cmds_tx(mfd, &otm8009a_tx_buf,
		otm8009a_display_off_cmds,
		ARRAY_SIZE(otm8009a_display_off_cmds));

	printk("%s: Done\n", __func__);
	return 0;
}

static int __devinit mipi_otm8009a_lcd_probe(struct platform_device *pdev)
{
	printk("%s: Enter\n", __func__);
	if (pdev->id == 0) {
		mipi_otm8009a_pdata = pdev->dev.platform_data;
		return 0;
	}
	msm_fb_add_device(pdev);

	printk("%s: Done\n", __func__);
	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_otm8009a_lcd_probe,
	.driver = {
		.name   = "mipi_otm8009a",
	},
};

static struct msm_fb_panel_data otm8009a_panel_data = {
	.on    = mipi_otm8009a_lcd_on,
	.off    = mipi_otm8009a_lcd_off,
	.set_backlight    = mipi_otm8009a_panel_set_backlight,
};

static int ch_used[3];

int mipi_otm8009a_device_register(struct msm_panel_info *pinfo,
                    u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	printk("%s\n", __func__);

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_otm8009a", (panel << 8)|channel);
	if (!pdev)
	return -ENOMEM;

	otm8009a_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &otm8009a_panel_data, sizeof(otm8009a_panel_data));
	if (ret) {
		printk(KERN_ERR"%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		printk(KERN_ERR"%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

	err_device_put:
	platform_device_put(pdev);
	return ret;
}

static int __init mipi_otm8009a_lcd_init(void)
{
	printk("%s\n", __func__);

	mipi_dsi_buf_alloc(&otm8009a_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&otm8009a_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}

module_init(mipi_otm8009a_lcd_init);

