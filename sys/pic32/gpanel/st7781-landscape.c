/*
 * ST7781 TFT LCD driver for PIC32.
 *
 * Based on code provided by Smoke And Wires
 * https://github.com/Smoke-And-Wires/TFT-Shield-Example-Code
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
#include <sys/gpanel.h>

/*
 * Display size.
 */
//#define WIDTH   240
//#define HEIGHT  320
#define WIDTH   320
#define HEIGHT  240

/*
 * Cursor position for text output.
 */
static int _col, _row;

/*
 * ID of the LCD controller chip.
 */
static int _chip_id;

/*
 * Delay for 100 nanoseconds.
 * Needed to match the /WR and /RD timing requirements.
 */
#if CPU_KHZ <= 10000
#   define delay100ns()     /* empty */
#elif CPU_KHZ <= 20000
#   define delay100ns()     asm volatile("nop")
#elif CPU_KHZ <= 30000
#   define delay100ns()     asm volatile("nop; nop")
#elif CPU_KHZ <= 40000
#   define delay100ns()     asm volatile("nop; nop; nop")
#elif CPU_KHZ <= 50000
#   define delay100ns()     asm volatile("nop; nop; nop; nop")
#elif CPU_KHZ <= 60000
#   define delay100ns()     asm volatile("nop; nop; nop; nop; nop")
#elif CPU_KHZ <= 70000
#   define delay100ns()     asm volatile("nop; nop; nop; nop; nop; nop")
#else
#   define delay100ns()     asm volatile("nop; nop; nop; nop; nop; nop; nop; nop")
#endif

/*
 * Signal mappings:
 *  /RESET  - reset and initialize the chip.
 *  /CS     - chip select when low.
 *  /RD     - read operation enable.
 *  /WR     - write operation enable.
 *  RS      - command or data mode selection.
 *  D0-D7   - data bus, bidirectional.
 */
#define RST_IDLE()      LAT_SET(LCD_RST_PORT) = 1<<LCD_RST_PIN
#define RST_ACTIVE()    LAT_CLR(LCD_RST_PORT) = 1<<LCD_RST_PIN

#define CS_IDLE()       LAT_SET(LCD_CS_PORT) = 1<<LCD_CS_PIN
#define CS_ACTIVE()     LAT_CLR(LCD_CS_PORT) = 1<<LCD_CS_PIN

#define RD_IDLE()       LAT_SET(LCD_RD_PORT) = 1<<LCD_RD_PIN
#define RD_ACTIVE()     LAT_CLR(LCD_RD_PORT) = 1<<LCD_RD_PIN

#define WR_IDLE()       LAT_SET(LCD_WR_PORT) = 1<<LCD_WR_PIN
#define WR_ACTIVE()     LAT_CLR(LCD_WR_PORT) = 1<<LCD_WR_PIN
#define WR_STROBE()     { WR_ACTIVE(); delay100ns(); WR_IDLE(); }

#define RS_DATA()       LAT_SET(LCD_RS_PORT) = 1<<LCD_RS_PIN
#define RS_COMMAND()    LAT_CLR(LCD_RS_PORT) = 1<<LCD_RS_PIN

/*
 * ST7781 registers.
 */
