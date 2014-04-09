 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *     
  * This file is part of the virtualmips distribution. 
  * See LICENSE file for terms of the license. 
  *
  */

#ifndef __VSDL_H__
#define __VSDL_H__

#ifdef SIM_LCD                  /*defined in Makefile */

#include "utils.h"
#include "SDL/SDL.h"

struct DisplayState {
    uint8_t *data;
    int linesize;
    int depth;
    int bgr;                    /* BGR color order instead of RGB. Only valid for depth == 32 */
    int width;
    int height;
    void *opaque;

    void (*dpy_update) (struct DisplayState * s, int x, int y, int w, int h);
    void (*dpy_resize) (struct DisplayState * s, int w, int h);
    void (*dpy_refresh) (struct DisplayState * s);
    void (*dpy_copy) (struct DisplayState * s, int src_x, int src_y,
        int dst_x, int dst_y, int w, int h);
};

typedef struct DisplayState DisplayState;

static inline void dpy_update (DisplayState * s, int x, int y, int w, int h)
{
    s->dpy_update (s, x, y, w, h);
}

static inline void dpy_resize (DisplayState * s, int w, int h)
{
    s->dpy_resize (s, w, h);
}

/*
static inline void draw_pixel(SDL_Surface *screen, Uint8 R, Uint8 G, Uint8 B,Uint32 x,Uint32 y)
{
	Uint32 color = SDL_MapRGB(screen->format, R, G, B);

    switch (screen->format->BytesPerPixel) {
        case 1: {
            Uint8 *bufp;

            bufp = (Uint8 *)screen->pixels + y*screen->pitch + x;
            *bufp = color;
        }
        break;

        case 2: {
            Uint16 *bufp;

            bufp = (Uint16 *)screen->pixels + y*screen->pitch/2 + x;
            *bufp = color;
        }
        break;

        case 3: {
            Uint8 *bufp;

            bufp = (Uint8 *)screen->pixels + y*screen->pitch + x;
            *(bufp+screen->format->Rshift/8) = R;
            *(bufp+screen->format->Gshift/8) = G;
            *(bufp+screen->format->Bshift/8) = B;
        }
        break;

        case 4: {
            Uint32 *bufp;

            bufp = (Uint32 *)screen->pixels + y*screen->pitch/4 + x;
            *bufp = color;
        }
        break;
    }

}
*/
void sdl_display_init (DisplayState * ds, int full_screen);
SDL_Event *sdl_getmouse_down ();
SDL_Event *sdl_getmouse_up ();
#endif

#endif
