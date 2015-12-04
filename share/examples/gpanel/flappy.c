#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sgtty.h>
#include <sys/time.h>
#include <sys/gpanel.h>

/*
 * Assign human-readable names to some common 16-bit color values:
 */
#define	BLACK   0x0000
#define	BLUE    0x002F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

/*
 * Redraw every 50 msec
 */
#define DRAW_LOOP_INTERVAL  50

/*
 * File name for saving the high score.
 */
#define SCORE_FILENAME      "flappy.score"

/*
 * Data from external font files.
 */
extern const struct gpanel_font_t font_lucidasans15;
extern const struct gpanel_font_t font_lucidasans28;

int wing;
int fx, fy, fall_rate;
int pillar_pos, gap_pos;
int score;
int high_score = 0;
int running = 0;
int crashed = 0;
int scr_press = 0;
time_t next_draw_time;

void rect(int color, int x, int y, int w, int h)
{
    gpanel_rect(color, x, y, x+w-1, y+h-1);
}

void fill(int color, int x, int y, int w, int h)
{
    gpanel_fill(color, x, y, x+w-1, y+h-1);
}

void draw_pillar(int x, int gap)
{
    if (x >= 320)
        return;

    fill(GREEN, x+2, 2,      46, gap-4);
    fill(GREEN, x+2, gap+92, 46, 136-gap);

    rect(BLACK, x,   0,      50, gap);
    rect(BLACK, x+1, 1,      48, gap-2);
    rect(BLACK, x,   gap+90, 50, 140-gap);
    rect(BLACK, x+1, gap+91, 48, 138-gap);
}

void clear_pillar(int x, int gap)
{
    if (x >= 320)
        return;

    /* "Cheat" slightly and just clear the right hand pixels
     * to help minimise flicker, the rest will be overdrawn. */
    fill(BLUE, x+45, 0,      5, gap);
    fill(BLUE, x+45, gap+90, 5, 140-gap);
}

void clear_flappy(int x, int y)
{
    fill(BLUE, x, y, 34, 24);
}

void draw_flappy(int x, int y)
{
    /* Upper & lower body */
    fill(BLACK, x+2,  y+8,  2,  10);
    fill(BLACK, x+4,  y+6,  2,  2);
    fill(BLACK, x+6,  y+4,  2,  2);
    fill(BLACK, x+8,  y+2,  4,  2);
    fill(BLACK, x+12, y,    12, 2);
    fill(BLACK, x+24, y+2,  2,  2);
    fill(BLACK, x+26, y+4,  2,  2);
    fill(BLACK, x+28, y+6,  2,  6);
    fill(BLACK, x+10, y+22, 10, 2);
    fill(BLACK, x+4,  y+18, 2,  2);
    fill(BLACK, x+6,  y+20, 4,  2);

    /* Body fill */
    fill(YELLOW, x+12, y+2,  6,  2);
    fill(YELLOW, x+8,  y+4,  8,  2);
    fill(YELLOW, x+6,  y+6,  10, 2);
    fill(YELLOW, x+4,  y+8,  12, 2);
    fill(YELLOW, x+4,  y+10, 14, 2);
    fill(YELLOW, x+4,  y+12, 16, 2);
    fill(YELLOW, x+4,  y+14, 14, 2);
    fill(YELLOW, x+4,  y+16, 12, 2);
    fill(YELLOW, x+6,  y+18, 12, 2);
    fill(YELLOW, x+10, y+20, 10, 2);

    /* Eye */
    fill(BLACK, x+18, y+2,  2, 2);
    fill(BLACK, x+16, y+4,  2, 6);
    fill(BLACK, x+18, y+10, 2, 2);
    fill(WHITE, x+18, y+4,  2, 6);
    fill(WHITE, x+20, y+2,  4, 10);
    fill(WHITE, x+24, y+4,  2, 8);
    fill(WHITE, x+26, y+6,  2, 6);
    fill(BLACK, x+24, y+6,  2, 4);

    /* Beak */
    fill(BLACK, x+20, y+12, 12, 2);
    fill(BLACK, x+18, y+14, 2,  2);
    fill(RED,   x+20, y+14, 12, 2);
    fill(BLACK, x+32, y+14, 2,  2);
    fill(BLACK, x+16, y+16, 2,  2);
    fill(RED,   x+18, y+16, 2,  2);
    fill(BLACK, x+20, y+16, 12, 2);
    fill(BLACK, x+18, y+18, 2,  2);
    fill(RED,   x+20, y+18, 10, 2);
    fill(BLACK, x+30, y+18, 2,  2);
    fill(BLACK, x+20, y+20, 10, 2);
}

