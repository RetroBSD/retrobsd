/*
 * Display driver for ILI9341 LCD controller.
 *
 * Copyright (C) 2015 Serge Vakulenko <serge@vak.ru>
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
#include <sys/param.h>
#include <sys/gpanel.h>

/*
 * Display size.
 */
static int _width, _height;

/*
 * ILI9341 registers.
 */
#define ILI9341_No_Operation                                0x00
#define ILI9341_Software_Reset                              0x01
#define ILI9341_Read_Display_Identification_Information     0x04
#define ILI9341_Read_Display_Status                         0x09
#define ILI9341_Read_Display_Power_Mode                     0x0A
#define ILI9341_Read_Display_MADCTL                         0x0B
#define ILI9341_Read_Display_Pixel_Format                   0x0C
#define ILI9341_Read_Display_Image_Format                   0x0D
#define ILI9341_Read_Display_Signal_Mode                    0x0E
#define ILI9341_Read_Display_Self_Diagnostic_Result         0x0F
#define ILI9341_Enter_Sleep_Mode                            0x10
#define ILI9341_Sleep_OUT                                   0x11
#define ILI9341_Partial_Mode_ON                             0x12
#define ILI9341_Normal_Display_Mode_ON                      0x13
#define ILI9341_Display_Inversion_OFF                       0x20
#define ILI9341_Display_Inversion_ON                        0x21
#define ILI9341_Gamma_Set                                   0x26
#define ILI9341_Display_OFF                                 0x28
#define ILI9341_Display_ON                                  0x29
#define ILI9341_Column_Address_Set                          0x2A
#define ILI9341_Page_Address_Set                            0x2B
#define ILI9341_Memory_Write                                0x2C
#define ILI9341_Color_SET                                   0x2D
#define ILI9341_Memory_Read                                 0x2E
#define ILI9341_Partial_Area                                0x30
#define ILI9341_Vertical_Scrolling_Definition               0x33
#define ILI9341_Tearing_Effect_Line_OFF                     0x34
#define ILI9341_Tearing_Effect_Line_ON                      0x35
#define ILI9341_Memory_Access_Control                       0x36
#define ILI9341_Vertical_Scrolling_Start_Address            0x37
#define ILI9341_Idle_Mode_OFF                               0x38
#define ILI9341_Idle_Mode_ON                                0x39
#define ILI9341_Pixel_Format_Set                            0x3A
#define ILI9341_Write_Memory_Continue                       0x3C
#define ILI9341_Read_Memory_Continue                        0x3E
#define ILI9341_Set_Tear_Scanline                           0x44
#define ILI9341_Get_Scanline                                0x45
#define ILI9341_Write_Display_Brightness                    0x51
#define ILI9341_Read_Display_Brightness                     0x52
#define ILI9341_Write_CTRL_Display                          0x53
#define ILI9341_Read_CTRL_Display                           0x54
#define ILI9341_Write_Content_Adaptive_Brightness_Control   0x55
#define ILI9341_Read_Content_Adaptive_Brightness_Control    0x56
#define ILI9341_Write_CABC_Minimum_Brightness               0x5E
#define ILI9341_Read_CABC_Minimum_Brightness                0x5F
#define ILI9341_Read_ID1                                    0xDA
#define ILI9341_Read_ID2                                    0xDB
#define ILI9341_Read_ID3                                    0xDC
#define ILI9341_RGB_Interface_Signal_Control                0xB0
#define ILI9341_Frame_Control_In_Normal_Mode                0xB1
#define ILI9341_Frame_Control_In_Idle_Mode                  0xB2
#define ILI9341_Frame_Control_In_Partial_Mode               0xB3
#define ILI9341_Display_Inversion_Control                   0xB4
#define ILI9341_Blanking_Porch_Control                      0xB5
#define ILI9341_Display_Function_Control                    0xB6
#define ILI9341_Entry_Mode_Set                              0xB7
#define ILI9341_Backlight_Control_1                         0xB8
#define ILI9341_Backlight_Control_2                         0xB9
#define ILI9341_Backlight_Control_3                         0xBA
#define ILI9341_Backlight_Control_4                         0xBB
#define ILI9341_Backlight_Control_5                         0xBC
#define ILI9341_Backlight_Control_7                         0xBE
#define ILI9341_Backlight_Control_8                         0xBF
#define ILI9341_Power_Control_1                             0xC0
#define ILI9341_Power_Control_2                             0xC1
#define ILI9341_VCOM_Control_1                              0xC5
#define ILI9341_VCOM_Control_2                              0xC7
#define ILI9341_NV_Memory_Write                             0xD0
#define ILI9341_NV_Memory_Protection Key                    0xD1
#define ILI9341_NV_Memory_Status Read                       0xD2
#define ILI9341_Read_ID4                                    0xD3
#define ILI9341_Positive_Gamma_Correction                   0xE0
#define ILI9341_Negative_Gamma_Correction                   0xE1
#define ILI9341_Digital_Gamma_Control_1                     0xE2
#define ILI9341_Digital_Gamma_Control_2                     0xE3
#define ILI9341_Interface_Control                           0xF6

