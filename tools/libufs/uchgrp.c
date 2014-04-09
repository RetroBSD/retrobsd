#include <stdio.h>
#include <stdlib.h>

#include "libufs.h"

int main(int argc, char *argv[])
{
    struct filesystem *f;
    int i;
    int mode;
    char temp[1024];
    char *cwd;
    char *fsname = getsetting("imagefile");;

    if (!fsname) {
        printf("Have you run ulogin?\n");
        return 10;
    }

    if (argc < 3) {
        printf("Usage: %s <gid> <file> ...\n", argv[0]);
        return 10;
    }

    sscanf(argv[1], "%o", &mode);
        
    f = fsopen(fsname);
    if (!f) {
        printf("Unable to open %s\n", fsname);
        return 10;
    }
    cwd = getsetting("cwd");
    for (i = 2; i < argc; i++) {
        if (argv[i][0] == '/') {
            uchgrp(f, argv[i], mode);
        } else {
            if (!cwd) {
                sprintf(temp, "/%s", argv[i]);
            } else {
                sprintf(temp, "%s/%s", cwd, argv[i]);
            }
            compresspath(temp);
            uchgrp(f, temp, mode);
        }
    }
    fsclose(f);
    if (cwd)
        free(cwd);
    free(fsname);
    return 0;
}
