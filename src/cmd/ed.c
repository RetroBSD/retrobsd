/*
 * Editor
 */
#include <sgtty.h>
#include <signal.h>
#include <stdlib.h>
#undef CEOF
#include <setjmp.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define NULL 0
#define FNSIZE 64
#define LBSIZE 512
#define ESIZE 128
#define GBSIZE 256
#define NBRA 5
#define EOF -1

#define CBRA 1
#define CCHR 2
#define CDOT 4
#define CCL 6
#define NCCL 8
#define CDOL 10
#define CEOF 11
#define CKET 12
#define CBACK 14

#define STAR 01

char Q[] = "";
char T[] = "TMP";
#define READ 0
#define WRITE 1

int peekc;
int lastc;
char savedfile[FNSIZE];
char file[FNSIZE];
char linebuf[LBSIZE];
char rhsbuf[LBSIZE / 2];
char expbuf[ESIZE + 4];
int circfl;
int *zero;
int *dot;
int *dol;
int *addr1;
int *addr2;
char genbuf[LBSIZE];
long count;
char *nextip;
char *linebp;
int ninbuf;
int io;
int pflag;
sig_t oldhup;
sig_t oldquit;
int vflag = 1;

int listf;
int col;
char *globp;
int tfile = -1;
int tline;
char *tfname;
char *loc1;
char *loc2;
char *locs;
char ibuff[512];
int iblock = -1;
char obuff[512];
int oblock = -1;
int ichanged;
int nleft;
char WRERR[] = "WRITE ERROR";
int names[26];
int anymarks;
char *braslist[NBRA];
char *braelist[NBRA];
int nbra;
int subnewa;
int subolda;
int fchange;
int wrapp;
unsigned nlall = 128;

jmp_buf savej;

static void error(char *s);
static void putchr(int ac);
static void putstr(char *sp);
static void putfile(void);
static void init(void);
static void commands(void);
static int *address(void);
static int getchr(void);
static void setdot(void);
static void newline(void);
static int append(int (*f)(), int *a);
static void delete(void);
static void setnoaddr(void);
static void filename(int comm);
static void global(int k);
static void nonzero(void);
static void join(void);
static void move(int cflag);
static char *getline(int tl);
static void setall(void);
static void exfile(void);
static void substitute(int inglob);
static void putd(void);
static void callunix(void);
static void compile(int aeof);
static int execute(int gf, int *addr);
static int putline(void);
static void rdelete(int *ad1, int *ad2);
static char *getblock(int atl, int iof);
static void blkio(int b, char *buf, int (*iofcn)());
static int compsub(void);
static void dosub(void);
static int getsub(void);
static char *place(char *sp, char *l1, char *l2);
static int getcopy(void);
static void reverse(int *a1, int *a2);
static int advance(char *lp, char *ep);
static int cclass(char *set, char c, int af);
static int backref(int i, char *lp);

void quit(int sig)
{
    if (vflag && fchange && dol != zero) {
        fchange = 0;
        error(Q);
    }
    unlink(tfname);
    exit(0);
}

void onintr(int sig)
{
    signal(SIGINT, onintr);
    putchr('\n');
    lastc = '\n';
    error(Q);
}

