/*
 * Example for RetroBSD on Olimex Duinomite board.
 * RGB LED is connected to D7, D6 and D5 pins of Duinomite board.
 * Use ioctl() calls to control LED.
 */
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/gpio.h>

/*
 * Pins D5, D6 and D7 are connected to signals RE5, RE6 and RE7 of pic32 chip.
 */
#define MASK_D5     (1 << 5)    /* signal RE5 */
#define MASK_D6     (1 << 6)    /* signal RE6 */
#define MASK_D7     (1 << 7)    /* signal RE7 */

int main()
{
    int fd;
    char *devname = "/dev/porta";

    /* Open GPIO driver. */
    fd = open(devname, 1);
    if (fd < 0) {
        perror(devname);
        return -1;
    }

    /* Configure pins as output. */
    ioctl(fd, GPIO_PORTE | GPIO_CONFOUT, MASK_D5 | MASK_D6 | MASK_D7);
    ioctl(fd, GPIO_PORTE | GPIO_CLEAR, MASK_D5 | MASK_D6 | MASK_D7);

    for (;;) {
        /* Clear D5, set D7. */
        ioctl(fd, GPIO_PORTE | GPIO_CLEAR, MASK_D5);
        ioctl(fd, GPIO_PORTE | GPIO_SET, MASK_D7);
        usleep(100000);

        /* Clear D7, set D6. */
        ioctl(fd, GPIO_PORTE | GPIO_CLEAR, MASK_D7);
        ioctl(fd, GPIO_PORTE | GPIO_SET, MASK_D6);
        usleep(100000);

        /* Clear D6, set D5. */
        ioctl(fd, GPIO_PORTE | GPIO_CLEAR, MASK_D6);
        ioctl(fd, GPIO_PORTE | GPIO_SET, MASK_D5);
        usleep(100000);
    }
    return 0;
}
