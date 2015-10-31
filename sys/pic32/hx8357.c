/*
 * HX8357 TFT driver for PIC32.
 *
 * Copyright (C) 2014 Majenko Technologies <matt@majenko.co.uk>
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
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/ioctl.h>
#include <sys/systm.h>
#include <sys/uio.h>
#include <sys/tty.h>
#include <sys/kconfig.h>
#include <sys/gpanel.h>

#define HX8357_EXIT_SLEEP_MODE              0x11
#define HX8357_SET_DISPLAY_OFF              0x28
#define HX8357_SET_DISPLAY_ON               0x29
#define HX8357_SET_COLUMN_ADDRESS           0x2A
#define HX8357_SET_PAGE_ADDRESS             0x2B
#define HX8357_WRITE_MEMORY_START           0x2C
#define HX8357_READ_MEMORY_START            0x2E
#define HX8357_SET_TEAR_ON                  0x35
#define HX8357_SET_ADDRESS_MODE             0x36
#define HX8357_SET_PIXEL_FORMAT             0x3A
#define HX8357_WRITE_MEMORY_CONTINUE        0x3C
#define HX8357_READ_MEMORY_CONTINUE         0x3E
#define HX8357_SET_INTERNAL_OSCILLATOR      0xB0
#define HX8357_SET_POWER_CONTROL            0xB1
#define HX8357_SET_DISPLAY_MODE             0xB4
#define HX8357_SET_VCOM_VOLTAGE             0xB6
#define HX8357_ENABLE_EXTENSION_COMMAND     0xB9
#define HX8357_SET_PANEL_DRIVING            0xC0    // not documented!
#define HX8357_SET_PANEL_CHARACTERISTIC     0xCC
#define HX8357_SET_GAMMA_CURVE              0xE0

/*
 * Display size.
 */
static int _width, _height;

/*
 * Cursor position for text output.
 */
static int _col, _row;

static void writeCommand(int c)
{
    while (PMMODE & PIC32_PMMODE_BUSY);
    PMADDR = 0x0000;
    PMDIN = c;
}

static void writeData(int c)
{
    while (PMMODE & PIC32_PMMODE_BUSY);
    PMADDR = 0x0001;
    PMDIN = c;
}

static inline void initDisplay()
{
    PMCONCLR = PIC32_PMCON_ON;
    udelay(1);

    PMCONSET = PIC32_PMCON_PTWREN | PIC32_PMCON_PTRDEN;
    PMCONCLR = PIC32_PMCON_CSF;
    PMAEN = 0x0001;             // Enable PMA0 pin for RS pin and CS1 as CS
    PMMODE = PIC32_PMMODE_MODE16 | PIC32_PMMODE_MODE_MAST2;
    PMADDR = 0;
    PMCONSET = PIC32_PMCON_ON;

    writeCommand(HX8357_EXIT_SLEEP_MODE);   //Sleep Out
    udelay(150000);
    writeCommand(HX8357_ENABLE_EXTENSION_COMMAND);
    writeData(0xFF);
    writeData(0x83);
    writeData(0x57);
    udelay(1000);
    writeCommand(HX8357_SET_POWER_CONTROL);
    writeData(0x00);
    writeData(0x12);
    writeData(0x12);
    writeData(0x12);
    writeData(0xC3);
    writeData(0x44);
    udelay(1000);
    writeCommand(HX8357_SET_DISPLAY_MODE);
    writeData(0x02);
    writeData(0x40);
    writeData(0x00);
    writeData(0x2A);
    writeData(0x2A);
    writeData(0x20);
    writeData(0x91);
    udelay(1000);
    writeCommand(HX8357_SET_VCOM_VOLTAGE);
    writeData(0x38);
    udelay(1000);
    writeCommand(HX8357_SET_INTERNAL_OSCILLATOR);
    writeData(0x68);
    writeCommand(0xE3);                     //Unknown Command
    writeData(0x2F);
    writeData(0x1F);
    writeCommand(0xB5);                     //Set BGP
    writeData(0x01);
    writeData(0x01);
    writeData(0x67);
    writeCommand(HX8357_SET_PANEL_DRIVING);
    writeData(0x70);
    writeData(0x70);
    writeData(0x01);
    writeData(0x3C);
    writeData(0xC8);
    writeData(0x08);
    udelay(1000);
    writeCommand(0xC2);                     // Set Gate EQ
    writeData(0x00);
    writeData(0x08);
    writeData(0x04);
    udelay(1000);
    writeCommand(HX8357_SET_PANEL_CHARACTERISTIC);
    writeData(0x09);
    udelay(1000);
    writeCommand(HX8357_SET_GAMMA_CURVE);
    writeData(0x01);
    writeData(0x02);
    writeData(0x03);
    writeData(0x05);
    writeData(0x0E);
    writeData(0x22);
    writeData(0x32);
    writeData(0x3B);
    writeData(0x5C);
    writeData(0x54);
    writeData(0x4C);
    writeData(0x41);
    writeData(0x3D);
    writeData(0x37);
    writeData(0x31);
    writeData(0x21);
    writeData(0x01);
    writeData(0x02);
    writeData(0x03);
    writeData(0x05);
    writeData(0x0E);
    writeData(0x22);
    writeData(0x32);
    writeData(0x3B);
    writeData(0x5C);
    writeData(0x54);
    writeData(0x4C);
    writeData(0x41);
    writeData(0x3D);
    writeData(0x37);
    writeData(0x31);
    writeData(0x21);
    writeData(0x00);
    writeData(0x01);
    udelay(1000);
    writeCommand(HX8357_SET_PIXEL_FORMAT);  //COLMOD RGB888
    writeData(0x55);
    writeCommand(HX8357_SET_ADDRESS_MODE);
    writeData(0x00);
    writeCommand(HX8357_SET_TEAR_ON);       //TE ON
    writeData(0x00);
    udelay(10000);
    writeCommand(HX8357_SET_DISPLAY_ON);    //Display On
    udelay(10000);
    writeCommand(HX8357_WRITE_MEMORY_START); //Write SRAM Data
}

