/*
 * tiny vi.c: A small 'vi' clone
 * Copyright (C) 2000, 2001 Sterling Huxley <sterling@europa.com>
 *
 * Licensed under the GPL v2 or later, see the file LICENSE in this tarball.
 * Revised:  4/23/20 brent@mbari.org -- NULL ptr deref on missing previous regex
 * Revised:	 5/21/20 brent@mbari.org -- extensive rework
 */

/*
 * Things To Do:
 *	EXINIT
 *	$HOME/.exrc  and  ./.exrc
 *	add magic to search	/foo.*bar
 *	add :help command
 *	:map macros
 *	if mark[] values were line numbers rather than pointers
 *	   it would be easier to change the mark when add/delete lines
 *	More intelligence in refresh()
 *	":r !cmd"  and  "!cmd"  to filter text through an external command
 *	A true "undo" facility
 *	An "ex" line oriented mode- maybe using "cmdedit"
 */
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef TERMIOS
#include <termios.h>
#endif

#define BB_VER "version 2.62"
#define BB_BT "brent@mbari.org"

#define vi_main main
#define CONFIG_FEATURE_VI_MAX_LEN 4096
#define ENABLE_FEATURE_VI_COLON 1
#define ENABLE_FEATURE_VI_YANKMARK 1
#define ENABLE_FEATURE_VI_SEARCH 1
#define ENABLE_FEATURE_VI_USE_SIGNALS 1
#define ENABLE_FEATURE_VI_DOT_CMD 1
#define ENABLE_FEATURE_VI_READONLY 1
#define ENABLE_FEATURE_VI_SETOPTS 1
#define ENABLE_FEATURE_VI_SET 1
#define ENABLE_FEATURE_VI_WIN_RESIZE 1
#define ENABLE_LOCALE_SUPPORT 1
#define ENABLE_FEATURE_VI_8BIT 1
#define ENABLE_FEATURE_VI_YANKMARK 1
#define ENABLE_FEATURE_VI_SEARCH 1
#define ENABLE_FEATURE_ALLOW_EXEC 1
#undef ENABLE_FEATURE_VI_OPTIMIZE_CURSOR

#define USE_FEATURE_VI_COLON(...) __VA_ARGS__
#define USE_FEATURE_VI_READONLY(...) __VA_ARGS__
#define USE_FEATURE_VI_YANKMARK(...) __VA_ARGS__
#define USE_FEATURE_VI_SEARCH(...) __VA_ARGS__

#define ALIGN1
#define FALSE 0
#define TRUE 1
#define MAIN_EXTERNALLY_VISIBLE
#define ATTRIBUTE_UNUSED __attribute__((__unused__))

#undef isdigit
#define isdigit(a) ((unsigned)((a) - '0') <= 9)

#define ARRAY_SIZE(x) ((unsigned)(sizeof(x) / sizeof((x)[0])))

#define INIT_G()
struct globals G;

typedef signed char smallint;

#define bb_show_usage()
#define bb_perror_msg(msg) perror(msg)

// To test editor using CRASHME:
//    vi -C filename
// To stop testing, wait until all to text[] is deleted, or
//    Ctrl-Z and kill -9 %1
// while in the editor Ctrl-T will toggle the crashme function on and off.
// #define CONFIG_FEATURE_VI_CRASHME		// randomly pick commands to execute

/* the CRASHME code is unmaintained, and doesn't currently build */
#define ENABLE_FEATURE_VI_CRASHME 0

#if ENABLE_LOCALE_SUPPORT

#if ENABLE_FEATURE_VI_8BIT
#define Isprint(c) isprint(c)
#else
#define Isprint(c) (isprint(c) && (unsigned char)(c) < 0x7f)
#endif

#else

/* 0x9b is Meta-ESC */
#if ENABLE_FEATURE_VI_8BIT
#define Isprint(c) ((unsigned char)(c) >= ' ' && (c) != 0x7f && (unsigned char)(c) != 0x9b)
#else
#define Isprint(c) ((unsigned char)(c) >= ' ' && (unsigned char)(c) < 0x7f)
#endif

#endif

#if ENABLE_FEATURE_VI_READONLY
#define EDIT_STATUS "%s: %s%s%s line %d/%d %d%%"
#else
#define EDIT_STATUS "%s: %s%s line %d/%d %d%%"
#endif

enum {
    MAX_TABSTOP = 32, // sanity limit
    // User input len. Need not be extra big.
    // Lines in file being edited *can* be bigger than this.
    MAX_INPUT_LEN = 128,
    // Sanity limits. We have only one buffer of this size.
    MAX_SCR_COLS = CONFIG_FEATURE_VI_MAX_LEN,
    MAX_SCR_ROWS = CONFIG_FEATURE_VI_MAX_LEN,
};

// Misc. non-Ascii keys that report an escape sequence
#define VI_K_UP (char)128       // cursor key Up
#define VI_K_DOWN (char)129     // cursor key Down
#define VI_K_RIGHT (char)130    // Cursor Key Right
#define VI_K_LEFT (char)131     // cursor key Left
#define VI_K_HOME (char)132     // Cursor Key Home
#define VI_K_END (char)133      // Cursor Key End
#define VI_K_INSERT (char)134   // Cursor Key Insert
#define VI_K_DELETE (char)135   // Cursor Key Insert
#define VI_K_PAGEUP (char)136   // Cursor Key Page Up
#define VI_K_PAGEDOWN (char)137 // Cursor Key Page Down
#define VI_K_FUN1 (char)138     // Function Key F1
#define VI_K_FUN2 (char)139     // Function Key F2
#define VI_K_FUN3 (char)140     // Function Key F3
#define VI_K_FUN4 (char)141     // Function Key F4
#define VI_K_FUN5 (char)142     // Function Key F5
#define VI_K_FUN6 (char)143     // Function Key F6
#define VI_K_FUN7 (char)144     // Function Key F7
#define VI_K_FUN8 (char)145     // Function Key F8
#define VI_K_FUN9 (char)146     // Function Key F9
#define VI_K_FUN10 (char)147    // Function Key F10
#define VI_K_FUN11 (char)148    // Function Key F11
#define VI_K_FUN12 (char)149    // Function Key F12

/* vt102 typical ESC sequence */
/* terminal standout start/normal ESC sequence */
#define SOlen (4) // length of SO escape sequence
static const char SOs[] ALIGN1 = "\033[7m";
static const char SOn[] ALIGN1 = "\033[0m";
/* terminal bell sequence */
static const char bell[] ALIGN1 = "\007";
/* Clear-end-of-line and Clear-end-of-screen ESC sequence */
static const char Ceol[] ALIGN1 = "\033[0K";
static const char Ceos[] ALIGN1 = "\033[0J";
/* Cursor motion arbitrary destination ESC sequence */
static const char CMrc[] ALIGN1 = "\033[%d;%dH";
#if ENABLE_FEATURE_VI_WIN_RESIZE
/* Report cursor positon */
static const char CtextAreaQuery[] ALIGN1 = "\033[r\033[999;999H\033[6n";
#endif
#ifdef ENABLE_FEATURE_VI_OPTIMIZE_CURSOR
/* Cursor motion up and down ESC sequence */
static const char CMup[] ALIGN1 = "\033[A";
static const char CMdown[] ALIGN1 = "\n";
#endif

enum {
    YANKONLY = FALSE,
    YANKDEL = TRUE,
    FORWARD = 1, // code depends on "1"  for array index
    BACK = -1,   // code depends on "-1" for array index
    LIMITED = 0, // how much of text[] in char_search
    FULL = 1,    // how much of text[] in char_search

    S_BEFORE_WS = 1, // used in skip_thing() for moving "dot"
    S_TO_WS = 2,     // used in skip_thing() for moving "dot"
    S_OVER_WS = 3,   // used in skip_thing() for moving "dot"
    S_END_PUNCT = 4, // used in skip_thing() for moving "dot"
    S_END_ALNUM = 5, // used in skip_thing() for moving "dot"
};

enum { // cmd_modes
    CMODE_COMMAND,
    CMODE_INSERT,
    CMODE_REPLACE,
    CMODES,
    CMODE_LINE_INPUT = 1 << 4
};

static const char *cmd_mode_indicator[] = { "COMMAND", "INSERT", "REPLACE", "?!?" };

/* vi.c expects chars to be unsigned. */
/* busybox build system provides that, but it's better */
/* to audit and fix the source */

struct globals {
    /* many references - keep near the top of globals */
    char *text, *end; // pointers to the user data in memory
    char *dot;        // where all the action takes place
    int text_size;    // size of the allocated buffer

    /* the rest */
    smallint vi_setops;
#define VI_AUTOINDENT 1
#define VI_SHOWMATCH 2
#define VI_IGNORECASE 4
#define VI_ERR_METHOD 8
#define autoindent (vi_setops & VI_AUTOINDENT)
#define showmatch (vi_setops & VI_SHOWMATCH)
#define ignorecase (vi_setops & VI_IGNORECASE)
/* indicate error with beep or flash */
#define err_method (vi_setops & VI_ERR_METHOD)

#if ENABLE_FEATURE_VI_READONLY
    smallint readonly_mode;
#define SET_READONLY_FILE(flags) ((flags) |= 0x01)
#define SET_READONLY_MODE(flags) ((flags) |= 0x02)
#define UNSET_READONLY_FILE(flags) ((flags) &= 0xfe)
#else
#define SET_READONLY_FILE(flags) ((void)0)
#define SET_READONLY_MODE(flags) ((void)0)
#define UNSET_READONLY_FILE(flags) ((void)0)
#endif

    smallint editing;       // >0 while we are editing a file
                            // [code audit says "can be 0 or 1 only"]
    smallint cmd_mode;      // 0=command  1=insert 2=replace
    int file_modified;      // buffer contents changed (counter, not flag!)
    int last_file_modified; // = -1;
    int fn_start;           // index of first cmd line file name
    int save_argc;          // how many file names on cmd line
    int cmdcnt;             // repetition count
    unsigned rows, columns; // the terminal screen is this size
    int crow, ccol;         // cursor is on Crow x Ccol
    int offset;             // chars scrolled off the screen to the left
    char *current_filename;
    char *screenbegin; // index into text[], of top line on the screen
    char *screen;      // pointer to the virtual screen buffer
    int screensize;    //            and its size
    int tabstop;
    char erase_char;        // the users erase character
    char last_input_char;   // last char read from user
    char last_forward_char; // last char searched for with 'f'

#if ENABLE_FEATURE_VI_DOT_CMD
    smallint adding2q;     // are we currently adding user input to q
    int lmc_len;           // length of last_modifying_cmd
    char *ioq, *ioq_start; // pointer to string for get_one_char to "read"
#endif
#if ENABLE_FEATURE_VI_OPTIMIZE_CURSOR
    int last_row; // where the cursor was last moved to
#endif
#if ENABLE_FEATURE_VI_USE_SIGNALS || ENABLE_FEATURE_VI_CRASHME
    int my_pid;
#endif
#if ENABLE_FEATURE_VI_DOT_CMD || ENABLE_FEATURE_VI_YANKMARK
    char *modifying_cmds; // cmds that modify text[]
#endif
#if ENABLE_FEATURE_VI_SEARCH
    char *last_search_pattern; // last pattern from a '/' or '?' search
#endif
    int chars_to_parse;
    /* former statics */
#if ENABLE_FEATURE_VI_YANKMARK
    char *edit_file__cur_line;
#endif
    int refresh__old_offset;
    int format_edit_status__tot;

    /* a few references only */
#if ENABLE_FEATURE_VI_YANKMARK
    int YDreg, Ureg; // default delete register and orig line for "U"
    char *reg[28];   // named register a-z, "D", and "U" 0-25,26,27
    char *mark[28];  // user marks points somewhere in text[]-  a-z and previous context ''
    char *context_start, *context_end;
#endif
#if ENABLE_FEATURE_VI_USE_SIGNALS
    sigjmp_buf restart; // catch_sig()
#endif
#ifdef TERMIOS
    struct termios term_orig, term_vi; // remember what the cooked mode was
#else
    struct sgttyb term_orig, term_vi; // remember what the cooked mode was
    struct tchars tchars_orig, tchars_vi;
    struct ltchars ltc_orig, ltc_vi;
#endif
    unsigned ticsPerChar;              // # of 100hz tics per character received
#if ENABLE_FEATURE_VI_COLON
    char *initial_cmds[3]; // currently 2 entries, NULL terminated
#endif
    // Should be just enough to hold a key sequence,
    // but CRASME mode uses it as generated command buffer too
    char readbuffer[128];
#define STATUS_BUFFER_LEN 200
    char status_buffer[STATUS_BUFFER_LEN];    // messages to the user
    char displayed_buffer[STATUS_BUFFER_LEN]; //  displayed status
#if ENABLE_FEATURE_VI_DOT_CMD
    char last_modifying_cmd[MAX_INPUT_LEN]; // last modifying cmd for "."
#endif
    char get_input_line__buf[MAX_INPUT_LEN]; /* former static */

    char scr_out_buf[MAX_SCR_COLS + MAX_TABSTOP * 2];
};
#define text (G.text)
#define text_size (G.text_size)
#define end (G.end)
#define dot (G.dot)
#define reg (G.reg)

#define vi_setops (G.vi_setops)
#define editing (G.editing)
#define cmd_mode (G.cmd_mode)
#define file_modified (G.file_modified)
#define last_file_modified (G.last_file_modified)
#define fn_start (G.fn_start)
#define save_argc (G.save_argc)
#define cmdcnt (G.cmdcnt)
#define rows (G.rows)
#define columns (G.columns)
#define crow (G.crow)
#define ccol (G.ccol)
#define offset (G.offset)
#define status_buffer (G.status_buffer)
#define displayed_buffer (G.displayed_buffer)
#define current_filename (G.current_filename)
#define screen (G.screen)
#define screensize (G.screensize)
#define screenbegin (G.screenbegin)
#define tabstop (G.tabstop)
#define erase_char (G.erase_char)
#define last_input_char (G.last_input_char)
#define last_forward_char (G.last_forward_char)
#if ENABLE_FEATURE_VI_READONLY
#define readonly_mode (G.readonly_mode)
#else
#define readonly_mode 0
#endif
#define adding2q (G.adding2q)
#define lmc_len (G.lmc_len)
#define ioq (G.ioq)
#define ioq_start (G.ioq_start)
#define last_row (G.last_row)
#define my_pid (G.my_pid)
#define modifying_cmds (G.modifying_cmds)
#define last_search_pattern (G.last_search_pattern)
#define chars_to_parse (G.chars_to_parse)

#define edit_file__cur_line (G.edit_file__cur_line)
#define refresh__old_offset (G.refresh__old_offset)
#define format_edit_status__tot (G.format_edit_status__tot)

#define YDreg (G.YDreg)
#define Ureg (G.Ureg)
#define mark (G.mark)
#define context_start (G.context_start)
#define context_end (G.context_end)
#define restart (G.restart)
#define term_orig (G.term_orig)
#define tchars_orig (G.tchars_orig)
#define ltc_orig (G.ltc_orig)
#define ticsPerChar (G.ticsPerChar)
#define term_vi (G.term_vi)
#define tchars_vi (G.tchars_vi)
#define ltc_vi (G.ltc_vi)
#define initial_cmds (G.initial_cmds)
#define readbuffer (G.readbuffer)
#define scr_out_buf (G.scr_out_buf)
#define last_modifying_cmd (G.last_modifying_cmd)
#define get_input_line__buf (G.get_input_line__buf)

static int init_text_buffer(char *); // init from file or create new
static void edit_file(char *);       // edit one file
static void do_cmd(char);            // execute a command
static int next_tabstop(int);
static void sync_cursor(char *, int *, int *);      // synchronize the screen cursor to dot
static char *begin_line(char *);                    // return pointer to cur line B-o-l
static char *end_line(char *);                      // return pointer to cur line E-o-l
static char *prev_line(char *);                     // return pointer to prev line B-o-l
static char *next_line(char *);                     // return pointer to next line B-o-l
static char *end_screen(void);                      // get pointer to last char on screen
static int count_lines(char *, char *);             // count line from start to stop
static char *find_line(int);                        // find begining of line #li
static char *move_to_col(char *, int);              // move "p" to column l
static void dot_left(void);                         // move dot left- dont leave line
static void dot_right(void);                        // move dot right- dont leave line
static void dot_begin(void);                        // move dot to B-o-l
static void dot_end(void);                          // move dot to E-o-l
static void dot_next(void);                         // move dot to next line B-o-l
static void dot_prev(void);                         // move dot to prev line B-o-l
static void dot_scroll(int, int);                   // move the screen up or down
static void dot_skip_over_ws(void);                 // move dot pat WS
static void dot_delete(void);                       // delete the char at 'dot'
static char *bound_dot(char *);                     // make sure  text[0] <= P < "end"
static char *new_screen(int, int);                  // malloc virtual screen memory
static char *char_insert(char *, char);             // insert the char c at 'p'
static char *stupid_insert(char *, char);           // stupidly insert the char c at 'p'
static int find_range(char **, char **, char);      // return pointers for an object
static int st_test(char *, int, int, char *);       // helper for skip_thing()
static char *skip_thing(char *, int, int, int);     // skip some object
static char *find_pair(char *, char);               // find matching pair ()  []  {}
static char *text_hole_delete(char *, char *);      // at "p", delete a 'size' byte hole
static char *text_hole_make(char *, int);           // at "p", make a 'size' byte hole
static char *yank_delete(char *, char *, int, int); // yank text[] into register then delete
static void show_help(void);                        // display some help info
static int rawmode(void);                           // set "raw" mode on tty
static void cookmode(void);                         // return to "cooked" mode on tty
static int awaitInput(int);                         // for specified number of 1/100th seconds
static char readit(void);                           // read (maybe cursor) key from stdin
static char get_one_char(void);                     // read 1 char from stdin
static int file_size(const char *);                 // what is the byte size of "fn"
#if ENABLE_FEATURE_VI_READONLY
static int file_insert(const char *, char *, int);
#else
static int file_insert(const char *, char *);
#endif
static int file_write(char *, char *, char *);
#if !ENABLE_FEATURE_VI_OPTIMIZE_CURSOR
#define place_cursor(a, b, optimize) place_cursor(a, b)
#endif
static void place_cursor(int, int, int);
static void screen_erase(void);
static void clear_to_eol(void);
static void clear_to_eos(void);
static void standout_start(void);           // send "start reverse video" sequence
static void standout_end(void);             // send "end reverse video" sequence
static void flash(int);                     // flash the terminal screen
static void show_status_line(void);         // put a message on the bottom line
static void status_line(const char *, ...); // print to status buf
static void status_line_bold(const char *, ...);
static void not_implemented(const char *);      // display "Not implemented" message
static int format_edit_status(const char *fmt); // file status on status line
static void redraw(void);                       // force a full screen refresh
static char *format_line(char * /*, int*/);
static void refresh(void); // update the terminal from screen[]

