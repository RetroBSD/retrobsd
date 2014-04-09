 /*
  * Copyright (C) yajin 2008<yajinzhou@gmail.com >
  *     
  * This file is part of the virtualmips distribution. 
  * See LICENSE file for terms of the license. 
  *
  */

#ifndef __VP_LOCK_H__
#define __VP_LOCK_H__

/*testandset from QEMU*/

#ifdef __i386__
static inline int testandset (int *p)
{
    long int readval = 0;

    __asm__ __volatile__ ("lock; cmpxchgl %2, %0":"+m" (*p),
        "+a" (readval):"r" (1):"cc");
    return readval;
}
#endif

#ifdef __x86_64__
static inline int testandset (int *p)
{
    long int readval = 0;

    __asm__ __volatile__ ("lock; cmpxchgl %2, %0":"+m" (*p),
        "+a" (readval):"r" (1):"cc");
    return readval;
}
#endif

#ifdef __ia64
#include <ia64intrin.h>

static inline int testandset (int *p)
{
    return __sync_lock_test_and_set (p, 1);
}
#endif

#endif
