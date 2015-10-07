/*
Arduino/RetroBSD Tetris
Copyright (C) 2015  João André Esteves Vilaça

https://github.com/vilaca/Handheld-Color-Console

Adapted for RetroBSD by Alexey Frunze.
Required Hardware:
- PICadillo-35T board
- Funduino joystick shield v1.a

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

volatile int quit = 0;
volatile int jx = 0, jy = 0;

#include <sys/ioctl.h>
#include <sys/gpio.h>
#include <sys/time.h>
#include <sys/gpanel.h>

int fd = -1;
int fdx = -1;
int fdy = -1;

struct timeval start_time;

unsigned long millis(void)
{
  static int init;
  struct timeval t;
  unsigned long ms;
  if (!init)
  {
    gettimeofday(&start_time, 0);
    init = 1;
    return 0;
  }
  gettimeofday(&t, 0);
  ms = (t.tv_sec - start_time.tv_sec) * 1000UL;
  ms += t.tv_usec / 1000;
  ms -= start_time.tv_usec / 1000;
  return ms;
}

void delay(unsigned long ms)
{
  usleep(ms * 1000);
}

void lineh(unsigned x, unsigned y,
           unsigned length, unsigned color)
{
  struct gpanel_line_t param;

  param.color = color;
  param.x0 = x;
  param.y0 = y;
  param.x1 = x + length - 1;
  param.y1 = y;
  ioctl(fd, GPANEL_LINE, &param);
}

void linev(unsigned x, unsigned y,
           unsigned length, unsigned color)
{
  struct gpanel_line_t param;

  param.color = color;
  param.x0 = x;
  param.y0 = y;
  param.x1 = x;
  param.y1 = y + length - 1;
  ioctl(fd, GPANEL_LINE, &param);
}

void box(unsigned x, unsigned y,
         unsigned width, unsigned height,
         unsigned color, int solid)
{
  if (solid)
  {
    struct gpanel_rect_t param;

    param.color = color;
    param.x0 = x;
    param.y0 = y;
    param.x1 = x + width - 1;
    param.y1 = y + height - 1;
    ioctl(fd, GPANEL_FILL, &param);
  }
  else
  {
    lineh(x, y, width, color);
    lineh(x, y + height - 1, width, color);
    linev(x, y, height, color);
    linev(x + width - 1, y, height, color);
  }
}

void beep(int frq, int d)
{
  (void)frq; // TBD???
  delay(d);
}

void boxb(unsigned x, unsigned y, unsigned w, unsigned h, unsigned short c)
{
  box(x, y, w, h - 1, c, 1);
  box(x, y + h - 1, 1, 1, c, 1);
}

int Joystick_fire(void)
{
  return 0; // TBD???
}

int Joystick_getX(void)
{
  char buf[21] = { 0 };
  jx = 512;
  if (read(fdx, buf, sizeof buf - 1) > 0)
    jx = strtol(buf, 0, 0);
  jx -= 512;
  jx = -jx;
  if (-128 < jx && jx < 128)
    jx = 0;
  return jx;
}

int Joystick_getY(void)
{
  char buf[21] = { 0 };
  jy = 512;
  if (read(fdy, buf, sizeof buf - 1) > 0)
    jy = strtol(buf, 0, 0);
  jy -= 512;
  jy = -jy;
  if (-128 < jy && jy < 128)
    jy = 0;
  return jy;
}

void Joystick_waitForRelease(int howLong)
{
  int c = 0;
  do
  {
    delay(10);
    c += 10;
  }
  while ((Joystick_fire() || Joystick_getY() || Joystick_getX()) && c < howLong);
}

void randomizer(void);
void score(void);
void scoreBoard(void);
void draw(void);
void moveDown(void);
void userInput(unsigned long now);
void chooseNewShape(void);
int touches(int xi, int yi, int roti);

//TFT resolution 480x320
#define LCD_WIDTH 480//320//480
#define LCD_HEIGHT 320//200//320

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

#define FONT_SPACE 6
#define FONT_X 8
extern unsigned char simpleFont[][8];

enum
{
  BOARD_WIDTH  = 11,
  BOARD_HEIGHT = 20,

  BLOCK_SIZE   = MIN(LCD_WIDTH / BOARD_WIDTH - 1, LCD_HEIGHT / BOARD_HEIGHT - 1),

  BOARD_LEFT   = (LCD_WIDTH - BOARD_WIDTH * BLOCK_SIZE) / 4 * 3,
  BOARD_RIGHT  = BOARD_LEFT + BLOCK_SIZE * BOARD_WIDTH,
  BOARD_TOP    = (LCD_HEIGHT - BOARD_HEIGHT * BLOCK_SIZE) / 2,
  BOARD_BOTTOM = BOARD_TOP + BOARD_HEIGHT * BLOCK_SIZE
};

//Basic Colors
#define BLACK       0x0000
#define BLUE        0x001f
#define CYAN        0x07ff
#define DARKCYAN    0x03EF      /*   0, 128, 128 */
#define DARKGREEN   0x03E0
#define DARKGREY    0x7BEF      /* 128, 128, 128 */
#define GRAY1       0x8410
#define GRAY2       0x4208
#define GRAY3       0x2104
#define GREEN       0x07e0
#define LIGHTGREEN  0xAFE5      /* 173, 255,  47 */
#define LIGHTGREY   0xC618      /* 192, 192, 192 */
#define MAGENTA     0xF81F      /* 255,   0, 255 */
#define MAROON      0x7800      /* 128,   0,   0 */
#define NAVY        0x000F      /*   0,   0, 128 */
#define OLIVE       0x7BE0      /* 128, 128,   0 */
#define ORANGE      0xFD20      /* 255, 165,   0 */
#define PURPLE      0x780F      /* 128,   0, 128 */
#define RED         0xf800
#define WHITE       0xffff
#define YELLOW      0xffe0

