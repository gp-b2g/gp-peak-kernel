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
#include "mipi_NT35510_alaska.h"

static struct msm_panel_common_pdata *mipi_nt35510_alaska_pdata;
static struct dsi_buf nt35510_alaska_tx_buf;
static struct dsi_buf nt35510_alaska_rx_buf;

/************************* Power ON Command **********************/
static char enable_page1[6]={
    0xF0,0x55,0xAA,0x52,0x08,0x01
    };
static char avdd_set1[4]={
    0xB6,0x44,0x44,0x44
    };
static char avdd_set2[4]={
    0xB0,0x0B,0x0B,0x0B
    };
static char avee_set1[4]={
    0xB7,0x34,0x34,0x34
    };
static char avee_set2[4]={
    0xB1,0x0B,0x0B,0x0B
    };

static char vcl_power_set1[2]={0xB8, 0x24};
static char vcl_power_set2[2]={0xB2, 0x00};

static char vgh_enable1[4]={
    0xB9,0x24,0x24,0x24
};

static char vgh_enable2[4]={
    0xB3,0x05,0x05,0x05
};

static char vgl_set[4]={
    0xBA,0x34,0x34,0x34
};

static char vgl_reg_set[4]={
    0xB5,0x0B,0x0B,0x0B
};

static char vgmp_set[4]={
    0xBC,0x00,0xA0,0x00
};

static char vgmn_set[4]={
    0xBD,0x00,0xA0,0x00
};

static char vcom_set[3]={
    0xBE,0x00,0x5B
};

static char red_set1[53]={
    0xD1,0x00,0x66,0x00,0x67,0x00,0x68,0x00,0x6C,
    0x00,0x78,0x00,0x8B,0x00,0xA6,0x00,0xC3,0x01,
    0x01,0x01,0x5E,0x01,0xA0,0x01,0xF6,0x02,0x37,
    0x02,0x3B,0x02,0x76,0x02,0xB2,0x02,0xD8,0x03,
    0x36,0x03,0x4B,0x03,0x8E,0x03,0xFE,0x03,0xFF,
    0x03,0xFF,0x03,0xFF,0x03,0xFF,0x03,0xFF
};

static char green_set1[53]={
    0xD2,0x01,0x0B,0x01,0x13,0x01,0x22,0x01,0x30,
    0x01,0x3C,0x01,0x52,0x01,0x65,0x01,0x87,0x01,
    0xA0,0x01,0xC8,0x01,0xEB,0x02,0x27,0x02,0x5B,
    0x02,0x5C,0x02,0x8D,0x02,0xC5,0x02,0xEC,0x03,
    0x3F,0x03,0x53,0x03,0x6F,0x03,0xAF,0x03,0xAF,
    0x03,0xAF,0x03,0xAF,0x03,0xAF,0x03,0xAF
};

static char blue_set1[53]={
    0xD3,0x01,0x44,0x01,0x45,0x01,0x46,0x01,0x47,
    0x01,0x4A,0x01,0x5D,0x01,0x6E,0x01,0x8C,0x01,
    0xA3,0x01,0xCA,0x01,0xEC,0x02,0x27,0x02,0x5B,
    0x02,0x5C,0x02,0x8D,0x02,0xC6,0x02,0xF0,0x03,
    0x34,0x03,0xDD,0x03,0xFE,0x03,0xFE,0x03,0xFF,
    0x03,0xFF,0x03,0xFF,0x03,0xFF,0x03,0xFF
};

static char red_set2[53]={
    0xD4,0x00,0x66,0x00,0x67,0x00,0x68,0x00,0x6C,
    0x00,0x78,0x00,0x8B,0x00,0xA6,0x00,0xC3,0x01,
    0x01,0x01,0x5E,0x01,0xA0,0x01,0xF6,0x02,0x37,
    0x02,0x3B,0x02,0x76,0x02,0xB2,0x02,0xD8,0x03,
    0x36,0x03,0x4B,0x03,0x8E,0x03,0xFE,0x03,0xFF,
    0x03,0xFF,0x03,0xFF,0x03,0xFF,0x03,0xFF
};

static char green_set2[53]={
    0xD5,0x01,0x0B,0x01,0x13,0x01,0x22,0x01,0x30,
    0x01,0x3C,0x01,0x52,0x01,0x65,0x01,0x87,0x01,
    0xA0,0x01,0xC8,0x01,0xEB,0x02,0x27,0x02,0x5B,
    0x02,0x5C,0x02,0x8D,0x02,0xC5,0x02,0xEC,0x03,
    0x3F,0x03,0x53,0x03,0x6F,0x03,0xAF,0x03,0xAF,
    0x03,0xAF,0x03,0xAF,0x03,0xAF,0x03,0xAF
};

