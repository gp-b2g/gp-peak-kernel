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
#include "mipi_NT35565.h"

static struct msm_panel_common_pdata *mipi_nt35565_pdata;
static struct dsi_buf nt35565_tx_buf;
static struct dsi_buf nt35565_rx_buf;
spinlock_t nt35565_spin_lock;

#define NT35565_SLEEP_OFF_DELAY 150
#define NT35565_DISPLAY_ON_DELAY 150

/* common setting */
static char exit_sleep[2] = {0x11, 0x00};
static char display_on[2] = {0x29, 0x00};
static char display_off[2] = {0x28, 0x00};
static char enter_sleep[2] = {0x10, 0x00};

//unlock the register of manufacture command set CMD2
static char cmd0[2] = {
	0xf3, 0xaa,
};

static char gamma0[2] = {0x24, 0x00};
static char gamma1[2] = {0x25, 0x07};
static char gamma2[2] = {0x26, 0x13};
static char gamma3[2] = {0x27, 0x1B};
static char gamma4[2] = {0x28, 0x1D};
static char gamma5[2] = {0x29, 0x30};
static char gamma6[2] = {0x2A, 0x60};
static char gamma7[2] = {0x2B, 0x32};
static char gamma8[2] = {0x2D, 0x1E};
static char gamma9[2] = {0x2F, 0x25};
static char gamma10[2] = {0x30, 0x78};
static char gamma11[2] = {0x31, 0x13};
static char gamma12[2] = {0x32, 0x3B};
static char gamma13[2] = {0x33, 0x53};
static char gamma14[2] = {0x34, 0x94};
static char gamma15[2] = {0x35, 0xB1};
static char gamma16[2] = {0x36, 0xB4};
static char gamma17[2] = {0x37, 0x70};
static char gamma18[2] = {0x38, 0x01};
static char gamma19[2] = {0x39, 0x07};
static char gamma20[2] = {0x3A, 0x13};
static char gamma21[2] = {0x3B, 0x1C};
static char gamma22[2] = {0x3D, 0x1D};
static char gamma23[2] = {0x3F, 0x31};
static char gamma24[2] = {0x40, 0x5F};
static char gamma25[2] = {0x41, 0x32};
static char gamma26[2] = {0x42, 0x1F};
static char gamma27[2] = {0x43, 0x26};
static char gamma28[2] = {0x44, 0x77};
static char gamma29[2] = {0x45, 0x13};
static char gamma30[2] = {0x46, 0x37};
static char gamma31[2] = {0x47, 0x5D};
static char gamma32[2] = {0x48, 0x9D};
static char gamma33[2] = {0x49, 0xC3};
static char gamma34[2] = {0x4A, 0xE0};
static char gamma35[2] = {0x4B, 0x70};
static char gamma36[2] = {0x4C, 0x14};
static char gamma37[2] = {0x4D, 0x1A};
static char gamma38[2] = {0x4E, 0x28};
static char gamma39[2] = {0x4F, 0x38};
static char gamma40[2] = {0x50, 0x1B};
static char gamma41[2] = {0x51, 0x2F};
static char gamma42[2] = {0x52, 0x5F};
static char gamma43[2] = {0x53, 0x40};
static char gamma44[2] = {0x54, 0x1F};
static char gamma45[2] = {0x55, 0x24};
static char gamma46[2] = {0x56, 0x7D};
static char gamma47[2] = {0x57, 0x15};
static char gamma48[2] = {0x58, 0x3B};
static char gamma49[2] = {0x59, 0x51};
static char gamma50[2] = {0x5A, 0x8F};
static char gamma51[2] = {0x5B, 0xA9};
static char gamma52[2] = {0x5C, 0xB4};
static char gamma53[2] = {0x5D, 0x35};
static char gamma54[2] = {0x5E, 0x17};
static char gamma55[2] = {0x5F, 0x1D};
static char gamma56[2] = {0x60, 0x27};
static char gamma57[2] = {0x61, 0x38};
static char gamma58[2] = {0x62, 0x1B};
static char gamma59[2] = {0x63, 0x30};
static char gamma60[2] = {0x64, 0x61};
static char gamma61[2] = {0x65, 0x40};
static char gamma62[2] = {0x66, 0x1F};
static char gamma63[2] = {0x67, 0x27};
static char gamma64[2] = {0x68, 0x7D};
static char gamma65[2] = {0x69, 0x15};
static char gamma66[2] = {0x6A, 0x3B};
static char gamma67[2] = {0x6B, 0x51};
static char gamma68[2] = {0x6C, 0x8F};
static char gamma69[2] = {0x6D, 0xCA};
static char gamma70[2] = {0x6E, 0xE0};
static char gamma71[2] = {0x6F, 0x61};
static char gamma72[2] = {0x70, 0x3D};
static char gamma73[2] = {0x71, 0x40};
static char gamma74[2] = {0x72, 0x4F};
static char gamma75[2] = {0x73, 0x5C};
static char gamma76[2] = {0x74, 0x18};
static char gamma77[2] = {0x75, 0x2C};
static char gamma78[2] = {0x76, 0x5F};
static char gamma79[2] = {0x77, 0x52};
static char gamma80[2] = {0x78, 0x1D};
static char gamma81[2] = {0x79, 0x24};
static char gamma82[2] = {0x7A, 0x88};
static char gamma83[2] = {0x7B, 0x10};
static char gamma84[2] = {0x7C, 0x35};
static char gamma85[2] = {0x7D, 0x51};
static char gamma86[2] = {0x7E, 0x9C};
static char gamma87[2] = {0x7F, 0xA1};
static char gamma88[2] = {0x80, 0x80};
static char gamma89[2] = {0x81, 0x10};
static char gamma90[2] = {0x82, 0x43};
static char gamma91[2] = {0x83, 0x47};
static char gamma92[2] = {0x84, 0x4F};
static char gamma93[2] = {0x85, 0x5C};
static char gamma94[2] = {0x86, 0x18};
static char gamma95[2] = {0x87, 0x2C};
static char gamma96[2] = {0x88, 0x5E};
static char gamma97[2] = {0x89, 0x52};
static char gamma98[2] = {0x8A, 0x1D};
static char gamma99[2] = {0x8B, 0x24};
static char gamma100[2] = {0x8C, 0x88};
static char gamma101[2] = {0x8D, 0x0F};
static char gamma102[2] = {0x8E, 0x35};
static char gamma103[2] = {0x8F, 0x51};
static char gamma104[2] = {0x90, 0xB3};
static char gamma105[2] = {0x91, 0xCB};
static char gamma106[2] = {0x92, 0xB0};
static char gamma107[2] = {0x93, 0x30};

