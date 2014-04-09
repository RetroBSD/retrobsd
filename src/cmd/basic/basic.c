/*
 * MICRO-BASIC:
 *
 * This is a small INTEGER BASIC interpreter that I wrote a number
 * of years ago, and subsequently ported to MICRO-C. While not a great
 * example of coding style (it was a quick and dirty hack job), It is
 * quite instructive, as a simple but fairly complete interpreter.
 *
 * Variables:
 *  260 Numeric variables   : A0-A9 ... Z0-Z9
 *  260 Character variables : A0$-A9$ ... Z0$-Z9$
 *  260 Numeric arrays      : A0()-A9() ... Z0()-Z9()
 *
 *  For convenience the '0' variables can be referenced by letter
 *  only. IE: A is equivalent to A0 ... Z$ is equivalent to Z0$
 *
 * Statements:
 *  BEEP freq,ms            - Generate a BEEP on the PC speaker
 *  CLEAR                   - Erase variables only
 *  CLOSE#n                 - Close file (0-9) opened with OPEN
 *  DATA                    - Enter "inline" data statements
 *  DELAY ms                - Stops for the indicated time
 *  DIM var(size)[, ... ]   - Dimension an array
 *  DOS "command"           - Execute a DOS program
 *  END                     - Terminate program with no message
 *  EXIT                    - Terminate MICRO-BASIC
 *  FOR v=init TO limit [STEP increment] - Perform a counted loop
 *  GOSUB line              - Call a subroutine
 *  GOTO  line              - Jump to line
 *  IF test THEN line       - Conditional goto
 *  IF test THEN statement  - Conditional statement (next statement only)
 *  INPUT var               - Get value for variable
 *  INPUT "prompt",var      - Get value of variable with prompt
 *      prompt must be a constant string, however you can use a char variable
 *      in prompt by concatinating it to such a string: INPUT ""+a$,b$
 *  INPUT#n,var             - Get value for variable from file (0-9)
 *  LET (default)           - variable = expression
 *  LIF test THEN statements- LONG IF (all statements to end of line)
 *  LIST [start,[end]]      - List program lines
 *  LIST#n ...              - List program to file (0-9)
 *  LOAD "name"             - Load program from disk file
 *      When LOAD is used within a program, execution continues with the
 *      first line of the newly loaded program. In this case, the user
 *      variables are NOT cleared. This provides a means of chaining
 *      to a new program, and passing information to it.
 *      Also note that LOAD must be the LAST statement on a line.
 *  NEW                     - Erase program and variables
 *  NEXT [v]                - End counted loop
 *  OPEN#n,"name","opts"    - Open file (0-9), opts are same as "fopen()"
 *  ORDER line              - Position data read pointer
 *  OUT port,expr           - Write to I/O port
 *  PRINT expr[,expr ...]   - Print to console
 *  PRINT#n,...             - Print to file (0-9)
 *  READ var[,var ...]      - Read data from program statements
 *      You MUST issue an "ORDER" statement targeting a line
 *      containing a valid DATA statement before using READ
 *  RETURN                  - Return from subroutine
 *  REM                     - Comment... reminder of line is ignored
 *  RUN [line]              - Run program
 *  SAVE ["name"]           - Save program to disk file
 *  STOP                    - Terminate program & issue message
 *  SRND seed               - Seeds random()
 *
 * Operators:
 *  +                       - Addition, string concatination
 *  -                       - Unary minus, subtraction
 *  *, /, %,                - multiplication, division, modulus
 *  &, |, ^                 - AND, OR, Exclusive OR
 *  =, <>                   - Assign/test equal, test NOTequal (num or string)
 *  <, <=, >, >=            - LT, LE, GT, GE (numbers only)
 *  !                       - Unary NOT
 *      The "test" operators (=, <>, <, <=, >, >=) can be used in any
 *      expression, and evaluate to 1 of the test is TRUE, and 0 if it
 *      is FALSE. The IF and LIF commands accept any non-zero value to
 *      indicate a TRUE condition.
 *
 * Functions:
 *  CHR$(value)         - Returns character of passed value
 *  STR$(value)         - Returns ASCII string of value's digits
 *  ASC(char)           - Returns value of passed character
 *  NUM(string)         - Convert string to number
 *  ABS(value)          - Returns absolute value of argument
 *  RND(value)          - Returns random number from 0 to (value-1)
 *  KEY()               - Test for key from keyboard
 *  INP(port)           - Read an I/O port
 *  SIN(value*10^6)     - 10^6*SIN(x)
 *  COS(value*10^6)     - 10^6*COS(x)
 *  TAN(value*10^6)     - 10^6*TAN(x)
 *  ASIN(value*10^6)    - 10^6*SIN(x)
 *  ACOS(value*10^6)    - 10^6*COS(x)
 *  ATAN(value*10^6)    - 10^6*TAN(x)
 *  LOG(value*10^6)     - 10^6*LOG(x)
 *  LOG10(value*10^6)   - 10^6*LOG10(x)
 *  EXP(value*10^6)     - 10^6*EXP(x)
 *
 * Copyright 1982-2000 Dave Dunfield
 * All rights reserved.
 *
 * Permission granted for personal (non-commercial) use only.
 *
 * Compile command: cc basic -fop
 */
#ifdef CROSS
#   include </usr/include/stdio.h>
#   define ustore(addr, value)  /*empty*/
#   define ufetch(addr)         0
#else
#   include <stdio.h>
#   include <machine/io.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>
#include <curses.h>
#include <signal.h>
#include <math.h>

/* Fixed parameters */
#define BUFFER_SIZE 100 /* input buffer size */
#define NUM_VAR     260 /* number of variables */
#define SA_SIZE     200 /* string accumulator size */

/* Control stack constant identifiers */
#define _FOR        1000    /* indicate FOR statement */
#define _GOSUB      _FOR+1  /* indicate GOSUB statement */

