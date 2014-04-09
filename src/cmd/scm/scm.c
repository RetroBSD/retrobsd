#include <unistd.h>
#include <string.h>

#include "scm.h"

/*
 * Global options.
 */
int trace;                      /* debug trace */
int verbose;                    /* verbose mode */

/*
 * Data for lexical analyser.
 */
#define MAXLEX  128             /* max size of strings */
#define MAXVECT 64              /* max size of vectors */
#define MEMSZ   2500            /* memory size */

long lexval;			/* value of numeric token */
double lexrealval;              /* value of float point token */
char lexsym[MAXLEX];            /* name of symbol token */
int backlexflag = 0;		/* flag of unget token */
int lexlex;			/* current token */
int lexlen;                     /* length of string token */

cell_t mem[MEMSZ];              /* list memory */
char gclabel[MEMSZ];            /* tags for garbage collector */
unsigned memsz = MEMSZ;         /* memory size */
lisp_t firstfree;               /* list of free cells */

lisp_t T;                       /* atom T */
lisp_t ZERO;                    /* atom 0 */
lisp_t ENV;                     /* top level context */

extern functab_t stdfunc [];    /* standard functions */

void fatal (char *s)
{
	fputs (s, stderr);
	fputc ('\n', stderr);
	exit (-1);
}

/*
 * Allocate memory for a new pair.
 */
lisp_t alloc (int type)
{
	lisp_t p = firstfree;
	if (p == NIL)
		fatal ("Out of memory");
	firstfree = cdr (firstfree);
	mem[p].type = type;
	return (p);
}

int isdigitx (int c, int radix)
{
	switch (c) {
	case '0': case '1':
		return (1);
	case '2': case '3': case '4': case '5': case '6': case '7':
		return (radix > 2);
	case '8': case '9':
		return (radix > 8);
	case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
		return (radix > 10);
	}
	return (0);
}

int isletterx (int c)
{
	switch (c) {
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
	case '+': case '-': case '.': case '*': case '/': case '<':
	case '=': case '>': case '!': case '?': case ':': case '$':
	case '%': case '_': case '&': case '^': case '~':
	case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
	case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
	case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
	case 's': case 't': case 'u': case 'v': case 'w': case 'x':
	case 'y': case 'z':
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
	case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
	case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
	case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
	case 'Y': case 'Z':
		return (1);
	}
	return ((unsigned char) c >= 0200);
}

/*
 * double strtodx (char *string, char **endPtr, int radix)
 *	This procedure converts a floating-point number from an ASCII
 *	decimal representation to internal double-precision format.
 *
 * Original sources taken from 386bsd and modified for variable radix
 * by Serge Vakulenko, <vak@kiae.su>.
 *
 * Arguments:
 * string
 *      A decimal ASCII floating-point number, optionally preceded
 *      by white space. Must have form "-I.FE-X", where I is the integer
 *      part of the mantissa, F is the fractional part of the mantissa,
 *      and X is the exponent.  Either of the signs may be "+", "-", or
 *      omitted.  Either I or F may be omitted, or both.  The decimal point
 *      isn't necessary unless F is present. The "E" may actually be an "e",
 *      or "E", "S", "s", "F", "f", "D", "d", "L", "l".
 *      E and X may both be omitted (but not just one).
 *
 * endPtr
 *      If non-NULL, store terminating character's address here.
 *
 * radix
 *      Radix of floating point, one of 2, 8, 10, 16.
 *
 * The return value is the double-precision floating-point
 * representation of the characters in string.  If endPtr isn't
 * NULL, then *endPtr is filled in with the address of the
 * next character after the last one that was part of the
 * floating-point number.
 */
