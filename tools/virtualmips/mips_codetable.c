/*
 * Code dispatch table.
 *
 * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
 *
 * This file is part of the virtualmips distribution.
 * See LICENSE file for terms of the license.
 */

/*
 * Take the 'reserved instruction' exception.
 */
static int unknown_op (cpu_mips_t * cpu, mips_insn_t insn)
{
#if 0
    printf ("--- Unknown instruction:\n");
    printf ("%08x:       %08x        ", cpu->pc, insn);
    print_mips (cpu->pc, insn, cpu->insn_len, cpu->is_mips16e, stdout);
    printf ("\n");
#endif
    mips_trigger_exception (cpu, MIPS_CP0_CAUSE_ILLOP, cpu->is_in_bdslot);
    return 1;
}

/*
 * Take the 'coprocessor unusable' exception.
 */
static int cop_unusable(cpu_mips_t * cpu, int cop_index)
{
    if (cpu->cp0.reg[MIPS_CP0_DEBUG] & MIPS_CP0_DEBUG_DM) {
        /* Coprocessor unusable in Debug mode. */
        mips_trigger_debug_exception (cpu, 0);
        cpu->cp0.reg[MIPS_CP0_DEBUG] |=
            MIPS_CP0_CAUSE_CP_UNUSABLE << MIPS_CP0_DEBUG_DEXCCODE_SHIFT;
    } else {
        /* Set Cause.CE field. */
        cpu->cp0.reg[MIPS_CP0_CAUSE] &= ~MIPS_CP0_CAUSE_CEMASK;
        cpu->cp0.reg[MIPS_CP0_CAUSE] |= cop_index << MIPS_CP0_CAUSE_CESHIFT;

        mips_trigger_exception (cpu, MIPS_CP0_CAUSE_CP_UNUSABLE, cpu->is_in_bdslot);
    }
    return 1;
}

static int add_op (cpu_mips_t *cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    m_ireg_t a = cpu->gpr[rs];
    m_ireg_t b = cpu->gpr[rt];
    m_ireg_t res = a + b;

    if ((a > 0 && b > 0 && res < 0) ||
        (a < 0 && b < 0 && res >= 0)) {
        /* Take overflow exception. */
        mips_trigger_exception (cpu, MIPS_CP0_CAUSE_OVFLW, cpu->is_in_bdslot);
        return 1;
    }
    cpu->reg_set (cpu, rd, sign_extend (res, 32));
    return (0);
}

static int addi_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int imm = bits (insn, 0, 15);
    m_ireg_t a = cpu->gpr[rs];
    m_ireg_t b = sign_extend (imm, 16);
    m_ireg_t res = a + b;

    if ((a > 0 && b > 0 && res < 0) ||
        (a < 0 && b < 0 && res >= 0)) {
        /* Take overflow exception. */
        mips_trigger_exception (cpu, MIPS_CP0_CAUSE_OVFLW, cpu->is_in_bdslot);
        return 1;
    }
    cpu->reg_set (cpu, rt, sign_extend (res, 32));
    return (0);
}

static int addiu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int imm = bits (insn, 0, 15);
    m_uint32_t res, val = sign_extend (imm, 16);

    res = (m_uint32_t) cpu->gpr[rs] + val;
    cpu->reg_set (cpu, rt, sign_extend (res, 32));
    return (0);
}

static int addu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    m_uint32_t res;

    res = (m_uint32_t) cpu->gpr[rs] + (m_uint32_t) cpu->gpr[rt];
    cpu->reg_set (cpu, rd, sign_extend (res, 32));
    return (0);
}

static int and_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);

    cpu->reg_set (cpu, rd, cpu->gpr[rs] & cpu->gpr[rt]);
    return (0);
}

static int andi_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int imm = bits (insn, 0, 15);

    cpu->reg_set (cpu, rt, cpu->gpr[rs] & imm);
    return (0);
}

static int bcond_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    uint16_t special_func = bits (insn, 16, 20);
    return mips_bcond_opcodes[special_func].func (cpu, insn);
}

static int beq_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* take the branch if gpr[rs] == gpr[rt] */
    res = (cpu->gpr[rs] == cpu->gpr[rt]);

    /* exec the instruction in the delay slot */
    int ins_res = mips_exec_bdslot (cpu);
    if (likely (!ins_res)) {
        if (res)
            cpu->pc = new_pc;
        else
            cpu->pc += 8;
    }

    return (1);
}

static int beql_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* take the branch if gpr[rs] == gpr[rt] */
    res = (cpu->gpr[rs] == cpu->gpr[rt]);

    /* take the branch if the test result is true */
    if (res) {
        int ins_res = mips_exec_bdslot (cpu);
        if (likely (!ins_res))
            cpu->pc = new_pc;
    } else
        cpu->pc += 8;

    return (1);
}

static int bgez_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* take the branch if gpr[rs] >= 0 */
    res = ((m_ireg_t) cpu->gpr[rs] >= 0);

    /* exec the instruction in the delay slot */
    /* exec the instruction in the delay slot */
    int ins_res = mips_exec_bdslot (cpu);

    if (likely (!ins_res)) {
        /* take the branch if the test result is true */
        if (res)
            cpu->pc = new_pc;
        else
            cpu->pc += 8;
    }
    return (1);
}

static int bgezal_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* set the return address (instruction after the delay slot) */
    cpu->reg_set (cpu, MIPS_GPR_RA, cpu->pc + 8);

    /* take the branch if gpr[rs] >= 0 */
    res = ((m_ireg_t) cpu->gpr[rs] >= 0);

    /* exec the instruction in the delay slot */
    int ins_res = mips_exec_bdslot (cpu);

    if (likely (!ins_res)) {
        /* take the branch if the test result is true */
        if (res)
            cpu->pc = new_pc;
        else
            cpu->pc += 8;
    }
    return (1);
}

static int bgezall_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* set the return address (instruction after the delay slot) */
    cpu->reg_set (cpu, MIPS_GPR_RA, cpu->pc + 8);

    /* take the branch if gpr[rs] >= 0 */
    res = ((m_ireg_t) cpu->gpr[rs] >= 0);

    /* take the branch if the test result is true */
    if (res) {
        mips_exec_bdslot (cpu);
        cpu->pc = new_pc;
    } else
        cpu->pc += 8;

    return (1);

}

static int bgezl_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* take the branch if gpr[rs] >= 0 */
    res = ((m_ireg_t) cpu->gpr[rs] >= 0);

    /* take the branch if the test result is true */
    if (res) {
        mips_exec_bdslot (cpu);
        cpu->pc = new_pc;
    } else
        cpu->pc += 8;

    return (1);

}

static int bgtz_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* take the branch if gpr[rs] > 0 */
    res = ((m_ireg_t) cpu->gpr[rs] > 0);

    /* exec the instruction in the delay slot */
    int ins_res = mips_exec_bdslot (cpu);

    if (likely (!ins_res)) {
        /* take the branch if the test result is true */
        if (res)
            cpu->pc = new_pc;
        else
            cpu->pc += 8;
    }
    return (1);
}

static int bgtzl_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* take the branch if gpr[rs] > 0 */
    res = ((m_ireg_t) cpu->gpr[rs] > 0);

    /* take the branch if the test result is true */
    if (res) {
        mips_exec_bdslot (cpu);
        cpu->pc = new_pc;
    } else
        cpu->pc += 8;

    return (1);
}

static int blez_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* take the branch if gpr[rs] <= 0 */
    res = ((m_ireg_t) cpu->gpr[rs] <= 0);

    /* exec the instruction in the delay slot */
    int ins_res = mips_exec_bdslot (cpu);

    if (likely (!ins_res)) {
        /* take the branch if the test result is true */
        if (res)
            cpu->pc = new_pc;
        else
            cpu->pc += 8;
    }
    return (1);
}

static int blezl_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* take the branch if gpr[rs] <= 0 */
    res = ((m_ireg_t) cpu->gpr[rs] <= 0);

    /* take the branch if the test result is true */
    if (res) {
        mips_exec_bdslot (cpu);
        cpu->pc = new_pc;
    } else
        cpu->pc += 8;

    return (1);
}

static int bltz_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* take the branch if gpr[rs] < 0 */
    res = ((m_ireg_t) cpu->gpr[rs] < 0);

    /* exec the instruction in the delay slot */
    int ins_res = mips_exec_bdslot (cpu);

    if (likely (!ins_res)) {
        /* take the branch if the test result is true */
        if (res)
            cpu->pc = new_pc;
        else
            cpu->pc += 8;
    }
    return (1);
}

static int bltzal_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* set the return address (instruction after the delay slot) */
    cpu->reg_set (cpu, MIPS_GPR_RA, cpu->pc + 8);

    /* take the branch if gpr[rs] < 0 */
    res = ((m_ireg_t) cpu->gpr[rs] < 0);

    /* exec the instruction in the delay slot */
    int ins_res = mips_exec_bdslot (cpu);

    if (likely (!ins_res)) {
        /* take the branch if the test result is true */
        if (res)
            cpu->pc = new_pc;
        else
            cpu->pc += 8;
    }
    return (1);
}

static int bltzall_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* set the return address (instruction after the delay slot) */
    cpu->reg_set (cpu, MIPS_GPR_RA, cpu->pc + 8);

    /* take the branch if gpr[rs] < 0 */
    res = ((m_ireg_t) cpu->gpr[rs] < 0);

    /* take the branch if the test result is true */
    if (res) {
        mips_exec_bdslot (cpu);
        cpu->pc = new_pc;
    } else
        cpu->pc += 8;

    return (1);
}

static int bltzl_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* take the branch if gpr[rs] < 0 */
    res = ((m_ireg_t) cpu->gpr[rs] < 0);

    /* take the branch if the test result is true */
    if (res) {
        mips_exec_bdslot (cpu);
        cpu->pc = new_pc;
    } else
        cpu->pc += 8;

    return (1);
}

static int bne_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* take the branch if gpr[rs] != gpr[rt] */
    res = (cpu->gpr[rs] != cpu->gpr[rt]);

    /* exec the instruction in the delay slot */
    int ins_res = mips_exec_bdslot (cpu);

    if (likely (!ins_res)) {
        /* take the branch if the test result is true */
        if (res)
            cpu->pc = new_pc;
        else
            cpu->pc += 8;
    }

    return (1);
}

static int bnel_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* take the branch if gpr[rs] != gpr[rt] */
    res = (cpu->gpr[rs] != cpu->gpr[rt]);

    /* take the branch if the test result is true */
    if (res) {
        mips_exec_bdslot (cpu);
        cpu->pc = new_pc;
    } else
        cpu->pc += 8;
    return (1);
}

static int break_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    u_int code = bits (insn, 6, 25);

    mips_exec_break (cpu, code);
    return (1);
}

