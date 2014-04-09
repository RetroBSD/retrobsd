/*
 * Interface to display - physical level.
 * Parsing escape sequences.
 *
 * RED editor for OS DEMOS
 * Alex P. Roudnev, Moscow, KIAE, 1984
 */
#include "r.defs.h"
#include <sgtty.h>

#ifdef TERMIOS
#include <termios.h>

#ifndef NCC
#   define NCC NCCS
#endif

static struct termios tioparam;

#else /* TERMIOS */

static struct sgttyb templ;
static struct tchars tchars0;
static struct ltchars ltchars0;

#endif /* TERMIOS */

/*
 * Output character buffer.
 */
#define OUTBUFSZ    256     /* Size of output buffer */

static char out_buf[OUTBUFSZ];
static int out_count = 0;

static int jsym_next = -1;
static int quote_flag = 0;

/*
 * Setup terminal modes.
 */
void ttstartup()
{
#ifdef TERMIOS
    {
    struct termios param;
    register int i;

    tcgetattr(0, &tioparam);
    param = tioparam;

    for (i=0; i<NCC; i++)
        param.c_cc[i] = 0377;

    if (param.c_cc[VINTR] == 0177)
        param.c_cc[VINTR] = 3;
    param.c_cc[VMIN ] = 1;
    param.c_cc[VTIME] = 2;

    /* input modes */
    param.c_iflag &= ~ICRNL;

    /* output modes */
    param.c_oflag &= ~OPOST;

    /* control modes */
    param.c_cflag |= CREAD;

    /* local modes */
    param.c_lflag = ECHOK | ISIG;

    tcsetattr(0, TCSADRAIN, &param);
    }
#else /* TERMIOS */
#ifdef TIOCGETC
    struct tchars tcharsw;
    struct ltchars ltc;

    ioctl(0, TIOCGETC, &tchars0);
    ioctl(0, TIOCGLTC, &ltchars0);
    tcharsw = tchars0;
    tcharsw.t_eofc = -1;        /* end-of-file */
    tcharsw.t_quitc = -1;       /* quit */
    tcharsw.t_intrc = -1;	/* interrupt */
    ltc = ltchars0;
    ltc.t_suspc = -1;           /* stop process */
    ltc.t_dsuspc = -1;          /* delayed stop process */
    ltc.t_rprntc = -1;          /* reprint line */
    ltc.t_flushc = -1;          /* flush output */
    ltc.t_werasc = -1;          /* word erase */
    ltc.t_lnextc = -1;          /* literal next character */
    ioctl(0, TIOCSETC, &tcharsw);
    ioctl(0, TIOCSLTC, &ltc);
#endif
#ifdef TIOCGETP
    {
    struct sgttyb templw;

    ioctl(0, TIOCGETP, &templ);
    templw = templ;

    templw.sg_flags &= ~(ECHO | CRMOD | XTABS | RAW);
    templw.sg_flags |= CBREAK;

    ioctl(0, TIOCSETP, &templw);
    }
#endif
#endif /* TERMIOS */
}

/*
 * Restore terminal modes.
 */
void ttcleanup()
{
#ifdef TERMIOS
    tcsetattr(0, TCSADRAIN, &tioparam);
#else /* TERMIOS */
    ioctl(0, TIOCSETP, &templ);
    ioctl(0, TIOCSETC, &tchars0);
    ioctl(0, TIOCSLTC, &ltchars0);
#endif /* TERMIOS */
}

/*
 * Output a raw character, buffered.
 */
static void putcbuf(c)
    int c;
{
    out_buf[out_count++] = c;
    if (out_count == OUTBUFSZ)
        dumpcbuf();
}

/*
 * Move screen cursor to given coordinates.
 */
void pcursor(col, lin)
    int col, lin;
{
    register char *c, sy;

    c = tgoto (curspos, col, lin);
    while ((sy = *c++)) {
        putcbuf(sy);
    }
}

/*
 * Output a symbol or output control code.
 * Return 0 on error.
 */
int putch(c)
    int c;
{
    register int cr;
    register char *s;

    cr = (unsigned char) c;
    if (cr >= 0 && cr <= COMCOD) {
        s = cvtout[cr];
        if (! s)
            return(0);
        while ((cr = *s++) != 0)
            putcbuf(cr);
    } else {
        out_buf[out_count++] = c;
        if (out_count == OUTBUFSZ)
            dumpcbuf();
    }
    return(1);
}

/*
 * Output a line of spaces.
 */
void putblanks(k)
    register int k;
{
    cursorcol += k;
    while (k--) {
        out_buf[out_count++] = ' ';
        if (out_count == OUTBUFSZ)
            dumpcbuf();
    }
    dumpcbuf();
}

/*
 * Flush output buffer.
 */
void dumpcbuf()
{
    if (out_count == 0)
        return;
    if (write(2, out_buf, out_count) != out_count)
        /* ignore errors */;
    out_count = 0;
}

/*
 * Decode an input key code.
 * Values *fb, *fe are NULL on first call,
 * then used for key search.
 * Return values:
 *      CONTF - need next char;
 *      BADF  - no such keycode;
 *      >=0   - keycode detected.
 */
