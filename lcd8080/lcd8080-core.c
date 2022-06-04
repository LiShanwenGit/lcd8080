#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>       
#include <asm/io.h>        
#include <asm/uaccess.h>     
#include <linux/device.h>   
#include <linux/cdev.h>
#include <linux/platform_device.h> 
#include <linux/of.h>       
#include <linux/kobject.h>  
#include <linux/sysfs.h>    
#include <linux/gpio/consumer.h> 
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/gpio.h>   
#include <linux/delay.h>
#include "lcd8080.h"
#include <video/mipi_display.h>


static u32 lcd8080_property_value(struct device *dev, const char *propname)
{
	int ret;
	u32 val = 0;
	ret = device_property_read_u32(dev, propname, &val);
	if (ret == 0)
		dev_info(dev, "%s: %s = %u\n", __func__, propname, val);
	return val;
}


static struct gpio_desc *lcd8080_parse_dt_signal(struct device *dev, const char *signal)
{
    struct gpio_desc *desp = devm_gpiod_get(dev,signal,GPIOF_OUT_INIT_LOW);
    if(IS_ERR(desp))
    {
        dev_err(dev,"%s: %s = %u\n", __func__, signal, desc_to_gpio(desp));
        return NULL;
    }
    return desp;
}

static int lcd8080_parse_display_var_dt(struct device *dev, struct lcd8080_par *par)
{
    struct lcd8080_display_var *display_var = &par->display_var;
    display_var->data_width = lcd8080_property_value(dev,"data-width");
    if(display_var->data_width == 0)
    {
        dev_info(dev,"%s use default data width bit: 8\n",__func__);
        display_var->data_width = 16;
    }
    display_var->display_height = lcd8080_property_value(dev,"height");
    if(display_var->display_height == 0)
    {
        dev_err(dev,"%s device tree no display height\n",__func__);
        return -1;
    }
    display_var->display_width = lcd8080_property_value(dev,"width");
    if(display_var->display_width == 0)
    {
        dev_err(dev,"%s device tree no display width\n",__func__);
        return -1;
    }
    display_var->rotate = lcd8080_property_value(dev,"rotate");
    if(display_var->rotate == 0)
    {
        dev_info(dev,"%s use default rotate: 0\n",__func__);
        display_var->rotate = 0;
    }
    display_var->fps = lcd8080_property_value(dev,"fps");
    if(display_var->fps == 0)
    {
        dev_info(dev,"%s use default fps: 0\n",__func__);
        display_var->fps = 10;
    }
    display_var->dma_enable = lcd8080_property_value(dev,"dma-enable");
    if(display_var->dma_enable == 0)
    {
        dev_info(dev,"%s :no use dma\n",__func__);
        display_var->dma_enable = 0;
    }
    else
    {
        dev_info(dev,"%s :use dma\n",__func__);
        display_var->dma_enable = 1;
    }
    display_var->bgr = lcd8080_property_value(dev,"bgr");
    if(display_var->bgr == 0)
    {
        dev_info(dev,"%s :no use bgr\n",__func__);
        display_var->dma_enable = 0;
    }
    return 0;
}


