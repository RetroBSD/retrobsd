/*
 * Graphical GLCD driver for PIC32.
 *
 * Copyright (C) 2012 Majenko Technologies <matt@majenko.co.uk>
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */
#include "param.h"
#include "conf.h"
#include "user.h"
#include "ioctl.h"
#include "systm.h"
#include "sys/uio.h"
#include "glcd.h"
#include "debug.h"

const struct devspec glcddevs[] = {
    { 0, "glcd0" },
    { 0, 0 }
};

#define _BC(R,B) (R &= ~(1<<B))
#define _BS(R,B) (R |= (1<<B))

unsigned char screen[9][128];

unsigned char mode;
unsigned char screen_x = 0;
unsigned char screen_y = 0;

#define MODE_TEXT 0
#define MODE_LOAD 1

const unsigned char font[] = {
    0x00, 0x00, 0x00, 0x00, 0x00,// (space)
	0x00, 0x00, 0x5F, 0x00, 0x00,// !
	0x00, 0x07, 0x00, 0x07, 0x00,// "
	0x14, 0x7F, 0x14, 0x7F, 0x14,// #
	0x24, 0x2A, 0x7F, 0x2A, 0x12,// $
	0x23, 0x13, 0x08, 0x64, 0x62,// %
	0x36, 0x49, 0x55, 0x22, 0x50,// &
	0x00, 0x05, 0x03, 0x00, 0x00,// '
	0x00, 0x1C, 0x22, 0x41, 0x00,// (
	0x00, 0x41, 0x22, 0x1C, 0x00,// )
	0x08, 0x2A, 0x1C, 0x2A, 0x08,// *
	0x08, 0x08, 0x3E, 0x08, 0x08,// +
	0x00, 0x50, 0x30, 0x00, 0x00,// ,
	0x08, 0x08, 0x08, 0x08, 0x08,// -
	0x00, 0x60, 0x60, 0x00, 0x00,// .
	0x20, 0x10, 0x08, 0x04, 0x02,// /
	0x3E, 0x51, 0x49, 0x45, 0x3E,// 0
	0x00, 0x42, 0x7F, 0x40, 0x00,// 1
	0x42, 0x61, 0x51, 0x49, 0x46,// 2
	0x21, 0x41, 0x45, 0x4B, 0x31,// 3
	0x18, 0x14, 0x12, 0x7F, 0x10,// 4
	0x27, 0x45, 0x45, 0x45, 0x39,// 5
	0x3C, 0x4A, 0x49, 0x49, 0x30,// 6
	0x01, 0x71, 0x09, 0x05, 0x03,// 7
	0x36, 0x49, 0x49, 0x49, 0x36,// 8
	0x06, 0x49, 0x49, 0x29, 0x1E,// 9
	0x00, 0x36, 0x36, 0x00, 0x00,// :
	0x00, 0x56, 0x36, 0x00, 0x00,// ;
	0x00, 0x08, 0x14, 0x22, 0x41,// <
	0x14, 0x14, 0x14, 0x14, 0x14,// =
	0x41, 0x22, 0x14, 0x08, 0x00,// >
	0x02, 0x01, 0x51, 0x09, 0x06,// ?
	0x32, 0x49, 0x79, 0x41, 0x3E,// @
	0x7E, 0x11, 0x11, 0x11, 0x7E,// A
	0x7F, 0x49, 0x49, 0x49, 0x36,// B
	0x3E, 0x41, 0x41, 0x41, 0x22,// C
	0x7F, 0x41, 0x41, 0x22, 0x1C,// D
	0x7F, 0x49, 0x49, 0x49, 0x41,// E
	0x7F, 0x09, 0x09, 0x01, 0x01,// F
	0x3E, 0x41, 0x41, 0x51, 0x32,// G
	0x7F, 0x08, 0x08, 0x08, 0x7F,// H
	0x00, 0x41, 0x7F, 0x41, 0x00,// I
	0x20, 0x40, 0x41, 0x3F, 0x01,// J
	0x7F, 0x08, 0x14, 0x22, 0x41,// K
	0x7F, 0x40, 0x40, 0x40, 0x40,// L
	0x7F, 0x02, 0x04, 0x02, 0x7F,// M
	0x7F, 0x04, 0x08, 0x10, 0x7F,// N
	0x3E, 0x41, 0x41, 0x41, 0x3E,// O
	0x7F, 0x09, 0x09, 0x09, 0x06,// P
	0x3E, 0x41, 0x51, 0x21, 0x5E,// Q
	0x7F, 0x09, 0x19, 0x29, 0x46,// R
	0x46, 0x49, 0x49, 0x49, 0x31,// S
	0x01, 0x01, 0x7F, 0x01, 0x01,// T
	0x3F, 0x40, 0x40, 0x40, 0x3F,// U
	0x1F, 0x20, 0x40, 0x20, 0x1F,// V
	0x7F, 0x20, 0x18, 0x20, 0x7F,// W
	0x63, 0x14, 0x08, 0x14, 0x63,// X
	0x03, 0x04, 0x78, 0x04, 0x03,// Y
	0x61, 0x51, 0x49, 0x45, 0x43,// Z
	0x00, 0x00, 0x7F, 0x41, 0x41,// [
	0x02, 0x04, 0x08, 0x10, 0x20,// "\"
	0x41, 0x41, 0x7F, 0x00, 0x00,// ]
	0x04, 0x02, 0x01, 0x02, 0x04,// ^
	0x40, 0x40, 0x40, 0x40, 0x40,// _
	0x00, 0x01, 0x02, 0x04, 0x00,// `
	0x20, 0x54, 0x54, 0x54, 0x78,// a
	0x7F, 0x48, 0x44, 0x44, 0x38,// b
	0x38, 0x44, 0x44, 0x44, 0x20,// c
	0x38, 0x44, 0x44, 0x48, 0x7F,// d
	0x38, 0x54, 0x54, 0x54, 0x18,// e
	0x08, 0x7E, 0x09, 0x01, 0x02,// f
	0x08, 0x14, 0x54, 0x54, 0x3C,// g
	0x7F, 0x08, 0x04, 0x04, 0x78,// h
	0x00, 0x44, 0x7D, 0x40, 0x00,// i
	0x20, 0x40, 0x44, 0x3D, 0x00,// j
	0x00, 0x7F, 0x10, 0x28, 0x44,// k
	0x00, 0x41, 0x7F, 0x40, 0x00,// l
	0x7C, 0x04, 0x18, 0x04, 0x78,// m
	0x7C, 0x08, 0x04, 0x04, 0x78,// n
	0x38, 0x44, 0x44, 0x44, 0x38,// o
	0x7C, 0x14, 0x14, 0x14, 0x08,// p
	0x08, 0x14, 0x14, 0x18, 0x7C,// q
	0x7C, 0x08, 0x04, 0x04, 0x08,// r
	0x48, 0x54, 0x54, 0x54, 0x20,// s
	0x04, 0x3F, 0x44, 0x40, 0x20,// t
	0x3C, 0x40, 0x40, 0x20, 0x7C,// u
	0x1C, 0x20, 0x40, 0x20, 0x1C,// v
	0x3C, 0x40, 0x30, 0x40, 0x3C,// w
	0x44, 0x28, 0x10, 0x28, 0x44,// x
	0x0C, 0x50, 0x50, 0x50, 0x3C,// y
	0x44, 0x64, 0x54, 0x4C, 0x44,// z
	0x00, 0x08, 0x36, 0x41, 0x00,// {
	0x00, 0x00, 0x7F, 0x00, 0x00,// |
	0x00, 0x41, 0x36, 0x08, 0x00,// }
	0x08, 0x08, 0x2A, 0x1C, 0x08,// ->
	0x08, 0x1C, 0x2A, 0x08, 0x08 // <-
};

