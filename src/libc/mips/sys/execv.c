/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <unistd.h>

extern char **environ;

int
execv (name, argv)
	const char *name;
	char *const *argv;
{
	return execve (name, argv, environ);
}
