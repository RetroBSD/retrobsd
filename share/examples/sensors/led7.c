/*
 * Example for RetroBSD on Olimex Duinomite board.
 * RGB LED is connected to D7, D6 and D5 pins of Duinomite board.
 * Use ioctl() calls to control LED.
 */
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/gpio.h>

/*
 * Pin D2 is connected to signal RE2 on pic32 chip.
 */
#define MASK_D2     (1 << 2)    /* signal RE2 */

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

    /* Configure pin as output. */
    ioctl(fd, GPIO_PORTE | GPIO_CONFOUT, MASK_D2);
    ioctl(fd, GPIO_PORTE | GPIO_CLEAR, MASK_D2);

    for (;;) {
        /* Set D2. */
        ioctl(fd, GPIO_PORTE | GPIO_SET, MASK_D2);
        sleep(15);

        /* Clear D2. */
        ioctl(fd, GPIO_PORTE | GPIO_CLEAR, MASK_D2);
        sleep(3);
    }
    return 0;
}