/* Primary keywords */
#define LET         1
#define EXIT        2
#define LIST        3
#define NEW         4
#define RUN         5
#define CLEAR       6
#define GOSUB       7
#define GOTO        8
#define RETURN      9
#define PRINT       10
#define FOR         11
#define NEXT        12
#define IF          13
#define LIF         14
#define REM         15
#define STOP        16
#define END         17
#define INPUT       18
#define OPEN        19
#define CLOSE       20
#define DIM         21
#define ORDER       22
#define READ        23
#define DATA        24
#define SAVE        25
#define LOAD        26
#define DELAY       27
#define BEEP        28
#define DOS         29
#define OUT         30

/* Secondary keywords */
#define TO          31  /* Also used as marker */
#define STEP        32
#define THEN        33

/* Operators and functions */
#define ADD         34  /* Also used as marker */
#define SUB         35
#define MUL         36
#define DIV         37
#define MOD         38
#define AND         39
#define OR          40
#define XOR         41
#define EQ          42
#define NE          43
#define LE          44
#define LT          45
#define GE          46
#define GT          47
#define CHR         48
#define STR         49
#define ASC         50
#define ABS         51
#define NUM         52
#define RND         53
#define KEY         54
#define INP         55
#define DATE        56
#define SRND        57
#define SIN         58
#define COS         59
#define TAN         60
#define ASIN        61
#define ACOS        62
#define ATAN        63
#define LOG         64
#define LOG10       65
#define EXP         66

#define RUN1        99

/* Make sure this token table matches the above definitions */
static char *reserved_words[] = {
    "LET", "EXIT", "LIST", "NEW", "RUN", "CLEAR", "GOSUB", "GOTO",
    "RETURN", "PRINT", "FOR", "NEXT", "IF", "LIF", "REM", "STOP",
    "END", "INPUT", "OPEN", "CLOSE", "DIM", "ORDER", "READ", "DATA",
    "SAVE", "LOAD", "DELAY", "BEEP", "DOS", "OUT",
    "TO", "STEP", "THEN",
    "+", "-", "*", "/", "%", "&", "|", "^",
    "=", "<>", "<=", "<", ">=", ">",
    "CHR$(", "STR$(", "ASC(", "ABS(", "NUM(", "RND(", "KEY(", "INP(",
    "DATE$(", "SRND", "SIN(", "COS(", "TAN(", "ASIN(", "ACOS(", "ATAN(",
    "LOG(", "LOG10(", "EXP(",
    0
};

/* Table of operator priorities */
static char priority[] = { 0, 1, 1, 2, 2, 2, 3, 3, 3, 1, 1, 1, 1, 1, 1 };

/* Table of error messages */
static char *error_messages[] = {
    "Syntax",
    "Illegal program",
    "Illegal direct",
    "Line number",
    "Wrong type",
    "Divide by zero",
    "Nesting",
    "File not open",
    "File already open",
    "Input",
    "Dimension",
    "Data",
    "Out of memory"
    };

struct line_record {
    unsigned Lnumber;
    struct line_record *Llink;
    char Ltext[1];
};

char sa1[SA_SIZE], sa2[SA_SIZE];    /* String accumulators */
struct line_record *pgm_start,      /* Indicates start of program */
    *runptr,                        /* Line we are RUNnning */
    *readptr;                       /* Line we are READing */

unsigned dim_check[NUM_VAR];        /* Check dim sizes for arrays */

FILE *filein, *fileout;             /* File I/O active pointers */

jmp_buf savjmp;                     /* Save area for set/longjmp */

/* Misc. global variables */
char *cmdptr,                       /* Command line parse pointer */
    *dataptr,                       /* Read data pointer */
    buffer[BUFFER_SIZE];            /* General input buffer */
unsigned mode = 0,                  /* 0=Stopped, !0=Running */
    expr_type,                      /* Type of last expression */
    nest;                           /* Nest level of expr. parser */
unsigned line,                      /* Current line number */
    ctl_ptr = 0,                    /* Control stack pointer */
    ctl_stk[50];                    /* Control stack */

/*
 * The following variables must be iniitialized to zero. If your
 * compiler does not automatically zero uninitialized global
 * variables, modify these declarations to initialize them.
 */
char filename[65];                  /* Name of program file */
FILE *files[10];                    /* File unit numbers */
int num_vars[NUM_VAR];              /* Numeric variables */
int *dim_vars[NUM_VAR];             /* Dimensioned arrays */
char *char_vars[NUM_VAR];           /* Character variables */

int eval_sub(void);

WINDOW *win;

void beep (int i, int t)
{
    printf("BEEP not implemented yet, at line %u\n", line);
}

void delay (int msec)
{
    usleep (msec * 1000);
}

int sigint(int sn)
{
    nocbreak();
    echo();
    endwin();
    exit(0);
}

unsigned kbtst ()
{
    unsigned int chr;

    signal(SIGINT,(sig_t)sigint);
    win = initscr();
    cbreak();
    noecho();
    chr = getch();
    nocbreak();
    echo();
    endwin();
    signal(SIGINT,SIG_DFL);
    return chr;
}

void out (int addr, int value)
{
    /* 2BSD system call extension. */
    ustore (addr, value);
}

unsigned in (int addr)
{
    /* 2BSD system call extension. */
    return ufetch (addr);
}

int _sin (int value)
{
    return (int)(1.0e6 * sin(value * 1.0e-6));
}

int _cos (int value)
{
    return (int)(1.0e6 * cos(value * 1.0e-6));
}

int _tan (int value)
{
    return (int)(1.0e6 * tan(value * 1.0e-6));
}

int _asin (int value)
{
    return (int)(1.0e6 * asin(value * 1.0e-6));
}

