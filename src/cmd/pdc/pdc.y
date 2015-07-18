/*
 * pdc.y
 *
 * The programmers desktop calculator. A desktop calculator supporting both
 * shifts and mixed base inputs.
 *
 * Copyright (C) 2001-2005, 2013
 *               Daniel Thompson <see help function for e-mail>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

%{
/* Includes ---------------------------------------------------------------- */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#if defined(HAVE_READLINE)
#include <readline/readline.h>
#include <readline/history.h>
#endif

#if defined(__DJGPP__)
#include <crt0.h>
char** __crt0_glob_function(char *d) { return 0; }
#if !defined(HAVE_READLINE)
void   __crt0_load_environment_file(char *d) { }
#endif
#endif

#if defined(__MINGW32__)
int _CRT_glob = 0;
#endif

/* Types ----------------------------------------------------------- */

typedef struct symbol {
	const char *name;
	const char *description;
	int type;
	union {
		long var;
		long (*func)(long);
	} value;
	struct symbol *next;
} symbol_t;

/* Variables ------------------------------------------------------- */

symbol_t *symbol_table = NULL;
symbol_t initial_symbols[];

const char *version_string = "1.0";

int yyargc;
char **yyargv;

/* are we taking input from the command line? */
static int input_from_cmdline = 0;

/* are we recovering from an internal error */
static int internal_error = 0;

#define LONGBITS (sizeof(long) * CHAR_BIT)
#define CHARMASK ((1 << CHAR_BIT) - 1)

/* Function Prototypes --------------------------------------------- */

int yyerror(char *s);
int yylex(void);
int yyparse();
symbol_t *getsym(const char *name);
symbol_t *putsym(const char *name, int type);
const char *num2str(unsigned long num, int base, int pad);
long print(long x);

%}

/* yylval's structure */
%union {
	long     integer;
	symbol_t *symbol;
}

%token <integer> INTEGER
%token <symbol>  VARIABLE FUNCTION
%type  <integer> expression

/* operators have C conventions for precedence */
%right '=' INC DEC MUL DIV MOD AND XOR OR LEFT RIGHT
%right '?' ':'
%left  LOGICAL_OR
%left  LOGICAL_AND
%left  '|'
%left  '^'
%left  '&'
%left  EQ NE
%left  '<' LE '>' GE
%left  LEFT_SHIFT RIGHT_SHIFT
%left  '+' '-'
%left  '*' '/' '%'
%left  '!' '~' NEG


%% /* YACC grammar follows */

input:	  /* empty */
	| input line
;

line	: expression '\n'		{ getsym("ans")->value.var = print($1); }
	| expression ','		{ getsym("ans")->value.var = $1; }
	| '\n'				/* do nothing */
	| error '\n'			{ yyerrok; }
;

