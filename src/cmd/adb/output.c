#include "defs.h"
#include <stdarg.h>

int     outfile = 1;
char    *printptr = printbuf;

static char *digitptr;

void
printc(c)
    int c;
{
    char d;
    char *q;
    int  posn, tabs, p;

    if (mkfault)
        return;

    *printptr = c;
    if (c == EOR) {
        tabs = 0;
        posn = 0;
        q = printbuf;
        for (p=0; p<printptr-printbuf; p++) {
            d = printbuf[p];
            if ((p & 7)==0 && posn) {
                tabs++;
                posn = 0;
            }
            if (d == SP) {
                posn++;
            } else {
                while (tabs > 0) {
                    *q++ = TB;
                    tabs--;
                }
                while (posn > 0) {
                    *q++ = SP;
                    posn--;
                }
                *q++ = d;
            }
        }
        *q++ = EOR;
        write(outfile, printbuf, q - printbuf);
        printptr = printbuf;

    } else if (c == TB) {
        *printptr++ = SP;
         while ((printptr - printbuf) & 7) {
            *printptr++ = SP;
        }
    } else if (c) {
        printptr++;
    }
    if (printptr >= &printbuf[MAXLIN-9]) {
        write(outfile, printbuf, printptr - printbuf);
        printptr = printbuf;
    }
}

void
flushbuf()
{
    if (printptr != printbuf) {
        printc(EOR);
    }
}

static int
convert(cp)
    register char **cp;
{
    register char c;
    int     n = 0;

    while ((c = *(*cp)++) >= '0' && c <= '9') {
        n = n*10 + c - '0';
    }
    (*cp)--;
    return n;
}

static void
printnum(n, fmat, base)
    register int n;
{
    register char k;
    register int *dptr;
    int digs[15];

    dptr = digs;
    if (n < 0 && fmat == 'd') {
        n = -n;
        *digitptr++ = '-';
    }
    while (n) {
        *dptr++ = ((u_int) n) % base;
        n = ((u_int) n ) / base;
    }
    if (dptr == digs) {
        *dptr++ = 0;
    }
    while (dptr != digs) {
        k = *--dptr;
        *digitptr++ = k + ((k <= 9) ? '0' : 'a'-10);
    }
}

static void
printoct(o, s)
    long    o;
    int     s;
{
    int     i;
    long    po = o;
    char    digs[12];

    if (s) {
        if (po < 0) {
            po = -po;
            *digitptr++ = '-';

        } else if (s > 0)
            *digitptr++='+';
    }
    for (i=0; i<=11; i++) {
        digs[i] = po & 7;
        po >>= 3;
    }
    digs[10] &= 03;
    digs[11] = 0;
    for (i=11; i>=0; i--) {
        if (digs[i])
            break;
    }
    for (i++; i>=0; i--) {
        *digitptr++ = digs[i] + '0';
     }
}

static void
printlong(lx, fmat, base)
    long    lx;
    int     fmat, base;
{
    int digs[20], *dptr;
    char k;
    unsigned long f, g;

    dptr = digs;
    f = lx;
    if (fmat == 'x')
        *digitptr++ = '#';
    else if (fmat == 'D' && lx < 0) {
        *digitptr++ = '-';
        f = -lx;
    }
    while (f) {
        g = f / base;
        *dptr++ = f - g * base;
        f = g;
    }
    if (dptr == digs) {
        *dptr++ = 0;
    }
    while (dptr != digs) {
        k = *--dptr;
        *digitptr++ = k + ((k <= 9) ? '0' : 'a'-10);
    }
}

static void
printdate(tvec)
    long    tvec;
{
    register int i;
    register char *timeptr;

    timeptr = ctime(&tvec);
    for (i=20; i<24; i++)
        *digitptr++ = timeptr[i];
    for (i=3; i<19; i++)
        *digitptr++ = timeptr[i];
}