int _acos (int value)
{
    return (int)(1.0e6 * acos(value * 1.0e-6));
}

int _atan (int value)
{
    return (int)(1.0e6 * atan(value * 1.0e-6));
}

int _log (int value)
{
    return (int)(1.0e6 * log(value * 1.0e-6));
}

int _log10 (int value)
{
    return (int)(1.0e6 * log10(value * 1.0e-6));
}

int _exp (int value)
{
    return (int)(1.0e6 * exp(value * 1.0e-6));
}

int isalnum(int c)
{
    c = (unsigned char) c;
    if (c >= 'a' && c <= 'z')
        return 1;
    if (c >= '0' && c <= '9')
        return 1;
    if (c >= 'A' && c <= 'Z')
        return 1;
    return 0;
}

int isalpha(int c)
{
    c = (unsigned char) c;
    if (c >= 'a' && c <= 'z')
        return 1;
    if (c >= 'A' && c <= 'Z')
        return 1;
    return 0;
}

int isdigit(int c)
{
    c = (unsigned char) c;
    return c >= '0' && c <= '9';
}

int toupper(int c)
{
    c = (unsigned char) c;
    if (c >= 'a' && c <= 'z')
        c += 'A' - 'a';
    return c;
}

void concat (char *ab, char *a, char *b)
{
    strcpy (ab, a);
    strcat (ab, b);
}

/*
 * Test for end of expression
 */
int is_e_end(char c)
{
    if ((c >= (-128+TO)) && (c < (-128+ADD)))
        return(1);
    return (c == '\0') || (c == ':') || (c == ')') || (c == ',');
}

/*
 * Test for end of statement
 */
int is_l_end(char c)
{
    return (c == '\0') || (c == ':');
}

/*
 * Test for terminator character
 */
int isterm(char c)
{
    return (c == ' ') || (c == '\t');
}

/*
 * Advance to next non-blank & retreive data
 */
char skip_blank()
{
    while (isterm(*cmdptr))
        ++cmdptr;
    return *cmdptr;
}

/*
 * Advance to., retrieve and skip next non blank
 */
char get_next()
{
    char c;

    while (isterm(c = *cmdptr))
        ++cmdptr;
    if (c)
        ++cmdptr;
    return c;
}

/*
 * Test for a specific character occuring next & remove if found
 */
int test_next(int token)
{
    if (skip_blank() == token) {
        ++cmdptr;
        return -1;
    }
    return 0;
}

/*
 * Dislay error message
 */
void error(unsigned en)
{
    printf("%s error", error_messages[en]);
    if (mode)
        printf(" in line %u", line);
    putc('\n',stdout);
    longjmp(savjmp, 1);
}

/*
 * Expect a specific token - syntax error if not found
 */
void expect(int token)
{
    if (get_next() != token)
        error(0);
}

/*
 * Lookup up word from command line in table
 */
unsigned lookup(char *table[])
{
    unsigned i;
    char *cptr, *optr;

    optr = cmdptr;
    for (i=0; (cptr = table[i]); ++i) {
        while ((*cptr) && (*cptr == toupper(*cmdptr))) {
            ++cptr;
            ++cmdptr;
        }
        if (! *cptr) {
            if (! (isalnum(*(cptr-1)) && isalnum(*cmdptr)) ) {
                skip_blank();
                return i+1;
            }
        }
        cmdptr = optr;
    }
    return 0;
}

/*
 * Get a number from the input buffer
 */
unsigned get_num()
{
    unsigned value;
    char c;

    value = 0;
    while (isdigit(c = *cmdptr)) {
        ++cmdptr;
        value = (value * 10) + (c - '0');
    }
    return value;
}

/*
 * Allocate memory and zero it
 */
char *allocate(unsigned size)
{
    char *ptr;

    ptr = malloc(size);
    if (! ptr)
        error(12);
    memset(ptr, 0, size);
    return ptr;
}

/*
 * Delete a line from the program
 */
void delete_line(unsigned lino)
{
    struct line_record *cptr, *bptr = 0;

    if (! pgm_start)                    /* no lines in pgm */
        return;
    cptr = pgm_start;
    do {
        if (lino == cptr->Lnumber) {    /* we have line to delete */
            if (cptr == pgm_start) {    /* first line in pgm */
                pgm_start = cptr->Llink;
                return;
            }
            if (bptr)                   /* skip it in linked list */
                bptr->Llink = cptr->Llink;
            free(cptr);                 /* let it go */
        }
        bptr = cptr;
    } while ((cptr = cptr->Llink));
}

/*
 * Insert a line into the program
 */
void insert_line(unsigned lino)
{
    struct line_record *cptr, *bptr, *optr;

    bptr = (struct line_record*)
        allocate(sizeof (struct line_record) + strlen (cmdptr));
    bptr->Lnumber = lino;
    strcpy (bptr->Ltext, cmdptr);
    if (! pgm_start || lino < pgm_start->Lnumber) {
        /* at start */
        bptr->Llink = pgm_start;
        pgm_start = bptr;
        return;
    }
    /* inserting into main part of pgm */
    cptr = pgm_start;
    for (;;) {
        optr = cptr;
        cptr = cptr->Llink;
        if (! cptr || lino < cptr->Lnumber) {
            bptr->Llink = optr->Llink;
            optr->Llink = bptr;
            break;
        }
    }
}

/*
 * Tokenize input line and Add/Replace a source line if suitable
 */
