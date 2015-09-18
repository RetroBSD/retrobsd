/*
 * Assembler for MIPS32.
 * The syntax is compatible with GCC output.
 *
 * Copyright (C) 2011-2012 Serge Vakulenko, <serge@vak.ru>
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */
#ifdef CROSS
#   include <stdio.h>
#   include <nlist.h>
#else
#   include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <a.out.h>

#define WORDSZ          4               /* word size in bytes */

/*
 * Locals beginning with L or dot are stripped off by -X flag.
 */
#define IS_LOCAL(s)     ((s)->n_name[0] == 'L' || (s)->n_name[0] == '.')

/*
 * Types of lexemes.
 */
enum {
    LEOF = 1,           /* end of file */
    LEOL,               /* end of line */
    LNAME,              /* identifier */
    LREG,               /* machine register */
    LNUM,               /* integer number */
    LLSHIFT,            /* << */
    LRSHIFT,            /* >> */
    LASCII,             /* .ascii */
    LBSS,               /* .bss */
    LCOMM,              /* .comm */
    LDATA,              /* .data */
    LGLOBL,             /* .globl */
    LHALF,              /* .half */
    LSTRNG,             /* .strng */
    LRDATA,             /* .rdata */
    LTEXT,              /* .text */
    LEQU,               /* .equ */
    LWORD,              /* .word */
    LBYTE,              /* .byte */
    LSPACE,             /* .space */
    LFILE,              /* .file */
    LSECTION,           /* .section */
    LSYMTYPE,           /* @type of symbol */
    LSECTYPE,           /* @type of section */
    LPREVIOUS,          /* .previous */
    LGNUATTR,           /* .gnu_attribute */
    LALIGN,             /* .align */
    LSET,               /* .set */
    LENT,               /* .ent */
    LTYPE,              /* .type */
    LFRAME,             /* .frame */
    LMASK,              /* .mask */
    LFMASK,             /* .fmask */
    LEND,               /* .end */
    LSIZE,              /* .size */
    LIDENT,             /* .ident */
    LWEAK,              /* .weak */
    LLOCAL,             /* .local */
    LNAN,               /* .nan */
};

/*
 * Segment ids.
 */
enum {
    STEXT,
    SDATA,
    SSTRNG,
    SBSS,
    SEXT,
    SABS,               /* special case for getexpr() */
};

/*
 * Sizes of tables.
 * Hash sizes should be powers of 2!
 */
#define HASHSZ  1024            /* symbol name hash table size */
#define HCMDSZ  256             /* instruction hash table size */
#define STSIZE  (HASHSZ*9/10)   /* symbol name table size */
#define MAXRLAB 200             /* max relative (digit) labels */

/*
 * On second pass, hashtab[] is not needed.
 * We use it under name newindex[] to reindex symbol references
 * when -x or -X options are enabled.
 */
#define newindex hashtab

/*
 * Convert segment id to symbol type.
 */
const int segmtype [] = {
    N_TEXT,             /* STEXT */
    N_DATA,             /* SDATA */
    N_STRNG,            /* SSTRNG */
    N_BSS,              /* SBSS */
    N_UNDF,             /* SEXT */
    N_ABS,              /* SABS */
};

/*
 * Convert segment id to relocation type.
 */
const int segmrel [] = {
    RTEXT,              /* STEXT */
    RDATA,              /* SDATA */
    RSTRNG,             /* SSTRNG */
    RBSS,               /* SBSS */
    REXT,               /* SEXT */
    RABS,               /* SABS */
};

/*
 * Convert symbol type to segment id.
 */
const int typesegm [] = {
    SEXT,               /* N_UNDF */
    SABS,               /* N_ABS */
    STEXT,              /* N_TEXT */
    SDATA,              /* N_DATA */
    SBSS,               /* N_BSS */
    SSTRNG,             /* N_STRNG */
};

/*
 * Table of local (numeric) labels.
 */
struct labeltab {
    int num;
    int value;
};

#define RLAB_OFFSET     (1 << 23)       /* index offset of relative label */
#define RLAB_MAXVAL     1000000         /* max value of relative label */

/*
 * Main opcode table.
 */
struct optable {
    unsigned opcode;                    /* instruction code */
    const char *name;                   /* instruction name */
    unsigned type;                      /* flags */
    void (*func) (unsigned, struct reloc*); /* handler for pseudo-instructions */
};

/*
 * Flags of instruction formats.
 */
#define FRD1    (1 << 0)        /* rd, ... */
#define FRD2    (1 << 1)        /* .., rd, ... */
#define FRT1    (1 << 2)        /* rt, ... */
#define FRT2    (1 << 3)        /* .., rt, ... */
#define FRT3    (1 << 4)        /* .., .., rt */
#define FRS1    (1 << 5)        /* rs, ... */
#define FRS2    (1 << 6)        /* .., rs, ... */
#define FRS3    (1 << 7)        /* .., .., rs */
#define FRSB    (1 << 8)        /* ... (rs) */
#define FCODE   (1 << 9)        /* immediate shifted <<6 */
#define FDSLOT  (1 << 10)       /* have delay slot */
#define FOFF16  (1 << 11)       /* 16-bit relocatable offset */
#define FHIGH16 (1 << 12)       /* high 16-bit relocatable offset */
#define FOFF18  (1 << 13)       /* 18-bit PC-relative relocatable offset shifted >>2 */
#define FAOFF18 (1 << 14)       /* 18-bit PC-relative relocatable offset shifted >>2 */
#define FAOFF28 (1 << 15)       /* 28-bit absolute relocatable offset shifted >>2 */
#define FSA     (1 << 16)       /* 5-bit shift amount */
#define FSEL    (1 << 17)       /* optional 3-bit COP0 register select */
#define FSIZE   (1 << 18)       /* bit field size */
#define FMSB    (1 << 19)       /* bit field msb */
#define FRTD    (1 << 20)       /* set rt to rd number */
#define FCODE16 (1 << 21)       /* immediate shifted <<16 */
#define FMOD    (1 << 22)       /* modifies the first register */

/*
 * Implement pseudo-instructions.
 * TODO: bge, bgeu, bgt, bgtu, ble, bleu, blt, bltu
 */
void emit_li (unsigned, struct reloc*);
void emit_la (unsigned, struct reloc*);

