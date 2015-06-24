/*
 * HX8357 TFT driver for PIC32.
 *
 * Copyright (C) 2014 Majenko Technologies <matt@majenko.co.uk>
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
#include "uio.h"
#include "adc.h"
#include "debug.h"
#include "hx8357.h"

#include "fonts/default.h"

char frame[40][80];

typedef struct {
    unsigned char linesPerCharacter;
    unsigned char bytesPerLine;
    unsigned char startGlyph;
    unsigned char endGlyph;
    unsigned char bitsPerPixel;
} FontHeader;

struct tty hx8357_ttys[1];

typedef struct {
    union {
        unsigned short value;
        struct {
            unsigned r:5;
            unsigned g:6;
            unsigned b:5;
        } __attribute__((packed));
    } __attribute__((packed));
} __attribute__((packed)) Color565;


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

static int _width = 320;
static int _height = 480;

static int cursor_x = 0;
static int cursor_y = 0;

static int rotation = 0;

static unsigned short textcolor = 0x7BEF;
static unsigned short texthicolor = 0xFFFF;
static unsigned short textbgcolor = 0x0000;

const unsigned char *font = Default;
const unsigned char _font_width = 6;
const unsigned char _font_height = 8;

const struct devspec hx8357devs[] = {
    { 0, "tft0" },
    { 1, "tftin0" },
    { 0, 0 }
};

void inline static writeCommand(unsigned short c)
{
    while (PMMODE & PIC32_PMMODE_BUSY);
    PMADDR = 0x0000;
    PMDIN = c;
}

void inline static writeData(unsigned short c)
{
    while (PMMODE & PIC32_PMMODE_BUSY);
    PMADDR = 0x0001;
    PMDIN = c;
}

void initDisplay()
{
    PMCONCLR = PIC32_PMCON_ON;
    asm volatile("nop");

    PMCONSET = PIC32_PMCON_PTWREN | PIC32_PMCON_PTRDEN;
    PMCONCLR = PIC32_PMCON_CSF;
    PMAEN = 0x0001; // Enable PMA0 pin for RS pin and CS1 as CS
    PMMODE = PIC32_PMMODE_MODE16 | PIC32_PMMODE_MODE_MAST2;
    PMADDR = 0;
    PMCONSET = PIC32_PMCON_ON;

    writeCommand(HX8357_EXIT_SLEEP_MODE); //Sleep Out
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
    writeCommand(0xE3); //Unknown Command
    writeData(0x2F);
    writeData(0x1F);
    writeCommand(0xB5); //Set BGP
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
    writeCommand(0xC2); // Set Gate EQ
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
    writeCommand(HX8357_SET_PIXEL_FORMAT); //COLMOD RGB888
    writeData(0x55);
    writeCommand(HX8357_SET_ADDRESS_MODE);
    writeData(0x00);
    writeCommand(HX8357_SET_TEAR_ON); //TE ON
    writeData(0x00);
    udelay(10000);
    writeCommand(HX8357_SET_DISPLAY_ON); //Display On
    udelay(10000);
    writeCommand(HX8357_WRITE_MEMORY_START); //Write SRAM Data
    int l, c;
    for (l = 0; l < 39; l++) {
        for (c = 0; c < 80; c++) {
            frame[l][c]=' ';
        }
    }
}

void inline static setAddrWindow(unsigned short x0, unsigned short y0, unsigned short x1, unsigned short y1)
{
    writeCommand(HX8357_SET_COLUMN_ADDRESS); // Column addr set
    writeData((x0) >> 8);
    writeData(x0);     // XSTART
    writeData((x1) >> 8);
    writeData(x1);     // XEND

    writeCommand(HX8357_SET_PAGE_ADDRESS); // Row addr set
    writeData((y0) >> 8);
    writeData(y0);     // YSTART
    writeData((y1) >> 8);
    writeData(y1);     // YEND

    writeCommand(HX8357_WRITE_MEMORY_START); //Write SRAM Data
}

void inline static setPixel(int x, int y, unsigned short color)
{
    if((x < 0) ||(x >= _width) || (y < 0) || (y >= _height))
        return;
    setAddrWindow(x,y,x+1,y+1);
    while (PMMODE & PIC32_PMMODE_BUSY);
    PMADDR = 0x0001;
    PMDIN = color;
}


void inline static fillRectangle(int x, int y, int w, int h, unsigned short color)
{
    setAddrWindow(x, y, x+w-1, y+h-1);

    while (PMMODE & PIC32_PMMODE_BUSY);
    PMADDR = 0x0001;
    for(y=h; y>0; y--) {
        for(x=w; x>0; x--) {
            while (PMMODE & PIC32_PMMODE_BUSY);
            PMDIN = color;
        }
    }
}

void inline static setRotation(unsigned char m)
{
    writeCommand(HX8357_SET_ADDRESS_MODE);
    rotation = m % 4; // can't be higher than 3
    switch (rotation) {
    case 0:
        //PORTRAIT
        writeData(0x0000);
        _width  = 320;
        _height = 480;
        break;
    case 1:
        //LANDSCAPE
        writeData(0x0060);
        _width  = 480;
        _height = 320;
        break;
    case 2:
        //UPSIDE DOWN PORTRAIT
        writeData(0x00C0);
        _width  = 320;
        _height = 480;
        break;
    case 3:
        //UPSIDE DOWN LANDSCAPE
        writeData(0x00A0);
        _width  = 480;
        _height = 320;
        break;
    }
}

unsigned short inline static mix(unsigned short a, unsigned short b, unsigned char pct)
{
    Color565 col_a;
    Color565 col_b;
    Color565 col_out;
    col_a.value = a;
    col_b.value = b;
    unsigned int temp;
    temp = (((int)col_a.r * (255-pct)) / 255) + (((unsigned int)col_b.r * pct) / 255);
    col_out.r = temp;
    temp = (((int)col_a.g * (255-pct)) / 255) + (((unsigned int)col_b.g * pct) / 255);
    col_out.g = temp;
    temp = (((int)col_a.b * (255-pct)) / 255) + (((unsigned int)col_b.b * pct) / 255);
    col_out.b = temp;
    return col_out.value;
}


unsigned char drawChar(int x, int y, unsigned char c, unsigned short color, unsigned short bg)
{
    if (font == NULL) {
        return 0;
    }

    FontHeader *header = (FontHeader *)font;

    if (c < header->startGlyph || c > header->endGlyph) {
        return 0;
    }

    c = c - header->startGlyph;

    // Start of this character's data is the character number multiplied by the
    // number of lines in a character (plus one for the character width) multiplied
    // by the number of bytes in a line, and then offset by 4 for the header.
    unsigned int charstart = (c * ((header->linesPerCharacter * header->bytesPerLine) + 1)) + sizeof(FontHeader); // Start of character data
    unsigned char charwidth = font[charstart++]; // The first byte of a block is the width of the character

    unsigned int bitmask = (1 << header->bitsPerPixel) - 1;

    setAddrWindow(x, y, x + charwidth - 1, y + header->linesPerCharacter - 1);

    char lineNumber = 0;
    for (lineNumber = 0; lineNumber < header->linesPerCharacter; lineNumber++ ) {
        unsigned char lineData = 0;

        char bitsLeft = -1;
        unsigned char byteNumber = 0;

        char pixelNumber = 0;
        for (pixelNumber = 0; pixelNumber < charwidth; pixelNumber++) {
            if (bitsLeft <= 0) {
                bitsLeft = 8;
                lineData = font[charstart + (lineNumber * header->bytesPerLine) + (header->bytesPerLine - byteNumber - 1)];
                byteNumber++;
            }
            unsigned int pixelValue = lineData & bitmask;
            if (pixelValue > 0) {
                writeData(color);
            } else {
                writeData(bg);
            }
            lineData >>= header->bitsPerPixel;
            bitsLeft -= header->bitsPerPixel;
        }
    }
    return charwidth;
}

void updateLine(int l)
{
    if (l < 0 || l > 39)
        return;

    int c;
    for (c = 0; c < 80; c++) {
        drawChar(c * _font_width, l * _font_height, frame[l][c] & 0x7F,
            frame[l][c] & 0x80 ? texthicolor : textcolor, textbgcolor);
    }
}

void scrollUp()
{
    int l, c;
    for (l = 0; l < 39; l++) {
        for (c = 0; c < 80; c++) {
            frame[l][c] = frame[l+1][c];
        }
        updateLine(l);
    }
    for (c = 0; c < 80; c++) {
        frame[39][c] = ' ';
    }
    updateLine(39);
}

int inEscape = 0;

int edValPos = 0;
int edVal[4];

int bold = 0;

void printChar(unsigned char c)
{
    switch (c) {
    case '\n':
        cursor_y ++;
        if (cursor_y > 39) {
            scrollUp();
            cursor_y = 39;
        }
        break;
    case '\r':
        cursor_x = 0;
        break;
    case 8:
        cursor_x--;
        if (cursor_x < 0) {
            cursor_x = 79;
            cursor_y--;
            if (cursor_y < 0) {
                cursor_y = 0;
            }
        }
        break;
    default:
        frame[cursor_y][cursor_x] = c | (bold ? 0x80 : 0x00);
        cursor_x++;
        if (cursor_x == 80) {
            cursor_x = 0;
            cursor_y++;
            if (cursor_y == 40) {
                cursor_y = 39;
                scrollUp();
            }
        }
        updateLine(cursor_y);
    }
}

int extended = 0;

void writeChar(unsigned char c)
{
    int i, j;
    updateLine(cursor_y);

    if (inEscape == 1) {
        switch (c) {
        case 27:
            inEscape = 0;
            printChar('^');
            printChar('[');
            break;
        case '[':
            extended = 1;
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            edVal[edValPos%4] *= 10;
            edVal[edValPos%4] += c - '0';
            break;
        case ';':
            edValPos++;
            edVal[edValPos%4] = 0;
            break;
        case 'J':
            if (extended == 1) {
                if (edVal[0] == 0) {
                    for (i = cursor_y; i < 40; i++) {
                        for (j = 0; j < 80; j++) {
                            frame[i][j] = ' ';
                        }
                        updateLine(i);
                    }
                } else if (edVal[0] == 1) {
                    for (i = 0; i < cursor_y; i++) {
                        for (j = 0; j < 80; j++) {
                            frame[i][j] = ' ';
                        }
                        updateLine(i);
                    }
                } else if (edVal[0] == 2) {
                    for (i = 0; i < 40; i++) {
                        for (j = 0; j < 80; j++) {
                            frame[i][j] = ' ';
                        }
                        updateLine(i);
                    }
                }
            }
            inEscape = 0;
            break;
        case 'H':
            if (extended == 1) {
                if (edVal[0] == 0) {
                    edVal[0] = 1;
                }
                if (edVal[1] == 0) {
                    edVal[1] = 1;
                }
                cursor_x = (edVal[1] - 1) % 80;
                cursor_y = (edVal[0] - 1) % 40;
            }
            inEscape = 0;
            break;
        case 'm':
            if (extended == 1) {
                if (edVal[0] == 0) {
                    bold = 0;
                } else if (edVal[0] == 1) {
                    bold = 1;
                }
            }
            inEscape = 0;
            break;

        case 'A':
            if (extended == 0) {
                cursor_y--;
                if (cursor_y > 0) {
                    cursor_y = 39;
                }
            }
            inEscape = 0;
            break;

        case 'B':
            if (extended == 0) {
                cursor_y++;
                if (cursor_y == 40) {
                    cursor_y = 0;
                }
            }
            inEscape = 0;
            break;

        case 'C':
            if (extended == 0) {
                cursor_x++;
                if (cursor_x == 80) {
                    cursor_x = 0;
                    cursor_y++;
                    if (cursor_y == 40) {
                        cursor_y = 0;
                    }
                }
            }
            inEscape = 0;
            break;

        case 'D':
            if (extended == 0) {
                cursor_x--;
                if (cursor_x < 0) {
                    cursor_x = 79;
                    cursor_y--;
                    if (cursor_y < 0) {
                        cursor_y = 39;
                    }
                }
            }
            inEscape = 0;
            break;

        case 'K':
            if (extended == 1) {
                if (edVal[0] == 0) {
                    for (i = cursor_x; i < 80; i++) {
                        frame[cursor_y][i] = ' ';
                    }
                } else if (edVal[1] == 1) {
                    for (i = 0; i < cursor_x; i++) {
                        frame[cursor_y][i] = ' ';
                    }
                } else {
                    for (i = 0; i < 80; i++) {
                        frame[cursor_y][i] = ' ';
                    }
                }
                updateLine(cursor_y);
            }
            inEscape = 0;
            break;

        default:
            if (extended) {
                printf("Unhandled extended escape code %c\n", c);
            } else {
                printf("Unhandled escape code %c\n", c);
            }
            inEscape = 0;
            break;
       }
    } else {
        if (c == 27) {
            inEscape = 1;
            extended = 0;
            edVal[0] = 0;
            edValPos = 0;
        } else {
            printChar(c);
        }
    }
    drawChar(cursor_x * _font_width, cursor_y * _font_height,
        frame[cursor_y][cursor_x] & 0x7F, 0x0000,
        bold ? texthicolor : textcolor);
#if 0
    for (i = 0; i < _font_width; i++) {
        setPixel(cursor_x * _font_width + i,
            (cursor_y + 1) * _font_height - 1, 0xFFFF);
    }
#endif
}

void hx8357_start(struct tty *tp)
{
    register int s;

    s = spltty();
    ttyowake(tp);
//    tp->t_state &= TS_BUSY;
    if (tp->t_outq.c_cc > 0) {
        led_control (LED_TTY, 1);
        while (tp->t_outq.c_cc != 0) {
            int c = getc (&tp->t_outq);
            writeChar(c);
        }
        led_control (LED_TTY, 0);
    }
//    tp->t_state |= TS_BUSY;
    splx (s);
}

void hx8357_init()
{
    initDisplay();
    setRotation(1);
    int i,j;
    for (i = 0; i < 40; i++) {
        for (j = 0; j < 80; j++) {
            frame[i][j] = ' ';
        }
        updateLine(i);
    }
}

int ttyIsOpen = 0;

int hx8357_open(dev_t dev, int flag, int mode)
{
    int unit = minor(dev);

    if (unit == 0) {
        ttyIsOpen = 1;
        struct tty *tp = &hx8357_ttys[0];
        tp->t_oproc = hx8357_start;
        tp->t_state = TS_ISOPEN | TS_CARR_ON;
        tp->t_flags = ECHO | XTABS | CRMOD | CRTBS | CRTERA | CTLECH | CRTKIL;
        return ttyopen(dev, tp);
    }
    if (unit == 1) {
        if (!ttyIsOpen) {
            return EIO;
        }
        return 0;
    }
    return ENODEV;
}

int hx8357_close(dev_t dev, int flag, int mode)
{
    int unit = minor(dev);

    if (unit == 0) {
        ttyIsOpen = 0;
        struct tty *tp = &hx8357_ttys[0];
        ttyclose(tp);
        return 0;
    }

    if (unit == 1) {
        return 0;
    }
    return ENODEV;
}

int hx8357_read(dev_t dev, struct uio *uio, int flag)
{
    int unit = minor(dev);

    if (unit == 0) {
        struct tty *tp = &hx8357_ttys[unit];
        return ttread(tp, uio, flag);
    }
    if (unit == 1) {
        return EIO;
    }

    return ENODEV;
}

int hx8357_write(dev_t dev, struct uio *uio, int flag)
{
    int unit = minor(dev);

    if (unit == 0) {
        struct tty *tp = &hx8357_ttys[0];
        return ttwrite(tp, uio, flag);
    }

    if (unit == 1) {
        if (!ttyIsOpen) {
            return EIO;
        }
        struct tty *tp = &hx8357_ttys[0];
        struct iovec *iov = uio->uio_iov;

        while (iov->iov_len  > 0) {
            ttyinput(*(iov->iov_base), tp);
            iov->iov_base ++;
            iov->iov_len --;
            uio->uio_resid --;
            uio->uio_offset ++;
        }
        return 0;
    }
    return ENODEV;
}

int hx8357_ioctl(dev_t dev, register u_int cmd, caddr_t addr, int flag)
{
    int unit = minor(dev);

    if (unit == 0) {
        struct tty *tp = &hx8357_ttys[unit];
        int error;

        error = ttioctl(tp, cmd, addr, flag);
        if (error < 0)
            error = ENOTTY;
        return (error);
    }
    if (unit == 1) {
        return EIO;
    }

    return ENODEV;

}

void hx8357_putc(dev_t dev, char c)
{
    writeChar(c);
}

char hx8357_getc(dev_t dev)
{
    return 0;
}

int hx8357_select (dev_t dev, int rw)
{
    int unit = minor(dev);
    if (unit == 0) {
        struct tty *tp = &hx8357_ttys[unit];
        return (ttyselect (tp, rw));
    }
    if (unit == 1) {
        return EIO;
    }
    return ENODEV;
}