/*
 * Devices:
 *      /dev/glcd 
 *
 * Write to the device outputs to GLCD memory as data.  Use ioctl() to send
 * comands:
 *
 *      ioctl(fd, GLCD_RESET, 0)  - reset the GLCD
 *      ioctl(fd, GLCD_SET_PAGE, page)  - set the page address
 *      ioctl(fd, GLCD_SET_Y, y)  - set the page offset
 *
 */

int abs(int x)
{
    if(x>0)
        return x;
    return 0-x;
}

void glcd_delay()
{
    unsigned long c;
    for(c=0; c<200; c++)
        asm("nop");
}

void glcd_write_value(int chip, unsigned char data)
{
    _BC(LAT_RW, PIN_RW);
    _BC(TRIS_DB0, PIN_DB0);
    _BC(TRIS_DB1, PIN_DB1);
    _BC(TRIS_DB2, PIN_DB2);
    _BC(TRIS_DB3, PIN_DB3);
    _BC(TRIS_DB4, PIN_DB4);
    _BC(TRIS_DB5, PIN_DB5);
    _BC(TRIS_DB6, PIN_DB6);
    _BC(TRIS_DB7, PIN_DB7);

    if(chip==0)
        _BS(LAT_CS1, PIN_CS1);
    if(chip==1)
        _BS(LAT_CS2, PIN_CS2);

    if(data & 0b00000001) 
        _BS(LAT_DB0, PIN_DB0);
    else
        _BC(LAT_DB0, PIN_DB0);

    if(data & 0b00000010) 
        _BS(LAT_DB1, PIN_DB1);
    else
        _BC(LAT_DB1, PIN_DB1);

    if(data & 0b00000100) 
        _BS(LAT_DB2, PIN_DB2);
    else
        _BC(LAT_DB2, PIN_DB2);

    if(data & 0b00001000) 
        _BS(LAT_DB3, PIN_DB3);
    else
        _BC(LAT_DB3, PIN_DB3);

    if(data & 0b00010000) 
        _BS(LAT_DB4, PIN_DB4);
    else
        _BC(LAT_DB4, PIN_DB4);

    if(data & 0b00100000) 
        _BS(LAT_DB5, PIN_DB5);
    else
        _BC(LAT_DB5, PIN_DB5);

    if(data & 0b01000000) 
        _BS(LAT_DB6, PIN_DB6);
    else
        _BC(LAT_DB6, PIN_DB6);

    if(data & 0b10000000) 
        _BS(LAT_DB7, PIN_DB7);
    else
        _BC(LAT_DB7, PIN_DB7);

    _BS(LAT_E, PIN_E);
    glcd_delay();
    _BC(LAT_E, PIN_E);
    glcd_delay();

    if(chip==0)
        _BC(LAT_CS1, PIN_CS1);
    if(chip==1)
        _BC(LAT_CS2, PIN_CS2);
}

