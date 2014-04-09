/*
 * Copyright (c) 1984 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <stdio.h>
#include <alloca.h>

int
fputs(s, iop)
	register const char *s;
	register FILE *iop;
{
	register int r = 0, c;
	int unbuffered;

	unbuffered = iop->_flag & _IONBF;
	if (unbuffered) {
		iop->_flag &= ~_IONBF;
		iop->_ptr = iop->_base = alloca(BUFSIZ);
		iop->_bufsiz = BUFSIZ;
	}

	while ((c = *s++))
		r = putc(c, iop);

	if (unbuffered) {
		fflush(iop);
		iop->_flag |= _IONBF;
		iop->_base = NULL;
		iop->_bufsiz = NULL;
		iop->_cnt = 0;
	}

	return(r);
}
