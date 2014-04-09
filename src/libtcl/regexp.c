/*
 * regcomp and regexec -- regsub is elsewhere
 *
 *	Copyright (c) 1986 by University of Toronto.
 *	Written by Henry Spencer.  Not derived from licensed software.
 *
 *	Permission is granted to anyone to use this software for any
 *	purpose on any computer system, and to redistribute it freely,
 *	subject to the following restrictions:
 *
 *	1. The author is not responsible for the consequences of use of
 *		this software, no matter how awful, even if they arise
 *		from defects in it.
 *
 *	2. The origin of this software must not be misrepresented, either
 *		by explicit claim or by omission.
 *
 *	3. Altered versions must be plainly marked as such, and must not
 *		be misrepresented as being the original software.
 *** THIS IS AN ALTERED VERSION.  It was altered by Serge Vakulenko,
 *** vak@cronyx.ru, on 10 Novc 2002, to make it thread-safe.
 *** THIS IS AN ALTERED VERSION.  It was altered by John Gilmore,
 *** hoptoad!gnu, on 27 Dec 1986, to add \n as an alternative to |
 *** to assist in implementing egrep.
 *** THIS IS AN ALTERED VERSION.  It was altered by John Gilmore,
 *** hoptoad!gnu, on 27 Dec 1986, to add \< and \> for word-matching
 *** as in BSD grep and ex.
 *** THIS IS AN ALTERED VERSION.  It was altered by John Gilmore,
 *** hoptoad!gnu, on 28 Dec 1986, to optimize characters quoted with \.
 *** THIS IS AN ALTERED VERSION.  It was altered by James A. Woods,
 *** ames!jaw, on 19 June 1987, to quash a regcomp() redundancy.
 *
 * Beware that some of this code is subtly aware of the way operator
 * precedence is structured in regular expressions.  Serious changes in
 * regular-expression syntax might require a total rethink.
 */
#include <stdlib.h>
#include <string.h>
#include "regexp.h"
#include "regpriv.h"

/*
 * Structure for regexp "program".  This is essentially a linear encoding
 * of a nondeterministic finite-state machine (aka syntax charts or
 * "railroad normal form" in parsing technology).  Each node is an opcode
 * plus a "next" pointer, possibly plus an operand.  "Next" pointers of
 * all nodes except BRANCH implement concatenation; a "next" pointer with
 * a BRANCH on both ends of it is connecting two alternatives.  (Here we
 * have one of the subtle syntax dependencies:  an individual BRANCH (as
 * opposed to a collection of them) is never concatenated with anything
 * because of operator precedence.)  The operand of some types of node is
 * a literal string; for others, it is a node leading into a sub-FSM.  In
 * particular, the operand of a BRANCH node is the first node of the branch.
 * (NB this is *not* a tree structure:  the tail of the branch connects
 * to the thing following the set of BRANCHes.)  The opcodes are:
 */

/* definition	number	opnd?	meaning */
#define	END	0	/* no	End of program. */
#define	BOL	1	/* no	Match "" at beginning of line. */
#define	EOL	2	/* no	Match "" at end of line. */
#define	ANY	3	/* no	Match any one character. */
#define	ANYOF	4	/* str	Match any character in this string. */
#define	ANYBUT	5	/* str	Match any character not in this string. */
#define	BRANCH	6	/* node	Match this alternative, or the next... */
#define	BACK	7	/* no	Match "", "next" ptr points backward. */
#define	EXACTLY	8	/* str	Match this string. */
#define	NOTHING	9	/* no	Match empty string. */
#define	STAR	10	/* node	Match this (simple) thing 0 or more times. */
#define	PLUS	11	/* node	Match this (simple) thing 1 or more times. */
#define	WORDA	12	/* no	Match "" at wordchar, where prev is nonword */
#define	WORDZ	13	/* no	Match "" at nonwordchar, where prev is word */
#define	OPEN	20	/* no	Mark this point in input as start of #n. */
			/*	OPEN+1 is number 1, etc. */
#define	CLOSE	30	/* no	Analogous to OPEN. */

/*
 * Opcode notes:
 *
 * BRANCH	The set of branches constituting a single choice are hooked
 *		together with their "next" pointers, since precedence prevents
 *		anything being concatenated to any individual branch.  The
 *		"next" pointer of the last BRANCH in a choice points to the
 *		thing following the whole choice.  This is also where the
 *		final "next" pointer of each individual branch points; each
 *		branch starts with the operand node of a BRANCH node.
 *
 * BACK		Normal "next" pointers all implicitly point forward; BACK
 *		exists to make loop structures possible.
 *
 * STAR,PLUS	'?', and complex '*' and '+', are implemented as circular
 *		BRANCH structures using BACK.  Simple cases (one character
 *		per match) are implemented with STAR and PLUS for speed
 *		and to minimize recursive plunges.
 *
 * OPEN,CLOSE	...are numbered at compile time.
 */

