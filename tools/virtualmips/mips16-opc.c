/*
 * This is the opcodes table for the mips16 processor.  The format of
 * this table is intentionally identical to the one in mips-opc.c.
 * However, the special letters that appear in the argument string are
 * different, and the table uses some different flags.
 *
 * Copyright 1996, 1997, 1998, 2000, 2005, 2006, 2007
 * Free Software Foundation, Inc.
 * Contributed by Ian Lance Taylor, Cygnus Support
 * Adapted for VirtualMIPS by Serge Vakulenko.
 *
 * This file is part of the virtualmips distribution.
 * See LICENSE file for terms of the license.
 */

/* Use some short hand macros to keep down the length of the lines in
   the opcodes table.  */

#define UBD     INSN_UNCOND_BRANCH_DELAY
#define UBR     MIPS16_INSN_UNCOND_BRANCH
#define CBR     MIPS16_INSN_COND_BRANCH

#define WR_x    MIPS16_INSN_WRITE_X
#define WR_y    MIPS16_INSN_WRITE_Y
#define WR_z    MIPS16_INSN_WRITE_Z
#define WR_T    MIPS16_INSN_WRITE_T
#define WR_SP   MIPS16_INSN_WRITE_SP
#define WR_31   MIPS16_INSN_WRITE_31
#define WR_Y    MIPS16_INSN_WRITE_GPR_Y

#define RD_x    MIPS16_INSN_READ_X
#define RD_y    MIPS16_INSN_READ_Y
#define RD_Z    MIPS16_INSN_READ_Z
#define RD_T    MIPS16_INSN_READ_T
#define RD_SP   MIPS16_INSN_READ_SP
#define RD_31   MIPS16_INSN_READ_31
#define RD_PC   MIPS16_INSN_READ_PC
#define RD_X    MIPS16_INSN_READ_GPR_X

#define WR_HI   INSN_WRITE_HI
#define WR_LO   INSN_WRITE_LO
#define RD_HI   INSN_READ_HI
#define RD_LO   INSN_READ_LO

#define TRAP    INSN_TRAP