double strtodx (char *string, char **endPtr, int radix)
{
	int sign = 0, expSign = 0, fracSz, fracOff, i;
	double fraction, dblExp, *powTab;
	register char *p;
	register char c;

	/* Exponent read from "EX" field. */
	int exp = 0;

	/* Exponent that derives from the fractional part.  Under normal
	 * circumstances, it is the negative of the number of digits in F.
	 * However, if I is very long, the last digits of I get dropped
	 * (otherwise a long I with a large negative exponent could cause an
	 * unnecessary overflow on I alone).  In this case, fracExp is
	 * incremented one for each dropped digit. */
	int fracExp = 0;

	/* Number of digits in mantissa. */
	int mantSize;

	/* Number of mantissa digits BEFORE decimal point. */
	int decPt;

	/* Temporarily holds location of exponent in string. */
	char *pExp;

	/* Largest possible base 10 exponent.
	 * Any exponent larger than this will already
	 * produce underflow or overflow, so there's
	 * no need to worry about additional digits. */
	static int maxExponent = 307;

	/* Table giving binary powers of 10.
	 * Entry is 10^2^i.  Used to convert decimal
	 * exponents into floating-point numbers. */
	static double powersOf10[] = {
		1e1, 1e2, 1e4, 1e8, 1e16, 1e32, //1e64, 1e128, 1e256,
	};
	static double powersOf2[] = {
		2, 4, 16, 256, 65536, 4.294967296e9, 1.8446744073709551616e19,
                //3.4028236692093846346e38, 1.1579208923731619542e77, 1.3407807929942597099e154,
	};
	static double powersOf8[] = {
		8, 64, 4096, 2.81474976710656e14, 7.9228162514264337593e28,
                //6.2771017353866807638e57, 3.9402006196394479212e115, 1.5525180923007089351e231,
	};
	static double powersOf16[] = {
		16, 256, 65536, 1.8446744073709551616e19,
                //3.4028236692093846346e38, 1.1579208923731619542e77, 1.3407807929942597099e154,
	};

	/*
	 * Strip off leading blanks and check for a sign.
	 */
	p = string;
	while (*p==' ' || *p=='\t')
		++p;
	if (*p == '-') {
		sign = 1;
		++p;
	} else if (*p == '+')
		++p;

	/*
	 * Count the number of digits in the mantissa (including the decimal
	 * point), and also locate the decimal point.
	 */
	decPt = -1;
	for (mantSize=0; ; ++mantSize) {
		c = *p;
		if (!isdigitx (c, radix)) {
			if (c != '.' || decPt >= 0)
				break;
			decPt = mantSize;
		}
		++p;
	}

	/*
	 * Now suck up the digits in the mantissa.  Use two integers to
	 * collect 9 digits each (this is faster than using floating-point).
	 * If the mantissa has more than 18 digits, ignore the extras, since
	 * they can't affect the value anyway.
	 */
	pExp = p;
	p -= mantSize;
	if (decPt < 0)
		decPt = mantSize;
	else
		--mantSize;             /* One of the digits was the point. */

	switch (radix) {
	default:
	case 10: fracSz = 9;  fracOff = 1000000000; powTab = powersOf10; break;
	case 2:  fracSz = 30; fracOff = 1073741824; powTab = powersOf2;  break;
	case 8:  fracSz = 10; fracOff = 1073741824; powTab = powersOf8;  break;
	case 16: fracSz = 7;  fracOff = 268435456;  powTab = powersOf16; break;
	}
	if (mantSize > 2 * fracSz)
		mantSize = 2 * fracSz;
	fracExp = decPt - mantSize;
	if (mantSize == 0) {
		fraction = 0.0;
		p = string;
		goto done;
	} else {
		int frac1, frac2;

		for (frac1=0; mantSize>fracSz; --mantSize) {
			c = *p++;
			if (c == '.')
				c = *p++;
			frac1 = frac1 * radix + (c - '0');
		}
		for (frac2=0; mantSize>0; --mantSize) {
			c = *p++;
			if (c == '.')
				c = *p++;
			frac2 = frac2 * radix + (c - '0');
		}
		fraction = (double) fracOff * frac1 + frac2;
	}

	/*
	 * Skim off the exponent.
	 */
	p = pExp;
	if (*p=='E' || *p=='e' || *p=='S' || *p=='s' || *p=='F' || *p=='f' ||
	     *p=='D' || *p=='d' || *p=='L' || *p=='l') {
		++p;
		if (*p == '-') {
			expSign = 1;
			++p;
		} else if (*p == '+')
			++p;
		while (isdigitx (*p, radix))
			exp = exp * radix + (*p++ - '0');
	}
	if (expSign)
		exp = fracExp - exp;
	else
		exp = fracExp + exp;

	/*
	 * Generate a floating-point number that represents the exponent.
	 * Do this by processing the exponent one bit at a time to combine
	 * many powers of 2 of 10. Then combine the exponent with the
	 * fraction.
	 */
	if (exp < 0) {
		expSign = 1;
		exp = -exp;
	} else
		expSign = 0;
	if (exp > maxExponent)
		exp = maxExponent;
	dblExp = 1.0;
	for (i=0; exp; exp>>=1, ++i)
		if (exp & 01)
			dblExp *= powTab[i];
	if (expSign)
		fraction /= dblExp;
	else
		fraction *= dblExp;

done:
	if (endPtr)
		*endPtr = p;

	return sign ? -fraction : fraction;
}

/*
 * Initialize a free list.
 */
void initmem ()
{
	register int i;

	firstfree = NIL;
	for (i=0; i<memsz; ++i) {
		gclabel[i] = 0;
		mem[i].type = TPAIR;
		mem[i].pair.a = NIL;
		mem[i].pair.d = firstfree;
		firstfree = i;
	}
}

void glabelit (lisp_t r)
{
	while (r!=NIL && ! gclabel[r]) {
		assert (r>=0 && r<memsz);
		gclabel[r] = 1;
		if (mem[r].type != TPAIR && mem[r].type != TCLOSURE)
			return;
		glabelit (mem[r].pair.a);
		r = mem[r].pair.d;              /* avoid recursion */
	}
}

int gcollect ()
{
	register int i, n;

	firstfree = NIL;
	for (n=i=0; i<memsz; ++i)
		if (gclabel[i])
			gclabel[i] = 0;
		else {
			switch (mem[i].type) {
			case TSYMBOL:
				free (mem[i].symbol.name);
				break;
			case TSTRING:
				if (mem[i].string.length > 0)
					free (mem[i].string.array);
				break;
			case TVECTOR:
				if (mem[i].vector.length > 0)
					free (mem[i].vector.array);
				break;
			}
			mem[i].type = TPAIR;
			mem[i].pair.a = NIL;
			mem[i].pair.d = firstfree;
			firstfree = i;
			++n;
		}
	return (n);
}