const struct optable optable [] = {
    { 0x00000020,   "add",      FRD1 | FRS2 | FRT3 | FMOD },
    { 0x20000000,   "addi",     FRT1 | FRS2 | FOFF16 | FMOD },
    { 0x24000000,   "addiu",    FRT1 | FRS2 | FOFF16 | FMOD },
    { 0x00000021,   "addu",     FRD1 | FRS2 | FRT3 | FMOD },
    { 0x00000024,   "and",      FRD1 | FRS2 | FRT3 | FMOD },
    { 0x30000000,   "andi",     FRT1 | FRS2 | FOFF16 | FMOD },
    { 0x10000000,   "b",        FAOFF18 | FDSLOT },
    { 0x04110000,   "bal",      FAOFF18 | FDSLOT },
    { 0x10000000,   "beq",      FRS1 | FRT2 | FOFF18 | FDSLOT },
    { 0x50000000,   "beql",	FRS1 | FRT2 | FOFF18 | FDSLOT },
    { 0x10000000,   "beqz",	FRS1 | FOFF18 | FDSLOT },
    { 0x50000000,   "beqzl",	FRS1 | FOFF18 | FDSLOT },
    { 0x04010000,   "bgez",	FRS1 | FOFF18 | FDSLOT },
    { 0x04110000,   "bgezal",	FRS1 | FOFF18 | FDSLOT },
    { 0x04130000,   "bgezall",	FRS1 | FOFF18 | FDSLOT },
    { 0x04030000,   "bgezl",	FRS1 | FOFF18 | FDSLOT },
    { 0x1c000000,   "bgtz",	FRS1 | FOFF18 | FDSLOT },
    { 0x5c000000,   "bgtzl",	FRS1 | FOFF18 | FDSLOT },
    { 0x18000000,   "blez",	FRS1 | FOFF18 | FDSLOT },
    { 0x58000000,   "blezl",	FRS1 | FOFF18 | FDSLOT },
    { 0x04000000,   "bltz",	FRS1 | FOFF18 | FDSLOT },
    { 0x04100000,   "bltzal",	FRS1 | FOFF18 | FDSLOT },
    { 0x04120000,   "bltzall",	FRS1 | FOFF18 | FDSLOT },
    { 0x04020000,   "bltzl",	FRS1 | FOFF18 | FDSLOT },
    { 0x14000000,   "bne",	FRS1 | FRT2 | FOFF18 | FDSLOT },
    { 0x54000000,   "bnel",	FRS1 | FRT2 | FOFF18 | FDSLOT },
    { 0x14000000,   "bnez",	FRS1 | FOFF18 | FDSLOT },
    { 0x54000000,   "bnezl",	FRS1 | FOFF18 | FDSLOT },
    { 0x0000000d,   "break",	FCODE16 },
    { 0x70000021,   "clo",	FRD1 | FRS2 | FRTD | FMOD },
    { 0x70000020,   "clz",	FRD1 | FRS2 | FRTD | FMOD },
    { 0x4200001f,   "deret",	0 },
    { 0x41606000,   "di",	FRT1 | FMOD },
    { 0x0000001a,   "div",	FRS1 | FRT2 },
    { 0x0000001b,   "divu",	FRS1 | FRT2 },
    { 0x000000c0,   "ehb",	0 },
    { 0x41606020,   "ei",	FRT1 | FMOD },
    { 0x42000018,   "eret",	0 },
    { 0x7c000000,   "ext",	FRT1 | FRS2 | FSA | FSIZE | FMOD },
    { 0x7c000004,   "ins",	FRT1 | FRS2 | FSA | FMSB | FMOD },
    { 0x08000000,   "j",	FAOFF28 | FDSLOT },
    { 0x0c000000,   "jal",	FAOFF28 | FDSLOT },
    { 0x00000009,   "jalr",	FRD1 | FRS2 | FDSLOT },
    { 0x00000409,   "jalr.hb",	FRD1 | FRS2 | FDSLOT },
    { 0x00000008,   "jr",	FRS1 | FDSLOT },
    { 0x00000408,   "jr.hb",	FRS1 | FDSLOT },
    {          0,   "la",	FRT1 | FMOD, emit_la },
    { 0x80000000,   "lb",	FRT1 | FOFF16 | FRSB | FMOD },
    { 0x90000000,   "lbu",	FRT1 | FOFF16 | FRSB | FMOD },
    { 0x84000000,   "lh",	FRT1 | FOFF16 | FRSB | FMOD },
    { 0x94000000,   "lhu",	FRT1 | FOFF16 | FRSB | FMOD },
    {          0,   "li",	FRT1 | FMOD, emit_li },
    { 0xc0000000,   "ll",	FRT1 | FOFF16 | FRSB | FMOD },
    { 0x3c000000,   "lui",	FRT1 | FHIGH16 | FMOD },
    { 0x8c000000,   "lw",	FRT1 | FOFF16 | FRSB | FMOD },
    { 0x88000000,   "lwl",	FRT1 | FOFF16 | FRSB | FMOD },
    { 0x98000000,   "lwr",	FRT1 | FOFF16 | FRSB | FMOD },
    { 0x70000000,   "madd",	FRS1 | FRT2 | FMOD },
    { 0x70000001,   "maddu",	FRS1 | FRT2 | FMOD },
    { 0x40000000,   "mfc0",	FRT1 | FRD2 | FSEL | FMOD },
    { 0x00000010,   "mfhi",	FRD1 | FMOD },
    { 0x00000012,   "mflo",	FRD1 | FMOD },
    { 0x00000021,   "move",	FRD1 | FRS2 | FMOD },          // addu
    { 0x0000000b,   "movn",	FRD1 | FRS2 | FRT3 | FMOD },
    { 0x0000000a,   "movz",	FRD1 | FRS2 | FRT3 | FMOD },
    { 0x70000004,   "msub",	FRS1 | FRT2 | FMOD },
    { 0x70000005,   "msubu",	FRS1 | FRT2 | FMOD },
    { 0x40800000,   "mtc0",	FRT1 | FRD2 | FSEL },
    { 0x00000011,   "mthi",	FRS1 },
    { 0x00000013,   "mtlo",	FRS1 },
    { 0x70000002,   "mul",	FRD1 | FRS2 | FRT3 | FMOD },
    { 0x00000018,   "mult",	FRS1 | FRT2 },
    { 0x00000019,   "multu",	FRS1 | FRT2 },
    { 0x00000000,   "nop",	0 },
    { 0x00000027,   "nor",	FRD1 | FRS2 | FRT3 | FMOD },
    { 0x00000025,   "or",	FRD1 | FRS2 | FRT3 | FMOD },
    { 0x34000000,   "ori",	FRT1 | FRS2 | FOFF16 | FMOD },
    { 0x7c00003b,   "rdhwr",	FRT1 | FRD2 | FMOD },
    { 0x41400000,   "rdpgpr",	FRD1 | FRT2 | FMOD },
    { 0x00200002,   "ror",	FRD1 | FRT2 | FSA | FMOD },
    { 0x00000046,   "rorv",	FRD1 | FRT2 | FRS3 | FMOD },
    { 0xa0000000,   "sb",	FRT1 | FOFF16 | FRSB },
    { 0xe0000000,   "sc",	FRT1 | FOFF16 | FRSB },
    { 0x7000003f,   "sdbbp",	FCODE },
    { 0x7c000420,   "seb",	FRD1 | FRT2 },
    { 0x7c000620,   "seh",	FRD1 | FRT2 },
    { 0xa4000000,   "sh",	FRT1 | FOFF16 | FRSB },
    { 0x00000000,   "sll",	FRD1 | FRT2 | FSA | FMOD },
    { 0x00000004,   "sllv",	FRD1 | FRT2 | FRS3 | FMOD },
    { 0x0000002a,   "slt",	FRD1 | FRS2 | FRT3 | FMOD },
    { 0x28000000,   "slti",	FRT1 | FRS2 | FOFF16 | FMOD },
    { 0x2c000000,   "sltiu",	FRT1 | FRS2 | FOFF16 | FMOD },
    { 0x0000002b,   "sltu",	FRD1 | FRS2 | FRT3 | FMOD },
    { 0x00000003,   "sra",	FRD1 | FRT2 | FSA | FMOD },
    { 0x00000007,   "srav",	FRD1 | FRT2 | FRS3 | FMOD },
    { 0x00000002,   "srl",	FRD1 | FRT2 | FSA | FMOD },
    { 0x00000006,   "srlv",	FRD1 | FRT2 | FRS3 | FMOD },
    { 0x00000040,   "ssnop",	0 },
    { 0x00000022,   "sub",	FRD1 | FRS2 | FRT3 | FMOD },
    { 0x00000023,   "subu",	FRD1 | FRS2 | FRT3 | FMOD },
    { 0xac000000,   "sw",	FRT1 | FOFF16 | FRSB },
    { 0xa8000000,   "swl",	FRT1 | FOFF16 | FRSB },
    { 0xb8000000,   "swr",	FRT1 | FOFF16 | FRSB },
    { 0x0000000f,   "sync",	FCODE },
    { 0x0000000c,   "syscall",	FCODE },
    { 0x00000034,   "teq",	FRS1 | FRT2 | FCODE },
    { 0x040c0000,   "teqi",	FRS1 | FOFF16 },
    { 0x00000030,   "tge",	FRS1 | FRT2 | FCODE },
    { 0x04080000,   "tgei",	FRS1 | FOFF16 },
    { 0x04090000,   "tgeiu",	FRS1 | FOFF16 },
    { 0x00000031,   "tgeu",	FRS1 | FRT2 | FCODE },
    { 0x00000032,   "tlt",	FRS1 | FRT2 | FCODE },
    { 0x040a0000,   "tlti",	FRS1 | FOFF16 },
    { 0x040b0000,   "tltiu",	FRS1 | FOFF16 },
    { 0x00000033,   "tltu",	FRS1 | FRT2 | FCODE },
    { 0x00000036,   "tne",	FRS1 | FRT2 | FCODE },
    { 0x040e0000,   "tnei",	FRS1 | FOFF16 },
    { 0x42000020,   "wait",	FCODE },
    { 0x41c00000,   "wrpgpr",	FRD1 | FRT2 },
    { 0x7c0000a0,   "wsbh",	FRD1 | FRT2 | FMOD },
    { 0x00000026,   "xor",	FRD1 | FRS2 | FRT3 | FMOD },
    { 0x38000000,   "xori",	FRT1 | FRS2 | FOFF16 | FMOD },
    { 0,            0,          0 },
};

/*
 * Character classes.
 */
#define ISHEX(c)        (ctype[(c)&0377] & 1)
#define ISOCTAL(c)      (ctype[(c)&0377] & 2)
#define ISDIGIT(c)      (ctype[(c)&0377] & 4)
#define ISLETTER(c)     (ctype[(c)&0377] & 8)

const char ctype [256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,8,0,0,0,0,0,0,0,0,0,8,0,7,7,7,7,7,7,7,7,5,5,0,0,0,0,0,0,
    8,9,9,9,9,9,9,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,0,0,0,0,8,
    0,9,9,9,9,9,9,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,0,
};

FILE *sfile [SABS], *rfile [SABS];
unsigned count [SABS];
int segm;
char *infile, *outfile = "a.out";
char tfilename[] = "/tmp/asXXXXXX";
int line;                               /* Source line number */
int xflags, Xflag, uflag;
int stlength;                           /* Symbol table size in bytes */
int stalign;                            /* Symbol table alignment */
unsigned tbase, dbase, adbase, bbase;
struct nlist stab [STSIZE];
int stabfree;
char space [STSIZE*8];                  /* Area for symbol names */
int lastfree;                           /* Free space offset */
char name [256];
unsigned intval;
int extref;
int blexflag, backlex, blextype;
short hashtab [HASHSZ], hashctab [HCMDSZ];
struct labeltab labeltab [MAXRLAB];     /* relative labels */
int nlabels;
int mode_reorder = 1;                   /* .set reorder option (default) */
int mode_macro;                         /* .set macro option */
int mode_mips16;                        /* .set mips16 option */
int mode_micromips;                     /* .set micromips option */
int mode_at = 1;                        /* .set at option */
int reorder_full;                       /* instruction buffered for reorder */
unsigned reorder_word;                  /* buffered instruction... */
unsigned reorder_clobber;               /* ...modified this register */
struct reloc reorder_rel;               /* buffered relocation */
struct reloc relabs = { RABS };         /* absolute relocation */

int expr_flags;                         /* flags set by getexpr */
#define EXPR_GPREL  1                   /* gp relative relocation */
#define EXPR_HI     2                   /* %hi function */
#define EXPR_LO     4                   /* %lo function */

/* Forward declarations. */
unsigned getexpr (int *s);

/*
 * Fatal error message.
 */
void uerror (char *fmt, ...)
{
    va_list ap;

    va_start (ap, fmt);
    fprintf (stderr, "as: ");
    if (infile)
        fprintf (stderr, "%s, ", infile);
    if (line)
        fprintf (stderr, "%d: ", line);
    vfprintf (stderr, fmt, ap);
    va_end (ap);
    fprintf (stderr, "\n");
    exit (1);
}

/*
 * Read a 4-byte word from the file.
 * Little-endian.
 */
unsigned fgetword (f)
    FILE *f;
{
    unsigned int w;

    if (fread (&w, sizeof(w), 1, f) != 1)
        return 0;
    return w;
}

/*
 * Write a 4-byte word to the file.
 * Little-endian.
 */
void fputword (w, f)
    unsigned int w;
    FILE *f;
{
    fwrite (&w, sizeof(w), 1, f);
}

/*
* Read a relocation record: 1 to 6 bytes.
*/
void fgetrel (f, r)
    register FILE *f;
    register struct reloc *r;
{
    r->flags = getc (f);
    if ((r->flags & RSMASK) == REXT) {
        r->index = getc (f);
        r->index |= getc (f) << 8;
        r->index |= getc (f) << 16;
    }
    if ((r->flags & RFMASK) == RHIGH16 ||
        (r->flags & RFMASK) == RHIGH16S)
    {
        r->offset = getc (f);
        r->offset |= getc (f) << 8;
    }
}

/*
* Emit a relocation record: 1 to 6 bytes.
* Return a written length.
*/
unsigned fputrel (r, f)
    register struct reloc *r;
    register FILE *f;
{
    register unsigned nbytes = 1;

    putc (r->flags, f);
    if ((r->flags & RSMASK) == REXT) {
        putc (r->index, f);
        putc (r->index >> 8, f);
        putc (r->index >> 16, f);
        nbytes += 3;
    }
    if ((r->flags & RFMASK) == RHIGH16 ||
        (r->flags & RFMASK) == RHIGH16S)
    {
        putc (r->offset, f);
        putc (r->offset >> 8, f);
        nbytes += 2;
    }
    return nbytes;
}

/*
 * Write the a.out header to the file.
 * Little-endian.
 */
void fputhdr (filhdr, coutb)
    register struct exec *filhdr;
    register FILE *coutb;
{
    fputword (filhdr->a_magic, coutb);
    fputword (filhdr->a_text, coutb);
    fputword (filhdr->a_data, coutb);
    fputword (filhdr->a_bss, coutb);
    fputword (filhdr->a_reltext, coutb);
    fputword (filhdr->a_reldata, coutb);
    fputword (filhdr->a_syms, coutb);
    fputword (filhdr->a_entry, coutb);
}

/*
 * Emit the nlist record for the symbol.
 */
void fputsym (s, file)
    register struct nlist *s;
    register FILE *file;
{
    register int i;

    putc (s->n_len, file);
    putc (s->n_type & ~N_LOC, file);
    fputword (s->n_value, file);
    for (i=0; i<s->n_len; i++)
        putc (s->n_name[i], file);
}

