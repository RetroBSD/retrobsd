/*
 *+ Portable Forth interpreter.
 *+
 *+ Copyright (C) 1990-2012 Serge Vakulenko, <serge@vak.ru>
 *+
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "io.h"
#include "forth.h"

#define REALFUNCTION(Name,Fun) void Name () { real_t x = fpop (); fpush (Fun (x)); }

integer_t sintab [91] = {
    0,174,348,523,697,871,1045,1218,1391,1564,
    1736,1908,2079,2249,2419,2588,2756,2923,3090,3255,
    3420,3583,3746,3907,4067,4226,4383,4539,4694,4848,
    5000,5150,5299,5446,5591,5735,5877,6018,6156,6293,
    6427,6560,6691,6819,6946,7071,7193,7313,7431,7547,
    7660,7771,7880,7986,8090,8191,8290,8386,8480,8571,
    8660,8746,8829,8910,8987,9063,9135,9205,9271,9335,
    9396,9455,9510,9563,9612,9659,9702,9743,9781,9816,
    9848,9876,9902,9925,9945,9961,9975,9986,9993,9998,
    10000,
};

integer_t tantab [91] = {
    0,174,349,524,699,874,1051,1227,1405,1583,
    1763,1943,2125,2308,2493,2679,2867,3057,3249,3443,
    3639,3838,4040,4244,4452,4663,4877,5095,5317,5543,
    5773,6008,6248,6494,6745,7002,7265,7535,7812,8097,
    8390,8692,9004,9325,9656,10000,10355,10723,11106,11503,
    11917,12348,12799,13270,13763,14281,14825,15398,16003,16642,
    17320,18040,18807,19626,20503,21445,22460,23558,24750,26050,
    27474,29042,30776,32708,34874,37320,40107,43314,47046,51445,
    56712,63137,71153,81443,95143,114300,143006,190811,286362,572899,
    999999999,
};

ident_t *defname;

static integer_t int_div (integer_t x, integer_t y)
{
    if (x >= 0) {
        if (y > 0)
            return (x / y);
        return ((x - y - 1) / y);
    }
    if (y < 0)
        return (x / y);
    return (- (-x + y - 1) / y);
}

static integer_t long_div (long x, integer_t y)
{
    if (x >= 0) {
        if (y > 0)
            return (x / y);
        return ((x - y - 1) / y);
    }
    if (y < 0)
        return (x / y);
    return (- (-x + y - 1) / y);
}

static void printnum (int s, integer_t x)
{
    char *fmt;

    if (x < 0) {
        x = -x;
        putc ('-', stdout);
    }
    switch (s) {
    default:
    case 'b': {
        integer_t y;
        int n;

        for (y=0, n=0; x; x>>=1, ++n) {
            y <<= 1;
            if (x & 1)
                y |= 1;
        }
        for (; n; y>>=1, --n)
            putc (y&1 ? '1' : '0', stdout);
        putc (' ', stdout);
        return;
        }
    case 'd': fmt = "%ld "; break;
    case 'o': fmt = "%lo "; break;
    case 'h': fmt = "%lx "; break;
    }
    printf (fmt, x);
}

/*
 *+ (
 *+ Start a comment which is ended by a ")" or newline.
 */
void Fcomment ()
{
    getword (')', 0);
}

/*
 *+ \
 *+ Start a comment to end of line.
 */
void Flcomment ()
{
    getword ('\n', 0);
}

/*
 *+ dup ( x -- x x )
 *+ Duplicate the top item.
 */
void Fdup ()
{
    integer_t x;

    x = pop ();
    push (x);
    push (x);
}

/*
 *+ ?dup ( x -- x x )
 *+ Duplicate the top item if nonzero.
 */
void Fifdup ()
{
    integer_t x;

    x = pop ();
    push (x);
    if (x)
        push (x);
}

/*
 *+ 2dup ( x y -- x y x y )
 *+ Duplicate the top two words.
 */
void F2dup ()
{
    integer_t x, y;

    y = pop ();
    x = pop ();
    push (x);
    push (y);
    push (x);
    push (y);
}

/*
 *+ drop ( x -- )
 *+ Drop the top of stack.
 */
void Fdrop ()
{
    pop ();
}

/*
 *+ 2drop ( x y -- )
 *+ Drop the top two items from the stack.
 */
void F2drop ()
{
    pop ();
    pop ();
}

/*
 *+ over ( x y -- x y x )
 *+ Copy second to top
 */
void Fover ()
{
    integer_t x, y;

    y = pop ();
    x = pop ();
    push (x);
    push (y);
    push (x);
}

/*
 *+ 2over ( x y z w -- x y z w x y )
 *+     Copy second to top
 */
void F2over ()
{
    integer_t x, y, z, w;

    w = pop ();
    z = pop ();
    y = pop ();
    x = pop ();
    push (x);
    push (y);
    push (z);
    push (w);
    push (x);
    push (y);
}

/*
 *+ rot ( x y z -- y z x )
 *+ Move the third element to the top.
 */
void Frot ()
{
    integer_t x, y, z;

    z = pop ();
    y = pop ();
    x = pop ();
    push (y);
    push (z);
    push (x);
}

/*
 *+ 2rot ( x x y y z z -- y y z z x x )
 *+ Move the third element to the top.
 */
void F2rot ()
{
    integer_t x, x2, y, y2, z, z2;

    z2 = pop ();
    z = pop ();
    y2 = pop ();
    y = pop ();
    x2 = pop ();
    x = pop ();
    push (y);
    push (y2);
    push (z);
    push (z2);
    push (x);
    push (x2);
}

/*
 *+ -rot ( y z x -- x y z )
 *+ Move the top element to the third.
 */
void Fmrot ()
{
    integer_t x, y, z;

    x = pop ();
    z = pop ();
    y = pop ();
    push (x);
    push (y);
    push (z);
}

/*
 *+ swap ( x y -- y x )
 *+ Exchange the top two items.
 */
