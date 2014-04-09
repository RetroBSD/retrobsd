/*
 * Scaled down version of printf(3).
 * Based on FreeBSD sources, heavily rewritten.
 *
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Two additional formats:
 *
 * The format %b is supported to decode error registers.
 * Its usage is:
 *
 *	printf("reg=%b\n", regval, "<base><arg>*");
 *
 * where <base> is the output base expressed as a control character, e.g.
 * \10 gives octal; \20 gives hex.  Each arg is a sequence of characters,
 * the first of which gives the bit number to be inspected (origin 1), and
 * the next characters (up to a control character, i.e. a character <= 32),
 * give the name of the register.  Thus:
 *
 *	kvprintf("reg=%b\n", 3, "\10\2BITTWO\1BITONE\n");
 *
 * would produce output:
 *
 *	reg=3<BITTWO,BITONE>
 *
 * The format %D -- Hexdump, takes a pointer. Sharp flag - use `:' as
 * a separator, instead of a space. For example:
 *
 *	("%6D", ptr)       -> XX XX XX XX XX XX
 *	("%#*D", len, ptr) -> XX:XX:XX:XX ...
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <float.h>
#include <math.h>

/* Max number conversion buffer length: a long in base 2, plus NUL byte. */
#define MAXNBUF	(sizeof(long) * 8 + 1)

static unsigned char *ksprintn (unsigned char *buf, unsigned long v, unsigned char base,
	int width, unsigned char *lp);
static unsigned char mkhex (unsigned char ch);

static int cvt (double number, int prec, int sharpflag, unsigned char *negp,
	unsigned char fmtch, unsigned char *startp, unsigned char *endp);

