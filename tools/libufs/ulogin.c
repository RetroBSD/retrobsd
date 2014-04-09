#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "libufs.h"

int main(int argc, char *argv[])
{
    char *filename;
    struct filesystem *f;
    char fullpath[1024];
    char cwd[800];

    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 10;
    }

    filename = argv[1];

    f = fsopen(filename);
    if (!f) {
        printf("Unable to open %s\n", filename);
        return 10;
    }

    if (filename[0] == '/') {
        storesetting("imagefile", filename);
    } else {
        getcwd(cwd,800);
        sprintf(fullpath, "%s/%s", cwd, filename);
        storesetting("imagefile", fullpath);
    }

    storesetting("cwd","/");

    return 0;
}