/*
 * A node is one char of opcode followed by two chars of "next" pointer.
 * "Next" pointers are stored as two 8-bit pieces, high order first.  The
 * value is a positive offset from the opcode of the node containing it.
 * An operand, if any, simply follows the node.  (Note that much of the
 * code generation knows about this implicit relationship.)
 *
 * Using two bytes for the "next" pointer is vast overkill for most things,
 * but allows patterns to get big without disasters.
 */
#define	OP(p)		(*(p))
#define	NEXT(p)		(((*((p)+1)&0377)<<8) + (*((p)+2)&0377))
#define	OPERAND(p)	((p) + 3)

/*
 * Utility definitions.
 */
#define	ISMULT(c)	((c) == '*' || (c) == '+' || (c) == '?')
#define ISALNUM(c)	(((c)>='a' && (c)<='z') || \
			((c)>='A' && (c)<='Z') || \
			((c)>='0' && (c)<='9') || \
			((c)>=0300))

/*
 * Flags to be passed up and down.
 */
#define	HASWIDTH	01	/* Known never to match null string. */
#define	SIMPLE		02	/* Simple enough to be STAR/PLUS operand. */
#define	SPSTART		04	/* Starts with * or +. */
#define	WORST		0	/* Worst case. */

/*
 * Global work variables for regcomp().
 */
typedef struct {
	unsigned const char	*parse;	/* Input-scan pointer. */
	unsigned char	npar;	/* () count. */
	unsigned char	*code;	/* Code-emit pointer; &regdummy = don't. */
	unsigned short	size;	/* Code size. */
} compile_t;

static unsigned char regdummy;

/*
 * Forward declarations for regcomp()'s friends.
 */
static unsigned char *reg (compile_t *x, unsigned char paren, unsigned char *flagp);
static unsigned char *regbranch (compile_t *x, unsigned char *flagp);
static unsigned char *regpiece (compile_t *x, unsigned char *flagp);
static unsigned char *regatom (compile_t *x, unsigned char *flagp);
static unsigned char *regnode (compile_t *x, unsigned char op);
static void regc (compile_t *x, unsigned char b);
static void reginsert (compile_t *x, unsigned char op, unsigned char *opnd);
static unsigned char *regnext (unsigned char *p);
static void regtail (unsigned char *p, unsigned char *val);
static void regoptail (unsigned char *p, unsigned char *val);

/*
 * Global work variables for regexec().
 */
typedef struct {
	const unsigned char	*input;	/* String-input pointer. */
	const unsigned char	*bol;	/* Beginning of input, for ^ check. */
	const unsigned char	**startp; /* Pointer to startp array. */
	const unsigned char	**endp;	/* Ditto for endp. */
} execute_t;

/*
 * Forwards.
 */
static unsigned char regtry (regexp_t *prog, execute_t *z, const unsigned char *string);
static unsigned char regmatch (execute_t *z, unsigned char *prog);
static unsigned short regrepeat (execute_t *z, unsigned char *p);

#ifdef DEBUG_REGEXP
#include <stdio.h>
static unsigned char *regprop (unsigned char *op);
#endif

/*
 * Determine the required size.
 * On failure, returns 0.
 */
unsigned
regexp_size (const unsigned char *exp)
{
	compile_t x;
	unsigned char flags;

	if (! exp) {
		/* FAIL("NULL argument"); */
		return 0;
	}

	x.parse = exp;
	x.npar = 1;
	x.size = 0L;
	x.code = &regdummy;
	regc (&x, MAGIC);
	if (! reg (&x, 0, &flags))
		return 0;

	return sizeof (regexp_t) + x.size;
}

/*
 * Compile a regular expression into internal code.
 * Returns 1 on success, or 0 on failure.
 *
 * We can't allocate space until we know how big the compiled form will be,
 * but we can't compile it (and thus know how big it is) until we've got a
 * place to put the code.  So we cheat:  we compile it twice, once with code
 * generation turned off and size counting turned on, and once "for real".
 * This also means that we don't allocate space until we are sure that the
 * thing really will compile successfully, and we never have to move the
 * code and thus invalidate pointers into it.  (Note that it has to be in
 * one piece because free() must be able to free it all.)
 *
 * Beware that the optimization-preparation code in here knows about some
 * of the structure of the compiled regexp.
 */
