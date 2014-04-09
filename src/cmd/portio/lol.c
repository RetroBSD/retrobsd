/*
 * Basic LoL Shield Test for Duinomite running RetroBSD.
 *
 * Some LoL signals are used for SD card on Duinomite.
 * So I had to modify Duinomite-shield, by cutting signals
 * D9-D12 and connecting them to A1-A5.
 *
 * Writen for the LoL Shield, designed by Jimmie Rodgers:
 * http://jimmieprodgers.com/kits/lolshield/
 *
 * Copyright (C) 2012 Serge Vakulenko, <serge@vak.ru>
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/gpio.h>

#include "font6x10.c"

/*
 * Display a line of text.
 */
void display (fd, text)
    int fd;
    const char *text;
{
    static short picture [9];
    const unsigned char *glyph;
    int c, x, y;

    /* Clear image. */
    memset (picture, 0, sizeof(picture));

    /* Build a running text. */
    while (*text != 0) {
        /* Get next symbol. */
        c = *text++;
        if (c < ' ' || c > '~')
            continue;
        glyph = font6x10_bits + font6x10_offset [c];

        for (x=0; x<FONT_WIDTH; x++) {
            /* Shift image. */
            for (y=0; y<9; y++)
                picture[y] >>= 1;

            /* Add a column of pixels */
            for (y=0; y<FONT_HEIGHT; y++) {
                if ((glyph[y] << x) & 0x80)
                    picture[y] |= 1 << 13;
            }
            /* Display a frame. */
            ioctl (fd, GPIO_LOL | 120, picture);
        }
    }

    /* Shift out the rest of picture. */
    while (picture[0] | picture[1] | picture[2] | picture[3] |
        picture[4] | picture[5] | picture[6] | picture[7] | picture[8])
    {
        /* Shift image. */
        for (y=0; y<9; y++)
            picture[y] >>= 1;

        /* Display a frame. */
        ioctl (fd, GPIO_LOL | 120, picture);
    }
}

/*
 * Demo 1:
 * 1) vertical lines;
 * 2) horizontal lines;
 * 3) all LEDs on.
 */
void demo1 (fd)
{
    static unsigned short picture[9];
    int y, frame;

    printf ("LoL Demo 1 ");
    fflush (stdout);

    for (frame = 0; frame<14; frame++) {
        printf (".");
        fflush (stdout);
        memset (picture, 0, sizeof(picture));

        for (y=0; y<9; y++)
            picture[y] |= 1 << frame;

        /* Display a frame. */
        ioctl (fd, GPIO_LOL | 100, picture);
    }
    for (frame = 0; frame<9; frame++) {
        printf (":");
        fflush (stdout);
        memset (picture, 0, sizeof(picture));

        picture[frame] = (1 << 14) - 1;

        /* Display a frame. */
        ioctl (fd, GPIO_LOL | 100, picture);
    }
    printf (",");
    fflush (stdout);
    memset (picture, 0xFF, sizeof(picture));

    /* Display a frame. */
    ioctl (fd, GPIO_LOL | 250, picture);
    ioctl (fd, GPIO_LOL | 250, picture);

    printf (" Done\n");
}

void demo2 (fd)
{
    static unsigned short picture[9];
    int x, y, dx, dy;

    printf ("LoL Demo 2: press ^C to stop\n");
    memset (picture, 0, sizeof(picture));
    x = 0;
    y = 0;
    dx = 1;
    dy = 1;
    for (;;) {
        /* Draw ball. */
        picture[y] |= 1 << x;
        ioctl (fd, GPIO_LOL | 120, picture);
        picture[y] &= ~(1 << x);

        /* Move the ball. */
        x += dx;
        if (x < 0 || x >= 14) {
            dx = -dx;
            x += dx + dx;
        }
        y += dy;
        if (y < 0 || y >= 9) {
            dy = -dy;
            y += dy + dy;
        }
    }
}

void usage ()
{
    fprintf (stderr, "LoL shield utility.\n");
    fprintf (stderr, "Usage:\n");
    fprintf (stderr, "    lol [options] text\n");
    fprintf (stderr, "Options:\n");
    fprintf (stderr, "    -1        -- demo of vertical and horizontal lines\n");
    fprintf (stderr, "    -2        -- demo of ball in a box\n");
    fprintf (stderr, "    text      -- display a running text\n");
    exit (-1);
}

int main (argc, argv)
    char **argv;
{
    const char *devname = "/dev/porta";
    int fd, idle = 1;

    /* Open gpio device. */
    fd = open (devname, 1);
    if (fd < 0) {
        perror (devname);
        exit (-1);
    }

    for (;;) {
        switch (getopt (argc, argv, "12")) {
        case EOF:
            break;
        case '1':
            demo1 (fd);
            idle = 0;
            continue;
        case '2':
            demo2 (fd);
            idle = 0;
            continue;
        default:
            usage ();
        }
        break;
    }
    argc -= optind;
    argv += optind;
    if (argc < 1 && idle)
        usage ();

    while (argc-- > 0) {
        display (fd, *argv++);
    }
    return 0;
}
