#include "defs.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <ar.h>
#include "archive.h"    /* from 'ar's directory */

/* UNIX DEPENDENT PROCEDURES */

/* DEFAULT RULES FOR UNIX */

char *builtin[] = {
    ".SUFFIXES : .out .a .o .c .y .l .s .S",
    "YACC=yacc",
    "YFLAGS=",
    "LEX=lex",
    "LFLAGS=",
    "CC=cc",
    "AS=as",
    "LD=ld",
    "CFLAGS=",
    "ASFLAGS=",
    "LIBS=",

    ".c.a :",
    "\t$(CC) $(CFLAGS) -c $<",
    "\tar r $@ $*.o",
    "\trm -f $*.o",

    ".c.o :",
    "\t$(CC) $(CFLAGS) -c $<",

    ".S.o :",
    "\t$(CC) $(ASFLAGS) -c $<",

    ".s.o :",
    "\t$(AS) $(ASFLAGS) -o $@ $<",

    ".y.o :",
    "\t$(YACC) $(YFLAGS) $<",
    "\t$(CC) $(CFLAGS) -c y.tab.c",
    "\trm y.tab.c",
    "\tmv y.tab.o $@",

    ".l.o :",
    "\t$(LEX) $(LFLAGS) $<",
    "\t$(CC) $(CFLAGS) -c lex.yy.c",
    "\trm lex.yy.c",
    "\tmv lex.yy.o $@",

    ".y.c :",
    "\t$(YACC) $(YFLAGS) $<",
    "\tmv y.tab.c $@",

    ".l.c :",
    "\t$(LEX) $(LFLAGS) $<",
    "\tmv lex.yy.c $@",

    ".s.out .c.out .o.out :",
    "\t$(CC) $(CFLAGS) $< $(LIBS) -o $@",

    ".y.out :",
    "\t$(YACC) $(YFLAGS) $<",
    "\t$(CC) $(CFLAGS) y.tab.c $(LIBS) -ly -o $@",
    "\trm y.tab.c",

    ".l.out :",
    "\t$(LEX) $(LFLAGS) $<",
    "\t$(CC) $(CFLAGS) lex.yy.c $(LIBS) -ll -o $@",
    "\trm lex.yy.c",

    0 };

FSTATIC char nbuf[MAXNAMLEN + 1];
FSTATIC char *nbufend = &nbuf[MAXNAMLEN];

char arfile[MAXPATHLEN];
CHDR chdr;
char *arfname = chdr.name;
FILE *arfd;
off_t arpos;

/*
 * "borrowed" from 'ld' because we didn't want to drag in everything
 * from 'ar'.  The error checking was also ripped out, basically if any
 * of the criteria for being an archive are not met then a -1 is returned
 * and the rest of 'make' figures out what to do (bails out).
 */

typedef struct ar_hdr HDR;

/* Convert ar header field to an integer. */
#define AR_ATOI(from, to, len, base) { \
    bcopy(from, buf, len); \
    buf[len] = '\0'; \
    to = strtoul(buf, (char **)NULL, base); \
}

int getarch()
{
    char hb[sizeof(HDR) + 1];   /* real header */
    register HDR *hdr;
    register int len;
    int nr;
    register char *p;
    char buf[20];

    if (!arfd)
        return(0);
    fseek(arfd, arpos, 0);

    nr = fread(hb, 1, sizeof(HDR), arfd);
    if (nr != sizeof(HDR))
        return(0);

    hdr = (HDR *)hb;
    if (strncmp(hdr->ar_fmag, ARFMAG, sizeof(ARFMAG) - 1))
        return(0);

    /* Convert the header into the internal format. */
#define DECIMAL 10
#define OCTAL    8

    AR_ATOI(hdr->ar_date, chdr.date, sizeof(hdr->ar_date), DECIMAL);
    AR_ATOI(hdr->ar_uid, chdr.uid, sizeof(hdr->ar_uid), DECIMAL);
    AR_ATOI(hdr->ar_gid, chdr.gid, sizeof(hdr->ar_gid), DECIMAL);
    AR_ATOI(hdr->ar_mode, chdr.mode, sizeof(hdr->ar_mode), OCTAL);
    AR_ATOI(hdr->ar_size, chdr.size, sizeof(hdr->ar_size), DECIMAL);

    /* Leading spaces should never happen. */
    if (hdr->ar_name[0] == ' ')
        return(-1);

    /*
     * Long name support.  Set the "real" size of the file, and the
     * long name flag/size.
     */
    if (!bcmp(hdr->ar_name, AR_EFMT1, sizeof(AR_EFMT1) - 1)) {
        chdr.lname = len = atoi(hdr->ar_name + sizeof(AR_EFMT1) - 1);
        if (len <= 0 || len > MAXNAMLEN)
            return(-1);
        nr = fread(chdr.name, 1, (size_t)len, arfd);
        if (nr != len)
            return(0);
        chdr.name[len] = 0;
        chdr.size -= len;
    } else {
        chdr.lname = 0;
        bcopy(hdr->ar_name, chdr.name, sizeof(hdr->ar_name));

        /* Strip trailing spaces, null terminate. */
        for (p = chdr.name + sizeof(hdr->ar_name) - 1; *p == ' '; --p);
        *++p = '\0';
    }
    return(1);
}

