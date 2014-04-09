/*
 * Parsing termcap options and creating a table of commands.
 *
 * RED editor for OS DEMOS
 * Alex P. Roudnev, Moscow, KIAE, 1984
 */
#include "r.defs.h"

/*
 * Output codes
 */
char *cvtout[] = {
    /* COSTART */ "cl?is?ti?ho",    /* COUP   */ "up",
    /* CODN    */ "do",             /* CORN   */ "\015",
    /* COHO    */ "ho",             /* CORT   */ "nd",
    /* COLT    */ "le",             /* COCURS */ "cu",
    /* COBELL  */ "\007",           /* COFIN  */ "cl?fs?te",
    /* COERASE */ "cl",             /* COERLN */ "ce",
};

char *curspos;

/*
 * Meanings of the input control codes: ^A - ^H
 */
const char in0tab[32] = {
    -1,             /* ^@ */
    CCPARAM,        /* ^A */
    CCMISRCH,       /* ^B */
    CCCOPY,         /* ^C */
    CCDELCH,        /* ^D */
    -1,             /* ^E */
    CCPLSRCH,       /* ^F */
    CCGOTO,         /* ^G */
    CCBACKSPACE,    /* ^H */
    CCTAB,          /* ^I */
    CCRETURN,       /* ^J */
    -1,             /* ^K */
    CCREDRAW,       /* ^L */
    CCRETURN,       /* ^M */
    CCSETFILE,      /* ^N */
    CCINSLIN,       /* ^O */
    CCCTRLQUOTE,    /* ^P */
    -1 /*special*/, /* ^Q */
    -1,             /* ^R */
    -1 /*special*/, /* ^S */
    -1,             /* ^T */
    -1,             /* ^U */
    CCPASTE,        /* ^V */
    -1,             /* ^W */
    -1 /*special*/, /* ^X */
    CCDELLIN,       /* ^Y */
    -1,             /* ^Z */
    -1 /*special*/, /* ^[ */
    -1,             /* ^\ */
    -1,             /* ^] */
    -1,             /* ^^ */
    -1,             /* ^_ */
};

/*
 * Table of input key codes.
 */
keycode_t keytab[] = {
    { CCMOVEUP,     "ku",   },  { CCMOVEUP,     "\33OA",   },
    { CCMOVEDOWN,   "kd",   },  { CCMOVEDOWN,   "\33OB",   },
    { CCMOVERIGHT,  "kr",   },  { CCMOVERIGHT,  "\33OC",   },
    { CCMOVELEFT,   "kl",   },  { CCMOVELEFT,   "\33OD",   },
    { CCHOME,       "kh",   },  { CCHOME,       "\33OH",   },
    { CCEND,        "kH",   },  { CCEND,        "\33OF",   },
    { CCPGDOWN,     "kN",   },
    { CCPGUP,       "kP",   },
    { CCINSMODE,    "kI",   },
    { CCDELCH,      "kD",   },
    { CCPARAM,      "k1",   },  { CCPARAM,      "\33OP",   },
    { CCSAVEFILE,   "k2",   },  { CCSAVEFILE,   "\33OQ",   },
    { CCSETFILE,    "k3",   },  { CCSETFILE,    "\33OR",   },
    { CCDOCMD,      "k4",   },  { CCDOCMD,      "\33OS",   },
    { CCCOPY,       "k5",   },  { CCCOPY,       "\33OT",   },
    { CCPASTE,      "k6",   },
    { CCPLSRCH,     "k7",   },
    { CCGOTO,       "k8",   },
    { CCREDRAW,     "k9",   }, // free
    { CCREDRAW,     "k0",   }, // free
    { CCREDRAW,     "F1",   }, // free
    { CCREDRAW,     "F2",   }, // free
    { 0,            0,      },
    { 0,            0,      },
    { 0,            0,      },
    { 0,            0,      },
    { 0,            0,      },
    { 0,            0,      },
    { 0,            0,      },
    { 0,            0,      },
    { 0,            0,      },
};

/*
 * Parsing the termcap
 */

/*
 * Get a descriptor "tc".
 * tc="XXYY..ZZ"
 * Auestion mark bebore the code denodes an optional feature.
 */