static void Indicate_Error(void); // use flash or beep to indicate error
#define indicate_error(c) Indicate_Error()
static void Hit_Return(void);

#if ENABLE_FEATURE_VI_SEARCH
static char *char_search(char *, const char *, int, int); // search for pattern starting at p
static int mycmp(const char *, const char *, int);        // string cmp based in "ignorecase"
#endif
#if ENABLE_FEATURE_VI_COLON
static char *get_one_address(char *, int *);    // get colon addr, if present
static char *get_address(char *, int *, int *); // get two colon addrs, if present
static void colon(char *);                      // execute the "colon" mode cmds
#endif
#if ENABLE_FEATURE_VI_USE_SIGNALS
static void winch_sig(int);   // catch window size changes
static void suspend_sig(int); // catch ctrl-Z
static void catch_sig(int);   // catch ctrl-C and alarm time-outs
static void quit_sig(int);    // catch QUIT, TERM, PIPE, or HUP
#endif
#if ENABLE_FEATURE_VI_DOT_CMD
static void start_new_cmd_q(char); // new queue for command
static void end_cmd_q(void);       // stop saving input chars
#else
#define end_cmd_q() ((void)0)
#endif
#if ENABLE_FEATURE_VI_SETOPTS
static void showmatching(char *); // show the matching pair ()  []  {}
#endif
#if ENABLE_FEATURE_VI_YANKMARK || (ENABLE_FEATURE_VI_COLON && ENABLE_FEATURE_VI_SEARCH) || \
    ENABLE_FEATURE_VI_CRASHME
static char *string_insert(char *, char *); // insert the string at 'p'
#endif
#if ENABLE_FEATURE_VI_YANKMARK
static char *text_yank(char *, char *, int); // save copy of "p" into a register
static char what_reg(void);                  // what is letter of current YDreg
static void check_context(char);             // remember context for '' command
#endif
#if ENABLE_FEATURE_VI_CRASHME
static void crash_dummy();
static void crash_test();
static int crashme = 0;
#endif

void *xmalloc(size_t size)
{
    void *ptr = malloc(size);
    if (ptr)
        return ptr;
    perror("malloc");
    exit(65);
}

void *xzalloc(size_t size)
{
    return memset(xmalloc(size), 0, size);
}

void *xrealloc(void *old, size_t size)
{
    void *ptr = realloc(old, size);
    if (ptr)
        return ptr;
    perror("realloc");
    exit(68);
}

char *xstrdup(const char *old)
{
    if (old == NULL) {
        return NULL;
    }

    size_t len = strlen(old) + 1;
    char *ptr = malloc(len);
    if (ptr == NULL) {
        perror("malloc");
        exit(69);
    }
    strcpy(ptr, old);
    return ptr;
}

char *xstrndup(const char *old, size_t n)
{
    if (old == NULL) {
        return NULL;
    }

    size_t len = strlen(old);
    if (n < len) {
        len = n; // Limit the length to n
    }

    char *ptr = malloc(len + 1); // +1 for null terminator
    if (ptr == NULL) {
        perror("malloc");
        exit(70);
    }
    strncpy(ptr, old, len); // Copy at most n characters
    ptr[len] = '\0';        // Null-terminate the string
    return ptr;
}

static void *memrchr(const void *s, int c, size_t n)
{
    if (s == NULL || n == 0) {
        return NULL;
    }

    // Use unsigned char for byte-wise comparison
    const unsigned char *p = (const unsigned char *)s;
    for (const unsigned char *i = p + n - 1; i >= p; --i) {
        if (*i == (unsigned char)c) {
            return (void *)i;
        }
    }
    return NULL;
}

//
// Return pointer to either the found character or the null terminator
//
static char *strchrnul(const char *s, int c)
{
    if (s == NULL) {
        return NULL;
    }

    const char *p = s;
    while (*p != '\0' && *p != c) {
        p++;
    }
    return (char *)p;
}

size_t strnlen(const char *s, size_t maxlen)
{
    if (s == NULL) {
        return 0;
    }

    const char *p = s;
    size_t i = 0;
    while (i < maxlen && *p != '\0') {
        p++;
        i++;
    }
    return i;
}

//
// Check for space and horizontal tab.
//
static int isblank(int c)
{
    return (c == ' ' || c == '\t');
}

static const int strncasecmp(const char *s1, const char *s2, size_t n)
{
    if (s1 == NULL || s2 == NULL) {
        return 0;
    }
    if (n == 0) {
        return 0; // Empty strings are always equal
    }
    for (size_t i = 0; i < n; i++) {
        // Cast to unsigned char for tolower
        unsigned char c1 = (unsigned char)s1[i];
        unsigned char c2 = (unsigned char)s2[i];

        if (c1 == '\0' || c2 == '\0') {
            if (c1 == c2)
                return 0;
            if (c1 == '\0')
                return -1;
            return 1;
        }

        c1 = tolower(c1);
        c2 = tolower(c2);
        if (c1 != c2) {
            // Return difference of lowercase chars
            return c1 - c2;
        }
    }
    return 0;
}

/* Find out if the last character of a string matches the one given.
 * Don't underrun the buffer if the string length is 0.
 */
char *last_char_is(const char *s, int c)
{
    if (s && *s) {
        size_t sz = strlen(s) - 1;
        s += sz;
        if ((unsigned char)*s == c)
            return (char *)s;
    }
    return NULL;
}

int bb_putchar(int ch)
{
    return putc(ch, stdout);
}

ssize_t safe_read(int fd, void *buf, size_t count)
{
    ssize_t n;

    do {
        n = read(fd, buf, count);
    } while (n < 0 && errno == EINTR);

    return n;
}

ssize_t safe_write(int fd, const void *buf, size_t count)
{
    ssize_t n;

    do {
        n = write(fd, buf, count);
    } while (n < 0 && errno == EINTR);

    return n;
}

ssize_t full_write(int fd, const void *buf, size_t len)
{
    ssize_t cc;
    ssize_t total;

    total = 0;

    while (len) {
        cc = safe_write(fd, buf, len);

        if (cc < 0) {
            if (total) {
                /* we already wrote some! */
                /* user can do another write to know the error code */
                return total;
            }
            return cc; /* write() returns -1 on failure. */
        }

        total += cc;
        buf = ((const char *)buf) + cc;
        len -= cc;
    }

    return total;
}

static void write1(const char *out)
{
    fputs(out, stdout);
}

static void clear_screen(void)
{
    place_cursor(0, 0, FALSE); // put cursor in correct place
    clear_to_eos();            // tell terminal to erase display
}

static void gracefulExit(void)
{
    cookmode();
    place_cursor(rows - 1, 0, FALSE); // go to bottom of screen
    clear_to_eol();                   // Erase to end of line
    fflush(stdout);
}

void clampScreenSize(void)
{
    if (rows < 2)
        rows = 2;
    else if (rows > MAX_SCR_ROWS)
        rows = MAX_SCR_ROWS;
    if (columns < 2)
        columns = 2;
    else if (columns > MAX_SCR_COLS)
        columns = MAX_SCR_COLS;
}

#if ENABLE_FEATURE_VI_WIN_RESIZE
static const char *snchr(const char *s, int c, size_t n)
{
    while (n--)
        if (*s++ == c)
            return --s;
    return NULL;
}

// read response from STDIN into buf until timeout or endByte received
static ssize_t readResponse(char *buf, size_t bufSize, int endByte)
{
    size_t cursor = 0;
    while (cursor < bufSize) {
        if (!awaitInput(ticsPerChar + 9))
            return -ETIMEDOUT;
        int r = safe_read(STDIN_FILENO, buf + cursor, bufSize - cursor);
        if (r <= 0)
            return r < 0 ? r : -EIO;
        if (snchr(buf + cursor, endByte, r))
            return cursor + r;
        cursor += r;
    }
    return -E2BIG;
}

static void queueAnyInput(void)
// add any pending user input to readbuffer
{
    if (awaitInput(0)) {
        char *s = readbuffer + chars_to_parse;
        int r = safe_read(STDIN_FILENO, s, readbuffer + sizeof(readbuffer) - s);
        if (r > 0)
            chars_to_parse += r;
    }
}

void getScreenSize(void)
// assigns width and height to best guess as to actual screen size
// LINES and COLUMNS env vars take priority
// If either missing, query the terminal using VT100 escape codes
// 		If that fails, fall back to rows/cols info in the termios struct
// rows and columns retain their previous values if all methods fail
{
    struct winsize win = { 0, 0, 0, 0 };
    const char *lines = getenv("LINES");
    const char *cols = getenv("COLUMNS");
    if (!lines || !cols) { // if either missing in the environment
        queueAnyInput();
        if (!awaitInput(0)) { // can't query term if there's pending user input
            write1(CtextAreaQuery);
            char buf[16];
            int rspLen = readResponse(buf, sizeof(buf) - 1, 'R');
            if (rspLen > 5 && buf[0] == 27 && buf[1] == '[') {
                buf[rspLen] = 0; // terminate response string
                char *term;
                unsigned long ul = strtoul(buf + 2, &term, 10);
                if (*term == ';') {
                    win.ws_row = ul;
                    ul = strtoul(term + 1, &term, 10);
                    if (*term == 'R') {
                        win.ws_col = ul;
                    }
                }
            }
        }
        if (!win.ws_row || !win.ws_col) // try termios if textAreaQuery failed
            ioctl(STDIN_FILENO, TIOCGWINSZ, &win);
    }
    // environment variables trump all
    if (lines)
        rows = atoi(lines);
    else if (win.ws_row)
        rows = win.ws_row;
    if (cols)
        columns = atoi(cols);
    else if (win.ws_col)
        columns = win.ws_col;
}
#endif

static void createScreen(void)
{
#if ENABLE_FEATURE_VI_WIN_RESIZE
    getScreenSize();
    clampScreenSize();
#endif
    new_screen(rows, columns); // get memory for virtual screen
}

int vi_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int vi_main(int argc, char **argv)
{
    int c;

    INIT_G();
    rows = 24;
    columns = 80;
#if !ENABLE_FEATURE_VI_WIN_RESIZE
    { // try to get terminal dimensions from environment
        char *txt = getenv("LINES");
        if (txt)
            rows = atoi(txt);
        txt = getenv("COLUMNS");
        if (txt)
            columns = atoi(txt);
        clampScreenSize();
    }
#endif

#if ENABLE_FEATURE_VI_USE_SIGNALS || ENABLE_FEATURE_VI_CRASHME
    my_pid = getpid();
#endif
#if ENABLE_FEATURE_VI_CRASHME
    srand((long)my_pid);
#endif
#ifdef NO_SUCH_APPLET_YET
    /* If we aren't "vi", we are "view" */
    if (ENABLE_FEATURE_VI_READONLY && applet_name[2]) {
        SET_READONLY_MODE(readonly_mode);
    }
#endif

    vi_setops = VI_AUTOINDENT | VI_SHOWMATCH | VI_IGNORECASE;
#if ENABLE_FEATURE_VI_DOT_CMD || ENABLE_FEATURE_VI_YANKMARK
    modifying_cmds = "aAcCdDiIJoOpPrRsxX<>~"; // cmds modifying text[]
#endif

    //  1-  process $HOME/.exrc file (not inplemented yet)
    //  2-  process EXINIT variable from environment
    //  3-  process command line args
#if ENABLE_FEATURE_VI_COLON
    {
        char *p = getenv("EXINIT");
        if (p && *p)
            initial_cmds[0] = xstrndup(p, MAX_INPUT_LEN);
    }
#endif
    while ((c = getopt(argc, argv, "hCRH-" USE_FEATURE_VI_COLON("c:"))) != -1) {
        switch (c) {
#if ENABLE_FEATURE_VI_CRASHME
        case 'C':
            crashme = 1;
            break;
#endif
#if ENABLE_FEATURE_VI_READONLY
        case 'R': // Read-only flag
            SET_READONLY_MODE(readonly_mode);
            break;
#endif
#if ENABLE_FEATURE_VI_COLON
        case 'c': // cmd line vi command
            if (*optarg)
                initial_cmds[initial_cmds[0] != 0] = xstrndup(optarg, MAX_INPUT_LEN);
            break;
#endif
        case 'H':
        case '-':
            show_help();
            /* fall through */
        default:
            bb_show_usage();
            return 1;
        }
    }

    // The argv array can be used by the ":next"  and ":rewind" commands
    // save optind.
    fn_start = optind; // remember first file name for :next and :rew
    save_argc = argc;

    //----- This is the main file handling loop --------------
    if (optind >= argc) {
        edit_file(0);
    } else {
        for (; optind < argc; optind++) {
            edit_file(argv[optind]);
        }
    }
    //-----------------------------------------------------------

    return 0;
}

/* read text from file or create an empty buf */
/* will also update current_filename */
static int init_text_buffer(char *fn)
{
    int rc;
    int size = file_size(fn); // file size. -1 means does not exist.

    /* allocate/reallocate text buffer */
    free(text);
    text_size = size + 10240;
    screenbegin = dot = end = text = xzalloc(text_size);

    if (fn != current_filename) {
        free(current_filename);
        current_filename = xstrdup(fn);
    }
    if (size < 0) {
        // file dont exist. Start empty buf with dummy line
        char_insert(text, '\n');
        rc = 0;
    } else {
        rc = file_insert(fn, text USE_FEATURE_VI_READONLY(, 1));
    }
    file_modified = 0;
    last_file_modified = -1;
#if ENABLE_FEATURE_VI_YANKMARK
    /* init the marks. */
    memset(mark, 0, sizeof(mark));
#endif
    return rc;
}

static void edit_file(char *fn)
{
#if ENABLE_FEATURE_VI_YANKMARK
#define cur_line edit_file__cur_line
#endif
    char c;
    editing = 1; // 0 = exit, 1 = one file, 2 = multiple files
    if (rawmode()) {
        perror("vi");
        exit(5);
    }
    createScreen();
    init_text_buffer(fn);

#if ENABLE_FEATURE_VI_YANKMARK
    YDreg = 26;                 // default Yank/Delete reg
    Ureg = 27;                  // hold orig line for "U" cmd
    mark[26] = mark[27] = text; // init "previous context"
#endif

    last_forward_char = last_input_char = '\0';
    crow = 0;
    ccol = 0;
    tabstop = 8;
    offset = 0; // no horizontal offset
    clear_screen();

#if ENABLE_FEATURE_VI_USE_SIGNALS
    catch_sig(0);
    sigsetjmp(restart, 1);
    signal(SIGWINCH, winch_sig);
    signal(SIGTSTP, suspend_sig);
    signal(SIGQUIT, quit_sig);
    signal(SIGTERM, quit_sig);
    signal(SIGPIPE, quit_sig);
    signal(SIGHUP, quit_sig);
    signal(SIGILL, quit_sig);
    signal(SIGSEGV, quit_sig);
    signal(SIGBUS, quit_sig);
    signal(SIGABRT, quit_sig);
#endif

    cmd_mode = CMODE_COMMAND;
    cmdcnt = 0;
    c = '\0';
#if ENABLE_FEATURE_VI_DOT_CMD
    free(ioq_start);
    ioq = ioq_start = NULL;
    lmc_len = 0;
    adding2q = 0;
#endif

#if ENABLE_FEATURE_VI_COLON
    {
        char *p, *q;
        int n = 0;

        while ((p = initial_cmds[n])) {
            do {
                q = p;
                p = strchr(q, '\n');
                if (p)
                    while (*p == '\n')
                        *p++ = '\0';
                if (*q)
                    colon(q);
            } while (p);
            free(initial_cmds[n]);
            initial_cmds[n] = NULL;
            n++;
        }
    }
#endif

    //------This is the main Vi cmd handling loop -----------------------
    while (editing > 0) {
        refresh();
#if ENABLE_FEATURE_VI_CRASHME
        if (crashme > 0) {
            if ((end - text) > 1) {
                crash_dummy(); // generate a random command
            } else {
                crashme = 0;
                dot = string_insert(text, // insert the string
                                    "\n\n#####  Ran out of text to work on.  #####\n\n");
            }
        }
#endif
        last_input_char = c = get_one_char(); // get a cmd from user
        *status_buffer = 0;
#if ENABLE_FEATURE_VI_YANKMARK
        // save a copy of the current line- for the 'U" command
        if (begin_line(dot) != cur_line) {
            cur_line = begin_line(dot);
            text_yank(begin_line(dot), end_line(dot), Ureg);
        }
#endif
#if ENABLE_FEATURE_VI_DOT_CMD
        // These are commands that change text[].
        // Remember the input for the "." command
        if (!adding2q && ioq_start == NULL && strchr(modifying_cmds, c)) {
            start_new_cmd_q(c);
        }
#endif
        do_cmd(c); // execute the user command
#if ENABLE_FEATURE_VI_CRASHME
        if (crashme > 0)
            crash_test(); // test editor variables
#endif
    }
    //-------------------------------------------------------------------
    refresh();
    gracefulExit();
#undef cur_line
}