//change to page 1 of manufacture command set
static char cmd1[2] = {
	0x00, 0x01,
};

//set pwm pin high
static char cmd2[2] = {
	0x19, 0x82,
};

//lock the register page 1 of manufacture command set
static char cmd3[2] = {
	0x7f, 0xaa,
};

static char cmd4[2] = {
	0x53, 0x2c,
};

static char cmd5[2] = {
	0x55, 0x02,
};

static char cmd6[2] = {
	0x51, 0x80,
};

static char cmd7[2] = {
	0x36, 0xd4,
};


static char write_ram[2] = {
	0x2c, 0x00,
};

static struct dsi_cmd_desc nt35565_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(enter_sleep), enter_sleep}
};

static struct dsi_cmd_desc nt35565_cmd_display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on), display_on},

	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(cmd0), cmd0},

	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma0), gamma0},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma1), gamma1},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma2), gamma2},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma3), gamma3},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma4), gamma4},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma5), gamma5},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma6), gamma6},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma7), gamma7},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma8), gamma8},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma9), gamma9},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma10), gamma10},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma11), gamma11},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma12), gamma12},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma13), gamma13},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma14), gamma14},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma15), gamma15},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma16), gamma16},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma17), gamma17},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma18), gamma18},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma19), gamma19},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma20), gamma20},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma21), gamma21},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma22), gamma22},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma23), gamma23},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma24), gamma24},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma25), gamma25},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma26), gamma26},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma27), gamma27},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma28), gamma28},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma29), gamma29},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma30), gamma30},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma31), gamma31},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma32), gamma32},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma33), gamma33},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma34), gamma34},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma35), gamma35},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma36), gamma36},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma37), gamma37},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma38), gamma38},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma39), gamma39},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma40), gamma40},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma41), gamma41},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma42), gamma42},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma43), gamma43},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma44), gamma44},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma45), gamma45},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma46), gamma46},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma47), gamma47},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma48), gamma48},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma49), gamma49},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma50), gamma50},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma51), gamma51},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma52), gamma52},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma53), gamma53},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma54), gamma54},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma55), gamma55},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma56), gamma56},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma57), gamma57},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma58), gamma58},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma59), gamma59},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma60), gamma60},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma61), gamma61},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma62), gamma62},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma63), gamma63},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma64), gamma64},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma65), gamma65},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma66), gamma66},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma67), gamma67},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma68), gamma68},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma69), gamma69},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma70), gamma70},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma71), gamma71},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma72), gamma72},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma73), gamma73},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma74), gamma74},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma75), gamma75},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma76), gamma76},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma77), gamma77},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma78), gamma78},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma79), gamma79},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma80), gamma80},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma81), gamma81},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma82), gamma82},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma83), gamma83},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma84), gamma84},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma85), gamma85},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma86), gamma86},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma87), gamma87},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma88), gamma88},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma89), gamma89},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma90), gamma90},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma91), gamma91},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma92), gamma92},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma93), gamma93},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma94), gamma94},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma95), gamma95},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma96), gamma96},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma97), gamma97},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma98), gamma98},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma99), gamma99},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma100), gamma100},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma101), gamma101},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma102), gamma102},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma103), gamma103},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma104), gamma104},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma105), gamma105},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma106), gamma106},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(gamma107), gamma107},

	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(cmd1), cmd1},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(cmd2), cmd2},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(cmd3), cmd3},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0,	sizeof(cmd4), cmd4},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0,	sizeof(cmd5), cmd5},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0,	sizeof(cmd6), cmd6},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0,	sizeof(cmd7), cmd7},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0,	sizeof(write_ram), write_ram},
};

