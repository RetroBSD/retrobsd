/*
 * Test the gpanel functions for speed.
 */
#include <stdio.h>
#include <sys/gpanel.h>
#include <sys/time.h>

#define NPIXELS     100000
#define NLINES      3000
#define NRECT       1000
#define NCIRCLES    1000
#define NCHARS      10000

extern const struct gpanel_font_t font_lucidasans15;

int xsize, ysize;

/*
 * Get current time in milliseconds.
 */
unsigned current_msec()
{
    struct timeval t;

    gettimeofday(&t, 0);
    return t.tv_sec * 1000 + t.tv_usec / 1000;
}

/*
 * Compute elapsed time in milliseconds.
 */
unsigned elapsed_msec(unsigned t0)
{
    struct timeval t1;
    unsigned msec;

    gettimeofday (&t1, 0);
    msec = t1.tv_sec * 1000 + t1.tv_usec / 1000;
    msec -= t0;
    if (msec < 1)
        msec = 1;
    return msec;
}

int main()
{
    char *devname = "/dev/tft0";
    int x, y, x1, y1, color, i, r, sym;
    unsigned t0, msec;

    if (gpanel_open(devname) < 0) {
        printf("Cannot open %s\n", devname);
        exit(-1);
    }
    gpanel_clear(0, &xsize, &ysize);
    printf("Screen size %u x %u.\n", xsize, ysize);

    /* Use fixed seed, for repeatability. */
    srand(1);
    printf("Graphics speed:\n");
    sync();
    usleep(500000);

    /*
     * Pixels.
     */
    t0 = current_msec();
    for (i=0; i<NPIXELS; i++) {
        x = rand() % xsize;
        y = rand() % ysize;
        color = rand() << 1;
        gpanel_pixel(color, x, y);
    }
    msec = elapsed_msec(t0);
    printf (" %u pixels/second\n", NPIXELS*1000U / msec);

    /*
     * Lines.
     */
    t0 = current_msec();
    for (i=0; i<NLINES; i++) {
        x = rand() % xsize;
        y = rand() % ysize;
        x1 = rand() % xsize;
        y1 = rand() % ysize;
        color = rand() << 1;
        gpanel_line(color, x, y, x1, y1);
    }
    msec = elapsed_msec(t0);
    printf (" %u lines/second\n", NLINES*1000U / msec);

    /*
     * Filled rectangles.
     */
    t0 = current_msec();
    for (i=0; i<NRECT; i++) {
        x = rand() % xsize;
        y = rand() % ysize;
        x1 = rand() % xsize;
        y1 = rand() % ysize;
        color = rand() << 1;
        gpanel_fill(color, x, y, x1, y1);
    }
    msec = elapsed_msec(t0);
    printf (" %u rectangles/second\n", NRECT*1000U / msec);

    /*
     * Circles.
     */
    t0 = current_msec();
    for (i=0; i<NCIRCLES; i++) {
        x = rand() % xsize;
        y = rand() % ysize;
        r = rand() % ysize;
        color = rand() << 1;
        gpanel_circle(color, x, y, r);
    }
    msec = elapsed_msec(t0);
    printf (" %u circles/second\n", NCIRCLES*1000U / msec);

    /*
     * Characters.
     */
    t0 = current_msec();
    for (i=0; i<NCHARS; i++) {
        x = rand() % xsize;
        y = rand() % ysize;
        sym = '!' + rand() % ('~' - ' ');
        color = rand() << 1;
        gpanel_char(&font_lucidasans15, color, -1, x, y, sym);
    }
    msec = elapsed_msec(t0);
    printf (" %u characters/second\n", NCHARS*1000U / msec);

    return 0;
}