//----- The Colon commands -------------------------------------
#if ENABLE_FEATURE_VI_COLON
static char *get_one_address(char *p, int *addr) // get colon addr, if present
{
    int st;
    char *q;
    USE_FEATURE_VI_YANKMARK(char c;)
    USE_FEATURE_VI_SEARCH(char *pat;)

    *addr = -1;      // assume no addr
    if (*p == '.') { // the current line
        p++;
        q = begin_line(dot);
        *addr = count_lines(text, q);
    }
#if ENABLE_FEATURE_VI_YANKMARK
    else if (*p == '\'') { // is this a mark addr
        p++;
        c = tolower(*p);
        p++;
        if (c >= 'a' && c <= 'z') {
            // we have a mark
            c = c - 'a';
            q = mark[(unsigned char)c];
            if (q != NULL) {                  // is mark valid
                *addr = count_lines(text, q); // count lines
            }
        }
    }
#endif
#if ENABLE_FEATURE_VI_SEARCH
    else if (*p == '/') { // a search pattern
        q = strchrnul(++p, '/');
        pat = xstrndup(p, q - p); // save copy of pattern
        p = q;
        if (*p == '/')
            p++;
        q = char_search(dot, pat, FORWARD, FULL);
        if (q != NULL) {
            *addr = count_lines(text, q);
        }
        free(pat);
    }
#endif
    else if (*p == '$') { // the last line in file
        p++;
        q = begin_line(end - 1);
        *addr = count_lines(text, q);
    } else if (isdigit(*p)) { // specific line number
        sscanf(p, "%d%n", addr, &st);
        p += st;
    } else {
        // unrecognised address - assume -1
        *addr = -1;
    }
    return p;
}

static char *get_address(char *p, int *b, int *e) // get two colon addrs, if present
{
    //----- get the address' i.e., 1,3   'a,'b  -----
    // get FIRST addr, if present
    while (isblank(*p))
        p++;         // skip over leading spaces
    if (*p == '%') { // alias for 1,$
        p++;
        *b = 1;
        *e = count_lines(text, end - 1);
        goto ga0;
    }
    p = get_one_address(p, b);
    while (isblank(*p))
        p++;
    if (*p == ',') { // is there a address separator
        p++;
        while (isblank(*p))
            p++;
        // get SECOND addr, if present
        p = get_one_address(p, e);
    }
ga0:
    while (isblank(*p))
        p++; // skip over trailing spaces
    return p;
}

#if ENABLE_FEATURE_VI_SET && ENABLE_FEATURE_VI_SETOPTS
static void setops(const char *args, const char *opname, int flg_no, const char *short_opname,
                   int opt)
{
    const char *a = args + flg_no;
    int l = strlen(opname) - 1; /* opname have + ' ' */

    if (strncasecmp(a, opname, l) == 0 || strncasecmp(a, short_opname, 2) == 0) {
        if (flg_no)
            vi_setops &= ~opt;
        else
            vi_setops |= opt;
    }
}
#endif

#if ENABLE_FEATURE_VI_SETOPTS
// show the matching char of a pair,  ()  []  {}
static void showmatching(char *p)
{
    char *q, *save_dot;

    // we found half of a pair
    q = find_pair(p, *p); // get loc of matching char
    if (q == NULL) {
        indicate_error('3'); // no matching char
    } else {
        // "q" now points to matching pair
        save_dot = dot; // remember where we are
        dot = q;        // go to new loc
        refresh();      // let the user see it
        awaitInput(40); // give user some time
        dot = save_dot; // go back to old loc
        refresh();
    }
}

static char *stpcopy(char *dest, const char *src)
{
    while (*src)
        *dest++ = *src++;
    *dest = 0;
    return dest;
}
#endif /* FEATURE_VI_SETOPTS */

// buf must be no longer than MAX_INPUT_LEN!
static void colon(char *buf)
{
    char c, *orig_buf, *buf1, *q, *r;
    char *fn, cmd[MAX_INPUT_LEN], args[MAX_INPUT_LEN];
    int i, l, li, ch, b, e;
    int useforce, forced = FALSE;

    // :3154	// if (-e line 3154) goto it  else stay put
    // :4,33w! foo	// write a portion of buffer to file "foo"
    // :w		// write all of buffer to current file
    // :q		// quit
    // :q!		// quit- dont care about modified file
    // :'a,'z!sort -u   // filter block through sort
    // :'f		// goto mark "f"
    // :'fl		// list literal the mark "f" line
    // :.r bar	// read file "bar" into buffer before dot
    // :/123/,/abc/d    // delete lines from "123" line to "abc" line
    // :/xyz/	// goto the "xyz" line
    // :s/find/replace/ // substitute pattern "find" with "replace"
    // :!<cmd>	// run <cmd> then return
    //

    if (!buf[0])
        goto vc1;
    if (*buf == ':')
        buf++; // move past the ':'

    li = ch = i = 0;
    b = e = -1;
    q = text; // assume 1,$ for the range
    r = end - 1;
    li = count_lines(text, end - 1);
    fn = current_filename;

    // look for optional address(es)  :.  :1  :1,9   :'q,'a   :%
    buf = get_address(buf, &b, &e);

    // remember orig command line
    orig_buf = buf;

    // get the COMMAND into cmd[]
    buf1 = cmd;
    while (*buf != '\0') {
        if (isspace(*buf))
            break;
        *buf1++ = *buf++;
    }
    *buf1 = '\0';
    // get any ARGuments
    while (isblank(*buf))
        buf++;
    strcpy(args, buf);
    useforce = FALSE;
    buf1 = last_char_is(cmd, '!');
    if (buf1) {
        useforce = TRUE;
        *buf1 = '\0'; // get rid of !
    }
    if (b >= 0) {
        // if there is only one addr, then the addr
        // is the line number of the single line the
        // user wants. So, reset the end
        // pointer to point at end of the "b" line
        q = find_line(b); // what line is #b
        r = end_line(q);
        li = 1;
    }
    if (e >= 0) {
        // we were given two addrs.  change the
        // end pointer to the addr given by user.
        r = find_line(e); // what line is #e
        r = end_line(r);
        li = e - b + 1;
    }
    // ------------ now look for the command ------------
    i = strlen(cmd);
    if (i == 0) { // :123CR goto line #123
        if (b >= 0) {
            dot = find_line(b); // what line is #b
            dot_skip_over_ws();
        }
    }
#if ENABLE_FEATURE_ALLOW_EXEC
    else if (strncmp(cmd, "!", 1) == 0) { // run a cmd
        int retcode;
        // :!ls   run the <cmd>
        clear_screen();
        standout_start();
        write1(status_buffer + 2);
        standout_end();
        cookmode();
        write1("\n");
        retcode = system(orig_buf + 1) >> 8; // run the cmd
        if (retcode)
            printf("\nshell returned %i\n\n", retcode);
        rawmode();
        Hit_Return(); // let user see results
    }
#endif
    else if (strncmp(cmd, "=", i) == 0) { // where is the address
        if (b < 0) {                      // no addr given- use defaults
            b = e = count_lines(text, dot);
        }
        status_line("%d", b);
    } else if (strncasecmp(cmd, "delete", i) == 0) { // delete lines
        if (b < 0) {                                 // no addr given- use defaults
            q = begin_line(dot);                     // assume .,. for the range
            r = end_line(dot);
        }
        dot = yank_delete(q, r, 1, YANKDEL); // save, then delete lines
        dot_skip_over_ws();
    } else if (strncasecmp(cmd, "edit", i) == 0) { // Edit a file
        // don't edit, if the current file has been modified
        if (file_modified && !useforce) {
            status_line_bold("No write since last change (:edit! overrides)");
            goto vc1;
        }
        if (args[0]) {
            // the user supplied a file name
            fn = args;
        } else if (current_filename && current_filename[0]) {
            // no user supplied name- use the current filename
            // fn = current_filename;  was set by default
        } else {
            // no user file name, no current name- punt
            status_line_bold("No current filename");
            goto vc1;
        }

        if (init_text_buffer(fn) < 0)
            goto vc1;

#if ENABLE_FEATURE_VI_YANKMARK
        if (Ureg >= 0 && Ureg < 28 && reg[Ureg] != 0) {
            free(reg[Ureg]); //   free orig line reg- for 'U'
            reg[Ureg] = 0;
        }
        if (YDreg >= 0 && YDreg < 28 && reg[YDreg] != 0) {
            free(reg[YDreg]); //   free default yank/delete register
            reg[YDreg] = 0;
        }
#endif
        // how many lines in text[]?
        li = count_lines(text, end - 1);
        status_line("\"%s\"%s" USE_FEATURE_VI_READONLY("%s") " %dL, %dC", current_filename,
                    (file_size(fn) < 0 ? " [New file]" : ""),
                    USE_FEATURE_VI_READONLY(((readonly_mode) ? " [Readonly]" : ""), ) li, ch);
    } else if (strncasecmp(cmd, "file", i) == 0) { // what File is this
        if (b != -1 || e != -1) {
            not_implemented("No address allowed on this command");
            goto vc1;
        }
        if (args[0]) {
            // user wants a new filename
            free(current_filename);
            current_filename = xstrdup(args);
        }
    } else if (strncasecmp(cmd, "features", i) == 0) { // what features are available
        // print out values of all features
        place_cursor(rows - 1, 0, FALSE); // go to Status line, bottom of screen
        clear_to_eol();                   // clear the line
        cookmode();
        show_help();
        rawmode();
        Hit_Return();
    } else if (strncasecmp(cmd, "list", i) == 0) { // literal print line
        if (b < 0) {                               // no addr given- use defaults
            q = begin_line(dot);                   // assume .,. for the range
            r = end_line(dot);
        }
        place_cursor(rows - 1, 0, FALSE); // go to Status line, bottom of screen
        clear_to_eol();                   // clear the line
        puts("\r");
        for (; q <= r; q++) {
            int c_is_no_print;

            c = *q;
            c_is_no_print = (c & 0x80) && !Isprint(c);
            if (c_is_no_print) {
                c = '.';
                standout_start();
            }
            if (c == '\n') {
                write1("$\r");
            } else if (c < ' ' || c == 127) {
                bb_putchar('^');
                if (c == 127)
                    c = '?';
                else
                    c += '@';
            }
            bb_putchar(c);
            if (c_is_no_print)
                standout_end();
        }
        Hit_Return();
    } else if (strncasecmp(cmd, "quit", i) == 0    // Quit
               || strncasecmp(cmd, "next", i) == 0 // edit next file
    ) {
        if (useforce) {
            // force end of argv list
            if (*cmd == 'q') {
                optind = save_argc;
            }
            editing = 0;
            goto vc1;
        }
        // don't exit if the file been modified
        if (file_modified) {
            status_line_bold("No write since last change (:%s! overrides)",
                             (*cmd == 'q' ? "quit" : "next"));
            goto vc1;
        }
        // are there other file to edit
        if (*cmd == 'q' && optind < save_argc - 1) {
            status_line_bold("%d more file to edit", (save_argc - optind - 1));
            goto vc1;
        }
        if (*cmd == 'n' && optind >= save_argc - 1) {
            status_line_bold("No more files to edit");
            goto vc1;
        }
        editing = 0;
    } else if (strncasecmp(cmd, "read", i) == 0) { // read file into text[]
        fn = args;
        if (!fn[0]) {
            status_line_bold("No filename given");
            goto vc1;
        }
        if (b < 0) {             // no addr given- use defaults
            q = begin_line(dot); // assume "dot"
        }
        // read after current line- unless user said ":0r foo"
        if (b != 0)
            q = next_line(q);
        ch = file_insert(fn, q USE_FEATURE_VI_READONLY(, 0));
        if (ch < 0)
            goto vc1; // nothing was inserted
        // how many lines in text[]?
        li = count_lines(q, q + ch - 1);
        status_line("\"%s\"" USE_FEATURE_VI_READONLY("%s") " %dL, %dC", fn,
                    USE_FEATURE_VI_READONLY((readonly_mode ? " [Readonly]" : ""), ) li, ch);
        if (ch > 0) {
            // if the insert is before "dot" then we need to update
            if (q <= dot)
                dot += ch;
            file_modified++;
        }
    } else if (strncasecmp(cmd, "rewind", i) == 0) { // rewind cmd line args
        if (file_modified && !useforce) {
            status_line_bold("No write since last change (:rewind! overrides)");
        } else {
            // reset the filenames to edit
            optind = fn_start - 1;
            editing = 0;
        }
#if ENABLE_FEATURE_VI_SET
    } else if (strncasecmp(cmd, "set", i) == 0) { // set or clear features
#if ENABLE_FEATURE_VI_SETOPTS
        char *argp = args;
        while (*argp) {
            i = 0; // offset into args
            if (strncasecmp(argp, "no", 2) == 0)
                i = 2; // ":set noautoindent"
            setops(argp, "autoindent ", i, "ai", VI_AUTOINDENT);
            setops(argp, "flash ", i, "fl", VI_ERR_METHOD);
            setops(argp, "ignorecase ", i, "ic", VI_IGNORECASE);
            setops(argp, "showmatch ", i, "sm", VI_SHOWMATCH);
            /* tabstopXXXX */
            if (strncasecmp(argp + i, "tabstop=%d ", 7) == 0) {
                sscanf(strchr(argp + i, '='), &"tabstop=%d"[7], &ch);
                if (ch > 0 && ch <= MAX_TABSTOP)
                    tabstop = ch;
            }
            while (*argp && *argp != ' ')
                argp++; // skip to arg delimiter (i.e. blank)
            while (*argp && *argp == ' ')
                argp++; // skip all delimiting blanks
        }
        // display values of all options in status line
        char *cursor = status_buffer;
        *cursor = 0;
        if (!autoindent)
            cursor = stpcopy(cursor, "no");
        cursor = stpcopy(cursor, "autoindent ");
        if (!err_method)
            cursor = stpcopy(cursor, "no");
        cursor = stpcopy(cursor, "flash ");
        if (!ignorecase)
            cursor = stpcopy(cursor, "no");
        cursor = stpcopy(cursor, "ignorecase ");
        if (!showmatch)
            cursor = stpcopy(cursor, "no");
        cursor = stpcopy(cursor, "showmatch ");
        cursor += printf(cursor, "tabstop=%d ", tabstop);
#endif /* FEATURE_VI_SETOPTS */
#endif /* FEATURE_VI_SET */
#if ENABLE_FEATURE_VI_SEARCH
    } else if (strncasecmp(cmd, "s", 1) == 0) { // substitute a pattern with a replacement pattern
        char *ls, *F, *R;
        int gflag;

        // F points to the "find" pattern
        // R points to the "replace" pattern
        // replace the cmd line delimiters "/" with NULLs
        gflag = 0;        // global replace flag
        c = orig_buf[1];  // what is the delimiter
        F = orig_buf + 2; // start of "find"
        R = strchr(F, c); // middle delimiter
        if (!R)
            goto colon_s_fail;
        if (R == F) { // use previous search pattern if no find pattern given
            if (!(F = last_search_pattern))
                goto colon_no_regex;
            F++; // ignore search direction
        }
        *R++ = '\0'; // terminate "find"
        buf1 = strchr(R, c);
        if (buf1) {             // accept :s/foo/bar
            *buf1++ = '\0';     // terminate "replace"
            if (*buf1 == 'g') { // :s/foo/bar/g
                buf1++;
                gflag++; // turn on gflag
            }
        }
        q = begin_line(q);
        if (b < 0) {                  // maybe :s/foo/bar/
            q = begin_line(dot);      // start with cur line
            b = count_lines(text, q); // cur line number
        }
        if (e < 0)
            e = b;                 // maybe :.s/foo/bar/
        for (i = b; i <= e; i++) { // so, :20,23 s \0 find \0 replace \0
            ls = q;                // orig line start
        vc4:
            buf1 = char_search(q, F, FORWARD, LIMITED); // search cur line only for "find"
            if (buf1) {
                // we found the "find" pattern - delete it
                text_hole_delete(buf1, buf1 + strlen(F) - 1);
                // inset the "replace" patern
                string_insert(buf1, R); // insert the string
                // check for "global"  :s/foo/bar/g
                if (gflag == 1) {
                    if ((buf1 + strlen(R)) < end_line(ls)) {
                        q = buf1 + strlen(R);
                        goto vc4; // don't let q move past cur line
                    }
                }
            }
            q = next_line(ls);
        }
#endif                                                /* FEATURE_VI_SEARCH */
    } else if (strncasecmp(cmd, "version", i) == 0) { // show software version
        status_line(BB_VER " " BB_BT);
    } else if (strncasecmp(cmd, "write", i) == 0 // write text to file
               || strncasecmp(cmd, "wq", i) == 0 || strncasecmp(cmd, "wn", i) == 0 ||
               strncasecmp(cmd, "x", i) == 0) {
        // is there a file name to write to?
        if (args[0]) {
            fn = args;
        }
#if ENABLE_FEATURE_VI_READONLY
        if (readonly_mode && !useforce) {
            status_line_bold("\"%s\" File is read only", fn);
            goto vc3;
        }
#endif
        // how many lines in text[]?
        li = count_lines(q, r);
        ch = r - q + 1;
        // see if file exists- if not, its just a new file request
        if (useforce) {
            // if "fn" is not write-able, chmod u+w
            // sprintf(syscmd, "chmod u+w %s", fn);
            // system(syscmd);
            forced = TRUE;
        }
        l = file_write(fn, q, r);
        if (useforce && forced) {
            // chmod u-w
            // sprintf(syscmd, "chmod u-w %s", fn);
            // system(syscmd);
            forced = FALSE;
        }
        if (l < 0) {
            if (l == -1)
                status_line_bold("\"%s\" %s", fn, strerror(errno));
        } else {
            status_line("\"%s\" %dL, %dC", fn, li, l);
            if (q == text && r == end - 1 && l == ch) {
                file_modified = 0;
                last_file_modified = -1;
            }
            if ((cmd[0] == 'x' || cmd[1] == 'q' || cmd[1] == 'n' || cmd[0] == 'X' ||
                 cmd[1] == 'Q' || cmd[1] == 'N') &&
                l == ch) {
                editing = 0;
            }
        }
#if ENABLE_FEATURE_VI_READONLY
    vc3:;
#endif
#if ENABLE_FEATURE_VI_YANKMARK
    } else if (strncasecmp(cmd, "yank", i) == 0) { // yank lines
        if (b < 0) {                               // no addr given- use defaults
            q = begin_line(dot);                   // assume .,. for the range
            r = end_line(dot);
        }
        text_yank(q, r, YDreg);
        li = count_lines(q, r);
        status_line("Yank %d lines (%d chars) into [%c]", li, strlen(reg[YDreg]), what_reg());
#endif
    } else {
        // cmd unknown
        not_implemented(cmd);
    }
vc1:
    dot = bound_dot(dot); // make sure "dot" is valid
    return;
#if ENABLE_FEATURE_VI_SEARCH
colon_s_fail:
    status_line_bold(":s expression missing delimiters");
    return;
colon_no_regex:
    status_line_bold("No previous regular expression");
#endif
}

