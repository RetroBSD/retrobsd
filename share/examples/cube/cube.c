/*
 * Demo for LED cube 8x8x8.
 * Switch between two static images.
 */
#include <stdio.h>
#include <sys/time.h>
#include "cube.h"

void display(int duration, unsigned char *data)
{
    struct timeval t0, now;
    int z, msec;

    gettimeofday(&t0, 0);
    z = 0;
    for (;;) {
        /* Send layer data. Latch is active,
         * so previous layer is still displayed. */
        gpio_plane(data + z*CUBE_SIZE);

        /* Disable output, activate latch,
         * switch to next layer. */
        gpio_oe(0);
        gpio_le(0);
        gpio_le(1);
        gpio_layer(z);
        gpio_oe(1);

        /* Next layer. */
        z++;
        if (z >= CUBE_SIZE) {
            z = 0;

            /* Check time. */
            gettimeofday(&now, 0);
            msec = (now.tv_sec - t0.tv_sec) * 1000;
            msec += (now.tv_usec - t0.tv_usec) / 1000;
            if (msec >= duration)
                break;
        }
    }
}

int main()
{
    static unsigned char foo[64] = {
        0xff, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xff,
        0x81, 0,    0,    0,    0,    0,    0,    0x81,
        0x81, 0,    0,    0,    0,    0,    0,    0x81,
        0x81, 0,    0,    0,    0,    0,    0,    0x81,
        0x81, 0,    0,    0,    0,    0,    0,    0x81,
        0x81, 0,    0,    0,    0,    0,    0,    0x81,
        0x81, 0,    0,    0,    0,    0,    0,    0x81,
        0xff, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xff,
    };
    static unsigned char bar[64] = {
        0,    0,    0,    0,    0,    0,    0,    0,
        0,    0x7e, 0x42, 0x42, 0x42, 0x42, 0x7e, 0,
        0,    0x42, 0,    0,    0,    0,    0x42, 0,
        0,    0x42, 0,    0,    0,    0,    0x42, 0,
        0,    0x42, 0,    0,    0,    0,    0x42, 0,
        0,    0x42, 0,    0,    0,    0,    0x42, 0,
        0,    0x7e, 0x42, 0x42, 0x42, 0x42, 0x7e, 0,
        0,    0,    0,    0,    0,    0,    0,    0,
    };
    gpio_init();
    gpio_ext(1);
    for (;;) {
        display(500, foo);
        display(500, bar);
    }
    return 0;
}
