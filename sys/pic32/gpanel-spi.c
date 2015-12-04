/*
 * Generic TFT LCD driver with SPI interface.
 * Supported chips: ILI9341.
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
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/ioctl.h>
#include <sys/systm.h>
#include <sys/uio.h>
#include <sys/tty.h>
#include <sys/kconfig.h>
#include <sys/spi.h>
#include <sys/gpanel.h>

#define SGPANEL_KHZ     10000   /* clock 10 MHz */

/*
 * Descriptor for access to the hardware-level driver.
 */
static struct gpanel_hw hw;

/*
 * Cursor position for text output.
 */
static int _col, _row;

/*
 * ID of the LCD controller chip.
 */
static int _chip_id;

/*
 * Data/command signal pin.
 */
static int _dc;
static int _last_dc = -1;       /* last DC value */

static struct spiio _spiio;

/*
 * Display size.
 */
int gpanel_width, gpanel_height;

/*
 * Control /CS signal.
 */
void gpanel_cs_active() { spi_select(&_spiio); }
void gpanel_cs_idle()   { spi_deselect(&_spiio); }

/*
 * Send a command to the SPI bus.
 */
int gpanel_send_command(int value)
{
    if (_last_dc != 0) {
        gpio_clr(_dc);
        _last_dc = 0;
    }
    return spi_transfer(&_spiio, value);
}

/*
 * Send data to the SPI bus.
 */
int gpanel_send_data(int value)
{
    if (_last_dc != 1) {
        gpio_set(_dc);
        _last_dc = 1;
    }
    return spi_transfer(&_spiio, value);
}

/*
 * Draw a line.
 */
static void gpanel_draw_line(int x0, int y0, int x1, int y1, int color)
{
    int dx, dy, stepx, stepy, fraction;

    if (x0 == x1 || y0 == y1) {
        hw.fill_rectangle(x0, y0, x1, y1, color);
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
    hw.set_pixel(x0, y0, color);
    if (dx > dy) {
        fraction = dy - (dx >> 1);      /* same as 2*dy - dx */
        while (x0 != x1) {
            if (fraction >= 0) {
                y0 += stepy;
                fraction -= dx;         /* same as fraction -= 2*dx */
            }
            x0 += stepx;
            fraction += dy;             /* same as fraction -= 2*dy */
            hw.set_pixel(x0, y0, color);
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
            hw.set_pixel(x0, y0, color);
        }
    }
}

/*
 * Draw a rectangular frame.
 */
static void gpanel_draw_frame(int x0, int y0, int x1, int y1, int color)
{
    hw.fill_rectangle(x0, y0, x1, y0, color);
    hw.fill_rectangle(x0, y1, x1, y1, color);
    hw.fill_rectangle(x0, y0, x0, y1, color);
    hw.fill_rectangle(x1, y0, x1, y1, color);
}

/*
 * Draw a circle.
 */
static void gpanel_draw_circle(int x0, int y0, int radius, int color)
{
    int f = 1 - radius;
    int ddF_x = 0;
    int ddF_y = -2 * radius;
    int x = 0;
    int y = radius;

    hw.set_pixel(x0, y0 + radius, color);
    hw.set_pixel(x0, y0 - radius, color);
    hw.set_pixel(x0 + radius, y0, color);
    hw.set_pixel(x0 - radius, y0, color);
    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x + 1;
        hw.set_pixel(x0 + x, y0 + y, color);
        hw.set_pixel(x0 - x, y0 + y, color);
        hw.set_pixel(x0 + x, y0 - y, color);
        hw.set_pixel(x0 - x, y0 - y, color);
        hw.set_pixel(x0 + y, y0 + x, color);
        hw.set_pixel(x0 - y, y0 + x, color);
        hw.set_pixel(x0 + y, y0 - x, color);
        hw.set_pixel(x0 - y, y0 - x, color);
    }
}

/*
 * Draw a character from a specified font.
 */