static int lcd8080_request_io_dt(struct device *dev, struct lcd8080_par *par)
{
    int i = 0;
    struct lcd8080_display_var *display_var = &par->display_var;
    struct lcd8080_io_desp *io_desp = &par->io_desp;
    io_desp->cs = lcd8080_parse_dt_signal(dev, "cs");
    if(io_desp->cs == NULL)
    {
        dev_info(dev,"%s used default value\n",__func__);
    }
    else
    {
        gpiod_direction_output(io_desp->cs, 1);
        dev_info(dev,"%s cs-gpios = %d\n",__func__,desc_to_gpio(io_desp->cs));
    }
    io_desp->rd = lcd8080_parse_dt_signal(dev, "rd");
    if(io_desp->rd ==NULL)
    {
        dev_err(dev,"%s can not find rd pin\n",__func__);
        return -1;
    }
    else
    {
        gpiod_direction_output(io_desp->rd, 0);
        dev_info(dev,"%s rd-gpios = %d\n",__func__,desc_to_gpio(io_desp->rd));
    }
    io_desp->rst = lcd8080_parse_dt_signal(dev, "reset");
    if(io_desp->rst ==NULL)
    {
        dev_err(dev,"%s can not find rst pin\n",__func__);
        return -1;
    }
    else
    {
        gpiod_direction_output(io_desp->rst, 0);
        dev_info(dev,"%s rst-gpios = %d\n",__func__,desc_to_gpio(io_desp->rst));
    }
    io_desp->wr = lcd8080_parse_dt_signal(dev, "wr");
    if(io_desp->wr ==NULL)
    {
        dev_err(dev,"%s can not find wr pin\n",__func__);
        return -1;
    }
    else
    {
        gpiod_direction_output(io_desp->wr, 0);
        dev_info(dev,"%s wr-gpios = %d\n",__func__,desc_to_gpio(io_desp->wr));
    }
    io_desp->dc = lcd8080_parse_dt_signal(dev, "dc");
    if(io_desp->dc ==NULL)
    {
        dev_err(dev,"%s can not find dc pin\n",__func__);
        return -1;
    }
    else
    {
        gpiod_direction_output(io_desp->dc, 0);
        dev_info(dev,"%s dc-gpios = %d\n",__func__,desc_to_gpio(io_desp->dc));
    }
    io_desp->backlight = lcd8080_parse_dt_signal(dev, "backlight");
    if(io_desp->backlight ==NULL)
    {
        dev_info(dev,"%s no use backlight pin\n",__func__);
    }
    else
    {
        gpiod_direction_output(io_desp->backlight, 0);
        dev_info(dev,"%s backlight-gpios = %d\n",__func__,desc_to_gpio(io_desp->backlight));
    }
    for(i=0; i < display_var->data_width ; i++)
    {
        io_desp->data[i] = devm_gpiod_get_index(dev,"data",i,GPIOF_OUT_INIT_LOW);
        if(IS_ERR(io_desp->data[i]))
        {
            dev_err(dev,"%s: failed to request data[%d] = %u\n", __func__, i,desc_to_gpio(io_desp->data[i]));
            return -1;
        }
        gpiod_direction_output(io_desp->data[i], 0);
        dev_info(dev,"%s data[%d] pin = %d\n",__func__,i,desc_to_gpio(io_desp->data[i]));
    }
    return 0;    
}

static void lcd8080_defio(struct fb_info *fb_info, struct list_head *pagelist)
{
    //比较粗暴的方式，直接全部刷新
    fb_info->fbops->fb_pan_display(&fb_info->var,fb_info); //将应用层数据刷到 FrameBuffer 缓存中
}

void lcd8080_refresh(struct fb_info *fb_info, struct lcd8080_par *par)
{
    u8 *p = (u8 *)(fb_info->screen_base);
    par->ops.disp_ops->set_addr_win(par,0,0,
                                    par->display_var.display_height,
                                    par->display_var.display_width);
    par->ops.write(par,p,fb_info->screen_size);
}

static ssize_t lcd8080_fb_write(struct fb_info *info, const char __user *buf,size_t count, loff_t *ppos)
{
    struct lcd8080_par *par = info->par;
	unsigned long total_size;
	unsigned long p = *ppos;
	void *dst;
	total_size = info->fix.smem_len;
	if (p > total_size)
		return -EINVAL;
	if (count + p > total_size)
		count = total_size - p;
	if (!count)
		return -EINVAL;
	dst = info->screen_buffer + p;

	if (copy_from_user(dst, buf, count))
		return -EFAULT;
	lcd8080_refresh(info,par);
	*ppos += count;
	return count;
}

static void lcd8080_fb_fillrect(struct fb_info *info, const struct fb_fillrect *rect)
{
    struct lcd8080_par *par = info->par;
	sys_fillrect(info, rect);
	lcd8080_refresh(info,par);
}

static void lcd8080_fb_copyarea(struct fb_info *info, const struct fb_copyarea *area)
{
    struct lcd8080_par *par = info->par;
	sys_copyarea(info, area);
	lcd8080_refresh(info,par);
}

static void lcd8080_fb_imageblit(struct fb_info *info, const struct fb_image *image)
{
    struct lcd8080_par *par = info->par;
	sys_imageblit(info, image);
	lcd8080_refresh(info,par);
}


static inline unsigned int chan_to_field(unsigned int chan, struct fb_bitfield *bf)
{
    chan &= 0xffff;
    chan >>= 16 - bf->length;
    return chan << bf->offset;
}

