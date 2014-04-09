/*
 * Cisco router simulation platform.
 * Copyright (c) 2005,2006 Christophe Fillot (cf@utc.fr)
 */
/*
 * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
 *
 * This file is part of the virtualmips distribution.
 * See LICENSE file for terms of the license.
 *
 */

#ifndef __MIPS64_X86_TRANS_H__

#define __MIPS64_X86_TRANS_H__

#include "system.h"
#include "x86-codegen.h"

#ifdef _USE_JIT_

/* Manipulate bitmasks atomically */
static forced_inline void x86_atomic_or (m_uint32_t * v, m_uint32_t m)
{
    __asm__ __volatile__ ("lock; orl %1,%0":"=m" (*v):"ir" (m), "m" (*v));
}

static forced_inline void x86_atomic_and (m_uint32_t * v, m_uint32_t m)
{
    __asm__ __volatile__ ("lock; andl %1,%0":"=m" (*v):"ir" (m), "m" (*v));
}

/* Wrappers to x86-codegen functions */
#define mips_jit_tcb_set_patch x86_patch
#define mips_jit_tcb_set_jump  x86_jump_code

/* Push epilog for an x86 instruction block */
static forced_inline void mips_jit_tcb_push_epilog (mips_jit_tcb_t *
    block)
{
    x86_ret (block->jit_ptr);
}

/* Translated block function pointer */
typedef void (*insn_tblock_fptr) (void);
void fastcall mips_exec_single_step (cpu_mips_t * cpu,
    mips_insn_t instruction);

/* Execute JIT code */
static forced_inline
    void mips_jit_tcb_exec (cpu_mips_t * cpu, mips_jit_tcb_t * block)
{
    insn_tblock_fptr jit_code;
    m_uint32_t offset;

    offset = (cpu->pc & MIPS_MIN_PAGE_IMASK) >> 2;
    jit_code = (insn_tblock_fptr) block->jit_insn_ptr[offset];

    if (unlikely (!jit_code)) {
        mips_exec_single_step (cpu, vmtoh32 (block->mips_code[offset]));
        return;
    }

    asm volatile ("movl %0,%%edi"::"r" (cpu): "esi", "edi", "ecx", "edx");
    jit_code ();
}

void mips_set_pc (mips_jit_tcb_t * b, m_va_t new_pc);
void mips_emit_single_step (mips_jit_tcb_t * b, mips_insn_t insn);
void mips_check_cpu_pausing (mips_jit_tcb_t * b);
void mips_check_pending_irq (mips_jit_tcb_t * b);

#endif

#endif