bool_t
regexp_compile (regexp_t *r, const unsigned char *exp)
{
	unsigned char *scan;
	unsigned char *longest;
	compile_t x;
	unsigned short len;
	unsigned char flags;

	if (! r || ! exp) {
		/* FAIL("NULL argument"); */
		return 0;
	}

	/* Second pass: emit code. */
	x.parse = exp;
	x.npar = 1;
	x.code = r->program;
	regc (&x, MAGIC);
	if (! reg (&x, 0, &flags))
		return 0;

	/* Dig out information for optimizations. */
	r->start = '\0';	/* Worst-case defaults. */
	r->anchor = 0;
	r->must = 0;
	r->mustlen = 0;
	scan = r->program+1;			/* First BRANCH. */
	if (OP (regnext (scan)) == END) {	/* Only one top-level choice. */
		scan = OPERAND(scan);

		/* Starting-point info. */
		if (OP(scan) == EXACTLY)
			r->start = *OPERAND(scan);
		else if (OP(scan) == BOL)
			r->anchor++;

		/*
		 * If there's something expensive in the r.e., find the
		 * longest literal string that must appear and make it the
		 * `must'.  Resolve ties in favor of later strings, since
		 * the start check works with the beginning of the r.e.
		 * and avoiding duplication strengthens checking.  Not a
		 * strong reason, but sufficient in the absence of others.
		 */
		if (flags & SPSTART) {
			longest = 0;
			len = 0;
			for (; scan; scan=regnext(scan))
				if (OP(scan) == EXACTLY &&
				    strlen (OPERAND (scan)) >= len) {
					longest = OPERAND (scan);
					len = strlen(OPERAND (scan));
				}
			r->must = longest;
			r->mustlen = len;
		}
	}
	return 1;
}

/*
 - reg - regular expression, i.e. main body or parenthesized thing
 *
 * Caller must absorb opening parenthesis.
 *
 * Combining parenthesis handling with the base level of regular expression
 * is a trifle forced, but the need to tie the tails of the branches to what
 * follows makes it hard to avoid.
 */
static unsigned char *
reg (compile_t *x, unsigned char paren, unsigned char *flagp)
{
	unsigned char *ret;
	unsigned char *br;
	unsigned char *ender;
	unsigned char parno = 0;
	unsigned char flags;

	*flagp = HASWIDTH;	/* Tentatively. */

	/* Make an OPEN node, if parenthesized. */
	if (paren) {
		if (x->npar >= NSUBEXP) {
			/* FAIL("too many ()"); */
			return 0;
		}
		parno = x->npar;
		x->npar++;
		ret = regnode (x, OPEN+parno);
	} else
		ret = 0;

	/* Pick up the branches, linking them together. */
	br = regbranch (x, &flags);
	if (! br)
		return 0;

	if (ret)
		regtail (ret, br);	/* OPEN -> first. */
	else
		ret = br;

	if (!(flags&HASWIDTH))
		*flagp &= ~HASWIDTH;
	*flagp |= flags & SPSTART;
	while (*x->parse == '|' || *x->parse == '\n') {
		x->parse++;
		br = regbranch (x, &flags);
		if (! br)
			return 0;

		regtail (ret, br);	/* BRANCH -> BRANCH. */
		if (!(flags & HASWIDTH))
			*flagp &= ~HASWIDTH;
		*flagp |= flags & SPSTART;
	}

	/* Make a closing node, and hook it on the end. */
	ender = regnode (x, paren ? CLOSE+parno : END);
	regtail (ret, ender);

	/* Hook the tails of the branches to the closing node. */
	for (br=ret; br; br=regnext(br))
		regoptail (br, ender);

	/* Check for proper termination. */
	if (paren && *x->parse++ != ')') {
		/* FAIL("unmatched ()"); */
		return 0;
	}
	if (! paren && *x->parse != '\0') {
		if (*x->parse == ')') {
			/* FAIL("unmatched ()"); */
			return 0;
		}
		/* "Can't happen". */
		/* FAIL("junk on end"); */
		return 0;
		/* NOTREACHED */
	}
	return ret;
}

/*
 - regbranch - one alternative of an | operator
 *
 * Implements the concatenation operator.
 */