#define ST7781_Driver_ID_Code_Read                  0x00
#define ST7781_Driver_Output_Control                0x01
#define ST7781_LCD_Driving_Wave_Control             0x02
#define ST7781_Entry_Mode                           0x03
#define ST7781_Resize_Control                       0x04
#define ST7781_Display_Control_1                    0x07
#define ST7781_Display_control_2                    0x08
#define ST7781_Display_Control_3                    0x09
#define ST7781_Display_Control_4                    0x0A
#define ST7781_Frame_Marker_Position                0x0D
#define ST7781_Power_Control_1                      0x10
#define ST7781_Power_Control_2                      0x11
#define ST7781_Power_Control_3                      0x12
#define ST7781_Power_Control_4                      0x13
#define ST7781_DRAM_Horizontal_Address_Set          0x20
#define ST7781_DRAM_Vertical_Address_Set            0x21
#define ST7781_Write_Data_to_DRAM                   0x22
#define ST7781_Read_Data_from_DRAM                  0x22
#define ST7781_VCOMH_Control                        0x29
#define ST7781_Frame_Rate_and_Color_Control         0x2B
#define ST7781_Gamma_Control_1                      0x30
#define ST7781_Gamma_Control_2                      0x31
#define ST7781_Gamma_Control_3                      0x32
#define ST7781_Gamma_Control_4                      0x35
#define ST7781_Gamma_Control_5                      0x36
#define ST7781_Gamma_Control_6                      0x37
#define ST7781_Gamma_Control_7                      0x38
#define ST7781_Gamma_Control_8                      0x39
#define ST7781_Gamma_Control_9                      0x3C
#define ST7781_Gamma_Control_10                     0x3D
#define ST7781_Horizontal_Address_Start_Position    0x50
#define ST7781_Horizontal_Address_End_Position      0x51
#define ST7781_Vertical_Address_Start_Position      0x52
#define ST7781_Vertical_Address_End_Position        0x53
#define ST7781_Gate_Scan_Control_1                  0x60
#define ST7781_Gate_Scan_Control_2                  0x61
#define ST7781_Partial_Image_1_Display_Position     0x80
#define ST7781_Partial_Image_1_Start_Address        0x81
#define ST7781_Partial_Image_1_End_Address          0x82
#define ST7781_Partial_Image_2_Display_Position     0x83
#define ST7781_Partial_Image_2_Start_Address        0x84
#define ST7781_Partial_Image_2_End_Address          0x85
#define ST7781_Panel_Interface_Control_1            0x90
#define ST7781_Panel_Interface_Control_2            0x92
#define ST7781_EEPROM_ID_Code                       0xD2
#define ST7781_EEPROM_Control_Status                0xD9
#define ST7781_EEPROM_Wite_Command                  0xDF
#define ST7781_EEPROM_Enable                        0xFA
#define ST7781_EEPROM_VCOM_Offset                   0xFE
#define ST7781_FAh_FEh_Enable                       0xFF

/*
 * Set direction of data bus as output.
 */
static void setWriteDir()
{
    TRIS_CLR(LCD_D0_PORT) = 1 << LCD_D0_PIN;
    TRIS_CLR(LCD_D1_PORT) = 1 << LCD_D1_PIN;
    TRIS_CLR(LCD_D2_PORT) = 1 << LCD_D2_PIN;
    TRIS_CLR(LCD_D3_PORT) = 1 << LCD_D3_PIN;
    TRIS_CLR(LCD_D4_PORT) = 1 << LCD_D4_PIN;
    TRIS_CLR(LCD_D5_PORT) = 1 << LCD_D5_PIN;
    TRIS_CLR(LCD_D6_PORT) = 1 << LCD_D6_PIN;
    TRIS_CLR(LCD_D7_PORT) = 1 << LCD_D7_PIN;
}

/*
 * Set direction of data bus as input.
 */
static void setReadDir()
{
    TRIS_SET(LCD_D0_PORT) = 1 << LCD_D0_PIN;
    TRIS_SET(LCD_D1_PORT) = 1 << LCD_D1_PIN;
    TRIS_SET(LCD_D2_PORT) = 1 << LCD_D2_PIN;
    TRIS_SET(LCD_D3_PORT) = 1 << LCD_D3_PIN;
    TRIS_SET(LCD_D4_PORT) = 1 << LCD_D4_PIN;
    TRIS_SET(LCD_D5_PORT) = 1 << LCD_D5_PIN;
    TRIS_SET(LCD_D6_PORT) = 1 << LCD_D6_PIN;
    TRIS_SET(LCD_D7_PORT) = 1 << LCD_D7_PIN;
}

/*
 * Send a byte to the data bus.
 */
static void writeByte(unsigned value)
{
    if (value & 1) {
        LAT_SET(LCD_D0_PORT) = 1 << LCD_D0_PIN;
    } else {
        LAT_CLR(LCD_D0_PORT) = 1 << LCD_D0_PIN;
    }
    if (value & 2) {
        LAT_SET(LCD_D1_PORT) = 1 << LCD_D1_PIN;
    } else {
        LAT_CLR(LCD_D1_PORT) = 1 << LCD_D1_PIN;
    }
    if (value & 4) {
        LAT_SET(LCD_D2_PORT) = 1 << LCD_D2_PIN;
    } else {
        LAT_CLR(LCD_D2_PORT) = 1 << LCD_D2_PIN;
    }
    if (value & 8) {
        LAT_SET(LCD_D3_PORT) = 1 << LCD_D3_PIN;
    } else {
        LAT_CLR(LCD_D3_PORT) = 1 << LCD_D3_PIN;
    }
    if (value & 0x10) {
        LAT_SET(LCD_D4_PORT) = 1 << LCD_D4_PIN;
    } else {
        LAT_CLR(LCD_D4_PORT) = 1 << LCD_D4_PIN;
    }
    if (value & 0x20) {
        LAT_SET(LCD_D5_PORT) = 1 << LCD_D5_PIN;
    } else {
        LAT_CLR(LCD_D5_PORT) = 1 << LCD_D5_PIN;
    }
    if (value & 0x40) {
        LAT_SET(LCD_D6_PORT) = 1 << LCD_D6_PIN;
    } else {
        LAT_CLR(LCD_D6_PORT) = 1 << LCD_D6_PIN;
    }
    if (value & 0x80) {
        LAT_SET(LCD_D7_PORT) = 1 << LCD_D7_PIN;
    } else {
        LAT_CLR(LCD_D7_PORT) = 1 << LCD_D7_PIN;
    }
    WR_STROBE();
}

