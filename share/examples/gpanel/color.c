/*
 * Display a color palette.
 * Assume RGB 5-6-5 color format.
 * Color is a 16-bit value: rrrrrggggggbbbbb, where rrrrr,
 * gggggg and bbbbb are red, green and blue components.
 */
#include <stdio.h>
#include <sys/gpanel.h>

int xsize, ysize;

int scale_red(int x, int y)
{
    int r;

    r = 32 * y * (xsize-x-1) / xsize / ysize;
    if (r > 31)
        r = 31;
    return r;
}

int scale_green(int x, int y)
{
    int g;

    g = 64 * x / xsize;
    if (g > 63)
        g = 63;
    return g;
}

int scale_blue(int x, int y)
{
    int b;

    b = 32 * (ysize-y-1) * (xsize-x-1) / xsize / ysize;
    if (b > 31)
        b = 31;
    return b;
}

int main()
{
    char *devname = "/dev/tft0";
    int x, y, r, g, b, color;

    if (gpanel_open(devname) < 0) {
        printf("Cannot open %s\n", devname);
        exit(-1);
    }
    gpanel_clear(0, &xsize, &ysize);
    printf("Screen size %u x %u.\n", xsize, ysize);

    printf("Display color palette.\n");
    for (y=0; y<ysize; y++) {
        for (x=0; x<xsize; x++) {
            r = scale_red(x, y);
            g = scale_green(x, y);
            b = scale_blue(x, y);
            color = r << 11 | g << 5 | b;
            gpanel_pixel(color, x, y);
        }
    }
    return 0;
}