static unsigned char *
regbranch (compile_t *x, unsigned char *flagp)
{
	unsigned char *ret;
	unsigned char *chain;
	unsigned char *latest;
	unsigned char flags;

	*flagp = WORST;		/* Tentatively. */

	ret = regnode (x, BRANCH);
	chain = 0;
	while (*x->parse != '\0' && *x->parse != ')' &&
	       *x->parse != '\n' && *x->parse != '|') {
		latest = regpiece (x, &flags);
		if (! latest)
			return 0;

		*flagp |= flags & HASWIDTH;
		if (! chain)		/* First piece. */
			*flagp |= flags & SPSTART;
		else
			regtail (chain, latest);
		chain = latest;
	}
	if (! chain)			/* Loop ran zero times. */
		regnode (x, NOTHING);

	return ret;
}

/*
 - regpiece - something followed by possible [*+?]
 *
 * Note that the branching code sequences used for ? and the general cases
 * of * and + are somewhat optimized:  they use the same NOTHING node as
 * both the endmarker for their branch list and the body of the last branch.
 * It might seem that this node could be dispensed with entirely, but the
 * endmarker role is not redundant.
 */
static unsigned char *
regpiece (compile_t *x, unsigned char *flagp)
{
	unsigned char *ret;
	unsigned char op;
	unsigned char *next;
	unsigned char flags;

	ret = regatom (x, &flags);
	if (! ret)
		return 0;

	op = *x->parse;
	if (! ISMULT (op)) {
		*flagp = flags;
		return ret;
	}

	if (!(flags&HASWIDTH) && op != '?') {
		/* FAIL("*+ operand could be empty"); */
		return 0;
	}
	*flagp = (op != '+') ? (WORST | SPSTART) : (WORST | HASWIDTH);

	if (op == '*' && (flags&SIMPLE))
		reginsert (x, STAR, ret);
	else if (op == '*') {
		/* Emit x* as (x&|), where & means "self". */
		reginsert (x, BRANCH, ret);		/* Either x */
		regoptail (ret, regnode (x, BACK));	/* and loop */
		regoptail (ret, ret);			/* back */
		regtail (ret, regnode (x, BRANCH));	/* or */
		regtail (ret, regnode (x, NOTHING));	/* null. */
	} else if (op == '+' && (flags&SIMPLE))
		reginsert (x, PLUS, ret);
	else if (op == '+') {
		/* Emit x+ as x(&|), where & means "self". */
		next = regnode (x, BRANCH);		/* Either */
		regtail (ret, next);
		regtail (regnode (x, BACK), ret);	/* loop back */
		regtail (next, regnode (x, BRANCH));	/* or */
		regtail (ret, regnode (x, NOTHING));	/* null. */
	} else if (op == '?') {
		/* Emit x? as (x|) */
		reginsert (x, BRANCH, ret);		/* Either x */
		regtail (ret, regnode (x, BRANCH));	/* or */
		next = regnode (x, NOTHING);		/* null. */
		regtail (ret, next);
		regoptail (ret, next);
	}
	x->parse++;
	if (ISMULT (*x->parse)) {
		/* FAIL("nested *?+"); */
		return 0;
	}
	return ret;
}

/*
 - regatom - the lowest level
 *
 * Optimization:  gobbles an entire sequence of ordinary characters so that
 * it can turn them into a single node, which is smaller to store and
 * faster to run.  Backslashed characters are exceptions, each becoming a
 * separate node; the code is simpler that way and it's not worth fixing.
 */
