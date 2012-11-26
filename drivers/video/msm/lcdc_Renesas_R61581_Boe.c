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


#define CMD_HEAD			0
#define DATA_HEAD			1
#define CMD_ACCESS_PROTECT          0xB0
#define PANEL_DRIVE_SETTING         0xC0
#define DISPLAY_TIMING_SETTING    0xC1
#define DRIVE_TIMING_SETTING        0xC4
#define INTERFACE_SETTING              0xC6
#define GAMMA_SETTING			    0xC8
#define COL_ADDR_SETTING               0x2A
#define PAGE_ADDR_SETTING             0x2B
#define TEAR_OFF_SETTING                 0x34
#define PIXEL_FORMAT_SETTING	     0x3A
#define TEAR_SACNLINE_SETTING	0x44
#define FRAME_MEM_ACCESS               0xB3
#define DISPLAY_MODE_SETTING         0xB4
#define EXIT_SLEEP_MODE                   0x11

#define POWER_SETTING                      0xD0
#define VCOM_SETTING			      0xD1
#define POWER_SETTING_NORMAL       0xD2


#define  ADDRESS_MODE_SETTING      0x36



#define WRITE_MEM_START                0x2C
#define SET_DISPLAY_ON                    0x29

#define BKLIGHT_LVL_ORIGINAL  0xff
#define BKBLIGHT_LVL_SLEEP       0xee

static int spi_cs;
static int spi_sclk;

static int spi_sdi;
//static int spi_sdo;
static int lcd_reset;
//static int lcd_id;
//static struct timer_list read_timer;
//static struct work_struct mt9v114_wq;
//static int wake_up_flag;
enum wake_up_staus
{
	dead,
	active,
	inactive
};
//static spinlock_t prism_lock;
//static u8 prism_lcd_ison=1;
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


struct R61581_state_type{
	boolean disp_initialized;
};

static struct R61581_state_type R61581_state = {FALSE};
static struct msm_panel_common_pdata *lcdc_R61581_pdata;

static uint8 cmd_access_protect[] =
{
	0x00
};
/*the num is 8*/
static uint8 panel_drive_setting[] =
{
	0x13,	//0x03
	0x3B,
	0x00,
	0x00,	//0x03
	0x00,	//0x04
	0x01,
	0x00,
	0x43		//0xAA
};

static uint8 display_timing_setting[] =
{
	0x08,	//0x00
	0x15,	//0x10
	0x08,	//0x04
	0x08		//0x0A
};

static uint8 drive_timing_setting[] =
{
	0x15,	//0x11
	0x03,	//0x01
	0X03,	//0x73
	0x01		//0x05
};

static uint8 interface_setting[] =
{
	0x02
};
/*the num is 20*/
static uint8 gamma_setting[] =
{
	0x0C, 	//0x04,
	0x05, 	//0x00,
	0x0A, 	//0x0C,
	0x6B, 	//0x35,
	0x04,	//0x03
	0x06, 	//0x0D,
	0x15, 	//0x05,
	0x10,	//0x02
	0x00,	//0x00
	0x31,	//0x32
	0x10, 	//0x03,
	0x15, 	//0x03,
	0x06, 	//0x13,
	0x64,	//0x54,
	0x0B,	//0x07
	0x0A, 	//0x0A,
	0x05, 	//0x08,
	0x0C, 	//0x04,
	0x31,	//0x32
	0x00		//0x11
};

static uint8 col_addr_setting[] =
{
	0x00,
	0x00,
	0x01,
	0x3F
};
static uint8 page_addr_setting[] =
{
	0x00,
	0x00,
	0x01,
	0xDF
};

static uint8 pixel_format_setting[] =
{
	0x60
};	
/*
static uint8 tear_sacnline_setting[] =
{
	0x00,
	0x01
};
*/
static uint8 frame_mem_setting[] =
{
	0x02,
	0x00,
	0x00,
	0x10		//0x00
};

static uint8 display_mode_setting1[] =
{
	0x00		//0x00
};

static uint8 power_setting[] =
{
	0x07,
	0x07,
	0x14,	//0x1B
	0xA2		//0x66
};	

static uint8 vcom_setting[] =
{
	0x03,	//0x00
	0x30,	//0x3A
	0x0A		//0x0E
};		
static uint8 power_setting_normal[] =
{
	0x03,
	0x04,	//0x24
	0x04		//0x00
};	

static uint8 address_mode_setting[] =
{
	0x00
};	
static uint8 display_mode_setting2[] =
{
	0x10
};

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

	//gpio_set_value(spi_cs, 1);
	//lcdc_udelay(1);
	gpio_set_value(spi_cs, 0);
	lcdc_udelay(1);
	gpio_set_value(spi_sdi, head);
	//lcdc_udelay(2);
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
		//lcdc_udelay(2);
		gpio_set_value(spi_sclk, 0);
		//lcdc_udelay(1);
		ndelay(50);
		gpio_set_value(spi_sclk, 1);
		//lcdc_udelay(1);
		ndelay(50);
	}
	lcdc_udelay(1);
	gpio_set_value(spi_cs, 1);	
}

