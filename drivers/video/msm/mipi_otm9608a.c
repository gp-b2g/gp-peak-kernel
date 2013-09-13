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
#include "mipi_otm9608a.h"

//Fix white noise and recovery black screen issue.
extern bool kernel_booted;

static struct msm_panel_common_pdata *mipi_otm9608a_pdata;
static struct dsi_buf otm9608a_tx_buf;
static struct dsi_buf otm9608a_rx_buf;
spinlock_t otm9608a_spin_lock;

static char cmd0[] = {
	0xFF, 0x96, 0x08, 0x01,
};
static char cmd1[] = {
	0x00, 0x80,
};
static char cmd2[] = {
	0xFF, 0x96, 0x08,

};
static char cmd3[] = {
	0x00, 0x00,
};
static char cmd4[]={
    0xA0, 0x00,
};
static char cmd5[] = {
	0x00, 0x80,
};
static char cmd6[] = {
	0xB3, 0x00, 0x00, 0x20,
	0x00, 0x00,
};
static char cmd7[] = {
	0x00, 0xC0,
};
static char cmd8[] = {
	0xB3, 0x09,
};
static char cmd9[] = {
	0x00, 0xa0,
};
static char cmd10[] = {
	0x00, 0x80,
};
static char cmd11[] = {
	0xC0, 0x00, 0x48, 0x00,
	0x10, 0x10, 0x00, 0x47,
	0x10, 0x10,
};
static char cmd12[] = {
	0x00, 0x92,
};
static char cmd13[] = {
	0xC0, 0x00, 0x10, 0x00,
	0x13,
};
static char cmd14[] = {
	0x00, 0xA2,
};
static char cmd15[] = {
	0xC0, 0x0C, 0x05, 0x02,
};
static char cmd16[] = {
	0x00,0xB3,
};
static char cmd17[] = {
	0xC0, 0x00, 0x50,
};
static char cmd18[] = {
	0x00, 0x81,
};
static char cmd19[] = {
	0xC1, 0x55,
};
static char cmd20[] = {
	0x00, 0x80,
};
static char cmd21[] = {
	0xC4, 0x00, 0x84, 0xFC,
};
static char cmd22[] = {
	0x00, 0xA0,
};
static char cmd23[] = {
	0xB3, 0x10, 0x00,
};
static char cmd24[] = {
	0x00, 0xA0,
};
static char cmd25[] = {
	0xC0, 0x00,
};
static char cmd26[] = {
	0x00, 0xA0,
};
static char cmd27[] = {
	0xC4, 0x33, 0x09, 0x90,
	0x2B, 0x33, 0x09, 0x90,
	0x54,
};
static char cmd28[] = {
	0x00, 0x80,
};
static char cmd29[] = {
	0xC5, 0x08, 0x00, 0xA0,
	0x11,
};
static char cmd30[] = {
	0x00, 0x90,
};
static char cmd31[] = {
	0xC5, 0x96, 0x57, 0x01,
	0x57, 0x33, 0x33, 0x34,
};
static char cmd32[] = {
	0x00, 0xA0,
};
static char cmd33[] = {
	0xC5, 0x96, 0x57, 0x00,
	0x57, 0x33, 0x33, 0x34,
};
static char cmd34[] = {
	0x00, 0xB0,
};
static char cmd35[] = {
	0xC5, 0x04, 0xAC, 0x01,
	0x00, 0x71, 0xB1, 0x83,
};
static char cmd36[] = {
	0x00, 0x00,
};
static char cmd37[] = {
	0xD9, 0x61,
};
static char cmd38[] = {
	0x00, 0x80,
};
static char cmd39[] = {
	0xC6, 0x64,
};
static char cmd40[] = {
	0x00, 0xB0,
};
static char cmd41[] = {
	0xC6, 0x03, 0x10, 0x00,
	0x1F, 0x12,
};
static char cmd42[] = {
	0x00, 0x00,
};
static char cmd43[] = {
	0xD0, 0x40,
};
static char cmd44[] = {
	0x00, 0x00,
};
static char cmd45[] = {
	0xD1, 0x00, 0x00,
};
static char cmd46[] = {
	0x00, 0xB7,
};
static char cmd47[] = {
	0xB0, 0x10,
};
static char cmd48[] = {
	0x00, 0xC0,
};
static char cmd49[] = {
	0xB0, 0x55,
};
static char cmd50[] = {
	0x00, 0xB1,
};
static char cmd51[] = {
	0xB0, 0x03,
};
static char cmd52[] = {
	0x00, 0x81,
};
static char cmd53[] = {
	0xD6, 0x00,
};
static char cmd54[] = {
	0x00, 0x00,
};
static char cmd55[] = {
	0xe1, 0x01, 0x0D, 0x13,
	0x0F, 0x07, 0x11, 0x0B,
	0x0A, 0x03, 0x06, 0x0B,
	0x08, 0x0D, 0x0E, 0x09,
	0x01,
};
static char cmd56[] = {
	0x00, 0x00,
};
static char cmd57[] = {
	0xe2, 0x02, 0x0F, 0x15,
	0x0E, 0x08, 0x10, 0x0B,
	0x0C, 0x02, 0x04, 0x0B,
	0x04, 0x0E, 0x0D, 0x08,
	0x00,
};
static char cmd58[] = {
	0x00, 0x80,
};
static char cmd59[] = {
	0xCB, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00,
};
static char cmd60[] = {
	0x00, 0x90,
};
static char cmd61[] = {
	0xCB, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
};
static char cmd62[] = {
	0x00, 0xA0,
};
static char cmd63[] = {
	0xCB, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
};
static char cmd64[] = {
	0x00, 0xB0,
};
static char cmd65[] = {
	0xCB, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00,
};
static char cmd66[] = {
	0x00, 0xC0,
};
static char cmd67[] = {
	0xCB, 0x04, 0x04, 0x04,
	0x04, 0x08, 0x04, 0x08,
	0x04, 0x08, 0x04, 0x08,
	0x04, 0x04, 0x04, 0x08,
};
static char cmd68[] = {
	0x00, 0xD0,
};
static char cmd69[] = {
	0xCB, 0x08, 0x00, 0x00,
	0x00, 0x00, 0x04, 0x04,
	0x04, 0x04, 0x08, 0x04,
	0x08, 0x04, 0x08, 0x04,
};
static char cmd70[] = {
	0x00, 0xE0,
};
static char cmd71[] = {
	0xCB, 0x08, 0x04, 0x04,
	0x04, 0x08, 0x08, 0x00,
	0x00, 0x00, 0x00, 
};
static char cmd72[] = {
	0x00, 0xF0,
};
static char cmd73[] = {
	0xCB, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF,
};
static char cmd74[] = {
	0x00, 0x80,
};
static char cmd75[] = {
	0xCC, 0x26, 0x25, 0x23,
	0x24, 0x00, 0x0F, 0x00,
	0x0D, 0x00, 0x0B,
};
static char cmd76[] = {
	0x00, 0x90,
};
static char cmd77[] = {
	0xCC, 0x00, 0x09, 0x01,
	0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x26,
	0x25, 0x21, 0x22, 0x00,
};
static char cmd78[] = {
	0x00, 0xA0,
};

