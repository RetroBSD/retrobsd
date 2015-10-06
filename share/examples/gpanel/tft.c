#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/gpio.h>
#include <time.h>

#define BLACK 0x0000
#define RED   0xF800
#define GREEN 0x07E0
#define BLUE  0x001F
#define WHITE 0xFFFF

int fd = -1;

//#define PIX_BUF_SIZ 64 // 21 fps
#define PIX_BUF_SIZ 128 // 28 fps on full-screen continuous box()
//#define PIX_BUF_SIZ 256 // 32 fps
//#define PIX_BUF_SIZ 512 // 34 fps

void pixel(unsigned x, unsigned y,
           unsigned short color)
{
  unsigned short wincoo[4];
  unsigned short data[2];

  wincoo[0] = wincoo[2] = x;
  wincoo[1] = wincoo[3] = y;
  ioctl(fd, 0xC0C0, wincoo);

  *data = 1;
  data[1] = color;
  ioctl(fd, 0xDADA, data);
}

void fill(unsigned count, unsigned short color)
{
  unsigned short data[1 + PIX_BUF_SIZ];

  unsigned cnt = (count < PIX_BUF_SIZ) ? count : PIX_BUF_SIZ;
  while (cnt)
    data[cnt--] = color;

  while (count)
  {
    cnt = (count < PIX_BUF_SIZ) ? count : PIX_BUF_SIZ;
    *data = cnt;
    ioctl(fd, 0xDADA, data);
    count -= cnt;
  }
}

void lineh(unsigned x, unsigned y,
           unsigned length, unsigned short color)
{
  unsigned short wincoo[4];

  wincoo[0] = x;
  wincoo[1] = wincoo[3] = y;
  wincoo[2] = x + length - 1;
  ioctl(fd, 0xC0C0, wincoo);

  fill(length, color);
}

void linev(unsigned x, unsigned y,
           unsigned length, unsigned short color)
{
  unsigned short wincoo[4];

  wincoo[0] = wincoo[2] = x;
  wincoo[1] = y;
  wincoo[3] = y + length - 1;
  ioctl(fd, 0xC0C0, wincoo);

  fill(length, color);
}

void box(unsigned x, unsigned y,
         unsigned width, unsigned height,
         unsigned short color,
         int solid)
{
  if (solid)
  {
    unsigned short wincoo[4];

    wincoo[0] = x; wincoo[1] = y;
    wincoo[2] = x + width - 1; wincoo[3] = y + height - 1;
    ioctl(fd, 0xC0C0, wincoo);

    fill(width * height, color);
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
