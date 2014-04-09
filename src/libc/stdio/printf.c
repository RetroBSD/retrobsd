#include <stdio.h>
#include <stdarg.h>

int
printf (const char *fmt, ...)
{
	va_list args;

	va_start (args, fmt);
	_doprnt (fmt, args, stdout);
	va_end (args);
	return ferror (stdout) ? EOF : 0;
}
