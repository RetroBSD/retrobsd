/*
 * Tetris (C) Copyright 1995, Vadim Antonov
 * Port to RetroBSD (C) 2015, Serge Vakulenko
 *
 * This program is designed to run on Olimex Duinomite board
 * with SainSmart Graphic LCD4884 shield, modified for 3.3V.
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/gpio.h>

#define PITWIDTH    12
#define PITDEPTH    21

#define NSHAPES     7
#define NBLOCKS     5

#define FIN         999

typedef struct {
    int x, y;
} coord_t;

typedef struct {
    int dx, dy;
    coord_t p[NBLOCKS];
} shape_t;

const shape_t shape [NSHAPES] = {
/* OOOO */  { 0, 3, { {0,0}, {0,1}, {0,2}, {0,3}, {FIN,FIN} } },

/* O   */   { 1, 2, { {0,0}, {1,0}, {1,1}, {1,2}, {FIN,FIN} } },
/* OOO */

/*  O  */   { 1, 2, { {0,1}, {1,0}, {1,1}, {1,2}, {FIN,FIN} } },
/* OOO */

/*   O */   { 1, 2, { {0,2}, {1,0}, {1,1}, {1,2}, {FIN,FIN} } },
/* OOO */

/*  OO */   { 1, 2, { {0,0}, {0,1}, {1,1}, {1,2}, {FIN,FIN} } },
/* OO  */

/* OO  */   { 1, 2, { {0,1}, {0,2}, {1,0}, {1,1}, {FIN,FIN} } },
/*  OO */

/* OO */    { 1, 1, { {0,0}, {0,1}, {1,0}, {1,1}, {FIN,FIN} } },
/* OO */
};

int pit [PITDEPTH+1] [PITWIDTH];
int pitcnt [PITDEPTH];
coord_t old [NBLOCKS], new [NBLOCKS], chk [NBLOCKS];
int gpio;                               /* File descriptor of GPIO driver. */
int adc3;                               /* File descriptor of ADC3 driver. */

/*-------------------------------------------------------------
 * Definitions for a "digital" joystick at A0 analog input.
 * Button values are determined by resistors on a board.
 */
enum {
    JOYSTICK_LEFT,
    JOYSTICK_SELECT,
    JOYSTICK_DOWN,
    JOYSTICK_RIGHT,
    JOYSTICK_UP,
    JOYSTICK_IDLE = -1,
};

/*
 * Initialize ADC for a joystick.
 */
void joystick_init()
{
    /* Open ADC driver. */
    adc3 = open("/dev/adc3", 0);
    if (adc3 < 0) {
        perror("/dev/adc3");
        exit(-1);
    }
}

/*
 * Get a state of joystick.
 * Convert ADC value to key number.
 * Input buttons are connected to a series network of resistors:
 * GND - 3.3k - 1k - 620 - 330 - 2k - +3.3V
 * Expected values are:
 * 0 - 144 - 329 - 506 - 741 - 1023
 */
int joystick_get()
{
    static const unsigned level[5] = { 72, 236, 417, 623, 882 };
    unsigned input, k;
    char buf[16];

    if (read(adc3, buf, sizeof(buf)) <= 0) {
        perror("adc");
        exit(-1);
    }
    input = strtol(buf, 0, 10);
    for (k=0; k<5; k++) {
        if (input < level[k]) {
            return k;
        }
    }
    return JOYSTICK_IDLE;
}

/*-------------------------------------------------------------
 * Routines for Nokia 5110 display.
 * See Philips PCD8544 datasheet.
 */
#define NROW 48
#define NCOL 84

/*
 * Pinout for SainSmart Graphic LCD4884 Shield.
 */
#define MASKE_LCD_SCK   (1 << 2)    /* signal D2, pin RE2 */
#define MASKE_LCD_MOSI  (1 << 3)    /* signal D3, pin RE3 */
#define MASKE_LCD_DC    (1 << 4)    /* signal D4, pin RE4 */
#define MASKE_LCD_CS    (1 << 5)    /* signal D5, pin RE5 */
#define MASKE_LCD_RST   (1 << 6)    /* signal D6, pin RE6 */
#define MASKE_LCD_BL    (1 << 7)    /* signal D7, pin RE7 */

static unsigned char gpanel_screen [NROW*NCOL/8];
int gpanel_row, gpanel_col;

static void lcd_cs(unsigned on)
{
    if (on) {
        ioctl(gpio, GPIO_PORTE | GPIO_SET, MASKE_LCD_CS);
    } else {
        ioctl(gpio, GPIO_PORTE | GPIO_CLEAR, MASKE_LCD_CS);
    }
}