static char cmd79[] = {
	0xCC, 0x10, 0x00, 0x0E,
	0x00, 0x0C, 0x00, 0x0A,
	0x02, 0x04, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
};
static char cmd80[] = {
	0x00, 0xB0,
};
static char cmd81[] = {
	0xCC, 0x25, 0x26, 0x21,
	0x22, 0x00, 0x0A, 0x00,
	0x0C, 0x00, 0x0E,
};
static char cmd82[] = {
	0x00, 0xC0,
};
static char cmd83[] = {
	0xCC, 0x00, 0x10,
	0x04, 0x02, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x25, 0x26, 0x23, 0x24,
	0x00,
};
static char cmd84[] = {
	0x00, 0xD0,
};
static char cmd85[] = {
	0xCC, 0x09, 0x00, 0x0B,
	0x00, 0x0D, 0x00, 0x0F,
	0x03, 0x01, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
};
static char cmd86[] = {
	0x00, 0x80,
};
static char cmd87[] = {
	0xCE, 0x8A, 0x03, 0x06,
	0x89, 0x03, 0x06, 0x88,
	0x03, 0x06, 0x87, 0x03,
	0x06,
};
static char cmd88[] = {
	0x00, 0x90,
};
static char cmd89[] = {
	0xCE, 0xF0, 0x00, 0x00,
	0xF0, 0x00, 0x00, 0xF0,
	0x00, 0x00, 0xF0, 0x00,
	0x00, 0x00, 0x00, 
};
static char cmd90[] = {
	0x00, 0xA0,
};
static char cmd91[] = {
	0xCE, 0x38, 0x02, 0x03,
	0xC1, 0x00, 0x06, 0x00,
	0x38, 0x01, 0x03, 0xC2,
	0x00, 0x06, 0x00,
};
static char cmd92[] = {
	0x00, 0xB0,
};
static char cmd93[] = {
	0xCE, 0x38, 0x00, 0x03,
	0xC3, 0x00, 0x06, 0x00,
	0x30, 0x00, 0x03, 0xC4,
	0x00, 0x06, 0x00,
};
static char cmd94[] = {
	0x00,0xC0,
};
static char cmd95[] = {
	0xCE, 0x38, 0x06, 0x03,
	0xBD, 0x00, 0x06, 0x00,
	0x38, 0x05, 0x03, 0xBE,
	0x00, 0x06, 0x00,
};
static char cmd96[] = {
	0x00, 0xD0,
};
static char cmd97[] = {
	0xCE, 0x38, 0x04, 0x03,
	0xBF, 0x00, 0x06, 0x00,
	0x38, 0x03, 0x03, 0xC0,
	0x00, 0x06, 0x00,
};
static char cmd98[] = {
	0x00, 0x80,
};
static char cmd99[] = {
	0xCF, 0xF0, 0x00, 0x00,
	0x10, 0x00, 0x00, 0x00,
	0xF0, 0x00, 0x00, 0x10,
	0x00, 0x00, 0x00,
};
static char cmd100[] = {
	0x00, 0x90,
};
static char cmd101[] = {
	0xCF, 0xF0, 0x00, 0x00,
	0x10, 0x00, 0x00, 0x00,
	0xF0, 0x00, 0x00, 0x10,
	0x00, 0x00, 0x00,
};
static char cmd102[] = {
	0x00, 0xA0,
};
static char cmd103[] = {
	0xCF, 0xF0, 0x00, 0x00,
	0x10, 0x00, 0x00, 0x00,
	0xF0, 0x00, 0x00, 0x10,
	0x00, 0x00, 0x00,
};
static char cmd104[] = {
	0x00, 0xB0,
};
static char cmd105[] = {
	0xCF, 0xF0, 0x00, 0x00,
	0x10, 0x00, 0x00, 0x00,
	0xF0, 0x00, 0x00, 0x10,
	0x00, 0x00, 0x00,
};
static char cmd106[] = {
	0x00, 0xC0,
};
static char cmd107[] = {
	0xCF, 0x02, 0x02, 0x20,
	0x20, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x02,
};
static char cmd108[] = {
	0x00, 0x00,
};
static char cmd109[] = {
	0xD8, 0xa7, 0xa7,
};
static char cmd110[] = {
	0x11,

};
static char cmd111[] = {
	0x29,
};