void Fswap ()
{
    integer_t x, y;

    y = pop ();
    x = pop ();
    push (y);
    push (x);
}

/*
 *+ 2swap ( w x y z -- y z w x )
 *+ Swap the top two words with the second-to-the-top two words.
 */
void F2swap ()
{
    integer_t x, y, z, w;

    z = pop ();
    y = pop ();
    x = pop ();
    w = pop ();
    push (y);
    push (z);
    push (w);
    push (x);
}

/*
 *+ pick ( an, an-1, a0, n -- an, an-1, a0, an )
 *+ Get the "n"th word on the opstack (zero-based, starting
 *+ from the word below "n") to the top of stack.
 */
void Fpick ()
{
    stackptr [-1] = stackptr [-2 - stackptr [-1]];
}

/*
 *+ roll ( an, an-1, a0, n -- an-1, a0, an )
 *+ Roll n words on the opstack (zero-based, starting
 *+ from the word below "n").
 */
void Froll ()
{
    integer_t n, *p, an;

    n = pop ();
    an = stackptr [-1-n];
    for (p=stackptr-n; p<stackptr; ++p)
        p[-1] = p[0];
    stackptr [-1] = an;
}

/*
 *+ sempty
 *+ Empty data stack.
 */
void Fsempty ()
{
    stackptr = stack;
}

/*
 *+ /, f/, *, f*, -, f-, +, f+ ( x y -- d )
 *+ Return the result of the applied binary operation to the
 *+ two arguments. Dividing by zero is undefined.
 */
void Fdiv ()
{
    integer_t x, y;

    y = pop ();
    x = pop ();
    push (y ? int_div (x, y) : 0);
}

void Ffdiv ()
{
    real_t x, y;

    y = fpop ();
    x = fpop ();
    fpush (y ? x/y : 0);
}

void Fmul ()
{
    integer_t x, y;

    y = pop ();
    x = pop ();
    push (x * y);
}

void Ffmul ()
{
    real_t x, y;

    y = fpop ();
    x = fpop ();
    fpush (x * y);
}

void Fsub ()
{
    integer_t x, y;

    y = pop ();
    x = pop ();
    push (x - y);
}

void Ffsub ()
{
    real_t x, y;

    y = fpop ();
    x = fpop ();
    fpush (x - y);
}

void Fadd ()
{
    integer_t x, y;

    y = pop ();
    x = pop ();
    push (x + y);
}

void Ffadd ()
{
    real_t x, y;

    y = fpop ();
    x = fpop ();
    fpush (x + y);
}

/*
 *+ mod ( x y -- r )
 *+ Return the remainder of x/y. This is explicitly calculated
 *+ as x-int(x/y)*x.
 */
void Fmod ()
{
    integer_t x, y;

    y = pop ();
    x = pop ();
    push (y ? x - y * int_div (x, y) : 0);
}

/*
 *+ /mod ( x y -- r d )
 *+ Return x/y and the remainder of x/y.
 */
void Fdivmod ()
{
    integer_t x, y, d;

    y = pop ();
    x = pop ();
    if (! y) {
        push (0);
        return;
    }
    d = int_div (x, y);
    push (x - d*y);
    push (d);
}

/*
 *+ abs, fabs ( x -- |x| )
 *+ Change sign of top of stack if it's negative
 */
void Fabs ()
{
    integer_t x;

    x = pop ();
    push (x<0 ? -x : x);
}

void Ffabs ()
{
    real_t x;

    x = fpop ();
    fpush (x<0 ? -x : x);
}

/*
 *+ negate, fnegate ( x -- -x )
 *+ Replace top of stack with its negation.
 */
void Fnegate ()
{
    integer_t x;

    x = pop ();
    push (- x);
}

void Ffnegate ()
{
    real_t x;

    x = fpop ();
    fpush (- x);
}

/*
 *+ 1+, 1-, 2+, 2-, 2*, 2/ ( x -- op(x) )
 *+ Perform unary op.
 */
void F1add ()
{
    integer_t x;

    x = pop ();
    push (x + 1);
}

void F1sub ()
{
    integer_t x;

    x = pop ();
    push (x - 1);
}

void F2add ()
{
    integer_t x;

    x = pop ();
    push (x + 2);
}

void F2sub ()
{
    integer_t x;

    x = pop ();
    push (x - 2);
}

void F2mul ()
{
    integer_t x;

    x = pop ();
    if (x >= 0)
        push (x << 1);
    else
        push (- ((-x) << 1));
}

void F2div ()
{
    integer_t x;

    x = pop ();
    if (x >= 0)
        push (x >> 1);
    else
        push (- ((-x) >> 1));
}

/*
 *+ * / ( x y z -- w )
 *+ Return x*y/z.
 */
void Fmuldiv ()
{
    integer_t x, y, z;

    x = pop ();
    y = pop ();
    z = pop ();
    push (z ? long_div ((long) x*y, z) : 0);
}

/*
 *+ * /mod ( x y z -- r w )
 *+ Return x*y%z, x*y/z.
 */
void Fmdmod ()
{
    integer_t x, y, z, d, r;
    long m;

    x = pop ();
    y = pop ();
    z = pop ();
    if (! z) {
        push (0);
        return;
    }
    m = (long) x * y;
    d = long_div (m, z);
    r = m - (long) d * z;
    push (r);
    push (d);
}

/*
 *+ true, false ( -- b )
 *+ Push the boolean true and false values onto the stack. These
 *+ values are used uniformly by all of forth.
 */
void Ftrue ()
{
    push (~0);
}

void Ffalse ()
{
    push (0);
}

/*
 *+ or, and, xor, not
 *+ Bitwise OR and AND operations. These will work with "true"
 *+ and "false" to provide logical functionality.
 */
void For ()
{
    integer_t x, y;

    y = pop ();
    x = pop ();
    push (x | y);
}

void Fand ()
{
    integer_t x, y;

    y = pop ();
    x = pop ();
    push (x & y);
}

void Fxor ()
{
    integer_t x, y;

    y = pop ();
    x = pop ();
    push (x ^ y);
}