expression:
	  INTEGER			{ $$ = $1; }
	| VARIABLE			{ $$ = $1->value.var; }
	| FUNCTION '(' expression ')'	{ $$ = (*($1->value.func))($3); }
	| FUNCTION expression           { $$ = (*($1->value.func))($2); }
	| FUNCTION			{ $$ = (*($1->value.func))(getsym("ans")->value.var); }
	| VARIABLE '=' expression	{ $$ = $1->value.var = $3; }
	| VARIABLE INC expression	{ $$ = $1->value.var += $3; }
	| VARIABLE DEC expression	{ $$ = $1->value.var -= $3; }
	| VARIABLE MUL expression	{ $$ = $1->value.var *= $3; }
	| VARIABLE DIV expression	{ if (0 != $3) {
					          $$ = $1->value.var /= $3;
					  } else {
					          yyerror("divide by zero error");
						  internal_error = 1;
						  $$ = 0;
					  }
					}
	| VARIABLE MOD expression	{ $$ = $1->value.var %= $3; }
	| VARIABLE AND expression	{ $$ = $1->value.var &= $3; }
	| VARIABLE XOR expression	{ $$ = $1->value.var ^= $3; }
	| VARIABLE OR expression	{ $$ = $1->value.var |= $3; }
	| VARIABLE LEFT expression	{ $$ = $1->value.var <<= $3; }
	| VARIABLE RIGHT expression	{ $$ = $1->value.var >>= $3; }
	| expression '?' expression ':' expression
					{ $$ = $1 ? $3 : $5; }
	| expression '+' expression	{ $$ = $1 + $3; }
	| expression '-' expression	{ $$ = $1 - $3; }
	| expression '|' expression	{ $$ = $1 | $3; }
	| expression '^' expression	{ $$ = $1 ^ $3; }
	| expression '*' expression	{ $$ = $1 * $3; }
	| expression '/' expression	{ if (0 != $3) {
					          $$ = $1 / $3;
					  } else {
						  yyerror("divide by zero error");
						  internal_error = 1;
						  $$ = 0;
					  }
					}
	| expression '%' expression	{ $$ = $1 % $3; }
	| expression '&' expression	{ $$ = $1 & $3; }
	| expression '<' expression     { $$ = $1 < $3; }
	| expression '>' expression     { $$ = $1 > $3; }
	| expression EQ expression	{ $$ = $1 == $3; }
	| expression LE expression      { $$ = $1 <= $3; }
	| expression GE expression      { $$ = $1 >= $3; }
	| expression NE expression	{ $$ = $1 != $3; }
	| '~' expression		{ $$ = ~$2; }
	| '-' expression %prec NEG	{ $$ = -$2; }
	| expression LEFT_SHIFT expression
					{ $$ = $1 << $3; }
	| expression RIGHT_SHIFT expression
					{ $$ = $1 >> $3; }
	| expression LOGICAL_AND expression
					{ $$ = $1 && $3; }
	| expression LOGICAL_OR expression
					{ $$ = $1 || $3; }
	| '!' expression		{ $$ = !$2; }
	| '(' expression ')'		{ $$ = $2; }
;

%%

/* Functions --------------------------------------------------------------- */

#if defined(HAVE_READLINE)
int yygetc(void)
{
	static char *line, *pos;

	if (!line) {
		line = pos = readline("> ");
		if (!line) {
			return EOF;
		}

		/* if the line has any text in it, save it in the history */
		if ('\0' != *line) {
			add_history(line);
		}
	}

	if ('\0' == *pos) {
		free(line);
		line = 0;
		return '\n';
	}

	return *pos++;
}
#elif defined(HAVE_CMDEDIT)
/* Use the editing and history features provided by CmdEdit. */
#include <dpmi.h>
#include <go32.h>
#include <sys/farptr.h>
int yygetc(void)
{
	static char line[256], *pos = line;

	if (pos == line) {
		fputs("> ", stdout);
		fflush(stdout);
		__dpmi_regs regs;
		regs.h.ah = 0x0a;	/* buffered input */
		regs.x.ds = __tb >> 4;
		regs.x.dx = __tb & 0x0f;
		_farpokeb(_dos_ds, __tb, 255);
		_farpokeb(_dos_ds, __tb+1, 0);
		__dpmi_int(0x21, &regs);
		dosmemget(__tb+2, 256, line);
		putchar('\n');
		pos = line;
		if ('\x1a' == *pos) {
			return EOF;
		}
	}

	if ('\r' == *pos) {
		pos = line;
		return '\n';
	}

	return *pos++;
}
#else
int yygetc(void)
{
	static int lastch = '\n';

	/* manage the prompt */
	if ('\n' == lastch) {
		printf("> ");
		fflush(stdout);
	}

	lastch = getchar();
	return lastch;
}
#endif

int yyungetc;
int yygetchar(void)
{
	if ('\0' != yyungetc) {
		int ch = yyungetc;
		yyungetc = '\0';
		return ch;
	}

	if (input_from_cmdline) {
		static int arg=1, pos=0;

		if (arg >= yyargc) {
			return (arg++ == yyargc ? '\n' : EOF);
		}

		if ('\0' == yyargv[arg][pos]) {
			arg++;
			pos = 0;
			return ' ';
		}

		return yyargv[arg][pos++];
	}

	return yygetc();
}

int yyerror(char *s)
{
	printf("%s\n", s);
	fflush(stdout);
	return 0;
}

