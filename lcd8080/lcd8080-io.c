#include "lcd8080.h"


int lcd8080_write_16bit(struct lcd8080_par *par, void *buffer, size_t len)
{
    int i;
    u16 *data = (u16*)buffer;
    while(len-=2)
    {
        u16 write = *data;
        gpiod_set_value(par->io_desp.wr, 0);
        for(i=0;i<16;i++)
        {
            gpiod_set_value(par->io_desp.data[i],write&0x01);
            write >>= 1;
        }
        data ++ ;
        gpiod_set_value(par->io_desp.wr, 1);
    }
    return 0;
}

int lcd8080_write_8bit(struct lcd8080_par *par, void *buffer, size_t len)
{
    int i;
    u8 *data = (u8*)buffer;
    while(len--)
    {
        u8 write = *data;
        gpiod_set_value(par->io_desp.wr, 0);
        for(i=0;i<8;i++)
        {
            gpiod_set_value(par->io_desp.data[i],write&0x01);
            write >>= 1;
        }
        data ++ ;
        gpiod_set_value(par->io_desp.wr, 1);
    }
    return 0;
}


int lcd8080_read(struct lcd8080_par *par, void *buffer, size_t len)
{
    return 0;
}

void lcd8080_write_register(struct lcd8080_par *par, int len, ...)
{
    va_list args;
    int buff;
    int i;
    u8 reg,data;
    va_start(args, len);
    if(par->io_desp.cs)
    {
        gpiod_set_value(par->io_desp.cs, 0);
    }
    buff = va_arg(args, unsigned int);//send command
    reg = buff;
    gpiod_set_value(par->io_desp.wr, 1);
    for(i=0;i<8;i++)
    {
        gpiod_set_value(par->io_desp.data[i],reg&0x01);
        reg >>= 1;
    }
    while(buff)
    {
        buff = va_arg(args, unsigned int);//send data
        data = buff;
        gpiod_set_value(par->io_desp.wr, 0);
        for(i=0;i<8;i++)
        {
            gpiod_set_value(par->io_desp.data[i],data&0x01);
            data >>= 1;
        }
        gpiod_set_value(par->io_desp.wr, 1);
    }
    va_end(args);
    if(par->io_desp.cs)
    {
        gpiod_set_value(par->io_desp.cs, 1);
    }
}

