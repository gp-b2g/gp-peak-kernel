/*==================================================================
		FLASH CONTROLED BY GPIO

Project: C8680
Decription: Enable flash controlled through gpio
History: zhuangxiaojian	create	2012-06-11
===================================================================*/
#include <linux/gpio.h>
#include <linux/leds-sgm3141.h>
#include <linux/delay.h>

int sgm3141_set_flash_type(unsigned type)
{
	gpio_tlmm_config(GPIO_CFG(SGM3141_FLASH_EN, 0, GPIO_CFG_OUTPUT, 
					GPIO_CFG_NO_PULL, GPIO_CFG_8MA),GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(SGM3141_FLASH_NOW, 0, GPIO_CFG_OUTPUT, 
					GPIO_CFG_NO_PULL, GPIO_CFG_8MA),GPIO_CFG_ENABLE);
	
	printk("##### %s: type = %d",__func__,type);	
	switch(type){
		case SGM3141_FLASH_OFF:
			gpio_set_value(SGM3141_FLASH_EN, 0);
			gpio_set_value(SGM3141_FLASH_NOW, 0);
			break;
		case SGM3141_FLASH_LOW:
			gpio_set_value(SGM3141_FLASH_EN, 1);
			gpio_set_value(SGM3141_FLASH_NOW, 0);
			break;
		case SGM3141_FLASH_HIGH:
			gpio_set_value(SGM3141_FLASH_EN, 1);
			gpio_set_value(SGM3141_FLASH_NOW, 1);
			msleep(100);
			break;
		default:
			return -EINVAL;
			break;
	}
	
	return 0;
}
