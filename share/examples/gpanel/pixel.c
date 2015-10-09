/*
 * Draw random pixels.
 */
#include <stdio.h>
#include <sys/gpanel.h>

int xsize, ysize;

int main()
{
    char *devname = "/dev/tft0";
    int x, y, color;

    if (gpanel_open(devname) < 0) {
        printf("Cannot open %s\n", devname);
        exit(-1);
    }
    gpanel_clear(0, &xsize, &ysize);
    printf("Screen size %u x %u.\n", xsize, ysize);

    srand(time(0));
    printf("Draw random pixels.\n");
    printf("Press ^C to stop.\n");

    for (;;) {
        x = rand() % xsize;
        y = rand() % ysize;
        color = rand() << 1;
        gpanel_pixel(color, x, y);
    }
    return 0;
}
