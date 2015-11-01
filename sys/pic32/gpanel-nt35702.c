/*
 * Display driver for NT35702 LCD controller.
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
 * NT35702 registers.
 */
#define NT35702_NOP         0x00    /* No Operation */
#define NT35702_SWRESET     0x01    /* Software reset */
#define NT35702_RDDID       0x04    /* Read Display ID */
#define NT35702_RDDST       0x09    /* Read Display Status */
#define NT35702_RDDPM       0x0A    /* Read Display Power Mode */
#define NT35702_RDDMADCTR   0x0B    /* Read Display MADCTR */
#define NT35702_RDDCOLMOD   0x0C    /* Read Display Pixel Format */
#define NT35702_RDDIM       0x0D    /* Read Display Image Mode */
#define NT35702_RDDSM       0x0E    /* Read Display Signal Mode */
#define NT35702_RDDSDR      0x0F    /* Read Display Self-diagnostic result */
#define NT35702_SLPIN       0x10    /* Sleep in & booster off */
#define NT35702_SLPOUT      0x11    /* Sleep out & booster on */
#define NT35702_PTLON       0x12    /* Partial mode on */
#define NT35702_NORON       0x13    /* Partial off (Normal) */
#define NT35702_DSBCTL      0x15    /* Deep Standby mode control */
#define NT35702_INVOFF      0x20    /* Inversion off (normal) */
#define NT35702_INVON       0x21    /* Inversion on */
#define NT35702_DISPOFF     0x28    /* Display off */
#define NT35702_DISPON      0x29    /* Display on */
#define NT35702_CASET       0x2A    /* Column address set */
#define NT35702_RASET       0x2B    /* Row address set */
#define NT35702_RAMWR       0x2C    /* Memory write */
#define NT35702_PTLAR       0x30    /* Partial start/end address set */
#define NT35702_SCRLAR      0x33    /* Scroll area set */
#define NT35702_TEOFF       0x34    /* Tearing effect line off */
#define NT35702_TEON        0x35    /* Tearing effect mode set & on */
#define NT35702_MADCTL      0x36    /* Memory data access control */
#define NT35702_VSCSAD      0x37    /* Scroll start address of RAM */
#define NT35702_IDMOFF      0x38    /* Idle mode off */
#define NT35702_IDMON       0x39    /* Idle mode on */
#define NT35702_COLMOD      0x3A    /* Interface pixel format */
#define NT35702_WRDISBV     0x51    /* Write Display Brightness */
#define NT35702_WRCTRLD     0x53    /* Write CTRL Display */
#define NT35702_WRCABC      0x55    /* Write Content Adaptive Brightness Control */
#define NT35702_WRCABCMB    0x5E    /* Write CABC minimum brightness */
#define NT35702_RDPWM       0x6A    /* Read CABC Brightness */
#define NT35702_WRPWMF      0x6B    /* Write the PWM Frequency for CABC */
#define NT35702_CABC_FOR_CE 0x77    /* Force CABC PWM in Some Conditions */
#define NT35702_CABCDMT     0x79    /* Set Dimming Time Length for CABC */
#define NT35702_RDID1       0xDA    /* Read IDB */
#define NT35702_RDID2       0xDB    /* Read ID2 */
#define NT35702_RDID3       0xDC    /* Read ID3 */

#define NT35702_INVCTR      0xB4    /* Display inversion control */
#define NT35702_DISSET5     0xB6    /* Display function setting */
#define NT35702_SDOCTR      0xB7    /* SD output direction control */
#define NT35702_GDOCTR      0xB8    /* GD output direction control */
#define NT35702_PWCTR1      0xC0    /* Power control setting
                                     * VRH: Set the GVDD */
#define NT35702_PWCTR2      0xC1    /* Power control setting */
#define NT35702_PWCTR3      0xC2    /* In normal mode (Full colors)
                                     * AP: adjust the operational amplifier
                                     * DC: adjust the booster circuit for normal mode */
#define NT35702_PWCTR4      0xC3    /* In Idle mode (8-colors)
                                     * AP: adjust the operational amplifier
                                     * DC: adjust the booster circuit for Idle mode */
#define NT35702_PWCTR5      0xC4    /* In partial mode + Full colors
                                     * AP: adjust the operational amplifier
                                     * DC: adjust the booster circuit for Idle mode */
#define NT35702_VMCTR1      0xC5    /* VMH: VCOMH voltage control
                                     * VML: VCOML voltage control */
#define NT35702_VMOFCTR     0xC7    /* VMF: VCOM offset control */
#define NT35702_RVMOFCTR    0xC8    /* Read the VMOF value form NV memory */
#define NT35702_WRID2       0xD1    /* LCM version code
                                     * Write ID2 value to NV memory */
#define NT35702_WRID3       0xD2    /* Customer Project code
                                     * Write ID3 value to NV memory */
#define NT35702_RDID4       0xD3    /* ID41: IC vendor code
                                     * ID42: IC part number code
                                     * ID43 & ID44: chip version code */
#define NT35702_MTP         0xD4    /* MTP access program enable */
#define NT35702_EPWRITE     0xD5    /* NV write command */
#define NT35702_MTPSUP      0xD7    /* MTP speed up */
#define NT35702_GAMCTRP1    0xE0    /* Gamma adjustment (+ polarity) */
#define NT35702_GAMCTRN1    0xE1    /* Gamma adjustment (- polarity) */
#define NT35702_FRMCTR      0xFA    /* Frame rate control */
#define NT35702_AVDDCLP     0xFD    /* AVDD Clamp Voltage */