static unsigned char *
regatom (compile_t *x, unsigned char *flagp)
{
	unsigned char *ret;
	unsigned char flags;

	*flagp = WORST;		/* Tentatively. */

	switch (*x->parse++) {
	/* FIXME: these chars only have meaning at beg/end of pat? */
	case '^':
		ret = regnode (x, BOL);
		break;
	case '$':
		ret = regnode (x, EOL);
		break;
	case '.':
		ret = regnode (x, ANY);
		*flagp |= HASWIDTH | SIMPLE;
		break;
	case '[': {
			unsigned char class, classend;

			if (*x->parse == '^') {	/* Complement of range. */
				ret = regnode (x, ANYBUT);
				x->parse++;
			} else
				ret = regnode (x, ANYOF);
			if (*x->parse == ']' || *x->parse == '-')
				regc (x, *x->parse++);
			while (*x->parse != '\0' && *x->parse != ']') {
				if (*x->parse == '-') {
					x->parse++;
					if (*x->parse == ']' || *x->parse == '\0')
						regc (x, '-');
					else {
						class = UCHARAT(x->parse-2);
						classend = UCHARAT(x->parse);
						if (class > classend) {
							/* FAIL("invalid [] range"); */
							return 0;
						}
						for (class++; class <= classend; class++)
							regc (x, class);
						x->parse++;
					}
				} else
					regc (x, *x->parse++);
			}
			regc (x, '\0');
			if (*x->parse != ']') {
				/* FAIL("unmatched []"); */
				return 0;
			}
			x->parse++;
			*flagp |= HASWIDTH | SIMPLE;
		}
		break;
	case '(':
		ret = reg (x, 1, &flags);
		if (! ret)
			return 0;
		*flagp |= flags & (HASWIDTH | SPSTART);
		break;
	case '\0':
	case '|':
	case '\n':
	case ')':
		/* Supposed to be caught earlier. */
		/* FAIL("internal urp"); */
		return 0;
	case '?':
	case '+':
	case '*':
		/* FAIL("?+* follows nothing"); */
		return 0;
	case '\\':
		switch (*x->parse++) {
		case '\0':
			/* FAIL("trailing \\"); */
			return 0;
		case '<':
			ret = regnode (x, WORDA);
			break;
		case '>':
			ret = regnode (x, WORDZ);
			break;
		/* FIXME: Someday handle \1, \2, ... */
		default:
			/* Handle general quoted chars in exact-match routine */
			goto de_fault;
		}
		break;
	de_fault:
	default:
		/*
		 * Encode a string of characters to be matched exactly.
		 *
		 * This is a bit tricky due to quoted chars and due to
		 * '*', '+', and '?' taking the SINGLE char previous
		 * as their operand.
		 *
		 * On entry, the char at regparse[-1] is going to go
		 * into the string, no matter what it is.  (It could be
		 * following a \ if we are entered from the '\' case.)
		 *
		 * Basic idea is to pick up a good char in  ch  and
		 * examine the next char.  If it's *+? then we twiddle.
		 * If it's \ then we frozzle.  If it's other magic char
		 * we push  ch  and terminate the string.  If none of the
		 * above, we push  ch  on the string and go around again.
		 *
		 * `Regprev' is used to remember where "the current char"
		 * starts in the string, if due to a *+? we need to back
		 * up and put the current char in a separate, 1-char, string.
		 * When `regprev' is NULL,  ch  is the only char in the
		 * string; this is used in *+? handling, and in setting
		 * flags |= SIMPLE at the end.
		 */
		{
			const unsigned char *regprev;
			unsigned char ch;

			x->parse--;			/* Look at cur char */
			ret = regnode (x, EXACTLY);
			regprev = 0;
			for (;;) {
				ch = *x->parse++;	/* Get current char */
				switch (*x->parse) {	/* look at next one */

				default:
					regc (x, ch);	/* Add cur to string */
					break;

				case '.': case '[': case '(':
				case ')': case '|': case '\n':
				case '$': case '^':
				case '\0':
				/* FIXME, $ and ^ should not always be magic */
				magic:
					regc (x, ch);	/* dump cur char */
					goto done;	/* and we are done */

				case '?': case '+': case '*':
					if (! regprev) 	/* If just ch in str, */
						goto magic;	/* use it */
					/* End mult-char string one early */
					x->parse = regprev; /* Back up parse */
					goto done;

				case '\\':
					regc (x, ch);	/* Cur char OK */
					switch (x->parse[1]){ /* Look after \ */
					case '\0':
					case '<':
					case '>':
					/* FIXME: Someday handle \1, \2, ... */
						goto done; /* Not quoted */
					default:
						/* Backup point is \, scan							 * point is after it. */
						regprev = x->parse;
						x->parse++;
						continue;	/* NOT break; */
					}
				}
				regprev = x->parse;	/* Set backup point */
			}
		done:
			regc (x, '\0');
			*flagp |= HASWIDTH;
			if (! regprev)		/* One char? */
				*flagp |= SIMPLE;
		}
		break;
	}

	return ret;
}

/*
 - regnode - emit a node
 */
static unsigned char *			/* Location. */
regnode (compile_t *x, unsigned char op)
{
	unsigned char *ret;
	unsigned char *ptr;

	ret = x->code;
	if (ret == &regdummy) {
		x->size += 3;
		return ret;
	}

	ptr = ret;
	*ptr++ = op;
	*ptr++ = '\0';		/* Null "next" pointer. */
	*ptr++ = '\0';
	x->code = ptr;

	return ret;
}

/*
 - regc - emit (if appropriate) a byte of code
 */