int edit_program()
{
    unsigned value;
    char *ptr, c;

    cmdptr = ptr = buffer;
    /* Translate special tokens into codes */
    while ((c = *cmdptr)) {
        if (c == '\n' || c == '\r') {
            ++cmdptr;
            continue;
        }
        value = lookup(reserved_words);
        if (value)
            *ptr++ = value | 0x80;
        else {
            *ptr++ = c;
            ++cmdptr;
            if (c == '"') {             /* double quote */
                while ((c = *cmdptr) && (c != '"')) {
                    ++cmdptr;
                    *ptr++ = c;
                }
                *ptr++ = *cmdptr++;
            }
        }
    }
    *ptr = 0;
    cmdptr = buffer;

    if (isdigit(skip_blank())) {        /* Valid source line */
        value = get_num();              /* Get line number */
        delete_line(value);             /* Delete the old */
        if (skip_blank())
            insert_line(value);
        return -1;                      /* Insert the new */
    }
    return 0;
}

/*
 * Locate given line in source
 */
struct line_record *find_line(unsigned line)
{
    struct line_record *cptr;

    for (cptr = pgm_start; cptr; cptr = cptr->Llink)
        if (cptr->Lnumber == line)
            return cptr;
    error(3);
    return 0;
}

/*
 * Get index for variable from its name
 */
int get_var()
{
    unsigned index;
    char c;

    if (! isalpha(c = get_next()))
        error(0);
    index = ((c - 'A') & 0x1F) * 10;
    if (isdigit(c = *cmdptr)) {
        index += (c - '0');
        c = *++cmdptr;
    }
    if (c == '$') {
        ++cmdptr;
        expr_type = 1;
    } else
        expr_type = 0;

    return index;
}

/*
 * Evaluate an expression (numeric or character)
 */
int eval()
{
    unsigned value;

    nest = 0;
    value = eval_sub();
    if (nest != 1)
        error(0);
    return value;
}

/*
 * Evaluate number only (no character)
 */
int eval_num()
{
    unsigned value;

    value = eval();
    if (expr_type)
        error(4);
    return value;
}

/*
 * Evaluate character only (no numeric)
 */
void eval_char()
{
    eval();
    if (! expr_type)
        error(4);
}

/*
 * Compute variable address for assignment
 */
unsigned *address()
{
    unsigned i, j, *dptr;

    i = get_var();
    if (expr_type)
        return (unsigned*) &char_vars[i];

    if (! test_next('('))
        return (unsigned*) &num_vars[i];

    /* Array */
    if (expr_type)
        error(0);
    dptr = (unsigned*) dim_vars[i];
    if (! dptr)
        error(10);
    nest = 0;
    j = eval_sub();
    if (j >= dim_check[i])
        error(10);
    return &dptr[j];
}

/*
 * Test for file operator, and set up pointers
 */
int chk_file(char flag)
{
    unsigned i;

    i = -1;
    if (test_next('#')) {
        if (9 < (i = eval_num()))
            error(7);
        test_next(',');
        filein = fileout = files[i];
        if (flag && (!filein))
            error(7);
    } else {
        filein = stdin;
        fileout = stdout;
    }
    return i;
}

/*
 * Display program listing
 */
void disp_pgm(FILE *fp, unsigned i, unsigned j)
{
    unsigned k;
    struct line_record *cptr;
    char c;

    for (cptr = pgm_start; cptr; cptr = cptr->Llink) {
        k = cptr->Lnumber;
        if ((k >= i) && (k <= j)) {
            fprintf(fp,"%u ",k);
            for (k=0; (c = cptr->Ltext[k]); ++k)
                if (c < 0) {
                    c = c & 127;
                    fputs(reserved_words[c - 1], fp);
                    if (c < ADD)
                        putc(' ',fp);
                } else
                    putc(c,fp);
            putc('\n', fp);
        }
    }
}

/*
 * Clear all variables to zero
 */
void clear_vars()
{
    unsigned i;
    char *ptr;

    for (i=0; i < NUM_VAR; ++i) {
        num_vars[i] = 0;
        ptr = char_vars[i];     /* Character variables */
        if (ptr) {
            free(ptr);
            char_vars[i] = 0;
        }
        ptr = (char*) dim_vars[i];  /* Dimensioned arrays */
        if (ptr) {
            free(ptr);
            dim_vars[i] = 0;
        }
    }
}

/*
 * Clear program completely
 */
void clear_pgm()
{
    for (runptr = pgm_start; runptr; runptr = runptr->Llink)
        free(runptr);
    pgm_start = 0;
}

/*
 * Test for program only, and error if interactive
 */
void pgm_only()
{
    if (! mode)
        error(2);
}

/*
 * Test for direct only, and error if running
 */
void direct_only()
{
    if (mode)
        error(1);
}

/*
 * Skip rest of statement
 */
void skip_stmt()
{
    char c;

    while ((c = *cmdptr) && (c != ':')) {
        ++cmdptr;
        if (c == '"') {
            while ((c = *cmdptr) && (c != '"'))
                ++cmdptr;
            if (c)
                ++cmdptr;
        }
    }
}

const char *wday_s[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", 0
};
const char *wday_l[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday", 0
};
const char *month_l[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December",
    0
};
const char *month_s[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
    0
};

