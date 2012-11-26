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
#define VBPD_240320		((6-1)&0xff)
#define VFPD_240320		((6-1)&0xff)
#define VSPW_240320		((2-1) &0x3f)
#define HBPD_240320		((6-1)&0x7f)
#define HFPD_240320		((6-1)&0xff)
#define HSPW_240320		((2-1)&0xff)
#define CMD_HEAD					0
#define DATA_HEAD				1

#define WaitTime	mdelay
#define LCDSPI_InitCMD	spi_write_cmd
#define LCDSPI_InitDAT	spi_write_data
static int spi_write_type;

static int spi_cs;
static int spi_sclk;

static int spi_sdi;
static int lcd_reset;
//static int lcd_id;

#define DEBUG
#ifdef DEBUG
#define HX8347_DEBUG(format, args...)   printk(KERN_INFO "lcdc_Himax_HX8347:%s( )_%d_: " format, \
	__FUNCTION__ , __LINE__, ## args);
#else
#define HX8347_DEBUG(format, args...);
#endif

enum wake_up_staus
{
	dead,
	active,
	inactive
};


struct hx8347_state_type{
	struct semaphore sem;
	boolean disp_initialized;
};

static struct hx8347_state_type hx8347_state;
static struct msm_panel_common_pdata *lcdc_hx8347_pdata;

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
#if 1
	 unsigned short i;
	unsigned short MB=0X00;	
	gpio_set_value(spi_cs, 0);
	lcdc_udelay(10);
	lcdc_udelay(100);
	if (/*spi_write_type*/head ==0 )
   		{ MB=0X74;}
	else 
		{MB=0X76;}
	for(i=0;i<8;i++)
	  {
		gpio_set_value(spi_sclk, 0);
	   if(MB&0x80)
		gpio_set_value(spi_sdi, 1);
	   else
		gpio_set_value(spi_sdi, 0);
	  lcdc_udelay(100);
			gpio_set_value(spi_sclk, 1);
		  lcdc_udelay(100);
	   MB<<=1;
	  }

    MB=byt;
	
	for(i=0;i<8;i++)
	  {
	 
	 gpio_set_value(spi_sclk, 0); 
	   if(MB&0x80)
	gpio_set_value(spi_sdi, 1);
	   else
	gpio_set_value(spi_sdi, 0);
	  
	  lcdc_udelay(100);
		gpio_set_value(spi_sclk, 1);
	  lcdc_udelay(100);
	   MB<<=1;
	  }
	gpio_set_value(spi_cs, 1);	
#endif
}

static void spi_write_cmd(uint8 cmd)
{
	spi_write_type =0;
	spi_write(cmd,CMD_HEAD);	
}

static void spi_write_data(uint8 dat)
{
	spi_write_type =1;
	spi_write(dat, DATA_HEAD);
	
}

