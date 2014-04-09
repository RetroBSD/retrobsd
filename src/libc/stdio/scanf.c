#include <stdio.h>
#include <stdarg.h>

int
scanf (const char *fmt, ...)
{
	va_list args;
	int n;

	va_start (args, fmt);
	n = _doscan (stdin, fmt, args);
	va_end (args);
	return n;
}

int
fscanf (FILE *iop, const char *fmt, ...)
{
	va_list args;
	int n;

	va_start (args, fmt);
	n = _doscan(iop, fmt, args);
	va_end (args);
	return n;
}

int
sscanf (const char *str, const char *fmt, ...)
{
	FILE _strbuf;
	va_list args;
	int n;

	_strbuf._flag = _IOREAD|_IOSTRG;
	_strbuf._ptr = _strbuf._base = (void*) str;
	_strbuf._cnt = 0;
	while (*str++)
		_strbuf._cnt++;
	_strbuf._bufsiz = _strbuf._cnt;
	va_start (args, fmt);
	n = _doscan(&_strbuf, fmt, args);
	va_end (args);
	return n;
}
