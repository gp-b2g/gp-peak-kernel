/*=================================================
        LED FLASH MODULE

Project: C8681
Decription: tps61310 driver head file
History: zhuangxiaojian create  2012-08-30
=================================================*/
#ifndef _TPS61310_H_
#define _TPS61310_H_

#define MSM_LED_STATE_OFF 0
#define MSM_LED_STATE_LOW 1
#define MSM_LED_STATE_HIGH 2

#define GPIO_CAM_FLASH_NOW 10
#define GPIO_CAM_FLASH_CTRL_EN 9

int tps61310_config(unsigned led_state);

#endif