static void lcd_rst(unsigned on)
{
    if (on) {
        ioctl(gpio, GPIO_PORTE | GPIO_SET, MASKE_LCD_RST);
    } else {
        ioctl(gpio, GPIO_PORTE | GPIO_CLEAR, MASKE_LCD_RST);
    }
}

static void lcd_dc(unsigned on)
{
    if (on) {
        ioctl(gpio, GPIO_PORTE | GPIO_SET, MASKE_LCD_DC);
    } else {
        ioctl(gpio, GPIO_PORTE | GPIO_CLEAR, MASKE_LCD_DC);
    }
}

static void lcd_bl(unsigned on)
{
    if (on) {
        ioctl(gpio, GPIO_PORTE | GPIO_SET, MASKE_LCD_BL);
    } else {
        ioctl(gpio, GPIO_PORTE | GPIO_CLEAR, MASKE_LCD_BL);
    }
}

static void lcd_mosi(unsigned on)
{
    if (on) {
        ioctl(gpio, GPIO_PORTE | GPIO_SET, MASKE_LCD_MOSI);
    } else {
        ioctl(gpio, GPIO_PORTE | GPIO_CLEAR, MASKE_LCD_MOSI);
    }
}

static void lcd_sck(unsigned on)
{
    if (on) {
        ioctl(gpio, GPIO_PORTE | GPIO_SET, MASKE_LCD_SCK);
    } else {
        ioctl(gpio, GPIO_PORTE | GPIO_CLEAR, MASKE_LCD_SCK);
    }
}

static void lcd_write(unsigned byte, unsigned data_flag)
{
    unsigned i;

    lcd_cs(0);
    lcd_dc(data_flag);
    for (i=0; i<8; i++, byte<<=1) {
        lcd_mosi(byte & 0x80);          /* SDIN = bit[i] */
        lcd_sck(0);                     /* SCLK = 0 */
        lcd_sck(1);                     /* SCLK = 1 */
    }
    lcd_cs(1);
}

/*
 * Set up hardware for communication to Nokia 5110 LCD Display.
 * Do not clear the display.
 * Leave backlight turned off.
 */
void gpanel_init()
{
    gpanel_row = 0;
    gpanel_col = 0;

    /* Open GPIO driver. */
    gpio = open("/dev/porta", 0);
    if (gpio < 0) {
        perror("/dev/porta");
        exit(-1);
    }

    /*
     * Set pins as outputs.
     */
    ioctl(gpio, GPIO_PORTE | GPIO_CONFOUT, MASKE_LCD_SCK |
        MASKE_LCD_MOSI | MASKE_LCD_DC | MASKE_LCD_CS |
        MASKE_LCD_RST | MASKE_LCD_BL);
    ioctl(gpio, GPIO_PORTE | GPIO_SET, MASKE_LCD_RST | MASKE_LCD_CS);
    ioctl(gpio, GPIO_PORTE | GPIO_CLEAR, MASKE_LCD_SCK |
        MASKE_LCD_MOSI | MASKE_LCD_DC | MASKE_LCD_BL);

    /* Turn off backlight. */
    lcd_bl(0);

    /* Reset the display. */
    lcd_rst(0);
    usleep(10000);          // need 1 usec
    lcd_rst(1);
    usleep(10000);          // need 1 usec

    lcd_write(0x21, 0);     // Enable extended instruction set
    lcd_write(0xbf, 0);     // Set Vop - contrast level
    lcd_write(0x04, 0);     // Set temperature coefficient to 0
    lcd_write(0x14, 0);     // Set bias to 4
    lcd_write(0x20, 0);     // Back to normal instruction set
    lcd_write(0x0c, 0);     // Set normal mode

    /* Enable backlight. */
    lcd_bl(1);
}

/*
 * Clear the the LCD screen.
 */
void gpanel_clear()
{
    unsigned i;

    /* Clear data */
    lcd_write(0x40, 0);
    lcd_write(0x80, 0);
    for (i=0; i<NROW*NCOL/8; i++) {
        gpanel_screen[i] = 0;
        lcd_write(0, 1);
    }
    gpanel_row = 0;
    gpanel_col = 0;
}

/*
 * Lights a single pixel in the specified color
 * at the specified x and y addresses
 */
void gpanel_pixel(int x, int y, int color)
{
    unsigned char *data;

    if (x >= NCOL || y >= NROW)
        return;
    data = &gpanel_screen [(y >> 3) * NCOL + x];

    if (color)
        *data |= 1 << (y & 7);
    else
        *data &= ~(1 << (y & 7));

    lcd_write(0x40 | (y >> 3), 0);
    lcd_write(0x80 | x, 0);
    lcd_write(*data, 1);
}

/*
 * Draw a filled rectangle in the specified color from (x1,y1) to (x2,y2).
 *
 * The best way to fill a rectangle is to take advantage of the "wrap-around" featute
 * built into the Philips PCF8833 controller. By defining a drawing box, the memory can
 * be simply filled by successive memory writes until all pixels have been illuminated.
 */