/*
 * Garbage collection.
 */
void gc ()
{
	register int n;

	if (trace)
		fputs ("GC...", stderr);
	glabelit (T);
	glabelit (ZERO);
	glabelit (ENV);
	n = gcollect ();
	if (trace)
		fprintf (stderr, "%d OK ", n);
}

void *memcopy (void *p, int len)
{
        assert (len > 0);
	char *q = malloc (len);
	if (! q)
		fatal ("out of dynamic memory");
        memcpy (q, p, len);
	return (q);
}

int getoct (int c)
{
	int n = c - '0';
	c = getchar ();
	if (c<='7' && c>='0') {
		n = (n<<3) + c - '0';
		c = getchar ();
		if (c<='7' && c>='0')
			n = (n<<3) + c - '0';
		else ungetc (c, stdin);
	} else ungetc (c, stdin);
	return (n);
}

int getchr ()
{
	int c, n;

	switch (c = getchar ()) {
	default:   return (c);
	case '\n': return (' ');
	case '\\': break;
	case 's': case 'S':
		n = getchar ();
		if (n!='p' && n!='P') {
			ungetc (n, stdin);
			return (c);
		}
		if (((c=getchar())=='a' || c=='A') &&
		    ((c=getchar())=='c' || c=='C') &&
		    ((c=getchar())=='e' || c=='E'))
			return (' ');
		fatal ("invalid #\\space constant");
	case 'n': case 'N':
		n = getchar ();
		if (n!='e' && n!='E') {
			ungetc (n, stdin);
			return (c);
		}
		if (((c=getchar())=='w' || c=='W') &&
		    ((c=getchar())=='l' || c=='L') &&
		    ((c=getchar())=='i' || c=='I') &&
		    ((c=getchar())=='n' || c=='N') &&
		    ((c=getchar())=='e' || c=='E'))
			return ('\n');
		fatal ("invalid #\\newline constant");
	}
	switch (c = getchar ()) {
	case '0': case '1': case '2': case '3':
	case '4': case '5': case '6': case '7':
		return (getoct (c));
	case '\n': return (' ');
	case 'n':  return ('\n');
	case 't':  return ('\t');
	case 'r':  return ('\r');
	case 'b':  return ('\b');
	case 'f':  return ('\f');
	}
	return (c);
}

void putchr (int c, FILE *fd)
{
	fputs ("#\\", fd);
	switch (c) {
	case ' ':  fputs ("space", fd);   return;
	case '\n': fputs ("newline", fd); return;
	case '\t': fputs ("\\t", fd);     return;
	case '\r': fputs ("\\r", fd);     return;
	case '\b': fputs ("\\b", fd);     return;
	case '\f': fputs ("\\f", fd);     return;
	}
	if (c<' ' || c==177)
		fprintf (fd, "\\%03o", c);
	else
		putc (c, fd);
}

void putstring (lisp_t p, FILE *fd)
{
	int len;
	char *s;

	assert (p>=0 && p<memsz && mem[p].type==TSTRING);
	len = mem[p].string.length;
	s = mem[p].string.array;
	putc ('"', fd);
	while (--len >= 0) {
		static char mask[] = "\"\\\n\t\r\b\f", repl[] = "\"\\ntrbf";
		unsigned char c = *s++;
		char *p = strchr (mask, c);
		if (c && p) {
			putc ('\\', fd);
			putc (repl[p-mask], fd);
		} else if (c<' ' || c==177)
			fprintf (fd, "\\%03o", c);
		else
			putc (c, fd);
	}
	putc ('"', fd);
}

void putvector (lisp_t p, FILE *fd)
{
	int len;
	lisp_t *s;

	assert (p>=0 && p<memsz && mem[p].type==TVECTOR);
	len = mem[p].vector.length;
	s = mem[p].vector.array;
	fputs ("#(", fd);
	while (--len >= 0) {
		putexpr (*s++, fd);
		if (len)
			putc (' ', fd);
	}
	putc (')', fd);
}

void putatom (lisp_t p, FILE *fd)
{
	if (p == NIL) {
		fputs ("()", fd);
		return;
	}
	switch (mem[p].type) {
	case TSYMBOL:  fputs (symname (p), fd);                         break;
	case TBOOL:    fputs ("#t", fd);                                break;
	case TINTEGER: fprintf (fd, "%ld", numval (p));                 break;
	case TREAL:    fprintf (fd, "%#g", realval (p));                break;
	case TSTRING:  putstring (p, fd);                               break;
	case TVECTOR:  putvector (p, fd);                               break;
	case TCHAR:    putchr (charval (p), fd);                        break;
	case THARDW:   fprintf (fd, "<builtin-%x>", (unsigned) hardwval (p)); break;
	case TCLOSURE: fprintf (fd, "<closure-%x>", (unsigned) p);      break;
	default:       fputs ("<?>", fd);                               break;
	}
}

void putlist (lisp_t p, FILE *fd)
{
	int first = 1;
	while (istype (p, TPAIR)) {
		if (first)
			first = 0;
		else
			putc (' ', fd);
		putexpr (car (p), fd);
		p = cdr (p);
	}
	if (p != NIL) {
		fputs (" . ", fd);
		putatom (p, fd);
	}
}

