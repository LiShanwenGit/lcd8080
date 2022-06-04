// SPDX-License-Identifier: GPL-2.0+
/*
 * LCD8080 driver for the ILI9325 LCD Controller
 *
 * Copyright (C) 2020 shanwen.li
 *
 * Based on ili9325.c by Jeroen Domburg
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>

#include "lcd8080.h"

#define DRVNAME		"lcd8080_ili9325"
#define BPP		16
#define FPS		20
#define DEFAULT_GAMMA	"0F 00 7 2 0 0 6 5 4 1\n" \
			"04 16 2 7 6 3 2 1 7 7"

static unsigned int bt = 6; /* VGL=Vci*4 , VGH=Vci*4 */
module_param(bt, uint, 0000);
MODULE_PARM_DESC(bt, "Sets the factor used in the step-up circuits");

static unsigned int vc = 0x03; /* Vci1=Vci*0.80 */
module_param(vc, uint, 0000);
MODULE_PARM_DESC(vc, "Sets the ratio factor of Vci to generate the reference voltages Vci1");

static unsigned int vrh = 0x0d; /* VREG1OUT=Vci*1.85 */
module_param(vrh, uint, 0000);
MODULE_PARM_DESC(vrh, "Set the amplifying rate (1.6 ~ 1.9) of Vci applied to output the VREG1OUT");

static unsigned int vdv = 0x12; /* VCOMH amplitude=VREG1OUT*0.98 */
module_param(vdv, uint, 0000);
MODULE_PARM_DESC(vdv, "Select the factor of VREG1OUT to set the amplitude of Vcom");

static unsigned int vcm = 0x0a; /* VCOMH=VREG1OUT*0.735 */
module_param(vcm, uint, 0000);
MODULE_PARM_DESC(vcm, "Set the internal VcomH voltage");

/*
 * Verify that this configuration is within the Voltage limits
 *
 * Display module configuration: Vcc = IOVcc = Vci = 3.3V
 *
 * Voltages
 * ----------
 * Vci                                =   3.3
 * Vci1           =  Vci * 0.80       =   2.64
 * DDVDH          =  Vci1 * 2         =   5.28
 * VCL            = -Vci1             =  -2.64
 * VREG1OUT       =  Vci * 1.85       =   4.88
 * VCOMH          =  VREG1OUT * 0.735 =   3.59
 * VCOM amplitude =  VREG1OUT * 0.98  =   4.79
 * VGH            =  Vci * 4          =  13.2
 * VGL            = -Vci * 4          = -13.2
 *
 * Limits
 * --------
 * Power supplies
 * 1.65 < IOVcc < 3.30   =>  1.65 < 3.3 < 3.30
 * 2.40 < Vcc   < 3.30   =>  2.40 < 3.3 < 3.30
 * 2.50 < Vci   < 3.30   =>  2.50 < 3.3 < 3.30
 *
 * Source/VCOM power supply voltage
 *  4.50 < DDVDH < 6.0   =>  4.50 <  5.28 <  6.0
 * -3.0  < VCL   < -2.0  =>  -3.0 < -2.64 < -2.0
 * VCI - VCL < 6.0       =>  5.94 < 6.0
 *
 * Gate driver output voltage
 *  10  < VGH   < 20     =>   10 <  13.2  < 20
 * -15  < VGL   < -5     =>  -15 < -13.2  < -5
 * VGH - VGL < 32        =>   26.4 < 32
 *
 * VCOM driver output voltage
 * VCOMH - VCOML < 6.0   =>  4.79 < 6.0
 */

