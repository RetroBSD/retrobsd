#include <ctype.h>
#include <paths.h>
#include <pwd.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sysexits.h>
#include <time.h>
#include <unistd.h>
#include <utmp.h>
#include <fcntl.h>

/* copylet flags */
#define REMOTE 1 /* remote mail, add rmtmsg */
#define ORDINARY 2
#define ZAP 3 /* zap header and trailing empty line */
#define FORWARD 4

#define LSIZE 256
#define MAXLET 300    /* maximum number of letters */
#define MAILMODE 0600 /* mode of created mail */

char line[LSIZE];
char resp[LSIZE];

struct let {
    long adr;
    char change;
} let[MAXLET];

int nlet = 0;
char lfil[50];
long iop;
char lettmp[] = "/tmp/maXXXXX";
char maildir[] = _PATH_MAIL;
char mailfile[] = _PATH_MAIL "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
char dead[] = "dead.letter";
char forwmsg[] = " forwarded\n";
FILE *tmpf;
FILE *malf;
char my_name[60];
int error;
int changed;
int forward;
char from[] = "From ";
int flgf;
int flgp;
int delflg = 1;
int hseqno;
jmp_buf sjbuf;
int rmail;

static void done(void);
static void setsig(int i, sig_t f);
static void delex(int i);
static void panic(char *msg, ...);
static int any(int c, char *str);
static void printmail(int argc, char **argv);
static void bulkmail(int argc, char **argv);
static void cat(char *to, char *from1, char *from2);
static void copymt(FILE *f1, FILE *f2);
static void copylet(int n, FILE *f, int type);
static char *getarg(char *s, char *p);
static int sendmail(int n, char *name, char *fromaddr);
static void copyback(void);
static int isfrom(char *lp);
static void usage(void);
static int safefile(char *f);

int main(int argc, char **argv)
{
    int i;
    char *name;
    struct passwd *pwent;

    if (!(name = getlogin()) || !*name || !(pwent = getpwnam(name)) || getuid() != pwent->pw_uid)
        pwent = getpwuid(getuid());
    strncpy(my_name, pwent ? pwent->pw_name : "???", sizeof(my_name) - 1);
    if (setjmp(sjbuf))
        done();
    for (i = SIGHUP; i <= SIGTERM; i++)
        setsig(i, delex);
    i = mkstemp(lettmp);
    tmpf = fdopen(i, "r+w");
    if (i < 0 || tmpf == NULL)
        panic("mail: %s: cannot open for writing", lettmp);
    /*
     * This protects against others reading mail from temp file and
     * if we exit, the file will be deleted already.
     */
    unlink(lettmp);
    if (argv[0][0] == 'r')
        rmail++;
    if (argv[0][0] != 'r' && /* no favors for rmail*/
        (argc == 1 || argv[1][0] == '-' && !any(argv[1][1], "rhd")))
        printmail(argc, argv);
    else
        bulkmail(argc, argv);
    done();
}

void setsig(int i, sig_t f)
{
    if (signal(i, SIG_IGN) != SIG_IGN)
        signal(i, f);
}

int any(int c, char *str)
{
    while (*str)
        if (c == *str++)
            return (1);
    return (0);
}