static int lcd8080_fb_setcolreg(u32 regno, u32 red,u32 green, u32 blue,u32 transp, struct fb_info *info)
{
    unsigned int val;
    unsigned int *ppt = ((struct lcd8080_par*)info->par)->pseudo_palette;
    if (regno > 16)
    {
    return 1;
    }
    val = chan_to_field(red, &info->var.red);
    val |= chan_to_field(green, &info->var.green);
    val |= chan_to_field(blue, &info->var.blue);
    ppt[regno] = val;
    return 0;
}

static void lcd8080_set_addr_win_default(struct lcd8080_par *par, int xs, int ys, int xe,int ye)
{
	write_reg(par, MIPI_DCS_SET_COLUMN_ADDRESS,(xs+40) >> 8, xs+40, ((xe+40) >> 8) & 0xFF, (xe+40) & 0xFF);

	write_reg(par, MIPI_DCS_SET_PAGE_ADDRESS,((ys+52) >> 8) & 0xFF, (ys+52) & 0xFF, ((ye+52) >> 8) & 0xFF, (ye+52) & 0xFF);

	write_reg(par, MIPI_DCS_WRITE_MEMORY_START);
}

static struct fb_ops lcd8080_ops = {
    .owner = THIS_MODULE,
    .fb_write = lcd8080_fb_write,
    .fb_setcolreg = lcd8080_fb_setcolreg, /*设置颜色寄存器*/
    .fb_fillrect = lcd8080_fb_fillrect,  /*用像素行填充矩形框，通用库函数*/
    .fb_copyarea = lcd8080_fb_copyarea, /*将屏幕的一个矩形区域复制到另一个区域，通用库函数*/
    .fb_imageblit = lcd8080_fb_imageblit, /*显示一副图像，通用库函数*/
};

static struct fb_info *lcd8080_framebuffer_alloc(struct platform_device *pdev, struct lcd8080_par *par)
{
    char *gmem_addr;
    u32  gmem_size;
    struct fb_info *fb_info;
    struct device *dev =  &pdev->dev;
    struct fb_deferred_io *fb_defio;
    struct lcd8080_display_var *display_var = &par->display_var;
    fb_info = framebuffer_alloc(sizeof(struct fb_info), dev);
    if(fb_info == NULL)
    {
        dev_err(dev,"%s fail to alloc frame buffer\n",__func__);
        return NULL;
    }
    fb_info->var.rotate = display_var->rotate;
    fb_info->var.xres = display_var->display_width;
    fb_info->var.yres = display_var->display_height;
    fb_info->var.xres_virtual = display_var->display_width;
    fb_info->var.yres_virtual = display_var->display_width;
    fb_info->var.bits_per_pixel = display_var->data_width;
    fb_info->var.nonstd = 1;

    if(display_var->data_width == 16)
    {
        /* RGB565 */
        fb_info->var.red.offset = 11;
        fb_info->var.red.length = 5;
        fb_info->var.green.offset = 5;
        fb_info->var.green.length = 6;
        fb_info->var.blue.offset = 0;
        fb_info->var.blue.length = 5;
        fb_info->var.transp.offset = 0;
        fb_info->var.transp.length = 0;
    }
    else if(display_var->data_width == 8)
    {
        /* RGB323*/
        fb_info->var.red.offset = 6;
        fb_info->var.red.length = 3;
        fb_info->var.green.offset = 4;
        fb_info->var.green.length = 2;
        fb_info->var.blue.offset = 0;
        fb_info->var.blue.length = 3;
        fb_info->var.transp.offset = 0;
        fb_info->var.transp.length = 0;
    }
    fb_info->var.activate = FB_ACTIVATE_NOW;
    fb_info->var.vmode = FB_VMODE_NONINTERLACED;
    fb_info->fix.type = FB_TYPE_PACKED_PIXELS;
    fb_info->fix.visual = FB_VISUAL_TRUECOLOR;
    fb_info->fix.line_length = display_var->display_width*(display_var->data_width/8);
    fb_info->fix.accel = FB_ACCEL_NONE;
    //fb_info->fix.id = "lcd8080";

