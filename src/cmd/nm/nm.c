/*
 * nm - print name list. string table version
 */
#ifdef CROSS
#   include <stdint.h>
#   include <sys/types.h>
#   include <stdio.h>
#   include <string.h>
#   include <stdlib.h>
#   include <errno.h>
#   include <ctype.h>
#else
#   include <sys/types.h>
#   include <sys/dir.h>
#   include <sys/file.h>
#   include <stdio.h>
#   include <stdlib.h>
#   include <string.h>
#   include <strings.h>
#   include <errno.h>
#   include <ctype.h>
#endif
#include <ar.h>
#include <a.out.h>

#include "archive.h"

CHDR	chdr;

char	gflg, nflg, oflg, pflg, uflg, rflg = 1, archive;
char	**xargv;

union {
        char	mag_armag[SARMAG+1];
        struct	exec mag_exp;
} mag_un;

int	narg, errs;

void error(n, s)
        char *s;
{
	fprintf(stderr, "nm: %s:", *xargv);
	if (archive) {
		fprintf(stderr, "(%s)", chdr.name);
		fprintf(stderr, ": ");
	} else
		fprintf(stderr, " ");
	fprintf(stderr, "%s\n", s);
	if (n)
		exit(2);
	errs = 1;
}

/*
 * "borrowed" from 'ar' because we didn't want to drag in everything else
 * from 'ar'.  The error checking was also ripped out, basically if any
 * of the criteria for being an archive are not met then a -1 is returned
 * and the rest of 'ld' figures out what to do.
 */

/*
 * read the archive header for this member.  Use a file pointer
 * rather than a file descriptor.
 */
int get_arobj(fp)
	FILE *fp;
{
	struct ar_hdr *hdr;
	register int len, nr;
	register char *p;
	char buf[20];
        static char hb[sizeof(struct ar_hdr) + 1];	/* real header */

	nr = fread(hb, 1, sizeof(struct ar_hdr), fp);
	if (nr != sizeof(struct ar_hdr))
		return(-1);

	hdr = (struct ar_hdr *)hb;
	if (strncmp(hdr->ar_fmag, ARFMAG, sizeof(ARFMAG) - 1))
		return(-1);

/* Convert ar header field to an integer. */
#define	AR_ATOI(from, to, len, base) { \
	bcopy(from, buf, len); \
	buf[len] = '\0'; \
	to = strtoul(buf, (char **)NULL, base); }

	/* Convert the header into the internal format. */
	AR_ATOI(hdr->ar_date, chdr.date, sizeof(hdr->ar_date), 10);
	AR_ATOI(hdr->ar_uid, chdr.uid, sizeof(hdr->ar_uid), 10);
	AR_ATOI(hdr->ar_gid, chdr.gid, sizeof(hdr->ar_gid), 10);
	AR_ATOI(hdr->ar_mode, chdr.mode, sizeof(hdr->ar_mode), 8);
	AR_ATOI(hdr->ar_size, chdr.size, sizeof(hdr->ar_size), 10);

	/* Leading spaces should never happen. */
	if (hdr->ar_name[0] == ' ')
		return(-1);

	/*
	 * Long name support.  Set the "real" size of the file, and the
	 * long name flag/size.
	 */
	if (!bcmp(hdr->ar_name, AR_EFMT1, sizeof(AR_EFMT1) - 1)) {
		chdr.lname = len = atoi(hdr->ar_name + sizeof(AR_EFMT1) - 1);
		if (len <= 0 || len > MAXNAMLEN)
			return(-1);
		nr = fread(chdr.name, 1, (size_t)len, fp);
		if (nr != len)
			return(-1);
		chdr.name[len] = 0;
		chdr.size -= len;
	} else {
		chdr.lname = 0;
		bcopy(hdr->ar_name, chdr.name, sizeof(hdr->ar_name));

		/* Strip trailing spaces, null terminate. */
		for (p = chdr.name + sizeof(hdr->ar_name) - 1; *p == ' '; --p);
		*++p = '\0';
	}
	return(1);
}