static char cmd112[] = {
	0x36, 0xD0,
};

static char cmd113[] = {
	0x35, 0x00,
};

static char cmd114[] = {
	0x44, 0x03, 0xC0,
};

static char display_bringtness[2] = {
	0x51, 0xff,
};
static char crtl_display[2] = {
	0x53, 0x2c,
};
static char cabc[2] = {
	0x55, 0x02,
};

static struct dsi_cmd_desc otm9608a_cmd_display_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd0), cmd0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd1), cmd1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd2), cmd2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd3), cmd3},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10, sizeof(cmd4), cmd4},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd5), cmd5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd6), cmd6},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd7), cmd7},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd8), cmd8},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd9), cmd9},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd10), cmd10},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd11), cmd11},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd12), cmd12},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd13), cmd13},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd14), cmd14},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd15), cmd15},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd16), cmd16},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd17), cmd17},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd18), cmd18},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd19), cmd19},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd20), cmd20},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd21), cmd21},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd22), cmd22},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd23), cmd23},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd24), cmd24},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd25), cmd25},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd26), cmd26},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd27), cmd27},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd28), cmd28},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd29), cmd29},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd30), cmd30},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd31), cmd31},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd32), cmd32},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd33), cmd33},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd34), cmd34},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd35), cmd35},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd36), cmd36},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd37), cmd37},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd38), cmd38},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd39), cmd39},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd40), cmd40},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd41), cmd41},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd42), cmd42},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd43), cmd43},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd44), cmd44},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd45), cmd45},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd46), cmd46},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd47), cmd47},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd48), cmd48},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd49), cmd49},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd50), cmd50},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd51), cmd51},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd52), cmd52},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd53), cmd53},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd54), cmd54},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd55), cmd55},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd56), cmd56},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd57), cmd57},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd58), cmd58},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd59), cmd59},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd60), cmd60},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd61), cmd61},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd62), cmd62},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd63), cmd63},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd64), cmd64},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd65), cmd65},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd66), cmd66},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd67), cmd67},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd68), cmd68},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd69), cmd69},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd70), cmd70},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd71), cmd71},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd72), cmd72},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd73), cmd73},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd74), cmd74},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd75), cmd75},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd76), cmd76},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd77), cmd77},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd78), cmd78},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd79), cmd79},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd80), cmd80},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd81), cmd81},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd82), cmd82},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd83), cmd83},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd84), cmd84},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd85), cmd85},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd86), cmd86},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd87), cmd87},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd88), cmd88},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd89), cmd89},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd90), cmd90},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd91), cmd91},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd92), cmd92},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd93), cmd93},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd94), cmd94},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd95), cmd95},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd96), cmd96},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd97), cmd97},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd98), cmd98},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd99), cmd99},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd100), cmd100},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd101), cmd101},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd102), cmd102},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd103), cmd103},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd104), cmd104},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd105), cmd105},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd106), cmd106},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd107), cmd107},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,	sizeof(cmd108), cmd108},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,	sizeof(cmd109), cmd109},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(cmd110), cmd110},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(cmd111), cmd111},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd112), cmd112},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd113), cmd113},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd114), cmd114},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_bringtness), display_bringtness},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(crtl_display), crtl_display},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cabc), cabc},
};

