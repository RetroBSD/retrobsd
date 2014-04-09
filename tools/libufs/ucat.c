#include <stdio.h>
#include <stdlib.h>

#include "libufs.h"

int main(int argc, char *argv[])
{
    struct filesystem *f;
    char path[1024];
    char *image = getsetting("imagefile");
    UFILE *file;
    int c;

    if (!image) {
        printf("Have you run ulogin?\n");
        return 10;
    }

    char *cwd = getsetting("cwd");
    if (argc == 2) {
        if (argv[1][0] == '/') {
            sprintf(path, "%s", argv[1]);
        } else {
            sprintf(path, "%s/%s", cwd, argv[1]);
        }
    } else {
        sprintf(path, "%s", cwd);
    }
    free(cwd);
        
    f = fsopen(image);
    if (!f) {
        printf("Unable to open %s\n", image);
            fsclose(f);
            free(image);
            return(10);
    }

    file = ufopen(f, path, "r");
    if (!file) {
        printf("%s: not found\n", path);
        fsclose(f);
        free(image);
        return(10);
    }

    while ((c = ufgetc(file)) != EOF) {
        printf("%c", c);
    }

    ufclose(file);

    fsclose(f);
    free(image);
    return 0;
}
