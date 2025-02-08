#include <stdio.h>
#undef	clearerr

void
clearerr(FILE *iop)
{
	iop->_flag &= ~(_IOERR|_IOEOF);
}
