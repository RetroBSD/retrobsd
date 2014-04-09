/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <unistd.h>

extern char **environ;

int
execl (const char *name, const char *arg, ...)
{
	return execve (name, (char *const*) &arg, environ);
}