/*
 * Wing down.
 */
void draw_wing1(int x, int y)
{
    fill(BLACK, x,    y+14, 2,  6);
    fill(BLACK, x+2,  y+20, 8,  2);
    fill(BLACK, x+2,  y+12, 10, 2);
    fill(BLACK, x+12, y+14, 2,  2);
    fill(BLACK, x+10, y+16, 2,  2);
    fill(WHITE, x+2,  y+14, 8,  6);
    fill(BLACK, x+8,  y+18, 2,  2);
    fill(WHITE, x+10, y+14, 2,  2);
}

/*
 * Wing middle.
 */
void draw_wing2(int x, int y)
{
    fill(BLACK, x+2,  y+10, 10, 2);
    fill(BLACK, x+2,  y+16, 10, 2);
    fill(BLACK, x,    y+12, 2,  4);
    fill(BLACK, x+12, y+12, 2,  4);
    fill(WHITE, x+2,  y+12, 10, 4);
}

/*
 * Wing up.
 */
void draw_wing3(int x, int y)
{
    fill(BLACK, x+2,  y+6,  8, 2);
    fill(BLACK, x,    y+8,  2, 6);
    fill(BLACK, x+10, y+8,  2, 2);
    fill(BLACK, x+12, y+10, 2, 4);
    fill(BLACK, x+10, y+14, 2, 2);
    fill(BLACK, x+2,  y+14, 2, 2);
    fill(BLACK, x+4,  y+16, 6, 2);
    fill(WHITE, x+2,  y+8,  8, 6);
    fill(WHITE, x+4,  y+14, 6, 2);
    fill(WHITE, x+10, y+10, 2, 4);
}

