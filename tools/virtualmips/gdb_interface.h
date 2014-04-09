 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *
  * This file is part of the virtualmips distribution.
  * See LICENSE file for terms of the license.
  *
  */

#ifndef  __GDB_INTERFACE_H__
#define  __GDB_INTERFACE_H__
#include "utils.h"
#include "mips.h"

typedef enum Simdebug_result {
    SD_CONTINUE = -2,
    SD_NEXTI_ANYCPU = -1
} Simdebug_result;

typedef struct virtualBreakpoint {
    m_uint32_t addr;
    char *save;
    //int cpuno;
    int len;                    /*break point instruction len */
    struct virtualBreakpoint *next;
} virtual_breakpoint_t;

int cpu_hit_breakpoint (vm_instance_t * vm, m_uint32_t vaddr);

Simdebug_result Simdebug_run (vm_instance_t * vm, int sig);
void bad_memory_access_gdb (vm_instance_t * vm);
#endif
