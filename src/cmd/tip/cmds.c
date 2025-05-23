/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/wait.h>
#include "tip.h"

/*
 * tip
 *
 * miscellaneous commands
 */

int quant[] = { 60, 60, 24 };

char    null = '\0';
char    *sep[] = { "second", "minute", "hour" };
static char *argv[10];          /* argument vector for take and put */
static  jmp_buf intbuf;

int intprompt();                /* used in handling SIG_INT during prompt */

/*
 * interrupt routine for file transfers
 */
void
intcopy(int sig)
{
    raw();
    quit = 1;
    longjmp(intbuf, 1);
}

/*
 * Interrupt service routine for FTP
 */
void
stopsnd(int sig)
{
    stop = 1;
    signal(SIGINT, SIG_IGN);
}

/*
 * timeout function called on alarm
 */
void
timeout(int sig)
{
    signal(SIGALRM, timeout);
    timedout = 1;
}

static void prtime(char *s, time_t a)
{
    register int i;
    int nums[3];

    for (i = 0; i < 3; i++) {
        nums[i] = (int)(a % quant[i]);
        a /= quant[i];
    }
    printf("%s", s);
    while (--i >= 0)
        if (nums[i] || (i == 0 && nums[1] == 0 && nums[2] == 0))
            printf("%d %s%c ", nums[i], sep[i],
                nums[i] == 1 ? '\0' : 's');
    printf("\r\n!\r\n");
}

/*
 * Bulk transfer routine --
 *  used by getfl(), cu_take(), and pipefile()
 */
static void transfer(char *buf, int fd, char *eofchars)
{
    register int ct;
    char c, buffer[BUFSIZ];
    register char *p = buffer;
    register int cnt, eof;
    time_t start;
    sig_t f;

    pwrite(FD, buf, size(buf));
    quit = 0;
    kill(pid, SIGIOT);
    read(repdes[0], (char *)&ccc, 1);  /* Wait until read process stops */

    /*
     * finish command
     */
    pwrite(FD, "\r", 1);
    do
        read(FD, &c, 1);
    while ((c&0177) != '\n');
    ioctl(0, TIOCSETC, &defchars);

    (void) setjmp(intbuf);
    f = signal(SIGINT, intcopy);
    start = time(0);
    for (ct = 0; !quit;) {
        eof = read(FD, &c, 1) <= 0;
        c &= 0177;
        if (quit)
            continue;
        if (eof || any(c, eofchars))
            break;
        if (c == 0)
            continue;   /* ignore nulls */
        if (c == '\r')
            continue;
        *p++ = c;

        if (c == '\n' && boolean(value(VERBOSE)))
            printf("\r%d", ++ct);
        if ((cnt = (p-buffer)) == number(value(FRAMESIZE))) {
            if (write(fd, buffer, cnt) != cnt) {
                printf("\r\nwrite error\r\n");
                quit = 1;
            }
            p = buffer;
        }
    }
    cnt = p - buffer;
    if (cnt > 0)
        if (write(fd, buffer, cnt) != cnt)
            printf("\r\nwrite error\r\n");

    if (boolean(value(VERBOSE)))
        prtime(" lines transferred in ", time(0)-start);
    ioctl(0, TIOCSETC, &tchars);
    write(fildes[1], (char *)&ccc, 1);
    signal(SIGINT, f);
    close(fd);
}

/*
 * FTP - remote ==> local
 *  get a file from the remote host
 */
void getfl(int c)
{
    char buf[256], *cp;

    putchar(c);
    /*
     * get the UNIX receiving file's name
     */
    if (prompt("Local file name? ", copyname))
        return;
    cp = expand(copyname);
    if ((sfd = creat(cp, 0666)) < 0) {
        printf("\r\n%s: cannot creat\r\n", copyname);
        return;
    }

    /*
     * collect parameters
     */
    if (prompt("List command for remote system? ", buf)) {
        unlink(copyname);
        return;
    }
    transfer(buf, sfd, value(EOFREAD));
}

static int args(char *buf, char *a[])
{
    register char *p = buf, *start;
    register char **parg = a;
    register int n = 0;

    do {
        while (*p && (*p == ' ' || *p == '\t'))
            p++;
        start = p;
        if (*p)
            *parg = p;
        while (*p && (*p != ' ' && *p != '\t'))
            p++;
        if (p != start)
            parg++, n++;
        if (*p)
            *p++ = '\0';
    } while (*p);

    return(n);
}