void Fnot ()
{
    integer_t x;

    x = pop ();
    push (~ x);
}

/*
 *+ =, f= ( x y -- b )
 *+ Return whether x is equal to y.
 */
void Feq ()
{
    integer_t x, y;

    y = pop ();
    x = pop ();
    push (x == y ? ~0 : 0);
}

void Ffeq ()
{
    real_t x, y;

    y = pop ();
    x = pop ();
    push (x == y ? ~0 : 0);
}

/*
 *+ >, f> ( x y -- b )
 *+ Return whether x is greater than y.
 */
void Fgt ()
{
    integer_t x, y;

    y = pop ();
    x = pop ();
    push (x > y ? ~0 : 0);
}

void Ffgt ()
{
    real_t x, y;

    y = pop ();
    x = pop ();
    push (x > y ? ~0 : 0);
}

/*
 *+ <, f< ( x y -- b )
 *+ Return whether x is less than y.
 */
void Flt ()
{
    integer_t x, y;

    y = pop ();
    x = pop ();
    push (x < y ? ~0 : 0);
}

void Fflt ()
{
    real_t x, y;

    y = pop ();
    x = pop ();
    push (x < y ? ~0 : 0);
}

/*
 *+ u< ( x y -- b )
 *+ Return whether unsigned x is less than unsigned y.
 */
void Fult ()
{
    uinteger_t x, y;

    y = pop ();
    x = pop ();
    push (x < y ? ~0 : 0);
}

/*
 *+ max, fmax ( x y -- max(x,y) )
 *+ Take the greater of the top two elements
 */
void Fmax ()
{
    integer_t x, y;

    y = pop ();
    x = pop ();
    push (x>y ? x : y);
}

void Ffmax ()
{
    real_t x, y;

    y = fpop ();
    x = fpop ();
    fpush (x>y ? x : y);
}

/*
 *+ min, fmin (x y -- min(x,y) )
 *+ Take the lesser of the top two elements
 */
void Fmin ()
{
    integer_t x, y;

    y = pop ();
    x = pop ();
    push (x<y ? x : y);
}

void Ffmin ()
{
    real_t x, y;

    y = fpop ();
    x = fpop ();
    fpush (x<y ? x : y);
}

/*
 *+ i->f ( i -- f )
 *+ Convert the integer_t "i" to the equivalent floating format "f".
 */
void Fitof ()
{
    integer_t x;

    x = pop ();
    fpush ((real_t) x);
}

/*
 *+ f->i ( f -- i )
 *+ Convert the floating number "f" to the equivalent integer_t "i".
 *+ Integer portions of "f" will be truncated; for details, refer to the
 *+ "cvtfl" instruction in the VAX architecture handbook.
 */
void Fftoi ()
{
    real_t x;

    x = fpop ();
    push ((integer_t) x);
}

/*
 *+ f. ( f -- )
 *+ Print the floating-point number.
 */
void Ffprint ()
{
    real_t x;

    x = fpop ();
    printf ("%g ", (double) x);
}

/*
 *+ . ( i -- )
 *+ Print the integer_t.
 */
void Fprint ()
{
    integer_t x;
    int s;

    x = pop ();
    switch (base) {
    default:
    case DECIMAL:   s = 'd'; break;
    case OCTAL:     s = 'o'; break;
    case HEX:       s = 'h'; break;
    case BINARY:    s = 'b'; break;
    }
    printnum (s, x);
}

/*
 *+ .s ( -- )
 *+ Print the stack.
 */
void Fsprint ()
{
    integer_t *p;
    int s;

    switch (base) {
    default:
    case DECIMAL:   s = 'd'; break;
    case OCTAL:     s = 'o'; break;
    case HEX:       s = 'h'; break;
    case BINARY:    s = 'b'; break;
    }
    for (p=stack; p<stackptr; ++p)
        printnum (s, *p);
}

/*
 *+ halt
 *+ Exit back to OS.
 */
void Fhalt ()
{
    exit (0);
}

/*
 *+ quit
 *+ Start interpreting from the keyboard again,
 *+ don't clean the data stack.
 */
void Fquit ()
{
    forthresume (0);
}

/*
 *+ :
 *+ Start compilation mode for the next word in the stream.
 */
void Fdefine ()
{
    char *p;

    if (! (p = getword (' ', 0)))
        error ("bad definition");
    defname = enter (p);
    compilation = 1;
}

/*
 *+ ;
 *+ End compilation mode, unsmudge the entry.
 */
void Fenddef ()
{
    value_t *w, *x;

    w = (value_t*) malloc (sizeof (value_t) * (defptr - defbuf + 1));
    if (! w)
        error ("out of memory");
    defname->type = FSUBR;
    defname->val.v = w;
    defname->immed = 0;
    for (x=defbuf; x<defptr; ++x)
        *w++ = *x;
    w->i = 0;
    defptr = defbuf;
    ++dictptr;
    compilation = 0;
}

/*
 *+ immediate
 *+ Set 'immediate' mode for last compiled word
 */
void Fimmediate ()
{
    if (defname)
        defname->immed = 1;
}

/*
 *+ constant
 *+ fconstant
 *+ Like variable, but later references to this word return the
 *+ numerical constant. Thus
 *+         42 constant ascii_star
 *+         ascii_star emit
 *+ will print a star to the current output device.
 */
void Fconstant ()
{
    char *p;
    ident_t *i;

    if (! (p = getword (' ', 0)))
        error ("bad constant name");
    i = enter (p);
    ++dictptr;
    i->type = FGENINT;
    i->val.i = pop ();
    i->immed = 0;
}

void Ffconstant ()
{
    char *p;
    ident_t *i;

    if (! (p = getword (' ', 0)))
        error ("bad fconstant name");
    i = enter (p);
    ++dictptr;
    i->type = FGENINT;
    i->val.r = fpop ();
    i->immed = 0;
}