static char display_off[2] = {0x28, 0x00};
static char enter_sleep[2] = {0x10, 0x00};

static struct dsi_cmd_desc otm9608a_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(enter_sleep), enter_sleep}
};

#define MAX_BL_LEVEL 32
#define LCD_BL_EN 96
static void mipi_otm9608a_set_backlight(struct msm_fb_data_type *mfd)
{
	int level = mfd->bl_level;
	int max = mfd->panel_info.bl_max;
	int min = mfd->panel_info.bl_min;
	unsigned long flags;
	int i;

	printk("%s, level = %d\n", __func__, level);

	//Fix white noise and recovery black screen issue.
	if (!kernel_booted)
		kernel_booted = true;

	spin_lock_irqsave(&otm9608a_spin_lock, flags); //disable local irq and preemption
	if (level < min)
		level = min;
	if (level > max)
		level = max;

	if (level == 0) {
		gpio_set_value(LCD_BL_EN, 0);
		spin_unlock_irqrestore(&otm9608a_spin_lock, flags);
		return;
	}

	for (i = 0; i < (MAX_BL_LEVEL - level + 1); i++) {
		gpio_set_value(LCD_BL_EN, 0);
		udelay(1);
		gpio_set_value(LCD_BL_EN, 1);
		udelay(1);
	}
	spin_unlock_irqrestore(&otm9608a_spin_lock, flags);

	return;
}

static int mipi_otm9608a_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	printk("%s: Enter\n", __func__);

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi_dsi_cmds_tx(mfd, &otm9608a_tx_buf,
		otm9608a_cmd_display_on_cmds,
		ARRAY_SIZE(otm9608a_cmd_display_on_cmds));

	printk("%s: Done\n", __func__);
	return 0;
}

static int mipi_otm9608a_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	printk("%s: Enter\n", __func__);
	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi_dsi_cmds_tx(mfd, &otm9608a_tx_buf,
		otm9608a_display_off_cmds,
		ARRAY_SIZE(otm9608a_display_off_cmds));


	printk("%s: Done\n", __func__);
	return 0;
}

static int __devinit mipi_otm9608a_lcd_probe(struct platform_device *pdev)
{
	printk("%s: Enter\n", __func__);
	if (pdev->id == 0) {
		mipi_otm9608a_pdata = pdev->dev.platform_data;
		return 0;
	}

	spin_lock_init(&otm9608a_spin_lock);
	msm_fb_add_device(pdev);

	printk("%s: Done\n", __func__);
	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_otm9608a_lcd_probe,
	.driver = {
		.name   = "mipi_otm9608a",
	},
};

static struct msm_fb_panel_data otm9608a_panel_data = {
	.on    = mipi_otm9608a_lcd_on,
	.off    = mipi_otm9608a_lcd_off,
	.set_backlight    = mipi_otm9608a_set_backlight,
};

static int ch_used[3];

int mipi_otm9608a_device_register(struct msm_panel_info *pinfo,
                    u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	printk("%s\n", __func__);

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_otm9608a", (panel << 8)|channel);
	if (!pdev)
	return -ENOMEM;

	otm9608a_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &otm9608a_panel_data, sizeof(otm9608a_panel_data));
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

static int __init mipi_otm9608a_lcd_init(void)
{
	printk("%s\n", __func__);

	mipi_dsi_buf_alloc(&otm9608a_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&otm9608a_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}

module_init(mipi_otm9608a_lcd_init);