void putexpr (lisp_t p, FILE *fd)
{
	lisp_t h, a;

	if (! istype (p, TPAIR)) {
		putatom (p, fd);
		return;
	}
	if (istype (h = car (p), TSYMBOL) &&
	    istype (a = cdr (p), TPAIR) &&
	    cdr (a) == NIL) {
		char *funcname = symname (h);
		if (!strcmp (funcname, "quote")) {
			putc ('\'', fd);
			putexpr (car (a), fd);
			return;
		}
		if (!strcmp (funcname, "quasiquote")) {
			putc ('`', fd);
			putexpr (car (a), fd);
			return;
		}
		if (!strcmp (funcname, "unquote")) {
			putc (',', fd);
			putexpr (car (a), fd);
			return;
		}
		if (!strcmp (funcname, "unquote-splicing")) {
			putc (',', fd);
			putc ('@', fd);
			putexpr (car (a), fd);
			return;
		}
	}
	putc ('(', fd);
	putlist (p, fd);
	putc (')', fd);
}

void ungetlex ()
{
	backlexflag = 1;
}

/*
 * Lexical parser: get next token.
 * Side effect: saving values to lexval and lexsym.
 */
int getlex ()
{
	register int c, i, radix = 10, neg = 0;

	if (backlexflag) {
		backlexflag = 0;
		return (lexlex);
	}
loop:
	switch (c = getchar ()) {
	case EOF:
		return (0);
	case ';':
		while ((c = getchar ()) >= 0 && c!='\n')
			continue;
	case ' ': case '\t': case '\f': case '\n': case '\r':
		goto loop;
	case '"':
		lexlen = 0;
		while ((c = getchar ()) >= 0 && c!='"') {
			if (c == '\\') {
				c = getchar ();
				if (c < 0)
					break;
				switch (c) {
				case '\n': continue;
				case 'n': c = '\n'; break;
				case 't': c = '\t'; break;
				case 'r': c = '\r'; break;
				case 'b': c = '\b'; break;
				case 'f': c = '\f'; break;
				case '0': case '1': case '2': case '3':
				case '4': case '5': case '6': case '7':
					c = getoct (c);
					break;
				}
			}
			lexsym[lexlen++] = c;
		}
		return (lexlex = TSTRING);
	case '#':
		c = getchar ();
		switch (c) {
		default: ungetc (c, stdin); return (lexlex = '#');
		case '(': return (lexlex = TVECTOR);
		case '\\': lexval = getchr ();  return (lexlex = TCHAR);
		case 't': case 'T': lexval = 1; return (lexlex = TBOOL);
		case 'f': case 'F': lexval = 0; return (lexlex = TBOOL);
		case 'b': case 'B': radix = 2; break;
		case 'o': case 'O': radix = 8; break;
		case 'd': case 'D': radix = 10; break;
		case 'x': case 'X': radix = 16; break;
		}
		c = getchar ();
		if (! isdigitx (c, radix) && c!='-' && c!='+' && c!='.') {
			ungetc (c, stdin);
			lexval = 0;
			return (lexlex = TINTEGER);
		}
		break;
	}
	if (c=='-' || c=='+') {
		lexsym[0] = c;
		c = getchar ();
		if (! isdigitx (c, radix)) {
			ungetc (c, stdin);
			c = lexsym[0];
			goto lexsym;
		}
		neg = (lexsym[0] == '-');
	}
	if (isdigitx (c, radix) || c=='.') {
		i = 0;
		while (i < MAXLEX-1 && c>=0 && isdigitx (c, radix)) {
			lexsym[i++] = c;
			c = getchar ();
		}
		if ((c=='.' || c=='E' || c=='e' || c=='S' ||
		    c=='s' || c=='F' || c=='f' || c=='D' ||
		    c=='d' || c=='L' || c=='l') && i<MAXLEX-1) {
			if (c=='.') {
				lexsym[i++] = c;
				c = getchar ();
				while (i < MAXLEX-1 && c>=0 && isdigitx (c, radix)) {
					lexsym[i++] = c;
					c = getchar ();
				}
			}
			if ((c=='E' || c=='e' || c=='S' || c=='s' ||
			    c=='F' || c=='f' || c=='D' || c=='d' ||
			    c=='L' || c=='l') && i<MAXLEX-1) {
				lexsym[i++] = c;
				c = getchar ();
				if ((c=='+' || c=='-') && i<MAXLEX-1) {
					lexsym[i++] = c;
					c = getchar ();
				}
				while (i < MAXLEX-1 && c>=0 && isdigitx (c, radix)) {
					lexsym[i++] = c;
					c = getchar ();
				}
			}
			if (c >= 0)
				ungetc (c, stdin);
			lexsym[i] = 0;
			lexrealval = strtodx (lexsym, 0, radix);
			if (neg)
				lexrealval = -lexrealval;
			return (lexlex = TREAL);
		}
		if (c >= 0)
			ungetc (c, stdin);
		lexsym[i] = 0;
		lexval = strtol (lexsym, 0, radix);
		if (neg)
			lexval = -lexval;
		return (lexlex = TINTEGER);
	}
	if (isletterx (c)) {
lexsym:         i = 0;
		while (i < MAXLEX-1) {
			lexsym[i++] = c;
			c = getchar ();
			if (c < 0)
				break;
			if (! isletterx (c)) {
				ungetc (c, stdin);
				break;
			}
		}
		if (i == 1 && lexsym[0] == '.')
			return (lexlex = '.');
		lexsym[i] = 0;
		return (lexlex = TSYMBOL);
	}
	return (lexlex = c);
}

