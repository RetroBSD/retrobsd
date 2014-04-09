#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ioctl.h>
#include <sys/swap.h>

void usage()
{
	printf("Usage: talloc /dev/tempX <size>\n");
}

int main(int argc, char *argv[])
{
    int fd;
    int size;

    if (argc != 3) {
        usage();
        return 10;
    }

    fd = open(argv[1], O_RDONLY);
    if (!fd) {
        printf("Unable to open %s\n", argv[0]);
        return 10;
    }

    size = atoi(argv[2]);

    ioctl(fd, TFALLOC, &size);

    close(fd);

    return 0;
}
