/*
 * Front-end to the C compiler.
 *
 * Brief description of its syntax:
 * - Files that end with .c are passed via cpp->ccom->as->ld
 * - Files that end with .i are passed via ccom->as->ld
 * - Files that end with .s are passed as->ld
 * - Files that end with .S are passed via cpp->as->ld
 * - Files that end with .o are passed directly to ld
 * - Multiple files may be given on the command line.
 * - Unrecognized options are all sent directly to ld.
 * -c or -S cannot be combined with -o if multiple files are given.
 *
 * This file should be rewritten readable.
 *
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code and documentation must retain the above
 * copyright notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditionsand the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 * 	This product includes software developed or owned by Caldera
 *	International, Inc.
 * Neither the name of Caldera International, Inc. nor the names of other
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
 * INTERNATIONAL, INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OFLIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/types.h>
#include <sys/wait.h>

#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef CROSS
#   include </usr/include/stdio.h>
#   include </usr/include/errno.h>
#else
#   include <stdio.h>
#   include <errno.h>
#endif

#define	MKS(x) _MKS(x)
#define _MKS(x) #x

/*
 * Many specific definitions, should be declared elsewhere.
 */

#ifndef STDINC
#define	STDINC	  	"/usr/include"
#endif

#ifndef LIBDIR
#define LIBDIR		"/usr/lib"
#endif

#ifndef LIBEXECDIR
#define LIBEXECDIR	"/usr/libexec"
#endif

#ifndef PREPROCESSOR
#define PREPROCESSOR	"cpp"
#endif

#ifndef COMPILER
#define COMPILER	"ccom"
#endif

#ifndef ASSEMBLER
#define ASSEMBLER	"as"
#endif

#ifndef LINKER
#define LINKER		"ld"
#endif

#define MAXFIL 1000
#define MAXLIB 1000
#define MAXAV  1000
#define MAXOPT 100
char	*tmp_as;
char	*tmp_cpp;
char	*outfile, *ermfile;
char *Bprefix(char *);
char *copy(char *, int);
char *setsuf(char *, char);
int getsuf(char *);
int main(int, char *[]);
void error(char *, ...);
void errorx(int, char *, ...);
int callsys(char [], char *[]);
int cunlink(char *);
void dexit(int);
void idexit(int);
char *gettmp(void);
void *ccmalloc(int size);
char	*av[MAXAV];
char	*clist[MAXFIL];
char    *olist[MAXFIL];
char	*llist[MAXLIB];
char	*aslist[MAXAV];
char	*cpplist[MAXAV];
char	alist[20];
char	*xlist[100];
int	xnum;
char	*mlist[100];
char	*flist[100];
char	*wlist[100];
char	*idirafter;
char    *progname;
int	nm;
int	nf;
int	nw;
int	sspflag;
int	dflag;
int	pflag;
int	sflag;
int	cflag;
int	eflag;
int	gflag;
int	rflag;
int	vflag;
int	tflag;
int	Eflag;
int	Oflag;
int	kflag;	/* generate PIC/pic code */
#define F_PIC	1
#define F_pic	2
int	Mflag;	/* dependencies only */
int	pgflag;
int	exfail;
int	Xflag;
int	Wallflag;
int	Wflag;
int	nostartfiles, Bstatic, shared;
int	nostdinc, nostdlib;
int	onlyas;
int	pthreads;
int	xcflag;
int 	ascpp;

char	*passp = "/bin/" PREPROCESSOR;
char	*pass0 = LIBEXECDIR "/" COMPILER;
char	*as = ASSEMBLER;
char	*ld = LINKER;
char	*Bflag;

enum {
    MODE_LCC,
    MODE_PCC,
    MODE_SMALLC,
    MODE_SMALLERC,
} mode;

/* common cpp predefines */
char *cppadd[] = { "-D__LCC__", "-D__unix__", "-D__BSD__", "-D__RETROBSD__", NULL };

#ifdef __mips__
#   define	CPPMDADD { "-D__mips__", NULL, }
#endif
#ifdef __i386__
#   define	CPPMDADD { "-D__i386__", NULL, }
#endif

#ifdef DYNLINKER
char *dynlinker[] = DYNLINKER;
#endif
#ifdef CRT0FILE
char *crt0file = CRT0FILE;
#endif
#ifdef CRT0FILE_PROFILE
char *crt0file_profile = CRT0FILE_PROFILE;
#endif
#ifdef STARTFILES
char *startfiles[] = STARTFILES;
char *endfiles[] = ENDFILES;
#endif
#ifdef STARTFILES_T
char *startfiles_T[] = STARTFILES_T;
char *endfiles_T[] = ENDFILES_T;
#endif
#ifdef STARTFILES_S
char *startfiles_S[] = STARTFILES_S;
char *endfiles_S[] = ENDFILES_S;
#endif
#ifdef MULTITARGET
char *mach = DEFMACH;
struct cppmd {
	char *mach;
	char *cppmdadd[MAXCPPMDARGS];
};

