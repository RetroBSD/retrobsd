/*
 * MIPS opcode list.
 *
 * Copyright 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002
 * 2003, 2004, 2005, 2006, 2007, 2008, 2009  Free Software Foundation, Inc.
 * Contributed by Ralph Campbell and OSF
 * Commented and modified by Ian Lance Taylor, Cygnus Support
 * Extended for MIPS32 support by Anders Norlander, and by SiByte, Inc.
 * MIPS-3D, MDMX, and MIPS32 Release 2 support added by Broadcom
 * Corporation (SiByte).
 * Adapted for VirtualMIPS by Serge Vakulenko.
 *
 * This file is part of the virtualmips distribution.
 * See LICENSE file for terms of the license.
 */

/* Short hand so the lines aren't too long.  */

#define LDD     INSN_LOAD_MEMORY_DELAY
#define LCD     INSN_LOAD_COPROC_DELAY
#define UBD     INSN_UNCOND_BRANCH_DELAY
#define CBD     INSN_COND_BRANCH_DELAY
#define COD     INSN_COPROC_MOVE_DELAY
#define CLD     INSN_COPROC_MEMORY_DELAY
#define CBL     INSN_COND_BRANCH_LIKELY
#define TRAP    INSN_TRAP
#define SM      INSN_STORE_MEMORY

#define WR_d    INSN_WRITE_GPR_D
#define WR_t    INSN_WRITE_GPR_T
#define WR_31   INSN_WRITE_GPR_31
#define WR_D    INSN_WRITE_FPR_D
#define WR_T    INSN_WRITE_FPR_T
#define WR_S    INSN_WRITE_FPR_S
#define RD_s    INSN_READ_GPR_S
#define RD_b    INSN_READ_GPR_S
#define RD_t    INSN_READ_GPR_T
#define RD_S    INSN_READ_FPR_S
#define RD_T    INSN_READ_FPR_T
#define RD_R    INSN_READ_FPR_R
#define WR_CC   INSN_WRITE_COND_CODE
#define RD_CC   INSN_READ_COND_CODE
#define RD_C0   INSN_COP
#define RD_C1   INSN_COP
#define RD_C2   INSN_COP
#define RD_C3   INSN_COP
#define WR_C0   INSN_COP
#define WR_C1   INSN_COP
#define WR_C2   INSN_COP
#define WR_C3   INSN_COP
#define CP      INSN_COP

#define WR_HI   INSN_WRITE_HI
#define RD_HI   INSN_READ_HI
#define MOD_HI  WR_HI|RD_HI

#define WR_LO   INSN_WRITE_LO
#define RD_LO   INSN_READ_LO
#define MOD_LO  WR_LO|RD_LO

#define WR_HILO WR_HI|WR_LO
#define RD_HILO RD_HI|RD_LO
#define MOD_HILO WR_HILO|RD_HILO

#define IS_M    INSN_MULT

#define WR_MACC INSN2_WRITE_MDMX_ACC
#define RD_MACC INSN2_READ_MDMX_ACC

#define I1      INSN_ISA1
#define I2      INSN_ISA2
#define I3      INSN_ISA3
#define I4      INSN_ISA4
#define I5      INSN_ISA5
#define I32     INSN_ISA32
#define I64     INSN_ISA64
#define I33     INSN_ISA32R2
#define I65     INSN_ISA64R2
#define I3_32   INSN_ISA3_32
#define I3_33   INSN_ISA3_32R2
#define I4_32   INSN_ISA4_32
#define I4_33   INSN_ISA4_32R2
#define I5_33   INSN_ISA5_32R2

/* MIPS32 SmartMIPS ASE support.  */
#define SMT     INSN_SMARTMIPS

/* MIPS DSP ASE support.
   NOTE:
   1. MIPS DSP ASE includes 4 accumulators ($ac0 - $ac3).  $ac0 is the pair
   of original HI and LO.  $ac1, $ac2 and $ac3 are new registers, and have
   the same structure as $ac0 (HI + LO).  For DSP instructions that write or
   read accumulators (that may be $ac0), we add WR_a (WR_HILO) or RD_a
   (RD_HILO) attributes, such that HILO dependencies are maintained
   conservatively.

   2. For some mul. instructions that use integer registers as destinations
   but destroy HI+LO as side-effect, we add WR_HILO to their attributes.

   3. MIPS DSP ASE includes a new DSP control register, which has 6 fields
   (ccond, outflag, EFI, c, scount, pos).  Many DSP instructions read or write
   certain fields of the DSP control register.  For simplicity, we decide not
   to track dependencies of these fields.
   However, "bposge32" is a branch instruction that depends on the "pos"
   field.  In order to make sure that GAS does not reorder DSP instructions
   that writes the "pos" field and "bposge32", we add DSP_VOLA (INSN_TRAP)
   attribute to those instructions that write the "pos" field.  */

#define WR_a    WR_HILO /* Write dsp accumulators (reuse WR_HILO)  */
#define RD_a    RD_HILO /* Read dsp accumulators (reuse RD_HILO)  */
#define MOD_a   WR_a|RD_a
#define DSP_VOLA        INSN_TRAP
#define D32     INSN_DSP
#define D33     INSN_DSPR2
#define D64     INSN_DSP64

/* MIPS MT ASE support.  */
#define MT32    INSN_MT

/* The order of overloaded instructions matters.  Label arguments and
   register arguments look the same. Instructions that can have either
   for arguments must apear in the correct order in this table for the
   assembler to pick the right one. In other words, entries with
   immediate operands must apear after the same instruction with
   registers.

   Because of the lookup algorithm used, entries with the same opcode
   name must be contiguous.

   Many instructions are short hand for other instructions (i.e., The
   jal <register> instruction is short for jalr <register>).  */