/*
 * Create temporary files for STEXT, SDATA and SSTRNG segments.
 */
void startup ()
{
    register int i;

    int fd = mkstemp (tfilename);
    if (fd == -1) {
        uerror ("cannot create temporary file %s", tfilename);
    } else {
        close(fd);
    }
    for (i=STEXT; i<SBSS; i++) {
        sfile [i] = fopen (tfilename, "w+");
        if (! sfile [i])
            uerror ("cannot open %s", tfilename);
        unlink (tfilename);
        rfile [i] = fopen (tfilename, "w+");
        if (! rfile [i])
            uerror ("cannot open %s", tfilename);
        unlink (tfilename);
    }
    line = 1;
}

/*
 * Suboptimal 32-bit hash function.
 * Copyright (C) 2006 Serge Vakulenko.
 */
unsigned hash_rot13 (s)
    register const char *s;
{
    register unsigned hash, c;

    hash = 0;
    while ((c = (unsigned char) *s++) != 0) {
        hash += c;
        hash -= (hash << 13) | (hash >> 19);
    }
    return hash;
}

void hashinit ()
{
    register int i, h;
    register const struct optable *p;

    for (i=0; i<HCMDSZ; i++)
        hashctab[i] = -1;
    for (p=optable; p->name; p++) {
        h = hash_rot13 (p->name) & (HCMDSZ-1);
        while (hashctab[h] != -1)
            if (--h < 0)
                h += HCMDSZ;
        hashctab[h] = p - optable;
    }
    for (i=0; i<HASHSZ; i++)
        hashtab[i] = -1;
}

int hexdig (c)
    register int c;
{
    if (c <= '9')
        return (c - '0');
    else if (c <= 'F')
        return (c - 'A' + 10);
    else
        return (c - 'a' + 10);
}

/*
 * Get hexadecimal number 0xZZZ
 */
void gethnum ()
{
    register int c;
    register char *cp;

    c = getchar ();
    for (cp=name; ISHEX(c); c=getchar())
        *cp++ = hexdig (c);
    ungetc (c, stdin);
    intval = 0;
    for (c=0; c<32; c+=4) {
        if (--cp < name)
            return;
        intval |= *cp << c;
    }
}

/*
 * Get a number.
 * 1234 - decimal
 * 01234 - octal
 */
void getnum (c)
    register int c;
{
    register char *cp;
    int leadingzero;

    leadingzero = (c=='0');
    for (cp=name; ISDIGIT(c); c=getchar())
        *cp++ = hexdig (c);
    ungetc (c, stdin);
    intval = 0;
    if (leadingzero) {
        /* Octal. */
        for (c=0; c<=27; c+=3) {
            if (--cp < name)
                return;
            intval |= *cp << c;
        }
        if (--cp < name)
            return;
        intval |= *cp << 30;
        return;
    }
    /* Decimal. */
    for (c=1; ; c*=10) {
        if (--cp < name)
            return;
        intval += *cp * c;
    }
}

void getname (c)
    register int c;
{
    register char *cp;

    for (cp=name; ISLETTER (c) || ISDIGIT (c); c=getchar())
        *cp++ = c;
    *cp = 0;
    ungetc (c, stdin);
}

int looktype ()
{
    switch (name [1]) {
    case 'c':
        if (! strcmp ("@common", name)) return (LSYMTYPE);
        break;
    case 'f':
        if (! strcmp ("@fini_array", name)) return (LSECTYPE);
        if (! strcmp ("@function", name)) return (LSYMTYPE);
        break;
    case 'g':
        if (! strcmp ("@gnu_indirect_function", name)) return (LSYMTYPE);
        if (! strcmp ("@gnu_unique_object", name)) return (LSYMTYPE);
        break;
    case 'i':
        if (! strcmp ("@init_array", name)) return (LSECTYPE);
        break;
    case 'n':
        if (! strcmp ("@nobits", name)) return (LSECTYPE);
        if (! strcmp ("@note", name)) return (LSECTYPE);
        if (! strcmp ("@notype", name)) return (LSYMTYPE);
        break;
    case 'o':
        if (! strcmp ("@object", name)) return (LSYMTYPE);
        break;
    case 'p':
        if (! strcmp ("@progbits", name)) return (LSECTYPE);
        if (! strcmp ("@preinit_array", name)) return (LSECTYPE);
        break;
    case 't':
        if (! strcmp ("@tls_object", name)) return (LSYMTYPE);
        break;
    }
    return (-1);
}

int lookacmd ()
{
    switch (name [1]) {
    case 'a':
        if (! strcmp (".ascii", name)) return (LASCII);
        if (! strcmp (".align", name)) return (LALIGN);
        break;
    case 'b':
        if (! strcmp (".bss", name)) return (LBSS);
        if (! strcmp (".byte", name)) return (LBYTE);
        break;
    case 'c':
        if (! strcmp (".comm", name)) return (LCOMM);
        break;
    case 'd':
        if (! strcmp (".data", name)) return (LDATA);
        break;
    case 'e':
        if (! strcmp (".equ", name)) return (LEQU);
        if (! strcmp (".end", name)) return (LEND);
        if (! strcmp (".ent", name)) return (LENT);
        break;
    case 'f':
        if (! strcmp (".file", name)) return (LFILE);
        if (! strcmp (".fmask", name)) return (LFMASK);
        if (! strcmp (".frame", name)) return (LFRAME);
        break;
    case 'g':
        if (! strcmp (".globl", name)) return (LGLOBL);
        if (! strcmp (".gnu_attribute", name)) return (LGNUATTR);
        break;
    case 'h':
        if (! strcmp (".half", name)) return (LHALF);
        break;
    case 'i':
        if (! strcmp (".ident", name)) return (LIDENT);
        break;
    case 'l':
        if (! strcmp (".local", name)) return (LLOCAL);
        break;
    case 'm':
        if (! strcmp (".mask", name)) return (LMASK);
        break;
    case 'n':
        if (! strcmp (".nan", name)) return (LNAN);
        break;
    case 'p':
        if (! strcmp (".previous", name)) return (LPREVIOUS);
        break;
    case 'r':
        if (! strcmp (".rdata", name)) return (LRDATA);
        break;
    case 's':
        if (! strcmp (".section", name)) return (LSECTION);
        if (! strcmp (".set", name)) return (LSET);
        if (! strcmp (".size", name)) return (LSIZE);
        if (! strcmp (".space", name)) return (LSPACE);
        if (! strcmp (".strng", name)) return (LSTRNG);
        break;
    case 't':
        if (! strcmp (".text", name)) return (LTEXT);
        if (! strcmp (".type", name)) return (LTYPE);
        break;
    case 'w':
        if (! strcmp (".word", name)) return (LWORD);
        if (! strcmp (".weak", name)) return (LWEAK);
        break;
    }
    return (-1);
}

/*
 * Change a segment based on a section name.
 */
void setsection ()
{
    struct {
        const char *name;
        int len;
        int segm;
    } const *p, map[] = {
        { ".text",   5, STEXT  },
        { ".data",   5, SDATA  },
        { ".sdata",  6, SDATA  },
        { ".rodata", 7, SSTRNG },
        { ".bss",    4, SBSS   },
        { ".sbss",   5, SBSS   },
        { ".mdebug", 7, SSTRNG },
        { 0 },
    };

    for (p=map; p->name; p++) {
        if (strncmp (name, p->name, p->len) == 0 &&
            (p->name [p->len] == 0 ||
             p->name [p->len] == '.'))
        {
            segm = p->segm;
            return;
        }
    }
    uerror ("bad .section name");
}

int lookreg ()
{
    int val;
    char *cp;

    switch (name [1]) {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        val = 0;
        for (cp=name+1; ISDIGIT (*cp); cp++) {
            val *= 10;
            val += *cp - '0';
        }
        if (*cp != 0)
            break;
        return val;
    case 'a':
        if (name[3] == 0) {
            switch (name[2]) {
            case '0': return 4;         /* $a0 */
            case '1': return 5;         /* $a1 */
            case '2': return 6;         /* $a2 */
            case '3': return 7;         /* $a3 */
            case 't': return 1;         /* $at */
            }
        }
        break;
    case 'f':
        if (name[3] == 0 && name[2] == 'p')
            return 30;                  /* $fp */
        break;
    case 'g':
        if (name[3] == 0 && name[2] == 'p')
            return 28;                  /* $gp */
        break;
    case 'k':
        if (name[3] == 0) {
            switch (name[2]) {
            case '0': return 26;        /* $k0 */
            case '1': return 27;        /* $k1 */
            }
        }
        break;
    case 'r':
        if (name[3] == 0 && name[2] == 'a')
            return 31;                  /* $ra */
        break;
    case 's':
        if (name[3] == 0) {
            switch (name[2]) {
            case '0': return 16;        /* $s0 */
            case '1': return 17;        /* $s1 */
            case '2': return 18;        /* $s2 */
            case '3': return 19;        /* $s3 */
            case '4': return 20;        /* $s4 */
            case '5': return 21;        /* $s5 */
            case '6': return 22;        /* $s6 */
            case '7': return 23;        /* $s7 */
            case '8': return 30;        /* $s8 */
            case 'p': return 29;        /* $sp */
            }
        }
        break;
    case 't':
        if (name[3] == 0) {
            switch (name[2]) {
            case '0': return 8;         /* $t0 */
            case '1': return 9;         /* $t1 */
            case '2': return 10;        /* $t2 */
            case '3': return 11;        /* $t3 */
            case '4': return 12;        /* $t4 */
            case '5': return 13;        /* $t5 */
            case '6': return 14;        /* $t6 */
            case '7': return 15;        /* $t7 */
            case '8': return 24;        /* $t8 */
            case '9': return 25;        /* $t9 */
            }
        }
        break;
    case 'v':
        if (name[3] == 0) {
            switch (name[2]) {
            case '0': return 2;         /* $v0 */
            case '1': return 3;         /* $v1 */
            }
        }
        break;
    case 'z':
        if (! strcmp (name+2, "ero"))
            return 0;                   /* $zero */
        break;
    }
    return -1;
}

int lookcmd ()
{
    register int i, h;

    h = hash_rot13 (name) & (HCMDSZ-1);
    while ((i = hashctab[h]) != -1) {
        if (! strcmp (optable[i].name, name))
            return (i);
        if (--h < 0)
            h += HCMDSZ;
    }
    return (-1);
}

char *alloc (len)
{
    register int r;

    r = lastfree;
    lastfree += len;
    if (lastfree > sizeof(space))
        uerror ("out of memory");
    return (space + r);
}

int lookname ()
{
    register int i, h;

    /* Search for symbol name. */
    h = hash_rot13 (name) & (HASHSZ-1);
    while ((i = hashtab[h]) != -1) {
        if (! strcmp (stab[i].n_name, name))
            return (i);
        if (--h < 0)
            h += HASHSZ;
    }

    /* Add a new symbol to table. */
    if ((i = stabfree++) >= STSIZE)
        uerror ("symbol table overflow");
    stab[i].n_len = strlen (name);
    stab[i].n_name = alloc (1 + stab[i].n_len);
    strcpy (stab[i].n_name, name);
    stab[i].n_value = 0;
    stab[i].n_type = N_UNDF;
    hashtab[h] = i;
    return (i);
}

