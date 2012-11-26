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
#include "mipi_NT35516.h"

static struct msm_panel_common_pdata *mipi_nt35516_pdata;
static struct dsi_buf nt35516_tx_buf;
static struct dsi_buf nt35516_rx_buf;
spinlock_t nt35516_spin_lock;

/* common setting */
static char exit_sleep[2] = {0x11, 0x00};
static char display_on[2] = {0x29, 0x00};
static char display_off[2] = {0x28, 0x00};
static char enter_sleep[2] = {0x10, 0x00};

static char write_ram[2] = {0x2c, 0x00};
static char normal_mode[2] = {0x13, 0x00};

static char cmd0[2] = {
	0x51, 0x7F
};
static char cmd1[2] = {
	0x53, 0x2F
};
static char cmd2[2] = {
	0x55, 0x02,
};
static char cmd3[2] = {
	0x36, 0xd4,
};
static char cmd5[3] = {
	0xE0, 0x00, 0x01
};
static char cmd6[2] = {
	0xd0, 0x01
};
static char cmd55[] = {
	0XFF,0XAA,0X55,0X25,
	0X01,
};
static char cmd7[] = {
	0xF2,0x00,0x00,0x4A,
	0x0A,0xA8,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x0B,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x40,0x01,0x51,
	0x00,0x01,0x00,0x01
};
static char cmd8[] = {
	0xF3,0x02,0x03,0x07,
	0x45,0x88,0xD1,0x0D
};
static char cmd9[] = {
	0xF0,0x55,0xAA,0x52,
	0x08,0x00
};
static char cmd10[] = {
	0xB1,0xeC,0x00,0x00
};
static char cmd11[] = {
	0xB8,0x01,0x02,0x02,
	0x02,
};
static char cmd12[] = {
	0xBC,0x00,0x00,0x00
};
static char cmd13[] = {
	0xC9,0x63,0x06,0x0D,
	0x1A,0x17,0x00
};
static char cmd14[] = {
	0xF0,0x55,0xAA,0x52,
	0x08,0x01
};
static char cmd15[] = {
	0xB0,0x05,0x05,0x05
};
static char cmd16[] = {
	0xB1,0x05,0x05,0x05
};
static char cmd17[] = {
	0xB2,0x01,0x01,0x01
};
static char cmd18[] = {
	0xB3,0x0C,0x0C,0x0C
};
static char cmd19[] = {
	0xB4,0x09,0x09,0x09
};
static char cmd20[] = {
	0xB6,0x44,0x44,0x44
};
static char cmd21[] = {
	0xB7,0x34,0x34,0x34
};
static char cmd22[] = {
	0xB8,0x10,0x10,0x10
};
static char cmd23[] = {
	0xB9,0x26,0x26,0x26
};
static char cmd24[] = {
	0xBA,0x34,0x34,0x34
};
static char cmd25[] = {
	0xBC,0x00,0xC8,0x00
};
static char cmd26[] = {
	0xBD,0x00,0xC8,0x00
};
static char cmd27[] = {
	0xBE,0x7c
};
static char cmd28[] = {
	0xC0,0x04,0x00
};
static char cmd29[] = {
	0xCA,0x00
};
static char cmd30[] = {
	0xD0,0x0A,0x10,0x0D,
	0x0F,
};
static char cmd31[] = {
	0xD1,0x00,0x70,0x00,
	0xCE,0x00,0xF7,0x01,
	0x10,0x01,0x21,0x01,
	0x44,0x01,0x62,0x01,
	0x8D,
};
static char cmd32[] = {
	0xD2,0x01,0xAF,0x01,
	0xE4,0x02,0x0C,0x02,
	0x4D,0x02,0x82,0x02,
	0x84,0x02,0xB8,0x02,
	0xF0,
};
static char cmd33[] = {
	0xD3,0x03,0x14,0x03,
	0x42,0x03,0x5E,0x03,
	0x80,0x03,0x97,0x03,
	0xB0,0x03,0xC0,0x03,
	0xDF,
};
static char cmd34[] = {
	0xD4,0x03,0xFD,0x03,
	0xFF,
};
static char cmd35[] = {
	0xD5,0x00,0x70,0x00,
	0xCE,0x00,0xF7,0x01,
	0x10,0x01,0x21,0x01,
	0x44,0x01,0x62,0x01,
	0x8D,
};
static char cmd36[] = {
	0xD6,0x01,0xAF,0x01,
	0xE4,0x02,0x0C,0x02,
	0x4D,0x02,0x82,0x02,
	0x84,0x02,0xB8,0x02,
	0xF0
};
static char cmd37[] = {
	0xD7,0x03,0x14,0x03,
	0x42,0x03,0x5E,0x03,
	0x80,0x03,0x97,0x03,
	0xB0,0x03,0xC0,0x03,
	0xDF
};
static char cmd38[] = {
	0xD8,0x03,0xFD,0x03,
	0xFF
};
static char cmd39[] = {
	0xD9,0x00,0x70,0x00,
	0xCE,0x00,0xF7,0x01,
	0x10,0x01,0x21,0x01,
	0x44,0x01,0x62,0x01,
	0x8D
};
static char cmd40[] = {
	0xDD,0x01,0xAF,0x01,
	0xE4,0x02,0x0C,0x02,
	0x4D,0x02,0x82,0x02,
	0x84,0x02,0xB8,0x02,
	0xF0
};
static char cmd41[] = {
	0xDE,0x03,0x14,0x03,
	0x42,0x03,0x5E,0x03,
	0x80,0x03,0x97,0x03,
	0xB0,0x03,0xC0,0x03,
	0xDF
};
static char cmd42[] = {
	0xDF,0x03,0xFD,0x03,
	0xFF
};
static char cmd43[] = {
	0xE0,0x00,0x70,0x00,
	0xCE,0x00,0xF7,0x01,
	0x10,0x01,0x21,0x01,
	0x44,0x01,0x62,0x01,
	0x8D
};
static char cmd44[] = {
	0xE1,0x01,0xAF,0x01,
	0xE4,0x02,0x0C,0x02,
	0x4D,0x02,0x82,0x02,
	0x84,0x02,0xB8,0x02,
	0xF0
};
static char cmd45[] = {
	0xE2,0x03,0x14,0x03,
	0x42,0x03,0x5E,0x03,
	0x80,0x03,0x97,0x03,
	0xB0,0x03,0xC0,0x03,
	0xDF
};
static char cmd46[] = {
	0xE3,0x03,0xFD,0x03,
	0xFF
};
static char cmd47[] = {
	0xE4,0x00,0x70,0x00,
	0xCE,0x00,0xF7,0x01,
	0x10,0x01,0x21,0x01,
	0x44,0x01,0x62,0x01,
	0x8D
};
static char cmd48[] = {
	0xE5,0x01,0xAF,0x01,
	0xE4,0x02,0x0C,0x02,
	0x4D,0x02,0x82,0x02,
	0x84,0x02,0xB8,0x02,
	0xF0
};
static char cmd49[] = {
	0xE6,0x03,0x14,0x03,
	0x42,0x03,0x5E,0x03,
	0x80,0x03,0x97,0x03,
	0xB0,0x03,0xC0,0x03,
	0xDF
};
static char cmd50[] = {
	0xE7,0x03,0xFD,0x03,
	0xFF
};
static char cmd51[] = {
	0xE8,0x00,0x70,0x00,
	0xCE,0x00,0xF7,0x01,
	0x10,0x01,0x21,0x01,
	0x44,0x01,0x62,0x01,
	0x8D
};
static char cmd52[] = {
	0xE9,0x01,0xAF,0x01,
	0xE4,0x02,0x0C,0x02,
	0x4D,0x02,0x82,0x02,
	0x84,0x02,0xB8,0x02,
	0xF0
};
static char cmd53[] = {
	0xEA,0x03,0x14,0x03,
	0x42,0x03,0x5E,0x03,
	0x80,0x03,0x97,0x03,
	0xB0,0x03,0xC0,0x03,
	0xDF
};
static char cmd54[] = {
	0xEB,0x03,0xFD,0x03,
	0xFF
};