static unsigned readByte()
{
    unsigned value = 0;

    RD_ACTIVE();
    delay100ns();
    if (PORT_VAL(LCD_D0_PORT) & (1 << LCD_D0_PIN)) value |= 1;
    if (PORT_VAL(LCD_D1_PORT) & (1 << LCD_D1_PIN)) value |= 2;
    if (PORT_VAL(LCD_D2_PORT) & (1 << LCD_D2_PIN)) value |= 4;
    if (PORT_VAL(LCD_D3_PORT) & (1 << LCD_D3_PIN)) value |= 8;
    if (PORT_VAL(LCD_D4_PORT) & (1 << LCD_D4_PIN)) value |= 0x10;
    if (PORT_VAL(LCD_D5_PORT) & (1 << LCD_D5_PIN)) value |= 0x20;
    if (PORT_VAL(LCD_D6_PORT) & (1 << LCD_D6_PIN)) value |= 0x40;
    if (PORT_VAL(LCD_D7_PORT) & (1 << LCD_D7_PIN)) value |= 0x80;
    RD_IDLE();
    return value;
}

/*
 * Write a 16-bit value to the ST7781 register.
 */
static void writeReg(unsigned reg, unsigned value)
{
    RS_COMMAND();
    writeByte(reg >> 8);
    writeByte(reg);
    RS_DATA();
    writeByte(value >> 8);
    writeByte(value);
}

/*
 * Read device ID code.
 */
static unsigned readDeviceId()
{
    unsigned value;

    CS_ACTIVE();
    RS_COMMAND();
    writeByte(ST7781_Driver_ID_Code_Read);
    delay100ns();
    WR_STROBE();        // Repeat prior byte
    setReadDir();       // Switch data bus as input
    RS_DATA();
    value = readByte() << 8;
    value |= readByte();
    setWriteDir();      // Restore data bus as output
    CS_IDLE();
    return value;
}

/*
 * Detect the type of the LCD controller, and initialize it.
 * Return -1 in case of unknown chip.
 */
