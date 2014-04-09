/* alloca.h - Allocate memory on stack */

#ifndef ALLOCA_H
#define ALLOCA_H

#undef alloca

#ifdef __GNUC__
#define alloca(size) __builtin_alloca(size)
#else
#include <sys/types.h>
void *alloca(size_t);
#endif

#endif