static struct dsi_cmd_desc nt35516_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(enter_sleep), enter_sleep}
};

static struct dsi_cmd_desc nt35516_cmd_display_on_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd0), cmd0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd1), cmd1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd2), cmd2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd3), cmd3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd55), cmd55},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd7), cmd7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd8), cmd8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd9), cmd9},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd10), cmd10},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd11), cmd11},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd12), cmd12},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd13), cmd13},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd5), cmd5},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd6), cmd6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd14), cmd14},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd15), cmd15},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd16), cmd16},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd17), cmd17},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd18), cmd18},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd19), cmd19},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd20), cmd20},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd21), cmd21},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd22), cmd22},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd23), cmd23},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd24), cmd24},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd25), cmd25},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd26), cmd26},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd27), cmd27},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd28), cmd28},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd29), cmd29},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd30), cmd30},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd31), cmd31},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd32), cmd32},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd33), cmd33},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd34), cmd34},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd35), cmd35},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd36), cmd36},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd37), cmd37},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd38), cmd38},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd39), cmd39},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd40), cmd40},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd41), cmd41},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd42), cmd42},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd43), cmd43},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd44), cmd44},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd45), cmd45},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd46), cmd46},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd47), cmd47},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd48), cmd48},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd49), cmd49},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd50), cmd50},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd51), cmd51},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd52), cmd52},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd53), cmd53},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd54), cmd54},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(write_ram), write_ram},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(normal_mode), normal_mode},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
};