/*
 * Read a lexical element, return it's type and store a value into *val.
 * Returned type codes:
 * LEOL    - End of line.  Value is a line number.
 * LEOF    - End of file.
 * LNUM    - Integer value (into intval), *val undefined.
 * LNAME   - Identifier.  String value is in name[] array.
 * LREG    - Machine register.  Value is a register number.
 * LLSHIFT - << operator.
 * LRSHIFT - >> operator.
 * LASCII  - .ascii assembler instruction.
 * LBSS    - .bss assembler instruction.
 * LCOMM   - .comm assembler instruction.
 * LDATA   - .data assembler instruction.
 * LGLOBL  - .globl assembler instruction.
 * LHALF   - .half assembler instruction.
 * LSTRNG  - .strng assembler instruction.
 * LTEXT   - .text assembler instruction.
 * LEQU    - .equ assembler instruction.
 * LWORD   - .word assembler instruction.
 * LFILE   - .file assembler instruction.
 * LSECTION - .section assembler instruction.
 */
int getlex (pval)
    register int *pval;
{
    register int c;

    if (blexflag) {
        blexflag = 0;
        *pval = blextype;
        return (backlex);
    }
    for (;;) {
        switch (c = getchar()) {
        case '#':
skiptoeol:  while ((c = getchar()) != '\n')
                if (c == EOF)
                    return (LEOF);
        case '\n':
            ++line;
            c = getchar ();
            if (c == '#')
                goto skiptoeol;
            ungetc (c, stdin);
        case ';':
            *pval = line;
            return (LEOL);
        case ' ':
        case '\t':
            continue;
        case EOF:
            return (LEOF);
        case '\\':
            c = getchar ();
            if (c=='<')
                return (LLSHIFT);
            if (c=='>')
                return (LRSHIFT);
            ungetc (c, stdin);
            return ('\\');
        case '\'':      case '%':
        case '^':       case '&':       case '|':       case '~':
        case '+':       case '-':       case '*':       case '/':
        case '"':       case ',':       case '[':       case ']':
        case '(':       case ')':       case '{':       case '}':
        case '<':       case '>':       case '=':       case ':':
            return (c);
        case '0':
            if ((c = getchar ()) == 'x' || c=='X') {
                gethnum ();
                return (LNUM);
            }
            ungetc (c, stdin);
            c = '0';
        case '1':       case '2':       case '3':
        case '4':       case '5':       case '6':       case '7':
        case '8':       case '9':
            getnum (c);
            return (LNUM);
        default:
            if (! ISLETTER (c))
                uerror ("bad character: \\%o", c & 0377);
            getname (c);
            if (name[0] == '.') {
                if (name[1] == 0)
                    return ('.');
                *pval = lookacmd();
                if (*pval != -1)
                    return (*pval);
            }
            if (name[0] == '$') {
                *pval = lookreg();
                if (*pval != -1)
                    return (LREG);
            }
            if (name[0] == '@') {
                *pval = looktype();
                if (*pval != -1)
                    return (*pval);
            }
            return (LNAME);
        }
    }
}

void ungetlex (val, type)
{
    blexflag = 1;
    backlex = val;
    blextype = type;
}

int getterm ()
{
    register int ty;
    int cval, s;

    switch (getlex (&cval)) {
    default:
        uerror ("operand missed");
    case LNUM:
        cval = getchar ();
        if (cval == 'b' || cval == 'B')
            extref = RLAB_OFFSET - intval;
        else if (cval == 'f' || cval == 'F')
            extref = RLAB_OFFSET + intval;
        else {
            /* Integer literal. */
            ungetc (cval, stdin);
            return (SABS);
        }
        /* Local label. */
        if (intval >= RLAB_MAXVAL)
            uerror ("too large relative label");
        intval = 0;
        return (SEXT);
    case LNAME:
        intval = 0;
        cval = lookname();
        ty = stab[cval].n_type & N_TYPE;
        if (ty==N_UNDF || ty==N_COMM) {
            extref = cval;
            return (SEXT);
        }
        intval = stab[cval].n_value;
        return (typesegm [ty]);
    case '.':
        intval = count[segm];
        return (segm);
    case '%':
        if (getlex (&cval) != LNAME)
            uerror ("bad %function name");
        if (strcmp (name, "gp_rel") == 0) {
            /* GP relative reverence. */
            expr_flags |= EXPR_GPREL;
        } else if (strcmp (name, "hi") == 0) {
            expr_flags |= EXPR_HI;
        } else if (strcmp (name, "lo") == 0) {
            expr_flags |= EXPR_LO;
        } else
            uerror ("unknown function %s", name);
        if (getlex (&cval) != '(')
            uerror ("bad %s syntax", name);
        /* fall through */
    case '(':
        getexpr (&s);
        if (getlex (&cval) != ')')
            uerror ("bad () syntax");
        return (s);
    }
}

/*
 * Get an expression.
 * Return a value, put a base segment id to *s.
 * A copy of value is saved in intval.
 *
 * expression = [term] {op term}...
 * term       = LNAME | LNUM | "." | "(" expression ")"
 * op         = "+" | "-" | "&" | "|" | "^" | "~" | "<<" | ">>" | "/" | "*"
 */
unsigned getexpr (s)
    register int *s;
{
    register int clex;
    int cval, s2;
    unsigned rez;

    /* look a first lexeme */
    switch (clex = getlex (&cval)) {
    default:
        ungetlex (clex, cval);
        rez = 0;
        *s = SABS;
        break;
    case LNUM:
    case LNAME:
    case '.':
    case '(':
    case '%':
        ungetlex (clex, cval);
        *s = getterm ();
        rez = intval;
        break;
    }
    for (;;) {
        switch (clex = getlex (&cval)) {
        case '+':
            s2 = getterm ();
            if (*s == SABS)
                *s = s2;
            else if (s2 != SABS)
                uerror ("too complex expression");
            rez += intval;
            break;
        case '-':
            s2 = getterm ();
            if (s2 == *s && s2 != SEXT)
                *s = SABS;
            else if (s2 != SABS)
                uerror ("too complex expression");
            rez -= intval;
            break;
        case '&':
            s2 = getterm ();
            if (*s != SABS || s2 != SABS)
                uerror ("too complex expression");
            rez &= intval;
            break;
        case '|':
            s2 = getterm ();
            if (*s != SABS || s2 != SABS)
                uerror ("too complex expression");
            rez |= intval;
            break;
        case '^':
            s2 = getterm ();
            if (*s != SABS || s2 != SABS)
                uerror ("too complex expression");
            rez ^= intval;
            break;
        case '~':
            s2 = getterm ();
            if (*s != SABS || s2 != SABS)
                uerror ("too complex expression");
            rez ^= ~intval;
            break;
        case LLSHIFT:           /* сдвиг влево */
            s2 = getterm ();
            if (*s != SABS || s2 != SABS)
                uerror ("too complex expression");
            rez <<= intval & 037;
            break;
        case LRSHIFT:           /* сдвиг вправо */
            s2 = getterm ();
            if (*s != SABS || s2 != SABS)
                uerror ("too complex expression");
            rez >>= intval & 037;
            break;
        case '*':
            s2 = getterm ();
            if (*s != SABS || s2 != SABS)
                uerror ("too complex expression");
            rez *= intval;
            break;
        case '/':
            s2 = getterm ();
            if (*s != SABS || s2 != SABS)
                uerror ("too complex expression");
            if (intval == 0)
                uerror ("division by zero");
            rez /= intval;
            break;
        default:
            ungetlex (clex, cval);
            intval = rez;
            return (rez);
        }
    }
    /* NOTREACHED */
}

void reorder_flush ()
{
    if (reorder_full) {
        fputword (reorder_word, sfile[STEXT]);
        fputrel (&reorder_rel, rfile[STEXT]);
        reorder_full = 0;
    }
}

/*
 * Default emit function.
 */
void emitword (w, r, clobber_reg)
    register unsigned w;
    register struct reloc *r;
    int clobber_reg;
{
    if (mode_reorder && segm == STEXT) {
        reorder_flush();
        reorder_word = w;
        reorder_rel = *r;
        reorder_full = 1;
        reorder_clobber = clobber_reg & 31;
    } else {
        fputword (w, sfile[segm]);
        fputrel (r, rfile[segm]);
    }
    count[segm] += WORDSZ;
}

/*
 * LI pseudo instruction.
 */
void emit_li (opcode, relinfo)
    register unsigned opcode;
    register struct reloc *relinfo;
{
    register unsigned value;
    int cval, segment, reg;

    if (getlex (&cval) != ',')
        uerror ("comma expected");
    value = getexpr (&segment);
    reg = opcode >> 16;
    if (segment != SABS)
        uerror ("absolute value required");
    if (value <= 0xffff) {
        /* ori d, $zero, value */
        opcode |= 0x34000000 | value;
    } else if (value >= -0x8000) {
        /* addiu d, $zero, value */
        opcode |= 0x24000000 | (value & 0xffff);
    } else if ((value & 0xffff) == 0) {
        /* lui d, value[31:16] */
        opcode |= 0x3c000000 | (value >> 16);
    } else {
        /* lui d, value[31:16]
         * ori d, d, value[15:0]) */
        emitword (opcode | 0x3c000000 | (value >> 16), &relabs, reg);
        opcode |= 0x34000000 | (opcode & 0x1f0000) << 5 | (value & 0xffff);
    }
    emitword (opcode, relinfo, reg);
}

/*
 * LA pseudo instruction.
 */
void emit_la (opcode, relinfo)
    register unsigned opcode;
    register struct reloc *relinfo;
{
    register unsigned value, hi;
    int cval, segment;

    if (getlex (&cval) != ',')
        uerror ("comma expected");
    expr_flags = 0;
    value = getexpr (&segment);
    if (segment == SABS)
	uerror ("relocatable value required");
    relinfo->flags = segmrel [segment];
    if (relinfo->flags == REXT)
	relinfo->index = extref;
    if (expr_flags & EXPR_GPREL)
	relinfo->flags |= RGPREL;

    /* lui d, %hi(value)
     * addiu d, d, %lo(value) */
    relinfo->flags |= RHIGH16S;
    relinfo->offset = value & 0xffff;
    hi = (value + 0x8000) >> 16;
    emitword (opcode | 0x3c000000 | hi, relinfo, hi);

    relinfo->flags &= ~RHIGH16S;
    opcode |= 0x24000000 | (opcode & 0x1f0000) << 5 | (value & 0xffff);
    emitword (opcode, relinfo, value >> 16);
}

/*
 * Build and emit a machine instruction code.
 */
