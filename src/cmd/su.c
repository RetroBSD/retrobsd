/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <grp.h>
#include <paths.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

char userbuf[64] = "USER=";
char homebuf[128] = "HOME=";
char shellbuf[128] = "SHELL=";
char pathbuf[128] = "PATH=" _PATH_STDPATH;
char *cleanenv[] = { userbuf, homebuf, shellbuf, pathbuf, 0, 0 };
char *user = "root";
char *shell = _PATH_BSHELL;
int fulllogin;
int fastlogin;

struct passwd *pwd;

void setenvv(char *ename, char *eval, char *buf)
{
    char *cp, *dp;
    char **ep = environ;

    /*
     * this assumes an environment variable "ename" already exists
     */
    while ((dp = *ep++)) {
        for (cp = ename; *cp == *dp && *cp; cp++, dp++)
            continue;
        if (*cp == 0 && (*dp == '=' || *dp == 0)) {
            strcat(buf, eval);
            *--ep = buf;
            return;
        }
    }
}

char *getenvv(char *ename)
{
    char *cp, *dp;
    char **ep = environ;

    while ((dp = *ep++)) {
        for (cp = ename; *cp == *dp && *cp; cp++, dp++)
            continue;
        if (*cp == 0 && (*dp == '=' || *dp == 0))
            return (*--ep);
    }
    return ((char *)0);
}

int main(int argc, char *argv[])
{
    char *password;
    char buf[1000];
    FILE *fp;
    char *p;

    openlog("su", LOG_ODELAY, LOG_AUTH);

again:
    if (argc > 1 && strcmp(argv[1], "-f") == 0) {
        fastlogin++;
        argc--, argv++;
        goto again;
    }
    if (argc > 1 && strcmp(argv[1], "-") == 0) {
        fulllogin++;
        argc--, argv++;
        goto again;
    }
    if (argc > 1 && argv[1][0] != '-') {
        user = argv[1];
        argc--, argv++;
    }
    if ((pwd = getpwuid(getuid())) == NULL) {
        fprintf(stderr, "Who are you?\n");
        exit(1);
    }
    strcpy(buf, pwd->pw_name);
    if ((pwd = getpwnam(user)) == NULL) {
        fprintf(stderr, "Unknown login: %s\n", user);
        exit(1);
    }
    /*
     * Only allow those in group zero to su to root.
     */
    if (pwd->pw_uid == 0) {
        struct group *gr;
        int i;

        if ((gr = getgrgid(0)) != NULL) {
            for (i = 0; gr->gr_mem[i] != NULL; i++)
                if (strcmp(buf, gr->gr_mem[i]) == 0)
                    goto userok;
            fprintf(stderr, "You do not have permission to su %s\n", user);
            exit(1);
        }
    userok:
        setpriority(PRIO_PROCESS, 0, -2);
    }

#define Getlogin() (((p = getlogin()) && *p) ? p : buf)
    if (pwd->pw_passwd[0] == '\0' || getuid() == 0)
        goto ok;
    password = getpass("Password:");
    if (strcmp(pwd->pw_passwd, crypt(password, pwd->pw_passwd)) != 0) {
        fprintf(stderr, "Sorry\n");
        if (pwd->pw_uid == 0) {
            syslog(LOG_CRIT, "BAD SU %s on %s", Getlogin(), ttyname(2));
        }
        exit(2);
    }
ok:
    endpwent();
    if (pwd->pw_uid == 0) {
        syslog(LOG_NOTICE, "%s on %s", Getlogin(), ttyname(2));
        closelog();
    }
    if (setgid(pwd->pw_gid) < 0) {
        perror("su: setgid");
        exit(3);
    }
    if (initgroups(user, pwd->pw_gid)) {
        fprintf(stderr, "su: initgroups failed\n");
        exit(4);
    }
    if (setuid(pwd->pw_uid) < 0) {
        perror("su: setuid");
        exit(5);
    }
    if (pwd->pw_shell && *pwd->pw_shell)
        shell = pwd->pw_shell;
    if (fulllogin) {
        cleanenv[4] = getenvv("TERM");
        environ = cleanenv;
    }
    if (strcmp(user, "root"))
        setenvv("USER", pwd->pw_name, userbuf);
    setenvv("SHELL", shell, shellbuf);
    setenvv("HOME", pwd->pw_dir, homebuf);
    setpriority(PRIO_PROCESS, 0, 0);
    if (fastlogin) {
        *argv-- = "-f";
        *argv = "su";
    } else if (fulllogin) {
        if (chdir(pwd->pw_dir) < 0) {
            fprintf(stderr, "No directory\n");
            exit(6);
        }
        *argv = "-su";
    } else
        *argv = "su";
    execv(shell, argv);
    fprintf(stderr, "No shell\n");
    exit(7);
}
