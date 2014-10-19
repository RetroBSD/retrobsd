/*
 * Example of polling general purpose i/o pins.
 */
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/gpio.h>

int main(void)
{
    int fd, pnum, value;

    fd = open ("/dev/porta", O_RDWR);
    if (fd < 0) {
        perror ("/dev/porta");
        return -1;
    }

    for (pnum=0; pnum<7; pnum++) {
        value = ioctl (fd, GPIO_POLL | GPIO_PORT (pnum), 0);
        if (value < 0)
            perror ("GPIO_POLL");
        printf ("port%c = 0x%04x\n", pnum + 'A', value);
    }
    return 0;
}