static void spi_write_cmd(uint8 cmd)
{
	spi_write(cmd,CMD_HEAD);	
}
static void spi_write_data(uint8 dat)
{
	spi_write(dat, DATA_HEAD);
}

static void spi_write_data_array(uint8* array,uint8 count)
{
	uint8 i;
	for (i = 0; i < count; i++)
	{
		spi_write_data(array[i]);
		
	}
}

static void spi_config_reg(uint8 cmd, uint8 dat)
{
	spi_write_cmd(cmd);	
	spi_write_data(dat);
}

 static void spi_config_reg_muti(uint8 cmd, uint8* array, uint8 count)
{
	spi_write_cmd(cmd);	
	spi_write_data_array(array,count);
}

static void R61581_init(void)
{	
	gpio_set_value(lcd_reset, 0);
	lcdc_mdelay(2);	//20
	gpio_set_value(lcd_reset, 1);
	lcdc_mdelay(5);	//10

	spi_config_reg_muti(CMD_ACCESS_PROTECT,cmd_access_protect,1);
	spi_config_reg_muti(PANEL_DRIVE_SETTING,panel_drive_setting,8);
	spi_config_reg_muti(DISPLAY_TIMING_SETTING,display_timing_setting,4);
	spi_config_reg_muti(DRIVE_TIMING_SETTING,drive_timing_setting,4);
	spi_config_reg_muti(INTERFACE_SETTING ,interface_setting,1);
	spi_config_reg_muti(GAMMA_SETTING, gamma_setting, 20);
	spi_config_reg_muti(COL_ADDR_SETTING, col_addr_setting, 4);
	spi_config_reg_muti(PAGE_ADDR_SETTING, page_addr_setting, 4);
	spi_write_cmd(TEAR_OFF_SETTING);


	spi_config_reg_muti(PIXEL_FORMAT_SETTING,pixel_format_setting,1);
	//spi_config_reg_muti(TEAR_SACNLINE_SETTING, tear_sacnline_setting, 2);
	spi_config_reg_muti(FRAME_MEM_ACCESS,frame_mem_setting,4);
	
	spi_config_reg_muti(DISPLAY_MODE_SETTING,display_mode_setting1,1);

	spi_write_cmd(EXIT_SLEEP_MODE);
	
	lcdc_mdelay(120);
	spi_config_reg_muti(POWER_SETTING ,power_setting, 4);
	spi_config_reg_muti(VCOM_SETTING ,vcom_setting, 3);
	spi_config_reg_muti(POWER_SETTING_NORMAL ,power_setting_normal, 3);
	spi_config_reg_muti(CMD_ACCESS_PROTECT ,cmd_access_protect, 1);
	spi_config_reg_muti(FRAME_MEM_ACCESS ,frame_mem_setting,4);
	spi_config_reg_muti(ADDRESS_MODE_SETTING ,address_mode_setting,1);
	spi_config_reg_muti(COL_ADDR_SETTING ,col_addr_setting,4);
	spi_config_reg_muti(PAGE_ADDR_SETTING, page_addr_setting, 4);
	spi_config_reg_muti(DISPLAY_MODE_SETTING, display_mode_setting2,1);

	//lcdc_mdelay(5);	//10
	spi_write_cmd(WRITE_MEM_START);
	//lcdc_mdelay(12);	//40
	spi_write_cmd(SET_DISPLAY_ON);

	spi_write_cmd(WRITE_MEM_START);
}

static void R61581_disp_on(void)
{
	printk("into R61581_disp_on\n");
	R61581_init();		
}

#if 0
static void lcdc_R61581_sleep_out(void)
{
	u8 i = 0;
	for(i = 0; i < 2; i++)
	{
		gpio_set_value(spi_cs, 0);
		lcdc_udelay(100);		
		gpio_set_value(spi_cs, 1);
		lcdc_udelay(100);
	}
	lcdc_mdelay(2);
	for(i = 0; i < 4; i++)
	{
		gpio_set_value(spi_cs, 0);
		lcdc_udelay(100);			
		gpio_set_value(spi_cs, 1);
		lcdc_udelay(100);
	}
	lcdc_mdelay(10);
	
}
#endif

static int lcdc_R61581_panel_on(struct platform_device *pdev)
{
	if (!R61581_state.disp_initialized) {
		R61581_state.disp_initialized = TRUE;
	}
	else
	{	
		R61581_disp_on();
	}
	printk("lcdc_R61581_panel_on  end\n");	
	
	return 0;
	
}