struct cppmd cppmds[] = CPPMDADDS;
#else
char *cppmdadd[] = CPPMDADD;
#endif
#ifdef LIBCLIBS
char *libclibs[] = LIBCLIBS;
#else
char *libclibs[] = { "-lc", NULL };
#endif
#ifdef LIBCLIBS_PROFILE
char *libclibs_profile[] = LIBCLIBS_PROFILE;
#else
char *libclibs_profile[] = { "-lc_p", NULL };
#endif
#ifndef STARTLABEL
#define STARTLABEL "_start"
#endif
char *incdir = STDINC;
char *libdir = LIBDIR;
char *altincdir;
char *pccincdir;
char *pcclibdir;

/* handle gcc warning emulations */
struct Wflags {
	char *name;
	int flags;
#define	INWALL		1
#define	NEGATIVE	2
} Wflags[] = {
	{ "-Wtruncate", 0 },
	{ "-Wno-truncate", NEGATIVE },
	{ "-Werror", 0 },
	{ "-Wshadow", 0 },
	{ "-Wno-shadow", NEGATIVE },
	{ "-Wpointer-sign", INWALL },
	{ "-Wno-pointer-sign", NEGATIVE },
	{ "-Wsign-compare", 0 },
	{ "-Wno-sign-compare", NEGATIVE },
	{ "-Wunknown-pragmas", INWALL },
	{ "-Wno-unknown-pragmas", NEGATIVE },
	{ "-Wunreachable-code", 0 },
	{ "-Wno-unreachable-code", NEGATIVE },
	{ 0, 0 },
};

#define	SZWFL	(sizeof(Wflags)/sizeof(Wflags[0]))

#ifndef USHORT
/* copied from mip/manifest.h */
#define	USHORT		5
#define	INT		6
#define	UNSIGNED	7
#endif

/*
 * Wide char defines.
 */
#define	WCT "short unsigned int"
#define WCM "65535U"
#define WCS 2

#ifdef GCC_COMPAT
#ifndef REGISTER_PREFIX
#define REGISTER_PREFIX ""
#endif
#ifndef USER_LABEL_PREFIX
#define USER_LABEL_PREFIX ""
#endif
#endif

#ifndef PCC_PTRDIFF_TYPE
#define PCC_PTRDIFF_TYPE "long int"
#endif

/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t
strlcpy(char *dst, const char *src, size_t siz)
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

/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(initial dst) + strlen(src); if retval >= siz,
 * truncation occurred.
 */
size_t
strlcat(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (n-- != 0 && *d != '\0')
		d++;
	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return(dlen + strlen(s));
	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return(dlen + (s - src));	/* count does not include NUL */
}

void
usage()
{
	printf("Usage: %s [options] file...\n", progname);
	printf("Options:\n");
	printf("  -h               Display this information\n");
	printf("  --version        Display compiler version information\n");
	printf("  -c               Compile and assemble, but do not link\n");
	printf("  -S               Compile only; do not assemble or link\n");
	printf("  -E               Preprocess only; do not compile, assemble or link\n");
	printf("  -P               Preprocess to .i output file\n");
	printf("  -o <file>        Place the output into <file>\n");
	printf("  -O, -O0          Enable, disable optimization\n");
	printf("  -g               Create debug output\n");
	printf("  -v               Display the programs invoked by the compiler\n");
        if (mode == MODE_LCC || mode == MODE_PCC) {
	printf("  -k               Generate position-independent code\n");
	printf("  -Wall            Enable gcc-compatible warnings\n");
	printf("  -WW              Enable all warnings\n");
	printf("  -p, -pg          Generate profiled code\n");
	printf("  -r               Generate relocatable code\n");
        }
	printf("  -t               Use traditional preprocessor syntax\n");
	printf("  -C <option>      Pass preprocessor option\n");
	printf("  -Dname=val       Define preprocessor symbol\n");
	printf("  -Uname           Undefine preprocessor symbol\n");
	printf("  -Ipath           Add a directory to preprocessor path\n");
	printf("  -x <language>    Specify the language of the following input files\n");
	printf("                   Permissible languages include: c assembler-with-cpp\n");
        if (mode == MODE_LCC || mode == MODE_PCC) {
	printf("  -B <directory>   Add <directory> to the compiler's search paths\n");
	printf("  -m<option>       Target-dependent options\n");
	printf("  -f<option>       GCC-compatible flags: -fPI -fpicC -fsigned-char\n");
	printf("                   -fno-signed-char -funsigned-char -fno-unsigned-char\n");
	printf("                   -fstack-protector -fstack-protector-all\n");
	printf("                   -fno-stack-protector -fno-stack-protector-all\n");
        }
	printf("  -isystem dir     Add a system include directory\n");
	printf("  -include dir     Add an include directory\n");
	printf("  -idirafter dir   Set a last include directory\n");
	printf("  -nostdinc        Disable standard include directories\n");
	printf("  -nostdlib        Disable standard libraries and start files\n");
	printf("  -nostartfiles    Disable standard start files\n");
	printf("  -Wa,<options>    Pass comma-separated <options> on to the assembler\n");
	printf("  -Wp,<options>    Pass comma-separated <options> on to the preprocessor\n");
	printf("  -Wl,<options>    Pass comma-separated <options> on to the linker\n");
	printf("  -Wc,<options>    Pass comma-separated <options> on to the compiler\n");
	printf("  -M               Output a list of dependencies\n");
	printf("  -X               Leave temporary files\n");
	//printf("  -d               Debug mode ???\n");
        exit(0);
}