void printmail(int argc, char **argv)
{
    int flg, i, j, print;
    char *p;
    struct stat statb;

    setuid(getuid());
    cat(mailfile, maildir, my_name);
    for (; argc > 1; argv++, argc--) {
        if (argv[1][0] != '-')
            break;
        switch (argv[1][1]) {
        case 'p':
            flgp++;
            /* fall thru... */
        case 'q':
            delflg = 0;
            break;

        case 'f':
            if (argc >= 3) {
                strcpy(mailfile, argv[2]);
                argv++, argc--;
            }
            break;

        case 'b':
            forward = 1;
            break;

        default:
            panic("unknown option %c", argv[1][1]);
            /*NOTREACHED*/
        }
    }
    malf = fopen(mailfile, "r");
    if (malf == NULL) {
        printf("No mail.\n");
        return;
    }
    flock(fileno(malf), LOCK_SH);
    copymt(malf, tmpf);
    fclose(malf); /* implicit unlock */
    fseek(tmpf, 0L, L_SET);

    changed = 0;
    print = 1;
    for (i = 0; i < nlet;) {
        j = forward ? i : nlet - i - 1;
        if (setjmp(sjbuf)) {
            print = 0;
        } else {
            if (print)
                copylet(j, stdout, ORDINARY);
            print = 1;
        }
        if (flgp) {
            i++;
            continue;
        }
        setjmp(sjbuf);
        fputs("? ", stdout);
        fflush(stdout);
        if (fgets(resp, LSIZE, stdin) == NULL)
            break;
        switch (resp[0]) {
        default:
            printf("usage\n");
        case '?':
            print = 0;
            printf("q\tquit\n");
            printf("x\texit without changing mail\n");
            printf("p\tprint\n");
            printf("s[file]\tsave (default mbox)\n");
            printf("w[file]\tsame without header\n");
            printf("-\tprint previous\n");
            printf("d\tdelete\n");
            printf("+\tnext (no delete)\n");
            printf("m user\tmail to user\n");
            printf("! cmd\texecute cmd\n");
            break;

        case '+':
        case 'n':
        case '\n':
            i++;
            break;
        case 'x':
            changed = 0;
        case 'q':
            goto donep;
        case 'p':
            break;
        case '^':
        case '-':
            if (--i < 0)
                i = 0;
            break;
        case 'y':
        case 'w':
        case 's':
            flg = 0;
            if (resp[1] != '\n' && resp[1] != ' ') {
                printf("illegal\n");
                flg++;
                print = 0;
                continue;
            }
            if (resp[1] == '\n' || resp[1] == '\0') {
                p = getenv("HOME");
                if (p != 0)
                    cat(resp + 1, p, "/mbox");
                else
                    cat(resp + 1, "", "mbox");
            }
            for (p = resp + 1; (p = getarg(lfil, p)) != NULL;) {
                malf = fopen(lfil, "a");
                if (malf == NULL) {
                    printf("mail: %s: cannot append\n", lfil);
                    flg++;
                    continue;
                }
                copylet(j, malf, resp[0] == 'w' ? ZAP : ORDINARY);
                fclose(malf);
            }
            if (flg)
                print = 0;
            else {
                let[j].change = 'd';
                changed++;
                i++;
            }
            break;
        case 'm':
            flg = 0;
            if (resp[1] == '\n' || resp[1] == '\0') {
                i++;
                continue;
            }
            if (resp[1] != ' ') {
                printf("invalid command\n");
                flg++;
                print = 0;
                continue;
            }
            for (p = resp + 1; (p = getarg(lfil, p)) != NULL;)
                if (!sendmail(j, lfil, my_name))
                    flg++;
            if (flg)
                print = 0;
            else {
                let[j].change = 'd';
                changed++;
                i++;
            }
            break;
        case '!':
            system(resp + 1);
            printf("!\n");
            print = 0;
            break;
        case 'd':
            let[j].change = 'd';
            changed++;
            i++;
            if (resp[1] == 'q')
                goto donep;
            break;
        }
    }
donep:
    if (changed)
        copyback();
}

/* copy temp or whatever back to /var/mail */
void copyback()
{
    int i, c;
    sigset_t set;
    int fd, new = 0;
    struct stat stbuf;

    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGHUP);
    sigaddset(&set, SIGQUIT);
    (void)sigprocmask(SIG_BLOCK, &set, NULL);
    fd = open(mailfile, O_RDWR | O_CREAT, MAILMODE);
    if (fd >= 0) {
        flock(fd, LOCK_EX);
        malf = fdopen(fd, "r+w");
    }
    if (fd < 0 || malf == NULL)
        panic("can't rewrite %s", lfil);
    fstat(fd, &stbuf);
    if (stbuf.st_size != let[nlet].adr) { /* new mail has arrived */
        fseek(malf, let[nlet].adr, L_SET);
        fseek(tmpf, let[nlet].adr, L_SET);
        while ((c = getc(malf)) != EOF)
            putc(c, tmpf);
        let[++nlet].adr = stbuf.st_size;
        new = 1;
        fseek(malf, 0L, L_SET);
    }
    ftruncate(fd, 0L);
    for (i = 0; i < nlet; i++)
        if (let[i].change != 'd')
            copylet(i, malf, ORDINARY);
    fclose(malf); /* implict unlock */
    if (new)
        printf("New mail has arrived.\n");
    (void)sigprocmask(SIG_UNBLOCK, &set, NULL);
}

