/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * tip - UNIX link to other systems
 *  tip [-v] [-speed] system-name
 * or
 *  cu phone-number [-s speed] [-l line] [-a acu]
 */
#include <stdlib.h>
#include <string.h>
#include "tip.h"

/*
 * Baud rate mapping table
 */

int bauds[] = {
    0, 50, 75, 150, 200, 300, 600,
    1200, 1800, 2400, 4800, 9600, 19200,
    38400, 57600, 115200, 230400, 460800,
    500000, 576000, 921600, 1000000, 1152000,
    1500000, 2000000, 2500000, 3000000,
    3500000, 4000000, -1
};

int disc = 0; // OTTYDISC;      /* tip normally runs this way */
char    PNbuf[256];         /* This limits the size of a number */

char *DV;        /* UNIX device(s) to open */
char *EL;        /* chars marking an EOL */
char *CM;        /* initial connection message */
char *IE;        /* EOT to expect on input */
char *OE;        /* EOT to send to complete FT */
char *CU;        /* call unit if making a phone call */
char *AT;        /* acu type */
char *PN;        /* phone number(s) */
char *DI;        /* disconnect string */
char *PA;        /* parity to be generated */
char *PH;        /* phone number file */
char *HO;        /* host name */
int BR;          /* line speed for conversation */
int FS;          /* frame size for transfers */
char DU;         /* this host is dialed up */
char HW;         /* this device is hardwired, see hunt.c */
char *ES;        /* escape character */
char *EX;        /* exceptions */
char *FO;        /* force (literal next) char*/
char *RC;        /* raise character */
char *RE;        /* script record file */
char *PR;        /* remote prompt */
int DL;          /* line delay for file transfers to remote */
int CL;          /* char delay for file transfers to remote */
int ET;          /* echocheck timeout */
char HD;         /* this host is half duplex - do local echo */

int      vflag;      /* verbose during reading of .tiprc file */

struct sgttyb   arg;        /* current mode of local terminal */
struct sgttyb   defarg;     /* initial mode of local terminal */
struct tchars   tchars;     /* current state of terminal */
struct tchars   defchars;   /* initial state of terminal */
struct ltchars  ltchars;    /* current local characters of terminal */
struct ltchars  deflchars;  /* initial local characters of terminal */

FILE    *fscript;    /* FILE for scripting */
int fildes[2];       /* file transfer synchronization channel */
int repdes[2];       /* read process sychronization channel */
int FD;              /* open file descriptor to remote host */
int AC;              /* open file descriptor to dialer (v831 only) */
int vflag;           /* print .tiprc initialization sequence */
int sfd;             /* for ~< operation */
int pid;             /* pid of tipout */
uid_t uid, euid;     /* real and effective user id's */
gid_t gid, egid;     /* real and effective group id's */
int stop;            /* stop transfer session flag */
int quit;            /* same; but on other end */
int intflag;         /* recognized interrupt */
int stoprompt;       /* for interrupting a prompt session */
int timedout;        /* ~> transfer timedout */
int cumode;          /* simulating the "cu" program */
char fname[80];      /* file name buffer for ~< */
char copyname[80];   /* file name buffer for ~> */
char ccc;            /* synchronization character */
char ch;             /* for tipout */
char *uucplock;      /* name of lock file for uucp's */
int odisc;           /* initial tty line discipline */
int disc;            /* current tty discpline */

void intprompt(int i);
void timeout(int i);
void cleanup(int i);
char *sname();

/*
 * ****TIPIN   TIPIN****
 */
static void tipin()
{
    char gch, bol = 1;

    /*
     * Kinda klugey here...
     *   check for scripting being turned on from the .tiprc file,
     *   but be careful about just using setscript(), as we may
     *   send a SIGEMT before tipout has a chance to set up catching
     *   it; so wait a second, then setscript()
     */
    if (boolean(value(SCRIPT))) {
        sleep(1);
        setscript();
    }

    while (1) {
        gch = getchar()&0177;
        if ((gch == character(value(ESCAPE))) && bol) {
            if (!(gch = escape()))
                continue;
        } else if (!cumode && gch == character(value(RAISECHAR))) {
            boolean(value(RAISE)) = !boolean(value(RAISE));
            continue;
        } else if (gch == '\r') {
            bol = 1;
            pwrite(FD, &gch, 1);
            if (boolean(value(HALFDUPLEX)))
                printf("\r\n");
            continue;
        } else if (!cumode && gch == character(value(FORCE)))
            gch = getchar()&0177;
        bol = any(gch, value(EOL));
        if (boolean(value(RAISE)) && islower(gch))
            gch = toupper(gch);
        pwrite(FD, &gch, 1);
        if (boolean(value(HALFDUPLEX)))
            printf("%c", gch);
    }
}

