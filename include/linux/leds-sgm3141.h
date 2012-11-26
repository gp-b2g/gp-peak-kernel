/*==================================================================
		FLASH CONTROLED BY GPIO

Project: C8680
Decription: Enable flash controlled through gpio
History: zhuangxiaojian	create	2012-06-11
===================================================================*/
#ifndef _SGM3141_H_
#define _SGM3141_H_


#define SGM3141_FLASH_EN 13
#define SGM3141_FLASH_NOW 32

#define SGM3141_FLASH_OFF 0
#define SGM3141_FLASH_LOW 1
#define SGM3141_FLASH_HIGH 2

int sgm3141_set_flash_type(unsigned type);

#endif
