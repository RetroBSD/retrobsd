#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/gpanel.h>
#include <sys/time.h>
#ifdef CROSS
#   include <termios.h>
#   define sgttyb termio
#else
#   include <sgtty.h>
#endif

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
#define DRAW_LOOP_INTERVAL 50

/*
 * Data from external font files.
 */
extern const struct gpanel_font_t font_lucidasans15;
extern const struct gpanel_font_t font_lucidasans28;

int wing;
int fx, fy, fallRate;
int pillarPos, gapPos;
int score;
int highScore = 0;
int running = 0;
int crashed = 0;
int scrPress = 0;
time_t nextDrawLoopRunTime;

void rect(int color, int x, int y, int w, int h)
{
    gpanel_rect(color, x, y, x+w-1, y+h-1);
}

void fill(int color, int x, int y, int w, int h)
{
    gpanel_fill(color, x, y, x+w-1, y+h-1);
}

void drawPillar(int x, int gap)
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

void clearPillar(int x, int gap)
{
    if (x >= 320)
        return;

    // "cheat" slightly and just clear the right hand pixels
    // to help minimise flicker, the rest will be overdrawn
    fill(BLUE, x+45, 0,      5, gap);
    fill(BLUE, x+45, gap+90, 5, 140-gap);
}

void clearFlappy(int x, int y)
{
    fill(BLUE, x, y, 34, 24);
}

void drawFlappy(int x, int y)
{
    // Upper & lower body
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

    // Body fill
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

    // Eye
    fill(BLACK, x+18, y+2,  2, 2);
    fill(BLACK, x+16, y+4,  2, 6);
    fill(BLACK, x+18, y+10, 2, 2);
    fill(WHITE, x+18, y+4,  2, 6);
    fill(WHITE, x+20, y+2,  4, 10);
    fill(WHITE, x+24, y+4,  2, 8);
    fill(WHITE, x+26, y+6,  2, 6);
    fill(BLACK, x+24, y+6,  2, 4);

    // Beak
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

// Wing down
void drawWing1(int x, int y)
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

// Wing middle
void drawWing2(int x, int y)
{
    fill(BLACK, x+2,  y+10, 10, 2);
    fill(BLACK, x+2,  y+16, 10, 2);
    fill(BLACK, x,    y+12, 2,  4);
    fill(BLACK, x+12, y+12, 2,  4);
    fill(WHITE, x+2,  y+12, 10, 4);
}

// Wing up
void drawWing3(int x, int y)
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

void startGame()
{
    fx = 50;
    fy = 125;
    fallRate = -1;
    pillarPos = 320;
    gapPos = 60;
    crashed = 0;
    score = 0;

    gpanel_clear(BLUE, 0, 0);
    gpanel_text(&font_lucidasans28, WHITE, BLUE, 10, 10,
        "Flappy Bird");
    gpanel_text(&font_lucidasans15, WHITE, BLUE, 50, 180,
        "(Press Space to start)");

    //TODO: read high score value from file.
    highScore = 0;

    char scoreLine[80];
    sprintf(scoreLine, "High Score: %u", highScore);
    gpanel_text(&font_lucidasans28, GREEN, BLUE, 10, 60, scoreLine);

    // Draw Ground
    int tx, ty = 230;
    for (tx = 0; tx <= 300; tx += 20) {
        gpanel_fill_triangle(GREEN,  tx,    ty,   tx+9,  ty, tx,    ty+9);
        gpanel_fill_triangle(YELLOW, tx+9,  ty+9, tx+9,  ty, tx,    ty+9);
        gpanel_fill_triangle(YELLOW, tx+10, ty,   tx+19, ty, tx+10, ty+9);
        gpanel_fill_triangle(GREEN,  tx+19, ty+9, tx+19, ty, tx+10, ty+9);
    }

    nextDrawLoopRunTime = millis() + DRAW_LOOP_INTERVAL;
}

void drawLoop()
{
    // clear moving items
    clearPillar(pillarPos, gapPos);
    clearFlappy(fx, fy);

    // move items
    if (running) {
        fy += fallRate;
        fallRate++;

        pillarPos -= 5;
        if (pillarPos == 0) {
            score++;
        }
        else if (pillarPos < -50) {
            pillarPos = 320;
            gapPos = 20 + random() % 100;
        }
    }

    // draw moving items & animate
    drawPillar(pillarPos, gapPos);
    drawFlappy(fx, fy);
    switch (wing) {
    case 0:
    case 1:
        drawWing1(fx, fy);
        break;
    case 2:
    case 3:
        drawWing2(fx, fy);
        break;
    case 4:
    case 5:
        drawWing3(fx, fy);
        break;
    }
    wing++;
    if (wing == 6)
        wing = 0;
}

void checkCollision()
{
    // Collision with ground
    if (fy > 206)
        crashed = 1;

    // Collision with pillar
    if (fx + 34 > pillarPos && fx < pillarPos + 50)
        if (fy < gapPos || fy + 24 > gapPos + 90)
             crashed = 1;

    if (crashed) {
        gpanel_text(&font_lucidasans28, RED, BLUE, 50, 50, "Game Over!");

        char scoreLine[80];
        sprintf(scoreLine, "Score: %u", score);
        gpanel_text(&font_lucidasans28, RED, BLUE, 50, 100, scoreLine);

        if (score > highScore) {
            highScore = score;
            gpanel_text(&font_lucidasans28, RED, BLUE, 50, 150, "NEW HIGH!");

            //TODO: Write high score value to file.
        }

        // stop animation
        running = 0;

        // delay to stop any last minute clicks from restarting immediately
        sleep(1);
    }
}

#if 1
struct sgttyb origtty, newtty;

/*
 * Terminate the game when ^C pressed.
 */
void quit(int sig)
{
    signal(SIGINT, SIG_IGN);
    if (newtty.sg_flags != 0)
        ioctl(0, TIOCSETP, &origtty);
    exit(0);
}

/*
 * Return 1 when any key is pressed on console.
 */
int get_input()
{
    if (newtty.sg_flags == 0) {
        ioctl(0, TIOCGETP, &origtty);

        newtty = origtty;
        newtty.sg_flags &= ~(ECHO|CRMOD|XTABS);
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
}
#endif

int main()
{
    char *devname = "/dev/tft0";
    int xsize = 320, ysize = 240;

    if (gpanel_open(devname) < 0) {
        printf("Cannot open %s\n", devname);
        exit(-1);
    }
    gpanel_clear(BLUE, &xsize, &ysize);
    startGame();

    signal(SIGINT, quit);

    for (;;) {
        if (millis() > nextDrawLoopRunTime && !crashed) {
            drawLoop();
            checkCollision();
            nextDrawLoopRunTime += DRAW_LOOP_INTERVAL;
        }
        usleep(10000);

        // Get user input.
        int user_input = get_input();

        // Process "user input"
        if (user_input > 0 && !scrPress) {
            if (crashed) {
                // restart game
                startGame();
            }
            else if (!running) {
                // clear text & start scrolling
                gpanel_fill(BLUE, 0, 0, 320-1, 100);
                gpanel_fill(BLUE, 0, 180, 320-1, 205);
                running = 1;
            } else {
                // fly up
                fallRate = -8;
                scrPress = 1;
            }
        } else if (user_input == 0 && scrPress) {
            // Attempt to throttle presses
            scrPress = 0;
        }
    }
}
