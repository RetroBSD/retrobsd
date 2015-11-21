/*
 * Display driver for ST7781 LCD controller.
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
#include <sys/gpanel.h>

/*
 * Display orientation.
 */
typedef enum {
    PORTRAIT,
    LANDSCAPE,
    PORTRAIT_UPSIDE_DOWN,
    LANDSCAPE_UPSIDE_DOWN,
} orient_t;

static orient_t orientation;

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

/* Swap values of two integer variables. */
#define swapi(x,y) { int _t = x; x = y; y = _t; }

/*
 * Write a 16-bit value to the ST7781 register.
 */
static void write_reg(unsigned reg, unsigned value)
{
    gpanel_rs_command();
    gpanel_write_byte(reg >> 8);
    gpanel_write_byte(reg);
    gpanel_rs_data();
    gpanel_write_byte(value >> 8);
    gpanel_write_byte(value);
}

static void set_window(int x0, int y0, int x1, int y1)
{
    gpanel_cs_active();

    /* Check rotation, move pixel around if necessary. */
    switch (orientation) {
    case PORTRAIT:
        break;
    case LANDSCAPE:
        write_reg(ST7781_Entry_Mode, 0x1028);
        swapi(x0, y0);
        swapi(x1, y1);
        x0 = 240 - x0 - 1;
        x1 = 240 - x1 - 1;
        swapi(x0, x1);
        break;
    case PORTRAIT_UPSIDE_DOWN:
        write_reg(ST7781_Entry_Mode, 0x1000);
        x0 = 240 - x0 - 1;
        x1 = 240 - x1 - 1;
        swapi(x0, x1);
        y0 = 320 - y0 - 1;
        y1 = 320 - y1 - 1;
        swapi(y0, y1);
        break;
    case LANDSCAPE_UPSIDE_DOWN:
        write_reg(ST7781_Entry_Mode, 0x1018);
        swapi(x0, y0);
        swapi(x1, y1);
        y0 = 320 - y0 - 1;
        y1 = 320 - y1 - 1;
        swapi(y0, y1);
        break;
    }
    /* Set address window. */
    write_reg(ST7781_Horizontal_Address_Start_Position, x0);
    write_reg(ST7781_Horizontal_Address_End_Position,   x1);
    write_reg(ST7781_Vertical_Address_Start_Position,   y0);
    write_reg(ST7781_Vertical_Address_End_Position,     y1);

    /* Set address counter to top left. */
    write_reg(ST7781_DRAM_Horizontal_Address_Set, x0);
    write_reg(ST7781_DRAM_Vertical_Address_Set,   y0);
    gpanel_cs_idle();
}

static void clear_window()
{
    gpanel_cs_active();
    write_reg(ST7781_Entry_Mode, 0x1030);
    write_reg(ST7781_Horizontal_Address_Start_Position, 0);
    write_reg(ST7781_Horizontal_Address_End_Position,   240-1);
    write_reg(ST7781_Vertical_Address_Start_Position,   0);
    write_reg(ST7781_Vertical_Address_End_Position,     320-1);
    gpanel_cs_idle();
}

/*
 * Draw a pixel.
 */
static void st7781_set_pixel(int x, int y, int color)
{
    if (x < 0 || x >= gpanel_width || y < 0 || y >= gpanel_height)
        return;

    /* Check rotation, move pixel around if necessary. */
    switch (orientation) {
    case PORTRAIT:
        break;
    case LANDSCAPE:
        swapi(x, y);
        x = 240 - x - 1;
        break;
    case PORTRAIT_UPSIDE_DOWN:
        x = 240 - x - 1;
        y = 320 - y - 1;
        break;
    case LANDSCAPE_UPSIDE_DOWN:
        swapi(x, y);
        y = 320 - y - 1;
        break;
    }
    gpanel_cs_active();
    write_reg(ST7781_DRAM_Horizontal_Address_Set, x);
    write_reg(ST7781_DRAM_Vertical_Address_Set,   y);
    write_reg(ST7781_Write_Data_to_DRAM, color);
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

    gpanel_cs_active();
    gpanel_rs_command();
    gpanel_write_byte(0x00); /* High address byte */
    gpanel_write_byte(ST7781_Write_Data_to_DRAM);

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
    gpanel_cs_idle();
}

/*
 * Fill a rectangle with specified color.
 */
static void st7781_fill_rectangle(int x0, int y0, int x1, int y1, int color)
{
    if (x0 < 0) x0 = 0;
    if (y0 < 0) x0 = 0;
    if (x1 < 0) x1 = 0;
    if (y1 < 0) x1 = 0;
    if (x0 >= gpanel_width) x0 = gpanel_width-1;
    if (x1 >= gpanel_width) x1 = gpanel_width-1;
    if (y0 >= gpanel_height) y0 = gpanel_height-1;
    if (y1 >= gpanel_height) y1 = gpanel_height-1;

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
    clear_window();
}

/*
 * Fill a rectangle with user data.
 */
