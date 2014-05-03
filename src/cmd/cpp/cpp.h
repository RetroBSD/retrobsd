/*
 * Copyright (c) 2004 Anders Magnusson (ragge@ludd.luth.se).
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
#   include </usr/include/stdio.h>
#   include </usr/include/ctype.h>
#else
#   include <stdio.h> /* for obuf */
#   include <ctype.h>
#endif
#include <stdlib.h>

/* Version string */
#define VERSSTR "cpp for RetroBSD"

typedef unsigned char uchar;
#ifdef YYTEXT_POINTER
extern char *yytext;
#else
extern char yytext[];
#endif
extern uchar *stringbuf;

extern	int	trulvl;
extern	int	flslvl;
extern	int	elflvl;
extern	int	elslvl;
extern	int	tflag, Cflag, Pflag;
extern	int	Mflag, dMflag;
extern	uchar	*Mfile;
extern	int	ofd;

/* args for lookup() */
#define FIND    0
#define ENTER   1

/* buffer used internally */
#define CPPBUF  512

#define	NAMEMAX	CPPBUF	/* currently pushbackbuffer */

/* definition for include file info */
struct includ {
	struct includ *next;
	const uchar *fname;	/* current fn, changed if #line found */
	const uchar *orgfn;	/* current fn, not changed */
	int lineno;
	int infil;
	uchar *curptr;
	uchar *maxread;
	uchar *ostr;
	uchar *buffer;
	int idx;
	void *incs;
	const uchar *fn;
	uchar bbuf[NAMEMAX+CPPBUF+1];
} *ifiles;

/* Symbol table entry  */
struct symtab {
	const uchar *namep;
	const uchar *value;
	const uchar *file;
	int line;
};

struct initar {
	struct initar *next;
	int type;
	char *str;
};

/*
 * Struct used in parse tree evaluation.
 * op is one of:
 *	- number type (NUMBER, UNUMBER)
 *	- zero (0) if divided by zero.
 */
struct nd {
	int op;
	union {
		long long val;
		unsigned long long uval;
	} n;
};

#define nd_val n.val
#define nd_uval n.uval

struct recur;	/* not used outside cpp.c */
int subst(struct symtab *, struct recur *);
struct symtab *lookup(const uchar *namep, int enterf);
uchar *gotident(struct symtab *nl);
int slow;	/* scan slowly for new tokens */

int pushfile(const uchar *fname, const uchar *fn, int idx, void *incs);
void popfile(void);
void prtline(void);
int yylex(void);
int sloscan(void);
void cunput(int);
int curline(void);
char *curfile(void);
void setline(int);
void setfile(char *);
int yyparse(void);
void yyerror(const char *);
void unpstr(const uchar *);
uchar *savstr(const uchar *str);
void savch(int c);
void mainscan(void);
void putch(int);
void putstr(const uchar *s);
void line(void);
uchar *sheap(const char *fmt, ...);
void xwarning(uchar *);
void xerror(uchar *);
#define warning(args...) xwarning(sheap(args))
#define error(args...) xerror(sheap(args))
void expmac(struct recur *);
int cinput(void);
void getcmnt(void);