int yylex(void)
{
	int c, i;

	/* ignore whitespace */
	do {
		c = yygetchar();
	} while (strchr(" \t", c));

	/* handle end of input */
	if (EOF == c) {
		return 0;
	}

	/* handle numeric types */
	if (isdigit(c)) {
		int base;
		int nDigits;

		/* determine the base of this number */
		if (c != '0') {
			base = getsym("ibase")->value.var;
			yyungetc = c;
		} else {
			c = yygetchar();
			switch(c) {
			case 'd':
			case 'D':
				base = 10;
				break;
			case 'x':
			case 'X':
				base = 16;
				break;
			case 'b':
			case 'B':
				base = 2;
				break;
			default:
				base = 8;
				yyungetc = c;
			}
		}

		yylval.integer = 0;
		nDigits = 0;
		c = yygetchar();
		while (EOF != c && isxdigit(c)) {
			unsigned digit = (c <= '9') ?
                                         (c & 0xf) :
                                         (((c - 'A') & 7) + 10);
			if (digit >= base) {
				break;
			}

			nDigits++;
			yylval.integer *= base;
			yylval.integer += digit;

			c = yygetchar();
		}

		switch (c) {
		case 'K':
			yylval.integer *= 1024;
			break;
		case 'M':
			yylval.integer *= 1024*1024;
			break;
		case 'G':
			yylval.integer *= 1024*1024*1024;
			break;
		case 'b':
		case 'B':
		case '_':
			yylval.integer = 1l << yylval.integer;
			break;
		default:
			if (0 == nDigits && 2 == base)
				yylval.integer = 1;
			yyungetc = c;
		}

		return INTEGER;
	}

	/* handle single quoted strings including multi-character
	 * constants (a common extension to ANSI C)
	 */
	if ('\'' == c) {
		yylval.integer = 0;

		for (c = yygetchar(); EOF != c && '\'' != c; c = yygetchar()) {
			yylval.integer = (yylval.integer << 8) + (c & 255);
		}

		return INTEGER;
	}

	/* handle identifiers */
	if (isalpha(c)) {
		symbol_t *sym;
		static char *buf = NULL; /* this is allocated only once */
		static int length = 0;

		if (NULL == buf) {
			length = 40;
			buf = malloc(length + 1);
		}

		i = 0;
		do {
			/* grow the buffer if it is too small */
			if (i == length) {
				length *= 2;
				buf = realloc(buf, length +1);
			}

			buf[i++] = c;
			c = yygetchar();
		} while ((EOF != c) && isalnum(c));

		yyungetc = c;
		buf[i] = '\0';

		/* look up (or generate) the symbol */
		if (NULL == (sym = getsym(buf))) {
			sym = putsym(buf, VARIABLE);
		}
		yylval.symbol = sym;
		return sym->type;
	}

	/* check for the shift and logical operators */
	if (strchr("<>&|=+-*/%^!", c)) {
		int d = yygetchar();
		if (c == d) {
			switch (c) {
			case '<':
			case '>':
				d = yygetchar();
				if ('=' == d) {
					return ('<' == c ? LEFT : RIGHT);
				}
				yyungetc = d;
				return ('<' == c ? LEFT_SHIFT : RIGHT_SHIFT);
			case '&': return LOGICAL_AND;
			case '|': return LOGICAL_OR;
			case '=': return EQ;
			}
		} else if (d == '=') {
			switch (c) {
			case '<': return LE;
			case '>': return GE;
			case '!': return NE;
			case '+': return INC;
			case '-': return DEC;
			case '*': return MUL;
			case '/': return DIV;
			case '%': return MOD;
			case '&': return AND;
			case '^': return XOR;
			case '|': return OR;
			}
		} else {
			yyungetc = d;
		}
	}

	/* this is a single character terminal */
	return c;
}

long defaultfunc(long i)
{
	fprintf(stderr, "internal error\n");
	exit(0);

	return 0;
}

