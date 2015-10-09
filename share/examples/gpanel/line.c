/*
 * Draw random lines.
 */
#include <stdio.h>
#include <sys/gpanel.h>

int xsize, ysize;

int main()
{
    char *devname = "/dev/tft0";
    int x0, y0, x1, y1, color;

    if (gpanel_open(devname) < 0) {
        printf("Cannot open %s\n", devname);
        exit(-1);
    }
    gpanel_clear(0, &xsize, &ysize);
    printf("Screen size %u x %u.\n", xsize, ysize);

    srandom(time(0));
    printf("Draw random lines.\n");
    printf("Press ^C to stop.\n");

    for (;;) {
        x0 = random() % xsize;
        y0 = random() % ysize;
        x1 = random() % xsize;
        y1 = random() % ysize;
        color = random();
        gpanel_line(color, x0, y0, x1, y1);
    }
    return 0;
}