int
_doprnt (char const *fmt, va_list ap, FILE *stream)
{
#define PUTC(c) { putc (c, stream); ++retval; }
	unsigned char nbuf [MAXNBUF], padding, *q;
	const unsigned char *s;
	unsigned char c, base, lflag, ladjust, sharpflag, neg, dot, size;
	int n, width, dwidth, retval, uppercase, extrazeros, sign;
	unsigned long ul;

	if (! stream)
		return 0;
	if (! fmt)
		fmt = "(null)\n";

	retval = 0;
	for (;;) {
		while ((c = *fmt++) != '%') {
			if (! c)
				return retval;
			PUTC (c);
		}
		padding = ' ';
		width = 0; extrazeros = 0;
		lflag = 0; ladjust = 0; sharpflag = 0; neg = 0;
		sign = 0; dot = 0; uppercase = 0; dwidth = -1;
reswitch:	switch (c = *fmt++) {
		case '.':
			dot = 1;
			padding = ' ';
			dwidth = 0;
			goto reswitch;

		case '#':
			sharpflag = 1;
			goto reswitch;

		case '+':
			sign = -1;
			goto reswitch;

		case '-':
			ladjust = 1;
			goto reswitch;

		case '%':
			PUTC (c);
			break;

		case '*':
			if (! dot) {
				width = va_arg (ap, int);
				if (width < 0) {
					ladjust = !ladjust;
					width = -width;
				}
			} else {
				dwidth = va_arg (ap, int);
			}
			goto reswitch;

		case '0':
			if (! dot) {
				padding = '0';
				goto reswitch;
			}
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			for (n=0; ; ++fmt) {
				n = n * 10 + c - '0';
				c = *fmt;
				if (c < '0' || c > '9')
					break;
			}
			if (dot)
				dwidth = n;
			else
				width = n;
			goto reswitch;

		case 'b':
			ul = va_arg (ap, int);
			s = va_arg (ap, const unsigned char*);
			q = ksprintn (nbuf, ul, *s++, -1, 0);
			while (*q)
				PUTC (*q--);

			if (! ul)
				break;
			size = 0;
			while (*s) {
				n = *s++;
				if ((char) (ul >> (n-1)) & 1) {
					PUTC (size ? ',' : '<');
					for (; (n = *s) > ' '; ++s)
						PUTC (n);
					size = 1;
				} else
					while (*s > ' ')
						++s;
			}
			if (size)
				PUTC ('>');
			break;

		case 'c':
			if (! ladjust && width > 0)
				while (width--)
					PUTC (' ');

			PUTC (va_arg (ap, int));

			if (ladjust && width > 0)
				while (width--)
					PUTC (' ');
			break;

		case 'D':
			s = va_arg (ap, const unsigned char*);
			if (! width)
				width = 16;
			if (sharpflag)
				padding = ':';
			while (width--) {
				c = *s++;
				PUTC (mkhex (c >> 4));
				PUTC (mkhex (c));
				if (width)
					PUTC (padding);
			}
			break;

		case 'd':
			ul = lflag ? va_arg (ap, long) : va_arg (ap, int);
			if (! sign) sign = 1;
			base = 10;
			goto number;

		case 'l':
			lflag = 1;
			goto reswitch;

		case 'o':
			ul = lflag ? va_arg (ap, unsigned long) :
				va_arg (ap, unsigned int);
			base = 8;
			goto nosign;

		case 'p':
			ul = (size_t) va_arg (ap, void*);
			if (! ul) {
				s = (const unsigned char*) "(nil)";
				goto string;
			}
			base = 16;
			sharpflag = (width == 0);
			goto nosign;

		case 'n':
			ul = lflag ? va_arg (ap, unsigned long) :
				sign ? (unsigned long) va_arg (ap, int) :
				va_arg (ap, unsigned int);
			base = 10;
			goto number;

		case 's':
			s = va_arg (ap, unsigned char*);
			if (! s)
				s = (const unsigned char*) "(null)";
string:			if (! dot)
				n = strlen ((char*)s);
			else
				for (n=0; n<dwidth && s[n]; n++)
					continue;

			width -= n;

			if (! ladjust && width > 0)
				while (width--)
					PUTC (' ');
			while (n--)
				PUTC (*s++);
			if (ladjust && width > 0)
				while (width--)
					PUTC (' ');
			break;

		case 'r':
			/* Saturated counters. */
			base = 10;
			if (lflag) {
				ul = va_arg (ap, unsigned long);
				if (ul == -1) {
cnt_unknown:				if (ladjust)
						PUTC ('-');
					while (--width > 0)
						PUTC (' ');
					if (! ladjust)
						PUTC ('-');
					break;
				}
				if (ul >= -2) {
					ul = -3;
					neg = '>';
					goto nosign;
				}
			} else {
				ul = va_arg (ap, unsigned int);
				if (ul == (unsigned short) -1)
					goto cnt_unknown;
				if (ul >= (unsigned short) -2) {
					ul = (unsigned short) -3;
					neg = '>';
					goto nosign;
				}
			}
			goto nosign;

		case 'u':
			ul = lflag ? va_arg (ap, unsigned long) :
				va_arg (ap, unsigned int);
			base = 10;
			goto nosign;

		case 'x':
		case 'X':
			ul = lflag ? va_arg (ap, unsigned long) :
				va_arg (ap, unsigned int);
			base = 16;
			uppercase = (c == 'X');
			goto nosign;
		case 'z':
		case 'Z':
			ul = lflag ? va_arg (ap, unsigned long) :
				sign ? (unsigned long) va_arg (ap, int) :
				va_arg (ap, unsigned int);
			base = 16;
			uppercase = (c == 'Z');
			goto number;

nosign:			sign = 0;
number:		if (sign && ((long) ul != 0L)) {
				if ((long) ul < 0L) {
					neg = '-';
					ul = -(long) ul;
				} else if (sign < 0)
					neg = '+';
			}
			if (dwidth >= (int) sizeof(nbuf)) {
				extrazeros = dwidth - sizeof(nbuf) + 1;
				dwidth = sizeof(nbuf) - 1;
			}
			s = ksprintn (nbuf, ul, base, dwidth, &size);
			if (sharpflag && ul != 0) {
				if (base == 8)
					size++;
				else if (base == 16)
					size += 2;
			}
			if (neg)
				size++;

			if (! ladjust && width && padding == ' ' &&
			    (width -= size) > 0)
				do {
					PUTC (' ');
				} while (--width > 0);

			if (neg)
				PUTC (neg);

			if (sharpflag && ul != 0) {
				if (base == 8) {
					PUTC ('0');
				} else if (base == 16) {
					PUTC ('0');
					PUTC (uppercase ? 'X' : 'x');
				}
			}

			if (extrazeros)
				do {
					PUTC ('0');
				} while (--extrazeros > 0);

			if (! ladjust && width && (width -= size) > 0)
				do {
					PUTC (padding);
				} while (--width > 0);

			for (; *s; --s) {
				if (uppercase && *s>='a' && *s<='z') {
					PUTC (*s + 'A' - 'a');
				} else {
					PUTC (*s);
				}
			}

			if (ladjust && width && (width -= size) > 0)
				do {
					PUTC (' ');
				} while (--width > 0);
			break;

		case 'e':
		case 'E':
		case 'f':
		case 'F':
		case 'g':
		case 'G': {
			double d = va_arg (ap, double);
			/*
			 * don't do unrealistic precision; just pad it with
			 * zeroes later, so buffer size stays rational.
			 */
			if (dwidth > DBL_DIG) {
				if ((c != 'g' && c != 'G') || sharpflag)
					extrazeros = dwidth - DBL_DIG;
				dwidth = DBL_DIG;
			} else if (dwidth == -1) {
				dwidth = (lflag ? DBL_DIG : FLT_DIG);
			}
			/*
			 * softsign avoids negative 0 if d is < 0 and
			 * no significant digits will be shown
			 */
			if (d < 0) {
				neg = 1;
				d = -d;
			}
			/*
			 * cvt may have to round up past the "start" of the
			 * buffer, i.e. ``intf("%.2f", (double)9.999);'';
			 * if the first char isn't NULL, it did.
			 */
			if (isnan (d) || isinf (d)) {
				strcpy ((char*)nbuf, isnan (d) ? "NaN" : "Inf");
				size = 3;
				extrazeros = 0;
				s = nbuf;
			} else {
				*nbuf = 0;
				size = cvt (d, dwidth, sharpflag, &neg, c,
					nbuf, nbuf + sizeof(nbuf) - 1);
				if (*nbuf) {
					s = nbuf;
					nbuf [size] = 0;
				} else {
					s = nbuf + 1;
					nbuf [size + 1] = 0;
				}
			}
			if (neg)
				size++;
			if (! ladjust && width && padding == ' ' &&
			    (width -= size) > 0)
				do {
					PUTC (' ');
				} while (--width > 0);

			if (neg)
				PUTC ('-');

			if (! ladjust && width && (width -= size) > 0)
				do {
					PUTC (padding);
				} while (--width > 0);

			for (; *s; ++s) {
				if (extrazeros && (*s == 'e' || *s == 'E'))
					do {
						PUTC ('0');
					} while (--extrazeros > 0);

				PUTC (*s);
			}
			if (extrazeros)
				do {
					PUTC ('0');
				} while (--extrazeros > 0);

			if (ladjust && width && (width -= size) > 0)
				do {
					PUTC (' ');
				} while (--width > 0);
			break;
		}
		default:
			PUTC ('%');
			if (lflag)
				PUTC ('l');
			PUTC (c);
			break;
		}
	}
}