/*
 * Read vector EXPR...
 */
lisp_t getvector ()
{
	int len = 0;
	lisp_t vect[MAXVECT];

	for (;;) {
		switch (getlex ()) {
		case ')':
			ungetlex ();
			return (vector (len, vect));
		default:
			if (len >= MAXVECT)
				fatal ("too long vector constant");
			ungetlex ();
			vect[len++] = getexpr ();
			continue;
		case 0:
			fatal ("unexpected eof");
		}
	}
}

/*
 * Read list EXPR ('.' LIST | EXPR)...
 */
lisp_t getlist ()
{
	lisp_t p = cons (getexpr (), NIL);
	switch (getlex ()) {
	case '.':
		setcdr (p, getexpr ());
		break;
	case ')':
		ungetlex ();
		break;
	default:
		ungetlex ();
		setcdr (p, getlist ());
		break;
	case 0:
		fatal ("unexpected eof");
	}
	return (p);
}

/*
 * Read expression ATOM | NUMBER | '(' LIST ')'
 */
lisp_t getexpr ()
{
	lisp_t p;

	switch (getlex ()) {
	default:
		fatal ("syntax error");
	case ')':
		ungetlex ();
	case 0:
		return (NIL);
	case '(':
		if (getlex () == ')')
			return (NIL);
		ungetlex ();
		p = getlist ();
		if (getlex () != ')')
			fatal ("right parence expected");
		break;
	case '\'':
		p = cons (symbol ("quote"), cons (getexpr (), NIL));
		break;
	case '`':
		p = cons (symbol ("quasiquote"), cons (getexpr (), NIL));
		break;
	case ',':
		if (getlex () == '@')
			p = cons (symbol ("unquote-splicing"), cons (getexpr (), NIL));
		else {
			ungetlex ();
			p = cons (symbol ("unquote"), cons (getexpr (), NIL));
		}
		break;
	case TSYMBOL:
		p = symbol (lexsym);
		if (trace > 2)
			fprintf (stderr, "%s\n", lexsym);
		break;
	case TBOOL:
		p = lexval ? T : NIL;
		if (trace > 2)
			fprintf (stderr, "#%c\n", lexval ? 't' : 'f');
		break;
	case TCHAR:
		p = character (lexval);
		if (trace > 2)
			fprintf (stderr, "#\\\\%03o\n", (unsigned) lexval);
		break;
	case TINTEGER:
		p = number (lexval);
		if (trace > 2)
			fprintf (stderr, "%ld\n", lexval);
		break;
	case TREAL:
		p = real (lexrealval);
		if (trace > 2)
			fprintf (stderr, "%#g\n", lexrealval);
		break;
	case TSTRING:
		p = string (lexlen, lexsym);
		if (trace > 2) {
			putstring (p, stderr);
			fprintf (stderr, "\n");
		}
		break;
	case TVECTOR:
		p = getvector ();
		if (getlex () != ')')
			fatal ("right parence expected");
		break;
	}
	return (p);
}

lisp_t copy (lisp_t a, lisp_t *t)
{
	lisp_t val, tail;
	if (! istype (a, TPAIR))
		return (NIL);
	tail = val = cons (NIL, NIL);
	for (;;) {
		setcar (tail, car (a));
		if (! istype (a = cdr (a), TPAIR))
			break;
		setcdr (tail, cons (NIL, NIL));
		tail = cdr (tail);
	}
	if (t)
		*t = tail;
	return (val);
}

/*
 * Compare vectors.
 */
int eqvvector (lisp_t a, lisp_t b)
{
	lisp_t *s, *t;
	int len;

	assert (a>=0 && a<memsz && mem[a].type==TVECTOR);
	assert (b>=0 && b<memsz && mem[b].type==TVECTOR);
	if (mem[a].vector.length != mem[b].vector.length)
		return (0);
	len = mem[a].vector.length;
	s = mem[a].vector.array;
	t = mem[b].vector.array;
	while (--len >= 0)
		if (! eqv (*s++, *t++))
			return (0);
	return (1);
}

/*
 * Compare objects.
 */
int eqv (lisp_t a, lisp_t b)
{
	if (a == b)
		return (1);
	if (a == NIL || b == NIL)
		return (0);
	assert (a>=0 && a<memsz);
	assert (b>=0 && b<memsz);
	if (mem[a].type != mem[b].type)
		return (0);
	switch (mem[a].type) {
	case TBOOL:     return (1);
	case TCHAR:     return (mem[a].chr.value == mem[b].chr.value);
	case TINTEGER:  return (mem[a].integer.value == mem[b].integer.value);
	case TREAL:     return (mem[a].real.value == mem[b].real.value);
	case TSYMBOL:   return (!strcmp (mem[a].symbol.name, mem[b].symbol.name));
	case TVECTOR:   return (eqvvector (a, b));
	case TSTRING:   return (mem[a].string.length == mem[b].string.length &&
				(mem[a].string.length <= 0 ||
				memcmp (mem[a].string.array, mem[b].string.array,
                                    mem[a].string.length) == 0));
	}
	return (0);
}