static void hx8347_disp_on(void)
{
	
	gpio_set_value(lcd_reset, 1);
	mdelay(10); 
	gpio_set_value(lcd_reset, 0);
	mdelay(20); 
	gpio_set_value(lcd_reset, 1);
	mdelay(20); 


 //HX8347-H
LCDSPI_InitCMD(0xEA);LCDSPI_InitDAT(0x00); 
LCDSPI_InitCMD(0xEB);LCDSPI_InitDAT(0x00); 
LCDSPI_InitCMD(0xEC);LCDSPI_InitDAT(0x3C); 
LCDSPI_InitCMD(0xED);LCDSPI_InitDAT(0xC4); 
LCDSPI_InitCMD(0xE8);LCDSPI_InitDAT(0x50);  
LCDSPI_InitCMD(0xE9);LCDSPI_InitDAT(0x38); 
LCDSPI_InitCMD(0xF1);LCDSPI_InitDAT(0x01); 
LCDSPI_InitCMD(0xF2);LCDSPI_InitDAT(0x08); 

//Gamma 2.2 Setting         
LCDSPI_InitCMD(0x40);LCDSPI_InitDAT(0x00); 
LCDSPI_InitCMD(0x41);LCDSPI_InitDAT(0x06); 
LCDSPI_InitCMD(0x42);LCDSPI_InitDAT(0x06); 
LCDSPI_InitCMD(0x43);LCDSPI_InitDAT(0x2D); 
LCDSPI_InitCMD(0x44);LCDSPI_InitDAT(0x2F); 
LCDSPI_InitCMD(0x45);LCDSPI_InitDAT(0x3F); 
LCDSPI_InitCMD(0x46);LCDSPI_InitDAT(0x0B); 
LCDSPI_InitCMD(0x47);LCDSPI_InitDAT(0x72); 
LCDSPI_InitCMD(0x48);LCDSPI_InitDAT(0x07); 
LCDSPI_InitCMD(0x49);LCDSPI_InitDAT(0x0F); 
LCDSPI_InitCMD(0x4A);LCDSPI_InitDAT(0x13); 
LCDSPI_InitCMD(0x4B);LCDSPI_InitDAT(0x13); 
LCDSPI_InitCMD(0x4C);LCDSPI_InitDAT(0x1C); 

LCDSPI_InitCMD(0x50);LCDSPI_InitDAT(0x00); 
LCDSPI_InitCMD(0x51);LCDSPI_InitDAT(0x10); 
LCDSPI_InitCMD(0x52);LCDSPI_InitDAT(0x12); 
LCDSPI_InitCMD(0x53);LCDSPI_InitDAT(0x39); 
LCDSPI_InitCMD(0x54);LCDSPI_InitDAT(0x39); 
LCDSPI_InitCMD(0x55);LCDSPI_InitDAT(0x3F); 
LCDSPI_InitCMD(0x56);LCDSPI_InitDAT(0x0D); 
LCDSPI_InitCMD(0x57);LCDSPI_InitDAT(0x74); 
LCDSPI_InitCMD(0x58);LCDSPI_InitDAT(0x03); 
LCDSPI_InitCMD(0x59);LCDSPI_InitDAT(0x0C); 
LCDSPI_InitCMD(0x5A);LCDSPI_InitDAT(0x0C); 
LCDSPI_InitCMD(0x5B);LCDSPI_InitDAT(0x10); 
LCDSPI_InitCMD(0x5C);LCDSPI_InitDAT(0x18); 
LCDSPI_InitCMD(0x5D);LCDSPI_InitDAT(0xCC); 

//Power Voltage Setting 
LCDSPI_InitCMD(0x1B);LCDSPI_InitDAT(0x18);
LCDSPI_InitCMD(0x25);LCDSPI_InitDAT(0x18);
LCDSPI_InitCMD(0x1A);LCDSPI_InitDAT(0x04);

//VCOM 
LCDSPI_InitCMD(0x23);LCDSPI_InitDAT(0x53);//0x55

//Power on Setting 
LCDSPI_InitCMD(0x18);LCDSPI_InitDAT(0x30);
LCDSPI_InitCMD(0x19);LCDSPI_InitDAT(0x01);
WaitTime(1);
//LCDSPI_InitCMD(0x1C);LCDSPI_InitDAT(0x03);
WaitTime(1);
LCDSPI_InitCMD(0x01);LCDSPI_InitDAT(0x00);
LCDSPI_InitCMD(0x1F);LCDSPI_InitDAT(0xAC); 
WaitTime(5); 
LCDSPI_InitCMD(0x1F);LCDSPI_InitDAT(0xA4); 
WaitTime(5); 
LCDSPI_InitCMD(0x1F);LCDSPI_InitDAT(0xB4); 
WaitTime(5); 
LCDSPI_InitCMD(0x1F);LCDSPI_InitDAT(0xF4); 
WaitTime(5); 
LCDSPI_InitCMD(0x1F);LCDSPI_InitDAT(0xD4); 
WaitTime(5); 

//262k/65k color selection 
LCDSPI_InitCMD(0x17);LCDSPI_InitDAT(0x66);

//SET PANEL 
LCDSPI_InitCMD(0x36);LCDSPI_InitDAT(0x09);
LCDSPI_InitCMD(0x2F);LCDSPI_InitDAT(0x31);

//Display ON Setting 
LCDSPI_InitCMD(0x28);LCDSPI_InitDAT(0x38);
WaitTime(40); 
LCDSPI_InitCMD(0x28);LCDSPI_InitDAT(0x3C);

LCDSPI_InitCMD(0x31);LCDSPI_InitDAT(0x02);
LCDSPI_InitCMD(0x32);LCDSPI_InitDAT(0x80);
LCDSPI_InitCMD(0x33);LCDSPI_InitDAT(0x02);
LCDSPI_InitCMD(0x34);LCDSPI_InitDAT(0x02);

//Set GRAM Area 

LCDSPI_InitCMD(0x22);             


}

