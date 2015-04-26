#include <stdlib.h>
#include <unistd.h>

int errno;

extern void _cleanup();

void
exit (code)
	int code;
{
	_cleanup();
	_exit (code);
}