/*
 * Cu-like take command
 */
void cu_take(char cc)
{
    int fd, argc;
    char line[BUFSIZ], *cp;

    if (prompt("[take] ", copyname))
        return;
    argc = args(copyname, argv);
    if (argc < 1 || argc > 2) {
        printf("usage: <take> from [to]\r\n");
        return;
    }
    if (argc == 1)
        argv[1] = argv[0];
    cp = expand(argv[1]);
    if ((fd = creat(cp, 0666)) < 0) {
        printf("\r\n%s: cannot create\r\n", argv[1]);
        return;
    }
    sprintf(line, "cat %s;echo \01", argv[0]);
    transfer(line, fd, "\01");
}

static void execute(char *s)
{
    register char *cp;

    if ((cp = strrchr(value(SHELL), '/')) == NULL)
        cp = value(SHELL);
    else
        cp++;
    user_uid();
    execl(value(SHELL), cp, "-c", s, (char*)0);
}

/*
 * FTP - remote ==> local process
 *   send remote input to local process via pipe
 */
void pipefile()
{
    int cpid, pdes[2];
    char buf[256];
    int status, p;

    if (prompt("Local command? ", buf))
        return;

    if (pipe(pdes)) {
        printf("can't establish pipe\r\n");
        return;
    }

    if ((cpid = fork()) < 0) {
        printf("can't fork!\r\n");
        return;
    } else if (cpid) {
        if (prompt("List command for remote system? ", buf)) {
            close(pdes[0]), close(pdes[1]);
            kill (cpid, SIGKILL);
        } else {
            close(pdes[0]);
            signal(SIGPIPE, intcopy);
            transfer(buf, pdes[1], value(EOFREAD));
            signal(SIGPIPE, SIG_DFL);
            while ((p = wait(&status)) > 0 && p != cpid)
                ;
        }
    } else {
        register int f;

        dup2(pdes[0], 0);
        close(pdes[0]);
        for (f = 3; f < 20; f++)
            close(f);
        execute(buf);
        printf("can't execl!\r\n");
        exit(0);
    }
}

/*
 * FTP - send single character
 *  wait for echo & handle timeout
 */
static void send(int c)
{
    char cc;
    int retry = 0;

    cc = c;
    pwrite(FD, &cc, 1);
#ifdef notdef
    if (number(value(CDELAY)) > 0 && c != '\r')
        nap(number(value(CDELAY)));
#endif
    if (!boolean(value(ECHOCHECK))) {
#ifdef notdef
        if (number(value(LDELAY)) > 0 && c == '\r')
            nap(number(value(LDELAY)));
#endif
        return;
    }
tryagain:
    timedout = 0;
    alarm(number(value(ETIMEOUT)));
    read(FD, &cc, 1);
    alarm(0);
    if (timedout) {
        printf("\r\ntimeout error (%s)\r\n", ctrl(c));
        if (retry++ > 3)
            return;
        pwrite(FD, &null, 1); /* poke it */
        goto tryagain;
    }
}

/*
 * Bulk transfer routine to remote host --
 *   used by sendfile() and cu_put()
 */
