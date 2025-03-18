/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Tape Archival Program
 */
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/dir.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>

#define TBLOCK 512
#define NBLOCK 20
#define NAMSIZ 100

#define writetape(b) writetbuf(b, 1)
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

union hblock {
    char dummy[TBLOCK];
    struct header {
        char name[NAMSIZ];
        char mode[8];
        char uid[8];
        char gid[8];
        char size[12];
        char mtime[12];
        char chksum[8];
        char linkflag;
        char linkname[NAMSIZ];
    } dbuf;
};

struct linkbuf {
    ino_t inum;
    dev_t devnum;
    int count;
    char pathname[NAMSIZ];
    struct linkbuf *nextp;
};

union hblock dblock;
union hblock *tbuf;
struct linkbuf *ihead;
struct stat stbuf;

int rflag;
int xflag;
int vflag;
int tflag;
int cflag;
int mflag;
int fflag;
int iflag;
int oflag;
int pflag;
int wflag;
int hflag;
int Bflag;
int Fflag;

int mt;
int term;
int chksum;
int recno;
int first;
int prtlinkerr;
int freemem = 1;
int nblock = 0;

daddr_t low;
daddr_t high;
daddr_t bsrch(char *s, int n, daddr_t l, daddr_t h);

FILE *vfile = stdout;
FILE *tfile;
char tname[] = "/tmp/tarXXXXXX";
char *usefile;
char magtape[] = "/dev/rmt8";

static void usage(void);
static void done(int n);
static int openmt(char *tape, int writing);
static void dorep(char *argv[]);
static void doxtract(char *argv[]);
static void dotable(char *argv[]);
static void getdir(void);
static void passtape(void);
static int endtape(void);
static void backtape(void);
static void putfile(char *longname, char *shortname, char *parent);
static void putempty(void);
static void flushtape(void);
static int readtape(char *buffer);
static int checksum(void);
static int readtbuf(char **bufpp, int size);
static int checkupdate(char *arg);
static int checkw(int c, char *name);
static int checkf(char *name, int mode, int howmuch);
static void tomodes(struct stat *sp);
static int writetbuf(char *buffer, int n);
static int wantit(char *argv[]);
static int checkdir(char *name);
static void dodirtimes(union hblock *hp);
static void setimes(char *path, time_t mt);
static void longt(struct stat *st);
static void pmode(struct stat *st);
static void selectbits(int *pairp, struct stat *st);
static int response(void);
static int prefix(char *s1, char *s2);
static int cmp(char *b, char *s, int n);
static void getbuf(void);
static int bread(int fd, char *buf, int size);
static void mterr(char *operation, int i, int exitcode);
daddr_t lookup(char *s);

void onintr(int sig)
{
    (void)signal(SIGINT, SIG_IGN);
    term++;
}

void onquit(int sig)
{
    (void)signal(SIGQUIT, SIG_IGN);
    term++;
}

void onhup(int sig)
{
    (void)signal(SIGHUP, SIG_IGN);
    term++;
}

#ifdef notdef
void onterm(int sig)
{
    (void)signal(SIGTERM, SIG_IGN);
    term++;
}
#endif

int main(int argc, char *argv[])
{
    char *cp;

    if (argc < 2)
        usage();

    tfile = NULL;
    usefile = magtape;
    argv[argc] = 0;
    argv++;
    for (cp = *argv++; *cp; cp++)
        switch (*cp) {
        case 'f':
            if (*argv == 0) {
                fprintf(stderr, "tar: tapefile must be specified with 'f' option\n");
                usage();
            }
            usefile = *argv++;
            fflag++;
            break;

        case 'c':
            cflag++;
            rflag++;
            break;

        case 'o':
            oflag++;
            break;

        case 'p':
            pflag++;
            break;

        case 'u':
            mktemp(tname);
            if ((tfile = fopen(tname, "w")) == NULL) {
                fprintf(stderr, "tar: cannot create temporary file (%s)\n", tname);
                done(1);
            }
            fprintf(tfile, "!!!!!/!/!/!/!/!/!/! 000\n");
            /*FALL THRU*/

        case 'r':
            rflag++;
            break;

        case 'v':
            vflag++;
            break;

        case 'w':
            wflag++;
            break;

        case 'x':
            xflag++;
            break;

        case 't':
            tflag++;
            break;

        case 'm':
            mflag++;
            break;

        case '-':
            break;

        case '0':
        case '1':
        case '4':
        case '5':
        case '7':
        case '8':
            magtape[8] = *cp;
            usefile = magtape;
            break;

        case 'b':
            if (*argv == 0) {
                fprintf(stderr, "tar: blocksize must be specified with 'b' option\n");
                usage();
            }
            nblock = atoi(*argv);
            if (nblock <= 0) {
                fprintf(stderr, "tar: invalid blocksize \"%s\"\n", *argv);
                done(1);
            }
            argv++;
            break;

        case 'l':
            prtlinkerr++;
            break;

        case 'h':
            hflag++;
            break;

        case 'i':
            iflag++;
            break;

        case 'B':
            Bflag++;
            break;

        case 'F':
            Fflag++;
            break;

        default:
            fprintf(stderr, "tar: %c: unknown option\n", *cp);
            usage();
        }

    if (!rflag && !xflag && !tflag)
        usage();
    if (rflag) {
        if (cflag && tfile != NULL)
            usage();
        if (signal(SIGINT, SIG_IGN) != SIG_IGN)
            (void)signal(SIGINT, onintr);
        if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
            (void)signal(SIGHUP, onhup);
        if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
            (void)signal(SIGQUIT, onquit);
#ifdef notdef
        if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
            (void)signal(SIGTERM, onterm);
#endif
        mt = openmt(usefile, 1);
        dorep(argv);
        done(0);
    }
    mt = openmt(usefile, 0);
    if (xflag)
        doxtract(argv);
    else
        dotable(argv);
    done(0);
}