symbol_t *putsym(const char *name, int type)
{
	symbol_t *sym;

	sym = (symbol_t*) malloc(sizeof(*sym));
	if (NULL == sym) {
		return NULL;
	}

	sym->name = strdup(name);
	if (NULL == sym->name) {
		free(sym);
		return NULL;
	}

	sym->type = type;
	switch(type) {
	case VARIABLE:
		sym->value.var = 0;
		break;
	case FUNCTION:
		sym->value.func = defaultfunc;
		break;
	}

	/* Can't think of a good way to set that at present. */
	sym->description = NULL;
	sym->next = symbol_table;
	symbol_table = sym;

	return sym;
}

symbol_t *getsym(const char *name)
{
	symbol_t *p;

	for (p = symbol_table; p != NULL; p = p->next) {
		if (0 == strcmp(p->name, name)) {
			return p;
		}
	}

	return NULL;
}

const char *num2str(unsigned long num, int base, int pad) {
	static const char lookup[] = "0123456789abcdef";
	static char str[LONGBITS + 1];
	char *pStr, *padStr;

	/* check for unsupported bases */
	if (base < 2 || base >= sizeof(lookup)) {
		printf("(bad obase, assuming base 10)\n\t");
		base = 10;
	}

	/* and illegal pad lengths */
	if (pad < 1 || pad > LONGBITS) {
		printf("(bad pad, assuming pad 1)\n\t");
		pad = 1;
	}

	/* pad str with zeros */
	memset(str, '0', sizeof(str));

	pStr = &str[sizeof(str)];
	*--pStr = '\0';
	padStr = pStr - pad;

	do {
		*--pStr = lookup[num % base];
		num /= base;
	} while (num);

	return (padStr < pStr ? padStr : pStr);
}

long ascii(long x)
{
	long w;
	int  b;

	printf("\t'");
	if (0 == x) {
		printf("\\0");
	} else {
		/* ignore leading NILs */
		b = LONGBITS / CHAR_BIT;
		w = x;
		while (0 == ((w >> (LONGBITS - CHAR_BIT)) & CHARMASK)) {
			--b;
			w <<= CHAR_BIT;
		}
		for (; b > 0; w <<= CHAR_BIT, --b) {
			char c = (w >> (LONGBITS - CHAR_BIT)) & CHARMASK;
			if (0 == c) {
				printf("\\0");
			} else {
				printf(isprint(c) ? "%c" : "\\x%02x", c);
			}
		}
	}
	printf("'\n\n");

	return x;
}

long bitcnt(long x)
{
	long b;
	for (b=0; x!=0; b++) {
		x &= x-1; /* clear least significant (set) bit */
	}
	return b;
}

long bitfield(long x)
{
	symbol_t *N = getsym("N");
	long result;

	if (0 == N->value.var) {
		printf("WARNING: N is zero - automatically setting N to ans\n\n");
		N->value.var = getsym("ans")->value.var;
	}

	result = N->value.var & ~(-1l << x);
	N->value.var >>= x;

	/* force logical shift on all machines */
	N->value.var &= ~(-1l << (LONGBITS - x));

	return result;
}

long decompose(long x)
{
	char *separator = "";
	long i;

	if (0 != x) {
		printf("\t");

		for (i = LONGBITS - 1; i >= 0; i--) {
			if (0 != (x & (1l<<i))) {
				printf("%s%ld", separator, i);
				separator = ", ";
			}
		}

		printf("\n\n");
	} else {
		printf("\tNo set bits\n\n");
	}

	return x;
}

long lssb(long x)
{
	if (0 == x) {
		return -1;
	}

	x ^= x-1; /* isolate the least significant bit */
	return bitcnt(x-1);
}

long mssb(long x)
{
	long i;

	for (i = LONGBITS - 1; i >= 0; i--) {
		if (0 != (x & (1l<<i))) {
			return i;
		}
	}

	return -1;
}

long swap32(long d)
{
	return (d >> 24 & 0x000000ff) |
	       (d >>  8 & 0x0000ff00) |
	       (d <<  8 & 0x00ff0000) |
	       (d << 24 & 0xff000000);
}

long labs(long d)
{
	if (d < 0)
		return -d;
	return d;
}

long quit(long ret)
{
	exit((int) ret);
	return 0;
}

