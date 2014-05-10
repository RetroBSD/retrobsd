/*
 * The C preprocessor.
 * This code originates from the V6 preprocessor with some additions
 * from V7 cpp, and at last ansi/c99 support.
 *
 * Copyright (c) 2004,2010 Anders Magnusson (ragge@ludd.luth.se).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifdef CROSS
#   include </usr/include/string.h>
#   include <sys/stat.h>
#   include <time.h>
#else
#   include <sys/wait.h>
#   include <sys/stat.h>
#   include <string.h>
#endif

#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>

#include "cpp.h"
#include "y.tab.h"

#define	MAXARG	20	/* # of args to a macro, limited by char value */
#define	SBSIZE	(12*1024)

static uchar	sbf[SBSIZE];
/* C command */

int tflag;	/* traditional cpp syntax */
#ifdef CPP_DEBUG
int dflag;	/* debug printouts */
#define	DPRINT(x) if (dflag) printf x
#define	DDPRINT(x) if (dflag > 1) printf x
#else
#define DPRINT(x)
#define DDPRINT(x)
#endif

#define	GCC_VARI

int ofd;
uchar outbuf[CPPBUF];
int obufp, istty;
int Cflag, Mflag, dMflag, Pflag;
uchar *Mfile;
struct initar *initar;
int readmac;

/* avoid recursion */
struct recur {
	struct recur *next;
	struct symtab *sp;
};

/* include dirs */
struct incs {
	struct incs *next;
	uchar *dir;
	dev_t dev;
	ino_t ino;
} *incdir[2];
#define	INCINC 0
#define	SYSINC 1

static struct symtab *filloc;
static struct symtab *linloc;
int	trulvl;
int	flslvl;
int	elflvl;
int	elslvl;
uchar *stringbuf = sbf;

/*
 * Macro replacement list syntax:
 * - For object-type macros, replacement strings are stored as-is.
 * - For function-type macros, macro args are substituted for the
 *   character WARN followed by the argument number.
 * - The value element points to the end of the string, to simplify
 *   pushback onto the input queue.
 *
 * The first character (from the end) in the replacement list is
 * the number of arguments:
 *   VARG  - ends with ellipsis, next char is argcount without ellips.
 *   OBJCT - object-type macro
 *   0 	   - empty parenthesis, foo()
 *   1->   - number of args.
 *
 * WARN is used:
 *	- in stored replacement lists to tell that an argument comes
 *	- When expanding replacement lists to tell that the list ended.
 */

#define	GCCARG	0xfd	/* has gcc varargs that may be replaced with 0 */
#define	VARG	0xfe	/* has varargs */
#define	OBJCT	0xff
#define	WARN	1	/* SOH, not legal char */
#define	CONC	2	/* STX, not legal char */
#define	SNUFF	3	/* ETX, not legal char */
#define	NOEXP	4	/* EOT, not legal char */
#define	EXPAND	5	/* ENQ, not legal char */

/* args for lookup() */
#define	FIND	0
#define	ENTER	1

static void expdef(const uchar *proto, struct recur *, int gotwarn);
void define(void);
static int canexpand(struct recur *, struct symtab *np);
void include(void);
void include_next(void);
void line(void);
void flbuf(void);
void usage(void);
static void addidir(char *idir, struct incs **ww);

/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
static size_t
my_strlcpy(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0 && --n != 0) {
		do {
			if ((*d++ = *s++) == 0)
				break;
		} while (--n != 0);
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0)
			*d = '\0';	/* NUL-terminate dst */
		while (*s++)
			;
	}

	return(s - src - 1);	/* count does not include NUL */
}

static uchar *
xstrdup(const char *str)
{
	size_t len = strlen(str)+1;
	uchar *rv;

	rv = malloc(len);
	if (! rv)
		error("xstrdup: out of mem");
	my_strlcpy((char *)rv, str, len);
	return rv;
}