void usage()
{
    fprintf(stderr,
            "tar: usage: tar -{txru}[cvfblmhopwBi] [tapefile] [blocksize] file1 file2...\n");
    done(1);
}

int openmt(char *tape, int writing)
{
    if (strcmp(tape, "-") == 0) {
        /*
         * Read from standard input or write to standard output.
         */
        if (writing) {
            if (cflag == 0) {
                fprintf(stderr, "tar: can only create standard output archives\n");
                done(1);
            }
            vfile = stderr;
            setlinebuf(vfile);
            mt = dup(1);
        } else {
            mt = dup(0);
            Bflag++;
        }
    } else {
        /*
         * Use file or tape on local machine.
         */
        if (writing) {
            if (cflag)
                mt = open(tape, O_RDWR | O_CREAT | O_TRUNC, 0666);
            else
                mt = open(tape, O_RDWR);
        } else
            mt = open(tape, O_RDONLY);
        if (mt < 0) {
            fprintf(stderr, "tar: ");
            perror(tape);
            done(1);
        }
    }
    return (mt);
}

char *getcwd(char *buf)
{
    if (getwd(buf) == NULL) {
        fprintf(stderr, "tar: %s\n", buf);
        exit(1);
    }
    return (buf);
}

void dorep(char *argv[])
{
    char *cp, *cp2;
    char wdir[MAXPATHLEN], tempdir[MAXPATHLEN], *parent;

    if (!cflag) {
        getdir();
        do {
            passtape();
            if (term)
                done(0);
            getdir();
        } while (!endtape());
        backtape();
        if (tfile != NULL) {
            char buf[200];

            sprintf(
                buf,
                "sort +0 -1 +1nr %s -o %s; awk '$1 != prev {print; prev=$1}' %s >%sX; mv %sX %s",
                tname, tname, tname, tname, tname, tname);
            fflush(tfile);
            system(buf);
            freopen(tname, "r", tfile);
            fstat(fileno(tfile), &stbuf);
            high = stbuf.st_size;
        }
    }

    (void)getcwd(wdir);
    while (*argv && !term) {
        cp2 = *argv;
        if (!strcmp(cp2, "-C") && argv[1]) {
            argv++;
            if (chdir(*argv) < 0) {
                fprintf(stderr, "tar: can't change directories to ");
                perror(*argv);
            } else
                (void)getcwd(wdir);
            argv++;
            continue;
        }

        if (*argv[0] == '/') {
            parent = "";
        } else {
            parent = wdir;
        }

        for (cp = *argv; *cp; cp++)
            if (*cp == '/')
                cp2 = cp;
        if (cp2 != *argv) {
            *cp2 = '\0';
            if (chdir(*argv) < 0) {
                fprintf(stderr, "tar: can't change directories to ");
                perror(*argv);
                continue;
            }
            parent = getcwd(tempdir);
            *cp2 = '/';
            cp2++;
        }
        putfile(*argv++, cp2, parent);
        if (chdir(wdir) < 0) {
            fprintf(stderr, "tar: cannot change back?: ");
            perror(wdir);
        }
    }
    putempty();
    putempty();
    flushtape();
    if (prtlinkerr == 0)
        return;
    for (; ihead != NULL; ihead = ihead->nextp) {
        if (ihead->count == 0)
            continue;
        fprintf(stderr, "tar: missing links to %s\n", ihead->pathname);
    }
}