long print(long x)
{
	long obase = getsym("obase")->value.var;
	int pad = abs(getsym("pad")->value.var);

	if (internal_error) {
		internal_error = 0;
		return 0;
	}

	if (!input_from_cmdline) {
		printf("\t");
	}

	/* print the prefixes */
	switch(obase) {
	case 16:
		printf("0x");
		break;
	case 10:
		break;
	case 8:
		printf("0");
		break;
	case 2:
		printf("0b");
		break;
	case 0:
	case 1:
		break;
	default:
		printf("[base %ld] ", obase);
	}

	/* now print the actual values */
	switch(obase) {
	case 10:
		/* print base 10 values directly to keep signedness */
		printf("%0*ld\n", pad, x);
		break;
	case 1:
		/* special case base 1 (print dex, hex and binary) */
		printf("%0*ld\t[0x%0*lx]\t[0b%s]\n", pad, x, pad, x, num2str(x, 2, LONGBITS));
		break;
	case 0:
		/* special case base 0 (print dec and hex) */
		printf("%0*ld\t[0x%0*lx]\n", pad, x, pad, x);
		break;
	default:
		printf("%s\n", num2str(x, obase, pad));
	}

	return x;
}

void print_help(long mode)
{
	printf(
"pdc %s - the programmers desktop calculator\n"
"\n"
"Copyright (C) 2001-2005, 2013 Daniel Thompson <daniel\100redfelineninja\056org\056uk>\n"
"This is free software with ABSOLUTELY NO WARRANTY.\n"
"For details type `warranty'.\n"
"\n",
		version_string);

	if (1 == mode) {
		symbol_t *sym;
		printf(
"Contributors:\n"
"  Daniel Thompson          <d\056thompson\100gmx\056net>\n"
"  Paul Walker              <paul\100blacksun\056org\056uk>\n"
"  Jason Hood               <jadoxa\100yahoo\056com\056au>\n"
"\n"
		);
		printf("Variables:\n");
		for (sym=initial_symbols; NULL != sym->name; sym++) {
			if (VARIABLE == sym->type) {
				printf("  %-9s - %s\n", sym->name, sym->description);
			}
		}
		printf("\nFunctions:\n");
		for (sym=initial_symbols; NULL != sym->name; sym++) {
			if (FUNCTION == sym->type) {
				printf("  %-9s - %s\n", sym->name, sym->description);
			}
		}
		printf("\n");

		if (input_from_cmdline) {
			printf(
"Usage:\n"
"  pdc [<expression>] [, <expression>] ...\n"
"\n"
"  Without arguments pdc enters interactive mode; otherwise it evaluates its\n"
"  arguments and prints the result. Expressions are separated using the ,\n"
"  operator (e.g. 'pdc obase=2, 4*12'). Only the last expression evaluated\n"
"  will be printed automatically. Use the print function to display\n"
"  intermediate values if required.\n"
"\n"
			);
		}
	}

	if (2 == mode) {
		printf(
"This program is free software; you can redistribute it and/or modify\n"
"it under the terms of the GNU General Public License as published by\n"
"the Free Software Foundation; either version 2 of the License, or\n"
"(at your option) any later version.\n"
"\n"
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU General Public License for more details.\n"
"\n"
"You should have received a copy of the GNU General Public License along\n"
"with this program; if not, write to the Free Software Foundation, Inc.,\n"
"51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.\n"
"\n"
"Or see https://www.gnu.org/licenses/old-licenses/gpl-2.0.html\n"
"\n"
		);
	}
	if (3 == mode) {
		symbol_t *sym;

		printf("Compiled:           "__TIME__" "__DATE__"\n");
		printf("Built-in symbols:  ");
		for (sym=initial_symbols; NULL != sym->name; sym++) {
			printf(" %s%s", sym->name, (FUNCTION == sym->type ? "()" : ""));
		}
		printf("\n\n");
	}

}

long help(long ans)
{
	print_help(1);
	return ans;
}

long version(long ans)
{
	print_help(3);
	return ans;
}

long warranty(long ans)
{
	print_help(2);
	return ans;
}

#define BASE_FN(name, base) \
long name(long ans) \
{ \
	getsym("obase")->value.var = base; \
	return ans; \
}