int
main(int argc, char **argv)
{
	struct initar *it;
	struct symtab *nl;
	register int ch;
	const uchar *fn1, *fn2;

#ifdef TIMING
	struct timeval t1, t2;

	(void)gettimeofday(&t1, NULL);
#endif

	while ((ch = getopt(argc, argv, "CD:I:MPS:U:d:i:tvV?")) != -1)
		switch (ch) {
		case 'C': /* Do not discard comments */
			Cflag++;
			break;

		case 'i': /* include */
		case 'U': /* undef */
		case 'D': /* define something */
			/* XXX should not need malloc() here */
			it = malloc(sizeof(struct initar));
			if (it == NULL)
				error("couldn't apply -%c %s", ch, optarg);
			it->type = ch;
			it->str = optarg;
			it->next = initar;
			initar = it;
			break;

		case 'M': /* Generate dependencies for make */
			Mflag++;
			break;

		case 'P': /* Inhibit generation of line numbers */
			Pflag++;
			break;

		case 'S':
		case 'I':
			addidir(optarg, &incdir[ch == 'I' ? INCINC : SYSINC]);
			break;

#ifdef CPP_DEBUG
		case 'V':
			dflag++;
			break;
#endif
		case 'v':
			printf("cpp: %s\n", VERSSTR);
			break;
		case 'd':
			if (optarg[0] == 'M') {
				dMflag = 1;
				Mflag = 1;
			}
			/* ignore others */
			break;

		case 't':
			tflag = 1;
			break;

		case '?':
			usage();
		default:
			error("bad arg %c\n", ch);
		}
	argc -= optind;
	argv += optind;

	filloc = lookup((const uchar *)"__FILE__", ENTER);
	linloc = lookup((const uchar *)"__LINE__", ENTER);
	filloc->value = linloc->value = (const uchar *)""; /* Just something */

	if (tflag == 0) {
		time_t t = time(NULL);
		uchar *n = (uchar *)ctime(&t);

		/*
		 * Manually move in the predefined macros.
		 */
		nl = lookup((const uchar *)"__TIME__", ENTER);
		savch(0); savch('"');  n[19] = 0; savstr(&n[11]); savch('"');
		savch(OBJCT);
		nl->value = stringbuf-1;

		nl = lookup((const uchar *)"__DATE__", ENTER);
		savch(0); savch('"'); n[24] = n[11] = 0; savstr(&n[4]);
		savstr(&n[20]); savch('"'); savch(OBJCT);
		nl->value = stringbuf-1;

		nl = lookup((const uchar *)"__STDC__", ENTER);
		savch(0); savch('1'); savch(OBJCT);
		nl->value = stringbuf-1;

		nl = lookup((const uchar *)"__STDC_VERSION__", ENTER);
		savch(0); savstr((const uchar *)"199901L"); savch(OBJCT);
		nl->value = stringbuf-1;
	}

	if (Mflag && !dMflag) {
		uchar *c;

		if (argc < 1)
			error("-M and no infile");
		if ((c = (uchar *)strrchr(argv[0], '/')) == NULL)
			c = (uchar *)argv[0];
		else
			c++;
		Mfile = stringbuf;
		savstr(c); savch(0);
		if ((c = (uchar *)strrchr((char *)Mfile, '.')) == NULL)
			error("-M and no extension: ");
		c[1] = 'o';
		c[2] = 0;
	}

	if (argc == 2) {
		if ((ofd = open(argv[1], O_WRONLY|O_CREAT, 0600)) < 0)
			error("Can't creat %s", argv[1]);
	} else
		ofd = 1; /* stdout */
	istty = isatty(ofd);

	if (argc && strcmp(argv[0], "-")) {
		fn1 = fn2 = (uchar *)argv[0];
	} else {
		fn1 = NULL;
		fn2 = (const uchar *)"";
	}
	if (pushfile(fn1, fn2, 0, NULL))
		error("cannot open %s", argv[0]);

	flbuf();
	close(ofd);
#ifdef TIMING
	(void)gettimeofday(&t2, NULL);
	t2.tv_sec -= t1.tv_sec;
	t2.tv_usec -= t1.tv_usec;
	if (t2.tv_usec < 0) {
		t2.tv_usec += 1000000;
		t2.tv_sec -= 1;
	}
	fprintf(stderr, "cpp total time: %ld s %ld us\n",
	     t2.tv_sec, t2.tv_usec);
#endif
	return 0;
}

static void
addidir(char *idir, struct incs **ww)
{
	struct incs *w;
	struct stat st;

	if (stat(idir, &st) == -1 || S_ISDIR(st.st_mode) == 0)
		return; /* ignore */
	if (*ww != NULL) {
		for (w = *ww; w->next; w = w->next) {
			if (w->dev == st.st_dev && w->ino == st.st_ino)
				return;
		}
		if (w->dev == st.st_dev && w->ino == st.st_ino)
			return;
		ww = &w->next;
	}
	w = calloc(sizeof(struct incs), 1);
	if (w == NULL)
		error("couldn't add path %s", idir);
	w->dir = (uchar *)idir;
	w->dev = st.st_dev;
	w->ino = st.st_ino;
	*ww = w;
}

uchar *
gotident(struct symtab *nl)
{
	struct symtab *thisnl;
	uchar *osp, *ss2, *base;
	int c;

	thisnl = NULL;
	readmac++;
	base = osp = stringbuf;
	goto found;

	while ((c = sloscan()) != 0) {
		switch (c) {
		case IDENT:
			if (flslvl)
				break;
			osp = stringbuf;

			DPRINT(("IDENT0: %s\n", yytext));
			nl = lookup((uchar *)yytext, FIND);
			if (nl == 0 || thisnl == 0)
				goto found;
			if (thisnl == nl) {
				nl = 0;
				goto found;
			}
			ss2 = stringbuf;
			if ((c = sloscan()) == WSPACE) {
				savstr((uchar *)yytext);
				c = sloscan();
			}
			if (c != EXPAND) {
				unpstr((const uchar *)yytext);
				if (ss2 != stringbuf)
					unpstr(ss2);
				unpstr(nl->namep);
				(void)sloscan(); /* get yytext correct */
				nl = 0; /* ignore */
			} else {
				thisnl = NULL;
				if (nl->value[0] == OBJCT) {
					unpstr(nl->namep);
					(void)sloscan(); /* get yytext correct */
					nl = 0;
				}
			}
			stringbuf = ss2;

found:			if (nl == 0 || subst(nl, NULL) == 0) {
				if (nl)
					savstr(nl->namep);
				else
					savstr((uchar *)yytext);
			} else if (osp != stringbuf) {
				DPRINT(("IDENT1: unput osp %p stringbuf %p\n",
				    osp, stringbuf));
				ss2 = stringbuf;
				cunput(EXPAND);
				while (ss2 > osp)
					cunput(*--ss2);
				thisnl = nl;
				stringbuf = osp; /* clean up heap */
			}
			break;

		case EXPAND:
			DPRINT(("EXPAND!\n"));
			thisnl = NULL;
			break;

		case CMNT:
			getcmnt();
			break;

		case '\n':
			/* sloscan() will not eat */
			(void)cinput();
			savch(c);
			break;

		case STRING:
		case NUMBER:
		case WSPACE:
			savstr((uchar *)yytext);
			break;

		default:
			if (c < 256)
				savch(c);
			else
				savstr((uchar *)yytext);
			break;
		}
		if (thisnl == NULL) {
			readmac--;
			savch(0);
			return base;
		}
	}
	error("premature EOF");
	/* NOTREACHED */
	return NULL; /* XXX gcc */
}