static int cache_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int op = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    return (mips_exec_memop2 (cpu, MIPS_MEMOP_CACHE, base, offset, op,
            FALSE));
}

static int cfc0_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int clz_op (cpu_mips_t * cpu, mips_insn_t insn)
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
    cpu->reg_set (cpu, rd, val);
    return (0);
}

static int clo_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rd = bits (insn, 11, 15);
    int i;
    m_uint32_t val;
    val = 32;
    for (i = 31; i >= 0; i--) {
        if (! (cpu->gpr[rs] & (1 << i))) {
            val = 31 - i;
            break;
        }
    }
    cpu->reg_set (cpu, rd, val);
    return (0);
}

static int cop0_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    uint16_t special_func = bits (insn, 21, 25);
//printf ("cop0 instruction. func %x\n", special_func);
    return mips_cop0_opcodes[special_func].func (cpu, insn);
}

static int cop1_op (cpu_mips_t * cpu, mips_insn_t insn)
{
#if SOFT_FPU
    mips_exec_soft_fpu (cpu);
    return (1);
#else
    if (! (cpu->cp0.reg [MIPS_CP0_STATUS] & MIPS_CP0_STATUS_CU1)) {
        return cop_unusable (cpu, 1);
    }
    return unknown_op (cpu, insn);
#endif
}

static int cop2_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    if (! (cpu->cp0.reg [MIPS_CP0_STATUS] & MIPS_CP0_STATUS_CU2)) {
        return cop_unusable (cpu, 2);
    }
    return unknown_op (cpu, insn);
}

static int div_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);

    if (cpu->gpr[rt] == 0)
        return (0);

    if (cpu->gpr[rs] == 0x80000000 && cpu->gpr[rt] == 0xFFFFFFFF) {
        cpu->lo = 0x80000000;
        cpu->hi = 0;
        return (0);
    }

    cpu->lo = (m_int32_t) cpu->gpr[rs] / (m_int32_t) cpu->gpr[rt];
    cpu->hi = (m_int32_t) cpu->gpr[rs] % (m_int32_t) cpu->gpr[rt];

    cpu->lo = sign_extend (cpu->lo, 32);
    cpu->hi = sign_extend (cpu->hi, 32);
    return (0);
}

static int divu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);

    if (cpu->gpr[rt] == 0)
        return (0);

    cpu->lo = (m_uint32_t) cpu->gpr[rs] / (m_uint32_t) cpu->gpr[rt];
    cpu->hi = (m_uint32_t) cpu->gpr[rs] % (m_uint32_t) cpu->gpr[rt];

    cpu->lo = sign_extend (cpu->lo, 32);
    cpu->hi = sign_extend (cpu->hi, 32);
    return (0);

}

static int eret_op (cpu_mips_t * cpu, mips_insn_t insn)
{
#if 1
    if (cpu->trace_syscall &&
        (cpu->cp0.reg[MIPS_CP0_STATUS] & MIPS_CP0_STATUS_UM))
    {
        if (cpu->gpr[2] == 0xffffffff)
            printf ("    syscall failed, errno %u\n", cpu->gpr[8]);
        else {
            if (cpu->gpr[2] & ~0xffff)
                printf ("    syscall returned %08x\n", cpu->gpr[2]);
            else
                printf ("    syscall returned %u\n", cpu->gpr[2]);
        }
        cpu->trace_syscall = 0;
    }
#endif
    mips_exec_eret (cpu);
    return (1);
}

static int deret_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    mips_exec_deret (cpu);
    return (1);
}

static int j_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    u_int instr_index = bits (insn, 0, 25);
    m_va_t new_pc;

    /* compute the new pc */
    new_pc = cpu->pc & ~((1 << 28) - 1);
    new_pc |= instr_index << 2;

    /* exec the instruction in the delay slot */
    int ins_res = mips_exec_bdslot (cpu);
    if (likely (!ins_res))
        cpu->pc = new_pc;
    return (1);
}

static int jal_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    u_int instr_index = bits (insn, 0, 25);
    m_va_t new_pc;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) & ~((1 << 28) - 1);
    new_pc |= instr_index << 2;

    /* set the return address (instruction after the delay slot) */
    cpu->reg_set (cpu, MIPS_GPR_RA, cpu->pc + 8);

    int ins_res = mips_exec_bdslot (cpu);
    if (likely (!ins_res))
        cpu->pc = new_pc;

    return (1);
}

static int jalx_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    u_int instr_index = bits (insn, 0, 25);
    m_va_t new_pc;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) & ~((1 << 28) - 1);
    new_pc |= instr_index << 2;

    /* set the return address (instruction after the delay slot) */
    cpu->reg_set (cpu, MIPS_GPR_RA, cpu->pc + 8);

    int ins_res = mips_exec_bdslot (cpu);
    if (likely (!ins_res)) {
        cpu->is_mips16e = 1;
        cpu->pc = new_pc;
    }

    return (1);
}

static int jalr_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rd = bits (insn, 11, 15);
    m_va_t new_pc;

    /* set the return pc (instruction after the delay slot) in GPR[rd] */
    cpu->reg_set (cpu, rd, cpu->pc + 8);

    /* get the new pc */
    new_pc = cpu->gpr[rs];

    int ins_res = mips_exec_bdslot (cpu);
    if (likely (!ins_res)) {
        cpu->is_mips16e = new_pc & 1;
        cpu->pc = new_pc & 0xFFFFFFFE;
    }
    return (1);

}

static int jr_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    m_va_t new_pc;

    /* get the new pc */
    new_pc = cpu->gpr[rs];

    int ins_res = mips_exec_bdslot (cpu);
    if (likely (!ins_res)) {
        cpu->is_mips16e = new_pc & 1;
        cpu->pc = new_pc & 0xFFFFFFFE;
    }
    return (1);

}

static int lb_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    return (mips_exec_memop2 (cpu, MIPS_MEMOP_LB, base, offset, rt, TRUE));
}

static int lbu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    return (mips_exec_memop2 (cpu, MIPS_MEMOP_LBU, base, offset, rt, TRUE));
}

static int lh_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    return (mips_exec_memop2 (cpu, MIPS_MEMOP_LH, base, offset, rt, TRUE));
}

static int lhu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    return (mips_exec_memop2 (cpu, MIPS_MEMOP_LHU, base, offset, rt, TRUE));
}

static int ll_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    return (mips_exec_memop2 (cpu, MIPS_MEMOP_LL, base, offset, rt, TRUE));
}

static int lui_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rt = bits (insn, 16, 20);
    int imm = bits (insn, 0, 15);

    cpu->reg_set (cpu, rt, sign_extend (imm, 16) << 16);
    return (0);
}

static int lw_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    return (mips_exec_memop2 (cpu, MIPS_MEMOP_LW, base, offset, rt, TRUE));

}

static int lwl_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    return (mips_exec_memop2 (cpu, MIPS_MEMOP_LWL, base, offset, rt, TRUE));

}

static int lwr_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    return (mips_exec_memop2 (cpu, MIPS_MEMOP_LWR, base, offset, rt, TRUE));
}

static int spec3_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int index = bits (insn, 0, 5);
    return mips_spec3_opcodes[index].func (cpu, insn);
}

static int bshfl_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    int sa = bits (insn, 6, 10);
//printf ("seh rt=%d, rd=%d, sa=%x\n", rt, rd, sa);
    switch (sa) {
    case 0x02:
        /* wsbh - word swap bytes within halfwords */
        cpu->reg_set (cpu, rd, bits (cpu->gpr[rt], 16, 23) << 24 |
                       bits (cpu->gpr[rt], 24, 31) << 16 |
                       bits (cpu->gpr[rt], 0,  7)  << 8 |
                       bits (cpu->gpr[rt], 8, 15));
        return (0);
    case 0x10:
        /* seb - sign extend byte */
        cpu->reg_set (cpu, rd, sign_extend (cpu->gpr[rt], 8));
        return (0);
    case 0x18:
        /* seh - sign extend halfword */
        cpu->reg_set (cpu, rd, sign_extend (cpu->gpr[rt], 16));
        return (0);
    }
    return unknown_op (cpu, insn);
}

static int ext_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int msbd = bits (insn, 11, 15);
    int lsb = bits (insn, 6, 10);

    /* Extract bit field */
    cpu->reg_set (cpu, rt, (cpu->gpr[rs] >> lsb) & (~0U >> (31 - msbd)));
    return (0);
}

static int ins_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int msb = bits (insn, 11, 15);
    int lsb = bits (insn, 6, 10);

    /* Insert bit field */
    int mask = ~0U >> (31-msb+lsb) << lsb;
    cpu->gpr[rt] &= ~mask;
    cpu->gpr[rt] |= (cpu->gpr[rs] << lsb) & mask;
    return (0);
}

static int spec2_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int index = bits (insn, 0, 5);
    return mips_spec2_opcodes[index].func (cpu, insn);
}

static int madd_op (cpu_mips_t * cpu, mips_insn_t insn)
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

static int maddu_op (cpu_mips_t * cpu, mips_insn_t insn)
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

static int mfc0_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    int sel = bits (insn, 0, 2);
    //mfc rt,rd

    mips_cp0_exec_mfc0 (cpu, rt, rd, sel);
    return (0);
}

static int mfhi_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rd = bits (insn, 11, 15);

    if (rd)
        cpu->reg_set (cpu, rd, cpu->hi);
    return (0);
}

static int mflo_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rd = bits (insn, 11, 15);

    if (rd)
        cpu->reg_set (cpu, rd, cpu->lo);
    return (0);
}

static int movc_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    if (! (cpu->cp0.reg [MIPS_CP0_STATUS] & MIPS_CP0_STATUS_CU1)) {
        return cop_unusable (cpu, 1);
    }
    return unknown_op (cpu, insn);
}

static int movz_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rd = bits (insn, 11, 15);
    int rt = bits (insn, 16, 20);

    if ((cpu->gpr[rt]) == 0)
        cpu->reg_set (cpu, rd, sign_extend (cpu->gpr[rs], 32));
    return (0);
}

static int movn_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rd = bits (insn, 11, 15);
    int rt = bits (insn, 16, 20);

    // printf("pc %x rs %x rd %x rt %x\n",cpu->pc,rs,rd,rt);
    if ((cpu->gpr[rt]) != 0)
        cpu->reg_set (cpu, rd, sign_extend (cpu->gpr[rs], 32));
    return (0);
}

static int msub_op (cpu_mips_t * cpu, mips_insn_t insn)
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

static int msubu_op (cpu_mips_t * cpu, mips_insn_t insn)
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
    //val += temp;

    cpu->lo = sign_extend (temp, 32);
    cpu->hi = sign_extend (temp >> 32, 32);
    return (0);
}

