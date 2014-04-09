#include <stdio.h>
#include <stdlib.h>

#include "libufs.h"

int main(int argc, char *argv[])
{
    struct filesystem *f;
    int i;
    char *image = getsetting("imagefile");
    char temp[1024];
    char *cwd;

    if (argc < 2) {
        printf("Usage: %s <directory> ...\n", argv[0]);
        return 10;
    }
        
    f = fsopen(image);
    if (!f) {
        return 10;
    }
    cwd = getsetting("cwd");
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '/') {
            umkdir(f, argv[i]);
        } else {
            if (!cwd) {
                sprintf(temp, "/%s", argv[i]);
            } else {
                sprintf(temp, "%s/%s", cwd, argv[i]);
            }
            compresspath(temp);
            umkdir(f, temp);
        }
    }
    if (cwd)
        free(cwd);
    free(image);
    fsclose(f);
    return 0;
}