#define PIT_COLOR   CYAN
#define BG_COLOR    BLACK

typedef unsigned char byte;

// used to clear the position from the screen
typedef struct
{
  byte x, y, rot;
} Backup;

#define DROP_WAIT_INIT  1100

#define INPUT_WAIT_ROT  200
#define INPUT_WAIT_MOVE 100

#define INPUT_WAIT_NEW_SHAPE 400

// shapes definitions

byte l_shape[4][4][2] =
{
  { {0, 0}, {0, 1}, {0, 2}, {1, 2} },
  { {0, 1}, {1, 1}, {2, 0}, {2, 1} },
  { {0, 0}, {1, 0}, {1, 1}, {1, 2} },
  { {0, 0}, {0, 1}, {1, 0}, {2, 0} },
};

byte j_shape[4][4][2] =
{
  { {1, 0}, {1, 1}, {0, 2}, {1, 2} },
  { {0, 0}, {1, 0}, {2, 0}, {2, 1} },
  { {0, 0}, {1, 0}, {0, 1}, {0, 2} },
  { {0, 0}, {0, 1}, {1, 1}, {2, 1} },
};

byte o_shape[1][4][2] =
{
  { {0, 0}, {0, 1}, {1, 0}, {1, 1} }
};

byte s_shape[2][4][2] =
{
  { {0, 1}, {1, 0}, {1, 1}, {2, 0} },
  { {0, 0}, {0, 1}, {1, 1}, {1, 2} }
};

byte z_shape[2][4][2] =
{
  { {0, 0}, {1, 0}, {1, 1}, {2, 1} },
  { {1, 0}, {0, 1}, {1, 1}, {0, 2} }
};

byte t_shape[4][4][2] =
{
  { {0, 0}, {1, 0}, {2, 0}, {1, 1} },
  { {0, 0}, {0, 1}, {1, 1}, {0, 2} },
  { {1, 0}, {0, 1}, {1, 1}, {2, 1} },
  { {1, 0}, {0, 1}, {1, 1}, {1, 2} }
};

byte i_shape[2][4][2] =
{
  { {0, 0}, {1, 0}, {2, 0}, {3, 0} },
  { {0, 0}, {0, 1}, {0, 2}, {0, 3} } // TBD??? rotate at the center
};

// All game shapes and their colors

byte* all_shapes[7] =
{
  l_shape[0][0],
  j_shape[0][0],
  o_shape[0][0],
  s_shape[0][0],
  z_shape[0][0],
  t_shape[0][0],
  i_shape[0][0]
};


unsigned short colors[7] = { ORANGE, BLUE, YELLOW, GREEN, RED, MAGENTA, CYAN };

// how many rotated variations each shape has
byte shapes[7] = { 4, 4, 1, 2, 2, 4, 2 };

