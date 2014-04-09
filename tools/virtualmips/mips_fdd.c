/*
 * This is the basic fetch-decode-dispatch(fdd) routine.
 * Emulation speed is slow but easy to debug.
 *
 * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
 *
 * This file is part of the virtualmips distribution.
 * See LICENSE file for terms of the license.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <time.h>

#include "cpu.h"
#include "vm.h"
#include "mips_exec.h"
#include "mips_memory.h"
#include "mips.h"
#include "mips_cp0.h"
#include "debug.h"
#include "vp_timer.h"
#include "mips_hostalarm.h"

//#ifdef  _USE_FDD_

static const struct mips_op_desc mips_opcodes[];
static const struct mips_op_desc mips_spec_opcodes[];
static const struct mips_op_desc mips_bcond_opcodes[];
static const struct mips_op_desc mips_cop0_opcodes[];
static const struct mips_op_desc mips_spec2_opcodes[];
static const struct mips_op_desc mips_spec3_opcodes[];
static const struct mips_op_desc mips_tlb_opcodes[];

extern cpu_mips_t *current_cpu;

/*for emulation performance check*/
#ifdef DEBUG_MHZ
#define C_1000MHZ 1000000000
struct timeval pstart, pend;
float timeuse, performance;
m_uint64_t instructions_executed = 0;
#endif

static void forced_inline mips_main_loop_wait (cpu_mips_t * cpu,
    int timeout)
{
    vp_run_timers (&active_timers[VP_TIMER_REALTIME],
        vp_get_clock (rt_clock));
}

/* Execute a memory operation (2) */
static int forced_inline mips_exec_memop2 (cpu_mips_t * cpu, int memop,
    m_va_t base, int offset, u_int dst_reg, int keep_ll_bit)
{
    m_va_t vaddr = cpu->gpr[base] + sign_extend (offset, 16);
    mips_memop_fn fn;

    if (!keep_ll_bit)
        cpu->ll_bit = 0;
    fn = cpu->mem_op_fn[memop];
    return (fn (cpu, vaddr, dst_reg));
}

/* Fetch an instruction */
int mips_fetch_instruction (cpu_mips_t * cpu,
    m_va_t pc, mips_insn_t * insn)
{
    m_va_t exec_page;
    m_uint32_t offset;

    exec_page = pc & ~(m_va_t) MIPS_MIN_PAGE_IMASK;
    if (unlikely (exec_page != cpu->njm_exec_page)) {
        cpu->njm_exec_ptr = cpu->mem_op_lookup (cpu, exec_page);
    }
    if (cpu->njm_exec_ptr == NULL) {
        //exception when fetching instruction
        return (1);
    }
    cpu->njm_exec_page = exec_page;
    offset = (pc & MIPS_MIN_PAGE_IMASK) >> 2;
    *insn = vmtoh32 (cpu->njm_exec_ptr[offset]);
//  printf ("(%08x) %08x\n", pc, *insn);
    return (0);
}

/* Execute a single instruction */
static forced_inline int mips_exec_single_instruction (cpu_mips_t * cpu,
    mips_insn_t instruction)
{
#ifdef DEBUG_MHZ
    if (unlikely (instructions_executed == 0)) {
        gettimeofday (&pstart, NULL);
    }
    instructions_executed++;
    if (unlikely (instructions_executed == C_1000MHZ)) {
        gettimeofday (&pend, NULL);
        timeuse =
            1000000 * (pend.tv_sec - pstart.tv_sec) + pend.tv_usec -
            pstart.tv_usec;
        timeuse /= 1000000;
        performance = 1000 / timeuse;
        printf ("Used Time:%f seconds.  %f MHZ\n", timeuse, performance);
        exit (1);
    }
#endif
    register uint op;
    op = MAJOR_OP (instruction);
    return mips_opcodes[op].func (cpu, instruction);
}

/* Single-step execution */
void fastcall mips_exec_single_step (cpu_mips_t * cpu,
    mips_insn_t instruction)
{
    int res;

    res = mips_exec_single_instruction (cpu, instruction);
    /* Normal flow ? */
    if (likely (!res))
        cpu->pc += 4;
}

void dumpregs (cpu_mips_t *cpu)
{
	printf ("                t0 = %8x   s0 = %8x   t8 = %8x   lo = %8x\n",
		cpu->gpr[8], cpu->gpr[16], cpu->gpr[24], cpu->lo);
	printf ("at = %8x   t1 = %8x   s1 = %8x   t9 = %8x   hi = %8x\n",
		cpu->gpr[1], cpu->gpr[9], cpu->gpr[17], cpu->gpr[25], cpu->hi);
	printf ("v0 = %8x   t2 = %8x   s2 = %8x               status = %8x\n",
		cpu->gpr[2], cpu->gpr[10], cpu->gpr[18], cpu->cp0.reg[MIPS_CP0_STATUS]);
	printf ("v1 = %8x   t3 = %8x   s3 = %8x\n",
		cpu->gpr[3], cpu->gpr[11], cpu->gpr[19]);
	printf ("a0 = %8x   t4 = %8x   s4 = %8x   gp = %8x   pc = %8x\n",
		cpu->gpr[4], cpu->gpr[12], cpu->gpr[20], cpu->gpr[28], cpu->pc);
	printf ("a1 = %8x   t5 = %8x   s5 = %8x   sp = %8x\n",
		cpu->gpr[5], cpu->gpr[13], cpu->gpr[21], cpu->gpr[29]);
	printf ("a2 = %8x   t6 = %8x   s6 = %8x   fp = %8x\n",
		cpu->gpr[6], cpu->gpr[14], cpu->gpr[22], cpu->gpr[30]);
	printf ("a3 = %8x   t7 = %8x   s7 = %8x   ra = %8x\n",
		cpu->gpr[7], cpu->gpr[15], cpu->gpr[23], cpu->gpr[31]);
}