static void R61581_disp_off(void)
{
	spi_write_cmd(0x28);
	lcdc_mdelay(150);
	spi_write_cmd(0x10);	
	lcdc_mdelay(250);
	spi_config_reg(0xb4,0);
	spi_config_reg(0xb1,1);
	lcdc_mdelay(20);
}

static int lcdc_R61581_panel_off(struct platform_device *pdev)
{
	R61581_disp_off();
	printk("lcdc_R61581_panel_off end!\n");	
	return 0;
}

#define LCDC_MAX_LEVEL	26
static unsigned char lcdc_bl_level[LCDC_MAX_LEVEL+1]={0,  2,   4,   6,   8, 10, 12, 15, 18, 21, 
											24, 29, 34, 39, 44, 48, 51, 56, 60, 65, 
											70, 75, 80, 85, 90, 95, 100};

extern int pmic_set_led_intensity(enum ledtype type, int level);
static void lcdc_R61581_set_backlight(struct msm_fb_data_type *mfd)
{	
	int bl_level;
	int ret = -EPERM;

	bl_level = lcdc_bl_level[mfd->bl_level];

	pmapp_disp_backlight_init();	
	ret = pmapp_disp_backlight_set_brightness(bl_level);
	if(ret)
		printk(KERN_WARNING "%s: can't set lcd backlight!\n",
				__func__);
}


static int __devinit R61581_probe(struct platform_device *pdev)
{
	printk("R61581_probe %d\n",__LINE__);
	if (pdev->id == 0) {
		lcdc_R61581_pdata = pdev->dev.platform_data;
		return 0;
	}
	msm_fb_add_device(pdev);
	
	return 0;
}

static struct platform_driver this_driver = {
	.probe  = R61581_probe,
	.driver = {
		.name   = "lcdc_R61581_boe_fwvga_pt",
	},
};

static struct msm_fb_panel_data R61581_panel_data = {
	.on = lcdc_R61581_panel_on,
	.off = lcdc_R61581_panel_off,
	.set_backlight = lcdc_R61581_set_backlight,
};

static struct platform_device this_device = {
	.name   = "lcdc_R61581_boe_fwvga_pt",
	.id	= 1,
	.dev	= {
		.platform_data = &R61581_panel_data,
	}
};

static int __init lcdc_R61581_boe_panel_init(void)
{
	int ret;
	struct msm_panel_info *pinfo;

	printk("lcdc_R61581_boe_panel_init:	start\n");
	if(BOE_R61581 != tinboost_lcm_id){
		printk("this is not BOE_R61581\n");
		ret = 0;
		goto exit_lcm_id_failed;
	}

	lcd_reset = 88;
	spi_cs = 93;
	spi_sclk = 92;
	spi_sdi = 90;
	//spi_sdo = 91;
	//lcd_id = 124;

	gpio_request(lcd_reset, "lcd_reset");
	gpio_request(spi_cs, "spi_cs");
	gpio_request(spi_sclk, "spi_sclk");
	gpio_request(spi_sdi, "spi_sdi");
	//gpio_request(spi_sdo, "spi_sdo");
	//gpio_request(lcd_id, "lcd_id");

	gpio_direction_output(lcd_reset, 1);
	gpio_direction_output(spi_cs, 1);
	gpio_direction_output(spi_sclk, 1);
	gpio_direction_output(spi_sdi, 1);

	ret = platform_driver_register(&this_driver);
	if (ret)
		goto exit_register_driver_failed;

	pinfo = &R61581_panel_data.panel_info;

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

	pinfo->lcdc.h_back_porch = 2;	//20;
	pinfo->lcdc.h_front_porch = 2;	//10;
	pinfo->lcdc.h_pulse_width = 2;	//10;
	pinfo->lcdc.v_back_porch = 1;
	pinfo->lcdc.v_front_porch = 10;
	pinfo->lcdc.v_pulse_width = 2;
	pinfo->lcdc.border_clr = 0;     	/* blk */
	pinfo->lcdc.underflow_clr = 0;      //0xff /* blue */
	pinfo->lcdc.hsync_skew = 0;
	ret = platform_device_register(&this_device);
	if (ret)
		goto exit_register_device_failed;

	printk("lcdc_R61581_boe_panel_init:	end\n");

	return 0;

exit_register_device_failed:
	platform_driver_unregister(&this_driver);
exit_register_driver_failed:
	gpio_free(spi_cs);
	gpio_free(spi_sclk);
	gpio_free(spi_sdi);
	gpio_free(lcd_reset);
	//gpio_free(lcd_id);
exit_lcm_id_failed:
	return ret;
}

module_init(lcdc_R61581_boe_panel_init);
