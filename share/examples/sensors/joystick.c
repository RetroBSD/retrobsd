/*
 * Example for RetroBSD on Olimex Duinomite board.
 * Joystick is connected to A0, A1 and D7 pin of Duinomite board.
 */
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/gpio.h>

/*
 * Pin D7 is connected to signal RE7 on pic32 chip.
 */
#define MASK_BUTTON     (1 << 7)    /* signal RE7 */

/*
 * Read value from analog-to-digital converter.
 * Scale to given max value.
 */
int readadc(int fd, int max)
{
    char buf[16];
    int val;

    if (read(fd, buf, sizeof(buf)) <= 0) {
        perror("adc");
        exit(-1);
    }
    val = strtol(buf, 0, 10);
    return val * max / 1024;
}

int main()
{
    int gpio, adc3, adc4, porte, x, y;

    /* Open GPIO driver. */
    gpio = open("/dev/porta", 1);
    if (gpio < 0) {
        perror("/dev/porta");
        return -1;
    }

    /* Open ADC driver. */
    adc3 = open("/dev/adc3", 0);
    if (adc3 < 0) {
        perror("/dev/adc3");
        return -1;
    }
    adc4 = open("/dev/adc4", 0);
    if (adc4 < 0) {
        perror("/dev/adc4");
        return -1;
    }

    /* Configure D7 pin is input, active low. */
    ioctl(gpio, GPIO_PORTE | GPIO_CONFIN, MASK_BUTTON);

    /* Clear screen. */
    printf("\33[2J");

    for (;;) {
        /* Poll joystick every 20 msec. */
        usleep(20000);
        porte = ioctl(gpio, GPIO_PORTE | GPIO_POLL, 0);
        x = readadc(adc3, 50);
        y = readadc(adc4, 25);

        if (~porte & MASK_BUTTON) {
            /* When button pressed - clear screen. */
            printf("\33[2J");
        }
        printf("\33[H%u, %u  ", x, y);
        printf("\33[%u;%uHX", y, 78 - x);
        fflush(stdout);
    }
    return 0;
}