/*
 *+ variable
 *+ Take the next word and add it to the dictionary
 *+ as a variable. Subsequent references to this name
 *+ will return an address which is the word allocated
 *+ to this variable. Uses such as
 *+         variable foobar 400 allot
 *+ will make "foobar" return the address of a 404-byte array
 *+ (the initially allocated longword, 4 bytes, plus
 *+ the allot'ed 400 bytes).
 */
void Fvariable ()
{
    char *p;
    ident_t *i;

    if (! (p = getword (' ', 0)))
        error ("bad variable name");
    i = enter (p);
    ++dictptr;
    i->type = FGENINT;
    i->val.i = alloc ((integer_t) 1);
    i->immed = 0;
}

/*
 *+ create
 *+ Take the next word and add it to the dictionary
 *+ as a variable. Subsequent references to this name
 *+ will return current value of 'here'.
 */
void Fcreate ()
{
    char *p;
    ident_t *i;

    if (! (p = getword (' ', 0)))
        error ("bad create");
    i = enter (p);
    ++dictptr;
    i->type = FGENINT;
    i->val.i = here;
    i->immed = 0;
}

/*
 *+ forget
 *+ Take the next word and forget all words, defined later
 *+ than given one.  Depth is limited by 'freezedict'.
 */
void Fforget ()
{
    char *p;
    ident_t *i, *w;

    if (! (p = getword (' ', 0)))
        error ("bad forget");
    if (! (i = find (p)))
        return;
    if (i < lowdictptr)
        i = lowdictptr;
    for (w=dictptr-1; w>=i; --w)
        if (w->type == FSUBR)
            free ((char*) w->val.v);
    dictptr = i;
}

/*
 *+ empty
 *+ Forget all words, defined after 'freezedict'.
 */
void Fempty ()
{
    ident_t *w;

    for (w=dictptr-1; w>=lowdictptr; --w)
        if (w->type == FSUBR)
            free ((char*) w->val.v);
    dictptr = lowdictptr;
}

/*
 *+ freezedict
 *+ Update low margin of dictionary.
 *+ All words, defined up to moment, would not be
 *+ destroyed by 'forget'.
 */
void Ffreezedict ()
{
    lowdictptr = dictptr;
}

/*
 *+ here ( -- a )
 *+ Push the address of the next open memory location in the
 *+     dictionary to stack.
 */
void Fhere ()
{
    push (here);
}

/*
 *+ allot ( d -- )
 *+ Add "d" to HERE, effectively moving the bottom of the dictionary
 *+ forward "d" bytes.
 */
void Fallot ()
{
    integer_t x;

    x = pop ();
    allot (x);
}

/*
 *+ alloc ( d -- )
 *+ Alloc d words.
 */
void Falloc ()
{
    integer_t x;

    x = pop ();
    alloc (x);
}

/*
 *+ align ( -- )
 *+ Align 'here' on word boundary.
 */
void Falign ()
{
    here = (here + sizeof (value_t) - 1) / sizeof (value_t) * sizeof (value_t);
}

/*
 *+ @ ( a -- x )
 *+ Fetch a word at address "a".
 */
void Ffetch ()
{
    integer_t x;

    x = pop ();
    push (FETCHWORD(x).i);
}

/*
 *+ ! ( x a -- )
 *+ Store a word at address "a".
 */
void Fstore ()
{
    integer_t a, x;

    a = pop ();
    x = pop ();
    if (a<lowmem || a>=MEMSZ)
        error ("bad store %ld", a);
    STOREWORD (a, itov (x));
}

/*
 *+ c@ ( a -- d)
 *+ Fetch the byte quantity "d" from byte address "a".
 */
void Fcfetch ()
{
    integer_t x;

    x = pop ();
    push (FETCHBYTE (x));
}

/*
 *+ c! ( d a -- )
 *+ Store the byte quantity "d" at byte address "a".
 */
void Fcstore ()
{
    integer_t a, x;

    a = pop ();
    x = pop ();
    if (a<lowmem || a>=MEMSZ)
        error ("bad cstore %ld", a);
    STOREBYTE (a, x);
}

/*
 *+ fill ( a n d -- )
 *+ Fill "n" bytes of memory starting at "a" with the value "d".
 */
void Ffill ()
{
    integer_t a, n, d;

    d = pop ();
    n = pop ();
    a = pop ();
    if (n <= 0)
        return;
    if (a<lowmem || a+n>MEMSZ)
        error ("bad fill %ld %ld %ld", a, n, d);
    while (--n >= 0)
        STOREBYTE (a++, d);
}

/*
 *+ type ( a n -- )
 *+ Type string on stdout.
 */
void Ftype ()
{
    integer_t a, n;
    int c;

    n = pop ();
    a = pop ();
    if (n <= 0)
        return;
    if (a<0 || a+n>MEMSZ)
        error ("bad type %ld %ld", a, n);
    for (; --n>=0; ++a) {
        if (! (c = FETCHBYTE (a)))
            break;
        putc (c, stdout);
    }
}

/*
 *+ expect ( a n -- )
 *+ Read string from stdin.
 */
void Fexpect ()
{
    integer_t a, n;
    int c, s;

    n = pop ();
    a = pop ();
    if (n <= 0)
        return;
    if (a<lowmem || a+n>MEMSZ)
        error ("bad expect %ld %ld", a, n);
    for (s=0; --n>=0; ++s, ++a) {
        c = getc (stdin);
        if (c < 0 || c=='\n')
            break;
        STOREBYTE (a, c);
    }
    STOREBYTE (a, 0);
    span = s;
}

/*
 *+ , ( d -- )
 *+ Move the word "d" into the next open dictionary word,
 *+ advancing HERE.
 */
void Fcomma ()
{
    integer_t x;

    x = pop ();
    STOREWORD (alloc ((integer_t) 1), itov (x));
}

/*
 *+ c, ( d -- )
 *+ As ",", but only a byte operation is done.
 */
void Fccomma ()
{
    integer_t x;

    x = pop ();
    STOREBYTE (allot ((integer_t) 1), x);
}

