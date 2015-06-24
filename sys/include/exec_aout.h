#ifndef _SYS_EXEC_AOUT_H_
#define _SYS_EXEC_AOUT_H_

/*
 * Header prepended to each a.out file.
 */
struct  exec {
    unsigned a_midmag;      /* magic number */
    unsigned a_text;        /* size of text segment */
    unsigned a_data;        /* size of initialized data */
    unsigned a_bss;         /* size of uninitialized data */
    unsigned a_reltext;     /* size of text relocation info */
    unsigned a_reldata;     /* size of data relocation info */
    unsigned a_syms;        /* size of symbol table */
    unsigned a_entry;       /* entry point */
};

#define a_magic a_midmag & 0xffff

/* a_magic (a_midmag & 0x0000ffff) */
#define RMAGIC      0406    /* relocatable object file */
#define OMAGIC      0407    /* old impure format */
#define NMAGIC      0410    /* read-only text */

/*
 * a_mid ((a_midmag & 0x03ff0000) >> 16)
 */
#define MID_ZERO    0       /* unknown - implementation dependent */
#define MID_SUN010  1       /* sun 68010/68020 binary */
#define MID_SUN020  2       /* sun 68020-only binary */
#define MID_PC386   100     /* 386 PC binary. (so quoth BFD) */
#define MID_HP200   200     /* hp200 (68010) BSD binary */
#define MID_I386    134     /* i386 BSD binary */
#define MID_M68K    135     /* m68k BSD binary with 8K page sizes */
#define MID_M68K4K  136     /* m68k BSD binary with 4K page sizes */
#define MID_NS32532 137     /* ns32532 */
#define MID_SPARC   138     /* sparc */
#define MID_PMAX    139     /* pmax */
#define MID_VAX1K   140     /* vax 1K page size binaries */
#define MID_ALPHA   141     /* Alpha BSD binary */
#define MID_MIPS    142     /* big-endian MIPS */
#define MID_ARM6    143     /* ARM6 */
#define MID_SH3     145     /* SH3 */
#define MID_POWERPC 149     /* big-endian PowerPC */
#define MID_VAX     150     /* vax */
#define MID_SPARC64 151     /* LP64 sparc */
#define MID_HP300   300     /* hp300 (68020+68881) BSD binary */
#define MID_HPUX    0x20C   /* hp200/300 HP-UX binary */
#define MID_HPUX800 0x20B   /* hp800 HP-UX binary */

/*
 * a_flags ((a_midmag & 0xfc000000 ) << 26)
 */
#define EX_PIC      0x10
#define EX_DYNAMIC  0x20
#define EX_DPMASK   0x30
/*
 * Interpretation of the (a_flags & EX_DPMASK) bits:
 *
 *  00      traditional executable or object file
 *  01      object file contains PIC code (set by `as -k')
 *  10      dynamic executable
 *  11      position independent executable image
 *          (eg. a shared library)
 */

/*
 * The a.out structure's a_midmag field is a network-byteorder encoding
 * of this int
 *  FFFFFFmmmmmmmmmmMMMMMMMMMMMMMMMM
 * Where `F' is 6 bits of flag like EX_DYNAMIC,
 *       `m' is 10 bits of machine-id like MID_I386, and
 *       `M' is 16 bits worth of magic number, ie. ZMAGIC.
 * The macros below will set/get the needed fields.
 */
#define N_GETMAGIC(ex)  (((ex).a_midmag)&0x0000ffff)
#define N_GETMID(ex)    ((((ex).a_midmag)&0x03ff0000) >> 16)
#define N_GETFLAG(ex)   ((((ex).a_midmag)&0xfc000000 ) << 26)

/* Valid magic number check. */
#define N_BADMAG(x)     (N_GETMAGIC(x) != RMAGIC && \
                         N_GETMAGIC(x) != OMAGIC && \
                         N_GETMAGIC(x) != NMAGIC)

/* Text segment offset. */
#define N_TXTOFF(x)     sizeof(struct exec)

/* Data segment offset. */
#define N_DATOFF(x)     (N_TXTOFF(x) + (x).a_text)

/* Text relocation table offset. */
#define N_TRELOFF(x)    (N_DATOFF(x) + (x).a_data)

/* Data relocation table offset. */
#define N_DRELOFF(x)    (N_TRELOFF(x) + (x).a_reltext)

/* Symbol table offset. */
#define N_SYMOFF(x)     (N_GETMAGIC(x) == RMAGIC ? \
                         N_DRELOFF(x) + (x).a_reldata : \
                         N_DATOFF(x) + (x).a_data)
#endif
