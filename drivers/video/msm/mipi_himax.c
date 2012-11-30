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
#include "mipi_himax.h"
#include <mach/pmic.h>
#include <mach/rpc_pmapp.h>

static struct msm_panel_common_pdata *mipi_himax_pdata;
static struct dsi_buf himax_tx_buf;
static struct dsi_buf himax_rx_buf;
spinlock_t himax_spin_lock;

// set extended command set
static char password_set[4]={    
    0xB9,0xFF,0x83,0x69
};
// set power control
static char power_set[11]={
  	0xB1,0x12,0x83,0x77,
  	0x00,0x8F,0x0F,0x1A,
  	0x1A,0x0C,0x1A
};
// add for rgb interface
static char rgb[5] ={
	0xB3,0x83,0x00,0x31,
	0x03
};

// set display_set
static char display_set[16]={
    0xB2,0x00,0x20,0x05,
    0x05,0x70,0x00,0xFF,
    0x00,0x00,0x00,0x00,
    0x03,0x03,0x00,0x01
};

// set display wave
static char columm_set[2]={
    0xB4,0x00
};

// set vcom voltage
static char vcom[3] ={
	0xB6,0xA0,0xA0
};


// set gip timming control
static char cmd_set1[93]={
  	0xD5,0x00,0x00,0x12,
	0x03,0x31,0x03,0x35,
	0x12,0x01,0x20,0x12,
	0x00,0x21,0x60,0x1B,
	0x00,0x00,0x10,0x30,
	0x28,0x13,0x00,0x00,
	0x00,0xC3,0x00,0x00,
	0x00,0x00,0x03,0x00,
	0x00,0x03,0x00,0x00,
	0x04,0x00,0x00,0x89,
	0x00,0xAA,0x11,0x33,
	0x11,0x00,0x00,0x00,
	0x98,0x00,0x00,0x22,
	0x00,0x44,0x00,0x00,
	0x00,0x89,0x00,0x99,
	0x00,0x22,0x00,0x00,
	0x00,0x00,0x98,0x00,
	0x11,0x33,0x11,0x55,
	0x00,0x00,0x00,0x00,
	0x01,0x00,0x00,0x00,
	0x0F,0x00,0xCF,0xFF,
	0xFF,0x0F,0x00,0xCF,
	0xFF,0xFF,0x00,0x2A,
	0x5A
};

// set gamma
static char gamma_set[36]={
  	0xE0,0x00,0x20,0x25,
	0x3E,0x3E,0x3F,0x35,
	0x4F,0x07,0x0D,0x0E,
	0x12,0x14,0x13,0x14,
	0x0E,0x17,0x00,0x20,
	0x25,0x3E,0x3E,0x3F,
	0x32,0x4B,0x07,0x0D,
	0x0E,0x12,0x14,0x13,
	0x14,0x0E,0x17,0x00
};

// set BA
static char mipi_set[16]={
  	0xBA,0x11,0x00,0x16,
	0xC6,0x80,0x0A,0x00,
	0x10,0x24,0x02,0x21,
	0x21,0x9A,0x11,0x14
};

// set pixel format
static char cmd_set2[2]={
    	0x3A,0x077
};

// memory access control
static char cmd2[2] = {
	0x36, 0xd0,
};

// set pannel 
static char cmd3[2] = {
	0xCC,0x02
};

// set E3
static char cmd4[]={
	0xE3,0x07,0x07,0x07,
	0x07
};

// set C0
static char cmd5[7]={
	0xC0,0x73,0x50,0x00,
	0x34,0xC4,0x02
};


// sleep out
static char cmd_set6[2]={
   	 0x11,0x00
};
//display on
static char cmd_set7[2]={
    	0x29,0x00
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

static struct dsi_cmd_desc himax_cmd_on_cmds_video_mode[] = {
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(password_set), password_set},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd2), cmd2},
     {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd_set1), cmd_set1},
     {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd_set2), cmd_set2},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(power_set), power_set},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(rgb), rgb},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(display_set), display_set},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(columm_set), columm_set},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(vcom),vcom},
     {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd3), cmd3},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd4), cmd4},
   {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd5), cmd5},
   {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(mipi_set), mipi_set},
   {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set), gamma_set},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_bringtness), display_bringtness},
   {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(crtl_display), crtl_display},
   {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cabc), cabc},
   {DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(cmd_set6), cmd_set6},
   {DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(cmd_set7), cmd_set7}, 
    
};

static char display_off[2] = {0x28, 0x00};
static char enter_sleep[2] = {0x10, 0x00};

static struct dsi_cmd_desc himax_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(enter_sleep), enter_sleep}
};

static int mipi_himax_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	pr_info("%s\n", __func__);

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	msleep(20);
	mipi_dsi_cmds_tx(mfd, &himax_tx_buf, himax_cmd_on_cmds_video_mode,
			ARRAY_SIZE(himax_cmd_on_cmds_video_mode));

	return 0;
}

static int mipi_himax_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	pr_info("%s\n", __func__);

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi_dsi_cmds_tx(mfd, &himax_tx_buf, himax_display_off_cmds,
			ARRAY_SIZE(himax_display_off_cmds));

	return 0;
}

#define MAX_BL_LEVEL 32
#define LCD_BL_EN 85

extern int pmic_gpio_direction_output(unsigned gpio);
extern int pmic_gpio_set_value(unsigned gpio, int value);

static void mipi_himax_set_backlight(struct msm_fb_data_type *mfd)
{
	int level = mfd->bl_level;
	int max = mfd->panel_info.bl_max;
	int min = mfd->panel_info.bl_min;
	unsigned long flags;
	int i;

	pr_debug("%s, level = %d\n", __func__, level);
	spin_lock_irqsave(&himax_spin_lock,flags);
	if (level < min)
		level = min;
	if (level > max)
		level = max;

//	gpio_direction_output(LCD_BL_EN);
	if (level == 0) {
		gpio_set_value(LCD_BL_EN, 0);
		spin_unlock_irqrestore(&himax_spin_lock,flags);
		return;
//	} else {
//		gpio_set_value(LCD_BL_EN, 1);
	}
	for (i = 0; i < (MAX_BL_LEVEL - level + 1); i++) {
		gpio_set_value(LCD_BL_EN, 0);
		udelay(1);
		gpio_set_value(LCD_BL_EN, 1);
		udelay(1);
	}
	spin_unlock_irqrestore(&himax_spin_lock,flags);
	return;
}

static int __devinit mipi_himax_lcd_probe(struct platform_device *pdev)
{
	pr_info("%s\n", __func__);

	if (pdev->id == 0) {
		mipi_himax_pdata = pdev->dev.platform_data;
		return 0;
	}

	spin_lock_init(&himax_spin_lock);
	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_himax_lcd_probe,
	.driver = {
		.name   = "mipi_himax",
	},
};

static struct msm_fb_panel_data himax_panel_data = {
	.on		= mipi_himax_lcd_on,
	.off		= mipi_himax_lcd_off,
	.set_backlight	= mipi_himax_set_backlight,
};

static int ch_used[3];

int mipi_himax_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_himax", (panel << 8)|channel);

	if (!pdev)
		return -ENOMEM;

	himax_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &himax_panel_data,
				sizeof(himax_panel_data));
	if (ret) {
		pr_err("%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);

	if (ret) {
		pr_err("%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static int __init mipi_himax_lcd_init(void)
{
	pr_info("%s\n",__func__);
	mipi_dsi_buf_alloc(&himax_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&himax_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}

module_init(mipi_himax_lcd_init);
