/*
 * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
 *
 * This file is part of the virtualmips distribution.
 * See LICENSE file for terms of the license.
 */
#include "cpu.h"
#include "vm.h"
#include "mips_exec.h"
#include "mips_fdd.h"
#include "vp_timer.h"

cpu_mips_t *current_cpu;

/*select different main loop*/
void *mips_exec_run_cpu (cpu_mips_t * cpu)
{
#ifdef _USE_JIT_
    if (cpu->vm->jit_use) {
        mips_jit_init (cpu);
        return mips_jit_run_cpu (cpu);
    } else
#endif
        return mips_cpu_fdd (cpu);
}