// game progress
int lines, level;

// current shapes
byte current;

// tetris guidelines have all 7 shapes
// selected in sequence to avoid
// long runs without a shape
byte next[7];
byte next_c;

unsigned long lastInput, lastDrop;

byte board[BOARD_HEIGHT][BOARD_WIDTH];

byte x, y, rot;
Backup old;

int newShape;

unsigned long dropWait;

void drawChar(byte ascii, unsigned poX, unsigned poY, unsigned size, unsigned short fgcolor)
{
  if ((ascii < 32) || (ascii > 127))
    ascii = '?' - 32;

  for (int i = 0; i < FONT_X; i++)
  {
    byte temp = simpleFont[ascii - 32][i];
    for (byte f = 0; f < 8; f++)
      if ((temp >> f) & 1)
        box(poX + i * size, poY + f * size, size, size, fgcolor, 1);
  }
}

void drawString(char* string, unsigned poX, unsigned poY, unsigned size, unsigned short fgcolor)
{
  while (*string)
  {
    drawChar(*string++, poX, poY, size, fgcolor);
    poX += FONT_SPACE * size;
  }
}

void drawCenteredString(char* string, unsigned poY, unsigned size, unsigned short fgcolor)
{
  unsigned len = strlen(string) * FONT_SPACE * size;
  unsigned left = (LCD_WIDTH - len) / 2;
  drawString(string, left, poY, size, fgcolor);
}

void drawNumber(long long_num, unsigned poX, unsigned poY, unsigned size, unsigned short fgcolor)
{
  char buf[12], *p = buf + sizeof buf;
  int neg = long_num < 0;
  unsigned long n = neg ? -(unsigned long)long_num : (unsigned long)long_num;

  *--p = '\0';

  do
  {
    *--p = '0' + n % 10;
    n /= 10;
  } while (n);

  if (neg)
    *--p = '-';

  drawString(p, poX, poY, size, fgcolor);
}

void bcackground(void)
{
  // draw black-blue gradient background
  for (int i = 0; i < LCD_HEIGHT; i++)
  {
    int c = 31 - i * 31 / (LCD_HEIGHT - 1);
    if (i < BOARD_BOTTOM)
    {
      lineh(0, i, BOARD_LEFT - 1, c);
      lineh(BOARD_RIGHT, i, LCD_WIDTH - BOARD_RIGHT, c);
    }
    else
    {
      lineh(0, i, LCD_WIDTH, c);
    }
  }

  // draw the board left limit
  linev(BOARD_LEFT - 1, BOARD_TOP, BOARD_BOTTOM - BOARD_TOP, PIT_COLOR);

  // draw the board right limit
  linev(BOARD_RIGHT - 1, BOARD_TOP, BOARD_BOTTOM - BOARD_TOP, PIT_COLOR);

  // draw the board bottom limit
  lineh(BOARD_LEFT - 1, BOARD_BOTTOM - 1, BOARD_RIGHT - BOARD_LEFT + 1, PIT_COLOR);

  // draw the grid
  for (int i = BOARD_LEFT + BLOCK_SIZE - 1; i < BOARD_RIGHT - 1; i += BLOCK_SIZE)
    linev(i, BOARD_TOP, BOARD_BOTTOM - BOARD_TOP - 1, GRAY2);
  for (int i = BOARD_TOP + BLOCK_SIZE - 1; i < BOARD_BOTTOM - 1; i += BLOCK_SIZE)
    lineh(BOARD_LEFT, i, BOARD_RIGHT - BOARD_LEFT - 1, GRAY2);
}

void scoreBoard(void)
{
  box(6, 3, 128, 50, BLACK, 1);
  drawString("Level", 8, 8, 2, YELLOW);
  drawString("Lines", 8, 32, 2, 0x3f);
  drawNumber(level, 74, 8, 2, YELLOW);
  drawNumber(lines, 74, 32, 2, 0x3f);
  box(5, 2, 130, 52, WHITE, 0);
}

