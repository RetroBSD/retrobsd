/*
 * Print mips instructions.
 *
 * Copyright 1989, 1991, 1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999,
 * 2000, 2001, 2002, 2003, 2005, 2006, 2007, 2008, 2009
 * Free Software Foundation, Inc.
 * Contributed by Nobuyuki Hikichi (hikichi@sra.co.jp).
 * Rewritten for VirtualMIPS by Serge Vakulenko.
 *
 * This file is part of the virtualmips distribution.
 * See LICENSE file for terms of the license.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mips-opcode.h"
#include "mips-opc.c"
#include "mips16-opc.c"

/* FIXME: These are needed to figure out if the code is mips16 or
   not. The low bit of the address is often a good indicator.  No
   symbol table is available when this code runs out in an embedded
   system as when it is used for disassembler support in a monitor.  */

/* Mips instructions are at maximum this many bytes long.  */
#define INSNLEN 4

struct mips_cp0sel_name {
    unsigned int cp0reg;
    unsigned int sel;
    const char *const name;
};

/* The mips16 registers.  */
static const unsigned int mips16_to_32_reg_map[] = {
    16, 17, 2, 3, 4, 5, 6, 7
};

#define mips16_reg_names(rn)	mips_gpr_names[mips16_to_32_reg_map[rn]]

static const char *const mips_gpr_names[32] = {
    "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "s8", "ra"
};

static const char *const mips_fpr_names[32] = {
    "$f0", "$f1", "$f2", "$f3", "$f4", "$f5", "$f6", "$f7",
    "$f8", "$f9", "$f10", "$f11", "$f12", "$f13", "$f14", "$f15",
    "$f16", "$f17", "$f18", "$f19", "$f20", "$f21", "$f22", "$f23",
    "$f24", "$f25", "$f26", "$f27", "$f28", "$f29", "$f30", "$f31"
};

static const char *const mips_cp0_names[32] = {
    "c0_index", "c0_random", "c0_entrylo0", "c0_entrylo1",
    "c0_context", "c0_pagemask", "c0_wired", "c0_hwrena",
    "c0_badvaddr", "c0_count", "c0_entryhi", "c0_compare",
    "c0_status", "c0_cause", "c0_epc", "c0_prid",
    "c0_config", "c0_lladdr", "c0_watchlo", "c0_watchhi",
    "c0_xcontext", "$21", "$22", "c0_debug",
    "c0_depc", "c0_perfcnt", "c0_errctl", "c0_cacheerr",
    "c0_taglo", "c0_taghi", "c0_errorepc", "c0_desave",
};

static const struct mips_cp0sel_name mips_cp0sel_names[] = {
    {4, 1, "c0_contextconfig"},
    {0, 1, "c0_mvpcontrol"},
    {0, 2, "c0_mvpconf0"},
    {0, 3, "c0_mvpconf1"},
    {1, 1, "c0_vpecontrol"},
    {1, 2, "c0_vpeconf0"},
    {1, 3, "c0_vpeconf1"},
    {1, 4, "c0_yqmask"},
    {1, 5, "c0_vpeschedule"},
    {1, 6, "c0_vpeschefback"},
    {2, 1, "c0_tcstatus"},
    {2, 2, "c0_tcbind"},
    {2, 3, "c0_tcrestart"},
    {2, 4, "c0_tchalt"},
    {2, 5, "c0_tccontext"},
    {2, 6, "c0_tcschedule"},
    {2, 7, "c0_tcschefback"},
    {5, 1, "c0_pagegrain"},
    {6, 1, "c0_srsconf0"},
    {6, 2, "c0_srsconf1"},
    {6, 3, "c0_srsconf2"},
    {6, 4, "c0_srsconf3"},
    {6, 5, "c0_srsconf4"},
    {12, 1, "c0_intctl"},
    {12, 2, "c0_srsctl"},
    {12, 3, "c0_srsmap"},
    {15, 1, "c0_ebase"},
    {16, 1, "c0_config1"},
    {16, 2, "c0_config2"},
    {16, 3, "c0_config3"},
    {18, 1, "c0_watchlo,1"},
    {18, 2, "c0_watchlo,2"},
    {18, 3, "c0_watchlo,3"},
    {18, 4, "c0_watchlo,4"},
    {18, 5, "c0_watchlo,5"},
    {18, 6, "c0_watchlo,6"},
    {18, 7, "c0_watchlo,7"},
    {19, 1, "c0_watchhi,1"},
    {19, 2, "c0_watchhi,2"},
    {19, 3, "c0_watchhi,3"},
    {19, 4, "c0_watchhi,4"},
    {19, 5, "c0_watchhi,5"},
    {19, 6, "c0_watchhi,6"},
    {19, 7, "c0_watchhi,7"},
    {23, 1, "c0_tracecontrol"},
    {23, 2, "c0_tracecontrol2"},
    {23, 3, "c0_usertracedata"},
    {23, 4, "c0_tracebpc"},
    {25, 1, "c0_perfcnt,1"},
    {25, 2, "c0_perfcnt,2"},
    {25, 3, "c0_perfcnt,3"},
    {25, 4, "c0_perfcnt,4"},
    {25, 5, "c0_perfcnt,5"},
    {25, 6, "c0_perfcnt,6"},
    {25, 7, "c0_perfcnt,7"},
    {27, 1, "c0_cacheerr,1"},
    {27, 2, "c0_cacheerr,2"},
    {27, 3, "c0_cacheerr,3"},
    {28, 1, "c0_datalo"},
    {28, 2, "c0_taglo1"},
    {28, 3, "c0_datalo1"},
    {28, 4, "c0_taglo2"},
    {28, 5, "c0_datalo2"},
    {28, 6, "c0_taglo3"},
    {28, 7, "c0_datalo3"},
    {29, 1, "c0_datahi"},
    {29, 2, "c0_taghi1"},
    {29, 3, "c0_datahi1"},
    {29, 4, "c0_taghi2"},
    {29, 5, "c0_datahi2"},
    {29, 6, "c0_taghi3"},
    {29, 7, "c0_datahi3"},
};
static const int mips_cp0sel_names_len = sizeof (mips_cp0sel_names) / sizeof (*mips_cp0sel_names);