static int initDisplay()
{
    /*
     * Set all control bits to high (idle).
     * Signals are active low.
     */
    CS_IDLE();
    WR_IDLE();
    RD_IDLE();
    RST_IDLE();

    /* Enable outputs. */
    TRIS_CLR(LCD_CS_PORT) = 1 << LCD_CS_PIN;
    TRIS_CLR(LCD_RS_PORT) = 1 << LCD_RS_PIN;
    TRIS_CLR(LCD_WR_PORT) = 1 << LCD_WR_PIN;
    TRIS_CLR(LCD_RD_PORT) = 1 << LCD_RD_PIN;
    TRIS_CLR(LCD_RST_PORT) = 1 << LCD_RST_PIN;
    setWriteDir();

    /* Reset the chip. */
    RST_ACTIVE();
    udelay(1000);
    RST_IDLE();
    udelay(1000);

    /* Read the the chip ID register. */
    _chip_id = readDeviceId();
    switch (_chip_id) {
    case 0x7783:
        printf("swtft0: <Sitronix ST7781>\n");
        break;

    default:
        /* Disable outputs. */
        setReadDir();
        TRIS_SET(LCD_CS_PORT) = 1 << LCD_CS_PIN;
        TRIS_SET(LCD_RS_PORT) = 1 << LCD_RS_PIN;
        TRIS_SET(LCD_WR_PORT) = 1 << LCD_WR_PIN;
        TRIS_SET(LCD_RD_PORT) = 1 << LCD_RD_PIN;
        TRIS_SET(LCD_RST_PORT) = 1 << LCD_RST_PIN;
        printf("swtft0: Unknown chip ID = 0x%x\n", _chip_id);
        return -1;
    }

    /* Initialization of LCD controller. */
    CS_ACTIVE();
    writeReg(ST7781_Driver_Output_Control,    0x0100);
    writeReg(ST7781_LCD_Driving_Wave_Control, 0x0700);
    //writeReg(ST7781_Entry_Mode,               0x1030);
    writeReg(ST7781_Entry_Mode,               0x1028);
    writeReg(ST7781_Display_control_2,        0x0302);
    writeReg(ST7781_Display_Control_3,        0x0000);
    writeReg(ST7781_Display_Control_4,        0x0008);

    /* Power control registers. */
    writeReg(ST7781_Power_Control_1, 0x0790);
    writeReg(ST7781_Power_Control_2, 0x0005);
    writeReg(ST7781_Power_Control_3, 0x0000);
    writeReg(ST7781_Power_Control_4, 0x0000);

    /* Power supply startup 1 settings. */
    writeReg(ST7781_Power_Control_1, 0x12B0);
    writeReg(ST7781_Power_Control_2, 0x0007);

    /* Power supply startup 2 settings. */
    writeReg(ST7781_Power_Control_3, 0x008C);
    writeReg(ST7781_Power_Control_4, 0x1700);
    writeReg(ST7781_VCOMH_Control,   0x0022);

    /* Gamma cluster settings. */
    writeReg(ST7781_Gamma_Control_1,  0x0000);
    writeReg(ST7781_Gamma_Control_2,  0x0505);
    writeReg(ST7781_Gamma_Control_3,  0x0205);
    writeReg(ST7781_Gamma_Control_4,  0x0206);
    writeReg(ST7781_Gamma_Control_5,  0x0408);
    writeReg(ST7781_Gamma_Control_6,  0x0000);
    writeReg(ST7781_Gamma_Control_7,  0x0504);
    writeReg(ST7781_Gamma_Control_8,  0x0206);
    writeReg(ST7781_Gamma_Control_9,  0x0206);
    writeReg(ST7781_Gamma_Control_10, 0x0408);

    /* Frame rate settings. */
    writeReg(ST7781_Gate_Scan_Control_1,       0xA700);
    writeReg(ST7781_Gate_Scan_Control_2,       0x0001);
    writeReg(ST7781_Panel_Interface_Control_1, 0x0033); // RTNI setting

    /* Display on. */
    writeReg(ST7781_Display_Control_1, 0x0133);
    return 0;
}

static void setAddrWindow(int x0, int y0, int x1, int y1)
{
    if (WIDTH > HEIGHT) {
        /* Landscape mode. */
        int t;
        t = x0;
        x0 = HEIGHT-1 - y1;
        y1 = x1;
        x1 = HEIGHT-1 - y0;
        y0 = t;
    }
    /* Set address window. */
    CS_ACTIVE();
    writeReg(ST7781_Horizontal_Address_Start_Position, x0);
    writeReg(ST7781_Horizontal_Address_End_Position,   x1);
    writeReg(ST7781_Vertical_Address_Start_Position,   y0);
    writeReg(ST7781_Vertical_Address_End_Position,     y1);

    /* Set address counter to top left. */
    //writeReg(ST7781_DRAM_Horizontal_Address_Set, (WIDTH > HEIGHT) ? x1 : x0);
    writeReg(ST7781_DRAM_Horizontal_Address_Set, x0);
    writeReg(ST7781_DRAM_Vertical_Address_Set,   y0);
    CS_IDLE();
}

/*
 * Draw a pixel.
 */
static void setPixel(int x, int y, int color)
{
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT)
        return;
    CS_ACTIVE();
    if (WIDTH > HEIGHT) {
        /* Landscape mode. */
        writeReg(ST7781_DRAM_Horizontal_Address_Set, HEIGHT-1 - y);
        writeReg(ST7781_DRAM_Vertical_Address_Set,   x);
    } else {
        /* Portrait mode. */
        writeReg(ST7781_DRAM_Horizontal_Address_Set, x);
        writeReg(ST7781_DRAM_Vertical_Address_Set,   y);
    }
    writeReg(ST7781_Write_Data_to_DRAM, color);
    CS_IDLE();
}

/*
 * Fast block fill operation.
 * Requires setAddrWindow() has previously been called to set
 * the fill bounds.
 * 'npixels' is inclusive, MUST be >= 1.
 */
