#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ioctl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <pwm.h>

int pwm;

void error(char *message)
{
    fprintf(stderr, "Error: %s: %d\n", message, errno);
}

void open_device(int dev)
{
    char temp[20];
    int mode = PWM_MODE_PWM;

    sprintf(temp, "/dev/pwm%d", dev);
    pwm = open(temp, O_RDWR);
    if (! pwm) {
        error("Could not open PWM device");
        exit(-1);
    }
    if (ioctl(pwm, PWM_SET_MODE, &mode) < 0) {
        error("Could not switch channel to PWM mode");
        exit(-1);
    }
}

int main(int argc, char *argv[])
{
    int unit;
    unsigned duty;

    if (argc != 3) {
        error("Specify a channel and a duty cycle");
        exit(-1);
    }

    unit = atoi(argv[1]);
    duty = atol(argv[2]);

    if (unit < 0 || unit > 4) {
        error("Invalid channel specified");
        exit(-1);
    }
    open_device(unit);

    if (ioctl(pwm, PWM_DUTY, &duty) < 0) {
        error("Could not set duty");
        exit(-1);
    }
    return 0;
}
