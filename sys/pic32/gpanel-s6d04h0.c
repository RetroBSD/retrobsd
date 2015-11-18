/*
 * Display driver for Samsung S6D04H0 LCD controller.
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
 * Level 1 Commands
 */
#define S6D04H0_No_Operation                            0x00    /* C 0 */
#define S6D04H0_Software_Reset                          0x01    /* C 0 */
#define S6D04H0_Read_Display_Identification_Information 0x04    /* R 4 */
#define S6D04H0_Read_Display_Status                     0x09    /* R 5 */
#define S6D04H0_Read_Display_Power_Mode                 0x0A    /* R 2 */
#define S6D04H0_Read_Display_MADCTL                     0x0B    /* R 2 */
#define S6D04H0_Read_Display_Pixel_Format               0x0C    /* R 2 */
#define S6D04H0_Read_Display_Image_Mode                 0x0D    /* R 2 */
#define S6D04H0_Read_Display_Signal_Mode                0x0E    /* R 2 */
#define S6D04H0_Read_Display_Self_Diagnostic_Result     0x0F    /* R 2 */
#define S6D04H0_Sleep_In                                0x10    /* C 0 */
#define S6D04H0_Sleep_Out                               0x11    /* C 0 */
#define S6D04H0_Partial_Mode_On                         0x12    /* C 0 */
#define S6D04H0_Normal_Display_Mode_On                  0x13    /* C 0 */
#define S6D04H0_Display_Inversion_Off                   0x20    /* C 0 */
#define S6D04H0_Display_Inversion_On                    0x21    /* C 0 */
#define S6D04H0_Gamma_Set                               0x26    /* W 1 */
#define S6D04H0_Display_Off                             0x28    /* C 0 */
#define S6D04H0_Display_On                              0x29    /* C 0 */
#define S6D04H0_Column_Address_Set                      0x2A    /* W 4 */
#define S6D04H0_Page_Address_Set                        0x2B    /* W 4 */
#define S6D04H0_Memory_Write                            0x2C    /* W n */
#define S6D04H0_Memory_Read                             0x2E    /* R n */
#define S6D04H0_Partial_Area                            0x30    /* W 4 */
#define S6D04H0_Vertical_Scrolling_Definition           0x33    /* W 6 */
#define S6D04H0_Tearing_Effect_Line_Off                 0x34    /* C 0 */
#define S6D04H0_Tearing_Effect_Line_On                  0x35    /* W 1 */
#define S6D04H0_Memory_Data_Access_Control              0x36    /* W 1 */
#define S6D04H0_Vertical_Scrolling_Start_Address        0x37    /* W 2 */
#define S6D04H0_Idle_Mode_Off                           0x38    /* C 0 */
#define S6D04H0_Idle_Mode_On                            0x39    /* C 0 */
#define S6D04H0_Interface_Pixel_Format                  0x3A    /* W 1 */
#define S6D04H0_Memory_Write_Continue                   0x3C    /* W n */
#define S6D04H0_Memory_Read_Continue                    0x3E    /* R n */
#define S6D04H0_Set_Tear_Scanline                       0x44    /* W 2 */
#define S6D04H0_Get_Scanline                            0x45    /* R 2 */
#define S6D04H0_Write_Manual_Brightness                 0x51    /* W 1 */
#define S6D04H0_Read_Display_Brightness                 0x52    /* R 2 */
#define S6D04H0_Write_BL_Control                        0x53    /* W 1 */
#define S6D04H0_Read_BL_Control                         0x54    /* R 2 */
#define S6D04H0_Write_MIE_Mode                          0x55    /* W 1 */
#define S6D04H0_Read_MIE_Mode                           0x56    /* R 2 */
#define S6D04H0_Write_Minimum_Brightness                0x5E    /* W 1 */
#define S6D04H0_Read_Minimum_Brightness                 0x5F    /* R 2 */
#define S6D04H0_Read_DDB_Start                          0xA1    /* R n */
#define S6D04H0_Read_DDB_Continue                       0xA8    /* R n */
#define S6D04H0_Read_ID1                                0xDA    /* R 2 */
#define S6D04H0_Read_ID2                                0xDB    /* R 2 */
#define S6D04H0_Read_ID3                                0xDC    /* R 2 */