void date_string(char *format, char *ptr)
{
    char *f;
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char *fmt = strdup(format);

    for (f=fmt; *f; f++) {
        if (*f == '%') {
            f++;
            switch (*f) {
            case 'd':   // Day of the month, 2 digits, leading 0 - 01-31
                sprintf(ptr,"%02d",tm->tm_mday);
                break;
            case 'D':
                sprintf(ptr,"%s",wday_s[tm->tm_wday]);
                break;
            case 'j':
                sprintf(ptr,"%d",tm->tm_mday);
                break;
            case 'l':
                sprintf(ptr,"%s",wday_l[tm->tm_wday]);
                break;
            case 'N':
                sprintf(ptr,"%d",tm->tm_wday == 0 ? 7 : tm->tm_wday);
                break;
            case 'S':
                switch (tm->tm_mday) {
                case 1:
                case 21:
                case 31:
                    sprintf(ptr,"st");
                    break;
                case 2:
                case 22:
                sprintf(ptr,"nd");
                break;
                case 3:
                case 23:
                    sprintf(ptr,"rd");
                    break;
                default:
                    sprintf(ptr,"th");
                    break;
                }
                break;
            case 'w':
                sprintf(ptr,"%d",tm->tm_wday);
                break;
            case 'z':
                sprintf(ptr,"%d",tm->tm_yday);
                break;
            case 'F':
                sprintf(ptr,"%s",month_l[tm->tm_mon]);
                break;
            case 'm':
                sprintf(ptr,"%02d",tm->tm_mon+1);
                break;
            case 'M':
                sprintf(ptr,"%s",month_s[tm->tm_mon]);
                break;
            case 'n':
                sprintf(ptr,"%d",tm->tm_mon+1);
                break;
            case 't':
                switch (tm->tm_mon) {
                case 0:     sprintf(ptr,"%d",31); break; // Jan
                case 1:     sprintf(ptr,"%d",((tm->tm_year+1900) % 4 == 0) ? 29 : 28); break;    // Feb
                case 2:     sprintf(ptr,"%d",31); break; // Mar
                case 3:     sprintf(ptr,"%d",30); break; // Apr
                case 4:     sprintf(ptr,"%d",31); break; // May
                case 5:     sprintf(ptr,"%d",30); break; // Jun
                case 6:     sprintf(ptr,"%d",31); break; // Jul
                case 7:     sprintf(ptr,"%d",31); break; // Aug
                case 8:     sprintf(ptr,"%d",30); break; // Sep
                case 9:     sprintf(ptr,"%d",31); break; // Oct
                case 10:    sprintf(ptr,"%d",30); break; // Nov
                case 11:    sprintf(ptr,"%d",31); break; // Dec
                }
                break;
            case 'L':
                sprintf(ptr,"%d",((tm->tm_year+1900) % 4 == 0) ? 1 : 0);
                break;
            case 'o':
            case 'Y':
                sprintf(ptr,"%04d",tm->tm_year+1900);
                break;
            case 'y':
                sprintf(ptr,"%02d",(tm->tm_year+1900) % 100);
                break;
            case 'a':
                sprintf(ptr,"%s",(tm->tm_hour<12) ? "am" : "pm");
                break;
            case 'A':
                sprintf(ptr,"%s",(tm->tm_hour<12) ? "AM" : "PM");
                break;
            case 'g':
                sprintf(ptr,"%d",tm->tm_hour % 12);
                break;
            case 'G':
                sprintf(ptr,"%d",tm->tm_hour);
                break;
            case 'h':
                sprintf(ptr,"%02d",tm->tm_hour % 12);
                break;
            case 'H':
                sprintf(ptr,"%02d",tm->tm_hour);
                break;
            case 'i':
                sprintf(ptr,"%02d",tm->tm_min);
                break;
            case 's':
                sprintf(ptr,"%02d",tm->tm_sec);
                break;
            case 'u':
                sprintf(ptr,"000000");
                break;
            case 'e':
                sprintf(ptr,"%s",tm->tm_zone);
                break;
            case 'I':
                sprintf(ptr,"%d",tm->tm_isdst);
                break;
            case 'O':
                sprintf(ptr,"%02d%02d",(int)tm->tm_gmtoff/60,(int)tm->tm_gmtoff%60);
                break;
            case 'P':
                sprintf(ptr,"%02d:%02d",(int)tm->tm_gmtoff/60,(int)tm->tm_gmtoff%60);
                break;
            case 'T':
                sprintf(ptr,"%s",tm->tm_zone);
                break;
            default:
                *ptr++ = *f;
                *ptr = 0;
            }
        } else {
            *ptr++ = *f;
            *ptr = 0;
        }
        while (*ptr)
            ptr++;
    }
    free(fmt);
}

/*
 * Convert a number to a string, and place in memory
 */
void num_string(unsigned value, char *ptr)
{
    char cstack[12];
    int i;

    if (value > INT_MAX) {
        *ptr++ = '-';
        value = -value;
    }
    i = 0;
    do {
        cstack[i++] = (value % 10) + '0';
    } while (value /= 10);

    while (i)
        *ptr++ = cstack[--i];
    *ptr = 0;
}

/*
 * Execute a BASIC commands
 */
struct line_record *execute(char cmd)
{
    unsigned i, j, k, *dptr;
    int ii, jj;
    struct line_record *cptr;
    char c;

