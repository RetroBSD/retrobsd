/*
 * Interface to LED cube 8x8x8.
 * The cube is connected to pins 4-13 of Fubarino board.
 *
 * Pin  PIC32  Function
 * ---------------
 *  4   RD0    Y0  \
 *  5   RC13   Y1  | Layer select
 *  6   RC14   Y2  /
 *  7   RD1    Y3  - Upper backlignt
 *  8   RD2    Y4  - Lower backlight
 *  9   RD3    SDI - Serial data   \
 *  10  RD4    CLK - Clock         | to shift registers
 *  11  RD5    /LE - Latch enable  |
 *  12  RD6    /OE - Output enable /
 *  13  RD7    EXT - Unknown
 */
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/gpio.h>
#include "cube.h"

static int gpio;

void gpio_init()
{
    char *devname = "/dev/porta";

    /* Open GPIO driver. */
    gpio = open(devname, 1);
    if (gpio < 0) {
        perror(devname);
        exit(-1);
    }

    /* Configure pins RD0-RD7, RC13 and RC14 as output. */
    ioctl(gpio, GPIO_PORTD | GPIO_CONFOUT, 0xff);
    ioctl(gpio, GPIO_PORTD | GPIO_CLEAR, 0xff);
    ioctl(gpio, GPIO_PORTC | GPIO_CONFOUT, 3 << 13);
    ioctl(gpio, GPIO_PORTC | GPIO_CLEAR, 3 << 13);
}

void gpio_ext(int on)
{
    /* EXT signal at RD7. */
    if (on)
        ioctl(gpio, GPIO_PORTD | GPIO_SET, 1 << 7);
    else
        ioctl(gpio, GPIO_PORTD | GPIO_CLEAR, 1 << 7);
}

void gpio_oe(int active)
{
    /* /OE signal at RD6, active low. */
    if (active)
        ioctl(gpio, GPIO_PORTD | GPIO_CLEAR, 1 << 6);
    else
        ioctl(gpio, GPIO_PORTD | GPIO_SET, 1 << 6);
}

void gpio_le(int active)
{
    /* /LE signal at RD5, active low. */
    if (active)
        ioctl(gpio, GPIO_PORTD | GPIO_CLEAR, 1 << 5);
    else
        ioctl(gpio, GPIO_PORTD | GPIO_SET, 1 << 5);
}

void gpio_backlight_upper(int on)
{
    /* Y4 signal at RD2. */
    if (on)
        ioctl(gpio, GPIO_PORTD | GPIO_SET, 1 << 2);
    else
        ioctl(gpio, GPIO_PORTD | GPIO_CLEAR, 1 << 2);
}

void gpio_backlight_lower(int on)
{
    /* Y3 signal at RD1. */
    if (on)
        ioctl(gpio, GPIO_PORTD | GPIO_SET, 1 << 1);
    else
        ioctl(gpio, GPIO_PORTD | GPIO_CLEAR, 1 << 1);
}

void gpio_layer(int z)
{
    /* Y0-Y2 signals at RD0, RC13, RC14. */
    if (z & 1)
        ioctl(gpio, GPIO_PORTD | GPIO_CLEAR, 1 << 0);
    else
        ioctl(gpio, GPIO_PORTD | GPIO_SET, 1 << 0);

    if (z & 2)
        ioctl(gpio, GPIO_PORTC | GPIO_CLEAR, 1 << 13);
    else
        ioctl(gpio, GPIO_PORTC | GPIO_SET, 1 << 13);

    if (z & 4)
        ioctl(gpio, GPIO_PORTC | GPIO_CLEAR, 1 << 14);
    else
        ioctl(gpio, GPIO_PORTC | GPIO_SET, 1 << 14);
}

void gpio_plane(unsigned char *data)
{
    int i, n, val;

    /* Send 8 bytes of tada to shift registers. */
    for (i=0; i<8; i+=2) {
        val = *data++;
        for (n=0; n<8; n++) {
            /* SDI signal at RD3. */
            if (val & 0x80)
                ioctl(gpio, GPIO_PORTD | GPIO_SET, 1 << 3);
            else
                ioctl(gpio, GPIO_PORTD | GPIO_CLEAR, 1 << 3);

            /* CLK signal at RD4. */
            ioctl(gpio, GPIO_PORTD | GPIO_SET, 1 << 4);
            ioctl(gpio, GPIO_PORTD | GPIO_CLEAR, 1 << 4);

            val <<= 1;
        }
        val = *data++;
        for (n=0; n<8; n++) {
            /* SDI signal at RD3. */
            if (val & 1)
                ioctl(gpio, GPIO_PORTD | GPIO_SET, 1 << 3);
            else
                ioctl(gpio, GPIO_PORTD | GPIO_CLEAR, 1 << 3);

            /* CLK signal at RD4. */
            ioctl(gpio, GPIO_PORTD | GPIO_SET, 1 << 4);
            ioctl(gpio, GPIO_PORTD | GPIO_CLEAR, 1 << 4);

            val >>= 1;
        }
    }
}