#endif /* FEATURE_VI_COLON */

static void Hit_Return(void)
{
    char c;

    standout_start();
    write1("[Hit return to continue]");
    standout_end();
    while ((c = get_one_char()) != '\n' && c != '\r' && c != 27)
        continue;
    redraw();
}

static int next_tabstop(int col)
{
    return col + ((tabstop - 1) - (col % tabstop));
}

//----- Synchronize the cursor to Dot --------------------------
static void sync_cursor(char *d, int *row, int *col)
{
    char *beg_cur; // begin and end of "d" line
    char *tp;
    int cnt, ro, co;

    beg_cur = begin_line(d); // first char of cur line

    if (beg_cur < screenbegin) {
        // "d" is before top line on screen
        // how many lines do we have to move
        cnt = count_lines(beg_cur, screenbegin);
    sc1:
        screenbegin = beg_cur;
        if (cnt > (rows - 1) / 2) {
            // we moved too many lines. put "dot" in middle of screen
            for (cnt = 0; cnt < (rows - 1) / 2; cnt++) {
                screenbegin = prev_line(screenbegin);
            }
        }
    } else {
        char *end_scr;          // begin and end of screen
        end_scr = end_screen(); // last char of screen
        if (beg_cur > end_scr) {
            // "d" is after bottom line on screen
            // how many lines do we have to move
            cnt = count_lines(end_scr, beg_cur);
            if (cnt > (rows - 1) / 2)
                goto sc1; // too many lines
            for (ro = 0; ro < cnt - 1; ro++) {
                // move screen begin the same amount
                screenbegin = next_line(screenbegin);
                // now, move the end of screen
                end_scr = next_line(end_scr);
                end_scr = end_line(end_scr);
            }
        }
    }
    // "d" is on screen- find out which row
    tp = screenbegin;
    for (ro = 0; ro < rows - 1; ro++) { // drive "ro" to correct row
        if (tp == beg_cur)
            break;
        tp = next_line(tp);
    }

    // find out what col "d" is on
    co = 0;
    while (tp < d) {     // drive "co" to correct column
        if (*tp == '\n') // vda || *tp == '\0')
            break;
        if (*tp == '\t') {
            // handle tabs like real vi
            if (d == tp && (cmd_mode & CMODES) != CMODE_COMMAND) {
                break;
            }
            co = next_tabstop(co);
        } else if ((unsigned char)*tp < ' ' || *tp == 0x7f) {
            co++; // display as ^X, use 2 columns
        }
        co++;
        tp++;
    }

    // "co" is the column where "dot" is.
    // The screen has "columns" columns.
    // The currently displayed columns are  0+offset -- columns+ofset
    // |-------------------------------------------------------------|
    //               ^ ^                                ^
    //        offset | |------- columns ----------------|
    //
    // If "co" is already in this range then we do not have to adjust offset
    //      but, we do have to subtract the "offset" bias from "co".
    // If "co" is outside this range then we have to change "offset".
    // If the first char of a line is a tab the cursor will try to stay
    //  in column 7, but we have to set offset to 0.

    if (co < 0 + offset) {
        offset = co;
    }
    if (co >= columns + offset) {
        offset = co - columns + 1;
    }
    // if the first char of the line is a tab, and "dot" is sitting on it
    //  force offset to 0.
    if (d == beg_cur && *d == '\t') {
        offset = 0;
    }
    co -= offset;

    *row = ro;
    *col = co;
}

//----- Text Movement Routines ---------------------------------
static char *begin_line(char *p) // return pointer to first char cur line
{
    if (p > text) {
        p = memrchr(text, '\n', p - text);
        if (!p)
            return text;
        return p + 1;
    }
    return p;
}

static char *end_line(char *p) // return pointer to NL of cur line
{
    if (p < end - 1) {
        p = memchr(p, '\n', end - p - 1);
        if (!p)
            return end - 1;
    }
    return p;
}

static char *dollar_line(char *p) // return pointer to just before NL line
{
    p = end_line(p);
    // Try to stay off of the Newline
    if (*p == '\n' && (p - begin_line(p)) > 0)
        p--;
    return p;
}

static char *prev_line(char *p) // return pointer first char prev line
{
    p = begin_line(p); // goto begining of cur line
    if (p > text && p[-1] == '\n')
        p--;           // step to prev line
    p = begin_line(p); // goto begining of prev line
    return p;
}

static char *next_line(char *p) // return pointer first char next line
{
    p = end_line(p);
    if (p < end - 1 && *p == '\n')
        p++; // step to next line
    return p;
}

//----- Text Information Routines ------------------------------
static char *end_screen(void)
{
    char *q;
    int cnt;

    // find new bottom line
    q = screenbegin;
    for (cnt = 0; cnt < rows - 2; cnt++)
        q = next_line(q);
    q = end_line(q);
    return q;
}

// count line from start to stop
static int count_lines(char *start, char *stop)
{
    char *q;
    int cnt;

    if (stop < start) { // start and stop are backwards- reverse them
        q = start;
        start = stop;
        stop = q;
    }
    cnt = 0;
    stop = end_line(stop);
    while (start <= stop && start <= end - 1) {
        start = end_line(start);
        if (*start == '\n')
            cnt++;
        start++;
    }
    return cnt;
}

static char *find_line(int li) // find begining of line #li
{
    char *q;

    for (q = text; li > 1; li--) {
        q = next_line(q);
    }
    return q;
}

//----- Dot Movement Routines ----------------------------------
static void dot_left(void)
{
    if (dot > text && dot[-1] != '\n')
        dot--;
}

static void dot_right(void)
{
    if (dot < end - 1 && *dot != '\n')
        dot++;
}

static void dot_begin(void)
{
    dot = begin_line(dot); // return pointer to first char cur line
}

static void dot_end(void)
{
    dot = end_line(dot); // return pointer to last char cur line
}

static char *move_to_col(char *p, int l)
{
    int co;

    p = begin_line(p);
    co = 0;
    while (co < l && p < end) {
        if (*p == '\n') // vda || *p == '\0')
            break;
        if (*p == '\t') {
            co = next_tabstop(co);
        } else if (*p < ' ' || *p == 127) {
            co++; // display as ^X, use 2 columns
        }
        co++;
        p++;
    }
    return p;
}

static void dot_next(void)
{
    dot = next_line(dot);
}

static void dot_prev(void)
{
    dot = prev_line(dot);
}

static void dot_scroll(int cnt, int dir)
{
    char *q;

    for (; cnt > 0; cnt--) {
        if (dir < 0) {
            // scroll Backwards
            // ctrl-Y scroll up one line
            screenbegin = prev_line(screenbegin);
        } else {
            // scroll Forwards
            // ctrl-E scroll down one line
            screenbegin = next_line(screenbegin);
        }
    }
    // make sure "dot" stays on the screen so we dont scroll off
    if (dot < screenbegin)
        dot = screenbegin;
    q = end_screen(); // find new bottom line
    if (dot > q)
        dot = begin_line(q); // is dot is below bottom line?
    dot_skip_over_ws();
}

static void dot_skip_over_ws(void)
{
    // skip WS
    while (isspace(*dot) && *dot != '\n' && dot < end - 1)
        dot++;
}

static void dot_delete(void) // delete the char at 'dot'
{
    text_hole_delete(dot, dot);
}

static char *bound_dot(char *p) // make sure  text[0] <= P < "end"
{
    if (p >= end && end > text) {
        p = end - 1;
        indicate_error('1');
    }
    if (p < text) {
        p = text;
        indicate_error('2');
    }
    return p;
}

//----- Helper Utility Routines --------------------------------

//----------------------------------------------------------------
//----- Char Routines --------------------------------------------
/* Chars that are part of a word-
 *    0123456789_ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz
 * Chars that are Not part of a word (stoppers)
 *    !"#$%&'()*+,-./:;<=>?@[\]^`{|}~
 * Chars that are WhiteSpace
 *    TAB NEWLINE VT FF RETURN SPACE
 * DO NOT COUNT NEWLINE AS WHITESPACE
 */

static char *new_screen(int ro, int co)
{
    free(screen);
    screensize = ro * co + 8;
    screen = xmalloc(screensize);
    screen_erase();
    return screen;
}

#if ENABLE_FEATURE_VI_SEARCH
static int mycmp(const char *s1, const char *s2, int len)
{
    int i;

    i = strncmp(s1, s2, len);
    if (ENABLE_FEATURE_VI_SETOPTS && ignorecase) {
        i = strncasecmp(s1, s2, len);
    }
    return i;
}

// search for pattern starting at p
static char *char_search(char *p, const char *pat, int dir, int range)
{
#ifndef REGEX_SEARCH
    char *start, *stop;
    int len;

    len = strlen(pat);
    if (dir == FORWARD) {
        stop = end - 1; // assume range is p - end-1
        if (range == LIMITED)
            stop = next_line(p); // range is to next line
        for (start = p; start < stop; start++) {
            if (mycmp(start, pat, len) == 0) {
                return start;
            }
        }
    } else if (dir == BACK) {
        stop = text; // assume range is text - p
        if (range == LIMITED)
            stop = prev_line(p); // range is to prev line
        for (start = p - len; start >= stop; start--) {
            if (mycmp(start, pat, len) == 0) {
                return start;
            }
        }
    }
    // pattern not found
    return NULL;
#else  /* REGEX_SEARCH */
    char *q;
    struct re_pattern_buffer preg;
    int i;
    int size, range;

    re_syntax_options = RE_SYNTAX_POSIX_EXTENDED;
    preg.translate = 0;
    preg.fastmap = 0;
    preg.buffer = 0;
    preg.allocated = 0;

    // assume a LIMITED forward search
    q = next_line(p);
    q = end_line(q);
    q = end - 1;
    if (dir == BACK) {
        q = prev_line(p);
        q = text;
    }
    // count the number of chars to search over, forward or backward
    size = q - p;
    if (size < 0)
        size = p - q;
    // RANGE could be negative if we are searching backwards
    range = q - p;

    q = re_compile_pattern(pat, strlen(pat), &preg);
    if (q != 0) {
        // The pattern was not compiled
        status_line_bold("bad search pattern: \"%s\": %s", pat, q);
        i = 0; // return p if pattern not compiled
        goto cs1;
    }

    q = p;
    if (range < 0) {
        q = p - size;
        if (q < text)
            q = text;
    }
    // search for the compiled pattern, preg, in p[]
    // range < 0-  search backward
    // range > 0-  search forward
    // 0 < start < size
    // re_search() < 0  not found or error
    // re_search() > 0  index of found pattern
    //            struct pattern    char     int    int    int     struct reg
    // re_search (*pattern_buffer,  *string, size,  start, range,  *regs)
    i = re_search(&preg, q, size, 0, range, 0);
    if (i == -1) {
        p = 0;
        i = 0; // return NULL if pattern not found
    }
cs1:
    if (dir == FORWARD) {
        p = p + i;
    } else {
        p = p - i;
    }
    return p;
#endif /* REGEX_SEARCH */
}
#endif /* FEATURE_VI_SEARCH */

static char *char_insert(char *p, char c) // insert the char c at 'p'
{
    if (c == 22) {                 // Is this an ctrl-V?
        p = stupid_insert(p, '^'); // use ^ to indicate literal next
        p--;                       // backup onto ^
        refresh();                 // show the ^
        c = get_one_char();
        *p = c;
        p++;
        file_modified++;
    } else if (c == 27) { // Is this an ESC?
        cmd_mode = CMODE_COMMAND;
        cmdcnt = 0;
        end_cmd_q(); // stop adding to q
        if ((p[-1] != '\n') && (dot > text)) {
            p--;
        }
    } else if (c == erase_char || c == 8 || c == 127) { // Is this a BS
        //     123456789
        if ((p[-1] != '\n') && (dot > text)) {
            p--;
            p = text_hole_delete(p, p); // shrink buffer 1 char
        }
    } else {
        // insert a char into text[]
        char *sp; // "save p"

        if (c == 13)
            c = '\n';            // translate \r to \n
        sp = p;                  // remember addr of insert
        p = stupid_insert(p, c); // insert the char
#if ENABLE_FEATURE_VI_SETOPTS
        if (showmatch && strchr(")]}", *sp) != NULL) {
            showmatching(sp);
        }
        if (autoindent && c == '\n') { // auto indent the new line
            char *q;

            q = prev_line(p); // use prev line as templet
            for (; isblank(*q); q++) {
                p = stupid_insert(p, *q); // insert the char
            }
        }
#endif
    }
    return p;
}

static char *stupid_insert(char *p, char c) // stupidly insert the char c at 'p'
{
    p = text_hole_make(p, 1);
    *p = c;
    // file_modified++; - done by text_hole_make()
    return p + 1;
}

static int find_range(char **start, char **stop, char c)
{
    char *save_dot, *p, *q, *t;
    int cnt, multiline = 0;

    save_dot = dot;
    p = q = dot;

    if (strchr("cdy><", c)) {
        // these cmds operate on whole lines
        p = q = begin_line(p);
        for (cnt = 1; cnt < cmdcnt; cnt++) {
            q = next_line(q);
        }
        q = end_line(q);
    } else if (strchr("^%$0bBeEfth\b\177", c)) {
        // These cmds operate on char positions
        do_cmd(c); // execute movement cmd
        q = dot;
    } else if (strchr("wW", c)) {
        do_cmd(c); // execute movement cmd
        // if we are at the next word's first char
        // step back one char
        // but check the possibilities when it is true
        if (dot > text &&
            ((isspace(dot[-1]) && !isspace(dot[0])) || (ispunct(dot[-1]) && !ispunct(dot[0])) ||
             (isalnum(dot[-1]) && !isalnum(dot[0]))))
            dot--; // move back off of next word
        if (dot > text && *dot == '\n')
            dot--; // stay off NL
        q = dot;
    } else if (strchr("H-k{", c)) {
        // these operate on multi-lines backwards
        q = end_line(dot); // find NL
        do_cmd(c);         // execute movement cmd
        dot_begin();
        p = dot;
    } else if (strchr("L+j}\r\n", c)) {
        // these operate on multi-lines forwards
        p = begin_line(dot);
        do_cmd(c); // execute movement cmd
        dot_end(); // find NL
        q = dot;
    } else {
        // nothing -- this causes any other values of c to
        // represent the one-character range under the
        // cursor.  this is correct for ' ' and 'l', but
        // perhaps no others.
        //
    }
    if (q < p) {
        t = q;
        q = p;
        p = t;
    }

    // backward char movements don't include start position
    if (q > p && strchr("^0bBh\b\177", c))
        q--;

    multiline = 0;
    for (t = p; t <= q; t++) {
        if (*t == '\n') {
            multiline = 1;
            break;
        }
    }

    *start = p;
    *stop = q;
    dot = save_dot;
    return multiline;
}

static int st_test(char *p, int type, int dir, char *tested)
{
    char c, c0, ci;
    int test, inc;

    inc = dir;
    c = c0 = p[0];
    ci = p[inc];
    test = 0;

    if (type == S_BEFORE_WS) {
        c = ci;
        test = ((!isspace(c)) || c == '\n');
    }
    if (type == S_TO_WS) {
        c = c0;
        test = ((!isspace(c)) || c == '\n');
    }
    if (type == S_OVER_WS) {
        c = c0;
        test = ((isspace(c)));
    }
    if (type == S_END_PUNCT) {
        c = ci;
        test = ((ispunct(c)));
    }
    if (type == S_END_ALNUM) {
        c = ci;
        test = ((isalnum(c)) || c == '_');
    }
    *tested = c;
    return test;
}

static char *skip_thing(char *p, int linecnt, int dir, int type)
{
    char c;

    while (st_test(p, type, dir, &c)) {
        // make sure we limit search to correct number of lines
        if (c == '\n' && --linecnt < 1)
            break;
        if (dir >= 0 && p >= end - 1)
            break;
        if (dir < 0 && p <= text)
            break;
        p += dir; // move to next char
    }
    return p;
}

// find matching char of pair  ()  []  {}
static char *find_pair(char *p, const char c)
{
    char match, *q;
    int dir, level;

    match = ')';
    level = 1;
    dir = 1; // assume forward
    switch (c) {
    case '(':
        match = ')';
        break;
    case '[':
        match = ']';
        break;
    case '{':
        match = '}';
        break;
    case ')':
        match = '(';
        dir = -1;
        break;
    case ']':
        match = '[';
        dir = -1;
        break;
    case '}':
        match = '{';
        dir = -1;
        break;
    }
    for (q = p + dir; text <= q && q < end; q += dir) {
        // look for match, count levels of pairs  (( ))
        if (*q == c)
            level++; // increase pair levels
        if (*q == match)
            level--; // reduce pair level
        if (level == 0)
            break; // found matching pair
    }
    if (level != 0)
        q = NULL; // indicate no match
    return q;
}