int endtape()
{
    return (dblock.dbuf.name[0] == '\0');
}

void getdir()
{
    struct stat *sp;
    int i;
top:
    readtape((char *)&dblock);
    if (dblock.dbuf.name[0] == '\0')
        return;
    sp = &stbuf;
    sscanf(dblock.dbuf.mode, "%o", &i);
    sp->st_mode = i;
    sscanf(dblock.dbuf.uid, "%o", &i);
    sp->st_uid = i;
    sscanf(dblock.dbuf.gid, "%o", &i);
    sp->st_gid = i;
    sscanf(dblock.dbuf.size, "%lo", &sp->st_size);
    sscanf(dblock.dbuf.mtime, "%lo", &sp->st_mtime);
    sscanf(dblock.dbuf.chksum, "%o", &chksum);
    if (chksum != (i = checksum())) {
        fprintf(stderr, "tar: directory checksum error (%d != %d)\n", chksum, i);
        if (iflag)
            goto top;
        done(2);
    }
    if (tfile != NULL)
        fprintf(tfile, "%s %s\n", dblock.dbuf.name, dblock.dbuf.mtime);
}

void passtape()
{
    long blocks;
    char *bufp;

    if (dblock.dbuf.linkflag == '1')
        return;
    blocks = stbuf.st_size;
    blocks += TBLOCK - 1;
    blocks /= TBLOCK;

    while (blocks-- > 0)
        (void)readtbuf(&bufp, TBLOCK);
}

char *getmem(int size)
{
    char *p = malloc((unsigned)size);

    if (p == NULL && freemem) {
        fprintf(stderr, "tar: out of memory, link and directory modtime info lost\n");
        freemem = 0;
    }
    return (p);
}