BASE_FN(dechex, 0)
BASE_FN(dxb, 1)
BASE_FN(bin, 2)
BASE_FN(oct, 8)
BASE_FN(dec, 10)
BASE_FN(hex, 16)

/* Print routine in the help function isn't too clever, so VARIABLE and
 * FUNCTION should be grouped together, otherwise the display looks a
 * bit weird. Not hard to fix, but probably more work than it's worth.
 */
symbol_t initial_symbols[] = {
{ "ans",	"the result of the previous calculation",		VARIABLE, { 0              }, NULL },
{ "ibase",	"the default input base (to force decimal use 0d10)",	VARIABLE, { 10             }, NULL },
{ "obase",	"the output base (set to zero or one for combined bases)", VARIABLE, { 0              }, NULL },
{ "pad",	"the amount of zero padding used when displaying numbers", VARIABLE, { 1           }, NULL },
{ "N",          "global variable used by the bitfield function",	VARIABLE, { 0              }, NULL },
{ "abs",	"get the absolute value of x",				FUNCTION, { (long) labs    }, NULL },
{ "ascii",	"convert x into a character constant",			FUNCTION, { (long) ascii   }, NULL },
{ "bin",	"change output base to binary",				FUNCTION, { (long) bin     }, NULL },
{ "bitcnt",	"get the population count of x",			FUNCTION, { (long) bitcnt  }, NULL },
{ "bitfield",	"extract the bottom x bits of N and shift N",		FUNCTION, { (long) bitfield}, NULL },
{ "bits",	"alias for decompose",					FUNCTION, {(long) decompose}, NULL },
{ "dec",	"set the output base to decimal",			FUNCTION, { (long) dec     }, NULL },
{ "decompose",	"decompose x into a list of bits set",			FUNCTION, {(long) decompose}, NULL },
{ "default",	"set the default output base (decimal and hex)",	FUNCTION, { (long) dechex  }, NULL },
{ "dxb",	"output in decimal, hex and binary",			FUNCTION, { (long) dxb     }, NULL },
{ "help",	"display this help message",				FUNCTION, { (long) help    }, NULL },
{ "hex",	"change output base to hex",				FUNCTION, { (long) hex     }, NULL },
{ "lssb",	"get the least significant set bit in x",		FUNCTION, { (long) lssb    }, NULL },
{ "mssb",	"get the most significant set bit in x",		FUNCTION, { (long) mssb    }, NULL },
{ "oct",	"change output base to octal",				FUNCTION, { (long) oct     }, NULL },
{ "print",	"print an expression (useful for command line work)",   FUNCTION, { (long) print   }, NULL },
{ "quit",	"leave pdc",						FUNCTION, { (long) quit    }, NULL },
{ "swap32",	"perform a 32-bit byte swap",				FUNCTION, { (long) swap32  }, NULL },
{ "version",	"display version information",				FUNCTION, { (long) version }, NULL },
{ "warranty",	"display warranty and licencing information",		FUNCTION, { (long) warranty}, NULL },
{ "K",		"Defaults to KiB (1024 bytes)",				VARIABLE, { 1024 },           NULL },
{ "M",		"Defaults to MiB (1048576 bytes)",			VARIABLE, { 1024  * 1024 },   NULL },
{ "G",		"Defaults to GiB (1073741824 bytes)",			VARIABLE, { 1073741824 },     NULL },
{ NULL,		"",							0, 	  { 0              }, NULL }
};

int main(int argc, char *argv[])
{
	int i;

	/* setup the initial symbol table */
	for (i=1; NULL != initial_symbols[i].name; i++) {
		initial_symbols[i].next = &initial_symbols[i-1];
	}
	symbol_table = &initial_symbols[i-1];

	/* run in non-interactive mode if we have command line arguments */
	if (argc > 1) {
		input_from_cmdline = 1;
		yyargc = argc;
		yyargv = argv;
	} else {
		print_help(0);
	}

	/* run the calculator */
	yyparse();

#if defined(HAVE_READLINE) || (!defined(__DJGPP__) && !defined(__MINGW32__))
	if (!input_from_cmdline) {
		/* shutdown cleanly after a ^D */
		printf("\n");
	}
#endif

	return 0;
}
