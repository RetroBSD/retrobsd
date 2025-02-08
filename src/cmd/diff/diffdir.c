/*
 * diff - directory comparison
 */
#include <sys/wait.h>
#include "diff.h"

#define	d_flags	d_ino

#define	ONLY	1		/* Only in this directory */
#define	SAME	2		/* Both places and same */
#define	DIFFER	4		/* Both places and different */
#define	DIRECT	8		/* Directory */

struct dir {
	ino_t	d_ino;
	short	d_reclen;
	short	d_namlen;
	char	*d_entry;
};

struct	dir *setupdir();
int	header;
char	title[2*BUFSIZ], *etitle;

void setfile(char **fpp, char **epp, char *file);
int useless(char *cp);
void only(struct dir *dp, int which);
void compare(struct dir *dp);
void scanpr(struct dir *dp, int test, char *title, char *file1, char *efile1, char *file2, char *efile2);
void calldiff(char *wantpr);
int ascii(int f);

void
diffdir(argv)
	char **argv;
{
	register struct dir *d1, *d2;
	struct dir *dir1, *dir2;
	register int i;
	int cmp;

	if (opt == D_IFDEF) {
		fprintf(stderr, "diff: can't specify -I with directories\n");
		done(0);
	}
	if (opt == D_EDIT && (sflag || lflag))
		fprintf(stderr,
		    "diff: warning: shouldn't give -s or -l with -e\n");
	title[0] = 0;
	strcpy(title, "diff ");
	for (i = 1; diffargv[i+2]; i++) {
		if (!strcmp(diffargv[i], "-"))
			continue;	/* was -S, dont look silly */
		strcat(title, diffargv[i]);
		strcat(title, " ");
	}
	for (etitle = title; *etitle; etitle++)
		;
	setfile(&file1, &efile1, file1);
	setfile(&file2, &efile2, file2);
	argv[0] = file1;
	argv[1] = file2;
	dir1 = setupdir(file1);
	dir2 = setupdir(file2);
	d1 = dir1; d2 = dir2;
	while (d1->d_entry != 0 || d2->d_entry != 0) {
		if (d1->d_entry && useless(d1->d_entry)) {
			d1++;
			continue;
		}
		if (d2->d_entry && useless(d2->d_entry)) {
			d2++;
			continue;
		}
		if (d1->d_entry == 0)
			cmp = 1;
		else if (d2->d_entry == 0)
			cmp = -1;
		else
			cmp = strcmp(d1->d_entry, d2->d_entry);
		if (cmp < 0) {
			if (lflag)
				d1->d_flags |= ONLY;
			else if (opt == 0 || opt == 2)
				only(d1, 1);
			d1++;
		} else if (cmp == 0) {
			compare(d1);
			d1++;
			d2++;
		} else {
			if (lflag)
				d2->d_flags |= ONLY;
			else if (opt == 0 || opt == 2)
				only(d2, 2);
			d2++;
		}
	}
	if (lflag) {
		scanpr(dir1, ONLY, "Only in %.*s", file1, efile1, 0, 0);
		scanpr(dir2, ONLY, "Only in %.*s", file2, efile2, 0, 0);
		scanpr(dir1, SAME, "Common identical files in %.*s and %.*s",
		    file1, efile1, file2, efile2);
		scanpr(dir1, DIFFER, "Binary files which differ in %.*s and %.*s",
		    file1, efile1, file2, efile2);
		scanpr(dir1, DIRECT, "Common subdirectories of %.*s and %.*s",
		    file1, efile1, file2, efile2);
	}
	if (rflag) {
		if (header && lflag)
			printf("\f");
		for (d1 = dir1; d1->d_entry; d1++)  {
			if ((d1->d_flags & DIRECT) == 0)
				continue;
			strcpy(efile1, d1->d_entry);
			strcpy(efile2, d1->d_entry);
			calldiff(0);
		}
	}
}