static void setAddrWindow(int x0, int y0, int x1, int y1)
{
    writeCommand(HX8357_SET_COLUMN_ADDRESS); // Column addr set
    writeData(x0 >> 8);
    writeData(x0);                          // XSTART
    writeData(x1 >> 8);
    writeData(x1);                          // XEND

    writeCommand(HX8357_SET_PAGE_ADDRESS);  // Row addr set
    writeData(y0 >> 8);
    writeData(y0);                          // YSTART
    writeData(y1 >> 8);
    writeData(y1);                          // YEND

    writeCommand(HX8357_WRITE_MEMORY_START); //Write SRAM Data
}

static void setRotation(int rotation)
{
    writeCommand(HX8357_SET_ADDRESS_MODE);
    switch (rotation & 3) {
    case 0:                     /* Portrait */
        writeData(0x0000);
        _width  = 320;
        _height = 480;
        break;
    case 1:                     /* Landscape */
        writeData(0x0060);
        _width  = 480;
        _height = 320;
        break;
    case 2:                     /* Upside down portrait */
        writeData(0x00C0);
        _width  = 320;
        _height = 480;
        break;
    case 3:                     /* Upside down landscape */
        writeData(0x00A0);
        _width  = 480;
        _height = 320;
        break;
    }
}

/*
 * Draw a pixel.
 */
static void setPixel(int x, int y, int color)
{
    if (x < 0 || x >= _width || y < 0 || y >= _height)
        return;
    setAddrWindow(x, y, x+1, y+1);
    while (PMMODE & PIC32_PMMODE_BUSY);
    PMADDR = 0x0001;
    PMDIN = color;
}

/*
 * Fill a rectangle with specified color.
 */
static void fillRectangle(int x0, int y0, int x1, int y1, int color)
{
    int x, y;

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
    setAddrWindow(x0, y0, x1, y1);

    while (PMMODE & PIC32_PMMODE_BUSY);
    PMADDR = 0x0001;
    for (y=y0; y<=y1; y++) {
        for (x=x0; x<=x1; x++) {
            while (PMMODE & PIC32_PMMODE_BUSY);
            PMDIN = color;
        }
    }
}

/*
 * Fill a rectangle with user data.
 */
static void drawImage(int x, int y, int width, int height,
    const unsigned short *data)
{
    unsigned cnt = width * height;

    setAddrWindow(x, y, x + width - 1, y + height - 1);
    while (PMMODE & PIC32_PMMODE_BUSY);
    PMADDR = 0x0001;
    while (cnt--) {
        while (PMMODE & PIC32_PMMODE_BUSY);
        PMDIN = *data++;
    }
}

/*
 * Draw a line.
 */
