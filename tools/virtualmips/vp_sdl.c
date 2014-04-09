 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *     
  * This file is part of the virtualmips distribution. 
  * See LICENSE file for terms of the license. 
  *
  */

#ifdef SIM_LCD
#include "vp_sdl.h"
#include <signal.h>

SDL_Surface *screen;
SDL_Event ev;

static void sdl_update (DisplayState * ds, int x, int y, int w, int h)
{
    SDL_UpdateRect (screen, x, y, w, h);
}

static void sdl_resize (DisplayState * ds, int w, int h)
{
    int flags;

    // printf("resizing to %d %d\n", w, h);

    flags = SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_HWACCEL;
  again:
    screen = SDL_SetVideoMode (w, h, ds->depth, flags);
    if (!screen) {
        fprintf (stderr, "Could not open SDL display\n");
        exit (1);
    }
    if (!screen->pixels && (flags & SDL_HWSURFACE)
        && (flags & SDL_FULLSCREEN)) {
        flags &= ~SDL_HWSURFACE;
        goto again;
    }

    if (!screen->pixels) {
        fprintf (stderr, "Could not open SDL display\n");
        exit (1);
    }
    ds->data = screen->pixels;
    ds->linesize = screen->pitch;
    //ds->depth = screen->format->BitsPerPixel;
    if (ds->depth == 32 && screen->format->Rshift == 0) {
        ds->bgr = 1;
    } else {
        ds->bgr = 0;
    }
}

SDL_Event *sdl_getmouse_down ()
{
    if (SDL_PollEvent (&ev)) {
        if (ev.type == SDL_MOUSEBUTTONDOWN) {
            return &ev;
        }
    }
    return NULL;
}

SDL_Event *sdl_getmouse_up ()
{
    if (SDL_PollEvent (&ev)) {
        if (ev.type == SDL_MOUSEBUTTONUP) {
            return &ev;
        }
    }
    return NULL;
}

static void sdl_refresh (DisplayState * ds)
{

}

static void sdl_update_caption (void)
{
    char buf[1024];
    strcpy (buf, "VirtualMIPS");
    SDL_WM_SetCaption (buf, "");
}

/*
loading VirtualMIPS logo. 
A bit ugly logo.
Sorry CNN, I have not learned how to ps pictures. Please forgive me.
*/
static void sdl_display_logo (void)
{
    SDL_Surface *image;
    SDL_Rect dest;
    image = SDL_LoadBMP ("logo.bmp");
    if (image == NULL) {
        fprintf (stderr, "can not load logo logo.bmp: %s\n", SDL_GetError ());
        return;
    }
    dest.x = 0;
    dest.y = 0;
    dest.w = image->w;
    dest.h = image->h;
    SDL_BlitSurface (image, NULL, screen, &dest);

    SDL_UpdateRects (screen, 1, &dest);

}

static void sdl_cleanup (void)
{
    printf ("SDL Clean \n");
    SDL_Quit ();
}

void sdl_display_init (DisplayState * ds, int full_screen)
{
    int flags;

    flags = SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE;
    if (SDL_Init (flags)) {
        fprintf (stderr, "Could not initialize SDL - exiting\n");
        exit (1);
    }
#ifndef _WIN32
    signal (SIGINT, SIG_DFL);
    signal (SIGQUIT, SIG_DFL);
#endif

    ds->dpy_update = sdl_update;
    ds->dpy_resize = sdl_resize;
    ds->dpy_refresh = sdl_refresh;

    sdl_resize (ds, ds->width, ds->height);
    sdl_update_caption ();
    sdl_display_logo ();
    SDL_EnableUNICODE (1);

    atexit (sdl_cleanup);
}

#endif
