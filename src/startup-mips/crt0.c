/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <stdlib.h>
#include <unistd.h>

/*
 * C runtime startoff.  When an a.out is loaded by the kernel, the kernel
 * sets up the stack as follows:
 *
 *	_________________________________
 *	| (NULL)			| top of memory
 *	|-------------------------------|
 *	| 				|
 *	| environment strings		|
 *	| 				|
 *	|-------------------------------|
 *	| 				|
 *	| argument strings		|
 *	| 				|
 *	|-------------------------------|
 *	| envv[envc] (NULL)		| end of environment vector tag, a 0
 *	|-------------------------------|
 *	| envv[envc-1]			| pointer to last environment string
 *	|-------------------------------|
 *	| ...				|
 *	|-------------------------------|
 *	| envv[0]			| pointer to first environment string
 *	|-------------------------------|
 *	| argv[argc] (NULL)		| end of argument vector tag, a 0
 *	|-------------------------------|
 *	| argv[argc-1]			| pointer to last argument string
 *	|-------------------------------|
 *	| ...				|
 *	|-------------------------------|
 *	| argv[0]			| pointer to first argument string
 *	|-------------------------------|
 *	|				| space for fourth argument
 *	|-------------------------------|
 *	|				| space for third argument
 *	|-------------------------------|
 *	|				| space for second argument
 *	|-------------------------------|
 * sp->	| 				| space for first
 *	---------------------------------
 *
 * Arguments are passed in registers $a0, $a1 and $a2.
 * Register $gp is set to the start of data section.
 *
 * Crt0 simply moves the env to environ variable, calculates
 * the __progname and then calls main.
 */
extern int main (int, char **, char **);

char **environ;
const char *__progname = "";

void _start (int, char **, char **);

/* The entry function. */
void
_start (argc, argv, env)
	int argc;
	char **argv;
	char **env;
{
        asm volatile ("la $gp, _gp");

	environ = env;
	if (argc > 0 && argv[0] != 0) {
		const char *s;

		__progname = argv[0];
		for (s = __progname; *s != '\0'; s++)
			if (*s == '/')
				__progname = s + 1;
	}
	exit (main (argc, argv, env));
}