static void gpanel_draw_char(const struct gpanel_font_t *font,
    int color, int background, int sym)
{
    unsigned cindex, width;
    const unsigned short *bits;

    switch (sym) {
    case '\n':      /* goto next line */
        _row += font->height;
        _col = 0;
        if (_row > gpanel_height - font->height)
            _row = 0;
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

    /* Draw a character. */
    hw.draw_glyph(font, color, background, _col, _row, width, bits);
    _col += width;
}

/*
 * Draw a string of characters.
 * TODO: Decode UTF-8.
 */
static void gpanel_draw_text(const struct gpanel_font_t *font,
    int color, int background, int x, int y, const char *text)
{
    int sym;

    _col = x;
    _row = y;
    for (;;) {
        sym = *text++;
        if (! sym)
            break;

        gpanel_draw_char(font, color, background, sym);
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

            if (hw.resize != 0)
                hw.resize(&hw, param->xsize, param->ysize);
            hw.fill_rectangle(0, 0, gpanel_width - 1, gpanel_height - 1, param->color);
            param->xsize = gpanel_width;
            param->ysize = gpanel_height;
            break;
        }

        /*
         * Draw a single pixel.
         */
        case GPANEL_PIXEL: {
            struct gpanel_pixel_t *param = (struct gpanel_pixel_t*) addr;

            hw.set_pixel(param->x, param->y, param->color);
            break;
        }

        /*
         * Draw a line.
         */
        case GPANEL_LINE: {
            struct gpanel_line_t *param = (struct gpanel_line_t*) addr;

            gpanel_draw_line(param->x0, param->y0, param->x1, param->y1, param->color);
            break;
        }

        /*
         * Draw a rectangle frame.
         */
        case GPANEL_RECT: {
            struct gpanel_rect_t *param = (struct gpanel_rect_t*) addr;

            gpanel_draw_frame(param->x0, param->y0, param->x1, param->y1, param->color);
            break;
        }

        /*
         * Fill a rectangle with color.
         */
        case GPANEL_FILL: {
            struct gpanel_rect_t *param = (struct gpanel_rect_t*) addr;

            hw.fill_rectangle(param->x0, param->y0, param->x1, param->y1, param->color);
            break;
        }

        /*
         * Draw a circle.
         */
        case GPANEL_CIRCLE: {
            struct gpanel_circle_t *param = (struct gpanel_circle_t*) addr;

            gpanel_draw_circle(param->x, param->y, param->radius, param->color);
            break;
        }

        /*
         * Fill a rectangular area with the user-supplied data.
         */
        case GPANEL_IMAGE: {
            struct gpanel_image_t *param = (struct gpanel_image_t*) addr;

            hw.draw_image(param->x, param->y, param->width, param->height,
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
            gpanel_draw_char(param->font, param->color, param->background, param->sym);
            break;
        }

        /*
         * Draw a string of characters.
         */
        case GPANEL_TEXT: {
            struct gpanel_text_t *param = (struct gpanel_text_t*) addr;

            gpanel_draw_text(param->font, param->color, param->background,
                param->x, param->y, param->text);
            break;
        }
    }
    return 0;
}

/*
 * Draw a BSD logo on the screen.
 */
static void draw_logo()
{
#define K       7
#define COLOR_B 0xf81f
#define COLOR_S 0x07ff
#define COLOR_D 0xffe0

    int x = gpanel_width/2  - 17*K;
    int y = gpanel_height/2 + 11*K;

    hw.fill_rectangle(0, 0, gpanel_width - 1, gpanel_height - 1, 0);

    /* B */
    gpanel_draw_line( 0*K+x, y- 0*K,  0*K+x, y-11*K, COLOR_B);
    gpanel_draw_line( 0*K+x, y-11*K,  0*K+x, y-22*K, COLOR_B);
    gpanel_draw_line( 0*K+x, y-22*K, 10*K+x, y-19*K, COLOR_B);
    gpanel_draw_line(10*K+x, y-19*K,  0*K+x, y-11*K, COLOR_B);
    gpanel_draw_line( 0*K+x, y-11*K, 10*K+x, y- 8*K, COLOR_B);
    gpanel_draw_line(10*K+x, y- 8*K,  0*K+x, y- 0*K, COLOR_B);

    /* S */
    gpanel_draw_line(22*K+x, y-22*K, 12*K+x, y-19*K, COLOR_S);
    gpanel_draw_line(12*K+x, y-19*K, 22*K+x, y- 8*K, COLOR_S);
    gpanel_draw_line(22*K+x, y- 8*K, 12*K+x, y- 0*K, COLOR_S);

    /* D */
    gpanel_draw_line(24*K+x, y-22*K, 24*K+x, y- 0*K, COLOR_D);
    gpanel_draw_line(24*K+x, y-22*K, 34*K+x, y-19*K, COLOR_D);
    gpanel_draw_line(34*K+x, y-19*K, 34*K+x, y- 8*K, COLOR_D);
    gpanel_draw_line(34*K+x, y- 8*K, 24*K+x, y- 0*K, COLOR_D);
}

/*
 * Read the chip ID register.
 */
static int read_id()
{
    int id;

    /* Read ID4 from register #D3.
     * Use undocumented command #D9 to get access to ID4. */
    gpanel_cs_active();
    gpanel_send_command(0xD9);
    gpanel_send_data(0x11);
    gpanel_send_command(0xD3);
    id = gpanel_send_data(0xff) << 16;

    gpanel_send_command(0xD9);
    gpanel_send_data(0x12);
    gpanel_send_command(0xD3);
    id |= gpanel_send_data(0xff) << 8;

    gpanel_send_command(0xD9);
    gpanel_send_data(0x13);
    gpanel_send_command(0xD3);
    id |= gpanel_send_data(0xff);
    gpanel_cs_idle();
    return id;
}

/*
 * Detect the type of the LCD controller, and initialize it.
 * Return true if found and initialized ok.
 */
static int probe(config)
    struct conf_device *config;
{
    int cs;
    struct spiio *io = &_spiio;

    /* Only one device is allowed. */
    if (config->dev_unit > 0)
        return 0;
    cs = config->dev_pins[0];
    _dc = config->dev_pins[1];
    printf("sgpanel: port SPI%d, pins cs=R%c%d/dc=R%c%d\n",
        config->dev_ctlr, gpio_portname(cs), gpio_pinno(cs),
        gpio_portname(_dc), gpio_pinno(_dc));

    if (spi_setup(io, config->dev_ctlr, cs) != 0) {
        printf("sgpanel: cannot open SPI%u port\n", config->dev_ctlr);
        return 0;
    }
    spi_brg(io, SGPANEL_KHZ);
    spi_set(io, PIC32_SPICON_CKE);
    gpio_set_output(_dc);

    /* Identify the LCD controller chip. */
    _chip_id = read_id();
    switch (_chip_id) {
    default:
        /* Disable outputs. */
        gpio_set_input(_dc);
        gpio_set_input(cs);
        printf("gpanel0: Unknown chip ID = 0x%04x\n", _chip_id);
        return 0;

    case 0x009341:
        /* Ilitek ILI9341. */
        ili9341_init_display(&hw);
        break;
    }
    printf("gpanel0: <%s> display %ux%u\n", hw.name, gpanel_width, gpanel_height);
    draw_logo();
    return 1;
}

struct driver sgpaneldriver = {
    "sgpanel", probe,
};