static const struct mips_opcode mips_opcodes[] =
{
/* These instructions appear first so that the disassembler will find
   them first.  The assemblers uses a hash table based on the
   instruction name anyhow.  */
/* name,    args,       match,      mask,       pinfo,                  pinfo2,         membership */
{"pref",    "k,o(b)",   0xcc000000, 0xfc000000, RD_b,                   0,              I4_32|I4},
{"prefx",   "h,t(b)",   0x4c00000f, 0xfc0007ff, RD_b|RD_t|FP_S,         0,              I4_33   },
{"nop",     "",         0x00000000, 0xffffffff, 0,                      INSN2_ALIAS,    I1      }, /* sll */
{"ssnop",   "",         0x00000040, 0xffffffff, 0,                      INSN2_ALIAS,    I1      }, /* sll */
{"ehb",     "",         0x000000c0, 0xffffffff, 0,                      INSN2_ALIAS,    I1      }, /* sll */
{"li",      "t,j",      0x24000000, 0xffe00000, WR_t,                   INSN2_ALIAS,    I1      }, /* addiu */
{"li",      "t,i",      0x34000000, 0xffe00000, WR_t,                   INSN2_ALIAS,    I1      }, /* ori */
{"move",    "d,s",      0x0000002d, 0xfc1f07ff, WR_d|RD_s,              INSN2_ALIAS,    I3      },/* daddu */
{"move",    "d,s",      0x00000021, 0xfc1f07ff, WR_d|RD_s,              INSN2_ALIAS,    I1      },/* addu */
{"move",    "d,s",      0x00000025, 0xfc1f07ff, WR_d|RD_s,              INSN2_ALIAS,    I1      },/* or */
{"b",       "p",        0x10000000, 0xffff0000, UBD,                    INSN2_ALIAS,    I1      },/* beq 0,0 */
{"b",       "p",        0x04010000, 0xffff0000, UBD,                    INSN2_ALIAS,    I1      },/* bgez 0 */
{"bal",     "p",        0x04110000, 0xffff0000, UBD|WR_31,              INSN2_ALIAS,    I1      },/* bgezal 0*/

{"abs.s",   "D,V",      0x46000005, 0xffff003f, WR_D|RD_S|FP_S,         0,              I1      },
{"abs.d",   "D,V",      0x46200005, 0xffff003f, WR_D|RD_S|FP_D,         0,              I1      },
{"add",     "d,v,t",    0x00000020, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              I1      },
{"add.s",   "D,V,T",    0x46000000, 0xffe0003f, WR_D|RD_S|RD_T|FP_S,    0,              I1      },
{"add.d",   "D,V,T",    0x46200000, 0xffe0003f, WR_D|RD_S|RD_T|FP_D,    0,              I1      },
{"addi",    "t,r,j",    0x20000000, 0xfc000000, WR_t|RD_s,              0,              I1      },
{"addiu",   "t,r,j",    0x24000000, 0xfc000000, WR_t|RD_s,              0,              I1      },
{"addu",    "d,v,t",    0x00000021, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              I1      },
{"alnv.ps", "D,V,T,s",  0x4c00001e, 0xfc00003f, WR_D|RD_S|RD_T|FP_D,    0,              I5_33   },
{"and",     "d,v,t",    0x00000024, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              I1      },
{"andi",    "t,r,i",    0x30000000, 0xfc000000, WR_t|RD_s,              0,              I1      },
/* b is at the top of the table.  */
/* bal is at the top of the table.  */
/* bc0[tf]l? are at the bottom of the table.  */
{"bc1f",    "p",        0x45000000, 0xffff0000, CBD|RD_CC|FP_S,         0,              I1      },
{"bc1f",    "N,p",      0x45000000, 0xffe30000, CBD|RD_CC|FP_S,         0,              I4_32   },
{"bc1fl",   "p",        0x45020000, 0xffff0000, CBL|RD_CC|FP_S,         0,              I2      },
{"bc1fl",   "N,p",      0x45020000, 0xffe30000, CBL|RD_CC|FP_S,         0,              I4_32   },
{"bc1t",    "p",        0x45010000, 0xffff0000, CBD|RD_CC|FP_S,         0,              I1      },
{"bc1t",    "N,p",      0x45010000, 0xffe30000, CBD|RD_CC|FP_S,         0,              I4_32   },
{"bc1tl",   "p",        0x45030000, 0xffff0000, CBL|RD_CC|FP_S,         0,              I2      },
{"bc1tl",   "N,p",      0x45030000, 0xffe30000, CBL|RD_CC|FP_S,         0,              I4_32   },
/* bc2* are at the bottom of the table.  */
/* bc3* are at the bottom of the table.  */
{"beqz",    "s,p",      0x10000000, 0xfc1f0000, CBD|RD_s,               0,              I1      },
{"beqzl",   "s,p",      0x50000000, 0xfc1f0000, CBL|RD_s,               0,              I2      },
{"beq",     "s,t,p",    0x10000000, 0xfc000000, CBD|RD_s|RD_t,          0,              I1      },
{"beql",    "s,t,p",    0x50000000, 0xfc000000, CBL|RD_s|RD_t,          0,              I2      },
{"bgez",    "s,p",      0x04010000, 0xfc1f0000, CBD|RD_s,               0,              I1      },
{"bgezl",   "s,p",      0x04030000, 0xfc1f0000, CBL|RD_s,               0,              I2      },
{"bgezal",  "s,p",      0x04110000, 0xfc1f0000, CBD|RD_s|WR_31,         0,              I1      },
{"bgezall", "s,p",      0x04130000, 0xfc1f0000, CBL|RD_s|WR_31,         0,              I2      },
{"bgtz",    "s,p",      0x1c000000, 0xfc1f0000, CBD|RD_s,               0,              I1      },
{"bgtzl",   "s,p",      0x5c000000, 0xfc1f0000, CBL|RD_s,               0,              I2      },
{"blez",    "s,p",      0x18000000, 0xfc1f0000, CBD|RD_s,               0,              I1      },
{"blezl",   "s,p",      0x58000000, 0xfc1f0000, CBL|RD_s,               0,              I2      },
{"bltz",    "s,p",      0x04000000, 0xfc1f0000, CBD|RD_s,               0,              I1      },
{"bltzl",   "s,p",      0x04020000, 0xfc1f0000, CBL|RD_s,               0,              I2      },
{"bltzal",  "s,p",      0x04100000, 0xfc1f0000, CBD|RD_s|WR_31,         0,              I1      },
{"bltzall", "s,p",      0x04120000, 0xfc1f0000, CBL|RD_s|WR_31,         0,              I2      },
{"bnez",    "s,p",      0x14000000, 0xfc1f0000, CBD|RD_s,               0,              I1      },
{"bnezl",   "s,p",      0x54000000, 0xfc1f0000, CBL|RD_s,               0,              I2      },
{"bne",     "s,t,p",    0x14000000, 0xfc000000, CBD|RD_s|RD_t,          0,              I1      },
{"bnel",    "s,t,p",    0x54000000, 0xfc000000, CBL|RD_s|RD_t,          0,              I2      },
{"break",   "",         0x0000000d, 0xffffffff, TRAP,                   0,              I1      },
{"break",   "c",        0x0000000d, 0xfc00ffff, TRAP,                   0,              I1      },
{"break",   "c,q",      0x0000000d, 0xfc00003f, TRAP,                   0,              I1      },
{"c.f.d",   "S,T",      0x46200030, 0xffe007ff, RD_S|RD_T|WR_CC|FP_D,   0,              I1      },
{"c.f.d",   "M,S,T",    0x46200030, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I4_32   },
{"c.f.s",   "S,T",      0x46000030, 0xffe007ff, RD_S|RD_T|WR_CC|FP_S,   0,              I1      },
{"c.f.s",   "M,S,T",    0x46000030, 0xffe000ff, RD_S|RD_T|WR_CC|FP_S,   0,              I4_32   },
{"c.f.ps",  "M,S,T",    0x46c00030, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I5_33   },
{"c.un.d",  "S,T",      0x46200031, 0xffe007ff, RD_S|RD_T|WR_CC|FP_D,   0,              I1      },
{"c.un.d",  "M,S,T",    0x46200031, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I4_32   },
{"c.un.s",  "S,T",      0x46000031, 0xffe007ff, RD_S|RD_T|WR_CC|FP_S,   0,              I1      },
{"c.un.s",  "M,S,T",    0x46000031, 0xffe000ff, RD_S|RD_T|WR_CC|FP_S,   0,              I4_32   },
{"c.un.ps", "M,S,T",    0x46c00031, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I5_33   },
{"c.eq.d",  "S,T",      0x46200032, 0xffe007ff, RD_S|RD_T|WR_CC|FP_D,   0,              I1      },
{"c.eq.d",  "M,S,T",    0x46200032, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I4_32   },
{"c.eq.s",  "S,T",      0x46000032, 0xffe007ff, RD_S|RD_T|WR_CC|FP_S,   0,              I1      },
{"c.eq.s",  "M,S,T",    0x46000032, 0xffe000ff, RD_S|RD_T|WR_CC|FP_S,   0,              I4_32   },
{"c.eq.ps", "M,S,T",    0x46c00032, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I5_33   },
{"c.ueq.d", "S,T",      0x46200033, 0xffe007ff, RD_S|RD_T|WR_CC|FP_D,   0,              I1      },
{"c.ueq.d", "M,S,T",    0x46200033, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I4_32   },
{"c.ueq.s", "S,T",      0x46000033, 0xffe007ff, RD_S|RD_T|WR_CC|FP_S,   0,              I1      },
{"c.ueq.s", "M,S,T",    0x46000033, 0xffe000ff, RD_S|RD_T|WR_CC|FP_S,   0,              I4_32   },
{"c.ueq.ps","M,S,T",    0x46c00033, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I5_33   },
{"c.olt.d", "S,T",      0x46200034, 0xffe007ff, RD_S|RD_T|WR_CC|FP_D,   0,              I1      },
{"c.olt.d", "M,S,T",    0x46200034, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I4_32   },
{"c.olt.s", "S,T",      0x46000034, 0xffe007ff, RD_S|RD_T|WR_CC|FP_S,   0,              I1      },
{"c.olt.s", "M,S,T",    0x46000034, 0xffe000ff, RD_S|RD_T|WR_CC|FP_S,   0,              I4_32   },
{"c.olt.ps","M,S,T",    0x46c00034, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I5_33   },
{"c.ult.d", "S,T",      0x46200035, 0xffe007ff, RD_S|RD_T|WR_CC|FP_D,   0,              I1      },
{"c.ult.d", "M,S,T",    0x46200035, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I4_32   },
{"c.ult.s", "S,T",      0x46000035, 0xffe007ff, RD_S|RD_T|WR_CC|FP_S,   0,              I1      },
{"c.ult.s", "M,S,T",    0x46000035, 0xffe000ff, RD_S|RD_T|WR_CC|FP_S,   0,              I4_32   },
{"c.ult.ps","M,S,T",    0x46c00035, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I5_33   },
{"c.ole.d", "S,T",      0x46200036, 0xffe007ff, RD_S|RD_T|WR_CC|FP_D,   0,              I1      },
{"c.ole.d", "M,S,T",    0x46200036, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I4_32   },
{"c.ole.s", "S,T",      0x46000036, 0xffe007ff, RD_S|RD_T|WR_CC|FP_S,   0,              I1      },
{"c.ole.s", "M,S,T",    0x46000036, 0xffe000ff, RD_S|RD_T|WR_CC|FP_S,   0,              I4_32   },
{"c.ole.ps","M,S,T",    0x46c00036, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I5_33   },
{"c.ule.d", "S,T",      0x46200037, 0xffe007ff, RD_S|RD_T|WR_CC|FP_D,   0,              I1      },
{"c.ule.d", "M,S,T",    0x46200037, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I4_32   },
{"c.ule.s", "S,T",      0x46000037, 0xffe007ff, RD_S|RD_T|WR_CC|FP_S,   0,              I1      },
{"c.ule.s", "M,S,T",    0x46000037, 0xffe000ff, RD_S|RD_T|WR_CC|FP_S,   0,              I4_32   },
{"c.ule.ps","M,S,T",    0x46c00037, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I5_33   },
{"c.sf.d",  "S,T",      0x46200038, 0xffe007ff, RD_S|RD_T|WR_CC|FP_D,   0,              I1      },
{"c.sf.d",  "M,S,T",    0x46200038, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I4_32   },
{"c.sf.s",  "S,T",      0x46000038, 0xffe007ff, RD_S|RD_T|WR_CC|FP_S,   0,              I1      },
{"c.sf.s",  "M,S,T",    0x46000038, 0xffe000ff, RD_S|RD_T|WR_CC|FP_S,   0,              I4_32   },
{"c.sf.ps", "M,S,T",    0x46c00038, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I5_33   },
{"c.ngle.d","S,T",      0x46200039, 0xffe007ff, RD_S|RD_T|WR_CC|FP_D,   0,              I1      },
{"c.ngle.d","M,S,T",    0x46200039, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I4_32   },
{"c.ngle.s","S,T",      0x46000039, 0xffe007ff, RD_S|RD_T|WR_CC|FP_S,   0,              I1      },
{"c.ngle.s","M,S,T",    0x46000039, 0xffe000ff, RD_S|RD_T|WR_CC|FP_S,   0,              I4_32   },
{"c.ngle.ps","M,S,T",   0x46c00039, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I5_33   },
{"c.seq.d", "S,T",      0x4620003a, 0xffe007ff, RD_S|RD_T|WR_CC|FP_D,   0,              I1      },
{"c.seq.d", "M,S,T",    0x4620003a, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I4_32   },
{"c.seq.s", "S,T",      0x4600003a, 0xffe007ff, RD_S|RD_T|WR_CC|FP_S,   0,              I1      },
{"c.seq.s", "M,S,T",    0x4600003a, 0xffe000ff, RD_S|RD_T|WR_CC|FP_S,   0,              I4_32   },
{"c.seq.ps","M,S,T",    0x46c0003a, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I5_33   },
{"c.ngl.d", "S,T",      0x4620003b, 0xffe007ff, RD_S|RD_T|WR_CC|FP_D,   0,              I1      },
{"c.ngl.d", "M,S,T",    0x4620003b, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I4_32   },
{"c.ngl.s", "S,T",      0x4600003b, 0xffe007ff, RD_S|RD_T|WR_CC|FP_S,   0,              I1      },
{"c.ngl.s", "M,S,T",    0x4600003b, 0xffe000ff, RD_S|RD_T|WR_CC|FP_S,   0,              I4_32   },
{"c.ngl.ps","M,S,T",    0x46c0003b, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I5_33   },
{"c.lt.d",  "S,T",      0x4620003c, 0xffe007ff, RD_S|RD_T|WR_CC|FP_D,   0,              I1      },
{"c.lt.d",  "M,S,T",    0x4620003c, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I4_32   },
{"c.lt.s",  "S,T",      0x4600003c, 0xffe007ff, RD_S|RD_T|WR_CC|FP_S,   0,              I1      },
{"c.lt.s",  "M,S,T",    0x4600003c, 0xffe000ff, RD_S|RD_T|WR_CC|FP_S,   0,              I4_32   },
{"c.lt.ps", "M,S,T",    0x46c0003c, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I5_33   },
{"c.nge.d", "S,T",      0x4620003d, 0xffe007ff, RD_S|RD_T|WR_CC|FP_D,   0,              I1      },
{"c.nge.d", "M,S,T",    0x4620003d, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I4_32   },
{"c.nge.s", "S,T",      0x4600003d, 0xffe007ff, RD_S|RD_T|WR_CC|FP_S,   0,              I1      },
{"c.nge.s", "M,S,T",    0x4600003d, 0xffe000ff, RD_S|RD_T|WR_CC|FP_S,   0,              I4_32   },
{"c.nge.ps","M,S,T",    0x46c0003d, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I5_33   },
{"c.le.d",  "S,T",      0x4620003e, 0xffe007ff, RD_S|RD_T|WR_CC|FP_D,   0,              I1      },
{"c.le.d",  "M,S,T",    0x4620003e, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I4_32   },
{"c.le.s",  "S,T",      0x4600003e, 0xffe007ff, RD_S|RD_T|WR_CC|FP_S,   0,              I1      },
{"c.le.s",  "M,S,T",    0x4600003e, 0xffe000ff, RD_S|RD_T|WR_CC|FP_S,   0,              I4_32   },
{"c.le.ps", "M,S,T",    0x46c0003e, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I5_33   },
{"c.ngt.d", "S,T",      0x4620003f, 0xffe007ff, RD_S|RD_T|WR_CC|FP_D,   0,              I1      },
{"c.ngt.d", "M,S,T",    0x4620003f, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I4_32   },
{"c.ngt.s", "S,T",      0x4600003f, 0xffe007ff, RD_S|RD_T|WR_CC|FP_S,   0,              I1      },
{"c.ngt.s", "M,S,T",    0x4600003f, 0xffe000ff, RD_S|RD_T|WR_CC|FP_S,   0,              I4_32   },
{"c.ngt.ps","M,S,T",    0x46c0003f, 0xffe000ff, RD_S|RD_T|WR_CC|FP_D,   0,              I5_33   },
/* CW4010 instructions which are aliases for the cache instruction.  */
{"cache",   "k,o(b)",   0xbc000000, 0xfc000000, RD_b,                   0,              I3_32   },
{"ceil.l.d", "D,S",     0x4620000a, 0xffff003f, WR_D|RD_S|FP_D,         0,              I3_33   },
{"ceil.l.s", "D,S",     0x4600000a, 0xffff003f, WR_D|RD_S|FP_S|FP_D,    0,              I3_33   },
{"ceil.w.d", "D,S",     0x4620000e, 0xffff003f, WR_D|RD_S|FP_S|FP_D,    0,              I2      },
{"ceil.w.s", "D,S",     0x4600000e, 0xffff003f, WR_D|RD_S|FP_S,         0,              I2      },
{"cfc0",    "t,G",      0x40400000, 0xffe007ff, LCD|WR_t|RD_C0,         0,              I1      },
{"cfc1",    "t,G",      0x44400000, 0xffe007ff, LCD|WR_t|RD_C1|FP_S,    0,              I1      },
{"cfc1",    "t,S",      0x44400000, 0xffe007ff, LCD|WR_t|RD_C1|FP_S,    0,              I1      },
/* cfc2 is at the bottom of the table.  */
/* cfc3 is at the bottom of the table.  */
{"cftc1",   "d,E",      0x41000023, 0xffe007ff, TRAP|LCD|WR_d|RD_C1|FP_S, 0,            MT32    },
{"cftc1",   "d,T",      0x41000023, 0xffe007ff, TRAP|LCD|WR_d|RD_C1|FP_S, 0,            MT32    },
{"cftc2",   "d,E",      0x41000025, 0xffe007ff, TRAP|LCD|WR_d|RD_C2,    0,              MT32    },
{"clo",     "U,s",      0x70000021, 0xfc0007ff, WR_d|WR_t|RD_s,         0,              I32     },
{"clz",     "U,s",      0x70000020, 0xfc0007ff, WR_d|WR_t|RD_s,         0,              I32     },
{"ctc0",    "t,G",      0x40c00000, 0xffe007ff, COD|RD_t|WR_CC,         0,              I1      },
{"ctc1",    "t,G",      0x44c00000, 0xffe007ff, COD|RD_t|WR_CC|FP_S,    0,              I1      },
{"ctc1",    "t,S",      0x44c00000, 0xffe007ff, COD|RD_t|WR_CC|FP_S,    0,              I1      },
/* ctc2 is at the bottom of the table.  */
/* ctc3 is at the bottom of the table.  */
{"cttc1",   "t,g",      0x41800023, 0xffe007ff, TRAP|COD|RD_t|WR_CC|FP_S, 0,            MT32    },
{"cttc1",   "t,S",      0x41800023, 0xffe007ff, TRAP|COD|RD_t|WR_CC|FP_S, 0,            MT32    },
{"cttc2",   "t,g",      0x41800025, 0xffe007ff, TRAP|COD|RD_t|WR_CC,    0,              MT32    },
{"cvt.d.l", "D,S",      0x46a00021, 0xffff003f, WR_D|RD_S|FP_D,         0,              I3_33   },
{"cvt.d.s", "D,S",      0x46000021, 0xffff003f, WR_D|RD_S|FP_S|FP_D,    0,              I1      },
{"cvt.d.w", "D,S",      0x46800021, 0xffff003f, WR_D|RD_S|FP_S|FP_D,    0,              I1      },
{"cvt.l.d", "D,S",      0x46200025, 0xffff003f, WR_D|RD_S|FP_D,         0,              I3_33   },
{"cvt.l.s", "D,S",      0x46000025, 0xffff003f, WR_D|RD_S|FP_S|FP_D,    0,              I3_33   },
{"cvt.s.l", "D,S",      0x46a00020, 0xffff003f, WR_D|RD_S|FP_S|FP_D,    0,              I3_33   },
{"cvt.s.d", "D,S",      0x46200020, 0xffff003f, WR_D|RD_S|FP_S|FP_D,    0,              I1      },
{"cvt.s.w", "D,S",      0x46800020, 0xffff003f, WR_D|RD_S|FP_S,         0,              I1      },
{"cvt.s.pl","D,S",      0x46c00028, 0xffff003f, WR_D|RD_S|FP_S|FP_D,    0,              I5_33   },
{"cvt.s.pu","D,S",      0x46c00020, 0xffff003f, WR_D|RD_S|FP_S|FP_D,    0,              I5_33   },
{"cvt.w.d", "D,S",      0x46200024, 0xffff003f, WR_D|RD_S|FP_S|FP_D,    0,              I1      },
{"cvt.w.s", "D,S",      0x46000024, 0xffff003f, WR_D|RD_S|FP_S,         0,              I1      },
{"cvt.ps.s","D,V,T",    0x46000026, 0xffe0003f, WR_D|RD_S|RD_T|FP_S|FP_D, 0,            I5_33   },
{"dadd",    "d,v,t",    0x0000002c, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              I3      },
{"daddi",   "t,r,j",    0x60000000, 0xfc000000, WR_t|RD_s,              0,              I3      },
{"daddiu",  "t,r,j",    0x64000000, 0xfc000000, WR_t|RD_s,              0,              I3      },
{"daddu",   "d,v,t",    0x0000002d, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              I3      },
{"dclo",    "U,s",      0x70000025, 0xfc0007ff, RD_s|WR_d|WR_t,         0,              I64     },
{"dclz",    "U,s",      0x70000024, 0xfc0007ff, RD_s|WR_d|WR_t,         0,              I64     },
/* dctr and dctw are used on the r5000.  */
{"dctr",    "o(b)",     0xbc050000, 0xfc1f0000, RD_b,                   0,              I3      },
{"dctw",    "o(b)",     0xbc090000, 0xfc1f0000, RD_b,                   0,              I3      },
{"deret",   "",         0x4200001f, 0xffffffff, 0,                      0,              I32     },
{"dext",    "t,r,+A,+C", 0x7c000003, 0xfc00003f, WR_t|RD_s,             0,              I65     },
{"dextm",   "t,r,+A,+G", 0x7c000001, 0xfc00003f, WR_t|RD_s,             0,              I65     },
{"dextu",   "t,r,+E,+H", 0x7c000002, 0xfc00003f, WR_t|RD_s,             0,              I65     },
/* For ddiv, see the comments about div.  */
{"ddiv",    "z,s,t",    0x0000001e, 0xfc00ffff, RD_s|RD_t|WR_HILO,      0,              I3      },
/* For ddivu, see the comments about div.  */
{"ddivu",   "z,s,t",    0x0000001f, 0xfc00ffff, RD_s|RD_t|WR_HILO,      0,              I3      },
{"di",      "",         0x41606000, 0xffffffff, WR_t|WR_C0,             0,              I33     },
{"di",      "t",        0x41606000, 0xffe0ffff, WR_t|WR_C0,             0,              I33     },
{"dins",    "t,r,+A,+B", 0x7c000007, 0xfc00003f, WR_t|RD_s,             0,              I65     },
{"dinsm",   "t,r,+A,+F", 0x7c000005, 0xfc00003f, WR_t|RD_s,             0,              I65     },
{"dinsu",   "t,r,+E,+F", 0x7c000006, 0xfc00003f, WR_t|RD_s,             0,              I65     },
/* The MIPS assembler treats the div opcode with two operands as
   though the first operand appeared twice (the first operand is both
   a source and a destination).  To get the div machine instruction,
   you must use an explicit destination of $0.  */
{"div",     "z,s,t",    0x0000001a, 0xfc00ffff, RD_s|RD_t|WR_HILO,      0,              I1      },
{"div",     "z,t",      0x0000001a, 0xffe0ffff, RD_s|RD_t|WR_HILO,      0,              I1      },
{"div.d",   "D,V,T",    0x46200003, 0xffe0003f, WR_D|RD_S|RD_T|FP_D,    0,              I1      },
{"div.s",   "D,V,T",    0x46000003, 0xffe0003f, WR_D|RD_S|RD_T|FP_S,    0,              I1      },
/* For divu, see the comments about div.  */
{"divu",    "z,s,t",    0x0000001b, 0xfc00ffff, RD_s|RD_t|WR_HILO,      0,              I1      },
{"divu",    "z,t",      0x0000001b, 0xffe0ffff, RD_s|RD_t|WR_HILO,      0,              I1      },
{"dli",     "t,j",      0x24000000, 0xffe00000, WR_t,                   0,              I3      }, /* addiu */
{"dli",     "t,i",      0x34000000, 0xffe00000, WR_t,                   0,              I3      }, /* ori */
{"dmfc0",   "t,G",      0x40200000, 0xffe007ff, LCD|WR_t|RD_C0,         0,              I3      },
{"dmfc0",   "t,+D",     0x40200000, 0xffe007f8, LCD|WR_t|RD_C0,         0,              I64     },
{"dmfc0",   "t,G,H",    0x40200000, 0xffe007f8, LCD|WR_t|RD_C0,         0,              I64     },
{"dmt",     "",         0x41600bc1, 0xffffffff, TRAP,                   0,              MT32    },
{"dmt",     "t",        0x41600bc1, 0xffe0ffff, TRAP|WR_t,              0,              MT32    },
{"dmtc0",   "t,G",      0x40a00000, 0xffe007ff, COD|RD_t|WR_C0|WR_CC,   0,              I3      },
{"dmtc0",   "t,+D",     0x40a00000, 0xffe007f8, COD|RD_t|WR_C0|WR_CC,   0,              I64     },
{"dmtc0",   "t,G,H",    0x40a00000, 0xffe007f8, COD|RD_t|WR_C0|WR_CC,   0,              I64     },
{"dmfc1",   "t,S",      0x44200000, 0xffe007ff, LCD|WR_t|RD_S|FP_D,     0,              I3      },
{"dmfc1",   "t,G",      0x44200000, 0xffe007ff, LCD|WR_t|RD_S|FP_D,     0,              I3      },
{"dmtc1",   "t,S",      0x44a00000, 0xffe007ff, COD|RD_t|WR_S|FP_D,     0,              I3      },
{"dmtc1",   "t,G",      0x44a00000, 0xffe007ff, COD|RD_t|WR_S|FP_D,     0,              I3      },
/* dmfc2 is at the bottom of the table.  */
/* dmtc2 is at the bottom of the table.  */
/* dmfc3 is at the bottom of the table.  */
/* dmtc3 is at the bottom of the table.  */
{"dmult",   "s,t",      0x0000001c, 0xfc00ffff, RD_s|RD_t|WR_HILO,      0,              I3      },
{"dmultu",  "s,t",      0x0000001d, 0xfc00ffff, RD_s|RD_t|WR_HILO,      0,              I3      },
{"dneg",    "d,w",      0x0000002e, 0xffe007ff, WR_d|RD_t,              0,              I3      }, /* dsub 0 */
{"dnegu",   "d,w",      0x0000002f, 0xffe007ff, WR_d|RD_t,              0,              I3      }, /* dsubu 0*/
{"drem",    "z,s,t",    0x0000001e, 0xfc00ffff, RD_s|RD_t|WR_HILO,      0,              I3      },
{"dremu",   "z,s,t",    0x0000001f, 0xfc00ffff, RD_s|RD_t|WR_HILO,      0,              I3      },
{"drorv",   "d,t,s",    0x00000056, 0xfc0007ff, RD_t|RD_s|WR_d,         0,              I65     },
{"dror32",  "d,w,<",    0x0020003e, 0xffe0003f, WR_d|RD_t,              0,              I65     },
{"dsbh",    "d,w",      0x7c0000a4, 0xffe007ff, WR_d|RD_t,              0,              I65     },
{"dshd",    "d,w",      0x7c000164, 0xffe007ff, WR_d|RD_t,              0,              I65     },
{"dsllv",   "d,t,s",    0x00000014, 0xfc0007ff, WR_d|RD_t|RD_s,         0,              I3      },
{"dsll32",  "d,w,<",    0x0000003c, 0xffe0003f, WR_d|RD_t,              0,              I3      },
{"dsll",    "d,w,s",    0x00000014, 0xfc0007ff, WR_d|RD_t|RD_s,         0,              I3      }, /* dsllv */
{"dsll",    "d,w,>",    0x0000003c, 0xffe0003f, WR_d|RD_t,              0,              I3      }, /* dsll32 */
{"dsll",    "d,w,<",    0x00000038, 0xffe0003f, WR_d|RD_t,              0,              I3      },
{"dsrav",   "d,t,s",    0x00000017, 0xfc0007ff, WR_d|RD_t|RD_s,         0,              I3      },
{"dsra32",  "d,w,<",    0x0000003f, 0xffe0003f, WR_d|RD_t,              0,              I3      },
{"dsra",    "d,w,s",    0x00000017, 0xfc0007ff, WR_d|RD_t|RD_s,         0,              I3      }, /* dsrav */
{"dsra",    "d,w,>",    0x0000003f, 0xffe0003f, WR_d|RD_t,              0,              I3      }, /* dsra32 */
{"dsra",    "d,w,<",    0x0000003b, 0xffe0003f, WR_d|RD_t,              0,              I3      },
{"dsrlv",   "d,t,s",    0x00000016, 0xfc0007ff, WR_d|RD_t|RD_s,         0,              I3      },
{"dsrl32",  "d,w,<",    0x0000003e, 0xffe0003f, WR_d|RD_t,              0,              I3      },
{"dsrl",    "d,w,s",    0x00000016, 0xfc0007ff, WR_d|RD_t|RD_s,         0,              I3      }, /* dsrlv */
{"dsrl",    "d,w,>",    0x0000003e, 0xffe0003f, WR_d|RD_t,              0,              I3      }, /* dsrl32 */
{"dsrl",    "d,w,<",    0x0000003a, 0xffe0003f, WR_d|RD_t,              0,              I3      },
{"dsub",    "d,v,t",    0x0000002e, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              I3      },
{"dsubu",   "d,v,t",    0x0000002f, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              I3      },
{"dvpe",    "",         0x41600001, 0xffffffff, TRAP,                   0,              MT32    },
{"dvpe",    "t",        0x41600001, 0xffe0ffff, TRAP|WR_t,              0,              MT32    },
{"ei",      "",         0x41606020, 0xffffffff, WR_t|WR_C0,             0,              I33     },
{"ei",      "t",        0x41606020, 0xffe0ffff, WR_t|WR_C0,             0,              I33     },
{"emt",     "",         0x41600be1, 0xffffffff, TRAP,                   0,              MT32    },
{"emt",     "t",        0x41600be1, 0xffe0ffff, TRAP|WR_t,              0,              MT32    },
{"eret",    "",         0x42000018, 0xffffffff, 0,                      0,              I3_32   },
{"evpe",    "",         0x41600021, 0xffffffff, TRAP,                   0,              MT32    },
{"evpe",    "t",        0x41600021, 0xffe0ffff, TRAP|WR_t,              0,              MT32    },
{"ext",     "t,r,+A,+C", 0x7c000000, 0xfc00003f, WR_t|RD_s,             0,              I33     },
{"floor.l.d", "D,S",    0x4620000b, 0xffff003f, WR_D|RD_S|FP_D,         0,              I3_33   },
{"floor.l.s", "D,S",    0x4600000b, 0xffff003f, WR_D|RD_S|FP_S|FP_D,    0,              I3_33   },
{"floor.w.d", "D,S",    0x4620000f, 0xffff003f, WR_D|RD_S|FP_S|FP_D,    0,              I2      },
{"floor.w.s", "D,S",    0x4600000f, 0xffff003f, WR_D|RD_S|FP_S,         0,              I2      },
{"ins",     "t,r,+A,+B", 0x7c000004, 0xfc00003f, WR_t|RD_s,             0,              I33     },
{"jr",      "s",        0x00000008, 0xfc1fffff, UBD|RD_s,               0,              I1      },
/* jr.hb is officially MIPS{32,64}R2, but it works on R1 as jr with
   the same hazard barrier effect.  */
{"jr.hb",   "s",        0x00000408, 0xfc1fffff, UBD|RD_s,               0,              I32     },
{"j",       "s",        0x00000008, 0xfc1fffff, UBD|RD_s,               0,              I1      }, /* jr */
/* This form of j is used by the disassembler and internally by the
   assembler, but will never match user input (because the line above
   will match first).  */
{"j",       "a",        0x08000000, 0xfc000000, UBD,                    0,              I1      },
{"jalr",    "s",        0x0000f809, 0xfc1fffff, UBD|RD_s|WR_d,          0,              I1      },
{"jalr",    "d,s",      0x00000009, 0xfc1f07ff, UBD|RD_s|WR_d,          0,              I1      },
/* jalr.hb is officially MIPS{32,64}R2, but it works on R1 as jalr
   with the same hazard barrier effect.  */
{"jalr.hb", "s",        0x0000fc09, 0xfc1fffff, UBD|RD_s|WR_d,          0,              I32     },
{"jalr.hb", "d,s",      0x00000409, 0xfc1f07ff, UBD|RD_s|WR_d,          0,              I32     },
/* This form of jal is used by the disassembler and internally by the
   assembler, but will never match user input (because the line above
   will match first).  */
{"jal",     "a",        0x0c000000, 0xfc000000, UBD|WR_31,              0,              I1      },
{"jalx",    "a",        0x74000000, 0xfc000000, UBD|WR_31,              0,              I1      },
{"lb",      "t,o(b)",   0x80000000, 0xfc000000, LDD|RD_b|WR_t,          0,              I1      },
{"lbu",     "t,o(b)",   0x90000000, 0xfc000000, LDD|RD_b|WR_t,          0,              I1      },
{"ld",      "t,o(b)",   0xdc000000, 0xfc000000, WR_t|RD_b,              0,              I3      },
{"ldc1",    "T,o(b)",   0xd4000000, 0xfc000000, CLD|RD_b|WR_T|FP_D,     0,              I2      },
{"ldc1",    "E,o(b)",   0xd4000000, 0xfc000000, CLD|RD_b|WR_T|FP_D,     0,              I2      },
{"l.d",     "T,o(b)",   0xd4000000, 0xfc000000, CLD|RD_b|WR_T|FP_D,     0,              I2      }, /* ldc1 */
{"ldc2",    "E,o(b)",   0xd8000000, 0xfc000000, CLD|RD_b|WR_CC,         0,              I2      },
{"ldc3",    "E,o(b)",   0xdc000000, 0xfc000000, CLD|RD_b|WR_CC,         0,              I2      },
{"ldl",     "t,o(b)",   0x68000000, 0xfc000000, LDD|WR_t|RD_b,          0,              I3      },
{"ldr",     "t,o(b)",   0x6c000000, 0xfc000000, LDD|WR_t|RD_b,          0,              I3      },
{"ldxc1",   "D,t(b)",   0x4c000001, 0xfc00f83f, LDD|WR_D|RD_t|RD_b|FP_D, 0,             I4_33   },
{"lh",      "t,o(b)",   0x84000000, 0xfc000000, LDD|RD_b|WR_t,          0,              I1      },
{"lhu",     "t,o(b)",   0x94000000, 0xfc000000, LDD|RD_b|WR_t,          0,              I1      },
{"ll",      "t,o(b)",   0xc0000000, 0xfc000000, LDD|RD_b|WR_t,          0,              I2      },
{"lld",     "t,o(b)",   0xd0000000, 0xfc000000, LDD|RD_b|WR_t,          0,              I3      },
{"lui",     "t,u",      0x3c000000, 0xffe00000, WR_t,                   0,              I1      },
{"luxc1",   "D,t(b)",   0x4c000005, 0xfc00f83f, LDD|WR_D|RD_t|RD_b|FP_D, 0,             I5_33   },
{"lw",      "t,o(b)",   0x8c000000, 0xfc000000, LDD|RD_b|WR_t,          0,              I1      },
{"lwc0",    "E,o(b)",   0xc0000000, 0xfc000000, CLD|RD_b|WR_CC,         0,              I1      },
{"lwc1",    "T,o(b)",   0xc4000000, 0xfc000000, CLD|RD_b|WR_T|FP_S,     0,              I1      },
{"lwc1",    "E,o(b)",   0xc4000000, 0xfc000000, CLD|RD_b|WR_T|FP_S,     0,              I1      },
{"l.s",     "T,o(b)",   0xc4000000, 0xfc000000, CLD|RD_b|WR_T|FP_S,     0,              I1      }, /* lwc1 */
{"lwc2",    "E,o(b)",   0xc8000000, 0xfc000000, CLD|RD_b|WR_CC,         0,              I1      },
{"lwc3",    "E,o(b)",   0xcc000000, 0xfc000000, CLD|RD_b|WR_CC,         0,              I1      },
{"lwl",     "t,o(b)",   0x88000000, 0xfc000000, LDD|RD_b|WR_t,          0,              I1      },
{"lcache",  "t,o(b)",   0x88000000, 0xfc000000, LDD|RD_b|WR_t,          0,              I2      }, /* same */
{"lwr",     "t,o(b)",   0x98000000, 0xfc000000, LDD|RD_b|WR_t,          0,              I1      },
{"flush",   "t,o(b)",   0x98000000, 0xfc000000, LDD|RD_b|WR_t,          0,              I2      }, /* same */
{"fork",    "d,s,t",    0x7c000008, 0xfc0007ff, TRAP|WR_d|RD_s|RD_t,    0,              MT32    },
{"lwu",     "t,o(b)",   0x9c000000, 0xfc000000, LDD|RD_b|WR_t,          0,              I3      },
{"lwxc1",   "D,t(b)",   0x4c000000, 0xfc00f83f, LDD|WR_D|RD_t|RD_b|FP_S, 0,             I4_33   },
{"lwxs",    "d,t(b)",   0x70000088, 0xfc0007ff, LDD|RD_b|RD_t|WR_d,     0,              SMT     },
{"madd.d",  "D,R,S,T",  0x4c000021, 0xfc00003f, RD_R|RD_S|RD_T|WR_D|FP_D,    0,         I4_33   },
{"madd.s",  "D,R,S,T",  0x4c000020, 0xfc00003f, RD_R|RD_S|RD_T|WR_D|FP_S,    0,         I4_33   },
{"madd.ps", "D,R,S,T",  0x4c000026, 0xfc00003f, RD_R|RD_S|RD_T|WR_D|FP_D,    0,         I5_33   },
{"madd",    "s,t",      0x70000000, 0xfc00ffff, RD_s|RD_t|MOD_HILO,          0,         I32     },
{"madd",    "7,s,t",    0x70000000, 0xfc00e7ff, MOD_a|RD_s|RD_t,             0,         D32     },
{"maddp",   "s,t",      0x70000441, 0xfc00ffff, RD_s|RD_t|MOD_HILO,          0,         SMT     },
{"maddu",   "s,t",      0x70000001, 0xfc00ffff, RD_s|RD_t|MOD_HILO,          0,         I32     },
{"maddu",   "7,s,t",    0x70000001, 0xfc00e7ff, MOD_a|RD_s|RD_t,             0,         D32     },
{"mftacx",  "d",        0x41020021, 0xffff07ff, TRAP|WR_d|RD_a,         0,              MT32    },
{"mftacx",  "d,*",      0x41020021, 0xfff307ff, TRAP|WR_d|RD_a,         0,              MT32    },
{"mftc0",   "d,+t",     0x41000000, 0xffe007ff, TRAP|LCD|WR_d|RD_C0,    0,              MT32    },
{"mftc0",   "d,+T",     0x41000000, 0xffe007f8, TRAP|LCD|WR_d|RD_C0,    0,              MT32    },
{"mftc0",   "d,E,H",    0x41000000, 0xffe007f8, TRAP|LCD|WR_d|RD_C0,    0,              MT32    },
{"mftc1",   "d,T",      0x41000022, 0xffe007ff, TRAP|LCD|WR_d|RD_T|FP_S, 0,             MT32    },
{"mftc1",   "d,E",      0x41000022, 0xffe007ff, TRAP|LCD|WR_d|RD_T|FP_S, 0,             MT32    },
{"mftc2",   "d,E",      0x41000024, 0xffe007ff, TRAP|LCD|WR_d|RD_C2,    0,              MT32    },
{"mftdsp",  "d",        0x41100021, 0xffff07ff, TRAP|WR_d,              0,              MT32    },
{"mftgpr",  "d,t",      0x41000020, 0xffe007ff, TRAP|WR_d|RD_t,         0,              MT32    },
{"mfthc1",  "d,T",      0x41000032, 0xffe007ff, TRAP|LCD|WR_d|RD_T|FP_D, 0,             MT32    },
{"mfthc1",  "d,E",      0x41000032, 0xffe007ff, TRAP|LCD|WR_d|RD_T|FP_D, 0,             MT32    },
{"mfthc2",  "d,E",      0x41000034, 0xffe007ff, TRAP|LCD|WR_d|RD_C2,    0,              MT32    },
{"mfthi",   "d",        0x41010021, 0xffff07ff, TRAP|WR_d|RD_a,         0,              MT32    },
{"mfthi",   "d,*",      0x41010021, 0xfff307ff, TRAP|WR_d|RD_a,         0,              MT32    },
{"mftlo",   "d",        0x41000021, 0xffff07ff, TRAP|WR_d|RD_a,         0,              MT32    },
{"mftlo",   "d,*",      0x41000021, 0xfff307ff, TRAP|WR_d|RD_a,         0,              MT32    },
{"mftr",    "d,t,!,H,$", 0x41000000, 0xffe007c8, TRAP|WR_d,             0,              MT32    },
{"mfc0",    "t,G",      0x40000000, 0xffe007ff, LCD|WR_t|RD_C0,         0,              I1      },
{"mfc0",    "t,+D",     0x40000000, 0xffe007f8, LCD|WR_t|RD_C0,         0,              I32     },
{"mfc0",    "t,G,H",    0x40000000, 0xffe007f8, LCD|WR_t|RD_C0,         0,              I32     },
{"mfc1",    "t,S",      0x44000000, 0xffe007ff, LCD|WR_t|RD_S|FP_S,     0,              I1      },
{"mfc1",    "t,G",      0x44000000, 0xffe007ff, LCD|WR_t|RD_S|FP_S,     0,              I1      },
{"mfhc1",   "t,S",      0x44600000, 0xffe007ff, LCD|WR_t|RD_S|FP_D,     0,              I33     },
{"mfhc1",   "t,G",      0x44600000, 0xffe007ff, LCD|WR_t|RD_S|FP_D,     0,              I33     },
/* mfc2 is at the bottom of the table.  */
/* mfhc2 is at the bottom of the table.  */
/* mfc3 is at the bottom of the table.  */
{"mfhi",    "d",        0x00000010, 0xffff07ff, WR_d|RD_HI,             0,              I1      },
{"mfhi",    "d,9",      0x00000010, 0xff9f07ff, WR_d|RD_HI,             0,              D32     },
{"mflo",    "d",        0x00000012, 0xffff07ff, WR_d|RD_LO,             0,              I1      },
{"mflo",    "d,9",      0x00000012, 0xff9f07ff, WR_d|RD_LO,             0,              D32     },
{"mflhxu",  "d",        0x00000052, 0xffff07ff, WR_d|MOD_HILO,          0,              SMT     },
{"mov.d",   "D,S",      0x46200006, 0xffff003f, WR_D|RD_S|FP_D,         0,              I1      },
{"mov.s",   "D,S",      0x46000006, 0xffff003f, WR_D|RD_S|FP_S,         0,              I1      },
{"movf",    "d,s,N",    0x00000001, 0xfc0307ff, WR_d|RD_s|RD_CC|FP_S|FP_D, 0,           I4_32   },
{"movf.d",  "D,S,N",    0x46200011, 0xffe3003f, WR_D|RD_S|RD_CC|FP_D,   0,              I4_32   },
{"movf.s",  "D,S,N",    0x46000011, 0xffe3003f, WR_D|RD_S|RD_CC|FP_S,   0,              I4_32   },
{"movf.ps", "D,S,N",    0x46c00011, 0xffe3003f, WR_D|RD_S|RD_CC|FP_D,   0,              I5_33   },
{"movn",    "d,v,t",    0x0000000b, 0xfc0007ff, WR_d|RD_s|RD_t, 	0,		I4_32	},
{"movn.d",  "D,S,t",    0x46200013, 0xffe0003f, WR_D|RD_S|RD_t|FP_D,    0,              I4_32   },
{"movn.s",  "D,S,t",    0x46000013, 0xffe0003f, WR_D|RD_S|RD_t|FP_S,    0,              I4_32   },
{"movn.ps", "D,S,t",    0x46c00013, 0xffe0003f, WR_D|RD_S|RD_t|FP_D,    0,              I5_33   },
{"movt",    "d,s,N",    0x00010001, 0xfc0307ff, WR_d|RD_s|RD_CC|FP_S|FP_D, 0,           I4_32   },
{"movt.d",  "D,S,N",    0x46210011, 0xffe3003f, WR_D|RD_S|RD_CC|FP_D,   0,              I4_32   },
{"movt.s",  "D,S,N",    0x46010011, 0xffe3003f, WR_D|RD_S|RD_CC|FP_S,   0,              I4_32   },
{"movt.ps", "D,S,N",    0x46c10011, 0xffe3003f, WR_D|RD_S|RD_CC|FP_D,   0,              I5_33   },
{"movz",    "d,v,t",    0x0000000a, 0xfc0007ff, WR_d|RD_s|RD_t, 	0,		I4_32	},
{"movz.d",  "D,S,t",    0x46200012, 0xffe0003f, WR_D|RD_S|RD_t|FP_D,    0,              I4_32   },
{"movz.s",  "D,S,t",    0x46000012, 0xffe0003f, WR_D|RD_S|RD_t|FP_S,    0,              I4_32   },
{"movz.ps", "D,S,t",    0x46c00012, 0xffe0003f, WR_D|RD_S|RD_t|FP_D,    0,              I5_33   },
/* move is at the top of the table.  */
{"msub.d",  "D,R,S,T",  0x4c000029, 0xfc00003f, RD_R|RD_S|RD_T|WR_D|FP_D, 0,            I4_33   },
{"msub.s",  "D,R,S,T",  0x4c000028, 0xfc00003f, RD_R|RD_S|RD_T|WR_D|FP_S, 0,            I4_33   },
{"msub.ps", "D,R,S,T",  0x4c00002e, 0xfc00003f, RD_R|RD_S|RD_T|WR_D|FP_D, 0,            I5_33   },
{"msub",    "s,t",      0x70000004, 0xfc00ffff, RD_s|RD_t|MOD_HILO,     0,              I32     },
{"msub",    "7,s,t",    0x70000004, 0xfc00e7ff, MOD_a|RD_s|RD_t,        0,              D32     },
{"msubu",   "s,t",      0x70000005, 0xfc00ffff, RD_s|RD_t|MOD_HILO,     0,              I32     },
{"msubu",   "7,s,t",    0x70000005, 0xfc00e7ff, MOD_a|RD_s|RD_t,        0,              D32     },
{"mtc0",    "t,G",      0x40800000, 0xffe007ff, COD|RD_t|WR_C0|WR_CC,   0,              I1      },
{"mtc0",    "t,+D",     0x40800000, 0xffe007f8, COD|RD_t|WR_C0|WR_CC,   0,              I32     },
{"mtc0",    "t,G,H",    0x40800000, 0xffe007f8, COD|RD_t|WR_C0|WR_CC,   0,              I32     },
{"mtc1",    "t,S",      0x44800000, 0xffe007ff, COD|RD_t|WR_S|FP_S,     0,              I1      },
{"mtc1",    "t,G",      0x44800000, 0xffe007ff, COD|RD_t|WR_S|FP_S,     0,              I1      },
{"mthc1",   "t,S",      0x44e00000, 0xffe007ff, COD|RD_t|WR_S|FP_D,     0,              I33     },
{"mthc1",   "t,G",      0x44e00000, 0xffe007ff, COD|RD_t|WR_S|FP_D,     0,              I33     },
/* mtc2 is at the bottom of the table.  */
/* mthc2 is at the bottom of the table.  */
/* mtc3 is at the bottom of the table.  */
{"mthi",    "s",        0x00000011, 0xfc1fffff, RD_s|WR_HI,             0,              I1      },
{"mthi",    "s,7",      0x00000011, 0xfc1fe7ff, RD_s|WR_HI,             0,              D32     },
{"mtlo",    "s",        0x00000013, 0xfc1fffff, RD_s|WR_LO,             0,              I1      },
{"mtlo",    "s,7",      0x00000013, 0xfc1fe7ff, RD_s|WR_LO,             0,              D32     },
{"mtlhx",   "s",        0x00000053, 0xfc1fffff, RD_s|MOD_HILO,          0,              SMT     },
{"mttc0",   "t,G",      0x41800000, 0xffe007ff, TRAP|COD|RD_t|WR_C0|WR_CC, 0,           MT32    },
{"mttc0",   "t,+D",     0x41800000, 0xffe007f8, TRAP|COD|RD_t|WR_C0|WR_CC, 0,           MT32    },
{"mttc0",   "t,G,H",    0x41800000, 0xffe007f8, TRAP|COD|RD_t|WR_C0|WR_CC, 0,           MT32    },
{"mttc1",   "t,S",      0x41800022, 0xffe007ff, TRAP|COD|RD_t|WR_S|FP_S, 0,             MT32    },
{"mttc1",   "t,G",      0x41800022, 0xffe007ff, TRAP|COD|RD_t|WR_S|FP_S, 0,             MT32    },
{"mttc2",   "t,g",      0x41800024, 0xffe007ff, TRAP|COD|RD_t|WR_C2|WR_CC, 0,           MT32    },
{"mttacx",  "t",        0x41801021, 0xffe0ffff, TRAP|WR_a|RD_t,         0,              MT32    },
{"mttacx",  "t,&",      0x41801021, 0xffe09fff, TRAP|WR_a|RD_t,         0,              MT32    },
{"mttdsp",  "t",        0x41808021, 0xffe0ffff, TRAP|RD_t,              0,              MT32    },
{"mttgpr",  "t,d",      0x41800020, 0xffe007ff, TRAP|WR_d|RD_t,         0,              MT32    },
{"mtthc1",  "t,S",      0x41800032, 0xffe007ff, TRAP|COD|RD_t|WR_S|FP_D, 0,             MT32    },
{"mtthc1",  "t,G",      0x41800032, 0xffe007ff, TRAP|COD|RD_t|WR_S|FP_D, 0,             MT32    },
{"mtthc2",  "t,g",      0x41800034, 0xffe007ff, TRAP|COD|RD_t|WR_C2|WR_CC, 0,           MT32    },
{"mtthi",   "t",        0x41800821, 0xffe0ffff, TRAP|WR_a|RD_t,         0,              MT32    },
{"mtthi",   "t,&",      0x41800821, 0xffe09fff, TRAP|WR_a|RD_t,         0,              MT32    },
{"mttlo",   "t",        0x41800021, 0xffe0ffff, TRAP|WR_a|RD_t,         0,              MT32    },
{"mttlo",   "t,&",      0x41800021, 0xffe09fff, TRAP|WR_a|RD_t,         0,              MT32    },
{"mttr",    "t,d,!,H,$", 0x41800000, 0xffe007c8, TRAP|RD_t,             0,              MT32    },
{"mul.d",   "D,V,T",    0x46200002, 0xffe0003f, WR_D|RD_S|RD_T|FP_D,    0,              I1      },
{"mul.s",   "D,V,T",    0x46000002, 0xffe0003f, WR_D|RD_S|RD_T|FP_S,    0,              I1      },
{"mul",     "d,v,t",    0x70000002, 0xfc0007ff, WR_d|RD_s|RD_t|WR_HILO, 0,              I32     },
{"mult",    "s,t",      0x00000018, 0xfc00ffff, RD_s|RD_t|WR_HILO|IS_M, 0,              I1      },
{"mult",    "7,s,t",    0x00000018, 0xfc00e7ff, WR_a|RD_s|RD_t,         0,              D32     },
{"multp",   "s,t",      0x00000459, 0xfc00ffff, RD_s|RD_t|MOD_HILO,     0,              SMT     },
{"multu",   "s,t",      0x00000019, 0xfc00ffff, RD_s|RD_t|WR_HILO|IS_M, 0,              I1      },
{"multu",   "7,s,t",    0x00000019, 0xfc00e7ff, WR_a|RD_s|RD_t,         0,              D32     },
{"neg",     "d,w",      0x00000022, 0xffe007ff, WR_d|RD_t,              0,              I1      }, /* sub 0 */
{"negu",    "d,w",      0x00000023, 0xffe007ff, WR_d|RD_t,              0,              I1      }, /* subu 0 */
{"neg.d",   "D,V",      0x46200007, 0xffff003f, WR_D|RD_S|FP_D,         0,              I1      },
{"neg.s",   "D,V",      0x46000007, 0xffff003f, WR_D|RD_S|FP_S,         0,              I1      },
{"nmadd.d", "D,R,S,T",  0x4c000031, 0xfc00003f, RD_R|RD_S|RD_T|WR_D|FP_D, 0,            I4_33   },
{"nmadd.s", "D,R,S,T",  0x4c000030, 0xfc00003f, RD_R|RD_S|RD_T|WR_D|FP_S, 0,            I4_33   },
{"nmadd.ps","D,R,S,T",  0x4c000036, 0xfc00003f, RD_R|RD_S|RD_T|WR_D|FP_D, 0,            I5_33   },
{"nmsub.d", "D,R,S,T",  0x4c000039, 0xfc00003f, RD_R|RD_S|RD_T|WR_D|FP_D, 0,            I4_33   },
{"nmsub.s", "D,R,S,T",  0x4c000038, 0xfc00003f, RD_R|RD_S|RD_T|WR_D|FP_S, 0,            I4_33   },
{"nmsub.ps","D,R,S,T",  0x4c00003e, 0xfc00003f, RD_R|RD_S|RD_T|WR_D|FP_D, 0,            I5_33   },
/* nop is at the start of the table.  */
{"nor",     "d,v,t",    0x00000027, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              I1      },
{"not",     "d,v",      0x00000027, 0xfc1f07ff, WR_d|RD_s|RD_t,         0,              I1      },/*nor d,s,0*/
{"or",      "d,v,t",    0x00000025, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              I1      },
{"ori",     "t,r,i",    0x34000000, 0xfc000000, WR_t|RD_s,              0,              I1      },
{"pll.ps",  "D,V,T",    0x46c0002c, 0xffe0003f, WR_D|RD_S|RD_T|FP_D,    0,              I5_33   },
{"plu.ps",  "D,V,T",    0x46c0002d, 0xffe0003f, WR_D|RD_S|RD_T|FP_D,    0,              I5_33   },
  /* pref and prefx are at the start of the table.  */
{"pul.ps",  "D,V,T",    0x46c0002e, 0xffe0003f, WR_D|RD_S|RD_T|FP_D,    0,              I5_33   },
{"puu.ps",  "D,V,T",    0x46c0002f, 0xffe0003f, WR_D|RD_S|RD_T|FP_D,    0,              I5_33   },
{"pperm",   "s,t",      0x70000481, 0xfc00ffff, MOD_HILO|RD_s|RD_t,     0,              SMT     },
{"recip.d", "D,S",      0x46200015, 0xffff003f, WR_D|RD_S|FP_D,         0,              I4_33   },
{"recip.s", "D,S",      0x46000015, 0xffff003f, WR_D|RD_S|FP_S,         0,              I4_33   },
{"rem",     "z,s,t",    0x0000001a, 0xfc00ffff, RD_s|RD_t|WR_HILO,      0,              I1      },
{"remu",    "z,s,t",    0x0000001b, 0xfc00ffff, RD_s|RD_t|WR_HILO,      0,              I1      },
{"rdhwr",   "t,K",      0x7c00003b, 0xffe007ff, WR_t,                   0,              I33     },
{"rdpgpr",  "d,w",      0x41400000, 0xffe007ff, WR_d,                   0,              I33     },
{"rfe",     "",         0x42000010, 0xffffffff, 0,                      0,              I1      },
{"ror",     "d,w,<",    0x00200002, 0xffe0003f, WR_d|RD_t,              0,              I33|SMT },
{"rorv",    "d,t,s",    0x00000046, 0xfc0007ff, RD_t|RD_s|WR_d,         0,              I33|SMT },
{"round.l.d", "D,S",    0x46200008, 0xffff003f, WR_D|RD_S|FP_D,         0,              I3_33   },
{"round.l.s", "D,S",    0x46000008, 0xffff003f, WR_D|RD_S|FP_S|FP_D,    0,              I3_33   },
{"round.w.d", "D,S",    0x4620000c, 0xffff003f, WR_D|RD_S|FP_S|FP_D,    0,              I2      },
{"round.w.s", "D,S",    0x4600000c, 0xffff003f, WR_D|RD_S|FP_S,         0,              I2      },
{"rsqrt.d", "D,S",      0x46200016, 0xffff003f, WR_D|RD_S|FP_D,         0,              I4_33   },
{"rsqrt.s", "D,S",      0x46000016, 0xffff003f, WR_D|RD_S|FP_S,         0,              I4_33   },
{"sb",      "t,o(b)",   0xa0000000, 0xfc000000, SM|RD_t|RD_b,           0,              I1      },
{"sc",      "t,o(b)",   0xe0000000, 0xfc000000, SM|RD_t|WR_t|RD_b,      0,              I2      },
{"scd",     "t,o(b)",   0xf0000000, 0xfc000000, SM|RD_t|WR_t|RD_b,      0,              I3      },
{"sd",      "t,o(b)",   0xfc000000, 0xfc000000, SM|RD_t|RD_b,           0,              I3      },
{"sdbbp",   "",         0x7000003f, 0xffffffff, TRAP,                   0,              I32     },
{"sdbbp",   "B",        0x7000003f, 0xfc00003f, TRAP,                   0,              I32     },
{"sdc1",    "T,o(b)",   0xf4000000, 0xfc000000, SM|RD_T|RD_b|FP_D,      0,              I2      },
{"sdc1",    "E,o(b)",   0xf4000000, 0xfc000000, SM|RD_T|RD_b|FP_D,      0,              I2      },
{"sdc2",    "E,o(b)",   0xf8000000, 0xfc000000, SM|RD_C2|RD_b,          0,              I2      },
{"sdc3",    "E,o(b)",   0xfc000000, 0xfc000000, SM|RD_C3|RD_b,          0,              I2      },
{"s.d",     "T,o(b)",   0xf4000000, 0xfc000000, SM|RD_T|RD_b|FP_D,      0,              I2      },
{"sdl",     "t,o(b)",   0xb0000000, 0xfc000000, SM|RD_t|RD_b,           0,              I3      },
{"sdr",     "t,o(b)",   0xb4000000, 0xfc000000, SM|RD_t|RD_b,           0,              I3      },
{"sdxc1",   "S,t(b)",   0x4c000009, 0xfc0007ff, SM|RD_S|RD_t|RD_b|FP_D, 0,              I4_33   },
{"seb",     "d,w",      0x7c000420, 0xffe007ff, WR_d|RD_t,              0,              I33     },
{"seh",     "d,w",      0x7c000620, 0xffe007ff, WR_d|RD_t,              0,              I33     },
{"sh",      "t,o(b)",   0xa4000000, 0xfc000000, SM|RD_t|RD_b,           0,              I1      },
{"sllv",    "d,t,s",    0x00000004, 0xfc0007ff, WR_d|RD_t|RD_s,         0,              I1      },
{"sll",     "d,w,s",    0x00000004, 0xfc0007ff, WR_d|RD_t|RD_s,         0,              I1      }, /* sllv */
{"sll",     "d,w,<",    0x00000000, 0xffe0003f, WR_d|RD_t,              0,              I1      },
{"slt",     "d,v,t",    0x0000002a, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              I1      },
{"slti",    "t,r,j",    0x28000000, 0xfc000000, WR_t|RD_s,              0,              I1      },
{"sltiu",   "t,r,j",    0x2c000000, 0xfc000000, WR_t|RD_s,              0,              I1      },
{"sltu",    "d,v,t",    0x0000002b, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              I1      },
{"sqrt.d",  "D,S",      0x46200004, 0xffff003f, WR_D|RD_S|FP_D,         0,              I2      },
{"sqrt.s",  "D,S",      0x46000004, 0xffff003f, WR_D|RD_S|FP_S,         0,              I2      },
{"srav",    "d,t,s",    0x00000007, 0xfc0007ff, WR_d|RD_t|RD_s,         0,              I1      },
{"sra",     "d,w,s",    0x00000007, 0xfc0007ff, WR_d|RD_t|RD_s,         0,              I1      }, /* srav */
{"sra",     "d,w,<",    0x00000003, 0xffe0003f, WR_d|RD_t,              0,              I1      },
{"srlv",    "d,t,s",    0x00000006, 0xfc0007ff, WR_d|RD_t|RD_s,         0,              I1      },
{"srl",     "d,w,s",    0x00000006, 0xfc0007ff, WR_d|RD_t|RD_s,         0,              I1      }, /* srlv */
{"srl",     "d,w,<",    0x00000002, 0xffe0003f, WR_d|RD_t,              0,              I1      },
/* ssnop is at the start of the table.  */
{"sub",     "d,v,t",    0x00000022, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              I1      },
{"sub.d",   "D,V,T",    0x46200001, 0xffe0003f, WR_D|RD_S|RD_T|FP_D,    0,              I1      },
{"sub.s",   "D,V,T",    0x46000001, 0xffe0003f, WR_D|RD_S|RD_T|FP_S,    0,              I1      },
{"subu",    "d,v,t",    0x00000023, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              I1      },
{"suxc1",   "S,t(b)",   0x4c00000d, 0xfc0007ff, SM|RD_S|RD_t|RD_b|FP_D, 0,              I5_33   },
{"sw",      "t,o(b)",   0xac000000, 0xfc000000, SM|RD_t|RD_b,           0,              I1      },
{"swc0",    "E,o(b)",   0xe0000000, 0xfc000000, SM|RD_C0|RD_b,          0,              I1      },
{"swc1",    "T,o(b)",   0xe4000000, 0xfc000000, SM|RD_T|RD_b|FP_S,      0,              I1      },
{"swc1",    "E,o(b)",   0xe4000000, 0xfc000000, SM|RD_T|RD_b|FP_S,      0,              I1      },
{"s.s",     "T,o(b)",   0xe4000000, 0xfc000000, SM|RD_T|RD_b|FP_S,      0,              I1      }, /* swc1 */
{"swc2",    "E,o(b)",   0xe8000000, 0xfc000000, SM|RD_C2|RD_b,          0,              I1      },
{"swc3",    "E,o(b)",   0xec000000, 0xfc000000, SM|RD_C3|RD_b,          0,              I1      },
{"swl",     "t,o(b)",   0xa8000000, 0xfc000000, SM|RD_t|RD_b,           0,              I1      },
{"scache",  "t,o(b)",   0xa8000000, 0xfc000000, RD_t|RD_b,              0,              I2      }, /* same */
{"swr",     "t,o(b)",   0xb8000000, 0xfc000000, SM|RD_t|RD_b,           0,              I1      },
{"invalidate", "t,o(b)",0xb8000000, 0xfc000000, RD_t|RD_b,              0,              I2      }, /* same */
{"swxc1",   "S,t(b)",   0x4c000008, 0xfc0007ff, SM|RD_S|RD_t|RD_b|FP_S, 0,              I4_33   },
{"sync_acquire", "",    0x0000044f, 0xffffffff, INSN_SYNC,              0,              I33     },
{"sync_mb", "",         0x0000040f, 0xffffffff, INSN_SYNC,              0,              I33     },
{"sync_release", "",    0x0000048f, 0xffffffff, INSN_SYNC,              0,              I33     },
{"sync_rmb", "",        0x000004cf, 0xffffffff, INSN_SYNC,              0,              I33     },
{"sync_wmb", "",        0x0000010f, 0xffffffff, INSN_SYNC,              0,              I33     },
{"sync",    "",         0x0000000f, 0xffffffff, INSN_SYNC,              0,              I2      },
{"sync",    "1",        0x0000000f, 0xfffff83f, INSN_SYNC,              0,              I32     },
{"sync.p",  "",         0x0000040f, 0xffffffff, INSN_SYNC,              0,              I2      },
{"sync.l",  "",         0x0000000f, 0xffffffff, INSN_SYNC,              0,              I2      },
{"synci",   "o(b)",     0x041f0000, 0xfc1f0000, SM|RD_b,                0,              I33     },
{"syscall", "",         0x0000000c, 0xffffffff, TRAP,                   0,              I1      },
{"syscall", "B",        0x0000000c, 0xfc00003f, TRAP,                   0,              I1      },
{"teqi",    "s,j",      0x040c0000, 0xfc1f0000, RD_s|TRAP,              0,              I2      },
{"teq",     "s,t",      0x00000034, 0xfc00ffff, RD_s|RD_t|TRAP,         0,              I2      },
{"teq",     "s,t,q",    0x00000034, 0xfc00003f, RD_s|RD_t|TRAP,         0,              I2      },
{"teq",     "s,j",      0x040c0000, 0xfc1f0000, RD_s|TRAP,              0,              I2      }, /* teqi */
{"tgei",    "s,j",      0x04080000, 0xfc1f0000, RD_s|TRAP,              0,              I2      },
{"tge",     "s,t",      0x00000030, 0xfc00ffff, RD_s|RD_t|TRAP,         0,              I2      },
{"tge",     "s,t,q",    0x00000030, 0xfc00003f, RD_s|RD_t|TRAP,         0,              I2      },
{"tge",     "s,j",      0x04080000, 0xfc1f0000, RD_s|TRAP,              0,              I2      }, /* tgei */
{"tgeiu",   "s,j",      0x04090000, 0xfc1f0000, RD_s|TRAP,              0,              I2      },
{"tgeu",    "s,t",      0x00000031, 0xfc00ffff, RD_s|RD_t|TRAP,         0,              I2      },
{"tgeu",    "s,t,q",    0x00000031, 0xfc00003f, RD_s|RD_t|TRAP,         0,              I2      },
{"tgeu",    "s,j",      0x04090000, 0xfc1f0000, RD_s|TRAP,              0,              I2      }, /* tgeiu */
{"tlbp",    "",         0x42000008, 0xffffffff, INSN_TLB,               0,              I1      },
{"tlbr",    "",         0x42000001, 0xffffffff, INSN_TLB,               0,              I1      },
{"tlbwi",   "",         0x42000002, 0xffffffff, INSN_TLB,               0,              I1      },
{"tlbwr",   "",         0x42000006, 0xffffffff, INSN_TLB,               0,              I1      },
{"tlti",    "s,j",      0x040a0000, 0xfc1f0000, RD_s|TRAP,              0,              I2      },
{"tlt",     "s,t",      0x00000032, 0xfc00ffff, RD_s|RD_t|TRAP,         0,              I2      },
{"tlt",     "s,t,q",    0x00000032, 0xfc00003f, RD_s|RD_t|TRAP,         0,              I2      },
{"tlt",     "s,j",      0x040a0000, 0xfc1f0000, RD_s|TRAP,              0,              I2      }, /* tlti */
{"tltiu",   "s,j",      0x040b0000, 0xfc1f0000, RD_s|TRAP,              0,              I2      },
{"tltu",    "s,t",      0x00000033, 0xfc00ffff, RD_s|RD_t|TRAP,         0,              I2      },
{"tltu",    "s,t,q",    0x00000033, 0xfc00003f, RD_s|RD_t|TRAP,         0,              I2      },
{"tltu",    "s,j",      0x040b0000, 0xfc1f0000, RD_s|TRAP,              0,              I2      }, /* tltiu */
{"tnei",    "s,j",      0x040e0000, 0xfc1f0000, RD_s|TRAP,              0,              I2      },
{"tne",     "s,t",      0x00000036, 0xfc00ffff, RD_s|RD_t|TRAP,         0,              I2      },
{"tne",     "s,t,q",    0x00000036, 0xfc00003f, RD_s|RD_t|TRAP,         0,              I2      },
{"tne",     "s,j",      0x040e0000, 0xfc1f0000, RD_s|TRAP,              0,              I2      }, /* tnei */
{"trunc.l.d", "D,S",    0x46200009, 0xffff003f, WR_D|RD_S|FP_D,         0,              I3_33   },
{"trunc.l.s", "D,S",    0x46000009, 0xffff003f, WR_D|RD_S|FP_S|FP_D,    0,              I3_33   },
{"trunc.w.d", "D,S",    0x4620000d, 0xffff003f, WR_D|RD_S|FP_S|FP_D,    0,              I2      },
{"trunc.w.d", "D,S,x",  0x4620000d, 0xffff003f, WR_D|RD_S|FP_S|FP_D,    0,              I2      },
{"trunc.w.s", "D,S",    0x4600000d, 0xffff003f, WR_D|RD_S|FP_S,         0,              I2      },
{"trunc.w.s", "D,S,x",  0x4600000d, 0xffff003f, WR_D|RD_S|FP_S,         0,              I2      },
{"wait",    "",         0x42000020, 0xffffffff, TRAP,                   0,              I3_32   },
{"wait",    "J",        0x42000020, 0xfe00003f, TRAP,                   0,              I32     },
{"wrpgpr",  "d,w",      0x41c00000, 0xffe007ff, RD_t,                   0,              I33     },
{"wsbh",    "d,w",      0x7c0000a0, 0xffe007ff, WR_d|RD_t,              0,              I33     },
{"xor",     "d,v,t",    0x00000026, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              I1      },
{"xori",    "t,r,i",    0x38000000, 0xfc000000, WR_t|RD_s,              0,              I1      },
{"yield",   "s",        0x7c000009, 0xfc1fffff, TRAP|RD_s,              0,              MT32    },
{"yield",   "d,s",      0x7c000009, 0xfc1f07ff, TRAP|WR_d|RD_s,         0,              MT32    },

/* User Defined Instruction.  */
{"udi0",     "s,t,d,+1",0x70000010, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi0",     "s,t,+2",  0x70000010, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi0",     "s,+3",    0x70000010, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi0",     "+4",      0x70000010, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi1",     "s,t,d,+1",0x70000011, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi1",     "s,t,+2",  0x70000011, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi1",     "s,+3",    0x70000011, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi1",     "+4",      0x70000011, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi2",     "s,t,d,+1",0x70000012, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi2",     "s,t,+2",  0x70000012, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi2",     "s,+3",    0x70000012, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi2",     "+4",      0x70000012, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi3",     "s,t,d,+1",0x70000013, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi3",     "s,t,+2",  0x70000013, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi3",     "s,+3",    0x70000013, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi3",     "+4",      0x70000013, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi4",     "s,t,d,+1",0x70000014, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi4",     "s,t,+2",  0x70000014, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi4",     "s,+3",    0x70000014, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi4",     "+4",      0x70000014, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi5",     "s,t,d,+1",0x70000015, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi5",     "s,t,+2",  0x70000015, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi5",     "s,+3",    0x70000015, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi5",     "+4",      0x70000015, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi6",     "s,t,d,+1",0x70000016, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi6",     "s,t,+2",  0x70000016, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi6",     "s,+3",    0x70000016, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi6",     "+4",      0x70000016, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi7",     "s,t,d,+1",0x70000017, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi7",     "s,t,+2",  0x70000017, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi7",     "s,+3",    0x70000017, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi7",     "+4",      0x70000017, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi8",     "s,t,d,+1",0x70000018, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi8",     "s,t,+2",  0x70000018, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi8",     "s,+3",    0x70000018, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi8",     "+4",      0x70000018, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi9",     "s,t,d,+1",0x70000019, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi9",      "s,t,+2", 0x70000019, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi9",     "s,+3",    0x70000019, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi9",     "+4",      0x70000019, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi10",    "s,t,d,+1",0x7000001a, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi10",    "s,t,+2",  0x7000001a, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi10",    "s,+3",    0x7000001a, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi10",    "+4",      0x7000001a, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi11",    "s,t,d,+1",0x7000001b, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi11",    "s,t,+2",  0x7000001b, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi11",    "s,+3",    0x7000001b, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi11",    "+4",      0x7000001b, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi12",    "s,t,d,+1",0x7000001c, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi12",    "s,t,+2",  0x7000001c, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi12",    "s,+3",    0x7000001c, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi12",    "+4",      0x7000001c, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi13",    "s,t,d,+1",0x7000001d, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi13",    "s,t,+2",  0x7000001d, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi13",    "s,+3",    0x7000001d, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi13",    "+4",      0x7000001d, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi14",    "s,t,d,+1",0x7000001e, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi14",    "s,t,+2",  0x7000001e, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi14",    "s,+3",    0x7000001e, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi14",    "+4",      0x7000001e, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi15",    "s,t,d,+1",0x7000001f, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi15",    "s,t,+2",  0x7000001f, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi15",    "s,+3",    0x7000001f, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },
{"udi15",    "+4",      0x7000001f, 0xfc00003f, WR_d|RD_s|RD_t,         0,              I33     },

/* Coprocessor 2 move/branch operations overlap with VR5400 .ob format
   instructions so they are here for the latters to take precedence.  */
{"bc2f",    "p",        0x49000000, 0xffff0000, CBD|RD_CC,              0,              I1      },
{"bc2f",    "N,p",      0x49000000, 0xffe30000, CBD|RD_CC,              0,              I32     },
{"bc2fl",   "p",        0x49020000, 0xffff0000, CBL|RD_CC,              0,              I2      },
{"bc2fl",   "N,p",      0x49020000, 0xffe30000, CBL|RD_CC,              0,              I32     },
{"bc2t",    "p",        0x49010000, 0xffff0000, CBD|RD_CC,              0,              I1      },
{"bc2t",    "N,p",      0x49010000, 0xffe30000, CBD|RD_CC,              0,              I32     },
{"bc2tl",   "p",        0x49030000, 0xffff0000, CBL|RD_CC,              0,              I2      },
{"bc2tl",   "N,p",      0x49030000, 0xffe30000, CBL|RD_CC,              0,              I32     },
{"cfc2",    "t,G",      0x48400000, 0xffe007ff, LCD|WR_t|RD_C2,         0,              I1      },
{"ctc2",    "t,G",      0x48c00000, 0xffe007ff, COD|RD_t|WR_CC,         0,              I1      },
{"dmfc2",   "t,G",      0x48200000, 0xffe007ff, LCD|WR_t|RD_C2,         0,              I3      },
{"dmfc2",   "t,G,H",    0x48200000, 0xffe007f8, LCD|WR_t|RD_C2,         0,              I64     },
{"dmtc2",   "t,G",      0x48a00000, 0xffe007ff, COD|RD_t|WR_C2|WR_CC,   0,              I3      },
{"dmtc2",   "t,G,H",    0x48a00000, 0xffe007f8, COD|RD_t|WR_C2|WR_CC,   0,              I64     },
{"mfc2",    "t,G",      0x48000000, 0xffe007ff, LCD|WR_t|RD_C2,         0,              I1      },
{"mfc2",    "t,G,H",    0x48000000, 0xffe007f8, LCD|WR_t|RD_C2,         0,              I32     },
{"mfhc2",   "t,G",      0x48600000, 0xffe007ff, LCD|WR_t|RD_C2,         0,              I33     },
{"mfhc2",   "t,G,H",    0x48600000, 0xffe007f8, LCD|WR_t|RD_C2,         0,              I33     },
{"mfhc2",   "t,i",      0x48600000, 0xffe00000, LCD|WR_t|RD_C2,         0,              I33     },
{"mtc2",    "t,G",      0x48800000, 0xffe007ff, COD|RD_t|WR_C2|WR_CC,   0,              I1      },
{"mtc2",    "t,G,H",    0x48800000, 0xffe007f8, COD|RD_t|WR_C2|WR_CC,   0,              I32     },
{"mthc2",   "t,G",      0x48e00000, 0xffe007ff, COD|RD_t|WR_C2|WR_CC,   0,              I33     },
{"mthc2",   "t,G,H",    0x48e00000, 0xffe007f8, COD|RD_t|WR_C2|WR_CC,   0,              I33     },
{"mthc2",   "t,i",      0x48e00000, 0xffe00000, COD|RD_t|WR_C2|WR_CC,   0,              I33     },

/* Coprocessor 3 move/branch operations overlap with MIPS IV COP1X
   instructions, so they are here for the latters to take precedence.  */
{"bc3f",    "p",        0x4d000000, 0xffff0000, CBD|RD_CC,              0,              I1      },
{"bc3fl",   "p",        0x4d020000, 0xffff0000, CBL|RD_CC,              0,              I2      },
{"bc3t",    "p",        0x4d010000, 0xffff0000, CBD|RD_CC,              0,              I1      },
{"bc3tl",   "p",        0x4d030000, 0xffff0000, CBL|RD_CC,              0,              I2      },
{"cfc3",    "t,G",      0x4c400000, 0xffe007ff, LCD|WR_t|RD_C3,         0,              I1      },
{"ctc3",    "t,G",      0x4cc00000, 0xffe007ff, COD|RD_t|WR_CC,         0,              I1      },
{"dmfc3",   "t,G",      0x4c200000, 0xffe007ff, LCD|WR_t|RD_C3,         0,              I3      },
{"dmtc3",   "t,G",      0x4ca00000, 0xffe007ff, COD|RD_t|WR_C3|WR_CC,   0,              I3      },
{"mfc3",    "t,G",      0x4c000000, 0xffe007ff, LCD|WR_t|RD_C3,         0,              I1      },
{"mfc3",    "t,G,H",    0x4c000000, 0xffe007f8, LCD|WR_t|RD_C3,         0,              I32     },
{"mtc3",    "t,G",      0x4c800000, 0xffe007ff, COD|RD_t|WR_C3|WR_CC,   0,              I1      },
{"mtc3",    "t,G,H",    0x4c800000, 0xffe007f8, COD|RD_t|WR_C3|WR_CC,   0,              I32     },

/* MIPS DSP ASE */
{"absq_s.ph", "d,t",    0x7c000252, 0xffe007ff, WR_d|RD_t,              0,              D32     },
{"absq_s.pw", "d,t",    0x7c000456, 0xffe007ff, WR_d|RD_t,              0,              D64     },
{"absq_s.qh", "d,t",    0x7c000256, 0xffe007ff, WR_d|RD_t,              0,              D64     },
{"absq_s.w", "d,t",     0x7c000452, 0xffe007ff, WR_d|RD_t,              0,              D32     },
{"addq.ph", "d,s,t",    0x7c000290, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D32     },
{"addq.pw", "d,s,t",    0x7c000494, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D64     },
{"addq.qh", "d,s,t",    0x7c000294, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D64     },
{"addq_s.ph", "d,s,t",  0x7c000390, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D32     },
{"addq_s.pw", "d,s,t",  0x7c000594, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D64     },
{"addq_s.qh", "d,s,t",  0x7c000394, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D64     },
{"addq_s.w", "d,s,t",   0x7c000590, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D32     },
{"addsc",   "d,s,t",    0x7c000410, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D32     },
{"addu.ob", "d,s,t",    0x7c000014, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D64     },
{"addu.qb", "d,s,t",    0x7c000010, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D32     },
{"addu_s.ob", "d,s,t",  0x7c000114, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D64     },
{"addu_s.qb", "d,s,t",  0x7c000110, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D32     },
{"addwc",   "d,s,t",    0x7c000450, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D32     },
{"bitrev",  "d,t",      0x7c0006d2, 0xffe007ff, WR_d|RD_t,              0,              D32     },
{"bposge32", "p",       0x041c0000, 0xffff0000, CBD,                    0,              D32     },
{"bposge64", "p",       0x041d0000, 0xffff0000, CBD,                    0,              D64     },
{"cmp.eq.ph", "s,t",    0x7c000211, 0xfc00ffff, RD_s|RD_t,              0,              D32     },
{"cmp.eq.pw", "s,t",    0x7c000415, 0xfc00ffff, RD_s|RD_t,              0,              D64     },
{"cmp.eq.qh", "s,t",    0x7c000215, 0xfc00ffff, RD_s|RD_t,              0,              D64     },
{"cmpgu.eq.ob", "d,s,t", 0x7c000115, 0xfc0007ff, WR_d|RD_s|RD_t,        0,              D64     },
{"cmpgu.eq.qb", "d,s,t", 0x7c000111, 0xfc0007ff, WR_d|RD_s|RD_t,        0,              D32     },
{"cmpgu.le.ob", "d,s,t", 0x7c000195, 0xfc0007ff, WR_d|RD_s|RD_t,        0,              D64     },
{"cmpgu.le.qb", "d,s,t", 0x7c000191, 0xfc0007ff, WR_d|RD_s|RD_t,        0,              D32     },
{"cmpgu.lt.ob", "d,s,t", 0x7c000155, 0xfc0007ff, WR_d|RD_s|RD_t,        0,              D64     },
{"cmpgu.lt.qb", "d,s,t", 0x7c000151, 0xfc0007ff, WR_d|RD_s|RD_t,        0,              D32     },
{"cmp.le.ph", "s,t",    0x7c000291, 0xfc00ffff, RD_s|RD_t,              0,              D32     },
{"cmp.le.pw", "s,t",    0x7c000495, 0xfc00ffff, RD_s|RD_t,              0,              D64     },
{"cmp.le.qh", "s,t",    0x7c000295, 0xfc00ffff, RD_s|RD_t,              0,              D64     },
{"cmp.lt.ph", "s,t",    0x7c000251, 0xfc00ffff, RD_s|RD_t,              0,              D32     },
{"cmp.lt.pw", "s,t",    0x7c000455, 0xfc00ffff, RD_s|RD_t,              0,              D64     },
{"cmp.lt.qh", "s,t",    0x7c000255, 0xfc00ffff, RD_s|RD_t,              0,              D64     },
{"cmpu.eq.ob", "s,t",   0x7c000015, 0xfc00ffff, RD_s|RD_t,              0,              D64     },
{"cmpu.eq.qb", "s,t",   0x7c000011, 0xfc00ffff, RD_s|RD_t,              0,              D32     },
{"cmpu.le.ob", "s,t",   0x7c000095, 0xfc00ffff, RD_s|RD_t,              0,              D64     },
{"cmpu.le.qb", "s,t",   0x7c000091, 0xfc00ffff, RD_s|RD_t,              0,              D32     },
{"cmpu.lt.ob", "s,t",   0x7c000055, 0xfc00ffff, RD_s|RD_t,              0,              D64     },
{"cmpu.lt.qb", "s,t",   0x7c000051, 0xfc00ffff, RD_s|RD_t,              0,              D32     },
{"dextpdp", "t,7,6",    0x7c0002bc, 0xfc00e7ff, WR_t|RD_a|DSP_VOLA,     0,              D64     },
{"dextpdpv", "t,7,s",   0x7c0002fc, 0xfc00e7ff, WR_t|RD_a|RD_s|DSP_VOLA, 0,             D64     },
{"dextp",   "t,7,6",    0x7c0000bc, 0xfc00e7ff, WR_t|RD_a,              0,              D64     },
{"dextpv",  "t,7,s",    0x7c0000fc, 0xfc00e7ff, WR_t|RD_a|RD_s,         0,              D64     },
{"dextr.l", "t,7,6",    0x7c00043c, 0xfc00e7ff, WR_t|RD_a,              0,              D64     },
{"dextr_r.l", "t,7,6",  0x7c00053c, 0xfc00e7ff, WR_t|RD_a,              0,              D64     },
{"dextr_rs.l", "t,7,6", 0x7c0005bc, 0xfc00e7ff, WR_t|RD_a,              0,              D64     },
{"dextr_rs.w", "t,7,6", 0x7c0001bc, 0xfc00e7ff, WR_t|RD_a,              0,              D64     },
{"dextr_r.w", "t,7,6",  0x7c00013c, 0xfc00e7ff, WR_t|RD_a,              0,              D64     },
{"dextr_s.h", "t,7,6",  0x7c0003bc, 0xfc00e7ff, WR_t|RD_a,              0,              D64     },
{"dextrv.l", "t,7,s",   0x7c00047c, 0xfc00e7ff, WR_t|RD_a|RD_s,         0,              D64     },
{"dextrv_r.l", "t,7,s", 0x7c00057c, 0xfc00e7ff, WR_t|RD_a|RD_s,         0,              D64     },
{"dextrv_rs.l", "t,7,s", 0x7c0005fc, 0xfc00e7ff, WR_t|RD_a|RD_s,        0,              D64     },
{"dextrv_rs.w", "t,7,s", 0x7c0001fc, 0xfc00e7ff, WR_t|RD_a|RD_s,        0,              D64     },
{"dextrv_r.w", "t,7,s", 0x7c00017c, 0xfc00e7ff, WR_t|RD_a|RD_s,         0,              D64     },
{"dextrv_s.h", "t,7,s", 0x7c0003fc, 0xfc00e7ff, WR_t|RD_a|RD_s,         0,              D64     },
{"dextrv.w", "t,7,s",   0x7c00007c, 0xfc00e7ff, WR_t|RD_a|RD_s,         0,              D64     },
{"dextr.w", "t,7,6",    0x7c00003c, 0xfc00e7ff, WR_t|RD_a,              0,              D64     },
{"dinsv",   "t,s",      0x7c00000d, 0xfc00ffff, WR_t|RD_s,              0,              D64     },
{"dmadd",   "7,s,t",    0x7c000674, 0xfc00e7ff, MOD_a|RD_s|RD_t,        0,              D64     },
{"dmaddu",  "7,s,t",    0x7c000774, 0xfc00e7ff, MOD_a|RD_s|RD_t,        0,              D64     },
{"dmsub",   "7,s,t",    0x7c0006f4, 0xfc00e7ff, MOD_a|RD_s|RD_t,        0,              D64     },
{"dmsubu",  "7,s,t",    0x7c0007f4, 0xfc00e7ff, MOD_a|RD_s|RD_t,        0,              D64     },
{"dmthlip", "s,7",      0x7c0007fc, 0xfc1fe7ff, RD_s|MOD_a|DSP_VOLA,    0,              D64     },
{"dpaq_sa.l.pw", "7,s,t", 0x7c000334, 0xfc00e7ff, MOD_a|RD_s|RD_t,      0,              D64     },
{"dpaq_sa.l.w", "7,s,t", 0x7c000330, 0xfc00e7ff, MOD_a|RD_s|RD_t,       0,              D32     },
{"dpaq_s.w.ph", "7,s,t", 0x7c000130, 0xfc00e7ff, MOD_a|RD_s|RD_t,       0,              D32     },
{"dpaq_s.w.qh", "7,s,t", 0x7c000134, 0xfc00e7ff, MOD_a|RD_s|RD_t,       0,              D64     },
{"dpau.h.obl", "7,s,t", 0x7c0000f4, 0xfc00e7ff, MOD_a|RD_s|RD_t,        0,              D64     },
{"dpau.h.obr", "7,s,t", 0x7c0001f4, 0xfc00e7ff, MOD_a|RD_s|RD_t,        0,              D64     },
{"dpau.h.qbl", "7,s,t", 0x7c0000f0, 0xfc00e7ff, MOD_a|RD_s|RD_t,        0,              D32     },
{"dpau.h.qbr", "7,s,t", 0x7c0001f0, 0xfc00e7ff, MOD_a|RD_s|RD_t,        0,              D32     },
{"dpsq_sa.l.pw", "7,s,t", 0x7c000374, 0xfc00e7ff, MOD_a|RD_s|RD_t,      0,              D64     },
{"dpsq_sa.l.w", "7,s,t", 0x7c000370, 0xfc00e7ff, MOD_a|RD_s|RD_t,       0,              D32     },
{"dpsq_s.w.ph", "7,s,t", 0x7c000170, 0xfc00e7ff, MOD_a|RD_s|RD_t,       0,              D32     },
{"dpsq_s.w.qh", "7,s,t", 0x7c000174, 0xfc00e7ff, MOD_a|RD_s|RD_t,       0,              D64     },
{"dpsu.h.obl", "7,s,t", 0x7c0002f4, 0xfc00e7ff, MOD_a|RD_s|RD_t,        0,              D64     },
{"dpsu.h.obr", "7,s,t", 0x7c0003f4, 0xfc00e7ff, MOD_a|RD_s|RD_t,        0,              D64     },
{"dpsu.h.qbl", "7,s,t", 0x7c0002f0, 0xfc00e7ff, MOD_a|RD_s|RD_t,        0,              D32     },
{"dpsu.h.qbr", "7,s,t", 0x7c0003f0, 0xfc00e7ff, MOD_a|RD_s|RD_t,        0,              D32     },
{"dshilo",  "7,:",      0x7c0006bc, 0xfc07e7ff, MOD_a,                  0,              D64     },
{"dshilov", "7,s",      0x7c0006fc, 0xfc1fe7ff, MOD_a|RD_s,             0,              D64     },
{"extpdp",  "t,7,6",    0x7c0002b8, 0xfc00e7ff, WR_t|RD_a|DSP_VOLA,     0,              D32     },
{"extpdpv", "t,7,s",    0x7c0002f8, 0xfc00e7ff, WR_t|RD_a|RD_s|DSP_VOLA, 0,             D32     },
{"extp",    "t,7,6",    0x7c0000b8, 0xfc00e7ff, WR_t|RD_a,              0,              D32     },
{"extpv",   "t,7,s",    0x7c0000f8, 0xfc00e7ff, WR_t|RD_a|RD_s,         0,              D32     },
{"extr_rs.w", "t,7,6",  0x7c0001b8, 0xfc00e7ff, WR_t|RD_a,              0,              D32     },
{"extr_r.w", "t,7,6",   0x7c000138, 0xfc00e7ff, WR_t|RD_a,              0,              D32     },
{"extr_s.h", "t,7,6",   0x7c0003b8, 0xfc00e7ff, WR_t|RD_a,              0,              D32     },
{"extrv_rs.w", "t,7,s", 0x7c0001f8, 0xfc00e7ff, WR_t|RD_a|RD_s,         0,              D32     },
{"extrv_r.w", "t,7,s",  0x7c000178, 0xfc00e7ff, WR_t|RD_a|RD_s,         0,              D32     },
{"extrv_s.h", "t,7,s",  0x7c0003f8, 0xfc00e7ff, WR_t|RD_a|RD_s,         0,              D32     },
{"extrv.w", "t,7,s",    0x7c000078, 0xfc00e7ff, WR_t|RD_a|RD_s,         0,              D32     },
{"extr.w",  "t,7,6",    0x7c000038, 0xfc00e7ff, WR_t|RD_a,              0,              D32     },
{"insv",    "t,s",      0x7c00000c, 0xfc00ffff, WR_t|RD_s,              0,              D32     },
{"lbux",    "d,t(b)",   0x7c00018a, 0xfc0007ff, LDD|WR_d|RD_t|RD_b,     0,              D32     },
{"ldx",     "d,t(b)",   0x7c00020a, 0xfc0007ff, LDD|WR_d|RD_t|RD_b,     0,              D64     },
{"lhx",     "d,t(b)",   0x7c00010a, 0xfc0007ff, LDD|WR_d|RD_t|RD_b,     0,              D32     },
{"lwx",     "d,t(b)",   0x7c00000a, 0xfc0007ff, LDD|WR_d|RD_t|RD_b,     0,              D32     },
{"maq_sa.w.phl", "7,s,t", 0x7c000430, 0xfc00e7ff, MOD_a|RD_s|RD_t,      0,              D32     },
{"maq_sa.w.phr", "7,s,t", 0x7c0004b0, 0xfc00e7ff, MOD_a|RD_s|RD_t,      0,              D32     },
{"maq_sa.w.qhll", "7,s,t", 0x7c000434, 0xfc00e7ff, MOD_a|RD_s|RD_t,     0,              D64     },
{"maq_sa.w.qhlr", "7,s,t", 0x7c000474, 0xfc00e7ff, MOD_a|RD_s|RD_t,     0,              D64     },
{"maq_sa.w.qhrl", "7,s,t", 0x7c0004b4, 0xfc00e7ff, MOD_a|RD_s|RD_t,     0,              D64     },
{"maq_sa.w.qhrr", "7,s,t", 0x7c0004f4, 0xfc00e7ff, MOD_a|RD_s|RD_t,     0,              D64     },
{"maq_s.l.pwl", "7,s,t", 0x7c000734, 0xfc00e7ff, MOD_a|RD_s|RD_t,       0,              D64     },
{"maq_s.l.pwr", "7,s,t", 0x7c0007b4, 0xfc00e7ff, MOD_a|RD_s|RD_t,       0,              D64     },
{"maq_s.w.phl", "7,s,t", 0x7c000530, 0xfc00e7ff, MOD_a|RD_s|RD_t,       0,              D32     },
{"maq_s.w.phr", "7,s,t", 0x7c0005b0, 0xfc00e7ff, MOD_a|RD_s|RD_t,       0,              D32     },
{"maq_s.w.qhll", "7,s,t", 0x7c000534, 0xfc00e7ff, MOD_a|RD_s|RD_t,      0,              D64     },
{"maq_s.w.qhlr", "7,s,t", 0x7c000574, 0xfc00e7ff, MOD_a|RD_s|RD_t,      0,              D64     },
{"maq_s.w.qhrl", "7,s,t", 0x7c0005b4, 0xfc00e7ff, MOD_a|RD_s|RD_t,      0,              D64     },
{"maq_s.w.qhrr", "7,s,t", 0x7c0005f4, 0xfc00e7ff, MOD_a|RD_s|RD_t,      0,              D64     },
{"modsub",  "d,s,t",    0x7c000490, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D32     },
{"mthlip",  "s,7",      0x7c0007f8, 0xfc1fe7ff, RD_s|MOD_a|DSP_VOLA,    0,              D32     },
{"muleq_s.pw.qhl", "d,s,t", 0x7c000714, 0xfc0007ff, WR_d|RD_s|RD_t|WR_HILO, 0,          D64     },
{"muleq_s.pw.qhr", "d,s,t", 0x7c000754, 0xfc0007ff, WR_d|RD_s|RD_t|WR_HILO, 0,          D64     },
{"muleq_s.w.phl", "d,s,t", 0x7c000710, 0xfc0007ff, WR_d|RD_s|RD_t|WR_HILO, 0,           D32     },
{"muleq_s.w.phr", "d,s,t", 0x7c000750, 0xfc0007ff, WR_d|RD_s|RD_t|WR_HILO, 0,           D32     },
{"muleu_s.ph.qbl", "d,s,t", 0x7c000190, 0xfc0007ff, WR_d|RD_s|RD_t|WR_HILO, 0,          D32     },
{"muleu_s.ph.qbr", "d,s,t", 0x7c0001d0, 0xfc0007ff, WR_d|RD_s|RD_t|WR_HILO, 0,          D32     },
{"muleu_s.qh.obl", "d,s,t", 0x7c000194, 0xfc0007ff, WR_d|RD_s|RD_t|WR_HILO, 0,          D64     },
{"muleu_s.qh.obr", "d,s,t", 0x7c0001d4, 0xfc0007ff, WR_d|RD_s|RD_t|WR_HILO, 0,          D64     },
{"mulq_rs.ph", "d,s,t", 0x7c0007d0, 0xfc0007ff, WR_d|RD_s|RD_t|WR_HILO, 0,              D32     },
{"mulq_rs.qh", "d,s,t", 0x7c0007d4, 0xfc0007ff, WR_d|RD_s|RD_t|WR_HILO, 0,              D64     },
{"mulsaq_s.l.pw", "7,s,t", 0x7c0003b4, 0xfc00e7ff, MOD_a|RD_s|RD_t,     0,              D64     },
{"mulsaq_s.w.ph", "7,s,t", 0x7c0001b0, 0xfc00e7ff, MOD_a|RD_s|RD_t,     0,              D32     },
{"mulsaq_s.w.qh", "7,s,t", 0x7c0001b4, 0xfc00e7ff, MOD_a|RD_s|RD_t,     0,              D64     },
{"packrl.ph", "d,s,t",  0x7c000391, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D32     },
{"packrl.pw", "d,s,t",  0x7c000395, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D64     },
{"pick.ob", "d,s,t",    0x7c0000d5, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D64     },
{"pick.ph", "d,s,t",    0x7c0002d1, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D32     },
{"pick.pw", "d,s,t",    0x7c0004d5, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D64     },
{"pick.qb", "d,s,t",    0x7c0000d1, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D32     },
{"pick.qh", "d,s,t",    0x7c0002d5, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D64     },
{"preceq.pw.qhla", "d,t", 0x7c000396, 0xffe007ff, WR_d|RD_t,            0,              D64     },
{"preceq.pw.qhl", "d,t", 0x7c000316, 0xffe007ff, WR_d|RD_t,             0,              D64     },
{"preceq.pw.qhra", "d,t", 0x7c0003d6, 0xffe007ff, WR_d|RD_t,            0,              D64     },
{"preceq.pw.qhr", "d,t", 0x7c000356, 0xffe007ff, WR_d|RD_t,             0,              D64     },
{"preceq.s.l.pwl", "d,t", 0x7c000516, 0xffe007ff, WR_d|RD_t,            0,              D64     },
{"preceq.s.l.pwr", "d,t", 0x7c000556, 0xffe007ff, WR_d|RD_t,            0,              D64     },
{"precequ.ph.qbla", "d,t", 0x7c000192, 0xffe007ff, WR_d|RD_t,           0,              D32     },
{"precequ.ph.qbl", "d,t", 0x7c000112, 0xffe007ff, WR_d|RD_t,            0,              D32     },
{"precequ.ph.qbra", "d,t", 0x7c0001d2, 0xffe007ff, WR_d|RD_t,           0,              D32     },
{"precequ.ph.qbr", "d,t", 0x7c000152, 0xffe007ff, WR_d|RD_t,            0,              D32     },
{"precequ.pw.qhla", "d,t", 0x7c000196, 0xffe007ff, WR_d|RD_t,           0,              D64     },
{"precequ.pw.qhl", "d,t", 0x7c000116, 0xffe007ff, WR_d|RD_t,            0,              D64     },
{"precequ.pw.qhra", "d,t", 0x7c0001d6, 0xffe007ff, WR_d|RD_t,           0,              D64     },
{"precequ.pw.qhr", "d,t", 0x7c000156, 0xffe007ff, WR_d|RD_t,            0,              D64     },
{"preceq.w.phl", "d,t", 0x7c000312, 0xffe007ff, WR_d|RD_t,              0,              D32     },
{"preceq.w.phr", "d,t", 0x7c000352, 0xffe007ff, WR_d|RD_t,              0,              D32     },
{"preceu.ph.qbla", "d,t", 0x7c000792, 0xffe007ff, WR_d|RD_t,            0,              D32     },
{"preceu.ph.qbl", "d,t", 0x7c000712, 0xffe007ff, WR_d|RD_t,             0,              D32     },
{"preceu.ph.qbra", "d,t", 0x7c0007d2, 0xffe007ff, WR_d|RD_t,            0,              D32     },
{"preceu.ph.qbr", "d,t", 0x7c000752, 0xffe007ff, WR_d|RD_t,             0,              D32     },
{"preceu.qh.obla", "d,t", 0x7c000796, 0xffe007ff, WR_d|RD_t,            0,              D64     },
{"preceu.qh.obl", "d,t", 0x7c000716, 0xffe007ff, WR_d|RD_t,             0,              D64     },
{"preceu.qh.obra", "d,t", 0x7c0007d6, 0xffe007ff, WR_d|RD_t,            0,              D64     },
{"preceu.qh.obr", "d,t", 0x7c000756, 0xffe007ff, WR_d|RD_t,             0,              D64     },
{"precrq.ob.qh", "d,s,t", 0x7c000315, 0xfc0007ff, WR_d|RD_s|RD_t,       0,              D64     },
{"precrq.ph.w", "d,s,t", 0x7c000511, 0xfc0007ff, WR_d|RD_s|RD_t,        0,              D32     },
{"precrq.pw.l", "d,s,t", 0x7c000715, 0xfc0007ff, WR_d|RD_s|RD_t,        0,              D64     },
{"precrq.qb.ph", "d,s,t", 0x7c000311, 0xfc0007ff, WR_d|RD_s|RD_t,       0,              D32     },
{"precrq.qh.pw", "d,s,t", 0x7c000515, 0xfc0007ff, WR_d|RD_s|RD_t,       0,              D64     },
{"precrq_rs.ph.w", "d,s,t", 0x7c000551, 0xfc0007ff, WR_d|RD_s|RD_t,     0,              D32     },
{"precrq_rs.qh.pw", "d,s,t", 0x7c000555, 0xfc0007ff, WR_d|RD_s|RD_t,    0,              D64     },
{"precrqu_s.ob.qh", "d,s,t", 0x7c0003d5, 0xfc0007ff, WR_d|RD_s|RD_t,    0,              D64     },
{"precrqu_s.qb.ph", "d,s,t", 0x7c0003d1, 0xfc0007ff, WR_d|RD_s|RD_t,    0,              D32     },
{"raddu.l.ob", "d,s",   0x7c000514, 0xfc1f07ff, WR_d|RD_s,              0,              D64     },
{"raddu.w.qb", "d,s",   0x7c000510, 0xfc1f07ff, WR_d|RD_s,              0,              D32     },
{"rddsp",   "d",        0x7fff04b8, 0xffff07ff, WR_d,                   0,              D32     },
{"rddsp",   "d,'",      0x7c0004b8, 0xffc007ff, WR_d,                   0,              D32     },
{"repl.ob", "d,5",      0x7c000096, 0xff0007ff, WR_d,                   0,              D64     },
{"repl.ph", "d,@",      0x7c000292, 0xfc0007ff, WR_d,                   0,              D32     },
{"repl.pw", "d,@",      0x7c000496, 0xfc0007ff, WR_d,                   0,              D64     },
{"repl.qb", "d,5",      0x7c000092, 0xff0007ff, WR_d,                   0,              D32     },
{"repl.qh", "d,@",      0x7c000296, 0xfc0007ff, WR_d,                   0,              D64     },
{"replv.ob", "d,t",     0x7c0000d6, 0xffe007ff, WR_d|RD_t,              0,              D64     },
{"replv.ph", "d,t",     0x7c0002d2, 0xffe007ff, WR_d|RD_t,              0,              D32     },
{"replv.pw", "d,t",     0x7c0004d6, 0xffe007ff, WR_d|RD_t,              0,              D64     },
{"replv.qb", "d,t",     0x7c0000d2, 0xffe007ff, WR_d|RD_t,              0,              D32     },
{"replv.qh", "d,t",     0x7c0002d6, 0xffe007ff, WR_d|RD_t,              0,              D64     },
{"shilo",   "7,0",      0x7c0006b8, 0xfc0fe7ff, MOD_a,                  0,              D32     },
{"shilov",  "7,s",      0x7c0006f8, 0xfc1fe7ff, MOD_a|RD_s,             0,              D32     },
{"shll.ob", "d,t,3",    0x7c000017, 0xff0007ff, WR_d|RD_t,              0,              D64     },
{"shll.ph", "d,t,4",    0x7c000213, 0xfe0007ff, WR_d|RD_t,              0,              D32     },
{"shll.pw", "d,t,6",    0x7c000417, 0xfc0007ff, WR_d|RD_t,              0,              D64     },
{"shll.qb", "d,t,3",    0x7c000013, 0xff0007ff, WR_d|RD_t,              0,              D32     },
{"shll.qh", "d,t,4",    0x7c000217, 0xfe0007ff, WR_d|RD_t,              0,              D64     },
{"shll_s.ph", "d,t,4",  0x7c000313, 0xfe0007ff, WR_d|RD_t,              0,              D32     },
{"shll_s.pw", "d,t,6",  0x7c000517, 0xfc0007ff, WR_d|RD_t,              0,              D64     },
{"shll_s.qh", "d,t,4",  0x7c000317, 0xfe0007ff, WR_d|RD_t,              0,              D64     },
{"shll_s.w", "d,t,6",   0x7c000513, 0xfc0007ff, WR_d|RD_t,              0,              D32     },
{"shllv.ob", "d,t,s",   0x7c000097, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D64     },
{"shllv.ph", "d,t,s",   0x7c000293, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D32     },
{"shllv.pw", "d,t,s",   0x7c000497, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D64     },
{"shllv.qb", "d,t,s",   0x7c000093, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D32     },
{"shllv.qh", "d,t,s",   0x7c000297, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D64     },
{"shllv_s.ph", "d,t,s", 0x7c000393, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D32     },
{"shllv_s.pw", "d,t,s", 0x7c000597, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D64     },
{"shllv_s.qh", "d,t,s", 0x7c000397, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D64     },
{"shllv_s.w", "d,t,s",  0x7c000593, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D32     },
{"shra.ph", "d,t,4",    0x7c000253, 0xfe0007ff, WR_d|RD_t,              0,              D32     },
{"shra.pw", "d,t,6",    0x7c000457, 0xfc0007ff, WR_d|RD_t,              0,              D64     },
{"shra.qh", "d,t,4",    0x7c000257, 0xfe0007ff, WR_d|RD_t,              0,              D64     },
{"shra_r.ph", "d,t,4",  0x7c000353, 0xfe0007ff, WR_d|RD_t,              0,              D32     },
{"shra_r.pw", "d,t,6",  0x7c000557, 0xfc0007ff, WR_d|RD_t,              0,              D64     },
{"shra_r.qh", "d,t,4",  0x7c000357, 0xfe0007ff, WR_d|RD_t,              0,              D64     },
{"shra_r.w", "d,t,6",   0x7c000553, 0xfc0007ff, WR_d|RD_t,              0,              D32     },
{"shrav.ph", "d,t,s",   0x7c0002d3, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D32     },
{"shrav.pw", "d,t,s",   0x7c0004d7, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D64     },
{"shrav.qh", "d,t,s",   0x7c0002d7, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D64     },
{"shrav_r.ph", "d,t,s", 0x7c0003d3, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D32     },
{"shrav_r.pw", "d,t,s", 0x7c0005d7, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D64     },
{"shrav_r.qh", "d,t,s", 0x7c0003d7, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D64     },
{"shrav_r.w", "d,t,s",  0x7c0005d3, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D32     },
{"shrl.ob", "d,t,3",    0x7c000057, 0xff0007ff, WR_d|RD_t,              0,              D64     },
{"shrl.qb", "d,t,3",    0x7c000053, 0xff0007ff, WR_d|RD_t,              0,              D32     },
{"shrlv.ob", "d,t,s",   0x7c0000d7, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D64     },
{"shrlv.qb", "d,t,s",   0x7c0000d3, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D32     },
{"subq.ph", "d,s,t",    0x7c0002d0, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D32     },
{"subq.pw", "d,s,t",    0x7c0004d4, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D64     },
{"subq.qh", "d,s,t",    0x7c0002d4, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D64     },
{"subq_s.ph", "d,s,t",  0x7c0003d0, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D32     },
{"subq_s.pw", "d,s,t",  0x7c0005d4, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D64     },
{"subq_s.qh", "d,s,t",  0x7c0003d4, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D64     },
{"subq_s.w", "d,s,t",   0x7c0005d0, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D32     },
{"subu.ob", "d,s,t",    0x7c000054, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D64     },
{"subu.qb", "d,s,t",    0x7c000050, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D32     },
{"subu_s.ob", "d,s,t",  0x7c000154, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D64     },
{"subu_s.qb", "d,s,t",  0x7c000150, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D32     },
{"wrdsp",   "s",        0x7c1ffcf8, 0xfc1fffff, RD_s|DSP_VOLA,          0,              D32     },
{"wrdsp",   "s,8",      0x7c0004f8, 0xfc1e07ff, RD_s|DSP_VOLA,          0,              D32     },
/* MIPS DSP ASE Rev2 */
{"absq_s.qb", "d,t",    0x7c000052, 0xffe007ff, WR_d|RD_t,              0,              D33     },
{"addu.ph", "d,s,t",    0x7c000210, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D33     },
{"addu_s.ph", "d,s,t",  0x7c000310, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D33     },
{"adduh.qb", "d,s,t",   0x7c000018, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D33     },
{"adduh_r.qb", "d,s,t", 0x7c000098, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D33     },
{"append",  "t,s,h",    0x7c000031, 0xfc0007ff, WR_t|RD_t|RD_s,         0,              D33     },
{"balign",  "t,s,2",    0x7c000431, 0xfc00e7ff, WR_t|RD_t|RD_s,         0,              D33     },
{"cmpgdu.eq.qb", "d,s,t", 0x7c000611, 0xfc0007ff, WR_d|RD_s|RD_t,       0,              D33     },
{"cmpgdu.lt.qb", "d,s,t", 0x7c000651, 0xfc0007ff, WR_d|RD_s|RD_t,       0,              D33     },
{"cmpgdu.le.qb", "d,s,t", 0x7c000691, 0xfc0007ff, WR_d|RD_s|RD_t,       0,              D33     },
{"dpa.w.ph", "7,s,t",   0x7c000030, 0xfc00e7ff, MOD_a|RD_s|RD_t,        0,              D33     },
{"dps.w.ph", "7,s,t",   0x7c000070, 0xfc00e7ff, MOD_a|RD_s|RD_t,        0,              D33     },
{"mul.ph",  "d,s,t",    0x7c000318, 0xfc0007ff, WR_d|RD_s|RD_t|WR_HILO, 0,              D33     },
{"mul_s.ph", "d,s,t",   0x7c000398, 0xfc0007ff, WR_d|RD_s|RD_t|WR_HILO, 0,              D33     },
{"mulq_rs.w", "d,s,t",  0x7c0005d8, 0xfc0007ff, WR_d|RD_s|RD_t|WR_HILO, 0,              D33     },
{"mulq_s.ph", "d,s,t",  0x7c000790, 0xfc0007ff, WR_d|RD_s|RD_t|WR_HILO, 0,              D33     },
{"mulq_s.w", "d,s,t",   0x7c000598, 0xfc0007ff, WR_d|RD_s|RD_t|WR_HILO, 0,              D33     },
{"mulsa.w.ph", "7,s,t", 0x7c0000b0, 0xfc00e7ff, MOD_a|RD_s|RD_t,        0,              D33     },
{"precr.qb.ph", "d,s,t", 0x7c000351, 0xfc0007ff, WR_d|RD_s|RD_t,        0,              D33     },
{"precr_sra.ph.w", "t,s,h", 0x7c000791, 0xfc0007ff, WR_t|RD_t|RD_s,     0,              D33     },
{"precr_sra_r.ph.w", "t,s,h", 0x7c0007d1, 0xfc0007ff, WR_t|RD_t|RD_s,   0,              D33     },
{"prepend", "t,s,h",    0x7c000071, 0xfc0007ff, WR_t|RD_t|RD_s,         0,              D33     },
{"shra.qb", "d,t,3",    0x7c000113, 0xff0007ff, WR_d|RD_t,              0,              D33     },
{"shra_r.qb", "d,t,3",  0x7c000153, 0xff0007ff, WR_d|RD_t,              0,              D33     },
{"shrav.qb", "d,t,s",   0x7c000193, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D33     },
{"shrav_r.qb", "d,t,s", 0x7c0001d3, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D33     },
{"shrl.ph", "d,t,4",    0x7c000653, 0xfe0007ff, WR_d|RD_t,              0,              D33     },
{"shrlv.ph", "d,t,s",   0x7c0006d3, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D33     },
{"subu.ph", "d,s,t",    0x7c000250, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D33     },
{"subu_s.ph", "d,s,t",  0x7c000350, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D33     },
{"subuh.qb", "d,s,t",   0x7c000058, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D33     },
{"subuh_r.qb", "d,s,t", 0x7c0000d8, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D33     },
{"addqh.ph", "d,s,t",   0x7c000218, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D33     },
{"addqh_r.ph", "d,s,t", 0x7c000298, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D33     },
{"addqh.w", "d,s,t",    0x7c000418, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D33     },
{"addqh_r.w", "d,s,t",  0x7c000498, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D33     },
{"subqh.ph", "d,s,t",   0x7c000258, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D33     },
{"subqh_r.ph", "d,s,t", 0x7c0002d8, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D33     },
{"subqh.w", "d,s,t",    0x7c000458, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D33     },
{"subqh_r.w", "d,s,t",  0x7c0004d8, 0xfc0007ff, WR_d|RD_s|RD_t,         0,              D33     },
{"dpax.w.ph", "7,s,t",  0x7c000230, 0xfc00e7ff, MOD_a|RD_s|RD_t,        0,              D33     },
{"dpsx.w.ph", "7,s,t",  0x7c000270, 0xfc00e7ff, MOD_a|RD_s|RD_t,        0,              D33     },
{"dpaqx_s.w.ph", "7,s,t", 0x7c000630, 0xfc00e7ff, MOD_a|RD_s|RD_t,      0,              D33     },
{"dpaqx_sa.w.ph", "7,s,t", 0x7c0006b0, 0xfc00e7ff, MOD_a|RD_s|RD_t,     0,              D33     },
{"dpsqx_s.w.ph", "7,s,t", 0x7c000670, 0xfc00e7ff, MOD_a|RD_s|RD_t,      0,              D33     },
{"dpsqx_sa.w.ph", "7,s,t", 0x7c0006f0, 0xfc00e7ff, MOD_a|RD_s|RD_t,     0,              D33     },
/* Move bc0* after mftr and mttr to avoid opcode collision.  */
{"bc0f",    "p",        0x41000000, 0xffff0000, CBD|RD_CC,              0,              I1      },
{"bc0fl",   "p",        0x41020000, 0xffff0000, CBL|RD_CC,              0,              I2      },
{"bc0t",    "p",        0x41010000, 0xffff0000, CBD|RD_CC,              0,              I1      },
{"bc0tl",   "p",        0x41030000, 0xffff0000, CBL|RD_CC,              0,              I2      },
/* No hazard protection on coprocessor instructions--they shouldn't
   change the state of the processor and if they do it's up to the
   user to put in nops as necessary.  These are at the end so that the
   disassembler recognizes more specific versions first.  */
{"c0",      "C",        0x42000000, 0xfe000000, CP,                     0,              I1      },
{"c1",      "C",        0x46000000, 0xfe000000, FP_S,                   0,              I1      },
{"c2",      "C",        0x4a000000, 0xfe000000, CP,                     0,              I1      },
{"c3",      "C",        0x4e000000, 0xfe000000, CP,                     0,              I1      },
};