int main(argc, argv)
    char *argv[];
{
    char *system = NOSTR;
    register int i;
    register char *p;
    char sbuf[12];

    gid = getgid();
    egid = getegid();
    uid = getuid();
    euid = geteuid();
    if (equal(sname(argv[0]), "cu")) {
        cumode = 1;
        cumain(argc, argv);
        goto cucommon;
    }

    if (argc > 4) {
        fprintf(stderr, "usage: tip [-v] [-speed] [system-name]\n");
        exit(1);
    }
    if (!isatty(0)) {
        fprintf(stderr, "tip: must be interactive\n");
        exit(1);
    }

    for (; argc > 1; argv++, argc--) {
        if (argv[1][0] != '-')
            system = argv[1];
        else switch (argv[1][1]) {

        case 'v':
            vflag++;
            break;

        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            BR = atoi(&argv[1][1]);
            break;

        default:
            fprintf(stderr, "tip: %s, unknown option\n", argv[1]);
            break;
        }
    }

    if (system == NOSTR)
        goto notnumber;
    if (isalpha(*system))
        goto notnumber;
    /*
     * System name is really a phone number...
     * Copy the number then stomp on the original (in case the number
     *  is private, we don't want 'ps' or 'w' to find it).
     */
    if (strlen(system) > sizeof PNbuf - 1) {
        fprintf(stderr, "tip: phone number too long (max = %ld bytes)\n",
            (long) sizeof PNbuf - 1);
        exit(1);
    }
    strncpy( PNbuf, system, sizeof PNbuf - 1 );
    for (p = system; *p; p++)
        *p = '\0';
    PN = PNbuf;
    (void)sprintf(sbuf, "tip%d", BR);
    system = sbuf;

notnumber:
    signal(SIGINT, cleanup);
    signal(SIGQUIT, cleanup);
    signal(SIGHUP, cleanup);
    signal(SIGTERM, cleanup);

    if ((i = hunt(system)) == 0) {
        printf("all ports busy\n");
        exit(3);
    }
    if (i == -1) {
        printf("link down\n");
        delock(uucplock);
        exit(3);
    }
    setbuf(stdout, NULL);
    loginit();

    /*
     * Kludge, their's no easy way to get the initialization
     *   in the right order, so force it here
     */
    if ((PH = getenv("PHONES")) == NOSTR)
        PH = "/etc/phones";
    vinit();                /* init variables */
    setparity("even");          /* set the parity table */
    if ((i = speed(number(value(BAUDRATE)))) == NULL) {
        printf("tip: bad baud rate %d\n", number(value(BAUDRATE)));
        delock(uucplock);
        exit(3);
    }

    /*
     * Now that we have the logfile and the ACU open
     *  return to the real uid and gid.  These things will
     *  be closed on exit.  Swap real and effective uid's
     *  so we can get the original permissions back
     *  for removing the uucp lock.
     */
    user_uid();

    /*
     * Hardwired connections require the
     *  line speed set before they make any transmissions
     *  (this is particularly true of things like a DF03-AC)
     */
    if (HW)
        ttysetup(i);
    p = connect();
    if (p) {
        printf("\07%s\n[EOT]\n", p);
        daemon_uid();
        delock(uucplock);
        exit(1);
    }
    if (!HW)
        ttysetup(i);
cucommon:
    /*
     * From here down the code is shared with
     * the "cu" version of tip.
     */

    ioctl(0, TIOCGETP, (char *)&defarg);
    ioctl(0, TIOCGETC, (char *)&defchars);
    ioctl(0, TIOCGLTC, (char *)&deflchars);
    ioctl(0, TIOCGETD, (char *)&odisc);
    arg = defarg;
    arg.sg_flags = ANYP | CBREAK;
    tchars = defchars;
    tchars.t_intrc = tchars.t_quitc = -1;
    ltchars = deflchars;
    ltchars.t_suspc = ltchars.t_dsuspc = ltchars.t_flushc
        = ltchars.t_lnextc = -1;
    raw();

    pipe(fildes); pipe(repdes);
    signal(SIGALRM, timeout);

    /*
     * Everything's set up now:
     *  connection established (hardwired or dialup)
     *  line conditioned (baud rate, mode, etc.)
     *  internal data structures (variables)
     * so, fork one process for local side and one for remote.
     */
    printf(cumode ? "Connected\r\n" : "\07connected\r\n");
    pid = fork();
    if (pid)
        tipin();
    else
        tipout();
    /*NOTREACHED*/
    return 0;
}