static int mtc0_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    int sel = bits (insn, 0, 2);

    //printf("cpu->pc %x insn %x\n",cpu->pc,insn);
    mips_cp0_exec_mtc0 (cpu, rt, rd, sel);
    return (0);
}

static int mthi_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);

    cpu->hi = cpu->gpr[rs];
    return (0);
}

static int mtlo_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);

    cpu->lo = cpu->gpr[rs];
    return (0);
}

static int mul_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    m_int32_t val;

    /* note: after this instruction, HI/LO regs are undefined */
    val = (m_int32_t) cpu->gpr[rs] * (m_int32_t) cpu->gpr[rt];
    cpu->reg_set (cpu, rd, sign_extend (val, 32));
    return (0);
}

static int mult_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    m_int64_t val;

    val = (m_int64_t) (m_int32_t) cpu->gpr[rs];
    val *= (m_int64_t) (m_int32_t) cpu->gpr[rt];

    cpu->lo = sign_extend (val, 32);
    cpu->hi = sign_extend (val >> 32, 32);
    return (0);
}

static int multu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    m_int64_t val;              //must be 64 bit. not m_reg_t !!!

    val = (m_reg_t) (m_uint32_t) cpu->gpr[rs];
    val *= (m_reg_t) (m_uint32_t) cpu->gpr[rt];
    cpu->lo = sign_extend (val, 32);
    cpu->hi = sign_extend (val >> 32, 32);
    return (0);
}

static int nor_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);

    cpu->reg_set (cpu, rd, ~(cpu->gpr[rs] | cpu->gpr[rt]));
    return (0);
}

static int or_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);

    cpu->reg_set (cpu, rd, cpu->gpr[rs] | cpu->gpr[rt]);
    return (0);
}

static int ori_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int imm = bits (insn, 0, 15);

    cpu->reg_set (cpu, rt, cpu->gpr[rs] | imm);
    return (0);
}

static int pref_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return (0);
}

static int sb_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);
    return (mips_exec_memop2 (cpu, MIPS_MEMOP_SB, base, offset, rt, FALSE));
}

static int sc_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    return (mips_exec_memop2 (cpu, MIPS_MEMOP_SC, base, offset, rt, TRUE));
}

static int sh_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    return (mips_exec_memop2 (cpu, MIPS_MEMOP_SH, base, offset, rt, FALSE));
}

static int sll_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    int sa = bits (insn, 6, 10);
    m_uint32_t res;

    res = (m_uint32_t) cpu->gpr[rt] << sa;
    cpu->reg_set (cpu, rd, sign_extend (res, 32));
    return (0);
}

static int sllv_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    m_uint32_t res;

    res = (m_uint32_t) cpu->gpr[rt] << (cpu->gpr[rs] & 0x1f);
    cpu->reg_set (cpu, rd, sign_extend (res, 32));
    return (0);
}

static int slt_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);

    if ((m_ireg_t) cpu->gpr[rs] < (m_ireg_t) cpu->gpr[rt])
        cpu->reg_set (cpu, rd, 1);
    else
        cpu->reg_set (cpu, rd, 0);
    return (0);
}

static int slti_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int imm = bits (insn, 0, 15);
    m_ireg_t val = sign_extend (imm, 16);

    if ((m_ireg_t) cpu->gpr[rs] < val)
        cpu->reg_set (cpu, rt, 1);
    else
        cpu->reg_set (cpu, rt, 0);
    return (0);
}

static int sltiu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int imm = bits (insn, 0, 15);
    m_reg_t val = sign_extend (imm, 16);

    if (rs == 0 && rt == 0 && cpu->magic_opcodes) {
        if (imm == 0xabc1) {
            printf ("%08x: Test FAIL\n", cpu->pc);
            printf ("\n--- Stop simulation\n");
            fprintf (stderr, "Test FAIL\n");
            exit (EXIT_SUCCESS);
        }
        if (imm == 0xabc2) {
            printf ("%08x: Test PASS\n", cpu->pc);
            printf ("\n--- Stop simulation\n");
            fprintf (stderr, "Test PASS\n");
            exit (EXIT_SUCCESS);
        }
    }

    if (cpu->gpr[rs] < val)
        cpu->reg_set (cpu, rt, 1);
    else
        cpu->reg_set (cpu, rt, 0);
    return (0);
}

static int sltu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);

    if (cpu->gpr[rs] < cpu->gpr[rt])
        cpu->reg_set (cpu, rd, 1);
    else
        cpu->reg_set (cpu, rd, 0);
    return (0);
}

static int spec_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    uint16_t special_func = bits (insn, 0, 5);
    return mips_spec_opcodes[special_func].func (cpu, insn);
}

static int sra_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    int sa = bits (insn, 6, 10);
    m_int32_t res;

    res = (m_int32_t) cpu->gpr[rt] >> sa;
    cpu->reg_set (cpu, rd, sign_extend (res, 32));
    return (0);
}

static int srav_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    m_int32_t res;

    res = (m_int32_t) cpu->gpr[rt] >> (cpu->gpr[rs] & 0x1f);
    cpu->reg_set (cpu, rd, sign_extend (res, 32));
    return (0);
}

static int srl_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    int sa = bits (insn, 6, 10);
    m_uint32_t res;

    /* srl */
    res = (m_uint32_t) cpu->gpr[rt] >> sa;
    if ((insn >> 21) & 1) {
        /* rotr */
        res |= (m_uint32_t) cpu->gpr[rt] << (32 - sa);
    }
    cpu->reg_set (cpu, rd, sign_extend (res, 32));
    return (0);
}

static int srlv_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    m_uint32_t res, sa;

    /* srlv */
    sa = cpu->gpr[rs] & 0x1f;
    res = (m_uint32_t) cpu->gpr[rt] >> sa;
    if ((insn >> 6) & 1) {
        /* rotrv */
        res |= (m_uint32_t) cpu->gpr[rt] << (32 - sa);
    }
    cpu->reg_set (cpu, rd, sign_extend (res, 32));
    return (0);
}

static int sub_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    m_ireg_t a = cpu->gpr[rs];
    m_ireg_t b = cpu->gpr[rt];
    m_ireg_t res = a - b;

    if ((a > 0 && b < 0 && res < 0) ||
        (a < 0 && b > 0 && res >= 0)) {
        /* Take overflow exception. */
        mips_trigger_exception (cpu, MIPS_CP0_CAUSE_OVFLW, cpu->is_in_bdslot);
        return 1;
    }
    cpu->reg_set (cpu, rd, sign_extend (res, 32));
    return (0);
}

static int subu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    m_uint32_t res;
    res = (m_uint32_t) cpu->gpr[rs] - (m_uint32_t) cpu->gpr[rt];
    cpu->reg_set (cpu, rd, sign_extend (res, 32));
    return (0);
}

static int sw_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    return (mips_exec_memop2 (cpu, MIPS_MEMOP_SW, base, offset, rt, FALSE));
}

static int swl_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    return (mips_exec_memop2 (cpu, MIPS_MEMOP_SWL, base, offset, rt,
            FALSE));
}

static int swr_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    return (mips_exec_memop2 (cpu, MIPS_MEMOP_SWR, base, offset, rt,
            FALSE));
}

static int sync_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return (0);
}

static int syscall_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    mips_exec_syscall (cpu);
    return (1);
}

static int teq_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);

    if (unlikely (cpu->gpr[rs] == cpu->gpr[rt])) {
        mips_trigger_trap_exception (cpu);
        return (1);
    }
    return (0);
}

static int teqi_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int imm = bits (insn, 0, 15);
    m_reg_t val = sign_extend (imm, 16);

    if (unlikely (cpu->gpr[rs] == val)) {
        mips_trigger_trap_exception (cpu);
        return (1);
    }
    return (0);
}

/*
 * DI and EI instructions
 */
static int mfmc0_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    int func = bits (insn, 0, 10);

    if (rd == 12) {
        switch (func) {
        case 0x020:
            /* ei - enable interrupts */
            cpu->reg_set (cpu, rt, cpu->cp0.reg [MIPS_CP0_STATUS]);
            cpu->cp0.reg [MIPS_CP0_STATUS] |= MIPS_CP0_STATUS_IE;
            return 0;
        case 0x000:
            /* di - disable interrupts */
            cpu->reg_set (cpu, rt, cpu->cp0.reg [MIPS_CP0_STATUS]);
            cpu->cp0.reg [MIPS_CP0_STATUS] &= ~MIPS_CP0_STATUS_IE;
            return 0;
        }
    }
    return unknown_op (cpu, insn);
}

static int tlb_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    uint16_t func = bits (insn, 0, 5);
    return mips_tlb_opcodes[func].func (cpu, insn);
}

static int tlbp_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    mips_cp0_exec_tlbp (cpu);
    return (0);
}

static int tlbr_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    mips_cp0_exec_tlbr (cpu);
    return (0);
}

static int tlbwi_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    mips_cp0_exec_tlbwi (cpu);
    return (0);
}

static int tlbwr_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    mips_cp0_exec_tlbwr (cpu);
    return (0);
}

static int tge_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);

    if ((m_ireg_t) cpu->gpr[rs] >= (m_ireg_t) cpu->gpr[rt]) {
        mips_trigger_trap_exception (cpu);
        return (1);
    }
    return (0);
}

static int tgei_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    m_ireg_t val = sign_extend (bits (insn, 0, 15), 16);

    if (unlikely ((m_ireg_t) cpu->gpr[rs] >= val)) {
        mips_trigger_trap_exception (cpu);
        return (1);
    }
    return (0);
}

static int tgeiu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    m_reg_t val = sign_extend (bits (insn, 0, 15), 16);

    if (unlikely ((m_reg_t) cpu->gpr[rs] >= val)) {
        mips_trigger_trap_exception (cpu);
        return (1);
    }
    return (0);
}

static int tgeu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);

    if ((m_reg_t) cpu->gpr[rs] >= (m_reg_t) cpu->gpr[rt]) {
        mips_trigger_trap_exception (cpu);
        return (1);
    }
    return (0);
}

static int tlt_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);

    if ((m_ireg_t) cpu->gpr[rs] < (m_ireg_t) cpu->gpr[rt]) {
        mips_trigger_trap_exception (cpu);
        return (1);
    }
    return (0);
}

static int tlti_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    m_ireg_t val = sign_extend (bits (insn, 0, 15), 16);

    if (unlikely ((m_ireg_t) cpu->gpr[rs] < val)) {
        mips_trigger_trap_exception (cpu);
        return (1);
    }
    return (0);
}

static int tltiu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    m_reg_t val = sign_extend (bits (insn, 0, 15), 16);

    if (unlikely ((m_reg_t) cpu->gpr[rs] < val)) {
        mips_trigger_trap_exception (cpu);
        return (1);
    }
    return (0);
}

