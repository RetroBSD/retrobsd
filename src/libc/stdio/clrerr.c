#include <stdio.h>
#undef	clearerr

void
clearerr(iop)
	register FILE *iop;
{
	iop->_flag &= ~(_IOERR|_IOEOF);
}
