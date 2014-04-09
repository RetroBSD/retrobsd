/*
 * Global definitions.
 *
 * RED editor for OS DEMOS
 * Alex P. Roudnev, Moscow, KIAE, 1984
 */
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef DEBUG
#include <stdio.h>
#endif

/*
 * DEBUGCHECK: check consistency of segment chains.
 */
#ifdef DEBUG
#   define DEBUGCHECK checksegm()
#else
#   define DEBUGCHECK /* */
#endif

#define MOVECMD(x)  ((x) >= CCMOVEUP && (x) <= CCEND)
#define CTRLCHAR(x) (((x) >= 0 && (x) < ' ') || ((x) >= 0177 && (x) < 0240))
#define MAXCOLS     256     /* max. width of screen */
#define MAXLINES    64      /* max. height of screen */
#define LBUFFER     256     /* lower limit for the current line buffer */
#define PARAMWIDTH  (NCOLS-18) /* input field of paramwin */
#define FILEMODE    0664    /* access mode for newly created files */
#define MAXFILES    14      /* max. files under edit */
#define MAXWINLIST  10      /* max.windows */

#ifndef MULTIWIN
#define MULTIWIN    1       /* enable splitting a screen into multiple windows */
#endif

#define BADF        -1
#define CONTF       -2

/* OUTPUT CODES */

#define COSTART     0
#define COUP        1
#define CODN        2
#define CORN        3
#define COHO        4
#define CORT        5
#define COLT        6
#define COCURS      7
#define COBELL      8
#define COFIN       9
#define COERASE     10
#define COERLN      11
#define COMCOD      12      /* Number of output codes */

/* margin characters */
#define LMCH        '|'
#define RMCH        '|'
#define TMCH        '-'
#define BMCH        '-'
#define MRMCH       '>'
#define MLMCH       '<'
#define ELMCH       ';'
#define DOTCH       '+'

/*
 * Descriptor of file segment.  Contains from 1 to 127 lines of file,
 * written sequentially.  An elementary component of descriptor chain.
 */
#define FSDMAXL     127     /* Max. number of lines in descriptor */

typedef struct segment {
    struct segment *prev;   /* Previous descriptor in chain */
    struct segment *next;   /* Next descriptor in chain */
    int nlines;             /* Count of lines in descriptor,
                             * or 0 when end of chain */
    int fdesc;              /* File descriptor, or 0 for end of chain */
    off_t seek;             /* Byte offset in the file */
    char data;              /* Varying amount of data, needed
                             * to store the next nlines-1 lines. */
    /*
     * Interpretation of next byte:
     * 1-127   offset of this line from a previous line
     * 0       empty line
     * -n      line starts at offset n*128+next bytes
     *         from beginning of previous line.
     * There are at least nlines-1 bytes allocated or more,
     * in case of very long lines.
     */
} segment_t;

#define sizeof_segment_t    offsetof (segment_t, data)

typedef struct {
    segment_t *chain;       /* Chain of segments */
    char *name;             /* File name */
    char writable;          /* Do we have a write permission */
#define EDITED  2           /* Value when file was modified */

    char backup_done;       /* Already copied to .bak */
    int nlines;             /* Number of non-empty lines in file */
} file_t;

file_t file[MAXFILES];     /* Table of files */
int curfile;

/*
 * Workspace describes a generic file under edit.
 */
typedef struct {
    segment_t *cursegm;     /* Current segment */
    int topline;            /* Top left row on a display */
    int offset;             /* Column offset of a displayed text */
    int line;               /* Current line number */
    int segmline;           /* Number of first line in the current segment */
    int wfile;              /* File number, or 0 when not attached */
    int cursorcol;          /* Saved cursorcol, when not active */
    int cursorrow;          /* Saved cursorline, when not active */
} workspace_t;

workspace_t *curwksp, *pickwksp;

/*
 * Window - a region on the display.
 * All display coordinates, and also text_col and text_row, are measured relative
 * to (0,0) = top left of screen.
 */
typedef struct {
    workspace_t *wksp;      /* Pointer to workspace */
    workspace_t *altwksp;   /* Alternative workspace */
    int prevwinnum;         /* Number of previous window */
    int base_row;           /* Top row of the window */
    int base_col;           /* Left column of the window */
    int max_row;            /* Bottom row of the window */
    int max_col;            /* Right column of the window */
    int text_row;           /* Top row of text area */
    int text_col;           /* Left column of text area */
    int text_maxrow;        /* Height-1 of text area */
    int text_maxcol;         /* Width-1 of text area */
    unsigned char *firstcol; /* Numbers of first non-space symbols */
    unsigned char *lastcol; /* Numbers of last non-space symbols */
    char *leftbar;          /* Symbols on left edge */
    char *rightbar;         /* Symbols on right edge */
} window_t;