void transmit(FILE *fd, char *eofchars, char *command)
{
    char *pc, lastc;
    int c, ccount, lcount;
    time_t start_t, stop_t;
    sig_t f;

    kill(pid, SIGIOT);  /* put TIPOUT into a wait state */
    stop = 0;
    f = signal(SIGINT, stopsnd);
    ioctl(0, TIOCSETC, &defchars);
    read(repdes[0], (char *)&ccc, 1);
    if (command != NULL) {
        for (pc = command; *pc; pc++)
            send(*pc);
        if (boolean(value(ECHOCHECK)))
            read(FD, (char *)&c, 1);    /* trailing \n */
        else {
            struct sgttyb buf;

            ioctl(FD, TIOCGETP, &buf);  /* this does a */
            ioctl(FD, TIOCSETP, &buf);  /*   wflushtty */
            sleep(5); /* wait for remote stty to take effect */
        }
    }
    lcount = 0;
    lastc = '\0';
    start_t = time(0);
    while (1) {
        ccount = 0;
        do {
            c = getc(fd);
            if (stop)
                goto out;
            if (c == EOF)
                goto out;
            if (c == 0177 && !boolean(value(RAWFTP)))
                continue;
            lastc = c;
            if (c < 040) {
                if (c == '\n') {
                    if (!boolean(value(RAWFTP)))
                        c = '\r';
                }
                else if (c == '\t') {
                    if (!boolean(value(RAWFTP))) {
                        if (boolean(value(TABEXPAND))) {
                            send(' ');
                            while ((++ccount % 8) != 0)
                                send(' ');
                            continue;
                        }
                    }
                } else
                    if (!boolean(value(RAWFTP)))
                        continue;
            }
            send(c);
        } while (c != '\r' && !boolean(value(RAWFTP)));
        if (boolean(value(VERBOSE)))
            printf("\r%d", ++lcount);
        if (boolean(value(ECHOCHECK))) {
            timedout = 0;
            alarm(number(value(ETIMEOUT)));
            do {    /* wait for prompt */
                read(FD, (char *)&c, 1);
                if (timedout || stop) {
                    if (timedout)
                        printf("\r\ntimed out at eol\r\n");
                    alarm(0);
                    goto out;
                }
            } while ((c&0177) != character(value(PROMPT)));
            alarm(0);
        }
    }
out:
    if (lastc != '\n' && !boolean(value(RAWFTP)))
        send('\r');
    for (pc = eofchars; *pc; pc++)
        send(*pc);
    stop_t = time(0);
    fclose(fd);
    signal(SIGINT, f);
    if (boolean(value(VERBOSE))) {
        if (boolean(value(RAWFTP)))
            prtime(" chars transferred in ", stop_t-start_t);
        else
            prtime(" lines transferred in ", stop_t-start_t);
    }
    write(fildes[1], (char *)&ccc, 1);
    ioctl(0, TIOCSETC, &tchars);
}

/*
 * FTP - local ==> remote
 *  send local file to remote host
 *  terminate transmission with pseudo EOF sequence
 */
void sendfile(char cc)
{
    FILE *fd;
    char *fnamex;

    putchar(cc);
    /*
     * get file name
     */
    if (prompt("Local file name? ", fname))
        return;

    /*
     * look up file
     */
    fnamex = expand(fname);
    if ((fd = fopen(fnamex, "r")) == NULL) {
        printf("%s: cannot open\r\n", fname);
        return;
    }
    transmit(fd, value(EOFWRITE), NULL);
    if (!boolean(value(ECHOCHECK))) {
        struct sgttyb buf;

        ioctl(FD, TIOCGETP, &buf);  /* this does a */
        ioctl(FD, TIOCSETP, &buf);  /*   wflushtty */
    }
}

/*
 * Cu-like put command
 */
void cu_put(int cc)
{
    FILE *fd;
    char line[BUFSIZ];
    int argc;
    char *cpynamex;

    if (prompt("[put] ", copyname))
        return;
    argc = args(copyname, argv);
    if (argc < 1 || argc > 2) {
        printf("usage: <put> from [to]\r\n");
        return;
    }
    if (argc == 1)
        argv[1] = argv[0];
    cpynamex = expand(argv[0]);
    if ((fd = fopen(cpynamex, "r")) == NULL) {
        printf("%s: cannot open\r\n", cpynamex);
        return;
    }
    if (boolean(value(ECHOCHECK)))
        sprintf(line, "cat>%s\r", argv[1]);
    else
        sprintf(line, "stty -echo;cat>%s;stty echo\r", argv[1]);
    transmit(fd, "\04", line);
}

/*
 * Stolen from consh() -- puts a remote file on the output of a local command.
 *  Identical to consh() except for where stdout goes.
 */
void pipeout(int c)
{
    char buf[256];
    int cpid, status, p;
    time_t start;

    putchar(c);
    if (prompt("Local command? ", buf))
        return;
    kill(pid, SIGIOT);  /* put TIPOUT into a wait state */
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    ioctl(0, TIOCSETC, &defchars);
    read(repdes[0], (char *)&ccc, 1);
    /*
     * Set up file descriptors in the child and
     *  let it go...
     */
    cpid = fork();
    if (cpid < 0)
        printf("can't fork!\r\n");
    else if (cpid) {
        start = time(0);
        while ((p = wait(&status)) > 0 && p != cpid)
            ;
        if (boolean(value(VERBOSE)))
            prtime("away for ", time(0)-start);
    } else {
        register int i;

        dup2(FD, 1);
        for (i = 3; i < 20; i++)
            close(i);
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        execute(buf);
        printf("can't find `%s'\r\n", buf);
        exit(0);
    }
    write(fildes[1], (char *)&ccc, 1);
    ioctl(0, TIOCSETC, &tchars);
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
}

