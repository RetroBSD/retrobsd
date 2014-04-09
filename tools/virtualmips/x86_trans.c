/*
 * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
 *
 * This file is part of the virtualmips distribution.
 * See LICENSE file for terms of the license.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <signal.h>
#include <fcntl.h>
#include <assert.h>

#include "cpu.h"
#include "vm.h"
#include "mips_exec.h"
#include "mips_memory.h"
#include "mips.h"
#include "mips_cp0.h"
#include "debug.h"
#include "vp_timer.h"
#include "x86_trans.h"

#ifdef _USE_JIT_
struct mips_jit_desc mips_jit[];
static struct mips_jit_desc mips_spec_jit[];
static struct mips_jit_desc mips_bcond_jit[];
static struct mips_jit_desc mips_cop0_jit[];
static struct mips_jit_desc mips_mad_jit[];
static struct mips_jit_desc mips_tlb_jit[];
void fastcall mips_exec_single_step (cpu_mips_t * cpu,
    mips_insn_t instruction);

#define REG_OFFSET(reg)       (OFFSET(cpu_mips_t,gpr[(reg)]))
#define CP0_REG_OFFSET(c0reg) (OFFSET(cpu_mips_t,cp0.reg[(c0reg)]))
#define MEMOP_OFFSET(op)      (OFFSET(cpu_mips_t,mem_op_fn[(op)]))

/* Set the Pointer Counter (PC) register */
void mips_set_pc (mips_jit_tcb_t * b, m_va_t new_pc)
{
    x86_mov_membase_imm (b->jit_ptr, X86_EDI, OFFSET (cpu_mips_t, pc),
        new_pc & 0xFFFFFFFF, 4);
}

/* Set the Return Address (RA) register */
void mips_set_ra (mips_jit_tcb_t * b, m_va_t ret_pc)
{

    x86_mov_membase_imm (b->jit_ptr, X86_EDI, REG_OFFSET (MIPS_GPR_RA),
        ret_pc & 0xFFFFFFFF, 4);

}

/*
 * Try to branch directly to the specified JIT block without returning to
 * main loop.
 */
static void mips_try_direct_far_jump (cpu_mips_t * cpu,
    mips_jit_tcb_t * b, m_va_t new_pc)
{
    m_va_t new_page;
    m_uint32_t pc_hash, pc_offset;
    u_char *test1, *test2, *test3, *test4, *test5, *test6;

    new_page = new_pc & MIPS_MIN_PAGE_MASK;
    pc_offset = (new_pc & MIPS_MIN_PAGE_IMASK) >> 2;
    pc_hash = mips_jit_get_pc_hash (cpu, new_pc);

    /* Get JIT block info in %edx */
    x86_mov_reg_membase (b->jit_ptr, X86_EBX,
        X86_EDI, OFFSET (cpu_mips_t, exec_blk_map), 4);
    x86_mov_reg_membase (b->jit_ptr, X86_EDX, X86_EBX,
        pc_hash * sizeof (void *), 4);

    /* no JIT block found ? */
    x86_test_reg_reg (b->jit_ptr, X86_EDX, X86_EDX);
    test1 = b->jit_ptr;
    x86_branch8 (b->jit_ptr, X86_CC_Z, 0, 1);

    /* Check block PC */
    x86_mov_reg_imm (b->jit_ptr, X86_EAX, (m_uint32_t) new_page);
    x86_alu_reg_membase (b->jit_ptr, X86_CMP, X86_EAX, X86_EDX,
        OFFSET (mips_jit_tcb_t, start_pc));
    test2 = b->jit_ptr;
    x86_branch8 (b->jit_ptr, X86_CC_NE, 0, 1);

    /*whether newpc is mapped */
    x86_shift_reg_imm (b->jit_ptr, X86_SHR, X86_EAX, 29);
    x86_alu_reg_imm (b->jit_ptr, X86_AND, X86_EAX, 0x7);

    x86_alu_reg_imm (b->jit_ptr, X86_CMP, X86_EAX, 0x4);
    test4 = b->jit_ptr;
    x86_branch8 (b->jit_ptr, X86_CC_E, 0, 1);

    x86_alu_reg_imm (b->jit_ptr, X86_CMP, X86_EAX, 0x5);
    test5 = b->jit_ptr;
    x86_branch8 (b->jit_ptr, X86_CC_E, 0, 1);

    /*check ASID */
    x86_lea_membase (b->jit_ptr, X86_ESI, X86_EDI, OFFSET (cpu_mips_t, cp0));
    //x86_mov_reg_membase(b->jit_ptr,X86_ESI,
    //                   X86_EDI,OFFSET(cpu_mips_t,cp0),4);
    x86_mov_reg_membase (b->jit_ptr, X86_EBX,
        X86_ESI, OFFSET (mips_cp0_t, reg[MIPS_CP0_TLB_HI]), 4);
    x86_alu_reg_imm (b->jit_ptr, X86_AND, X86_EBX, MIPS_TLB_ASID_MASK);
    x86_alu_reg_membase (b->jit_ptr, X86_CMP, X86_EBX, X86_EDX,
        OFFSET (mips_jit_tcb_t, asid));
    test6 = b->jit_ptr;
    x86_branch8 (b->jit_ptr, X86_CC_NE, 0, 1);

    x86_patch (test4, b->jit_ptr);
    x86_patch (test5, b->jit_ptr);
    /* Jump to the code */
    x86_mov_reg_membase (b->jit_ptr, X86_ESI,
        X86_EDX, OFFSET (mips_jit_tcb_t, jit_insn_ptr), 4);
    x86_mov_reg_membase (b->jit_ptr, X86_EBX,
        X86_ESI, pc_offset * sizeof (void *), 4);

    x86_test_reg_reg (b->jit_ptr, X86_EBX, X86_EBX);
    test3 = b->jit_ptr;
    x86_branch8 (b->jit_ptr, X86_CC_Z, 0, 1);
    x86_jump_reg (b->jit_ptr, X86_EBX);

    /* Returns to caller... */
    x86_patch (test1, b->jit_ptr);
    x86_patch (test2, b->jit_ptr);
    x86_patch (test3, b->jit_ptr);
    x86_patch (test6, b->jit_ptr);

    mips_set_pc (b, new_pc);
    mips_jit_tcb_push_epilog (b);
}

/* Set Jump */
static void mips_set_jump (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    m_va_t new_pc, int local_jump)
{
    int return_to_caller = FALSE;
    u_char *jump_ptr;

    if (!local_jump)
        return_to_caller = TRUE;

    if (!return_to_caller && mips_jit_tcb_local_addr (b, new_pc, &jump_ptr)) {
        if (jump_ptr) {
            x86_jump_code (b->jit_ptr, jump_ptr);
        } else {
            /* Never jump directly to code in a delay slot. */
            /*Hi yajin, why a delay slot can have entry point or why have to jmp
             * to delay slot?
             * The following is the code from celinux 2.4.
             *
             * 802a19d4:   1500004a        bnez    t0,802a1b00 <src_unaligned_dst_aligned>
             *
             * 802a19d8 <both_aligned>:
             * 802a19d8:   00064142        srl     t0,a2,0x5
             *
             * if call function both_aligned(0x802a19d8), it is in delay slot of 0x802a19d4 but it is
             * the entry of function both_aligned.
             * Just set pc to 0x802a19d8 and return main loop.
             */
            if (mips_jit_is_delay_slot (b, new_pc)) {
                mips_set_pc (b, new_pc);
                mips_jit_tcb_push_epilog (b);
                return;
            }

            mips_jit_tcb_record_patch (b, b->jit_ptr, new_pc);
            x86_jump32 (b->jit_ptr, 0);
        }
    } else {
        mips_try_direct_far_jump (cpu, b, new_pc);

#if 0
        if (cpu->exec_blk_direct_jump) {
            /* Block lookup optimization */
            mips_try_direct_far_jump (cpu, b, new_pc);
        } else {
            mips_set_pc (b, new_pc);
            mips_jit_tcb_push_epilog (b);
        }
#endif
    }

}

/* Basic C call */
void mips_emit_basic_c_call (mips_jit_tcb_t * b, void *f)
{
    x86_mov_reg_imm (b->jit_ptr, X86_EBX, f);
    x86_call_reg (b->jit_ptr, X86_EBX);
}

/* Emit a simple call to a C function without any parameter */
static void mips_emit_c_call (mips_jit_tcb_t * b, void *f)
{
    mips_set_pc (b, b->start_pc + ((b->mips_trans_pos - 1) << 2));
    mips_emit_basic_c_call (b, f);
}

/* Single-step operation */
void mips_emit_single_step (mips_jit_tcb_t * b, mips_insn_t insn)
{
    x86_mov_reg_reg (b->jit_ptr, X86_EAX, X86_EDI, 4);
    x86_mov_reg_imm (b->jit_ptr, X86_EDX, insn);
    mips_emit_basic_c_call (b, mips_exec_single_step);
}

/* Memory operation */
/*we use EAX EDX ECX to transfer parameter. yajin.
Makesure memory operation DONOT have more than 3 parameters*/
static void mips_emit_memop (mips_jit_tcb_t * b, int op, int base,
    int offset, int target, int keep_ll_bit)
{
    m_va_t val = sign_extend (offset, 16);
    m_uint8_t *test1;

    /* Save PC for exception handling */
    mips_set_pc (b, b->start_pc + ((b->mips_trans_pos - 1) << 2));

    if (!keep_ll_bit) {
        x86_clear_reg (b->jit_ptr, X86_EAX);
        x86_mov_membase_reg (b->jit_ptr, X86_EDI, OFFSET (cpu_mips_t, ll_bit),
            X86_EAX, 4);
    }

    x86_mov_reg_imm (b->jit_ptr, X86_EDX, val);

    /* EDX = GPR[base] + sign-extended offset */
    x86_alu_reg_membase (b->jit_ptr, X86_ADD, X86_EDX, X86_EDI,
        REG_OFFSET (base));

    /* ECX = target register */
    x86_mov_reg_imm (b->jit_ptr, X86_ECX, target);

    /* EAX = CPU instance pointer */
    x86_mov_reg_reg (b->jit_ptr, X86_EAX, X86_EDI, 4);

    x86_mov_reg_membase (b->jit_ptr, X86_EBX, X86_EDI, MEMOP_OFFSET (op), 4);
    x86_call_reg (b->jit_ptr, X86_EBX);

    /*check the return value */
    x86_alu_reg_imm (b->jit_ptr, X86_CMP, X86_EAX, 0);
    /*IF return value==0.NO exception. */
    test1 = b->jit_ptr;
    x86_branch8 (b->jit_ptr, X86_CC_E, 0, 1);
    mips_jit_tcb_push_epilog (b);
    x86_patch (test1, b->jit_ptr);

}

/* Unknown opcode handler */
static asmlinkage void mips_unknown_opcode (cpu_mips_t * cpu,
    m_uint32_t opcode)
{
    printf ("MIPS64: unhandled opcode 0x%8.8x at 0x%" LL "x (ra=0x%" LL
        "x)\n", opcode, cpu->pc, cpu->gpr[MIPS_GPR_RA]);

    //mips_dump_regs(cpu);
}