static void prword (ident_t *w)
{
    switch (w->type) {
    case FGENINT:
        printf ("%s\tint\t%ld\n", w->name, w->val.i);
        break;
    case FGENFLT:
        printf ("%s\tfloat\t%g\n", w->name, (double) w->val.r);
        break;
    case FSUBR:
        printf ("%s\tfunc\t%lx\n", w->name, (long) w->val.v);
        break;
    }
}

/*
 *+ allwords
 *+ List all user defined words.
 */
void Fallwords ()
{
    ident_t *w;

    for (w=dictptr-1; w>=dict; --w)
        prword (w);
}

/*
 *+ words
 *+ List words, defined after 'freezedict'.
 */
void Fwords ()
{
    ident_t *w;

    for (w=dictptr-1; w>=lowdictptr; --w)
        prword (w);
}

/*
 *+ list
 *+ Take the next word in the input stream as a name of word.
 *+ If this word is user defined, print it's definition.
 */
void Flist ()
{
    ident_t *w;
    char *p;

    if (! (p = getword (' ', 0)))
        error ("bad list");
    w = find (p);
    if (!w || w->type!=FSUBR)
        return;
    printf (": %s ", w->name);
    decompile (w->val.v);
    printf (";");
    if (w->immed)
        printf (" immediate");
    printf ("\n");
}

/*
 *+ fsqrt ( f -- s )
 *+ Compute square root of f, both f and s are float.
 *+ flog ( f -- s )
 *+ Compute logarithm of f.
 *+ fexp ( f -- s )
 *+ Compute e to the f power.
 */
REALFUNCTION (Ffsqrt, sqrt)
REALFUNCTION (Fflog, log)
REALFUNCTION (Ffexp, exp)

/*
 *+ sin ( i -- s )
 *+ "i" is a degree measure; "s" is sin(i)*10000.
 *+ fsin ( f -- s )
 *+ "f" is the radian measure; "s" is the sin() value.
 *+ cos, fcos
 *+ As sin, fsin, but for cos() values.
 *+ tan, ftan
 *+ As sin, fsin, but for tan() values.
 */
static integer_t isin (integer_t x)
{
    int neg = 0;

    if (x < 0) {
        x = -x;
        neg ^= 1;
    }
    if (x >= 360)
        x %= 360;
    if (x > 180) {
        x = 360 - x;
        neg ^= 1;
    }
    if (x > 90)
        x = 180 - x;
    x = sintab [x];
    return (neg ? -x : x);
}

static integer_t itan (integer_t x)
{
    int neg = 0;

    if (x < 0) {
        x = -x;
        neg ^= 1;
    }
    if (x >= 180)
        x %= 180;
    if (x > 90) {
        x = 180 - x;
        neg ^= 1;
    }
    x = tantab [x];
    return (neg ? -x : x);
}

void Fsin () { integer_t x = pop (); push (isin (x)); }
void Fcos () { integer_t x = pop (); push (isin (90 - x)); }
void Ftan () { integer_t x = pop (); push (itan (x)); }

REALFUNCTION (Ffsin, sin)
REALFUNCTION (Ffcos, cos)
REALFUNCTION (Fftan, tan)

/*
 *+ fasin ( s -- f )
 *+ Compute asin(s) in radians (float).
 *+ Return value is in range -pi/2..pi/2.
 *+ facos
 *+ As fasin, but for acos() values.
 *+ Return value is in range 0..pi.
 *+ fatan
 *+ As fasin, but for atan() values.
 *+ Return value is in range -pi/2..pi/2.
 */
REALFUNCTION (Ffasin, asin)
REALFUNCTION (Ffacos, acos)
REALFUNCTION (Ffatan, atan)

/*
 *+ fsinh ( f -- s )
 *+ Compute hyperbolic sine function.
 *+ fcosh
 *+ As fsinh, but for cosh() values.
 *+ ftanh
 *+ As fsinh, but for tanh() values.
 */
REALFUNCTION (Ffsinh, sinh)
REALFUNCTION (Ffcosh, cosh)
REALFUNCTION (Fftanh, tanh)

/*
 *+ key ( -- c )
 *+ Read character from input stream.
 *+ All characters codes are non-negative, -1 means EOF.
 */
void Fkey ()
{
    push (getc (stdin));
}

/*
 *+ emit ( c -- )
 *+ Print the specified character to the current output unit.
 */
void Femit ()
{
    integer_t x;

    x = pop ();
    putc ((int) x, stdout);
}

/*
 *+ cr
 *+ Print a newline sequence to the current output unit.
 */
void Fcr ()
{
    putc ('\n', stdout);
}

/*
 *+ space
 *+ Print a space to the current output unit.
 */
void Fspace ()
{
    putc (' ', stdout);
}

/*
 *+ spaces ( n -- )
 *+ Print n spaces to the current output unit.
 */
void Fspaces ()
{
    integer_t n;

    n = pop ();
    while (--n >= 0)
        putc (' ', stdout);
}

/*
 *+ outpop
 *+ Close the current output file & start using the previous output
 *+ file. This is a no-op if this is the first output file.
 */
void Foutpop ()
{
    if (outptr <= 0)
        return;
    fclose (stdout);
    fdopen (dup (outstack [--outptr]), "a");
    close (outstack [outptr]);
}

/*
 *+ output
 *+ Take the next word in the input stream & try to open it
 *+ for writing. If you can't, call "abort". Otherwise, make
 *+ it the current output file, pushing the current output
 *+ onto a stack so that a later "outpop" will close
 *+ this file & continue with the old one.
 */
void Foutput ()
{
    char *p;

    if (! (p = getword (' ', 0)))
        error ("bad output");
    if (outptr >= OUTSZ)
        error ("output stack overflow");
    outstack [outptr++] = dup (fileno (stdout));
    fclose (stdout);
    if (! fopen (p, "a"))
        error ("cannot write to %s", p);
}

/*
 *+ input
 *+ As output, but open for reading. There is no corresponding
 *+ "inpop", as EOF status will cause the equivalent action.
 */