void
line()
{
	static uchar *lbuf;
	static int llen;
	uchar *p;
	int c;

	if ((c = yylex()) != NUMBER)
		goto bad;
	ifiles->lineno = (int)(yylval.node.nd_val - 1);

	if ((c = yylex()) == '\n')
		return;

	if (c != STRING)
		goto bad;

	p = (uchar *)yytext;
	if (*p == 'L')
		p++;
	c = strlen((char *)p);
	if (llen < c) {
		/* XXX may loose heap space */
		lbuf = stringbuf;
		stringbuf += c;
		llen = c;
	}
	p[strlen((char *)p)-1] = 0;
	if (my_strlcpy((char *)lbuf, (char *)&p[1], SBSIZE) >= SBSIZE)
		error("line exceeded buffer size");

	ifiles->fname = lbuf;
	if (yylex() == '\n')
		return;

bad:	error("bad line directive");
}

/*
 * Search for and include next file.
 * Return 1 on success.
 */
static int
fsrch(const uchar *fn, int idx, struct incs *w)
{
	for (; idx <= SYSINC; idx++) {
		if (idx == SYSINC)
			w = incdir[SYSINC];
		for (; w; w = w->next) {
			uchar *nm = stringbuf;

			savstr(w->dir); savch('/');
			savstr(fn); savch(0);
			if (pushfile(nm, fn, idx, w->next) == 0)
				return 1;
			stringbuf = nm;
		}
	}
	return 0;
}

/*
 * Include a file. Include order:
 * - For <...> files, first search -I directories, then system directories.
 * - For "..." files, first search "current" dir, then as <...> files.
 */
void
include()
{
	struct symtab *nl;
	uchar *osp;
	uchar *fn, *safefn;
	int c;

	if (flslvl)
		return;
	osp = stringbuf;

	while ((c = sloscan()) == WSPACE)
		;
	if (c == IDENT) {
		/* sloscan() will not expand idents */
		if ((nl = lookup((uchar *)yytext, FIND)) == NULL)
			goto bad;
		unpstr(gotident(nl));
		stringbuf = osp;
		c = yylex();
	}
	if (c != STRING && c != '<')
		goto bad;

	if (c == '<') {
		fn = stringbuf;
		while ((c = sloscan()) != '>' && c != '\n') {
			if (c == '\n') /* XXX check - cannot reach */
				goto bad;
			savstr((uchar *)yytext);
		}
		savch('\0');
		while ((c = sloscan()) == WSPACE)
			;
		if (c != '\n')
			goto bad;
		safefn = fn;
	} else {
		uchar *nm = stringbuf;

		yytext[strlen(yytext)-1] = 0;
		fn = (uchar *)&yytext[1];
		/* first try to open file relative to previous file */
		/* but only if it is not an absolute path */
		if (*fn != '/') {
			savstr(ifiles->orgfn);
			stringbuf = (uchar *)strrchr((char *)nm, '/');
			if (stringbuf == NULL)
				stringbuf = nm;
			else
				stringbuf++;
		}
		safefn = stringbuf;
		savstr(fn); savch(0);
		c = yylex();
		if (c != '\n')
			goto bad;
		if (pushfile(nm, safefn, 0, NULL) == 0)
			goto okret;
		/* XXX may loose stringbuf space */
	}

	if (fsrch(safefn, INCINC, incdir[INCINC]))
		goto okret;

	error("cannot find '%s'", safefn);
	/* error() do not return */

bad:	error("bad include");
	/* error() do not return */
okret:
	prtline();
}

void
include_next()
{
	struct symtab *nl;
	uchar *osp;
	uchar *fn;
	int c;

	if (flslvl)
		return;
	osp = stringbuf;
	while ((c = sloscan()) == WSPACE)
		;
	if (c == IDENT) {
		/* sloscan() will not expand idents */
		if ((nl = lookup((uchar *)yytext, FIND)) == NULL)
			goto bad;
		unpstr(gotident(nl));
		stringbuf = osp;
		c = yylex();
	}
	if (c != STRING && c != '<')
		goto bad;

	fn = stringbuf;
	if (c == STRING) {
		savstr((uchar *)&yytext[1]);
		stringbuf[-1] = 0;
	} else { /* < > */
		while ((c = sloscan()) != '>') {
			if (c == '\n')
				goto bad;
			savstr((uchar *)yytext);
		}
		savch('\0');
	}
	while ((c = sloscan()) == WSPACE)
		;
	if (c != '\n')
		goto bad;

	if (fsrch(fn, ifiles->idx, ifiles->incs) == 0)
		error("cannot find '%s'", fn);
	prtline();
	return;

bad:	error("bad include");
	/* error() do not return */
}

static int
definp(void)
{
	int c;

	do
		c = sloscan();
	while (c == WSPACE);
	return c;
}

void
getcmnt(void)
{
	int c;

	savstr((uchar *)yytext);
	savch(cinput()); /* Lost * */
	for (;;) {
		c = cinput();
		if (c == '*') {
			c = cinput();
			if (c == '/') {
				savstr((const uchar *)"*/");
				return;
			}
			cunput(c);
			c = '*';
		}
		savch(c);
	}
}