//  open a hole in text[]
static char *text_hole_make(char *p, int size) // at "p", make a 'size' byte hole
{
    if (size <= 0)
        return p;
    end += size; // adjust the new END
    if (end >= (text + text_size)) {
        char *new_text;
        text_size += end - (text + text_size) + 10240;
        new_text = xrealloc(text, text_size);
        screenbegin = new_text + (screenbegin - text);
        dot = new_text + (dot - text);
        end = new_text + (end - text);
        p = new_text + (p - text);
        text = new_text;
    }
    memmove(p + size, p, end - size - p);
    memset(p, ' ', size); // clear new hole
    file_modified++;
    return p;
}

//  close a hole in text[]
static char *text_hole_delete(char *p, char *q) // delete "p" through "q", inclusive
{
    char *src, *dest;
    int cnt, hole_size;

    // move forwards, from beginning
    // assume p <= q
    src = q + 1;
    dest = p;
    if (q < p) { // they are backward- swap them
        src = p + 1;
        dest = q;
    }
    hole_size = q - p + 1;
    cnt = end - src;
    if (src < text || src > end)
        goto thd0;
    if (dest < text || dest >= end)
        goto thd0;
    if (src >= end)
        goto thd_atend; // just delete the end of the buffer
    memmove(dest, src, cnt);
thd_atend:
    end = end - hole_size; // adjust the new END
    if (dest >= end)
        dest = end - 1; // make sure dest in below end-1
    if (end <= text)
        dest = end = text; // keep pointers valid
    file_modified++;
thd0:
    return dest;
}

// copy text into register, then delete text.
// if dist <= 0, do not include, or go past, a NewLine
//
static char *yank_delete(char *start, char *stop, int dist, int yf)
{
    char *p;

    // make sure start <= stop
    if (start > stop) {
        // they are backwards, reverse them
        p = start;
        start = stop;
        stop = p;
    }
    if (dist <= 0) {
        // we cannot cross NL boundaries
        p = start;
        if (*p == '\n')
            return p;
        // dont go past a NewLine
        for (; p + 1 <= stop; p++) {
            if (p[1] == '\n') {
                stop = p; // "stop" just before NewLine
                break;
            }
        }
    }
    p = start;
#if ENABLE_FEATURE_VI_YANKMARK
    text_yank(start, stop, YDreg);
#endif
    if (yf == YANKDEL) {
        p = text_hole_delete(start, stop);
    } // delete lines
    return p;
}

static void show_help(void)
{
    puts(
        "These features are available:"
#if ENABLE_FEATURE_VI_SEARCH
        "\n\tPattern searches with / and ?"
#endif
#if ENABLE_FEATURE_VI_DOT_CMD
        "\n\tLast command repeat with \'.\'"
#endif
#if ENABLE_FEATURE_VI_YANKMARK
        "\n\tLine marking with 'x"
        "\n\tNamed buffers with \"x"
#endif
#if ENABLE_FEATURE_VI_READONLY
#ifdef NO_SUCH_APPLET_YET
        "\n\tReadonly if vi is called as \"view\""
#endif
        "\n\tReadonly with -R command line arg"
#endif
#if ENABLE_FEATURE_VI_SET
        "\n\tSome colon mode commands with \':\'"
#endif
#if ENABLE_FEATURE_VI_SETOPTS
        "\n\tSettable options with \":set\""
#endif
#if ENABLE_FEATURE_VI_USE_SIGNALS
        "\n\tSignal catching- ^C"
        "\n\tJob suspend and resume with ^Z"
#endif
        "\n\tLINES and COLUMNS env vars determine window size"
#if ENABLE_FEATURE_VI_WIN_RESIZE
        "\n\tAdapt to window re-sizes (if LINES and COLUMNS env vars unset!)"
#endif
    );
}

#if ENABLE_FEATURE_VI_DOT_CMD
static void start_new_cmd_q(char c)
{
    // get buffer for new cmd
    // if there is a current cmd count put it in the buffer first
    if (cmdcnt > 0)
        lmc_len = sprintf(last_modifying_cmd, "%d%c", cmdcnt, c);
    else { // just save char c onto queue
        last_modifying_cmd[0] = c;
        lmc_len = 1;
    }
    adding2q = 1;
}

static void end_cmd_q(void)
{
#if ENABLE_FEATURE_VI_YANKMARK
    YDreg = 26; // go back to default Yank/Delete reg
#endif
    adding2q = 0;
}
#endif /* FEATURE_VI_DOT_CMD */

#if ENABLE_FEATURE_VI_YANKMARK || (ENABLE_FEATURE_VI_COLON && ENABLE_FEATURE_VI_SEARCH) || \
    ENABLE_FEATURE_VI_CRASHME
static char *string_insert(char *p, char *s) // insert the string at 'p'
{
    int cnt, i;

    i = strlen(s);
    text_hole_make(p, i);
    strncpy(p, s, i);
    for (cnt = 0; *s != '\0'; s++) {
        if (*s == '\n')
            cnt++;
    }
#if ENABLE_FEATURE_VI_YANKMARK
    status_line("Put %d lines (%d chars) from [%c]", cnt, i, what_reg());
#endif
    return p;
}
#endif

#if ENABLE_FEATURE_VI_YANKMARK
static char *text_yank(char *p, char *q, int dest) // copy text into a register
{
    char *t;
    int cnt;

    if (q < p) { // they are backwards- reverse them
        t = q;
        q = p;
        p = t;
    }
    cnt = q - p + 1;
    t = reg[dest];
    free(t);                  //  if already a yank register, free it
    t = xmalloc(cnt + 1);     // get a new register
    memset(t, '\0', cnt + 1); // clear new text[]
    strncpy(t, p, cnt);       // copy text[] into bufer
    reg[dest] = t;
    return p;
}

static char what_reg(void)
{
    char c;

    c = 'D'; // default to D-reg
    if (0 <= YDreg && YDreg <= 25)
        c = 'a' + (char)YDreg;
    if (YDreg == 26)
        c = 'D';
    if (YDreg == 27)
        c = 'U';
    return c;
}

static void check_context(char cmd)
{
    // A context is defined to be "modifying text"
    // Any modifying command establishes a new context.

    if (dot < context_start || dot > context_end) {
        if (strchr(modifying_cmds, cmd) != NULL) {
            // we are trying to modify text[]- make this the current context
            mark[27] = mark[26]; // move cur to prev
            mark[26] = dot;      // move local to cur
            context_start = prev_line(prev_line(dot));
            context_end = next_line(next_line(dot));
            // loiter= start_loiter= now;
        }
    }
}

static char *swap_context(char *p) // goto new context for '' command make this the current context
{
    char *tmp;

    // the current context is in mark[26]
    // the previous context is in mark[27]
    // only swap context if other context is valid
    if (text <= mark[27] && mark[27] <= end - 1) {
        tmp = mark[27];
        mark[27] = mark[26];
        mark[26] = tmp;
        p = mark[26]; // where we are going- previous context
        context_start = prev_line(prev_line(prev_line(p)));
        context_end = next_line(next_line(next_line(p)));
    }
    return p;
}
#endif /* FEATURE_VI_YANKMARK */

//----- Set terminal attributes --------------------------------
static int rawmode(void)
{
#ifdef TERMIOS
    int err = tcgetattr(0, &term_orig);
    if (err)
        return err;
    term_vi = term_orig;
    term_vi.c_lflag &= (~ICANON & ~ECHO); // leave ISIG ON- allow intr's
    term_vi.c_iflag &= (~IXON & ~ICRNL);
    term_vi.c_oflag &= (~ONLCR);
    term_vi.c_cc[VMIN] = 1;
    term_vi.c_cc[VTIME] = 0;
    erase_char = term_vi.c_cc[VERASE];
    tcsetattr(0, TCSANOW, &term_vi);
#else
    ioctl(0, TIOCGETP, &term_orig);
    term_vi = term_orig;
    term_vi.sg_flags &= ~(ECHO | CRMOD | XTABS | RAW);
    term_vi.sg_flags |= CBREAK;
    ioctl(0, TIOCSETP, &term_vi);

    ioctl(0, TIOCGETC, &tchars_orig);
    tchars_vi = tchars_orig;
    tchars_vi.t_eofc = -1;          /* end-of-file */
    tchars_vi.t_quitc = -1;         /* quit */
    tchars_vi.t_intrc = -1;         /* interrupt */
    ioctl(0, TIOCSETC, &tchars_vi);

    ioctl(0, TIOCGLTC, &ltc_orig);
    ltc_vi = ltc_orig;
    ltc_vi.t_suspc = -1;            /* stop process */
    ltc_vi.t_dsuspc = -1;           /* delayed stop process */
    ltc_vi.t_rprntc = -1;           /* reprint line */
    ltc_vi.t_flushc = -1;           /* flush output */
    ltc_vi.t_werasc = -1;           /* word erase */
    ltc_vi.t_lnextc = -1;           /* literal next character */
    ioctl(0, TIOCSLTC, &ltc_vi);
#endif

    unsigned tics = 1;
    ticsPerChar = tics;
    return 0;
}

static void cookmode(void)
{
#ifdef TERMIOS
    tcsetattr(0, TCSANOW, &term_orig);
#else
    ioctl(0, TIOCSETP, &term_orig);
    ioctl(0, TIOCSETC, &tchars_orig);
    ioctl(0, TIOCSLTC, &ltc_orig);
#endif
}

//----- Come here when we get a window resize signal ---------
#if ENABLE_FEATURE_VI_USE_SIGNALS
static void winch_sig(int sig ATTRIBUTE_UNUSED)
{
    createScreen();
    redraw();
    fflush(stdout);
}

//----- Come here on QUIT, PIPE, TERM, HUP ----------------------
static void quit_sig(int sig)
{
    gracefulExit();
    signal(sig, SIG_DFL);
    kill(my_pid, sig);
}

//----- Come here when we get a continue signal -------------------
static void cont_sig(int sig ATTRIBUTE_UNUSED)
{
    rawmode();          // terminal to "raw"
    *status_buffer = 0; // force status update
    redraw();
    fflush(stdout);
    signal(SIGTSTP, suspend_sig);
    signal(SIGCONT, SIG_DFL);
    kill(my_pid, SIGCONT);
}

//----- Come here when we get a Suspend signal -------------------
static void suspend_sig(int sig ATTRIBUTE_UNUSED)
{
    gracefulExit();
    signal(SIGCONT, cont_sig);
    signal(SIGTSTP, SIG_DFL);
    kill(my_pid, SIGTSTP);
}

//----- Come here when we get a INT signal ---------------------------
static void catch_sig(int sig)
{
    signal(SIGINT, catch_sig);
    if (sig)
        siglongjmp(restart, sig);
}
#endif /* FEATURE_VI_USE_SIGNALS */

//
// returns true if input is becomes available within tics/100 seconds
//
static int awaitInput(int tics)
{
    fflush(stdout);
#ifdef TERMIOS
    tcdrain(STDOUT_FILENO);
#endif
#if 0
    struct pollfd pfd[1];
    pfd[0].fd = 0;
    pfd[0].events = POLLIN;
    return safe_poll(pfd, 1, tics * 10) > 0;
#else
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);

    struct timeval tv;
    tv.tv_sec = tics / 100;
    tv.tv_usec = tics % 100 * 10000;

    int retval = select(1, &rfds, NULL, NULL, &tv);
    if (retval < -1) {
        perror("select");
    } else if (retval == 0) {
        // Timeout occurred
    } else if (FD_ISSET(0, &rfds)) {
        // Data is available
        return 1;
    }
#endif
    return 0;
}

//----- IO Routines --------------------------------------------
static char readit(void) // read (maybe cursor) key from stdin
{
    char c;
    size_t n;
    struct esc_cmds {
        const char seq[4];
        char val;
    };

    static const struct esc_cmds esccmds[] = {
        { "OA", VI_K_UP },        // cursor key Up
        { "OB", VI_K_DOWN },      // cursor key Down
        { "OC", VI_K_RIGHT },     // Cursor Key Right
        { "OD", VI_K_LEFT },      // cursor key Left
        { "OH", VI_K_HOME },      // Cursor Key Home
        { "OF", VI_K_END },       // Cursor Key End
        { "[A", VI_K_UP },        // cursor key Up
        { "[B", VI_K_DOWN },      // cursor key Down
        { "[C", VI_K_RIGHT },     // Cursor Key Right
        { "[D", VI_K_LEFT },      // cursor key Left
        { "[H", VI_K_HOME },      // Cursor Key Home
        { "[F", VI_K_END },       // Cursor Key End
        { "[1~", VI_K_HOME },     // Cursor Key Home
        { "[2~", VI_K_INSERT },   // Cursor Key Insert
        { "[3~", VI_K_DELETE },   // Cursor Key Delete
        { "[4~", VI_K_END },      // Cursor Key End
        { "[5~", VI_K_PAGEUP },   // Cursor Key Page Up
        { "[6~", VI_K_PAGEDOWN }, // Cursor Key Page Down
        { "OP", VI_K_FUN1 },      // Function Key F1
        { "OQ", VI_K_FUN2 },      // Function Key F2
        { "OR", VI_K_FUN3 },      // Function Key F3
        { "OS", VI_K_FUN4 },      // Function Key F4
        // careful: these have no terminating NUL!
        { "[11~", VI_K_FUN1 },  // Function Key F1
        { "[12~", VI_K_FUN2 },  // Function Key F2
        { "[13~", VI_K_FUN3 },  // Function Key F3
        { "[14~", VI_K_FUN4 },  // Function Key F4
        { "[15~", VI_K_FUN5 },  // Function Key F5
        { "[17~", VI_K_FUN6 },  // Function Key F6
        { "[18~", VI_K_FUN7 },  // Function Key F7
        { "[19~", VI_K_FUN8 },  // Function Key F8
        { "[20~", VI_K_FUN9 },  // Function Key F9
        { "[21~", VI_K_FUN10 }, // Function Key F10
        { "[23~", VI_K_FUN11 }, // Function Key F11
        { "[24~", VI_K_FUN12 }, // Function Key F12
    };
    enum { ESCCMDS_COUNT = ARRAY_SIZE(esccmds) };

    n = chars_to_parse;
    // get input from User- are there already input chars in Q?
    if (n <= 0) {
        // the Q is empty, wait for a typed char
        fflush(stdout);
        n = safe_read(STDIN_FILENO, readbuffer, sizeof(readbuffer));
        if (n < 0) {
            if (errno == EBADF || errno == EFAULT || errno == EINVAL || errno == EIO)
                editing = 0; // want to exit
            errno = 0;
        }
        if (n <= 0)
            return 0; // error
        if (readbuffer[0] == 27) {
            // This is an ESC char. Is this Esc sequence?
            // Could be bare Esc key. See if there are any
            // more chars to read after the ESC. This would
            // be a Function or Cursor Key sequence.
            // keep reading while there are input chars and room in buffer
            // for a complete ESC sequence (assuming 8 chars is enough)
            while (awaitInput(ticsPerChar)) {
                // read the rest of the ESC string
                int r = safe_read(STDIN_FILENO, readbuffer + n, sizeof(readbuffer) - n);
                if (r <= 0)
                    break;
                n += r;
                if (n > sizeof(readbuffer) - 8)
                    break;
            }
        }
        chars_to_parse = n;
    }
    c = readbuffer[0];
    if (c == 27 && n > 1) {
        // Maybe cursor or function key?
        const struct esc_cmds *eindex;

        for (eindex = esccmds; eindex < &esccmds[ESCCMDS_COUNT]; eindex++) {
            int cnt = strnlen(eindex->seq, 4);
            if (n <= cnt)
                continue;
            if (strncmp(eindex->seq, readbuffer + 1, cnt) != 0)
                continue;
            c = eindex->val; // magic char value
            n = cnt + 1;     // squeeze out the ESC sequence
            goto found;
        }
        // defined ESC sequence not found
    }
    n = 1;
found:
    // remove key sequence from Q
    chars_to_parse -= n;
    memmove(readbuffer, readbuffer + n, sizeof(readbuffer) - n);
    return c;
}

//----- IO Routines --------------------------------------------
static char get_one_char(void)
{
    char c;

#if ENABLE_FEATURE_VI_DOT_CMD
    if (!adding2q) {
        // we are not adding to the q.
        // but, we may be reading from a q
        if (ioq == 0) {
            // there is no current q, read from STDIN
            c = readit(); // get the users input
        } else {
            // there is a queue to get chars from first
            c = *ioq++;
            if (c == '\0') {
                // the end of the q, read from STDIN
                free(ioq_start);
                ioq_start = ioq = 0;
                c = readit(); // get the users input
            }
        }
    } else {
        // adding STDIN chars to q
        c = readit(); // get the users input
        if (lmc_len >= MAX_INPUT_LEN - 1) {
            status_line_bold("last_modifying_cmd overrun");
        } else {
            // add new char to q
            last_modifying_cmd[lmc_len++] = c;
        }
    }
#else
    c = readit(); // get the users input
#endif /* FEATURE_VI_DOT_CMD */
    return c;
}

// Get input line (uses "status line" area)
static char *get_input_line(const char *prompt)
{
    // char [MAX_INPUT_LEN]
#define buf status_buffer

    char c;
    int i;

    *displayed_buffer = 0; // force status update
    cmd_mode |= CMODE_LINE_INPUT;
    strcpy(buf, prompt);
    place_cursor(rows - 1, 0, FALSE); // go to Status line, bottom of screen
    clear_to_eol();                   // clear the line
    write1(prompt);                   // write out the :, /, or ? prompt

    i = strlen(buf);
    while (i < MAX_INPUT_LEN) {
        c = get_one_char();
        if (c == '\n' || c == '\r' || c == 27)
            break; // this is end of input
        if (c == erase_char || c == 8 || c == 127) {
            // user wants to erase prev char
            buf[--i] = '\0';
            write1("\b \b"); // erase char on screen
            if (i <= 0)      // user backs up before b-o-l, exit
                break;
        } else {
            buf[i] = c;
            buf[++i] = '\0';
            bb_putchar(c);
        }
    }
    cmd_mode &= ~CMODE_LINE_INPUT;
    return strcpy(get_input_line__buf, buf);
#undef buf
}