/* copy mail (f1) to temp (f2) */
void copymt(FILE *f1, FILE *f2)
{
    long nextadr;

    nlet = nextadr = 0;
    let[0].adr = 0;
    while (fgets(line, LSIZE, f1) != NULL) {
        if (isfrom(line))
            let[nlet++].adr = nextadr;
        nextadr += strlen(line);
        fputs(line, f2);
    }
    let[nlet].adr = nextadr; /* last plus 1 */
}

void copylet(int n, FILE *f, int type)
{
    int ch;
    long k;
    char hostname[MAXHOSTNAMELEN];

    fseek(tmpf, let[n].adr, L_SET);
    k = let[n + 1].adr - let[n].adr;
    while (k-- > 1 && (ch = getc(tmpf)) != '\n')
        if (type != ZAP)
            putc(ch, f);
    switch (type) {
    case REMOTE:
        gethostname(hostname, sizeof(hostname));
        fprintf(f, " remote from %s\n", hostname);
        break;

    case FORWARD:
        fprintf(f, forwmsg);
        break;

    case ORDINARY:
        putc(ch, f);
        break;

    case ZAP:
        break;

    default:
        panic("Bad letter type %d to copylet.", type);
    }
    while (k-- > 1) {
        ch = getc(tmpf);
        putc(ch, f);
    }
    if (type != ZAP || ch != '\n')
        putc(getc(tmpf), f);
}

int isfrom(char *lp)
{
    char *p;

    for (p = from; *p;)
        if (*lp++ != *p++)
            return (0);
    return (1);
}

void bulkmail(int argc, char **argv)
{
    char *truename;
    int first;
    char *cp;
    char *newargv[1000];
    char **ap;
    char **vp;
    int dflag;

    dflag = 0;
    delflg = 0;
    if (argc < 1) {
        fprintf(stderr, "puke\n");
        return;
    }
    for (vp = argv, ap = newargv + 1; (*ap = *vp++) != 0; ap++)
        if (ap[0][0] == '-' && ap[0][1] == 'd')
            dflag++;
    if (!dflag) {
        /* give it to sendmail, rah rah! */
        unlink(lettmp);
        ap = newargv + 1;
        if (rmail)
            *ap-- = "-s";
        *ap = "-sendmail";
        setuid(getuid());
        execv(_PATH_SENDMAIL, ap);
        perror(_PATH_SENDMAIL);
        exit(EX_UNAVAILABLE);
    }

    truename = 0;
    line[0] = '\0';

    /*
     * When we fall out of this, argv[1] should be first name,
     * argc should be number of names + 1.
     */

    while (argc > 1 && *argv[1] == '-') {
        cp = *++argv;
        argc--;
        switch (cp[1]) {
        case 'r':
            if (argc <= 1)
                usage();
            truename = argv[1];
            fgets(line, LSIZE, stdin);
            if (strncmp("From", line, 4) == 0)
                line[0] = '\0';
            argv++;
            argc--;
            break;

        case 'h':
            if (argc <= 1)
                usage();
            hseqno = atoi(argv[1]);
            argv++;
            argc--;
            break;

        case 'd':
            break;

        default:
            usage();
        }
    }
    if (argc <= 1)
        usage();
    if (truename == 0)
        truename = my_name;
    time(&iop);
    fprintf(tmpf, "%s%s %s", from, truename, ctime(&iop));
    iop = ftell(tmpf);
    flgf = first = 1;
    for (;;) {
        if (first) {
            first = 0;
            if (*line == '\0' && fgets(line, LSIZE, stdin) == NULL)
                break;
        } else {
            if (fgets(line, LSIZE, stdin) == NULL)
                break;
        }
        if (*line == '.' && line[1] == '\n' && isatty(fileno(stdin)))
            break;
        if (isfrom(line))
            putc('>', tmpf);
        fputs(line, tmpf);
        flgf = 0;
    }
    putc('\n', tmpf);
    nlet = 1;
    let[0].adr = 0;
    let[1].adr = ftell(tmpf);
    if (flgf)
        return;
    while (--argc > 0)
        if (!sendmail(0, *++argv, truename))
            error++;
    if (error && safefile(dead)) {
        setuid(getuid());
        malf = fopen(dead, "w");
        if (malf == NULL) {
            printf("mail: cannot open %s\n", dead);
            fclose(tmpf);
            return;
        }
        copylet(0, malf, ZAP);
        fclose(malf);
        printf("Mail saved in %s\n", dead);
    }
    fclose(tmpf);
}

