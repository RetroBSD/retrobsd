#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libufs.h"

int main(int argc, char *argv[])
{
    struct filesystem *f;
    char *temp = malloc(1024);
    char *image;

    image = getsetting("imagefile");

    if (!image) {
        printf("Have you run ulogin?\n");
        return 10;
    }

    if (argc != 2) {
        printf("Usage: %s <dir>\n", argv[0]);
        free(image);
        return 10;
    }

    f = fsopen(image);
    if (!f) {
        if(image) free(image);
        return 10;
    }

    if (argv[1][0] == '/') {
        if (!inodebypath(f, argv[1])) {
            printf("%s not found\n", argv[1]);
        } else {
            storesetting("cwd", argv[1]);
        }
        fsclose(f);
        free(image);
        return 0;
    } else {
        char *cwd = getsetting("cwd");
        if (!cwd) {
            sprintf(temp, "/%s", argv[1]);
        } else {
            sprintf(temp, "%s/%s", cwd, argv[1]);
            compresspath(temp);
            free(cwd);
        }
        if (!inodebypath(f, temp)) {
            printf("%s not found\n", temp);
        } else {
            storesetting("cwd", temp);
        }
        free(image);
        fsclose(f);
        return 0;
    }
    return 0;
}