/*
 * Recursive comparison.
 */
int equal (lisp_t a, lisp_t b)
{
	if (a == b)
		return (1);
	while (istype (a, TPAIR)) {
		if (! istype (b, TPAIR) || ! equal (car (a), car (b)))
			return (0);
		a = cdr (a);
		b = cdr (b);
	}
	return (eqv (a, b));
}

/*
 * Find an atom by context.
 * Context is a list of pairs (name, value).
 */
lisp_t findatom (lisp_t atom, lisp_t ctx)
{
	if (! istype (atom, TSYMBOL))
		return (NIL);
	/* Current context first. */
	for (; ctx!=NIL; ctx=cdr(ctx)) {
		lisp_t pair = car (ctx);
		lisp_t sym = car (pair);
		if (atom == sym || !strcmp (symname (atom), symname (sym)))
			return (pair);
	}
	/* Next is a context of upper level. */
	for (ctx=ENV; ctx!=NIL; ctx=cdr(ctx)) {
		lisp_t pair = car (ctx);
		lisp_t sym = car (pair);
		if (atom == sym || !strcmp (symname (atom), symname (sym)))
			return (pair);
	}
	return (NIL);
}

/*
 * Assign a value to a variable.
 */
void setatom (lisp_t atom, lisp_t value, lisp_t ctx)
{
	lisp_t pair = findatom (atom, ctx);
	if (pair == NIL) {
		fprintf (stderr, "unbound symbol: `%s'\n", symname (atom));
		return;
	}
	setcdr (pair, value);
}

/*
 * Check the type.
 */
int istype (lisp_t p, int type)
{
	if (p == NIL) return (0);
	assert (p>=0 && p<memsz);
	return (mem[p].type == type);
}

lisp_t evallist (lisp_t expr, lisp_t ctx)
{
	lisp_t val, tail;
	tail = val = cons (NIL, NIL);
	for (;;) {
		setcar (tail, eval (car (expr), &ctx));
		if (! istype (expr = cdr (expr), TPAIR))
			return (val);
		setcdr (tail, cons (NIL, NIL));
		tail = cdr (tail);
	}
}

/*
 * Evaluate a block in a given context.
 */
lisp_t evalblock (lisp_t expr, lisp_t ctx)
{
	lisp_t value = NIL;
	while (istype (expr, TPAIR)) {
		value = eval (car (expr), &ctx);
		expr = cdr (expr);
	}
	return (value);
}

lisp_t evalclosure (lisp_t func, lisp_t expr)
{
	lisp_t ctx = closurectx (func), body = closurebody (func);
	lisp_t arg = car (body);

	/* Extend the context by arguments. */
	while (istype (arg, TPAIR)) {
		lisp_t val;
		if (istype (expr, TPAIR)) {
			val = car (expr);
			expr = cdr (expr);
		} else {
			/* Set missing arguments to NIL. */
			val = NIL;
                }
		if (istype (car (arg), TSYMBOL))
			ctx = cons (cons (car (arg), val), ctx);
		arg = cdr (arg);
	}
	if (istype (arg, TSYMBOL))
		ctx = cons (cons (arg, expr), ctx);
	if (trace) {
		printf ("CALL ");
		putexpr (cdr (body), stdout);
		printf ("\nCONTEXT ");
		putexpr (ctx, stdout);
		printf ("\n");
	}
	return (evalblock (cdr (body), ctx));
}

lisp_t quasiquote (lisp_t expr, lisp_t ctx, int level)
{
	lisp_t val, tail, func, v;
	if (! istype (expr, TPAIR))
		return (expr);
	if (istype (func = car (expr), TSYMBOL)) {
		char *funcname = symname (func);
		if (!strcmp (funcname, "quasiquote")) {
			v = !istype (v = cdr (expr), TPAIR) ? NIL :
				quasiquote (car (v), ctx, level+1);
			return (cons (func, cons (v, NIL)));
		}
		if (!strcmp (funcname, "unquote") ||
		    !strcmp (funcname, "unquote-splicing")) {
			if (!istype (v = cdr (expr), TPAIR))
				return (level ? expr : NIL);
			if (level)
				return (cons (func, cons (quasiquote (car (v),
					ctx, level-1), NIL)));
			return (eval (car (v), &ctx));
		}
	}
	tail = val = cons (NIL, NIL);
	for (;;) {
		v = car (expr);
		if (! istype (v, TPAIR))
			setcar (tail, v);
		else if (istype (func = car (v), TSYMBOL) &&
		     !strcmp (symname (func), "unquote-splicing")) {
			if (!istype (v = cdr (v), TPAIR)) {
				if (level)
					setcar (tail, car (expr));
			} else if (level)
				setcar (tail, cons (func,
					cons (quasiquote (car (v), ctx,
					level-1), NIL)));
			else {
				v = eval (car (v), &ctx);
				if (istype (v, TPAIR)) {
					lisp_t newtail;
					setcar (tail, car (v));
					setcdr (tail, copy (cdr (v), &newtail));
					tail = newtail;
				} else if (v != NIL) {
					setcar (tail, v);
					setcdr (tail, cons (NIL, NIL));
					tail = cdr (tail);
				}
			}
		} else
			setcar (tail, quasiquote (v, ctx, level));
		if (! istype (expr = cdr (expr), TPAIR)) {
			setcdr (tail, expr);
			return (val);
		}
		setcdr (tail, cons (NIL, NIL));
		tail = cdr (tail);
	}
}