    switch (cmd & 0x7F) {
    case LET:
        dptr = address();
        j = expr_type;

        expect(-128+EQ);

        k = eval();

        if (j != expr_type)
            error(0);
        if (! expr_type)                /* numeric assignment */
            *dptr = k;
        else {                          /* character assignment */
            if (*dptr)
                free((void*) *dptr);
            if (*sa1) {
                *dptr = (unsigned) allocate(strlen(sa1)+1);
                strcpy((char*) *dptr, sa1);
            } else
                *dptr = 0;
        }
        break;
    case EXIT:
        exit(0);
    case LIST:
        chk_file(1);
        if (! isdigit(skip_blank())) {
            i=0; j=-1;
        } else {
            i = get_num();
            if (get_next() == ',') {
                if (isdigit(skip_blank()))
                    j=get_num();
                else
                    j = -1;
            } else
                j=i;
        }
        disp_pgm(fileout,i,j);
        break;
    case NEW:
        clear_vars();
        clear_pgm();
        longjmp(savjmp, 1);
    case RUN:
        direct_only();
        clear_vars();
    case RUN1:          /* No clearing */
        if (is_e_end(skip_blank()))
            runptr = pgm_start;
        else
            runptr = find_line(eval_num());
        --mode;         /* indicate running */
newline:
        while (runptr) {
            cmdptr = runptr->Ltext;
            line = runptr->Lnumber;
            do {
                if ((cmd = skip_blank()) < 0) {
                    ++cmdptr;
                    dptr = (unsigned*) execute(cmd);
                    if (dptr) {
                        runptr = (struct line_record*) dptr;
                        goto newline;
                    }
                } else
                    execute(1);
            } while ((c = get_next()) == ':');
            if (c)
                error(0);
            runptr = runptr->Llink;
        }
        mode = 0;
        break;
    case CLEAR:
        clear_vars();
        break;
    case GOSUB:
        ctl_stk[ctl_ptr++] = (unsigned) runptr;
        ctl_stk[ctl_ptr++] = (unsigned) cmdptr;
        ctl_stk[ctl_ptr++] = _GOSUB;
    case GOTO:
        pgm_only();
        return find_line(eval_num());
    case RETURN:
        pgm_only();
        if (ctl_stk[--ctl_ptr] != _GOSUB)
            error(6);
        cmdptr = (char*) ctl_stk[--ctl_ptr];
        runptr = (struct line_record*) ctl_stk[--ctl_ptr];
        line = runptr->Lnumber;
        skip_stmt();
        break;
    case PRINT:
        chk_file(1);
        j = 0;
        do {
            if (is_l_end(skip_blank()))
                --j;
            else {
                i = eval();
                if (! expr_type) {
                    num_string(i, sa1);
                    putc(' ',fileout);
                }
                fputs(sa1, fileout);
            }
        } while (test_next(','));
        if (! j)
            putc('\n', fileout);
        break;
    case FOR:
        pgm_only();
        ii = 1;                         /* default step value */
        i = get_var();
        if (expr_type)
            error(0);
        expect(-128+EQ);
        num_vars[i] = eval();
        if (expr_type)
            error(0);
        expect(-128+TO);
        jj = eval();
        if (test_next(-128+STEP))
            ii = eval();
        skip_stmt();
        ctl_stk[ctl_ptr++] = (unsigned) runptr; /* line */
        ctl_stk[ctl_ptr++] = (unsigned) cmdptr; /* command pointer */
        ctl_stk[ctl_ptr++] = ii;        /* step value */
        ctl_stk[ctl_ptr++] = jj;        /* limit value */
        ctl_stk[ctl_ptr++] = i;         /* variable number */
        ctl_stk[ctl_ptr++] = _FOR;
        break;
    case NEXT:
        pgm_only();
        if (ctl_stk[ctl_ptr-1] != _FOR)
            error(6);
        i = ctl_stk[ctl_ptr-2];
        if (!is_l_end(skip_blank()))
            if (get_var() != i)
                error(6);
        jj = ctl_stk[ctl_ptr-3];        /* get limit */
        ii = ctl_stk[ctl_ptr-4];        /* get step */
        num_vars[i] += ii;
        if ((ii < 0) ? num_vars[i] >= jj : num_vars[i] <= jj) {
            cmdptr = (char*) ctl_stk[ctl_ptr-5];
            runptr = (struct line_record*) ctl_stk[ctl_ptr-6];
            line = runptr->Lnumber;
        } else
            ctl_ptr -= 6;
        break;
    case IF:
        i = eval_num();
        expect(-128+THEN);
        if (i) {
            if (isdigit(cmd = skip_blank()))
                return find_line(eval_num());
            else if (cmd < 0) {
                ++cmdptr;
                return execute(cmd);
            } else
                execute(1);
        } else
            skip_stmt();
        break;
    case LIF:
        i = eval_num();
        expect(-128+THEN);
        if (i) {
            if ((cmd = skip_blank()) < 0) {
                ++cmdptr;
                return execute(cmd);
            } else
                execute(1);
            break;
        }
    case DATA:
        pgm_only();
    case REM:
        if (mode) {
            if ((cptr = runptr->Llink))
                return cptr;
            longjmp(savjmp, 1);
        }
        break;
    case STOP:
        pgm_only();
        printf("STOP in line %u\n",line);
    case END:
        pgm_only();
        longjmp(savjmp, 1);
    case INPUT:
        ii = chk_file(1);
        if (skip_blank() == '"') {      /* special prompt */
            eval();
            expect(',');
        } else
            strcpy(sa1, "? ");
        dptr = address();
        cptr = (struct line_record*) cmdptr;
input:  if (ii == -1)
            fputs(sa1, stdout);
        if (! fgets(buffer, sizeof(buffer)-1, filein))
            exit (0);
        /* Remove trailing eols. */
        cmdptr = buffer + strlen(buffer);
        while (cmdptr > buffer &&
            (cmdptr[-1] == '\n' || cmdptr[-1] == '\r'))
            *--cmdptr = 0;
        cmdptr = buffer;
        if (expr_type) {
            if (*dptr)
                free((void*) *dptr);
            *dptr = (unsigned) allocate(strlen(buffer)+1);
            strcpy((char*) *dptr, buffer);
        } else {
            k = 0;
            if (test_next('-'))
                --k;
            if (! isdigit(*cmdptr)) {
                if (ii != -1)
                    error(9);
                fputs("Input error\n", stdout);
                goto input;
            }
            j = get_num();
            *dptr = (k) ? 0-j: j;
        }
        cmdptr = (char*) cptr;
        break;
    case OPEN:
        if (skip_blank() != '#')
            error(0);
        if (files[i = chk_file(0)])
            error(8);
        eval_char();
        strcpy(buffer, sa1);
        expect(',');
        eval_char();
        files[i] = fopen(buffer,sa1);
        break;
    case CLOSE:
        if ((i = chk_file(1)) == -1)
            error(0);
        if (! filein)
            error(8);
        fclose(files[i]);
        files[i] = 0;
        break;
    case DIM:
        do {
            i = get_var();
            dptr = (unsigned*) dim_vars[i];
            if (dptr)
                free(dptr);
            dim_check[i] = eval_num() + 1;
            dim_vars[i] = (int*) allocate(dim_check[i] * sizeof(int));
        } while (test_next(','));
        break;
    case ORDER:
        readptr = find_line(eval_num());
        dptr = (unsigned*) cmdptr;
        cmdptr = readptr->Ltext;
        if (get_next() != -128+DATA)
            error(11);
        dataptr = cmdptr;
        cmdptr = (char*) dptr;
        break;
    case READ:
        do {
            dptr = address();
            j = expr_type;
            cptr = (struct line_record*) cmdptr;
            cmdptr = dataptr;
            ii = line;
            if (! skip_blank()) {       /* End of line */
                readptr = readptr->Llink;
                cmdptr = readptr->Ltext;
                if (get_next() != -128+DATA)
                    error(11);
            }
            line = readptr->Lnumber;
            k = eval();
            test_next(',');
            dataptr = cmdptr;
            cmdptr = (char*) cptr;
            line = ii;
            if (j != expr_type)
                error(11);
            if (! expr_type)            /* numeric assignment */
                *dptr = k;
            else {                      /* character assignment */
                if (*dptr)
                    free((void*) *dptr);
                if (*sa1) {
                    *dptr = (unsigned) allocate(strlen(sa1)+1);
                    strcpy((char*) *dptr, sa1);
                } else
                    *dptr = 0;
            }
        } while (test_next(','));
        break;
    case DELAY:
        delay(eval_num());
        break;
    case SRND:
        srandom(eval_num());
        break;
    case BEEP:
        i = eval_num();
        expect(',');
        beep(i, eval_num());
        break;
    case DOS:
        eval_char();
        fflush(stdout);
        system(sa1);
        break;
    case OUT:
        i = eval_num();
        expect(',');
        out(i, eval_num());
        break;
    case SAVE:
        direct_only();
        if (skip_blank()) {
            eval_char();
            concat(filename, sa1, "");
        }
        fileout = fopen(filename, "wv");
        if (fileout) {
            disp_pgm(fileout, 0, -1);
            fclose(fileout);
        } else
            perror (filename);
        break;
    case LOAD:
        eval_char();
        concat(filename, sa1, "");
        filein = fopen(filename, "rv");
        if (filein) {
            if (! mode)
                clear_vars();
            clear_pgm();
            while (fgets(buffer, sizeof(buffer)-1, filein))
                edit_program();
            fclose(filein);
            return pgm_start;
        } else
            perror (filename);
        longjmp(savjmp, 1);
    default:            /* unknown */
        error(0);
    }
    return 0;
}

