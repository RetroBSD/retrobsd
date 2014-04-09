/*
 * Example of reading ADC data.
 */
#include <fcntl.h>

char buf[100];

main()
{
    int i, fd, value;

    for (i=0; i<16; i++) {
        sprintf(buf, "/dev/adc%d", i);
        fd = open(buf, O_RDWR);
        if (fd < 0) {
            printf("Error: unable to open %s\n", buf);
        } else {
            if (read(fd, buf, 20) > 0) {
                value = strtol (buf, 0, 0);
                printf("adc%-2d = %d\n", i, value);
            }
            close(fd);
        }
    }
    return 0;
}