int
main(int argc, char *argv[])
{
	struct Wflags *Wf;
	char *t, *u;
	char *assource;
	char **pv, *ptemp[MAXOPT], **pvt;
	int nc, nl, nas, ncpp, i, j, c, nxo, na;
#ifdef MULTITARGET
	int k;
#endif

#ifdef INCLUDEDIR
        altincdir = INCLUDEDIR "pcc/";
#endif
#ifdef PCCINCDIR
        pccincdir = PCCINCDIR;
#endif
#ifdef PCCLIBDIR
        pcclibdir = PCCLIBDIR;
#endif

        progname = strrchr (argv[0], '/');
        progname = progname ? progname+1 : argv[0];

        /*
         * Select a compiler mode.
         */
        if (strcmp ("pcc", progname) == 0) {
                /* PCC: portable C compiler. */
                mode = MODE_PCC;
                cppadd[0] = "-D__PCC__";
                pass0 = LIBEXECDIR "/ccom";

        } else if (strcmp ("scc", progname) == 0) {
                /* SmallC. */
                mode = MODE_SMALLC;
                cppadd[0] = "-D__SMALLC__";
                pass0 = LIBEXECDIR "/smallc";
                incdir = STDINC "/smallc";

        } else if (strcmp ("lcc", progname) == 0) {
                /* LCC: retargetable C compiler. */
                mode = MODE_LCC;
                cppadd[0] = "-D__LCC__";
                pass0 = LIBEXECDIR "/lccom";
        } else {
                /* Smaller C. */
                mode = MODE_SMALLERC;
                cppadd[0] = "-D__SMALLER_C__";
                pass0 = LIBEXECDIR "/smlrc";
        }

        if (argc == 1)
                usage();
	i = nc = nl = nas = ncpp = nxo = 0;
	pv = ptemp;
	while(++i < argc) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
			default:
				goto passa;
			case 'h':
			        usage();
#ifdef notyet
	/* must add library options first (-L/-l/...) */
				error("unrecognized option `-%c'", argv[i][1]);
				break;
#endif

			case '-': /* double -'s */
				if (strcmp(argv[i], "--version") == 0) {
					printf("%s\n", VERSSTR);
					return 0;
				} else if (strcmp(argv[i], "--param") == 0)
					/* NOTHING YET */;
				else
					goto passa;
				break;

			case 'B': /* other search paths for binaries */
				Bflag = &argv[i][2];
				break;

#ifdef MULTITARGET
			case 'b':
				t = &argv[i][2];
				if (*t == '\0' && i + 1 < argc) {
					t = argv[i+1];
					i++;
				}
				if (strncmp(t, "?", 1) == 0) {
					/* show machine targets */
					printf("Available machine targets:");
					for (j=0; cppmds[j].mach; j++)
						printf(" %s",cppmds[j].mach);
					printf("\n");
					exit(0);
				}
				for (j=0; cppmds[j].mach; j++)
					if (strcmp(t, cppmds[j].mach) == 0) {
						mach = cppmds[j].mach;
						break;
					}
				if (cppmds[j].mach == NULL)
					errorx(1, "unknown target arch %s", t);
				break;
#endif

			case 'X':
				Xflag++;
				break;
			case 'W': /* Ignore (most of) W-flags */
				if (strncmp(argv[i], "-Wl,", 4) == 0) {
					/* options to the linker */
					t = &argv[i][4];
					while ((u = strchr(t, ','))) {
						*u++ = 0;
						llist[nl++] = t;
						t = u;
					}
					llist[nl++] = t;
				} else if (strncmp(argv[i], "-Wa,", 4) == 0) {
					/* options to the assembler */
					t = &argv[i][4];
					while ((u = strchr(t, ','))) {
						*u++ = 0;
						aslist[nas++] = t;
						t = u;
					}
					aslist[nas++] = t;
				} else if (strncmp(argv[i], "-Wc,", 4) == 0) {
					/* options to ccom */
					t = &argv[i][4];
					while ((u = strchr(t, ','))) {
						*u++ = 0;
						wlist[nw++] = t;
						t = u;
					}
					wlist[nw++] = t;
				} else if (strncmp(argv[i], "-Wp,", 4) == 0) {
					/* preprocessor */
					t = &argv[i][4];
					while ((u = strchr(t, ','))) {
						*u++ = 0;
						cpplist[ncpp++] = t;
						t = u;
					}
					cpplist[ncpp++] = t;
				} else if (strcmp(argv[i], "-Wall") == 0) {
					Wallflag = 1;
				} else if (strcmp(argv[i], "-WW") == 0) {
					Wflag = 1;
				} else {
					/* check and set if available */
					for (Wf = Wflags; Wf->name; Wf++) {
						if (strcmp(argv[i], Wf->name))
							continue;
						wlist[nw++] = Wf->name;
					}
				}
				break;

			case 'f': /* GCC compatibility flags */
				if (strcmp(argv[i], "-fPIC") == 0)
					kflag = F_PIC;
				else if (strcmp(argv[i], "-fpic") == 0)
					kflag = F_pic;
				else if (strcmp(argv[i],
				    "-fsigned-char") == 0)
					flist[nf++] = argv[i];
				else if (strcmp(argv[i],
				    "-fno-signed-char") == 0)
					flist[nf++] = argv[i];
				else if (strcmp(argv[i],
				    "-funsigned-char") == 0)
					flist[nf++] = argv[i];
				else if (strcmp(argv[i],
				    "-fno-unsigned-char") == 0)
					flist[nf++] = argv[i];
				else if (strcmp(argv[i],
				    "-fstack-protector") == 0) {
					flist[nf++] = argv[i];
					sspflag++;
				} else if (strcmp(argv[i],
				    "-fstack-protector-all") == 0) {
					flist[nf++] = argv[i];
					sspflag++;
				} else if (strcmp(argv[i],
				    "-fno-stack-protector") == 0) {
					flist[nf++] = argv[i];
					sspflag = 0;
				} else if (strcmp(argv[i],
				    "-fno-stack-protector-all") == 0) {
					flist[nf++] = argv[i];
					sspflag = 0;
				}
				/* silently ignore the rest */
				break;

			case 'g': /* create debug output */
				gflag++;
				break;

			case 'i':
				if (strcmp(argv[i], "-isystem") == 0) {
					*pv++ = "-S";
					*pv++ = argv[++i];
				} else if (strcmp(argv[i], "-include") == 0) {
					*pv++ = "-i";
					*pv++ = argv[++i];
				} else if (strcmp(argv[i], "-idirafter") == 0) {
					idirafter = argv[++i];
				} else
					goto passa;
				break;

			case 'k': /* generate PIC code */
				kflag = F_pic;
				break;

			case 'm': /* target-dependent options */
				mlist[nm++] = argv[i];
				break;

			case 'n': /* handle -n flags */
				if (strcmp(argv[i], "-nostdinc") == 0)
					nostdinc++;
				else if (strcmp(argv[i], "-nostdlib") == 0) {
					nostdlib++;
					nostartfiles++;
				} else if (strcmp(argv[i], "-nostartfiles") == 0)
					nostartfiles = 1;
				else
					goto passa;
				break;

			case 'p':
				if (strcmp(argv[i], "-pg") == 0 ||
				    strcmp(argv[i], "-p") == 0)
					pgflag++;
				else if (strcmp(argv[i], "-pthread") == 0)
					pthreads++;
				else if (strcmp(argv[i], "-pipe") == 0)
					/* NOTHING YET */;
				else if (strcmp(argv[i], "-pedantic") == 0)
					/* NOTHING YET */;
				else if (strcmp(argv[i],
				    "-print-prog-name=ld") == 0) {
					printf("%s\n", LINKER);
					return 0;
				} else
					errorx(1, "unknown option %s", argv[i]);
				break;

			case 'r':
				rflag = 1;
				break;

			case 'x':
				t = &argv[i][2];
				if (*t == 0)
					t = argv[++i];
				if (strcmp(t, "c") == 0)
					xcflag = 1; /* default */
				else if (strcmp(t, "assembler-with-cpp") == 0)
					ascpp = 1;
#ifdef notyet
				else if (strcmp(t, "c++") == 0)
					cxxflag++;
#endif
				else
					xlist[xnum++] = argv[i];
				break;
			case 't':
				tflag++;
				break;
			case 'S':
				sflag++;
				cflag++;
				break;
			case 'o':
				if (outfile)
					errorx(8, "too many -o");
				outfile = argv[++i];
				break;
			case 'O':
				if (argv[i][2] == '0')
					Oflag = 0;
				else
					Oflag++;
				break;
			case 'E':
				Eflag++;
				break;
			case 'P':
				pflag++;
				*pv++ = argv[i];
			case 'c':
				cflag++;
				break;

#if 0
			case '2':
				if(argv[i][2] == '\0')
					pref = "/lib/crt2.o";
				else {
					pref = "/lib/crt20.o";
				}
				break;
#endif
			case 'C':
				cpplist[ncpp++] = argv[i];
				break;
			case 'D':
			case 'I':
			case 'U':
				*pv++ = argv[i];
				if (argv[i][2] == 0)
					*pv++ = argv[++i];
				if (pv >= ptemp+MAXOPT) {
					error("Too many DIU options");
					--pv;
				}
				break;

			case 'M':
				Mflag++;
				break;

			case 'd':
				if (strcmp(argv[i], "-d") == 0) {
					dflag++;
					strlcpy(alist, argv[i], sizeof (alist));
				}
				break;
			case 'v':
				printf("%s\n", VERSSTR);
				vflag++;
				break;

			case 's':
				if (strcmp(argv[i], "-shared") == 0) {
					shared = 1;
					nostdlib = 1;

				} else if (strcmp(argv[i], "-static") == 0) {
					Bstatic = 1;

				} else if (strncmp(argv[i], "-std", 4) == 0) {
					/* ignore gcc -std= */;
				} else
					goto passa;
				break;
			}
		} else {
		passa:
			t = argv[i];
			if (*argv[i] == '-' && argv[i][1] == 'L')
				;
			else if((c=getsuf(t))=='c' || c=='S' || c=='i' ||
			    c=='s'|| Eflag || xcflag) {
				clist[nc++] = t;
				if (nc>=MAXFIL) {
					error("Too many source files");
					exit(1);
				}
			}

			/* Check for duplicate .o files. */
			for (j = getsuf(t) == 'o' ? 0 : nl; j < nl; j++) {
				if (strcmp(llist[j], t) == 0)
					break;
			}
			if ((c=getsuf(t))!='c' && c!='S' &&
			    c!='s' && c!='i' && j==nl) {
				llist[nl++] = t;
				if (nl >= MAXLIB) {
					error("Too many object/library files");
					exit(1);
				}
				if (getsuf(t)=='o')
					nxo++;
			}
		}
	}
	/* Sanity checking */
	if (nc == 0 && nl == 0)
		errorx(8, "no input files");
	if (outfile && (cflag || sflag || Eflag) && nc > 1)
		errorx(8, "-o given with -c || -E || -S and more than one file");
	if (outfile && clist[0] && strcmp(outfile, clist[0]) == 0)
		errorx(8, "output file will be clobbered");
	if (nc==0)
		goto nocom;
	if (pflag==0) {
		if (!sflag)
			tmp_as = gettmp();
		tmp_cpp = gettmp();
	}
	if (Bflag) {
                if (altincdir)
                        altincdir = Bflag;
                if (pccincdir)
                        pccincdir = Bflag;
                if (pcclibdir)
                        pcclibdir = Bflag;
	}
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)	/* interrupt */
		signal(SIGINT, idexit);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)	/* terminate */
		signal(SIGTERM, idexit);
