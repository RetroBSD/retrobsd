#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <machine/adc.h>
#include <unistd.h>

int fd[16];
int value[16];

void putsym(int c, int count)
{
    while (count-- > 1)
        putchar(c);
}

int main(int argc, char **argv)
{
    char buf[100];
    int i, opt;
    long flags;
    unsigned long delay_msec = 100;

    while ((opt = getopt(argc, argv, ":d:")) != -1) {
        switch (opt) {
            case 'd':
                delay_msec = strtol(optarg, 0, 0);
                break;
        }
    }

    for (i=0; i<16; i++) {
        sprintf(buf, "/dev/adc%d", i);
        fd[i] = open(buf, O_RDWR);
        if (fd[i] < 0) {
            perror(buf);
            return -1;
        }
        fcntl(fd[i], F_GETFD, &flags);
        flags |= O_NONBLOCK;
        fcntl(fd[i], F_SETFD, &flags);
    }

    /* Clear screen. */
    printf("\33[2J");

    for (;;) {
        /* Top of screen. */
        printf("\33[H");

        for (i=0; i<16; i++) {
            if (read(fd[i], buf, 20) > 0) {
                value[i] = strtol(buf, 0, 0);
            }
            printf("adc%-2d %4d  ", i, value[i]);
            putsym('=', value[i] >> 4);

            /* Clear to end of line. */
            printf("\33[K");
            printf("\n");
        }
        usleep(delay_msec * 1000);
    }
}
