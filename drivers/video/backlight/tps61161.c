/* Copyright (c) 2010-2012, Code Aurora Forum. All rights reserved.
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/spi/spi.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/delay.h>
#include <mach/gpio.h>
#include <mach/board.h>
#include <mach/socinfo.h>
#include <mach/rpc_pmapp.h>

/* TPS61161 timing params */
#define DELAY_TSTART udelay(4)
#define DELAY_TEOS udelay(4)
#define DELAY_THLB udelay(33)
#define DELAY_TLLB udelay(67)
#define DELAY_THHB udelay(67)
#define DELAY_TLHB udelay(33)
#define DELAY_TESDET udelay(286)
#define DELAY_TESDELAY udelay(190)
#define DELAY_TESWIN mdelay(1)
#define DELAY_TOFF mdelay(4)

spinlock_t tps61161_spin_lock;

static int prev_bl = 0;
static int bl_ctl_num = 0;

static struct msm_panel_common_pdata *tps61161_backlight_pdata;

static void tps61161_send_bit(int bit_data)
{
	if (bit_data == 0) {
		gpio_set_value(bl_ctl_num, 0);;
		DELAY_TLLB;
		gpio_set_value(bl_ctl_num, 1);;
		DELAY_THLB;
	} else {
		gpio_set_value(bl_ctl_num, 0);;
		DELAY_TLHB;
		gpio_set_value(bl_ctl_num, 1);;
		DELAY_THHB;
	}
}

static void tps61161_send_byte(unsigned char byte_data)
{
	int n;

	for (n = 0; n < 8; n++) {
		tps61161_send_bit((byte_data & 0x80) ? 1 : 0);
		byte_data = byte_data << 1;
	}
}

static int tps61161_set_bl(int level, int max, int min)
{
	unsigned long flags;
	int ret = 0;

	printk("[DISP]%s: set backlight to %d, min = %d, max = %d\n",
		__func__, level, min, max);


	if((machine_is_msm8625_qrd5() && hw_version_is(3, 0)) || (machine_is_msm7x27a_qrd5a() && hw_version_is(3, 0)))
	{
		ret = pmapp_disp_backlight_set_brightness(level);
		if (ret)
			pr_err("%s: can't set lcd backlight!\n", __func__);
	} else {
		spin_lock_irqsave(&tps61161_spin_lock, flags); //disable local irq and preemption
		if (level == 0) {
			gpio_set_value(bl_ctl_num, 0);
			prev_bl = level;
			spin_unlock_irqrestore(&tps61161_spin_lock, flags);
			return 0;
		} else if (prev_bl == 0) {
			printk("[DISP]%s: prev_bl = 0, enter easy scale first\n", __func__);
			gpio_set_value(bl_ctl_num, 0);
			DELAY_TOFF;
			gpio_set_value(bl_ctl_num, 1);
			DELAY_TESDELAY;
			gpio_set_value(bl_ctl_num, 0);
			DELAY_TESDET;
			gpio_set_value(bl_ctl_num, 1);
			DELAY_TESWIN;
		}

		prev_bl = level;

		/* device address byte = 0x72 */
		tps61161_send_byte(0x72);

		/* t-EOS and t-start */
		gpio_set_value(bl_ctl_num, 0);
		DELAY_TEOS;
		gpio_set_value(bl_ctl_num, 1);
		DELAY_TSTART;

		/* data byte */
		tps61161_send_byte(level & 0x1F); //RFA = 0, address bit = 00, 5 bit level
		/* t-EOS */
		gpio_set_value(bl_ctl_num, 0);
		DELAY_TEOS;
		gpio_set_value(bl_ctl_num, 1);
		DELAY_TSTART;
		spin_unlock_irqrestore(&tps61161_spin_lock, flags);
	}


	return ret;
}

static int __devinit tps61161_backlight_probe(struct platform_device *pdev)
{
    printk("[DISP]%s\n", __func__);

    if (pdev->id == 0) {
        tps61161_backlight_pdata = (struct msm_panel_common_pdata *)pdev->dev.platform_data;

		if (tps61161_backlight_pdata) {
			tps61161_backlight_pdata->backlight_level = tps61161_set_bl;
			bl_ctl_num = tps61161_backlight_pdata->gpio;
			printk("[DISP]%s: bl_ctl_num = %d\n", __func__, bl_ctl_num);

                       /* FIXME: if continuous splash is enabled, set prev_bl to
                        * a non-zero value to avoid entering easyscale.
                        */
                       if (tps61161_backlight_pdata->cont_splash_enabled)
                           prev_bl = 1;
		}

        if((machine_is_msm8625_qrd5() && hw_version_is(3, 0))
            || (machine_is_msm7x27a_qrd5a() && hw_version_is(3, 0)))
            pmapp_disp_backlight_init();

        return 0;
    }

    printk("[DISP]%s: Wrong ID\n", __func__);
    return -1;
}

static struct platform_driver this_driver = {
    .probe  = tps61161_backlight_probe,
    .driver = {
        .name   = "tps6116_backlight",
    },
};

static int __init tps61161_backlight_init(void)
{
    printk("[DISP]%s\n", __func__);

	spin_lock_init(&tps61161_spin_lock);

    return platform_driver_register(&this_driver);
}

module_init(tps61161_backlight_init);

