/*
 * Main program: enter/exit, open windows, parse options.
 *
 * Usage:
 *      re file [line-number]   - Open file
 * or
 *      re                      - Open last edited file
 * or
 *      re -                    - Replay last session after crash
 *
 * RED editor for OS DEMOS
 * Alex P. Roudnev, Moscow, KIAE, 1984
 */
#include "r.defs.h"
#include <signal.h>
#include <sys/stat.h>

int keysym = -1;         /* -1 when the symbol already processed */

/*
 * Parameters for file scrolling.
 */
int defplline  = 10;                /* +LINE        */
int defmiline  = 10;                /* -LINE        */
int defloffset = 24;                /* LEFT OFFSET  */
int defroffset = 24;                /* RIGHT OFFSET */

char deffile[] = "/share/re.help";  /* Help file    */

/* Initial values. */
int cline_max  = 0;
int cline_incr = 20; /* Increment for line expand */
int clineno = -1;
char cline_modified = 0;

int journal  = -1;                  /* File descriptor for a journal */
int inputfile = 0;                  /* Journal or stdin */

int oldttmode;                      /* Saved access mode of /dev/tty */

static char *ttynm, *jname, *rfile;
static clipboard_t pb, db;

static void checksig(int);

/*
 * Fatal signal catched.
 */
void fatalsig(n)
    int n;
{
    fatal("Fatal signal");
}

/*
 * Ignore signals ^C and ^\
 */
static void ignsig(n)
    int n;
{
    signal(SIGQUIT, ignsig);
    signal(SIGINT, checksig);
}

/*
 * On first ^C from a user - set intrflag.
 * On second ^C - abort the editor session.
 */
static void checksig(n)
    int n;
{
    signal(SIGINT, ignsig);
    if (intrflag)
        fatal("RE WAS INTERRUPTED\n");
    ignsig(0);
    intrflag = 1;
}

/*
 * Initialize modes and files.
 * 0 - start a new session
 * 1 - restore a previous session
 * 2 - replay the session from a journal
 */
static void startup(restart)
    int restart;
{
    register int i;
    register char *c, *name;

    if (nice(-10) < 0)
        /* ignore errors */;
    userid = getuid();
    groupid = getgid();
    setuid(userid);
    setgid(groupid);
    name = getenv("USER");
    if (! name)
        name = getnm(userid);
    ttynm = ttyname(0);
    if (! ttynm)
        ttynm = "nottyno\0\0";
    c = ttynm + strlen (ttynm) - 2;
    if (*c == '/')
        *c = '-';
    tmpname = append(append("/tmp/ret", c), name);
    jname   = append(append("/tmp/rej", c), name);
    rfile   = append(append("/tmp/res", c), name);
    if (restart) {
        journal = open(jname, 2);
        if (journal >= 0 && restart != 2)
            close(journal);
    }
    if (restart != 2) {
        unlink(jname);
        journal = creat(jname, FILEMODE);
    } else
        inputfile = journal;
    if (journal < 0) {
        puts1("can't open journal.\n");
        exit(1);
    }
    unlink(tmpname);
    i = creat(tmpname, 0600);
    if (i < 0) {
        puts1("Can't open temporary file.\n");
        exit(1);
    }
    /* Temporary file should be rd/wr, need to reopen. */
    close(i);
    i = open(tmpname, 2);
    file[i].name = tmpname;
    file[i].writable = 1;
    tempfile = i;
    /* Pseudo-file '#' - store all deleted and copied/pasted. */
    file[2].name = "#";
    file[2].nlines = 0;
    pickwksp = (workspace_t*) salloc(sizeof(workspace_t));
    pickwksp->cursegm = file[2].chain = (segment_t*) salloc(sizeof_segment_t);
    pickwksp->wfile = 2;
    pickbuf = &pb;
    deletebuf = &db;

    /* Set tty modes. */
    ttstartup();

    /* Setup a window for a whole screen. */
    win_create(&wholescreen, 0, NCOLS-1, 0, NLINES-1, 0);

    /* Setup a window for a Cmd line. */
    win_create(&paramwin, 0, NCOLS-1, NLINES-1, NLINES-1, 0);
    paramwin.text_maxcol = PARAMWIDTH;

    /* Close the terminal from 'write' messages from other users. */
    oldttmode = getpriv(0);
    chmod(ttynm, 0600);

    /*
     * Catch signals.
     */
    for (i=SIGTERM; i; i--)
        signal(i, fatalsig);
    signal(SIGINT, checksig);
    signal(SIGQUIT, ignsig);
    curwin = &wholescreen;
    putch(COSTART);
    putch(COHO);
}