static void flood(int color, int npixels)
{
    unsigned blocks, i;
    unsigned hi = color >> 8,
             lo = color;

    CS_ACTIVE();
    RS_COMMAND();
    writeByte(0x00); /* High address byte */
    writeByte(ST7781_Write_Data_to_DRAM);

    /* Write first pixel normally, decrement counter by 1. */
    RS_DATA();
    writeByte(hi);
    writeByte(lo);
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
                delay100ns(); WR_STROBE();
                delay100ns(); WR_STROBE();
                delay100ns(); WR_STROBE();
                delay100ns(); WR_STROBE();
                delay100ns(); WR_STROBE();
                delay100ns(); WR_STROBE();
                delay100ns(); WR_STROBE();
                delay100ns(); WR_STROBE();
            }
        }
        /* Fill any remaining pixels (1 to 64). */
        for (i = npixels & 63; i > 0; i--) {
            delay100ns(); WR_STROBE();
            delay100ns(); WR_STROBE();
        }
    } else {
        while (blocks--) {
            /* 64 pixels/block / 4 pixels/pass. */
            for (i = 16; i > 0; i--) {
                writeByte(hi); writeByte(lo); writeByte(hi); writeByte(lo);
                writeByte(hi); writeByte(lo); writeByte(hi); writeByte(lo);
            }
        }
        for (i = npixels & 63; i > 0; i--) {
            writeByte(hi);
            writeByte(lo);
        }
    }
    CS_IDLE();
}

/*
 * Fill a rectangle with specified color.
 */
static void fillRectangle(int x0, int y0, int x1, int y1, int color)
{
    if (x0 < 0) x0 = 0;
    if (y0 < 0) x0 = 0;
    if (x1 < 0) x1 = 0;
    if (y1 < 0) x1 = 0;
    if (x0 >= WIDTH) x0 = WIDTH-1;
    if (x1 >= WIDTH) x1 = WIDTH-1;
    if (y0 >= HEIGHT) y0 = HEIGHT-1;
    if (y1 >= HEIGHT) y1 = HEIGHT-1;

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
    flood(color, (x1 - x0 + 1) * (y1 - y0 + 1));
    setAddrWindow(0, 0, WIDTH-1, HEIGHT-1);
}

/*
 * Fill a rectangle with user data.
 */
static void drawImage(int x, int y, int width, int height,
    const unsigned short *data)
{
    unsigned cnt = width * height;
    int color;

    setAddrWindow(x, y, x + width - 1, y + height - 1);
    CS_ACTIVE();
    RS_COMMAND();
    writeByte(0x00); /* High address byte */
    writeByte(ST7781_Write_Data_to_DRAM);
    RS_DATA();
    while (cnt--) {
        color = *data++;
        writeByte(color >> 8);
        writeByte(color);
    }
    CS_IDLE();
    if (WIDTH > HEIGHT)
        setAddrWindow(0, 0, WIDTH-1, HEIGHT-1);
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
    if (_row > HEIGHT - font->height)
        _row = 0;
}

/*
 * Draw a glyph of one symbol.
 */
static void drawGlyph(const struct gpanel_font_t *font,
    int color, int background, int width, const unsigned short *bits)
{
    int h, w, c;
    unsigned bitmask = 0;

    if (background >= 0) {
        /*
         * Clear background.
         */
        setAddrWindow(_col, _row, _col + width - 1, _row + font->height - 1);
        CS_ACTIVE();
        RS_COMMAND();
        writeByte(0x00); /* High address byte */
        writeByte(ST7781_Write_Data_to_DRAM);
        RS_DATA();

        /* Loop on each glyph row. */
        for (h=0; h<font->height; h++) {
            /* Loop on every pixel in the row (left to right). */
            for (w=0; w<width; w++) {
                if ((w & 15) == 0)
                    bitmask = *bits++;
                else
                    bitmask <<= 1;

                c = (bitmask & 0x8000) ? color : background;
                writeByte(c >> 8);
                writeByte(c);
            }
        }
        CS_IDLE();
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

    /* Draw a character. */
    if (_col < WIDTH)
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
    if (WIDTH > HEIGHT)
        setAddrWindow(0, 0, WIDTH-1, HEIGHT-1);
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

            setAddrWindow(0, 0, WIDTH-1, HEIGHT-1);
            flood(param->color, WIDTH * HEIGHT);
            if (WIDTH > HEIGHT)
                setAddrWindow(0, 0, WIDTH-1, HEIGHT-1);
            param->xsize = WIDTH;
            param->ysize = HEIGHT;
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
            if (WIDTH > HEIGHT)
                setAddrWindow(0, 0, WIDTH-1, HEIGHT-1);
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
swtftprobe(config)
    struct conf_device *config;
{
    if (initDisplay() < 0)
        return 0;

    printf("swtft0: display %ux%u\n", WIDTH, HEIGHT);
    setAddrWindow(0, 0, WIDTH-1, HEIGHT-1);
    return 1;
}

struct driver swtftdriver = {
    "swtft", swtftprobe,
};