static void drawLine(int x0, int y0, int x1, int y1, int color)
{
    int dx, dy, stepx, stepy, fraction;

    if (x0 == x1 || y0 == y1) {
        fillRectangle(x0, y0, x1, y1, color);
        return;
    }

    /* Use Bresenham's line algorithm. */
    dy = y1 - y0;
    if (dy < 0) {
        dy = -dy;
        stepy = -1;
    } else {
        stepy = 1;
    }
    dx = x1 - x0;
    if (dx < 0) {
        dx = -dx;
        stepx = -1;
    } else {
        stepx = 1;
    }
    dy <<= 1;                           /* dy is now 2*dy */
    dx <<= 1;                           /* dx is now 2*dx */
    setPixel(x0, y0, color);
    if (dx > dy) {
        fraction = dy - (dx >> 1);      /* same as 2*dy - dx */
        while (x0 != x1) {
            if (fraction >= 0) {
                y0 += stepy;
                fraction -= dx;         /* same as fraction -= 2*dx */
            }
            x0 += stepx;
            fraction += dy;             /* same as fraction -= 2*dy */
            setPixel(x0, y0, color);
        }
    } else {
        fraction = dx - (dy >> 1);
        while (y0 != y1) {
            if (fraction >= 0) {
                x0 += stepx;
                fraction -= dy;
            }
            y0 += stepy;
            fraction += dx;
            setPixel(x0, y0, color);
        }
    }
}

/*
 * Draw a rectangular frame.
 */
static void drawFrame(int x0, int y0, int x1, int y1, int color)
{
    fillRectangle(x0, y0, x1, y0, color);
    fillRectangle(x0, y1, x1, y1, color);
    fillRectangle(x0, y0, x0, y1, color);
    fillRectangle(x1, y0, x1, y1, color);
}

/*
 * Draw a circle.
 */
static void drawCircle(int x0, int y0, int radius, int color)
{
    int f = 1 - radius;
    int ddF_x = 0;
    int ddF_y = -2 * radius;
    int x = 0;
    int y = radius;

    setPixel(x0, y0 + radius, color);
    setPixel(x0, y0 - radius, color);
    setPixel(x0 + radius, y0, color);
    setPixel(x0 - radius, y0, color);
    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x + 1;
        setPixel(x0 + x, y0 + y, color);
        setPixel(x0 - x, y0 + y, color);
        setPixel(x0 + x, y0 - y, color);
        setPixel(x0 - x, y0 - y, color);
        setPixel(x0 + y, y0 + x, color);
        setPixel(x0 - y, y0 + x, color);
        setPixel(x0 + y, y0 - x, color);
        setPixel(x0 - y, y0 - x, color);
    }
}

/*
 * Start a new line: increase row.
 */
static void newLine(const struct gpanel_font_t *font)
{
    _col = 0;
    _row += font->height;
    if (_row > _height - font->height)
        _row = 0;
}

/*
 * Draw a glyph of one symbol.
 */
void drawGlyph(const struct gpanel_font_t *font,
    int color, int background, int width, const unsigned short *bits)
{
    int h, w;
    unsigned bitmask = 0;

    if (background >= 0) {
        /*
         * Clear background.
         */
        setAddrWindow(_col, _row, _col + width - 1, _row + font->height - 1);
        while (PMMODE & PIC32_PMMODE_BUSY);
        PMADDR = 0x0001;

        /* Loop on each glyph row. */
        for (h=0; h<font->height; h++) {
            /* Loop on every pixel in the row (left to right). */
            for (w=0; w<width; w++) {
                if ((w & 15) == 0)
                    bitmask = *bits++;
                else
                    bitmask <<= 1;

                while (PMMODE & PIC32_PMMODE_BUSY)
                    ;
                if (bitmask & 0x8000)
                    PMDIN = color;
                else
                    PMDIN = background;
            }
        }
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
                    setPixel(_col + w, _row + h, color);
            }
        }
    }
}

/*
 * Draw a character from a specified font.
 */
static void drawChar(const struct gpanel_font_t *font,
    int color, int background, int sym)
{
    unsigned cindex, width;
    const unsigned short *bits;

    switch (sym) {
    case '\n':      /* goto next line */
        newLine(font);
        return;
    case '\r':      /* carriage return - go to begin of line */
        _col = 0;
        return;
    case '\t':      /* tab replaced by space */
        sym = ' ';
        break;
    }

    if (sym < font->firstchar || sym >= font->firstchar + font->size)
        sym = font->defaultchar;
    cindex = sym - font->firstchar;

    /* Get font bitmap depending on fixed pitch or not. */
    if (font->width) {
        /* Proportional font. */
        width = font->width[cindex];
    } else {
        /* Fixed width font. */
        width = font->maxwidth;
    }
    if (font->offset) {
        bits = font->bits + font->offset[cindex];
    } else {
        bits = font->bits + cindex * font->height;
    }

    /* Scrolling. */
    if (_col > _width - width) {
        newLine(font);
    }

    /* Draw a character. */
    drawGlyph(font, color, background, width, bits);
    _col += width;
}