    gmem_size = display_var->display_height * display_var->display_width * display_var->data_width/2;
    if(display_var->dma_enable)
    {

    }
    else
    {
        gmem_addr = devm_kzalloc(dev, gmem_size, GFP_KERNEL);
        if(gmem_addr == NULL)
        {
            dev_err(dev,"%s fail to alloc gmem buffer\n", __func__);
            return NULL;
        }
        fb_info->screen_buffer = gmem_addr; 
        fb_info->screen_size = gmem_size;
        fb_info->fix.smem_len = gmem_size;          
        fb_info->fix.smem_start = (u32)gmem_addr;
        memset((void *)fb_info->fix.smem_start, 0, fb_info->fix.smem_len);
    }
    fb_info->pseudo_palette = par->pseudo_palette;
    fb_defio = devm_kzalloc(dev, sizeof(struct fb_deferred_io), GFP_KERNEL);
    if(fb_defio == NULL)
    {
        dev_err(dev,"%s fail to alloc fb_defio\n",__func__);
        return NULL;
    }
    fb_info->fbdefio = fb_defio;
    fb_info->fbdefio->delay  =  HZ / par->display_var.fps; 
    fb_info->fbdefio->deferred_io = lcd8080_defio;
    return fb_info;
}   



static void lcd8080_framebuffer_release(struct platform_device *pdev)
{
    struct lcd8080_par *par;
    struct fb_info *fb_info;
    struct device *dev =  &pdev->dev;
    par = dev_get_drvdata(dev);
    fb_info = par->fb_info;
    devm_kfree(dev,fb_info->screen_buffer);
    framebuffer_release(fb_info);
    dev_info(dev,"%s frame buffer release\n", __func__);
}


static void lcd8080_reset(struct lcd8080_par *par)
{
    gpiod_set_value(par->io_desp.rst,GPIOD_OUT_HIGH);
    msleep(100);
    gpiod_set_value(par->io_desp.rst,GPIOD_OUT_LOW);
    msleep(100);
    gpiod_set_value(par->io_desp.rst,GPIOD_OUT_HIGH);
    msleep(100);
}


static void lcd8080_set_ops(struct lcd8080_par *par)
{
    par->ops.reset = lcd8080_reset;
    if(par->display_var.data_width == 16)
    {
        par->ops.write = lcd8080_write_16bit;
    }
    else if(par->display_var.data_width == 8)
    {
        par->ops.write = lcd8080_write_8bit;
    }
    par->ops.write_reg = lcd8080_write_register;

}


static int lcd8080_init(struct lcd8080_par *par)
{
    par->ops.disp_ops->init_display(par);
    return 0;
}


int lcd8080_probe_common(struct lcd8080_display_ops *display_ops, struct platform_device *pdev)
{
    int ret;
    struct device *dev = &pdev->dev;
    struct fb_info *fb_info;
    struct lcd8080_par *par =  devm_kzalloc(dev,sizeof(struct lcd8080_par),GFP_KERNEL);
    if(par == NULL)
    {
        dev_err(dev,"%s fail to alloc lcd8080_par\n", __func__);
        return -1;
    }
    par->ops.disp_ops = display_ops;
    if(par->ops.disp_ops->set_addr_win == NULL) //if not define set_addr_win, enable default function
    {
        par->ops.disp_ops->set_addr_win = lcd8080_set_addr_win_default;
    }
    ret = lcd8080_parse_display_var_dt(dev, par);
    if(ret < 0)
    {
        goto error;
    }
    ret = lcd8080_request_io_dt(dev, par);
    if(ret < 0)
    {
        goto error;
    }
    lcd8080_set_ops(par);
    dev_set_drvdata(dev,par);
    fb_info = lcd8080_framebuffer_alloc(pdev, par);
    if(fb_info == NULL)
    {
        lcd8080_framebuffer_release(pdev);
        goto error;
    }
    par->fb_info = fb_info;
    fb_info->par = par;
    fb_info->fbops = &lcd8080_ops;
    ret = register_framebuffer(fb_info);
    if(ret)
    {
        goto error;
    }
    lcd8080_init(par);
    return 0;
error:
    dev_err(dev,"%s fail to probe\n", __func__);
    unregister_framebuffer(fb_info);
    devm_kfree(dev,par->fb_info);
    devm_kfree(dev,par);
    return -1;
}


int lcd8080_remove_common(struct platform_device *pdev)
{
    struct lcd8080_par *par;
    struct fb_info *fb_info;
    struct device *dev =  &pdev->dev;
    par = dev_get_drvdata(dev);
    fb_info = par->fb_info;
    unregister_framebuffer(fb_info);
    lcd8080_framebuffer_release(pdev);
    devm_kfree(dev,par->fb_info);
    devm_kfree(dev,par);
    return 0;
}