#ifdef MULTITARGET
	pass0 = copy(LIBEXECDIR "/ccom_", k = strlen(mach));
	strlcat(pass0, mach, sizeof(LIBEXECDIR "/ccom_") + k);
#endif
	pvt = pv;
	for (i=0; i<nc; i++) {
		/*
		 * C preprocessor
		 */
		if (nc>1 && !Eflag)
			printf("%s:\n", clist[i]);
		onlyas = 0;
		assource = tmp_as;
		if (getsuf(clist[i])=='S')
			ascpp = 1;
		if (getsuf(clist[i])=='i') {
			if(Eflag)
				continue;
			goto com;
		} else if (ascpp) {
			onlyas = 1;
		} else if (getsuf(clist[i])=='s') {
			assource = clist[i];
			goto assemble;
		}
		if (pflag)
			tmp_cpp = setsuf(clist[i], 'i');
		na = 0;
		av[na++] = "cpp";
		if (vflag)
			av[na++] = "-v";
                if (mode == MODE_PCC) {
                        av[na++] = "-D__PCC__=" MKS(PCC_MAJOR);
                        av[na++] = "-D__PCC_MINOR__=" MKS(PCC_MINOR);
                        av[na++] = "-D__PCC_MINORMINOR__=" MKS(PCC_MINORMINOR);
                }
#ifdef GCC_COMPAT
                av[na++] = "-D__GNUC__=4";
                av[na++] = "-D__GNUC_MINOR__=3";
                av[na++] = "-D__GNUC_PATCHLEVEL__=1";
                av[na++] = "-D__GNUC_STDC_INLINE__=1";
#endif
		if (ascpp)
			av[na++] = "-D__ASSEMBLER__";
		if (sspflag)
			av[na++] = "-D__SSP__=1";
		if (pthreads)
			av[na++] = "-D_PTHREADS";
		if (Mflag)
			av[na++] = "-M";
		if (Oflag)
			av[na++] = "-D__OPTIMIZE__";
#ifdef GCC_COMPAT
		av[na++] = "-D__REGISTER_PREFIX__=" REGISTER_PREFIX;
		av[na++] = "-D__USER_LABEL_PREFIX__=" USER_LABEL_PREFIX;
		if (Oflag)
			av[na++] = "-D__OPTIMIZE__";
#endif
		if (dflag)
			av[na++] = alist;
		for (j = 0; cppadd[j]; j++)
			av[na++] = cppadd[j];
		for (j = 0; j < ncpp; j++)
			av[na++] = cpplist[j];
		av[na++] = "-D__STDC_ISO_10646__=200009L";
		av[na++] = "-D__WCHAR_TYPE__=" WCT;
		av[na++] = "-D__SIZEOF_WCHAR_T__=" MKS(WCS);
		av[na++] = "-D__WCHAR_MAX__=" WCM;
		av[na++] = "-D__WINT_TYPE__=unsigned int";
		av[na++] = "-D__SIZE_TYPE__=unsigned long";
		av[na++] = "-D__PTRDIFF_TYPE__=" PCC_PTRDIFF_TYPE;
		av[na++] = "-D__SIZEOF_WINT_T__=4";
#ifdef MULTITARGET
		for (k = 0; cppmds[k].mach; k++) {
			if (strcmp(cppmds[k].mach, mach) != 0)
				continue;
			for (j = 0; cppmds[k].cppmdadd[j]; j++)
				av[na++] = cppmds[k].cppmdadd[j];
			break;
		}
#else
		for (j = 0; cppmdadd[j]; j++)
			av[na++] = cppmdadd[j];
#endif
		if (tflag)
			av[na++] = "-t";
		for(pv=ptemp; pv <pvt; pv++)
			av[na++] = *pv;
		if (!nostdinc) {
                        if (altincdir)
                                av[na++] = "-S", av[na++] = altincdir;
			av[na++] = "-S", av[na++] = incdir;
                        if (pccincdir)
                                av[na++] = "-S", av[na++] = pccincdir;
		}
		if (idirafter) {
			av[na++] = "-I";
			av[na++] = idirafter;
		}
		av[na++] = clist[i];
		if (!Eflag && !Mflag)
                        av[na++] = tmp_cpp;
		if ((Eflag || Mflag) && outfile)
			ermfile = av[na++] = outfile;
		av[na++]=0;
		if (callsys(passp, av)) {
			exfail++;
			eflag++;
		}
		if (Eflag || Mflag)
			continue;
		if (onlyas) {
			assource = tmp_cpp;
			goto assemble;
		}

		/*
		 * C compiler
		 */
	com:
		na = 0;
		av[na++]= "ccom";
		if (Wallflag) {
			/* Set only the same flags as gcc */
			for (Wf = Wflags; Wf->name; Wf++) {
				if (Wf->flags != INWALL)
					continue;
				av[na++] = Wf->name;
			}
		}
		if (Wflag) {
			/* set all positive flags */
			for (Wf = Wflags; Wf->name; Wf++) {
				if (Wf->flags == NEGATIVE)
					continue;
				av[na++] = Wf->name;
			}
		}
		for (j = 0; j < nw; j++)
			av[na++] = wlist[j];
		for (j = 0; j < nf; j++)
			av[na++] = flist[j];
		if (vflag)
			av[na++] = "-v";
		if (pgflag)
			av[na++] = "-p";
		if (gflag)
			av[na++] = "-g";
		if (kflag)
			av[na++] = "-k";
		if (Oflag) {
			av[na++] = "-xtemps";
			av[na++] = "-xdeljumps";
			av[na++] = "-xinline";
		}
		for (j = 0; j < xnum; j++)
			av[na++] = xlist[j];
		for (j = 0; j < nm; j++)
			av[na++] = mlist[j];
		if (getsuf(clist[i])=='i')
			av[na++] = clist[i];
		else
			av[na++] = tmp_cpp; /* created by cpp */
		if (pflag || exfail) {
			cflag++;
			continue;
		}
		if (sflag) {
			if (outfile)
				tmp_as = outfile;
			else
				tmp_as = setsuf(clist[i], 's');
		}
		ermfile = av[na++] = tmp_as;
#if 0
		if (proflag) {
			av[3] = "-XP";
			av[4] = 0;
		} else
			av[3] = 0;
#endif
		av[na++] = NULL;
		if (callsys(pass0, av)) {
			cflag++;
			eflag++;
			continue;
		}
		if (sflag)
			continue;

		/*
		 * Assembler
		 */
	assemble:
		na = 0;
		av[na++] = as;
		for (j = 0; j < nas; j++)
			av[na++] = aslist[j];
		if (vflag)
			av[na++] = "-v";
		if (kflag)
			av[na++] = "-k";
		av[na++] = "-o";
		if (outfile && cflag)
			ermfile = av[na++] = outfile;
		else if (cflag)
			ermfile = av[na++] = olist[i] = setsuf(clist[i], 'o');
		else
			ermfile = av[na++] = olist[i] = gettmp();
		av[na++] = assource;
		if (dflag)
			av[na++] = alist;
		av[na++] = 0;
		if (callsys(as, av)) {
			cflag++;
			eflag++;
                        if (ascpp)
                                cunlink(tmp_cpp);
			continue;
		}
                if (ascpp)
                        cunlink(tmp_cpp);
	}

	if (Eflag || Mflag)
		dexit(eflag);

	/*
	 * Linker
	 */