void cleanup(int i)
{
    daemon_uid();
    delock(uucplock);
    if (odisc)
        ioctl(0, TIOCSETD, (char *)&odisc);
    exit(0);
}

/*
 * Muck with user ID's.  We are setuid to the owner of the lock
 * directory when we start.  user_uid() reverses real and effective
 * ID's after startup, to run with the user's permissions.
 * daemon_uid() switches back to the privileged uid for unlocking.
 * Finally, to avoid running a shell with the wrong real uid,
 * shell_uid() sets real and effective uid's to the user's real ID.
 */
static int uidswapped;

void user_uid()
{
    if (uidswapped == 0) {
        setregid(egid, gid);
        setreuid(euid, uid);
        uidswapped = 1;
    }
}

void daemon_uid()
{

    if (uidswapped) {
        setreuid(uid, euid);
        setregid(gid, egid);
        uidswapped = 0;
    }
}

void shell_uid()
{
    setreuid(uid, uid);
    setregid(gid, gid);
}

/*
 * put the controlling keyboard into raw mode
 */
void raw()
{
    ioctl(0, TIOCSETP, &arg);
    ioctl(0, TIOCSETC, &tchars);
    ioctl(0, TIOCSLTC, &ltchars);
    ioctl(0, TIOCSETD, (char *)&disc);
}


/*
 * return keyboard to normal mode
 */
void unraw()
{
    ioctl(0, TIOCSETD, (char *)&odisc);
    ioctl(0, TIOCSETP, (char *)&defarg);
    ioctl(0, TIOCSETC, (char *)&defchars);
    ioctl(0, TIOCSLTC, (char *)&deflchars);
}

static  jmp_buf promptbuf;

/*
 * Print string ``s'', then read a string
 *  in from the terminal.  Handles signals & allows use of
 *  normal erase and kill characters.
 */
int prompt(s, p)
    char *s;
    register char *p;
{
    register char *b = p;
    sig_t oint;

    stoprompt = 0;
    oint = signal(SIGINT, intprompt);
    oint = signal(SIGQUIT, SIG_IGN);
    unraw();
    printf("%s", s);
    if (setjmp(promptbuf) == 0)
        while ((*p = getchar()) != EOF && *p != '\n')
            p++;
    *p = '\0';

    raw();
    signal(SIGINT, oint);
    signal(SIGQUIT, oint);
    return (stoprompt || p == b);
}

/*
 * Interrupt service routine during prompting
 */
void intprompt(int i)
{

    signal(SIGINT, SIG_IGN);
    stoprompt = 1;
    printf("\r\n");
    longjmp(promptbuf, 1);
}

/*
 * Escape handler --
 *  called on recognition of ``escapec'' at the beginning of a line
 */
int escape()
{
    register int gch;
    register esctable_t *p;
    char c = character(value(ESCAPE));
    extern esctable_t etable[];

    gch = (getchar()&0177);
    for (p = etable; p->e_char; p++)
        if (p->e_char == gch) {
            if ((p->e_flags&PRIV) && uid)
                continue;
            printf("%s", ctrl(c));
            (*p->e_func)(gch);
            return (0);
        }
    /* ESCAPE ESCAPE forces ESCAPE */
    if (c != gch)
        pwrite(FD, &c, 1);
    return (gch);
}

