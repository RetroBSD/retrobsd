/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <unistd.h>
#include <stdarg.h>

int
execle (const char *name, const char *arg, ...)
{
	va_list ap;
	char **envp;

	va_start (ap, arg);
	while ((va_arg (ap, char *)) != NULL)
		continue;
	envp = va_arg (ap, char **);
	va_end (ap);

	return execve (name, (char *const*) &arg, envp);
}