nocom:
	if (cflag==0 && nc+nl != 0) {
		j = 0;
		av[j++] = ld;
		if (vflag)
			av[j++] = "-v";
		av[j++] = "-X";
		if (shared) {
			av[j++] = "-shared";
		} else {
			av[j++] = "-d";
			if (rflag) {
				av[j++] = "-r";
			} else {
				av[j++] = "-e";
				av[j++] = STARTLABEL;
			}
			if (Bstatic == 0) { /* Dynamic linkage */
#ifdef DYNLINKER
				for (i = 0; dynlinker[i]; i++)
					av[j++] = dynlinker[i];
#endif
			} else {
				av[j++] = "-Bstatic";
			}
		}
		if (outfile) {
			av[j++] = "-o";
			av[j++] = outfile;
		}
#ifdef STARTFILES_S
		if (shared) {
			if (!nostartfiles) {
				for (i = 0; startfiles_S[i]; i++)
					av[j++] = Bprefix(startfiles_S[i]);
			}
		} else
#endif
		{
			if (!nostartfiles) {
#ifdef CRT0FILE_PROFILE
				if (pgflag) {
					av[j++] = Bprefix(crt0file_profile);
				} else
#endif
				{
#ifdef CRT0FILE
					av[j++] = Bprefix(crt0file);
#endif
				}
#ifdef STARTFILES_T
				if (Bstatic) {
					for (i = 0; startfiles_T[i]; i++)
						av[j++] = Bprefix(startfiles_T[i]);
				} else
#endif
				{
#ifdef STARTFILES
					for (i = 0; startfiles[i]; i++)
						av[j++] = Bprefix(startfiles[i]);
#endif
				}
			}
		}
		i = 0;
		while (i<nc) {
			av[j++] = olist[i++];
			if (j >= MAXAV)
				error("Too many ld options");
		}
		i = 0;
		while(i<nl) {
			av[j++] = llist[i++];
			if (j >= MAXAV)
				error("Too many ld options");
		}
		if (gflag)
			av[j++] = "-g";
#if 0
		if (gflag)
			av[j++] = "-lg";
#endif
		if (pthreads)
			av[j++] = "-lpthread";
		if (!nostdlib) {
#define	DL	"-L"
			char *s;
                        if (pcclibdir) {
                                s = copy(DL, i = strlen(pcclibdir));
                                strlcat(s, pcclibdir, sizeof(DL) + i);
                                av[j++] = s;
                        }
			if (pgflag) {
				for (i = 0; libclibs_profile[i]; i++)
					av[j++] = Bprefix(libclibs_profile[i]);
			} else {
				for (i = 0; libclibs[i]; i++)
					av[j++] = Bprefix(libclibs[i]);
			}
		}
		if (!nostartfiles) {
#ifdef STARTFILES_S
			if (shared) {
				for (i = 0; endfiles_S[i]; i++)
					av[j++] = Bprefix(endfiles_S[i]);
			} else
#endif
			{
#ifdef STARTFILES_T
				if (Bstatic) {
					for (i = 0; endfiles_T[i]; i++)
						av[j++] = Bprefix(endfiles_T[i]);
				} else
#endif
				{
#ifdef STARTFILES
					for (i = 0; endfiles[i]; i++)
						av[j++] = Bprefix(endfiles[i]);
#endif
				}
			}
		}
		av[j++] = 0;
		eflag |= callsys(ld, av);
		if (nc==1 && nxo==1 && eflag==0)
			cunlink(olist[0]);
		else if (nc > 0 && eflag == 0) {
			/* remove .o files XXX ugly */
			for (i = 0; i < nc; i++)
				cunlink(olist[i]);
		}
	}
	dexit(eflag);
	return 0;
}