lisp_t evalfunc (lisp_t func, lisp_t arg, lisp_t ctx)
{
	/* Hard-wired function. */
	if (istype (func, THARDW))
		return ((*hardwval (func)) (arg, ctx));

	/* Generic function: compute it's value. */
	if (istype (func, TCLOSURE))
		return (evalclosure (func, arg));

	/* Not a function, ignore. */
	return (NIL);
}

lisp_t eval (lisp_t expr, lisp_t *ctxp)
{
	lisp_t ctx = ctxp ? *ctxp : NIL;
	lisp_t func;
again:
	if (expr == NIL)
		return (NIL);

	/* If a symbol, get it's value. */
	if (istype (expr, TSYMBOL)) {
		/* Find a value in a context. */
		lisp_t pair = findatom (expr, ctx);
		if (pair == NIL) {
			fprintf (stderr, "unbound symbol: `%s'\n", symname (expr));
			return (NIL);
		}
		return (cdr (pair));
	}

	/* All but atom and list: leave as is. */
	if (! istype (expr, TPAIR))
		return (expr);

	/* Built-in special forms:
	 * quote define set! begin lambda let let* letrec if
	 * and or cond else => quasiquote unquote unquote-splicing
	 */
	/* Reserved names:
	 * delay do case
	 */
	func = car (expr);
	if (istype (func, TSYMBOL)) {
		char *funcname = symname (func);
		if (!strcmp (funcname, "quote")) {
			if (! istype (expr = cdr (expr), TPAIR))
				return (NIL);
			return (car (expr));
		}
		if (!strcmp (funcname, "define")) {
			lisp_t value, atom, pair, arg = 0;
			int lambda;
			if (! istype (expr = cdr (expr), TPAIR))
				return (NIL);
			lambda = istype (atom = car (expr), TPAIR);
			if (lambda) {
				/* define, combined with lambda. */
				arg = cdr (atom);
				atom = car (atom);
			}
			if (! istype (atom, TSYMBOL) ||
			    ! istype (expr = cdr (expr), TPAIR))
				return (NIL);
			pair = findatom (atom, ctx);
			if (pair == NIL) {
				/* Extend the context. */
				pair = cons (atom, NIL);
				if (ctxp) {
					/* Local context. */
					*ctxp = ctx = cons (pair, ctx);
				} else {
					/* Upper level context. */
					ENV = cons (pair, ENV);
                                }
			}
			if (lambda)
				value = closure (cons (arg, expr), ctx);
			else
				value = evalblock (expr, ctx);
			setcdr (pair, value);
			return (value);
		}
		if (!strcmp (funcname, "set!")) {
			lisp_t value = NIL;
			if (! istype (expr = cdr (expr), TPAIR))
				return (NIL);
			if (istype (cdr (expr), TPAIR))
				value = evalblock (cdr (expr), ctx);
			setatom (car (expr), value, ctx);
			return (value);
		}
		if (!strcmp (funcname, "begin"))
			return (evalblock (cdr (expr), ctx));
		if (!strcmp (funcname, "lambda")) {
			lisp_t arg = NIL;
			if (istype (expr = cdr (expr), TPAIR)) {
				arg = car (expr);
				if (! istype (expr = cdr (expr), TPAIR))
					expr = NIL;
			}
			return (closure (cons (arg, expr), ctx));
		}
		if (!strcmp (funcname, "let")) {
			lisp_t arg = NIL, oldctx = ctx;
			if (istype (expr = cdr (expr), TPAIR)) {
				arg = car (expr);
				if (! istype (expr = cdr (expr), TPAIR))
					expr = NIL;
			}
			/* Extend the context by new variables. */
			while (istype (arg, TPAIR)) {
				lisp_t var = car (arg);
				arg = cdr (arg);
				/* Compute values in old context. */
				if (istype (var, TPAIR))
					ctx = cons (cons (car (var),
						evalblock (cdr (var), oldctx)),
						ctx);
				else if (istype (var, TSYMBOL))
					ctx = cons (cons (var, NIL), ctx);
			}
			return (evalblock (expr, ctx));
		}
		if (!strcmp (funcname, "let*")) {
			lisp_t arg = NIL;
			if (istype (expr = cdr (expr), TPAIR)) {
				arg = car (expr);
				if (! istype (expr = cdr (expr), TPAIR))
					expr = NIL;
			}
			/* Extend the context by new variables. */
			while (istype (arg, TPAIR)) {
				lisp_t var = car (arg);
				arg = cdr (arg);
				/* Compute values in the current context. */
				if (istype (var, TPAIR))
					ctx = cons (cons (car (var),
						evalblock (cdr (var), ctx)),
						ctx);
				else if (istype (var, TSYMBOL))
					ctx = cons (cons (var, NIL), ctx);
			}
			return (evalblock (expr, ctx));
		}
		if (!strcmp (funcname, "letrec")) {
			lisp_t arg = NIL, a;
			if (istype (expr = cdr (expr), TPAIR)) {
				arg = car (expr);
				if (! istype (expr = cdr (expr), TPAIR))
					expr = NIL;
			}
			/* Extend the context by new variables with NIL values.. */
			for (a=arg; istype (a, TPAIR); a=cdr(a)) {
				lisp_t var = car (a);
				if (istype (var, TPAIR))
					ctx = cons (cons (car (var), NIL), ctx);
				else if (istype (var, TSYMBOL))
					ctx = cons (cons (var, NIL), ctx);
			}
			/* Compute values in the new context. */
			for (a=arg; istype (a, TPAIR); a=cdr(a)) {
				lisp_t var = car (a);
				if (istype (var, TPAIR))
					setatom (car (var),
						evalblock (cdr (var), ctx),
						ctx);
			}
			return (evalblock (expr, ctx));
		}
		if (!strcmp (funcname, "if")) {
			lisp_t iftrue = NIL, iffalse = NIL, test;
			if (! istype (expr = cdr (expr), TPAIR))
				return (NIL);
			test = car (expr);
			if (istype (expr = cdr (expr), TPAIR)) {
				iftrue = car (expr);
				iffalse = cdr (expr);
			}
			if (eval (test, &ctx) != NIL)
				return (eval (iftrue, &ctx));
			return (evalblock (iffalse, ctx));
		}
		if (!strcmp (funcname, "and")) {
			while (istype (expr = cdr (expr), TPAIR))
				if (eval (car (expr), &ctx) == NIL)
					return (NIL);
			return (T);
		}
		if (!strcmp (funcname, "or")) {
			while (istype (expr = cdr (expr), TPAIR))
				if (eval (car (expr), &ctx) == NIL)
					return (T);
			return (NIL);
		}
		if (!strcmp (funcname, "cond")) {
			lisp_t oldctx = ctx, test, clause;
			while (istype (expr = cdr (expr), TPAIR)) {
				if (! istype (clause = car (expr), TPAIR))
					continue;
				ctx = oldctx;
				if (istype (car (clause), TSYMBOL) &&
				    ! strcmp (symname (car (clause)), "else"))
					return (evalblock (cdr (clause), ctx));
				test = eval (car (clause), &ctx);
				if (test == NIL ||
				    ! istype (clause = cdr (clause), TPAIR))
					continue;
				if (istype (car (clause), TSYMBOL) &&
				    ! strcmp (symname (car (clause)), "=>")) {
					clause = evalblock (cdr (clause), ctx);
					if (istype (clause, THARDW))
						return ((*hardwval (clause)) (cons (test, NIL), ctx));
					if (istype (clause, TCLOSURE))
						return (evalclosure (clause, cons (test, NIL)));
					return (NIL);
				}
				return (evalblock (clause, ctx));
			}
			return (NIL);
		}
		if (!strcmp (funcname, "quasiquote")) {
			if (! istype (expr = cdr (expr), TPAIR))
				return (NIL);
			return (quasiquote (car (expr), ctx, 0));
		}
		if (!strcmp (funcname, "unquote") ||
		    !strcmp (funcname, "unquote-splicing")) {
			if (! istype (expr = cdr (expr), TPAIR))
				return (NIL);
			expr = car (expr);
			goto again;
		}
	}

	/* Compute all arguments. */
	expr = evallist (expr, ctx);
	return (evalfunc (car (expr), cdr (expr), ctxp ? *ctxp : TOPLEVEL));
}

