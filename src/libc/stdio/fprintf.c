/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <stdio.h>
#include <stdarg.h>
#include <alloca.h>

int
fprintf (register FILE *iop, const char *fmt, ...)
{
	va_list ap;

	va_start (ap, fmt);
	if (iop->_flag & _IONBF) {
		iop->_flag &= ~_IONBF;
		iop->_ptr = iop->_base = alloca(BUFSIZ);
		iop->_bufsiz = BUFSIZ;
		_doprnt(fmt, ap, iop);
		fflush(iop);
		iop->_flag |= _IONBF;
		iop->_base = NULL;
		iop->_bufsiz = NULL;
		iop->_cnt = 0;
	} else
		_doprnt(fmt, ap, iop);
	va_end (ap);
	return(ferror(iop)? EOF: 0);
}