static int tltu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);

    if ((m_reg_t) cpu->gpr[rs] < (m_reg_t) cpu->gpr[rt]) {
        mips_trigger_trap_exception (cpu);
        return (1);
    }
    return (0);
}

static int tne_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);

    if ((m_ireg_t) cpu->gpr[rs] != (m_ireg_t) cpu->gpr[rt]) {
        mips_trigger_trap_exception (cpu);
        return (1);
    }
    return (0);
}

static int tnei_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    m_reg_t val = sign_extend (bits (insn, 0, 15), 16);

    if (unlikely (cpu->gpr[rs] != val)) {
        mips_trigger_trap_exception (cpu);
        return (1);
    }
    return (0);
}

static int wait_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    if (! (cpu->cp0.reg [MIPS_CP0_STATUS] & MIPS_CP0_STATUS_IE)) {
        /* Wait instruction with interrupts disabled - stop the simulator. */
        printf ("%08x: wait instruction with interrupts disabled - stop the simulator.\n",
            cpu->pc);
        kill (0, SIGQUIT);
    }
    usleep (1000);
    return (0);
}

static int xor_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);

    cpu->reg_set (cpu, rd, cpu->gpr[rs] ^ cpu->gpr[rt]);
    return (0);
}

static int rdpgpr_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);

    printf ("%08x: unsupported RDPGPR $%u,$%u instruction.\n", cpu->pc, rd, rt);
    return (0);
}

static int wrpgpr_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);

    printf ("%08x: unsupported WRPGPR $%u,$%u instruction.\n", cpu->pc, rd, rt);
    return (0);
}

static int xori_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int imm = bits (insn, 0, 15);

    cpu->reg_set (cpu, rt, cpu->gpr[rs] ^ imm);
    return (0);
}

static int sdbbp_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    /* Clear status of previos Debug exception. */
    cpu->cp0.reg[MIPS_CP0_DEBUG] &= ~MIPS_CP0_DEBUG_DEXCCODE;

    if (cpu->cp0.reg[MIPS_CP0_DEBUG] & MIPS_CP0_DEBUG_DM) {
        /* Already in Debug mode: take nested debug exception. */
        mips_trigger_debug_exception (cpu, 0);

        /* Set nested exception type. */
        cpu->cp0.reg[MIPS_CP0_DEBUG] |=
            MIPS_CP0_CAUSE_BP << MIPS_CP0_DEBUG_DEXCCODE_SHIFT;

    } else {
        /* Take a Breakpoint exception. */
        mips_trigger_debug_exception (cpu, MIPS_CP0_DEBUG_DBP);
    }
    return 1;
}