void putfile(char *longname, char *shortname, char *parent)
{
    int infile = 0;
    long blocks;
    char buf[TBLOCK];
    char *bigbuf;
    char *cp;
    struct direct *dp;
    DIR *dirp;
    int i;
    long l;
    char newparent[NAMSIZ + 64];
    extern int errno;
    int maxread;
    int hint; /* amount to write to get "in sync" */

    if (!hflag)
        i = lstat(shortname, &stbuf);
    else
        i = stat(shortname, &stbuf);
    if (i < 0) {
        fprintf(stderr, "tar: ");
        perror(longname);
        return;
    }
    if (tfile != NULL && checkupdate(longname) == 0)
        return;
    if (checkw('r', longname) == 0)
        return;
    if (Fflag && checkf(shortname, stbuf.st_mode, Fflag) == 0)
        return;

    switch (stbuf.st_mode & S_IFMT) {
    case S_IFDIR:
        for (i = 0, cp = buf; (*cp++ = longname[i++]);)
            ;
        *--cp = '/';
        *++cp = 0;
        if (!oflag) {
            if ((cp - buf) >= NAMSIZ) {
                fprintf(stderr, "tar: %s: file name too long\n", longname);
                return;
            }
            stbuf.st_size = 0;
            tomodes(&stbuf);
            strcpy(dblock.dbuf.name, buf);
            sprintf(dblock.dbuf.chksum, "%6o", checksum());
            (void)writetape((char *)&dblock);
        }
        sprintf(newparent, "%s/%s", parent, shortname);
        if (chdir(shortname) < 0) {
            perror(shortname);
            return;
        }
        if ((dirp = opendir(".")) == NULL) {
            fprintf(stderr, "tar: %s: directory read error\n", longname);
            if (chdir(parent) < 0) {
                fprintf(stderr, "tar: cannot change back?: ");
                perror(parent);
            }
            return;
        }
        while ((dp = readdir(dirp)) != NULL && !term) {
            if (dp->d_ino == 0)
                continue;
            if (!strcmp(".", dp->d_name) || !strcmp("..", dp->d_name))
                continue;
            strcpy(cp, dp->d_name);
            l = telldir(dirp);
            closedir(dirp);
            putfile(buf, cp, newparent);
            dirp = opendir(".");
            seekdir(dirp, l);
        }
        closedir(dirp);
        if (chdir(parent) < 0) {
            fprintf(stderr, "tar: cannot change back?: ");
            perror(parent);
        }
        break;

    case S_IFLNK:
        tomodes(&stbuf);
        if (strlen(longname) >= NAMSIZ) {
            fprintf(stderr, "tar: %s: file name too long\n", longname);
            return;
        }
        strcpy(dblock.dbuf.name, longname);
        if (stbuf.st_size + 1 >= NAMSIZ) {
            fprintf(stderr, "tar: %s: symbolic link too long\n", longname);
            return;
        }
        i = readlink(shortname, dblock.dbuf.linkname, NAMSIZ - 1);
        if (i < 0) {
            fprintf(stderr, "tar: can't read symbolic link ");
            perror(longname);
            return;
        }
        dblock.dbuf.linkname[i] = '\0';
        dblock.dbuf.linkflag = '2';
        if (vflag)
            fprintf(vfile, "a %s symbolic link to %s\n", longname, dblock.dbuf.linkname);
        sprintf(dblock.dbuf.size, "%11lo", 0L);
        sprintf(dblock.dbuf.chksum, "%6o", checksum());
        (void)writetape((char *)&dblock);
        break;

    case S_IFREG:
        if ((infile = open(shortname, 0)) < 0) {
            fprintf(stderr, "tar: ");
            perror(longname);
            return;
        }
        tomodes(&stbuf);
        if (strlen(longname) >= NAMSIZ) {
            fprintf(stderr, "tar: %s: file name too long\n", longname);
            close(infile);
            return;
        }
        strcpy(dblock.dbuf.name, longname);
        if (stbuf.st_nlink > 1) {
            struct linkbuf *lp;
            int found = 0;

            for (lp = ihead; lp != NULL; lp = lp->nextp)
                if (lp->inum == stbuf.st_ino && lp->devnum == stbuf.st_dev) {
                    found++;
                    break;
                }
            if (found) {
                strcpy(dblock.dbuf.linkname, lp->pathname);
                dblock.dbuf.linkflag = '1';
                sprintf(dblock.dbuf.chksum, "%6o", checksum());
                (void)writetape((char *)&dblock);
                if (vflag)
                    fprintf(vfile, "a %s link to %s\n", longname, lp->pathname);
                lp->count--;
                close(infile);
                return;
            }
            lp = (struct linkbuf *)getmem(sizeof(*lp));
            if (lp != NULL) {
                lp->nextp = ihead;
                ihead = lp;
                lp->inum = stbuf.st_ino;
                lp->devnum = stbuf.st_dev;
                lp->count = stbuf.st_nlink - 1;
                strcpy(lp->pathname, longname);
            }
        }
        blocks = (stbuf.st_size + (TBLOCK - 1)) / TBLOCK;
        if (vflag)
            fprintf(vfile, "a %s %ld blocks\n", longname, blocks);
        sprintf(dblock.dbuf.chksum, "%6o", checksum());
        hint = writetape((char *)&dblock);
        maxread = max(stbuf.st_blksize, (nblock * TBLOCK));
        if ((bigbuf = malloc((unsigned)maxread)) == 0) {
            maxread = TBLOCK;
            bigbuf = buf;
        }

        while ((i = read(infile, bigbuf, min((hint * TBLOCK), maxread))) > 0 && blocks > 0) {
            int nblks;

            nblks = ((i - 1) / TBLOCK) + 1;
            if (nblks > blocks)
                nblks = blocks;
            hint = writetbuf(bigbuf, nblks);
            blocks -= nblks;
        }
        close(infile);
        if (bigbuf != buf)
            free(bigbuf);
        if (i < 0) {
            fprintf(stderr, "tar: Read error on ");
            perror(longname);
        } else if (blocks != 0 || i != 0)
            fprintf(stderr, "tar: %s: file changed size\n", longname);
        while (--blocks >= 0)
            putempty();
        break;

    default:
        fprintf(stderr, "tar: %s is not a file. Not dumped\n", longname);
        break;
    }
}

