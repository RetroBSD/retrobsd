#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "libufs.h"

int main(int argc, char *argv[])
{
    struct stat sb;
    int opt;
    int blocks = 0;
    int bpi = BPI;
    struct filesystem *f;

    while ((opt = getopt(argc, argv, "i:b:")) != -1) {
        switch (opt) {
        case 'i':
            bpi = atoi(optarg);
            break;
        case 'b':
            blocks = atoi(optarg);
            break;
        }
    }

    if (argc <= optind) {
        printf("Usage: %s [-i bpi] [-b blocks] file\n", argv[0]);
        printf("  Format an file image with a UFS filesystem\n");
        return 10;
    }

    if (stat(argv[optind], &sb) != -1) {
        f = fsopen(argv[optind]);
        if (!f) {
            printf("Error opening image file\n");
            return 10;
        }
        f->fs->fs_fsize = sb.st_size / DEV_BSIZE;
        fsformat(f, bpi);
    } else {
        f = fsnew(argv[optind], blocks, bpi);
    }

    fsclose(f);

    return 0;
}
