#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/gpio.h>
#include <sys/gpanel.h>
#include <time.h>

#define BLACK 0x0000
#define RED   0xF800
#define GREEN 0x07E0
#define BLUE  0x001F
#define WHITE 0xFFFF

int fd = -1;

void pixel(unsigned x, unsigned y, unsigned color)
{
  struct gpanel_pixel_t param;

  param.color = color;
  param.x = x;
  param.y = y;
  ioctl(fd, GPANEL_PIXEL, &param);
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

void line(int x1, int y1,
          int x2, int y2,
          unsigned short color)
{
  int sx, sy, dx1, dy1, dx2, dy2, x, y, m, n, k, cnt;

  sx = x2 - x1;
  sy = y2 - y1;

  if (sy < 0 || sy == 0 && sx < 0)
  {
    k = x1; x1 = x2; x2 = k;
    k = y1; y1 = y2; y2 = k;
    sx = -sx;
    sy = -sy;
  }

  if (sx > 0) dx1 = 1;
  else if (sx < 0) dx1 = -1;
  else dy1 = 0;

  if (sy > 0) dy1 = 1;
  else if (sy < 0) dy1 = -1;
  else dy1 = 0;

  m = (sx >= 0) ? sx : -sx;
  n = (sy >= 0) ? sy : -sy;
  dx2 = dx1;
  dy2 = 0;

  if (m < n)
  {
    m = (sy >= 0) ? sy : -sy;
    n = (sx >= 0) ? sx : -sx;
    dx2 = 0;
    dy2 = dy1;
  }

  x = x1; y = y1;
  cnt = m + 1;
  k = n / 2;

  while (cnt--)
  {
    if (x >= 0 && x < 480 && y >= 0 && y < 320)
      pixel(x, y, color);

    k += n;
    if (k < m)
    {
      x += dx2;
      y += dy2;
    }
    else
    {
      k -= m;
      x += dx1;
      y += dy1;
    }
  }
}

int main(int argc, char* argv[])
{
  if ((fd = open ("/dev/tft0", O_RDWR)) < 0)
  {
    perror("/dev/tft0");
    return 1;
  }

  if (argc > 1)
  {
    if (!strcmp(argv[1], "-blank"))
    {
      box(0, 0, 480, 320, BLACK, 1);
      return 0;
    }
  }

  for (int y = 0; y < 320; y++)
    for (int x = 0; x < 480; x++)
      pixel(x, y, (x<<3) ^ (y<<3));

  sleep(2);

  box(0, 0*100, 480, 100, RED, 1);
  box(0, 1*100, 480, 100, GREEN, 1);
  box(0, 2*100, 480, 100, BLUE, 1);

  box(0, 0, 480, 320, WHITE, 0);

  line(0, 0, 479, 319, RED | GREEN);
  line(479, 0, 0, 319, GREEN | BLUE);

  sleep(2);

//  for (;;) line(rand() & 511, rand() & 511, rand() & 511, rand() & 511, rand());

#if 01
  time_t t1 = time(NULL), t2;
  int fps = 0;
  do
  {
    t2 = time(NULL);
    box(0, 0, 480, 320, rand(), 1);
    fps++;
  } while (t2 < t1 + 10);

  printf("%d fps\n", fps / 10);
#endif

  sleep(2);

  box(0, 0, 480, 320, BLACK, 1);

  return 0;
}