void openarch(f)
    register char *f;
{
    char    magic[SARMAG];

    arfd = fopen(f, "r");
    if (arfd == NULL)
        return;

    fseek(arfd, 0L, 0);
    fread(magic, SARMAG, 1, arfd);
    arpos = SARMAG;
    if (strncmp(magic, ARMAG, SARMAG))
        fatal1("%s is not an archive", f);
}

void clarch()
{
    if (arfd) {
        fclose (arfd);
        arfd = 0;
    }
}

/*
 * look inside archive for notation a(b)
 * a(b)    is file member   b   in archive a
 */
TIMETYPE lookarch(filename)
    char *filename;
{
    char *p, *q, *send, s[MAXNAMLEN + 1];

    for (p = filename; *p!= '(' ; ++p)
        ;
    *p = '\0';
    strcpy(arfile, filename);
    openarch(filename);
    *p++ = '(';

    send = s + sizeof(s);

    for( q = s; q < send && *p!='\0' && *p!=')' ; *q++ = *p++)
        ;
    *q++ = '\0';
    while (getarch()) {
        if (! strcmp(arfname, s)) {
            clarch();
            return(chdr.date);
        }
        arpos += (chdr.size + (chdr.size + (chdr.lname & 1)));
        arpos += sizeof (struct ar_hdr);
    }
    strcpy(chdr.name, s);
    clarch();
    return(0L);
}

char *execat(s1, s2, si)
    register char *s1, *s2;
    char *si;
{
    register char *s;

    s = si;
    while (*s1 && *s1 != ':' && *s1 != '-')
        *s++ = *s1++;
    if (si != s)
        *s++ = '/';
    while (*s2)
        *s++ = *s2++;
    *s = '\0';
    return(*s1? ++s1: 0);
}

/*
 * findfl(name)    (like execvp, but does path search and finds files)
 */
static char fname[128];

char *findfl(name)
    register char *name;
{
    register char *p;
    struct varblock *cp;
    struct stat buf;

    for (p = name; *p; p++)
        if(*p == '/') return(name);

    cp = varptr("VPATH");
    if(cp->varval == NULL || *cp->varval == 0)
        p = ":";
    else
        p = cp->varval;

    do {
        p = execat(p, name, fname);
        if(stat(fname,&buf) >= 0)
            return(fname);
    } while (p);
    return((char *)-1);
}

TIMETYPE exists(pname)
    struct nameblock *pname;
{
    struct stat buf;
    register char *s, *filename;

    filename = pname->namep;

    for(s = filename ; *s!='\0' && *s!='(' ; ++s)
        ;

    if(*s == '(')
        return(lookarch(filename));

    if (stat(filename, &buf) >= 0)
        return(buf.st_mtime);

    s = findfl(filename);
    if (s != (char *)-1) {
        pname->alias = copys(s);
        if(stat(pname->alias, &buf) == 0)
            return(buf.st_mtime);
    }
    return(0);
}

TIMETYPE prestime()
{
    TIMETYPE t;

    time(&t);
    return(t);
}

static int amatch (char *s, char *p);

static int umatch(s, p)
    char *s, *p;
{
    if (*p == 0)
        return 1;
    while (*s)
        if (amatch(s++, p))
            return 1;
    return 0;
}

/*
 * stolen from glob through find
 */
static int amatch(s, p)
    char *s, *p;
{
    register int cc, scc, k;
    int c, lc;

    scc = *s;
    lc = 077777;
    c = *p;
    switch (c) {
    case '[':
        k = 0;
        while ((cc = *++p)) {
            switch (cc) {
            case ']':
                if (k)
                    return amatch(++s, ++p);
                else
                    return 0;
            case '-':
                k |= (lc <= scc) & (scc <= (cc = p[1]));
            }
            if (scc == (lc = cc))
                k++;
        }
        return 0;

    case '?':
    caseq:
        if (scc)
            return amatch(++s, ++p);
        return 0;
    case '*':
        return umatch(s, ++p);
    case 0:
        return ! scc;
    }
    if (c == scc)
        goto caseq;
    return 0;
}