void glcd_write_data(int chip, unsigned char value)
{
    _BS(LAT_DI, PIN_DI);
    glcd_write_value(chip,value);
}

void glcd_write_command(int chip, unsigned char value)
{
    _BC(LAT_DI, PIN_DI);
    glcd_write_value(chip,value);
}

void glcd_init()
{
    _BC(TRIS_DI, PIN_DI);
    _BC(TRIS_RW, PIN_RW);
    _BC(TRIS_E, PIN_E);
    _BC(TRIS_DB0, PIN_DB0);
    _BC(TRIS_DB1, PIN_DB1);
    _BC(TRIS_DB2, PIN_DB2);
    _BC(TRIS_DB3, PIN_DB3);
    _BC(TRIS_DB4, PIN_DB4);
    _BC(TRIS_DB5, PIN_DB5);
    _BC(TRIS_DB6, PIN_DB6);
    _BC(TRIS_DB7, PIN_DB7);
    _BC(TRIS_CS1, PIN_CS1);
    _BC(TRIS_CS2, PIN_CS2);
    _BC(TRIS_RES, PIN_RES);

    _BC(LAT_E, PIN_E);
    _BC(LAT_CS1, PIN_CS1);
    _BC(LAT_CS2, PIN_CS2);
    _BS(LAT_RES, PIN_RES);

    glcd_write_command(0,GLCD_CMD_ON);
    glcd_write_command(1,GLCD_CMD_ON);
    glcd_write_command(0,GLCD_CMD_START);
    glcd_write_command(1,GLCD_CMD_START);

}

void glcd_reset()
{
    _BC(LAT_RES, PIN_RES);
    glcd_delay();
    _BS(LAT_RES, PIN_RES);
    glcd_init();
}

void glcd_set_page(int chip, int page)
{
    glcd_write_command(chip, GLCD_CMD_SET_PAGE | (page & 0x07));
}