static const char *const mips_hwr_names[32] = {
    "$0", "$1", "$2", "$3", "$4", "$5", "$6", "$7",
    "$8", "$9", "$10", "$11", "$12", "$13", "$14", "$15",
    "$16", "$17", "$18", "$19", "$20", "$21", "$22", "$23",
    "$24", "$25", "$26", "$27", "$28", "$29", "$30", "$31"
};

/*
 * If set disassemble as most general inst.
 */
static int no_aliases = 0;

static int big_endian;
static int branch_delay_insns;
static int data_size;
static unsigned target;

enum dis_insn_type {
    dis_noninsn,		/* Not a valid instruction.  */
    dis_nonbranch,		/* Not a branch instruction.  */
    dis_branch,			/* Unconditional branch.  */
    dis_condbranch,		/* Conditional branch.  */
    dis_jsr,			/* Jump to subroutine.  */
    dis_condjsr,		/* Conditional jump to subroutine.  */
    dis_dref,			/* Data reference instruction.  */
    dis_dref2			/* Two data references in instruction.  */
};

static enum dis_insn_type insn_type;

#if 0
/*
 * Get LENGTH bytes from info's buffer, at target address memaddr.
 *  Transfer them to myaddr.
 */
int
read_memory (unsigned memaddr,
		    unsigned char *myaddr,
		    unsigned int nbytes)
{
    //unsigned char *buffer;
    //unsigned int buffer_length;
    //unsigned int buffer_vma;

    if (memaddr < buffer_vma ||
        memaddr - buffer_vma > buffer_length ||
        memaddr - buffer_vma + nbytes > buffer_length) {
        /* Out of bounds.  Use EIO because GDB uses it.  */
        return EIO;
    }
    memcpy (myaddr, info->buffer + memaddr - buffer_vma, nbytes);
    return 0;
}
#endif

static void
print_address (unsigned address, FILE *stream)
{
    if (address < 16)
        fprintf (stream, "%d", address);
    else
        fprintf (stream, "0x%x", address);
}

static void
memory_error (int status, unsigned memaddr, FILE *stream)
{
    fprintf (stream, "Error reading memory at 0x%08x: %s", memaddr, strerror (status));
}

static const struct mips_cp0sel_name *
lookup_mips_cp0sel_name (const struct
    mips_cp0sel_name *names, unsigned int len, unsigned int cp0reg,
    unsigned int sel)
{
    unsigned int i;

    for (i = 0; i < len; i++)
        if (names[i].cp0reg == cp0reg && names[i].sel == sel)
            return &names[i];
    return NULL;
}

/*
 * CP0 register including 'sel' code for mftc0, to be
 * printed textually if known.  If not known, print both
 * CP0 register name and sel numerically since CP0 register
 * with sel 0 may have a name unrelated to register being
 * printed.
 */
const char *cp0reg_name (unsigned cp0reg, unsigned sel)
{
    const struct mips_cp0sel_name *n;
    static char name [32];

    if (sel == 0)
        return mips_cp0_names[cp0reg];

    n = lookup_mips_cp0sel_name (mips_cp0sel_names,
        mips_cp0sel_names_len, cp0reg, sel);
    if (n != NULL)
        return n->name;

    sprintf (name, "CP0_R[%d,%d]", cp0reg, sel);
    return name;
}

/* Print insn arguments for 32/64-bit code.  */