void doxtract(char *argv[])
{
    long blocks, bytes;
    int ofile, i;

    for (;;) {
        if ((i = wantit(argv)) == 0)
            continue;
        if (i == -1)
            break; /* end of tape */
        if (checkw('x', dblock.dbuf.name) == 0) {
            passtape();
            continue;
        }
        if (Fflag) {
            char *s;

            if ((s = rindex(dblock.dbuf.name, '/')) == 0)
                s = dblock.dbuf.name;
            else
                s++;
            if (checkf(s, stbuf.st_mode, Fflag) == 0) {
                passtape();
                continue;
            }
        }
        if (checkdir(dblock.dbuf.name)) { /* have a directory */
            if (mflag == 0)
                dodirtimes(&dblock);
            continue;
        }
        if (dblock.dbuf.linkflag == '2') { /* symlink */
            /*
             * only unlink non directories or empty
             * directories
             */
            if (rmdir(dblock.dbuf.name) < 0) {
                if (errno == ENOTDIR)
                    unlink(dblock.dbuf.name);
            }
            if (symlink(dblock.dbuf.linkname, dblock.dbuf.name) < 0) {
                fprintf(stderr, "tar: %s: symbolic link failed: ", dblock.dbuf.name);
                perror("");
                continue;
            }
            if (vflag)
                fprintf(vfile, "x %s symbolic link to %s\n", dblock.dbuf.name,
                        dblock.dbuf.linkname);
#ifdef notdef
            /* ignore alien orders */
            chown(dblock.dbuf.name, stbuf.st_uid, stbuf.st_gid);
            if (mflag == 0)
                setimes(dblock.dbuf.name, stbuf.st_mtime);
            if (pflag)
                chmod(dblock.dbuf.name, stbuf.st_mode & 07777);
#endif
            continue;
        }
        if (dblock.dbuf.linkflag == '1') { /* regular link */
            /*
             * only unlink non directories or empty
             * directories
             */
            if (rmdir(dblock.dbuf.name) < 0) {
                if (errno == ENOTDIR)
                    unlink(dblock.dbuf.name);
            }
            if (link(dblock.dbuf.linkname, dblock.dbuf.name) < 0) {
                fprintf(stderr, "tar: can't link %s to %s: ", dblock.dbuf.name,
                        dblock.dbuf.linkname);
                perror("");
                continue;
            }
            if (vflag)
                fprintf(vfile, "%s linked to %s\n", dblock.dbuf.name, dblock.dbuf.linkname);
            continue;
        }
        if ((ofile = creat(dblock.dbuf.name, stbuf.st_mode & 0xfff)) < 0) {
            fprintf(stderr, "tar: can't create %s: ", dblock.dbuf.name);
            perror("");
            passtape();
            continue;
        }
        chown(dblock.dbuf.name, stbuf.st_uid, stbuf.st_gid);
        blocks = ((bytes = stbuf.st_size) + TBLOCK - 1) / TBLOCK;
        if (vflag)
            fprintf(vfile, "x %s, %ld bytes, %ld tape blocks\n", dblock.dbuf.name, bytes, blocks);
        for (; blocks > 0;) {
            int nread;
            char *bufp;
            int nwant;

            nwant = NBLOCK * TBLOCK;
            if (nwant > (blocks * TBLOCK))
                nwant = (blocks * TBLOCK);
            nread = readtbuf(&bufp, nwant);
            if (write(ofile, bufp, (int)min(nread, bytes)) < 0) {
                fprintf(stderr, "tar: %s: HELP - extract write error", dblock.dbuf.name);
                perror("");
                done(2);
            }
            bytes -= nread;
            blocks -= (((nread - 1) / TBLOCK) + 1);
        }
        close(ofile);
        if (mflag == 0)
            setimes(dblock.dbuf.name, stbuf.st_mtime);
        if (pflag)
            chmod(dblock.dbuf.name, stbuf.st_mode & 07777);
    }
    if (mflag == 0) {
        dblock.dbuf.name[0] = '\0'; /* process the whole stack */
        dodirtimes(&dblock);
    }
}

void dotable(char *argv[])
{
    int i;

    for (;;) {
        if ((i = wantit(argv)) == 0)
            continue;
        if (i == -1)
            break; /* end of tape */
        if (vflag)
            longt(&stbuf);
        printf("%s", dblock.dbuf.name);
        if (dblock.dbuf.linkflag == '1')
            printf(" linked to %s", dblock.dbuf.linkname);
        if (dblock.dbuf.linkflag == '2')
            printf(" symbolic link to %s", dblock.dbuf.linkname);
        printf("\n");
        passtape();
    }
}

void putempty()
{
    char buf[TBLOCK];

    bzero(buf, sizeof(buf));
    (void)writetape(buf);
}

void longt(struct stat *st)
{
    char *cp;

    pmode(st);
    printf("%3d/%1d", st->st_uid, st->st_gid);
    printf("%7ld", st->st_size);
    cp = ctime(&st->st_mtime);
    printf(" %-12.12s %-4.4s ", cp + 4, cp + 20);
}

#define SUID 04000
#define SGID 02000
#define ROWN 0400
#define WOWN 0200
#define XOWN 0100
#define RGRP 040
#define WGRP 020
#define XGRP 010
#define ROTH 04
#define WOTH 02
#define XOTH 01
#define STXT 01000