/*
 * Memory Access Control register
 */
#define MADCTL_MY           0x80    /* Row address order */
#define MADCTL_MX           0x40    /* Column address order */
#define MADCTL_MV           0x20    /* Row/column exchange */
#define MADCTL_ML           0x10    /* Vertical refresh order */
#define MADCTL_BGR          0x08    /* Color filter selector: 0=RGB, 1=BGR */
#define MADCTL_MH           0x04    /* Horisontal refresh direction: 1=left-to-right */

/*
 * Write a 8-bit value to the ILI9341 Command register.
 */
static void write_command(int cmd)
{
    gpanel_rs_command();
    gpanel_write_byte(cmd);
}

/*
 * Write a 8-bit value to the ILI9341 Data register.
 */
static void write_data(int cmd)
{
    gpanel_rs_data();
    gpanel_write_byte(cmd);
}

/*
 * Set address window.
 */
static void set_window(int x0, int y0, int x1, int y1)
{
    write_command(ILI9341_Column_Address_Set);
    write_data(x0 >> 8);
    gpanel_write_byte(x0);
    gpanel_write_byte(x1 >> 8);
    gpanel_write_byte(x1);

    write_command(ILI9341_Page_Address_Set);
    write_data(y0 >> 8);
    gpanel_write_byte(y0);
    gpanel_write_byte(y1 >> 8);
    gpanel_write_byte(y1);

    write_command(ILI9341_Memory_Write);
}

/*
 * Draw a pixel.
 */
static void ili9341_set_pixel(int x, int y, int color)
{
    if (x < 0 || x >= _width || y < 0 || y >= _height)
        return;
    gpanel_cs_active();
    set_window(x, y, x, y);
    write_data(color >> 8);
    write_data(color);
    gpanel_cs_idle();
}

/*
 * Fast block fill operation.
 * Requires set_window() has previously been called to set
 * the fill bounds.
 * 'npixels' is inclusive, MUST be >= 1.
 */
static void flood(int color, int npixels)
{
    unsigned blocks, i;
    unsigned hi = color >> 8,
             lo = color;

    /* Write first pixel normally, decrement counter by 1. */
    gpanel_rs_data();
    gpanel_write_byte(hi);
    gpanel_write_byte(lo);
    npixels--;

    /* 64 pixels/block. */
    blocks = npixels >> 6;
    if (hi == lo) {
        /* High and low bytes are identical.  Leave prior data
         * on the port(s) and just toggle the write strobe. */
        while (blocks--) {
            /* 64 pixels/block / 4 pixels/pass. */
            for (i = 16; i > 0; i--) {
                /* 2 bytes/pixel x 4 pixels. */
                gpanel_wr_strobe();
                gpanel_wr_strobe();
                gpanel_wr_strobe();
                gpanel_wr_strobe();
                gpanel_wr_strobe();
                gpanel_wr_strobe();
                gpanel_wr_strobe();
                gpanel_wr_strobe();
            }
        }
        /* Fill any remaining pixels (1 to 64). */
        for (i = npixels & 63; i > 0; i--) {
            gpanel_wr_strobe();
            gpanel_wr_strobe();
        }
    } else {
        while (blocks--) {
            /* 64 pixels/block / 4 pixels/pass. */
            for (i = 16; i > 0; i--) {
                gpanel_write_byte(hi); gpanel_write_byte(lo);
                gpanel_write_byte(hi); gpanel_write_byte(lo);
                gpanel_write_byte(hi); gpanel_write_byte(lo);
                gpanel_write_byte(hi); gpanel_write_byte(lo);
            }
        }
        for (i = npixels & 63; i > 0; i--) {
            gpanel_write_byte(hi);
            gpanel_write_byte(lo);
        }
    }
}

/*
 * Switch the screen orientation.
 */
static void set_rotation(int rotation)
{
    write_command(ILI9341_Memory_Access_Control);
    switch (rotation & 3) {
    case 0:                     /* Portrait */
        write_data(MADCTL_MX | MADCTL_BGR);
        _width  = 240;
        _height = 320;
        break;
    case 1:                     /* Landscape */
        write_data(MADCTL_MV | MADCTL_BGR);
        _width  = 320;
        _height = 240;
        break;
    case 2:                     /* Upside down portrait */
        write_data(MADCTL_MY | MADCTL_BGR);
        _width  = 240;
        _height = 320;
        break;
    case 3:                     /* Upside down landscape */
        write_data(MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR);
        _width  = 320;
        _height = 240;
        break;
    }
}

