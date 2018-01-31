/*
 * Display driver for NT35702 LCD controller.
 * This controller is partially compatible with ILI9341 chip,
 * so we can reuse most of ili_xxx() routines.
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
 * Memory Access Control register
 */
#define MADCTL_MY           0x80    /* Row address order */
#define MADCTL_MX           0x40    /* Column address order */
#define MADCTL_MV           0x20    /* Row/column exchange */
#define MADCTL_ML           0x10    /* Vertical refresh order */
#define MADCTL_BGR          0x08    /* Color filter selector: 0=RGB, 1=BGR */
#define MADCTL_MH           0x04    /* Horisontal refresh direction: 1=left-to-right */

/*
 * Reuse ILI9341 routines.
 */
extern void ili_set_pixel(int x, int y, int color);
extern void ili_fill_rectangle(int x0, int y0, int x1, int y1, int color);
extern void ili_draw_image(int x, int y, int width, int height,
    const unsigned short *data);
extern void ili_draw_glyph(const struct gpanel_font_t *font,
    int color, int background, int x, int y, int width,
    const unsigned short *bits);

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
 * Switch the screen orientation.
 */
static void set_rotation(int rotation)
{
    write_command(NT35702_MADCTL);
    switch (rotation & 3) {
    case 0:                     /* Portrait */
        write_data(MADCTL_MX | MADCTL_MY | MADCTL_BGR);
        gpanel_width  = 240;
        gpanel_height = 320;
        break;
    case 1:                     /* Landscape */
        write_data(MADCTL_MY | MADCTL_MV | MADCTL_BGR);
        gpanel_width  = 320;
        gpanel_height = 240;
        break;
    case 2:                     /* Upside down portrait */
        write_data(MADCTL_BGR);
        gpanel_width  = 240;
        gpanel_height = 320;
        break;
    case 3:                     /* Upside down landscape */
        write_data(MADCTL_MX | MADCTL_MV | MADCTL_BGR);
        gpanel_width  = 320;
        gpanel_height = 240;
        break;
    }
}

static void nt35702_resize(struct gpanel_hw *h, int width, int height)
{
    gpanel_cs_active();

    /* Switch screen orientaation. */
    if (width > height)
        set_rotation(1);        /* Landscape */
    else if (width < height)
        set_rotation(0);        /* Portrait */

    gpanel_cs_idle();
}

/*
 * Initialize the LCD controller.
 * Fill the gpanel_hw descriptor.
 */
void nt35702_init_display(struct gpanel_hw *h)
{
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
    h->resize         = nt35702_resize;
    h->set_pixel      = ili_set_pixel;
    h->fill_rectangle = ili_fill_rectangle;
    h->draw_image     = ili_draw_image;
    h->draw_glyph     = ili_draw_glyph;
}