/* Emit unhandled instruction code */
static int mips_emit_unknown (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    mips_insn_t opcode)
{
    x86_mov_reg_imm (b->jit_ptr, X86_EAX, opcode);
    x86_alu_reg_imm (b->jit_ptr, X86_SUB, X86_ESP, 4);
    x86_push_reg (b->jit_ptr, X86_EAX);
    x86_push_reg (b->jit_ptr, X86_EDI);
    mips_emit_c_call (b, mips_unknown_opcode);
    x86_alu_reg_imm (b->jit_ptr, X86_ADD, X86_ESP, 12);
    return (0);
}

/* Invalid delay slot handler */
static fastcall void mips_invalid_delay_slot (cpu_mips_t * cpu)
{
    printf ("MIPS64: invalid instruction in delay slot at 0x%" LL "x (ra=0x%"
        LL "x)\n", cpu->pc, cpu->gpr[MIPS_GPR_RA]);

    //mips_dump_regs(cpu);

    /* Halt the virtual CPU */
    cpu->pc = 0;
}

/* Emit unhandled instruction code */
int mips_emit_invalid_delay_slot (mips_jit_tcb_t * b)
{
    x86_mov_reg_reg (b->jit_ptr, X86_EAX, X86_EDI, 4);
    mips_emit_c_call (b, mips_invalid_delay_slot);
    x86_alu_reg_imm (b->jit_ptr, X86_ADD, X86_ESP, 12);

    mips_jit_tcb_push_epilog (b);
    return (0);
}

void mips_check_cpu_pausing (mips_jit_tcb_t * b)
{
    u_char *test1;

    /* Check pause_request */
    x86_mov_reg_membase (b->jit_ptr, X86_EAX,
        X86_EDI, OFFSET (cpu_mips_t, pause_request), 4);
    x86_alu_reg_imm (b->jit_ptr, X86_AND, X86_EAX, CPU_INTERRUPT_EXIT);
    x86_alu_reg_imm (b->jit_ptr, X86_CMP, X86_EAX, CPU_INTERRUPT_EXIT);
    test1 = b->jit_ptr;
    x86_branch32 (b->jit_ptr, X86_CC_NE, 0, 1);
    /*if (cpu->pause_request)&CPU_INTERRUPT_EXIT==CPU_INTERRUPT_EXIT,
     * set cpu->state and return to main loop */
    mips_set_pc (b, b->start_pc + ((b->mips_trans_pos - 1) << 2));
    x86_mov_membase_imm (b->jit_ptr, X86_EDI, OFFSET (cpu_mips_t, state),
        CPU_STATE_PAUSING, 4);
    mips_jit_tcb_push_epilog (b);
    /*else do noting */
    x86_patch (test1, b->jit_ptr);

}

/* Check if there are pending IRQ */
void mips_check_pending_irq (mips_jit_tcb_t * b)
{
    u_char *test1;

    /* Check the pending IRQ flag */
    x86_mov_reg_membase (b->jit_ptr, X86_EAX,
        X86_EDI, OFFSET (cpu_mips_t, irq_pending), 4);
    x86_test_reg_reg (b->jit_ptr, X86_EAX, X86_EAX);
    test1 = b->jit_ptr;
    x86_branch32 (b->jit_ptr, X86_CC_Z, 0, 1);

    /* Save PC */
    mips_set_pc (b, b->start_pc + ((b->mips_trans_pos - 1) << 2));

    /* Trigger the IRQ */
    x86_mov_reg_reg (b->jit_ptr, X86_EAX, X86_EDI, 4);
    mips_emit_basic_c_call (b, mips_trigger_irq);
    mips_jit_tcb_push_epilog (b);

    x86_patch (test1, b->jit_ptr);
}

/* ADD */
static int mips_emit_add (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    //printf("rs %d rt %d rd %d \n",rs,rt,rd);

    /* TODO: Exception handling */
    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);
    x86_alu_reg_membase (b->jit_ptr, X86_ADD, X86_EAX, X86_EDI,
        REG_OFFSET (rt));
    x86_mov_membase_reg (b->jit_ptr, X86_EDI, REG_OFFSET (rd), X86_EAX, 4);

    return (0);
}

/* ADDI */
static int mips_emit_addi (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int imm = bits (insn, 0, 15);
    m_uint32_t val = sign_extend (imm, 16);
    // printf("rs %d rt %d val %d \n",rs,rt,val);
    /* TODO: Exception handling */

    x86_mov_reg_imm (b->jit_ptr, X86_EAX, val);
    x86_alu_reg_membase (b->jit_ptr, X86_ADD, X86_EAX, X86_EDI,
        REG_OFFSET (rs));
    x86_mov_membase_reg (b->jit_ptr, X86_EDI, REG_OFFSET (rt), X86_EAX, 4);
    return (0);
}

/* ADDI */
static int mips_emit_addiu (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int imm = bits (insn, 0, 15);
    m_uint32_t val = sign_extend (imm, 16);
    // printf("rs %d rt %d val %d \n",rs,rt,val);
    /* TODO: Exception handling */

    x86_mov_reg_imm (b->jit_ptr, X86_EAX, val);
    x86_alu_reg_membase (b->jit_ptr, X86_ADD, X86_EAX, X86_EDI,
        REG_OFFSET (rs));
    x86_mov_membase_reg (b->jit_ptr, X86_EDI, REG_OFFSET (rt), X86_EAX, 4);
    return (0);
}

/* ADDu */
static int mips_emit_addu (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    //  printf("rs %d rt %d rd %d \n",rs,rt,rd);

    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);
    x86_alu_reg_membase (b->jit_ptr, X86_ADD, X86_EAX, X86_EDI,
        REG_OFFSET (rt));
    x86_mov_membase_reg (b->jit_ptr, X86_EDI, REG_OFFSET (rd), X86_EAX, 4);

    return (0);
}

/* AND */
static int mips_emit_and (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);

    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);
    x86_alu_reg_membase (b->jit_ptr, X86_AND, X86_EAX, X86_EDI,
        REG_OFFSET (rt));
    x86_mov_membase_reg (b->jit_ptr, X86_EDI, REG_OFFSET (rd), X86_EAX, 4);
    return (0);
}

/* ANDI */
static int mips_emit_andi (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int imm = bits (insn, 0, 15);

    x86_mov_reg_imm (b->jit_ptr, X86_EAX, imm);
    x86_alu_reg_membase (b->jit_ptr, X86_AND, X86_EAX, X86_EDI,
        REG_OFFSET (rs));
    x86_mov_membase_reg (b->jit_ptr, X86_EDI, REG_OFFSET (rt), X86_EAX, 4);
    return (0);
}

static int mips_emit_bcond (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    uint16_t special_func = bits (insn, 16, 20);
    return mips_bcond_jit[special_func].emit_func (cpu, b, insn);
}

/*
Hi yajin, why we need call mips_jit_fetch_and_emit twice?
Why do we translate beq like this:

mips_jit_fetch_and_emit(cpu,b,1);
x86_mov_reg_membase(b->jit_ptr,X86_EAX,X86_EDI,REG_OFFSET(rs),4);
x86_alu_reg_membase(b->jit_ptr,X86_CMP,X86_EAX,X86_EDI,REG_OFFSET(rt));
test1 = b->jit_ptr;
x86_branch32(b->jit_ptr, X86_CC_NE, 0, 1);
mips_set_jump(cpu,b,new_pc,1);
x86_patch(test1,b->jit_ptr);

That is fetching the delay slot code first and then jumping to new pc according to
the comparing result of register rs and register rt.

This seems right but it is wrong and not easy to catch this bug!!!

The instruction in delay slot may change the content of register rs and rt and the
comparing result can not be trusted anymore.
We MUST compare register rs and rt and then run the code in delay slot.

*/
/* BEQ (Branch On Equal) */
static int mips_emit_beq (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);
    u_char *test1;
    m_va_t new_pc;

    /* compute the new pc */
    new_pc = b->start_pc + (b->mips_trans_pos << 2);
    new_pc += sign_extend (offset << 2, 18);

    /*
     * compare gpr[rs] and gpr[rt].
     * compare the low 32 bits first (higher probability).
     */
    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);
    x86_alu_reg_membase (b->jit_ptr, X86_CMP, X86_EAX, X86_EDI,
        REG_OFFSET (rt));
    test1 = b->jit_ptr;
    x86_branch32 (b->jit_ptr, X86_CC_NE, 0, 1);

    /* insert the instruction in the delay slot */
    mips_jit_fetch_and_emit (cpu, b, 2);

    /* set the new pc in cpu structure */
    mips_set_jump (cpu, b, new_pc, 1);

    x86_patch (test1, b->jit_ptr);

    /* if the branch is not taken, we have to execute the delay slot too */
    mips_jit_fetch_and_emit (cpu, b, 1);
    return (0);
}

/* BEQL (Branch On Equal Likely) */
static int mips_emit_beql (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);
    u_char *test1;
    m_va_t new_pc;

    /* compute the new pc */
    new_pc = b->start_pc + (b->mips_trans_pos << 2);
    new_pc += sign_extend (offset << 2, 18);

    /*
     * compare gpr[rs] and gpr[rt].
     * compare the low 32 bits first (higher probability).
     */
    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);
    x86_alu_reg_membase (b->jit_ptr, X86_CMP, X86_EAX, X86_EDI,
        REG_OFFSET (rt));
    test1 = b->jit_ptr;
    x86_branch32 (b->jit_ptr, X86_CC_NE, 0, 1);
    /* insert the instruction in the delay slot */
    mips_jit_fetch_and_emit (cpu, b, 1);
    /* set the new pc in cpu structure */
    mips_set_jump (cpu, b, new_pc, 1);

    x86_patch (test1, b->jit_ptr);
    return (0);
}

/* BGEZ (Branch On Greater or Equal Than Zero) */
static int mips_emit_bgez (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    u_char *test1;
    m_va_t new_pc;

    /* compute the new pc */
    new_pc = b->start_pc + (b->mips_trans_pos << 2);
    new_pc += sign_extend (offset << 2, 18);

    /* If sign bit is set, don't take the branch */
    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);
    x86_test_reg_reg (b->jit_ptr, X86_EAX, X86_EAX);
    test1 = b->jit_ptr;
    x86_branch32 (b->jit_ptr, X86_CC_S, 0, 1);

    /* insert the instruction in the delay slot */
    mips_jit_fetch_and_emit (cpu, b, 2);

    /* set the new pc in cpu structure */
    mips_set_jump (cpu, b, new_pc, 1);

    x86_patch (test1, b->jit_ptr);

    /* if the branch is not taken, we have to execute the delay slot too */
    mips_jit_fetch_and_emit (cpu, b, 1);
    return (0);
}

