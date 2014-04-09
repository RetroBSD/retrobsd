/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * defs that come from uucp.h
 */
#define NAMESIZE 32
#define FAIL -1
#define SAME 0
#define SLCKTIME 28800  /* system/device timeout (LCK.. files) in seconds (8 hours) */
#define ASSERT(e, f, v) if (!(e)) {\
    fprintf(stderr, "AERROR - (%s) ", "e");\
    fprintf(stderr, f, v);\
    finish();\
}

#define LOCKPRE "/var/lock/LCK."

/*
 * This code is taken almost directly from uucp and follows the same
 * conventions.  This is important since uucp and tip should
 * respect each others locks.
 */

    /*  ulockf 3.2  10/26/79  11:40:29  */
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#define MAXLOCKS 10 /* maximum number of lock files */
char *Lockfile[MAXLOCKS];
int Nlocks = 0;

void finish (void);

/***
 *  stlock(name)    put name in list of lock files
 *  char *name;
 *
 *  return codes:  none
 */
static void
stlock(name)
    char *name;
{
    char *p;
    int i;

    for (i = 0; i < Nlocks; i++) {
        if (Lockfile[i] == NULL)
            break;
    }
    ASSERT(i < MAXLOCKS, "TOO MANY LOCKS %d", i);
    if (i >= Nlocks)
        i = Nlocks++;
    p = calloc(strlen(name) + 1, sizeof (char));
    ASSERT(p != NULL, "CAN NOT ALLOCATE FOR %s", name);
    strcpy(p, name);
    Lockfile[i] = p;
}

/*
 * this stuff from pjw
 *  /usr/pjw/bin/recover - check pids to remove unnecessary locks
 *
 *  onelock(pid,tempfile,name) makes lock a name
 *  on behalf of pid.  Tempfile must be in the same
 *  file system as name.
 *
 *  lock(pid,tempfile,names) either locks all the
 *  names or none of them
 */
static int
onelock(pid, tempfile, name)
    char *tempfile, *name;
{
    int fd;

    fd = creat(tempfile, 0444);
    if (fd < 0)
        return (-1);
    write(fd,(char *)&pid, sizeof(int));
    close(fd);
    if (link(tempfile, name) < 0) {
        unlink(tempfile);
        return (-1);
    }
    unlink(tempfile);
    return (0);
}

/*******
 *  ulockf(file, atime)
 *  char *file;
 *  time_t atime;
 *
 *  ulockf  -  this routine will create a lock file (file).
 *  If one already exists, the create time is checked for
 *  older than the age time (atime).
 *  If it is older, an attempt will be made to unlink it
 *  and create a new one.
 *
 *  return codes:  0  |  FAIL
 */
static int
ulockf(file, atime)
    char *file;
    time_t atime;
{
    struct stat stbuf;
    time_t ptime;
    int ret;
    static int pid = -1;
    static char tempfile[NAMESIZE];

    if (pid < 0) {
        pid = getpid();
        sprintf(tempfile, "/var/lock/LTMP.%d", pid);
    }
    if (onelock(pid, tempfile, file) == -1) {
        /* lock file exists */
        /* get status to check age of the lock file */
        ret = stat(file, &stbuf);
        if (ret != -1) {
            time(&ptime);
            if ((ptime - stbuf.st_ctime) < atime) {
                /* file not old enough to delete */
                return (FAIL);
            }
        }
        ret = unlink(file);
        ret = onelock(pid, tempfile, file);
        if (ret != 0)
            return (FAIL);
    }
    stlock(file);
    return (0);
}

/***
 *  rmlock(name)    remove all lock files in list
 *  char *name; or name
 *
 *  return codes: none
 */
static void
rmlock(name)
    char *name;
{
    int i;

    for (i = 0; i < Nlocks; i++) {
        if (Lockfile[i] == NULL)
            continue;
        if (name == NULL || strcmp(name, Lockfile[i]) == SAME) {
            unlink(Lockfile[i]);
            free(Lockfile[i]);
            Lockfile[i] = NULL;
        }
    }
}

/***
 *  delock(s)   remove a lock file
 *  char *s;
 *
 *  return codes:  0  |  FAIL
 */
void
delock(s)
    char *s;
{
    char ln[30];

    sprintf(ln, "%s.%s", LOCKPRE, s);
    rmlock(ln);
}

/***
 *  mlock(sys)  create system lock
 *  char *sys;
 *
 *  return codes:  0  |  FAIL
 */
int mlock(sys)
    char *sys;
{
    char lname[30];
    sprintf(lname, "%s.%s", LOCKPRE, sys);
    return (ulockf(lname, (time_t) SLCKTIME ) < 0 ? FAIL : 0);
}