static int file_size(const char *fn) // what is the byte size of "fn"
{
    struct stat st_buf;
    int cnt;

    cnt = -1;
    if (fn && fn[0] && stat(fn, &st_buf) == 0) // see if file exists
        cnt = (int)st_buf.st_size;
    return cnt;
}

static int file_insert(const char *fn, char *p USE_FEATURE_VI_READONLY(, int update_ro_status))
{
    int cnt = -1;
    int fd, size;
    struct stat statbuf;

    /* Validate file */
    if (stat(fn, &statbuf) < 0) {
        status_line_bold("\"%s\" %s", fn, strerror(errno));
        goto fi0;
    }
    if (!S_ISREG(statbuf.st_mode)) {
        // This is not a regular file
        status_line_bold("\"%s\" Not a regular file", fn);
        goto fi0;
    }
    if (p < text || p > end) {
        status_line_bold("Trying to insert file outside of memory");
        goto fi0;
    }

    // read file to buffer
    fd = open(fn, O_RDONLY);
    if (fd < 0) {
        status_line_bold("\"%s\" %s", fn, strerror(errno));
        goto fi0;
    }
    size = statbuf.st_size;
    p = text_hole_make(p, size);
    cnt = safe_read(fd, p, size);
    if (cnt < 0) {
        status_line_bold("\"%s\" %s", fn, strerror(errno));
        p = text_hole_delete(p, p + size - 1); // un-do buffer insert
    } else if (cnt < size) {
        // There was a partial read, shrink unused space text[]
        p = text_hole_delete(p + cnt, p + (size - cnt) - 1); // un-do buffer insert
        status_line_bold("cannot read all of file \"%s\"", fn);
    }
    if (cnt >= size)
        file_modified++;
    close(fd);
fi0:
#if ENABLE_FEATURE_VI_READONLY
    if (update_ro_status && ((access(fn, W_OK) < 0) ||
                             /* root will always have access()
                              * so we check fileperms too */
                             !(statbuf.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)))) {
        SET_READONLY_FILE(readonly_mode);
    }
#endif
    return cnt;
}

static int file_write(char *fn, char *first, char *last)
{
    int fd, cnt, charcnt;

    if (fn == 0) {
        status_line_bold("No current filename");
        return -2;
    }
    charcnt = 0;
    /* By popular request we do not open file with O_TRUNC,
     * but instead ftruncate() it _after_ successful write.
     * Might reduce amount of data lost on power fail etc.
     */
    fd = open(fn, (O_WRONLY | O_CREAT), 0666);
    if (fd < 0)
        return -1;
    cnt = last - first + 1;
    charcnt = full_write(fd, first, cnt);
    ftruncate(fd, charcnt);
    if (charcnt == cnt) {
        // good write
        // file_modified = FALSE;
    } else {
        charcnt = 0;
    }
    close(fd);
    return charcnt;
}

//----- Terminal Drawing ---------------------------------------
// The terminal is made up of 'rows' line of 'columns' columns.
// classically this would be 24 x 80.
//  screen coordinates
//  0,0     ...     0,79
//  1,0     ...     1,79
//  .       ...     .
//  .       ...     .
//  22,0    ...     22,79
//  23,0    ...     23,79   <- status line

//----- Move the cursor to row x col (count from 0, not 1) -------
static void place_cursor(int row, int col, int optimize)
{
    char cm1[sizeof(CMrc) + sizeof(int) * 3 * 2];
    char *cm;

    if (row < 0)
        row = 0;
    if (row >= rows)
        row = rows - 1;
    if (col < 0)
        col = 0;
    if (col >= columns)
        col = columns - 1;

    //----- 1.  Try the standard terminal ESC sequence
    sprintf(cm1, CMrc, row + 1, col + 1);
    cm = cm1;

#if ENABLE_FEATURE_VI_OPTIMIZE_CURSOR
    if (optimize && col < 16) {
        enum {
            SZ_UP = sizeof(CMup),
            SZ_DN = sizeof(CMdown),
            SEQ_SIZE = SZ_UP > SZ_DN ? SZ_UP : SZ_DN,
        };
        char cm2[SEQ_SIZE * 5 + 32]; // bigger than worst case size
        char *screenp;
        int Rrow = last_row;
        int diff = Rrow - row;

        if (diff < -5 || diff > 5)
            goto skip;

        //----- find the minimum # of chars to move cursor -------------
        //----- 2.  Try moving with discreet chars (Newline, [back]space, ...)
        cm2[0] = '\0';

        // move to the correct row
        while (row < Rrow) {
            // the cursor has to move up
            strcat(cm2, CMup);
            Rrow--;
        }
        while (row > Rrow) {
            // the cursor has to move down
            strcat(cm2, CMdown);
            Rrow++;
        }

        // now move to the correct column
        strcat(cm2, "\r"); // start at col 0
        // just send out orignal source char to get to correct place
        screenp = &screen[row * columns]; // start of screen line
        strncat(cm2, screenp, col);

        // pick the shortest cursor motion to send out
        if (strlen(cm2) < strlen(cm)) {
            cm = cm2;
        }
    skip:;
    }
    last_row = row;
#endif /* FEATURE_VI_OPTIMIZE_CURSOR */
    write1(cm);
}

//----- Erase from cursor to end of line -----------------------
static void clear_to_eol(void)
{
    write1(Ceol); // Erase from cursor to end of line
}

//----- Erase from cursor to end of screen -----------------------
static void clear_to_eos(void)
{
    write1(Ceos);          // Erase from cursor to end of screen
    *displayed_buffer = 0; // status line was also cleared
}

//----- Start standout mode ------------------------------------
static void standout_start(void) // send "start reverse video" sequence
{
    write1(SOs); // Start reverse video mode
}

//----- End standout mode --------------------------------------
static void standout_end(void) // send "end reverse video" sequence
{
    write1(SOn); // End reverse video mode
}

//----- Flash the screen  --------------------------------------
static void flash(int h)
{
    standout_start(); // send "start reverse video" sequence
    redraw();
    awaitInput(h);
    standout_end(); // send "end reverse video" sequence
    redraw();
}

static void Indicate_Error(void)
{
#if ENABLE_FEATURE_VI_CRASHME
    if (crashme > 0)
        return; // generate a random command
#endif
    if (!err_method) {
        write1(bell); // send out a bell character
    } else {
        flash(10);
    }
}

//----- Screen[] Routines --------------------------------------
//----- Erase the Screen[] memory ------------------------------
static void screen_erase(void)
{
    memset(screen, ' ', screensize); // clear new screen
}

static const char *scompare(const char *s, const char *ref)
// why isn't this in the ANSI 'C' library?
{
    while (*s && *s == *ref)
        s++, ref++;
    return *s == *ref ? NULL : s;
}

//----- Draw the status line at bottom of the screen -------------
static void show_status_line(void)
{
    // either we already have an error or status message, or we
    // create one.
    const char *buffer = status_buffer;
    if (!*buffer)
        format_edit_status(EDIT_STATUS);
    const char *changed = scompare(buffer, displayed_buffer);
    if (changed) {
        size_t unchanged = changed - buffer;
        strcpy(displayed_buffer + unchanged, changed);
        // place cursor on correct column if line begins with standout text
        size_t escapes = *buffer == *SOs ? 2 * SOlen : 0;
        place_cursor(rows - 1, // put cursor on status line
                     escapes ? unchanged - SOlen : unchanged, FALSE);
        clear_to_eol();           // NOTE: assumes entire status text was in stand-out mode
        if (unchanged && escapes) // need to start in stand-out mode
            fwrite(buffer, SOlen, 1, stdout);
        size_t len = unchanged + strlen(buffer = changed);
        if (len - escapes > columns) {
            const char *limit = status_buffer + columns;
            if (escapes)
                limit += SOlen;
            fwrite(buffer, limit - buffer, 1, stdout);
            buffer = status_buffer + len;
            if (escapes && len > 2 * SOlen) // end w/restore to normal ESC sequence
                buffer -= SOlen;
        }
        write1(buffer); // this leaves cursor correctly placed for LINE_INPUT
        if (!(cmd_mode & CMODE_LINE_INPUT))
            place_cursor(crow, ccol, TRUE);            // otherwise, replace it in text area
    } else if (cmd_mode & CMODE_LINE_INPUT)            // in case status area was correct
        place_cursor(rows - 1, strlen(buffer), FALSE); // put cursor at its end
    else                                               // put cursor back in text area
        place_cursor(crow, ccol, TRUE);
}

//----- format the status buffer, the bottom line of screen ------
// format status buffer, with STANDOUT mode
static void status_line_bold(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    strcpy(status_buffer, SOs); // Terminal standout mode on
    vsprintf(status_buffer + sizeof(SOs) - 1, format, args);
    strcat(status_buffer, SOn); // Terminal standout mode off
    va_end(args);
}

// format status buffer
static void status_line(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vsprintf(status_buffer, format, args);
    va_end(args);
}

// copy s to buf, convert unprintable
static void print_literal(char *buf, const char *s)
{
    unsigned char c;
    char b[2];

    b[1] = '\0';
    buf[0] = '\0';
    if (!s[0])
        s = "(NULL)";
    for (; *s; s++) {
        int c_is_no_print;

        c = *s;
        c_is_no_print = (c & 0x80) && !Isprint(c);
        if (c_is_no_print) {
            strcat(buf, SOn);
            c = '.';
        }
        if (c < ' ' || c == 127) {
            strcat(buf, "^");
            if (c == 127)
                c = '?';
            else
                c += '@';
        }
        b[0] = c;
        strcat(buf, b);
        if (c_is_no_print)
            strcat(buf, SOs);
        if (*s == '\n')
            strcat(buf, "$");
        if (strlen(buf) > MAX_INPUT_LEN - 10) // paranoia
            break;
    }
}

static void not_implemented(const char *s)
{
    char buf[MAX_INPUT_LEN];

    print_literal(buf, s);
    status_line_bold("\'%s\' is not implemented", buf);
}

// show file status on status line
static int format_edit_status(const char *fmt)
{
#define tot format_edit_status__tot

    int cur, percent, ret, trunc_at;

    // file_modified is now a counter rather than a flag.  this
    // helps reduce the amount of line counting we need to do.
    // (this will cause a mis-reporting of modified status
    // once every MAXINT editing operations.)

    // it would be nice to do a similar optimization here -- if
    // we haven't done a motion that could have changed which line
    // we're on, then we shouldn't have to do this count_lines()
    cur = count_lines(text, dot);

    // reduce counting -- the total lines can't have
    // changed if we haven't done any edits.
    if (file_modified != last_file_modified) {
        tot = cur + count_lines(dot, end - 1) - 1;
        last_file_modified = file_modified;
    }

    //    current line         percent
    //   -------------    ~~ ----------
    //    total lines            100
    if (tot > 0) {
        percent = (100 * cur) / tot;
    } else {
        cur = tot = 0;
        percent = 100;
    }

    trunc_at = columns < STATUS_BUFFER_LEN - 1 ? columns : STATUS_BUFFER_LEN - 1;

    ret = snprintf(status_buffer, trunc_at + 1, fmt, cmd_mode_indicator[cmd_mode & CMODES],
                   (current_filename != NULL ? current_filename : "No file"),
#if ENABLE_FEATURE_VI_READONLY
                   (readonly_mode ? " [Readonly]" : ""),
#endif
                   (file_modified ? " [Modified]" : ""), cur, tot, percent, offset + ccol + 1,
                   cmdcnt);

    if (ret >= 0 && ret < trunc_at)
        return ret; /* it all fit */

    return trunc_at; /* had to truncate */
#undef tot
}

//----- Force refresh of all Lines -----------------------------
static void redraw(void)
{
    clear_screen(); // clear teminal screen and our image of it
    screen_erase();
    refresh();
}

//----- Format a text[] line into a buffer ---------------------
static char *format_line(char *src /*, int li*/)
{
    unsigned char c;
    int co;
    int ofs = offset;
    char *dest = scr_out_buf; // [MAX_SCR_COLS + MAX_TABSTOP * 2]

    c = '~'; // char in col 0 in non-existent lines is '~'
    co = 0;
    while (co < columns + tabstop) {
        // have we gone past the end?
        if (src < end) {
            c = *src++;
            if (c == '\n')
                break;
            if ((c & 0x80) && !Isprint(c)) {
                c = '.';
            }
            if (c < ' ' || c == 0x7f) {
                if (c == '\t') {
                    c = ' ';
                    //      co %    8     !=     7
                    while ((co % tabstop) != (tabstop - 1)) {
                        dest[co++] = c;
                    }
                } else {
                    dest[co++] = '^';
                    if (c == 0x7f)
                        c = '?';
                    else
                        c += '@'; // Ctrl-X -> 'X'
                }
            }
        }
        dest[co++] = c;
        // discard scrolled-off-to-the-left portion,
        // in tabstop-sized pieces
        if (ofs >= tabstop && co >= tabstop) {
            memmove(dest, dest + tabstop, co);
            co -= tabstop;
            ofs -= tabstop;
        }
        if (src >= end)
            break;
    }
    // check "short line, gigantic offset" case
    if (co < ofs)
        ofs = co;
    // discard last scrolled off part
    co -= ofs;
    dest += ofs;
    // fill the rest with spaces
    if (co < columns)
        memset(&dest[co], ' ', columns - co);
    return dest;
}

//----- Refresh the changed screen lines -----------------------
// Copy the source line from text[] into the buffer and note
// if the current screenline is different from the new buffer.
// If they differ then that line needs redrawing on the terminal.
//
static void refresh(void)
{
#define old_offset refresh__old_offset

    int li, changed;
    char *tp, *sp; // pointer into text[] and screen[]

    // poll to see if there is input already waiting. if we are
    // not able to display output fast enough to keep up, skip
    // the display update until we catch up with input.
    if (chars_to_parse || awaitInput(0))
        return;

    sync_cursor(dot, &crow, &ccol); // where cursor will be (on "dot")
    tp = screenbegin;               // index into text[] of top line

    // compare text[] to screen[] and mark screen[] lines that need updating
    for (li = 0; li < rows - 1 && !awaitInput(0); li++) {
        int cs, ce; // column start & end
        char *out_buf;
        // format current text line
        out_buf = format_line(tp /*, li*/);

        // skip to the end of the current text[] line
        if (tp < end) {
            char *t = memchr(tp, '\n', end - tp);
            if (!t)
                t = end - 1;
            tp = t + 1;
        }

        // see if there are any changes between vitual screen and out_buf
        changed = FALSE; // assume no change
        cs = 0;
        ce = columns - 1;
        sp = &screen[li * columns]; // start of screen line
        // compare newly formatted buffer with virtual screen
        // look forward for first difference between buf and screen
        for (; cs <= ce; cs++) {
            if (out_buf[cs] != sp[cs]) {
                changed = TRUE; // mark for redraw
                break;
            }
        }

        // look backward for last difference between out_buf and screen
        for (; ce >= cs; ce--) {
            if (out_buf[ce] != sp[ce]) {
                changed = TRUE; // mark for redraw
                break;
            }
        }
        // now, cs is index of first diff, and ce is index of last diff

        // if horz offset has changed, force a redraw
        if (offset != old_offset) {
            changed = TRUE;
        }

        // make a sanity check of columns indexes
        if (cs < 0)
            cs = 0;
        if (ce > columns - 1)
            ce = columns - 1;
        if (cs > ce) {
            cs = 0;
            ce = columns - 1;
        }
        // is there a change between virtual screen and out_buf
        if (changed) {
            // copy changed part of buffer to virtual screen
            memcpy(sp + cs, out_buf + cs, ce - cs + 1);

            // move cursor to column of first change
            // if (offset != old_offset) {
            //	// place_cursor is still too stupid
            //	// to handle offsets correctly
            //	place_cursor(li, cs, FALSE);
            //} else {
            place_cursor(li, cs, TRUE);
            //}

            // write line out to terminal
            fwrite(&sp[cs], ce - cs + 1, 1, stdout);
        }
    }

    place_cursor(crow, ccol, TRUE);

    old_offset = offset;
#undef old_offset
    show_status_line();
}

//---------------------------------------------------------------------
//----- the Ascii Chart -----------------------------------------------
//
//  00 nul   01 soh   02 stx   03 etx   04 eot   05 enq   06 ack   07 bel
//  08 bs    09 ht    0a nl    0b vt    0c np    0d cr    0e so    0f si
//  10 dle   11 dc1   12 dc2   13 dc3   14 dc4   15 nak   16 syn   17 etb
//  18 can   19 em    1a sub   1b esc   1c fs    1d gs    1e rs    1f us
//  20 sp    21 !     22 "     23 #     24 $     25 %     26 &     27 '
//  28 (     29 )     2a *     2b +     2c ,     2d -     2e .     2f /
//  30 0     31 1     32 2     33 3     34 4     35 5     36 6     37 7
//  38 8     39 9     3a :     3b ;     3c <     3d =     3e >     3f ?
//  40 @     41 A     42 B     43 C     44 D     45 E     46 F     47 G
//  48 H     49 I     4a J     4b K     4c L     4d M     4e N     4f O
//  50 P     51 Q     52 R     53 S     54 T     55 U     56 V     57 W
//  58 X     59 Y     5a Z     5b [     5c \     5d ]     5e ^     5f _
//  60 `     61 a     62 b     63 c     64 d     65 e     66 f     67 g
//  68 h     69 i     6a j     6b k     6c l     6d m     6e n     6f o
//  70 p     71 q     72 r     73 s     74 t     75 u     76 v     77 w
//  78 x     79 y     7a z     7b {     7c |     7d }     7e ~     7f del
//---------------------------------------------------------------------

