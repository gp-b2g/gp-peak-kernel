/*
 * Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
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

#ifndef __ARCH_ARM_MACH_MSM_BOARD_7X27A_REGULATOR_H__
#define __ARCH_ARM_MACH_MSM_BOARD_7X27A_REGULATOR_H__

#include <linux/regulator/gpio-regulator.h>
#include "proccomm-regulator.h"

#define MSM7X27A_GPIO_VREG_ID_EXT_2V8	0
#define MSM7X27A_GPIO_VREG_ID_EXT_1V8	1

extern struct gpio_regulator_platform_data msm7x27a_evb_gpio_regulator_pdata[];
extern struct gpio_regulator_platform_data msm7x27a_sku3_gpio_regulator_pdata[];
extern struct gpio_regulator_platform_data msm7x27a_sku7_gpio_regulator_pdata[];

extern struct proccomm_regulator_platform_data msm7x27a_proccomm_regulator_data;

#endif