/*
 * Compare two replacement lists, taking in account comments etc.
 */
static int
cmprepl(const uchar *o, const uchar *n)
{
	for (; *o; o--, n--) {
		/* comment skip */
		if (*o == '/' && o[-1] == '*') {
			while (*o != '*' || o[-1] != '/')
				o--;
			o -= 2;
		}
		if (*n == '/' && n[-1] == '*') {
			while (*n != '*' || n[-1] != '/')
				n--;
			n -= 2;
		}
		while (*o == ' ' || *o == '\t')
			o--;
		while (*n == ' ' || *n == '\t')
			n--;
		if (*o != *n)
			return 1;
	}
	return 0;
}

static int
isell(void)
{
	int ch;

	if ((ch = cinput()) != '.') {
		cunput(ch);
		return 0;
	}
	if ((ch = cinput()) != '.') {
		cunput(ch);
		cunput('.');
		return 0;
	}
	return 1;
}

void
define()
{
	struct symtab *np;
	uchar *args[MAXARG], *ubuf, *sbeg;
	int c, i, redef;
	int mkstr = 0, narg = -1;
	int ellips = 0;
#ifdef GCC_VARI
	uchar *gccvari = NULL;
	int wascon;
#endif

	if (flslvl)
		return;
	if (sloscan() != WSPACE || sloscan() != IDENT)
		goto bad;

	if (isdigit((int)yytext[0]))
		goto bad;

	np = lookup((uchar *)yytext, ENTER);
	redef = np->value != NULL;

	readmac = 1;
	sbeg = stringbuf;
	if ((c = sloscan()) == '(') {
		narg = 0;
		/* function-like macros, deal with identifiers */
		c = definp();
		for (;;) {
			if (c == ')')
				break;
			if (c == '.' && isell()) {
				ellips = 1;
				if (definp() != ')')
					goto bad;
				break;
			}
			if (c == IDENT) {
				/* make sure there is no arg of same name */
				for (i = 0; i < narg; i++)
					if (!strcmp((char *) args[i], yytext))
						error("Duplicate macro "
						  "parameter \"%s\"", yytext);
				args[narg++] = xstrdup(yytext);
				if ((c = definp()) == ',') {
					if ((c = definp()) == ')')
						goto bad;
					continue;
				}
#ifdef GCC_VARI
				if (c == '.' && isell()) {
					if (definp() != ')')
						goto bad;
					gccvari = args[--narg];
					break;
				}
#endif
				if (c == ')')
					break;
			}
			goto bad;
		}
		c = sloscan();
	} else if (c == '\n') {
		/* #define foo */
		;
	} else if (c != WSPACE)
		goto bad;

	while (c == WSPACE)
		c = sloscan();

	/* replacement list cannot start with ## operator */
	if (c == '#') {
		if ((c = sloscan()) == '#')
			goto bad;
		savch('\0');
#ifdef GCC_VARI
		wascon = 0;
#endif
		goto in2;
	}

	/* parse replacement-list, substituting arguments */
	savch('\0');
	while (c != '\n') {
#ifdef GCC_VARI
		wascon = 0;
loop:
#endif
		switch (c) {
		case WSPACE:
			/* remove spaces if it surrounds a ## directive */
			ubuf = stringbuf;
			savstr((uchar *)yytext);
			c = sloscan();
			if (c == '#') {
				if ((c = sloscan()) != '#')
					goto in2;
				stringbuf = ubuf;
				savch(CONC);
				if ((c = sloscan()) == WSPACE)
					c = sloscan();
#ifdef GCC_VARI
				if (c == '\n')
					break;
				wascon = 1;
				goto loop;
#endif
			}
			continue;

		case '#':
			c = sloscan();
			if (c == '#') {
				/* concat op */
				savch(CONC);
				if ((c = sloscan()) == WSPACE)
					c = sloscan();
#ifdef GCC_VARI
				if (c == '\n')
					break;
				wascon = 1;
				goto loop;
#else
				continue;
#endif
			}
in2:			if (narg < 0) {
				/* no meaning in object-type macro */
				savch('#');
				continue;
			}
			/* remove spaces between # and arg */
			savch(SNUFF);
			if (c == WSPACE)
				c = sloscan(); /* whitespace, ignore */
			mkstr = 1;
			if (c == IDENT && strcmp(yytext, "__VA_ARGS__") == 0)
				continue;

			/* FALLTHROUGH */
		case IDENT:
			if (strcmp(yytext, "__VA_ARGS__") == 0) {
				if (ellips == 0)
					error("unwanted %s", yytext);
				savch(VARG);
				savch(WARN);
				if (mkstr)
					savch(SNUFF), mkstr = 0;
				break;
			}
			if (narg < 0)
				goto id; /* just add it if object */
			/* check if its an argument */
			for (i = 0; i < narg; i++)
				if (strcmp(yytext, (char *)args[i]) == 0)
					break;
			if (i == narg) {
#ifdef GCC_VARI
				if (gccvari &&
				    strcmp(yytext, (char *)gccvari) == 0) {
					savch(wascon ? GCCARG : VARG);
					savch(WARN);
					if (mkstr)
						savch(SNUFF), mkstr = 0;
					break;
				}
#endif
				if (mkstr)
					error("not argument");
				goto id;
			}
			savch(i);
			savch(WARN);
			if (mkstr)
				savch(SNUFF), mkstr = 0;
			break;

		case CMNT: /* save comments */
			getcmnt();
			break;

		default:
id:			savstr((uchar *)yytext);
			break;
		}
		c = sloscan();
	}
	readmac = 0;
	/* remove trailing whitespace */
	while (stringbuf > sbeg) {
		if (stringbuf[-1] == ' ' || stringbuf[-1] == '\t')
			stringbuf--;
		/* replacement list cannot end with ## operator */
		else if (stringbuf[-1] == CONC)
			goto bad;
		else
			break;
	}
#ifdef GCC_VARI
	if (gccvari) {
		savch(narg);
		savch(VARG);
	} else
#endif
	if (ellips) {
		savch(narg);
		savch(VARG);
	} else
		savch(narg < 0 ? OBJCT : narg);
	if (redef) {
		if (cmprepl(np->value, stringbuf-1))
			error("%s redefined\nprevious define: %s:%d",
			    np->namep, np->file, np->line);
		stringbuf = sbeg;  /* forget this space */
	} else
		np->value = stringbuf-1;

#ifdef CPP_DEBUG
	if (dflag) {
		const uchar *w = np->value;

		printf("!define: ");
		if (*w == OBJCT)
			printf("[object]");
		else if (*w == VARG)
			printf("[VARG%d]", *--w);
		while (*--w) {
			switch (*w) {
			case WARN: printf("<%d>", *--w); break;
			case CONC: printf("<##>"); break;
			case SNUFF: printf("<\">"); break;
			default: putchar(*w); break;
			}
		}
		putchar('\n');
	}
#endif
	for (i = 0; i < narg; i++)
		free(args[i]);
	return;

bad:	error("bad define");
}

