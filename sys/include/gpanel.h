/*
 * Generic interface to a graphics panel device (TFT LCD, OLED and others).
 *
 * Copyright (C) 2015 Serge Vakulenko, <serge@vak.ru>
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
#ifndef _GPANEL_H
#define _GPANEL_H

/*
 * Proportional/fixed font structure.
 */
struct gpanel_font_t {
    const char *    name;           /* font name */
    int             maxwidth;       /* max width in pixels */
    unsigned int    height;         /* height in pixels */
    int             ascent;         /* ascent (baseline) height */
    int             firstchar;      /* first character in bitmap */
    int             size;           /* font size in characters */
    const unsigned short *bits;     /* 16-bit right-padded bitmap data */
    const unsigned short *offset;   /* offsets into bitmap data */
    const unsigned char *width;     /* character widths or 0 if fixed */
    int             defaultchar;    /* default char (not glyph index) */
    long            bits_size;      /* # words of bits */
};

struct gpanel_pixel_t {
    int             color;          /* pixel color */
    int             x, y;           /* pixel position */
};

struct gpanel_clear_t {
    int             color;          /* pixel color */
    int             xsize, ysize;   /* screen size */
};

struct gpanel_line_t {
    int             color;          /* line color */
    int             x0, y0;         /* start point */
    int             x1, y1;         /* end point */
};

struct gpanel_rect_t {
    int             color;          /* border or fill color */
    int             x0, y0;         /* start point */
    int             x1, y1;         /* end point */
};

struct gpanel_circle_t {
    int             color;          /* border color */
    int             x, y;           /* center point */
    int             radius;         /* circle radius */
};

struct gpanel_image_t {
    int             x, y;           /* start point */
    int             width, height;  /* image size radius */
    const unsigned short *image;    /* array of pixels */
};

struct gpanel_char_t {
    const struct gpanel_font_t *font; /* font data */
    int             color;          /* text color */
    int             background;     /* background color or -1 for transparent */
    int             x, y;           /* position */
    int             sym;            /* unicode symbol index */
};

struct gpanel_text_t {
    const struct gpanel_font_t *font; /* font data */
    int             color;          /* text color */
    int             background;     /* background color or -1 for transparent */
    int             x, y;           /* position */
    const char      *text;          /* UTF-8 text */
};

#define GPANEL_CLEAR       _IOW('g', 1, struct gpanel_clear_t)
#define GPANEL_PIXEL       _IOW('g', 2, struct gpanel_pixel_t)
#define GPANEL_LINE        _IOW('g', 3, struct gpanel_line_t)
#define GPANEL_RECT        _IOW('g', 4, struct gpanel_rect_t)
#define GPANEL_FILL        _IOW('g', 5, struct gpanel_rect_t)
#define GPANEL_CIRCLE      _IOW('g', 6, struct gpanel_circle_t)
#define GPANEL_IMAGE       _IOW('g', 7, struct gpanel_image_t)
#define GPANEL_CHAR        _IOW('g', 8, struct gpanel_char_t)
#define GPANEL_TEXT        _IOW('g', 9, struct gpanel_text_t)

#ifndef KERNEL
/*
 * User-level library.
 */
int gpanel_open(const char *devname);
void gpanel_close(void);
void gpanel_clear(int color, int *xsize, int *ysize);
void gpanel_pixel(int color, int x, int y);
void gpanel_line(int color, int x0, int y0, int x1, int y1);
void gpanel_rect(int color, int x0, int y0, int x1, int y1);
void gpanel_fill(int color, int x0, int y0, int x1, int y1);
void gpanel_fill_triangle(int color, int x0, int y0, int x1, int y1, int x2, int y2);
void gpanel_circle(int color, int x, int y, int radius);
void gpanel_image(int x, int y, int width, int height, const unsigned short *data);
void gpanel_char(const struct gpanel_font_t *font, int color, int background, int x, int y, int sym);
void gpanel_text(const struct gpanel_font_t *font, int color, int background, int x, int y, const char *text);
int gpanel_text_width(const struct gpanel_font_t *font, const char *text, int nchars);

extern int _gpanel_fd;

#else /* KERNEL */
/*
 * Kernel driver routines.
 */
struct uio;
extern int gpanel_open(dev_t dev, int flag, int mode);
extern int gpanel_close(dev_t dev, int flag, int mode);
extern int gpanel_read(dev_t dev, struct uio *uio, int flag);
extern int gpanel_write(dev_t dev, struct uio *uio, int flag);
extern int gpanel_ioctl(dev_t dev, u_int cmd, caddr_t addr, int flag);

extern int gpanel_read_byte(void);
extern void gpanel_write_byte(int value);
extern void gpanel_cs_active(void);
extern void gpanel_cs_idle(void);
extern void gpanel_rs_command(void);
extern void gpanel_rs_data(void);
extern void gpanel_wr_strobe(void);
extern void gpanel_read_dir(void);
extern void gpanel_write_dir(void);
extern int gpanel_send_command(int value);
extern int gpanel_send_data(int value);

/*
 * Descriptor for access to the hardware-level driver.
 */
struct gpanel_hw {
    const char *name;
    void (*resize)(struct gpanel_hw *hw, int width, int height);
    void (*set_pixel)(int x, int y, int color);
    void (*fill_rectangle)(int x0, int y0, int x1, int y1, int color);
    void (*draw_image)(int x, int y, int width, int height,
        const unsigned short *data);
    void (*draw_glyph)(const struct gpanel_font_t *font,
        int color, int background, int x, int y, int width,
        const unsigned short *bits);
};
extern int gpanel_width;
extern int gpanel_height;
extern void st7781_init_display(struct gpanel_hw *hw);
extern void nt35702_init_display(struct gpanel_hw *hw);
extern void ili9341_init_display(struct gpanel_hw *hw);
extern void ili9481_init_display(struct gpanel_hw *hw);
extern void s6d04h0_init_display(struct gpanel_hw *hw);

#endif /* KERNEL */

#endif /* _GPANEL_H */