void
print(char *fmat, ...)
{
    va_list args, prev_args;
    char    *fptr, *s;
    int     width, prec;
    char    c, adj;
    int     decpt, n;
    char    digits[64];
    union {
        int int32;
        float float32;
    } word;

    va_start(args, fmat);
    fptr = fmat;
    while ((c = *fptr++)) {
        if (c != '%') {
            printc(c);
            continue;
        }

        if (*fptr == '-') {
            adj = 'l';
            fptr++;
        } else {
            adj = 'r';
        }
        width = convert(&fptr);
        if (*fptr == '*') {
            width = va_arg(args, int);
            fptr++;
        }
        if (*fptr == '.') {
            fptr++;
            prec = convert(&fptr);
            if (*fptr == '*') {
                prec = va_arg(args, int);
                fptr++;
            }
        } else
            prec = -1;

        digitptr = digits;
        prev_args = args;
        word.int32 = va_arg(args, int);
        s = 0;
        switch (c = *fptr++) {
        case 'd':
        case 'u':
            printnum(word.int32, c, 10);
            break;
        case 'o':
            printoct(word.int32, 0);
            break;
        case 'q':
        case 'x':
            printlong(word.int32, 'x', 16);
            break;
        case 'Y':
            printdate(word.int32);
            break;
        case 'D':
        case 'U':
            printlong(word.int32, c, 10);
            break;
        case 'O':
            printoct(word.int32, 0);
            break;
        case 'Q':
        case 'X':
            printlong(word.int32, 'x', 16);
            break;
        case 'c':
            printc(word.int32);
            break;
        case 's':
            s = (char*) word.int32;
            break;
        case 'f':
        case 'F':
            s = ecvt(word.float32, prec, &decpt, &n);
            *digitptr++ = n ? '-' : '+';
            *digitptr++ = (decpt <= 0) ? '0' : *s++;
            if (decpt > 0) {
                decpt--;
            }
            *digitptr++ = '.';
            while (*s && prec-- ) {
                *digitptr++ = *s++;
            }
            while (*--digitptr=='0');
            digitptr += (digitptr - digits >= 3) ? 1 : 2;
            if (decpt) {
                *digitptr++ = 'e';
                printnum(decpt, 'd', 10);
            }
            s = 0;
            prec = -1;
            break;
        case 'm':
            args = prev_args;
            break;
        case 'M':
            width = word.int32;
            break;
        case 'T':
        case 't':
            if (c == 'T') {
                width = word.int32;
            } else {
                args = prev_args;
            }
            if (width) {
                width -= (printptr - printbuf) % width;
            }
            break;
        default:
            printc(c);
            args = prev_args;
        }

        if (s == 0) {
            *digitptr = 0;
            s = digits;
        }
        n = strlen(s);
        n = (prec < n && prec >= 0) ? prec : n;
        width -= n;
        if (adj == 'r') {
            while (width-- > 0)
                printc(SP);
        }
        while (n--)
            printc(*s++);
        while (width-- > 0)
            printc(SP);
        digitptr = digits;
    }
    va_end(args);
}

#define MAXIFD  5
struct {
    int     fd;
    long    r9;
} istack[MAXIFD];

int ifiledepth;

void
iclose(stack, err)
{
    if (err) {
        if (infile) {
            close(infile);
            infile = 0;
        }
        while (--ifiledepth >= 0) {
            if (istack[ifiledepth].fd) {
                close(istack[ifiledepth].fd);
                infile = 0;
            }
        }
        ifiledepth = 0;
    } else if (stack == 0) {
        if (infile) {
            close(infile);
            infile = 0;
        }
    } else if (stack > 0) {
        if (ifiledepth >= MAXIFD) {
            error(TOODEEP);
        }
        istack[ifiledepth].fd = infile;
        istack[ifiledepth].r9 = var[9];
        ifiledepth++;
        infile = 0;
    } else {
        if (infile) {
            close(infile);
            infile = 0;
        }
        if (ifiledepth > 0) {
            infile = istack[--ifiledepth].fd;
            var[9] = istack[ifiledepth].r9;
        }
    }
}

void
oclose()
{
    if (outfile != 1) {
        flushbuf();
        close(outfile);
        outfile = 1;
    }
}

void
endline()
{
    if ((printptr - printbuf) >= maxpos)
        printc('\n');
}