void onhup(int sig)
{
    signal(SIGINT, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    if (dol > zero) {
        addr1 = zero + 1;
        addr2 = dol;
        io = creat("ed.hup", 0666);
        if (io > 0)
            putfile();
    }
    fchange = 0;
    quit(0);
}

int main(int argc, char **argv)
{
    char *p1, *p2;
    sig_t oldintr;

    oldquit = signal(SIGQUIT, SIG_IGN);
    oldhup = signal(SIGHUP, SIG_IGN);
    oldintr = signal(SIGINT, SIG_IGN);
    if ((int)signal(SIGTERM, SIG_IGN) == 0)
        signal(SIGTERM, quit);
    argv++;
    while (argc > 1 && **argv == '-') {
        switch ((*argv)[1]) {
        case '\0':
            vflag = 0;
            break;

        case 'q':
            signal(SIGQUIT, SIG_DFL);
            vflag = 1;
            break;
        }
        argv++;
        argc--;
    }

    if (argc > 1) {
        p1 = *argv;
        p2 = savedfile;
        while ((*p2++ = *p1++))
            ;
        globp = "r";
    }
    zero = (int *)malloc(nlall * sizeof(int));
    tfname = mktemp("/tmp/eXXXXX");
    if (!tfname) {
        putstr("ed: cannot create /tmp/file");
        exit(-1);
    }
    init();
    if (((int)oldintr & 01) == 0)
        signal(SIGINT, onintr);
    if (((int)oldhup & 01) == 0)
        signal(SIGHUP, onhup);
    setjmp(savej);
    commands();
    quit(0);
}

void commands()
{
    int getfile(), gettty();
    int *a1, c;

    for (;;) {
        if (pflag) {
            pflag = 0;
            addr1 = addr2 = dot;
            goto print;
        }
        addr1 = 0;
        addr2 = 0;
        do {
            addr1 = addr2;
            if ((a1 = address()) == 0) {
                c = getchr();
                break;
            }
            addr2 = a1;
            if ((c = getchr()) == ';') {
                c = ',';
                dot = a1;
            }
        } while (c == ',');
        if (addr1 == 0)
            addr1 = addr2;
        switch (c) {
        case 'a':
            setdot();
            newline();
            append(gettty, addr2);
            continue;

        case 'c':
            delete();
            append(gettty, addr1 - 1);
            continue;

        case 'd':
            delete();
            continue;

        case 'E':
            fchange = 0;
            c = 'e';
        case 'e':
            setnoaddr();
            if (vflag && fchange) {
                fchange = 0;
                error(Q);
            }
            filename(c);
            init();
            addr2 = zero;
            goto caseread;

        case 'f':
            setnoaddr();
            filename(c);
            putstr(savedfile);
            continue;

        case 'g':
            global(1);
            continue;

        case 'i':
            setdot();
            nonzero();
            newline();
            append(gettty, addr2 - 1);
            continue;

        case 'j':
            if (addr2 == 0) {
                addr1 = dot;
                addr2 = dot + 1;
            }
            setdot();
            newline();
            nonzero();
            join();
            continue;

        case 'k':
            if ((c = getchr()) < 'a' || c > 'z')
                error(Q);
            newline();
            setdot();
            nonzero();
            names[c - 'a'] = *addr2 & ~01;
            anymarks |= 01;
            continue;

        case 'm':
            move(0);
            continue;

        case '\n':
            if (addr2 == 0)
                addr2 = dot + 1;
            addr1 = addr2;
            goto print;

        case 'l':
            listf++;
        case 'p':
        case 'P':
            newline();
        print:
            setdot();
            nonzero();
            a1 = addr1;
            do {
                putstr(getline(*a1++));
            } while (a1 <= addr2);
            dot = addr2;
            listf = 0;
            continue;

        case 'Q':
            fchange = 0;
        case 'q':
            setnoaddr();
            newline();
            quit(0);

        case 'r':
            filename(c);
        caseread:
            if ((io = open(file, 0)) < 0) {
                lastc = '\n';
                error(file);
            }
            setall();
            ninbuf = 0;
            c = zero != dol;
            append(getfile, addr2);
            exfile();
            fchange = c;
            continue;

        case 's':
            setdot();
            nonzero();
            substitute(globp != 0);
            continue;

        case 't':
            move(1);
            continue;

        case 'u':
            setdot();
            nonzero();
            newline();
            if ((*addr2 & ~01) != subnewa)
                error(Q);
            *addr2 = subolda;
            dot = addr2;
            continue;

        case 'v':
            global(0);
            continue;

        case 'W':
            wrapp++;
        case 'w':
            setall();
            nonzero();
            filename(c);
            if (!wrapp || ((io = open(file, 1)) == -1) || ((lseek(io, 0L, 2)) == -1))
                if ((io = creat(file, 0666)) < 0)
                    error(file);
            wrapp = 0;
            putfile();
            exfile();
            if (addr1 == zero + 1 && addr2 == dol)
                fchange = 0;
            continue;

        case '=':
            setall();
            newline();
            count = (addr2 - zero) & 077777;
            putd();
            putchr('\n');
            continue;

        case '!':
            callunix();
            continue;

        case EOF:
            return;
        }
        error(Q);
    }
}

int *address()
{
    int *a1, minus, c;
    int n, relerr;

    minus = 0;
    a1 = 0;
    for (;;) {
        c = getchr();
        if ('0' <= c && c <= '9') {
            n = 0;
            do {
                n *= 10;
                n += c - '0';
            } while ((c = getchr()) >= '0' && c <= '9');
            peekc = c;
            if (a1 == 0)
                a1 = zero;
            if (minus < 0)
                n = -n;
            a1 += n;
            minus = 0;
            continue;
        }
        relerr = 0;
        if (a1 || minus)
            relerr++;
        switch (c) {
        case ' ':
        case '\t':
            continue;

        case '+':
            minus++;
            if (a1 == 0)
                a1 = dot;
            continue;

        case '-':
        case '^':
            minus--;
            if (a1 == 0)
                a1 = dot;
            continue;

        case '?':
        case '/':
            compile(c);
            a1 = dot;
            for (;;) {
                if (c == '/') {
                    a1++;
                    if (a1 > dol)
                        a1 = zero;
                } else {
                    a1--;
                    if (a1 < zero)
                        a1 = dol;
                }
                if (execute(0, a1))
                    break;
                if (a1 == dot)
                    error(Q);
            }
            break;

        case '$':
            a1 = dol;
            break;

        case '.':
            a1 = dot;
            break;

        case '\'':
            if ((c = getchr()) < 'a' || c > 'z')
                error(Q);
            for (a1 = zero; a1 <= dol; a1++)
                if (names[c - 'a'] == (*a1 & ~01))
                    break;
            break;

        default:
            peekc = c;
            if (a1 == 0)
                return (0);
            a1 += minus;
            if (a1 < zero || a1 > dol)
                error(Q);
            return (a1);
        }
        if (relerr)
            error(Q);
    }
}

void setdot()
{
    if (addr2 == 0)
        addr1 = addr2 = dot;
    if (addr1 > addr2)
        error(Q);
}

void setall()
{
    if (addr2 == 0) {
        addr1 = zero + 1;
        addr2 = dol;
        if (dol == zero)
            addr1 = zero;
    }
    setdot();
}

void setnoaddr()
{
    if (addr2)
        error(Q);
}

void nonzero()
{
    if (addr1 <= zero || addr2 > dol)
        error(Q);
}

void newline()
{
    int c;

    if ((c = getchr()) == '\n')
        return;
    if (c == 'p' || c == 'l') {
        pflag++;
        if (c == 'l')
            listf++;
        if (getchr() == '\n')
            return;
    }
    error(Q);
}

void filename(int comm)
{
    char *p1, *p2;
    int c;

    count = 0;
    c = getchr();
    if (c == '\n' || c == EOF) {
        p1 = savedfile;
        if (*p1 == 0 && comm != 'f')
            error(Q);
        p2 = file;
        while ((*p2++ = *p1++))
            ;
        return;
    }
    if (c != ' ')
        error(Q);
    while ((c = getchr()) == ' ')
        ;
    if (c == '\n')
        error(Q);
    p1 = file;
    do {
        *p1++ = c;
        if (c == ' ' || c == EOF)
            error(Q);
    } while ((c = getchr()) != '\n');
    *p1++ = 0;
    if (savedfile[0] == 0 || comm == 'e' || comm == 'f') {
        p1 = savedfile;
        p2 = file;
        while ((*p1++ = *p2++))
            ;
    }
}

void exfile()
{
    close(io);
    io = -1;
    if (vflag) {
        putd();
        putchr('\n');
    }
}

void error(char *s)
{
    int c;

    wrapp = 0;
    listf = 0;
    putchr('?');
    putstr(s);
    count = 0;
    lseek(0, (long)0, 2);
    pflag = 0;
    if (globp)
        lastc = '\n';
    globp = 0;
    peekc = lastc;
    if (lastc)
        while ((c = getchr()) != '\n' && c != EOF)
            ;
    if (io > 0) {
        close(io);
        io = -1;
    }
    longjmp(savej, 1);
}

int getchr()
{
    char c;
    if ((lastc = peekc)) {
        peekc = 0;
        return (lastc);
    }
    if (globp) {
        if ((lastc = *globp++) != 0)
            return (lastc);
        globp = 0;
        return (EOF);
    }
    if (read(0, &c, 1) <= 0)
        return (lastc = EOF);
    lastc = c & 0177;
    return (lastc);
}

int gettty()
{
    int c;
    char *gf;
    char *p;

    p = linebuf;
    gf = globp;
    while ((c = getchr()) != '\n') {
        if (c == EOF) {
            if (gf)
                peekc = c;
            return (c);
        }
        if ((c &= 0177) == 0)
            continue;
        *p++ = c;
        if (p >= &linebuf[LBSIZE - 2])
            error(Q);
    }
    *p++ = 0;
    if (linebuf[0] == '.' && linebuf[1] == 0)
        return (EOF);
    return (0);
}

int getfile()
{
    int c;
    char *lp, *fp;

    lp = linebuf;
    fp = nextip;
    do {
        if (--ninbuf < 0) {
            if ((ninbuf = read(io, genbuf, LBSIZE) - 1) < 0)
                return (EOF);
            fp = genbuf;
            while (fp < &genbuf[ninbuf]) {
                if (*fp++ & 0200) {
                    break;
                }
            }
            fp = genbuf;
        }
        c = *fp++;
        if (c == '\0')
            continue;
        if (c & 0200 || lp >= &linebuf[LBSIZE]) {
            lastc = '\n';
            error(Q);
        }
        *lp++ = c;
        count++;
    } while (c != '\n');
    *--lp = 0;
    nextip = fp;
    return (0);
}

void putfile()
{
    int *a1, n;
    char *fp, *lp;
    int nib;

    nib = 512;
    fp = genbuf;
    a1 = addr1;
    do {
        lp = getline(*a1++);
        for (;;) {
            if (--nib < 0) {
                n = fp - genbuf;
                if (write(io, genbuf, n) != n) {
                    putstr(WRERR);
                    error(Q);
                }
                nib = 511;
                fp = genbuf;
            }
            count++;
            if ((*fp++ = *lp++) == 0) {
                fp[-1] = '\n';
                break;
            }
        }
    } while (a1 <= addr2);
    n = fp - genbuf;
    if (write(io, genbuf, n) != n) {
        putstr(WRERR);
        error(Q);
    }
}

int append(int (*f)(), int *a)
{
    int *a1, *a2, *rdot;
    int nline, tl;

    nline = 0;
    dot = a;
    while ((*f)() == 0) {
        if ((dol - zero) + 1 >= nlall) {
            int *ozero = zero;
            nlall += 512;
            if ((zero = (int *)realloc((char *)zero, nlall * sizeof(int))) == NULL) {
                lastc = '\n';
                zero = ozero;
                error("MEM?");
            }
            dot += zero - ozero;
            dol += zero - ozero;
        }
        tl = putline();
        nline++;
        a1 = ++dol;
        a2 = a1 + 1;
        rdot = ++dot;
        while (a1 > rdot)
            *--a2 = *--a1;
        *rdot = tl;
    }
    return (nline);
}

void callunix()
{
    int pid, rpid;
    sig_t savint;
    int retcode;

    setnoaddr();
    if ((pid = fork()) == 0) {
        signal(SIGHUP, oldhup);
        signal(SIGQUIT, oldquit);
        execl("/bin/sh", "sh", "-t", 0);
        exit(0100);
    }
    savint = signal(SIGINT, SIG_IGN);
    while ((rpid = wait(&retcode)) != pid && rpid != -1)
        ;
    signal(SIGINT, savint);
    putstr("!");
}

void delete()
{
    setdot();
    newline();
    nonzero();
    rdelete(addr1, addr2);
}

void rdelete(int *ad1, int *ad2)
{
    int *a1, *a2, *a3;

    a1 = ad1;
    a2 = ad2 + 1;
    a3 = dol;
    dol -= a2 - a1;
    do {
        *a1++ = *a2++;
    } while (a2 <= a3);
    a1 = ad1;
    if (a1 > dol)
        a1 = dol;
    dot = a1;
    fchange = 1;
}

void gdelete()
{
    int *a1, *a2, *a3;

    a3 = dol;
    for (a1 = zero + 1; (*a1 & 01) == 0; a1++)
        if (a1 >= a3)
            return;
    for (a2 = a1 + 1; a2 <= a3;) {
        if (*a2 & 01) {
            a2++;
            dot = a1;
        } else
            *a1++ = *a2++;
    }
    dol = a1 - 1;
    if (dot > dol)
        dot = dol;
    fchange = 1;
}

char *getline(int tl)
{
    char *bp, *lp;
    int nl;

    lp = linebuf;
    bp = getblock(tl, READ);
    nl = nleft;
    tl &= ~0377;
    while ((*lp++ = *bp++))
        if (--nl == 0) {
            bp = getblock(tl += 0400, READ);
            nl = nleft;
        }
    return (linebuf);
}

int putline()
{
    char *bp, *lp;
    int nl;
    int tl;

    fchange = 1;
    lp = linebuf;
    tl = tline;
    bp = getblock(tl, WRITE);
    nl = nleft;
    tl &= ~0377;
    while ((*bp = *lp++)) {
        if (*bp++ == '\n') {
            *--bp = 0;
            linebp = lp;
            break;
        }
        if (--nl == 0) {
            bp = getblock(tl += 0400, WRITE);
            nl = nleft;
        }
    }
    nl = tline;
    tline += (((lp - linebuf) + 03) >> 1) & 077776;
    return (nl);
}

char *getblock(int atl, int iof)
{
    int bno, off;
    char *p1, *p2;
    int n;

    bno = (atl >> 8) & 0377;
    off = (atl << 1) & 0774;
    if (bno >= 255) {
        lastc = '\n';
        error(T);
    }
    nleft = 512 - off;
    if (bno == iblock) {
        ichanged |= iof;
        return (ibuff + off);
    }
    if (bno == oblock)
        return (obuff + off);
    if (iof == READ) {
        if (ichanged) {
            blkio(iblock, ibuff, write);
        }
        ichanged = 0;
        iblock = bno;
        blkio(bno, ibuff, read);
        return (ibuff + off);
    }
    if (oblock >= 0) {
        blkio(oblock, obuff, write);
    }
    oblock = bno;
    return (obuff + off);
}

void blkio(int b, char *buf, int (*iofcn)())
{
    lseek(tfile, (long)b << 9, 0);
    if ((*iofcn)(tfile, buf, 512) != 512) {
        error(T);
    }
}

void init()
{
    int *markp;

    close(tfile);
    tline = 2;
    for (markp = names; markp < &names[26];)
        *markp++ = 0;
    subnewa = 0;
    anymarks = 0;
    iblock = -1;
    oblock = -1;
    ichanged = 0;
    close(creat(tfname, 0600));
    tfile = open(tfname, 2);
    dot = dol = zero;
}

void global(int k)
{
    char *gp;
    int c;
    int *a1;
    char globuf[GBSIZE];

    if (globp)
        error(Q);
    setall();
    nonzero();
    if ((c = getchr()) == '\n')
        error(Q);
    compile(c);
    gp = globuf;
    while ((c = getchr()) != '\n') {
        if (c == EOF)
            error(Q);
        if (c == '\\') {
            c = getchr();
            if (c != '\n')
                *gp++ = '\\';
        }
        *gp++ = c;
        if (gp >= &globuf[GBSIZE - 2])
            error(Q);
    }
    *gp++ = '\n';
    *gp++ = 0;
    for (a1 = zero; a1 <= dol; a1++) {
        *a1 &= ~01;
        if (a1 >= addr1 && a1 <= addr2 && execute(0, a1) == k)
            *a1 |= 01;
    }
    /*
     * Special case: g/.../d (avoid n^2 algorithm)
     */
    if (globuf[0] == 'd' && globuf[1] == '\n' && globuf[2] == '\0') {
        gdelete();
        return;
    }
    for (a1 = zero; a1 <= dol; a1++) {
        if (*a1 & 01) {
            *a1 &= ~01;
            dot = a1;
            globp = globuf;
            commands();
            a1 = zero;
        }
    }
}

void join()
{
    char *gp, *lp;
    int *a1;

    gp = genbuf;
    for (a1 = addr1; a1 <= addr2; a1++) {
        lp = getline(*a1);
        while ((*gp = *lp++))
            if (gp++ >= &genbuf[LBSIZE - 2])
                error(Q);
    }
    lp = linebuf;
    gp = genbuf;
    while ((*lp++ = *gp++))
        ;
    *addr1 = putline();
    if (addr1 < addr2)
        rdelete(addr1 + 1, addr2);
    dot = addr1;
}

void substitute(int inglob)
{
    int *markp, *a1, nl;
    int gsubf;

    gsubf = compsub();
    for (a1 = addr1; a1 <= addr2; a1++) {
        int *ozero;
        if (execute(0, a1) == 0)
            continue;
        inglob |= 01;
        dosub();
        if (gsubf) {
            while (*loc2) {
                if (execute(1, (int *)0) == 0)
                    break;
                dosub();
            }
        }
        subnewa = putline();
        *a1 &= ~01;
        if (anymarks) {
            for (markp = names; markp < &names[26]; markp++)
                if (*markp == *a1)
                    *markp = subnewa;
        }
        subolda = *a1;
        *a1 = subnewa;
        ozero = zero;
        nl = append(getsub, a1);
        nl += zero - ozero;
        a1 += nl;
        addr2 += nl;
    }
    if (inglob == 0)
        error(Q);
}

int compsub()
{
    int seof, c;
    char *p;

    if ((seof = getchr()) == '\n' || seof == ' ')
        error(Q);
    compile(seof);
    p = rhsbuf;
    for (;;) {
        c = getchr();
        if (c == '\\')
            c = getchr() | 0200;
        if (c == '\n') {
            if (globp)
                c |= 0200;
            else
                error(Q);
        }
        if (c == seof)
            break;
        *p++ = c;
        if (p >= &rhsbuf[LBSIZE / 2])
            error(Q);
    }
    *p++ = 0;
    if ((peekc = getchr()) == 'g') {
        peekc = 0;
        newline();
        return (1);
    }
    newline();
    return (0);
}

int getsub()
{
    char *p1, *p2;

    p1 = linebuf;
    if ((p2 = linebp) == 0)
        return (EOF);
    while ((*p1++ = *p2++))
        ;
    linebp = 0;
    return (0);
}

void dosub()
{
    char *lp, *sp, *rp;
    int c;

    lp = linebuf;
    sp = genbuf;
    rp = rhsbuf;
    while (lp < loc1)
        *sp++ = *lp++;
    while ((c = *rp++ & 0377)) {
        if (c == '&') {
            sp = place(sp, loc1, loc2);
            continue;
        } else if (c & 0200 && (c &= 0177) >= '1' && c < nbra + '1') {
            sp = place(sp, braslist[c - '1'], braelist[c - '1']);
            continue;
        }
        *sp++ = c & 0177;
        if (sp >= &genbuf[LBSIZE])
            error(Q);
    }
    lp = loc2;
    loc2 = sp - genbuf + linebuf;
    while ((*sp++ = *lp++))
        if (sp >= &genbuf[LBSIZE])
            error(Q);
    lp = linebuf;
    sp = genbuf;
    while ((*lp++ = *sp++))
        ;
}

char *place(char *sp, char *l1, char *l2)
{
    while (l1 < l2) {
        *sp++ = *l1++;
        if (sp >= &genbuf[LBSIZE])
            error(Q);
    }
    return (sp);
}

void move(int cflag)
{
    int *adt, *ad1, *ad2;

    setdot();
    nonzero();
    if ((adt = address()) == 0)
        error(Q);
    newline();
    if (cflag) {
        int *ozero, delta;
        ad1 = dol;
        ozero = zero;
        append(getcopy, ad1++);
        ad2 = dol;
        delta = zero - ozero;
        ad1 += delta;
        adt += delta;
    } else {
        ad2 = addr2;
        for (ad1 = addr1; ad1 <= ad2;)
            *ad1++ &= ~01;
        ad1 = addr1;
    }
    ad2++;
    if (adt < ad1) {
        dot = adt + (ad2 - ad1);
        if ((++adt) == ad1)
            return;
        reverse(adt, ad1);
        reverse(ad1, ad2);
        reverse(adt, ad2);
    } else if (adt >= ad2) {
        dot = adt++;
        reverse(ad1, ad2);
        reverse(ad2, adt);
        reverse(ad1, adt);
    } else
        error(Q);
    fchange = 1;
}

void reverse(int *a1, int *a2)
{
    int t;

    for (;;) {
        t = *--a2;
        if (a2 <= a1)
            return;
        *a2 = *a1;
        *a1++ = t;
    }
}

int getcopy()
{
    if (addr1 > addr2)
        return (EOF);
    getline(*addr1++);
    return (0);
}

void compile(int aeof)
{
    int eof, c;
    char *ep;
    char *lastep;
    char bracket[NBRA], *bracketp;
    int cclcnt;

    ep = expbuf;
    eof = aeof;
    bracketp = bracket;
    if ((c = getchr()) == eof) {
        if (*ep == 0)
            error(Q);
        return;
    }
    circfl = 0;
    nbra = 0;
    if (c == '^') {
        c = getchr();
        circfl++;
    }
    peekc = c;
    lastep = 0;
    for (;;) {
        if (ep >= &expbuf[ESIZE])
            goto cerror;
        c = getchr();
        if (c == eof) {
            if (bracketp != bracket)
                goto cerror;
            *ep++ = CEOF;
            return;
        }
        if (c != '*')
            lastep = ep;
        switch (c) {
        case '\\':
            if ((c = getchr()) == '(') {
                if (nbra >= NBRA)
                    goto cerror;
                *bracketp++ = nbra;
                *ep++ = CBRA;
                *ep++ = nbra++;
                continue;
            }
            if (c == ')') {
                if (bracketp <= bracket)
                    goto cerror;
                *ep++ = CKET;
                *ep++ = *--bracketp;
                continue;
            }
            if (c >= '1' && c < '1' + NBRA) {
                *ep++ = CBACK;
                *ep++ = c - '1';
                continue;
            }
            *ep++ = CCHR;
            if (c == '\n')
                goto cerror;
            *ep++ = c;
            continue;

        case '.':
            *ep++ = CDOT;
            continue;

        case '\n':
            goto cerror;

        case '*':
            if (lastep == 0 || *lastep == CBRA || *lastep == CKET)
                goto defchar;
            *lastep |= STAR;
            continue;

        case '$':
            if ((peekc = getchr()) != eof)
                goto defchar;
            *ep++ = CDOL;
            continue;

        case '[':
            *ep++ = CCL;
            *ep++ = 0;
            cclcnt = 1;
            if ((c = getchr()) == '^') {
                c = getchr();
                ep[-2] = NCCL;
            }
            do {
                if (c == '\n')
                    goto cerror;
                if (c == '-' && ep[-1] != 0) {
                    if ((c = getchr()) == ']') {
                        *ep++ = '-';
                        cclcnt++;
                        break;
                    }
                    while (ep[-1] < c) {
                        *ep = ep[-1] + 1;
                        ep++;
                        cclcnt++;
                        if (ep >= &expbuf[ESIZE])
                            goto cerror;
                    }
                }
                *ep++ = c;
                cclcnt++;
                if (ep >= &expbuf[ESIZE])
                    goto cerror;
            } while ((c = getchr()) != ']');
            lastep[1] = cclcnt;
            continue;

        defchar:
        default:
            *ep++ = CCHR;
            *ep++ = c;
        }
    }
cerror:
    expbuf[0] = 0;
    nbra = 0;
    error(Q);
}

int execute(int gf, int *addr)
{
    char *p1, *p2, c;

    for (c = 0; c < NBRA; c++) {
        braslist[c] = 0;
        braelist[c] = 0;
    }
    if (gf) {
        if (circfl)
            return (0);
        p1 = linebuf;
        p2 = genbuf;
        while ((*p1++ = *p2++))
            ;
        locs = p1 = loc2;
    } else {
        if (addr == zero)
            return (0);
        p1 = getline(*addr);
        locs = 0;
    }
    p2 = expbuf;
    if (circfl) {
        loc1 = p1;
        return (advance(p1, p2));
    }
    /* fast check for first character */
    if (*p2 == CCHR) {
        c = p2[1];
        do {
            if (*p1 != c)
                continue;
            if (advance(p1, p2)) {
                loc1 = p1;
                return (1);
            }
        } while (*p1++);
        return (0);
    }
    /* regular algorithm */
    do {
        if (advance(p1, p2)) {
            loc1 = p1;
            return (1);
        }
    } while (*p1++);
    return (0);
}

int advance(char *lp, char *ep)
{
    char *curlp;
    int i;

    for (;;)
        switch (*ep++) {
        case CCHR:
            if (*ep++ == *lp++)
                continue;
            return (0);

        case CDOT:
            if (*lp++)
                continue;
            return (0);

        case CDOL:
            if (*lp == 0)
                continue;
            return (0);

        case CEOF:
            loc2 = lp;
            return (1);

        case CCL:
            if (cclass(ep, *lp++, 1)) {
                ep += *ep;
                continue;
            }
            return (0);

        case NCCL:
            if (cclass(ep, *lp++, 0)) {
                ep += *ep;
                continue;
            }
            return (0);

        case CBRA:
            braslist[*ep++] = lp;
            continue;

        case CKET:
            braelist[*ep++] = lp;
            continue;

        case CBACK:
            if (braelist[i = *ep++] == 0)
                error(Q);
            if (backref(i, lp)) {
                lp += braelist[i] - braslist[i];
                continue;
            }
            return (0);

        case CBACK | STAR:
            if (braelist[i = *ep++] == 0)
                error(Q);
            curlp = lp;
            while (backref(i, lp))
                lp += braelist[i] - braslist[i];
            while (lp >= curlp) {
                if (advance(lp, ep))
                    return (1);
                lp -= braelist[i] - braslist[i];
            }
            continue;

        case CDOT | STAR:
            curlp = lp;
            while (*lp++)
                ;
            goto star;

        case CCHR | STAR:
            curlp = lp;
            while (*lp++ == *ep)
                ;
            ep++;
            goto star;

        case CCL | STAR:
        case NCCL | STAR:
            curlp = lp;
            while (cclass(ep, *lp++, ep[-1] == (CCL | STAR)))
                ;
            ep += *ep;
            goto star;

        star:
            do {
                lp--;
                if (lp == locs)
                    break;
                if (advance(lp, ep))
                    return (1);
            } while (lp > curlp);
            return (0);

        default:
            error(Q);
        }
}

int backref(int i, char *lp)
{
    char *bp;

    bp = braslist[i];
    while (*bp++ == *lp++)
        if (bp >= braelist[i])
            return (1);
    return (0);
}

int cclass(char *set, char c, int af)
{
    int n;

    if (c == 0)
        return (0);
    n = *set++;
    while (--n)
        if (*set++ == c)
            return (af);
    return (!af);
}

void putd()
{
    int r;

    r = count % 10;
    count /= 10;
    if (count)
        putd();
    putchr(r + '0');
}

void putstr(char *sp)
{
    col = 0;
    while (*sp)
        putchr(*sp++);
    putchr('\n');
}

char line[70];
char *linp = line;

void putchr(int ac)
{
    char *lp;
    int c;

    lp = linp;
    c = ac;
    if (listf) {
        col++;
        if (col >= 72) {
            col = 0;
            *lp++ = '\\';
            *lp++ = '\n';
        }
        if (c == '\t') {
            c = '>';
            goto esc;
        }
        if (c == '\b') {
            c = '<';
        esc:
            *lp++ = '-';
            *lp++ = '\b';
            *lp++ = c;
            goto out;
        }
        if (c < ' ' && c != '\n') {
            *lp++ = '\\';
            *lp++ = (c >> 3) + '0';
            *lp++ = (c & 07) + '0';
            col += 2;
            goto out;
        }
    }
    *lp++ = c;
out:
    if (c == '\n' || lp >= &line[64]) {
        linp = line;
        write(1, line, lp - line);
        return;
    }
    linp = lp;
}