static void
print_insn_args (const char *d,
    register unsigned long int l,
    unsigned pc, FILE *stream, const struct mips_opcode *opp)
{
    int op, delta;
    unsigned int lsb, msb, msbd;

    lsb = 0;

    for (; *d != '\0'; d++) {
        switch (*d) {
        case ',':
        case '(':
        case ')':
        case '[':
        case ']':
            fprintf (stream, "%c", *d);
            break;

        case '+':
            /* Extension character; switch for second char.  */
            d++;
            switch (*d) {
            case '\0':
                /* xgettext:c-format */
                fprintf (stream,
                    "# internal error, incomplete extension sequence (+)");
                return;

            case 'A':
                lsb = (l >> OP_SH_SHAMT) & OP_MASK_SHAMT;
                fprintf (stream, "0x%x", lsb);
                break;

            case 'B':
                msb = (l >> OP_SH_INSMSB) & OP_MASK_INSMSB;
                fprintf (stream, "0x%x", msb - lsb + 1);
                break;

            case '1':
                fprintf (stream, "0x%lx",
                    (l >> OP_SH_UDI1) & OP_MASK_UDI1);
                break;

            case '2':
                fprintf (stream, "0x%lx",
                    (l >> OP_SH_UDI2) & OP_MASK_UDI2);
                break;

            case '3':
                fprintf (stream, "0x%lx",
                    (l >> OP_SH_UDI3) & OP_MASK_UDI3);
                break;

            case '4':
                fprintf (stream, "0x%lx",
                    (l >> OP_SH_UDI4) & OP_MASK_UDI4);
                break;

            case 'C':
            case 'H':
                msbd = (l >> OP_SH_EXTMSBD) & OP_MASK_EXTMSBD;
                fprintf (stream, "0x%x", msbd + 1);
                break;

            case 'D':
                {
                    const struct mips_cp0sel_name *n;
                    unsigned int cp0reg, sel;

                    cp0reg = (l >> OP_SH_RD) & OP_MASK_RD;
                    sel = (l >> OP_SH_SEL) & OP_MASK_SEL;

                    /* CP0 register including 'sel' code for mtcN (et al.), to be
                     * printed textually if known.  If not known, print both
                     * CP0 register name and sel numerically since CP0 register
                     * with sel 0 may have a name unrelated to register being
                     * printed.  */
                    n = lookup_mips_cp0sel_name (mips_cp0sel_names,
                        mips_cp0sel_names_len, cp0reg, sel);
                    if (n != NULL)
                        fprintf (stream, "%s", n->name);
                    else
                        fprintf (stream, "$%d,%d", cp0reg,
                            sel);
                    break;
                }

            case 'E':
                lsb = ((l >> OP_SH_SHAMT) & OP_MASK_SHAMT) + 32;
                fprintf (stream, "0x%x", lsb);
                break;

            case 'F':
                msb = ((l >> OP_SH_INSMSB) & OP_MASK_INSMSB) + 32;
                fprintf (stream, "0x%x", msb - lsb + 1);
                break;

            case 'G':
                msbd = ((l >> OP_SH_EXTMSBD) & OP_MASK_EXTMSBD) + 32;
                fprintf (stream, "0x%x", msbd + 1);
                break;

            case 't':          /* Coprocessor 0 reg name */
                fprintf (stream, "%s",
                    mips_cp0_names[(l >> OP_SH_RT) & OP_MASK_RT]);
                break;

            case 'T':          /* Coprocessor 0 reg name */
                {
                    const struct mips_cp0sel_name *n;
                    unsigned int cp0reg, sel;

                    cp0reg = (l >> OP_SH_RT) & OP_MASK_RT;
                    sel = (l >> OP_SH_SEL) & OP_MASK_SEL;

                    /* CP0 register including 'sel' code for mftc0, to be
                     * printed textually if known.  If not known, print both
                     * CP0 register name and sel numerically since CP0 register
                     * with sel 0 may have a name unrelated to register being
                     * printed.  */
                    n = lookup_mips_cp0sel_name (mips_cp0sel_names,
                        mips_cp0sel_names_len, cp0reg, sel);
                    if (n != NULL)
                        fprintf (stream, "%s", n->name);
                    else
                        fprintf (stream, "$%d,%d", cp0reg,
                            sel);
                    break;
                }

            case 'x':          /* bbit bit index */
                fprintf (stream, "0x%lx",
                    (l >> OP_SH_BBITIND) & OP_MASK_BBITIND);
                break;

            case 'p':          /* cins, cins32, exts and exts32 position */
                fprintf (stream, "0x%lx",
                    (l >> OP_SH_CINSPOS) & OP_MASK_CINSPOS);
                break;

            case 's':          /* cins and exts length-minus-one */
                fprintf (stream, "0x%lx",
                    (l >> OP_SH_CINSLM1) & OP_MASK_CINSLM1);
                break;

            case 'S':          /* cins32 and exts32 length-minus-one field */
                fprintf (stream, "0x%lx",
                    (l >> OP_SH_CINSLM1) & OP_MASK_CINSLM1);
                break;

            case 'Q':          /* seqi/snei immediate field */
                op = (l >> OP_SH_SEQI) & OP_MASK_SEQI;
                /* Sign-extend it.  */
                op = (op ^ 512) - 512;
                fprintf (stream, "%d", op);
                break;

            default:
                /* xgettext:c-format */
                fprintf (stream, "# internal error, undefined extension sequence (+%c)", *d);
                return;
            }
            break;

        case '2':
            fprintf (stream, "0x%lx",
                (l >> OP_SH_BP) & OP_MASK_BP);
            break;

        case '3':
            fprintf (stream, "0x%lx",
                (l >> OP_SH_SA3) & OP_MASK_SA3);
            break;

        case '4':
            fprintf (stream, "0x%lx",
                (l >> OP_SH_SA4) & OP_MASK_SA4);
            break;

        case '5':
            fprintf (stream, "0x%lx",
                (l >> OP_SH_IMM8) & OP_MASK_IMM8);
            break;

        case '6':
            fprintf (stream, "0x%lx",
                (l >> OP_SH_RS) & OP_MASK_RS);
            break;

        case '7':
            fprintf (stream, "$ac%ld",
                (l >> OP_SH_DSPACC) & OP_MASK_DSPACC);
            break;

        case '8':
            fprintf (stream, "0x%lx",
                (l >> OP_SH_WRDSP) & OP_MASK_WRDSP);
            break;

        case '9':
            fprintf (stream, "$ac%ld",
                (l >> OP_SH_DSPACC_S) & OP_MASK_DSPACC_S);
            break;

        case '0':              /* dsp 6-bit signed immediate in bit 20 */
            delta = ((l >> OP_SH_DSPSFT) & OP_MASK_DSPSFT);
            if (delta & 0x20)   /* test sign bit */
                delta |= ~OP_MASK_DSPSFT;
            fprintf (stream, "%d", delta);
            break;

        case ':':              /* dsp 7-bit signed immediate in bit 19 */
            delta = ((l >> OP_SH_DSPSFT_7) & OP_MASK_DSPSFT_7);
            if (delta & 0x40)   /* test sign bit */
                delta |= ~OP_MASK_DSPSFT_7;
            fprintf (stream, "%d", delta);
            break;

        case '\'':
            fprintf (stream, "0x%lx",
                (l >> OP_SH_RDDSP) & OP_MASK_RDDSP);
            break;

        case '@':              /* dsp 10-bit signed immediate in bit 16 */
            delta = ((l >> OP_SH_IMM10) & OP_MASK_IMM10);
            if (delta & 0x200)  /* test sign bit */
                delta |= ~OP_MASK_IMM10;
            fprintf (stream, "%d", delta);
            break;

        case '!':
            fprintf (stream, "%ld",
                (l >> OP_SH_MT_U) & OP_MASK_MT_U);
            break;

        case '$':
            fprintf (stream, "%ld",
                (l >> OP_SH_MT_H) & OP_MASK_MT_H);
            break;

        case '*':
            fprintf (stream, "$ac%ld",
                (l >> OP_SH_MTACC_T) & OP_MASK_MTACC_T);
            break;

        case '&':
            fprintf (stream, "$ac%ld",
                (l >> OP_SH_MTACC_D) & OP_MASK_MTACC_D);
            break;

        case 'g':
            /* Coprocessor register for CTTC1, MTTC2, MTHC2, CTTC2.  */
            fprintf (stream, "$%ld",
                (l >> OP_SH_RD) & OP_MASK_RD);
            break;

        case 's':
        case 'b':
        case 'r':
        case 'v':
            fprintf (stream, "%s",
                mips_gpr_names[(l >> OP_SH_RS) & OP_MASK_RS]);
            break;

        case 't':
        case 'w':
            fprintf (stream, "%s",
                mips_gpr_names[(l >> OP_SH_RT) & OP_MASK_RT]);
            break;

        case 'i':
        case 'u':
            fprintf (stream, "0x%lx",
                (l >> OP_SH_IMMEDIATE) & OP_MASK_IMMEDIATE);
            break;

        case 'j':              /* Same as i, but sign-extended.  */
        case 'o':
            delta = (l >> OP_SH_DELTA) & OP_MASK_DELTA;
            if (delta & 0x8000)
                delta |= ~0xffff;
            fprintf (stream, "%d", delta);
            break;

        case 'h':
            fprintf (stream, "0x%x",
                (unsigned int) ((l >> OP_SH_PREFX)
                    & OP_MASK_PREFX));
            break;

        case 'k':
            fprintf (stream, "0x%x",
                (unsigned int) ((l >> OP_SH_CACHE)
                    & OP_MASK_CACHE));
            break;

        case 'a':
            target = (((pc + 4) & ~(unsigned) 0x0fffffff)
                | (((l >> OP_SH_TARGET) & OP_MASK_TARGET) << 2));
            /* For gdb disassembler, force odd address on jalx.  */
            if (strcmp (opp->name, "jalx") == 0)
                target |= 1;
            print_address (target, stream);
            break;

        case 'p':
            /* Sign extend the displacement.  */
            delta = (l >> OP_SH_DELTA) & OP_MASK_DELTA;
            if (delta & 0x8000)
                delta |= ~0xffff;
            target = (delta << 2) + pc + INSNLEN;
            print_address (target, stream);
            break;

        case 'd':
            fprintf (stream, "%s",
                mips_gpr_names[(l >> OP_SH_RD) & OP_MASK_RD]);
            break;

        case 'U':
            {
                /* First check for both rd and rt being equal.  */
                unsigned int reg = (l >> OP_SH_RD) & OP_MASK_RD;
                if (reg == ((l >> OP_SH_RT) & OP_MASK_RT))
                    fprintf (stream, "%s",
                        mips_gpr_names[reg]);
                else {
                    /* If one is zero use the other.  */
                    if (reg == 0)
                        fprintf (stream, "%s",
                            mips_gpr_names[(l >> OP_SH_RT) & OP_MASK_RT]);
                    else if (((l >> OP_SH_RT) & OP_MASK_RT) == 0)
                        fprintf (stream, "%s",
                            mips_gpr_names[reg]);
                    else        /* Bogus, result depends on processor.  */
                        fprintf (stream, "%s or %s",
                            mips_gpr_names[reg],
                            mips_gpr_names[(l >> OP_SH_RT) & OP_MASK_RT]);
                }
            }
            break;

        case 'z':
            fprintf (stream, "%s", mips_gpr_names[0]);
            break;

        case '<':
        case '1':
            fprintf (stream, "0x%lx",
                (l >> OP_SH_SHAMT) & OP_MASK_SHAMT);
            break;

        case 'c':
            fprintf (stream, "0x%lx",
                (l >> OP_SH_CODE) & OP_MASK_CODE);
            break;

        case 'q':
            fprintf (stream, "0x%lx",
                (l >> OP_SH_CODE2) & OP_MASK_CODE2);
            break;

        case 'C':
            fprintf (stream, "0x%lx",
                (l >> OP_SH_COPZ) & OP_MASK_COPZ);
            break;

        case 'B':
            fprintf (stream, "0x%lx",
                (l >> OP_SH_CODE20) & OP_MASK_CODE20);
            break;

        case 'J':
            fprintf (stream, "0x%lx",
                (l >> OP_SH_CODE19) & OP_MASK_CODE19);
            break;

        case 'S':
        case 'V':
            fprintf (stream, "%s",
                mips_fpr_names[(l >> OP_SH_FS) & OP_MASK_FS]);
            break;

        case 'T':
        case 'W':
            fprintf (stream, "%s",
                mips_fpr_names[(l >> OP_SH_FT) & OP_MASK_FT]);
            break;

        case 'D':
            fprintf (stream, "%s",
                mips_fpr_names[(l >> OP_SH_FD) & OP_MASK_FD]);
            break;

        case 'R':
            fprintf (stream, "%s",
                mips_fpr_names[(l >> OP_SH_FR) & OP_MASK_FR]);
            break;

        case 'E':
            /* Coprocessor register for lwcN instructions, et al.
             *
             * Note that there is no load/store cp0 instructions, and
             * that FPU (cp1) instructions disassemble this field using
             * 'T' format.  Therefore, until we gain understanding of
             * cp2 register names, we can simply print the register
             * numbers.  */
            fprintf (stream, "$%ld",
                (l >> OP_SH_RT) & OP_MASK_RT);
            break;

        case 'G':
            /* Coprocessor register for mtcN instructions, et al.  Note
             * that FPU (cp1) instructions disassemble this field using
             * 'S' format.  Therefore, we only need to worry about cp0,
             * cp2, and cp3.  */
            op = (l >> OP_SH_OP) & OP_MASK_OP;
            if (op == OP_OP_COP0)
                fprintf (stream, "%s",
                    mips_cp0_names[(l >> OP_SH_RD) & OP_MASK_RD]);
            else
                fprintf (stream, "$%ld",
                    (l >> OP_SH_RD) & OP_MASK_RD);
            break;

        case 'K':
            fprintf (stream, "%s",
                mips_hwr_names[(l >> OP_SH_RD) & OP_MASK_RD]);
            break;

        case 'N':
            fprintf (stream,
                ((opp->pinfo & (FP_D | FP_S)) != 0
                    ? "$fcc%ld" : "$cc%ld"), (l >> OP_SH_BCC) & OP_MASK_BCC);
            break;

        case 'M':
            fprintf (stream, "$fcc%ld",
                (l >> OP_SH_CCC) & OP_MASK_CCC);
            break;

        case 'P':
            fprintf (stream, "%ld",
                (l >> OP_SH_PERFREG) & OP_MASK_PERFREG);
            break;

        case 'e':
            fprintf (stream, "%ld",
                (l >> OP_SH_VECBYTE) & OP_MASK_VECBYTE);
            break;

        case '%':
            fprintf (stream, "%ld",
                (l >> OP_SH_VECALIGN) & OP_MASK_VECALIGN);
            break;

        case 'H':
            fprintf (stream, "%ld",
                (l >> OP_SH_SEL) & OP_MASK_SEL);
            break;

        case 'O':
            fprintf (stream, "%ld",
                (l >> OP_SH_ALN) & OP_MASK_ALN);
            break;

        case 'Q':
            {
                unsigned int vsel = (l >> OP_SH_VSEL) & OP_MASK_VSEL;

                if ((vsel & 0x10) == 0) {
                    int fmt;

                    vsel &= 0x0f;
                    for (fmt = 0; fmt < 3; fmt++, vsel >>= 1)
                        if ((vsel & 1) == 0)
                            break;
                    fprintf (stream, "$v%ld[%d]",
                        (l >> OP_SH_FT) & OP_MASK_FT, vsel >> 1);
                } else if ((vsel & 0x08) == 0) {
                    fprintf (stream, "$v%ld",
                        (l >> OP_SH_FT) & OP_MASK_FT);
                } else {
                    fprintf (stream, "0x%lx",
                        (l >> OP_SH_FT) & OP_MASK_FT);
                }
            }
            break;

        case 'X':
            fprintf (stream, "$v%ld",
                (l >> OP_SH_FD) & OP_MASK_FD);
            break;

        case 'Y':
            fprintf (stream, "$v%ld",
                (l >> OP_SH_FS) & OP_MASK_FS);
            break;

        case 'Z':
            fprintf (stream, "$v%ld",
                (l >> OP_SH_FT) & OP_MASK_FT);
            break;

        default:
            /* xgettext:c-format */
            fprintf (stream, "# internal error, undefined modifier (%c)", *d);
            return;
        }
    }
}