off_t nextel(af, off)
        FILE *af;
        off_t off;
{
	fseek(af, off, SEEK_SET);
	if (get_arobj(af) < 0)
		return 0;
	off += sizeof (struct ar_hdr) + chdr.size + (chdr.lname & 1);
	return off;
}

unsigned int fgetword (f)
    register FILE *f;
{
        register unsigned int h;

        h = getc (f);
        h |= getc (f) << 8;
        h |= getc (f) << 16;
        h |= getc (f) << 24;
        return h;
}

/*
 * Read a symbol table entry.
 * Return a number of bytes read, or -1 on EOF.
 * Format of symbol record:
 *  1 byte: length of name in bytes
 *  1 byte: type of symbol (N_UNDF, N_ABS, N_TEXT, etc)
 *  4 bytes: value
 *  N bytes: name
 */
int fgetsym (fi, name, value, type)
        register FILE *fi;
        register char *name;
        unsigned *value;
        unsigned short *type;
{
        register int len;
        unsigned nbytes;

        len = getc (fi);
        if (len <= 0)
                return -1;
        *type = getc (fi);
        *value = fgetword (fi);
        nbytes = len + 6;
        if (name) {
                while (len-- > 0)
                        *name++ = getc (fi);
                *name = '\0';
        } else
                fseek (fi, len, SEEK_CUR);
        return nbytes;
}

int compare(p1, p2)
        register struct nlist *p1, *p2;
{

	if (nflg) {
		if (p1->n_value > p2->n_value)
			return(rflg);
		if (p1->n_value < p2->n_value)
			return(-rflg);
	}
	return (rflg * strcmp(p1->n_name, p2->n_name));
}

void psyms(symp, nsyms)
	register struct nlist *symp;
	int nsyms;
{
	register int n, c;

	for (n=0; n<nsyms; n++) {
		c = symp[n].n_type;
		if (c == N_FN)
			c = 'f';
		else switch (c&N_TYPE) {

		case N_UNDF:
			c = 'u';
			if (symp[n].n_value)
				c = 'c';
			break;
		case N_ABS:
			c = 'a';
			break;
		case N_TEXT:
			c = 't';
			break;
		case N_DATA:
			c = 'd';
			break;
		case N_BSS:
			c = 'b';
			break;
		case N_STRNG:
			c = 's';
			break;
		case N_COMM:
			c = 'c';
			break;
		case N_FN:
			c = 'f';
			break;
		default:
			c = '?';
			break;
		}
		if (uflg && c!='u')
			continue;
		if (oflg) {
			if (archive)
				printf("%s:", *xargv);
			printf("%s:", archive ? chdr.name : *xargv);
		}
		if (symp[n].n_type & N_WEAK)
			c = 'w';
		if (symp[n].n_type & N_EXT)
			c = toupper(c);
		if (! uflg) {
			if (c=='u' || c=='U')
				printf("        ");
			else
				printf("%08x", symp[n].n_value);
			printf(" %c ", c);
		}
		printf("%s\n", symp[n].n_name);
	}
}