/*
 * MIPS64 fetch->decode->dispatch main loop
 */
void *mips_cpu_fdd (cpu_mips_t * cpu)
{
    mips_insn_t insn = 0;
    int res;

    cpu->cpu_thread_running = TRUE;
    current_cpu = cpu;

    mips_init_host_alarm ();

start_cpu:
    for (;;) {
        if (unlikely (cpu->state != CPU_STATE_RUNNING))
            break;

        if (unlikely ((cpu->pause_request) & CPU_INTERRUPT_EXIT)) {
            cpu->state = CPU_STATE_PAUSING;
            break;
        }

        /* Reset "zero register" (for safety) */
        cpu->gpr[0] = 0;

        /* Check IRQ */
        if (unlikely (cpu->irq_pending)) {
            mips_trigger_irq (cpu);
            continue;
        }
        /* Fetch  the instruction */
        res = mips_fetch_instruction (cpu, cpu->pc, &insn);

        if (cpu->vm->trace_address == cpu->pc) {
            /* Trace address. */
            printf ("*** %08x: %08x ", cpu->pc, insn);
            print_insn_mips (cpu->pc, insn, stdout);
            printf ("\n");
            dumpregs (cpu);
        }

        if (unlikely (res == 1)) {
            /*exception when fetching instruction */
            printf ("%08x: exception when fetching instruction\n", cpu->pc);
            if (cpu->pc == 0)
                exit(-1);
            continue;
        }
        if (unlikely ((cpu->vm->mipsy_debug_mode)
                && ((cpu_hit_breakpoint (cpu->vm, cpu->pc) == SUCCESS)
                    || (cpu->vm->gdb_interact_sock == -1)
                    || (cpu->vm->mipsy_break_nexti == MIPS_BREAKANYCPU)))) {
            if (mips_debug (cpu->vm, 1)) {
                continue;
            }
        }
        if (cpu->vm->debug_level > 2 || (cpu->vm->debug_level > 1 &&
            (cpu->cp0.reg[MIPS_CP0_STATUS] & MIPS_CP0_STATUS_UM) &&
            ! (cpu->cp0.reg[MIPS_CP0_STATUS] & MIPS_CP0_STATUS_EXL)))
        {
            /* Print instructions in user mode. */
            printf ("%08x:       %08x        ", cpu->pc, insn);
            print_insn_mips (cpu->pc, insn, stdout);
            printf ("\n");
            fflush (stdout);
#if 0
            m_uint32_t dummy;
            unsigned char *p = physmem_get_hptr (cpu->vm, 0x00010000, 0, MTS_READ, &dummy);
            if (p) {
                unsigned nbytes;
                for (nbytes=0x40; nbytes>0; p+=16, nbytes-=16) {
                    printf ("%08x: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                        (unsigned) p, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
                        p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
                }
            }
#endif
        }
        res = mips_exec_single_instruction (cpu, insn);

        /* Normal flow ? */
        if (likely (!res))
            cpu->pc += sizeof (mips_insn_t);
    }

    while (cpu->cpu_thread_running) {
        switch (cpu->state) {
        case CPU_STATE_RUNNING:
            cpu->state = CPU_STATE_RUNNING;
            goto start_cpu;

        case CPU_STATE_HALTED:
            cpu->cpu_thread_running = FALSE;
            break;
        case CPU_STATE_RESTARTING:
            cpu->state = CPU_STATE_RESTARTING;
            /*Just waiting for cpu restart. */
            break;
        case CPU_STATE_PAUSING:
            /*main loop must wait for me. heihei :) */
            mips_main_loop_wait (cpu, 0);
            cpu->state = CPU_STATE_RUNNING;
            cpu->pause_request &= ~CPU_INTERRUPT_EXIT;
            /*start cpu again */
            goto start_cpu;

        }
    }
    return NULL;
}

/* Execute the instruction in delay slot */
static forced_inline int mips_exec_bdslot (cpu_mips_t * cpu)
{
    mips_insn_t insn;
    int res = 0;
    cpu->is_in_bdslot = 1;

    /* Fetch the instruction in delay slot */
    res = mips_fetch_instruction (cpu, cpu->pc + 4, &insn);
    if (res == 1) {
        /*exception when fetching instruction */
        cpu->is_in_bdslot = 0;
        return (1);
    }
    cpu->is_in_bdslot = 1;

    if (cpu->vm->debug_level > 2 || (cpu->vm->debug_level > 1 &&
        (cpu->cp0.reg[MIPS_CP0_STATUS] & MIPS_CP0_STATUS_UM) &&
        ! (cpu->cp0.reg[MIPS_CP0_STATUS] & MIPS_CP0_STATUS_EXL)))
    {
        /* Print instructions in user mode. */
        printf ("%08x:       %08x        ", cpu->pc + 4, insn);
        print_insn_mips (cpu->pc, insn, stdout);
        printf ("\n");
        fflush (stdout);
    }

    /* Execute the instruction */
    res = mips_exec_single_instruction (cpu, insn);
    cpu->is_in_bdslot = 0;
    return res;
}

#include "mips_codetable.c"

//#endif