int m1[] = { 1, ROWN, 'r', '-' };
int m2[] = { 1, WOWN, 'w', '-' };
int m3[] = { 2, SUID, 's', XOWN, 'x', '-' };
int m4[] = { 1, RGRP, 'r', '-' };
int m5[] = { 1, WGRP, 'w', '-' };
int m6[] = { 2, SGID, 's', XGRP, 'x', '-' };
int m7[] = { 1, ROTH, 'r', '-' };
int m8[] = { 1, WOTH, 'w', '-' };
int m9[] = { 2, STXT, 't', XOTH, 'x', '-' };

int *m[] = { m1, m2, m3, m4, m5, m6, m7, m8, m9 };

void pmode(struct stat *st)
{
    int **mp;

    for (mp = &m[0]; mp < &m[9];)
        selectbits(*mp++, st);
}

void selectbits(int *pairp, struct stat *st)
{
    int n, *ap;

    ap = pairp;
    n = *ap++;
    while (--n >= 0 && (st->st_mode & *ap++) == 0)
        ap++;
    putchar(*ap);
}

/*
 * Make all directories needed by `name'.  If `name' is itself
 * a directory on the tar tape (indicated by a trailing '/'),
 * return 1; else 0.
 */
int checkdir(char *name)
{
    char *cp;

    /*
     * Quick check for existence of directory.
     */
    if ((cp = rindex(name, '/')) == 0)
        return (0);
    *cp = '\0';
    if (access(name, 0) == 0) { /* already exists */
        *cp = '/';
        return (cp[1] == '\0'); /* return (lastchar == '/') */
    }
    *cp = '/';

    /*
     * No luck, try to make all directories in path.
     */
    for (cp = name; *cp; cp++) {
        if (*cp != '/')
            continue;
        *cp = '\0';
        if (access(name, 0) < 0) {
            if (mkdir(name, 0777) < 0) {
                perror(name);
                *cp = '/';
                return (0);
            }
            chown(name, stbuf.st_uid, stbuf.st_gid);
            if (pflag && cp[1] == '\0') /* dir on the tape */
                chmod(name, stbuf.st_mode & 07777);
        }
        *cp = '/';
    }
    return (cp[-1] == '/');
}

void tomodes(struct stat *sp)
{
    char *cp;

    for (cp = dblock.dummy; cp < &dblock.dummy[TBLOCK]; cp++)
        *cp = '\0';
    sprintf(dblock.dbuf.mode, "%6o ", sp->st_mode & 07777);
    sprintf(dblock.dbuf.uid, "%6o ", sp->st_uid);
    sprintf(dblock.dbuf.gid, "%6o ", sp->st_gid);
    sprintf(dblock.dbuf.size, "%11lo ", sp->st_size);
    sprintf(dblock.dbuf.mtime, "%11lo ", sp->st_mtime);
}

int checksum()
{
    int i;
    char *cp;

    for (cp = dblock.dbuf.chksum; cp < &dblock.dbuf.chksum[sizeof(dblock.dbuf.chksum)]; cp++)
        *cp = ' ';
    i = 0;
    for (cp = dblock.dummy; cp < &dblock.dummy[TBLOCK]; cp++)
        i += *cp;
    return (i);
}

int checkw(int c, char *name)
{
    if (!wflag)
        return (1);
    printf("%c ", c);
    if (vflag)
        longt(&stbuf);
    printf("%s: ", name);
    return (response() == 'y');
}

int response()
{
    char c;

    c = getchar();
    if (c != '\n')
        while (getchar() != '\n')
            ;
    else
        c = 'n';
    return (c);
}

int checkf(char *name, int mode, int howmuch)
{
    int l;

    if ((mode & S_IFMT) == S_IFDIR) {
        if ((strcmp(name, "SCCS") == 0) || (strcmp(name, "RCS") == 0))
            return (0);
        return (1);
    }
    if ((l = strlen(name)) < 3)
        return (1);
    if (howmuch > 1 && name[l - 2] == '.' && name[l - 1] == 'o')
        return (0);
    if (strcmp(name, "core") == 0 || strcmp(name, "errs") == 0 ||
        (howmuch > 1 && strcmp(name, "a.out") == 0))
        return (0);
    /* SHOULD CHECK IF IT IS EXECUTABLE */
    return (1);
}

/* Is the current file a new file, or the newest one of the same name? */
int checkupdate(char *arg)
{
    char name[100];
    long mtime;
    daddr_t seekp;

    rewind(tfile);
    for (;;) {
        if ((seekp = lookup(arg)) < 0)
            return (1);
        fseek(tfile, seekp, 0);
        fscanf(tfile, "%s %lo", name, &mtime);
        return (stbuf.st_mtime > mtime);
    }
}