/* BGEZAL (Branch On Greater or Equal Than Zero And Link) */
static int mips_emit_bgezal (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    u_char *test1;
    m_va_t new_pc;

    /* compute the new pc */
    new_pc = b->start_pc + (b->mips_trans_pos << 2);
    new_pc += sign_extend (offset << 2, 18);

    /* set the return address (instruction after the delay slot) */
    mips_set_ra (b, b->start_pc + ((b->mips_trans_pos + 1) << 2));

    /* If sign bit is set, don't take the branch */
    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);
    x86_test_reg_reg (b->jit_ptr, X86_EAX, X86_EAX);
    test1 = b->jit_ptr;
    x86_branch32 (b->jit_ptr, X86_CC_S, 0, 1);

    /* insert the instruction in the delay slot */
    mips_jit_fetch_and_emit (cpu, b, 2);

    /* set the new pc in cpu structure */
    mips_set_jump (cpu, b, new_pc, 1);

    x86_patch (test1, b->jit_ptr);

    /* if the branch is not taken, we have to execute the delay slot too */
    mips_jit_fetch_and_emit (cpu, b, 1);
    return (0);
}

/* BGEZALL (Branch On Greater or Equal Than Zero and Link Likely) */
static int mips_emit_bgezall (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    u_char *test1;
    m_va_t new_pc;

    /* compute the new pc */
    new_pc = b->start_pc + (b->mips_trans_pos << 2);
    new_pc += sign_extend (offset << 2, 18);

    /* set the return address (instruction after the delay slot) */
    mips_set_ra (b, b->start_pc + ((b->mips_trans_pos + 1) << 2));

    /* if sign bit is set, don't take the branch */
    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);
    x86_test_reg_reg (b->jit_ptr, X86_EAX, X86_EAX);
    test1 = b->jit_ptr;
    x86_branch32 (b->jit_ptr, X86_CC_S, 0, 1);

    /* insert the instruction in the delay slot */
    mips_jit_fetch_and_emit (cpu, b, 1);

    /* set the new pc in cpu structure */
    mips_set_jump (cpu, b, new_pc, 1);

    x86_patch (test1, b->jit_ptr);
    return (0);
}

/* BGEZL (Branch On Greater or Equal Than Zero Likely) */
static int mips_emit_bgezl (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    u_char *test1;
    m_va_t new_pc;

    /* compute the new pc */
    new_pc = b->start_pc + (b->mips_trans_pos << 2);
    new_pc += sign_extend (offset << 2, 18);

    /* if sign bit is set, don't take the branch */
    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);
    x86_test_reg_reg (b->jit_ptr, X86_EAX, X86_EAX);
    test1 = b->jit_ptr;
    x86_branch32 (b->jit_ptr, X86_CC_S, 0, 1);

    /* insert the instruction in the delay slot */
    mips_jit_fetch_and_emit (cpu, b, 1);

    /* set the new pc in cpu structure */
    mips_set_jump (cpu, b, new_pc, 1);

    x86_patch (test1, b->jit_ptr);
    return (0);
}

/* BGTZ (Branch On Greater Than Zero) */
static int mips_emit_bgtz (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    u_char *test1, *test2;
    m_va_t new_pc;

    /* compute the new pc */
    new_pc = b->start_pc + (b->mips_trans_pos << 2);
    new_pc += sign_extend (offset << 2, 18);

    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);
    x86_test_reg_reg (b->jit_ptr, X86_EAX, X86_EAX);
    /*goto end if <0 */
    test1 = b->jit_ptr;
    x86_branch32 (b->jit_ptr, X86_CC_S, 0, 1);
    /*goto end if =0 */
    test2 = b->jit_ptr;
    x86_branch32 (b->jit_ptr, X86_CC_Z, 0, 1);

    /* here, we take the branch */
    //x86_patch(test2,b->jit_ptr);

    /* insert the instruction in the delay slot */
    mips_jit_fetch_and_emit (cpu, b, 2);

    /* set the new pc in cpu structure */
    mips_set_jump (cpu, b, new_pc, 1);

    x86_patch (test1, b->jit_ptr);
    x86_patch (test2, b->jit_ptr);

    /* if the branch is not taken, we have to execute the delay slot too */
    mips_jit_fetch_and_emit (cpu, b, 1);
    return (0);
}

/* BGTZL (Branch On Greater Than Zero Likely) */
static int mips_emit_bgtzl (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    u_char *test1, *test2;
    m_va_t new_pc;

    /* compute the new pc */
    new_pc = b->start_pc + (b->mips_trans_pos << 2);
    new_pc += sign_extend (offset << 2, 18);

    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);
    x86_test_reg_reg (b->jit_ptr, X86_EAX, X86_EAX);
    /*goto end if <0 */
    test1 = b->jit_ptr;
    x86_branch32 (b->jit_ptr, X86_CC_S, 0, 1);
    /*goto end if =0 */
    test2 = b->jit_ptr;
    x86_branch32 (b->jit_ptr, X86_CC_Z, 0, 1);

    /* here, we take the branch */
    // x86_patch(test2,b->jit_ptr);

    /* insert the instruction in the delay slot */
    mips_jit_fetch_and_emit (cpu, b, 1);

    /* set the new pc in cpu structure */
    mips_set_jump (cpu, b, new_pc, 1);

    x86_patch (test1, b->jit_ptr);
    x86_patch (test2, b->jit_ptr);
    return (0);
}

/* BLEZ (Branch On Less or Equal Than Zero) */
static int mips_emit_blez (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    u_char *test1, *test2;
    m_va_t new_pc;

    /* compute the new pc */
    new_pc = b->start_pc + (b->mips_trans_pos << 2);
    new_pc += sign_extend (offset << 2, 18);

    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);
    x86_test_reg_reg (b->jit_ptr, X86_EAX, X86_EAX);
    /*if <0, take the branch */
    test1 = b->jit_ptr;
    x86_branch32 (b->jit_ptr, X86_CC_S, 0, 1);
    /*else if !=0, do not take the branch */
    test2 = b->jit_ptr;
    x86_branch32 (b->jit_ptr, X86_CC_NZ, 0, 1);

    /* here, we take the branch */
    x86_patch (test1, b->jit_ptr);

    /* insert the instruction in the delay slot */
    mips_jit_fetch_and_emit (cpu, b, 2);

    /* set the new pc in cpu structure */
    mips_set_jump (cpu, b, new_pc, 1);

    x86_patch (test2, b->jit_ptr);

    /* if the branch is not taken, we have to execute the delay slot too */
    mips_jit_fetch_and_emit (cpu, b, 1);
    return (0);
}

/* BLEZL (Branch On Less or Equal Than Zero Likely) */
static int mips_emit_blezl (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    u_char *test1, *test2;
    m_va_t new_pc;

    /* compute the new pc */
    new_pc = b->start_pc + (b->mips_trans_pos << 2);
    new_pc += sign_extend (offset << 2, 18);

    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);
    x86_test_reg_reg (b->jit_ptr, X86_EAX, X86_EAX);
    /*if <0, take the branch */
    test1 = b->jit_ptr;
    x86_branch32 (b->jit_ptr, X86_CC_S, 0, 1);
    /*else if !=0, do not take the branch */
    test2 = b->jit_ptr;
    x86_branch32 (b->jit_ptr, X86_CC_NZ, 0, 1);

    /* here, we take the branch */
    x86_patch (test1, b->jit_ptr);

    /* insert the instruction in the delay slot */
    mips_jit_fetch_and_emit (cpu, b, 1);

    /* set the new pc in cpu structure */
    mips_set_jump (cpu, b, new_pc, 1);

    x86_patch (test2, b->jit_ptr);
    return (0);
}

/* BLTZ (Branch On Less Than Zero) */
static int mips_emit_bltz (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    u_char *test1;
    m_va_t new_pc;

    /* compute the new pc */
    new_pc = b->start_pc + (b->mips_trans_pos << 2);
    new_pc += sign_extend (offset << 2, 18);

    /*
     * test the sign bit of gpr[rs], if set, take the branch.
     */
    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);
    x86_test_reg_reg (b->jit_ptr, X86_EAX, X86_EAX);
    test1 = b->jit_ptr;
    x86_branch32 (b->jit_ptr, X86_CC_NS, 0, 1);

    /* insert the instruction in the delay slot */
    mips_jit_fetch_and_emit (cpu, b, 2);

    /* set the new pc in cpu structure */
    mips_set_jump (cpu, b, new_pc, 1);

    x86_patch (test1, b->jit_ptr);

    /* if the branch is not taken, we have to execute the delay slot too */
    mips_jit_fetch_and_emit (cpu, b, 1);
    return (0);
}

/* BLTZAL (Branch On Less Than Zero And Link) */
static int mips_emit_bltzal (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    u_char *test1;
    m_va_t new_pc;

    /* compute the new pc */
    new_pc = b->start_pc + (b->mips_trans_pos << 2);
    new_pc += sign_extend (offset << 2, 18);

    /* set the return address (instruction after the delay slot) */
    mips_set_ra (b, b->start_pc + ((b->mips_trans_pos + 1) << 2));

    /*
     * test the sign bit of gpr[rs], if set, take the branch.
     */
    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);
    x86_test_reg_reg (b->jit_ptr, X86_EAX, X86_EAX);
    test1 = b->jit_ptr;
    x86_branch32 (b->jit_ptr, X86_CC_NS, 0, 1);

    /* insert the instruction in the delay slot */
    mips_jit_fetch_and_emit (cpu, b, 2);

    /* set the new pc in cpu structure */
    mips_set_jump (cpu, b, new_pc, 1);

    x86_patch (test1, b->jit_ptr);

    /* if the branch is not taken, we have to execute the delay slot too */
    mips_jit_fetch_and_emit (cpu, b, 1);
    return (0);
}

/* BLTZALL (Branch On Less Than Zero And Link Likely) */
static int mips_emit_bltzall (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    u_char *test1;
    m_va_t new_pc;

    /* compute the new pc */
    new_pc = b->start_pc + (b->mips_trans_pos << 2);
    new_pc += sign_extend (offset << 2, 18);

    /* set the return address (instruction after the delay slot) */
    mips_set_ra (b, b->start_pc + ((b->mips_trans_pos + 1) << 2));

    /*
     * test the sign bit of gpr[rs], if set, take the branch.
     */
    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);
    x86_test_reg_reg (b->jit_ptr, X86_EAX, X86_EAX);
    test1 = b->jit_ptr;
    x86_branch32 (b->jit_ptr, X86_CC_NS, 0, 1);

    /* insert the instruction in the delay slot */
    mips_jit_fetch_and_emit (cpu, b, 1);

    /* set the new pc in cpu structure */
    mips_set_jump (cpu, b, new_pc, 1);

    x86_patch (test1, b->jit_ptr);
    return (0);
}

/* BLTZL (Branch On Less Than Zero Likely) */
static int mips_emit_bltzl (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    u_char *test1;
    m_va_t new_pc;

    /* compute the new pc */
    new_pc = b->start_pc + (b->mips_trans_pos << 2);
    new_pc += sign_extend (offset << 2, 18);

    /*
     * test the sign bit of gpr[rs], if set, take the branch.
     */
    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);
    x86_test_reg_reg (b->jit_ptr, X86_EAX, X86_EAX);
    test1 = b->jit_ptr;
    x86_branch32 (b->jit_ptr, X86_CC_NS, 0, 1);

    /* insert the instruction in the delay slot */
    mips_jit_fetch_and_emit (cpu, b, 1);

    /* set the new pc in cpu structure */
    mips_set_jump (cpu, b, new_pc, 1);

    x86_patch (test1, b->jit_ptr);
    return (0);
}

