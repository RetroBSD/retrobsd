#include <stdlib.h>
#include <unistd.h>

int errno;

/*
 * This stub is linked in, when application uses no stdio calls.
 */
__attribute__((weak))
void _cleanup()
{
        /* Nothing to do. */
}

void
exit (code)
	int code;
{
	_cleanup();
	_exit (code);
}
