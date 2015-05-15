/*
 * Example for RetroBSD on Olimex Duinomite board.
 * Infrared LED is connected to D0 pin of Duinomite board.
 * Use ioctl() calls to poll the user button, and control the buzzer.
 */
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/gpio.h>

/*
 * Pin D0 is connected to signal RE0 on pic32 chip.
 * User button is connected to signal RD8 on pic32 chip.
 */
#define MASK_D0     (1 << 0)    /* signal RE0 */
#define MASK_BUTTON (1 << 8)    /* signal RD8 */

int main()
{
    int fd, portb;
    char *devname = "/dev/porta";

    /* Open GPIO driver. */
    fd = open(devname, 1);
    if (fd < 0) {
        perror(devname);
        return -1;
    }

    /* Configure pins. */
    ioctl(fd, GPIO_PORTD | GPIO_CONFIN, MASK_BUTTON);
    ioctl(fd, GPIO_PORTE | GPIO_CONFOUT, MASK_D0);
    ioctl(fd, GPIO_PORTE | GPIO_CLEAR, MASK_D0);

    for (;;) {
        /* Poll button at RD8 (active low). */
        portb = ioctl(fd, GPIO_PORTD | GPIO_POLL, 0);

        if (~portb & MASK_BUTTON) {
            /* Transmit a series of three pulses 50 msec each,
             * repeat every second. */
            ioctl(fd, GPIO_PORTE | GPIO_SET, MASK_D0);
            usleep(50000);
            ioctl(fd, GPIO_PORTE | GPIO_CLEAR, MASK_D0);
            usleep(50000);
            ioctl(fd, GPIO_PORTE | GPIO_SET, MASK_D0);
            usleep(50000);
            ioctl(fd, GPIO_PORTE | GPIO_CLEAR, MASK_D0);
            usleep(50000);
            ioctl(fd, GPIO_PORTE | GPIO_SET, MASK_D0);
            usleep(50000);
            ioctl(fd, GPIO_PORTE | GPIO_CLEAR, MASK_D0);
            usleep(750000);
        }
    }
    return 0;
}
