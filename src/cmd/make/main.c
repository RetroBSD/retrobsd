/*
 * command make to update programs.
 * Flags:
 *  'd'  print out debugging comments
 *  'p'  print out a version of the input graph
 *  's'  silent mode--don't print out commands
 *  'f'  the next argument is the name of the description file;
 *       "makefile" is the default
 *  'i'  ignore error codes from the shell
 *  'S'  stop after any command fails (normally do parallel work)
 *  'n'  don't issue, just print, commands
 *  't'  touch (update time of) files but don't issue command
 *  'q'  don't do anything, but check if object is up to date;
 *       returns exit code 0 if up to date, -1 if not
 *  'e'  environment variables have precedence over makefiles
 */
#include "defs.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#ifdef CROSS
#   include </usr/include/sys/utsname.h>
#else
#   include <sys/utsname.h>
#endif

struct nameblock *mainname  = NULL;
struct nameblock *firstname = NULL;
struct lineblock *sufflist  = NULL;
struct varblock *firstvar   = NULL;
struct pattern *firstpat    = NULL;
struct dirhdr *firstod      = NULL;

int sigivalue   = 0;
int sigqvalue   = 0;
int wpid        = 0;

int dbgflag     = NO;
int prtrflag    = NO;
int silflag     = NO;
int noexflag    = NO;
int keepgoing   = NO;
int noruleflag  = NO;
int touchflag   = NO;
int questflag   = NO;
int ndocoms     = NO;
int ignerr      = NO;    /* default is to stop on error */
int okdel       = YES;
int doenvlast   = NO;
int inarglist;
char *prompt    = "";   /* other systems -- pick what you want */
int nopdir      = 0;

char junkname[20];
char funny[128];
char options[26 + 1] = { '-' };

char **linesptr = builtin;

FILE * fin;
int firstrd = 0;

extern char **environ;

int isprecious(p)
char *p;
{
    register struct lineblock *lp;
    register struct depblock *dp;
    register struct nameblock *np;

    np = srchname(".PRECIOUS");
    if (np)
        for (lp = np->linep ; lp ; lp = lp->nxtlineblock)
            for (dp = lp->depp ; dp ; dp = dp->nxtdepblock)
                if (! unequal(p, dp->depname->namep))
                    return YES;

    return NO;
}

void intrupt (sig)
    int sig;
{
    char *p;
    struct stat sbuf;

    if (okdel && !noexflag && !touchflag &&
        (p = varptr("@")->varval) &&
        (stat(p, &sbuf) >= 0 && (sbuf.st_mode&S_IFMT) == S_IFREG) &&
        ! isprecious(p))
    {
        fprintf(stderr, "\n***  %s removed.", p);
        unlink(p);
    }

    if (junkname[0])
        unlink(junkname);
    fprintf(stderr, "\n");
    exit(2);
}

int rdd1(k)
    FILE * k;
{
    extern int yylineno;
    extern char *zznextc;

    fin = k;
    yylineno = 0;
    zznextc = 0;

    if (yyparse())
        fatal("Description file error");

    if(fin != NULL && fin != stdin)
        fclose(fin);

    return(0);
}

void readenv()
{
    register char **ep, *p;

    for(ep=environ; *ep; ++ep) {
        for (p = *ep; *p; p++) {
            if (isalnum(*p))
                continue;
            if (*p == '=') {
                eqsign(*ep);
            }
            break;
        }
    }
}

/*
 * read and parse description
 */
int rddescf(descfile)
    char *descfile;
{
    FILE * k;

    if (! firstrd++) {
        if (! noruleflag)
            rdd1 ((FILE *) NULL);

        if (doenvlast == NO)
            readenv();
    }
    if (! unequal(descfile, "-"))
        return rdd1(stdin);

    k = fopen(descfile, "r");
    if (k != NULL)
        return rdd1(k);

    return(1);
}

/*
 * This is done in a function by itself because 'uname()' uses a 640
 * structure which we do not want permanently allocated on main()'s stack.
 */
void setmachine()
{
    struct utsname foo;

    if (uname(&foo) < 0)
        strcpy(foo.machine, "?");
    setvar("MACHINE", foo.machine);
}