static char blue_set2[53]={
    0xD6,0x01,0x44,0x01,0x45,0x01,0x46,0x01,0x47,
    0x01,0x4A,0x01,0x5D,0x01,0x6E,0x01,0x8C,0x01,
    0xA3,0x01,0xCA,0x01,0xEC,0x02,0x27,0x02,0x5B,
    0x02,0x5C,0x02,0x8D,0x02,0xC6,0x02,0xF0,0x03,
    0x34,0x03,0xDD,0x03,0xFE,0x03,0xFE,0x03,0xFF,
    0x03,0xFF,0x03,0xFF,0x03,0xFF,0x03,0xFF
};

static char enable_page0[6]={
    0xF0,0x55,0xAA,0x52,0x08,0x00
};

static char color_enhance[]={
    0xB4,0x10
};

static char rgb_set[6]={
    0xB0,0x00,0x05,0x02,0x05,0x02
};

static char sdt_set[2]={
    0xB6,0x05
};

static char gate_eq_set[3]={
    0xB7,0x00,0x00
};

static char clkb_set[2]={
    0xBA,0x01
};

static char src_eq_set[5]={
    0xB8,0x01,0x05,0x05,0x05
};

static char clm_set[4]={
    0xBC,0x00,0x00,0x00
};

static char boe_set[4]={
    0xCC,0x03,0x00,0x00
};

static char dis_time_set[6]={
    0xBD,0x01,0x84,0x07,0x31,0x00
};

static char cmd_set1[6]={
    0xB1,0xF8,0x00
};

static char cmd_set2[2]={
    0x3A,0x77
};

static char cmd_set3[1]={
    0x11
};

static char cmd_set4[1]={
    0x29
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

static char Set_address_mode[2] = {
	0x36, 0xD0,
};

static struct dsi_cmd_desc nt35510_alaska_cmd_on_cmds_video_mode[] = {
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(enable_page1), enable_page1},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(avdd_set1), avdd_set1},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(avdd_set2), avdd_set2},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(avee_set1), avee_set1},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(avee_set2), avee_set2},
    {DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(vcl_power_set1), vcl_power_set1},
    {DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(vcl_power_set2), vcl_power_set2},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(vgh_enable1), vgh_enable1},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(vgh_enable2), vgh_enable2},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(vgl_set), vgl_set},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(vgl_reg_set), vgl_reg_set},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(vgmp_set), vgmp_set},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(vgmn_set), vgmn_set},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(vcom_set), vcom_set},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(red_set1), red_set1},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(green_set1), green_set1},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(blue_set1), blue_set1},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(red_set2), red_set2},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(green_set2), green_set2},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(blue_set2), blue_set2},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(enable_page0), enable_page0},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(color_enhance), color_enhance},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(rgb_set), rgb_set},
    {DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(sdt_set), sdt_set},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(gate_eq_set), gate_eq_set},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(src_eq_set), src_eq_set},
    {DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(clkb_set), clkb_set},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(clm_set), clm_set},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(boe_set), boe_set},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(dis_time_set), dis_time_set},
    {DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(cmd_set1), cmd_set1},
    {DTYPE_GEN_WRITE, 1, 0, 0, 10, sizeof(cmd_set2), cmd_set2},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_bringtness), display_bringtness},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(crtl_display), crtl_display},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(Set_address_mode), Set_address_mode},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cabc), cabc},
    {DTYPE_GEN_WRITE, 1, 0, 0, 0, sizeof(cmd_set3), cmd_set3},
    {DTYPE_GEN_WRITE, 1, 0, 0, 0, sizeof(cmd_set4), cmd_set4},
    {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(enable_page0), enable_page0},
};

/************************* Power OFF Command **********************/
static char stabdby_off_1[2] = {0x28, 0x00};

static char stabdby_off_2[2] = {0x10, 0x00};

static struct dsi_cmd_desc nt35510_alaska_display_off_cmds[] = {
    {DTYPE_GEN_WRITE, 1, 0, 0, 0, sizeof(stabdby_off_1), stabdby_off_1},
    {DTYPE_GEN_WRITE, 1, 0, 0, 0, sizeof(stabdby_off_2), stabdby_off_2},
};

