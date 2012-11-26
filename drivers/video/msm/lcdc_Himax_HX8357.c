/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora Forum nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * Alternatively, provided that this notice is retained in full, this software
 * may be relicensed by the recipient under the terms of the GNU General Public
 * License version 2 ("GPL") and only version 2, in which case the provisions of
 * the GPL apply INSTEAD OF those given above.  If the recipient relicenses the
 * software under the GPL, then the identification text in the MODULE_LICENSE
 * macro must be changed to reflect "GPLv2" instead of "Dual BSD/GPL".  Once a
 * recipient changes the license terms to the GPL, subsequent recipients shall
 * not relicense under alternate licensing terms, including the BSD or dual
 * BSD/GPL terms.  In addition, the following license statement immediately
 * below and between the words START and END shall also then apply when this
 * software is relicensed under the GPL:
 *
 * START
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 and only version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <linux/delay.h>
#include <mach/gpio.h>

#include <../../../../arch/arm/mach-msm/pmic.h>
#include "msm_fb.h"
#include <mach/rpc_pmapp.h>

#include "lcdc_lcm_id.h"

#define CMD_HEAD					0
#define DATA_HEAD				1

static int spi_cs;
static int spi_sclk;

static int spi_sdi;
//static int lcd_reset;
//static int lcd_id;

enum wake_up_staus
{
	dead,
	active,
	inactive
};

static unsigned char bit_shift[8] = {
	(1 << 7),				/* MSB */
	(1 << 6),
	(1 << 5),
	(1 << 4),
	(1 << 3),
	(1 << 2),
	(1 << 1),
	(1 << 0)		               /* LSB */
};

struct hx8357_state_type{
	struct semaphore sem;
	boolean disp_initialized;
};

static struct hx8357_state_type hx8357_state;
static struct msm_panel_common_pdata *lcdc_hx8357_pdata;

static void lcdc_mdelay(uint32 m)
{
	mdelay(m);
}

static void lcdc_udelay(uint32 m)
{
	udelay(m);
}
  
static void spi_write(uint8  byt, uint8 head)
{
	uint8 i;
	gpio_set_value(spi_cs, 1);
	lcdc_udelay(1);
	gpio_set_value(spi_cs, 0);
	lcdc_udelay(1);
	gpio_set_value(spi_sdi, head);
	lcdc_udelay(1);
	gpio_set_value(spi_sclk, 0);
	lcdc_udelay(1);
	gpio_set_value(spi_sclk, 1);
	lcdc_udelay(1);
	for (i = 0; i < 8; i++)
	{
		if (byt & bit_shift[i])
			gpio_set_value(spi_sdi, 1);
		else
			gpio_set_value(spi_sdi, 0);
		lcdc_udelay(1);
		gpio_set_value(spi_sclk, 0);
		lcdc_udelay(1);
		gpio_set_value(spi_sclk, 1);
		lcdc_udelay(1);	
	}
	lcdc_udelay(1);
	gpio_set_value(spi_cs, 1);	
}

static void spi_write_cmd(uint8 cmd)
{
	spi_write(cmd,CMD_HEAD);	
}

static void hx8357_disp_on(void)
{
	printk("hx8357_disp_on start!\n");
	spi_write_cmd(0x11);
	lcdc_mdelay(150);	
	spi_write_cmd(0x29);
	lcdc_mdelay(20);
}

#if 0
static void lcdc_hx8357_sleep_out(void)
{
	u8 i = 0;
	for(i = 0; i < 2; i++)
	{
		gpio_set_value(spi_cs, 0);
		lcdc_udelay(5);		
		gpio_set_value(spi_cs, 1);
		lcdc_udelay(5);
	}
	lcdc_mdelay(1);
	for(i = 0; i < 4; i++)
	{
		gpio_set_value(spi_cs, 0);
		lcdc_udelay(5);			
		gpio_set_value(spi_cs, 1);
		lcdc_udelay(5);
	}
	lcdc_mdelay(10);
	
}
#endif
static int lcd_on_flag = 0;
static int lcdc_hx8357_panel_on(struct platform_device *pdev)
{
	down(&hx8357_state.sem);

	if (!hx8357_state.disp_initialized) {
		hx8357_state.disp_initialized = TRUE;
	}
	else{	
		//lcdc_hx8357_sleep_out();
		hx8357_disp_on();
		lcd_on_flag = 1;
	}
	
	printk("lcdc_hx8357_panel_on end!\n");	
	up(&hx8357_state.sem);
	return 0;
}

static void hx8357_disp_off(void)
{
	spi_write_cmd(0x28);
	lcdc_mdelay(20);
	spi_write_cmd(0x10);
	lcdc_mdelay(120);
}

static int lcdc_hx8357_panel_off(struct platform_device *pdev)
{
	down(&hx8357_state.sem);

	hx8357_disp_off();

	printk("lcdc_hx8357_panel_off end!\n");	
	up(&hx8357_state.sem);
	return 0;
}

