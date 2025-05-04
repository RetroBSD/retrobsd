/*
 * Format of object files and executables for BESM-6.
 */
#ifndef _SYS_EXEC_AOUT_H_
#define _SYS_EXEC_AOUT_H_

/*
 * Header prepended to each a.out file.
 */
struct  exec {
    unsigned a_magic;       /* magic number */
    unsigned a_const;       /* size of const segment */
    unsigned a_text;        /* size of text segment */
    unsigned a_data;        /* size of initialized data */
    unsigned a_bss;         /* size of uninitialized data */
    unsigned a_syms;        /* size of symbol table */
    unsigned a_entry;       /* entry point */
    unsigned a_unused;      /* unused for now */
};

/* a_magic */
#define RMAGIC      0406    /* relocatable object file */
#define OMAGIC      0407    /* old impure format */
#define NMAGIC      0410    /* read-only text */

/* Valid magic number check. */
#define N_BADMAG(x)     (N_GETMAGIC(x) != RMAGIC && \
                         N_GETMAGIC(x) != OMAGIC && \
                         N_GETMAGIC(x) != NMAGIC)

/* Text segment offset. */
#define N_TXTOFF(x)     48

/* Data segment offset. */
#define N_DATOFF(x)     (N_TXTOFF(x) + (x).a_text)

/* Text relocation table offset. */
#define N_TRELOFF(x)    (N_DATOFF(x) + (x).a_data)

/* Data relocation table offset. */
#define N_DRELOFF(x)    (N_TRELOFF(x) + (x).a_text)

/* Symbol table offset. */
#define N_SYMOFF(x)     (N_GETMAGIC(x) == RMAGIC ? \
                         N_DRELOFF(x) + (x).a_data : \
                         N_DATOFF(x) + (x).a_data)
#endif