void
xwarning(uchar *s)
{
	uchar *t;
	uchar *sb = stringbuf;

	flbuf();
	savch(0);
	if (ifiles != NULL) {
		t = sheap("%s:%d: warning: ", ifiles->fname, ifiles->lineno);
		if (write (2, t, strlen((char *)t)) < 0)
                    /* ignore */;
	}
	if (write (2, s, strlen((char *)s)) < 0)
	    /* ignore */;
	if (write (2, "\n", 1) < 0)
	    /* ignore */;
	stringbuf = sb;
}

void
xerror(uchar *s)
{
	uchar *t;

	flbuf();
	savch(0);
	if (ifiles != NULL) {
		t = sheap("%s:%d: error: ", ifiles->fname, ifiles->lineno);
		if (write (2, t, strlen((char *)t)) < 0)
                    /* ignore */;
	}
	if (write (2, s, strlen((char *)s)) < 0)
	    /* ignore */;
	if (write (2, "\n", 1) < 0)
	    /* ignore */;
	exit(1);
}

/*
 * store a character into the "define" buffer.
 */
void
savch(int c)
{
	if (stringbuf-sbf < SBSIZE) {
		*stringbuf++ = (uchar)c;
	} else {
		stringbuf = sbf; /* need space to write error message */
		error("Too much defining");
	}
}

/*
 * substitute namep for sp->value.
 */
int
subst(struct symtab *sp, struct recur *rp)
{
	struct recur rp2;
	register const uchar *vp, *cp;
	register uchar *obp;
	int c, nl;

	DPRINT(("subst: %s\n", sp->namep));
	/*
	 * First check for special macros.
	 */
	if (sp == filloc) {
		(void)sheap("\"%s\"", ifiles->fname);
		return 1;
	} else if (sp == linloc) {
		(void)sheap("%d", ifiles->lineno);
		return 1;
	}
	vp = sp->value;

	rp2.next = rp;
	rp2.sp = sp;

	if (*vp-- != OBJCT) {
		int gotwarn = 0;

		/* should we be here at all? */
		/* check if identifier is followed by parentheses */

		obp = stringbuf;
		nl = 0;
		do {
			c = cinput();
			*stringbuf++ = (uchar)c;
			if (c == WARN) {
				gotwarn++;
				if (rp == NULL)
					break;
			}
			if (c == '\n')
				nl++;
		} while (c == ' ' || c == '\t' || c == '\n' ||
			    c == '\r' || c == WARN);

		DPRINT(("c %d\n", c));
		if (c == '(' ) {
			cunput(c);
			stringbuf = obp;
			ifiles->lineno += nl;
			expdef(vp, &rp2, gotwarn);
			return 1;
		} else {
	 		*stringbuf = 0;
			unpstr(obp);
			unpstr(sp->namep);
			if ((c = sloscan()) != IDENT)
				error("internal sync error");
			stringbuf = obp;
			return 0;
		}
	} else {
		cunput(WARN);
		cp = vp;
		while (*cp) {
			if (*cp != CONC)
				cunput(*cp);
			cp--;
		}
		expmac(&rp2);
	}
	return 1;
}

/*
 * Maybe an indentifier (for macro expansion).
 */
static int
mayid(uchar *s)
{
	for (; *s; s++)
		if (!isdigit(*s) && !isalpha(*s) && *s != '_')
			return 0;
	return 1;
}

/*
 * do macro-expansion until WARN character read.
 * read from lex buffer and store result on heap.
 * will recurse into lookup() for recursive expansion.
 * when returning all expansions on the token list is done.
 */