/*
 * Make or restore window state.
 * '!' - restore, ' ' - make new.
 */
static void getstate(init_flag)
    char init_flag;
{
    int nletters, base_col, max_col, base_row, max_row, row, col, winnum, gf;
    register int i, n;
    register char *f1;
    char *fname;
    int gbuf;
    window_t *win;

    if (init_flag != '!' || (gbuf = open(rfile, 0)) <= 0 ||
        (nwinlist = get1w(gbuf)) == -1)
    {
make:   /* Create initial window state. */
        nwinlist = 1;
        win = (window_t*) salloc(sizeof(window_t));
        winlist[0] = win;
        win_create(win, 0, NCOLS-1, 0, NLINES-2, MULTIWIN);
        win_borders(win, 0);
        poscursor(0, 0);
        return;
    }
    winnum = get1w(gbuf);
    for (n=0; n<nwinlist; n++) {
        win = (window_t*) salloc(sizeof(window_t));
        winlist[n] = win;
        win->prevwinnum = get1w(gbuf);
        base_col = get1w(gbuf);
        max_col = get1w(gbuf);
        base_row = get1w(gbuf);
        max_row = get1w(gbuf);
        win_create(win, base_col, max_col, base_row, max_row, MULTIWIN);
        win_borders(win, 0);
        gf = 0;
        nletters = get1w(gbuf);
        if (nletters != 0) {
            f1 = fname = salloc(nletters);
            do {
                *f1 = get1c(gbuf);
            } while (*f1++);
            row = get1w(gbuf);
            col = get1w(gbuf);
            if (editfile(fname, row, col, 0, 0) == 1)
                gf = 1;
            curwksp->cursorcol = get1w(gbuf);
            curwksp->cursorrow = get1w(gbuf);
            poscursor(curwksp->cursorcol, curwksp->cursorrow);
        }
        nletters = get1w(gbuf);
        if (nletters < 0)
            goto make;
        f1 = fname = salloc(nletters);
        do {
            *f1 = get1c(gbuf);
        } while (*f1++);
        row = get1w(gbuf);
        col = get1w(gbuf);
        if (editfile(fname, row, col, 0, (n != winnum)) == 1)
            gf = 1;
        curwksp->cursorcol = get1w(gbuf);
        curwksp->cursorrow = get1w(gbuf);
        if (gf == 0) {
            if (editfile(deffile, 0, 0, 0, (n != winnum)) <= 0)
                error("Cannot open help file.");
            curwksp->cursorcol = curwksp->cursorrow = 0;
        }
        poscursor(curwksp->cursorcol, curwksp->cursorrow);
    }
    win_switch(winlist[winnum]);
    poscursor(curwksp->cursorcol, curwksp->cursorrow);
    if (nwinlist > 1)
        for (i=0; i<nwinlist; i++)
            win_goto(-1);
    close(gbuf);
}

/*
 * Write a command with arguments to protocol file.
 */
static void writefile(code1, str, code2)
    int code1, code2;
    char *str;
{
    int len;

    if (write(journal, &code1, 1) != 1)
        /* ignore errors */;
    len = strlen (str);
    if (len > 0 && write(journal, str, len) != len)
        /* ignore errors */;
    if (write(journal, &code2, 1) != 1)
        /* ignore errors */;
}

/*
 * Write state of windows, to restore session later.
 */
static void savestate()
{
    int i, nletters;
    register int winnum;
    register char *f1;
    char *fname;
    register window_t *win;
    int sbuf;

    curwksp->cursorcol = cursorcol;
    curwksp->cursorrow = cursorline;
    unlink(rfile);
    sbuf = creat(rfile, FILEMODE);
    if (sbuf <= 0)
        return;
    put1w(nwinlist, sbuf);
    for (winnum=0; winnum < nwinlist; winnum++)
        if (winlist[winnum] == curwin)
            break;
    put1w(winnum, sbuf);
    for (i=0; i<nwinlist; i++) {
        win = winlist[i];
        put1w(win->prevwinnum, sbuf);
        put1w(win->base_col, sbuf);
        put1w(win->max_col, sbuf);
        put1w(win->base_row, sbuf);
        put1w(win->max_row, sbuf);
        f1 = fname = file[win->altwksp->wfile].name;
        if (f1) {
            while (*f1++);
            nletters = f1 - fname;
            put1w(nletters, sbuf);
            f1 = fname;
            do {
                put1c(*f1, sbuf);
            } while (*f1++);
            put1w(win->altwksp->topline, sbuf);
            put1w(win->altwksp->offset, sbuf);
            put1w(win->altwksp->cursorcol, sbuf);
            put1w(win->altwksp->cursorrow, sbuf);
        } else
            put1w(0, sbuf);

        f1 = fname = file[win->wksp->wfile].name;
        if (f1) {
            while (*f1++);
            nletters = f1 - fname;
            put1w(nletters, sbuf);
            f1 = fname;
            do {
                put1c(*f1, sbuf);
            } while (*f1++);
            put1w(win->wksp->topline, sbuf);
            put1w(win->wksp->offset, sbuf);
            put1w(win->wksp->cursorcol, sbuf);
            put1w(win->wksp->cursorrow, sbuf);
        }
    }
    close(sbuf);
}