/*
 * Print the mips instruction at address MEMADDR in debugged memory.
 * Returns length of the instruction, in bytes, which is
 * always INSNLEN.
 */
int
print_insn_mips (unsigned memaddr,
    unsigned long int word, FILE *stream)
{
    const struct mips_opcode *op;
    static int init = 0;
    static const struct mips_opcode *mips_hash[OP_MASK_OP + 1];

    /* Build a hash table to shorten the search time.  */
    if (! init) {
        unsigned int i;

        for (i = 0; i <= OP_MASK_OP; i++) {
            for (op = mips_opcodes; op < &mips_opcodes[mips_num_opcodes]; op++) {
                if (op->pinfo == INSN_MACRO
                    || (no_aliases && (op->pinfo2 & INSN2_ALIAS)))
                    continue;
                if (i == ((op->match >> OP_SH_OP) & OP_MASK_OP)) {
                    mips_hash[i] = op;
                    break;
                }
            }
        }

        init = 1;
    }

    branch_delay_insns = 0;
    data_size = 0;
    insn_type = dis_nonbranch;
    target = 0;

    op = mips_hash[(word >> OP_SH_OP) & OP_MASK_OP];
    if (op != NULL) {
        for (; op < &mips_opcodes[mips_num_opcodes]; op++) {
            if (op->pinfo != INSN_MACRO
                && !(no_aliases && (op->pinfo2 & INSN2_ALIAS))
                && (word & op->mask) == op->match) {
                const char *d;

                /* Figure out instruction type and branch delay information.  */
                if ((op->pinfo & INSN_UNCOND_BRANCH_DELAY) != 0) {
                    if ((op->pinfo & (INSN_WRITE_GPR_31
                                | INSN_WRITE_GPR_D)) != 0)
                        insn_type = dis_jsr;
                    else
                        insn_type = dis_branch;
                    branch_delay_insns = 1;
                } else if ((op->pinfo & (INSN_COND_BRANCH_DELAY
                            | INSN_COND_BRANCH_LIKELY)) != 0) {
                    if ((op->pinfo & INSN_WRITE_GPR_31) != 0)
                        insn_type = dis_condjsr;
                    else
                        insn_type = dis_condbranch;
                    branch_delay_insns = 1;
                } else if ((op->pinfo & (INSN_STORE_MEMORY
                            | INSN_LOAD_MEMORY_DELAY)) != 0)
                    insn_type = dis_dref;

                fprintf (stream, "%s", op->name);

                d = op->args;
                if (d != NULL && *d != '\0') {
                    fprintf (stream, "\t");
                    print_insn_args (d, word, memaddr, stream, op);
                }

                return INSNLEN;
            }
        }
    }

    /* Handle undefined instructions.  */
    insn_type = dis_noninsn;
    fprintf (stream, "0x%lx", word);
    return INSNLEN;
}