/*
 * Perform an arithmetic operation
 */
int do_arith(char opr, unsigned op1, unsigned op2)
{
    unsigned value;

    switch (opr) {
    case ADD-(ADD-1):       /* addition */
        value = op1 + op2;
        break;
    case SUB-(ADD-1):       /* subtraction */
        value = op1 - op2;
        break;
    case MUL-(ADD-1):       /* multiplication */
        value = op1 * op2;
        break;
    case DIV-(ADD-1):       /* division */
        value = op1 / op2;
        break;
    case MOD-(ADD-1):       /* modulus */
        value = op1 % op2;
        break;
    case AND-(ADD-1):       /* logical and */
        value = op1 & op2;
        break;
    case OR-(ADD-1):        /* logical or */
        value = op1 | op2;
        break;
    case XOR-(ADD-1):       /* exclusive or */
        value = op1 ^ op2;
        break;
    case EQ-(ADD-1):        /* equals */
        value = op1 == op2;
        break;
    case NE-(ADD-1):        /* not-equals */
        value = op1 != op2;
        break;
    case LE-(ADD-1):        /* less than or equal to */
        value = op1 <= op2;
        break;
    case LT-(ADD-1):        /* less than */
        value = op1 < op2;
        break;
    case GE-(ADD-1):        /* greater than or equal to */
        value = op1 >= op2;
        break;
    case GT-(ADD-1):        /* greater than */
        value = op1 > op2;
        break;
    default:
        error(0);
        value = 0;
    }
    return value;
}

/*
 * Get character value
 */
void get_char_value(char *ptr)
{
    unsigned i;
    char c, *st;

    if ((c = get_next()) == '"') {      /* character value */
        while ((c = *cmdptr++) != '"') {
            if (! c)
                error(0);
            *ptr++ = c;
        }
        *ptr = 0;
    } else if (isalpha(c)) {            /* variable */
        --cmdptr;
        i = get_var();
        if (! expr_type)
            error(0);
        st = char_vars[i];
        if (st)
            strcpy(ptr,st);
        else
            strcpy(ptr,"");
    } else if (c == -128+CHR) {         /* Convert number to character */
        *ptr++ = eval_sub();
        if (expr_type)
            error(4);
        *ptr = 0;
    } else if (c == -128+STR) {         /* Convert number to string */
        num_string(eval_sub(), ptr);
        if (expr_type)
            error(4);
    } else if (c == -128+DATE) {        /* Format a date */
        eval_sub();
        if (! expr_type)
            error(4);
        date_string(sa1,ptr);
    } else
        error(0);
    expr_type = 1;
}

/*
 * Get a value element for an expression
 */