/*
 * Level 2 Commands
 */
#define S6D04H0_MIECTL      0xC0    /* W(3)  MIE control */
#define S6D04H0_BCMODE      0xC1    /* W(1)  MIE control */
#define S6D04H0_WRMIECTL    0xC2    /* W(9)  MIE control */
#define S6D04H0_WRBLCTL     0xC3    /* W(2)  MIE control */
#define S6D04H0_MTPCTL      0xD0    /* W(1)  MTP control */
#define S6D04H0_MTPACCS     0xD2    /* W(2)  MTP control */
#define S6D04H0_MTPRD       0xD3    /* R(8)  MTP control */
#define S6D04H0_DSTB        0xDF    /* W(1)  Deep Standby */
#define S6D04H0_PASSWD1     0xF0    /* W(2)  Test Key */
#define S6D04H0_PASSWD2     0xF1    /* W(2)  Test Key */
#define S6D04H0_DISCTL      0xF2    /* W(17) Display control */
#define S6D04H0_MANPWRSEQ   0xF3    /* W(5)  Power sequence control */
#define S6D04H0_PWRCTL      0xF4    /* W(20) Power control */
#define S6D04H0_VCMCTL      0xF5    /* W(12) VCOM control */
#define S6D04H0_SRCCTL      0xF6    /* W(9)  Source control */
#define S6D04H0_IFCTL       0xF7    /* W(4)  Interface control */
#define S6D04H0_PANELCTL    0xF8    /* W(3)  Gate control */
#define S6D04H0_GAMMASEL    0xF9    /* W(1)  Gamma selection */
#define S6D04H0_PGAMMACTL   0xFA    /* W(12) Positive gamma control */
#define S6D04H0_NGAMMACTL   0xFB    /* W(12) Negative gamma control */

/*
 * Memory Access Control register
 */
#define MADCTL_MY           0x80
#define MADCTL_MX           0x40
#define MADCTL_MV           0x20
#define MADCTL_ML           0x10
#define MADCTL_BGR          0x08
#define MADCTL_MH           0x04
#define MADCTL_RGB          0x00

/*
 * Write a 8-bit value to the S6D04H0 Command register.
 */
static void write_command(int cmd)
{
    gpanel_cs_active();
    gpanel_rs_command();
    gpanel_write_byte(cmd);
    gpanel_cs_idle();
}

/*
 * Write a 8-bit value to the S6D04H0 Data register.
 */
static void write_data(int cmd)
{
    gpanel_cs_active();
    gpanel_rs_data();
    gpanel_write_byte(cmd);
    gpanel_cs_idle();
}

/*
 * Set address window.
 */
static void set_window(int x0, int y0, int x1, int y1)
{
    write_command(S6D04H0_Column_Address_Set);
    write_data(x0 >> 8);
    gpanel_write_byte(x0);
    gpanel_write_byte(x1 >> 8);
    gpanel_write_byte(x1);

    write_command(S6D04H0_Page_Address_Set);
    write_data(y0 >> 8);
    gpanel_write_byte(y0);
    gpanel_write_byte(y1 >> 8);
    gpanel_write_byte(y1);

    write_command(S6D04H0_Memory_Write);
}

/*
 * Draw a pixel.
 */
static void s6d04h0_set_pixel(int x, int y, int color)
{
    if (x < 0 || x >= _width || y < 0 || y >= _height)
        return;
    set_window(x, y, x, y);
    write_data(color >> 8);
    write_data(color);
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
    write_command(S6D04H0_Memory_Data_Access_Control);
    switch (rotation & 3) {
    case 0:                     /* Portrait */
        write_data(MADCTL_MY | MADCTL_BGR);
        _width  = 240;
        _height = 320;
        break;
    case 1:                     /* Landscape */
        write_data(MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR);
        _width  = 320;
        _height = 240;
        break;
    case 2:                     /* Upside down portrait */
        write_data(MADCTL_MX | MADCTL_BGR);
        _width  = 240;
        _height = 320;
        break;
    case 3:                     /* Upside down landscape */
        write_data(MADCTL_MV | MADCTL_BGR);
        _width  = 320;
        _height = 240;
        break;
    }
}

