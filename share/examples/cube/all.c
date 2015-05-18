/*
 * Demo for LED cube 8x8x8.
 * Turn on all LEDs.
 */
#include <stdio.h>
#include "cube.h"

int main()
{
    static unsigned char data[8] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    };
    int i;

    gpio_init();
    gpio_le(0);
    gpio_plane(data);
    gpio_ext(1);
    for (;;) {
        for (i=0; i<8; i++) {
            gpio_layer(i);
        }
    }
    return 0;
}
