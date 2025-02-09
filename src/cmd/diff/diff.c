/*
 * diff - driver and subroutines
 */
#include "diff.h"

int	opt;
int	tflag;			/* expand tabs on output */
int	hflag;			/* -h, use halfhearted DIFFH */
int	bflag;			/* ignore blanks in comparisons */
int	wflag;			/* totally ignore blanks in comparisons */
int	iflag;			/* ignore case in comparisons */
int	lflag;			/* long output format with header */
int	rflag;			/* recursively trace directories */
int	sflag;			/* announce files which are same */
char	*start;			/* do file only if name >= this */
int	wantelses;		/* -E */
char	*ifdef1;		/* String for -1 */
char	*ifdef2;		/* String for -2 */
char	*endifname;		/* What we will print on next #endif */
int	inifdef;
int	context;		/* lines of context to be printed */
int	status;
int	anychange;
char	*tempfile;		/* used when comparing against std input */
char	**diffargv;		/* option list to pass to recursive diffs */
char	*file1, *file2, *efile1, *efile2;
struct	stat stb1, stb2;

char	diff[] = DIFF;
char	diffh[] = DIFFH;
char	pr[] = PR;

void noroom(void);

int
main(int argc, char **argv)
{
	register char *argp;

	ifdef1 = "FILE1"; ifdef2 = "FILE2";
	status = 2;
	diffargv = argv;
	argc--, argv++;
	while (argc > 2 && argv[0][0] == '-') {
		argp = &argv[0][1];
		argv++, argc--;
		while (*argp) switch(*argp++) {

#ifdef notdef
		case 'I':
			opt = D_IFDEF;
			wantelses = 0;
			continue;
		case 'E':
			opt = D_IFDEF;
			wantelses = 1;
			continue;
		case '1':
			opt = D_IFDEF;
			ifdef1 = argp;
			*--argp = 0;
			continue;
#endif
		case 'D':
			/* -Dfoo = -E -1 -2foo */
			wantelses = 1;
			ifdef1 = "";
			/* fall through */
#ifdef notdef
		case '2':
#endif
			opt = D_IFDEF;
			ifdef2 = argp;
			*--argp = 0;
			continue;
		case 'e':
			opt = D_EDIT;
			continue;
		case 'f':
			opt = D_REVERSE;
			continue;
		case 'n':
			opt = D_NREVERSE;
			continue;
		case 'b':
			bflag = 1;
			continue;
		case 'w':
			wflag = 1;
			continue;
		case 'i':
			iflag = 1;
			continue;
		case 't':
			tflag = 1;
			continue;
		case 'c':
			opt = D_CONTEXT;
			if (isdigit(*argp)) {
				context = atoi(argp);
				while (isdigit(*argp))
					argp++;
				if (*argp) {
					fprintf(stderr,
					    "diff: -c: bad count\n");
					done(0);
				}
				argp = "";
			} else
				context = 3;
			continue;
		case 'h':
			hflag++;
			continue;
		case 'S':
			if (*argp == 0) {
				fprintf(stderr, "diff: use -Sstart\n");
				done(0);
			}
			start = argp;
			*--argp = 0;		/* don't pass it on */
			continue;
		case 'r':
			rflag++;
			continue;
		case 's':
			sflag++;
			continue;
		case 'l':
			lflag++;
			continue;
		default:
			fprintf(stderr, "diff: -%s: unknown option\n",
			    --argp);
			done(0);
		}
	}
	if (argc != 2) {
		fprintf(stderr, "diff: two filename arguments required\n");
		done(0);
	}
	file1 = argv[0];
	file2 = argv[1];
	if (hflag && opt) {
		fprintf(stderr,
		    "diff: -h doesn't support -e, -f, -n, -c, or -I\n");
		done(0);
	}
	if (!strcmp(file1, "-"))
		stb1.st_mode = S_IFREG;
	else if (stat(file1, &stb1) < 0) {
		fprintf(stderr, "diff: ");
		perror(file1);
		done(0);
	}
	if (!strcmp(file2, "-"))
		stb2.st_mode = S_IFREG;
	else if (stat(file2, &stb2) < 0) {
		fprintf(stderr, "diff: ");
		perror(file2);
		done(0);
	}
	if ((stb1.st_mode & S_IFMT) == S_IFDIR &&
	    (stb2.st_mode & S_IFMT) == S_IFDIR) {
		diffdir(argv);
	} else
		diffreg();
	done(0);
}

char *
savestr(char *cp)
{
	register char *dp = malloc(strlen(cp)+1);

	if (dp == 0) {
		fprintf(stderr, "diff: ran out of memory\n");
		done(0);
	}
	strcpy(dp, cp);
	return (dp);
}

int
min(int a, int b)
{

	return (a < b ? a : b);
}

int
max(int a, int b)
{

	return (a > b ? a : b);
}

void
done(int sig)
{
	if (tempfile)
		unlink(tempfile);
	exit(status);
}

char *
talloc(int n)
{
	register char *p;

	p = malloc((unsigned)n);
	if (p == NULL)
		noroom();
	return(p);
}

char *
ralloc(char *p, int n)
{
	register char *q;

	if ((q = realloc(p, (unsigned)n)) == NULL)
		noroom();
	return(q);
}

void
noroom()
{
	fprintf(stderr, "diff: files too big, try -h\n");
	done(0);
}
