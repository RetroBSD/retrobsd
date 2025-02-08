/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Hugh Smith at The University of Guelph.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifdef CROSS
#   include <time.h>
#   include <fcntl.h>
#   include <stdint.h>
#   include <sys/file.h>
#   include <sys/stat.h>
#   include <stdio.h>
#   include <string.h>
#   include <strings.h>
#   include <stdlib.h>
#   include <unistd.h>
#   include <errno.h>
#else
#   include <sys/param.h>
#   include <sys/stat.h>
#   include <sys/dir.h>
#   include <sys/file.h>
#   include <stdio.h>
#   include <string.h>
#   include <strings.h>
#   include <stdlib.h>
#   include <unistd.h>
#   include <errno.h>
#   include <fcntl.h>
#endif
#include <ar.h>
#include "archive.h"
#include "extern.h"

extern CHDR chdr;			/* converted header */
extern char *archive;			/* archive name */
extern int errno;

typedef struct ar_hdr HDR;
static char hb[sizeof(HDR) + 1];	/* real header */

int
open_archive(int mode)
{
	int created, fd, nr;
	char buf[SARMAG];

	created = 0;
	if (mode & O_CREAT) {
		mode |= O_EXCL;
		fd = open(archive, mode, 0666);
		if (fd >= 0) {
			/* POSIX.2 puts create message on stderr. */
			if (!(options & AR_C))
				(void)fprintf(stderr,
				    "ar: creating archive %s.\n", archive);
			created = 1;
			goto opened;
		}
		if (errno != EEXIST)
			error(archive);
		mode &= ~O_EXCL;
	}

	fd = open(archive, mode, 0666);
	if (fd < 0)
		error(archive);

	/*
	 * Attempt to place a lock on the opened file - if we get an
	 * error then someone is already working on this library (or
	 * it's going across NFS).
	 */
opened: if (flock(fd, LOCK_EX|LOCK_NB) && errno != EOPNOTSUPP)
		error(archive);

	/*
	 * If not created, O_RDONLY|O_RDWR indicates that it has to be
	 * in archive format.
	 */
	if (!created &&
	    ((mode & 3) == O_RDONLY || (mode & 3) == O_RDWR)) {
		if ((nr = read(fd, buf, SARMAG) != SARMAG)) {
			if (nr >= 0)
				badfmt();
			error(archive);
		} else if (bcmp(buf, ARMAG, SARMAG))
			badfmt();
	} else if (write(fd, ARMAG, SARMAG) != SARMAG)
		error(archive);
	return(fd);
}

void
close_archive(int fd)
{
	(void)close(fd);			/* Implicit unlock. */
}

/* Convert ar header field to an integer. */
#define	AR_ATOI(from, to, len, base) { \
	bcopy(from, buf, len); \
	buf[len] = '\0'; \
	to = strtol(buf, (char **)NULL, base); \
}

/*
 * get_arobj --
 *	read the archive header for this member
 */