int sendrmt(int n, char *name)
{
    FILE *rmf, *popen();
    char *p;
    char rsys[64], cmd[64];
    int pid;
    int sts;

    for (p = rsys; *name != '!'; *p++ = *name++)
        if (*name == '\0')
            return (0); /* local address, no '!' */
    *p = '\0';
    if (name[1] == '\0') {
        printf("null name\n");
        return (0);
    }
skip:
    if ((pid = fork()) == -1) {
        fprintf(stderr, "mail: can't create proc for remote\n");
        return (0);
    }
    if (pid) {
        while (wait(&sts) != pid) {
            if (wait(&sts) == -1)
                return (0);
        }
        return (!sts);
    }
    setuid(getuid());
    if (any('!', name + 1))
        (void)sprintf(cmd, "uux - %s!rmail \\(%s\\)", rsys, name + 1);
    else
        (void)sprintf(cmd, "uux - %s!rmail %s", rsys, name + 1);
    if ((rmf = popen(cmd, "w")) == NULL)
        exit(1);
    copylet(n, rmf, REMOTE);
    exit(pclose(rmf) != 0);
}

void usage()
{
    fprintf(stderr, "Usage: mail [ -f ] people . . .\n");
    error = EX_USAGE;
    done();
}

int sendmail(int n, char *name, char *fromaddr)
{
    char file[256];
    int fd;
    struct passwd *pw;
    char buf[128];
    off_t oldsize;

    if (*name == '!')
        name++;
    if (any('!', name))
        return (sendrmt(n, name));
    if ((pw = getpwnam(name)) == NULL) {
        printf("mail: can't send to %s\n", name);
        return (0);
    }
    cat(file, maildir, name);
    if (!safefile(file))
        return (0);
    fd = open(file, O_WRONLY | O_CREAT | O_EXLOCK, MAILMODE);
    if (fd >= 0)
        malf = fdopen(fd, "a");
    if (fd < 0 || malf == NULL) {
        close(fd);
        printf("mail: %s: cannot append\n", file);
        return (0);
    }
    fchown(fd, pw->pw_uid, pw->pw_gid);
    oldsize = ftell(malf);
    (void)sprintf(buf, "%s@%ld\n", name, oldsize);

    copylet(n, malf, ORDINARY); /* Try to deliver the message */

    /* If there is any error during the delivery of the message,
     * the mail file may be corrupted (incomplete last line) and
     * any subsequent mail will be apparently lost, since the
     * <NL> before the 'From ' won't be there.  So, restore the
     * file to the pre-delivery size and report an error.
     *
     * fflush does "_flag |= _IOERR" so we don't need to check both the
     # return from fflush and the ferror status.
    */
    (void)fflush(malf);
    if (ferror(malf)) {
        printf("mail: %s: cannot append\n", file);
        ftruncate(fd, oldsize);
        fclose(malf);
        return (0);
    }
    fclose(malf);
    return (1);
}

void delex(int i)
{
    sigset_t sigt;

    if (i != SIGINT) {
        setsig(i, SIG_DFL);
        sigemptyset(&sigt);
        sigaddset(&sigt, i);
        sigprocmask(SIG_UNBLOCK, &sigt, NULL);
    }
    putc('\n', stderr);
    if (delflg)
        longjmp(sjbuf, 1);
    if (error == 0)
        error = i;
    done();
}

void done()
{
    unlink(lettmp);
    exit(error);
}

void cat(char *to, char *from1, char *from2)
{
    char *cp, *dp;

    cp = to;
    for (dp = from1; (*cp = *dp++); cp++)
        ;
    for (dp = from2; (*cp++ = *dp++);)
        ;
}

/* copy p... into s, update p */
char *getarg(char *s, char *p)
{
    while (*p == ' ' || *p == '\t')
        p++;
    if (*p == '\n' || *p == '\0')
        return (NULL);
    while (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\0')
        *s++ = *p++;
    *s = '\0';
    return (p);
}

int safefile(char *f)
{
    struct stat statb;

    if (lstat(f, &statb) < 0)
        return (1);
    if (statb.st_nlink != 1 || (statb.st_mode & S_IFMT) == S_IFLNK) {
        fprintf(stderr, "mail: %s has more than one link or is a symbolic link\n", f);
        return (0);
    }
    return (1);
}

void panic(char *msg, ...)
{
    va_list args;
    va_start(args, msg);
    fprintf(stderr, "mail: ");
    vfprintf(stderr, msg, args);
    fprintf(stderr, "\n");
    va_end(args);
    done();
}