/*
 * Write a 8-bit value to the NT35702 Command register.
 */
static void write_command(int cmd)
{
    gpanel_rs_command();
    gpanel_write_byte(cmd);
}

/*
 * Write a 8-bit value to the NT35702 Data register.
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
    write_command(NT35702_CASET);
    write_data(x0 >> 8);
    gpanel_write_byte(x0);
    gpanel_write_byte(x1 >> 8);
    gpanel_write_byte(x1);

    write_command(NT35702_RASET);
    write_data(y0 >> 8);
    gpanel_write_byte(y0);
    gpanel_write_byte(y1 >> 8);
    gpanel_write_byte(y1);

    write_command(NT35702_RAMWR);
}

/*
 * Draw a pixel.
 */
static void nt35702_set_pixel(int x, int y, int color)
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
    write_command(NT35702_MADCTL);
    switch (rotation & 3) {
    case 0:                     /* Portrait */
        write_data(0xC8);
        _width  = 240;
        _height = 320;
        break;
    case 1:                     /* Landscape */
        write_data(0xA8);
        _width  = 320;
        _height = 240;
        break;
    case 2:                     /* Upside down portrait */
        write_data(0x08);
        _width  = 240;
        _height = 320;
        break;
    case 3:                     /* Upside down landscape */
        write_data(0x68);
        _width  = 320;
        _height = 240;
        break;
    }
}

static void nt35702_clear(struct gpanel_hw *h, int color, int width, int height)
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
static void nt35702_fill_rectangle(int x0, int y0, int x1, int y1, int color)
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
static void nt35702_draw_image(int x, int y, int width, int height,
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
static void nt35702_draw_glyph(const struct gpanel_font_t *font,
    int color, int background, int x, int y, int width,
    const unsigned short *bits)
{
    int h, w, c;
    unsigned bitmask = 0;

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
                    nt35702_set_pixel(x + w, y + h, color);
            }
        }
    }
}

/*
 * Initialize the LCD controller.
 * Fill the gpanel_hw descriptor.
 */
void nt35702_init_display(struct gpanel_hw *h)
{
    gpanel_cs_active();
    gpanel_cs_active();
    write_command(NT35702_SWRESET);
    udelay(20000);

    write_command(NT35702_SLPOUT);
    udelay(120000);

    write_command(NT35702_PWCTR3);
    write_data(0x05);               // APA2 APA1 APA0   Large
    write_data(0x00);               // Step-up cycle in Booster circuit 1
                                    // Step-up cycle in Booster circuit 2,3
    write_command(NT35702_PWCTR4);
    write_data(0x05);               // APA2 APA1 APA0   Large
    write_data(0x00);               // Step-up cycle in Booster circuit 1
                                    // Step-up cycle in Booster circuit 2,3
    write_command(NT35702_PWCTR5);
    write_data(0x05);               // APA2 APA1 APA0   Large
    write_data(0x00);               // Step-up cycle in Booster circuit 1
                                    // Step-up cycle in Booster circuit 2,3
    write_command(NT35702_COLMOD);
    write_data(0x55);

    write_command(NT35702_MTPSUP);
    write_data(0x40);
    write_data(0xE0);

    write_command(NT35702_AVDDCLP);
    write_data(0x06);
    write_data(0x11);

    write_command(NT35702_FRMCTR);
    write_data(0x38);
    write_data(0x20);
    write_data(0x1C);
    write_data(0x10);
    write_data(0x37);
    write_data(0x12);
    write_data(0x22);
    write_data(0x1E);

    write_command(NT35702_PWCTR1);  // Set GVDD
    write_data(0x05);

    write_command(NT35702_VMCTR1);  // Set Vcom
    write_data(0x60);
    write_data(0x00);

    write_command(NT35702_VMOFCTR); // Set VCOM-OFFSET
    write_data(0xA9);               // You can fine-tune to improve flicker

    set_rotation(1);                /* Landscape */

    write_command(NT35702_GAMCTRP1);
    write_data(0x22);
    write_data(0x23);
    write_data(0x25);
    write_data(0x08);
    write_data(0x10);
    write_data(0x14);
    write_data(0x40);
    write_data(0x7B);
    write_data(0x50);
    write_data(0x0B);
    write_data(0x1B);
    write_data(0x22);
    write_data(0x20);
    write_data(0x2F);
    write_data(0x37);

    write_command(NT35702_GAMCTRN1);
    write_data(0x0C);
    write_data(0x14);
    write_data(0x23);
    write_data(0x0E);
    write_data(0x14);
    write_data(0x15);
    write_data(0x36);
    write_data(0x59);
    write_data(0x46);
    write_data(0x0B);
    write_data(0x1F);
    write_data(0x27);
    write_data(0x1F);
    write_data(0x20);
    write_data(0x22);

    write_command(NT35702_DISPON);

    write_command(NT35702_RAMWR);
    gpanel_cs_idle();

    /*
     * Fill the gpanel_hw descriptor.
     */
    h->name           = "Novatek NT35702";
    h->width          = _width;
    h->height         = _height;
    h->clear          = nt35702_clear;
    h->set_pixel      = nt35702_set_pixel;
    h->fill_rectangle = nt35702_fill_rectangle;
    h->draw_image     = nt35702_draw_image;
    h->draw_glyph     = nt35702_draw_glyph;
}
