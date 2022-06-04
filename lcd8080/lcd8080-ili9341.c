#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <video/mipi_display.h>
#include "lcd8080.h"


#define DRVNAME		"lcd8080_ili9341"

#define TXBUFLEN	(4 * PAGE_SIZE)
#define DEFAULT_GAMMA	"1F 1A 18 0A 0F 06 45 87 32 0A 07 02 07 05 00\n" \
			"00 25 27 05 10 09 3A 78 4D 05 18 0D 38 3A 1F"

static int display_init(struct lcd8080_par *par)
{
    par->ops.reset(par);
	/* startup sequence for MI0283QT-9A */
	write_reg(par, MIPI_DCS_SOFT_RESET);
	mdelay(5);
	write_reg(par, MIPI_DCS_SET_DISPLAY_OFF);
	/* --------------------------------------------------------- */
	write_reg(par, 0xCF, 0x00, 0x83, 0x30);
	write_reg(par, 0xED, 0x64, 0x03, 0x12, 0x81);
	write_reg(par, 0xE8, 0x85, 0x01, 0x79);
	write_reg(par, 0xCB, 0x39, 0X2C, 0x00, 0x34, 0x02);
	write_reg(par, 0xF7, 0x20);
	write_reg(par, 0xEA, 0x00, 0x00);
	/* ------------power control-------------------------------- */
	write_reg(par, 0xC0, 0x26);
	write_reg(par, 0xC1, 0x11);
	/* ------------VCOM --------- */
	write_reg(par, 0xC5, 0x35, 0x3E);
	write_reg(par, 0xC7, 0xBE);
	/* ------------memory access control------------------------ */
	write_reg(par, MIPI_DCS_SET_PIXEL_FORMAT, 0x55); /* 16bit pixel */
	/* ------------frame rate----------------------------------- */
	write_reg(par, 0xB1, 0x00, 0x1B);
	/* ------------Gamma---------------------------------------- */
	/* write_reg(par, 0xF2, 0x08); */ /* Gamma Function Disable */
	write_reg(par, MIPI_DCS_SET_GAMMA_CURVE, 0x01);
	/* ------------display-------------------------------------- */
	write_reg(par, 0xB7, 0x07); /* entry mode set */
	write_reg(par, 0xB6, 0x0A, 0x82, 0x27, 0x00);
	write_reg(par, MIPI_DCS_EXIT_SLEEP_MODE);
	mdelay(100);
	write_reg(par, MIPI_DCS_SET_DISPLAY_ON);
	mdelay(20);
	return 0;
}

static void set_addr_win(struct lcd8080_par *par, int xs, int ys, int xe, int ye)
{
	write_reg(par, MIPI_DCS_SET_COLUMN_ADDRESS,
		  (xs >> 8) & 0xFF, xs & 0xFF, (xe >> 8) & 0xFF, xe & 0xFF);

	write_reg(par, MIPI_DCS_SET_PAGE_ADDRESS,
		  (ys >> 8) & 0xFF, ys & 0xFF, (ye >> 8) & 0xFF, ye & 0xFF);

	write_reg(par, MIPI_DCS_WRITE_MEMORY_START);
}

#define MEM_Y   BIT(7) /* MY row address order */
#define MEM_X   BIT(6) /* MX column address order */
#define MEM_V   BIT(5) /* MV row / column exchange */
#define MEM_L   BIT(4) /* ML vertical refresh order */
#define MEM_H   BIT(2) /* MH horizontal refresh order */
#define MEM_BGR (3) /* RGB-BGR Order */
static int set_var(struct lcd8080_par *par)
{
	switch (par->display_var.rotate) {
	case 0:
		write_reg(par, MIPI_DCS_SET_ADDRESS_MODE,
			  MEM_X | (par->display_var.bgr << MEM_BGR));
		break;
	case 270:
		write_reg(par, MIPI_DCS_SET_ADDRESS_MODE,
			  MEM_V | MEM_L | (par->display_var.bgr << MEM_BGR));
		break;
	case 180:
		write_reg(par, MIPI_DCS_SET_ADDRESS_MODE,
			  MEM_Y | (par->display_var.bgr << MEM_BGR));
		break;
	case 90:
		write_reg(par, MIPI_DCS_SET_ADDRESS_MODE,
			  MEM_Y | MEM_X | MEM_V | (par->display_var.bgr << MEM_BGR));
		break;
	}

	return 0;
}


static struct lcd8080_display_ops ili9341_display= 
{
    .init_display = display_init,
    .set_addr_win = set_addr_win,
    .set_var = set_var,
};

LCD8080_DRIVER_REGISTER(DRVNAME, "lcd8080-ili9341", &ili9341_display);

MODULE_DESCRIPTION("LCD8080 driver for the ILI9341 LCD Controller");
MODULE_AUTHOR("1477153217@qq.com");
MODULE_LICENSE("GPL");