/* BNE (Branch On Not Equal) */
static int mips_emit_bne (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);
    u_char *test2;
    m_va_t new_pc;

    /* compute the new pc */
    new_pc = b->start_pc + (b->mips_trans_pos << 2);
    new_pc += sign_extend (offset << 2, 18);

    /*
     * compare gpr[rs] and gpr[rt].
     * compare the low 32 bits first (higher probability).
     */
    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);
    x86_alu_reg_membase (b->jit_ptr, X86_CMP, X86_EAX, X86_EDI,
        REG_OFFSET (rt));
    test2 = b->jit_ptr;
    x86_branch32 (b->jit_ptr, X86_CC_E, 0, 1);

    /* insert the instruction in the delay slot */
    mips_jit_fetch_and_emit (cpu, b, 2);

    /* set the new pc in cpu structure */
    mips_set_jump (cpu, b, new_pc, 1);

    x86_patch (test2, b->jit_ptr);

    /* if the branch is not taken, we have to execute the delay slot too */
    mips_jit_fetch_and_emit (cpu, b, 1);
    return (0);
}

/* BNEL (Branch On Not Equal Likely) */
static int mips_emit_bnel (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);
    u_char *test2;
    m_va_t new_pc;

    /* compute the new pc */
    new_pc = b->start_pc + (b->mips_trans_pos << 2);
    new_pc += sign_extend (offset << 2, 18);

    /*
     * compare gpr[rs] and gpr[rt].
     * compare the low 32 bits first (higher probability).
     */
    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);
    x86_alu_reg_membase (b->jit_ptr, X86_CMP, X86_EAX, X86_EDI,
        REG_OFFSET (rt));
    test2 = b->jit_ptr;
    x86_branch32 (b->jit_ptr, X86_CC_E, 0, 1);

    /* insert the instruction in the delay slot */
    mips_jit_fetch_and_emit (cpu, b, 1);

    /* set the new pc in cpu structure */
    mips_set_jump (cpu, b, new_pc, 1);

    x86_patch (test2, b->jit_ptr);

    return (0);
}

/* BREAK */
static int mips_emit_break (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    u_int code = bits (insn, 6, 25);

    //x86_alu_reg_imm(b->jit_ptr,X86_SUB,X86_ESP,12);
    x86_mov_reg_imm (b->jit_ptr, X86_EDX, code);
    x86_mov_reg_reg (b->jit_ptr, X86_EAX, X86_EDI, 4);
    mips_emit_basic_c_call (b, mips_exec_break);
    x86_alu_reg_imm (b->jit_ptr, X86_ADD, X86_ESP, 12);

    mips_jit_tcb_push_epilog (b);
    return (0);
}

/* CACHE */
static int mips_emit_cache (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int base = bits (insn, 21, 25);
    int op = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    mips_emit_memop (b, MIPS_MEMOP_CACHE, base, offset, op, FALSE);
    return (0);
}