/*
 * exit and cleanup after interrupt.
 */
void
idexit(int arg)
{
	dexit(100);
}

/*
 * exit and cleanup.
 */
void
dexit(int eval)
{
	if (!pflag && !Xflag) {
		if (sflag==0)
			cunlink(tmp_as);
		cunlink(tmp_cpp);
	}
	if (exfail || eflag)
		cunlink(ermfile);
	if (eval == 100)
		_exit(eval);
	exit(eval);
}

static void
ccerror(char *s, va_list ap)
{
	vfprintf(Eflag ? stderr : stdout, s, ap);
	putc('\n', Eflag? stderr : stdout);
	exfail++;
	cflag++;
	eflag++;
}

/*
 * complain a bit.
 */
void
error(char *s, ...)
{
	va_list ap;

	va_start(ap, s);
	ccerror(s, ap);
	va_end(ap);
}

/*
 * complain a bit and then exit.
 */
void
errorx(int eval, char *s, ...)
{
	va_list ap;

	va_start(ap, s);
	ccerror(s, ap);
	va_end(ap);
	dexit(eval);
}

char *
Bprefix(char *s)
{
	char *suffix;
	char *str;
	int i;

	if (Bflag == NULL || s[0] != '/')
		return s;
	suffix = strrchr(s, '/');

	if (suffix == NULL)
		suffix = s;

	str = copy(Bflag, i = strlen(suffix));
	strlcat(str, suffix, strlen(Bflag) + i + 1);
	return str;
}