static unsigned
getb16 (const unsigned char *buf)
{
    return (buf[0] << 8) | buf[1];
}

static unsigned
getl16 (const unsigned char *buf)
{
    return (buf[1] << 8) | buf[0];
}

static unsigned
getb32 (const unsigned char *buf)
{
    unsigned long v;

    v = (unsigned long) buf[0] << 24;
    v |= (unsigned long) buf[1] << 16;
    v |= (unsigned long) buf[2] << 8;
    v |= (unsigned long) buf[3];
    return v;
}

static unsigned
getl32 (const unsigned char *buf)
{
    unsigned long v;

    v = (unsigned long) buf[0];
    v |= (unsigned long) buf[1] << 8;
    v |= (unsigned long) buf[2] << 16;
    v |= (unsigned long) buf[3] << 24;
    return v;
}

/*
 * Disassemble an operand for a mips16 instruction.
 */
static void
print_mips16_insn_arg (char type,
    const struct mips_opcode *op,
    int l,
    int use_extend,
    int extend, unsigned memaddr, FILE *stream,
    int (*read_memory) (unsigned addr, unsigned char *buf, unsigned nbytes))
{
    switch (type) {
    case ',':
    case '(':
    case ')':
        fprintf (stream, "%c", type);
        break;

    case 'y':
    case 'w':
        fprintf (stream, "%s",
            mips16_reg_names (((l >> MIPS16OP_SH_RY)
                    & MIPS16OP_MASK_RY)));
        break;

    case 'x':
    case 'v':
        fprintf (stream, "%s",
            mips16_reg_names (((l >> MIPS16OP_SH_RX)
                    & MIPS16OP_MASK_RX)));
        break;

    case 'z':
        fprintf (stream, "%s",
            mips16_reg_names (((l >> MIPS16OP_SH_RZ)
                    & MIPS16OP_MASK_RZ)));
        break;

    case 'Z':
        fprintf (stream, "%s",
            mips16_reg_names (((l >> MIPS16OP_SH_MOVE32Z)
                    & MIPS16OP_MASK_MOVE32Z)));
        break;

    case '0':
        fprintf (stream, "%s", mips_gpr_names[0]);
        break;

    case 'S':
        fprintf (stream, "%s", mips_gpr_names[29]);
        break;

    case 'P':
        fprintf (stream, "$pc");
        break;

    case 'R':
        fprintf (stream, "%s", mips_gpr_names[31]);
        break;

    case 'X':
        fprintf (stream, "%s",
            mips_gpr_names[((l >> MIPS16OP_SH_REGR32)
                    & MIPS16OP_MASK_REGR32)]);
        break;

    case 'Y':
        fprintf (stream, "%s",
            mips_gpr_names[MIPS16OP_EXTRACT_REG32R (l)]);
        break;

    case '<':
    case '>':
    case '[':
    case ']':
    case '4':
    case '5':
    case 'H':
    case 'W':
    case 'D':
    case 'j':
    case '6':
    case '8':
    case 'V':
    case 'C':
    case 'U':
    case 'k':
    case 'K':
    case 'p':
    case 'q':
    case 'A':
    case 'B':
    case 'E':
        {
            int immed, nbits, shift, signedp, extbits, pcrel, extu, branch;

            shift = 0;
            signedp = 0;
            extbits = 16;
            pcrel = 0;
            extu = 0;
            branch = 0;
            switch (type) {
            case '<':
                nbits = 3;
                immed = (l >> MIPS16OP_SH_RZ) & MIPS16OP_MASK_RZ;
                extbits = 5;
                extu = 1;
                break;
            case '>':
                nbits = 3;
                immed = (l >> MIPS16OP_SH_RX) & MIPS16OP_MASK_RX;
                extbits = 5;
                extu = 1;
                break;
            case '[':
                nbits = 3;
                immed = (l >> MIPS16OP_SH_RZ) & MIPS16OP_MASK_RZ;
                extbits = 6;
                extu = 1;
                break;
            case ']':
                nbits = 3;
                immed = (l >> MIPS16OP_SH_RX) & MIPS16OP_MASK_RX;
                extbits = 6;
                extu = 1;
                break;
            case '4':
                nbits = 4;
                immed = (l >> MIPS16OP_SH_IMM4) & MIPS16OP_MASK_IMM4;
                signedp = 1;
                extbits = 15;
                break;
            case '5':
                nbits = 5;
                immed = (l >> MIPS16OP_SH_IMM5) & MIPS16OP_MASK_IMM5;
                insn_type = dis_dref;
                data_size = 1;
                break;
            case 'H':
                nbits = 5;
                shift = 1;
                immed = (l >> MIPS16OP_SH_IMM5) & MIPS16OP_MASK_IMM5;
                insn_type = dis_dref;
                data_size = 2;
                break;
            case 'W':
                nbits = 5;
                shift = 2;
                immed = (l >> MIPS16OP_SH_IMM5) & MIPS16OP_MASK_IMM5;
                if ((op->pinfo & MIPS16_INSN_READ_PC) == 0
                    && (op->pinfo & MIPS16_INSN_READ_SP) == 0) {
                    insn_type = dis_dref;
                    data_size = 4;
                }
                break;
            case 'D':
                nbits = 5;
                shift = 3;
                immed = (l >> MIPS16OP_SH_IMM5) & MIPS16OP_MASK_IMM5;
                insn_type = dis_dref;
                data_size = 8;
                break;
            case 'j':
                nbits = 5;
                immed = (l >> MIPS16OP_SH_IMM5) & MIPS16OP_MASK_IMM5;
                signedp = 1;
                break;
            case '6':
                nbits = 6;
                immed = (l >> MIPS16OP_SH_IMM6) & MIPS16OP_MASK_IMM6;
                break;
            case '8':
                nbits = 8;
                immed = (l >> MIPS16OP_SH_IMM8) & MIPS16OP_MASK_IMM8;
                break;
            case 'V':
                nbits = 8;
                shift = 2;
                immed = (l >> MIPS16OP_SH_IMM8) & MIPS16OP_MASK_IMM8;
                /* FIXME: This might be lw, or it might be addiu to $sp or
                 * $pc.  We assume it's load.  */
                insn_type = dis_dref;
                data_size = 4;
                break;
            case 'C':
                nbits = 8;
                shift = 3;
                immed = (l >> MIPS16OP_SH_IMM8) & MIPS16OP_MASK_IMM8;
                insn_type = dis_dref;
                data_size = 8;
                break;
            case 'U':
                nbits = 8;
                immed = (l >> MIPS16OP_SH_IMM8) & MIPS16OP_MASK_IMM8;
                extu = 1;
                break;
            case 'k':
                nbits = 8;
                immed = (l >> MIPS16OP_SH_IMM8) & MIPS16OP_MASK_IMM8;
                signedp = 1;
                break;
            case 'K':
                nbits = 8;
                shift = 3;
                immed = (l >> MIPS16OP_SH_IMM8) & MIPS16OP_MASK_IMM8;
                signedp = 1;
                break;
            case 'p':
                nbits = 8;
                immed = (l >> MIPS16OP_SH_IMM8) & MIPS16OP_MASK_IMM8;
                signedp = 1;
                pcrel = 1;
                branch = 1;
                break;
            case 'q':
                nbits = 11;
                immed = (l >> MIPS16OP_SH_IMM11) & MIPS16OP_MASK_IMM11;
                signedp = 1;
                pcrel = 1;
                branch = 1;
                break;
            case 'A':
                nbits = 8;
                shift = 2;
                immed = (l >> MIPS16OP_SH_IMM8) & MIPS16OP_MASK_IMM8;
                pcrel = 1;
                /* FIXME: This can be lw or la.  We assume it is lw.  */
                insn_type = dis_dref;
                data_size = 4;
                break;
            case 'B':
                nbits = 5;
                shift = 3;
                immed = (l >> MIPS16OP_SH_IMM5) & MIPS16OP_MASK_IMM5;
                pcrel = 1;
                insn_type = dis_dref;
                data_size = 8;
                break;
            case 'E':
                nbits = 5;
                shift = 2;
                immed = (l >> MIPS16OP_SH_IMM5) & MIPS16OP_MASK_IMM5;
                pcrel = 1;
                break;
            default:
                fprintf (stream, "# internal disassembler error, unrecognised mips16 type (%c)", type);
                abort ();
            }

            if (!use_extend) {
                if (signedp && immed >= (1 << (nbits - 1)))
                    immed -= 1 << nbits;
                immed <<= shift;
                if ((type == '<' || type == '>' || type == '[' || type == ']')
                    && immed == 0)
                    immed = 8;
            } else {
                if (extbits == 16)
                    immed |= ((extend & 0x1f) << 11) | (extend & 0x7e0);
                else if (extbits == 15)
                    immed |= ((extend & 0xf) << 11) | (extend & 0x7f0);
                else
                    immed = ((extend >> 6) & 0x1f) | (extend & 0x20);
                immed &= (1 << extbits) - 1;
                if (!extu && immed >= (1 << (extbits - 1)))
                    immed -= 1 << extbits;
            }

            if (!pcrel)
                fprintf (stream, "%d", immed);
            else {
                unsigned baseaddr;

                if (branch) {
                    immed *= 2;
                    baseaddr = memaddr + 2;
                } else if (use_extend)
                    baseaddr = memaddr - 2;
                else {
                    int status;
                    unsigned char buffer[2];

                    baseaddr = memaddr;

                    /* If this instruction is in the delay slot of a jr
                     * instruction, the base address is the address of the
                     * jr instruction.  If it is in the delay slot of jalr
                     * instruction, the base address is the address of the
                     * jalr instruction.  This test is unreliable: we have
                     * no way of knowing whether the previous word is
                     * instruction or data.  */
                    status = read_memory (memaddr - 4, buffer, 2);
                    if (status == 0
                        && (((big_endian ? getb16 (buffer)
                                         : getl16 (buffer))
                                & 0xf800) == 0x1800))
                        baseaddr = memaddr - 4;
                    else {
                        status = read_memory (memaddr - 2, buffer, 2);
                        if (status == 0
                            && (((big_endian ? getb16 (buffer)
                                             : getl16 (buffer))
                                    & 0xf81f) == 0xe800))
                            baseaddr = memaddr - 2;
                    }
                }
                target = (baseaddr & ~((1 << shift) - 1)) + immed;
                if (pcrel && branch)
                    /* For gdb disassembler, maintain odd address.  */
                    target |= 1;
                print_address (target, stream);
            }
        }
        break;

    case 'a':
        {
            int jalx = l & 0x400;

            if (!use_extend)
                extend = 0;
            l = ((l & 0x1f) << 23) | ((l & 0x3e0) << 13) | (extend << 2);
            if (! jalx)
                /* For gdb disassembler, maintain odd address.  */
                l |= 1;
        }
        target = ((memaddr + 4) & ~(unsigned) 0x0fffffff) | l;
        print_address (target, stream);
        break;

    case 'l':
    case 'L':
        {
            int need_comma, amask, smask;

            need_comma = 0;

            l = (l >> MIPS16OP_SH_IMM6) & MIPS16OP_MASK_IMM6;

            amask = (l >> 3) & 7;

            if (amask > 0 && amask < 5) {
                fprintf (stream, "%s", mips_gpr_names[4]);
                if (amask > 1)
                    fprintf (stream, "-%s",
                        mips_gpr_names[amask + 3]);
                need_comma = 1;
            }

            smask = (l >> 1) & 3;
            if (smask == 3) {
                fprintf (stream, "%s??",
                    need_comma ? "," : "");
                need_comma = 1;
            } else if (smask > 0) {
                fprintf (stream, "%s%s",
                    need_comma ? "," : "", mips_gpr_names[16]);
                if (smask > 1)
                    fprintf (stream, "-%s",
                        mips_gpr_names[smask + 15]);
                need_comma = 1;
            }

            if (l & 1) {
                fprintf (stream, "%s%s",
                    need_comma ? "," : "", mips_gpr_names[31]);
                need_comma = 1;
            }

            if (amask == 5 || amask == 6) {
                fprintf (stream, "%s$f0",
                    need_comma ? "," : "");
                if (amask == 6)
                    fprintf (stream, "-$f1");
            }
        }
        break;

    case 'm':
    case 'M':
        /* MIPS16e save/restore.  */
        {
            int need_comma = 0;
            int amask, args, statics;
            int nsreg, smask;
            int framesz;
            int i, j;

            l = l & 0x7f;
            if (use_extend)
                l |= extend << 16;

            amask = (l >> 16) & 0xf;
            if (amask == MIPS16_ALL_ARGS) {
                args = 4;
                statics = 0;
            } else if (amask == MIPS16_ALL_STATICS) {
                args = 0;
                statics = 4;
            } else {
                args = amask >> 2;
                statics = amask & 3;
            }

            if (args > 0) {
                fprintf (stream, "%s", mips_gpr_names[4]);
                if (args > 1)
                    fprintf (stream, "-%s",
                        mips_gpr_names[4 + args - 1]);
                need_comma = 1;
            }

            framesz = (((l >> 16) & 0xf0) | (l & 0x0f)) * 8;
            if (framesz == 0 && !use_extend)
                framesz = 128;

            fprintf (stream, "%s%d",
                need_comma ? "," : "", framesz);

            if (l & 0x40)       /* $ra */
                fprintf (stream, ",%s",
                    mips_gpr_names[31]);

            nsreg = (l >> 24) & 0x7;
            smask = 0;
            if (l & 0x20)       /* $s0 */
                smask |= 1 << 0;
            if (l & 0x10)       /* $s1 */
                smask |= 1 << 1;
            if (nsreg > 0)      /* $s2-$s8 */
                smask |= ((1 << nsreg) - 1) << 2;

            /* Find first set static reg bit.  */
            for (i = 0; i < 9; i++) {
                if (smask & (1 << i)) {
                    fprintf (stream, ",%s",
                        mips_gpr_names[i == 8 ? 30 : (16 + i)]);
                    /* Skip over string of set bits.  */
                    for (j = i; smask & (2 << j); j++)
                        continue;
                    if (j > i)
                        fprintf (stream, "-%s",
                            mips_gpr_names[j == 8 ? 30 : (16 + j)]);
                    i = j + 1;
                }
            }

            /* Statics $ax - $a3.  */
            if (statics == 1)
                fprintf (stream, ",%s",
                    mips_gpr_names[7]);
            else if (statics > 0)
                fprintf (stream, ",%s-%s",
                    mips_gpr_names[7 - statics + 1], mips_gpr_names[7]);
        }
        break;

    default:
        /* xgettext:c-format */
        fprintf (stream, "# internal disassembler error, unrecognised modifier (%c)", type);
        abort ();
    }
}