void glcd_set_y(int chip, int y)
{
    glcd_write_command(chip, GLCD_CMD_SET_Y | (y & 0x63));
}

void glcd_update()
{
    unsigned char x,y;
    for(y=0; y<8; y++)
    {
        glcd_set_page(0,y);
        glcd_set_page(1,y);
        glcd_set_y(0,0);
        glcd_set_y(1,0);
        for(x=0; x<64; x++)
        {
            glcd_write_data(0,screen[y][x]);
            glcd_write_data(1,screen[y][x+64]);
        }
    }
}

void glcd_cls()
{
    unsigned char x,y;
    for(y=0; y<8; y++)
    {
        for(x=0; x<128; x++)
        {
            screen[y][x] = 0;
        }
    }
    screen_x = 0;
    screen_y = 0;
}

void glcd_load_page()
{
    screen_x = 0;
    screen_y = 0;
    mode = MODE_LOAD;
}

int
glcd_open (dev, flag, mode)
	dev_t dev;
{
	int unit = minor(dev);

	if (unit >= 2)
		return ENXIO;
	if (u.u_uid != 0)
		return EPERM;

        glcd_init();
        mode = MODE_TEXT;
	DEBUG("glcd%d: Openend\n",unit);
        //screen_x = 0;
        //screen_y = 0;
	return 0;
}

int glcd_close (dev_t dev, int flag, int mode)
{
	return 0;
}

int glcd_read (dev_t dev, struct uio *uio, int flag)
{
        // TODO
	return ENODEV;
}

void glcd_scrollUp(void)
{
        unsigned char x,y;
        for(y=0; y<9; y++)
        {
                for(x=0; x<128; x++)
                {
                        screen[y][x] = screen[y][x]>>1;
                        if(y<8)
                                screen[y][x] |= (screen[y+1][x]<<7);
                }
        }
}

void glcd_putc(char c)
{
    unsigned int cpos;
    if(c=='\r')
    {
        screen_x = 0;
        return;
    }

    if(c=='\n')
    {
        screen_y++;
	screen_x=0;
        if(screen_y>7)
        {
            screen_y--;
            for(cpos=0; cpos<8; cpos++)
                glcd_scrollUp();
        }
        return;
    }

    if(c==0x08)
    {
        if(screen_x>0)
        {
            screen_x-=6;
        } else {
            if(screen_y>0)
            {
                screen_x = 127-7;
                screen_y--;
            }
        }
        return;
    }
    if(c==0x07)
    {
        return;
    }
    if(c==0x0C)
    {
        glcd_cls();
        glcd_update();
        return;
    }
    if(screen_x > 127-6)
    {
        screen_x=0;
        screen_y++;
        if(screen_y>7)
        {
            screen_y--;
            for(cpos=0; cpos<8; cpos++)
                glcd_scrollUp();
        }
    }
    cpos = (c - ' ') * 5;
    screen[screen_y][screen_x++] = font[cpos++];
    screen[screen_y][screen_x++] = font[cpos++];
    screen[screen_y][screen_x++] = font[cpos++];
    screen[screen_y][screen_x++] = font[cpos++];
    screen[screen_y][screen_x++] = font[cpos++];
    screen[screen_y][screen_x++] = 0;
}

void glcd_setPixel(int x, int y)
{
        unsigned char row;
        unsigned char pixel;
        unsigned char mask;

        if(x>127)
                return;
        if(y>63)
                return;
        if(x<0)
                return;
        if(y<0)
                return;

        row = y>>3;
        pixel = y & 0x07;

        mask = 1 << pixel;

        screen[row][x] |= mask;
}

void glcd_clearPixel(int x, int y)
{
        unsigned char row;
        unsigned char pixel;
        unsigned char mask;

        if(x>127)
                return;
        if(y>63)
                return;
        if(x<0)
                return;
        if(y<0)
                return;

        row = y>>3;
        pixel = y & 0x07;

        mask = 1 << pixel;

        screen[row][x] &= ~mask;
}

