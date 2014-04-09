/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "user.h"
#include "buf.h"
#include "msgbuf.h"
#include "conf.h"
#include "ioctl.h"
#include "tty.h"
#include "reboot.h"
#include "systm.h"
#include "syslog.h"

#define TOCONS	0x1
#define TOTTY	0x2
#define TOLOG	0x4

/*
 * In case console is off,
 * panicstr contains argument to last
 * call to panic.
 */
char	*panicstr;

/*
 * Print a character on console or users terminal.
 * If destination is console then the last MSGBUFS characters
 * are saved in msgbuf for inspection later.
 */
static void
putchar (c, flags, tp)
	int c, flags;
	register struct tty *tp;
{
	if (flags & TOTTY) {
		register int s = spltty();

		if (tp && (tp->t_state & (TS_CARR_ON | TS_ISOPEN)) ==
			(TS_CARR_ON | TS_ISOPEN)) {
			if (c == '\n')
				(void) ttyoutput('\r', tp);
			(void) ttyoutput(c, tp);
			ttstart(tp);
		}
		splx(s);
	}
#ifdef LOG_ENABLED
	if ((flags & TOLOG) && c != '\0' && c != '\r' && c != 0177) {
		char sym = c;
		logwrt (&sym, 1, logMSG);
	}
#endif
	if ((flags & TOCONS) && c != '\0')
		cnputc(c);
}

static unsigned
mkhex (unsigned ch)
{
	ch &= 15;
	if (ch > 9)
		return ch + 'a' - 10;
	return ch + '0';
}

/*
 * Put a NUL-terminated ASCII number (base <= 16) in a buffer in reverse
 * order; return an optional length and a pointer to the last character
 * written in the buffer (i.e., the first character of the string).
 * The buffer pointed to by `nbuf' must have length >= MAXNBUF.
 */
