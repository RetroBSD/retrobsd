/*
 * Returns 1 iff file is a tty
 */
#include <sgtty.h>

int
isatty(f)
        int f;
{
	struct sgttyb ttyb;

	if (ioctl(f, TIOCGETP, &ttyb) < 0)
		return(0);
	return(1);
}