int speed(n)
    int n;
{
    register int *p;

    for (p = bauds; *p != -1;  p++)
        if (*p == n)
            return (p - bauds);
    return (NULL);
}

int any(c, p)
    register int c;
    register char *p;
{
    while (p && *p)
        if (*p++ == c)
            return (1);
    return (0);
}

int size(s)
    register char   *s;
{
    register int i = 0;

    while (s && *s++)
        i++;
    return (i);
}

char *
interp(s)
    register char *s;
{
    static char buf[256];
    register char *p = buf, c, *q;

    while ((c = *s++)) {
        for (q = "\nn\rr\tt\ff\033E\bb"; *q; q++)
            if (*q++ == c) {
                *p++ = '\\'; *p++ = *q;
                goto next;
            }
        if (c < 040) {
            *p++ = '^'; *p++ = c + 'A'-1;
        } else if (c == 0177) {
            *p++ = '^'; *p++ = '?';
        } else
            *p++ = c;
    next:
        ;
    }
    *p = '\0';
    return (buf);
}

char *
ctrl(c)
    int c;
{
    static char s[3];

    if (c < 040 || c == 0177) {
        s[0] = '^';
        s[1] = c == 0177 ? '?' : c+'A'-1;
        s[2] = '\0';
    } else {
        s[0] = c;
        s[1] = '\0';
    }
    return (s);
}

/*
 * Help command
 */
void help(c)
    char c;
{
    register esctable_t *p;
    extern esctable_t etable[];

    printf("%c\r\n", c);
    for (p = etable; p->e_char; p++) {
        if ((p->e_flags&PRIV) && uid)
            continue;
        printf("%2s", ctrl(character(value(ESCAPE))));
        printf("%-2s %c   %s\r\n", ctrl(p->e_char),
            p->e_flags&EXP ? '*': ' ', p->e_help);
    }
}

/*
 * Set up the "remote" tty's state
 */
void ttysetup(speed)
    int speed;
{
    unsigned bits = LDECCTQ;

    arg.sg_ispeed = arg.sg_ospeed = speed;
    arg.sg_flags = RAW;
    if (boolean(value(TAND)))
        arg.sg_flags |= TANDEM;
    ioctl(FD, TIOCSETP, (char *)&arg);
    ioctl(FD, TIOCLBIS, (char *)&bits);
}

/*
 * Return "simple" name from a file name,
 * strip leading directories.
 */
char *
sname(s)
    register char *s;
{
    register char *p = s;

    while (*s)
        if (*s++ == '/')
            p = s;
    return (p);
}

static char partab[0200];

/*
 * Do a write to the remote machine with the correct parity.
 * We are doing 8 bit wide output, so we just generate a character
 * with the right parity and output it.
 */
void pwrite(fd, buf, n)
    int fd;
    char *buf;
    register int n;
{
    //register int i;
//    register char *bp;
    extern int errno;

//    bp = buf;
    //for (i = 0; i < n; i++) {
    //    *bp = partab[(*bp) & 0177];
    //    bp++;
    //}
    if (write(fd, buf, n) < 0) {
        if (errno == EIO)
            tipabort("Lost carrier.");
        /* this is questionable */
        perror("write");
    }
}

/*
 * Build a parity table with appropriate high-order bit.
 */
void setparity(defparity)
    char *defparity;
{
    register int i;
    char *parity;
    extern char evenpartab[];

    if (defparity != NOSTR && value(PARITY) == NOSTR)
        value(PARITY) = defparity;
    parity = value(PARITY);
    for (i = 0; i < 0200; i++)
        partab[i] = evenpartab[i];
    if (equal(parity, "even"))
        return;
    if (equal(parity, "odd")) {
        for (i = 0; i < 0200; i++)
            partab[i] ^= 0200;  /* reverse bit 7 */
        return;
    }
    if (equal(parity, "none") || equal(parity, "zero")) {
        for (i = 0; i < 0200; i++)
            partab[i] &= ~0200; /* turn off bit 7 */
        return;
    }
    if (equal(parity, "one")) {
        for (i = 0; i < 0200; i++)
            partab[i] |= 0200;  /* turn on bit 7 */
        return;
    }
    fprintf(stderr, "%s: unknown parity value\n", PA);
    fflush(stderr);
}