void
expmac(struct recur *rp)
{
	struct symtab *nl;
	int c, noexp = 0, orgexp;
	uchar *och, *stksv;

#ifdef CPP_DEBUG
	if (dflag) {
		struct recur *rp2 = rp;
		printf("\nexpmac\n");
		while (rp2) {
			printf("do not expand %s\n", rp2->sp->namep);
			rp2 = rp2->next;
		}
	}
#endif
	readmac++;
	while ((c = sloscan()) != WARN) {
		switch (c) {
		case NOEXP: noexp++; break;
		case EXPAND: noexp--; break;

		case NUMBER: /* handled as ident if no .+- in it */
			if (!mayid((uchar *)yytext))
				goto def;
			/* FALLTHROUGH */
		case IDENT:
			/*
			 * Handle argument concatenation here.
			 * If an identifier is found and directly
			 * after EXPAND or NOEXP then push the
			 * identifier back on the input stream and
			 * call sloscan() again.
			 * Be careful to keep the noexp balance.
			 */
			och = stringbuf;
			savstr((uchar *)yytext);
			DDPRINT(("id: str %s\n", och));

			orgexp = 0;
			while ((c = sloscan()) == EXPAND || c == NOEXP)
				if (c == EXPAND)
					orgexp--;
				else
					orgexp++;

			DDPRINT(("id1: typ %d noexp %d orgexp %d\n",
			    c, noexp, orgexp));
			if (c == IDENT ||
			    (c == NUMBER && mayid((uchar *)yytext))) {
				DDPRINT(("id2: str %s\n", yytext));
				/* OK to always expand here? */
				savstr((uchar *)yytext);
				switch (orgexp) {
				case 0: /* been EXP+NOEXP */
					if (noexp == 0)
						break;
					if (noexp != 1)
						error("case 0");
					cunput(NOEXP);
					noexp = 0;
					break;
				case -1: /* been EXP */
					if (noexp != 1)
						error("case -1");
					noexp = 0;
					break;
				case 1:
					if (noexp != 0)
						error("case 1");
					cunput(NOEXP);
					break;
				default:
					error("orgexp = %d", orgexp);
				}
				unpstr(och);
				stringbuf = och;
				continue; /* New longer identifier */
			}
			unpstr((const uchar *)yytext);
			if (orgexp == -1)
				cunput(EXPAND);
			else if (orgexp == -2)
				cunput(EXPAND), cunput(EXPAND);
			else if (orgexp == 1)
				cunput(NOEXP);
			unpstr(och);
			stringbuf = och;


			sloscan(); /* XXX reget last identifier */

			if ((nl = lookup((uchar *)yytext, FIND)) == NULL)
				goto def;

			if (canexpand(rp, nl) == 0)
				goto def;
			/*
			 * If noexp == 0 then expansion of any macro is
			 * allowed.  If noexp == 1 then expansion of a
			 * fun-like macro is allowed iff there is an
			 * EXPAND between the identifier and the '('.
			 */
			if (noexp == 0) {
				if ((c = subst(nl, rp)) == 0)
					goto def;
				break;
			}
			if (noexp != 1)
				error("bad noexp %d", noexp);
			stksv = NULL;
			if ((c = sloscan()) == WSPACE) {
				stksv = xstrdup(yytext);
				c = sloscan();
			}
			/* only valid for expansion if fun macro */
			if (c == EXPAND && *nl->value != OBJCT) {
				noexp--;
				if (subst(nl, rp))
					break;
				savstr(nl->namep);
				if (stksv)
					savstr(stksv);
			} else {
				unpstr((const uchar *)yytext);
				if (stksv)
					unpstr(stksv);
				savstr(nl->namep);
			}
			if (stksv)
				free(stksv);
			break;

		case CMNT:
			getcmnt();
			break;

		case '\n':
			cinput();
			savch(' ');
			break;

		case STRING:
			/* remove EXPAND/NOEXP from strings */
			if (yytext[1] == NOEXP) {
				savch('"');
				och = (uchar *)&yytext[2];
				while (*och != EXPAND)
					savch(*och++);
				savch('"');
				break;
			}
			/* FALLTHROUGH */

def:		default:
			savstr((uchar *)yytext);
			break;
		}
	}
	if (noexp)
		error("expmac noexp=%d", noexp);
	readmac--;
	DPRINT(("return from expmac\n"));
}

/*
 * expand a function-like macro.
 * vp points to end of replacement-list
 * reads function arguments from sloscan()
 * result is written on top of heap
 */