void initcontext (functab_t *p)
{
	for (; p->name; ++p)
		ENV = cons (cons (symbol (p->name), hardw (p->func)), ENV);
}

int main (int argc, char **argv)
{
	lisp_t expr, val;
	char *progname, *prog = 0;

	progname = *argv++;
	for (--argc; argc>0 && **argv=='-'; --argc, ++argv) {
		char *p;

		for (p=1+*argv; *p; ++p) switch (*p) {
		case 'h':
			fprintf (stderr, "Usage: %s [-h] [-v] [-t] prog [arg...]\n",
				progname);
			return (0);
		case 't':
			++trace;
			break;
		case 'v':
			++verbose;
			break;
		}
	}
	if (argc > 0) {
		prog = *argv++;
		--argc;
	}

	if (verbose) {
		fprintf (stderr, "Micro Scheme Interpreter, Release 1.0\n");
		fprintf (stderr, "Memory size = %lu bytes\n",
                        (unsigned long) memsz * sizeof (cell_t));
	}

	if (prog && freopen (prog, "r", stdin) != stdin) {
		fprintf (stderr, "Cannot open %s\n", prog);
		return (-1);
	}

	initmem ();
	T = alloc (TBOOL);              /* boolean true #t */
	ZERO = number (0);              /* integer zero */
	ENV = cons (cons (symbol ("version"), number (10)), NIL);
	initcontext (stdfunc);
	for (;;) {
		gc ();
		if (isatty (0))
			printf ("> ");
		expr = getexpr ();
		if (feof (stdin))
			break;
		val = eval (expr, 0);
		if (verbose) {
			putexpr (expr, stdout);
			printf (" --> ");
			putexpr (val, stdout);
			putchar ('\n');
		}
	}
	return (0);
}
