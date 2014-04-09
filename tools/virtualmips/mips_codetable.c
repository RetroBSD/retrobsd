/*
 * Code dispatch table.
 *
 * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
 *
 * This file is part of the virtualmips distribution.
 * See LICENSE file for terms of the license.
 */

static int unknown_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    printf ("--- Unknown instruction:\n");
    printf ("%08x:       %08x        ", cpu->pc, insn);
    print_insn_mips (cpu->pc, insn, stdout);
    printf ("\n");
    exit (EXIT_FAILURE);
}

static int add_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    m_reg_t res;

    /* TODO: Exception handling */
    res = (m_uint32_t) cpu->gpr[rs] + (m_uint32_t) cpu->gpr[rt];
    cpu->reg_set (cpu, rd, sign_extend (res, 32));
    return (0);
}

static int addi_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int imm = bits (insn, 0, 15);
    m_uint32_t res, val = sign_extend (imm, 16);

    /* TODO: Exception handling */
    res = (m_uint32_t) cpu->gpr[rs] + val;
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
    return unknown_op (cpu, insn);
#endif
}

static int cop1x_op (cpu_mips_t * cpu, mips_insn_t insn)
{
#if SOFT_FPU
    mips_exec_soft_fpu (cpu);
    return (1);
#else
    return unknown_op (cpu, insn);
#endif
}

static int cop2_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int dadd_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int daddi_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int daddiu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int daddu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int ddiv_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int ddivu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int div_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);

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

static int dmfc0_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int dmtc0_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int dmult_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int dmultu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int dsll_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int dsllv_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int dsrlv_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int dsrav_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int dsub_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int dsubu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int dsrl_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int dsra_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int dsll32_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int dsrl32_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int dsra32_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
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
    new_pc = cpu->pc & ~((1 << 28) - 1);
    new_pc |= instr_index << 2;

    /* set the return address (instruction after the delay slot) */
    cpu->reg_set (cpu, MIPS_GPR_RA, cpu->pc + 8);

    int ins_res = mips_exec_bdslot (cpu);
    if (likely (!ins_res))
        cpu->pc = new_pc;

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
    if (likely (!ins_res))
        cpu->pc = new_pc;
    return (1);

}

static int jr_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    m_va_t new_pc;

    /* get the new pc */
    new_pc = cpu->gpr[rs];

    int ins_res = mips_exec_bdslot (cpu);
    if (likely (!ins_res))
        cpu->pc = new_pc;
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

static int ld_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int ldc1_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int ldc2_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int ldl_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int ldr_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
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

static int lld_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
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

static int lwc1_op (cpu_mips_t * cpu, mips_insn_t insn)
{
#if SOFT_FPU
    mips_exec_soft_fpu (cpu);
    return (1);
#else
    return unknown_op (cpu, insn);
#endif

}

static int lwc2_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
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

static int lwu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    return (mips_exec_memop2 (cpu, MIPS_MEMOP_LWU, base, offset, rt, TRUE));
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

static int scd_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int sd_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int sdc1_op (cpu_mips_t * cpu, mips_insn_t insn)
{
#if SOFT_FPU
    mips_exec_soft_fpu (cpu);
    return (1);
#else
    return unknown_op (cpu, insn);
#endif
}

static int sdc2_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int sdl_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int sdr_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
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
    m_uint32_t res;

    /* TODO: Exception handling */
    res = (m_uint32_t) cpu->gpr[rs] - (m_uint32_t) cpu->gpr[rt];
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

static int swc1_op (cpu_mips_t * cpu, mips_insn_t insn)
{
#if SOFT_FPU
    mips_exec_soft_fpu (cpu);
    return (1);
#else
    return unknown_op (cpu, insn);
#endif
}

static int swc2_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
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

static int mfmc0_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    int func = bits (insn, 0, 5);

    if (rd != 12)
        return unknown_op (cpu, insn);

    cpu->reg_set (cpu, rt, cpu->cp0.reg [MIPS_CP0_STATUS]);
    if (func & 0x20) {
        /* ei - enable interrupts */
        cpu->cp0.reg [MIPS_CP0_STATUS] |= MIPS_CP0_STATUS_IE;
    } else {
        /* di - disable interrupts */
        cpu->cp0.reg [MIPS_CP0_STATUS] &= ~MIPS_CP0_STATUS_IE;
    }
    return 0;
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
    return unknown_op (cpu, insn);
}

static int tgei_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int tgeiu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int tgeu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int tlt_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int tlti_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int tltiu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int tltu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int tne_op (cpu_mips_t * cpu, mips_insn_t insn)
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