#if 0
static void lcdc_hx8347_sleep_out(void)
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

static void lcdc_hx8347_init(void)
{
#if 0
	gpio_set_value(lcd_reset, 1);
	mdelay(10); 
	gpio_set_value(lcd_reset, 0);
	mdelay(20); 
	gpio_set_value(lcd_reset, 1);
	mdelay(20); 

#if 1
 //HX8347-H
LCDSPI_InitCMD(0xEA);LCDSPI_InitDAT(0x00); 
LCDSPI_InitCMD(0xEB);LCDSPI_InitDAT(0x00); 
LCDSPI_InitCMD(0xEC);LCDSPI_InitDAT(0x3C); 
LCDSPI_InitCMD(0xED);LCDSPI_InitDAT(0xC4); 
LCDSPI_InitCMD(0xE8);LCDSPI_InitDAT(0x50);  
LCDSPI_InitCMD(0xE9);LCDSPI_InitDAT(0x38); 
LCDSPI_InitCMD(0xF1);LCDSPI_InitDAT(0x01); 
LCDSPI_InitCMD(0xF2);LCDSPI_InitDAT(0x08); 

//Gamma 2.2 Setting         
LCDSPI_InitCMD(0x40);LCDSPI_InitDAT(0x00); 
LCDSPI_InitCMD(0x41);LCDSPI_InitDAT(0x06); 
LCDSPI_InitCMD(0x42);LCDSPI_InitDAT(0x06); 
LCDSPI_InitCMD(0x43);LCDSPI_InitDAT(0x2D); 
LCDSPI_InitCMD(0x44);LCDSPI_InitDAT(0x2F); 
LCDSPI_InitCMD(0x45);LCDSPI_InitDAT(0x3F); 
LCDSPI_InitCMD(0x46);LCDSPI_InitDAT(0x0B); 
LCDSPI_InitCMD(0x47);LCDSPI_InitDAT(0x72); 
LCDSPI_InitCMD(0x48);LCDSPI_InitDAT(0x07); 
LCDSPI_InitCMD(0x49);LCDSPI_InitDAT(0x0F); 
LCDSPI_InitCMD(0x4A);LCDSPI_InitDAT(0x13); 
LCDSPI_InitCMD(0x4B);LCDSPI_InitDAT(0x13); 
LCDSPI_InitCMD(0x4C);LCDSPI_InitDAT(0x1C); 

LCDSPI_InitCMD(0x50);LCDSPI_InitDAT(0x00); 
LCDSPI_InitCMD(0x51);LCDSPI_InitDAT(0x10); 
LCDSPI_InitCMD(0x52);LCDSPI_InitDAT(0x12); 
LCDSPI_InitCMD(0x53);LCDSPI_InitDAT(0x39); 
LCDSPI_InitCMD(0x54);LCDSPI_InitDAT(0x39); 
LCDSPI_InitCMD(0x55);LCDSPI_InitDAT(0x3F); 
LCDSPI_InitCMD(0x56);LCDSPI_InitDAT(0x0D); 
LCDSPI_InitCMD(0x57);LCDSPI_InitDAT(0x74); 
LCDSPI_InitCMD(0x58);LCDSPI_InitDAT(0x03); 
LCDSPI_InitCMD(0x59);LCDSPI_InitDAT(0x0C); 
LCDSPI_InitCMD(0x5A);LCDSPI_InitDAT(0x0C); 
LCDSPI_InitCMD(0x5B);LCDSPI_InitDAT(0x10); 
LCDSPI_InitCMD(0x5C);LCDSPI_InitDAT(0x18); 
LCDSPI_InitCMD(0x5D);LCDSPI_InitDAT(0xCC); 

//Power Voltage Setting 
LCDSPI_InitCMD(0x1B);LCDSPI_InitDAT(0x18);
LCDSPI_InitCMD(0x25);LCDSPI_InitDAT(0x18);
LCDSPI_InitCMD(0x1A);LCDSPI_InitDAT(0x04);

//VCOM 
LCDSPI_InitCMD(0x23);LCDSPI_InitDAT(0x53);//0x55

//Power on Setting 
LCDSPI_InitCMD(0x18);LCDSPI_InitDAT(0x30);
LCDSPI_InitCMD(0x19);LCDSPI_InitDAT(0x01);
WaitTime(1);
//LCDSPI_InitCMD(0x1C);LCDSPI_InitDAT(0x03);
WaitTime(1);
LCDSPI_InitCMD(0x01);LCDSPI_InitDAT(0x00);
LCDSPI_InitCMD(0x1F);LCDSPI_InitDAT(0xAC); 
WaitTime(5); 
LCDSPI_InitCMD(0x1F);LCDSPI_InitDAT(0xA4); 
WaitTime(5); 
LCDSPI_InitCMD(0x1F);LCDSPI_InitDAT(0xB4); 
WaitTime(5); 
LCDSPI_InitCMD(0x1F);LCDSPI_InitDAT(0xF4); 
WaitTime(5); 
LCDSPI_InitCMD(0x1F);LCDSPI_InitDAT(0xD4); 
WaitTime(5); 

//262k/65k color selection 
LCDSPI_InitCMD(0x17);LCDSPI_InitDAT(0x66);

//SET PANEL 
LCDSPI_InitCMD(0x36);LCDSPI_InitDAT(0x09);
LCDSPI_InitCMD(0x2F);LCDSPI_InitDAT(0x31);

//Display ON Setting 
LCDSPI_InitCMD(0x28);LCDSPI_InitDAT(0x38);
WaitTime(40); 
LCDSPI_InitCMD(0x28);LCDSPI_InitDAT(0x3C);

LCDSPI_InitCMD(0x31);LCDSPI_InitDAT(0x02);
LCDSPI_InitCMD(0x32);LCDSPI_InitDAT(0x80);
LCDSPI_InitCMD(0x33);LCDSPI_InitDAT(0x02);
LCDSPI_InitCMD(0x34);LCDSPI_InitDAT(0x02);

//Set GRAM Area 

LCDSPI_InitCMD(0x22);             
#endif
#endif

}
static int lcd_on_flag = 0;
static int lcdc_hx8347_panel_on(struct platform_device *pdev)
{	
	down(&hx8347_state.sem);

	if (!hx8347_state.disp_initialized) {
		hx8347_state.disp_initialized = TRUE;
		lcdc_hx8347_init();
	}
	else{	
		///lcdc_hx8347_sleep_out();
		hx8347_disp_on();
		lcd_on_flag = 1;
	}
	
	printk("lcdc_hx8347_panel_on end!\n");	
	up(&hx8347_state.sem);
	return 0;
}