void
setfile(fpp, epp, file)
	char **fpp, **epp;
	char *file;
{
	register char *cp;

	*fpp = malloc(BUFSIZ);
	if (*fpp == 0) {
		fprintf(stderr, "diff: ran out of memory\n");
		exit(1);
	}
	strcpy(*fpp, file);
	for (cp = *fpp; *cp; cp++)
		continue;
	*cp++ = '/';
	*epp = cp;
}

void
scanpr(dp, test, title, file1, efile1, file2, efile2)
	register struct dir *dp;
	int test;
	char *title, *file1, *efile1, *file2, *efile2;
{
	int titled = 0;

	for (; dp->d_entry; dp++) {
		if ((dp->d_flags & test) == 0)
			continue;
		if (titled == 0) {
			if (header == 0)
				header = 1;
			else
				printf("\n");
			printf(title,
			    efile1 - file1 - 1, file1,
			    efile2 - file2 - 1, file2);
			printf(":\n");
			titled = 1;
		}
		printf("\t%s\n", dp->d_entry);
	}
}

void
only(dp, which)
	struct dir *dp;
	int which;
{
	char *file = which == 1 ? file1 : file2;
	char *efile = which == 1 ? efile1 : efile2;

	printf("Only in %.*s: %s\n", efile - file - 1, file, dp->d_entry);
}

int	entcmp();

struct dir *
setupdir(cp)
	char *cp;
{
	register struct dir *dp = 0, *ep;
	register struct direct *rp;
	register int nitems, n;
	DIR *dirp;

	dirp = opendir(cp);
	if (dirp == NULL) {
		fprintf(stderr, "diff: ");
		perror(cp);
		done(0);
	}
	nitems = 0;
#ifdef	pdp11
	while (readdir(dirp))
		nitems++;
	rewinddir(dirp);
	dp = (struct dir *)calloc(nitems+1, sizeof (struct dir));
	nitems = 0;
#else
	dp = (struct dir *)malloc(sizeof (struct dir));
#endif
	if (dp == 0) {
		fprintf(stderr, "diff: ran out of memory\n");
		done(0);
	}
	while ((rp = readdir(dirp))) {
		ep = &dp[nitems++];
		ep->d_reclen = rp->d_reclen;
		ep->d_namlen = rp->d_namlen;
		ep->d_entry = 0;
		ep->d_flags = 0;
		if (ep->d_namlen > 0) {
			ep->d_entry = malloc(ep->d_namlen + 1);
			if (ep->d_entry == 0) {
				fprintf(stderr, "diff: out of memory\n");
				done(0);
			}
			strcpy(ep->d_entry, rp->d_name);
		}
#ifndef	pdp11
		dp = (struct dir *)realloc((char *)dp,
			(nitems + 1) * sizeof (struct dir));
		if (dp == 0) {
			fprintf(stderr, "diff: ran out of memory\n");
			done(0);
		}
#endif
	}
	dp[nitems].d_entry = 0;		/* delimiter */
	closedir(dirp);
	qsort(dp, nitems, sizeof (struct dir), entcmp);
	return (dp);
}

int
entcmp(d1, d2)
	struct dir *d1, *d2;
{
	return (strcmp(d1->d_entry, d2->d_entry));
}