//----- Execute a Vi Command -----------------------------------
static void do_cmd(char c)
{
    const char *msg;
    char c1, *p, *q, *save_dot;
    char buf[12];
    int dir, cnt, i, j;

again:
    /* if this is a cursor key, skip these checks */
    switch (c) {
    case VI_K_UP:
    case VI_K_DOWN:
    case VI_K_LEFT:
    case VI_K_RIGHT:
    case VI_K_HOME:
    case VI_K_END:
    case VI_K_PAGEUP:
    case VI_K_PAGEDOWN:
        goto key_cmd_mode;
    }

    if (cmd_mode == CMODE_REPLACE) {
        //  flip-flop Insert/Replace mode
        if (c == VI_K_INSERT)
            goto dc_i;
        // we are 'R'eplacing the current *dot with new char
        if (*dot == '\n') {
            // don't Replace past E-o-l
            cmd_mode = CMODE_INSERT; // convert to insert
        } else {
            if (1 <= c || Isprint(c)) {
                if (c != 27)
                    dot = yank_delete(dot, dot, 0, YANKDEL); // delete char
                dot = char_insert(dot, c);                   // insert new char
            }
            goto dc1;
        }
    }
    if (cmd_mode == CMODE_INSERT) {
        //  hitting "Insert" twice means "R" replace mode
        if (c == VI_K_INSERT)
            goto dc5;
        // insert the char c at "dot"
        if (1 <= c || Isprint(c)) {
            dot = char_insert(dot, c);
        }
        goto dc1;
    }

key_cmd_mode:
    switch (c) {
        // case 0x01:	// soh
        // case 0x09:	// ht
        // case 0x0b:	// vt
        // case 0x0e:	// so
        // case 0x0f:	// si
        // case 0x10:	// dle
        // case 0x11:	// dc1
        // case 0x13:	// dc3
#if ENABLE_FEATURE_VI_CRASHME
    case 0x14: // dc4  ctrl-T
        crashme = (crashme == 0) ? 1 : 0;
        break;
#endif
        // case 0x16:	// syn
        // case 0x17:	// etb
        // case 0x18:	// can
        // case 0x1c:	// fs
        // case 0x1d:	// gs
        // case 0x1e:	// rs
        // case 0x1f:	// us
        // case '!':	// !-
        // case '#':	// #-
        // case '&':	// &-
        // case '(':	// (-
        // case ')':	// )-
        // case '*':	// *-
        // case '=':	// =-
        // case '@':	// @-
        // case 'F':	// F-
        // case 'K':	// K-
        // case 'Q':	// Q-
        // case 'S':	// S-
        // case 'T':	// T-
        // case 'V':	// V-
        // case '[':	// [-
        // case '\\':	// \-
        // case ']':	// ]-
        // case '_':	// _-
        // case '`':	// `-
        // case 'u':	// u- FIXME- there is no undo
        // case 'v':	// v-
    default: // unrecognised command
        buf[0] = c;
        buf[1] = '\0';
        if (c < ' ') {
            buf[0] = '^';
            buf[1] = c + '@';
            buf[2] = '\0';
        }
        not_implemented(buf);
        end_cmd_q(); // stop adding to q
    case 0x00:       // nul- ignore
        break;
    case 2:           // ctrl-B  scroll up   full screen
    case VI_K_PAGEUP: // Cursor Key Page Up
        dot_scroll(rows - 2, -1);
        break;
    case 4: // ctrl-D  scroll down half screen
        dot_scroll((rows - 2) / 2, 1);
        break;
    case 5: // ctrl-E  scroll down one line
        dot_scroll(1, 1);
        break;
    case 6:             // ctrl-F  scroll down full screen
    case VI_K_PAGEDOWN: // Cursor Key Page Down
        dot_scroll(rows - 2, 1);
        break;
    case 7: // ctrl-G  show current status
        format_edit_status(EDIT_STATUS " col %d");
        break;
    case 'h':       // h- move left
    case VI_K_LEFT: // cursor key Left
    case 8:         // ctrl-H- move left    (This may be ERASE char)
    case 0x7f:      // DEL- move left   (This may be ERASE char)
        dot_left();
    repeat:
        if (cmdcnt-- > 1)
            goto again;
        break;
    case 10:                                   // Newline ^J
    case 'j':                                  // j- goto next line, same col
    case VI_K_DOWN:                            // cursor key Down
        dot_next();                            // go to next B-o-l
        dot = move_to_col(dot, ccol + offset); // try stay in same col
        goto repeat;
    case 12: // ctrl-L  force redraw whole screen
    case 18: // ctrl-R  force redraw
        createScreen();
        redraw();
        break;
    case 13:  // Carriage Return ^M
    case '+': // +- goto next line
        dot_next();
        dot_skip_over_ws();
        goto repeat;
    case 21: // ctrl-U  scroll up   half screen
        dot_scroll((rows - 2) / 2, -1);
        break;
    case 25: // ctrl-Y  scroll up one line
        dot_scroll(1, -1);
        break;
    case 27: // esc
        if (cmd_mode == CMODE_COMMAND)
            indicate_error(c);
        cmd_mode = CMODE_COMMAND; // stop insrting
        end_cmd_q();
        break;
    case ' ':        // move right
    case 'l':        // move right
    case VI_K_RIGHT: // Cursor Key Right
        dot_right();
        goto repeat;
#if ENABLE_FEATURE_VI_YANKMARK
    case '"': // "- name a register to use for Delete/Yank
        c1 = get_one_char();
        c1 = tolower(c1);
        if (islower(c1)) {
            YDreg = c1 - 'a';
        } else {
            indicate_error(c);
        }
        break;
    case '\'': // '- goto a specific mark
        c1 = get_one_char();
        c1 = tolower(c1);
        if (islower(c1)) {
            c1 = c1 - 'a';
            // get the b-o-l
            q = mark[(unsigned char)c1];
            if (text <= q && q < end) {
                dot = q;
                dot_begin(); // go to B-o-l
                dot_skip_over_ws();
            }
        } else if (c1 == '\'') {     // goto previous context
            dot = swap_context(dot); // swap current and previous context
            dot_begin();             // go to B-o-l
            dot_skip_over_ws();
        } else {
            indicate_error(c);
        }
        break;
    case 'm': // m- Mark a line
        // this is really stupid.  If there are any inserts or deletes
        // between text[0] and dot then this mark will not point to the
        // correct location! It could be off by many lines!
        // Well..., at least its quick and dirty.
        c1 = get_one_char();
        c1 = tolower(c1);
        if (islower(c1)) {
            c1 = c1 - 'a';
            // remember the line
            mark[(int)c1] = dot;
        } else {
            indicate_error(c);
        }
        break;
    case 'P': // P- Put register before
    case 'p': // p- put register after
        p = reg[YDreg];
        if (p == 0) {
            status_line_bold("Nothing in register %c", what_reg());
            break;
        }
        // are we putting whole lines or strings
        if (strchr(p, '\n') != NULL) {
            if (c == 'P') {
                dot_begin(); // putting lines- Put above
            }
            if (c == 'p') {
                // are we putting after very last line?
                if (end_line(dot) == (end - 1)) {
                    dot = end; // force dot to end of text[]
                } else {
                    dot_next(); // next line, then put before
                }
            }
        } else {
            if (c == 'p')
                dot_right(); // move to right, can move to NL
        }
        dot = string_insert(dot, p); // insert the string
        end_cmd_q();                 // stop adding to q
        break;
    case 'U': // U- Undo; replace current line with original version
        if (reg[Ureg] != 0) {
            p = begin_line(dot);
            q = end_line(dot);
            p = text_hole_delete(p, q);      // delete cur line
            p = string_insert(p, reg[Ureg]); // insert orig line
            dot = p;
            dot_skip_over_ws();
        }
        break;
#endif             /* FEATURE_VI_YANKMARK */
    case '$':      // $- goto end of line
    case VI_K_END: // Cursor Key End
        dot = end_line(dot);
        goto repeat;
    case '%': // %- find matching char of pair () [] {}
        for (q = dot; q < end && *q != '\n'; q++) {
            if (strchr("()[]{}", *q) != NULL) {
                // we found half of a pair
                p = find_pair(q, *q);
                if (p == NULL) {
                    indicate_error(c);
                } else {
                    dot = p;
                }
                break;
            }
        }
        if (*q == '\n')
            indicate_error(c);
        break;
    case 'f':                               // f- forward to a user specified char
        last_forward_char = get_one_char(); // get the search char
                                            //
                                            // dont separate these two commands. 'f' depends on ';'
                                            //
                                            //**** fall through to ... ';'
    case ';':                               // ;- look at rest of line for last forward char
        if (last_forward_char == 0)
            break;
        q = dot + 1;
        while (q < end - 1 && *q != '\n' && *q != last_forward_char) {
            q++;
        }
        if (*q == last_forward_char)
            dot = q;
        c = ';';
        goto repeat;
    case ',': // repeat latest 'f' in opposite direction
        if (last_forward_char == 0)
            break;
        q = dot - 1;
        while (q >= text && *q != '\n' && *q != last_forward_char) {
            q--;
        }
        if (q >= text && *q == last_forward_char)
            dot = q;
        c = ',';
        goto repeat;

    case '-': // -- goto prev line
        dot_prev();
        dot_skip_over_ws();
        goto repeat;
#if ENABLE_FEATURE_VI_DOT_CMD
    case '.': // .- repeat the last modifying command
        // Stuff the last_modifying_cmd back into stdin
        // and let it be re-executed.
        if (lmc_len > 0) {
            last_modifying_cmd[lmc_len] = 0;
            ioq = ioq_start = xstrdup(last_modifying_cmd);
        }
        break;
#endif
#if ENABLE_FEATURE_VI_SEARCH
    case '?': // /- search for a pattern
    case '/': // /- search for a pattern
        buf[0] = c;
        buf[1] = '\0';
        q = get_input_line(buf); // get input line- use "status line"
        if (!*q)
            break;  // bail out if user erased the entire pattern
        if (q[1]) { // strlen(q) > 1: new pat- save it and find
            free(last_search_pattern);
            last_search_pattern = xstrdup(q);
            goto findNormal; // new pattern determines search direction
        } // Reuse pattern. If c=='?', search in direction opposite pattern's
        if (c == '/')
            goto findNormal;
        c = 'N';
    case 'N':       // N- repeat search in opposite direction of last pattern
        dir = BACK; // BACK here means "opposite" pattern's spec'd direction
        goto findPattern;

    findNormal:
        c = 'n';
    case 'n': // n- repeat search for last pattern in "normal" direction
        // search rest of text[] starting at next char
        // if search fails return orignal "p" not the "p+1" address
        dir = FORWARD; // in pattern's spec'd direction
    findPattern:
        if (!last_search_pattern) {
            msg = "No previous regular expression";
            goto dc2;
        } // derive absolute search direction from that relative to pattern's
        if (*last_search_pattern == '?')
            dir = -dir; // relies on FORWARD/BACK being 1/-1, respectively
        p = dot + dir;
        q = char_search(p, last_search_pattern + 1, dir, FULL);
        if (q != NULL) {
            dot = q; // good search, update "dot"
            goto repeat;
        }
        // no pattern found between "dot" and "end"- continue at top
        p = text;
        if (dir == BACK) {
            p = end - 1;
        }
        q = char_search(p, last_search_pattern + 1, dir, FULL);
        if (q != NULL) { // found something
            dot = q;     // found new pattern- goto it
            msg = "search hit BOTTOM, continuing at TOP";
            if (dir == BACK)
                msg = "search hit TOP, continuing at BOTTOM";
        } else
            msg = "Pattern not found";
    dc2:
        status_line_bold("%s", msg);
        break;
    case '{': // {- move backward paragraph
        q = char_search(dot, "\n\n", BACK, FULL);
        if (q != NULL) {        // found blank line
            dot = next_line(q); // move to next blank line
        }
        break;
    case '}': // }- move forward paragraph
        q = char_search(dot, "\n\n", FORWARD, FULL);
        if (q != NULL) {        // found blank line
            dot = next_line(q); // move to next blank line
        }
        break;
#endif        /* FEATURE_VI_SEARCH */
    case '0': // 0- goto begining of line
    case '1': // 1-
    case '2': // 2-
    case '3': // 3-
    case '4': // 4-
    case '5': // 5-
    case '6': // 6-
    case '7': // 7-
    case '8': // 8-
    case '9': // 9-
        if (c == '0' && cmdcnt < 1) {
            dot_begin(); // this was a standalone zero
        } else {
            if (cmdcnt >= INT_MAX / 10) {
                cmdcnt = 0;
                status_line_bold("Repeat Count OVERFLOW");
                return;
            }
            cmdcnt = cmdcnt * 10 + (c - '0'); // this 0 is part of a number
            if (cmdcnt != 1)
                format_edit_status(EDIT_STATUS " col %d {%d times}");
        }
        break;
    case ':':                    // :- the colon mode commands
        p = get_input_line(":"); // get input line- use "status line"
#if ENABLE_FEATURE_VI_COLON
        colon(p); // execute the command
#else
        if (*p == ':')
            p++; // move past the ':'
        cnt = strlen(p);
        if (cnt <= 0)
            break;
        if (strncasecmp(p, "quit", cnt) == 0 || strncasecmp(p, "q!", cnt) == 0 // delete lines
        ) {
            if (file_modified && p[1] != '!') {
                status_line_bold("No write since last change (:quit! overrides)");
            } else {
                editing = 0;
            }
        } else if (strncasecmp(p, "write", cnt) == 0 || strncasecmp(p, "wq", cnt) == 0 ||
                   strncasecmp(p, "wn", cnt) == 0 || strncasecmp(p, "x", cnt) == 0) {
            cnt = file_write(current_filename, text, end - 1);
            if (cnt < 0) {
                if (cnt == -1)
                    status_line_bold("Write error: %s", strerror(errno));
            } else {
                file_modified = 0;
                last_file_modified = -1;
                status_line("\"%s\" %dL, %dC", current_filename, count_lines(text, end - 1), cnt);
                if (p[0] == 'x' || p[1] == 'q' || p[1] == 'n' || p[0] == 'X' || p[1] == 'Q' ||
                    p[1] == 'N') {
                    editing = 0;
                }
            }
        } else if (strncasecmp(p, "file", cnt))
            if (sscanf(p, "%d", &j) > 0) {
                dot = find_line(j); // go to line # j
                dot_skip_over_ws();
            } else { // unrecognised cmd
                not_implemented(p);
            }
#endif /* !FEATURE_VI_COLON */
        break;
    case '<':                         // <- Left  shift something
    case '>':                         // >- Right shift something
        cnt = count_lines(text, dot); // remember what line we are on
        c1 = get_one_char();          // get the type of thing to delete
        find_range(&p, &q, c1);
        yank_delete(p, q, 1, YANKONLY); // save copy before change
        p = begin_line(p);
        q = end_line(q);
        i = count_lines(p, q); // # of lines we are shifting
        for (; i > 0; i--, p = next_line(p)) {
            if (c == '<') {
                // shift left- remove tab or 8 spaces
                if (*p == '\t') {
                    // shrink buffer 1 char
                    text_hole_delete(p, p);
                } else if (*p == ' ') {
                    // we should be calculating columns, not just SPACE
                    for (j = 0; *p == ' ' && j < tabstop; j++) {
                        text_hole_delete(p, p);
                    }
                }
            } else if (c == '>') {
                // shift right -- add tab or 8 spaces
                char_insert(p, '\t');
            }
        }
        dot = find_line(cnt); // what line were we on
        dot_skip_over_ws();
        end_cmd_q(); // stop adding to q
        break;
    case 'A':      // A- append at e-o-l
        dot_end(); // go to e-o-l
                   //**** fall through to ... 'a'
    case 'a':      // a- append after current char
        if (*dot != '\n')
            dot++;
        goto dc_i;
        break;
    case 'B': // B- back a blank-delimited Word
    case 'E': // E- end of a blank-delimited word
    case 'W': // W- forward a blank-delimited word
        dir = FORWARD;
        if (c == 'B')
            dir = BACK;
        if (c == 'W' || isspace(dot[dir])) {
            dot = skip_thing(dot, 1, dir, S_TO_WS);
            dot = skip_thing(dot, 2, dir, S_OVER_WS);
        }
        if (c != 'W')
            dot = skip_thing(dot, 1, dir, S_BEFORE_WS);
        goto repeat;
    case 'C': // C- Change to e-o-l
    case 'D': // D- delete to e-o-l
        save_dot = dot;
        dot = dollar_line(dot); // move to before NL
        // copy text into a register and delete
        dot = yank_delete(save_dot, dot, 0, YANKDEL); // delete to e-o-l
        if (c == 'C')
            goto dc_i; // start inserting
#if ENABLE_FEATURE_VI_DOT_CMD
        if (c == 'D')
            end_cmd_q(); // stop adding to q
#endif
        break;
    case 'g': // 'gg' goto a line number (from vim)
              // (default to first line in file)
        c1 = get_one_char();
        if (c1 != 'g') {
            buf[0] = 'g';
            buf[1] = c1;
            buf[2] = '\0';
            not_implemented(buf);
            break;
        }
        if (cmdcnt == 0)
            cmdcnt = 1;
        /* fall through */
    case 'G':          // G- goto to a line number (default= E-O-F)
        dot = end - 1; // assume E-O-F
        if (cmdcnt > 0) {
            dot = find_line(cmdcnt); // what line is #cmdcnt
        }
        dot_skip_over_ws();
        break;
    case 'H': // H- goto top line on screen
        dot = screenbegin;
        if (cmdcnt > (rows - 1)) {
            cmdcnt = (rows - 1);
        }
        if (cmdcnt-- > 1) {
            do_cmd('+');
        } // repeat cnt
        dot_skip_over_ws();
        break;
    case 'I':        // I- insert before first non-blank
        dot_begin(); // 0
        dot_skip_over_ws();
        //**** fall through to ... 'i'
    case 'i':         // i- insert before current char
    case VI_K_INSERT: // Cursor Key Insert
    dc_i:
        cmd_mode = CMODE_INSERT; // start insrting
        break;
    case 'J':                // J- join current and next lines together
        dot_end();           // move to NL
        if (dot < end - 1) { // make sure not last char in text[]
            *dot++ = ' ';    // replace NL with space
            file_modified++;
            while (isblank(*dot)) { // delete leading WS
                dot_delete();
            }
        }
        end_cmd_q(); // stop adding to q
        if (cmdcnt-- > 2)
            goto again;
        break;
    case 'L': // L- goto bottom line on screen
        dot = end_screen();
        if (cmdcnt > (rows - 1)) {
            cmdcnt = (rows - 1);
        }
        if (cmdcnt-- > 1) {
            do_cmd('-');
        } // repeat cnt
        dot_begin();
        dot_skip_over_ws();
        break;
    case 'M': // M- goto middle line on screen
        dot = screenbegin;
        for (cnt = 0; cnt < (rows - 1) / 2; cnt++)
            dot = next_line(dot);
        break;
    case 'O': // O- open a empty line above
        //    0i\n ESC -i
        p = begin_line(dot);
        if (p[-1] == '\n') {
            dot_prev();
        case 'o': // o- open a empty line below; Yes, I know it is in the middle of the "if (..."
            dot_end();
            dot = char_insert(dot, '\n');
        } else {
            dot_begin();                  // 0
            dot = char_insert(dot, '\n'); // i\n ESC
            dot_prev();                   // -
        }
        goto dc_i;
        break;
    case 'R': // R- continuous Replace char
    dc5:
        cmd_mode = CMODE_REPLACE;
        break;
    case VI_K_DELETE:
        c = 'x';
        // fall through
    case 'X': // X- delete char before dot
    case 'x': // x- delete the current char
    case 's': // s- substitute the current char
        dir = 0;
        if (c == 'X')
            dir = -1;
        if (dot[dir] != '\n') {
            if (c == 'X')
                dot--;                               // delete prev char
            dot = yank_delete(dot, dot, 0, YANKDEL); // delete char
        }
        if (c == 's')
            goto dc_i; // start insrting
        end_cmd_q();   // stop adding to q
        goto repeat;
    case 'Z': // Z- if modified, {write}; exit
        // ZZ means to save file (if necessary), then exit
        c1 = get_one_char();
        if (c1 != 'Z') {
            indicate_error(c);
            break;
        }
        if (file_modified) {
            if (ENABLE_FEATURE_VI_READONLY && readonly_mode) {
                status_line_bold("\"%s\" File is read only", current_filename);
                break;
            }
            cnt = file_write(current_filename, text, end - 1);
            if (cnt < 0) {
                if (cnt == -1)
                    status_line_bold("Write error: %s", strerror(errno));
            } else if (cnt == (end - 1 - text + 1)) {
                editing = 0;
            }
        } else {
            editing = 0;
        }
        break;
    case '^': // ^- move to first non-blank on line
        dot_begin();
        dot_skip_over_ws();
        break;
    case 'b': // b- back a word
    case 'e': // e- end of word
        dir = FORWARD;
        if (c == 'b')
            dir = BACK;
        if ((dot + dir) < text || (dot + dir) > end - 1)
            break;
        dot += dir;
        if (isspace(*dot)) {
            dot = skip_thing(dot, (c == 'e') ? 2 : 1, dir, S_OVER_WS);
        }
        if (isalnum(*dot) || *dot == '_') {
            dot = skip_thing(dot, 1, dir, S_END_ALNUM);
        } else if (ispunct(*dot)) {
            dot = skip_thing(dot, 1, dir, S_END_PUNCT);
        }
        goto repeat;
    case 'c': // c- change something
    case 'd': // d- delete something
#if ENABLE_FEATURE_VI_YANKMARK
    case 'y': // y- yank   something
    case 'Y': // Y- Yank a line
#endif
    {
        int yf, ml, whole = 0;
        yf = YANKDEL; // assume either "c" or "d"
#if ENABLE_FEATURE_VI_YANKMARK
        if (c == 'y' || c == 'Y')
            yf = YANKONLY;
#endif
        c1 = 'y';
        if (c != 'Y')
            c1 = get_one_char(); // get the type of thing to delete
        // determine range, and whether it spans lines
        ml = find_range(&p, &q, c1);
        if (c1 == 27) {  // ESC- user changed mind and wants out
            c = c1 = 27; // Escape- do nothing
        } else if (strchr("wW", c1)) {
            if (c == 'c') {
                // don't include trailing WS as part of word
                while (isblank(*q)) {
                    if (q <= text || q[-1] == '\n')
                        break;
                    q--;
                }
            }
            dot = yank_delete(p, q, ml, yf); // delete word
        } else if (strchr("^0bBeEft%$ lh\b\177", c1)) {
            // partial line copy text into a register and delete
            dot = yank_delete(p, q, ml, yf); // delete word
        } else if (strchr("cdykjHL+-{}\r\n", c1)) {
            // whole line copy text into a register and delete
            dot = yank_delete(p, q, ml, yf); // delete lines
            whole = 1;
        } else {
            // could not recognize object
            c = c1 = 27; // error-
            ml = 0;
            indicate_error(c);
        }
        if (ml && whole) {
            if (c == 'c') {
                dot = char_insert(dot, '\n');
                // on the last line of file don't move to prev line
                if (whole && dot != (end - 1)) {
                    dot_prev();
                }
            } else if (c == 'd') {
                dot_begin();
                dot_skip_over_ws();
            }
        }
        if (c1 != 27) {
            // if CHANGING, not deleting, start inserting after the delete
            if (c == 'c') {
                strcpy(buf, "Change");
                goto dc_i; // start inserting
            }
            if (c == 'd') {
                strcpy(buf, "Delete");
            }
#if ENABLE_FEATURE_VI_YANKMARK
            if (c == 'y' || c == 'Y') {
                strcpy(buf, "Yank");
            }
            p = reg[YDreg];
            q = p + strlen(p);
            for (cnt = 0; p <= q; p++) {
                if (*p == '\n')
                    cnt++;
            }
            status_line("%s %d lines (%d chars) using [%c]", buf, cnt, strlen(reg[YDreg]),
                        what_reg());
#endif
            end_cmd_q(); // stop adding to q
        }
    } break;
    case 'k':     // k- goto prev line, same col
    case VI_K_UP: // cursor key Up
        dot_prev();
        dot = move_to_col(dot, ccol + offset); // try stay in same col
        goto repeat;
    case 'r':                // r- replace the current char with user input
        c1 = get_one_char(); // get the replacement char
        if (*dot != '\n') {
            *dot = c1;
            file_modified++;
        }
        end_cmd_q(); // stop adding to q
        break;
    case 't': // t- move to char prior to next x
        last_forward_char = get_one_char();
        do_cmd(';');
        if (*dot == last_forward_char)
            dot_left();
        last_forward_char = 0;
        break;
    case 'w':                               // w- forward a word
        if (isalnum(*dot) || *dot == '_') { // we are on ALNUM
            dot = skip_thing(dot, 1, FORWARD, S_END_ALNUM);
        } else if (ispunct(*dot)) { // we are on PUNCT
            dot = skip_thing(dot, 1, FORWARD, S_END_PUNCT);
        }
        if (dot < end - 1)
            dot++; // move over word
        if (isspace(*dot)) {
            dot = skip_thing(dot, 2, FORWARD, S_OVER_WS);
        }
        goto repeat;
    case 'z':                // z-
        c1 = get_one_char(); // get the replacement char
        cnt = 0;
        if (c1 == '.')
            cnt = (rows - 2) / 2; // put dot at center
        if (c1 == '-')
            cnt = rows - 2;            // put dot at bottom
        screenbegin = begin_line(dot); // start dot at top
        dot_scroll(cnt, -1);
        break;
    case '|':                               // |- move to column "cmdcnt"
        dot = move_to_col(dot, cmdcnt - 1); // try to move to column
        break;
    case '~': // ~- flip the case of letters   a-z -> A-Z
        if (islower(*dot)) {
            *dot = toupper(*dot);
            file_modified++;
        } else if (isupper(*dot)) {
            *dot = tolower(*dot);
            file_modified++;
        }
        dot_right();
        end_cmd_q(); // stop adding to q
        goto repeat;
        //----- The Cursor and Function Keys -----------------------------
    case VI_K_HOME: // Cursor Key Home
        dot_begin();
        break;
        // The Fn keys could point to do_macro which could translate them
    case VI_K_FUN1:  // Function Key F1
    case VI_K_FUN2:  // Function Key F2
    case VI_K_FUN3:  // Function Key F3
    case VI_K_FUN4:  // Function Key F4
    case VI_K_FUN5:  // Function Key F5
    case VI_K_FUN6:  // Function Key F6
    case VI_K_FUN7:  // Function Key F7
    case VI_K_FUN8:  // Function Key F8
    case VI_K_FUN9:  // Function Key F9
    case VI_K_FUN10: // Function Key F10
    case VI_K_FUN11: // Function Key F11
    case VI_K_FUN12: // Function Key F12
        break;
    }

dc1:
    // if text[] just became empty, add back an empty line
    if (end == text) {
        char_insert(text, '\n'); // start empty buf with dummy line
        dot = text;
    }
    // it is OK for dot to exactly equal to end, otherwise check dot validity
    if (dot != end) {
        dot = bound_dot(dot); // make sure "dot" is valid
    }
#if ENABLE_FEATURE_VI_YANKMARK
    check_context(c); // update the current context
#endif

    if (!isdigit(c))
        cmdcnt = 0; // cmd was not a number, reset cmdcnt
    cnt = dot - begin_line(dot);
    // Try to stay off of the Newline
    if (*dot == '\n' && cnt > 0 && cmd_mode == CMODE_COMMAND)
        dot--;
}