static int undef_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int undef_bcond (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int undef_cop0 (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int undef_spec2 (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int undef_spec3 (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int undef_spec (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int undef_tlb (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

/*
 * Main instruction table, indexed by bits 31:26 of opcode.
 */
static const struct mips_op_desc mips_opcodes[] = {
    {"spec",    spec_op,    0x00},      /* indexed by FUNC field */
    {"bcond",   bcond_op,   0x01},      /* indexed by RT field */
    {"j",       j_op,       0x02},
    {"jal",     jal_op,     0x03},
    {"beq",     beq_op,     0x04},
    {"bne",     bne_op,     0x05},
    {"blez",    blez_op,    0x06},
    {"bgtz",    bgtz_op,    0x07},
    {"addi",    addi_op,    0x08},
    {"addiu",   addiu_op,   0x09},
    {"slti",    slti_op,    0x0A},
    {"sltiu",   sltiu_op,   0x0B},
    {"andi",    andi_op,    0x0C},
    {"ori",     ori_op,     0x0D},
    {"xori",    xori_op,    0x0E},
    {"lui",     lui_op,     0x0F},
    {"cop0",    cop0_op,    0x10},      /* indexed by RS field */
    {"cop1",    cop1_op,    0x11},
    {"cop2",    cop2_op,    0x12},
    {"cop1x",   cop1_op,    0x13},
    {"beql",    beql_op,    0x14},
    {"bnel",    bnel_op,    0x15},
    {"blezl",   blezl_op,   0x16},
    {"bgtzl",   bgtzl_op,   0x17},
    {"daddi",   unknown_op, 0x18},
    {"daddiu",  unknown_op, 0x19},
    {"ldl",     unknown_op, 0x1A},
    {"ldr",     unknown_op, 0x1B},
    {"spec2",   spec2_op,   0x1C},      /* indexed by FUNC field */
    {"jalx",    jalx_op,    0x1D},
    {"undef",   undef_op,   0x1E},
    {"spec3",   spec3_op,   0x1F},      /* indexed by FUNC field */
    {"lb",      lb_op,      0x20},
    {"lh",      lh_op,      0x21},
    {"lwl",     lwl_op,     0x22},
    {"lw",      lw_op,      0x23},
    {"lbu",     lbu_op,     0x24},
    {"lhu",     lhu_op,     0x25},
    {"lwr",     lwr_op,     0x26},
    {"lwu",     unknown_op, 0x27},
    {"sb",      sb_op,      0x28},
    {"sh",      sh_op,      0x29},
    {"swl",     swl_op,     0x2A},
    {"sw",      sw_op,      0x2B},
    {"sdl",     unknown_op, 0x2C},
    {"sdr",     unknown_op, 0x2D},
    {"swr",     swr_op,     0x2E},
    {"cache",   cache_op,   0x2F},
    {"ll",      ll_op,      0x30},
    {"lwc1",    cop1_op,    0x31},
    {"lwc2",    cop2_op,    0x32},
    {"pref",    pref_op,    0x33},
    {"lld",     unknown_op, 0x34},
    {"ldc1",    cop1_op,    0x35},
    {"ldc2",    cop2_op,    0x36},
    {"ld",      unknown_op, 0x37},
    {"sc",      sc_op,      0x38},
    {"swc1",    cop1_op,    0x39},
    {"swc2",    cop2_op,    0x3A},
    {"undef",   undef_op,   0x3B},
    {"scd",     unknown_op, 0x3C},
    {"sdc1",    cop1_op,    0x3D},
    {"sdc2",    cop2_op,    0x3E},
    {"sd",      unknown_op, 0x3F},
};

/*
 * SPEC opcode: indexed by FUNC field.
 */
static const struct mips_op_desc mips_spec_opcodes[] = {
    {"sll",     sll_op,     0x00},
    {"movc",	movc_op,	0x01},
    {"srl",		srl_op,		0x02},
    {"sra",		sra_op,		0x03},
    {"sllv",	sllv_op,	0x04},
    {"?spec",   undef_spec,	0x05},
    {"srlv",	srlv_op,	0x06},
    {"srav",	srav_op,	0x07},
    {"jr",		jr_op,		0x08},
    {"jalr",	jalr_op,	0x09},
    {"movz",	movz_op,	0x0A},
    {"movn",	movn_op,	0x0B},
    {"syscall",	syscall_op,	0x0C},
    {"break",	break_op,	0x0D},
    {"spim",	undef_spec,	0x0E},
    {"sync",	sync_op,	0x0F},
    {"mfhi",	mfhi_op,	0x10},
    {"mthi",	mthi_op,	0x11},
    {"mflo",	mflo_op,	0x12},
    {"mtlo",	mtlo_op,	0x13},
    {"dsllv",	unknown_op,	0x14},
    {"?spec",   undef_spec,	0x15},
    {"dsrlv",	unknown_op,	0x16},
    {"dsrav",	unknown_op,	0x17},
    {"mult",	mult_op,	0x18},
    {"multu",	multu_op,	0x19},
    {"div",		div_op,		0x1A},
    {"divu",	divu_op,	0x1B},
    {"dmult",	unknown_op,	0x1C},
    {"dmultu",	unknown_op,	0x1D},
    {"ddiv",	unknown_op,	0x1E},
    {"ddivu",	unknown_op,	0x1F},
    {"add",		add_op,		0x20},
    {"addu",	addu_op,	0x21},
    {"sub",		sub_op,		0x22},
    {"subu",	subu_op,	0x23},
    {"and",		and_op,		0x24},
    {"or",		or_op,		0x25},
    {"xor",		xor_op,		0x26},
    {"nor",		nor_op,		0x27},
    {"?spec",   undef_spec,	0x28},
    {"?spec",   undef_spec,	0x29},
    {"slt",		slt_op,		0x2A},
    {"sltu",	sltu_op,	0x2B},
    {"dadd",	unknown_op,	0x2C},
    {"daddu",	unknown_op,	0x2D},
    {"dsub",	unknown_op,	0x2E},
    {"dsubu",	unknown_op,	0x2F},
    {"tge",		tge_op,		0x30},
    {"tgeu",	tgeu_op,	0x31},
    {"tlt",		tlt_op,		0x32},
    {"tltu",	tltu_op,	0x33},
    {"teq",		teq_op,		0x34},
    {"?spec",   undef_spec,	0x35},
    {"tne",		tne_op,		0x36},
    {"?spec",   undef_spec,	0x37},
    {"dsll",	unknown_op,	0x38},
    {"?spec",   undef_spec,	0x39},
    {"dsrl",	unknown_op,	0x3A},
    {"dsra",	unknown_op,	0x3B},
    {"dsll32",	unknown_op,	0x3C},
    {"?spec",   undef_spec,	0x3D},
    {"dsrl32",	unknown_op,	0x3E},
    {"dsra32",	unknown_op,	0x3F}
};

/*
 * BCOND opcode: indexed by RT field.
 */
static const struct mips_op_desc mips_bcond_opcodes[] = {
    {"bltz",	bltz_op,	0x00},
    {"bgez",	bgez_op,	0x01},
    {"bltzl",	bltzl_op,	0x02},
    {"bgezl",	bgezl_op,	0x03},
    {"spimi",	undef_bcond,0x04},
    {"?bcond",  undef_bcond,0x05},
    {"?bcond",  undef_bcond,0x06},
    {"?bcond",  undef_bcond,0x07},
    {"tgei",	tgei_op,	0x08},
    {"tgeiu",	tgeiu_op,	0x09},
    {"tlti",	tlti_op,	0x0A},
    {"tltiu",	tltiu_op,	0x0B},
    {"teqi",	teqi_op,	0x0C},
    {"?bcond",  undef_bcond,0x0D},
    {"tnei",	tnei_op,	0x0E},
    {"?bcond",  undef_bcond,0x0F},
    {"bltzal",	bltzal_op,	0x10},
    {"bgezal",	bgezal_op,	0x11},
    {"bltzall",	bltzall_op,	0x12},
    {"bgezall",	bgezall_op,	0x13},
    {"?bcond",  undef_bcond,0x14},
    {"?bcond",  undef_bcond,0x15},
    {"?bcond",  undef_bcond,0x16},
    {"?bcond",  undef_bcond,0x17},
    {"?bcond",  undef_bcond,0x18},
    {"?bcond",  undef_bcond,0x19},
    {"?bcond",  undef_bcond,0x1A},
    {"?bcond",  undef_bcond,0x1B},
    {"?bcond",  undef_bcond,0x1C},
    {"?bcond",  undef_bcond,0x1D},
    {"?bcond",  undef_bcond,0x1E},
    {"?bcond",  undef_bcond,0x1F}
};

/*
 * COP0 opcode: indexed by RS field.
 */
static const struct mips_op_desc mips_cop0_opcodes[] = {
    {"mfc0",	mfc0_op,	0x0},
    {"dmfc0",	unknown_op,	0x1},
    {"cfc0",	cfc0_op,	0x2},
    {"?cop0",   undef_cop0,	0x3},
    {"mtc0",	mtc0_op,	0x4},
    {"dmtc0",	unknown_op,	0x5},
    {"?cop0",   undef_cop0,	0x6},
    {"?cop0",   undef_cop0,	0x7},
    {"?cop0",   undef_cop0,	0x8},
    {"?cop0",   undef_cop0,	0x9},
    {"?cop0",   rdpgpr_op,	0xa},
    {"?cop0",   mfmc0_op,   0xb},
    {"?cop0",   undef_cop0,	0xc},
    {"?cop0",   undef_cop0,	0xd},
    {"wrpgpr",  wrpgpr_op,	0xe},
    {"?cop0",   undef_cop0,	0xf},
    {"tlb",		tlb_op,		0x10},      /* indexed by FUNC field */
    {"?cop0",   undef_cop0,	0x11},
    {"?cop0",   undef_cop0,	0x12},
    {"?cop0",   undef_cop0,	0x13},
    {"?cop0",   undef_cop0,	0x14},
    {"?cop0",   undef_cop0,	0x15},
    {"?cop0",   undef_cop0,	0x16},
    {"?cop0",   undef_cop0,	0x17},
    {"?cop0",   undef_cop0,	0x18},
    {"?cop0",   undef_cop0,	0x19},
    {"?cop0",   undef_cop0,	0x1a},
    {"?cop0",   undef_cop0,	0x1b},
    {"?cop0",   undef_cop0,	0x1c},
    {"?cop0",   undef_cop0,	0x1d},
    {"?cop0",   undef_cop0,	0x1e},
    {"?cop0",   undef_cop0,	0x1f},
};

/*
 * SPEC2 opcode: indexed by FUNC field.
 */
static const struct mips_op_desc mips_spec2_opcodes[] = {
    {"madd",	madd_op,	0x0},
    {"maddu",	maddu_op,	0x1},
    {"mul",		mul_op,		0x2},
    {"?spec2",	undef_spec2,0x3},
    {"msub",	msub_op,	0x4},
    {"msubu",	msubu_op,	0x5},
    {"?spec2",	undef_spec2,0x6},
    {"?spec2",	undef_spec2,0x7},
    {"?spec2",	undef_spec2,0x8},
    {"?spec2",	undef_spec2,0x9},
    {"?spec2",	undef_spec2,0xa},
    {"?spec2",	undef_spec2,0xb},
    {"?spec2",	undef_spec2,0xc},
    {"?spec2",	undef_spec2,0xd},
    {"?spec2",	undef_spec2,0xe},
    {"?spec2",	undef_spec2,0xf},
    {"?spec2",	undef_spec2,0x10},
    {"?spec2",	undef_spec2,0x11},
    {"?spec2",	undef_spec2,0x12},
    {"?spec2",	undef_spec2,0x13},
    {"?spec2",	undef_spec2,0x14},
    {"?spec2",	undef_spec2,0x15},
    {"?spec2",	undef_spec2,0x16},
    {"?spec2",	undef_spec2,0x17},
    {"?spec2",	undef_spec2,0x18},
    {"?spec2",	undef_spec2,0x19},
    {"?spec2",	undef_spec2,0x1a},
    {"?spec2",	undef_spec2,0x1b},
    {"?spec2",	undef_spec2,0x1c},
    {"?spec2",	undef_spec2,0x1d},
    {"?spec2",	undef_spec2,0x1e},
    {"?spec2",	undef_spec2,0x1f},
    {"clz",     clz_op,     0x20},
    {"clo",     clo_op,     0x21},
    {"?spec2",	undef_spec2,0x22},
    {"?spec2",	undef_spec2,0x23},
    {"?spec2",	undef_spec2,0x24},
    {"?spec2",	undef_spec2,0x25},
    {"?spec2",	undef_spec2,0x26},
    {"?spec2",	undef_spec2,0x27},
    {"?spec2",	undef_spec2,0x28},
    {"?spec2",	undef_spec2,0x29},
    {"?spec2",	undef_spec2,0x2a},
    {"?spec2",	undef_spec2,0x2b},
    {"?spec2",	undef_spec2,0x2c},
    {"?spec2",	undef_spec2,0x2d},
    {"?spec2",	undef_spec2,0x2e},
    {"?spec2",	undef_spec2,0x2f},
    {"?spec2",	undef_spec2,0x30},
    {"?spec2",	undef_spec2,0x31},
    {"?spec2",	undef_spec2,0x32},
    {"?spec2",	undef_spec2,0x33},
    {"?spec2",	undef_spec2,0x34},
    {"?spec2",	undef_spec2,0x35},
    {"?spec2",	undef_spec2,0x36},
    {"?spec2",	undef_spec2,0x37},
    {"?spec2",	undef_spec2,0x38},
    {"?spec2",	undef_spec2,0x39},
    {"?spec2",	undef_spec2,0x3a},
    {"?spec2",	undef_spec2,0x3b},
    {"?spec2",	undef_spec2,0x3c},
    {"?spec2",	undef_spec2,0x3d},
    {"?spec2",	undef_spec2,0x3e},
    {"sdbbp",	sdbbp_op,   0x3f},
};

/*
 * SPEC3 opcode: indexed by FUNC field.
 */
static const struct mips_op_desc mips_spec3_opcodes[] = {
    {"ext",		ext_op,     0x0},
    {"?spec3",	undef_spec3,0x1},
    {"?spec3",	undef_spec3,0x2},
    {"?spec3",	undef_spec3,0x3},
    {"ins",     ins_op,     0x4},
    {"?spec3",	undef_spec3,0x5},
    {"?spec3",	undef_spec3,0x6},
    {"?spec3",	undef_spec3,0x7},
    {"?spec3",	undef_spec3,0x8},
    {"?spec3",	undef_spec3,0x9},
    {"?spec3",	undef_spec3,0xa},
    {"?spec3",	undef_spec3,0xb},
    {"?spec3",	undef_spec3,0xc},
    {"?spec3",	undef_spec3,0xd},
    {"?spec3",	undef_spec3,0xe},
    {"?spec3",	undef_spec3,0xf},
    {"?spec3",	undef_spec3,0x10},
    {"?spec3",	undef_spec3,0x11},
    {"?spec3",	undef_spec3,0x12},
    {"?spec3",	undef_spec3,0x13},
    {"?spec3",	undef_spec3,0x14},
    {"?spec3",	undef_spec3,0x15},
    {"?spec3",	undef_spec3,0x16},
    {"?spec3",	undef_spec3,0x17},
    {"?spec3",	undef_spec3,0x18},
    {"?spec3",	undef_spec3,0x19},
    {"?spec3",	undef_spec3,0x1a},
    {"?spec3",	undef_spec3,0x1b},
    {"?spec3",	undef_spec3,0x1c},
    {"?spec3",	undef_spec3,0x1d},
    {"?spec3",	undef_spec3,0x1e},
    {"?spec3",	undef_spec3,0x1f},
    {"bshfl",	bshfl_op,	0x20},
    {"?spec3",	undef_spec3,0x21},
    {"?spec3",	undef_spec3,0x22},
    {"?spec3",	undef_spec3,0x23},
    {"?spec3",	undef_spec3,0x24},
    {"?spec3",	undef_spec3,0x25},
    {"?spec3",	undef_spec3,0x26},
    {"?spec3",	undef_spec3,0x27},
    {"?spec3",	undef_spec3,0x28},
    {"?spec3",	undef_spec3,0x29},
    {"?spec3",	undef_spec3,0x2a},
    {"?spec3",	undef_spec3,0x2b},
    {"?spec3",	undef_spec3,0x2c},
    {"?spec3",	undef_spec3,0x2d},
    {"?spec3",	undef_spec3,0x2e},
    {"?spec3",	undef_spec3,0x2f},
    {"?spec3",	undef_spec3,0x30},
    {"?spec3",	undef_spec3,0x31},
    {"?spec3",	undef_spec3,0x32},
    {"?spec3",	undef_spec3,0x33},
    {"?spec3",	undef_spec3,0x34},
    {"?spec3",	undef_spec3,0x35},
    {"?spec3",	undef_spec3,0x36},
    {"?spec3",	undef_spec3,0x37},
    {"?spec3",	undef_spec3,0x38},
    {"?spec3",	undef_spec3,0x39},
    {"?spec3",	undef_spec3,0x3a},
    {"?spec3",	undef_spec3,0x3b},
    {"?spec3",	undef_spec3,0x3c},
    {"?spec3",	undef_spec3,0x3d},
    {"?spec3",	undef_spec3,0x3e},
    {"?spec3",	undef_spec3,0x3f},
};

/*
 * TLB opcode: indexed by FUNC field.
 */
static const struct mips_op_desc mips_tlb_opcodes[] = {
    {"?tlb",	undef_tlb,	0x0},
    {"tlbr",	tlbr_op,	0x1},
    {"tlbwi",	tlbwi_op,	0x2},
    {"?tlb",	undef_tlb,	0x3},
    {"?tlb",	undef_tlb,	0x4},
    {"?tlb",	undef_tlb,	0x5},
    {"tlbwi",	tlbwr_op,	0x6},
    {"?tlb",	undef_tlb,	0x7},
    {"tlbp",	tlbp_op,	0x8},
    {"?tlb",	undef_tlb,	0x9},
    {"?tlb",	undef_tlb,	0xa},
    {"?tlb",	undef_tlb,	0xb},
    {"?tlb",	undef_tlb,	0xc},
    {"?tlb",	undef_tlb,	0xd},
    {"?tlb",	undef_tlb,	0xe},
    {"?tlb",	undef_tlb,	0xf},
    {"?tlb",	undef_tlb,	0x10},
    {"?tlb",	undef_tlb,	0x11},
    {"?tlb",	undef_tlb,	0x12},
    {"?tlb",	undef_tlb,	0x13},
    {"?tlb",	undef_tlb,	0x14},
    {"?tlb",	undef_tlb,	0x15},
    {"?tlb",	undef_tlb,	0x16},
    {"?tlb",	undef_tlb,	0x17},
    {"eret",	eret_op,	0x18},
    {"?tlb",	undef_tlb,	0x19},
    {"?tlb",	undef_tlb,	0x1a},
    {"?tlb",	undef_tlb,	0x1b},
    {"?tlb",	undef_tlb,	0x1c},
    {"?tlb",	undef_tlb,	0x1d},
    {"?tlb",	undef_tlb,	0x1e},
    {"deret",	deret_op,	0x1f},
    {"wait",	wait_op,	0x20},
    {"?tlb",	undef_tlb,	0x21},
    {"?tlb",	undef_tlb,	0x22},
    {"?tlb",	undef_tlb,	0x23},
    {"?tlb",	undef_tlb,	0x24},
    {"?tlb",	undef_tlb,	0x25},
    {"?tlb",	undef_tlb,	0x26},
    {"?tlb",	undef_tlb,	0x27},
    {"?tlb",	undef_tlb,	0x28},
    {"?tlb",	undef_tlb,	0x29},
    {"?tlb",	undef_tlb,	0x2a},
    {"?tlb",	undef_tlb,	0x2b},
    {"?tlb",	undef_tlb,	0x2c},
    {"?tlb",	undef_tlb,	0x2d},
    {"?tlb",	undef_tlb,	0x2e},
    {"?tlb",	undef_tlb,	0x2f},
    {"?tlb",	undef_tlb,	0x30},
    {"?tlb",	undef_tlb,	0x31},
    {"?tlb",	undef_tlb,	0x32},
    {"?tlb",	undef_tlb,	0x33},
    {"?tlb",	undef_tlb,	0x34},
    {"?tlb",	undef_tlb,	0x35},
    {"?tlb",	undef_tlb,	0x36},
    {"?tlb",	undef_tlb,	0x37},
    {"?tlb",	undef_tlb,	0x38},
    {"?tlb",	undef_tlb,	0x39},
    {"?tlb",	undef_tlb,	0x3a},
    {"?tlb",	undef_tlb,	0x3b},
    {"?tlb",	undef_tlb,	0x3c},
    {"?tlb",	undef_tlb,	0x3d},
    {"?tlb",	undef_tlb,	0x3e},
    {"?tlb",	undef_tlb,	0x3f},
};

static int mips_exec_mips16e(cpu_mips_t* cpu, mips_insn_t instr)
{
    mips_insn_t extend = instr >> 16;
    const m_va_t pc = cpu->pc;
    m_va_t nextPc = pc + cpu->insn_len;
    int res = 0;

    instr &= 0xFFFF;

#define xlat(r) ((r) | (((r) - 2) & 16))
#define op      (instr >> 11)
#define imm2    (instr & 0x3) // RRR/SHIFT-funct
#define imm3    (instr & 0x7) // MOV32R rz
#define imm4    (instr & 0xF) // SVRS framesize
#define simm4   (imm4 - ((instr & 0x8) << 1))
#define imm5    (instr & 0x1F) // MOVR32 r32
#define imm8    ((uint8_t)instr)
#define simm8   ((int8_t)instr)
#define imm11   (instr & 0x7FF)
#define simm11  (imm11 - ((instr & 0x400) << 1))
#define imm15   (((extend & 0xF) << 11) | (extend & 0x7F0) | imm4) // EXT-RRI-A addiu
#define simm15  (imm15 - ((extend & 0x8) << 12))
#define imm16   (((extend & 0x1F) << 11) | (extend & 0x7E0) | imm5)
#define simm16  ((int16_t)imm16)
#define imm26   (((extend & 0x1F) << 21) | ((extend & 0x3E0) << 11) | (uint16_t)instr) // jal(x)
#define rx      ((instr >> 8) & 0x7) // funct/SVRS
#define ry      ((instr >> 5) & 0x7) // RR-funct
#define rz      ((instr >> 2) & 0x7) // sa
#define r32s    ((instr & 0x18) | ((instr >> 5) & 0x7)) // MOV32R split/swapped r32
#define sa5     ((extend >> 6) & 0x1F) // EXT-SHIFT
#define fmsz8   ((extend & 0xF0) | imm4) // EXT-SVRS
#define aregs   (extend & 0xF) // EXT-SVRS
#define xsregs  ((extend >> 8) & 0x7) // EXT-SVRS
#define code6   ((instr >> 5) & 0x3F) // break, sdbbp

    if ((extend >> 11) == 3) {
        // jal(x) adr26<<2 (32-bit instruction; delay slot)
        cpu->reg_set(cpu, MIPS_GPR_RA, nextPc + 3); // 2 for non-extended instruction in delay slot + 1 for ISA Mode
        if (mips_exec_bdslot(cpu) == 0) {
            nextPc = (nextPc & 0xF0000000) | (imm26 << 2);
            cpu->pc = nextPc;
            cpu->is_mips16e = (extend & 0x400) == 0; // jalx switches to MIPS32
        }
        res = 1;
    } else if (!extend) {
        switch (op) {
        case 0: // addiu[sp] rx, sp, imm8<<2
            cpu->reg_set(cpu, xlat(rx), cpu->gpr[MIPS_GPR_SP] + (imm8 << 2));
            break;
        case 1: // addiu[pc] rx, pc, imm8<<2
            cpu->reg_set(cpu, xlat(rx), (pc + (imm8 << 2)) & 0xFFFFFFFC);
            break;
        case 2: // b ofs11<<1 (no delay slot)
            nextPc += simm11 << 1;
            cpu->pc = nextPc;
            res = 1;
            break;
        case 4: // beqz rx, ofs8<<1 (no delay slot)
            if (cpu->gpr[xlat(rx)] == 0)
                nextPc += simm8 << 1;
            cpu->pc = nextPc;
            res = 1;
            break;
        case 5: // bnez rx, ofs8<<1 (no delay slot)
            if (cpu->gpr[xlat(rx)] != 0)
                nextPc += simm8 << 1;
            cpu->pc = nextPc;
            res = 1;
            break;
        case 6: // SHIFT
            switch (imm2) {
            case 0: // sll rx, ry, imm3
                cpu->reg_set(cpu, xlat(rx), cpu->gpr[xlat(ry)] << (rz | ((rz - 1) & 8)));
                break;
            case 2: // srl rx, ry, imm3
                cpu->reg_set(cpu, xlat(rx), cpu->gpr[xlat(ry)] >> (rz | ((rz - 1) & 8)));
                break;
            case 3: // sra rx, ry, imm3
                cpu->reg_set(cpu, xlat(rx), (int32_t)cpu->gpr[xlat(ry)] >> (rz | ((rz - 1) & 8)));
                break;
            default:
                goto lInvalidInstruction;
            }
            break;
        case 8: // RRI-A addiu ry, rx, simm4
            if (unlikely(instr & 0x10))
                goto lInvalidInstruction;
            cpu->reg_set(cpu, xlat(ry), cpu->gpr[xlat(rx)] + simm4);
            break;
        case 9: // addiu[8] rx, simm8
            cpu->reg_set(cpu, xlat(rx), cpu->gpr[xlat(rx)] + simm8);
            break;
        case 10: // slti rx, imm8
            cpu->reg_set(cpu, MIPS_GPR_T8, (int32_t)cpu->gpr[xlat(rx)] < (int32_t)imm8);
            break;
        case 11: // sltiu rx, imm8
            cpu->reg_set(cpu, MIPS_GPR_T8, cpu->gpr[xlat(rx)] < imm8);
            break;
        case 12: // I8
            switch (rx) {
            case 0: // bteqz ofs8<<1 (no delay slot)
                if (cpu->gpr[MIPS_GPR_T8] == 0)
                    nextPc += simm8 << 1;
                cpu->pc = nextPc;
                res = 1;
                break;
            case 1: // btnez ofs8<<1 (no delay slot)
                if (cpu->gpr[MIPS_GPR_T8] != 0)
                    nextPc += simm8 << 1;
                cpu->pc = nextPc;
                res = 1;
                break;
            case 2: // sw[rasp] ra, ofs8<<2(sp)
                res = mips_exec_memop2(cpu, MIPS_MEMOP_SW, MIPS_GPR_SP, imm8 << 2, MIPS_GPR_RA, FALSE);
                break;
            case 3: // ADJSP AKA addiu sp, simm8<<3
                cpu->reg_set(cpu, MIPS_GPR_SP, cpu->gpr[MIPS_GPR_SP] + (simm8 << 3));
                break;
            case 4: // SVRS
                if (instr & 0x80) { // save
                    uint32_t temp = cpu->gpr[MIPS_GPR_SP], temp2 = temp - (imm4 ? imm4 * 8 : 128);
                    if (instr & 0x40) // ra
                        res = mips_exec_memop(cpu, MIPS_MEMOP_SW, temp -= 4, MIPS_GPR_RA, FALSE);
                    if ((instr & 0x10) && !res) // s1
                        res = mips_exec_memop(cpu, MIPS_MEMOP_SW, temp -= 4, MIPS_GPR_S1, FALSE);
                    if ((instr & 0x20) && !res) // s0
                        res = mips_exec_memop(cpu, MIPS_MEMOP_SW, temp -= 4, MIPS_GPR_S0, FALSE);
                    if (!res)
                        cpu->reg_set(cpu, MIPS_GPR_SP, temp2);
                } else { // restore
                    uint32_t temp = cpu->gpr[MIPS_GPR_SP] + (imm4 ? imm4 * 8 : 128), temp2 = temp;
                    if (instr & 0x40) // ra
                        res = mips_exec_memop(cpu, MIPS_MEMOP_LW, temp -= 4, MIPS_GPR_RA, TRUE);
                    if ((instr & 0x10) && !res) // s1
                        res = mips_exec_memop(cpu, MIPS_MEMOP_LW, temp -= 4, MIPS_GPR_S1, TRUE);
                    if ((instr & 0x20) && !res) // s0
                        res = mips_exec_memop(cpu, MIPS_MEMOP_LW, temp -= 4, MIPS_GPR_S0, TRUE);
                    if (!res)
                        cpu->reg_set(cpu, MIPS_GPR_SP, temp2);
                }
                break;
            case 5: // move r32, rz (nop = move $0, $16)
                cpu->reg_set(cpu, r32s, cpu->gpr[xlat(imm3)]);
                cpu->gpr[0] = 0;
                break;
            case 7: // move ry, r32
                cpu->reg_set(cpu, xlat(ry), cpu->gpr[imm5]);
                break;
            default:
                goto lInvalidInstruction;
            }
            break;
        case 13: // li rx, imm8
            cpu->reg_set(cpu, xlat(rx), imm8);
            break;
        case 14: // cmpi rx, imm8
            cpu->reg_set(cpu, MIPS_GPR_T8, cpu->gpr[xlat(rx)] ^ imm8);
            break;
        case 16: // lb ry, ofs5(rx)
            res = mips_exec_memop2(cpu, MIPS_MEMOP_LB, xlat(rx), imm5, xlat(ry), TRUE);
            break;
        case 17: // lh ry, ofs5<<1(rx)
            res = mips_exec_memop2(cpu, MIPS_MEMOP_LH, xlat(rx), imm5 << 1, xlat(ry), TRUE);
            break;
        case 18: // lw[sp] rx, ofs8<<2(sp)
            res = mips_exec_memop2(cpu, MIPS_MEMOP_LW, MIPS_GPR_SP, imm8 << 2, xlat(rx), TRUE);
            break;
        case 19: // lw ry, ofs5<<2(rx)
            res = mips_exec_memop2(cpu, MIPS_MEMOP_LW, xlat(rx), imm5 << 2, xlat(ry), TRUE);
            break;
        case 20: // lbu ry, ofs5(rx)
            res = mips_exec_memop2(cpu, MIPS_MEMOP_LBU, xlat(rx), imm5, xlat(ry), TRUE);
            break;
        case 21: // lhu ry, ofs5<<1(rx)
            res = mips_exec_memop2(cpu, MIPS_MEMOP_LHU, xlat(rx), imm5 << 1, xlat(ry), TRUE);
            break;
        case 22: // lw[pc] rx, ofs8<<2(pc)
            res = mips_exec_memop(cpu, MIPS_MEMOP_LW, (pc + (imm8 << 2)) & 0xFFFFFFFC, xlat(rx), TRUE);
            break;
        case 24: // sb ry, ofs5(rx)
            res = mips_exec_memop2(cpu, MIPS_MEMOP_SB, xlat(rx), imm5, xlat(ry), FALSE);
            break;
        case 25: // sh ry, ofs5<<1(rx)
            res = mips_exec_memop2(cpu, MIPS_MEMOP_SH, xlat(rx), imm5 << 1, xlat(ry), FALSE);
            break;
        case 26: // sw[sp] rx, ofs8<<2(sp)
            res = mips_exec_memop2(cpu, MIPS_MEMOP_SW, MIPS_GPR_SP, imm8 << 2, xlat(rx), FALSE);
            break;
        case 27: // sw ry, ofs5<<2(rx)
            res = mips_exec_memop2(cpu, MIPS_MEMOP_SW, xlat(rx), imm5 << 2, xlat(ry), FALSE);
            break;
        case 28: // RRR
            switch (imm2) {
            case 1: // addu rz, rx, ry
                cpu->reg_set(cpu, xlat(rz), cpu->gpr[xlat(rx)] + cpu->gpr[xlat(ry)]);
                break;
            case 3: // subu rz, rx, ry
                cpu->reg_set(cpu, xlat(rz), cpu->gpr[xlat(rx)] - cpu->gpr[xlat(ry)]);
                break;
            default:
                goto lInvalidInstruction;
            }
            break;
        case 29: // RR
            switch (imm5) {
            case 0: // J(AL)R(C)
                switch (ry) {
                case 0: // jr rx (delay slot)
                    nextPc = cpu->gpr[xlat(rx)];
                    if (mips_exec_bdslot(cpu) == 0) {
                        cpu->pc = nextPc & 0xFFFFFFFE;
                        cpu->is_mips16e = nextPc & 1; // may switch to MIPS32
                    }
                    res = 1;
                    break;
                case 1: // jr ra (delay slot)
                    if (unlikely(rx))
                        goto lInvalidInstruction;
                    nextPc = cpu->gpr[MIPS_GPR_RA];
                    if (mips_exec_bdslot(cpu) == 0) {
                        cpu->pc = nextPc & 0xFFFFFFFE;
                        cpu->is_mips16e = nextPc & 1; // may switch to MIPS32
                    }
                    res = 1;
                    break;
                case 2: // jalr (delay slot)
                    cpu->reg_set(cpu, MIPS_GPR_RA, nextPc + 3); // 2 for non-extended instruction in delay slot + 1 for ISA Mode
                    nextPc = cpu->gpr[xlat(rx)];
                    if (mips_exec_bdslot(cpu) == 0) {
                        cpu->pc = nextPc & 0xFFFFFFFE;
                        cpu->is_mips16e = nextPc & 1; // may switch to MIPS32
                    }
                    res = 1;
                    break;
                case 4: // jrc rx (no delay slot)
                    nextPc = cpu->gpr[xlat(rx)];
                    cpu->pc = nextPc & 0xFFFFFFFE;
                    cpu->is_mips16e = nextPc & 1; // may switch to MIPS32
                    res = 1;
                    break;
                case 5: // jrc ra (no delay slot)
                    if (unlikely(rx))
                        goto lInvalidInstruction;
                    nextPc = cpu->gpr[MIPS_GPR_RA];
                    cpu->pc = nextPc & 0xFFFFFFFE;
                    cpu->is_mips16e = nextPc & 1; // may switch to MIPS32
                    res = 1;
                    break;
                case 6: // jalrc (no delay slot)
                    cpu->reg_set(cpu, MIPS_GPR_RA, nextPc + 1); // 1 for ISA Mode
                    nextPc = cpu->gpr[xlat(rx)];
                    cpu->pc = nextPc & 0xFFFFFFFE;
                    cpu->is_mips16e = nextPc & 1; // may switch to MIPS32
                    res = 1;
                    break;
                default:
                    goto lInvalidInstruction;
                }
                break;
            case 1: // sdbbp imm6
                res = sdbbp_op(cpu, instr);
                break;
            case 2: // slt rx, ry
                cpu->reg_set(cpu, MIPS_GPR_T8, (int32_t)cpu->gpr[xlat(rx)] < (int32_t)cpu->gpr[xlat(ry)]);
                break;
            case 3: // sltu rx, ry
                cpu->reg_set(cpu, MIPS_GPR_T8, cpu->gpr[xlat(rx)] < cpu->gpr[xlat(ry)]);
                break;
            case 4: // sllv ry, rx
                cpu->reg_set(cpu, xlat(ry), cpu->gpr[xlat(ry)] << (cpu->gpr[xlat(rx)] & 31));
                break;
            case 5: // break imm6
                mips_exec_break(cpu, code6);
                res = 1;
                break;
            case 6: // srlv ry, rx
                cpu->reg_set(cpu, xlat(ry), cpu->gpr[xlat(ry)] >> (cpu->gpr[xlat(rx)] & 31));
                break;
            case 7: // srav ry, rx
                cpu->reg_set(cpu, xlat(ry), (int32_t)cpu->gpr[xlat(ry)] >> (cpu->gpr[xlat(rx)] & 31));
                break;
            case 10: // cmp rx, ry
                cpu->reg_set(cpu, MIPS_GPR_T8, cpu->gpr[xlat(rx)] ^ cpu->gpr[xlat(ry)]);
                break;
            case 11: // neg rx, ry
                cpu->reg_set(cpu, xlat(rx), -cpu->gpr[xlat(ry)]);
                break;
            case 12: // and rx, ry
                cpu->reg_set(cpu, xlat(rx), cpu->gpr[xlat(rx)] & cpu->gpr[xlat(ry)]);
                break;
            case 13: // or rx, ry
                cpu->reg_set(cpu, xlat(rx), cpu->gpr[xlat(rx)] | cpu->gpr[xlat(ry)]);
                break;
            case 14: // xor rx, ry
                cpu->reg_set(cpu, xlat(rx), cpu->gpr[xlat(rx)] ^ cpu->gpr[xlat(ry)]);
                break;
            case 15: // not rx, ry
                cpu->reg_set(cpu, xlat(rx), ~cpu->gpr[xlat(ry)]);
                break;
            case 16: // mfhi rx
                if (unlikely(ry))
                    goto lInvalidInstruction;
                cpu->reg_set(cpu, xlat(rx), cpu->hi);
                break;
            case 17: // CNVT
                switch (ry) {
                case 0: // zeb rx
                    cpu->reg_set(cpu, xlat(rx), cpu->gpr[xlat(rx)] & 0xFF);
                    break;
                case 1: // zeh rx
                    cpu->reg_set(cpu, xlat(rx), cpu->gpr[xlat(rx)] & 0xFFFF);
                    break;
                case 4: // seb rx
                    cpu->reg_set(cpu, xlat(rx), (int8_t)cpu->gpr[xlat(rx)]);
                    break;
                case 5: // seh rx
                    cpu->reg_set(cpu, xlat(rx), (int16_t)cpu->gpr[xlat(rx)]);
                    break;
                default:
                    goto lInvalidInstruction;
                }
                break;
            case 18: // mflo rx
                if (unlikely(ry))
                    goto lInvalidInstruction;
                cpu->reg_set(cpu, xlat(rx), cpu->lo);
                break;
            case 24: { // mult rx, ry
                int64_t p = (int64_t)(int32_t)cpu->gpr[xlat(rx)] * (int32_t)cpu->gpr[xlat(ry)];
                cpu->lo = (uint32_t)p;
                cpu->hi = (uint32_t)(p >> 32);
                }
                break;
            case 25: { // multu rx, ry
                uint64_t p = (uint64_t)cpu->gpr[xlat(rx)] * cpu->gpr[xlat(ry)];
                cpu->lo = (uint32_t)p;
                cpu->hi = (uint32_t)(p >> 32);
                }
                break;
            case 26: // div rx, ry
                if (!(cpu->gpr[xlat(ry)] == 0 ||
                      (cpu->gpr[xlat(rx)] == 0x80000000 && cpu->gpr[xlat(ry)] == 0xFFFFFFFF))) {
                    cpu->lo = (int32_t)cpu->gpr[xlat(rx)] / (int32_t)cpu->gpr[xlat(ry)];
                    cpu->hi = (int32_t)cpu->gpr[xlat(rx)] % (int32_t)cpu->gpr[xlat(ry)];
                }
                break;
            case 27: // divu rx, ry
                if (cpu->gpr[xlat(ry)]) {
                    cpu->lo = cpu->gpr[xlat(rx)] / cpu->gpr[xlat(ry)];
                    cpu->hi = cpu->gpr[xlat(rx)] % cpu->gpr[xlat(ry)];
                }
                break;
            default:
                goto lInvalidInstruction;
            }
            break;
        default:
            goto lInvalidInstruction;
        }
        // ^^^ NON-EXTENDED ^^^
    } else {
        // vvv EXTENDED vvv
        if (unlikely(cpu->is_in_bdslot))
            goto lInvalidInstruction;
        switch (op) {
        case 0: // addiu[sp] rx, sp, simm16
            if (unlikely(ry))
                goto lInvalidInstruction;
            cpu->reg_set(cpu, xlat(rx), cpu->gpr[MIPS_GPR_SP] + simm16);
            break;
        case 1: // addiu[pc] rx, pc, simm16
            if (unlikely(ry))
                goto lInvalidInstruction;
            cpu->reg_set(cpu, xlat(rx), (pc & 0xFFFFFFFC) + simm16);
            break;
        case 2: // b ofs16<<1 (no delay slot)
            if (unlikely(instr & 0x7E0))
                goto lInvalidInstruction;
            nextPc += simm16 << 1;
            cpu->pc = nextPc;
            res = 1;
            break;
        case 4: // beqz rx, ofs16<<1 (no delay slot)
            if (unlikely(ry))
                goto lInvalidInstruction;
            if (cpu->gpr[xlat(rx)] == 0)
                nextPc += simm16 << 1;
            cpu->pc = nextPc;
            res = 1;
            break;
        case 5: // bnez rx, ofs16<<1 (no delay slot)
            if (unlikely(ry))
                goto lInvalidInstruction;
            if (cpu->gpr[xlat(rx)] != 0)
                nextPc += simm16 << 1;
            cpu->pc = nextPc;
            res = 1;
            break;
        case 6: // SHIFT
            switch (imm2) {
            case 0: // sll rx, ry, imm5
                if (unlikely((extend & 0x3F) | rz))
                    goto lInvalidInstruction;
                cpu->reg_set(cpu, xlat(rx), cpu->gpr[xlat(ry)] << sa5);
                break;
            case 2: // srl rx, ry, imm5
                if (unlikely((extend & 0x3F) | rz))
                    goto lInvalidInstruction;
                cpu->reg_set(cpu, xlat(rx), cpu->gpr[xlat(ry)] >> sa5);
                break;
            case 3: // sra rx, ry, imm5
                if (unlikely((extend & 0x3F) | rz))
                    goto lInvalidInstruction;
                cpu->reg_set(cpu, xlat(rx), (int32_t)cpu->gpr[xlat(ry)] >> sa5);
                break;
            default:
                goto lInvalidInstruction;
            }
            break;
        case 8: // RRI-A addiu ry, rx, simm15
            if (unlikely(instr & 0x10))
                goto lInvalidInstruction;
            cpu->reg_set(cpu, xlat(ry), cpu->gpr[xlat(rx)] + simm15);
            break;
        case 9: // addiu rx, simm16
            if (unlikely(ry))
                goto lInvalidInstruction;
            cpu->reg_set(cpu, xlat(rx), cpu->gpr[xlat(rx)] + simm16);
            break;
        case 10: // slti rx, simm16
            if (unlikely(ry))
                goto lInvalidInstruction;
            cpu->reg_set(cpu, MIPS_GPR_T8, (int32_t)cpu->gpr[xlat(rx)] < simm16);
            break;
        case 11: // sltiu rx, simm16
            if (unlikely(ry))
                goto lInvalidInstruction;
            cpu->reg_set(cpu, MIPS_GPR_T8, cpu->gpr[xlat(rx)] < (uint32_t)simm16);
            break;
        case 12: // I8
            switch (rx) {
            case 0: // bteqz ofs16<<1 (no delay slot)
                if (unlikely(ry))
                    goto lInvalidInstruction;
                if (cpu->gpr[MIPS_GPR_T8] == 0)
                    nextPc += simm16 << 1;
                cpu->pc = nextPc;
                res = 1;
                break;
            case 1: // btnez ofs16<<1 (no delay slot)
                if (unlikely(ry))
                    goto lInvalidInstruction;
                if (cpu->gpr[MIPS_GPR_T8] != 0)
                    nextPc += simm16 << 1;
                cpu->pc = nextPc;
                res = 1;
                break;
            case 2: // sw[rasp] ra, ofs16(sp)
                if (unlikely(ry))
                    goto lInvalidInstruction;
                res = mips_exec_memop2(cpu, MIPS_MEMOP_SW, MIPS_GPR_SP, simm16, MIPS_GPR_RA, FALSE);
                break;
            case 3: // ADJSP AKA addiu sp, simm16
                if (unlikely(ry))
                    goto lInvalidInstruction;
                cpu->reg_set(cpu, MIPS_GPR_SP, cpu->gpr[MIPS_GPR_SP] + simm16);
                break;
            case 4: { // SVRS
                uint32_t astatic = 0;
                uint32_t i;
                switch (aregs) {
                case 1: case 5: case 9: case 13: astatic = 1; break;
                case 2: case 6: case 10: astatic = 2; break;
                case 3: case 7: astatic = 3; break;
                case 11: astatic = 4; break;
                case 15:
                    goto lInvalidInstruction; // TBD!!! or address error???
                }
                if (instr & 0x80) { // save
                    uint32_t temp = cpu->gpr[MIPS_GPR_SP], temp2 = temp - fmsz8 * 8;
                    uint32_t args = 0;
                    switch (aregs) {
                    case 4: case 5: case 6: case 7: args = 1; break;
                    case 8: case 9: case 10: args = 2; break;
                    case 12: case 13: args = 3; break;
                    case 14: args = 4; break;
                    case 15:
                        goto lInvalidInstruction; // TBD!!! or address error???
                    }
                    for (i = 0; i < args && !res; i++)
                        res = mips_exec_memop(cpu, MIPS_MEMOP_SW, temp + i * 4, 4 + i, FALSE);
                    if ((instr & 0x40) && !res) // ra
                        res = mips_exec_memop(cpu, MIPS_MEMOP_SW, temp -= 4, MIPS_GPR_RA, FALSE);
                    for (i = xsregs; i && !res; i--)
                        res = mips_exec_memop(cpu, MIPS_MEMOP_SW, temp -= 4, (i == 7) ? 30 : 17 + i, FALSE);
                    if ((instr & 0x10) && !res) // s1
                        res = mips_exec_memop(cpu, MIPS_MEMOP_SW, temp -= 4, MIPS_GPR_S1, FALSE);
                    if ((instr & 0x20) && !res) // s0
                        res = mips_exec_memop(cpu, MIPS_MEMOP_SW, temp -= 4, MIPS_GPR_S0, FALSE);
                    for (i = 0; i < astatic && !res; i++)
                        res = mips_exec_memop(cpu, MIPS_MEMOP_SW, temp -= 4, 7 - i, FALSE);
                    if (!res)
                        cpu->reg_set(cpu, MIPS_GPR_SP, temp2);
                } else { // restore
                    uint32_t temp2 = cpu->gpr[MIPS_GPR_SP] + fmsz8 * 8, temp = temp2;
                    if (instr & 0x40) // ra
                        res = mips_exec_memop(cpu, MIPS_MEMOP_LW, temp -= 4, MIPS_GPR_RA, TRUE);
                    for (i = xsregs; i && !res; i--)
                        res = mips_exec_memop(cpu, MIPS_MEMOP_LW, temp -= 4, (i == 7) ? 30 : 17 + i, TRUE);
                    if ((instr & 0x10) && !res) // s1
                        res = mips_exec_memop(cpu, MIPS_MEMOP_LW, temp -= 4, MIPS_GPR_S1, TRUE);
                    if ((instr & 0x20) && !res) // s0
                        res = mips_exec_memop(cpu, MIPS_MEMOP_LW, temp -= 4, MIPS_GPR_S0, TRUE);
                    for (i = 0; i < astatic && !res; i++)
                        res = mips_exec_memop(cpu, MIPS_MEMOP_LW, temp -= 4, 7 - i, TRUE);
                    if (!res)
                        cpu->reg_set(cpu, MIPS_GPR_SP, temp2);
                }
                }
                break;
            default:
                goto lInvalidInstruction;
            }
            break;
        case 13: // li rx, imm16
            if (unlikely(ry))
                goto lInvalidInstruction;
            cpu->reg_set(cpu, xlat(rx), imm16);
            break;
        case 14: // cmpi rx, imm16
            if (unlikely(ry))
                goto lInvalidInstruction;
            cpu->reg_set(cpu, MIPS_GPR_T8, cpu->gpr[xlat(rx)] ^ imm16);
            break;
        case 16: // lb ry, ofs16(rx)
            res = mips_exec_memop2(cpu, MIPS_MEMOP_LB, xlat(rx), simm16, xlat(ry), TRUE);
            break;
        case 17: // lh ry, ofs16(rx)
            res = mips_exec_memop2(cpu, MIPS_MEMOP_LH, xlat(rx), simm16, xlat(ry), TRUE);
            break;
        case 18: // lw[sp] rx, ofs16(sp)
            if (unlikely(ry))
                goto lInvalidInstruction;
            res = mips_exec_memop2(cpu, MIPS_MEMOP_LW, MIPS_GPR_SP, simm16, xlat(rx), TRUE);
            break;
        case 19: // lw ry, ofs16(rx)
            res = mips_exec_memop2(cpu, MIPS_MEMOP_LW, xlat(rx), simm16, xlat(ry), TRUE);
            break;
        case 20: // lbu ry, ofs16(rx)
            res = mips_exec_memop2(cpu, MIPS_MEMOP_LBU, xlat(rx), simm16, xlat(ry), TRUE);
            break;
        case 21: // lhu ry, ofs16(rx)
            res = mips_exec_memop2(cpu, MIPS_MEMOP_LHU, xlat(rx), simm16, xlat(ry), TRUE);
            break;
        case 22: // lw[pc] rx, ofs16(pc)
            if (unlikely(ry))
                goto lInvalidInstruction;
            res = mips_exec_memop(cpu, MIPS_MEMOP_LW, (pc & 0xFFFFFFFC) + simm16, xlat(rx), TRUE);
            break;
        case 24: // sb ry, ofs16(rx)
            res = mips_exec_memop2(cpu, MIPS_MEMOP_SB, xlat(rx), simm16, xlat(ry), FALSE);
            break;
        case 25: // sh ry, ofs16(rx)
            res = mips_exec_memop2(cpu, MIPS_MEMOP_SH, xlat(rx), simm16, xlat(ry), FALSE);
            break;
        case 26: // sw[sp] rx, ofs16(sp)
            if (unlikely(ry))
                goto lInvalidInstruction;
            res = mips_exec_memop2(cpu, MIPS_MEMOP_SW, MIPS_GPR_SP, simm16, xlat(rx), FALSE);
            break;
        case 27: // sw ry, ofs16(rx)
            res = mips_exec_memop2(cpu, MIPS_MEMOP_SW, xlat(rx), simm16, xlat(ry), FALSE);
            break;
        default:
            goto lInvalidInstruction;
        }
    }

    return res;

lInvalidInstruction:
    unknown_op(cpu, (extend << 16) | instr);
    return 1;
#undef xlat
#undef op
#undef imm2
#undef imm3
#undef imm4
#undef simm4
#undef imm5
#undef imm8
#undef simm8
#undef imm11
#undef simm11
#undef imm15
#undef simm15
#undef imm16
#undef simm16
#undef imm26
#undef rx
#undef ry
#undef rz
#undef r32s
#undef sa5
#undef fmsz8
#undef aregs
#undef xsregs
#undef code6
}