#define LCDC_MAX_LEVEL	26
static unsigned char lcdc_bl_level[LCDC_MAX_LEVEL+1]={0,  2,   4,   6,   8, 10, 12, 15, 18, 21, 
											24, 29, 34, 39, 44, 48, 51, 56, 60, 65, 
											70, 75, 80, 85, 90, 95, 100};

extern int pmic_set_led_intensity(enum ledtype type, int level);
static void lcdc_hx8357_set_backlight(struct msm_fb_data_type *mfd)
{
	
	int bl_level;
	int ret = -EPERM;

	bl_level = lcdc_bl_level[mfd->bl_level];

	//printk("lcdc_hx8357_set_backlight:	mfd->bl_level = %d, bl_level = %d\n", mfd->bl_level, bl_level);
	if( (mfd->bl_level <= 2) && (1 == lcd_on_flag)){
		mdelay(30);
		lcd_on_flag = 0;
	}
	pmapp_disp_backlight_init();
	ret = pmapp_disp_backlight_set_brightness(bl_level);
	if(ret)
		printk(KERN_WARNING "%s: can't set lcd backlight!\n",
				__func__);
}


static int __devinit hx8357_probe(struct platform_device *pdev)
{
	printk("hx8357_probe %d\n",__LINE__);
	if (pdev->id == 0) {
		lcdc_hx8357_pdata = pdev->dev.platform_data;
		return 0;
	}
	msm_fb_add_device(pdev);
	
	return 0;
}

static struct platform_driver this_driver = {
	.probe  = hx8357_probe,
	.driver = {
		.name   = "lcdc_hx8357_fwvga_pt",
	},
};

static struct msm_fb_panel_data hx8357_panel_data = {
	.on = lcdc_hx8357_panel_on,
	.off = lcdc_hx8357_panel_off,
	.set_backlight = lcdc_hx8357_set_backlight,
};

static struct platform_device this_device = {
	.name   = "lcdc_hx8357_fwvga_pt",
	.id	= 1,
	.dev	= {
		.platform_data = &hx8357_panel_data,
	}
};

static int __init lcdc_hx8357_panel_init(void)
{
	int ret;
	struct msm_panel_info *pinfo;

	printk("lcdc_hx8357_panel_init:	start\n");

	if(TRULY_HIMAX != tinboost_lcm_id){
		printk("this is not TRULY_HIMAX\n");
		ret = 0;
		goto exit_lcm_id_failed;
	}
	
	//lcd_reset = 88;
	spi_cs = 93;
	spi_sclk = 92;
	spi_sdi = 90;
	//lcd_id = 124;

	gpio_request(spi_cs, "spi_cs");
	gpio_request(spi_sclk, "spi_sclk");
	gpio_request(spi_sdi, "spi_sdi");
	//gpio_request(lcd_id, "lcd_id");

	gpio_direction_output(spi_cs, 1);
	gpio_direction_output(spi_sclk, 1);
	gpio_direction_output(spi_sdi, 1);
	//gpio_direction_input(lcd_id);

	ret = platform_driver_register(&this_driver);
	if (ret)
		goto exit_register_driver_failed;

	pinfo = &hx8357_panel_data.panel_info;

	hx8357_state.disp_initialized = false;
	sema_init(&hx8357_state.sem, 1);

	pinfo->xres = 320;
	pinfo->yres = 480;
	pinfo->type = LCDC_PANEL;
	pinfo->pdest = DISPLAY_1;
	pinfo->wait_cycle = 0;
	pinfo->bpp = 18;
	pinfo->fb_num = 2;
	pinfo->clk_rate = 16400000;
	pinfo->bl_max = LCDC_MAX_LEVEL;
	pinfo->bl_min = 0;

	pinfo->lcdc.h_back_porch = 20;		//20;  		
	pinfo->lcdc.h_front_porch = 20;    		//10
	pinfo->lcdc.h_pulse_width = 10;  		//10
	pinfo->lcdc.v_back_porch = 20;
	pinfo->lcdc.v_front_porch = 20;
	pinfo->lcdc.v_pulse_width = 20;
	pinfo->lcdc.border_clr = 0;     /* blk */
	pinfo->lcdc.underflow_clr = 0; //0xff;       /* blue */
	pinfo->lcdc.hsync_skew = 0;
	ret = platform_device_register(&this_device);
	if (ret)
		goto exit_register_device_failed;

	printk("lcdc_hx8357_panel_init:	end\n");

	return 0;

exit_register_device_failed:
	platform_driver_unregister(&this_driver);
exit_register_driver_failed:
	gpio_free(spi_cs);
	gpio_free(spi_sclk);
	gpio_free(spi_sdi);
	//gpio_free(lcd_id);
exit_lcm_id_failed:
	return ret;
}

module_init(lcdc_hx8357_panel_init);