static void hx8347_disp_off(void)
{
	#if 0
//disp off
	spi_write_cmd(0x28);
	spi_write_data(0x38);  //GON='1' DTE='1' D[1:0]='10' 
	lcdc_mdelay(40); 
	spi_write_cmd(0x28);
	spi_write_data(0x20);  //GON='1' DTE='0' D[1:0]='00' 
	
	#endif
// sleep in
	spi_write_cmd(0x28);
	spi_write_data(0x38);
	lcdc_mdelay(40);	
	
	spi_write_cmd(0x28);
	spi_write_data(0x20);
	lcdc_mdelay(5);
	

	spi_write_cmd(0x1F);
	spi_write_data(0xA9);
	lcdc_mdelay(5);
	
	spi_write_cmd(0x19);
	spi_write_data(0x00);
	
}

static int lcdc_hx8347_panel_off(struct platform_device *pdev)
{
	down(&hx8347_state.sem);
	hx8347_disp_off();
	up(&hx8347_state.sem);
	return 0;
}

#define LCDC_MAX_LEVEL	8

//#define SGM3727_BACKLIGHT
#ifdef SGM3727_BACKLIGHT

#define BACKLIGHT_GPIO_ID  28 
#define BACKLIGHT_GPIO 17
#define SGM3727_MAX_LEVEL 	32
#define DEFAULT_BL_LEVEL	15 

static spinlock_t prism_lock=SPIN_LOCK_UNLOCKED;
static unsigned char sgm3727_bl_level[LCDC_MAX_LEVEL+1]={0, 4, 8, 12, 16, 20, 24, 28, 32};
static int bl_init=0;
static int bl_id=0;