void gpanel_rect_filled(int x0, int y0, int x1, int y1, int color)
{
    /* Temporary solution */
    int xmin, xmax, ymin, ymax, x, y;

    /* calculate the min and max for x and y directions */
    if (x0 <= x1) {
        xmin = x0;
        xmax = x1;
    } else {
        xmin = x1;
        xmax = x0;
    }
    if (y0 <= y1) {
        ymin = y0;
        ymax = y1;
    } else {
        ymin = y1;
        ymax = y0;
    }
    for (y=ymin; y<=ymax; y++)
        for (x=xmin; x<=xmax; x++)
            gpanel_pixel(x, y, color);
}

/*-------------------------------------------------------------
 * Output piece coordinates given its center and angle
 */
void translate(const shape_t *t, const coord_t *c, int a, coord_t *res)
{
    coord_t org, tmp;
    int yw, xw, i;

    if (a & 1) {            /* 90 deg */
        xw = t->dy;
        yw = t->dx;
    } else {
        xw = t->dx;
        yw = t->dy;
    }
    org = *c;
    org.x -= (xw + 1) / 2;
    org.y -= yw / 2;
    if (org.y < 0)
        org.y = 0;
    if (org.y + yw >= PITWIDTH && c->y <= PITWIDTH)
        org.y = PITWIDTH-1 - yw;
    for (i=0; t->p[i].x!=FIN; i++) {
        switch (a) {
        case 0:
            res[i].x = t->p[i].x;
            res[i].y = t->p[i].y;
            break;
        case 3:
            res[i].x = xw - t->p[i].y;
            res[i].y = t->p[i].x;
            break;
        case 2:
            res[i].x = xw - t->p[i].x;
            res[i].y = yw - t->p[i].y;
            break;
        case 1:
            res[i].x = t->p[i].y;
            res[i].y = yw - t->p[i].x;
        }
        res[i].x += org.x;
        res[i].y += org.y;
    }
    res[i].x = res[i].y = FIN;

    do {
        xw = 0;
        for (i=0; res[i+1].x!=FIN; i++) {
            if (res[i].x < res[i+1].x)
                continue;
            if (res[i].x == res[i+1].x && res[i].y <= res[i+1].y)
                continue;
            xw++;
            tmp = res[i];
            res[i] = res[i+1];
            res[i+1] = tmp;
        }
    } while (xw);
}

/*
 * Draw the block
 */
void draw_block(int h, int w, int visible)
{
    h *= 4;
    w *= 4;
    if (visible) {
        gpanel_rect_filled(NCOL-1 - h, w, NCOL-1 - (h + 3), w + 3, 1);
    } else {
        gpanel_rect_filled(NCOL-1 - h, w, NCOL-1 - (h + 3), w + 3, 0);

        if (h == (PITDEPTH-1)*5)
            gpanel_pixel(NCOL-1 - (h + 3), w + 2, 1);

        if (w == 0)
            gpanel_pixel(NCOL-1 - (h + 2), w, 1);
        else if (w % 16 == 12)
            gpanel_pixel(NCOL-1 - (h + 2), w + 3, 1);
    }
}

/*
 * Move the piece
 */
void move(coord_t *old, coord_t *new)
{
    for (;;) {
        if (old->x == FIN) {
draw:       if (new->x == FIN)
                break;
            if (new->x >= 0)
                draw_block(new->x, new->y, 1);
            new++;
            continue;
        }
        if (new->x == FIN) {
clear:      if (old->x >= 0)
                draw_block(old->x, old->y, 0);
            old++;
            continue;
        }
        if (old->x > new->x)
            goto draw;
        if (old->x < new->x)
            goto clear;
        if (old->y > new->y)
            goto draw;
        if (old->y != new->y)
            goto clear;
        /* old & new at the same place */
        old++;
        new++;
    }
}

/*
 * Draw the pit
 */
void clear()
{
    int h, w;

    for (h=0; h<PITDEPTH; h++) {
        for (w=0; w<PITWIDTH; w++) {
            draw_block(h, w, 0);
            pit[h][w] = 0;
        }
        pitcnt[h] = 0;
    }
    for (w=0; w<PITWIDTH; w++)
        pit[PITDEPTH][w] = 1;
}

/*
 * The piece reached the bottom
 */