void glcd_drawLine(int x, int y, int x2, int y2, unsigned char ink)
{
        int w, h;
        int dx1=0, dy1=0, dx2=0, dy2=0;
        int longest, shortest;
        int numerator;
        int i;

                w = x2 - x;
        h = y2 - y;

        if(w<0)
        {
                dx1 = -1;
                dx2 = -1;
        }
        else if(w>0)
        {
                dx1 = 1;
                dx2 = 1;
        }

        if(h<0)
                dy1 = -1;
        else if(h>0)
                dy1 = 1;

        longest = abs(w);
        shortest = abs(h);
        if(!(longest>shortest))
        {
                longest = abs(h);
                shortest = abs(w);
                if(h<0)
                        dy2 = -1;
                else if(h>0)
                        dy2 = 1;
                dx2=0;
        }

        numerator = longest >> 1;
        for(i=0; i<=longest; i++)
        {
                if(ink==1)
                    glcd_setPixel(x,y);
                else 
                    glcd_clearPixel(x,y);
                numerator += shortest;
                if(!(numerator<longest))
                {
                        numerator -= longest;
                        x += dx1;
                        y += dy1;
                } else {
                        x += dx2;
                        y += dy2;
                }
        }
}

void glcd_set_pixel(struct glcd_command *c)
{
    glcd_setPixel(c->x1, c->y1);
}

void glcd_clear_pixel(struct glcd_command *c)
{
    glcd_clearPixel(c->x1, c->y1);
}

void glcd_draw_line(struct glcd_command *c)
{
    glcd_drawLine(c->x1, c->y1, c->x2, c->y2, c->ink);
}

void glcd_draw_box(struct glcd_command *c)
{
    glcd_drawLine(c->x1, c->y1, c->x1, c->y2, c->ink);
    glcd_drawLine(c->x1, c->y1, c->x2, c->y1, c->ink);
    glcd_drawLine(c->x2, c->y2, c->x1, c->y2, c->ink);
    glcd_drawLine(c->x2, c->y2, c->x2, c->y1, c->ink);
}

void glcd_draw_filled_box(struct glcd_command *c)
{
    unsigned char y;
    int y1,y2;
    if(c->y1 > c->y2)
    {
        y1 = c->y2; 
        y2 = c->y1;
    } else {
        y1 = c->y1;
        y2 = c->y2;
    }
    for(y=y1; y<=y2; y++)
    {
        glcd_drawLine(c->x1, y, c->x2, y, c->ink);
    }
}

void glcd_goto_xy(struct glcd_command *c)
{
    screen_x = c->x1;
    screen_y = c->y1;
}

int glcd_write (dev_t dev, struct uio *uio, int flag)
{
    struct iovec *iov;
    int i;

    iov = uio->uio_iov;

    switch(mode)
    {
        case MODE_LOAD:
            for(i=0; i<iov->iov_len; i++)
            {
                screen[screen_y][screen_x] = *(iov->iov_base+i);
                screen_x++;
                if(screen_x==128)
                {
                    screen_y++;
                    if(screen_y==8)
                    {
                        mode=MODE_TEXT;
                        glcd_update();
                    }
                }
            }
            break;
        case MODE_TEXT:
            for(i=0; i<iov->iov_len; i++)
            {
                glcd_putc(*(iov->iov_base+i));
            }
            glcd_update();
            break;
    }
    return 0;
}

int glcd_ioctl (dev_t dev, register u_int cmd, caddr_t addr, int flag)
{
	if(cmd == GLCD_RESET) {
            glcd_reset();
        }

        if(cmd == GLCD_UPDATE) {
            glcd_update();
        }

        if(cmd == GLCD_CLS) {
            glcd_cls();
        }

        if (cmd == GLCD_LOAD_PAGE) {
            glcd_load_page();
        }

        if (cmd == GLCD_SET_PIXEL) {
            glcd_set_pixel((struct glcd_command *)addr);
        }

        if (cmd == GLCD_CLEAR_PIXEL) {
            glcd_clear_pixel((struct glcd_command *)addr);
        }

        if (cmd == GLCD_LINE) {
            glcd_draw_line((struct glcd_command *)addr);
        }

        if (cmd == GLCD_BOX) {
            glcd_draw_box((struct glcd_command *)addr);
        }

        if (cmd == GLCD_FILLED_BOX) {
            glcd_draw_filled_box((struct glcd_command *)addr);
        }

        if (cmd == GLCD_GOTO_XY) {
            glcd_goto_xy((struct glcd_command *)addr);
        }

	return 0;
}