time_t millis()
{
    struct timeval tv;

    gettimeofday(&tv, 0);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

void start_game()
{
    fx = 50;
    fy = 125;
    fall_rate = -1;
    pillar_pos = 320;
    gap_pos = 60;
    crashed = 0;
    score = 0;

    gpanel_clear(BLUE, 0, 0);
    gpanel_text(&font_lucidasans28, WHITE, BLUE, 10, 10,
        "Flappy Bird");
    gpanel_text(&font_lucidasans15, WHITE, BLUE, 50, 180,
        "(Press Space to start)");

    char score_line[80];
    sprintf(score_line, "High Score: %u", high_score);
    gpanel_text(&font_lucidasans28, GREEN, BLUE, 10, 60, score_line);

    /* Draw ground. */
    int tx, ty = 230;
    for (tx = 0; tx <= 300; tx += 20) {
        gpanel_fill_triangle(GREEN,  tx,    ty,   tx+9,  ty, tx,    ty+9);
        gpanel_fill_triangle(YELLOW, tx+9,  ty+9, tx+9,  ty, tx,    ty+9);
        gpanel_fill_triangle(YELLOW, tx+10, ty,   tx+19, ty, tx+10, ty+9);
        gpanel_fill_triangle(GREEN,  tx+19, ty+9, tx+19, ty, tx+10, ty+9);
    }

    next_draw_time = millis() + DRAW_LOOP_INTERVAL;
}

void draw_loop()
{
    /* Clear moving items. */
    clear_pillar(pillar_pos, gap_pos);

    /* Move items. */
    if (running) {
        clear_flappy(fx, fy);

        fy += fall_rate;
        fall_rate++;

        pillar_pos -= 5;
        if (pillar_pos == 0) {
            score++;
        }
        else if (pillar_pos < -50) {
            pillar_pos = 320;
            gap_pos = 20 + random() % 100;
        }
    }

    /* Draw moving items & animate. */
    draw_flappy(fx, fy);
    draw_pillar(pillar_pos, gap_pos);
    switch (wing) {
    case 0:
    case 1:
        draw_wing1(fx, fy);
        break;
    case 2:
    case 3:
        draw_wing2(fx, fy);
        break;
    case 4:
    case 5:
        draw_wing3(fx, fy);
        break;
    }
    wing++;
    if (wing == 6)
        wing = 0;
}

/*
 * Write high score value to file.
 */
void save_score()
{
    int fd, nbytes;
    char line[80];

    fd = open(SCORE_FILENAME, O_WRONLY | O_CREAT, 0644);
    if (fd < 0) {
        perror(SCORE_FILENAME);
        return;
    }
    sprintf(line, "%u\n", high_score);
    nbytes = strlen(line);
    if (write(fd, line, nbytes) != nbytes)
        perror(SCORE_FILENAME);
    close(fd);
}

/*
 * Read high score value from file.
 */
void load_score()
{
    int fd, nbytes;
    char line[80];

    fd = open(SCORE_FILENAME, O_RDONLY);
    if (fd < 0) {
        /* No high score file yet. */
        return;
    }
    nbytes = read(fd, line, sizeof(line));
    if (nbytes <= 0) {
        if (nbytes < 0)
            perror(SCORE_FILENAME);
        return;
    }
    close(fd);
    high_score = strtol(line, 0, 0);
}

void check_collision()
{
    /* Collision with ground. */
    if (fy > 206)
        crashed = 1;

    /* Collision with pillar. */
    if (fx + 34 > pillar_pos && fx < pillar_pos + 50)
        if (fy < gap_pos || fy + 24 > gap_pos + 90)
             crashed = 1;

    if (crashed) {
        gpanel_text(&font_lucidasans28, RED, BLUE, 50, 50, "Game Over!");

        char score_line[80];
        sprintf(score_line, "Score: %u", score);
        gpanel_text(&font_lucidasans28, RED, BLUE, 50, 100, score_line);

        if (score > high_score) {
            high_score = score;
            gpanel_text(&font_lucidasans28, RED, BLUE, 50, 150, "NEW HIGH!");
            save_score();
        }

        /* Stop animation. */
        running = 0;

        /* Delay to stop any last minute clicks from restarting immediately. */
        sleep(1);
    }
}

#if 1
#ifndef SDL
struct sgttyb origtty, newtty;
#endif

/*
 * Terminate the game when ^C pressed.
 */
void quit(int sig)
{
    signal(SIGINT, SIG_IGN);
#ifndef SDL
    if (newtty.sg_flags != 0)
        ioctl(0, TIOCSETP, &origtty);
#endif
    gpanel_close();
    exit(0);
}

/*
 * Return 1 when any key is pressed on console.
 */
int get_input()
{
#ifdef SDL
    extern int gpanel_input(void);

    return gpanel_input();
#else
    if (newtty.sg_flags == 0) {
        ioctl(0, TIOCGETP, &origtty);

        newtty = origtty;
        newtty.sg_flags &= ~(ECHO|XTABS);
        newtty.sg_flags |= CBREAK;
        ioctl(0, TIOCSETP, &newtty);
    }
    int nchars = 0;
    ioctl(0, FIONREAD, &nchars);
    if (nchars <= 0)
        return 0;

    char c;
    read(0, &c, 1);
    return 1;
#endif
}
#endif

int main()
{
    char *devname = "/dev/tft0";
    int xsize = 320, ysize = 240;

    signal(SIGINT, quit);
    if (gpanel_open(devname) < 0) {
        printf("Cannot open %s\n", devname);
        exit(-1);
    }
    gpanel_clear(BLUE, &xsize, &ysize);

    load_score();
    start_game();

    for (;;) {
        if (millis() > next_draw_time && !crashed) {
            draw_loop();
            check_collision();
            next_draw_time += DRAW_LOOP_INTERVAL;
        }
        usleep(10000);

        /* Get user input. */
        int user_input = get_input();

        /* Process "user input". */
        if (user_input > 0 && !scr_press) {
            if (crashed) {
                /* Restart game. */
                start_game();
            }
            else if (!running) {
                /* Clear text & start scrolling. */
                gpanel_fill(BLUE, 0, 0, 320-1, 100);
                gpanel_fill(BLUE, 0, 180, 320-1, 205);
                running = 1;
            } else {
                /* Fly up. */
                fall_rate = -8;
                scr_press = 1;
            }
        } else if (user_input == 0 && scr_press) {
            /* Attempt to throttle presses. */
            scr_press = 0;
        }
    }
}