/*
 * Put a NUL-terminated ASCII number (base <= 16) in a buffer in reverse
 * order; return an optional length and a pointer to the last character
 * written in the buffer (i.e., the first character of the string).
 * The buffer pointed to by `nbuf' must have length >= MAXNBUF.
 */
static unsigned char *
ksprintn (unsigned char *nbuf, unsigned long ul, unsigned char base, int width,
	unsigned char *lenp)
{
	unsigned char *p;

	p = nbuf;
	*p = 0;
	for (;;) {
		*++p = mkhex (ul % base);
		ul /= base;
		if (--width > 0)
			continue;
		if (! ul)
			break;
	}
	if (lenp)
		*lenp = p - nbuf;
	return (p);
}

static unsigned char
mkhex (unsigned char ch)
{
	ch &= 15;
	if (ch > 9)
		return ch + 'a' - 10;
	return ch + '0';
}

static unsigned char *
cvtround (double fract, int *exp, unsigned char *start, unsigned char *end, unsigned char ch,
	unsigned char *negp)
{
	double tmp;

	if (fract) {
		modf (fract * 10, &tmp);
	} else {
		tmp = ch - '0';
	}
	if (tmp > 4) {
		for (;; --end) {
			if (*end == '.') {
				--end;
			}
			if (++*end <= '9') {
				break;
			}
			*end = '0';
			if (end == start) {
				if (exp) {	/* e/E; increment exponent */
					*end = '1';
					++*exp;
				} else {	/* f; add extra digit */
					*--end = '1';
					--start;
				}
				break;
			}
		}
	} else if (*negp) {
		/*
		 * ``"%.3f", (double)-0.0004'' gives you a negative 0.
		 */
		for (;; --end) {
			if (*end == '.') {
				--end;
			}
			if (*end != '0') {
				break;
			}
			if (end == start) {
				*negp = 0;
			}
		}
	}
	return start;
}