static void
regc (compile_t *x, unsigned char b)
{
	if (x->code != &regdummy)
		*x->code++ = b;
	else
		x->size++;
}

/*
 - reginsert - insert an operator in front of already-emitted operand
 *
 * Means relocating the operand.
 */
static void
reginsert (compile_t *x, unsigned char op, unsigned char *opnd)
{
	unsigned char *src;
	unsigned char *dst;
	unsigned char *place;

	if (x->code == &regdummy) {
		x->size += 3;
		return;
	}

	src = x->code;
	x->code += 3;
	dst = x->code;
	while (src > opnd)
		*--dst = *--src;

	place = opnd;		/* Op node, where operand used to be. */
	*place++ = op;
	*place++ = '\0';
	*place++ = '\0';
}

/*
 - regtail - set the next-pointer at the end of a node chain
 */
static void
regtail (unsigned char *p, unsigned char *val)
{
	unsigned char *scan;
	unsigned char *temp;
	unsigned short offset;

	if (p == &regdummy)
		return;

	/* Find last node. */
	scan = p;
	for (;;) {
		temp = regnext (scan);
		if (! temp)
			break;
		scan = temp;
	}

	if (OP(scan) == BACK)
		offset = scan - val;
	else
		offset = val - scan;
	*(scan+1) = (offset >> 8) & 0377;
	*(scan+2) = offset & 0377;
}

/*
 - regoptail - regtail on operand of first argument; nop if operandless
 */
static void
regoptail (unsigned char *p, unsigned char *val)
{
	/* "Operandless" and "op != BRANCH" are synonymous in practice. */
	if (! p || p == &regdummy || OP(p) != BRANCH)
		return;
	regtail (OPERAND(p), val);
}

/*
 * regexec and friends
 */

/*
 * Match a regular expression against a string.
 * Returns 1 on success, or 0 on failure.
 */
bool_t
regexp_execute (regexp_t *prog, const unsigned char *string)
{
	execute_t z;
	const unsigned char *s;

	/* Be paranoid... */
	if (! prog || ! string) {
		/* regerror("NULL parameter"); */
		return 0;
	}

	/* Check validity of program. */
	if (UCHARAT (prog->program) != MAGIC) {
		/* regerror("corrupted program"); */
		return 0;
	}

	/* If there is a "must appear" string, look for it. */
	if (prog->must) {
		s = string;
		while ((s = strchr (s, prog->must[0])) != 0) {
			if (strncmp (s, prog->must, prog->mustlen) == 0)
				break;	/* Found it. */
			s++;
		}
		if (! s)	/* Not present. */
			return 0;
	}

	/* Mark beginning of line for ^ . */
	z.bol = string;

	/* Simplest case:  anchored match need be tried only once. */
	if (prog->anchor)
		return regtry (prog, &z, string);

	/* Messy cases:  unanchored match. */
	s = string;
	if (prog->start != '\0')
		/* We know what char it must start with. */
		while ((s = strchr (s, prog->start)) != 0) {
			if (regtry (prog, &z, s))
				return 1;
			s++;
		}
	else
		/* We don't -- general case. */
		do {
			if (regtry (prog, &z, s))
				return 1;
		} while (*s++ != '\0');

	/* Failure. */
	return 0;
}

/*
 - regtry - try match at specific point
 */
static unsigned char			/* 0 failure, 1 success */
regtry (regexp_t *prog, execute_t *z, const unsigned char *string)
{
	unsigned char i;
	const unsigned char **sp;
	const unsigned char **ep;

	z->input = string;
	z->startp = prog->startp;
	z->endp = prog->endp;

	sp = prog->startp;
	ep = prog->endp;
	for (i=NSUBEXP; i>0; i--) {
		*sp++ = 0;
		*ep++ = 0;
	}
	if (regmatch (z, prog->program + 1)) {
		prog->startp[0] = string;
		prog->endp[0] = z->input;
		return 1;
	}
	return 0;
}

/*
 - regmatch - main matching routine
 *
 * Conceptually the strategy is simple:  check to see whether the current
 * node matches, call self recursively to see whether the rest matches,
 * and then act accordingly.  In practice we make some effort to avoid
 * recursion, in particular by going through "ordinary" nodes (that don't
 * need to know whether the rest of the match failed) by a loop instead of
 * by recursion.
 */