void printdesc(prntflag)
    int prntflag;
{
    struct nameblock *p;
    struct depblock *dp;
    struct varblock *vp;
    struct dirhdr *od;
    struct shblock *sp;
    struct lineblock *lp;

    if (prntflag) {
        printf("Open directories:\n");
        for (od = firstod; od; od = od->nxtopendir)
            printf("\t%d: %s\n", od->dirfc->dd_fd, od->dirn);
    }

    if (firstvar != 0)
        printf("Macros:\n");
    for (vp = firstvar; vp ; vp = vp->nxtvarblock)
        printf("\t%s = %s\n", vp->varname, vp->varval);

    for (p = firstname; p; p = p->nxtnameblock) {
        printf("\n\n%s", p->namep);
        if (p->linep != 0)
            printf(":");
        if (prntflag)
            printf("  done=%d",p->done);
        if (p == mainname)
            printf("  (MAIN NAME)");
        for (lp = p->linep ; lp ; lp = lp->nxtlineblock) {
            dp = lp->depp;
            if (dp != 0) {
                printf("\n depends on:");
                for (; dp ; dp = dp->nxtdepblock)
                    if (dp->depname != 0)
                        printf(" %s ", dp->depname->namep);
            }

            sp = lp->shp;
            if (sp != 0) {
                printf("\n commands:\n");
                for (; sp != 0; sp = sp->nxtshblock)
                    printf("\t%s\n", sp->shbp);
            }
        }
    }
    printf("\n");
    fflush(stdout);
}

void enbint(k)
    void (*k)(int);
{
    if (sigivalue == 0)
        signal(SIGINT,k);
    if (sigqvalue == 0)
        signal(SIGQUIT,k);
}

int main(argc, argv)
    int argc;
    char *argv[];
{
    register struct nameblock *p;
    register int i, j;
    int descset, nfargs;
    TIMETYPE tjunk;
    char c, *s;
    static char onechar[2] = "X\0";
    char *op = options + 1;

#ifdef METERFILE
    meter(METERFILE);
#endif

    descset = 0;

    funny['\0'] = (META | TERMINAL);
    for(s = "=|^();&<>*?[]:$`'\"\\\n" ; *s ; ++s)
        funny[(int)*s] |= META;
    for(s = "\n\t :;&>|" ; *s ; ++s)
        funny[(int)*s] |= TERMINAL;

    inarglist = 1;
    for(i=1; i<argc; ++i)
        if (argv[i]!=0 && argv[i][0]!='-' && eqsign(argv[i]))
            argv[i] = 0;

    setvar("$","$");
    inarglist = 0;

    for (i=1; i<argc; ++i) {
        if (argv[i]!=0 && argv[i][0]=='-') {
            for (j=1 ; (c=argv[i][j])!='\0' ; ++j) {
                *op++ = c;
                switch (c) {

                case 'd':
                    dbgflag = YES;
                    break;

                case 'p':
                    prtrflag = YES;
                    break;

                case 's':
                    silflag = YES;
                    break;

                case 'i':
                    ignerr = YES;
                    break;

                case 'S':
                    keepgoing = NO;
                    break;

                case 'k':
                    keepgoing = YES;
                    break;

                case 'n':
                    noexflag = YES;
                    break;

                case 'r':
                    noruleflag = YES;
                    break;

                case 't':
                    touchflag = YES;
                    break;

                case 'q':
                    questflag = YES;
                    break;

                case 'f':
                    op--;       /* don't pass this one */
                    if (i >= argc-1)
                        fatal("No description argument after -f flag");
                    if (rddescf(argv[i+1]) != 0)
                        fatal1("Cannot open %s", argv[i+1]);
                    argv[i+1] = 0;
                    ++descset;
                    break;

                case 'e':
                    doenvlast = YES;
                    break;

                default:
                    onechar[0] = c; /* to make lint happy */
                    fatal1("Unknown flag argument %s", onechar);
                }
            }
            argv[i] = 0;
        }
    }
    *op++ = '\0';
    if (strcmp(options, "-") == 0)
        *options = '\0';
    setvar("MFLAGS", options);      /* MFLAGS=options to make */

    setmachine();

    if (! descset) {
        if (rddescf("makefile"))
            rddescf("Makefile");
    }
    if (doenvlast == YES)
        readenv();

    if (prtrflag)
        printdesc(NO);

    if (srchname(".IGNORE"))
        ++ignerr;
    if (srchname(".SILENT"))
        silflag = 1;
    p = srchname(".SUFFIXES");
    if (p != 0)
        sufflist = p->linep;
    if (! sufflist)
        fprintf(stderr,"No suffix list.\n");

    sigivalue = (int) signal(SIGINT, SIG_IGN) & 01;
    sigqvalue = (int) signal(SIGQUIT, SIG_IGN) & 01;
    enbint(intrupt);

    nfargs = 0;

    for(i=1; i<argc; ++i) {
        if ((s = argv[i]) != 0) {
            if ((p = srchname(s)) == 0) {
                p = makename(s);
            }
            ++nfargs;
            doname(p, 0, &tjunk);
            if (dbgflag)
                printdesc(YES);
        }
    }
    /*
     * If no file arguments have been encountered, make the first
     * name encountered that doesn't start with a dot
     */
    if (nfargs == 0) {
        if (mainname == 0)
            fatal("No arguments or description file");
        else {
            doname(mainname, 0, &tjunk);
            if (dbgflag)
                printdesc(YES);
        }
    }
    exit(0);
}