int
get_arobj(int fd)
{
	struct ar_hdr *hdr;
	register int len, nr;
	register char *p;
	char buf[20];

	nr = read(fd, hb, sizeof(HDR));
	if (nr != sizeof(HDR)) {
		if (!nr)
			return(0);
		if (nr < 0)
			error(archive);
		badfmt();
	}

	hdr = (struct ar_hdr *)hb;
	if (strncmp(hdr->ar_fmag, ARFMAG, sizeof(ARFMAG) - 1))
		badfmt();

	/* Convert the header into the internal format. */
#define	DECIMAL	10
#define	OCTAL	 8

	AR_ATOI(hdr->ar_date, chdr.date, sizeof(hdr->ar_date), DECIMAL);
	AR_ATOI(hdr->ar_uid, chdr.uid, sizeof(hdr->ar_uid), DECIMAL);
	AR_ATOI(hdr->ar_gid, chdr.gid, sizeof(hdr->ar_gid), DECIMAL);
	AR_ATOI(hdr->ar_mode, chdr.mode, sizeof(hdr->ar_mode), OCTAL);
	AR_ATOI(hdr->ar_size, chdr.size, sizeof(hdr->ar_size), DECIMAL);

	/* Leading spaces should never happen. */
	if (hdr->ar_name[0] == ' ')
		badfmt();

	/*
	 * Long name support.  Set the "real" size of the file, and the
	 * long name flag/size.
	 */
	if (!bcmp(hdr->ar_name, AR_EFMT1, sizeof(AR_EFMT1) - 1)) {
		chdr.lname = len = atoi(hdr->ar_name + sizeof(AR_EFMT1) - 1);
		if (len <= 0 || len > MAXNAMLEN)
			badfmt();
		nr = read(fd, chdr.name, (size_t)len);
		if (nr != len) {
			if (nr < 0)
				error(archive);
			badfmt();
		}
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

static int already_written;

/*
 * copy_ar --
 *	Copy size bytes from one file to another - taking care to handle the
 *	extra byte (for odd size files) when reading archives and writing an
 *	extra byte if necessary when adding files to archive.  The length of
 *	the object is the long name plus the object itself; the variable
 *	already_written gets set if a long name was written.
 *
 *	The padding is really unnecessary, and is almost certainly a remnant
 *	of early archive formats where the header included binary data which
 *	a PDP-11 required to start on an even byte boundary.  (Or, perhaps,
 *	because 16-bit word addressed copies were faster?)  Anyhow, it should
 *	have been ripped out long ago.
 */
void
copy_ar(CF *cfp, off_t size)
{
	static char pad = '\n';
	off_t sz;
	register int from, nr, nw, off, to;
#ifdef	pdp11
	char buf[2*1024];
#else
	char buf[8*1024];
#endif

	if (!(sz = size))
		return;

	from = cfp->rfd;
	to = cfp->wfd;
	while (sz) {
	        nr = read(from, buf, sz < sizeof(buf) ? sz : sizeof(buf));
		if (nr <= 0)
		        break;
		sz -= nr;
		for (off = 0; off < nr; nr -= off, off += nw)
			if ((nw = write(to, buf + off, (size_t)nr)) < 0)
				error(cfp->wname);
	}
	if (sz) {
		if (nr == 0)
			badfmt();
		error(cfp->rname);
	}

	if (cfp->flags & RPAD && size & 1 && (nr = read(from, buf, 1)) != 1) {
		if (nr == 0)
			badfmt();
		error(cfp->rname);
	}
	if (cfp->flags & WPAD && (size + already_written) & 1 &&
	    write(to, &pad, 1) != 1)
		error(cfp->wname);
}

/*
 * put_arobj --
 *	Write an archive member to a file.
 */
void
put_arobj(CF *cfp, struct stat *sb)
{
	register int lname;
	register char *name;
	struct ar_hdr *hdr;
	off_t size;

	/*
	 * If passed an sb structure, reading a file from disk.  Get stat(2)
	 * information, build a name and construct a header.  (Files are named
	 * by their last component in the archive.)  If not, then just write
	 * the last header read.
	 */
	if (sb) {
		name = rname(cfp->rname);
		(void)fstat(cfp->rfd, sb);

		/*
		 * If not truncating names and the name is too long or contains
		 * a space, use extended format 1.
		 */
		lname = strlen(name);
		if (options & AR_TR) {
			if (lname > OLDARMAXNAME) {
				(void)fflush(stdout);
				(void)fprintf(stderr,
				    "ar: warning: %s truncated to %.*s\n",
				    name, OLDARMAXNAME, name);
				(void)fflush(stderr);
			}
			(void)sprintf(hb, HDR3, name, sb->st_mtime, sb->st_uid,
			    sb->st_gid, sb->st_mode, (long) sb->st_size, ARFMAG);
			lname = 0;
		} else if (lname > sizeof(hdr->ar_name) || index(name, ' '))
			(void)sprintf(hb, HDR1, AR_EFMT1, lname, sb->st_mtime,
			    sb->st_uid, sb->st_gid, sb->st_mode,
			    (long) sb->st_size + lname, ARFMAG);
		else {
			lname = 0;
			(void)sprintf(hb, HDR2, name, sb->st_mtime, sb->st_uid,
			    sb->st_gid, sb->st_mode, (long) sb->st_size, ARFMAG);
		}
		size = sb->st_size;
	} else {
		lname = chdr.lname;
		name = chdr.name;
		size = chdr.size;
	}

	if (write(cfp->wfd, hb, sizeof(HDR)) != sizeof(HDR))
		error(cfp->wname);
	if (lname) {
		if (write(cfp->wfd, name, (size_t)lname) != lname)
			error(cfp->wname);
		already_written = lname;
	}
	copy_ar(cfp, size);
	already_written = 0;
}

/*
 * skip_arobj -
 *	Skip over an object -- taking care to skip the pad bytes.
 */
void
skip_arobj(int fd)
{
	off_t len;

	len = chdr.size + (chdr.lname & 1);
	if (lseek(fd, len, SEEK_CUR) == (off_t)-1)
		error(archive);
}