void namelist()
{
	off_t   off;
	char	ibuf[BUFSIZ];
	register FILE	*fi;

	archive = 0;
	fi = fopen(*xargv, "r");
	if (fi == NULL) {
		error(0, "cannot open");
		return;
	}
	setbuf(fi, ibuf);

	off = 0;
	if (fread((char *)&mag_un, 1, sizeof(mag_un), fi) != sizeof(mag_un)) {
		error(0, "read error");
		goto out;
	}

	if (strncmp(mag_un.mag_armag, ARMAG, SARMAG)==0) {
		archive++;
		off = SARMAG;
	}
	else if (N_BADMAG(mag_un.mag_exp)) {
		error(0, "bad format");
		goto out;
	}
	rewind(fi);

	if (archive) {
		off = nextel(fi, off);
		if (narg > 1)
			printf("\n%s:\n", *xargv);
	}

	do {
		off_t	o, curpos;
		register int i, n;
		struct nlist *symp = NULL;

		curpos = ftell(fi);
		if (fread((char *)&mag_un.mag_exp, 1,
                    sizeof(struct exec), fi) != sizeof(struct exec))
			continue;
		if (N_BADMAG(mag_un.mag_exp))
			continue;

		o = N_SYMOFF(mag_un.mag_exp);
		fseek(fi, curpos + o, SEEK_SET);
		n = mag_un.mag_exp.a_syms;
		if (n == 0) {
			error(0, "no name list");
			continue;
		}

		i = 0;
		while (n > 0) {
			unsigned value;
                        unsigned short type;

                        int c = fgetsym (fi, 0, &value, &type);
                        if (c <= 0)
                                break;
                        n -= c;
			if (gflg && (type & N_EXT) == 0)
				continue;
			if (uflg && (type & N_TYPE) == N_UNDF && value != 0)
				continue;
			i++;
		}

		fseek(fi, curpos + o, SEEK_SET);
		symp = (struct nlist *)malloc((i+1) * sizeof (struct nlist));
		if (symp == 0)
			error(1, "out of memory");
		i = 0;
		n = mag_un.mag_exp.a_syms;
		while (n > 0) {
		        char name [256];

                        int c = fgetsym(fi, name, &symp[i].n_value, &symp[i].n_type);
                        if (c <= 0)
                                break;
                        n -= c;
			if (gflg && (symp[i].n_type & N_EXT) == 0)
				continue;
			if (uflg && (symp[i].n_type & N_TYPE) == N_UNDF &&
                            symp[i].n_value != 0)
				continue;

                        symp[i].n_name = malloc(c - 5);
                        if (! symp[i].n_name)
                                error(1, "out of memory");
                        strcpy (symp[i].n_name, name);
			i++;
		}

		if (pflg==0)
			qsort(symp, i, sizeof(struct nlist), compare);
		if ((archive || narg>1) && oflg==0)
			printf("\n%s:\n", archive ? chdr.name : *xargv);

		psyms(symp, i);
		if (symp) {
		        for (n=0; n>i; n++)
                                free (symp[i].n_name);
			free((char *)symp);
                        symp = NULL;
                }
	} while(archive && (off = nextel(fi, off)) != 0);
out:
	fclose(fi);
}

int main(argc, argv)
	int	argc;
	char	**argv;
{
	if (--argc>0 && argv[1][0]=='-' && argv[1][1]!=0) {
		argv++;
		while (*++*argv) switch (**argv) {

		case 'n':
			nflg++;
			continue;
		case 'g':
			gflg++;
			continue;
		case 'u':
			uflg++;
			continue;
		case 'r':
			rflg = -1;
			continue;
		case 'p':
			pflg++;
			continue;
		case 'o':
			oflg++;
			continue;
		case 'h':
usage:                  fprintf (stderr, "Usage:\n");
                        fprintf (stderr, "  nm [-gunrpo] file...\n");
                        fprintf (stderr, "Options:\n");
                        fprintf (stderr, "  -g      Display only external symbols\n");
                        fprintf (stderr, "  -u      Display only undefined symbols\n");
                        fprintf (stderr, "  -n      Sort symbols numerically by address\n");
                        fprintf (stderr, "  -r      Reverse the order of the sort\n");
                        fprintf (stderr, "  -p      Do not sort the symbols\n");
                        fprintf (stderr, "  -o      Precede each symbol by the file name\n");
                        return(1);
		default:
			fprintf(stderr, "nm: invalid argument -%c\n",
			    *argv[0]);
                        goto usage;
		}
		argc--;
	}
	if (argc == 0) {
		argc = 1;
		argv[1] = "a.out";
	}
	narg = argc;
	xargv = argv;
	while (argc--) {
		++xargv;
		namelist();
	}
	return(errs);
}
