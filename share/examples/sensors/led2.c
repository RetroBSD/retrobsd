/*
 * Example for RetroBSD on Olimex Duinomite board.
 * RGB LED is connected to D1 and D0 pins of Duinomite board.
 * Use ioctl() calls to control LED.
 */
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/gpio.h>

/*
 * Pins D0 and D1 are connected to signals RE0 and RE1 of pic32 chip.
 */
#define MASK_D0     (1 << 0)    /* signal RE0 */
#define MASK_D1     (1 << 1)    /* signal RE1 */

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
    ioctl(fd, GPIO_PORTE | GPIO_CONFOUT, MASK_D0 | MASK_D1);
    ioctl(fd, GPIO_PORTE | GPIO_CLEAR, MASK_D0 | MASK_D1);

    for (;;) {
        /* Clear D0, set D1. */
        ioctl(fd, GPIO_PORTE | GPIO_CLEAR, MASK_D0);
        ioctl(fd, GPIO_PORTE | GPIO_SET, MASK_D1);
        usleep(250000);

        /* Clear D1, set D0. */
        ioctl(fd, GPIO_PORTE | GPIO_CLEAR, MASK_D1);
        ioctl(fd, GPIO_PORTE | GPIO_SET, MASK_D0);
        usleep(250000);
    }
    return 0;
}
