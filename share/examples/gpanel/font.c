/*
 * Draw samples of various fonts.
 */
#include <stdio.h>
#include <sys/gpanel.h>

/*
 * Data from external font files.
 */
extern const struct gpanel_font_t font_lucidasans15;
extern const struct gpanel_font_t font_lucidasans11;
extern const struct gpanel_font_t font_lucidasans9;
extern const struct gpanel_font_t font_lucidasans7;
extern const struct gpanel_font_t font_verdana7;
extern const struct gpanel_font_t font_6x9;
extern const struct gpanel_font_t font_5x7;
extern const struct gpanel_font_t font_digits32;
extern const struct gpanel_font_t font_digits20;

/*
 * Color constants.
 */
#define COLOR_RGB(r,g,b)    ((r)<<11 | (g)<<5 | (b))
#define COLOR_BLACK         0
#define COLOR_WHITE         COLOR_RGB(31, 63, 31)
#define COLOR_YELLOW        COLOR_RGB(31, 63, 0)
#define COLOR_MAGENTA       COLOR_RGB(31, 0,  31)
#define COLOR_CYAN          COLOR_RGB(0,  63, 31)
#define COLOR_RED           COLOR_RGB(31, 0,  0)
#define COLOR_GREEN         COLOR_RGB(0,  63, 0)
#define COLOR_BLUE          COLOR_RGB(0,  0,  31)

/*
 * Screen size.
 */
int xsize, ysize;

void show(const struct gpanel_font_t *font, const char *title, int digits_only)
{
    char line[100];
    int x = 0, y = 0, i, color;
    const char *phrase = digits_only ? "0123456789" :
                         "The quick brown fox jumps over the lazy dog.";
    static const int colortab[] = {
        COLOR_YELLOW, COLOR_CYAN,  COLOR_MAGENTA,
        COLOR_RED,    COLOR_GREEN, COLOR_BLUE,
        0,
    };

    gpanel_clear(COLOR_BLACK, 0, 0);
    gpanel_text(&font_lucidasans15, COLOR_WHITE, COLOR_BLACK, x, y, title);
    y += font_lucidasans15.height * 2;

    for (i=0; y<ysize; i++) {
        color = colortab[i];
        if (color == 0)
            color = colortab[i = 0];

        gpanel_text(font, color, COLOR_BLACK, x, y, phrase);
        y += font->height;
    }

    printf("Font %s: press Enter...", title);
    fflush(stdout);
    fgets(line, sizeof(line), stdin);
}

int main()
{
    char *devname = "/dev/tft0";
    int x, y, color;

    if (gpanel_open(devname) < 0) {
        printf("Cannot open %s\n", devname);
        exit(-1);
    }
    gpanel_clear(0, &xsize, &ysize);
    printf("Screen size %u x %u.\n", xsize, ysize);
    printf("Draw fonts.\n");
    printf("Press ^C to stop.\n");

    for (;;) {
        show(&font_lucidasans15, "Lucida Sans 15", 0);
        show(&font_lucidasans11, "Lucida Sans 11", 0);
        show(&font_lucidasans9, "Lucida Sans 9", 0);
        show(&font_lucidasans7, "Lucida Sans 7", 0);
        show(&font_verdana7, "Verdana 7", 0);
        show(&font_6x9, "Fixed 6x9", 0);
        show(&font_5x7, "Fixed 5x7", 0);
        show(&font_digits32, "Digits 32", 1);
        show(&font_digits20, "Digits 20", 1);
    }
    return 0;
}