static int tnei_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
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
 * Main instruction table.
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
    {"cop1x",   cop1x_op,   0x13},
    {"beql",    beql_op,    0x14},
    {"bnel",    bnel_op,    0x15},
    {"blezl",   blezl_op,   0x16},
    {"bgtzl",   bgtzl_op,   0x17},
    {"daddi",   daddi_op,   0x18},
    {"daddiu",  daddiu_op,  0x19},
    {"ldl",     ldl_op,     0x1A},
    {"ldr",     ldr_op,     0x1B},
    {"spec2",   spec2_op,   0x1C},      /* indexed by FUNC field */
    {"undef",   undef_op,   0x1D},
    {"undef",   undef_op,   0x1E},
    {"spec3",   spec3_op,   0x1F},      /* indexed by FUNC field */
    {"lb",      lb_op,      0x20},
    {"lh",      lh_op,      0x21},
    {"lwl",     lwl_op,     0x22},
    {"lw",      lw_op,      0x23},
    {"lbu",     lbu_op,     0x24},
    {"lhu",     lhu_op,     0x25},
    {"lwr",     lwr_op,     0x26},
    {"lwu",     lwu_op,     0x27},
    {"sb",      sb_op,      0x28},
    {"sh",      sh_op,      0x29},
    {"swl",     swl_op,     0x2A},
    {"sw",      sw_op,      0x2B},
    {"sdl",     sdl_op,     0x2C},
    {"sdr",     sdr_op,     0x2D},
    {"swr",     swr_op,     0x2E},
    {"cache",   cache_op,   0x2F},
    {"ll",      ll_op,      0x30},
    {"lwc1",    lwc1_op,    0x31},
    {"lwc2",    lwc2_op,    0x32},
    {"pref",    pref_op,    0x33},
    {"lld",     lld_op,     0x34},
    {"ldc1",    ldc1_op,    0x35},
    {"ldc2",    ldc2_op,    0x36},
    {"ld",      ld_op,      0x37},
    {"sc",      sc_op,      0x38},
    {"swc1",    swc1_op,    0x39},
    {"swc2",    swc2_op,    0x3A},
    {"undef",   undef_op,   0x3B},
    {"scd",     scd_op,     0x3C},
    {"sdc1",    sdc1_op,    0x3D},
    {"sdc2",    sdc2_op,    0x3E},
    {"sd",      sd_op,      0x3F},
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
    {"dsllv",	dsllv_op,	0x14},
    {"?spec",   undef_spec,	0x15},
    {"dsrlv",	dsrlv_op,	0x16},
    {"dsrav",	dsrav_op,	0x17},
    {"mult",	mult_op,	0x18},
    {"multu",	multu_op,	0x19},
    {"div",		div_op,		0x1A},
    {"divu",	divu_op,	0x1B},
    {"dmult",	dmult_op,	0x1C},
    {"dmultu",	dmultu_op,	0x1D},
    {"ddiv",	ddiv_op,	0x1E},
    {"ddivu",	ddivu_op,	0x1F},
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
    {"dadd",	dadd_op,	0x2C},
    {"daddu",	daddu_op,	0x2D},
    {"dsub",	dsub_op,	0x2E},
    {"dsubu",	dsubu_op,	0x2F},
    {"tge",		tge_op,		0x30},
    {"tgeu",	tgeu_op,	0x31},
    {"tlt",		tlt_op,		0x32},
    {"tltu",	tltu_op,	0x33},
    {"teq",		teq_op,		0x34},
    {"?spec",   undef_spec,	0x35},
    {"tne",		tne_op,		0x36},
    {"?spec",   undef_spec,	0x37},
    {"dsll",	dsll_op,	0x38},
    {"?spec",   undef_spec,	0x39},
    {"dsrl",	dsrl_op,	0x3A},
    {"dsra",	dsra_op,	0x3B},
    {"dsll32",	dsll32_op,	0x3C},
    {"?spec",   undef_spec,	0x3D},
    {"dsrl32",	dsrl32_op,	0x3E},
    {"dsra32",	dsra32_op,	0x3F}
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
    {"dmfc0",	dmfc0_op,	0x1},
    {"cfc0",	cfc0_op,	0x2},
    {"?cop0",   undef_cop0,	0x3},
    {"mtc0",	mtc0_op,	0x4},
    {"dmtc0",	dmtc0_op,	0x5},
    {"?cop0",   undef_cop0,	0x6},
    {"?cop0",   undef_cop0,	0x7},
    {"?cop0",   undef_cop0,	0x8},
    {"?cop0",   undef_cop0,	0x9},
    {"?cop0",   rdpgpr_op,	0xa},
    {"?cop0",   mfmc0_op,	0xb},
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
    {"clz",		clz_op,		0x20},
    {"?spec2",	undef_spec2,0x21},
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
    {"?spec2",	undef_spec2,0x3f},
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
    {"?tlb",	undef_tlb,	0x1f},
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