static int mips_emit_cfc0 (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int fastcall clz_emu (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rd = bits (insn, 11, 15);
    int i;
    m_uint32_t val;
    val = 32;
    for (i = 31; i >= 0; i--) {
        if (cpu->gpr[rs] & (1 << i)) {
            val = 31 - i;
            break;
        }
    }
    cpu->gpr[rd] = val;
    return (0);

}

static int mips_emit_clz (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    //x86_alu_reg_imm(b->jit_ptr,X86_SUB,X86_ESP,12);
    x86_mov_reg_imm (b->jit_ptr, X86_EDX, insn);
    x86_mov_reg_reg (b->jit_ptr, X86_EAX, X86_EDI, 4);
    mips_emit_basic_c_call (b, clz_emu);
    return (0);
}

static int mips_emit_cop0 (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    uint16_t special_func = bits (insn, 21, 25);
    return mips_cop0_jit[special_func].emit_func (cpu, b, insn);
}

static int mips_emit_cop1 (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
#if SOFT_FPU
    /* Save PC for exception handling */
    mips_set_pc (b, b->start_pc + ((b->mips_trans_pos - 1) << 2));
    x86_mov_reg_reg (b->jit_ptr, X86_EAX, X86_EDI, 4);
    mips_emit_basic_c_call (b, mips_exec_soft_fpu);
    mips_jit_tcb_push_epilog (b);
    return (0);
#else
    mips_emit_unknown (cpu, b, insn);
    return (0);
#endif
}

static int mips_emit_cop1x (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
#if SOFT_FPU
    /* Save PC for exception handling */
    mips_set_pc (b, b->start_pc + ((b->mips_trans_pos - 1) << 2));
    x86_mov_reg_reg (b->jit_ptr, X86_EAX, X86_EDI, 4);
    mips_emit_basic_c_call (b, mips_exec_soft_fpu);
    mips_jit_tcb_push_epilog (b);
    return (0);
#else
    mips_emit_unknown (cpu, b, insn);
    return (0);
#endif

}

static int mips_emit_cop2 (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_dadd (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_daddi (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_daddiu (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_daddu (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_ddiv (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_ddivu (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_div (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);

    /* eax = gpr[rs] */
    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);
    x86_cdq (b->jit_ptr);
    /* ebx = gpr[rt] */
    x86_mov_reg_membase (b->jit_ptr, X86_EBX, X86_EDI, REG_OFFSET (rt), 4);

    /* eax = quotient (LO), edx = remainder (HI) */
    x86_div_reg (b->jit_ptr, X86_EBX, 1);

    /* store LO */
    x86_mov_membase_reg (b->jit_ptr, X86_EDI, OFFSET (cpu_mips_t, lo),
        X86_EAX, 4);
    /* store HI */
    x86_mov_membase_reg (b->jit_ptr, X86_EDI, OFFSET (cpu_mips_t, hi),
        X86_EDX, 4);

    return (0);
}

static int mips_emit_divu (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);

    /* eax = gpr[rs] */
    x86_clear_reg (b->jit_ptr, X86_EDX);
    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);
    /* ebx = gpr[rt] */
    x86_mov_reg_membase (b->jit_ptr, X86_EBX, X86_EDI, REG_OFFSET (rt), 4);

    /* eax = quotient (LO), edx = remainder (HI) */
    x86_div_reg (b->jit_ptr, X86_EBX, 0);

    /* store LO */
    x86_mov_membase_reg (b->jit_ptr, X86_EDI, OFFSET (cpu_mips_t, lo),
        X86_EAX, 4);
    /* store HI */
    x86_mov_membase_reg (b->jit_ptr, X86_EDI, OFFSET (cpu_mips_t, hi),
        X86_EDX, 4);
    return (0);
}

static int mips_emit_dmfc0 (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_dmtc0 (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_dmult (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_dmultu (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_dsll (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_dsllv (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_dsrlv (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_dsrav (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_dsub (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_dsubu (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_dsrl (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_dsra (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_dsll32 (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_dsrl32 (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_dsra32 (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_eret (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_set_pc (b, b->start_pc + ((b->mips_trans_pos - 1) << 2));

    x86_mov_reg_reg (b->jit_ptr, X86_EAX, X86_EDI, 4);
    mips_emit_basic_c_call (b, mips_exec_eret);
    mips_jit_tcb_push_epilog (b);
    return (0);
}

/* J (Jump) */
static int mips_emit_j (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    u_int instr_index = bits (insn, 0, 25);
    m_va_t new_pc;

    /* compute the new pc */
    new_pc = b->start_pc + (b->mips_trans_pos << 2);
    new_pc &= ~((1 << 28) - 1);
    new_pc |= instr_index << 2;

    /* insert the instruction in the delay slot */
    mips_jit_fetch_and_emit (cpu, b, 1);

    /* set the new pc in cpu structure */
    mips_set_jump (cpu, b, new_pc, 1);
    return (0);
}

static int mips_emit_jal (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    u_int instr_index = bits (insn, 0, 25);
    m_va_t new_pc, ret_pc;

    /* compute the new pc */
    new_pc = b->start_pc + (b->mips_trans_pos << 2);
    new_pc &= ~((1 << 28) - 1);
    new_pc |= instr_index << 2;

    /* set the return address (instruction after the delay slot) */
    ret_pc = b->start_pc + ((b->mips_trans_pos + 1) << 2);
    mips_set_ra (b, ret_pc);

    /* insert the instruction in the delay slot */
    mips_jit_fetch_and_emit (cpu, b, 1);

    /* set the new pc in cpu structure */
    mips_set_jump (cpu, b, new_pc, 0);
    return (0);

}

static int mips_emit_jalr (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rd = bits (insn, 11, 15);
    m_va_t ret_pc;

    /* set the return pc (instruction after the delay slot) in GPR[rd] */
    ret_pc = b->start_pc + ((b->mips_trans_pos + 1) << 2);
    x86_mov_membase_imm (b->jit_ptr, X86_EDI, REG_OFFSET (rd), ret_pc, 4);

    /* get the new pc */
    x86_mov_reg_membase (b->jit_ptr, X86_ECX, X86_EDI, REG_OFFSET (rs), 4);
    x86_mov_membase_reg (b->jit_ptr, X86_EDI, OFFSET (cpu_mips_t, ret_pc),
        X86_ECX, 4);

    /* insert the instruction in the delay slot */
    mips_jit_fetch_and_emit (cpu, b, 1);

    /* set the new pc */
    x86_mov_reg_membase (b->jit_ptr, X86_ECX,
        X86_EDI, OFFSET (cpu_mips_t, ret_pc), 4);
    x86_mov_membase_reg (b->jit_ptr, X86_EDI, OFFSET (cpu_mips_t, pc),
        X86_ECX, 4);

    /* returns to the caller which will determine the next path */
    mips_jit_tcb_push_epilog (b);
    return (0);

}

static int mips_emit_jr (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);

    /* get the new pc */
    x86_mov_reg_membase (b->jit_ptr, X86_ECX, X86_EDI, REG_OFFSET (rs), 4);
    x86_mov_membase_reg (b->jit_ptr, X86_EDI, OFFSET (cpu_mips_t, ret_pc),
        X86_ECX, 4);

    /* insert the instruction in the delay slot */
    mips_jit_fetch_and_emit (cpu, b, 1);

    /* set the new pc */
    x86_mov_reg_membase (b->jit_ptr, X86_ECX,
        X86_EDI, OFFSET (cpu_mips_t, ret_pc), 4);
    x86_mov_membase_reg (b->jit_ptr, X86_EDI, OFFSET (cpu_mips_t, pc),
        X86_ECX, 4);
    /* returns to the caller which will determine the next path */
    mips_jit_tcb_push_epilog (b);
    return (0);

}

static int mips_emit_lb (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    mips_emit_memop (b, MIPS_MEMOP_LB, base, offset, rt, TRUE);
    return (0);
}

static int mips_emit_lbu (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    mips_emit_memop (b, MIPS_MEMOP_LBU, base, offset, rt, TRUE);
    return (0);
}

static int mips_emit_ld (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_ldc1 (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_ldc2 (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_ldl (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_ldr (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_lh (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    mips_emit_memop (b, MIPS_MEMOP_LH, base, offset, rt, TRUE);
    return (0);
}

static int mips_emit_lhu (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    mips_emit_memop (b, MIPS_MEMOP_LHU, base, offset, rt, TRUE);
    return (0);
}

static int mips_emit_ll (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    mips_emit_memop (b, MIPS_MEMOP_LL, base, offset, rt, TRUE);
    return (0);
}

static int mips_emit_lld (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_lui (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rt = bits (insn, 16, 20);
    int imm = bits (insn, 0, 15);
    m_reg_t val = sign_extend (imm, 16) << 16;

    x86_mov_membase_imm (b->jit_ptr, X86_EDI, REG_OFFSET (rt), val, 4);

    return (0);
}

static int mips_emit_lw (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    mips_emit_memop (b, MIPS_MEMOP_LW, base, offset, rt, TRUE);
    return (0);
}

static int mips_emit_lwc1 (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
#if SOFT_FPU
    /* Save PC for exception handling */
    mips_set_pc (b, b->start_pc + ((b->mips_trans_pos - 1) << 2));
    x86_mov_reg_reg (b->jit_ptr, X86_EAX, X86_EDI, 4);
    mips_emit_basic_c_call (b, mips_exec_soft_fpu);
    mips_jit_tcb_push_epilog (b);
    return (0);
#else
    mips_emit_unknown (cpu, b, insn);
    return (0);
#endif

}

static int mips_emit_lwc2 (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_lwl (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    mips_emit_memop (b, MIPS_MEMOP_LWL, base, offset, rt, TRUE);
    return (0);
}

static int mips_emit_lwr (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    mips_emit_memop (b, MIPS_MEMOP_LWR, base, offset, rt, TRUE);
    return (0);
}

static int mips_emit_lwu (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    mips_emit_memop (b, MIPS_MEMOP_LWU, base, offset, rt, TRUE);
    return (0);
}

static int mips_emit_mad (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int index = bits (insn, 0, 5);
    return mips_mad_jit[index].emit_func (cpu, b, insn);
}

static int fastcall madd_emu (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    m_int64_t val, temp;

    val = (m_int32_t) (m_int32_t) cpu->gpr[rs];
    val *= (m_int32_t) (m_int32_t) cpu->gpr[rt];

    temp = cpu->hi;
    temp = temp << 32;
    temp += cpu->lo;
    val += temp;

    cpu->lo = sign_extend (val, 32);
    cpu->hi = sign_extend (val >> 32, 32);
    return (0);

}

static int mips_emit_madd (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    //x86_alu_reg_imm(b->jit_ptr,X86_SUB,X86_ESP,12);
    x86_mov_reg_imm (b->jit_ptr, X86_EDX, insn);
    x86_mov_reg_reg (b->jit_ptr, X86_EAX, X86_EDI, 4);
    mips_emit_basic_c_call (b, madd_emu);
    return (0);
}

static int fastcall maddu_emu (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    m_int64_t val, temp;

    val = (m_uint32_t) (m_uint32_t) cpu->gpr[rs];
    val *= (m_uint32_t) (m_uint32_t) cpu->gpr[rt];

    temp = cpu->hi;
    temp = temp << 32;
    temp += cpu->lo;
    val += temp;

    cpu->lo = sign_extend (val, 32);
    cpu->hi = sign_extend (val >> 32, 32);
    return (0);

}

static int mips_emit_maddu (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    //x86_alu_reg_imm(b->jit_ptr,X86_SUB,X86_ESP,12);
    x86_mov_reg_imm (b->jit_ptr, X86_EDX, insn);
    x86_mov_reg_reg (b->jit_ptr, X86_EAX, X86_EDI, 4);
    mips_emit_basic_c_call (b, maddu_emu);
    return (0);
}

static int mips_emit_mfc0 (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    x86_mov_reg_imm (b->jit_ptr, X86_EDX, insn);
    x86_mov_reg_reg (b->jit_ptr, X86_EAX, X86_EDI, 4);
    mips_emit_basic_c_call (b, mips_cp0_exec_mfc0_fastcall);
    return (0);

}

static int mips_emit_mfhi (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rd = bits (insn, 11, 15);
    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, OFFSET (cpu_mips_t,
            hi), 4);
    x86_mov_membase_reg (b->jit_ptr, X86_EDI, REG_OFFSET (rd), X86_EAX, 4);

    return (0);
}

static int mips_emit_mflo (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rd = bits (insn, 11, 15);
    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, OFFSET (cpu_mips_t,
            lo), 4);
    x86_mov_membase_reg (b->jit_ptr, X86_EDI, REG_OFFSET (rd), X86_EAX, 4);
    return (0);
}

static int mips_emit_movc (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_movz (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rd = bits (insn, 11, 15);
    int rt = bits (insn, 16, 20);
    u_char *test1;

    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rt), 4);
    x86_test_reg_reg (b->jit_ptr, X86_EAX, X86_EAX);
    /*goto end if !=0 */
    test1 = b->jit_ptr;
    x86_branch32 (b->jit_ptr, X86_CC_NZ, 0, 1);
    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);
    x86_mov_membase_reg (b->jit_ptr, X86_EDI, REG_OFFSET (rd), X86_EAX, 4);

    x86_patch (test1, b->jit_ptr);
    return (0);

}

static int mips_emit_movn (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rd = bits (insn, 11, 15);
    int rt = bits (insn, 16, 20);
    u_char *test1;

    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rt), 4);
    x86_test_reg_reg (b->jit_ptr, X86_EAX, X86_EAX);
    /*goto end if ==0 */
    test1 = b->jit_ptr;
    x86_branch32 (b->jit_ptr, X86_CC_Z, 0, 1);
    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);
    x86_mov_membase_reg (b->jit_ptr, X86_EDI, REG_OFFSET (rd), X86_EAX, 4);

    x86_patch (test1, b->jit_ptr);
    return (0);

}

static int fastcall msub_emu (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    m_int64_t val, temp;

    val = (m_int32_t) (m_int32_t) cpu->gpr[rs];
    val *= (m_int32_t) (m_int32_t) cpu->gpr[rt];

    temp = cpu->hi;
    temp = temp << 32;
    temp += cpu->lo;

    temp -= val;
    //val += temp;

    cpu->lo = sign_extend (temp, 32);
    cpu->hi = sign_extend (temp >> 32, 32);
    return (0);

}

static int mips_emit_msub (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    //x86_alu_reg_imm(b->jit_ptr,X86_SUB,X86_ESP,12);
    x86_mov_reg_imm (b->jit_ptr, X86_EDX, insn);
    x86_mov_reg_reg (b->jit_ptr, X86_EAX, X86_EDI, 4);
    mips_emit_basic_c_call (b, msub_emu);
    return (0);
}

static int fastcall msubu_emu (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    m_int64_t val, temp;

    val = (m_uint32_t) (m_uint32_t) cpu->gpr[rs];
    val *= (m_uint32_t) (m_uint32_t) cpu->gpr[rt];

    temp = cpu->hi;
    temp = temp << 32;
    temp += cpu->lo;

    temp -= val;

    cpu->lo = sign_extend (temp, 32);
    cpu->hi = sign_extend (temp >> 32, 32);
    return (0);

}

static int mips_emit_msubu (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    //x86_alu_reg_imm(b->jit_ptr,X86_SUB,X86_ESP,12);
    x86_mov_reg_imm (b->jit_ptr, X86_EDX, insn);
    x86_mov_reg_reg (b->jit_ptr, X86_EAX, X86_EDI, 4);
    mips_emit_basic_c_call (b, msubu_emu);
    return (0);
}

static int mips_emit_mtc0 (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    x86_mov_reg_imm (b->jit_ptr, X86_EDX, insn);
    x86_mov_reg_reg (b->jit_ptr, X86_EAX, X86_EDI, 4);
    mips_emit_basic_c_call (b, mips_cp0_exec_mtc0_fastcall);
    return (0);

}

static fastcall int mthi_emu (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);

    cpu->hi = cpu->gpr[rs];
    return (0);
}

static fastcall int mtlo_emu (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);

    cpu->lo = cpu->gpr[rs];
    return (0);

}

static int mips_emit_mthi (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    //x86_alu_reg_imm(b->jit_ptr,X86_SUB,X86_ESP,12);
    x86_mov_reg_imm (b->jit_ptr, X86_EDX, insn);
    x86_mov_reg_reg (b->jit_ptr, X86_EAX, X86_EDI, 4);
    mips_emit_basic_c_call (b, mthi_emu);
    return (0);
}

static int mips_emit_mtlo (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    //x86_alu_reg_imm(b->jit_ptr,X86_SUB,X86_ESP,12);
    x86_mov_reg_imm (b->jit_ptr, X86_EDX, insn);
    x86_mov_reg_reg (b->jit_ptr, X86_EAX, X86_EDI, 4);
    mips_emit_basic_c_call (b, mtlo_emu);
    return (0);
}

/* MUL */
static int mips_emit_mul (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);

    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);
    x86_mov_reg_membase (b->jit_ptr, X86_EBX, X86_EDI, REG_OFFSET (rt), 4);

    x86_mul_reg (b->jit_ptr, X86_EBX, 1);

    /* store result in gpr[rd] */
    x86_mov_membase_reg (b->jit_ptr, X86_EDI, REG_OFFSET (rd), X86_EAX, 4);
    return (0);
}

static int mips_emit_mult (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);

    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);
    x86_mov_reg_membase (b->jit_ptr, X86_EBX, X86_EDI, REG_OFFSET (rt), 4);

    x86_mul_reg (b->jit_ptr, X86_EBX, 1);

    /* store LO */
    x86_mov_membase_reg (b->jit_ptr, X86_EDI, OFFSET (cpu_mips_t, lo),
        X86_EAX, 4);
    /* store HI */
    x86_mov_membase_reg (b->jit_ptr, X86_EDI, OFFSET (cpu_mips_t, hi),
        X86_EDX, 4);
    return (0);
}

static int mips_emit_multu (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);

    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);
    x86_mov_reg_membase (b->jit_ptr, X86_EBX, X86_EDI, REG_OFFSET (rt), 4);

    x86_mul_reg (b->jit_ptr, X86_EBX, 0);

    /* store LO */
    x86_mov_membase_reg (b->jit_ptr, X86_EDI, OFFSET (cpu_mips_t, lo),
        X86_EAX, 4);
    /* store HI */
    x86_mov_membase_reg (b->jit_ptr, X86_EDI, OFFSET (cpu_mips_t, hi),
        X86_EDX, 4);
    return (0);
}

static int mips_emit_nor (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);

    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);
    x86_alu_reg_membase (b->jit_ptr, X86_OR, X86_EAX, X86_EDI,
        REG_OFFSET (rt));
    x86_not_reg (b->jit_ptr, X86_EAX);
    x86_mov_membase_reg (b->jit_ptr, X86_EDI, REG_OFFSET (rd), X86_EAX, 4);
    return (0);

}

static int mips_emit_or (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);

    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);
    x86_alu_reg_membase (b->jit_ptr, X86_OR, X86_EAX, X86_EDI,
        REG_OFFSET (rt));
    x86_mov_membase_reg (b->jit_ptr, X86_EDI, REG_OFFSET (rd), X86_EAX, 4);
    return (0);

}

static int mips_emit_ori (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int imm = bits (insn, 0, 15);

    x86_mov_reg_imm (b->jit_ptr, X86_EAX, imm & 0xffff);
    x86_alu_reg_membase (b->jit_ptr, X86_OR, X86_EAX, X86_EDI,
        REG_OFFSET (rs));
    x86_mov_membase_reg (b->jit_ptr, X86_EDI, REG_OFFSET (rt), X86_EAX, 4);
    return (0);

}

static int mips_emit_pref (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    return (0);
}

static int mips_emit_sb (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    mips_emit_memop (b, MIPS_MEMOP_SB, base, offset, rt, TRUE);
    return (0);
}

static int mips_emit_sc (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    mips_emit_memop (b, MIPS_MEMOP_SC, base, offset, rt, TRUE);
    return (0);
}

static int mips_emit_scd (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_sd (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_sdc1 (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
#if SOFT_FPU
    /* Save PC for exception handling */
    mips_set_pc (b, b->start_pc + ((b->mips_trans_pos - 1) << 2));
    x86_mov_reg_reg (b->jit_ptr, X86_EAX, X86_EDI, 4);
    mips_emit_basic_c_call (b, mips_exec_soft_fpu);
    mips_jit_tcb_push_epilog (b);
    return (0);
#else
    mips_emit_unknown (cpu, b, insn);
    return (0);
#endif

}

static int mips_emit_sdc2 (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_sdl (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_sdr (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_sh (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    mips_emit_memop (b, MIPS_MEMOP_SH, base, offset, rt, TRUE);
    return (0);
}

static int mips_emit_sll (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    int sa = bits (insn, 6, 10);

    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rt), 4);
    x86_shift_reg_imm (b->jit_ptr, X86_SHL, X86_EAX, sa);

    x86_mov_membase_reg (b->jit_ptr, X86_EDI, REG_OFFSET (rd), X86_EAX, 4);
    return (0);
}

static int mips_emit_sllv (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);

    x86_mov_reg_membase (b->jit_ptr, X86_ECX, X86_EDI, REG_OFFSET (rs), 4);
    x86_alu_reg_imm (b->jit_ptr, X86_AND, X86_ECX, 0x1f);

    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rt), 4);
    x86_shift_reg (b->jit_ptr, X86_SHL, X86_EAX);

    x86_mov_membase_reg (b->jit_ptr, X86_EDI, REG_OFFSET (rd), X86_EAX, 4);
    return (0);

}

static int mips_emit_slt (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    u_char *test1;

    /* eax = gpr[rt] */
    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rt), 4);
    /* ecx = gpr[rs] */
    x86_mov_reg_membase (b->jit_ptr, X86_ECX, X86_EDI, REG_OFFSET (rs), 4);

    /* we set rd to 1 when gpr[rs] < gpr[rt] */
    x86_clear_reg (b->jit_ptr, X86_ESI);
    x86_mov_membase_reg (b->jit_ptr, X86_EDI, REG_OFFSET (rd), X86_ESI, 4);

    x86_alu_reg_reg (b->jit_ptr, X86_CMP, X86_ECX, X86_EAX);
    /* rs >= rt => end */
    test1 = b->jit_ptr;
    x86_branch8 (b->jit_ptr, X86_CC_GE, 0, 1);
    x86_inc_membase (b->jit_ptr, X86_EDI, REG_OFFSET (rd));
    /* end */
    x86_patch (test1, b->jit_ptr);
    return (0);

}

static int mips_emit_slti (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int imm = bits (insn, 0, 15);
    m_ireg_t val = sign_extend (imm, 16);
    u_char *test1;

    /* eax =val */
    x86_mov_reg_imm (b->jit_ptr, X86_EAX, val);
    /* ecx = gpr[rs] */
    x86_mov_reg_membase (b->jit_ptr, X86_ECX, X86_EDI, REG_OFFSET (rs), 4);

    /* we set rt to 1 when gpr[rs] < val */
    x86_clear_reg (b->jit_ptr, X86_ESI);
    x86_mov_membase_reg (b->jit_ptr, X86_EDI, REG_OFFSET (rt), X86_ESI, 4);

    x86_alu_reg_reg (b->jit_ptr, X86_CMP, X86_ECX, X86_EAX);
    /* rs >= val => end */
    test1 = b->jit_ptr;
    x86_branch8 (b->jit_ptr, X86_CC_GE, 0, 1);
    x86_inc_membase (b->jit_ptr, X86_EDI, REG_OFFSET (rt));
    /* end */
    x86_patch (test1, b->jit_ptr);
    return (0);

}

static int mips_emit_sltiu (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int imm = bits (insn, 0, 15);
    m_reg_t val = sign_extend (imm, 16);
    u_char *test1;

    /* eax =val */
    x86_mov_reg_imm (b->jit_ptr, X86_EAX, val);
    /* ecx = gpr[rs] */
    x86_mov_reg_membase (b->jit_ptr, X86_ECX, X86_EDI, REG_OFFSET (rs), 4);

    /* we set rt to 1 when gpr[rs] < val */
    x86_clear_reg (b->jit_ptr, X86_ESI);
    x86_mov_membase_reg (b->jit_ptr, X86_EDI, REG_OFFSET (rt), X86_ESI, 4);

    x86_alu_reg_reg (b->jit_ptr, X86_CMP, X86_ECX, X86_EAX);
    /* rs() >= val => end */
    test1 = b->jit_ptr;
    x86_branch8 (b->jit_ptr, X86_CC_GE, 0, 0);
    x86_inc_membase (b->jit_ptr, X86_EDI, REG_OFFSET (rt));
    /* end */
    x86_patch (test1, b->jit_ptr);
    return (0);

}

static int mips_emit_sltu (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    u_char *test1;

    /* eax = gpr[rt] */
    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rt), 4);
    /* ecx = gpr[rs] */
    x86_mov_reg_membase (b->jit_ptr, X86_ECX, X86_EDI, REG_OFFSET (rs), 4);

    /* we set rd to 1 when gpr[rs] < gpr[rt] */
    x86_clear_reg (b->jit_ptr, X86_ESI);
    x86_mov_membase_reg (b->jit_ptr, X86_EDI, REG_OFFSET (rd), X86_ESI, 4);

    x86_alu_reg_reg (b->jit_ptr, X86_CMP, X86_ECX, X86_EAX);
    /* rs() >= rt(high) => end */
    test1 = b->jit_ptr;
    x86_branch8 (b->jit_ptr, X86_CC_GE, 0, 0);
    x86_inc_membase (b->jit_ptr, X86_EDI, REG_OFFSET (rd));
    /* end */
    x86_patch (test1, b->jit_ptr);
    return (0);

}

static int mips_emit_spec (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    uint16_t special_func = bits (insn, 0, 5);
    return mips_spec_jit[special_func].emit_func (cpu, b, insn);
}

static int mips_emit_sra (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    int sa = bits (insn, 6, 10);

    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rt), 4);
    x86_shift_reg_imm (b->jit_ptr, X86_SAR, X86_EAX, sa);

    x86_mov_membase_reg (b->jit_ptr, X86_EDI, REG_OFFSET (rd), X86_EAX, 4);
    return (0);

}