window_t *winlist[MAXWINLIST];
int nwinlist;

window_t *curwin;           /* Current window */
window_t wholescreen;       /* The whole screen */
window_t paramwin;          /* Window to enter arguments */

/*
 * Copy-and-paste clipboard.
 */
typedef struct {
    int linenum;            /* Index of first line in "#" */
    int nrows;              /* Number of lines */
    int ncolumns;           /* Number of columns */
} clipboard_t;

clipboard_t *pickbuf, *deletebuf;

/*
 * Control codes.
 */
#define CCCTRLQUOTE 0       /* knockdown next char  ^P              */
#define CCMOVEUP    1       /* move up 1 line               up      */
#define CCMOVEDOWN  2       /* move down 1 line             down    */
#define CCRETURN    3       /* return               ^M              */
#define CCHOME      4       /* home cursor          ^X h    home    */
#define CCMOVERIGHT 5       /* move right                   right   */
#define CCMOVELEFT  6       /* move left                    left    */
#define CCTAB       7       /* tab                  ^I              */
#define CCPGDOWN    010     /* next page            ^X n    pg down */
#define CCPGUP      011     /* previous page        ^X p    page up */
#define CCEND       012     /* cursor to end        ^X e    end     */
#define CCCOPY      013     /* copy to clipboard    ^C      f5      */
#define CCMAKEWIN   014     /* make a window        --disabled--    */
#define CCINSLIN    015     /* insert line/block    ^O              */
#define CCSETFILE   016     /* set file             ^N      f3      */
#define CCCHWINDOW  017     /* change window        --disabled--    */
#define CCGOTO      020     /* goto linenumber      ^G      f8      */
#define CCDOCMD     021     /* execute a filter     ^X x    f4      */
#define CCPLSRCH    022     /* plus search          ^F      f7      */
#define CCROFFSET   023     /* shift view right     ^X f            */
#define CCDELCH     024     /* character delete     ^D      delete  */
#define CCSAVEFILE  025     /* make new file                f2      */
#define CCMISRCH    030     /* minus search         ^B              */
#define CCLOFFSET   031     /* shift view left      ^X b            */
#define CCPASTE     032     /* paste from clipboard ^V      f6      */
#define CCREDRAW    033     /* redraw all           ^L              */
#define CCINSMODE   034     /* insert mode          ^X i    insert  */
#define CCBACKSPACE 035     /* backspace and erase  ^H              */
#define CCDELLIN    036     /* delete line/block    ^Y              */
#define CCPARAM     037     /* enter parameter      ^A      f1      */
#define CCQUIT      0177    /* terminate session    ^X ^C           */
#define CCINTRUP    0237    /* interrupt (journal) */

int cursorline;             /* physical position of */
int cursorcol;              /* cursor from (0,0)=ulhc of text in window */

int NCOLS, NLINES;          /* size of the screen */

extern char *curspos, *cvtout[];
extern char cntlmotions[];
extern const char in0tab[]; /* input control codes */

extern int keysym;          /* Current input symbol, -1 - need more */
char intrflag;              /* INTR signal occured */
int highlight_position;     /* Highlight the current cursor position */
int message_displayed;      /* Arg area contains an error message */

/* Defaults. */
extern int defloffset, defroffset;
extern char deffile[];

/*
 * Global variables for param().
 * param_len    - length of the parameter;
 * param_str    - string value of the parameter,
 * param_type - type of the parameter,
 * param_c0, param_r0, param_c1,
 * param_r1   - coordinates of cursor-defined area
 */
int param_len;
char *param_str, param_type;
int param_c0, param_r0, param_c1, param_r1;

/*
 * Current line.
 */
char *cline;                /* allocated array */
int cline_max;              /* allocated length */
int cline_len;              /* used length */
int cline_incr;             /* increment for reallocation */
char cline_modified;        /* line was modified */
int clineno;                /* line number in file */

/*
 * File descriptors:
 */
int tempfile;               /* Temporary file */
off_t tempseek;             /* Offset in temporary file */
int journal;                /* Journaling file */
int inputfile;              /* Input file (stdin or journal) */

char *searchkey;

int userid, groupid;

char *tmpname;              /* name of file, for do command */