int
getsuf(char *s)
{
	register char *p;

	if ((p = strrchr(s, '.')) && p[1] != '\0' && p[2] == '\0')
		return p[1];
	return(0);
}

/*
 * Get basename of string s and change its suffix to ch.
 */
char *
setsuf(char *s, char ch)
{
	char *p, *lastp;
	int len;

	/* Strip trailing slashes, if any. */
	lastp = s + strlen(s) - 1;
	while (lastp != s && *lastp == '/')
		lastp--;

	/* Now find the beginning of this (final) component. */
	p = lastp;
	while (p != s && *(p - 1) != '/')
		p--;

	/* ...and copy the result into the result buffer. */
	len = (lastp - p) + 1;
	s = ccmalloc(len + 3);
	memcpy(s, p, len);
	s[len] = '\0';

        /* Find and change a suffix */
	p = strrchr(s, '.');
	if (! p) {
		p = s + len;
		p[0] = '.';
	}
	p[1] = ch;
	p[2] = '\0';
	return(s);
}

int
callsys(char *f, char *v[])
{
	int t, status = 0;
	pid_t p;
	char *s;
	char * volatile a = NULL;
	volatile size_t len;

	if (vflag) {
		fprintf(stderr, "%s ", f);
		for (t = 1; v[t]; t++)
			fprintf(stderr, "%s ", v[t]);
		fprintf(stderr, "\n");
	}

	if (Bflag) {
		len = strlen(Bflag) + 8;
		a = malloc(len);
	}
#ifdef HAVE_VFORK
	if ((p = vfork()) == 0) {
#else
	if ((p = fork()) == 0) {
#endif
		if (Bflag) {
			if (a == NULL) {
				error("callsys: malloc failed");
				exit(1);
			}
			if ((s = strrchr(f, '/'))) {
				strlcpy(a, Bflag, len);
				strlcat(a, s, len);
				execv(a, v);
			}
		}
		execvp(f, v);
		if ((s = strrchr(f, '/')))
			execvp(s+1, v);
		fprintf(stderr, "Can't find %s\n", f);
		_exit(100);
	}
	if (p == -1) {
		fprintf(stderr, "fork() failed, try again\n");
		return(100);
	}
	if (Bflag) {
		free(a);
	}
	while (waitpid(p, &status, 0) == -1 && errno == EINTR)
		;
	if (WIFEXITED(status))
		return (WEXITSTATUS(status));
	if (WIFSIGNALED(status))
		dexit(eflag ? eflag : 1);
	errorx(8, "Fatal error in %s", f);

	return 0;
}

/*
 * Make a copy of string as, mallocing extra bytes in the string.
 */
char *
copy(char *s, int extra)
{
	int len = strlen(s)+1;
	char *rv;

	rv = ccmalloc(len+extra);
	strlcpy(rv, s, len);
	return rv;
}

int
cunlink(char *f)
{
	if (f==0 || Xflag)
		return(0);
	return (unlink(f));
}

char *
gettmp(void)
{
	char *sfn = copy("/tmp/ctm.XXXXXX", 0);
	int fd = -1;

	if ((fd = mkstemp(sfn)) == -1) {
		fprintf(stderr, "%s: %s\n", sfn, strerror(errno));
		exit(8);
	}
	close(fd);
	return sfn;
}

void *
ccmalloc(int size)
{
	void *rv;

	if ((rv = malloc(size)) == NULL)
		error("malloc failed");
	return rv;
}
