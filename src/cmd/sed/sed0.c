#include <stdio.h>
#include <stdlib.h>
#include "sed.h"

struct label	*labtab = ltab;
char	CGMES[]	= "command garbled: %s\n";
char	TMMES[]	= "Too much text: %s\n";
char	LTL[]	= "Label too long: %s\n";
char	AD0MES[]	= "No addresses allowed: %s\n";
char	AD1MES[]	= "Only one address allowed: %s\n";
char	bittab[]  = {
		1,
		2,
		4,
		8,
		16,
		32,
		64,
		128
	};

FILE	*fin;
union reptr	*abuf[ABUFSIZE];
union reptr **aptr;
char	*lastre;
char	ibuf[BUFSIZ];
char	*cbp;
char	*ebp;
char	genbuf[LBSIZE];
char	*loc1;
char	*loc2;
char	*locs;
char	seof;
char	*reend;
char	*lbend;
char	*hend;
char	*lcomend;
union reptr	*ptrend;
int	eflag;
int	dolflag;
int	sflag;
int	jflag;
int	numbra;
int	delflag;
long	lnum;
char	linebuf[LBSIZE+1];
char	holdsp[LBSIZE+1];
char	*spend;
char	*hspend;
int	nflag;
int	gflag;
char	*braelist[NBRA];
char	*braslist[NBRA];
long	tlno[NLINES];
int	nlno;
char	fname[12][40];
FILE	*fcode[12];
int	nfiles;
char	*cp;
char	*reend;
char	*lbend;
union reptr	ptrspace[PTRSIZE], *rep;
char	respace[RESIZE];
struct label	ltab[LABSIZE];
struct label	*lab;
struct label	*labend;
int	f;
int	depth;
int	eargc;
char	**eargv;
char	bittab[];
union reptr	**cmpend[DEPTH];
int	depth;
union reptr	*pending;
char	*badp;
char	bad;
char	compfl;

void fcomp(void);
void dechain(void);
int rline(char *lbuf);
struct label *search(struct label *ptr);
char *text(char *textbuf);
char *compile(char *expbuf);
char *compsub(char *rhsbuf);
int cmp(char *a, char *b);
char *ycomp(char *expbuf);

int main(argc, argv)
        char *argv[];
{

	eargc = argc;
	eargv = argv;

	badp = &bad;
	aptr = abuf;
	lab = labtab + 1;	/* 0 reserved for end-pointer */
	rep = ptrspace;
	rep->A.ad1 = respace;
	lbend = &linebuf[LBSIZE];
	hend = &holdsp[LBSIZE];
	lcomend = &genbuf[71];
	ptrend = &ptrspace[PTRSIZE];
	reend = &respace[RESIZE];
	labend = &labtab[LABSIZE];
	lnum = 0;
	pending = 0;
	depth = 0;
	spend = linebuf;
	hspend = holdsp;
	fcode[0] = stdout;
	nfiles = 1;

	if (eargc == 1)
		exit(0);


	while (--eargc > 0 && (++eargv)[0][0] == '-')
		switch (eargv[0][1]) {

		case 'n':
			nflag++;
			continue;

		case 'f':
			if (eargc-- <= 0)	exit(2);

			if ((fin = fopen(*++eargv, "r")) == NULL) {
				fprintf(stderr, "Cannot open pattern-file: %s\n", *eargv);
				exit(2);
			}

			fcomp();
			fclose(fin);
			continue;

		case 'e':
			eflag++;
			fcomp();
			eflag = 0;
			continue;

		case 'g':
			gflag++;
			continue;

		default:
			fprintf(stdout, "Unknown flag: %c\n", eargv[0][1]);
			continue;
		}


	if (compfl == 0) {
		eargv--;
		eargc++;
		eflag++;
		fcomp();
		eargv++;
		eargc--;
		eflag = 0;
	}

	if (depth) {
		fprintf(stderr, "Too many {'s");
		exit(2);
	}

	labtab->address = rep;

	dechain();

        /*abort();*/	/*DEBUG*/

	if (eargc <= 0)
		execute((char *)NULL);
	else while (--eargc >= 0) {
		execute(*eargv++);
	}
	fclose(stdout);
	exit(0);
}