void mipi_nt35510_alaska_panel_set_backlight(struct msm_fb_data_type *mfd)
{
    int32 level;
    int max = mfd->panel_info.bl_max;
    int min = mfd->panel_info.bl_min;

    if (mipi_nt35510_alaska_pdata && mipi_nt35510_alaska_pdata->backlight_level) {
        level = mipi_nt35510_alaska_pdata->backlight_level(mfd->bl_level, max, min);

        if (level < 0) {
            printk("%s: backlight level control failed\n", __func__);
        }
    } else {
        printk("%s: missing baclight control function\n", __func__);
    }

    return;
}

static int mipi_nt35510_alaska_lcd_on(struct platform_device *pdev)
{
    struct msm_fb_data_type *mfd;

    printk("%s: Enter\n", __func__);

    mfd = platform_get_drvdata(pdev);
    if (!mfd)
        return -ENODEV;
    if (mfd->key != MFD_KEY)
        return -EINVAL;

    mipi_dsi_cmds_tx(mfd, &nt35510_alaska_tx_buf,
            nt35510_alaska_cmd_on_cmds_video_mode,    ARRAY_SIZE(nt35510_alaska_cmd_on_cmds_video_mode));

#if 0
    if(mfd->panel_info.type == MIPI_VIDEO_PANEL)
    {
        printk("[LCM]in %s. video panel\n", __func__);
        mipi_dsi_cmds_tx(mfd, &nt35510_alaska_tx_buf,
            nt35510_alaska_cmd_on_cmds_video_mode,    ARRAY_SIZE(nt35510_alaska_cmd_on_cmds_video_mode));
    }
    else//if(mfd->panel_info.type = MIPI_CMD_PANEL)
    {
        printk("[LCM]in %s. cmd panel\n", __func__);
        mipi_dsi_cmds_tx(mfd, &nt35510_alaska_tx_buf,
            nt35510_alaska_cmd_on_cmds_video_mode, ARRAY_SIZE(nt35510_alaska_cmd_on_cmds_video_mode));
    }
#endif
    printk("%s: Done\n", __func__);
    return 0;
}

static int mipi_nt35510_alaska_lcd_off(struct platform_device *pdev)
{
    struct msm_fb_data_type *mfd;

    printk("%s: Enter\n", __func__);
    mfd = platform_get_drvdata(pdev);

    if (!mfd)
        return -ENODEV;
    if (mfd->key != MFD_KEY)
        return -EINVAL;
    mipi_dsi_cmds_tx(mfd, &nt35510_alaska_tx_buf, nt35510_alaska_display_off_cmds, ARRAY_SIZE(nt35510_alaska_display_off_cmds));

    printk("%s: Done\n", __func__);
    return 0;
}

static int __devinit mipi_nt35510_alaska_lcd_probe(struct platform_device *pdev)
{
    printk("%s: Enter\n", __func__);
    if (pdev->id == 0) {
        mipi_nt35510_alaska_pdata = pdev->dev.platform_data;
        return 0;
    }
    msm_fb_add_device(pdev);

    printk("%s: Done\n", __func__);
    return 0;
}

static struct platform_driver this_driver = {
    .probe  = mipi_nt35510_alaska_lcd_probe,
    .driver = {
        .name   = "mipi_nt35510_alaska",
    },
};

static struct msm_fb_panel_data nt35510_alaska_panel_data = {
    .on    = mipi_nt35510_alaska_lcd_on,
    .off    = mipi_nt35510_alaska_lcd_off,
    .set_backlight    = mipi_nt35510_alaska_panel_set_backlight,
};

static int ch_used[3];

int mipi_nt35510_alaska_device_register(struct msm_panel_info *pinfo,
                    u32 channel, u32 panel)
{
    struct platform_device *pdev = NULL;
    int ret;

    printk("%s\n", __func__);

    if ((channel >= 3) || ch_used[channel])
        return -ENODEV;

    ch_used[channel] = TRUE;

    pdev = platform_device_alloc("mipi_nt35510_alaska", (panel << 8)|channel);
    if (!pdev)
        return -ENOMEM;

    nt35510_alaska_panel_data.panel_info = *pinfo;

    ret = platform_device_add_data(pdev, &nt35510_alaska_panel_data, sizeof(nt35510_alaska_panel_data));
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

static int __init mipi_nt35510_alaska_lcd_init(void)
{
    printk("%s\n", __func__);

    mipi_dsi_buf_alloc(&nt35510_alaska_tx_buf, DSI_BUF_SIZE);
    mipi_dsi_buf_alloc(&nt35510_alaska_rx_buf, DSI_BUF_SIZE);

    return platform_driver_register(&this_driver);
}

module_init(mipi_nt35510_alaska_lcd_init);

