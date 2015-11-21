/*
 * aclock - ascii clock for UNIX Console
 *
 * Copyright (c) 1994-2013 Antoni Sawicki <as@tenoware.com>
 * Version 2.3 (unix-termcap); Mountain View, July 2013
 *
 * Ported to RetroBSD by Serge Vakulenko
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define FontWH_Ratio 2

void cls(void)
{
    printf("\33[2J");
}

void draw_point(int x, int y, char c)
{
    printf("\33[%u;%uH%c", y+1, x+1, c);
}

void draw_text(int x, int y, char *string)
{
    printf("\33[%u;%uH%s", y+1, x+1, string);
}

int icos(int n, int maxval)
{
    static const int tab[] = {
        1000000000, 994521895,  978147600,  951056516,  913545457,
        866025403,  809016994,  743144825,  669130606,  587785252,
        500000000,  406736643,  309016994,  207911690,  104528463,
        0,          -104528463, -207911690, -309016994, -406736643,
        -499999999, -587785252, -669130606, -743144825, -809016994,
        -866025403, -913545457, -951056516, -978147600, -994521895,
        -1000000000, -994521895, -978147600, -951056516, -913545457,
        -866025403, -809016994, -743144825, -669130606, -587785252,
        -500000000, -406736643, -309016994, -207911690, -104528463,
        0,          104528463,  207911690,  309016994,  406736643,
        500000000,  587785252,  669130606,  743144825,  809016994,
        866025403,  913545457,  951056516,  978147600,  994521895,
    };
    if (n < 0)
        n += 60;
    return tab[n%60] / 1000 * maxval / 1000000;
}

int isin(int n, int maxval)
{
    static const int tab[] = {
        0,          104528463,  207911690,  309016994,  406736643,
        499999999,  587785252,  669130606,  743144825,  809016994,
        866025403,  913545457,  951056516,  978147600,  994521895,
        1000000000, 994521895,  978147600,  951056516,  913545457,
        866025403,  809016994,  743144825,  669130606,  587785252,
        499999999,  406736643,  309016994,  207911690,  104528463,
        0,          -104528463, -207911690, -309016994, -406736643,
        -500000000, -587785252, -669130606, -743144825, -809016994,
        -866025403, -913545457, -951056516, -978147600, -994521895,
        -1000000000,-994521895, -978147600, -951056516, -913545457,
        -866025403, -809016994, -743144825, -669130606, -587785252,
        -499999999, -406736643, -309016994, -207911690, -104528463,
    };
    if (n < 0)
        n += 60;
    return tab[n%60] / 1000 * maxval / 1000000;
}

void draw_circle(int hand_max, int sYcen, int sXcen)
{
    int x, y, r;
    char c;

    for (r=0; r<60; r++) {
        x = icos(r, hand_max) * FontWH_Ratio + sXcen;
        y = isin(r, hand_max) + sYcen;
        switch (r) {
            case 0:
            case 5:
            case 10:
            case 15:
            case 20:
            case 25:
            case 30:
            case 35:
            case 40:
            case 45:
            case 50:
            case 55:
                c='o';
                break;
            default:
                c='.';
                break;
        }
        draw_point(x, y, c);
    }
}

void draw_hand(int minute, int hlength, char c, int sXcen, int sYcen)
{
    int x, y, n;

    for (n=1; n<hlength; n++) {
        x = icos(minute - 15, n) * FontWH_Ratio + sXcen;
        y = isin(minute - 15, n) + sYcen;
        draw_point(x, y, c);
    }
}

int main(int argc, char **argv)
{
    char digital_time[32];
    int sXmax, sYmax, sXmaxo, sYmaxo, smax, hand_max, sXcen, sYcen;
    time_t t;
    struct tm *ltime;
    struct winsize ws;

    sXmaxo = sYmaxo = sXmax = sYmax = 0;
    sXcen = sYcen = 0;
    hand_max = 0;

    while (1) {
        sXmaxo = sXmax;
        sYmaxo = sYmax;

        ioctl(1, TIOCGWINSZ, &ws);

        sXmax = ws.ws_col;
        if (sXmax == 0)
            sXmax = 80;
        sYmax = ws.ws_row;
        if (sYmax == 0)
            sYmax = 24;

        if (sXmax != sXmaxo || sYmax != sYmaxo) {
            if (sXmax/2 <= sYmax)
                smax = sXmax/2;
            else
                smax = sYmax;

            hand_max = (smax/2)-1;

            sXcen = sXmax/2;
            sYcen = sYmax/2;

            cls();
            draw_circle(hand_max, sYcen, sXcen);
        }

        time(&t);
        ltime = localtime(&t);

        draw_hand((ltime->tm_hour*5) + (ltime->tm_min/10), 2*hand_max/3, 'h', sXcen, sYcen);
        draw_hand(ltime->tm_min, hand_max-2, 'm', sXcen, sYcen);
        draw_hand(ltime->tm_sec, hand_max-1, '.', sXcen, sYcen);

        draw_text(sXcen - 4, sYcen - (3*hand_max/5), "RetroBSD");
        sprintf(digital_time, "[%02d:%02d:%02d]", ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
        draw_text(sXcen - 5, sYcen + (3*hand_max/5), digital_time);

        fflush(stdout);
        sleep(1);

        draw_hand((ltime->tm_hour*5) + (ltime->tm_min/10), 2*hand_max/3, ' ', sXcen, sYcen);
        draw_hand(ltime->tm_min, hand_max-2, ' ', sXcen, sYcen);
        draw_hand(ltime->tm_sec, hand_max-1, ' ', sXcen, sYcen);
    }
    return 0;
}
