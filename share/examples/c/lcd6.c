/*
 * Demo for 6 digit LCD module based on HT1261 controller.
 * Based on example sources from http://www.canton-electronics.com
 *
 * Copyright (C) 2015 Serge Vakulenko
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
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/gpio.h>

/*
 * Digits, decimal dots and battery levels:
 *   A     B     C     D     E     F
 *   --    --    --    --    --    --  z
 *  |  |  |  |  |  |  |  |  |  |  |  | y
 *   --    --    --    --    --    --  x
 *  |  |  |  |  |  |  |  |  |  |  |  |
 *   --    --    -- c  -- d  -- e  --
 *
 * Memory map:
 *  Byte 0, bits 0-6    - segments of digit 'F'
 *  Byte 0, bit 7       - decimal dot 'e'
 *  Byte 1, bits 0-6    - segments of digit 'E'
 *  Byte 1, bit 7       - decimal dot 'd'
 *  Byte 2, bits 0-6    - segments of digit 'D'
 *  Byte 2, bit 7       - decimal dot 'c'
 *  Byte 3, bits 0-6    - segments of digit 'C'
 *  Byte 3, bit 7       - battery level 'x'
 *  Byte 4, bits 0-6    - segments of digit 'B'
 *  Byte 4, bit 7       - battery level 'y'
 *  Byte 5, bits 0-6    - segments of digit 'A'
 *  Byte 5, bit 7       - battery level 'z'
 *
 * Segments are mapped to bits 0-6:
 *  --4--
 *  0---5
 *  --1--
 *  2---6
 *  --3--
 */

/*
 * Signal assignment.
 * LED modulee is connected to pins 16,17,18 of Fubarino SD board.
 *
 * Fubarino PIC32  LED module
 * --------------------------
 *  16      RE0    CS
 *  17      RE1    WR
 *  18      RE2    DATA
 */
#define gpio_cs_clear()     ioctl(gpio, GPIO_PORTE | GPIO_CLEAR, 1 << 0)
#define gpio_cs_set()       ioctl(gpio, GPIO_PORTE | GPIO_SET,   1 << 0)
#define gpio_wr_clear()     ioctl(gpio, GPIO_PORTE | GPIO_CLEAR, 1 << 1)
#define gpio_wr_set()       ioctl(gpio, GPIO_PORTE | GPIO_SET,   1 << 1)
#define gpio_data_clear()   ioctl(gpio, GPIO_PORTE | GPIO_CLEAR, 1 << 2)
#define gpio_data_set()     ioctl(gpio, GPIO_PORTE | GPIO_SET,   1 << 2)

/*
 * HT1621 commands
 */
#define HT_SYS_DIS      0x00    /* Turn off system osc and bias generator */
#define HT_SYS_EN       0x01    /* Turn on system oscillator */
#define HT_LCD_OFF      0x02    /* Turn off LCD bias generator */
#define HT_LCD_ON       0x03    /* Turn on LCD bias generator */
#define HT_CLOCK_XTAL   0x14    /* Crystal 32kHz */
#define HT_CLOCK_RC     0x18    /* On-chip RC oscillator 256kHz */
#define HT_CLOCK_EXT    0x1c    /* External clock */
#define HT_BIAS_1_3     0x21    /* LCD 1/3 bias option, 2 commons default */
#define HT_COMMONS_3    0x04    /* 3 commons option */
#define HT_COMMONS_4    0x08    /* 4 commons option */

/*
 * Mapping of symbols to segments.
 *  0, 1, 2, 3, 4, 5, 6, 7,
 *  8, 9, A, b, C, c, d, E,
 *  F, H, h, L, n, N, o, P,
 *  r, t, U, -,  ,
 */
const char char_to_segm[] = {
    0x7D,   0x60,   0x3E,   0x7A,   0x63,   0x5B,   0x5F,   0x70,
    0x7F,   0x7B,   0x77,   0x4F,   0x1D,   0x0E,   0x6E,   0x1F,
    0x17,   0x67,   0x47,   0x0D,   0x46,   0x75,   0x37,   0x06,
    0x0F,   0x6D,   0x02,   0x00,
};

/*
 * File descriptor for GPIO driver.
 */
int gpio;

/*
 * Suspend the process for some amount of milliseconds.
 */