static const struct mips_opcode mips16_opcodes[] =
{
/* name,    args,       match,  mask,       pinfo */
{"nop",     "",         0x6500, 0xffff,     RD_Z, },        /* move $0,$Z */
{"la",      "x,A",      0x0800, 0xf800,     WR_x|RD_PC, },
{"abs",     "x,w",      0, (int) M_ABS,     INSN_MACRO, },
{"addiu",   "y,x,4",    0x4000, 0xf810,     WR_y|RD_x, },
{"addiu",   "x,k",      0x4800, 0xf800,     WR_x|RD_x, },
{"addiu",   "S,K",      0x6300, 0xff00,     WR_SP|RD_SP, },
{"addiu",   "S,S,K",    0x6300, 0xff00,     WR_SP|RD_SP, },
{"addiu",   "x,P,V",    0x0800, 0xf800,     WR_x|RD_PC, },
{"addiu",   "x,S,V",    0x0000, 0xf800,     WR_x|RD_SP, },
{"addu",    "z,v,y",    0xe001, 0xf803,     WR_z|RD_x|RD_y, },
{"addu",    "y,x,4",    0x4000, 0xf810,     WR_y|RD_x, },
{"addu",    "x,k",      0x4800, 0xf800,     WR_x|RD_x, },
{"addu",    "S,K",      0x6300, 0xff00,     WR_SP|RD_SP, },
{"addu",    "S,S,K",    0x6300, 0xff00,     WR_SP|RD_SP, },
{"addu",    "x,P,V",    0x0800, 0xf800,     WR_x|RD_PC, },
{"addu",    "x,S,V",    0x0000, 0xf800,     WR_x|RD_SP, },
{"and",     "x,y",      0xe80c, 0xf81f,     WR_x|RD_x|RD_y, },
{"b",       "q",        0x1000, 0xf800,     UBR, },
{"beq",     "x,y,p",    0, (int) M_BEQ,     INSN_MACRO, },
{"beq",     "x,U,p",    0, (int) M_BEQ_I,   INSN_MACRO, },
{"beqz",    "x,p",      0x2000, 0xf800,     CBR|RD_x, },
{"bge",     "x,y,p",    0, (int) M_BGE,     INSN_MACRO, },
{"bge",     "x,8,p",    0, (int) M_BGE_I,   INSN_MACRO, },
{"bgeu",    "x,y,p",    0, (int) M_BGEU,    INSN_MACRO, },
{"bgeu",    "x,8,p",    0, (int) M_BGEU_I,  INSN_MACRO, },
{"bgt",     "x,y,p",    0, (int) M_BGT,     INSN_MACRO, },
{"bgt",     "x,8,p",    0, (int) M_BGT_I,   INSN_MACRO, },
{"bgtu",    "x,y,p",    0, (int) M_BGTU,    INSN_MACRO, },
{"bgtu",    "x,8,p",    0, (int) M_BGTU_I,  INSN_MACRO, },
{"ble",     "x,y,p",    0, (int) M_BLE,     INSN_MACRO, },
{"ble",     "x,8,p",    0, (int) M_BLE_I,   INSN_MACRO, },
{"bleu",    "x,y,p",    0, (int) M_BLEU,    INSN_MACRO, },
{"bleu",    "x,8,p",    0, (int) M_BLEU_I,  INSN_MACRO, },
{"blt",     "x,y,p",    0, (int) M_BLT,     INSN_MACRO, },
{"blt",     "x,8,p",    0, (int) M_BLT_I,   INSN_MACRO, },
{"bltu",    "x,y,p",    0, (int) M_BLTU,    INSN_MACRO, },
{"bltu",    "x,8,p",    0, (int) M_BLTU_I,  INSN_MACRO, },
{"bne",     "x,y,p",    0, (int) M_BNE,     INSN_MACRO, },
{"bne",     "x,U,p",    0, (int) M_BNE_I,   INSN_MACRO, },
{"bnez",    "x,p",      0x2800, 0xf800,     CBR|RD_x, },
{"break",   "6",        0xe805, 0xf81f,     TRAP, },
{"bteqz",   "p",        0x6000, 0xff00,     CBR|RD_T, },
{"btnez",   "p",        0x6100, 0xff00,     CBR|RD_T, },
{"cmpi",    "x,U",      0x7000, 0xf800,     WR_T|RD_x, },
{"cmp",     "x,y",      0xe80a, 0xf81f,     WR_T|RD_x|RD_y, },
{"cmp",     "x,U",      0x7000, 0xf800,     WR_T|RD_x, },
{"dla",     "y,E",      0xfe00, 0xff00,     WR_y|RD_PC, },
{"daddiu",  "y,x,4",    0x4010, 0xf810,     WR_y|RD_x, },
{"daddiu",  "y,j",      0xfd00, 0xff00,     WR_y|RD_y, },
{"daddiu",  "S,K",      0xfb00, 0xff00,     WR_SP|RD_SP, },
{"daddiu",  "S,S,K",    0xfb00, 0xff00,     WR_SP|RD_SP, },
{"daddiu",  "y,P,W",    0xfe00, 0xff00,     WR_y|RD_PC, },
{"daddiu",  "y,S,W",    0xff00, 0xff00,     WR_y|RD_SP, },
{"daddu",   "z,v,y",    0xe000, 0xf803,     WR_z|RD_x|RD_y, },
{"daddu",   "y,x,4",    0x4010, 0xf810,     WR_y|RD_x, },
{"daddu",   "y,j",      0xfd00, 0xff00,     WR_y|RD_y, },
{"daddu",   "S,K",      0xfb00, 0xff00,     WR_SP|RD_SP, },
{"daddu",   "S,S,K",    0xfb00, 0xff00,     WR_SP|RD_SP, },
{"daddu",   "y,P,W",    0xfe00, 0xff00,     WR_y|RD_PC, },
{"daddu",   "y,S,W",    0xff00, 0xff00,     WR_y|RD_SP, },
{"ddiv",    "0,x,y",    0xe81e, 0xf81f,     RD_x|RD_y|WR_HI|WR_LO,  },
{"ddiv",    "z,v,y",    0, (int) M_DDIV_3,  INSN_MACRO, },
{"ddivu",   "0,x,y",    0xe81f, 0xf81f,     RD_x|RD_y|WR_HI|WR_LO,  },
{"ddivu",   "z,v,y",    0, (int) M_DDIVU_3, INSN_MACRO, },
{"div",     "0,x,y",    0xe81a, 0xf81f,     RD_x|RD_y|WR_HI|WR_LO,  },
{"div",     "z,v,y",    0, (int) M_DIV_3,   INSN_MACRO, },
{"divu",    "0,x,y",    0xe81b, 0xf81f,     RD_x|RD_y|WR_HI|WR_LO,  },
{"divu",    "z,v,y",    0, (int) M_DIVU_3,  INSN_MACRO, },
{"dmul",    "z,v,y",    0, (int) M_DMUL,    INSN_MACRO, },
{"dmult",   "x,y",      0xe81c, 0xf81f,     RD_x|RD_y|WR_HI|WR_LO,  },
{"dmultu",  "x,y",      0xe81d, 0xf81f,     RD_x|RD_y|WR_HI|WR_LO,  },
{"drem",    "0,x,y",    0xe81e, 0xf81f,     RD_x|RD_y|WR_HI|WR_LO,  },
{"drem",    "z,v,y",    0, (int) M_DREM_3,  INSN_MACRO, },
{"dremu",   "0,x,y",    0xe81f, 0xf81f,     RD_x|RD_y|WR_HI|WR_LO,  },
{"dremu",   "z,v,y",    0, (int) M_DREMU_3, INSN_MACRO, },
{"dsllv",   "y,x",      0xe814, 0xf81f,     WR_y|RD_y|RD_x, },
{"dsll",    "x,w,[",    0x3001, 0xf803,     WR_x|RD_y, },
{"dsll",    "y,x",      0xe814, 0xf81f,     WR_y|RD_y|RD_x, },
{"dsrav",   "y,x",      0xe817, 0xf81f,     WR_y|RD_y|RD_x, },
{"dsra",    "y,]",      0xe813, 0xf81f,     WR_y|RD_y, },
{"dsra",    "y,x",      0xe817, 0xf81f,     WR_y|RD_y|RD_x, },
{"dsrlv",   "y,x",      0xe816, 0xf81f,     WR_y|RD_y|RD_x, },
{"dsrl",    "y,]",      0xe808, 0xf81f,     WR_y|RD_y, },
{"dsrl",    "y,x",      0xe816, 0xf81f,     WR_y|RD_y|RD_x, },
{"dsubu",   "z,v,y",    0xe002, 0xf803,     WR_z|RD_x|RD_y, },
{"dsubu",   "y,x,4",    0, (int) M_DSUBU_I, INSN_MACRO, },
{"dsubu",   "y,j",      0, (int) M_DSUBU_I_2, INSN_MACRO, },
{"exit",    "L",        0xed09, 0xff1f,     TRAP, },
{"exit",    "L",        0xee09, 0xff1f,     TRAP, },
{"exit",    "L",        0xef09, 0xff1f,     TRAP, },
{"entry",   "l",        0xe809, 0xf81f,     TRAP, },
{"jalr",    "x",        0xe840, 0xf8ff,     UBD|WR_31|RD_x, },
{"jalr",    "R,x",      0xe840, 0xf8ff,     UBD|WR_31|RD_x, },
{"jal",     "x",        0xe840, 0xf8ff,     UBD|WR_31|RD_x, },
{"jal",     "R,x",      0xe840, 0xf8ff,     UBD|WR_31|RD_x, },
{"jal",     "a",        0x1800, 0xfc00,     UBD|WR_31, },
{"jalx",    "a",        0x1c00, 0xfc00,     UBD|WR_31, },
{"jr",      "x",        0xe800, 0xf8ff,     UBD|RD_x, },
{"jr",      "R",        0xe820, 0xffff,     UBD|RD_31, },
{"j",       "x",        0xe800, 0xf8ff,     UBD|RD_x, },
{"j",       "R",        0xe820, 0xffff,     UBD|RD_31, },
{"lb",      "y,5(x)",   0x8000, 0xf800,     WR_y|RD_x, },
{"lbu",     "y,5(x)",   0xa000, 0xf800,     WR_y|RD_x, },
{"ld",      "y,D(x)",   0x3800, 0xf800,     WR_y|RD_x, },
{"ld",      "y,B",      0xfc00, 0xff00,     WR_y|RD_PC, },
{"ld",      "y,D(P)",   0xfc00, 0xff00,     WR_y|RD_PC, },
{"ld",      "y,D(S)",   0xf800, 0xff00,     WR_y|RD_SP, },
{"lh",      "y,H(x)",   0x8800, 0xf800,     WR_y|RD_x, },
{"lhu",     "y,H(x)",   0xa800, 0xf800,     WR_y|RD_x, },
{"li",      "x,U",      0x6800, 0xf800,     WR_x, },
{"lw",      "y,W(x)",   0x9800, 0xf800,     WR_y|RD_x, },
{"lw",      "x,A",      0xb000, 0xf800,     WR_x|RD_PC, },
{"lw",      "x,V(P)",   0xb000, 0xf800,     WR_x|RD_PC, },
{"lw",      "x,V(S)",   0x9000, 0xf800,     WR_x|RD_SP, },
{"lwu",     "y,W(x)",   0xb800, 0xf800,     WR_y|RD_x, },
{"mfhi",    "x",        0xe810, 0xf8ff,     WR_x|RD_HI, },
{"mflo",    "x",        0xe812, 0xf8ff,     WR_x|RD_LO, },
{"move",    "y,X",      0x6700, 0xff00,     WR_y|RD_X, },
{"move",    "Y,Z",      0x6500, 0xff00,     WR_Y|RD_Z, },
{"mul",     "z,v,y",    0, (int) M_MUL,     INSN_MACRO, },
{"mult",    "x,y",      0xe818, 0xf81f,     RD_x|RD_y|WR_HI|WR_LO,  },
{"multu",   "x,y",      0xe819, 0xf81f,     RD_x|RD_y|WR_HI|WR_LO,  },
{"neg",     "x,w",      0xe80b, 0xf81f,     WR_x|RD_y, },
{"not",     "x,w",      0xe80f, 0xf81f,     WR_x|RD_y, },
{"or",      "x,y",      0xe80d, 0xf81f,     WR_x|RD_x|RD_y, },
{"rem",     "0,x,y",    0xe81a, 0xf81f,     RD_x|RD_y|WR_HI|WR_LO,  },
{"rem",     "z,v,y",    0, (int) M_REM_3,   INSN_MACRO, },
{"remu",    "0,x,y",    0xe81b, 0xf81f,     RD_x|RD_y|WR_HI|WR_LO,  },
{"remu",    "z,v,y",    0, (int) M_REMU_3,  INSN_MACRO, },
{"sb",      "y,5(x)",   0xc000, 0xf800,     RD_y|RD_x, },
{"sd",      "y,D(x)",   0x7800, 0xf800,     RD_y|RD_x, },
{"sd",      "y,D(S)",   0xf900, 0xff00,     RD_y|RD_PC, },
{"sd",      "R,C(S)",   0xfa00, 0xff00,     RD_31|RD_PC, },
{"sh",      "y,H(x)",   0xc800, 0xf800,     RD_y|RD_x, },
{"sllv",    "y,x",      0xe804, 0xf81f,     WR_y|RD_y|RD_x, },
{"sll",     "x,w,<",    0x3000, 0xf803,     WR_x|RD_y, },
{"sll",     "y,x",      0xe804, 0xf81f,     WR_y|RD_y|RD_x, },
{"slti",    "x,8",      0x5000, 0xf800,     WR_T|RD_x, },
{"slt",     "x,y",      0xe802, 0xf81f,     WR_T|RD_x|RD_y, },
{"slt",     "x,8",      0x5000, 0xf800,     WR_T|RD_x, },
{"sltiu",   "x,8",      0x5800, 0xf800,     WR_T|RD_x, },
{"sltu",    "x,y",      0xe803, 0xf81f,     WR_T|RD_x|RD_y, },
{"sltu",    "x,8",      0x5800, 0xf800,     WR_T|RD_x, },
{"srav",    "y,x",      0xe807, 0xf81f,     WR_y|RD_y|RD_x, },
{"sra",     "x,w,<",    0x3003, 0xf803,     WR_x|RD_y, },
{"sra",     "y,x",      0xe807, 0xf81f,     WR_y|RD_y|RD_x, },
{"srlv",    "y,x",      0xe806, 0xf81f,     WR_y|RD_y|RD_x, },
{"srl",     "x,w,<",    0x3002, 0xf803,     WR_x|RD_y, },
{"srl",     "y,x",      0xe806, 0xf81f,     WR_y|RD_y|RD_x, },
{"subu",    "z,v,y",    0xe003, 0xf803,     WR_z|RD_x|RD_y, },
{"subu",    "y,x,4",    0, (int) M_SUBU_I,  INSN_MACRO, },
{"subu",    "x,k",      0, (int) M_SUBU_I_2, INSN_MACRO, },
{"sw",      "y,W(x)",   0xd800, 0xf800,     RD_y|RD_x, },
{"sw",      "x,V(S)",   0xd000, 0xf800,     RD_x|RD_SP, },
{"sw",      "R,V(S)",   0x6200, 0xff00,     RD_31|RD_SP, },
{"xor",     "x,y",      0xe80e, 0xf81f,     WR_x|RD_x|RD_y, },
  /* MIPS16e additions */
{"jalrc",   "x",        0xe8c0, 0xf8ff,     UBR|WR_31|RD_x|TRAP,    },
{"jalrc",   "R,x",      0xe8c0, 0xf8ff,     UBR|WR_31|RD_x|TRAP,    },
{"jrc",     "x",        0xe880, 0xf8ff,     UBR|RD_x|TRAP, },
{"jrc",     "R",        0xe8a0, 0xffff,     UBR|RD_31|TRAP, },
{"restore", "M",        0x6400, 0xff80,     WR_31|RD_SP|WR_SP|TRAP, },
{"save",    "m",        0x6480, 0xff80,     RD_31|RD_SP|WR_SP|TRAP, },
{"sdbbp",   "6",        0xe801, 0xf81f,     TRAP, },
{"seb",     "x",        0xe891, 0xf8ff,     WR_x|RD_x, },
{"seh",     "x",        0xe8b1, 0xf8ff,     WR_x|RD_x, },
{"sew",     "x",        0xe8d1, 0xf8ff,     WR_x|RD_x, },
{"zeb",     "x",        0xe811, 0xf8ff,     WR_x|RD_x, },
{"zeh",     "x",        0xe831, 0xf8ff,     WR_x|RD_x, },
{"zew",     "x",        0xe851, 0xf8ff,     WR_x|RD_x, },
};

static const int mips16_num_opcodes =
  ((sizeof mips16_opcodes) / (sizeof (mips16_opcodes[0])));

#undef UBD
#undef UBR
#undef CBR
#undef WR_x
#undef WR_y
#undef WR_z
#undef WR_T
#undef WR_SP
#undef WR_31
#undef WR_Y
#undef RD_x
#undef RD_y
#undef RD_Z
#undef RD_T
#undef RD_SP
#undef RD_31
#undef RD_PC
#undef RD_X
#undef WR_HI
#undef WR_LO
#undef RD_HI
#undef RD_LO
#undef TRAP