static char *gettcs(termcap, tc)
    char *termcap, *tc;
{
    char name[3], buftc0[128], *buftc = buftc0;
    register char *c;
    int optional;

    if (*tc < ' ' && *tc >= 0)
        return(tc);
    c = tc;
    while (*c != 0) {
        if (*c == '?') {
            optional = 1;
            c++;
        } else if (*c < ' ') {
            *buftc++ = *c++;
            continue;
        } else
            optional = 0;
        name[0] = *c++;
        name[1] = *c++;
        name[2] = 0;
        if (tgetstr(termcap, name, &buftc) == 0) {
            /* not found */
            if (! optional)
                return(0);
            *buftc++ = 0;
        }
        buftc--;
    }
    *buftc++ = 0;
    buftc = strdup (buftc0);
#ifdef TEST
    printf("%s=", tc);
    ptss(buftc);
#endif
    return(buftc);
}

/*
 * Sort the keytab for the function findt().
 */
static void itsort(fb, fe, ns)
    keycode_t *fb,*fe;
    int ns;
{
    register keycode_t *fr, *fw;
    char c;
    keycode_t temp;

    fw = fb - 1;
    while (fw != fe) {
        fr = fb = ++fw;
        c = fw->value[ns];
        while (fr++ != fe) {
            if (fr->value[ns] == c) {
                if (fr != ++fw) {
                    temp = *fr;
                    *fr = *fw;
                    *fw = temp;
                }
            }
        }
        if (c != 0 && (fw - fb) > 1)
            itsort(fb, fw, ns+1);
    }
}

/*
 * Load termcap descriptions.
 */
void tcread()
{
    register int i;
    register keycode_t *ir, *iw;
    char *termcap;

    /* Terminal description is placed in TERMCAP variable. */
    termcap = getenv("TERMCAP");
    if (! termcap) {
        /* Default: linux console. */
        termcap = ":co#80:li#25:cm=\33[%i%d;%dH:cl=\33[H\33[2J:ho=\33[H:"
                  "up=\33[A:do=\33[B:nd=\33[C:le=\10:cu=\33[7m@\33[m:"
                  "ku=\33[A:kd=\33[B:kr=\33[C:kl=\33[D:kP=\33[5~:kN=\33[6~:"
                  "kI=\33[2~:kD=\33[3~:kh=\33[1~:kH=\33[4~:"
                  "k1=\33[A:k2=\33[B:k3=\33[C:k4=\33[D:k5=\33[15~:"
                  "k6=\33[17~:k7=\33[18~:k8=\33[19~:k9=\33[20~:k0=\33[21~:"
                  "F1=\33[23~:F2=\33[24~:";
    }
    curspos = gettcs(termcap, "cm");
    if (! curspos) {
        puts1 ("re: terminal does not support direct positioning\n");
        exit(1);
    }
    NCOLS = tgetnum(termcap, "co");
    NLINES = tgetnum(termcap, "li");
    if (NCOLS <= 60 || NLINES < 8) {
        puts1 ("re: too small screen size\n");
        exit(1);
    }
    if (NCOLS > MAXCOLS)
        NCOLS = MAXCOLS;
    if (NLINES > MAXLINES)
        NLINES = MAXLINES;
    for (i=0; i<COMCOD; i++) {
        cvtout[i] = gettcs(termcap, cvtout[i]);
    }
    if (tgetflag(termcap, "nb"))
        cvtout[COBELL] = "\0";
    if (! cvtout[COCURS])
        cvtout[COCURS] = "@";
    if (! cvtout[COERLN])
        cvtout[COERLN] = "\33[K";

    /* Input codes. */
    iw = keytab;
    for (ir=iw; ir->value; ir++) {
        if (ir->value[0] > ' ') {
            iw->value = gettcs(termcap, ir->value);
            if (iw->value == 0) {
                continue;
            }
        } else
            iw->value = ir->value;
        iw->keysym = ir->keysym;
        iw++;
    }
    iw->value = NULL;
    iw->keysym = 0;
    itsort(keytab, iw-1, 0);
}