#ifdef CONNECT
/*
 * Fork a program with:
 *  0 <-> local tty in
 *  1 <-> local tty out
 *  2 <-> local tty out
 *  3 <-> remote tty in
 *  4 <-> remote tty out
 */
void consh(int c)
{
    char buf[256];
    int cpid, status, p;
    time_t start;

    putchar(c);
    if (prompt("Local command? ", buf))
        return;
    kill(pid, SIGIOT);  /* put TIPOUT into a wait state */
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    ioctl(0, TIOCSETC, &defchars);
    read(repdes[0], (char *)&ccc, 1);
    /*
     * Set up file descriptors in the child and
     *  let it go...
     */
    if ((cpid = fork()) < 0)
        printf("can't fork!\r\n");
    else if (cpid) {
        start = time(0);
        while ((p = wait(&status)) > 0 && p != cpid)
            ;
        if (boolean(value(VERBOSE)))
            prtime("away for ", time(0)-start);
    } else {
        register int i;

        dup2(FD, 3);
        dup2(3, 4);
        for (i = 5; i < 20; i++)
            close(i);
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        execute(buf);
        printf("can't find `%s'\r\n", buf);
        exit(0);
    }
    write(fildes[1], (char *)&ccc, 1);
    ioctl(0, TIOCSETC, &tchars);
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
}
#endif

/*
 * Escape to local shell
 */
void shell()
{
    int shpid, status;
    char *cp;

    printf("[sh]\r\n");
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    unraw();
    shpid = fork();
    if (shpid != 0) {
        while (shpid != wait(&status));
        raw();
        printf("\r\n!\r\n");
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        return;
    } else {
        signal(SIGQUIT, SIG_DFL);
        signal(SIGINT, SIG_DFL);
        if ((cp = strrchr(value(SHELL), '/')) == NULL)
            cp = value(SHELL);
        else
            cp++;
        shell_uid();
        execl(value(SHELL), cp, (char*) 0);
        printf("\r\ncan't execl!\r\n");
        exit(1);
    }
}

/*
 * TIPIN portion of scripting
 *   initiate the conversation with TIPOUT
 */
void setscript()
{
    char c;
    /*
     * enable TIPOUT side for dialogue
     */
    kill(pid, SIGEMT);
    if (boolean(value(SCRIPT)))
        write(fildes[1], value(RECORD), size(value(RECORD)));
    write(fildes[1], "\n", 1);
    /*
     * wait for TIPOUT to finish
     */
    read(repdes[0], &c, 1);
    if (c == 'n')
        printf("can't create %s\r\n", value(RECORD));
}

/*
 * Change current working directory of
 *   local portion of tip
 */
void chdirectory()
{
    char dirname[80];
    register char *cp = dirname;

    if (prompt("[cd] ", dirname)) {
        if (stoprompt)
            return;
        cp = value(HOME);
    }
    if (chdir(cp) < 0)
        printf("%s: bad directory\r\n", cp);
    printf("!\r\n");
}

void tipabort(char *msg)
{
    kill(pid, SIGTERM);
    disconnect(msg);
    if (msg != NOSTR)
        printf("\r\n%s", msg);
    printf("\r\n[EOT]\r\n");
    daemon_uid();
    delock(uucplock);
    unraw();
    exit(0);
}

void finish()
{
    char *dismsg;

    if ((dismsg = value(DISCONNECT)) != NOSTR) {
        write(FD, dismsg, strlen(dismsg));
        sleep(5);
    }
    tipabort(NOSTR);
}

/*
 * Turn tandem mode on or off for remote tty.
 */