/*
 * Draw a string of characters.
 * TODO: Decode UTF-8.
 */
static void drawText(const struct gpanel_font_t *font,
    int color, int background, int x, int y, const char *text)
{
    int sym;

    _col = x;
    _row = y;
    for (;;) {
        sym = *text++;
        if (! sym)
            break;

        drawChar(font, color, background, sym);
    }
}

int gpanel_open(dev_t dev, int flag, int mode)
{
    if (minor(dev) != 0)
        return ENODEV;
    return 0;
}

int gpanel_close(dev_t dev, int flag, int mode)
{
    return 0;
}

int gpanel_read(dev_t dev, struct uio *uio, int flag)
{
    return ENODEV;
}

int gpanel_write(dev_t dev, struct uio *uio, int flag)
{
    return ENODEV;
}

/*
 * TODO: check whether user pointers are valid.
 */
int gpanel_ioctl(dev_t dev, register u_int cmd, caddr_t addr, int flag)
{
    switch (cmd) {
        /*
         * Clear the whole screen with a given color.
         */
        case GPANEL_CLEAR: {
            struct gpanel_clear_t *param = (struct gpanel_clear_t*) addr;

            if (param->xsize != _width || param->ysize != _height) {
                /* Change the screen orientation. */
                if (param->xsize > param->ysize) {
                    /* Landscape */
                    setRotation(1);
                } else if (param->xsize < param->ysize) {
                    /* Portrait */
                    setRotation(0);
                }
            }
            fillRectangle(0, 0, _width, _height, param->color);
            param->xsize = _width;
            param->ysize = _height;
            break;
        }

        /*
         * Draw a single pixel.
         */
        case GPANEL_PIXEL: {
            struct gpanel_pixel_t *param = (struct gpanel_pixel_t*) addr;

            setPixel(param->x, param->y, param->color);
            break;
        }

        /*
         * Draw a line.
         */
        case GPANEL_LINE: {
            struct gpanel_line_t *param = (struct gpanel_line_t*) addr;

            drawLine(param->x0, param->y0, param->x1, param->y1, param->color);
            break;
        }

        /*
         * Draw a rectangle frame.
         */
        case GPANEL_RECT: {
            struct gpanel_rect_t *param = (struct gpanel_rect_t*) addr;

            drawFrame(param->x0, param->y0, param->x1, param->y1, param->color);
            break;
        }

        /*
         * Fill a rectangle with color.
         */
        case GPANEL_FILL: {
            struct gpanel_rect_t *param = (struct gpanel_rect_t*) addr;

            fillRectangle(param->x0, param->y0, param->x1, param->y1, param->color);
            break;
        }

        /*
         * Draw a circle.
         */
        case GPANEL_CIRCLE: {
            struct gpanel_circle_t *param = (struct gpanel_circle_t*) addr;

            drawCircle(param->x, param->y, param->radius, param->color);
            break;
        }

        /*
         * Fill a rectangular area with the user-supplied data.
         */
        case GPANEL_IMAGE: {
            struct gpanel_image_t *param = (struct gpanel_image_t*) addr;

            drawImage(param->x, param->y, param->width, param->height,
                param->image);
            break;
        }

        /*
         * Draw a character.
         */
        case GPANEL_CHAR: {
            struct gpanel_char_t *param = (struct gpanel_char_t*) addr;

            _col = param->x;
            _row = param->y;
            drawChar(param->font, param->color, param->background, param->sym);
            break;
        }

        /*
         * Draw a string of characters.
         */
        case GPANEL_TEXT: {
            struct gpanel_text_t *param = (struct gpanel_text_t*) addr;

            drawText(param->font, param->color, param->background,
                param->x, param->y, param->text);
            break;
        }
    }
    return 0;
}

/*
 * Test to see if device is present.
 * Return true if found and initialized ok.
 */
static int
hxtftprobe(config)
    struct conf_device *config;
{
    initDisplay();
    setRotation(1);
    printf("hxtft0: display %ux%u\n", _width, _height);
    return 1;
}

struct driver hxtftdriver = {
    "hxtft", hxtftprobe,
};