static int mips_emit_srav (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);

    x86_mov_reg_membase (b->jit_ptr, X86_ECX, X86_EDI, REG_OFFSET (rs), 4);
    x86_alu_reg_imm (b->jit_ptr, X86_AND, X86_ECX, 0x1f);

    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rt), 4);
    x86_shift_reg (b->jit_ptr, X86_SAR, X86_EAX);

    x86_mov_membase_reg (b->jit_ptr, X86_EDI, REG_OFFSET (rd), X86_EAX, 4);
    return (0);

}

static int mips_emit_srl (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    int sa = bits (insn, 6, 10);

    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rt), 4);
    x86_shift_reg_imm (b->jit_ptr, X86_SHR, X86_EAX, sa);

    x86_mov_membase_reg (b->jit_ptr, X86_EDI, REG_OFFSET (rd), X86_EAX, 4);
    return (0);

}

static int mips_emit_srlv (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);

    x86_mov_reg_membase (b->jit_ptr, X86_ECX, X86_EDI, REG_OFFSET (rs), 4);
    x86_alu_reg_imm (b->jit_ptr, X86_AND, X86_ECX, 0x1f);

    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rt), 4);
    x86_shift_reg (b->jit_ptr, X86_SHR, X86_EAX);

    x86_mov_membase_reg (b->jit_ptr, X86_EDI, REG_OFFSET (rd), X86_EAX, 4);
    return (0);

}

static int mips_emit_sub (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);

    /* TODO: Exception handling */
    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);
    x86_alu_reg_membase (b->jit_ptr, X86_SUB, X86_EAX, X86_EDI,
        REG_OFFSET (rt));

    x86_mov_membase_reg (b->jit_ptr, X86_EDI, REG_OFFSET (rd), X86_EAX, 4);
    return (0);

}

static int mips_emit_subu (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);

    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);
    x86_alu_reg_membase (b->jit_ptr, X86_SUB, X86_EAX, X86_EDI,
        REG_OFFSET (rt));

    x86_mov_membase_reg (b->jit_ptr, X86_EDI, REG_OFFSET (rd), X86_EAX, 4);
    return (0);

}

static int mips_emit_sw (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    mips_emit_memop (b, MIPS_MEMOP_SW, base, offset, rt, TRUE);
    return (0);
}

static int mips_emit_swc1 (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
#if SOFT_FPU
    /* Save PC for exception handling */
    mips_set_pc (b, b->start_pc + ((b->mips_trans_pos - 1) << 2));
    x86_mov_reg_reg (b->jit_ptr, X86_EAX, X86_EDI, 4);
    mips_emit_basic_c_call (b, mips_exec_soft_fpu);
    mips_jit_tcb_push_epilog (b);
    return (0);
#else
    mips_emit_unknown (cpu, b, insn);
    return (0);
#endif

}