static void sgm3727_set_backlight(int  bl_level)
{
	static int  prev_pwm = 0xff;
	int  pwm_num = 0;
	int cur_pwm = 0;
	int  i = 0;
	unsigned long  flags = 0;
	int level=0;

	level= bl_level;
	if (level > SGM3727_MAX_LEVEL)
		level = SGM3727_MAX_LEVEL;

	if (!level) 
	{
		gpio_set_value(BACKLIGHT_GPIO, 0);
		mdelay(3);
		prev_pwm = 0;
	} 
	else
	{
		cur_pwm = SGM3727_MAX_LEVEL + 1 - level;
		if (prev_pwm == 0xff) 
		{
			if (cur_pwm < (SGM3727_MAX_LEVEL + 1 - DEFAULT_BL_LEVEL))
				pwm_num = SGM3727_MAX_LEVEL + cur_pwm - (SGM3727_MAX_LEVEL + 1 - DEFAULT_BL_LEVEL);	
			else
				pwm_num = cur_pwm - (SGM3727_MAX_LEVEL + 1 - DEFAULT_BL_LEVEL);
		}
		else
		{
			if (cur_pwm < prev_pwm)
				pwm_num = SGM3727_MAX_LEVEL + cur_pwm - prev_pwm;	
			else
				pwm_num = cur_pwm - prev_pwm;
		}
		for (i = 0; i < pwm_num; i++)
		{
			spin_lock_irqsave(&prism_lock, flags);
			gpio_set_value(BACKLIGHT_GPIO, 0);
			udelay(10);
			gpio_set_value(BACKLIGHT_GPIO, 1);
			udelay(32);	
			spin_unlock_irqrestore(&prism_lock, flags);		
		}	
		prev_pwm = cur_pwm;	
		
	}
	
}
#endif

//#define LCDC_MAX_LEVEL	8
static unsigned char lcdc_bl_level[LCDC_MAX_LEVEL+1]={0, 10, 30, 50, 65, 75, 82, 92, 100};

extern int pmic_set_led_intensity(enum ledtype type, int level);
static void lcdc_hx8347_set_backlight(struct msm_fb_data_type *mfd)
{
	
	int bl_level;
	int ret = -EPERM;
	
	down(&hx8347_state.sem);
#ifdef SGM3727_BACKLIGHT
	if (bl_id)
	{	
		if (bl_init == 0)
		{
			pmapp_disp_backlight_init();
			ret = pmapp_disp_backlight_set_brightness(0);
			if(ret)
				printk(KERN_WARNING "%s: can't set lcd backlight!\n",
						__func__);
			bl_init=1;
		}
		
		bl_level=sgm3727_bl_level[mfd->bl_level];
		sgm3727_set_backlight(bl_level);
	}
	else
	{
		if (bl_init == 0)
		{
			sgm3727_set_backlight(0);
			bl_init=1;
		}
		
		bl_level = lcdc_bl_level[mfd->bl_level];
		//printk("lcdc_hx8347_set_backlight:	mfd->bl_level = %d, bl_level = %d\n", mfd->bl_level, bl_level);
		pmapp_disp_backlight_init();
		ret = pmapp_disp_backlight_set_brightness(bl_level);
		if(ret)
			printk(KERN_WARNING "%s: can't set lcd backlight!\n",
					__func__);
       }
#else
	bl_level = lcdc_bl_level[mfd->bl_level];
	printk("lcdc_hx8347_set_backlight:	mfd->bl_level = %d, bl_level = %d\n", mfd->bl_level, bl_level);
	printk("lcd_on_flag = %d\n", lcd_on_flag);
	if( (mfd->bl_level <= 2) && (1 == lcd_on_flag)){
		mdelay(30);
		lcd_on_flag = 0;
		printk("delay set _backlight\n");
	}
	//printk("lcdc_hx8347_set_backlight:	mfd->bl_level = %d, bl_level = %d\n", mfd->bl_level, bl_level);
	pmapp_disp_backlight_init();
	ret = pmapp_disp_backlight_set_brightness(bl_level);
	if(ret)
		printk(KERN_WARNING "%s: can't set lcd backlight!\n",
				__func__);

#endif
	up(&hx8347_state.sem);

	 
}

static int __devinit hx8347_probe(struct platform_device *pdev)
{
	printk("hx8347_probe %d\n",__LINE__);
	if (pdev->id == 0) {
		lcdc_hx8347_pdata = pdev->dev.platform_data;
		return 0;
	}
	msm_fb_add_device(pdev);
	
	return 0;
}

static struct platform_driver this_driver = {
	.probe  = hx8347_probe,
	.driver = {
		.name   = "lcdc_hx8347_fwvga_pt",
	},
};