void
expdef(const uchar *vp, struct recur *rp, int gotwarn)
{
	const uchar **args, *ap, *bp, *sp;
	uchar *sptr;
	int narg, c, i, plev, snuff, instr;
	int ellips = 0, shot = gotwarn;
	size_t allocsz;

	DPRINT(("expdef rp %s\n", (rp ? (const char *)rp->sp->namep : "")));
	c = sloscan();
	if (c != '(')
		error("got %c, expected (", c);

	if (vp[1] == VARG) {
		narg = *vp--;
		ellips = 1;
	} else
		narg = vp[1];

	// The code depends on malloc(0) returning a non-NULL pointer, which is not guaranteed.
	// Workaround it.
	allocsz = sizeof(uchar *) * ((narg+ellips) ? (narg+ellips) : 1);
	args = malloc(allocsz);
	if (args == NULL)
		error("expdef: out of mem");

	/*
	 * read arguments and store them on heap.
	 * will be removed just before return from this function.
	 */
	sptr = stringbuf;
	instr = 0;
	for (i = 0; i < narg && c != ')'; i++) {
		args[i] = stringbuf;
		plev = 0;
		while ((c = sloscan()) == WSPACE || c == '\n')
			if (c == '\n')
				putch(cinput());
		DDPRINT((":AAA (%d)", c));
		if (instr == -1)
			savch(NOEXP), instr = 1;
		if (c == NOEXP)
			instr = 1;
		for (;;) {
			if (plev == 0 && (c == ')' || c == ','))
				break;
			if (c == '(')
				plev++;
			if (c == ')')
				plev--;
			savstr((uchar *)yytext);
			while ((c = sloscan()) == '\n') {
				putch(cinput());
				savch('\n');
			}
			while (c == CMNT) {
				getcmnt();
				c = sloscan();
			}
			if (c == EXPAND)
				instr = 0;
			if (c == 0)
				error("eof in macro");
		}
		while (args[i] < stringbuf &&
		    (stringbuf[-1] == ' ' || stringbuf[-1] == '\t'))
			stringbuf--;
		if (instr == 1)
			savch(EXPAND), instr = -1;
		savch('\0');
	}
	if (ellips)
		args[i] = (const uchar *)"";
	if (ellips && c != ')') {
		args[i] = stringbuf;
		plev = 0;
		instr = 0;
		while ((c = sloscan()) == WSPACE)
			;
		if (c == NOEXP)
			instr++;
		DDPRINT((":AAY (%d)", c));
		for (;;) {
			if (plev == 0 && c == ')')
				break;
			if (c == '(')
				plev++;
			if (c == ')')
				plev--;
			if (plev == 0 && c == ',' && instr) {
				savch(EXPAND);
				savch(',');
				savch(NOEXP);
			} else
				savstr((uchar *)yytext);
			while ((c = sloscan()) == '\n') {
				cinput();
				savch('\n');
			}
			if (c == EXPAND)
				instr--;
		}
		while (args[i] < stringbuf &&
		    (stringbuf[-1] == ' ' || stringbuf[-1] == '\t'))
			stringbuf--;
		savch('\0');

	}
	if (narg == 0 && ellips == 0)
		while ((c = sloscan()) == WSPACE || c == '\n')
			if (c == '\n')
				cinput();

	if (c != ')' || (i != narg && ellips == 0) || (i < narg && ellips == 1))
		error("wrong arg count");

	while (gotwarn--)
		cunput(WARN);

	sp = vp;
	instr = snuff = 0;

	/*
	 * push-back replacement-list onto lex buffer while replacing
	 * arguments.
	 */
	cunput(WARN);
	while (*sp != 0) {
		if (*sp == SNUFF)
			cunput('\"'), snuff ^= 1;
		else if (*sp == CONC)
			;
		else if (*sp == WARN) {

			if (sp[-1] == VARG) {
				bp = ap = args[narg];
				sp--;
#ifdef GCC_VARI
			} else if (sp[-1] == GCCARG) {
				ap = args[narg];
				if (ap[0] == 0)
					ap = (const uchar *)"0";
				bp = ap;
				sp--;
#endif
			} else
				bp = ap = args[(int)*--sp];
			if (sp[2] != CONC && !snuff && sp[-1] != CONC) {
				struct recur *r2 = rp->next;
				cunput(WARN);
				while (*bp)
					bp++;
				while (bp > ap)
					cunput(*--bp);
				DPRINT(("expand arg %d string %s\n", *sp, ap));
				bp = ap = stringbuf;
				savch(NOEXP);
				while (shot && r2)
					r2 = r2->next, shot--;
				expmac(r2);
				savch(EXPAND);
				savch('\0');
			}
			while (*bp)
				bp++;
			while (bp > ap) {
				bp--;
				if (snuff && !instr &&
				    (*bp == ' ' || *bp == '\t' || *bp == '\n')){
					while (*bp == ' ' || *bp == '\t' ||
					    *bp == '\n') {
						bp--;
					}
					cunput(' ');
				}
				if (!snuff || (*bp != EXPAND && *bp != NOEXP))
					cunput(*bp);
				if ((*bp == '\'' || *bp == '"')
				     && bp[-1] != '\\' && snuff) {
					instr ^= 1;
					if (instr == 0 && *bp == '"')
						cunput('\\');
				}
				if (instr && (*bp == '\\' || *bp == '"'))
					cunput('\\');
			}
		} else
			cunput(*sp);
		sp--;
	}
	stringbuf = sptr;

	/* scan the input buffer (until WARN) and save result on heap */
	expmac(rp);
	free(args);
}

uchar *
savstr(const uchar *str)
{
	uchar *rv = stringbuf;

	do {
		if (stringbuf >= &sbf[SBSIZE])   {
			stringbuf = sbf; /* need space to write error message */
			error("out of macro space!");
		}
	} while ((*stringbuf++ = *str++));
	stringbuf--;
	return rv;
}

int
canexpand(struct recur *rp, struct symtab *np)
{
	struct recur *w;

	for (w = rp; w && w->sp != np; w = w->next)
		;
	if (w != NULL)
		return 0;
	return 1;
}

void
unpstr(const uchar *c)
{
	const uchar *d = c;

	while (*d)
		d++;
	while (d > c) {
		cunput(*--d);
	}
}

void
flbuf()
{
	if (obufp == 0)
		return;
	if (Mflag == 0 && write(ofd, outbuf, obufp) < 0)
		error("obuf write error");
	obufp = 0;
}

void
putch(int ch)
{
	outbuf[obufp++] = (uchar)ch;
	if (obufp == CPPBUF || (istty && ch == '\n'))
		flbuf();
}

void
putstr(const uchar *s)
{
	for (; *s; s++) {
		outbuf[obufp++] = *s;
		if (obufp == CPPBUF || (istty && *s == '\n'))
			flbuf();
	}
}

/*
 * convert a number to an ascii string. Store it on the heap.
 */