void fcomp()
{
	char	*p, *op, *tp;
	char	*address();
	union reptr	*pt, *pt1;
	int	i;
	struct label	*lpt;

	compfl = 1;
	op = lastre;

	if (rline(linebuf) < 0)	return;
	if (*linebuf == '#') {
		if (linebuf[1] == 'n')
			nflag = 1;
	}
	else {
		cp = linebuf;
		goto comploop;
	}

	for (;;) {
		if (rline(linebuf) < 0)	break;

		cp = linebuf;

comploop:
	/*fprintf(stdout, "cp: %s\n", cp);*/	/*DEBUG*/
		while (*cp == ' ' || *cp == '\t')	cp++;
		if (*cp == '\0' || *cp == '#')		continue;
		if (*cp == ';') {
			cp++;
			goto comploop;
		}

		p = address(rep->A.ad1);
		if (p == badp) {
			fprintf(stderr, CGMES, linebuf);
			exit(2);
		}

		if (p == rep->A.ad1) {
			if (op)
				rep->A.ad1 = op;
			else {
				fprintf(stderr, "First RE may not be null\n");
				exit(2);
			}
		} else if (p == 0) {
			p = rep->A.ad1;
			rep->A.ad1 = 0;
		} else {
			op = rep->A.ad1;
			if (*cp == ',' || *cp == ';') {
				cp++;
				if ((rep->A.ad2 = p) > reend) {
					fprintf(stderr, TMMES, linebuf);
					exit(2);
				}
				p = address(rep->A.ad2);
				if (p == badp || p == 0) {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				if (p == rep->A.ad2)
					rep->A.ad2 = op;
				else
					op = rep->A.ad2;

			} else
				rep->A.ad2 = 0;
		}

		if (p > reend) {
			fprintf(stderr, "Too much text: %s\n", linebuf);
			exit(2);
		}

		while (*cp == ' ' || *cp == '\t')	cp++;

swit:
		switch (*cp++) {

			default:
				fprintf(stderr, "Unrecognized command: %s\n", linebuf);
				exit(2);

			case '!':
				rep->A.negfl = 1;
				goto swit;

			case '{':
				rep->A.command = BCOM;
				rep->A.negfl = !(rep->A.negfl);
				cmpend[depth++] = &rep->B.lb1;
				if (++rep >= ptrend) {
					fprintf(stderr, "Too many commands: %s\n", linebuf);
					exit(2);
				}
				rep->A.ad1 = p;
				if (*cp == '\0')	continue;

				goto comploop;

			case '}':
				if (rep->A.ad1) {
					fprintf(stderr, AD0MES, linebuf);
					exit(2);
				}

				if (--depth < 0) {
					fprintf(stderr, "Too many }'s\n");
					exit(2);
				}
				*cmpend[depth] = rep;

				rep->A.ad1 = p;
				continue;

			case '=':
				rep->A.command = EQCOM;
				if (rep->A.ad2) {
					fprintf(stderr, AD1MES, linebuf);
					exit(2);
				}
				break;

			case ':':
				if (rep->A.ad1) {
					fprintf(stderr, AD0MES, linebuf);
					exit(2);
				}

				while (*cp++ == ' ');
				cp--;


				tp = lab->asc;
				while ((*tp++ = *cp++))
					if (tp >= &(lab->asc[8])) {
						fprintf(stderr, LTL, linebuf);
						exit(2);
					}
				*--tp = '\0';

				if ((lpt = search(lab))) {
					if (lpt->address) {
						fprintf(stderr, "Duplicate labels: %s\n", linebuf);
						exit(2);
					}
				} else {
					lab->chain = 0;
					lpt = lab;
					if (++lab >= labend) {
						fprintf(stderr, "Too many labels: %s\n", linebuf);
						exit(2);
					}
				}
				lpt->address = rep;
				rep->A.ad1 = p;

				continue;

			case 'a':
				rep->A.command = ACOM;
				if (rep->A.ad2) {
					fprintf(stderr, AD1MES, linebuf);
					exit(2);
				}
				if (*cp == '\\')	cp++;
				if (*cp++ != '\n') {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				rep->A.re1 = p;
				p = text(rep->A.re1);
				break;
			case 'c':
				rep->A.command = CCOM;
				if (*cp == '\\')	cp++;
				if (*cp++ != ('\n')) {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				rep->A.re1 = p;
				p = text(rep->A.re1);
				break;
			case 'i':
				rep->A.command = ICOM;
				if (rep->A.ad2) {
					fprintf(stderr, AD1MES, linebuf);
					exit(2);
				}
				if (*cp == '\\')	cp++;
				if (*cp++ != ('\n')) {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				rep->A.re1 = p;
				p = text(rep->A.re1);
				break;

			case 'g':
				rep->A.command = GCOM;
				break;

			case 'G':
				rep->A.command = CGCOM;
				break;

			case 'h':
				rep->A.command = HCOM;
				break;

			case 'H':
				rep->A.command = CHCOM;
				break;

			case 't':
				rep->A.command = TCOM;
				goto jtcommon;

			case 'b':
				rep->A.command = BCOM;
jtcommon:
				while (*cp++ == ' ');
				cp--;

				if (*cp == '\0') {
					if ((pt = labtab->chain)) {
						while ((pt1 = pt->B.lb1))
							pt = pt1;
						pt->B.lb1 = rep;
					} else
						labtab->chain = rep;
					break;
				}
				tp = lab->asc;
				while ((*tp++ = *cp++))
					if (tp >= &(lab->asc[8])) {
						fprintf(stderr, LTL, linebuf);
						exit(2);
					}
				cp--;
				*--tp = '\0';

				if ((lpt = search(lab))) {
					if (lpt->address) {
						rep->B.lb1 = lpt->address;
					} else {
						pt = lpt->chain;
						while ((pt1 = pt->B.lb1))
							pt = pt1;
						pt->B.lb1 = rep;
					}
				} else {
					lab->chain = rep;
					lab->address = 0;
					if (++lab >= labend) {
						fprintf(stderr, "Too many labels: %s\n", linebuf);
						exit(2);
					}
				}
				break;

			case 'n':
				rep->A.command = NCOM;
				break;

			case 'N':
				rep->A.command = CNCOM;
				break;

			case 'p':
				rep->A.command = PCOM;
				break;

			case 'P':
				rep->A.command = CPCOM;
				break;

			case 'r':
				rep->A.command = RCOM;
				if (rep->A.ad2) {
					fprintf(stderr, AD1MES, linebuf);
					exit(2);
				}
				if (*cp++ != ' ') {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				rep->A.re1 = p;
				p = text(rep->A.re1);
				break;

			case 'd':
				rep->A.command = DCOM;
				break;

			case 'D':
				rep->A.command = CDCOM;
				rep->B.lb1 = ptrspace;
				break;

			case 'q':
				rep->A.command = QCOM;
				if (rep->A.ad2) {
					fprintf(stderr, AD1MES, linebuf);
					exit(2);
				}
				break;

			case 'l':
				rep->A.command = LCOM;
				break;

			case 's':
				rep->A.command = SCOM;
				seof = *cp++;
				rep->A.re1 = p;
				p = compile(rep->A.re1);
				if (p == badp) {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				if (p == rep->A.re1) {
					if (op)
					    rep->A.re1 = op;
					else {
					    fprintf(stderr,
						"First RE may not be null\n");
					    exit(2);
					}
				} else {
					op = rep->A.re1;
				}

				if ((rep->A.rhs = p) > reend) {
					fprintf(stderr, TMMES, linebuf);
					exit(2);
				}

				if ((p = compsub(rep->A.rhs)) == badp) {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				if (*cp == 'g') {
					cp++;
					rep->A.gfl++;
				} else if (gflag)
					rep->A.gfl++;

				if (*cp == 'p') {
					cp++;
					rep->A.pfl = 1;
				}

				if (*cp == 'P') {
					cp++;
					rep->A.pfl = 2;
				}

				if (*cp == 'w') {
					cp++;
					if (*cp++ !=  ' ') {
						fprintf(stderr, CGMES, linebuf);
						exit(2);
					}
					if (nfiles >= 10) {
						fprintf(stderr, "Too many files in w commands\n");
						exit(2);
					}

					text(fname[nfiles]);
					for (i = nfiles - 1; i >= 0; i--)
						if (cmp(fname[nfiles],fname[i]) == 0) {
							rep->A.fcode = fcode[i];
							goto done;
						}
					if ((rep->A.fcode = fopen(fname[nfiles], "w")) == NULL) {
						fprintf(stderr, "cannot open %s\n", fname[nfiles]);
						exit(2);
					}
					fcode[nfiles++] = rep->A.fcode;
				}
				break;

			case 'w':
				rep->A.command = WCOM;
				if (*cp++ != ' ') {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				if (nfiles >= 10){
					fprintf(stderr, "Too many files in w commands\n");
					exit(2);
				}

				text(fname[nfiles]);
				for (i = nfiles - 1; i >= 0; i--)
					if (cmp(fname[nfiles], fname[i]) == 0) {
						rep->A.fcode = fcode[i];
						goto done;
					}

				if ((rep->A.fcode = fopen(fname[nfiles], "w")) == NULL) {
					fprintf(stderr, "Cannot create %s\n", fname[nfiles]);
					exit(2);
				}
				fcode[nfiles++] = rep->A.fcode;
				break;

			case 'x':
				rep->A.command = XCOM;
				break;

			case 'y':
				rep->A.command = YCOM;
				seof = *cp++;
				rep->A.re1 = p;
				p = ycomp(rep->A.re1);
				if (p == badp) {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				if (p > reend) {
					fprintf(stderr, TMMES, linebuf);
					exit(2);
				}
				break;

		}
done:
		if (++rep >= ptrend) {
			fprintf(stderr, "Too many commands, last: %s\n", linebuf);
			exit(2);
		}

		rep->A.ad1 = p;

		if (*cp++ != '\0') {
			if (cp[-1] == ';')
				goto comploop;
			fprintf(stderr, CGMES, linebuf);
			exit(2);
		}

	}
	rep->A.command = 0;
	lastre = op;
}

char *compsub(rhsbuf)
        char *rhsbuf;
{
	char	*p, *q;

	p = rhsbuf;
	q = cp;
	for (;;) {
		if ((*p = *q++) == '\\') {
			*p = *q++;
			if (*p > numbra + '0' && *p <= '9')
				return(badp);
			*p++ |= 0200;
			continue;
		}
		if (*p == seof) {
			*p++ = '\0';
			cp = q;
			return(p);
		}
		if (*p++ == '\0') {
			return(badp);
		}

	}
}

char *compile(expbuf)
        char *expbuf;
{
	int	c;
	char	*ep, *sp;
	char	neg;
	char *lastep, *cstart;
	int cclcnt;
	int	closed;
	char	bracket[NBRA], *bracketp;

	if (*cp == seof) {
		cp++;
		return(expbuf);
	}

	ep = expbuf;
	lastep = 0;
	bracketp = bracket;
	closed = numbra = 0;
	sp = cp;
	if (*sp == '^') {
		*ep++ = 1;
		sp++;
	} else {
		*ep++ = 0;
	}
	for (;;) {
		if (ep >= &expbuf[ESIZE]) {
			cp = sp;
			return(badp);
		}
		if ((c = *sp++) == seof) {
			if (bracketp != bracket) {
				cp = sp;
				return(badp);
			}
			cp = sp;
			*ep++ = CEOF;
			return(ep);
		}
		if (c != '*')
			lastep = ep;
		switch (c) {

		case '\\':
			if ((c = *sp++) == '(') {
				if (numbra >= NBRA) {
					cp = sp;
					return(badp);
				}
				*bracketp++ = numbra;
				*ep++ = CBRA;
				*ep++ = numbra++;
				continue;
			}
			if (c == ')') {
				if (bracketp <= bracket) {
					cp = sp;
					return(badp);
				}
				*ep++ = CKET;
				*ep++ = *--bracketp;
				closed++;
				continue;
			}

			if (c >= '1' && c <= '9') {
				if ((c -= '1') >= closed)
					return(badp);

				*ep++ = CBACK;
				*ep++ = c;
				continue;
			}
			if (c == '\n') {
				cp = sp;
				return(badp);
			}
			if (c == 'n') {
				c = '\n';
			}
			goto defchar;

		case '\0':
			continue;
		case '\n':
			cp = sp;
			return(badp);

		case '.':
			*ep++ = CDOT;
			continue;

		case '*':
			if (lastep == 0)
				goto defchar;
			if (*lastep == CKET) {
				cp = sp;
				return(badp);
			}
			*lastep |= STAR;
			continue;

		case '$':
			if (*sp != seof)
				goto defchar;
			*ep++ = CDOL;
			continue;

		case '[':
			if (&ep[17] >= &expbuf[ESIZE]) {
				fprintf(stderr, "RE too long: %s\n", linebuf);
				exit(2);
			}

			*ep++ = CCL;

			neg = 0;
			if ((c = *sp++) == '^') {
				neg = 1;
				c = *sp++;
			}

			cstart = sp;
			do {
				if (c == '\0') {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				if (c=='-' && sp>cstart && *sp!=']') {
					for (c = sp[-2]; c<*sp; c++)
						ep[c>>3] |= bittab[c&07];
				}
				if (c == '\\') {
					switch (c = *sp++) {
						case 'n':
							c = '\n';
							break;
					}
				}

				ep[c >> 3] |= bittab[c & 07];
			} while ((c = *sp++) != ']');

			if (neg)
				for (cclcnt = 0; cclcnt < 16; cclcnt++)
					ep[cclcnt] ^= -1;
			ep[0] &= 0376;

			ep += 16;

			continue;

		defchar:
		default:
			*ep++ = CCHR;
			*ep++ = c;
		}
	}
}

int rline(lbuf)
        char *lbuf;
{
	char	*p, *q;
	int	t;
	static char	*saveq;

	p = lbuf - 1;

	if (eflag) {
		if (eflag > 0) {
			eflag = -1;
			if (eargc-- <= 0)
				exit(2);
			q = *++eargv;
			while ((*++p = *q++)) {
				if (*p == '\\') {
					if ((*++p = *q++) == '\0') {
						saveq = 0;
						return(-1);
					} else
						continue;
				}
				if (*p == '\n') {
					*p = '\0';
					saveq = q;
					return(1);
				}
			}
			saveq = 0;
			return(1);
		}
		if ((q = saveq) == 0)	return(-1);

		while ((*++p = *q++)) {
			if (*p == '\\') {
				if ((*++p = *q++) == '0') {
					saveq = 0;
					return(-1);
				} else
					continue;
			}
			if (*p == '\n') {
				*p = '\0';
				saveq = q;
				return(1);
			}
		}
		saveq = 0;
		return(1);
	}

	while ((t = getc(fin)) != EOF) {
		*++p = t;
		if (*p == '\\') {
			t = getc(fin);
			*++p = t;
		}
		else if (*p == '\n') {
			*p = '\0';
			return(1);
		}
	}
	*++p = '\0';
	return(-1);
}

char *address(expbuf)
        char *expbuf;
{
	char	*rcp;
	long	lno;

	if (*cp == '$') {
		cp++;
		*expbuf++ = CEND;
		*expbuf++ = CEOF;
		return(expbuf);
	}

	if (*cp == '/') {
		seof = '/';
		cp++;
		return(compile(expbuf));
	}

	rcp = cp;
	lno = 0;

	while (*rcp >= '0' && *rcp <= '9')
		lno = lno*10 + *rcp++ - '0';

	if (rcp > cp) {
		*expbuf++ = CLNUM;
		*expbuf++ = nlno;
		tlno[nlno++] = lno;
		if (nlno >= NLINES) {
			fprintf(stderr, "Too many line numbers\n");
			exit(2);
		}
		*expbuf++ = CEOF;
		cp = rcp;
		return(expbuf);
	}
	return(0);
}

int cmp(a, b)
        char *a,*b;
{
	char	*ra, *rb;

	ra = a - 1;
	rb = b - 1;

	while (*++ra == *++rb)
		if (*ra == '\0')	return(0);
	return(1);
}

char *text(textbuf)
        char *textbuf;
{
	char	*p, *q;

	p = textbuf;
	q = cp;
	while (*q == '\t' || *q == ' ')	q++;
	for (;;) {

		if ((*p = *q++) == '\\')
			*p = *q++;
		if (*p == '\0') {
			cp = --q;
			return(++p);
		}
		if (*p == '\n') {
			while (*q == '\t' || *q == ' ')	q++;
		}
		p++;
	}
}

struct label *search(ptr)
        struct label *ptr;
{
	struct label	*rp;

	rp = labtab;
	while (rp < ptr) {
		if (cmp(rp->asc, ptr->asc) == 0)
			return(rp);
		rp++;
	}

	return(0);
}

void dechain()
{
	struct label	*lptr;
	union reptr	*rptr, *trptr;

	for (lptr = labtab; lptr < lab; lptr++) {

		if (lptr->address == 0) {
			fprintf(stderr, "Undefined label: %s\n", lptr->asc);
			exit(2);
		}

		if (lptr->chain) {
			rptr = lptr->chain;
			while ((trptr = rptr->B.lb1)) {
				rptr->B.lb1 = lptr->address;
				rptr = trptr;
			}
			rptr->B.lb1 = lptr->address;
		}
	}
}

char *ycomp(expbuf)
        char *expbuf;
{
	char	c, *ep, *tsp;
	char	*sp;

	ep = expbuf;
	sp = cp;
	for (tsp = cp; *tsp != seof; tsp++) {
		if (*tsp == '\\')
			tsp++;
		if (*tsp == '\n')
			return(badp);
	}
	tsp++;

	while ((c = *sp++ & 0177) != seof) {
		if (c == '\\' && *sp == 'n') {
			sp++;
			c = '\n';
		}
		if ((ep[c] = *tsp++) == '\\' && *tsp == 'n') {
			ep[c] = '\n';
			tsp++;
		}
		if (ep[c] == seof || ep[c] == '\0')
			return(badp);
	}
	if (*tsp != seof)
		return(badp);
	cp = ++tsp;

	for (c = 0; !(c & 0200); c++)
		if (ep[c] == 0)
			ep[c] = c;

	return(ep + 0200);
}
