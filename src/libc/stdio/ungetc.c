#include <stdio.h>

int
ungetc(c, iop)
	register FILE *iop;
{
	if (c == EOF || (iop->_flag & (_IOREAD|_IORW)) == 0 ||
	    iop->_ptr == NULL || iop->_base == NULL)
		return (EOF);

	if (iop->_ptr == iop->_base) {
		if (iop->_cnt == 0)
			iop->_ptr++;
		else
			return (EOF);
        }

	iop->_cnt++;
	*--iop->_ptr = c;

	return (c);
}