void hint(void)
{
  // draw next shape hint box
  box(30, 100, BLOCK_SIZE * 6, BLOCK_SIZE * 5, BLACK, 1);
  box(29, 99, BLOCK_SIZE * 6 + 1, BLOCK_SIZE * 5 + 1, WHITE, 0);

  byte* shape = all_shapes[next[next_c]];
  for (int i = 0; i < 4; i++)
  {
    byte* block = shape + i * 2;
    boxb(30 + BLOCK_SIZE + block[0] * BLOCK_SIZE,
         100 + BLOCK_SIZE + block[1] * BLOCK_SIZE,
         BLOCK_SIZE - 1,
         BLOCK_SIZE - 1,
         colors[next[next_c]]);
  }
}

void gameLoop(void)
{
  box(0, 0, LCD_WIDTH, LCD_HEIGHT, BG_COLOR, 1);

  // initialize game logic
  newShape = 1;
  lines = 0;
  lastInput = 0;
  lastDrop = 0;
  dropWait = DROP_WAIT_INIT;
  level = 1;

  // clean board
  for (int i = 0; i < BOARD_WIDTH; i++)
    for (int j = 0; j < BOARD_HEIGHT; j++)
      board[j][i] = 0;

  // next shape
  randomizer();

  bcackground();

  scoreBoard();

  do
  {
    // get clock
    unsigned long now = millis();

    // display new shape
    if (newShape)
    {
      Joystick_waitForRelease(INPUT_WAIT_NEW_SHAPE);
      newShape = 0;

      // a new shape enters the game
      chooseNewShape();

      // draw next shape hint box
      hint();

      // check if new shape is placed over other shape(s)
      // on the board
      if (touches(0, 0, 0))
      {
        // draw shape to screen
        draw();
        // game over
        return;
      }

      // draw shape to screen
      draw();
    }
    else
    {
      // check if enough time has passed since last time the shape
      // was moved down the board
      if (now - lastDrop > dropWait || Joystick_getY() > 0)
      {
        // update clock
        lastDrop = now;

        moveDown();
      }
    }
    if (!newShape && now - lastInput > INPUT_WAIT_MOVE)
    {
      userInput(now);
    }
  } while (!quit);
}

void chooseNewShape(void)
{
  current = next[next_c];

  next_c++;

  if (next_c == 7)
    randomizer();

  // new shape must be postioned at the middle of
  // the top of the board
  // with zero rotation
  rot = 0;
  y = 0;
  x = BOARD_WIDTH / 2 - 1;

  old.rot = rot;
  old.y = y;
  old.x = x;
}

void userInput(unsigned long now)
{
  int jx = Joystick_getX();
  if (jx < 0 && x > 0 && !touches(-1, 0, 0))
  {
    x--;
  }
  else if (jx > 0 && x < BOARD_WIDTH && !touches(1, 0, 0))
  {
    x++;
  }
  else if (Joystick_fire())
  {
    while (!touches(0, 1, 0))
      y++;
  }
  else if (now - lastInput > INPUT_WAIT_ROT)
  {
    if (Joystick_getY() < 0 && !touches(0, 0, 1))
    {
      rot++;
      rot %= shapes[current];
    }
  }
  else
  {
    return;
  }
  lastInput = now;
  draw();
}

void moveDown(void)
{
  // prepare to move down
  // check if board is clear bellow
  if (touches(0, 1, 0))
  {
    // moving down touches another shape
    newShape = 1;

    // this shape wont move again
    // add it to the board
    byte* shape = all_shapes[current];
    for (int i = 0; i < 4; i++)
    {
      byte* block = shape + (rot * 4 + i) * 2;
      board[block[1] + y][block[0] + x] = current + 1;
    }

    // check if lines were made
    score();
    beep(1500, 25);
  }
  else
  {
    // move shape down
    y++;
    draw();
  }
}

void draw(void)
{
  byte* shape = all_shapes[current];
  for (int i = 0; i < 4; i++)
  {
    byte* block = shape + (rot * 4 + i) * 2;
    boxb(BOARD_LEFT + block[0] * BLOCK_SIZE + BLOCK_SIZE * x,
         BOARD_TOP + block[1] * BLOCK_SIZE + BLOCK_SIZE * y,
         BLOCK_SIZE - 1,
         BLOCK_SIZE - 1,
         colors[current]);
    board[block[1] + y][block[0] + x] = 255;
  }

  // erase old
  for (int i = 0; i < 4; i++)
  {
    byte* block = shape + (old.rot * 4 + i) * 2;

    if (board[block[1] + old.y][block[0] + old.x] == 255)
      continue;

    boxb(BOARD_LEFT + block[0] * BLOCK_SIZE + BLOCK_SIZE * old.x,
         BOARD_TOP + block[1] * BLOCK_SIZE + BLOCK_SIZE * old.y,
         BLOCK_SIZE - 1,
         BLOCK_SIZE - 1,
         BG_COLOR);
  }

  for (int i = 0; i < 4; i++)
  {
    byte* block = shape + (rot * 4 + i) * 2;
    board[block[1] + y][block[0] + x] = 0;
  }

  old.x = x;
  old.y = y;
  old.rot = rot;
}