static const int mips_num_opcodes = sizeof (mips_opcodes) / sizeof (mips_opcodes[0]);

#undef LDD
#undef LCD
#undef UBD
#undef CBD
#undef COD
#undef CLD
#undef CBL
#undef TRAP
#undef SM
#undef WR_d
#undef WR_t
#undef WR_31
#undef WR_D
#undef WR_T
#undef WR_S
#undef RD_s
#undef RD_b
#undef RD_t
#undef RD_S
#undef RD_T
#undef RD_R
#undef WR_CC
#undef RD_CC
#undef RD_C0
#undef RD_C1
#undef RD_C2
#undef RD_C3
#undef WR_C0
#undef WR_C1
#undef WR_C2
#undef WR_C3
#undef CP
#undef WR_HI
#undef RD_HI
#undef MOD_HI
#undef WR_LO
#undef RD_LO
#undef MOD_LO
#undef WR_HILO
#undef RD_HILO
#undef MOD_HILO
#undef IS_M
#undef WR_MACC
#undef RD_MACC
#undef I1
#undef I2
#undef I3
#undef I4
#undef I5
#undef I32
#undef I64
#undef I33
#undef I65
#undef I3_32
#undef I3_33
#undef I4_32
#undef I4_33
#undef I5_33
#undef SMT
#undef WR_a
#undef RD_a
#undef MOD_a
#undef DSP_VOLA
#undef D32
#undef D33
#undef D64
#undef MT32