void makecmd (opcode, type, emitfunc)
    unsigned opcode;
    void (*emitfunc) (unsigned, struct reloc*);
{
    unsigned offset, orig_opcode = 0;
    struct reloc relinfo;
    int clex, cval, segment, clobber_reg, negate_literal;

    offset = 0;
    relinfo.flags = RABS;
    negate_literal = 0;

    /*
     * GCC can generate "j" instead of "jr".
     * Need to detect it early.
     */
    if (type == (FAOFF28 | FDSLOT)) {
        clex = getlex (&cval);
        ungetlex (clex, cval);
        if (clex == LREG) {
            if (opcode == 0x08000000) { /* j - replace by jr */
                opcode = 0x00000008;
                type = FRS1 | FDSLOT;
            }
            if (opcode == 0x0c000000) { /* jal - replace by jalr */
                opcode = 0x00000009;
                type = FRD1 | FRS2 | FDSLOT;
            }
        }
    }

    /*
     * First register.
     */
    cval = 0;
    clobber_reg = 0;
    if (type & FRD1) {
        clex = getlex (&cval);
        if (clex != LREG)
            uerror ("bad rd register");
        opcode |= cval << 11;           /* rd, ... */
    }
    if (type & FRT1) {
        clex = getlex (&cval);
        if (clex != LREG)
            uerror ("bad rt register");
        opcode |= cval << 16;           /* rt, ... */
    }
    if (type & FRS1) {
frs1:   clex = getlex (&cval);
        if (clex != LREG)
            uerror ("bad rs register");
        if (cval == 0 && (opcode == 0x0000001a ||   /* div */
                          opcode == 0x0000001b)) {  /* divu */
            /* Div instruction with three args.
             * Treat it as a 2-arg variant. */
            if (getlex (&cval) != ',')
                uerror ("comma expected");
            goto frs1;
        }
        opcode |= cval << 21;           /* rs, ... */
    }
    if (type & FRTD) {
        opcode |= cval << 16;           /* rt equals rd */
    }
    if ((type & FMOD) && (type & (FRD1 | FRT1 | FRS1)))
        clobber_reg = cval;

    /*
     * Second register.
     */
    if (type & FRD2) {
        if (getlex (&cval) != ',')
            uerror ("comma expected");
        clex = getlex (&cval);
        if (clex != LREG)
            uerror ("bad rd register");
        opcode |= cval << 11;           /* .., rd, ... */
    }
    if (type & FRT2) {
        if (getlex (&cval) != ',')
            uerror ("comma expected");
        clex = getlex (&cval);
        if (clex != LREG) {
            if ((type & FRD1) && (type & FSA)) {
                /* Second register operand omitted.
                 * Need to restore the missing operand. */
                ungetlex (clex, cval);
                cval = (opcode >> 11) & 31; /* get 1-st register */
                opcode |= cval << 16;       /* use 1-st reg as 2-nd */
                goto fsa;
            }
            uerror ("bad rt register");
        }
        opcode |= cval << 16;           /* .., rt, ... */
    }
    if (type & FRS2) {
        clex = getlex (&cval);
        if (clex != ',') {
            if ((opcode & 0xfc00003f) != 0x00000009)
                uerror ("comma expected");
            /* Jalr with one argument.
             * Treat as if the first argument is $31. */
            ungetlex (clex, cval);
            cval = (opcode >> 11) & 31; /* get 1-st register */
            opcode |= cval << 21;       /* use 1-st reg as 2-nd */
            opcode |= 31 << 11;         /* set 1-st reg to 31 */
            clobber_reg = 31;
            goto done3;
        }
        clex = getlex (&cval);
        if (clex != LREG) {
            if ((type & FRT1) && (type & FOFF16)) {
                /* Second register operand omitted.
                 * Need to restore the missing operand. */
                ungetlex (clex, cval);
                cval = (opcode >> 16) & 31; /* get 1-st register */
                opcode |= cval << 21;       /* use 1-st reg as 2-nd */
                goto foff16;
            }
            uerror ("bad rs register");
        }
        opcode |= cval << 21;           /* .., rs, ... */
    }

    /*
     * Third register.
     */
    if (type & FRT3) {
        clex = getlex (&cval);
        if (clex != ',') {
            /* Three-operand instruction used with two operands.
             * Need to restore the missing operand. */
            ungetlex (clex, cval);
            cval = (opcode >> 21) & 31;
            opcode &= ~(31 << 21);                  /* clear 2-nd register */
            opcode |= ((opcode >> 11) & 31) << 21;  /* use 1-st reg as 2-nd */
            opcode |= cval << 16;                   /* add 3-rd register */
            goto done3;
        }
        clex = getlex (&cval);
        if (clex != LREG) {
            if ((type & FRD1) && (type & FRS2)) {
                /* Three-operand instruction used with literal operand.
                 * Convert it to immediate type. */
                unsigned newop;
                switch (opcode & 0xfc0007ff) {
                case 0x00000020: newop = 0x20000000; break; // add -> addi
                case 0x00000021: newop = 0x24000000; break; // addu -> addiu
                case 0x00000024: newop = 0x30000000; break; // and -> andi
                case 0x00000025: newop = 0x34000000; break; // or -> ori
                case 0x0000002a: newop = 0x28000000; break; // slt -> slti
                case 0x0000002b: newop = 0x2c000000; break; // sltu -> sltiu
                case 0x00000022: newop = 0x20000000;        // sub -> addi, negate
                                 negate_literal = 1; break;
                case 0x00000023: newop = 0x24000000;        // subu -> addiu, negate
                                 negate_literal = 1; break;
                case 0x00000026: newop = 0x38000000; break; // xor -> xori
                default:
                    uerror ("bad rt register");
                    return;
                }
                ungetlex (clex, cval);
                cval = (opcode >> 11) & 31;         /* get 1-st register */
                newop |= cval << 16;                /* set 1-st register */
                newop |= opcode & (31 << 21);       /* set 2-nd register */
                orig_opcode = opcode;
                opcode = newop;
                type = FRT1 | FRS2 | FOFF16 | FMOD;
                goto foff16;
            }
            uerror ("bad rt register");
        }
        opcode |= cval << 16;           /* .., .., rt */
    }
    if (type & FRS3) {
        clex = getlex (&cval);
        if (clex != ',') {
            /* Three-operand instruction used with two operands.
             * Need to restore the missing operand. */
            ungetlex (clex, cval);
            cval = (opcode >> 16) & 31;
            opcode &= ~(31 << 16);                  /* clear 2-nd register */
            opcode |= ((opcode >> 11) & 31) << 16;  /* use 1-st reg as 2-nd */
            opcode |= cval << 21;                   /* add 3-rd register */
            goto done3;
        }
        clex = getlex (&cval);
        if (clex != LREG)
            uerror ("bad rs register");
        opcode |= cval << 21;           /* .., .., rs */
    }
done3:

    /*
     * Immediate argument.
     */
    if (type & FSEL) {
        /* optional COP0 register select */
        clex = getlex (&cval);
        if (clex == ',') {
            offset = getexpr (&segment);
            if (segment != SABS)
                uerror ("absolute value required");
            opcode |= offset & 7;
        } else
            ungetlex (clex, cval);

    } else if (type & (FCODE | FCODE16 | FSA)) {
        /* Non-relocatable offset */
        if (type & FSA) {
            if (getlex (&cval) != ',')
                uerror ("comma expected");
            clex = getlex (&cval);
            if (clex == LREG && type == (FRD1 | FRT2 | FSA | FMOD)) {
                /* Literal-operand shift instruction used with register operand.
                 * Convert it to 3-register type. */
                unsigned newop;
                switch (opcode & 0xffe0003f) {
                case 0x00200002: newop = 0x00000046; break; // ror -> rorv
                case 0x00000000: newop = 0x00000004; break; // sll -> sllv
                case 0x00000003: newop = 0x00000007; break; // sra -> srav
                case 0x00000002: newop = 0x00000006; break; // srl -> srlv
                default:
                    uerror ("bad shift amount");
                    return;
                }
                newop |= opcode & (0x3ff << 11);    /* set 1-st and 2-nd regs */
                newop |= cval << 21;                /* set 3-rd register */
                opcode = newop;
                type = FRD1 | FRT2 | FRS3 | FMOD;
                goto done3;
            }
            ungetlex (clex, cval);
        }
        if ((type & FCODE) && (type & FRT2)) {
            /* Optional code for trap instruction. */
            clex = getlex (&cval);
            if (clex != ',') {
                ungetlex (clex, cval);
                goto done;
            }
        }
fsa:    offset = getexpr (&segment);
        if (segment != SABS)
            uerror ("absolute value required");
        switch (type & (FCODE | FCODE16 | FSA)) {
        case FCODE:                     /* immediate shifted <<6 */
            opcode |= offset << 6;
            break;
        case FCODE16:                   /* immediate shifted <<16 */
            opcode |= offset << 16;
            break;
        case FSA:                       /* shift amount */
            opcode |= (offset & 0x1f) << 6;
            break;
        }
    } else if (type & (FOFF16 | FOFF18 | FAOFF18 | FAOFF28 | FHIGH16)) {
        /* Relocatable offset */
        int valid_range;

        if ((type & (FOFF16 | FOFF18 | FHIGH16)) && getlex (&cval) != ',')
            uerror ("comma expected");
foff16: expr_flags = 0;
        offset = getexpr (&segment);
        relinfo.flags = segmrel [segment];
        if (relinfo.flags == REXT)
            relinfo.index = extref;
        if (expr_flags & EXPR_GPREL)
            relinfo.flags |= RGPREL;
        switch (type & (FOFF16 | FOFF18 | FAOFF18 | FAOFF28 | FHIGH16)) {
        case FOFF16:                    /* low 16-bit byte address */
            /* Test whether the immediate is in valid range
             * for the opcode. */
            if (negate_literal) {
                // Negate literal arg for sub and subu
                offset = -offset;
                if (relinfo.flags != RABS)
                    uerror ("cannot negate relocatable literal");
            }
            switch (opcode & 0xfc000000) {
            default:                    /* addi, addiu, slti, sltiu, lw, sw */
                /* 16-bit signed value. */
                valid_range = (offset >= -0x8000) || (offset <= 0x7fff);
                break;
            case 0x30000000:            /* andi */
            case 0x34000000:            /* ori */
            case 0x38000000:            /* xori */
                /* 16-bit unsigned value. */
                valid_range = (offset <= 0xffff);
                break;
            }
            if (valid_range) {
                opcode |= offset & 0xffff;
            } else if (orig_opcode == 0 || ! mode_at) {
                uerror ("value out of range");
            } else {
                /* Convert back to 3-reg opcode.
                 * Insert an extra LI instruction. */
                if (segment != SABS)
                    uerror ("absolute value required");
                if (negate_literal)
                    offset = -offset;

                if (offset <= 0xffff) {
                    /* ori $1, $zero, value */
                    emitword (0x34010000 | offset, &relabs, 1);
                } else if (offset >= -0x8000) {
                    /* addiu $1, $zero, value */
                    emitword (0x24010000 | (offset & 0xffff), &relabs, 1);
                } else if ((offset & 0xffff) == 0) {
                    /* lui $1, value[31:16] */
                    emitword (0x3c010000 | (offset >> 16), &relabs, 1);
                } else {
                    /* lui $1, value[31:16]
                     * ori $1, $1, value[15:0]) */
                    emitword (0x3c010000 | (offset >> 16), &relabs, 1);
                    emitword (0x34210000 | (offset & 0xffff), &relabs, 1);
                }
                opcode = orig_opcode | 0x10000;
            }
            break;
        case FHIGH16:                   /* high 16-bit byte address */
            if (expr_flags & EXPR_HI) {
                /* %hi function - assume signed offset */
                relinfo.flags |= RHIGH16S;
                relinfo.offset = offset & 0xffff;
                offset += 0x8000;
                opcode |= offset >> 16;
            } else {
                opcode |= offset & 0xffff;
            }
            break;
        case FOFF18:                    /* 18-bit PC-relative word address */
        case FAOFF18:
            if (segment == segm) {
                offset -= count[segm] + 4;
                relinfo.flags = RABS;
            } else if (segment == SEXT) {
                relinfo.flags |= RWORD16;
            } else
                uerror ("invalid segment %d", segment);
            opcode |= (offset >> 2) & 0xffff;
            break;
        case FAOFF28:                   /* 28-bit word address */
            opcode |= (offset >> 2) & 0x3ffffff;
            relinfo.flags |= RWORD26;
            break;
        }
    }

    /*
     * Last argument.
     */
    if (type & FRSB) {
        if (getlex (&cval) != '(')
            uerror ("left par expected");
        clex = getlex (&cval);
        if (clex != LREG)
            uerror ("bad rs register");
        if (getlex (&cval) != ')')
            uerror ("right par expected");
        opcode |= cval << 21;           /* ... (rs) */
    }
    if (type & FSIZE) {
        if (getlex (&cval) != ',')
            uerror ("comma expected");
        offset = getexpr (&segment);
        if (segment != SABS)
            uerror ("absolute value required");
        opcode |= ((offset - 1) & 0x1f) << 11; /* bit field size */
    }
    if (type & FMSB) {
        if (getlex (&cval) != ',')
            uerror ("comma expected");
        offset += getexpr (&segment);
        if (segment != SABS)
            uerror ("absolute value required");
        if (offset > 32)
            offset = 32;
        opcode |= ((offset - 1) & 0x1f) << 11; /* msb */
    }
done:

    /* Output resulting values. */
    if (emitfunc) {
        emitfunc (opcode, &relinfo);
    } else if (mode_reorder && (type & FDSLOT) && segm == STEXT) {
        /* Need a delay slot. */
        if (reorder_full && reorder_clobber != 0) {
            /* Analyse register dependency.
             * Flush the instruction if needed. */
            int rt = (opcode >> 16) & 31;
            int rs = (opcode >> 21) & 31;
            if (((type & (FRS1 | FRS2)) && rs == reorder_clobber) ||
                ((type & FRT2) && rt == reorder_clobber))
                reorder_flush();
        }
        if (reorder_full && (type & (FOFF18 | FAOFF18)) &&
            relinfo.flags == RABS && (opcode & 0x8000)) {
            /* Branch instruction with negative offset is being displaced
             * by one word.  Need to update the offset field. */
            offset = opcode + 1;
            opcode &= ~0xffff;
            opcode |= (offset & 0xffff);
        }
        fputword (opcode, sfile[segm]);
        fputrel (&relinfo, rfile[segm]);
        if (reorder_full) {
            /* Delay slot: insert a previous instruction. */
            reorder_flush();
        } else {
            /* Insert NOP in delay slot. */
            fputword (0, sfile[segm]);
            fputrel (&relabs, rfile[segm]);
            count[segm] += WORDSZ;
        }
        count[segm] += WORDSZ;
    } else {
        emitword (opcode, &relinfo, clobber_reg);
    }
}

