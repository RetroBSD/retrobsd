/*
 * Interface to LED cube 8x8x8.
 */
#define CUBE_SIZE 8

void gpio_init(void);
void gpio_ext(int on);
void gpio_oe(int active);
void gpio_le(int active);
void gpio_backlight_upper(int on);
void gpio_backlight_lower(int on);
void gpio_layer(int z);
void gpio_plane(unsigned char *data);