#if 0
static char manufacture_id[2] = {0x04, 0x00};
static struct dsi_cmd_desc nt35565_manufacture_id_cmd = {
	DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(manufacture_id), manufacture_id};

static char addr_mode[2] = {0x36, 0x00};
static struct dsi_cmd_desc nt35565_addr_mode_cmd = {
	DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(addr_mode), addr_mode};

static uint32 mipi_nt35565_manufacture_id(struct msm_fb_data_type *mfd)
{
	struct dsi_buf *rp, *tp;
	struct dsi_cmd_desc *cmd;
	uint32 *lp;

	tp = &nt35565_tx_buf;
	rp = &nt35565_rx_buf;
	cmd = &nt35565_manufacture_id_cmd;
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 3);
	lp = (uint32 *)rp->data;
	printk("%s: manufacture_id=%x\n", __func__, *lp);

	cmd = &nt35565_addr_mode_cmd;
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 3);
	lp = (uint32 *)rp->data;
	printk("%s: nt35565_addr_mode_cmd=%x\n", __func__, *lp);

	return *lp;
}
#endif

static int mipi_nt35565_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;

	pr_debug("mipi_nt35565_lcd_on E\n");
	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;

	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi  = &mfd->panel_info.mipi;

	mipi_dsi_cmds_tx(mfd, &nt35565_tx_buf,
		nt35565_cmd_display_on_cmds,
		ARRAY_SIZE(nt35565_cmd_display_on_cmds));

	#if 0
	mipi_dsi_cmd_bta_sw_trigger(); /* clean up ack_err_status */
	mipi_nt35565_manufacture_id(mfd);
	#endif

	pr_debug("mipi_nt35565_lcd_on X\n");

	return 0;
}

static int mipi_nt35565_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	pr_debug("mipi_nt35565_lcd_off E\n");

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi_dsi_cmds_tx(mfd, &nt35565_tx_buf, nt35565_display_off_cmds,
			ARRAY_SIZE(nt35565_display_off_cmds));

	pr_debug("mipi_nt35565_lcd_off X\n");
	return 0;
}

static int __devinit mipi_nt35565_lcd_probe(struct platform_device *pdev)
{
	pr_debug("%s\n", __func__);

	if (pdev->id == 0) {
		mipi_nt35565_pdata = pdev->dev.platform_data;
		return 0;
	}

	spin_lock_init(&nt35565_spin_lock);
	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_nt35565_lcd_probe,
	.driver = {
		.name   = "mipi_NT35565",
	},
};

#define MAX_BL_LEVEL 32
#define LCD_BL_EN 96

static void mipi_nt35565_set_backlight(struct msm_fb_data_type *mfd)
{
	int level = mfd->bl_level;
	int max = mfd->panel_info.bl_max;
	int min = mfd->panel_info.bl_min;
	unsigned long flags;
	int i;

	pr_debug("%s, level = %d\n", __func__, level);
	spin_lock_irqsave(&nt35565_spin_lock, flags); //disable local irq and preemption
	if (level < min)
		level = min;
	if (level > max)
		level = max;

	if (level == 0) {
		gpio_set_value(LCD_BL_EN, 0);
		spin_unlock_irqrestore(&nt35565_spin_lock, flags);
		return;
	}

	for (i = 0; i < (MAX_BL_LEVEL - level + 1); i++) {
		gpio_set_value(LCD_BL_EN, 0);
		udelay(1);
		gpio_set_value(LCD_BL_EN, 1);
		udelay(1);
	}
	spin_unlock_irqrestore(&nt35565_spin_lock, flags);

	return;
}

static struct msm_fb_panel_data nt35565_panel_data = {
	.on	= mipi_nt35565_lcd_on,
	.off = mipi_nt35565_lcd_off,
	.set_backlight = mipi_nt35565_set_backlight,
};

static int ch_used[3];

static int mipi_nt35565_lcd_init(void)
{
	mipi_dsi_buf_alloc(&nt35565_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&nt35565_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}
int mipi_nt35565_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	ret = mipi_nt35565_lcd_init();
	if (ret) {
		pr_err("mipi_nt35565_lcd_init() failed with ret %u\n", ret);
		return ret;
	}

	pdev = platform_device_alloc("mipi_NT35565", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	nt35565_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &nt35565_panel_data,
				sizeof(nt35565_panel_data));
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