static int findt (fb, fe, sy, ns)
    keycode_t **fb, **fe;
    char sy;
    int ns;
{
    char sy1;
    register keycode_t *fi;

    fi = *fb ? *fb : keytab;
    *fb = 0;
    if (sy == 0)
        return BADF;
    for (; fi != *fe; fi++) {
        if (! *fe && ! fi->value)
            goto exit;
        sy1 = fi->value[ns];
        if (*fb) {
            if (sy != sy1)
                goto exit;
        } else {
            if (sy == sy1) {
                *fb = fi;
            }
        }
    }
exit:
    *fe = fi; /* for "addkey" */
    if (! *fb)
        return BADF;
    fi = *fb;
    if (fi->value[ns + 1])
        return CONTF;
    return fi->keysym;
}

/*
 * Read a command from the journal file.
 * Return 0 on EOF.
 */
static int readfc()
{
    char sy1 = CCQUIT;
    do {
        keysym = jsym_next;
        if (intrflag || read(inputfile, &sy1, 1) != 1) {
            if (inputfile != journal)
                close(inputfile);
            else
                lseek(journal, (long) -1, 1);
            inputfile = 0;
            intrflag = 0;
            putch(COBELL);
            dumpcbuf();
            return 0;
        }
        jsym_next = (unsigned char) sy1;
    } while (keysym < 0);
    return 1;
}

/*
 * Read a symbol from the user terminal, or from journal.
 * Received symbol or keycode is stored in keysym.
 */
int getkeysym()
{
    keycode_t *i1, *i2;
    int ts, k;
    char sy1;

    dumpcbuf();

    /* Previous symbol not consumed yet. */
    if (keysym != -1)
        return keysym;

    /* Read from a jornal file. */
    if (inputfile != 0 && readfc())
        return keysym;

    /* Read from a terminal keyboard. */
new:
    intrflag = 0;
    if (read(inputfile, &sy1, 1) != 1)
        goto readquit;

    keysym = (unsigned char) sy1;
    if (keysym == 0177) {
        keysym = CCBACKSPACE;
        goto readychr;
    }
    if (keysym > 037)
        goto readychr;
    if (quote_flag) {
        keysym += 0100;
        goto readychr;
    }
    /* Decode ^X sequences. */
    if (keysym == ('X' & 037)) {
        if (read(inputfile, &sy1, 1) != 1)
            goto readquit;

        switch (sy1) {
        case 'C' & 037:     /* ^X ^C */
            keysym = CCQUIT;
            goto readychr;
        case 'n':           /* ^X n */
        case 'N':           /* ^X N */
            keysym = CCPGDOWN;
            goto readychr;
        case 'p':           /* ^X p */
        case 'P':           /* ^X P */
            keysym = CCPGUP;
            goto readychr;
        case 'f':           /* ^X f */
        case 'F':           /* ^X F */
            keysym = CCROFFSET;
            goto readychr;
        case 'b':           /* ^X b */
        case 'B':           /* ^X B */
            keysym = CCLOFFSET;
            goto readychr;
        case 'h':           /* ^X h */
        case 'H':           /* ^X H */
            keysym = CCHOME;
            goto readychr;
        case 'e':           /* ^X e */
        case 'E':           /* ^X E */
            keysym = CCEND;
            goto readychr;
        case 'i':           /* ^X i */
        case 'I':           /* ^X I */
            keysym = CCINSMODE;
            goto readychr;
        case 'x':           /* ^X x */
        case 'X':           /* ^X X */
            keysym = CCDOCMD;
            goto readychr;
        default:
            ;
        }
        keysym = sy1 & 037;
        goto corrcntr;
    }
    /* Control code detected - search in the table. */
    i1 = i2 = 0;
    ts = 0;
    sy1 = keysym;
    while ((k = findt(&i1, &i2, sy1, ts++)) == CONTF) {
        if (read(inputfile, &sy1, 1) != 1)
            goto readquit;
    }
    if (k == BADF) {
        if (ts == 1)
            goto corrcntr;
        goto new;
    }
    keysym = k;
    goto readychr;

corrcntr:
    if (keysym > 0 && keysym <= 037)
        keysym = in0tab[keysym];
readychr:
    if (keysym == -1)
        goto new;
    quote_flag = 0;
    if (keysym == CCCTRLQUOTE) {
        quote_flag = 1;
    }
    sy1 = keysym;
    if (write (journal, &sy1, 1) != 1)
        /* ignore errors */;
    keysym = (unsigned char) keysym;
    return keysym;

readquit:
    if (intrflag) {
        keysym = CCPARAM;
        intrflag = 0;
        goto readychr;
    }
    keysym = CCQUIT;
    goto readychr;
}

/*
 * We were interrupted?
 */
int interrupt()
{
    char sy1;

    if (inputfile) {
        if (jsym_next == CCINTRUP) {
            jsym_next = -1;
            return 1;
        }
        return 0;
    }
    if (intrflag) {
        intrflag = 0;
        sy1 = CCINTRUP;
        if (write(journal, &sy1, 1) != 1)
            /* ignore errors */;
        return 1;
    }
    return 0;
}