void mdelay(unsigned msec)
{
    usleep(msec * 1000);
}

/*
 * Send a series of bits to HT1621.
 * Up to 8 bits, high bit first.
 * Max clock rate is 150kHz.
 */
void ht_send(int nbits, int data)
{
    data <<= (8 - nbits);
    gpio_wr_clear();
    while (nbits-- > 0) {
        if (data & 0x80)
            gpio_data_set();
        else
            gpio_data_clear();
        gpio_wr_set();
        gpio_wr_clear();
        data <<= 1;
    }
}

/*
 * Send command to HT1621.
 */
void ht_cmd(int command)
{
    gpio_cs_clear();
    ht_send(3, 0x4);        /* Mode "100" */
    ht_send(8, command);
    gpio_cs_set();
}

/*
 * Send data and command.
 */
void ht_write(int addr, int data)
{
    gpio_cs_clear();
    ht_send(3, 0x5);        /* Mode "101" */
    ht_send(6, addr << 1);  /* Half-byte address 6 bits */
    ht_send(8, data);       /* Data 8 bits */
    gpio_cs_set();
}

/*
 * Initialize LCD controller.
 */
void lcd_init()
{
    /* Open GPIO driver. */
    gpio = open("/dev/porta", 1);
    if (gpio < 0) {
        perror("/dev/porta");
        exit(-1);
    }

    /* Configure pins RE0-RE2 as outputs. */
    ioctl(gpio, GPIO_PORTE | GPIO_CONFOUT, 0x07);
    gpio_cs_set();
    gpio_wr_clear();

    /* Setup appropriate HT1621 mode. */
    ht_cmd(HT_SYS_EN);
    ht_cmd(HT_CLOCK_RC);
    ht_cmd(HT_BIAS_1_3 | HT_COMMONS_4);
    ht_cmd(HT_LCD_ON);
}

/*
 * Set display memory to given value.
 */
void lcd_clear(int value)
{
    int i;

    for (i=0; i<6; i++) {
        ht_write(i, value);
    }
}

/*
 * LCD on/off.
 */
void lcd_enable(int on)
{
    if (on)
        ht_cmd(HT_LCD_ON);
    else
        ht_cmd(HT_LCD_OFF);
}

/*
 * Display data.
 *  val - Data to be displayed, 0-999999
 *  dot - Display decimal dot, 0-3
 *  bat - Battery level, 0-3
 */
void lcd_display(unsigned val, int dot, int bat)
{
    int i, byte[6];

    /* Set numeric value. */
    byte[5] = char_to_segm[val / 100000];
    byte[4] = char_to_segm[(val / 10000) % 10];
    byte[3] = char_to_segm[(val / 1000) % 10];
    byte[2] = char_to_segm[(val / 100) % 10];
    byte[1] = char_to_segm[(val / 10) % 10];
    byte[0] = char_to_segm[val % 10];

    /* Enable decimal dot/ */
    switch (dot) {
    case 1:
        byte[0] |= 1 << 7;
        break;
    case 2:
        byte[1] |= 1 << 7;
        break;
    case 3:
        byte[2] |= 1 << 7;
        break;
    default:
        break;
    }
    if (bat > 0)
        byte[3] |= 1 << 7;
    if (bat > 1)
        byte[4] |= 1 << 7;
    if (bat > 2)
        byte[5] |= 1 << 7;

    for (i=0; i<6; i++) {
        ht_write(i, byte[i]);
    }
}

int main()
{
    int i;

    /* Initialize hardware. */
    lcd_init();

    /* Blink all segments twice. */
    lcd_clear(0xff);
    mdelay(1000);
    lcd_clear(0);
    mdelay(1000);
    lcd_clear(0xff);
    mdelay(1000);
    lcd_clear(0);
    mdelay(1000);

    /* Show all characters on all segments. */
    for (i=0; i<sizeof(char_to_segm); i++) {
        lcd_clear(char_to_segm[i]);
        mdelay(200);
    }
    lcd_clear(0);

    /* Display counter 0 to 999999. */
    for (;;) {
       for (i=0; i<999999; i++) {
            lcd_display(i, i/10%4, i/10%4);
            mdelay(100);
       }
    }
    return 0;
}