static char *
ksprintn (char *nbuf, unsigned long ul, int base, int width, int *lenp)
{
	char *p;

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

void puts(char *s, int flags, struct tty *ttyp)
{
    while(*s)
        putchar(*(s++), flags, ttyp);
}

/*
 * Scaled down version of printf(3).
 * Two additional formats: %b anf %D.
 * Based on FreeBSD sources.
 * Heavily rewritten by Serge Vakulenko.
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

#define PUTC(C) putchar(C,flags,ttyp)

#define HION "\e[1m"
#define HIOFF "\e[0m"
static void
prf (fmt, ap, flags, ttyp)
	register char *fmt;
	register u_int *ap;
	int flags;
	struct tty *ttyp;
{
#define va_arg(ap,type)	*(type*) (void*) (ap++)

	char *q, nbuf [sizeof(long) * 8 + 1];
	const char *s;
	int c, padding, base, lflag, ladjust, sharpflag, neg, dot, size;
	int n, width, dwidth, uppercase, extrazeros, sign;
	unsigned long ul;

#ifdef KERNEL_HIGHLIGHT
    puts(HION,flags,ttyp);
#endif

	if (! fmt)
		fmt = "(null)\n";

	for (;;) {
		while ((c = *fmt++) != '%') {
			if (! c) {
#ifdef KERNEL_HIGHLIGHT
                puts(HIOFF,flags,ttyp);
#endif
				return;
            }
			PUTC (c);
		}
		padding = ' ';
		width = 0; extrazeros = 0;
		lflag = 0; ladjust = 0; sharpflag = 0; neg = 0;
		sign = 0; dot = 0; uppercase = 0; dwidth = -1;
reswitch:	c = *fmt++;
		switch (c) {
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
			s = va_arg (ap, const char*);
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
			s = va_arg (ap, const char*);
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
				s = "(nil)";
				goto const_string;
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
			s = va_arg (ap, char*);
			if (! s)
				s = (const char*) "(null)";
const_string:
			if (! dot)
				n = strlen (s);
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
number:			if (sign && ((long) ul != 0L)) {
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
		default:
			PUTC ('%');
			if (lflag)
				PUTC ('l');
			PUTC (c);
			break;
		}
	}
#ifdef KERNEL_HIGHLIGHT
    puts(HIOFF,flags,ttyp);
#endif
}

static void
logpri (level)
	int level;
{
	putchar ('<', TOLOG, (struct tty*) 0);
	prf ("%u", &level, TOLOG, (struct tty*) 0);
	putchar ('>', TOLOG, (struct tty*) 0);
}

/*
 * Scaled down version of C Library printf.
 * Used to print diagnostic information directly on console tty.
 * Since it is not interrupt driven, all system activities are
 * suspended.  Printf should not be used for chit-chat.
 *
 * One additional format: %b is supported to decode error registers.
 * Usage is:
 *	printf("reg=%b\n", regval, "<base><arg>*");
 * Where <base> is the output base expressed as a control character,
 * e.g. \10 gives octal; \20 gives hex.  Each arg is a sequence of
 * characters, the first of which gives the bit number to be inspected
 * (origin 1), and the next characters (up to a control character, i.e.
 * a character <= 32), give the name of the register.  Thus
 *	printf("reg=%b\n", 3, "\10\2BITTWO\1BITONE\n");
 * would produce output:
 *	reg=3<BITTWO,BITONE>
 */
void
printf(char *fmt, ...)
{
	prf(fmt, &fmt + 1, TOCONS | TOLOG, (struct tty *)0);
}

/*
 * Microchip MPLABX C32 compiler generates calls to _printf_s()
 * and other strange names.
 */
#ifdef __MPLABX__
void _printf_s(char *fmt, ...)
    __attribute__((alias ("printf")));
void _printf_cdnopuxX(char *fmt, ...)
    __attribute__((alias ("printf")));
void _printf_cdnopsuxX(char *fmt, ...)
    __attribute__((alias ("printf")));
#endif

/*
 * Uprintf prints to the current user's terminal,
 * guarantees not to sleep (so could be called by interrupt routines;
 * but prints on the tty of the current process)
 * and does no watermark checking - (so no verbose messages).
 * NOTE: with current kernel mapping scheme, the user structure is
 * not guaranteed to be accessible at interrupt level (see seg.h);
 * a savemap/restormap would be needed here or in putchar if uprintf
 * was to be used at interrupt time.
 */
void
uprintf (char *fmt, ...)
{
	register struct tty *tp;

	tp = u.u_ttyp;
	if (tp == NULL)
		return;

	if (ttycheckoutq (tp, 1))
		prf (fmt, &fmt+1, TOTTY, tp);
}

/*
 * tprintf prints on the specified terminal (console if none)
 * and logs the message.  It is designed for error messages from
 * single-open devices, and may be called from interrupt level
 * (does not sleep).
 */
void
tprintf (register struct tty *tp, char *fmt, ...)
{
	int flags = TOTTY | TOLOG;

	logpri (LOG_INFO);
	if (tp == (struct tty*) NULL)
		tp = &cnttys[0];
	if (ttycheckoutq (tp, 0) == 0)
		flags = TOLOG;
	prf (fmt, &fmt + 1, flags, tp);
#ifdef LOG_ENABLED
	logwakeup (logMSG);
#endif
}

/*
 * Log writes to the log buffer,
 * and guarantees not to sleep (so can be called by interrupt routines).
 * If there is no process reading the log yet, it writes to the console also.
 */
/*VARARGS2*/
void
log (int level, char *fmt, ...)
{
	register int s = splhigh();

	logpri(level);
	prf(fmt, &fmt + 1, TOLOG, (struct tty *)0);
	splx(s);
#ifdef LOG_ENABLED
	if (! logisopen(logMSG))
#endif
		prf(fmt, &fmt + 1, TOCONS, (struct tty *)0);
#ifdef LOG_ENABLED
	logwakeup(logMSG);
#endif
}

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: mesg", and then reboots.
 * If we are called twice, then we avoid trying to
 * sync the disks as this often leads to recursive panics.
 */
void
panic(s)
	char *s;
{
	int bootopt = RB_HALT | RB_DUMP;

	if (panicstr) {
		bootopt |= RB_NOSYNC;
	} else {
		panicstr = s;
	}
	printf ("panic: %s\n", s);
	boot (rootdev, bootopt);
}