struct depblock *srchdir(pat, mkchain, nextdbl)
    register char *pat;         /* pattern to be matched in directory */
    int mkchain;                /* nonzero if results to be remembered */
    struct depblock *nextdbl;   /* final value for chain */
{
    register DIR *dirf;
    int cldir;
    char *dirname, *dirpref, *endir, *filepat, *p, temp[MAXPATHLEN];
    char fullname[MAXPATHLEN], *p1, *p2;
    struct nameblock *q;
    struct depblock *thisdbl;
    struct dirhdr *od;
    struct pattern *patp;
    struct varblock *cp, *varptr();
    char *path, pth[MAXPATHLEN], *strcpy();
    struct direct *dptr;

    thisdbl = 0;

    if (mkchain == NO)
        for(patp=firstpat ; patp ; patp = patp->nxtpattern)
            if (! unequal(pat, patp->patval))
                return(0);

    patp = ALLOC(pattern);
    patp->nxtpattern = firstpat;
    firstpat = patp;
    patp->patval = copys(pat);

    endir = 0;

    for (p=pat; *p!='\0'; ++p)
        if (*p == '/')
            endir = p;

    if (endir == 0) {
        dirpref = "";
        filepat = pat;
        cp = varptr("VPATH");
        if (cp->varval == NULL)
            path = ".";
        else {
            path = pth;
            *path = '\0';
            /*
             * expand VPATH; this is almost surely not the place to do
             * this, but I have no intention whatsoever of attempting
             * to understand this code.
             */
            if (strncmp(cp->varval, ".:", 2) != 0) {
                strcpy(pth,".:");
                subst(cp->varval, pth + 2);
            }
            else
                subst(cp->varval, pth);
            }
    } else {
        *endir = '\0';
        path = strcpy(pth, pat);
        dirpref = concat(pat, "/", temp);
        filepat = endir+1;
    }

    while (*path) {         /* Loop thru each VPATH directory */
        dirname = path;
        for (; *path; path++)
            if (*path == ':') {
                *path++ = '\0';
                break;
            }

        dirf = NULL;
        cldir = NO;

        for (od = firstod; od; od = od->nxtopendir)
            if (! unequal(dirname, od->dirn)) {
                dirf = od->dirfc;
                if (dirf != NULL)
                    rewinddir(dirf); /* start over at the beginning  */
                break;
            }

        if (dirf == NULL) {
            dirf = opendir(dirname);
            if (nopdir >= MAXDIR)
                cldir = YES;
            else {
                ++nopdir;
                od = ALLOC(dirhdr);
                od->nxtopendir = firstod;
                firstod = od;
                od->dirfc = dirf;
                od->dirn = copys(dirname);
                fcntl(dirf->dd_fd, F_SETFD, 1);
            }
        }

        if (dirf == NULL) {
            fprintf(stderr, "Directory %s: ", dirname);
            fatal("Cannot open");
        }
        else for (dptr = readdir(dirf); dptr != NULL; dptr = readdir(dirf)) {
            p1 = dptr->d_name;
            p2 = nbuf;
            while ((p2<nbufend) && (*p2++ = *p1++)!='\0')
                /* void */;
            if (amatch(nbuf, filepat)) {
                concat(dirpref, nbuf, fullname);
                q = srchname(fullname);
                if (q == 0)
                    q = makename(copys(fullname));
                if (mkchain) {
                    thisdbl = ALLOC(depblock);
                    thisdbl->nxtdepblock = nextdbl;
                    thisdbl->depname = q;
                    nextdbl = thisdbl;
                }
            }
        }

        if (endir != 0)
            *endir = '/';

        if (cldir) {
            closedir(dirf);
            dirf = NULL;
        }
    } /* End of VPATH loop */
    return thisdbl;
}

#ifdef METERFILE
#include <pwd.h>

int meteron = 0;    /* default: metering off */

void meter(file)
    char *file;
{
    TIMETYPE tvec;
    char *p;
    FILE *mout;
    struct passwd *pwd;

    if (file == 0 || meteron == 0)
        return;

    pwd = getpwuid(getuid());

    time(&tvec);

    mout = fopen(file, "a");
    if (mout != NULL) {
        p = ctime(&tvec);
        p[16] = '\0';
        fprintf(mout, "User %s, %s\n", pwd->pw_name, p+4);
        fclose(mout);
    }
}
#endif

/*
 * copy s to d, changing file names to file aliases
 */
void fixname(s, d)
    char *s, *d;
{
    register char *r, *q;
    struct nameblock *pn;
    char name[MAXPATHLEN];

    while (*s) {
        if (isspace(*s)) *d++ = *s++;
        else {
            r = name;
            while (*s) {
                if (isspace(*s)) break;
                *r++ = *s++;
                }
            *r = '\0';

            if (((pn = srchname(name)) != 0) && (pn->alias))
                q = pn->alias;
            else q = name;

            while (*q) *d++ = *q++;
            }
        }
    *d = '\0';
}