int get_value()
{
    unsigned value, v, *dptr;
    char c, *ptr;

    expr_type = 0;
    if (isdigit(c = skip_blank()))
        value = get_num();
    else {
        ++cmdptr;
        switch (c) {
        case '(':                       /* nesting */
            value = eval_sub();
            break;
        case '!':                       /* not */
            value = ~get_value();
            break;
        case -128+SUB:                  /* negate */
            value = -get_value();
            break;
        case -128+ASC:                  /* Convert character to number */
            eval_sub();
            if (! expr_type)
                error(4);
            value = *sa1 & 255;
            expr_type = 0;
            break;
        case -128+NUM:                  /* Convert string to number */
            eval_sub();
            if (! expr_type)
                error(4);
            value = atoi(sa1);
            expr_type = 0;
            break;
        case -128+ABS:                  /* Absolute value */
            if ((value = eval_sub()) > INT_MAX)
                value = -value;
            goto number_only;
        case -128+RND:                  /* Random number */
            value = random() % eval_sub();
            goto number_only;
        case -128+SIN:                  /* SIN */
            value = _sin(eval_sub());
            goto number_only;
        case -128+COS:                  /* COS */
            value = _cos(eval_sub());
            goto number_only;
        case -128+TAN:                  /* TAN */
            value = _tan(eval_sub());
            goto number_only;
        case -128+ASIN:                 /* ASIN */
            value = _asin(eval_sub());
            goto number_only;
        case -128+ACOS:                 /* ACOS */
            value = _acos(eval_sub());
            goto number_only;
        case -128+ATAN:                 /* ATAN */
            value = _atan(eval_sub());
            goto number_only;
        case -128+LOG:                  /* LN */
            value = _log(eval_sub());
            goto number_only;
        case -128+LOG10:                /* LOG */
            value = _log10(eval_sub());
            goto number_only;
        case -128+EXP:                  /* EXP */
            value = _exp(eval_sub());
            goto number_only;
        case -128+KEY:                  /* Keyboard test */
            /* eval_sub(); */
            expect(')');
            value = kbtst();
            break;
        case -128+INP:                  /* Read from port */
            value = in(eval_sub());
number_only:
            if (expr_type)
                error(4);
            break;
        default:                        /* test for character expression */
            --cmdptr;
            if (isalpha(c)) {           /* variable */
                value = get_var();
                if (expr_type) {        /* char */
                    ptr = char_vars[value];
                    if (ptr)
                        strcpy(sa1, ptr);
                    else
                        strcpy(sa1, "");
                } else {
                    if (test_next('(')) { /* Array */
                        dptr = (unsigned*) dim_vars[value];
                        if (! dptr)
                            error(10);
                        if ((v = eval_sub()) >= dim_check[value])
                            error(10);
                        value = dptr[v];
                    } else              /* Std variable */
                        value = num_vars[value];
                }
            } else {
                get_char_value(sa1);
                value = 0;
            }
        }
    }
    return value;
}

/*
 * Evaluate a sub expression
 */
int eval_sub()
{
    unsigned value, nstack[10], nptr, optr;
    int ostack[10], c;

    ++nest;                             /* indicate we went down */

    /* establish first entry on number and operator stacks */
    ostack[optr = nptr = 0] = 0;        /* add zero to init */

    nstack[++nptr] = get_value();       /* get next value */

    /* string operations */
    if (expr_type) {                    /* string operations */
        while (! is_e_end(c = skip_blank())) {
            ++cmdptr;
            c &= 0x7F;
            get_char_value(sa2);
            if (c == ADD)               /* String concatination */
                strcat(sa1, sa2);
            else {
                if (c == EQ)            /* String EQUALS */
                    value = !strcmp(sa1, sa2);
                else if (c == NE)       /* String NOT EQUALS */
                    value = strcmp(sa1, sa2) != 0;
                else {
                    error(0);
                    value = 0;
                }
                nstack[nptr] = value;
                expr_type = 0;
            }
        }

    /* numeric operations */
    } else {
        while (! is_e_end(c = skip_blank())) {
            ++cmdptr;
            c = (c & 0x7F) - (ADD-1);   /* 0 based priority table */
            if (priority[c] <= priority[ostack[optr]]) {
                /* execute operand */
                value = nstack[nptr--];
                nstack[nptr] = do_arith(ostack[optr--], nstack[nptr], value);
            }
            nstack[++nptr] = get_value();   /* stack next operand */
            if (expr_type)
                error(0);
            ostack[++optr] = c;
        }
        while (optr) {                  /* clean up all pending operations */
            value = nstack[nptr--];
            nstack[nptr] = do_arith(ostack[optr--], nstack[nptr], value);
        }
    }
    if (c == ')') {
        --nest;
        ++cmdptr;
    }
    return nstack[nptr];
}

/*
 * Main program
 */
int main(int argc, char *argv[])
{
    int i, j;

    pgm_start = 0;
    srandom (time (0));

    /*
     * If arguments are given, copy them into the A0$, A1$, A2$ ... variables
     */
    j = 0;
    for (i=1; i < argc; ++i)
        char_vars[j++] = strdup (argv[i]);

    /*
     * If a name is given on the command line, load it as a program and
     * run immediately. If the program does not explicitly EXIT, we will
     * then proceed to an interactive session
     */
    if (j) {
        concat(filename, char_vars[0], "");
        filein = fopen(filename, "rv");
        if (filein) {
            while (fgets(buffer, sizeof(buffer)-1, filein))
                edit_program();
            fclose(filein);
            cmdptr = "";
            if (! setjmp(savjmp))
                execute(RUN1);
        } else
            perror (filename);
    }

    /*
     * Display header AFTER running command line, so that programs run as
     * "batch" commands terminating with EXIT do not get "noise" output.
     */
    printf("MICRO-BASIC 2.1 - Copyright 1982-2000 Dave Dunfield.\n");

    setjmp(savjmp);
    for (;;) {                          /* Main interactive loop */
        fputs("Ready\n", stdout);
noprompt:
        mode = 0;
        ctl_ptr = 0;
        if (! fgets(buffer, sizeof(buffer)-1, stdin))
            exit (0);
        if (edit_program())             /* Tokenize & edit if OK */
            goto noprompt;
        i = *cmdptr;
        if (i < 0) {                    /* Keyword, execute command */
            ++cmdptr;
            execute(i);
        } else if (i)                   /* Unknown, assume LET */
            execute(LET);
    }
}