/*
 * Increment the current segment by nbytes.
 * Emit a needed amount of zeroes to sfile and rfile.
 * Part of data have already been sent to rfile;
 * length specified by 'done' argument.
 */
void add_space (nbytes, fill_data)
    unsigned nbytes, fill_data;
{
    unsigned c;

    if (segm < SBSS) {
        /* Emit data and relocation. */
        for (c=0; c<nbytes; c++) {
            count[segm]++;
            if (fill_data)
                fputc (0, sfile[segm]);
            if (! (count[segm] & 3))
                fputrel (&relabs, rfile[segm]);
        }
    } else
        count[segm] += nbytes;
}

void makeascii ()
{
    register int c, nbytes;
    int cval;

    c = getlex (&cval);
    if (c != '"')
        uerror ("no .ascii parameter");
    nbytes = 0;
    for (;;) {
        c = getchar ();
        switch (c) {
        case EOF:
            uerror ("EOF in text string");
        case '"':
            break;
        case '\\':
            c = getchar ();
            switch (c) {
            case EOF:
                uerror ("EOF in text string");
            case '\n':
                continue;
            case '0': case '1': case '2': case '3':
            case '4': case '5': case '6': case '7':
                cval = c & 07;
                c = getchar ();
                if (c>='0' && c<='7') {
                    cval = (cval << 3) | (c & 7);
                    c = getchar ();
                    if (c>='0' && c<='7') {
                        cval = (cval << 3) | (c & 7);
                    } else
                        ungetc (c, stdin);
                } else
                    ungetc (c, stdin);
                c = cval;
                break;
            case 't':
                c = '\t';
                break;
            case 'b':
                c = '\b';
                break;
            case 'r':
                c = '\r';
                break;
            case 'n':
                c = '\n';
                break;
            case 'f':
                c = '\f';
                break;
            }
        default:
            fputc (c, sfile[segm]);
            nbytes++;
            continue;
        }
        break;
    }
    add_space (nbytes, 0);
}

/*
 * Skip a string from the input file.
 */
void skipstring ()
{
    int c, cval;

    c = getlex (&cval);
    if (c != '"')
        uerror ("no string parameter");
    for (;;) {
        c = getchar ();
        switch (c) {
        case EOF:
            uerror ("EOF in text string");
        case '"':
            break;
        case '\\':
            c = getchar ();
            switch (c) {
            case EOF:
                uerror ("EOF in text string");
            case '\n':
                continue;
            case '0': case '1': case '2': case '3':
            case '4': case '5': case '6': case '7':
                c = getchar ();
                if (c>='0' && c<='7') {
                    c = getchar ();
                    if (c>='0' && c<='7') {
                    } else
                        ungetc (c, stdin);
                } else
                    ungetc (c, stdin);
                break;
            }
        default:
            continue;
        }
        break;
    }
}

/*
 * Set assembler option.
 */
void setoption ()
{
    const char *option = name;
    int enable = 1;

    if (option[0] == 'n' && option[1] == 'o') {
        enable = 0;
        option += 2;
    }
    if (! strcmp ("reorder", option)) {
        /* reorder mode */
        mode_reorder = enable;
        if (! mode_reorder)
            reorder_flush();
        return;
    }
    if (! strcmp ("macro", option)) {
        /* macro mode */
        mode_macro = enable;
        return;
    }
    if (! strcmp ("mips16", option)) {
        /* mips16 mode */
        mode_mips16 = enable;
        return;
    }
    if (! strcmp ("micromips", option)) {
        /* micromips mode */
        mode_micromips = enable;
        return;
    }
    if (! strcmp ("at", option)) {
        /* at mode */
        mode_at = enable;
        return;
    }
    uerror ("unknown option %s", option);
}

/*
 * Align the current segment.
 */
void align (align_bits)
{
    unsigned nbytes, align_mask, c;

    align_mask = (1 << align_bits) - 1;
    nbytes = count[segm] & align_mask;
    if (nbytes == 0)
        return;
    nbytes = align_mask + 1 - nbytes;
    if (segm < SBSS) {
        /* Emit data and relocation. */
        for (c=0; c<nbytes; c++) {
            count[segm]++;
            fputc (0, sfile[segm]);
            if (! (count[segm] & 3))
                fputrel (&relabs, rfile[segm]);
        }
    } else
        count[segm] += nbytes;
}

