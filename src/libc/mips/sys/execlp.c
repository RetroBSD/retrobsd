/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <unistd.h>
#include <stdarg.h>

int
execlp(const char *name, const char *arg, ...)
{
    va_list ap, copy;
    unsigned argc;

    // Compute number of arguments.
    va_start(ap, arg);
    va_copy(copy, ap);
    for (argc = 1; va_arg(ap, char *); ) {
        ++argc;
    }
    va_end(ap);

    // Allocate args on stack.
    const char *argv[argc + 1];
    argv[0] = arg;
    for (unsigned i = 1; i <= argc; i++) {
        argv[i] = va_arg(copy, char *);
    }
    va_end(copy);

    return execvp(name, (char *const *)argv);
}