static int mips_emit_swc2 (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_swl (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    mips_emit_memop (b, MIPS_MEMOP_SWL, base, offset, rt, TRUE);
    return (0);
}

static int mips_emit_swr (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    mips_emit_memop (b, MIPS_MEMOP_SWR, base, offset, rt, TRUE);
    return (0);
}

static int mips_emit_sync (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    return (0);
}

static int mips_emit_syscall (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_set_pc (b, b->start_pc + ((b->mips_trans_pos - 1) << 2));

    x86_mov_reg_reg (b->jit_ptr, X86_EAX, X86_EDI, 4);
    mips_emit_basic_c_call (b, mips_exec_syscall);

    mips_jit_tcb_push_epilog (b);
    return (0);

}

static int mips_emit_teq (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    u_char *test1;

    /* Compare low part */
    x86_mov_reg_membase (b->jit_ptr, X86_ECX, X86_EDI, REG_OFFSET (rs), 4);
    x86_alu_reg_membase (b->jit_ptr, X86_CMP, X86_ECX, X86_EDI,
        REG_OFFSET (rt));
    test1 = b->jit_ptr;
    x86_branch32 (b->jit_ptr, X86_CC_NE, 0, 1);

    /* Generate trap exception */
    x86_alu_reg_imm (b->jit_ptr, X86_SUB, X86_ESP, 12);
    x86_mov_reg_reg (b->jit_ptr, X86_EAX, X86_EDI, 4);
    mips_emit_c_call (b, mips_trigger_trap_exception);
    x86_alu_reg_imm (b->jit_ptr, X86_ADD, X86_ESP, 12);

    mips_jit_tcb_push_epilog (b);

    /* end */
    x86_patch (test1, b->jit_ptr);
    return (0);

}

static int mips_emit_teqi (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int imm = bits (insn, 0, 15);
    m_reg_t val = sign_extend (imm, 16);
    u_char *test1;

    /* edx:eax = val */
    x86_mov_reg_imm (b->jit_ptr, X86_EAX, val);

    /* Compare low part */
    x86_mov_reg_membase (b->jit_ptr, X86_ECX, X86_EDI, REG_OFFSET (rs), 4);
    x86_alu_reg_reg (b->jit_ptr, X86_CMP, X86_ECX, X86_EAX);
    test1 = b->jit_ptr;
    x86_branch32 (b->jit_ptr, X86_CC_NE, 0, 1);

    /* Generate trap exception */
    x86_alu_reg_imm (b->jit_ptr, X86_SUB, X86_ESP, 12);
    x86_mov_reg_reg (b->jit_ptr, X86_EAX, X86_EDI, 4);
    mips_emit_c_call (b, mips_trigger_trap_exception);
    x86_alu_reg_imm (b->jit_ptr, X86_ADD, X86_ESP, 12);

    mips_jit_tcb_push_epilog (b);

    /* end */
    x86_patch (test1, b->jit_ptr);
    return (0);

}

static int mips_emit_tlb (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    uint16_t func = bits (insn, 0, 5);
    return mips_tlb_jit[func].emit_func (cpu, b, insn);
}

static int mips_emit_tlbp (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_set_pc (b, b->start_pc + ((b->mips_trans_pos - 1) << 2));
    x86_mov_reg_reg (b->jit_ptr, X86_EAX, X86_EDI, 4);
    mips_emit_basic_c_call (b, mips_cp0_exec_tlbp);
    return (0);

}

static int mips_emit_tlbr (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_set_pc (b, b->start_pc + ((b->mips_trans_pos - 1) << 2));
    x86_mov_reg_reg (b->jit_ptr, X86_EAX, X86_EDI, 4);
    mips_emit_basic_c_call (b, mips_cp0_exec_tlbr);
    return (0);

}

static int mips_emit_tlbwi (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_set_pc (b, b->start_pc + ((b->mips_trans_pos - 1) << 2));
    x86_mov_reg_reg (b->jit_ptr, X86_EAX, X86_EDI, 4);
    mips_emit_basic_c_call (b, mips_cp0_exec_tlbwi);
    return (0);

}

static int mips_emit_tlbwr (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_set_pc (b, b->start_pc + ((b->mips_trans_pos - 1) << 2));
    x86_mov_reg_reg (b->jit_ptr, X86_EAX, X86_EDI, 4);
    mips_emit_basic_c_call (b, mips_cp0_exec_tlbwr);
    return (0);

}

static int mips_emit_tge (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_tgei (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_tgeu (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_tgeiu (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_tlt (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_tlti (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_tltiu (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_tltu (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int fastcall tne_emu (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);

    if ((m_ireg_t) cpu->gpr[rs] != (m_ireg_t) cpu->gpr[rt]) {
        /*take a trap */
        mips_trigger_trap_exception (cpu);
        return (1);
    } else
        return (0);

}

static int mips_emit_tne (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    x86_mov_reg_imm (b->jit_ptr, X86_EDX, insn);
    x86_mov_reg_reg (b->jit_ptr, X86_EAX, X86_EDI, 4);
    mips_emit_basic_c_call (b, tne_emu);
    return (0);

}

static int mips_emit_tnei (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_wait (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    return (0);
}

static int mips_emit_xor (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);

    x86_mov_reg_membase (b->jit_ptr, X86_EAX, X86_EDI, REG_OFFSET (rs), 4);

    x86_alu_reg_membase (b->jit_ptr, X86_XOR, X86_EAX, X86_EDI,
        REG_OFFSET (rt));

    x86_mov_membase_reg (b->jit_ptr, X86_EDI, REG_OFFSET (rd), X86_EAX, 4);

    return (0);

}

static int mips_emit_xori (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int imm = bits (insn, 0, 15);

    x86_mov_reg_imm (b->jit_ptr, X86_EAX, imm);
    x86_alu_reg_membase (b->jit_ptr, X86_XOR, X86_EAX, X86_EDI,
        REG_OFFSET (rs));

    x86_mov_membase_reg (b->jit_ptr, X86_EDI, REG_OFFSET (rt), X86_EAX, 4);

    return (0);

}

static int mips_emit_undef (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_unknownBcond (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_unknowncop0 (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_unknownmad (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_unknownSpec (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

static int mips_emit_unknowntlb (cpu_mips_t * cpu, mips_jit_tcb_t * b,
    unsigned int insn)
{
    mips_emit_unknown (cpu, b, insn);
    return (0);
}

/*instruction table*/

struct mips_jit_desc mips_jit[] = {
    {"spec", mips_emit_spec, 0x00, 0x99},
    {"bcond", mips_emit_bcond, 0x01, 0x99},
    {"j", mips_emit_j, 0x02, 0x0},
    {"jal", mips_emit_jal, 0x03, 0x0},
    {"beq", mips_emit_beq, 0x04, 0x0},
    {"bne", mips_emit_bne, 0x05, 0x0},
    {"blez", mips_emit_blez, 0x06, 0x0},
    {"bgtz", mips_emit_bgtz, 0x07, 0x0},
    {"addi", mips_emit_addi, 0x08, 0x1},
    {"addiu", mips_emit_addiu, 0x09, 0x1},
    {"slti", mips_emit_slti, 0x0A, 0x1},
    {"sltiu", mips_emit_sltiu, 0x0B, 0x1},
    {"andi", mips_emit_andi, 0x0C, 0x1},
    {"ori", mips_emit_ori, 0x0D, 0x1},
    {"xori", mips_emit_xori, 0x0E, 0x1},
    {"lui", mips_emit_lui, 0x0F, 0x1},
    {"cop0", mips_emit_cop0, 0x10, 0x99},
    {"cop1", mips_emit_cop1, 0x11, 0x1},
    {"cop2", mips_emit_cop2, 0x12, 0x1},
    {"cop1x", mips_emit_cop1x, 0x13, 0x1},
    {"beql", mips_emit_beql, 0x14, 0x0},
    {"bnel", mips_emit_bnel, 0x15, 0x0},
    {"blezl", mips_emit_blezl, 0x16, 0x0},
    {"bgtzl", mips_emit_bgtzl, 0x17, 0x0},
    {"daddi", mips_emit_daddi, 0x18, 0x1},
    {"daddiu", mips_emit_daddiu, 0x19, 0x1},
    {"ldl", mips_emit_ldl, 0x1A, 0x1},
    {"ldr", mips_emit_ldr, 0x1B, 0x1},
    {"undef", mips_emit_mad, 0x1C, 0x99},
    {"undef", mips_emit_undef, 0x1D, 0x1},
    {"undef", mips_emit_undef, 0x1E, 0x1},
    {"undef", mips_emit_undef, 0x1F, 0x1},
    {"lb", mips_emit_lb, 0x20, 0x1},
    {"lh", mips_emit_lh, 0x21, 0x1},
    {"lwl", mips_emit_lwl, 0x22, 0x1},
    {"lw", mips_emit_lw, 0x23, 0x1},
    {"lbu", mips_emit_lbu, 0x24, 0x1},
    {"lhu", mips_emit_lhu, 0x25, 0x1},
    {"lwr", mips_emit_lwr, 0x26, 0x1},
    {"lwu", mips_emit_lwu, 0x27, 0x1},
    {"sb", mips_emit_sb, 0x28, 0x1},
    {"sh", mips_emit_sh, 0x29, 0x1},
    {"swl", mips_emit_swl, 0x2A, 0x1},
    {"sw", mips_emit_sw, 0x2B, 0x1},
    {"sdl", mips_emit_sdl, 0x2C, 0x1},
    {"sdr", mips_emit_sdr, 0x2D, 0x1},
    {"swr", mips_emit_swr, 0x2E, 0x1},
    {"cache", mips_emit_cache, 0x2F, 0x1},
    {"ll", mips_emit_ll, 0x30, 0x1},
    {"lwc1", mips_emit_lwc1, 0x31, 0x1},
    {"lwc2", mips_emit_lwc2, 0x32, 0x1},
    {"pref", mips_emit_pref, 0x33, 0x1},
    {"lld", mips_emit_lld, 0x34, 0x1},
    {"ldc1", mips_emit_ldc1, 0x35, 0x1},
    {"ldc2", mips_emit_ldc2, 0x36, 0x1},
    {"ld", mips_emit_ld, 0x37, 0x1},
    {"sc", mips_emit_sc, 0x38, 0x1},
    {"swc1", mips_emit_swc1, 0x39, 0x1},
    {"swc2", mips_emit_swc2, 0x3A, 0x1},
    {"undef", mips_emit_undef, 0x3B, 0x1},
    {"scd", mips_emit_scd, 0x3C, 0x1},
    {"sdc1", mips_emit_sdc1, 0x3D, 0x1},
    {"sdc2", mips_emit_sdc2, 0x3E, 0x1},
    {"sd", mips_emit_sd, 0x3F, 0x1},
};

/* Based on the func field of spec opcode */
static struct mips_jit_desc mips_spec_jit[] = {
    {"sll", mips_emit_sll, 0x00, 0x1},
    {"movc", mips_emit_movc, 0x01, 0x1},
    {"srl", mips_emit_srl, 0x02, 0x1},
    {"sra", mips_emit_sra, 0x03, 0x1},
    {"sllv", mips_emit_sllv, 0x04, 0x1},
    {"unknownSpec", mips_emit_unknownSpec, 0x05, 0x1},
    {"srlv", mips_emit_srlv, 0x06, 0x1},
    {"srav", mips_emit_srav, 0x07, 0x1},
    {"jr", mips_emit_jr, 0x08, 0x0},
    {"jalr", mips_emit_jalr, 0x09, 0x0},
    {"movz", mips_emit_movz, 0x0A, 0x1},
    {"movn", mips_emit_movn, 0x0B, 0x1},
    {"syscall", mips_emit_syscall, 0x0C, 0x1},
    {"break", mips_emit_break, 0x0D, 0x1},
    {"spim", mips_emit_unknownSpec, 0x0E, 0x1},
    {"sync", mips_emit_sync, 0x0F, 0x1},
    {"mfhi", mips_emit_mfhi, 0x10, 0x1},
    {"mthi", mips_emit_mthi, 0x11, 0x1},
    {"mflo", mips_emit_mflo, 0x12, 0x1},
    {"mtlo", mips_emit_mtlo, 0x13, 0x1},
    {"dsllv", mips_emit_dsllv, 0x14, 0x1},
    {"unknownSpec", mips_emit_unknownSpec, 0x15, 0x1},
    {"dsrlv", mips_emit_dsrlv, 0x16, 0x1},
    {"dsrav", mips_emit_dsrav, 0x17, 0x1},
    {"mult", mips_emit_mult, 0x18, 0x1},
    {"multu", mips_emit_multu, 0x19, 0x1},
    {"div", mips_emit_div, 0x1A, 0x1},
    {"divu", mips_emit_divu, 0x1B, 0x1},
    {"dmult", mips_emit_dmult, 0x1C, 0x1},
    {"dmultu", mips_emit_dmultu, 0x1D, 0x1},
    {"ddiv", mips_emit_ddiv, 0x1E, 0x1},
    {"ddivu", mips_emit_ddivu, 0x1F, 0x1},
    {"add", mips_emit_add, 0x20, 0x1},
    {"addu", mips_emit_addu, 0x21, 0x1},
    {"sub", mips_emit_sub, 0x22, 0x1},
    {"subu", mips_emit_subu, 0x23, 0x1},
    {"and", mips_emit_and, 0x24, 0x1},
    {"or", mips_emit_or, 0x25, 0x1},
    {"xor", mips_emit_xor, 0x26, 0x1},
    {"nor", mips_emit_nor, 0x27, 0x1},
    {"unknownSpec", mips_emit_unknownSpec, 0x28, 0x1},
    {"unknownSpec", mips_emit_unknownSpec, 0x29, 0x1},
    {"slt", mips_emit_slt, 0x2A, 0x1},
    {"sltu", mips_emit_sltu, 0x2B, 0x1},
    {"dadd", mips_emit_dadd, 0x2C, 0x1},
    {"daddu", mips_emit_daddu, 0x2D, 0x1},
    {"dsub", mips_emit_dsub, 0x2E, 0x1},
    {"dsubu", mips_emit_dsubu, 0x2F, 0x1},
    {"tge", mips_emit_tge, 0x30, 0x1},
    {"tgeu", mips_emit_tgeu, 0x31, 0x1},
    {"tlt", mips_emit_tlt, 0x32, 0x1},
    {"tltu", mips_emit_tltu, 0x33, 0x1},
    {"teq", mips_emit_teq, 0x34, 0x1},
    {"unknownSpec", mips_emit_unknownSpec, 0x35, 0x1},
    {"tne", mips_emit_tne, 0x36, 0x1},
    {"unknownSpec", mips_emit_unknownSpec, 0x37, 0x1},
    {"dsll", mips_emit_dsll, 0x38, 0x1},
    {"unknownSpec", mips_emit_unknownSpec, 0x39, 0x1},
    {"dsrl", mips_emit_dsrl, 0x3A, 0x1},
    {"dsra", mips_emit_dsra, 0x3B, 0x1},
    {"dsll32", mips_emit_dsll32, 0x3C, 0x1},
    {"unknownSpec", mips_emit_unknownSpec, 0x3D, 0x1},
    {"dsrl32", mips_emit_dsrl32, 0x3E, 0x1},
    {"dsra32", mips_emit_dsra32, 0x3F, 0x1}
};

/* Based on the rt field of bcond opcodes */
static struct mips_jit_desc mips_bcond_jit[] = {
    {"bltz", mips_emit_bltz, 0x00, 0x0},
    {"bgez", mips_emit_bgez, 0x01, 0x0},
    {"bltzl", mips_emit_bltzl, 0x02, 0x0},
    {"bgezl", mips_emit_bgezl, 0x03, 0x0},
    {"spimi", mips_emit_unknownBcond, 0x04, 0x0},
    {"unknownBcond", mips_emit_unknownBcond, 0x05, 0x1},
    {"unknownBcond", mips_emit_unknownBcond, 0x06, 0x1},
    {"unknownBcond", mips_emit_unknownBcond, 0x07, 0x1},
    {"tgei", mips_emit_tgei, 0x08, 0x1},
    {"tgeiu", mips_emit_tgeiu, 0x09, 0x1},
    {"tlti", mips_emit_tlti, 0x0A, 0x1},
    {"tltiu", mips_emit_tltiu, 0x0B, 0x1},
    {"teqi", mips_emit_teqi, 0x0C, 0x1},
    {"unknownBcond", mips_emit_unknownBcond, 0x0D, 0x1},
    {"tnei", mips_emit_tnei, 0x0E, 0x1},
    {"unknownBcond", mips_emit_unknownBcond, 0x0F, 0x1},
    {"bltzal", mips_emit_bltzal, 0x10, 0x0},
    {"bgezal", mips_emit_bgezal, 0x11, 0x0},
    {"bltzall", mips_emit_bltzall, 0x12, 0x0},
    {"bgezall", mips_emit_bgezall, 0x13, 0x0},
    {"unknownBcond", mips_emit_unknownBcond, 0x14, 0x1},
    {"unknownBcond", mips_emit_unknownBcond, 0x15, 0x1},
    {"unknownBcond", mips_emit_unknownBcond, 0x16, 0x1},
    {"unknownBcond", mips_emit_unknownBcond, 0x17, 0x1},
    {"unknownBcond", mips_emit_unknownBcond, 0x18, 0x1},
    {"unknownBcond", mips_emit_unknownBcond, 0x19, 0x1},
    {"unknownBcond", mips_emit_unknownBcond, 0x1A, 0x1},
    {"unknownBcond", mips_emit_unknownBcond, 0x1B, 0x1},
    {"unknownBcond", mips_emit_unknownBcond, 0x1C, 0x1},
    {"unknownBcond", mips_emit_unknownBcond, 0x1D, 0x1},
    {"unknownBcond", mips_emit_unknownBcond, 0x1E, 0x1},
    {"unknownBcond", mips_emit_unknownBcond, 0x1F, 0x1}
};

static struct mips_jit_desc mips_cop0_jit[] = {
    {"mfc0", mips_emit_mfc0, 0x0, 0x1},
    {"dmfc0", mips_emit_dmfc0, 0x1, 0x1},
    {"cfc0", mips_emit_cfc0, 0x2, 0x1},
    {"unknowncop0", mips_emit_unknowncop0, 0x3, 0x1},
    {"mtc0", mips_emit_mtc0, 0x4, 0x1},
    {"dmtc0", mips_emit_dmtc0, 0x5, 0x1},
    {"unknowncop0", mips_emit_unknowncop0, 0x6, 0x1},
    {"unknowncop0", mips_emit_unknowncop0, 0x7, 0x1},
    {"unknowncop0", mips_emit_unknowncop0, 0x8, 0x1},
    {"unknowncop0", mips_emit_unknowncop0, 0x9, 0x1},
    {"unknowncop0", mips_emit_unknowncop0, 0xa, 0x1},
    {"unknowncop0", mips_emit_unknowncop0, 0xb, 0x1},
    {"unknowncop0", mips_emit_unknowncop0, 0xc, 0x1},
    {"unknowncop0", mips_emit_unknowncop0, 0xd, 0x1},
    {"unknowncop0", mips_emit_unknowncop0, 0xe, 0x1},
    {"unknowncop0", mips_emit_unknowncop0, 0xf, 0x1},
    {"tlb", mips_emit_tlb, 0x10, 0x1},
    {"unknowncop0", mips_emit_unknowncop0, 0x11, 0x1},
    {"unknowncop0", mips_emit_unknowncop0, 0x12, 0x1},
    {"unknowncop0", mips_emit_unknowncop0, 0x13, 0x1},
    {"unknowncop0", mips_emit_unknowncop0, 0x14, 0x1},
    {"unknowncop0", mips_emit_unknowncop0, 0x15, 0x1},
    {"unknowncop0", mips_emit_unknowncop0, 0x16, 0x1},
    {"unknowncop0", mips_emit_unknowncop0, 0x17, 0x1},
    {"unknowncop0", mips_emit_unknowncop0, 0x18, 0x1},
    {"unknowncop0", mips_emit_unknowncop0, 0x19, 0x1},
    {"unknowncop0", mips_emit_unknowncop0, 0x1a, 0x1},
    {"unknowncop0", mips_emit_unknowncop0, 0x1b, 0x1},
    {"unknowncop0", mips_emit_unknowncop0, 0x1c, 0x1},
    {"unknowncop0", mips_emit_unknowncop0, 0x1d, 0x1},
    {"unknowncop0", mips_emit_unknowncop0, 0x1e, 0x1},
    {"unknowncop0", mips_emit_unknowncop0, 0x1f, 0x1},

};

static struct mips_jit_desc mips_mad_jit[] = {
    {"mad", mips_emit_madd, 0x0, 0x1},
    {"maddu", mips_emit_maddu, 0x1, 0x1},
    {"mul", mips_emit_mul, 0x2, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x3, 0x1},
    {"msub", mips_emit_msub, 0x4, 0x1},
    {"msubu", mips_emit_msubu, 0x5, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x6, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x7, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x8, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x9, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0xa, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0xb, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0xc, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0xd, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0xe, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0xf, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x10, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x11, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x12, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x13, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x14, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x15, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x16, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x17, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x18, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x19, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x1a, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x1b, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x1c, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x1d, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x1e, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x1f, 0x1},
    {"clz", mips_emit_clz, 0x20, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x21, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x22, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x23, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x24, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x25, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x26, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x27, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x28, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x29, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x2a, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x2b, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x2c, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x2d, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x2e, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x2f, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x30, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x31, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x32, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x33, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x34, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x35, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x36, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x37, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x38, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x39, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x3a, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x3b, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x3c, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x3d, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x3e, 0x1},
    {"unknownmad_op", mips_emit_unknownmad, 0x3f, 0x1},

};

static struct mips_jit_desc mips_tlb_jit[] = {
    {"unknowntlb_op", mips_emit_unknowntlb, 0x0, 0x1},
    {"tlbr", mips_emit_tlbr, 0x1, 0x1},
    {"tlbwi", mips_emit_tlbwi, 0x2, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x3, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x4, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x5, 0x1},
    {"tlbwi", mips_emit_tlbwr, 0x6, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x7, 0x1},
    {"tlbp", mips_emit_tlbp, 0x8, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x9, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0xa, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0xb, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0xc, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0xd, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0xe, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0xf, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x10, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x11, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x12, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x13, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x14, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x15, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x16, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x17, 0x1},
    {"eret", mips_emit_eret, 0x18, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x19, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x1a, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x1b, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x1c, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x1d, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x1e, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x1f, 0x1},
    {"wait", mips_emit_wait, 0x20, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x21, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x22, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x23, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x24, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x25, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x26, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x27, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x28, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x29, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x2a, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x2b, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x2c, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x2d, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x2e, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x2f, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x30, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x31, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x32, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x33, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x34, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x35, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x36, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x37, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x38, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x39, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x3a, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x3b, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x3c, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x3d, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x3e, 0x1},
    {"unknowntlb_op", mips_emit_unknowntlb, 0x3f, 0x1},

};
#endif