void pass1 ()
{
    register int clex;
    int cval, tval, csegm, nbytes;
    register unsigned addr;

    segm = STEXT;
    for (;;) {
        clex = getlex (&cval);
        switch (clex) {
        case LEOF:
done:       reorder_flush();
            segm = STEXT;
            align (2);
            segm = SDATA;
            align (2);
            segm = SSTRNG;
            align (2);
            segm = SBSS;
            align (2);
            return;
        case LEOL:
            continue;
        case ':':
            continue;
        case '.':
            if (getlex (&cval) != '=')
                uerror ("bad instruction");
            addr = getexpr (&csegm);
            if (csegm != segm)
                uerror ("bad count assignment");
            if (addr < count[segm])
                uerror ("negative count increment");
            reorder_flush();
            if (segm == SBSS)
                count [segm] = addr;
            else {
                while (count[segm] < addr) {
                    emitword (0, &relabs, 0);
                }
            }
            break;
        case LNAME:
            cval = lookcmd();
            clex = getlex (&tval);
            if (clex == ':') {
                /* Label. */
                reorder_flush();
                cval = lookname();
                stab[cval].n_value = count[segm];
                stab[cval].n_type &= ~N_TYPE;
                stab[cval].n_type |= segmtype [segm];
                continue;
            } else if (clex=='=') {
                /* Symbol definition. */
                cval = lookname();
                stab[cval].n_value = getexpr (&csegm);
                if (csegm == SEXT)
                    uerror ("indirect equivalence");
                stab[cval].n_type &= N_EXT;
                stab[cval].n_type |= segmtype [csegm];
                break;
            }
            /* Machine instruction. */
            if (cval < 0)
                uerror ("bad instruction");
            ungetlex (clex, tval);
            align (2);
            makecmd (optable[cval].opcode, optable[cval].type,
                optable[cval].func);
            break;
        case LNUM:
            /* Local label. */
            if (nlabels >= MAXRLAB)
                uerror ("too many digital labels");
            reorder_flush();
            labeltab[nlabels].num = intval;
            labeltab[nlabels].value = count[segm];
            ++nlabels;
            clex = getlex (&tval);
            if (clex != ':')
                uerror ("bad digital label");
            continue;
        case LTEXT:
            segm = STEXT;
            reorder_flush();
            break;
        case LDATA:
            segm = SDATA;
            break;
        case LSTRNG:
        case LRDATA:
            segm = SSTRNG;
            break;
        case LBSS:
            segm = SBSS;
            break;
        case LWORD:
            reorder_flush();
            align (2);
            for (;;) {
                struct reloc relinfo;
                expr_flags = 0;
                getexpr (&cval);
                relinfo.flags = RBYTE32 | segmrel [cval];
                if (cval == SEXT)
                    relinfo.index = extref;
                if (expr_flags & EXPR_GPREL)
                    relinfo.flags |= RGPREL;
                emitword (intval, &relinfo, 0);
                clex = getlex (&cval);
                if (clex != ',') {
                    ungetlex (clex, cval);
                    break;
                }
            }
            break;
        case LBYTE:
            reorder_flush();
	    nbytes = 0;
            for (;;) {
                getexpr (&cval);
		fputc (intval, sfile[segm]);
		nbytes++;
                clex = getlex (&cval);
                if (clex != ',') {
                    ungetlex (clex, cval);
                    break;
                }
            }
            add_space (nbytes, 0);
            break;
        case LHALF:
            reorder_flush();
            align (1);
	    nbytes = 0;
            for (;;) {
                getexpr (&cval);
		fputc (intval, sfile[segm]);
		fputc (intval >> 8, sfile[segm]);
		nbytes += 2;
                clex = getlex (&cval);
                if (clex != ',') {
                    ungetlex (clex, cval);
                    break;
                }
            }
            add_space (nbytes, 0);
            break;
        case LSPACE:
            /* .space num */
            getexpr (&cval);
            reorder_flush();
            add_space (intval, 1);
            break;
        case LALIGN:
            /* .align num */
            if (getlex (&cval) != LNUM)
                uerror ("bad parameter of .align");
            reorder_flush();
            align (intval);
            break;
        case LASCII:
            reorder_flush();
            makeascii ();
            break;
        case LGLOBL:
            /* .globl name, ... */
            for (;;) {
                clex = getlex (&cval);
                if (clex != LNAME)
                    uerror ("bad parameter of .globl");
                cval = lookname();
                if (stab[cval].n_type & N_LOC)
                    uerror ("local name redefined as global");
                stab[cval].n_type |= N_EXT;
                clex = getlex (&cval);
                if (clex != ',') {
                    ungetlex (clex, cval);
                    break;
                }
            }
            break;
        case LLOCAL:
            /* .local name, ... */
            for (;;) {
                clex = getlex (&cval);
                if (clex != LNAME)
                    uerror ("bad parameter of .local");
                cval = lookname();
                if (stab[cval].n_type & N_EXT)
                    uerror ("global name redefined as local");
                stab[cval].n_type |= N_LOC;
                clex = getlex (&cval);
                if (clex != ',') {
                    ungetlex (clex, cval);
                    break;
                }
            }
            break;
        case LWEAK:
            /* .weak name */
            for (;;) {
                clex = getlex (&cval);
                if (clex != LNAME)
                    uerror ("bad parameter of .weak");
                cval = lookname();
                stab[cval].n_type |= N_WEAK;
                clex = getlex (&cval);
                if (clex != ',') {
                    ungetlex (clex, cval);
                    break;
                }
            }
            break;
        case LEQU:
            /* .equ name,value */
            if (getlex (&cval) != LNAME)
                uerror ("bad parameter of .equ");
            cval = lookname();
            if (stab[cval].n_type != N_UNDF &&
                stab[cval].n_type != N_LOC &&
                (stab[cval].n_type & N_TYPE) != N_COMM)
                uerror ("name already defined");
            clex = getlex (&tval);
            if (clex != ',')
                uerror ("bad value of .equ");
            stab[cval].n_value = getexpr (&csegm);
            if (csegm == SEXT)
                uerror ("indirect equivalence");
            stab[cval].n_type &= N_EXT;
            stab[cval].n_type |= segmtype [csegm];
            break;
        case LCOMM:
            /* .comm name,len[,alignment] */
            if (getlex (&cval) != LNAME)
                uerror ("bad parameter of .comm");
            cval = lookname();
            if (stab[cval].n_type != N_UNDF &&
                stab[cval].n_type != N_LOC &&
                (stab[cval].n_type & N_TYPE) != N_COMM)
                uerror ("name already defined");
            if (stab[cval].n_type & N_LOC)
                stab[cval].n_type = N_COMM;
            else
                stab[cval].n_type = N_EXT | N_COMM;
            clex = getlex (&tval);
            if (clex == ',') {
                getexpr (&tval);
                if (tval != SABS)
                    uerror ("bad length of .comm");
            } else {
                ungetlex (clex, cval);
                intval = 1;
            }
            stab[cval].n_value = intval;
            clex = getlex (&cval);
            if (clex != ',') {
                ungetlex (clex, cval);
                break;
            }
            getexpr (&tval);
            if (tval != SABS)
                uerror ("bad .comm alignment");
            break;
        case LFILE:
            /* .file line filename */
            if (getlex (&cval) != LNUM)
                uerror ("bad parameter of .file");
            skipstring();
            break;
        case LIDENT:
            /* .ident string */
            skipstring();
            break;
        case LSECTION:
            /* .section name[,"flags"[,type[,entsize]]] */
            clex = getlex (&cval);
            if (clex != LNAME && clex != LBSS && clex != LTEXT && clex != LDATA)
                uerror ("bad name of .section");
            setsection();
            clex = getlex (&cval);
            if (clex != ',') {
                ungetlex (clex, cval);
                break;
            }
            clex = getlex (&cval);
            if (clex == '"') {
                ungetlex (clex, cval);
                skipstring();
            } else if (clex != LNAME)
                uerror ("bad type of .section");
            clex = getlex (&cval);
            if (clex != ',') {
                ungetlex (clex, cval);
                break;
            }
            if (getlex (&cval) != LSECTYPE)
                uerror ("bad type of .section");
            clex = getlex (&cval);
            if (clex != ',') {
                ungetlex (clex, cval);
                break;
            }
            if (getlex (&cval) != LNUM)
                uerror ("bad entry size of .section");
            break;
        case LPREVIOUS:
            /* .previous - ignore */
            break;
        case LGNUATTR:
            /* .gnu_attribute num[,num] */
            if (getlex (&cval) != LNUM)
                uerror ("bad parameter of .gnu_attribute");
            clex = getlex (&cval);
            if (clex != ',' || getlex (&cval) != LNUM)
                uerror ("bad parameter of .gnu_attribute");
            break;
        case LSET:
            /* .set option */
            if (getlex (&cval) != LNAME)
                uerror ("bad parameter of .set");
            setoption();
            break;
        case LENT:
            /* .ent name */
            clex = getlex (&cval);
            if (clex != LNAME)
                uerror ("bad parameter of .ent");
            cval = lookname();
            break;
        case LEND:
            /* .end name */
            clex = getlex (&cval);
            if (clex != LNAME)
                uerror ("bad parameter of .end");
            cval = lookname();
            break;
        case LNAN:
            /* .nan name */
            clex = getlex (&cval);
            if (clex != LNAME)
                uerror ("bad parameter of .nan");
            break;
        case LTYPE:
            /* .type name,type */
            if (getlex (&cval) != LNAME)
                uerror ("bad name of .type");
            clex = getlex (&cval);
            if (clex != ',') {
                ungetlex (clex, cval);
                break;
            }
            if (getlex (&cval) != LSYMTYPE)
                uerror ("bad type of .type");
            break;
        case LFRAME:
            /* .frame reg,num,reg */
            if (getlex (&cval) != LREG)
                uerror ("bad register of .frame");
            clex = getlex (&cval);
            if (clex != ',' || getlex (&cval) != LNUM)
                uerror ("bad parameter of .frame");
            clex = getlex (&cval);
            if (clex != ',' || getlex (&cval) != LREG)
                uerror ("bad register of .frame");
            break;
        case LMASK:
            /* .mask mask,expr */
            if (getlex (&cval) != LNUM)
                uerror ("bad mask of .mask");
            clex = getlex (&cval);
            if (clex != ',')
                uerror ("bad parameter of .mask");
            getexpr (&cval);
            if (cval != SABS)
                uerror ("bad expression of .mask");
            break;
        case LFMASK:
            /* .fmask mask,expr */
            if (getlex (&cval) != LNUM)
                uerror ("bad mask of .fmask");
            clex = getlex (&cval);
            if (clex != ',')
                uerror ("bad parameter of .fmask");
            getexpr (&cval);
            if (cval != SABS)
                uerror ("bad expression of .fmask");
            break;
        case LSIZE:
            /* .size name,expr */
            if (getlex (&cval) != LNAME)
                uerror ("bad name of .size");
            clex = getlex (&cval);
            if (clex != ',') {
                ungetlex (clex, cval);
                break;
            }
            nbytes = getexpr (&csegm);
            if (csegm != SABS)
                uerror ("bad value of .size");
            break;
        default:
            uerror ("bad syntax");
        }
        clex = getlex (&cval);
        if (clex == LEOF)
            goto done;
        if (clex != LEOL)
            uerror ("bad instruction arguments");
    }
}

/*
 * Find the relative label address,
 * by the reference address and the label number.
 * Backward references have negative label numbers.
 */
int findlabel (int addr, int sym)
{
    struct labeltab *p;

    if (sym < 0) {
        /* Backward reference. */
        for (p=labeltab+nlabels-1; p>=labeltab; --p) {
            if (p->value <= addr && p->num == -sym) {
                return p->value;
            }
        }
        uerror ("undefined label %db at address %d", -sym, addr);
    } else {
        /* Forward reference. */
        for (p=labeltab; p<labeltab+nlabels; ++p) {
            if (p->value > addr && p->num == sym) {
                return p->value;
            }
        }
        uerror ("undefined label %df at address %d", sym, addr);
    }
    return 0;
}

void middle ()
{
    register int i, snum, nbytes;

    stlength = 0;
    for (snum=0, i=0; i<stabfree; i++) {
        switch (stab[i].n_type) {
        case N_UNDF:
            /* Without -u option, undefined symbol is considered external */
            if (uflag)
                uerror ("name undefined", stab[i].n_name);
            stab[i].n_type |= N_EXT;
            break;
        case N_COMM:
            /* Allocate a local common block */
            /* Align BSS count. */
            count[SBSS] = (count[SBSS] + WORDSZ-1) & ~(WORDSZ-1);
            nbytes = stab[i].n_value;
            stab[i].n_value = count[SBSS];
            stab[i].n_type = N_BSS;
            count[SBSS] += nbytes;
            break;
        }
        if (xflags)
            newindex[i] = snum;

        if (! xflags || (stab[i].n_type & N_EXT) ||
            (Xflag && ! IS_LOCAL(&stab[i])))
        {
            stlength += 2 + WORDSZ + stab[i].n_len;
            snum++;
        }
    }
    stalign = WORDSZ - stlength % WORDSZ;
    stlength += stalign;
    line = 0;
}

void makeheader (rtsize, rdsize)
{
    struct exec hdr;

    /* Align BSS size. */
    count[SBSS] = (count[SBSS] + WORDSZ-1) & ~(WORDSZ-1);

    hdr.a_midmag = RMAGIC;
    hdr.a_text = count [STEXT];
    hdr.a_data = count [SDATA] + count [SSTRNG];
    hdr.a_bss = count [SBSS];
    hdr.a_reltext = rtsize;
    hdr.a_reldata = rdsize;
    hdr.a_syms = stlength;
    hdr.a_entry = 0;
    fseek (stdout, 0, 0);
    fputhdr (&hdr, stdout);
}