static unsigned char			/* 0 failure, 1 success */
regmatch (execute_t *z, unsigned char *prog)
{
	unsigned char *scan;		/* Current node. */
	unsigned char *next;		/* Next node. */

	scan = prog;
#ifdef DEBUG_REGEXP
	if (scan && regsub_narrate)
		fprintf (stderr, "%s(\n", regprop (scan));
#endif
	while (scan) {
#ifdef DEBUG_REGEXP
		if (regsub_narrate)
			fprintf (stderr, "%s...\n", regprop (scan));
#endif
		next = regnext (scan);

		switch (OP(scan)) {
		case BOL:
			if (z->input != z->bol)
				return 0;
			break;
		case EOL:
			if (*z->input != '\0')
				return 0;
			break;
		case WORDA:
			/* Must be looking at a letter, digit, or _ */
			if ((!ISALNUM((unsigned char)*z->input)) && *z->input != '_')
				return 0;
			/* Prev must be BOL or nonword */
			if (z->input > z->bol &&
			    (ISALNUM((unsigned char)z->input[-1]) || z->input[-1] == '_'))
				return 0;
			break;
		case WORDZ:
			/* Must be looking at non letter, digit, or _ */
			if (ISALNUM((unsigned char)*z->input) || *z->input == '_')
				return 0;
			/* We don't care what the previous char was */
			break;
		case ANY:
			if (*z->input == '\0')
				return 0;
			z->input++;
			break;
		case EXACTLY: {
				unsigned short len;
				unsigned char *opnd;

				opnd = OPERAND(scan);
				/* Inline the first character, for speed. */
				if (*opnd != *z->input)
					return 0;
				len = strlen(opnd);
				if (len > 1 && strncmp(opnd, z->input, len) != 0)
					return 0;
				z->input += len;
			}
			break;
		case ANYOF:
 			if (*z->input == '\0' || strchr(OPERAND(scan), *z->input) == 0)
				return 0;
			z->input++;
			break;
		case ANYBUT:
 			if (*z->input == '\0' || strchr(OPERAND(scan), *z->input) != 0)
				return 0;
			z->input++;
			break;
		case NOTHING:
			break;
		case BACK:
			break;
		case OPEN+1:
		case OPEN+2:
		case OPEN+3:
		case OPEN+4:
		case OPEN+5:
		case OPEN+6:
		case OPEN+7:
		case OPEN+8:
		case OPEN+9: {
				unsigned char no;
				const unsigned char *save;

				no = OP(scan) - OPEN;
				save = z->input;

				if (regmatch (z, next)) {
					/*
					 * Don't set startp if some later
					 * invocation of the same parentheses
					 * already has.
					 */
					if (! z->startp[no])
						z->startp[no] = save;
					return 1;
				}
				return 0;
			}
			break;
		case CLOSE+1:
		case CLOSE+2:
		case CLOSE+3:
		case CLOSE+4:
		case CLOSE+5:
		case CLOSE+6:
		case CLOSE+7:
		case CLOSE+8:
		case CLOSE+9: {
				unsigned char no;
				const unsigned char *save;

				no = OP(scan) - CLOSE;
				save = z->input;

				if (regmatch (z, next)) {
					/*
					 * Don't set endp if some later
					 * invocation of the same parentheses
					 * already has.
					 */
					if (! z->endp[no])
						z->endp[no] = save;
					return 1;
				}
				return 0;
			}
			break;
		case BRANCH: {
				const unsigned char *save;

				if (OP(next) != BRANCH)		/* No choice. */
					next = OPERAND(scan);	/* Avoid recursion. */
				else {
					do {
						save = z->input;
						if (regmatch (z, OPERAND(scan)))
							return 1;
						z->input = save;
						scan = regnext (scan);
					} while (scan && OP(scan) == BRANCH);
					return 0;
					/* NOTREACHED */
				}
			}
			break;
		case STAR:
		case PLUS: {
				unsigned char nextch;
				unsigned short no, min;
				const unsigned char *save;

				/*
				 * Lookahead to avoid useless match attempts
				 * when we know what character comes next.
				 */
				nextch = '\0';
				if (OP(next) == EXACTLY)
					nextch = *OPERAND(next);
				min = (OP(scan) == STAR) ? 0 : 1;
				save = z->input;
				no = regrepeat (z, OPERAND(scan));
				while (no >= min) {
					/* If it could work, try it. */
					if (nextch == '\0' || *z->input == nextch)
						if (regmatch (z, next))
							return 1;
					/* Couldn't or didn't -- back up. */
					no--;
					z->input = save + no;
				}
				return 0;
			}
			break;
		case END:
			return 1;	/* Success! */
		default:
			/* regerror("memory corruption"); */
			return 0;
		}

		scan = next;
	}

	/*
	 * We get here only if there's trouble -- normally "case END" is
	 * the terminating point.
	 */
	/* regerror("corrupted pointers"); */
	return 0;
}

