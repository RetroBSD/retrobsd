/******************************************************
 * Convert retroImage to retroImage16 and retroImage64
 * and create big endian versions of images.
 ******************************************************/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

int32_t input[1000000];
int16_t output16[1000000];
int64_t output64[1000000];
int16_t output16BE[1000000];
int32_t output32BE[1000000];
int64_t output64BE[1000000];

#ifdef RXBE
uint32_t bitswap32(uint32_t x)
{
  return ((x << 24) & 0xff000000) |
         ((x <<  8) & 0x00ff0000) |
         ((x <<  8) & 0x0000ff00) |
         ((x << 24) & 0x000000ff);
}
#endif

int load_image(char *image)
{
  FILE *fp;
  int x;

  if ((fp = fopen(image, "rb")) == NULL)
  {
    fprintf(stderr, "Sorry, but I couldn't open %s\n", image);
    exit(-1);
  }

  x = fread(input, sizeof(int32_t), 1000000, fp);
  fclose(fp);

  return x;
}

int save_image()
{
  FILE *fp[5];
  int x[5], i;
  u_int32_t image_size = input[3];

#ifdef RXBE
  image_size = bitswap32(image_size);
#endif

  if ((fp[0] = fopen("retroImage16", "w")) == NULL) exit(-1);
  if ((fp[1] = fopen("retroImage64", "w")) == NULL) exit(-1);
  if ((fp[2] = fopen("retroImage16BE", "w")) == NULL) exit(-1);
  if ((fp[3] = fopen("retroImageBE", "w")) == NULL) exit(-1);
  if ((fp[4] = fopen("retroImage64BE", "w")) == NULL) exit(-1);

  x[0] = fwrite(output16,   sizeof(int16_t), image_size, fp[0]);
  x[1] = fwrite(output64,   sizeof(int64_t), image_size, fp[1]);
  x[2] = fwrite(output16BE, sizeof(int16_t), image_size, fp[2]);
  x[3] = fwrite(output32BE, sizeof(int32_t), image_size, fp[3]);
  x[4] = fwrite(output64BE, sizeof(int64_t), image_size, fp[4]);

  for (i = 0; i < 5; ++i)
      fclose(fp[i]);
  for (i = 0; i < 5; ++i)
    if (x[i] != image_size) {
        fprintf(stderr, "Some images could not be written properly (%d-%d:%d).\n", i, x[i], image_size);
        exit(-1);
    }

  return image_size;
}

void convert()
{
  int i, cells;
  int32_t be;
  fprintf(stderr, "Loading...\n");
  cells = load_image("retroImage");

  fprintf(stderr, "Converting...\n");
  for (i = 0; i < cells; i++)
  {
    be = bswap32(input[i]);
    output16[i] = (int16_t)input[i];
    output64[i] = (int64_t)input[i];
    output16BE[i] = (int16_t)be;
    output32BE[i] = be;
    output64BE[i] = (int64_t)be;
  }

  fprintf(stderr, "Saving...\n\n");
  save_image();
}

int main(int argc, char **argv)
{
  convert();
  return 0;
}