int touches(int xi, int yi, int roti)
{
  byte* shape = all_shapes[current];
  for (int i = 0; i < 4; i++)
  {
    byte* block = shape + (((rot + roti) % shapes[current]) * 4 + i) * 2;

    int x2 = x + block[0] + xi;
    int y2 = y + block[1] + yi;

    if (y2 == BOARD_HEIGHT || x2 == BOARD_WIDTH || board[y2][x2])
      return 1;
  }
  return 0;
}

void score(void)
{
  // we scan a max of 4 lines
  int ll = y + 3; // BUG!!! was uninitialized
  if (y + 3 >= BOARD_HEIGHT)
    ll = BOARD_HEIGHT - 1;

  // scan board from current position
  for (int l = y; l <= ll; l++)
  {
    // check if there's a complete line on the board
    int line = 1;
    for (int c = 0; c < BOARD_WIDTH; c++)
    {
      if (board[l][c] == 0)
      {
        line = 0;
        break;
      }
    }

    if (!line)
    {
      // move to next line
      continue;
    }

    beep(3000, 50);

    lines++;

    if (lines % 10 == 0)
    {
      level++;
      dropWait /= 2;
    }

    scoreBoard();

    // move board down
    for (int row = l; row > 0; row --)
    {
      for (int c = 0; c < BOARD_WIDTH; c++)
      {
        byte v = board[row - 1][c];

        board[row][c] = v;
        boxb(BOARD_LEFT + BLOCK_SIZE * c,
             BOARD_TOP + BLOCK_SIZE * row,
             BLOCK_SIZE - 1,
             BLOCK_SIZE - 1,
             v == 0 ? BLACK : colors[v - 1]);
      }
    }

    // clear top line
    for (int c = 0; c < BOARD_WIDTH; c++)
      board[0][c] = 0;

    box(BOARD_LEFT,
        0,
        BOARD_RIGHT - BOARD_LEFT,
        BLOCK_SIZE,
        BLACK,
        1);
  }

  delay(350);
}

// create a sequence of 7 random shapes
void randomizer(void)
{
  // randomize 7 shapes

  for (byte i = 0; i < 7; i ++)
  {
    int retry;
    byte shape;
    do
    {
      shape = rand() % 7;

      // check if already in sequence

      retry = 0;
      for (int j = 0; j < i; j++)
      {
        if (shape == next[j])
        {
          retry = 1;
          break;
        }
      }

    } while (retry);
    next[i] = shape;
  }
  next_c = 0;
}

void drawPreGameScreen(void)
{
  box(0, 0, LCD_WIDTH, LCD_HEIGHT, WHITE, 1);
  drawCenteredString("Tetris", 40, 8, BLUE);
  drawCenteredString("Move joystick to start", 110, 2, BLACK);
  drawCenteredString("http://vilaca.eu", 220, 2, PURPLE);
}

void gameOver(void)
{
  box(32, LCD_HEIGHT / 2 - 24, LCD_WIDTH - 64, 48, BLACK, 1);
  drawCenteredString("Game Over", LCD_HEIGHT / 2 - 16, 4, WHITE);
  box(32, LCD_HEIGHT / 2 - 24, LCD_WIDTH - 64, 48, RED, 0);

  beep(600, 200);
  delay(300);
  beep(600, 200);
  delay(300);
  beep(200, 600);
  delay(1500);
}