static void
num2str(int num)
{
	static uchar buf[12];
	uchar *b = buf;
	int m = 0;

	if (num < 0)
		num = -num, m = 1;
	do {
		*b++ = (uchar)(num % 10 + '0');
		num /= 10;
	} while (num);
	if (m)
		*b++ = '-';
	while (b > buf)
		savch(*--b);
}

/*
 * similar to sprintf, but only handles %s and %d.
 * saves result on heap.
 */
uchar *
sheap(const char *fmt, ...)
{
	va_list ap;
	uchar *op = stringbuf;

	va_start(ap, fmt);
	for (; *fmt; fmt++) {
		if (*fmt == '%') {
			fmt++;
			switch (*fmt) {
			case 's':
				savstr(va_arg(ap, uchar *));
				break;
			case 'd':
				num2str(va_arg(ap, int));
				break;
			case 'c':
				savch(va_arg(ap, int));
				break;
			default:
				break; /* cannot call error() here */
			}
		} else
			savch(*fmt);
	}
	va_end(ap);
	*stringbuf = 0;
	return op;
}

void
usage()
{
	error("Usage: cpp [-Cdt] [-Dvar=val] [-Uvar] [-Ipath] [-Spath]");
}

/*
 * Symbol table stuff.
 * The data structure used is a patricia tree implementation.
 */
struct tree {
	int bitno;              /* bit number in the string */
	struct tree *lr[2];     /* left/right element */
};

#define BITNO(x)		((x) & ~(LEFT_IS_LEAF|RIGHT_IS_LEAF))
#define LEFT_IS_LEAF		0x80000000
#define RIGHT_IS_LEAF		0x40000000
#define IS_LEFT_LEAF(x)		(((x) & LEFT_IS_LEAF) != 0)
#define IS_RIGHT_LEAF(x)	(((x) & RIGHT_IS_LEAF) != 0)
#define P_BIT(key, bit)		(key[bit >> 3] >> (bit & 7)) & 1
#define CHECKBITS		8

static struct tree *sympole;
static int numsyms;

/*
 * Allocate a symtab struct and store the string.
 */
static struct symtab *
getsymtab(const uchar *str)
{
	struct symtab *sp = malloc(sizeof(struct symtab));

	if (sp == NULL)
		error("getsymtab: couldn't allocate symtab");
	sp->namep = savstr(str);
	savch('\0');
	sp->value = NULL;
	sp->file = ifiles ? ifiles->orgfn : (const uchar *)"<initial>";
	sp->line = ifiles ? ifiles->lineno : 0;
	return sp;
}

/*
 * Do symbol lookup in a patricia tree.
 * Only do full string matching, no pointer optimisations.
 */
struct symtab *
lookup(const uchar *key, int enterf)
{
	struct symtab *sp;
	struct tree *w, *new, *last;
	int len, cix, bit, fbit, svbit, ix, bitno;
	const uchar *k, *m;

	/* Count full string length */
	for (k = key, len = 0; *k; k++, len++)
		;

	switch (numsyms) {
	case 0: /* no symbols yet */
		if (enterf != ENTER)
			return NULL;
		sympole = (struct tree *)getsymtab(key);
		numsyms++;
		return (struct symtab *)sympole;

	case 1:
		w = sympole;
		break;

	default:
		w = sympole;
		bitno = len * CHECKBITS;
		for (;;) {
			bit = BITNO(w->bitno);
			fbit = bit > bitno ? 0 : P_BIT(key, bit);
			svbit = fbit ? IS_RIGHT_LEAF(w->bitno) :
			    IS_LEFT_LEAF(w->bitno);
			w = w->lr[fbit];
			if (svbit)
				break;
		}
	}

	sp = (struct symtab *)w;

	m = sp->namep;
	k = key;

	/* Check for correct string and return */
	for (cix = 0; *m && *k && *m == *k; m++, k++, cix += CHECKBITS)
		;
	if (*m == 0 && *k == 0) {
		if (enterf != ENTER && sp->value == NULL)
			return NULL;
		return sp;
	}

	if (enterf != ENTER)
		return NULL; /* no string found and do not enter */

	ix = *m ^ *k;
	while ((ix & 1) == 0)
		ix >>= 1, cix++;

	/* Create new node */
	new = malloc(sizeof *new);
	if (! new)
		error("getree: couldn't allocate tree");
	bit = P_BIT(key, cix);
	new->bitno = cix | (bit ? RIGHT_IS_LEAF : LEFT_IS_LEAF);
	new->lr[bit] = (struct tree *)getsymtab(key);

	if (numsyms++ == 1) {
		new->lr[!bit] = sympole;
		new->bitno |= (bit ? LEFT_IS_LEAF : RIGHT_IS_LEAF);
		sympole = new;
		return (struct symtab *)new->lr[bit];
	}

	w = sympole;
	last = NULL;
	svbit = 0; /* XXX gcc */
	for (;;) {
		fbit = w->bitno;
		bitno = BITNO(w->bitno);
		if (bitno == cix)
			error("bitno == cix");
		if (bitno > cix)
			break;
		svbit = P_BIT(key, bitno);
		last = w;
		w = w->lr[svbit];
		if (fbit & (svbit ? RIGHT_IS_LEAF : LEFT_IS_LEAF))
			break;
	}

	new->lr[!bit] = w;
	if (last == NULL) {
		sympole = new;
	} else {
		last->lr[svbit] = new;
		last->bitno &= ~(svbit ? RIGHT_IS_LEAF : LEFT_IS_LEAF);
	}
	if (bitno < cix)
		new->bitno |= (bit ? LEFT_IS_LEAF : RIGHT_IS_LEAF);
	return (struct symtab *)new->lr[bit];
}