static void s6d04h0_clear(struct gpanel_hw *h, int color, int width, int height)
{
    /* Switch screen orientaation. */
    if (width > height)
        set_rotation(1);        /* Landscape */
    else if (width < height)
        set_rotation(0);        /* Portrait */

    /* Fill the screen with a color. */
    set_window(0, 0, _width-1, _height-1);
    flood(color, _width * _height);
}

/*
 * Fill a rectangle with specified color.
 */
static void s6d04h0_fill_rectangle(int x0, int y0, int x1, int y1, int color)
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
    set_window(x0, y0, x1, y1);
    flood(color, (x1 - x0 + 1) * (y1 - y0 + 1));
}

/*
 * Fill a rectangle with user data.
 */
static void s6d04h0_draw_image(int x, int y, int width, int height,
    const unsigned short *data)
{
    unsigned cnt = width * height;
    int color;

    set_window(x, y, x + width - 1, y + height - 1);
    gpanel_rs_data();
    while (cnt--) {
        color = *data++;
        gpanel_write_byte(color >> 8);
        gpanel_write_byte(color);
    }
}

/*
 * Draw a glyph of one symbol.
 */
static void s6d04h0_draw_glyph(const struct gpanel_font_t *font,
    int color, int background, int x, int y, int width,
    const unsigned short *bits)
{
    int h, w, c;
    unsigned bitmask = 0;

    if (background >= 0) {
        /*
         * Clear background.
         */
        set_window(x, y, x + width - 1, y + font->height - 1);
        gpanel_cs_active();
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
                    s6d04h0_set_pixel(x + w, y + h, color);
            }
        }
    }
}

/*
 * Initialize the LCD controller.
 * Fill the gpanel_hw descriptor.
 */