/*
 * Translation of control codes to escape sequences.
 */
typedef struct {
    int keysym;
    char *value;
} keycode_t;

extern keycode_t keytab[];

int getkeysym (void);           /* read command from terminal */
void getlin (int);              /* get a line from current file */
void putline (void);            /* put a line to current file */
void movecursor (int);          /* cursor movement operation */
void poscursor (int, int);      /* position a cursor in current window */
void pcursor (int, int);        /* move screen cursor */
void drawlines (int, int);      /* show lines of file */
void cline_expand (int);        /* extend cline array */
int putch (int);                /* output symbol or op */
void wputc (int, int);          /* put a symbol to a current window */
void wputs (char *, int);       /* put a string to a current window */
void putblanks (int);           /* output a line of spaces */
int endit (void);               /* end a session and write all */
void switchfile (void);         /* switch to alternative file */
void search (int);              /* search a text in file */
void paste (clipboard_t *, int, int); /* put a buffer into file */
int mfetch (clipboard_t *, char *); /* get a buffer by name */
void mstore (clipboard_t *, char *); /* store a buffer by name */
void gtfcn (int);               /* go to line by number */
int savefile (char *, int);     /* save a file */
void error (char *);            /* display error message */
char *param (void);             /* get a parameter */
int cline_read (int);           /* read next line from file */
int int_to_ext (char *, int);   /* conversion of line from internal to external form */
void cleanup (void);            /* cleanup before exit */
void fatal (char *);            /* print error message and exit */
int editfile (char *, int, int, int, int); /* open a file for editing */
void splitline (int, int);      /* split a line at given position */
void combineline (int, int);    /* merge a line with next one */
int msvtag (char *name);        /* save a file position by name */
int mdeftag (char *);           /* define a file area by name */
void redisplay (void);          /* redraw all */
int mgotag (char *);            /* return a cursor back to named position */
void execr (char **);           /* run command with given parameters */
void telluser (char *, int);    /* display a message in arg area */
void doreplace(int, int, int, int*); /* replace lines via pipe */
void win_open (char *);         /* make new window */
void win_remove (void);         /* remove last window */
void win_goto (int);            /* switch to window by number */
void win_switch (window_t *);   /* switch to given window */
void win_borders (window_t *, int); /* draw borders for a window */
void win_create (window_t *, int, int, int, int, int);
                                /* create new window */
void dumpcbuf (void);           /* flush output buffer */
char *s2i (char *, int *);      /* convert a string to number */
char *salloc (int);             /* allocate zeroed memory */
void insertlines (int, int);    /* insert lines */
void deletelines (int, int);    /* delete lines from file */
void picklines (int, int);      /* get lines from file to pick workspace */
void openspaces (int, int, int, int); /* insert spaces */
void closespaces (int, int, int, int); /* delete rectangular area */
void pickspaces (int, int, int, int); /* get rectangular area to pick buffer */
int interrupt (void);           /* we have been interrupted? */
void wksp_switch (void);        /* switch to alternative workspace */
int wksp_seek (workspace_t *, int); /* set file position by line number */
int wksp_position (workspace_t *, int); /* set workspace position */
void wksp_forward (int);        /* move page down or up */
void wksp_offset (int);         /* shift a text view to right or left */
void wksp_redraw (workspace_t *, int, int, int, int);
                                /* redisplay windows of the file */
void cgoto (int, int, int, int); /* scroll window to show a given area */
char *append (char *, char *);  /* append strings */
char *tgoto (char *, int, int); /* cursor addressing */
segment_t *file2segm (int);     /* create a file descriptor list for a file */
void puts1 (char *);            /* write a string to stdout */
void checksegm (void);          /* check segm correctness */
int segmwrite (segment_t *, int, int); /* write descriptor chain to file */
void printsegm (segment_t *);   /* debug output of segm chains */
int checkpriv (int);            /* check access rights */
int getpriv (int);              /* get file access modes */
void tcread (void);             /* load termcap descriptions */
char *tgetstr(char *, char *, char **); /* get string option from termcap */
int tgetnum (char *, char *);   /* get a numeric termcap option */
int tgetflag (char *, char *);  /* get a flag termcap option */
char *getnm (int);              /* get user id as printable text */
void ttstartup (void);          /* setup terminal modes */
void ttcleanup (void);          /* restore terminal modes */
int get1w (int);                /* read word */
int get1c (int);                /* read byte */
void put1w (int, int);          /* write word */
void put1c (int, int);          /* write byte */
void mainloop (void);           /* main editor loop */
