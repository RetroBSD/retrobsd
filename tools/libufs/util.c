#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "libufs.h"

void compresspath(char *path)
{
    char *work;
    char *p, *q;
    char **dirs;
    int ndir;
    int i,j;

    // First, remove any double slashes

    for (p = path; *(p+1); p++) {
        while ((*p == '/') && (*(p+1) == '/')) {
            for (q = p+1; *q; q++) {
                *q = *(q+1);
            }
        }
    }

    // Now we need to split it all up and remove any single dots, and
    // cd up any double dots.

    work = strdup(path);

    dirs = splitpath(work, &ndir);

    for (i = 0; i < ndir; i++) {
        if (!strcmp(dirs[i], ".")) {
            for (j = i; j < ndir-1; j++) {
                dirs[j] = dirs[j+1];
            }
            ndir--;
        }
        if (!strcmp(dirs[i], "..")) {
            for (j = i-1; j < ndir-1; j++) {
                dirs[j] = dirs[j+1];
            }
            ndir--;
            for (j = i-1; j < ndir-1; j++) {
                dirs[j] = dirs[j+1];
            }
            ndir--;
        }
    }

    sprintf(path, "%s", "");
    for (i=0; i<ndir; i++) {
        sprintf(path, "%s/%s", path, dirs[i]);
    }

    if (path[0] == '\0') {
        path[0] = '/';
        path[1] = '\0';
    }
    free(work);
    free(dirs);
}