void Finput ()
{
    char *p;
    FILE *fd;

    if (! (p = getword (' ', 0)))
        error ("bad input");
    if (inptr >= OUTSZ)
        error ("input stack overflow");
    fd = fopen (p, "r");
    if (! fd)
        error ("cannot read %s", p);
    instack [inptr++] = dup (fileno (stdin));
    fclose (stdin);
    fdopen (dup (fileno (fd)), "r");
    fclose (fd);
    tty = isatty (fileno (stdin));
    boln = 1;
}

/*
 *+ count ( a -- a n )
 *+ Count characters in string a.
 */
void Fcount ()
{
    integer_t a;
    int c;

    a = pop ();
    push (a);
    c = 0;
    while (FETCHBYTE (a++))
        ++c;
    push (c);
}

/*
 *+ word ( c -- a )
 *+ Input word to delimiter c, placing it 'here'. Return address
 *+ of 'here'.
 */
void Fword ()
{
    char *p;
    integer_t c, a;

    c = pop ();
    if (! (p = getword ((int) c, 0)))
        error ("bad word");
    if (here + strlen (p) + 1 > MEMSZ)
        error ("too long word");
    a = here;
    while (*p)
        STOREBYTE (a++, *p++);
    STOREBYTE (a, 0);
    push (here);
}

/*
 *+ <#
 *+ Begin format processing, set 'hld' to 'pad'.
 */
void Ffmtbeg ()
{
    STOREBYTE (hld = pad, 0);
}

/*
 *+ hold ( c -- )
 *+ Add character to pad.
 */
void Fhold ()
{
    integer_t x;

    x = pop ();
    STOREBYTE (--hld, x);
}

/*
 *+ # ( x -- x/base )
 *+ Add remainder as a character to format string.
 */
void Ffmt ()
{
    integer_t x;

    x = pop ();
    push (x / base);
    x %= base;
    STOREBYTE (--hld, x<=9 ? x+'0' : x-10+'a');
}

/*
 *+ #s ( x -- 0 )
 *+ Add ascii representation of unsigned value
 *+ to format string. If value is zero, add '0'.
 */
void Fsfmt ()
{
    unsigned long x;
    int d;

    x = pop ();
    do {
        d = x % base;
        STOREBYTE (--hld, d<=9 ? d+'0' : d-10+'a');
        x /= base;
    } while (x);
    push (x);
}

/*
 *+ sign ( x -- )
 *+ Add minus to format string if value is negative.
 */
void Fsign ()
{
    integer_t x;

    x = pop ();
    if (x < 0)
        STOREBYTE (--hld, '-');
}

/*
 *+ #> ( x -- a n )
 *+ Close format processing, return address and length.
 */
void Ffmtend ()
{
    pop ();
    push (hld);
    push (MEMSZ-1 - hld);
}

/*
 *+ .(
 *+ Print the string immediately (in interpretive mode) or compile
 *+ code which will print the string (in compilation mode).
 */
void Fcstring ()
{
    char *p;
    int i;

    if (! (p = getword (')', 0)))
        error ("bad .(");
    if (compilation) {
        (defptr++)->i = FCSTRING;
        (defptr++)->i = i = strlen (p);
        strcpy ((char*) defptr, p);
        defptr += i / sizeof (value_t) + 1;
    } else
        for (; *p; ++p)
            putc (*p, stdout);
}

/*
 *+ ."
 *+ Print the string immediately (in interpretive mode) or compile
 *+ code which will print the string (in compilation mode).
 */
void Fstring ()
{
    char *p;
    int i;

    if (! (p = getword ('"', 0)))
        error ("bad .\"");
    if (compilation) {
        (defptr++)->i = FSTRING;
        (defptr++)->i = i = strlen (p);
        strcpy ((char*) defptr, p);
        defptr += i / sizeof (value_t) + 1;
    } else
        for (; *p; ++p)
            putc (*p, stdout);
}

/*
 *+ "
 *+ Place string in data area. 'Here' will point on it.
 */
void Fquote ()
{
    char *p;
    int i;

    p = getword ('"', 0);
    if (! p)
        error ("bad \"");
    if (compilation) {
        i = strlen (p);
        (defptr++)->i = FQUOTE;
        (defptr++)->i = strlen (p);
        strcpy ((char*) defptr, p);
        defptr += i / sizeof (value_t) + 1;
    } else {
        i = here;
        push (i);
        while (*p)
            STOREBYTE (i++, *p++);
        STOREBYTE (i, 0);
    }
}

/*
 *+ ascii ( -- c )
 *+ Get the next word and push in stack the ascii value
 *+ of the first character.
 */
void Fcquote ()
{
    char *p;

    if (! (p = getword (' ', 0)))
        error ("bad ascii");
    if (compilation) {
        (defptr++)->i = FGENINT;
        (defptr++)->i = *p & 0377;
    } else
        push (*p & 0377);
}

/*
 *+ ncompile ( n -- )
 *+ Get the integer_t value from stack and compile the code
 *+ generating that value.
 */
void Fncompile ()
{
    integer_t x;

    x = pop ();
    (defptr++)->i = FGENINT;
    (defptr++)->i = x;
}

/*
 *+ fcompile ( f -- )
 *+ Get the float value from stack and compile the code
 *+ generating that value.
 */
void Ffcompile ()
{
    real_t x;

    x = fpop ();
    (defptr++)->i = FGENFLT;
    (defptr++)->r = x;
}

/*
 *+ compile xxx ( -- )
 *+ Compile the code for executing given symbol.
 */
void Fcompile ()
{
    if (execcnt<0 || !execptr)
badcomp:    error ("bad compile");
    switch (execptr->i) {
    default:
        goto badcomp;
    case FGENINT:
    case FGENFLT:
    case FSUBR:
    case FHARDWARE:
        break;
    }
    *defptr++ = *execptr++;
    *defptr++ = *execptr++;
}

/*
 *+ execute ( i -- )
 *+ Get the index of symbol from stack and execute it.
 */
