/*
 * Copyright (c) 1980, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
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
#include <errno.h>
#include <fstab.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <paths.h>
#include <sys/uio.h>

static FILE *_fs_fp;
static struct fstab _fs_fstab;

static void
error(int err)
{
	register int saverrno;

	saverrno = errno;
	errno = err;
	warn("%s", _PATH_FSTAB);
	errno = saverrno;
}

static int
fstabscan()
{
	char *cp;
	register char *bp;
#define	MAXLINELENGTH	256
	static char line[MAXLINELENGTH];
	char subline[MAXLINELENGTH], *colon = ":";
	int typexx;

	for (;;) {
		if (!fgets(line, sizeof(line), _fs_fp))
			return(0);
		bp = index(line, '\n');
		if (!bp)
			return(0);
		*bp = '\0';
		cp = line;
/* OLD_STYLE_FSTAB */
		if (!strpbrk(cp, " \t")) {
			_fs_fstab.fs_spec = strsep(&cp, colon);
			_fs_fstab.fs_file = strsep(&cp, colon);
			_fs_fstab.fs_type = strsep(&cp, colon);
			if (_fs_fstab.fs_type) {
				if (!strcmp(_fs_fstab.fs_type, FSTAB_XX))
					continue;
				_fs_fstab.fs_mntops = _fs_fstab.fs_type;
				_fs_fstab.fs_vfstype =
				    strcmp(_fs_fstab.fs_type, FSTAB_SW) ?
				    "ufs" : "swap";
                                bp = strsep(&cp, colon);
				if (bp) {
					_fs_fstab.fs_freq = atoi(bp);
					bp = strsep(&cp, colon);
					if (bp) {
						_fs_fstab.fs_passno = atoi(bp);
						return(1);
					}
				}
			}
			goto bad;
		}
/* OLD_STYLE_FSTAB */
		_fs_fstab.fs_spec = strtok(cp, " \t");
		if (!_fs_fstab.fs_spec || *_fs_fstab.fs_spec == '#')
			continue;
		_fs_fstab.fs_file = strtok((char *)NULL, " \t");
		_fs_fstab.fs_vfstype = strtok((char *)NULL, " \t");
		_fs_fstab.fs_mntops = strtok((char *)NULL, " \t");
		if (_fs_fstab.fs_mntops == NULL)
			goto bad;
		_fs_fstab.fs_freq = 0;
		_fs_fstab.fs_passno = 0;
		if ((cp = strtok((char *)NULL, " \t")) != NULL) {
			_fs_fstab.fs_freq = atoi(cp);
			if ((cp = strtok((char *)NULL, " \t")) != NULL)
				_fs_fstab.fs_passno = atoi(cp);
		}
		strcpy(subline, _fs_fstab.fs_mntops);
		for (typexx = 0, cp = strtok(subline, ","); cp;
		     cp = strtok((char *)NULL, ",")) {
			if (strlen(cp) != 2)
				continue;
			if (!strcmp(cp, FSTAB_RW)) {
				_fs_fstab.fs_type = FSTAB_RW;
				break;
			}
			if (!strcmp(cp, FSTAB_RQ)) {
				_fs_fstab.fs_type = FSTAB_RQ;
				break;
			}
			if (!strcmp(cp, FSTAB_RO)) {
				_fs_fstab.fs_type = FSTAB_RO;
				break;
			}
			if (!strcmp(cp, FSTAB_SW)) {
				_fs_fstab.fs_type = FSTAB_SW;
				break;
			}
			if (!strcmp(cp, FSTAB_XX)) {
				_fs_fstab.fs_type = FSTAB_XX;
				typexx++;
				break;
			}
		}
		if (typexx)
			continue;
		if (cp != NULL)
			return(1);

bad:		/* no way to distinguish between EOF and syntax error */
		error(EFTYPE);
	}
	/* NOTREACHED */
}

struct fstab *
getfsent()
{
	if (! _fs_fp && ! setfsent())
		return 0;
	if (! fstabscan())
		return 0;
	return &_fs_fstab;
}

struct fstab *
getfsspec(const char *name)
{
	if (setfsent())
		while (fstabscan())
			if (!strcmp(_fs_fstab.fs_spec, name))
				return(&_fs_fstab);
	return((struct fstab *)NULL);
}

struct fstab *
getfsfile(const char *name)
{
	if (setfsent())
		while (fstabscan())
			if (!strcmp(_fs_fstab.fs_file, name))
				return(&_fs_fstab);
	return((struct fstab *)NULL);
}

int
setfsent()
{
	if (_fs_fp) {
		rewind(_fs_fp);
		return(1);
	}
	_fs_fp = fopen(_PATH_FSTAB, "r");
	if (_fs_fp)
		return(1);
	error(errno);
	return(0);
}

void
endfsent()
{
	if (_fs_fp) {
		(void)fclose(_fs_fp);
		_fs_fp = NULL;
	}
}