static unsigned char *
exponent (unsigned char *p, int exp, unsigned char fmtch)
{
	unsigned char expbuf [8], *t;

	*p++ = fmtch;
	if (exp < 0) {
		exp = -exp;
		*p++ = '-';
	} else {
		*p++ = '+';
	}
	t = expbuf + sizeof(expbuf);
	if (exp > 9) {
		do {
			*--t = exp % 10 + '0';
		} while ((exp /= 10) > 9);
		*--t = exp + '0';
		for (; t < expbuf + sizeof(expbuf); *p++ = *t++)
			continue;
	} else {
		*p++ = '0';
		*p++ = exp + '0';
	}
	return p;
}

static int
cvt (double number, int prec, int sharpflag, unsigned char *negp, unsigned char fmtch,
	unsigned char *startp, unsigned char *endp)
{
	unsigned char *p, *t;
	double fract;
	int dotrim, expcnt, gformat;
	double integer, tmp;

	expcnt = 0;
	dotrim = expcnt = gformat = 0;
	fract = modf (number, &integer);

	/*
	 * get an extra slot for rounding
	 */
	t = ++startp;

	/*
	 * get integer portion of number; put into the end of the buffer; the
	 * .01 is added for modf (356.0 / 10, &integer) returning .59999999...
	 */
	for (p = endp - 1; integer; ++expcnt) {
		tmp = modf (integer / 10, &integer);
		*p-- = (int) ((tmp + .01) * 10) + '0';
	}
	switch (fmtch) {
	case 'f':
		/* reverse integer into beginning of buffer */
		if (expcnt) {
			for (; ++p < endp; *t++ = *p);
		} else {
			*t++ = '0';
		}

		/*
		 * if precision required or alternate flag set, add in a
		 * decimal point.
		 */
		if (prec || sharpflag) {
			*t++ = '.';
		}

		/*
		 * if requires more precision and some fraction left
		 */
		if (fract) {
			if (prec) {
				do {
					fract = modf (fract * 10, &tmp);
					*t++ = (int)tmp + '0';
				} while (--prec && fract);
			}
			if (fract) {
				startp = cvtround (fract, 0, startp,
					t - 1, '0', negp);
			}
		}
		for (; prec--; *t++ = '0');
		break;
	case 'e':
	case 'E':
eformat:	if (expcnt) {
			*t++ = *++p;
			if (prec || sharpflag) {
				*t++ = '.';
			}

			/*
			 * if requires more precision and some integer left
			 */
			for (; prec && ++p < endp; --prec) {
				*t++ = *p;
			}

			/*
			 * if done precision and more of the integer component,
			 * round using it; adjust fract so we don't re-round
			 * later.
			 */
			if (! prec && ++p < endp) {
				fract = 0;
				startp = cvtround (0, &expcnt, startp,
					t - 1, *p, negp);
			}
			/*
			 * adjust expcnt for digit in front of decimal
			 */
			--expcnt;
		}
		/*
		 * until first fractional digit, decrement exponent
		 */
		else if (fract) {
			/*
			 * adjust expcnt for digit in front of decimal
			 */
			for (expcnt = -1;; --expcnt) {
				fract = modf (fract * 10, &tmp);
				if (tmp) {
					break;
				}
			}
			*t++ = (int)tmp + '0';
			if (prec || sharpflag) {
				*t++ = '.';
			}
		} else {
			*t++ = '0';
			if (prec || sharpflag) {
				*t++ = '.';
			}
		}
		/*
		 * if requires more precision and some fraction left
		 */
		if (fract) {
			if (prec) {
				do {
					fract = modf (fract * 10, &tmp);
					*t++ = (int)tmp + '0';
				} while (--prec && fract);
			}
			if (fract) {
				startp = cvtround (fract, &expcnt, startp,
					t - 1, '0', negp);
			}
		}
		/*
		 * if requires more precision
		 */
		for (; prec--; *t++ = '0');

		/*
		 * unless alternate flag, trim any g/G format trailing 0's
		 */
		if (gformat && ! sharpflag) {
			while (t > startp && *--t == '0');
			if (*t == '.') {
				--t;
			}
			++t;
		}
		t = exponent (t, expcnt, fmtch);
		break;
	case 'g':
	case 'G':
		/*
		 * a precision of 0 is treated as a precision of 1
		 */
		if (!prec) {
			++prec;
		}

		/*
		 * ``The style used depends on the value converted; style e
		 * will be used only if the exponent resulting from the
		 * conversion is less than -4 or greater than the precision.''
		 *	-- ANSI X3J11
		 */
		if (expcnt > prec || (! expcnt && fract && fract < .0001)) {
			/*
			 * g/G format counts "significant digits, not digits of
			 * precision; for the e/E format, this just causes an
			 * off-by-one problem, i.e. g/G considers the digit
			 * before the decimal point significant and e/E doesn't
			 * count it as precision.
			 */
			--prec;
			fmtch -= 2;		/* G->E, g->e */
			gformat = 1;
			goto eformat;
		}
		/*
		 * reverse integer into beginning of buffer,
		 * note, decrement precision
		 */
		if (expcnt) {
			for (; ++p < endp; *t++ = *p, --prec);
		} else {
			*t++ = '0';
		}
		/*
		 * if precision required or alternate flag set, add in a
		 * decimal point.  If no digits yet, add in leading 0.
		 */
		if (prec || sharpflag) {
			dotrim = 1;
			*t++ = '.';
		} else {
			dotrim = 0;
		}
		/*
		 * if requires more precision and some fraction left
		 */
		while (prec && fract) {
			fract = modf (fract * 10, &tmp);
			*t++ = (int)tmp + '0';
			prec--;
		}
		if (fract) {
			startp = cvtround (fract, 0, startp, t - 1, '0', negp);
		}
		/*
		 * alternate format, adds 0's for precision, else trim 0's
		 */
		if (sharpflag) {
			for (; prec--; *t++ = '0');
		} else if (dotrim) {
			while (t > startp && *--t == '0');
			if (*t != '.') {
				++t;
			}
		}
	}
	return t - startp;
}