void Fexecute ()
{
    int x;

    x = pop ();
    sexecute (dict + x);
}

/*
 *+ latest ( -- i )
 *+ Push in stack the index of current symbol under definition.
 */
void Flatest ()
{
    integer_t x;

    x = defname - dict;
    push (x);
}

/*
 *+ find ( a -- i )
 *+ Stack contains the address of string with the name of symbol.
 *+ Find this symbol in dictionary and return it's index.
 */
void Ffind ()
{
    integer_t x;
    ident_t *w;

    x = pop ();
    if (x<0 || x>here)
        error ("bad find address");
    w = find (memory.c + x);
    if (! w)
        error ("bad find");
    x = w - dict;
    push (x);
}

/*
 *+ base
 *+ A variable which holds the current base.
 *+ bl
 *+ A constant which holds the code for blank character.
 *+ hld
 *+ A variable which holds the address of pad.
 *+ pad
 *+ A constant which holds the end address of memory.
 *+ state
 *+ A variable which holds the current state; 0 = interpreting,
 *+ non-0 means compiling.
 *+ if ... [ else ] ... endif
 *+ The conditional structure. Note "endif", not "then".
 *+ 'endif' is equivalent to 'then'.
 *+ begin ... again
 *+ Unconditional looping structure.
 *+ 'again' is equivalent to 'repeat'.
 *+ begin ... until
 *+ Conditional looping--will loop until the "until" receives a
 *+ boolean "true" on the stack.
 *+ begin ... while ... repeat
 *+ Looping structure where the test is at the "while" word.
 *+ do ... loop
 *+ Counting loop.
 *+ do ... +loop
 *+ As do...loop, but +loop takes the amount to increment by.
 *+ ?do
 *+ The same as do, but does not execute loop if upper and lower
 *+ bounds are aqual.
 *+ leave
 *+ Causes the innermost loop to reach its exit condition. The
 *+ next execution of "loop" or "+loop" will fall through.
 *+ i, j, k
 *+ The loop indices of (respectively) the innermost, second, and
 *+ third loops.
 *+ myself
 *+ recurse
 *+ Compile address of function under definition.
 *+ Results in calling function by itself.
 *+ For example,
 *+     : hihi ." Hi! " recurse ;
 *+ calls itself indefinitely.
 */