/* Disassemble mips16 instructions.  */

static int print_insn_mips16 (unsigned memaddr, FILE *stream,
    int (*read_memory) (unsigned addr, unsigned char *buf, unsigned nbytes))
{
    int status;
    unsigned char buffer[2];
    int length;
    int insn;
    int use_extend;
    int extend = 0;
    const struct mips_opcode *op, *opend;

    branch_delay_insns = 0;
    data_size = 0;
    insn_type = dis_nonbranch;
    target = 0;

    status = read_memory (memaddr, buffer, 2);
    if (status != 0) {
        memory_error (status, memaddr, stream);
        return -1;
    }

    length = 2;

    if (big_endian)
        insn = getb16 (buffer);
    else
        insn = getl16 (buffer);

    /* Handle the extend opcode specially.  */
    use_extend = 0;
    if ((insn & 0xf800) == 0xf000) {
        use_extend = 1;
        extend = insn & 0x7ff;

        memaddr += 2;

        status = read_memory (memaddr, buffer, 2);
        if (status != 0) {
            fprintf (stream, "extend 0x%x",
                (unsigned int) extend);
            memory_error (status, memaddr, stream);
            return -1;
        }

        if (big_endian)
            insn = getb16 (buffer);
        else
            insn = getl16 (buffer);

        /* Check for an extend opcode followed by an extend opcode.  */
        if ((insn & 0xf800) == 0xf000) {
            fprintf (stream, "extend 0x%x",
                (unsigned int) extend);
            insn_type = dis_noninsn;
            return length;
        }

        length += 2;
    }

    /* FIXME: Should probably use a hash table on the major opcode here.  */

    opend = mips16_opcodes + mips16_num_opcodes;
    for (op = mips16_opcodes; op < opend; op++) {
        if (op->pinfo != INSN_MACRO
            && !(no_aliases && (op->pinfo2 & INSN2_ALIAS))
            && (insn & op->mask) == op->match) {
            const char *s;

            if (strchr (op->args, 'a') != NULL) {
                if (use_extend) {
                    fprintf (stream, "extend 0x%x",
                        (unsigned int) extend);
                    insn_type = dis_noninsn;
                    return length - 2;
                }

                use_extend = 0;

                memaddr += 2;

                status = read_memory (memaddr, buffer, 2);
                if (status == 0) {
                    use_extend = 1;
                    if (big_endian)
                        extend = getb16 (buffer);
                    else
                        extend = getl16 (buffer);
                    length += 2;
                }
            }

            fprintf (stream, "%s", op->name);
            if (op->args[0] != '\0')
                fprintf (stream, "\t");

            for (s = op->args; *s != '\0'; s++) {
                if (*s == ','
                    && s[1] == 'w'
                    && (((insn >> MIPS16OP_SH_RX) & MIPS16OP_MASK_RX)
                        == ((insn >> MIPS16OP_SH_RY) & MIPS16OP_MASK_RY))) {
                    /* Skip the register and the comma.  */
                    ++s;
                    continue;
                }
                if (*s == ','
                    && s[1] == 'v'
                    && (((insn >> MIPS16OP_SH_RZ) & MIPS16OP_MASK_RZ)
                        == ((insn >> MIPS16OP_SH_RX) & MIPS16OP_MASK_RX))) {
                    /* Skip the register and the comma.  */
                    ++s;
                    continue;
                }
                print_mips16_insn_arg (*s, op, insn, use_extend, extend,
                    memaddr, stream, read_memory);
            }

            /* Figure out branch instruction type and delay slot information.  */
            if ((op->pinfo & INSN_UNCOND_BRANCH_DELAY) != 0)
                branch_delay_insns = 1;
            if ((op->pinfo & (INSN_UNCOND_BRANCH_DELAY
                        | MIPS16_INSN_UNCOND_BRANCH)) != 0) {
                if ((op->pinfo & INSN_WRITE_GPR_31) != 0)
                    insn_type = dis_jsr;
                else
                    insn_type = dis_branch;
            } else if ((op->pinfo & MIPS16_INSN_COND_BRANCH) != 0)
                insn_type = dis_condbranch;

            return length;
        }
    }

    if (use_extend)
        fprintf (stream, "0x%x", extend | 0xf000);
    fprintf (stream, "0x%x", insn);
    insn_type = dis_noninsn;

    return length;
}

/*
 * In an environment where we do not know the symbol type of the
 * instruction we are forced to assume that the low order bit of the
 * instructions' address may mark it as a mips16 instruction.  If we
 * are single stepping, or the pc is within the disassembled function,
 * this works.  Otherwise, we need a clue.  Sometimes.
 */
int
print_mips (unsigned memaddr, FILE *stream, int bigendian_flag,
    int (*read_memory) (unsigned addr, unsigned char *buf, unsigned nbytes))
{
    unsigned char buffer[INSNLEN];
    int status;

    big_endian = bigendian_flag;
#if 1
    /* FIXME: If odd address, this is CLEARLY a mips 16 instruction.  */
    /* Only a few tools will work this way.  */
    if (memaddr & 0x01)
        return print_insn_mips16 (memaddr, stream, read_memory);
#endif

    status = read_memory (memaddr, buffer, INSNLEN);
    if (status == 0) {
        unsigned long insn;

        if (big_endian)
            insn = (unsigned long) getb32 (buffer);
        else
            insn = (unsigned long) getl32 (buffer);

        return print_insn_mips (memaddr, insn, stream);
    } else {
        memory_error (status, memaddr, stream);
        return -1;
    }
}
