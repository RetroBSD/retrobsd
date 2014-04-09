/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "tip.h"
#include <time.h>

#ifdef ACULOG
static  FILE *flog = NULL;

/*
 * Log file maintenance routines
 */
void logent(group, num, acu, message)
    char *group, *num, *acu, *message;
{
    char *user, *timestamp;
    struct passwd *pwd;
    long t;

    if (flog == NULL)
        return;
    if (flock(fileno(flog), LOCK_EX) < 0) {
        perror("tip: flock");
        return;
    }
    user = getlogin();
    if (user == NOSTR) {
        if ((pwd = getpwuid(getuid())) == NOPWD)
            user = "???";
        else
            user = pwd->pw_name;
    }
    t = time(0);
    timestamp = ctime(&t);
    timestamp[24] = '\0';
    fprintf(flog, "%s (%s) <%s, %s, %s> %s\n",
        user, timestamp, group,
#ifdef PRISTINE
        "",
#else
        num,
#endif
        acu, message);
    (void) fflush(flog);
    (void) flock(fileno(flog), LOCK_UN);
}

void loginit()
{
    flog = fopen(value(LOG), "a");
    if (flog == NULL)
        fprintf(stderr, "can't open log file %s.\r\n", value(LOG));
}
#endif