void
compare(dp)
	register struct dir *dp;
{
	register int i, j;
	int f1, f2, fmt1, fmt2;
	struct stat stb1, stb2;
	int flag = 0;
	char buf1[BUFSIZ], buf2[BUFSIZ];

	strcpy(efile1, dp->d_entry);
	strcpy(efile2, dp->d_entry);
	f1 = open(file1, 0);
	if (f1 < 0) {
		perror(file1);
		return;
	}
	f2 = open(file2, 0);
	if (f2 < 0) {
		perror(file2);
		close(f1);
		return;
	}
	fstat(f1, &stb1); fstat(f2, &stb2);
	fmt1 = stb1.st_mode & S_IFMT;
	fmt2 = stb2.st_mode & S_IFMT;
	if (fmt1 != S_IFREG || fmt2 != S_IFREG) {
		if (fmt1 == fmt2) {
			if (fmt1 != S_IFDIR && stb1.st_rdev == stb2.st_rdev)
				goto same;
			if (fmt1 == S_IFDIR) {
				dp->d_flags = DIRECT;
				if (lflag || opt == D_EDIT)
					goto closem;
				printf("Common subdirectories: %s and %s\n",
				    file1, file2);
				goto closem;
			}
		}
		goto notsame;
	}
	if (stb1.st_size != stb2.st_size)
		goto notsame;
	for (;;) {
		i = read(f1, buf1, BUFSIZ);
		j = read(f2, buf2, BUFSIZ);
		if (i < 0 || j < 0 || i != j)
			goto notsame;
		if (i == 0 && j == 0)
			goto same;
		for (j = 0; j < i; j++)
			if (buf1[j] != buf2[j])
				goto notsame;
	}
same:
	if (sflag == 0)
		goto closem;
	if (lflag)
		dp->d_flags = SAME;
	else
		printf("Files %s and %s are identical\n", file1, file2);
	goto closem;
notsame:
	if (!ascii(f1) || !ascii(f2)) {
		if (lflag)
			dp->d_flags |= DIFFER;
		else if (opt == D_NORMAL || opt == D_CONTEXT)
			printf("Binary files %s and %s differ\n",
			    file1, file2);
		goto closem;
	}
	close(f1); close(f2);
	anychange = 1;
	if (lflag)
		calldiff(title);
	else {
		if (opt == D_EDIT) {
			printf("ed - %s << '-*-END-*-'\n", dp->d_entry);
			calldiff(0);
		} else {
			printf("%s%s %s\n", title, file1, file2);
			calldiff(0);
		}
		if (opt == D_EDIT)
			printf("w\nq\n-*-END-*-\n");
	}
	return;
closem:
	close(f1); close(f2);
}

char	*prargs[] = { "pr", "-h", 0, "-f", 0, 0 };

void
calldiff(wantpr)
	char *wantpr;
{
	int pid, status, status2, pv[2];

	prargs[2] = wantpr;
	fflush(stdout);
	if (wantpr) {
		sprintf(etitle, "%s %s", file1, file2);
		pipe(pv);
		pid = fork();
		if (pid == -1) {
			fprintf(stderr, "No more processes");
			done(0);
		}
		if (pid == 0) {
			close(0);
			dup(pv[0]);
			close(pv[0]);
			close(pv[1]);
			execv(pr+4, prargs);
			execv(pr, prargs);
			perror(pr);
			done(0);
		}
	}
	pid = fork();
	if (pid == -1) {
		fprintf(stderr, "diff: No more processes\n");
		done(0);
	}
	if (pid == 0) {
		if (wantpr) {
			close(1);
			dup(pv[1]);
			close(pv[0]);
			close(pv[1]);
		}
		execv(diff+4, diffargv);
		execv(diff, diffargv);
		perror(diff);
		done(0);
	}
	if (wantpr) {
		close(pv[0]);
		close(pv[1]);
	}
	while (wait(&status) != pid)
		continue;
	while (wait(&status2) != -1)
		continue;
/*
	if ((status >> 8) >= 2)
		done(0);
*/
}

#include <a.out.h>

int
ascii(f)
	int f;
{
	char buf[BUFSIZ];
	register int cnt;
	register char *cp;

	lseek(f, (long)0, 0);
	cnt = read(f, buf, BUFSIZ);
	if (cnt >= sizeof (struct exec)) {
		struct exec hdr;
		hdr = *(struct exec *)buf;
		if (!N_BADMAG(hdr))
			return (0);
	}
	cp = buf;
	while (--cnt >= 0)
		if (*cp++ & 0200)
			return (0);
	return (1);
}

/*
 * THIS IS CRUDE.
 */
int
useless(cp)
register char *cp;
{

	if (cp[0] == '.') {
		if (cp[1] == '\0')
			return (1);	/* directory "." */
		if (cp[1] == '.' && cp[2] == '\0')
			return (1);	/* directory ".." */
	}
	if (start && strcmp(start, cp) > 0)
		return (1);
	return (0);
}