struct table func_table [] = {
    { "!",          Fstore,     FHARDWARE,  0 },
    { "#",          Ffmt,       FHARDWARE,  0 },
    { "#>",         Ffmtend,    FHARDWARE,  0 },
    { "#s",         Fsfmt,      FHARDWARE,  0 },
    { "(",          Fcomment,   FHARDWARE,  1 },
    { "*",          Fmul,       FHARDWARE,  0 },
    { "*/",         Fmuldiv,    FHARDWARE,  0 },
    { "*/mod",      Fmdmod,     FHARDWARE,  0 },
    { "+",          Fadd,       FHARDWARE,  0 },
    { "+loop",      0,          FPLOOP,     0 },
    { ",",          Fcomma,     FHARDWARE,  0 },
    { "-",          Fsub,       FHARDWARE,  0 },
    { "-rot",       Fmrot,      FHARDWARE,  0 },
    { ".",          Fprint,     FHARDWARE,  0 },
    { ".(",         Fcstring,   FHARDWARE,  1 },
    { ".\"",        Fstring,    FHARDWARE,  1 },
    { ".s",         Fsprint,    FHARDWARE,  0 },
    { "/",          Fdiv,       FHARDWARE,  0 },
    { "/mod",       Fdivmod,    FHARDWARE,  0 },
    { "1+",         F1add,      FHARDWARE,  0 },
    { "1-",         F1sub,      FHARDWARE,  0 },
    { "2*",         F2mul,      FHARDWARE,  0 },
    { "2+",         F2add,      FHARDWARE,  0 },
    { "2-",         F2sub,      FHARDWARE,  0 },
    { "2/",         F2div,      FHARDWARE,  0 },
    { "2drop",      F2drop,     FHARDWARE,  0 },
    { "2dup",       F2dup,      FHARDWARE,  0 },
    { "2over",      F2over,     FHARDWARE,  0 },
    { "2rot",       F2rot,      FHARDWARE,  0 },
    { "2swap",      F2swap,     FHARDWARE,  0 },
    { ":",          Fdefine,    FHARDWARE,  0 },
    { ";",          Fenddef,    FHARDWARE,  1 },
    { "<",          Flt,        FHARDWARE,  0 },
    { "<#",         Ffmtbeg,    FHARDWARE,  0 },
    { "=",          Feq,        FHARDWARE,  0 },
    { ">",          Fgt,        FHARDWARE,  0 },
    { "?do",        0,          FIFDO,      0 },
    { "?dup",       Fifdup,     FHARDWARE,  0 },
    { "@",          Ffetch,     FHARDWARE,  0 },
    { "\"",         Fquote,     FHARDWARE,  1 },
    { "\\",         Flcomment,  FHARDWARE,  0 },
    { "abs",        Fabs,       FHARDWARE,  0 },
    { "again",      0,          FREPEAT,    0 },
    { "align",      Falign,     FHARDWARE,  0 },
    { "alloc",      Falloc,     FHARDWARE,  0 },
    { "allot",      Fallot,     FHARDWARE,  0 },
    { "allwords",   Fallwords,  FHARDWARE,  0 },
    { "and",        Fand,       FHARDWARE,  0 },
    { "ascii",      Fcquote,    FHARDWARE,  1 },
    { "begin",      0,          FBEGIN,     0 },
    { "c!",         Fcstore,    FHARDWARE,  0 },
    { "c,",         Fccomma,    FHARDWARE,  0 },
    { "c@",         Fcfetch,    FHARDWARE,  0 },
    { "compile",    Fcompile,   FHARDWARE,  0 },
    { "constant",   Fconstant,  FHARDWARE,  0 },
    { "cos",        Fcos,       FHARDWARE,  0 },
    { "count",      Fcount,     FHARDWARE,  0 },
    { "cr",         Fcr,        FHARDWARE,  0 },
    { "create",     Fcreate,    FHARDWARE,  0 },
    { "do",         0,          FDO,        0 },
    { "drop",       Fdrop,      FHARDWARE,  0 },
    { "dup",        Fdup,       FHARDWARE,  0 },
    { "else",       0,          FCONDELSE,  0 },
    { "emit",       Femit,      FHARDWARE,  0 },
    { "empty",      Fempty,     FHARDWARE,  0 },
    { "endif",      0,          FCONDTHEN,  0 },
    { "execute",    Fexecute,   FHARDWARE,  0 },
    { "exit",       0,          FEXIT,      0 },
    { "expect",     Fexpect,    FHARDWARE,  0 },
    { "f*",         Ffmul,      FHARDWARE,  0 },
    { "f+",         Ffadd,      FHARDWARE,  0 },
    { "f-",         Ffsub,      FHARDWARE,  0 },
    { "f->i",       Fftoi,      FHARDWARE,  0 },
    { "f.",         Ffprint,    FHARDWARE,  0 },
    { "f/",         Ffdiv,      FHARDWARE,  0 },
    { "f<",         Fflt,       FHARDWARE,  0 },
    { "f=",         Ffeq,       FHARDWARE,  0 },
    { "f>",         Ffgt,       FHARDWARE,  0 },
    { "fabs",       Ffabs,      FHARDWARE,  0 },
    { "facos",      Ffacos,     FHARDWARE,  0 },
    { "false",      Ffalse,     FHARDWARE,  0 },
    { "fasin",      Ffasin,     FHARDWARE,  0 },
    { "fatan",      Ffatan,     FHARDWARE,  0 },
    { "fcompile",   Ffcompile,  FHARDWARE,  0 },
    { "fconstant",  Ffconstant, FHARDWARE,  0 },
    { "fcos",       Ffcos,      FHARDWARE,  0 },
    { "fcosh",      Ffcosh,     FHARDWARE,  0 },
    { "fexp",       Ffexp,      FHARDWARE,  0 },
    { "fill",       Ffill,      FHARDWARE,  0 },
    { "find",       Ffind,      FHARDWARE,  0 },
    { "flog",       Fflog,      FHARDWARE,  0 },
    { "fmax",       Ffmax,      FHARDWARE,  0 },
    { "fnegate",    Ffnegate,   FHARDWARE,  0 },
    { "fmin",       Ffmin,      FHARDWARE,  0 },
    { "forget",     Fforget,    FHARDWARE,  0 },
    { "freezedict", Ffreezedict,FHARDWARE,  0 },
    { "fsin",       Ffsin,      FHARDWARE,  0 },
    { "fsinh",      Ffsinh,     FHARDWARE,  0 },
    { "fsqrt",      Ffsqrt,     FHARDWARE,  0 },
    { "ftan",       Fftan,      FHARDWARE,  0 },
    { "ftanh",      Fftanh,     FHARDWARE,  0 },
    { "halt",       Fhalt,      FHARDWARE,  0 },
    { "here",       Fhere,      FHARDWARE,  0 },
    { "hold",       Fhold,      FHARDWARE,  0 },
    { "i",          0,          FCOUNTI,    0 },
    { "i->f",       Fitof,      FHARDWARE,  0 },
    { "if",         0,          FCONDIF,    0 },
    { "immediate",  Fimmediate, FHARDWARE,  0 },
    { "input",      Finput,     FHARDWARE,  0 },
    { "j",          0,          FCOUNTJ,    0 },
    { "k",          0,          FCOUNTK,    0 },
    { "key",        Fkey,       FHARDWARE,  0 },
    { "latest",     Flatest,    FHARDWARE,  0 },
    { "leave",      0,          FLEAVE,     0 },
    { "list",       Flist,      FHARDWARE,  0 },
    { "loop",       0,          FLOOP,      0 },
    { "max",        Fmax,       FHARDWARE,  0 },
    { "min",        Fmin,       FHARDWARE,  0 },
    { "mod",        Fmod,       FHARDWARE,  0 },
    { "ncompile",   Fncompile,  FHARDWARE,  0 },
    { "negate",     Fnegate,    FHARDWARE,  0 },
    { "not",        Fnot,       FHARDWARE,  0 },
    { "or",         For,        FHARDWARE,  0 },
    { "outpop",     Foutpop,    FHARDWARE,  0 },
    { "output",     Foutput,    FHARDWARE,  0 },
    { "over",       Fover,      FHARDWARE,  0 },
    { "pick",       Fpick,      FHARDWARE,  0 },
    { "quit",       Fquit,      FHARDWARE,  0 },
    { "repeat",     0,          FREPEAT,    0 },
    { "roll",       Froll,      FHARDWARE,  0 },
    { "rot",        Frot,       FHARDWARE,  0 },
    { "sempty",     Fsempty,    FHARDWARE,  0 },
    { "sign",       Fsign,      FHARDWARE,  0 },
    { "sin",        Fsin,       FHARDWARE,  0 },
    { "space",      Fspace,     FHARDWARE,  0 },
    { "spaces",     Fspaces,    FHARDWARE,  0 },
    { "swap",       Fswap,      FHARDWARE,  0 },
    { "tan",        Ftan,       FHARDWARE,  0 },
    { "then",       0,          FCONDTHEN,  0 },
    { "true",       Ftrue,      FHARDWARE,  0 },
    { "type",       Ftype,      FHARDWARE,  0 },
    { "until",      0,          FUNTIL,     0 },
    { "u<",         Fult,       FHARDWARE,  0 },
    { "variable",   Fvariable,  FHARDWARE,  0 },
    { "while",      0,          FWHILE,     0 },
    { "word",       Fword,      FHARDWARE,  0 },
    { "words",      Fwords,     FHARDWARE,  0 },
    { "xor",        Fxor,       FHARDWARE,  0 },

    { 0,            0,          0,          0 },
};