void done(int n)
{
    unlink(tname);
    exit(n);
}

/*
 * Do we want the next entry on the tape, i.e. is it selected?  If
 * not, skip over the entire entry.  Return -1 if reached end of tape.
 */
int wantit(char *argv[])
{
    char **cp;

    getdir();
    if (endtape())
        return (-1);
    if (*argv == 0)
        return (1);
    for (cp = argv; *cp; cp++)
        if (prefix(*cp, dblock.dbuf.name))
            return (1);
    passtape();
    return (0);
}

/*
 * Does s2 begin with the string s1, on a directory boundary?
 */
int prefix(char *s1, char *s2)
{
    while (*s1)
        if (*s1++ != *s2++)
            return (0);
    if (*s2)
        return (*s2 == '/');
    return (1);
}

#define N 200
int njab;

daddr_t lookup(char *s)
{
    int i;
    daddr_t a;

    for (i = 0; s[i]; i++)
        if (s[i] == ' ')
            break;
    a = bsrch(s, i, low, high);
    return (a);
}

daddr_t bsrch(char *s, int n, daddr_t l, daddr_t h)
{
    int i, j;
    char b[N];
    daddr_t m, m1;

    njab = 0;

loop:
    if (l >= h)
        return ((daddr_t)-1);
    m = l + (h - l) / 2 - N / 2;
    if (m < l)
        m = l;
    fseek(tfile, m, 0);
    fread(b, 1, N, tfile);
    njab++;
    for (i = 0; i < N; i++) {
        if (b[i] == '\n')
            break;
        m++;
    }
    if (m >= h)
        return ((daddr_t)-1);
    m1 = m;
    j = i;
    for (i++; i < N; i++) {
        m1++;
        if (b[i] == '\n')
            break;
    }
    i = cmp(b + j, s, n);
    if (i < 0) {
        h = m;
        goto loop;
    }
    if (i > 0) {
        l = m1;
        goto loop;
    }
    return (m);
}

int cmp(char *b, char *s, int n)
{
    int i;

    if (b[0] != '\n')
        exit(2);
    for (i = 0; i < n; i++) {
        if (b[i + 1] > s[i])
            return (-1);
        if (b[i + 1] < s[i])
            return (1);
    }
    return (b[i + 1] == ' ' ? 0 : -1);
}

int readtape(char *buffer)
{
    char *bufp;

    if (first == 0)
        getbuf();
    (void)readtbuf(&bufp, TBLOCK);
    bcopy(bufp, buffer, TBLOCK);
    return (TBLOCK);
}

int readtbuf(char **bufpp, int size)
{
    int i;

    if (recno >= nblock || first == 0) {
        if ((i = bread(mt, (char *)tbuf, TBLOCK * nblock)) < 0)
            mterr("read", i, 3);
        if (first == 0) {
            if ((i % TBLOCK) != 0) {
                fprintf(stderr, "tar: tape blocksize error\n");
                done(3);
            }
            i /= TBLOCK;
            if (i != nblock) {
                fprintf(stderr, "tar: blocksize = %d\n", i);
                nblock = i;
            }
            first = 1;
        }
        recno = 0;
    }
    if (size > ((nblock - recno) * TBLOCK))
        size = (nblock - recno) * TBLOCK;
    *bufpp = (char *)&tbuf[recno];
    recno += (size / TBLOCK);
    return (size);
}

int writetbuf(char *buffer, int n)
{
    int i;

    if (first == 0) {
        getbuf();
        first = 1;
    }
    if (recno >= nblock) {
        i = write(mt, (char *)tbuf, TBLOCK * nblock);
        if (i != TBLOCK * nblock)
            mterr("write", i, 2);
        recno = 0;
    }

    /*
     *  Special case:  We have an empty tape buffer, and the
     *  users data size is >= the tape block size:  Avoid
     *  the bcopy and dma direct to tape.  BIG WIN.  Add the
     *  residual to the tape buffer.
     */
    while (recno == 0 && n >= nblock) {
        i = write(mt, buffer, TBLOCK * nblock);
        if (i != TBLOCK * nblock)
            mterr("write", i, 2);
        n -= nblock;
        buffer += (nblock * TBLOCK);
    }

    while (n-- > 0) {
        bcopy(buffer, (char *)&tbuf[recno++], TBLOCK);
        buffer += TBLOCK;
        if (recno >= nblock) {
            i = write(mt, (char *)tbuf, TBLOCK * nblock);
            if (i != TBLOCK * nblock)
                mterr("write", i, 2);
            recno = 0;
        }
    }

    /* Tell the user how much to write to get in sync */
    return (nblock - recno);
}

