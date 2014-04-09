/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <stdio.h>
#include <strings.h>

size_t
fwrite(vptr, size, count, iop)
	const void *vptr;
	size_t size, count;
	register FILE *iop;
{
	register const char *ptr = vptr;
	register unsigned s;

	s = size * count;
	if (iop->_flag & _IOLBF)
		while (s > 0) {
			if (--iop->_cnt > -iop->_bufsiz && *ptr != '\n')
				*iop->_ptr++ = *ptr++;
			else if (_flsbuf(*(unsigned char *)ptr++, iop) == EOF)
				break;
			s--;
		}
	else while (s > 0) {
		if (iop->_cnt < s) {
			if (iop->_cnt > 0) {
				bcopy(ptr, iop->_ptr, iop->_cnt);
				ptr += iop->_cnt;
				iop->_ptr += iop->_cnt;
				s -= iop->_cnt;
			}
			if (_flsbuf(*(unsigned char *)ptr++, iop) == EOF)
				break;
			s--;
		}
		if (iop->_cnt >= s) {
			bcopy(ptr, iop->_ptr, s);
			iop->_ptr += s;
			iop->_cnt -= s;
			return (count);
		}
	}
	return (size != 0 ? count - ((s + size - 1) / size) : 0);
}
