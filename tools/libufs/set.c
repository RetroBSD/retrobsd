#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "libufs.h"

char *getsetting(char *setting)
{
    FILE *f;
    char *s = NULL;
    char *home;
    char temp[1024];
    char sub[1024];
    char fmt[1024];

    home = getenv("HOME");

    sprintf(temp, "%s/.libufs", home);

    f = fopen(temp, "r");
    if (!f) {
        return s;
    }

    sprintf(fmt, "%s=%%s", setting);

    while (fgets(temp, 1024, f)) {
        bzero(sub, 800);
        if (sscanf(temp, fmt, sub)) {
            s = strdup(sub);
        }
    }

    fclose(f);
    return s;
}

void storesetting(char *setting, char *value)
{
    FILE *f1;
    FILE *f2;
    char *home;
    char temp1[1024];
    char temp2[1024];

    if (!setting) {
        return;
    }

    home = getenv("HOME");

    sprintf(temp1, "%s/.libufs.bak", home);
    sprintf(temp2, "%s/.libufs", home);
    
    rename(temp2, temp1);

    f1 = fopen(temp1, "r");
    if (!f1) {
        return;
    }
    f2 = fopen(temp2, "w");
    if (!f2) {
        fclose(f2);
        return;
    }

    sprintf(temp2, "%s=", setting);

    while (fgets(temp1, 1024, f1)) {
        if (strncmp(temp1, temp2, strlen(temp2)) != 0) {
            if (strlen(temp1) > 2) {
                fprintf(f2, "%s", temp1);
            }
        }
    }    

    if (value) {
        fprintf(f2, "%s=%s\n", setting, value);
    }
    fclose(f1);
    fclose(f2);
}