static struct msm_fb_panel_data hx8347_panel_data = {
	.on = lcdc_hx8347_panel_on,
	.off = lcdc_hx8347_panel_off,
	.set_backlight = lcdc_hx8347_set_backlight,
};

static struct platform_device this_device = {
	.name   = "lcdc_hx8347_fwvga_pt",
	.id	= 1,
	.dev	= {
		.platform_data = &hx8347_panel_data,
	}
};

static int __init lcdc_hx8347_panel_init(void)
{
	int ret ,rc;
	struct msm_panel_info *pinfo;

	printk("lcdc_hx8347_panel_init:	start\n");
	printk("lcdc_hx8347_panel_init:	tinboost_lcm_id = %d\n", tinboost_lcm_id);

	if(TRULY_HIMAX_HX8347!= tinboost_lcm_id){
		printk("this is not TRULY_HIMAX\n");
		ret = 0;
		goto exit_lcm_id_failed;
	}
	
	lcd_reset = 88;
	spi_cs = 93;
	spi_sclk = 92;
	spi_sdi = 90;
	//lcd_id = 124;

	gpio_request(lcd_reset, "lcd_reset");
	gpio_request(spi_cs, "spi_cs");
	gpio_request(spi_sclk, "spi_sclk");
	 rc = gpio_tlmm_config(GPIO_CFG(spi_sclk, 0,GPIO_CFG_OUTPUT,
                            GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	  printk("8347 gpio_request(%d) ",spi_sclk);
	gpio_request(spi_sdi, "spi_sdi");
	//gpio_request(lcd_id, "lcd_id");

	gpio_direction_output(lcd_reset, 1);
	gpio_direction_output(spi_cs, 1);
	
	gpio_direction_output(spi_sclk, 1);
	rc=gpio_get_value(spi_sclk);
	printk("dddddddd ===%d++++",rc);
	gpio_direction_output(spi_sdi, 1);
	//gpio_direction_input(lcd_id);
#ifdef SGM3727_BACKLIGHT
	gpio_request(BACKLIGHT_GPIO, "lcd bl");
	gpio_tlmm_config(GPIO_CFG(BACKLIGHT_GPIO, 0, 
					GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, 
					GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_request(BACKLIGHT_GPIO_ID, "lcd bl id");
	gpio_tlmm_config(GPIO_CFG(BACKLIGHT_GPIO_ID, 0, 
					GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, 
					GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	bl_id=gpio_get_value(BACKLIGHT_GPIO_ID);
#endif
	
	ret = platform_driver_register(&this_driver);
	if (ret)
		goto exit_register_driver_failed;

	pinfo = &hx8347_panel_data.panel_info;

	hx8347_state.disp_initialized = false;
	sema_init(&hx8347_state.sem, 1);

	pinfo->xres = 240;
	pinfo->yres = 320;
	pinfo->type = LCDC_PANEL;
	pinfo->pdest = DISPLAY_1;
	pinfo->wait_cycle = 0;
	pinfo->bpp = 18;
	pinfo->fb_num = 2;
	pinfo->clk_rate = 7500000;
	pinfo->bl_max = LCDC_MAX_LEVEL;
	pinfo->bl_min = 0;

	pinfo->lcdc.h_back_porch = 20;		//20;  		
	pinfo->lcdc.h_front_porch = 20;    		//10
	pinfo->lcdc.h_pulse_width = 10;  		//10
	pinfo->lcdc.v_back_porch = 20;
	pinfo->lcdc.v_front_porch = 6;
	pinfo->lcdc.v_pulse_width = 20;
	pinfo->lcdc.border_clr = 0;     /* blk */
	pinfo->lcdc.underflow_clr = 0; //0xff;       /* blue */
	pinfo->lcdc.hsync_skew = 0;
	ret = platform_device_register(&this_device);
	if (ret)
		goto exit_register_device_failed;

	printk("lcdc_hx8347_panel_init:	end\n");

	return 0;

exit_register_device_failed:
	platform_driver_unregister(&this_driver);
exit_register_driver_failed:
	gpio_free(spi_cs);
	gpio_free(spi_sclk);
	gpio_free(spi_sdi);
	gpio_free(lcd_reset);
exit_lcm_id_failed:
	return ret;
}

module_init(lcdc_hx8347_panel_init);