unsigned relocate (opcode, offset, relinfo)
    register unsigned opcode, offset;
    register struct reloc *relinfo;
{
    switch (relinfo->flags & RFMASK) {
    case RBYTE32:                       /* 32 bits of byte address */
        opcode += offset;
        break;
    case RBYTE16:                       /* low 16 bits of byte address */
        offset += opcode & 0xffff;
        opcode &= ~0xffff;
        opcode |= offset & 0xffff;
        break;
    case RHIGH16:                       /* high 16 bits of byte address */
        offset += (opcode & 0xffff) << 16;
        offset += relinfo->offset;
        opcode &= ~0xffff;
        opcode |= (offset >> 16) & 0xffff;
        relinfo->offset = offset & 0xffff;
        break;
    case RHIGH16S:                      /* high 16 bits of byte address */
        offset += (opcode & 0xffff) << 16;
        offset += (signed short) relinfo->offset;
        opcode &= ~0xffff;
        opcode |= ((offset + 0x8000) >> 16) & 0xffff;
        relinfo->offset = offset & 0xffff;
        break;
    case RWORD16:                       /* 16 bits of relative word address */
        uerror ("bad relative relocation: opcode %08x, relinfo %02x", opcode, relinfo->flags);
        break;
    case RWORD26:                       /* 26 bits of word address */
        offset += (opcode & 0x3ffffff) << 2;
        opcode &= ~0x3ffffff;
        opcode |= (offset >> 2) & 0x3ffffff;
        break;
    }
    return (opcode);
}

unsigned makeword (opcode, relinfo, offset)
    register unsigned opcode, offset;
    register struct reloc *relinfo;
{
    struct nlist *sym;
    unsigned value;

    switch (relinfo->flags & RSMASK) {
    case RABS:
        break;
    case RTEXT:
        opcode = relocate (opcode, tbase, relinfo);
        break;
    case RDATA:
        opcode = relocate (opcode, dbase, relinfo);
        break;
    case RSTRNG:
        opcode = relocate (opcode, adbase, relinfo);
        break;
    case RBSS:
        opcode = relocate (opcode, bbase, relinfo);
        break;
    case REXT:
        if (relinfo->index >= RLAB_OFFSET - RLAB_MAXVAL) {
            /* Relative label.
             * Change relocation to segment type. */
            sym = 0;
            value = findlabel (offset, relinfo->index - RLAB_OFFSET);
            relinfo->flags &= RGPREL | RFMASK;
            relinfo->flags |= segmrel[segm];
        } else {
            /* Symbol name. */
            sym = &stab[relinfo->index];
            if (sym->n_type == N_EXT+N_UNDF || sym->n_type == N_EXT+N_COMM)
                return opcode;
            value = sym->n_value;
        }

        switch (relinfo->flags & RFMASK) {
        case RWORD16:
            /* Relative word address.
             * Change relocation to absolute. */
            if (sym && (sym->n_type & N_TYPE) != segmtype[segm])
                uerror ("%s: bad segment for relative relocation, offset %u",
                    sym->n_name, offset);
            offset = value - offset - 4;
            if (segm == SDATA)
                offset -= dbase;
            else if (segm == SSTRNG)
                offset -= adbase;
            offset += (opcode & 0xffff) << 2;
            opcode &= ~0xffff;
            opcode |= (offset >> 2) & 0xffff;
            relinfo->flags = RABS;
            return opcode;
        case RHIGH16:
            value += relinfo->offset;
            break;
        case RHIGH16S:
            value += (signed short) relinfo->offset;
            break;
        }
        opcode = relocate (opcode, value, relinfo);
        break;
    }
    return opcode;
}

void pass2 ()
{
    register int i;
    register unsigned h;

    tbase = 0;
    dbase = tbase + count[STEXT];
    adbase = dbase + count[SDATA];
    bbase = adbase + count[SSTRNG];

    /* Adjust indexes in symbol name */
    for (i=0; i<stabfree; i++) {
        switch (stab[i].n_type & N_TYPE) {
        case N_UNDF:
        case N_ABS:
            break;
        case N_TEXT:
            stab[i].n_value += tbase;
            break;
        case N_DATA:
            stab[i].n_value += dbase;
            break;
        case N_STRNG:
            stab[i].n_value += adbase;
            stab[i].n_type += N_DATA - N_STRNG;
            break;
        case N_BSS:
            stab[i].n_value += bbase;
            break;
        }
    }
    fseek (stdout, sizeof(struct exec), 0);
    for (segm=STEXT; segm<SBSS; segm++) {
        /* Need to rewrite a relocation file. */
        FILE *rfd = fopen (tfilename, "w+");
        if (! rfd)
            uerror ("cannot open %s", tfilename);
        unlink (tfilename);

        rewind (sfile [segm]);
        rewind (rfile [segm]);
        for (h=0; h<count[segm]; h+=WORDSZ) {
            struct reloc relinfo;
            unsigned word = fgetword (sfile[segm]);
            fgetrel (rfile[segm], &relinfo);
            word = makeword (word, &relinfo, h);
            fputword (word, stdout);
            fputrel (&relinfo, rfd);
        }
        fclose (rfile [segm]);
        rfile [segm] = rfd;
    }
}

/*
 * Convert symbol type to relocation type.
 */
int typerel (t)
{
    switch (t & N_TYPE) {
    case N_ABS:     return (RABS);
    case N_TEXT:    return (RTEXT);
    case N_DATA:    return (RDATA);
    case N_BSS:     return (RBSS);
    case N_STRNG:   return (RDATA);
    case N_UNDF:
    case N_COMM:
    case N_FN:
    default:        return (0);
    }
}

/*
 * Relocate a relocation info word.
 * Remap symbol indexes.
 * Put string pseudo-section to data section.
 */
void relrel (relinfo)
    register struct reloc *relinfo;
{
    register unsigned type;

    switch ((int) relinfo->flags & REXT) {
    case RSTRNG:
        relinfo->flags &= ~RSMASK;
        relinfo->flags |= RDATA;
        break;
    case REXT:
        type = stab[relinfo->index].n_type;
        if (type == N_EXT+N_UNDF || type == N_EXT+N_COMM)
        {
            /* Reindexing */
            if (xflags)
                relinfo->index = newindex [relinfo->index];
        } else {
            relinfo->flags &= ~RSMASK;
            relinfo->flags |= typerel (type);
        }
        break;
    }
}

/*
 * Emit a relocation info for a given segment.
 * Copy it from scratch file to output.
 * Return a size of relocation data in bytes.
 */
unsigned makereloc (s)
    register int s;
{
    register unsigned i, nbytes;
    struct reloc relinfo;

    if (count [s] <= 0)
        return 0;
    rewind (rfile [s]);
    nbytes = 0;
    for (i=0; i<count[s]; i+=WORDSZ) {
        fgetrel (rfile[s], &relinfo);
        relrel (&relinfo);
        nbytes += fputrel (&relinfo, stdout);
    }
    return nbytes;
}

/*
 * Align the relocation section to an integral number of words.
 */
unsigned alignreloc (nbytes)
    register unsigned nbytes;
{
    while (nbytes % WORDSZ) {
        putchar (0);
        nbytes++;
    }
    return nbytes;
}

void makesymtab ()
{
    register int i;

    for (i=0; i<stabfree; i++) {
        if (! xflags || (stab[i].n_type & N_EXT) ||
            (Xflag && stab[i].n_name[0] != 'L'))
        {
            fputsym (&stab[i], stdout);
        }
    }
    while (stalign--)
        putchar (0);
}

void usage ()
{
    fprintf (stderr, "Usage:\n");
    fprintf (stderr, "  as [-uxX] [-o outfile] [infile]\n");
    fprintf (stderr, "Options:\n");
    fprintf (stderr, "  -o filename     Set output file name, default stdout\n");
    fprintf (stderr, "  -u              Treat undefined names as error\n");
    fprintf (stderr, "  -x              Discard local symbols\n");
    fprintf (stderr, "  -X              Discard locals starting with 'L' or '.'\n");
    exit (1);
}

int main (argc, argv)
    register char *argv[];
{
    register int i;
    register char *cp;
    int ofile = 0;
    unsigned rtsize, rdsize;

    /*
     * Parse options.
     */
    for (i=1; i<argc; i++) {
        switch (argv[i][0]) {
        case '-':
            for (cp=argv[i]+1; *cp; cp++) {
                switch (*cp) {
                case 'X':       /* strip L* and .* locals */
                    Xflag++;
                case 'x':       /* strip local symbols */
                    xflags++;
                    break;
                case 'u':       /* treat undefines as error */
                    uflag++;
                    break;
                case 'o':       /* output file name */
                    if (ofile)
                        uerror ("too many -o flags");
                    ofile = 1;
                    if (cp [1]) {
                        /* -ofile */
                        outfile = cp+1;
                        while (*++cp);
                        --cp;
                    } else if (i+1 < argc)
                        /* -o file */
                        outfile = argv[++i];
                    break;
                case 'v':       /* verbose mode */
                    // TODO
                    break;
                case 'g':       /* debug mode */
                    // TODO
                    break;
                case 'I':       /* include dir */
                    // TODO
                    if (cp[1] == 0) {
                        i++;
                    } else {
                        while (*++cp);
                        --cp;
                    }
                    break;
                case 'O':       /* optimization level */
                    // TODO
                    while (*++cp);
                    --cp;
                    break;
                case '-':       /* long option - skip */
                    while (*++cp);
                    --cp;
                    break;
                case 'n':       /* -no-xyz option - skip */
                    while (*++cp);
                    --cp;
                    break;
                case 'm':       /* -mips32r2, -mabi=32 - ignore */
                    while (*++cp);
                    --cp;
                    break;
                case 'E':       /* -EL, -EB - endianness */
                    if (cp[1] != 'L')
                        uerror ("only little endian is supported");
                    while (*++cp);
                    --cp;
                    break;
                default:
                    fprintf (stderr, "Unknown option: %s\n", cp);
                    usage();
                }
            }
            break;
        default:
            if (infile)
                uerror ("too many input files");
            infile = argv[i];
            break;
        }
    }
    if (! infile && isatty(0))
        usage();

    /*
     * Setup input-output.
     */
    if (infile && ! freopen (infile, "r", stdin))
        uerror ("cannot open %s", infile);
    if (! freopen (outfile, "w", stdout))
        uerror ("cannot open %s", outfile);

    startup ();                         /* Open temporary files */
    hashinit ();                        /* Initialize hash tables */
    pass1 ();                           /* First pass */
    middle ();                          /* Prepare symbol table */
    pass2 ();                           /* Second pass */
    rtsize = makereloc (STEXT);         /* Emit relocation info: text */
    rtsize = alignreloc (rtsize);
    rdsize = makereloc (SDATA);         /* data */
    rdsize += makereloc (SSTRNG);       /* rodata */
    rdsize = alignreloc (rdsize);
    makesymtab ();                      /* Emit symbol table */
    makeheader (rtsize, rdsize);        /* Write a.out header */
    return 0;
}
