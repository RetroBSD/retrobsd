#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include "defs.h"

#define MAXARGV 254 /* execvp can only handle 254 anyhow */

int await()
{
    int status;
    register int pid;

    enbint(SIG_IGN);
    while ((pid = wait(&status)) != wpid)
        if (pid == -1)
            fatal("bad wait code");
    wpid = 0;
    enbint(intrupt);
    return (status);
}

/*
 * Close open directory files before exec'ing
 */
void doclose()
{
    register struct dirhdr *od;

    for (od = firstod; od; od = od->nxtopendir) {
        if (od->dirfc != NULL) {
            /*
             * vfork kludge...
             * we cannot call closedir since this will modify
             * the parents data space; just call close directly.
             */
            close(od->dirfc->dd_fd);
        }
    }
}

int doexec(char *str)
{
    register char *t;
    static char *argv[MAXARGV]; /* docom() ate most of the stack already */
    register char **p;

    while (*str == ' ' || *str == '\t')
        ++str;
    if (*str == '\0')
        return (-1); /* no command */

    p = argv;
    for (t = str; *t;) {
        if (p >= argv + MAXARGV)
            fatal1("%s: Too many arguments.", str);
        *p++ = t;
        while (*t != ' ' && *t != '\t' && *t != '\0')
            ++t;
        if (*t)
            for (*t++ = '\0'; *t == ' ' || *t == '\t'; ++t)
                ;
    }
    *p = NULL;

    wpid = vfork();
    if (wpid == 0) {
        enbint(SIG_DFL);
        doclose();
        enbint(intrupt);
        execvp(str, argv);
        fatal1("Cannot load %s", str);
    }
    return await();
}

int doshell(char *comstring, int nohalt)
{
#ifdef SHELLENV
    char *shellcom = getenv("SHELL");
    char *shellstr;
#endif
    wpid = vfork();
    if (wpid == 0) {
        enbint(SIG_DFL);
        doclose();

#ifdef SHELLENV
        if (shellcom == 0)
            shellcom = SHELLCOM;
        shellstr = rindex(shellcom, '/') + 1;
        execl(shellcom, shellstr, (nohalt ? "-c" : "-ce"), comstring, (char *)0);
#else
        execl(SHELLCOM, "sh", (nohalt ? "-c" : "-ce"), comstring, (char *)0);
#endif
        fatal("Couldn't load Shell");
    }
    return await();
}

/*
 * Are there are any Shell meta-characters?
 */
int metas(char *s)
{
    register int c;

    for (;;) {
        c = *s++;
        if (!c)
            break;
        if (funny[c] & META)
            return c;
    }
    return 0;
}

int dosys(char *comstring, int nohalt)
{
    register int status;

    if (metas(comstring))
        status = doshell(comstring, nohalt);
    else
        status = doexec(comstring);

    return (status);
}

#include <errno.h>
#include <sys/stat.h>

void touch(int force, char *name)
{
    struct stat stbuff;
    char junk[1];
    int fd;

    if (stat(name, &stbuff) < 0) {
        if (force)
            goto create;
        fprintf(stderr, "touch: file %s does not exist.\n", name);
        return;
    }

    if (stbuff.st_size == 0)
        goto create;

    fd = open(name, 2);
    if (fd < 0)
        goto bad;

    if (read(fd, junk, 1) < 1) {
        close(fd);
        goto bad;
    }
    lseek(fd, 0L, 0);
    if (write(fd, junk, 1) < 1) {
        close(fd);
        goto bad;
    }
    close(fd);
    return;

bad:
    fprintf(stderr, "Cannot touch %s\n", name);
    return;

create:
    fd = creat(name, 0666);
    if (fd < 0)
        goto bad;
    close(fd);
}
