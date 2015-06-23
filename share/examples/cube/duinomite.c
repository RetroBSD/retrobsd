/*
 * Interface to LED cube 8x8x8.
 * The cube is connected to pins D0-D9 of Duinomite board.
 *
 * Pin  PIC32  Function
 * ---------------
 *  D0  RE0    Y0  \
 *  D1  RE1    Y1  | Layer select
 *  D2  RE2    Y2  /
 *  D3  RE3    Y3  - Upper backlignt
 *  D4  RE4    Y4  - Lower backlight
 *  D5  RE5    SDI - Serial data   \
 *  D6  RE6    CLK - Clock         | to shift registers
 *  D7  RE7    /LE - Latch enable  |
 *  D8  RB11   /OE - Output enable /
 *  D10 RD11   EXT - Unknown
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

    /* Configure pins RE0-RE7, RB11 and RD11 as output. */
    ioctl(gpio, GPIO_PORTE | GPIO_CONFOUT, 0xff);
    ioctl(gpio, GPIO_PORTE | GPIO_CLEAR, 0xff);
    ioctl(gpio, GPIO_PORTB | GPIO_CONFOUT, 1 << 11);
    ioctl(gpio, GPIO_PORTB | GPIO_CLEAR, 1 << 11);
    ioctl(gpio, GPIO_PORTD | GPIO_CONFOUT, 1 << 11);
    ioctl(gpio, GPIO_PORTD | GPIO_CLEAR, 1 << 11);
}

void gpio_ext(int on)
{
    /* EXT signal at RD11. */
    if (on)
        ioctl(gpio, GPIO_PORTD | GPIO_SET, 1 << 11);
    else
        ioctl(gpio, GPIO_PORTD | GPIO_CLEAR, 1 << 11);
}

void gpio_oe(int active)
{
    /* /OE signal at RB11, active low. */
    if (active)
        ioctl(gpio, GPIO_PORTB | GPIO_CLEAR, 1 << 11);
    else
        ioctl(gpio, GPIO_PORTB | GPIO_SET, 1 << 11);
}

void gpio_le(int active)
{
    /* /LE signal at RE7, active low. */
    if (active)
        ioctl(gpio, GPIO_PORTE | GPIO_CLEAR, 1 << 7);
    else
        ioctl(gpio, GPIO_PORTE | GPIO_SET, 1 << 7);
}

void gpio_backlight_upper(int on)
{
    /* Y4 signal at RE4. */
    if (on)
        ioctl(gpio, GPIO_PORTE | GPIO_SET, 1 << 4);
    else
        ioctl(gpio, GPIO_PORTE | GPIO_CLEAR, 1 << 4);
}

void gpio_backlight_lower(int on)
{
    /* Y3 signal at RE3. */
    if (on)
        ioctl(gpio, GPIO_PORTE | GPIO_SET, 1 << 3);
    else
        ioctl(gpio, GPIO_PORTE | GPIO_CLEAR, 1 << 3);
}

void gpio_layer(int z)
{
    /* Y0-Y2 signals at RE0-RE23. */
    if (z & 1)
        ioctl(gpio, GPIO_PORTE | GPIO_CLEAR, 1 << 0);
    else
        ioctl(gpio, GPIO_PORTE | GPIO_SET, 1 << 0);

    if (z & 2)
        ioctl(gpio, GPIO_PORTE | GPIO_CLEAR, 1 << 1);
    else
        ioctl(gpio, GPIO_PORTE | GPIO_SET, 1 << 1);

    if (z & 4)
        ioctl(gpio, GPIO_PORTE | GPIO_CLEAR, 1 << 2);
    else
        ioctl(gpio, GPIO_PORTE | GPIO_SET, 1 << 2);
}

void gpio_plane(unsigned char *data)
{
    int i, n, val;

    /* Send 8 bytes of tada to shift registers. */
    for (i=0; i<8; i+=2) {
        val = *data++;
        for (n=0; n<8; n++) {
            /* SDI signal at RE5. */
            if (val & 0x80)
                ioctl(gpio, GPIO_PORTE | GPIO_SET, 1 << 5);
            else
                ioctl(gpio, GPIO_PORTE | GPIO_CLEAR, 1 << 5);

            /* CLK signal at RE6. */
            ioctl(gpio, GPIO_PORTE | GPIO_SET, 1 << 6);
            ioctl(gpio, GPIO_PORTE | GPIO_CLEAR, 1 << 6);

            val <<= 1;
        }
        val = *data++;
        for (n=0; n<8; n++) {
            /* SDI signal at RE5. */
            if (val & 1)
                ioctl(gpio, GPIO_PORTE | GPIO_SET, 1 << 5);
            else
                ioctl(gpio, GPIO_PORTE | GPIO_CLEAR, 1 << 5);

            /* CLK signal at RE6. */
            ioctl(gpio, GPIO_PORTE | GPIO_SET, 1 << 6);
            ioctl(gpio, GPIO_PORTE | GPIO_CLEAR, 1 << 6);

            val >>= 1;
        }
    }
}
