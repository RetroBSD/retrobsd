/*
 * Draw random circles.
 */
#include <stdio.h>
#include <sys/gpanel.h>

int xsize, ysize;

int main()
{
    char *devname = "/dev/tft0";
    int x, y, r, color;

    if (gpanel_open(devname) < 0) {
        printf("Cannot open %s\n", devname);
        exit(-1);
    }
    gpanel_clear(0, &xsize, &ysize);
    printf("Screen size %u x %u.\n", xsize, ysize);

    srandom(time(0));
    printf("Draw random circles.\n");
    printf("Press ^C to stop.\n");

    for (;;) {
        x = random() % xsize;
        y = random() % ysize;
        r = random() % ysize;
        color = random();
        gpanel_circle(color, x, y, r);
    }
    return 0;
}