void s6d04h0_init_display(struct gpanel_hw *h)
{
    write_command(S6D04H0_PASSWD1);
    write_data(0x5A);
    write_data(0x5A);

    write_command(0xFC);
    write_data(0x5A);
    write_data(0x5A);

    write_command(0xFD);
    write_data(0x00);
    write_data(0x00);
    write_data(0x10);
    write_data(0x14);
    write_data(0x12);
    write_data(0x00);
    write_data(0x04);
    write_data(0x48);
    write_data(0x40);
    write_data(0x16);
    write_data(0x16);

    write_command(S6D04H0_Tearing_Effect_Line_On);

    write_command(S6D04H0_Display_Off);

    set_rotation(3);

    write_command(S6D04H0_Interface_Pixel_Format);
    write_data(0x55);

    write_command(S6D04H0_DISCTL);
    write_data(0x28);
    write_data(0x5B);
    write_data(0x7F);
    write_data(0x08);
    write_data(0x08);
    write_data(0x00);
    write_data(0x00);
    write_data(0x15);
    write_data(0x48);
    write_data(0x04);
    write_data(0x07);
    write_data(0x01);
    write_data(0x00);
    write_data(0x00);
    write_data(0x63);
    write_data(0x08);
    write_data(0x08);

    write_command(S6D04H0_IFCTL);
    write_data(0x01);
    write_data(0x00);
    write_data(0x10);
    write_data(0x00);

    write_command(S6D04H0_PANELCTL);
    write_data(0x33);
    write_data(0x00);
    write_data(0x00);

    write_command(S6D04H0_SRCCTL);
    write_data(0x01);
    write_data(0x01);
    write_data(0x07);
    write_data(0x00);
    write_data(0x01);
    write_data(0x0C);
    write_data(0x03);
    write_data(0x0C);
    write_data(0x03);

    write_command(S6D04H0_VCMCTL);
    write_data(0x00);
    write_data(0x2E);
    write_data(0x40);
    write_data(0x00);
    write_data(0x00);
    write_data(0x01);
    write_data(0x00);
    write_data(0x00);
    write_data(0x0D);
    write_data(0x0D);
    write_data(0x00);
    write_data(0x00);

    write_command(S6D04H0_PWRCTL);
    write_data(0x07);
    write_data(0x00);
    write_data(0x00);
    write_data(0x00);
    write_data(0x22);
    write_data(0x64);
    write_data(0x01);
    write_data(0x02);
    write_data(0x2A);
    write_data(0x4D);
    write_data(0x06);
    write_data(0x2A);
    write_data(0x00);
    write_data(0x06);
    write_data(0x00);
    write_data(0x00);
    write_data(0x00);
    write_data(0x00);
    write_data(0x00);
    write_data(0x00);

    write_command(S6D04H0_MANPWRSEQ);
    write_data(0x01);

    write_command(S6D04H0_GAMMASEL);
    write_data(0x04);

    write_command(S6D04H0_PGAMMACTL);
    write_data(0x0A);
    write_data(0x04);
    write_data(0x0C);
    write_data(0x19);
    write_data(0x25);
    write_data(0x33);
    write_data(0x2D);
    write_data(0x27);
    write_data(0x22);
    write_data(0x1E);
    write_data(0x1A);
    write_data(0x00);

    write_command(S6D04H0_NGAMMACTL);
    write_data(0x0C);
    write_data(0x04);
    write_data(0x19);
    write_data(0x1E);
    write_data(0x20);
    write_data(0x23);
    write_data(0x18);
    write_data(0x3D);
    write_data(0x25);
    write_data(0x19);
    write_data(0x0B);
    write_data(0x00);

    write_command(S6D04H0_GAMMASEL);
    write_data(0x02);

    write_command(S6D04H0_PGAMMACTL);
    write_data(0x0A);
    write_data(0x04);
    write_data(0x0C);
    write_data(0x19);
    write_data(0x25);
    write_data(0x33);
    write_data(0x2D);
    write_data(0x27);
    write_data(0x22);
    write_data(0x1E);
    write_data(0x1A);
    write_data(0x00);

    write_command(S6D04H0_NGAMMACTL);
    write_data(0x0C);
    write_data(0x04);
    write_data(0x19);
    write_data(0x1E);
    write_data(0x20);
    write_data(0x23);
    write_data(0x18);
    write_data(0x3D);
    write_data(0x25);
    write_data(0x19);
    write_data(0x0B);
    write_data(0x00);

    write_command(S6D04H0_GAMMASEL);
    write_data(0x01);

    write_command(S6D04H0_PGAMMACTL);
    write_data(0x0A);
    write_data(0x04);
    write_data(0x0C);
    write_data(0x19);
    write_data(0x25);
    write_data(0x33);
    write_data(0x2D);
    write_data(0x27);
    write_data(0x22);
    write_data(0x1E);
    write_data(0x1A);
    write_data(0x00);

    write_command(S6D04H0_NGAMMACTL);
    write_data(0x0C);
    write_data(0x04);
    write_data(0x19);
    write_data(0x1E);
    write_data(0x20);
    write_data(0x23);
    write_data(0x18);
    write_data(0x3D);
    write_data(0x25);
    write_data(0x19);
    write_data(0x0B);
    write_data(0x00);

    write_command(S6D04H0_Sleep_Out);
    udelay(150000);

    write_command(S6D04H0_PASSWD1);
    write_data(0xA5);
    write_data(0xA5);

    write_command(0xFC);
    write_data(0xA5);
    write_data(0xA5);

    write_command(S6D04H0_Display_On);

    set_window(0, 0, _width-1, _height-1);

    /*
     * Fill the gpanel_hw descriptor.
     */
    h->name           = "Samsung S6D04H0";
    h->width          = _width;
    h->height         = _height;
    h->clear          = s6d04h0_clear;
    h->set_pixel      = s6d04h0_set_pixel;
    h->fill_rectangle = s6d04h0_fill_rectangle;
    h->draw_image     = s6d04h0_draw_image;
    h->draw_glyph     = s6d04h0_draw_glyph;
}