static void st7781_draw_image(int x, int y, int width, int height,
    const unsigned short *data)
{
    unsigned cnt = width * height;
    int color;

    set_window(x, y, x + width - 1, y + height - 1);
    gpanel_cs_active();
    gpanel_rs_command();
    gpanel_write_byte(0x00); /* High address byte */
    gpanel_write_byte(ST7781_Write_Data_to_DRAM);
    gpanel_rs_data();
    while (cnt--) {
        color = *data++;
        gpanel_write_byte(color >> 8);
        gpanel_write_byte(color);
    }
    gpanel_cs_idle();
    clear_window();
}

/*
 * Draw a glyph of one symbol.
 */
static void st7781_draw_glyph(const struct gpanel_font_t *font,
    int color, int background, int x, int y, int width,
    const unsigned short *bits)
{
    int h, w, c;
    unsigned bitmask = 0;

    if (x + width > gpanel_width ||  y + font->height > gpanel_height)
        return;

    if (background >= 0) {
        /*
         * Clear background.
         */
        set_window(x, y, x + width - 1, y + font->height - 1);
        gpanel_cs_active();
        gpanel_rs_command();
        gpanel_write_byte(0x00); /* High address byte */
        gpanel_write_byte(ST7781_Write_Data_to_DRAM);
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
        clear_window();
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
                    st7781_set_pixel(x + w, y + h, color);
            }
        }
    }
}

/*
 * Switch the screen orientation.
 */
static void set_rotation(orient_t rotation)
{
    orientation = rotation;
    switch (orientation) {
    case PORTRAIT:
        gpanel_width  = 240;
        gpanel_height = 320;
        break;
    case LANDSCAPE:
        gpanel_width  = 320;
        gpanel_height = 240;
        break;
    case PORTRAIT_UPSIDE_DOWN:
        gpanel_width  = 240;
        gpanel_height = 320;
        break;
    case LANDSCAPE_UPSIDE_DOWN:
        gpanel_width  = 320;
        gpanel_height = 240;
        break;
    }
}

static void st7781_resize(struct gpanel_hw *h, int width, int height)
{
    gpanel_cs_active();

    /* Switch screen orientaation. */
    if (width > height)
        set_rotation(LANDSCAPE);
    else if (width < height)
        set_rotation(PORTRAIT);

    gpanel_cs_idle();
}

/*
 * Initialize the LCD controller.
 * Fill the gpanel_hw descriptor.
 */
void st7781_init_display(struct gpanel_hw *h)
{
    /* Initialization of LCD controller. */
    gpanel_cs_active();
    write_reg(ST7781_Driver_Output_Control,    0x0100);
    write_reg(ST7781_LCD_Driving_Wave_Control, 0x0700);
    write_reg(ST7781_Display_control_2,        0x0302);
    write_reg(ST7781_Display_Control_3,        0x0000);
    write_reg(ST7781_Display_Control_4,        0x0008);

    /* Power control registers. */
    write_reg(ST7781_Power_Control_1, 0x0790);
    write_reg(ST7781_Power_Control_2, 0x0005);
    write_reg(ST7781_Power_Control_3, 0x0000);
    write_reg(ST7781_Power_Control_4, 0x0000);

    /* Power supply startup 1 settings. */
    write_reg(ST7781_Power_Control_1, 0x12B0);
    write_reg(ST7781_Power_Control_2, 0x0007);

    /* Power supply startup 2 settings. */
    write_reg(ST7781_Power_Control_3, 0x008C);
    write_reg(ST7781_Power_Control_4, 0x1700);
    write_reg(ST7781_VCOMH_Control,   0x0022);

    /* Gamma cluster settings. */
    write_reg(ST7781_Gamma_Control_1,  0x0000);
    write_reg(ST7781_Gamma_Control_2,  0x0505);
    write_reg(ST7781_Gamma_Control_3,  0x0205);
    write_reg(ST7781_Gamma_Control_4,  0x0206);
    write_reg(ST7781_Gamma_Control_5,  0x0408);
    write_reg(ST7781_Gamma_Control_6,  0x0000);
    write_reg(ST7781_Gamma_Control_7,  0x0504);
    write_reg(ST7781_Gamma_Control_8,  0x0206);
    write_reg(ST7781_Gamma_Control_9,  0x0206);
    write_reg(ST7781_Gamma_Control_10, 0x0408);

    /* Frame rate settings. */
    write_reg(ST7781_Gate_Scan_Control_1,       0xA700);
    write_reg(ST7781_Gate_Scan_Control_2,       0x0001);
    write_reg(ST7781_Panel_Interface_Control_1, 0x0033); // RTNI setting

    /* Display on. */
    write_reg(ST7781_Display_Control_1, 0x0133);
    clear_window();

    /* Screen orientation. */
    set_rotation(LANDSCAPE);

    /* Fill the gpanel_hw descriptor. */
    h->name           = "Sitronix ST7781";
    h->resize         = st7781_resize;
    h->set_pixel      = st7781_set_pixel;
    h->fill_rectangle = st7781_fill_rectangle;
    h->draw_image     = st7781_draw_image;
    h->draw_glyph     = st7781_draw_glyph;
}