static void ili9341_clear(struct gpanel_hw *h, int color, int width, int height)
{
    gpanel_cs_active();

    /* Switch screen orientaation. */
    if (width > height)
        set_rotation(1);        /* Landscape */
    else if (width < height)
        set_rotation(0);        /* Portrait */

    /* Fill the screen with a color. */
    set_window(0, 0, _width-1, _height-1);
    flood(color, _width * _height);
    gpanel_cs_idle();
}

/*
 * Fill a rectangle with specified color.
 */
static void ili9341_fill_rectangle(int x0, int y0, int x1, int y1, int color)
{
    if (x0 < 0) x0 = 0;
    if (y0 < 0) x0 = 0;
    if (x1 < 0) x1 = 0;
    if (y1 < 0) x1 = 0;
    if (x0 >= _width) x0 = _width-1;
    if (x1 >= _width) x1 = _width-1;
    if (y0 >= _height) y0 = _height-1;
    if (y1 >= _height) y1 = _height-1;

    if (x1 < x0) {
        int t = x0;
        x0 = x1;
        x1 = t;
    }
    if (y1 < y0) {
        int t = y0;
        y0 = y1;
        y1 = t;
    }
    gpanel_cs_active();
    set_window(x0, y0, x1, y1);
    flood(color, (x1 - x0 + 1) * (y1 - y0 + 1));
    gpanel_cs_idle();
}

/*
 * Fill a rectangle with user data.
 */
static void ili9341_draw_image(int x, int y, int width, int height,
    const unsigned short *data)
{
    unsigned cnt = width * height;
    int color;

    gpanel_cs_active();
    set_window(x, y, x + width - 1, y + height - 1);
    gpanel_rs_data();
    while (cnt--) {
        color = *data++;
        gpanel_write_byte(color >> 8);
        gpanel_write_byte(color);
    }
    gpanel_cs_idle();
}

/*
 * Draw a glyph of one symbol.
 */
static void ili9341_draw_glyph(const struct gpanel_font_t *font,
    int color, int background, int x, int y, int width,
    const unsigned short *bits)
{
    int h, w, c;
    unsigned bitmask = 0;

    if (x + width > _width ||  y + font->height > _height)
        return;

    if (background >= 0) {
        /*
         * Clear background.
         */
        gpanel_cs_active();
        set_window(x, y, x + width - 1, y + font->height - 1);
        gpanel_rs_data();

        /* Loop on each glyph row. */
        for (h=0; h<font->height; h++) {
            /* Loop on every pixel in the row (left to right). */
            for (w=0; w<width; w++) {
                if ((w & 15) == 0)
                    bitmask = *bits++;
                else
                    bitmask <<= 1;

                c = (bitmask & 0x8000) ? color : background;
                gpanel_write_byte(c >> 8);
                gpanel_write_byte(c);
            }
        }
        gpanel_cs_idle();
    } else {
        /*
         * Transparent background.
         */
        /* Loop on each glyph row. */
        for (h=0; h<font->height; h++) {
            /* Loop on every pixel in the row (left to right). */
            for (w=0; w<width; w++) {
                if ((w & 15) == 0)
                    bitmask = *bits++;
                else
                    bitmask <<= 1;

                if (bitmask & 0x8000)
                    ili9341_set_pixel(x + w, y + h, color);
            }
        }
    }
}

/*
 * Initialize the LCD controller.
 * Fill the gpanel_hw descriptor.
 */
void ili9341_init_display(struct gpanel_hw *h)
{
    /* Use a few NOPs to synchronize after the hard Reset. */
    gpanel_cs_active();
    write_command(ILI9341_No_Operation);
    write_command(ILI9341_No_Operation);
    write_command(ILI9341_No_Operation);
    write_command(ILI9341_No_Operation);

    write_command(ILI9341_Sleep_OUT);
    udelay(150000);

    write_command(ILI9341_Display_OFF);

    write_command(ILI9341_Power_Control_1);
    write_data(0x23);

    write_command(ILI9341_Power_Control_2);
    write_data(0x10);

    write_command(ILI9341_VCOM_Control_1);
    write_data(0x2B);
    write_data(0x2B);

    write_command(ILI9341_VCOM_Control_2);
    write_data(0xC0);

    write_command(ILI9341_Pixel_Format_Set);
    write_data(0x55);

    write_command(ILI9341_Frame_Control_In_Normal_Mode);
    write_data(0x00);
    write_data(0x1B);

    write_command(ILI9341_Entry_Mode_Set);
    write_data(0x07);

    write_command(ILI9341_Display_ON);

    set_rotation(1);                /* Landscape */
    set_window(0, 0, _width-1, _height-1);
    gpanel_cs_idle();

    /*
     * Fill the gpanel_hw descriptor.
     */
    h->name           = "Ilitek ILI9341";
    h->width          = _width;
    h->height         = _height;
    h->clear          = ili9341_clear;
    h->set_pixel      = ili9341_set_pixel;
    h->fill_rectangle = ili9341_fill_rectangle;
    h->draw_image     = ili9341_draw_image;
    h->draw_glyph     = ili9341_draw_glyph;
}