void stopped(coord_t *c)
{
    int h, nfull, w, k;

    /* Count the full lines */
    nfull = 0;
    for (; c->x!=FIN; c++) {
        if (c->x <= 0) {
            /* Game over. */
            clear();
            return;
        }
        pit[c->x][c->y] = 1;
        ++pitcnt[c->x];
        if (pitcnt[c->x] == PITWIDTH)
            nfull++;
    }
    if (! nfull)
        return;

    /* Clear upper nfull lines */
    for (h=0; h<nfull; h++) {
        for (w=0; w<PITWIDTH; w++) {
            if (pit[h][w]) {
                draw_block(h, w, 0);
            }
        }
    }

    /* Move lines down */
    k = nfull;
    for (h=nfull; h<PITDEPTH && k>0; h++) {
        if (pitcnt[h-k] == PITWIDTH) {
            k--;
            h--;
            continue;
        }
        for (w=0; w<PITWIDTH; w++) {
            if (pit[h][w] != pit[h-k][w]) {
                draw_block(h, w, pit[h-k][w]);
            }
        }
    }

    /* Now fix the pit contents */
    for (h=PITDEPTH-1; h>0; h--) {
        if (pitcnt[h] != PITWIDTH)
            continue;
        memmove(pit[0]+PITWIDTH, pit[0], h * sizeof(pit[0]));
        memset(pit[0], 0, sizeof(pit[0]));
        memmove(pitcnt+1, pitcnt, h * sizeof(pitcnt[0]));
        pitcnt[0] = 0;
        h++;
    }
}

int main()
{
    int ptype;              /* Piece type */
    int angle, anew;        /* Angle */
    int msec;               /* Timeout */
    coord_t c, cnew, *cp;
    unsigned up_pressed = 0, left_pressed = 0;
    unsigned right_pressed = 0, down_pressed = 0;

    joystick_init();
    gpanel_init();
    gpanel_clear();

    /* Draw the pit */
    clear();

newpiece:
    ptype = rand() % NSHAPES;
    angle = rand() % 3;

    c.y = PITWIDTH/2 - 1;
    for (c.x= -2; ; c.x++) {
        translate(&shape[ptype], &c, angle, new);
        for (cp=new; cp->x!=FIN; cp++) {
            if (cp->x >= 0)
                goto ok;
        }
    }
ok:
    /* Draw new piece */
    for (cp=new; cp->x!=FIN; cp++) {
        if (cp->x >= 0) {
            draw_block(cp->x, cp->y, 1);
        }
    }
    memcpy(old, new, sizeof old);

    msec = 500;
    for (;;) {
        cnew = c;
        anew = angle;

        if (msec <= 0) {
            /* Timeout: move down */
            msec = 500;
            cnew.x++;
            translate(&shape[ptype], &cnew, anew, chk);
            for (cp=chk; cp->x!=FIN; cp++) {
                if (cp->x >= 0 && pit[cp->x][cp->y]) {
                    stopped(new);
                    goto newpiece;
                }
            }
            goto check;
        }

        int key = joystick_get();
        if (key != JOYSTICK_RIGHT)
            right_pressed = 0;
        else if (! right_pressed) {
            right_pressed = 1;

            /* Right: rotate */
            if (--anew < 0)
                anew = 3;
            translate(&shape[ptype], &cnew, anew, chk);
            goto check;
        }

        if (key != JOYSTICK_UP)
            up_pressed = 0;
        else if (! up_pressed) {
            up_pressed = 1;

            /* Up: move left. */
            if (cnew.y <= 0)
                continue;
            cnew.y--;
            translate(&shape[ptype], &cnew, anew, chk);
            goto check;
        }

        if (key != JOYSTICK_DOWN)
            down_pressed = 0;
        else if (! down_pressed) {
            down_pressed = 1;

            /* Down: move right */
            if (cnew.y >= PITWIDTH-1)
                continue;
            cnew.y++;
            translate(&shape[ptype], &cnew, anew, chk);
            goto check;
        }

        if (key != JOYSTICK_LEFT)
            left_pressed = 0;
        else if (! left_pressed) {
            left_pressed = 1;

            /* Right: drop */
            for (;;) {
                cnew.x++;
                translate(&shape[ptype], &cnew, anew, chk);
                for (cp=chk; cp->x!=FIN; cp++) {
                    if (cp->x >= 0 && pit[cp->x][cp->y]) {
                        cnew.x--;
                        translate(&shape[ptype], &cnew, anew, chk);
                        move(new, chk);
                        stopped(chk);
                        goto newpiece;
                    }
                }
            }
        }

        usleep(10000);
        msec -= 10;
        continue;
check:
        for (cp=chk; cp->x!=FIN; cp++) {
            if (cp->y < 0 || cp->y >= PITWIDTH)
                goto done;
        }
        for (cp=chk; cp->x!=FIN; cp++) {
            if (cp->x >= 0 && pit[cp->x][cp->y])
                goto done;
        }
        c = cnew;
        angle = anew;
        memcpy(old, new, sizeof old);
        memcpy(new, chk, sizeof new);
        move(old, new);
done:   ;
    }
}