static void tandem(char *option)
{
    struct sgttyb rmtty;

    ioctl(FD, TIOCGETP, &rmtty);
    if (strcmp(option,"on") == 0) {
        rmtty.sg_flags |= TANDEM;
        arg.sg_flags |= TANDEM;
    } else {
        rmtty.sg_flags &= ~TANDEM;
        arg.sg_flags &= ~TANDEM;
    }
    ioctl(FD, TIOCSETP, &rmtty);
    ioctl(0,  TIOCSETP, &arg);
}

void variable()
{
    char    buf[256];

    if (prompt("[set] ", buf))
        return;
    vlex(buf);
    if (vtable[BEAUTIFY].v_access&CHANGED) {
        vtable[BEAUTIFY].v_access &= ~CHANGED;
        kill(pid, SIGSYS);
    }
    if (vtable[SCRIPT].v_access&CHANGED) {
        vtable[SCRIPT].v_access &= ~CHANGED;
        setscript();
        /*
         * So that "set record=blah script" doesn't
         *  cause two transactions to occur.
         */
        if (vtable[RECORD].v_access&CHANGED)
            vtable[RECORD].v_access &= ~CHANGED;
    }
    if (vtable[RECORD].v_access&CHANGED) {
        vtable[RECORD].v_access &= ~CHANGED;
        if (boolean(value(SCRIPT)))
            setscript();
    }
    if (vtable[TAND].v_access&CHANGED) {
        vtable[TAND].v_access &= ~CHANGED;
        if (boolean(value(TAND)))
            tandem("on");
        else
            tandem("off");
    }
    if (vtable[LECHO].v_access&CHANGED) {
        vtable[LECHO].v_access &= ~CHANGED;
        HD = boolean(value(LECHO));
    }
    if (vtable[PARITY].v_access&CHANGED) {
        vtable[PARITY].v_access &= ~CHANGED;
        setparity(NOSTR);
    }
}

/*
 * Send a break.
 */
void genbrk()
{
    ioctl(FD, TIOCSBRK, NULL);
    sleep(1);
    ioctl(FD, TIOCCBRK, NULL);
}

/*
 * Suspend tip
 */
void suspend(int c)
{
    unraw();
    kill(c == CTRL(y) ? getpid() : 0, SIGTSTP);
    raw();
}


/*
 * Are any of the characters in the two strings the same?
 */
static int anyof(char *s1, char *s2)
{
    register int c;

    while ((c = *s1++))
        if (any(c, s2))
            return(1);
    return(0);
}
/*
 *  expand a file name if it includes shell meta characters
 */
char *
expand(char name[])
{
    static char xname[BUFSIZ];
    char cmdbuf[BUFSIZ];
    register int pid, l;
    register char *cp, *Shell;
    int s, pivec[2];

    if (! anyof(name, "~{[*?$`'\"\\"))
        return(name);
    /* sigint = signal(SIGINT, SIG_IGN); */
    if (pipe(pivec) < 0) {
        perror("pipe");
        /* signal(SIGINT, sigint) */
        return(name);
    }
    sprintf(cmdbuf, "echo %s", name);
    if ((pid = vfork()) == 0) {
        Shell = value(SHELL);
        if (Shell == NOSTR)
            Shell = "/bin/sh";
        close(pivec[0]);
        close(1);
        dup(pivec[1]);
        close(pivec[1]);
        close(2);
        shell_uid();
        execl(Shell, Shell, "-c", cmdbuf, (char*)0);
        _exit(1);
    }
    if (pid == -1) {
        perror("fork");
        close(pivec[0]);
        close(pivec[1]);
        return(NOSTR);
    }
    close(pivec[1]);
    l = read(pivec[0], xname, BUFSIZ);
    close(pivec[0]);
    while (wait(&s) != pid)
        ;
    s &= 0377;
    if (s != 0 && s != SIGPIPE) {
        fprintf(stderr, "\"Echo\" failed\n");
        return(NOSTR);
    }
    if (l < 0) {
        perror("read");
        return(NOSTR);
    }
    if (l == 0) {
        fprintf(stderr, "\"%s\": No match\n", name);
        return(NOSTR);
    }
    if (l == BUFSIZ) {
        fprintf(stderr, "Buffer overflow expanding \"%s\"\n", name);
        return(NOSTR);
    }
    xname[l] = 0;
    for (cp = &xname[l-1]; *cp == '\n' && cp > xname; cp--)
        ;
    *++cp = '\0';
    return(xname);
}