void backtape()
{
    static int mtdev = 1;
    static struct mtop mtop = { MTBSR, 1 };
    struct mtget mtget;

    if (mtdev == 1)
        mtdev = ioctl(mt, MTIOCGET, (char *)&mtget);
    if (mtdev == 0) {
        if (ioctl(mt, MTIOCTOP, (char *)&mtop) < 0) {
            fprintf(stderr, "tar: tape backspace error: ");
            perror("");
            done(4);
        }
    } else
        lseek(mt, (daddr_t)-TBLOCK * nblock, 1);
    recno--;
}

void flushtape()
{
    int i;

    i = write(mt, (char *)tbuf, TBLOCK * nblock);
    if (i != TBLOCK * nblock)
        mterr("write", i, 2);
}

void mterr(char *operation, int i, int exitcode)
{
    fprintf(stderr, "tar: tape %s error: ", operation);
    if (i < 0)
        perror("");
    else
        fprintf(stderr, "unexpected EOF\n");
    done(exitcode);
}

int bread(int fd, char *buf, int size)
{
    int count;
    static int lastread = 0;

    if (!Bflag)
        return (read(fd, buf, size));

    for (count = 0; count < size; count += lastread) {
        lastread = read(fd, buf, size - count);
        if (lastread <= 0) {
            if (count > 0)
                return (count);
            return (lastread);
        }
        buf += lastread;
    }
    return (count);
}

void getbuf()
{
    if (nblock == 0) {
        fstat(mt, &stbuf);
        if ((stbuf.st_mode & S_IFMT) == S_IFCHR)
            nblock = NBLOCK;
        else {
            nblock = stbuf.st_blksize / TBLOCK;
            if (nblock == 0)
                nblock = NBLOCK;
        }
    }
    tbuf = (union hblock *)malloc((unsigned)nblock * TBLOCK);
    if (tbuf == NULL) {
        fprintf(stderr, "tar: blocksize %d too big, can't get memory\n", nblock);
        done(1);
    }
}

/*
 * Save this directory and its mtime on the stack, popping and setting
 * the mtimes of any stacked dirs which aren't parents of this one.
 * A null directory causes the entire stack to be unwound and set.
 *
 * Since all the elements of the directory "stack" share a common
 * prefix, we can make do with one string.  We keep only the current
 * directory path, with an associated array of mtime's, one for each
 * '/' in the path.  A negative mtime means no mtime.  The mtime's are
 * offset by one (first index 1, not 0) because calling this with a null
 * directory causes mtime[0] to be set.
 *
 * This stack algorithm is not guaranteed to work for tapes created
 * with the 'r' option, but the vast majority of tapes with
 * directories are not.  This avoids saving every directory record on
 * the tape and setting all the times at the end.
 */
char dirstack[NAMSIZ];
#define NTIM (NAMSIZ / 2 + 1) /* a/b/c/d/... */
time_t mtime[NTIM];

void dodirtimes(union hblock *hp)
{
    char *p = dirstack;
    char *q = hp->dbuf.name;
    int ndir = 0;
    char *savp;
    int savndir;

    /* Find common prefix */
    while (*p == *q) {
        if (*p++ == '/')
            ++ndir;
        q++;
    }

    savp = p;
    savndir = ndir;
    while (*p) {
        /*
         * Not a child: unwind the stack, setting the times.
         * The order we do this doesn't matter, so we go "forward."
         */
        if (*p++ == '/')
            if (mtime[++ndir] >= 0) {
                *--p = '\0'; /* zap the slash */
                setimes(dirstack, mtime[ndir]);
                *p++ = '/';
            }
    }
    p = savp;
    ndir = savndir;

    /* Push this one on the "stack" */
    while ((*p = *q++)) /* append the rest of the new dir */
        if (*p++ == '/')
            mtime[++ndir] = -1;
    mtime[ndir] = stbuf.st_mtime; /* overwrite the last one */
}

void setimes(char *path, time_t mt)
{
    struct timeval tv[2];

    tv[0].tv_sec = time((time_t *)0);
    tv[1].tv_sec = mt;
    tv[0].tv_usec = tv[1].tv_usec = 0;
    if (utimes(path, tv) < 0) {
        fprintf(stderr, "tar: can't set time on %s: ", path);
        perror("");
    }
}
