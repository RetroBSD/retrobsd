/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
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
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

/*
 * This has been slightly trimmed from the 4.4BSD version for use with 2.11BSD.
 * In particular 1) the flag names were changed back to the original ones
 * since I didn't feel like porting all of 4.4's stdio package right now and
 * 2) The constant BUFSIZ is used rather than importing the "optimum buffer
 * size selection" logic from 4.4 (besides, a PDP11 can't afford more than 1kb
 * most of the time anyhow).
 *
 * Set one of the three kinds of buffering, optionally including
 * a buffer.
 */
int
setvbuf(fp, buf, mode, size)
	register FILE *fp;
	char *buf;
	register int mode;
	size_t size;
{
	int	ret;
	register int flags;

/*
 * Verify arguments. Note, buf and size are ignored when setting _IONBF.
 */
	if (mode != _IONBF)
		if ((mode != _IOFBF && mode != _IOLBF) || (int)size < 0)
			return (EOF);

	/*
	 * Write current buffer, if any.  Discard unread input, cancel
	 * line buffering, and free old buffer if malloc()ed.
	 */
	(void)fflush(fp);
	fp->_cnt = fp->_bufsiz = 0;
	flags = fp->_flag;
	if (flags & _IOMYBUF)
		free((void *)fp->_base);
	flags &= ~(_IOLBF | _IONBF | _IOMYBUF);
	ret = 0;

	/* If setting unbuffered mode, skip all the hard work. */
	if (mode == _IONBF)
		goto nbf;

	if (size == 0) {
		buf = NULL;	/* force local allocation */
		size = BUFSIZ;
	}

	/* Allocate buffer if needed. */
	if (buf == NULL) {
		if ((buf = (char *)malloc(size)) == NULL) {
			/*
			 * Unable to honor user's request.  We will return
			 * failure, but try again with file system size.
			 */
			ret = EOF;
			if (size != BUFSIZ) {
				size = BUFSIZ;
				buf = (char *)malloc(size);
			}
		}
		if (buf == NULL) {
			/* No luck; switch to unbuffered I/O. */
nbf:
			fp->_flag = flags | _IONBF;
			fp->_base = fp->_ptr = NULL;
			return (ret);
		}
		flags |= _IOMYBUF;
	}

	/*
	 * Fix up the FILE fields.  If in r/w mode, go to the unknown state
	 * so that the the first read performs its initial call to _filbuf and
	 * the first write has an empty buffer to fill.
	 */
	if (mode == _IOLBF)
		flags |= _IOLBF;
	if (flags & _IORW)
		flags &= ~(_IOREAD | _IOWRT);
	fp->_flag = flags;
	fp->_base = fp->_ptr = (char *)buf;
	fp->_bufsiz = size;
	return (ret);
}