static int mipi_nt35516_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;

	pr_debug("mipi_nt35516_lcd_on E\n");
	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;

	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi  = &mfd->panel_info.mipi;

	mipi_dsi_cmds_tx(mfd, &nt35516_tx_buf,
		nt35516_cmd_display_on_cmds,
		ARRAY_SIZE(nt35516_cmd_display_on_cmds));

	pr_debug("mipi_nt35516_lcd_on X\n");

	return 0;
}

static int mipi_nt35516_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	pr_debug("mipi_nt35516_lcd_off E\n");

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi_dsi_cmds_tx(mfd, &nt35516_tx_buf, nt35516_display_off_cmds,
			ARRAY_SIZE(nt35516_display_off_cmds));

	pr_debug("mipi_nt35516_lcd_off X\n");
	return 0;
}

static int __devinit mipi_nt35516_lcd_probe(struct platform_device *pdev)
{
	pr_debug("%s\n", __func__);

	if (pdev->id == 0) {
		mipi_nt35516_pdata = pdev->dev.platform_data;
		return 0;
	}

	spin_lock_init(&nt35516_spin_lock);
	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_nt35516_lcd_probe,
	.driver = {
		.name   = "mipi_NT35516",
	},
};

#define MAX_BL_LEVEL 32
#define LCD_BL_EN 96

static void mipi_nt35516_set_backlight(struct msm_fb_data_type *mfd)
{
	int level = mfd->bl_level;
	int max = mfd->panel_info.bl_max;
	int min = mfd->panel_info.bl_min;
	unsigned long flags;
	int i;

	pr_debug("%s, level = %d\n", __func__, level);
	spin_lock_irqsave(&nt35516_spin_lock, flags); //disable local irq and preemption
	if (level < min)
		level = min;
	if (level > max)
		level = max;

	if (level == 0) {
		gpio_set_value(LCD_BL_EN, 0);
		spin_unlock_irqrestore(&nt35516_spin_lock, flags);
		return;
	}

	for (i = 0; i < (MAX_BL_LEVEL - level + 1); i++) {
		gpio_set_value(LCD_BL_EN, 0);
		udelay(1);
		gpio_set_value(LCD_BL_EN, 1);
		udelay(1);
	}
	spin_unlock_irqrestore(&nt35516_spin_lock, flags);

	return;
}

static struct msm_fb_panel_data nt35516_panel_data = {
	.on	= mipi_nt35516_lcd_on,
	.off = mipi_nt35516_lcd_off,
	.set_backlight = mipi_nt35516_set_backlight,
};

static int ch_used[3];

static int mipi_nt35516_lcd_init(void)
{
	mipi_dsi_buf_alloc(&nt35516_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&nt35516_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}

int mipi_nt35516_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	ret = mipi_nt35516_lcd_init();
	if (ret) {
		pr_err("mipi_nt35516_lcd_init() failed with ret %u\n", ret);
		return ret;
	}

	pdev = platform_device_alloc("mipi_NT35516", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	nt35516_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &nt35516_panel_data,
				sizeof(nt35516_panel_data));
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
