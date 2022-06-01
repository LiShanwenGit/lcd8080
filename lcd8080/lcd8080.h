#ifndef __LCD_8080__
#define __LCD_8080__

#include <linux/spinlock.h>
#include <linux/spi/spi.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/io.h> //含有 ioremap 函数 iounmap 函数
#include <linux/uaccess.h> //含有 copy_from_user 函数和含有 copy_to_user 函数
#include <linux/device.h> //含有类相关的设备函数
#include <linux/cdev.h>
#include <linux/of.h> //包含设备树相关函数
#include <linux/fb.h> //包含 frame buffer


#define  LCD8080_DATA_WIDTH       16U


#ifdef __cplusplus 
extern "C" { 
#endif

struct lcd8080_par;

struct lcd8080_io_desp
{
    struct gpio_desc *rst;        //reset signal
    struct gpio_desc *cs;         //chip select signal
    struct gpio_desc *wr;         //write enable signal
    struct gpio_desc *rd;         //read enable signal
    struct gpio_desc *backlight;  //back light signal
    struct gpio_desc *data[LCD8080_DATA_WIDTH];   //data signal
};

struct lcd8080_display_ops
{
    void (*set_addr_win)(struct lcd8080_par *par,int xs, int ys, int xe, int ye);
	int  (*init_display)(struct lcd8080_par *par);
    int  (*set_var)(struct lcd8080_par *par);
    int  (*set_gamma)(struct lcd8080_par *par, u32 *curves);
};

struct lcd8080_ops
{   
    int  (*write)(struct lcd8080_par *par, void *buffer, size_t len);
    int  (*read)(struct lcd8080_par *par, void *buffer, size_t len);
    void (*write_reg)(struct lcd8080_par *par, int len, ...);
	void (*reset)(struct lcd8080_par *par);
	void (*update_display)(struct lcd8080_par *par,unsigned int start_line, unsigned int end_line);
	int  (*blank)(struct lcd8080_par *par, bool on);
    struct lcd8080_display_ops *disp_ops;
};

struct lcd8080_display_var
{
    u32 display_width;       
    u32 display_height;
    u32 data_width;
    u32 rotate;
    u32 fps;
    u32 dma_enable;
    u32 bgr;
};


struct lcd8080_par
{
    unsigned int               pseudo_palette[16];
    struct fb_info             *fb_info;
    struct lcd8080_display_var display_var;
    struct lcd8080_io_desp     io_desp;
    struct lcd8080_ops         ops;
};



extern int lcd8080_write_16bit(struct lcd8080_par *par, void *buffer, size_t len);
extern int lcd8080_write_8bit(struct lcd8080_par *par, void *buffer, size_t len);
extern int lcd8080_read(struct lcd8080_par *par, void *buffer, size_t len);
extern void lcd8080_write_register(struct lcd8080_par *par, int len, ...);


extern int lcd8080_probe_common(struct lcd8080_display_ops *display_ops, struct platform_device *pdev);
extern int lcd8080_remove_common(struct platform_device *pdev);

#define                                                  \
LCD8080_DRIVER_REGISTER(_name, _compatible, _display)    \
                                                         \
                                                         \
                                                         \
static int lcd8080_probe(struct platform_device *pdev)   \
{                                                        \
    return lcd8080_probe_common(_display,pdev);          \
}                                                        \
                                                         \
static int lcd8080_remove(struct platform_device *pdev)  \
{                                                        \
    return lcd8080_remove_common(pdev);                  \
}                                                        \
                                                         \
static struct of_device_id   lcd8080_dt_ids[] =          \
{                                                        \
    {.compatible = _compatible,},                        \
};                                                       \
                                                         \
static struct platform_driver lcd8080_driver =           \
{                                                        \
	.driver = {                                          \
		.name   = _name,                                 \
		.owner  = THIS_MODULE,                           \
		.of_match_table = lcd8080_dt_ids,                \
	},                                                   \
	.probe  = lcd8080_probe,                             \
	.remove = lcd8080_remove,                            \
};                                                       \
                                                         \
module_platform_driver(lcd8080_driver);

#ifdef __cplusplus 
} 
#endif 


#endif