unsigned char simpleFont[][8] =
{
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x5F,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x07,0x00,0x07,0x00,0x00,0x00},
  {0x00,0x14,0x7F,0x14,0x7F,0x14,0x00,0x00},
  {0x00,0x24,0x2A,0x7F,0x2A,0x12,0x00,0x00},
  {0x00,0x23,0x13,0x08,0x64,0x62,0x00,0x00},
  {0x00,0x36,0x49,0x55,0x22,0x50,0x00,0x00},
  {0x00,0x00,0x05,0x03,0x00,0x00,0x00,0x00},
  {0x00,0x1C,0x22,0x41,0x00,0x00,0x00,0x00},
  {0x00,0x41,0x22,0x1C,0x00,0x00,0x00,0x00},
  {0x00,0x08,0x2A,0x1C,0x2A,0x08,0x00,0x00},
  {0x00,0x08,0x08,0x3E,0x08,0x08,0x00,0x00},
  {0x00,0xA0,0x60,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x08,0x08,0x08,0x08,0x08,0x00,0x00},
  {0x00,0x60,0x60,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x20,0x10,0x08,0x04,0x02,0x00,0x00},
  {0x00,0x3E,0x51,0x49,0x45,0x3E,0x00,0x00},
  {0x00,0x00,0x42,0x7F,0x40,0x00,0x00,0x00},
  {0x00,0x62,0x51,0x49,0x49,0x46,0x00,0x00},
  {0x00,0x22,0x41,0x49,0x49,0x36,0x00,0x00},
  {0x00,0x18,0x14,0x12,0x7F,0x10,0x00,0x00},
  {0x00,0x27,0x45,0x45,0x45,0x39,0x00,0x00},
  {0x00,0x3C,0x4A,0x49,0x49,0x30,0x00,0x00},
  {0x00,0x01,0x71,0x09,0x05,0x03,0x00,0x00},
  {0x00,0x36,0x49,0x49,0x49,0x36,0x00,0x00},
  {0x00,0x06,0x49,0x49,0x29,0x1E,0x00,0x00},
  {0x00,0x00,0x36,0x36,0x00,0x00,0x00,0x00},
  {0x00,0x00,0xAC,0x6C,0x00,0x00,0x00,0x00},
  {0x00,0x08,0x14,0x22,0x41,0x00,0x00,0x00},
  {0x00,0x14,0x14,0x14,0x14,0x14,0x00,0x00},
  {0x00,0x41,0x22,0x14,0x08,0x00,0x00,0x00},
  {0x00,0x02,0x01,0x51,0x09,0x06,0x00,0x00},
  {0x00,0x32,0x49,0x79,0x41,0x3E,0x00,0x00},
  {0x00,0x7E,0x09,0x09,0x09,0x7E,0x00,0x00},
  {0x00,0x7F,0x49,0x49,0x49,0x36,0x00,0x00},
  {0x00,0x3E,0x41,0x41,0x41,0x22,0x00,0x00},
  {0x00,0x7F,0x41,0x41,0x22,0x1C,0x00,0x00},
  {0x00,0x7F,0x49,0x49,0x49,0x41,0x00,0x00},
  {0x00,0x7F,0x09,0x09,0x09,0x01,0x00,0x00},
  {0x00,0x3E,0x41,0x41,0x51,0x72,0x00,0x00},
  {0x00,0x7F,0x08,0x08,0x08,0x7F,0x00,0x00},
  {0x00,0x41,0x7F,0x41,0x00,0x00,0x00,0x00},
  {0x00,0x20,0x40,0x41,0x3F,0x01,0x00,0x00},
  {0x00,0x7F,0x08,0x14,0x22,0x41,0x00,0x00},
  {0x00,0x7F,0x40,0x40,0x40,0x40,0x00,0x00},
  {0x00,0x7F,0x02,0x0C,0x02,0x7F,0x00,0x00},
  {0x00,0x7F,0x04,0x08,0x10,0x7F,0x00,0x00},
  {0x00,0x3E,0x41,0x41,0x41,0x3E,0x00,0x00},
  {0x00,0x7F,0x09,0x09,0x09,0x06,0x00,0x00},
  {0x00,0x3E,0x41,0x51,0x21,0x5E,0x00,0x00},
  {0x00,0x7F,0x09,0x19,0x29,0x46,0x00,0x00},
  {0x00,0x26,0x49,0x49,0x49,0x32,0x00,0x00},
  {0x00,0x01,0x01,0x7F,0x01,0x01,0x00,0x00},
  {0x00,0x3F,0x40,0x40,0x40,0x3F,0x00,0x00},
  {0x00,0x1F,0x20,0x40,0x20,0x1F,0x00,0x00},
  {0x00,0x3F,0x40,0x38,0x40,0x3F,0x00,0x00},
  {0x00,0x63,0x14,0x08,0x14,0x63,0x00,0x00},
  {0x00,0x03,0x04,0x78,0x04,0x03,0x00,0x00},
  {0x00,0x61,0x51,0x49,0x45,0x43,0x00,0x00},
  {0x00,0x7F,0x41,0x41,0x00,0x00,0x00,0x00},
  {0x00,0x02,0x04,0x08,0x10,0x20,0x00,0x00},
  {0x00,0x41,0x41,0x7F,0x00,0x00,0x00,0x00},
  {0x00,0x04,0x02,0x01,0x02,0x04,0x00,0x00},
  {0x00,0x80,0x80,0x80,0x80,0x80,0x00,0x00},
  {0x00,0x01,0x02,0x04,0x00,0x00,0x00,0x00},
  {0x00,0x20,0x54,0x54,0x54,0x78,0x00,0x00},
  {0x00,0x7F,0x48,0x44,0x44,0x38,0x00,0x00},
  {0x00,0x38,0x44,0x44,0x28,0x00,0x00,0x00},
  {0x00,0x38,0x44,0x44,0x48,0x7F,0x00,0x00},
  {0x00,0x38,0x54,0x54,0x54,0x18,0x00,0x00},
  {0x00,0x08,0x7E,0x09,0x02,0x00,0x00,0x00},
  {0x00,0x18,0xA4,0xA4,0xA4,0x7C,0x00,0x00},
  {0x00,0x7F,0x08,0x04,0x04,0x78,0x00,0x00},
  {0x00,0x00,0x7D,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x80,0x84,0x7D,0x00,0x00,0x00,0x00},
  {0x00,0x7F,0x10,0x28,0x44,0x00,0x00,0x00},
  {0x00,0x41,0x7F,0x40,0x00,0x00,0x00,0x00},
  {0x00,0x7C,0x04,0x18,0x04,0x78,0x00,0x00},
  {0x00,0x7C,0x08,0x04,0x7C,0x00,0x00,0x00},
  {0x00,0x38,0x44,0x44,0x38,0x00,0x00,0x00},
  {0x00,0xFC,0x24,0x24,0x18,0x00,0x00,0x00},
  {0x00,0x18,0x24,0x24,0xFC,0x00,0x00,0x00},
  {0x00,0x00,0x7C,0x08,0x04,0x00,0x00,0x00},
  {0x00,0x48,0x54,0x54,0x24,0x00,0x00,0x00},
  {0x00,0x04,0x7F,0x44,0x00,0x00,0x00,0x00},
  {0x00,0x3C,0x40,0x40,0x7C,0x00,0x00,0x00},
  {0x00,0x1C,0x20,0x40,0x20,0x1C,0x00,0x00},
  {0x00,0x3C,0x40,0x30,0x40,0x3C,0x00,0x00},
  {0x00,0x44,0x28,0x10,0x28,0x44,0x00,0x00},
  {0x00,0x1C,0xA0,0xA0,0x7C,0x00,0x00,0x00},
  {0x00,0x44,0x64,0x54,0x4C,0x44,0x00,0x00},
  {0x00,0x08,0x36,0x41,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x7F,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x41,0x36,0x08,0x00,0x00,0x00,0x00},
  {0x00,0x02,0x01,0x01,0x02,0x01,0x00,0x00},
  {0x00,0x02,0x05,0x05,0x02,0x00,0x00,0x00}
};

int main(void)
{
  // get TFT display handle
  if ((fd = open("/dev/tft0", O_RDWR)) < 0)
  {
    perror("/dev/tft0");
    return 1;
  }

  // get joystick's x ADC handle
  if ((fdx = open("/dev/adc0", O_RDWR)) < 0)
  {
    perror("/dev/adc0");
    return 1;
  }

  // get joystick's y ADC handle
  if ((fdy = open("/dev/adc1", O_RDWR)) < 0)
  {
    perror("/dev/adc1");
    return 1;
  }

  srand(time(NULL));

  drawPreGameScreen();
  while (!Joystick_fire() && !Joystick_getY() && !Joystick_getX());

  gameLoop();
  gameOver();

  while (!Joystick_fire() && !Joystick_getY() && !Joystick_getX());
  box(0, 0, LCD_WIDTH, LCD_HEIGHT, BLACK, 1);
}
