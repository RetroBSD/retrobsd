/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
#include <sys/types.h>
#include <sys/file.h>
#include <a.out.h>
#include <stdio.h>
#include <string.h>

#define	ISVALID(p)	(p->n_name && p->n_name[0])

int
nlist(name, list)
	char *name;
	struct nlist *list;
{
	register struct nlist *p;
	struct exec ebuf;
	register FILE *fsym;
	off_t symbol_offset, symbol_size;
	int entries, len, maxlen, type;
	register int c;
	register unsigned value;
	char sbuf[128];

	entries = -1;

	if (!(fsym = fopen(name, "r")))
		return(-1);
	if (fread((char *)&ebuf, 1, sizeof(ebuf), fsym) != sizeof (ebuf) ||
	    N_BADMAG(ebuf))
		goto done;

	symbol_offset = N_SYMOFF(ebuf);
	symbol_size = ebuf.a_syms;
	if (fseek(fsym, symbol_offset, L_SET))
		goto done;

	/*
	 * clean out any left-over information for all valid entries.
	 * Type and value defined to be 0 if not found; historical
	 * versions cleared other and desc as well.  Also figure out
	 * the largest string length so don't read any more of the
	 * string table than we have to.
	 */
	for (p = list, entries = maxlen = 0; ISVALID(p); ++p, ++entries) {
		p->n_type = 0;
		p->n_value = 0;
		if ((len = strlen(p->n_name)) > maxlen)
			maxlen = len;
	}
	if (++maxlen > sizeof(sbuf)) {		/* for the NULL */
		(void)fprintf(stderr, "nlist: sym 2 big\n");
		entries = -1;
		goto done;
	}

	for (; symbol_size; symbol_size -= len + 6) {
                len = getc (fsym);
                if (len <= 0)
			break;

                type = getc (fsym);
                value = getc (fsym);
                value |= getc (fsym) << 8;
                value |= getc (fsym) << 16;
                value |= getc (fsym) << 24;
                for (c=0; c<len && c<maxlen; c++)
                        sbuf [c] = getc (fsym);
                sbuf [c] = '\0';

		for (p = list; ISVALID(p); p++)
			if (strcmp(p->n_name, sbuf) == 0) {
				p->n_value = value;
				p->n_type = type;
				if (!--entries)
					goto done;
			}
	}
done:	(void)fclose(fsym);
	return(entries);
}