/* NB!  the CRASHME code is unmaintained, and doesn't currently build */
#if ENABLE_FEATURE_VI_CRASHME
static int totalcmds = 0;
static int Mp = 85; // Movement command Probability
static int Np = 90; // Non-movement command Probability
static int Dp = 96; // Delete command Probability
static int Ip = 97; // Insert command Probability
static int Yp = 98; // Yank command Probability
static int Pp = 99; // Put command Probability
static int M = 0, N = 0, I = 0, D = 0, Y = 0, P = 0, U = 0;
static const char chars[20] = "\t012345 abcdABCD-=.$";
static const char *const words[20] = { "this",  "is",        "a",       "test",    "broadcast",
                                       "the",   "emergency", "of",      "system",  "quick",
                                       "brown", "fox",       "jumped",  "over",    "lazy",
                                       "dogs",  "back",      "January", "Febuary", "March" };
static const char *const lines[20] = {
    "You should have received a copy of the GNU General Public License\n",
    "char c, cm, *cmd, *cmd1;\n",
    "generate a command by percentages\n",
    "Numbers may be typed as a prefix to some commands.\n",
    "Quit, discarding changes!\n",
    "Forced write, if permission originally not valid.\n",
    "In general, any ex or ed command (such as substitute or delete).\n",
    "I have tickets available for the Blazers vs LA Clippers for Monday, Janurary 1 at 1:00pm.\n",
    "Please get w/ me and I will go over it with you.\n",
    "The following is a list of scheduled, committed changes.\n",
    "1.   Launch Norton Antivirus (Start, Programs, Norton Antivirus)\n",
    "Reminder....Town Meeting in Central Perk cafe today at 3:00pm.\n",
    "Any question about transactions please contact Sterling Huxley.\n",
    "I will try to get back to you by Friday, December 31.\n",
    "This Change will be implemented on Friday.\n",
    "Let me know if you have problems accessing this;\n",
    "Sterling Huxley recently added you to the access list.\n",
    "Would you like to go to lunch?\n",
    "The last command will be automatically run.\n",
    "This is too much english for a computer geek.\n",
};
static char *multilines[20] = {
    "You should have received a copy of the GNU General Public License\n",
    "char c, cm, *cmd, *cmd1;\n",
    "generate a command by percentages\n",
    "Numbers may be typed as a prefix to some commands.\n",
    "Quit, discarding changes!\n",
    "Forced write, if permission originally not valid.\n",
    "In general, any ex or ed command (such as substitute or delete).\n",
    "I have tickets available for the Blazers vs LA Clippers for Monday, Janurary 1 at 1:00pm.\n",
    "Please get w/ me and I will go over it with you.\n",
    "The following is a list of scheduled, committed changes.\n",
    "1.   Launch Norton Antivirus (Start, Programs, Norton Antivirus)\n",
    "Reminder....Town Meeting in Central Perk cafe today at 3:00pm.\n",
    "Any question about transactions please contact Sterling Huxley.\n",
    "I will try to get back to you by Friday, December 31.\n",
    "This Change will be implemented on Friday.\n",
    "Let me know if you have problems accessing this;\n",
    "Sterling Huxley recently added you to the access list.\n",
    "Would you like to go to lunch?\n",
    "The last command will be automatically run.\n",
    "This is too much english for a computer geek.\n",
};

// create a random command to execute
static void crash_dummy()
{
    static int sleeptime; // how long to pause between commands
    char c, cm, *cmd, *cmd1;
    int i, cnt, thing, rbi, startrbi, percent;

    // "dot" movement commands
    cmd1 = " \n\r\002\004\005\006\025\0310^$-+wWeEbBhjklHL";

    // is there already a command running?
    if (chars_to_parse > 0)
        goto cd1;
cd0:
    startrbi = rbi = 0;
    sleeptime = 0; // how long to pause between commands
    memset(readbuffer, '\0', sizeof(readbuffer));
    // generate a command by percentages
    percent = (int)lrand48() % 100; // get a number from 0-99
    if (percent < Mp) {             //  Movement commands
        // available commands
        cmd = cmd1;
        M++;
    } else if (percent < Np) { //  non-movement commands
        cmd = "mz<>\'\"";      // available commands
        N++;
    } else if (percent < Dp) { //  Delete commands
        cmd = "dx";            // available commands
        D++;
    } else if (percent < Ip) { //  Inset commands
        cmd = "iIaAsrJ";       // available commands
        I++;
    } else if (percent < Yp) { //  Yank commands
        cmd = "yY";            // available commands
        Y++;
    } else if (percent < Pp) { //  Put commands
        cmd = "pP";            // available commands
        P++;
    } else {
        // We do not know how to handle this command, try again
        U++;
        goto cd0;
    }
    // randomly pick one of the available cmds from "cmd[]"
    i = (int)lrand48() % strlen(cmd);
    cm = cmd[i];
    if (strchr(":\024", cm))
        goto cd0;           // dont allow colon or ctrl-T commands
    readbuffer[rbi++] = cm; // put cmd into input buffer

    // now we have the command-
    // there are 1, 2, and multi char commands
    // find out which and generate the rest of command as necessary
    if (strchr("dmryz<>\'\"", cm)) { // 2-char commands
        cmd1 = " \n\r0$^-+wWeEbBhjklHL";
        if (cm == 'm' || cm == '\'' || cm == '\"') { // pick a reg[]
            cmd1 = "abcdefghijklmnopqrstuvwxyz";
        }
        thing = (int)lrand48() % strlen(cmd1); // pick a movement command
        c = cmd1[thing];
        readbuffer[rbi++] = c; // add movement to input buffer
    }
    if (strchr("iIaAsc", cm)) { // multi-char commands
        if (cm == 'c') {
            // change some thing
            thing = (int)lrand48() % strlen(cmd1); // pick a movement command
            c = cmd1[thing];
            readbuffer[rbi++] = c; // add movement to input buffer
        }
        thing = (int)lrand48() % 4; // what thing to insert
        cnt = (int)lrand48() % 10;  // how many to insert
        for (i = 0; i < cnt; i++) {
            if (thing == 0) { // insert chars
                readbuffer[rbi++] = chars[((int)lrand48() % strlen(chars))];
            } else if (thing == 1) { // insert words
                strcat(readbuffer, words[(int)lrand48() % 20]);
                strcat(readbuffer, " ");
                sleeptime = 0;       // how fast to type
            } else if (thing == 2) { // insert lines
                strcat(readbuffer, lines[(int)lrand48() % 20]);
                sleeptime = 0; // how fast to type
            } else {           // insert multi-lines
                strcat(readbuffer, multilines[(int)lrand48() % 20]);
                sleeptime = 0; // how fast to type
            }
        }
        strcat(readbuffer, "\033");
    }
    chars_to_parse = strlen(readbuffer);
cd1:
    totalcmds++;
    if (sleeptime > 0)
        awaitInput(sleeptime); // sleep 1/100 sec
}

// test to see if there are any errors
static void crash_test()
{
    static time_t oldtim;

    time_t tim;
    char d[2], msg[80];

    msg[0] = '\0';
    if (end < text) {
        strcat(msg, "end<text ");
    }
    if (end > textend) {
        strcat(msg, "end>textend ");
    }
    if (dot < text) {
        strcat(msg, "dot<text ");
    }
    if (dot > end) {
        strcat(msg, "dot>end ");
    }
    if (screenbegin < text) {
        strcat(msg, "screenbegin<text ");
    }
    if (screenbegin > end - 1) {
        strcat(msg, "screenbegin>end-1 ");
    }

    if (msg[0]) {
        printf("\n\n%d: \'%c\' %s\n\n\n%s[Hit return to continue]%s", totalcmds, last_input_char,
               msg, SOs, SOn);
        fflush(stdout);
        while (safe_read(STDIN_FILENO, d, 1) > 0) {
            if (d[0] == '\n' || d[0] == '\r')
                break;
        }
    }
    tim = time(NULL);
    if (tim >= (oldtim + 3)) {
        sprintf(status_buffer, "Tot=%d: M=%d N=%d I=%d D=%d Y=%d P=%d U=%d size=%d", totalcmds, M,
                N, I, D, Y, P, U, end - text + 1);
        oldtim = tim;
    }
}
#endif