/*
 - regrepeat - repeatedly match something simple, report how many
 */
static unsigned short
regrepeat (execute_t *z, unsigned char *p)
{
	unsigned short count = 0;
	const unsigned char *scan;
	unsigned char *opnd;

	scan = z->input;
	opnd = OPERAND(p);
	switch (OP(p)) {
	case ANY:
		count = strlen(scan);
		scan += count;
		break;
	case EXACTLY:
		while (*opnd == *scan) {
			count++;
			scan++;
		}
		break;
	case ANYOF:
		while (*scan != '\0' && strchr(opnd, *scan) != 0) {
			count++;
			scan++;
		}
		break;
	case ANYBUT:
		while (*scan != '\0' && strchr(opnd, *scan) == 0) {
			count++;
			scan++;
		}
		break;
	default:		/* Oh dear.  Called inappropriately. */
		/* regerror("internal foulup"); */
		count = 0;	/* Best compromise. */
		break;
	}
	z->input = scan;

	return count;
}

/*
 - regnext - dig the "next" pointer out of a node
 */
static unsigned char *
regnext (unsigned char *p)
{
	unsigned short offset;

	if (p == &regdummy)
		return 0;

	offset = NEXT(p);
	if (offset == 0)
		return 0;

	if (OP(p) == BACK)
		return p - offset;
	else
		return p + offset;
}

#ifdef DEBUG_REGEXP

/*
 * Dump a regexp onto stdout in vaguely comprehensible form
 */
void
regsub_dump (regexp_t *r)
{
	unsigned char *s;
	unsigned char op = EXACTLY;	/* Arbitrary non-END op. */
	unsigned char *next;

	s = r->program + 1;
	while (op != END) {	/* While that wasn't END last time... */
		op = OP(s);
		printf("%2d%s", s-r->program, regprop (s));	/* Where, what. */
		next = regnext (s);
		if (! next)		/* Next ptr. */
			printf("(0)");
		else
			printf("(%d)", (s-r->program)+(next-s));
		s += 3;
		if (op == ANYOF || op == ANYBUT || op == EXACTLY) {
			/* Literal string, where present. */
			while (*s != '\0') {
				putchar(*s);
				s++;
			}
			s++;
		}
		putchar('\n');
	}

	/* Header fields of interest. */
	if (r->start != '\0')
		printf("start `%c' ", r->start);
	if (r->anchor)
		printf("anchored ");
	if (r->must)
		printf("must have \"%s\"", r->must);
	printf("\n");
}

/*
 - regprop - printable representation of opcode
 */
static unsigned char *
regprop (unsigned char *op)
{
	unsigned char *p;
	static unsigned char buf[50];

	strcpy (buf, ":");

	switch (OP(op)) {
	case BOL:	p = "BOL";	break;
	case EOL:	p = "EOL";	break;
	case ANY:	p = "ANY";	break;
	case ANYOF:	p = "ANYOF";	break;
	case ANYBUT:	p = "ANYBUT";	break;
	case BRANCH:	p = "BRANCH";	break;
	case EXACTLY:	p = "EXACTLY";	break;
	case NOTHING:	p = "NOTHING";	break;
	case BACK:	p = "BACK";	break;
	case END:	p = "END";	break;
	case STAR:	p = "STAR";	break;
	case PLUS:	p = "PLUS";	break;
	case WORDA:	p = "WORDA";	break;
	case WORDZ:	p = "WORDZ";	break;
	case OPEN+1:
	case OPEN+2:
	case OPEN+3:
	case OPEN+4:
	case OPEN+5:
	case OPEN+6:
	case OPEN+7:
	case OPEN+8:
	case OPEN+9:
		sprintf (buf + strlen (buf), "OPEN%d", OP(op) - OPEN);
		p = 0;
		break;
	case CLOSE+1:
	case CLOSE+2:
	case CLOSE+3:
	case CLOSE+4:
	case CLOSE+5:
	case CLOSE+6:
	case CLOSE+7:
	case CLOSE+8:
	case CLOSE+9:
		sprintf (buf + strlen (buf), "CLOSE%d", OP(op) - CLOSE);
		p = 0;
		break;
	default:
		/* corrupted opcode */
		p = "???";
		break;
	}
	if (p)
		strcat (buf, p);
	return buf;
}
#endif