static int init_display(struct lcd8080_par *par)
{
	par->ops.reset(par);

	bt &= 0x07;
	vc &= 0x07;
	vrh &= 0x0f;
	vdv &= 0x1f;
	vcm &= 0x3f;

	/* Initialization sequence from ILI9325 Application Notes */

	/* ----------- Start Initial Sequence ----------- */
	write_reg(par, 0x00E3, 0x3008); /* Set internal timing */
	write_reg(par, 0x00E7, 0x0012); /* Set internal timing */
	write_reg(par, 0x00EF, 0x1231); /* Set internal timing */
	write_reg(par, 0x0001, 0x0100); /* set SS and SM bit */
	write_reg(par, 0x0002, 0x0700); /* set 1 line inversion */
	write_reg(par, 0x0004, 0x0000); /* Resize register */
	write_reg(par, 0x0008, 0x0207); /* set the back porch and front porch */
	write_reg(par, 0x0009, 0x0000); /* set non-display area refresh cycle */
	write_reg(par, 0x000A, 0x0000); /* FMARK function */
	write_reg(par, 0x000C, 0x0000); /* RGB interface setting */
	write_reg(par, 0x000D, 0x0000); /* Frame marker Position */
	write_reg(par, 0x000F, 0x0000); /* RGB interface polarity */

	/* ----------- Power On sequence ----------- */
	write_reg(par, 0x0010, 0x0000); /* SAP, BT[3:0], AP, DSTB, SLP, STB */
	write_reg(par, 0x0011, 0x0007); /* DC1[2:0], DC0[2:0], VC[2:0] */
	write_reg(par, 0x0012, 0x0000); /* VREG1OUT voltage */
	write_reg(par, 0x0013, 0x0000); /* VDV[4:0] for VCOM amplitude */
	mdelay(200); /* Dis-charge capacitor power voltage */
	write_reg(par, 0x0010, /* SAP, BT[3:0], AP, DSTB, SLP, STB */
		BIT(12) | (bt << 8) | BIT(7) | BIT(4));
	write_reg(par, 0x0011, 0x220 | vc); /* DC1[2:0], DC0[2:0], VC[2:0] */
	mdelay(50); /* Delay 50ms */
	write_reg(par, 0x0012, vrh); /* Internal reference voltage= Vci; */
	mdelay(50); /* Delay 50ms */
	write_reg(par, 0x0013, vdv << 8); /* Set VDV[4:0] for VCOM amplitude */
	write_reg(par, 0x0029, vcm); /* Set VCM[5:0] for VCOMH */
	write_reg(par, 0x002B, 0x000C); /* Set Frame Rate */
	mdelay(50); /* Delay 50ms */
	write_reg(par, 0x0020, 0x0000); /* GRAM horizontal Address */
	write_reg(par, 0x0021, 0x0000); /* GRAM Vertical Address */

	/*------------------ Set GRAM area --------------- */
	write_reg(par, 0x0050, 0x0000); /* Horizontal GRAM Start Address */
	write_reg(par, 0x0051, 0x00EF); /* Horizontal GRAM End Address */
	write_reg(par, 0x0052, 0x0000); /* Vertical GRAM Start Address */
	write_reg(par, 0x0053, 0x013F); /* Vertical GRAM Start Address */
	write_reg(par, 0x0060, 0xA700); /* Gate Scan Line */
	write_reg(par, 0x0061, 0x0001); /* NDL,VLE, REV */
	write_reg(par, 0x006A, 0x0000); /* set scrolling line */

	/*-------------- Partial Display Control --------- */
	write_reg(par, 0x0080, 0x0000);
	write_reg(par, 0x0081, 0x0000);
	write_reg(par, 0x0082, 0x0000);
	write_reg(par, 0x0083, 0x0000);
	write_reg(par, 0x0084, 0x0000);
	write_reg(par, 0x0085, 0x0000);

	/*-------------- Panel Control ------------------- */
	write_reg(par, 0x0090, 0x0010);
	write_reg(par, 0x0092, 0x0600);
	write_reg(par, 0x0007, 0x0133); /* 262K color and display ON */

	return 0;
}

static void set_addr_win(struct lcd8080_par *par, int xs, int ys, int xe, int ye)
{
	switch (par->display_var.rotate) {
	/* R20h = Horizontal GRAM Start Address */
	/* R21h = Vertical GRAM Start Address */
	case 0:
		write_reg(par, 0x0020, xs);
		write_reg(par, 0x0021, ys);
		break;
	case 180:
		write_reg(par, 0x0020, par->display_var.display_width - 1 - xs);
		write_reg(par, 0x0021, par->display_var.display_height - 1 - ys);
		break;
	case 270:
		write_reg(par, 0x0020, par->display_var.display_width - 1 - ys);
		write_reg(par, 0x0021, xs);
		break;
	case 90:
		write_reg(par, 0x0020, ys);
		write_reg(par, 0x0021, par->display_var.display_height - 1 - xs);
		break;
	}
	write_reg(par, 0x0022); /* Write Data to GRAM */
}

static int set_var(struct lcd8080_par *par)
{
	switch (par->display_var.rotate) {
	/* AM: GRAM update direction */
	case 0:
		write_reg(par, 0x03, 0x0030 | (par->display_var.bgr << 12));
		break;
	case 180:
		write_reg(par, 0x03, 0x0000 | (par->display_var.bgr << 12));
		break;
	case 270:
		write_reg(par, 0x03, 0x0028 | (par->display_var.bgr << 12));
		break;
	case 90:
		write_reg(par, 0x03, 0x0018 | (par->display_var.bgr << 12));
		break;
	}

	return 0;
}

static struct lcd8080_display_ops ili9325_display= 
{
    .init_display = init_display,
	.set_addr_win = set_addr_win,
    .set_var = set_var,
};

LCD8080_DRIVER_REGISTER(DRVNAME, "lcd8080-ili9325", &ili9325_display);

MODULE_DESCRIPTION("LCD8080 driver for the ILI9325 LCD Controller");
MODULE_AUTHOR("1477153217@qq.com");
MODULE_LICENSE("GPL");