/*
 * Main routine.
 */
int main(nargs, args)
    int nargs;
    char *args[];
{
    int restart, i;
    char *cp, init_flag = ' ';

    tcread();

    /* Mode: 0 - normal, 1 - restart, 2 - from journal file /tmp/rej.. */
    restart = 0;
    if (nargs == 1) {
        restart = 1;
        init_flag = '!';

    } else if (*(cp = args[1]) == '-') {
        if (*++cp) {
            /* Journal file specified. */
            inputfile = open(cp, 0);
            if (inputfile < 0) {
                puts1("Can't open journal file.\n");
                return 1;
            }
        } else
            restart = 2;
        nargs = 1;
    }

    startup(restart);
    if (inputfile) {
        if (read(inputfile, &init_flag, 1) <= 0) {
            cleanup();
            puts1("Journal file is empty.\n");
            return 1;
        }
    } else {
        /* For repeated session */
        if (write(journal, &init_flag, 1) != 1)
            /* ignore errors */;
    }
    getstate(init_flag);

    if (nargs > 1 && *args[1] != '\0') {
        i = defplline + 1;
        if ((nargs > 2) && (s2i(args[2],&i) || i <= defplline+1))
            i = defplline+1;
        poscursor(curwksp->cursorcol, curwksp->cursorrow);
        writefile(CCPARAM, args[1], CCSETFILE);
        if (editfile(args[1], i - defplline - 1, 0, 1, 1) <= 0) {
            /* Failed to open file - use empty buffer. */
            drawlines(0, curwin->text_maxrow);
            poscursor(curwksp->cursorcol, curwksp->cursorrow);
        } else {
            /* File opened. */
            if (nargs > 2 && i > 1)
                writefile(CCPARAM, args[2], CCGOTO);
        }
    } else {
        /* Saved session restored. */
        drawlines(0, curwin->text_maxrow);
        poscursor(curwksp->cursorcol, curwksp->cursorrow);
    }
    telluser("", 0);
    mainloop();
    putch(COFIN);
    dumpcbuf();
    wputc('\n', 0);
    cleanup();
    savestate();    /* Save a session. */
    dumpcbuf();
    return 0;
}

/*
 * Cleanup before exit.
 */
void cleanup()
{
    /* Restore tty mode and exit */
    ttcleanup();
    close(tempfile);
    unlink(tmpname);
    close(journal);
    chmod(ttynm, 07777 & oldttmode);
}

/*
 * Print error message and exit.
 */
void fatal(s)
    char *s;
{
    putch(COFIN);
    putch(COBELL);
    dumpcbuf();
    ttcleanup();
    puts1("\nFirst the bad news: editor just ");
    if (s) {
        puts1("died:\n");
        puts1(s);
    } else
        puts1("ran out of space.\n");
    puts1("\nNow the good news - your editing session can be reproduced\n");
    puts1("from file ");
    puts1(jname);
    puts1("\nUse command 're -' to do it.\n");
#ifdef DEBUG
    if (inputfile || ! isatty(1)) {
        register int i;
        register workspace_t *w;

        if (s)
            printf("%s\n\n", s);
        for (i = 0; i < MAXFILES; i++)
            if (file[i].chain) {
                printf("\n*** File[%d] = %s\n", i, file[i].name);
                printsegm(file[i].chain);
            }
        for (i = 0; i < nwinlist; i++) {
            w = winlist[i]->wksp;
            printf("\nWindow #%d: chain %d, current line %d at block %p,\n",
                i, w->wfile, w->line, w->cursegm);
            printf(" first line %d, ulhc (%d,%d)\n",
                w->segmline, w->offset, w->topline);
        }
        for (i=12; i; i--)
            signal(i, 0);
    }
#endif
    close(journal);
    exit(1);
}
