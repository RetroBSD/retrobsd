/*  $NetBSD: exec_elf.h,v 1.37.4.1 2000/07/26 23:57:06 mycroft Exp $    */

/*-
 * Copyright (c) 1994 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by the NetBSD
 *  Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _SYS_EXEC_ELF_H_
#define _SYS_EXEC_ELF_H_

#ifndef _SYS_TYPES_H_
#include <machine/types.h>
#endif

/*
 * ELF Header
 */
#define ELF_NIDENT  16

struct elf_ehdr {
    unsigned char   e_ident[ELF_NIDENT];    /* Id bytes */
    unsigned short  e_type;         /* file type */
    unsigned short  e_machine;      /* machine type */
    unsigned int    e_version;      /* version number */
    unsigned int    e_entry;        /* entry point */
    unsigned int    e_phoff;        /* Program header table offset */
    unsigned int    e_shoff;        /* Section header table offset */
    unsigned int    e_flags;        /* Processor flags (currently unused, should be 0) */
    unsigned short  e_ehsize;       /* sizeof elf_ehdr */
    unsigned short  e_phentsize;    /* Program header entry size */
    unsigned short  e_phnum;        /* Number of program headers */
    unsigned short  e_shentsize;    /* Section header entry size */
    unsigned short  e_shnum;        /* Number of section headers */
    unsigned short  e_shstrndx;     /* String table index */
};

/* e_ident offsets */
#define EI_MAG0         0       /* first byte of magic number */
#define ELFMAG0         0x7f
#define EI_MAG1         1       /* second byte of magic number */
#define ELFMAG1         'E'
#define EI_MAG2         2       /* third byte of magic number */
#define ELFMAG2         'L'
#define EI_MAG3         3       /* fourth byte of magic number */
#define ELFMAG3         'F'

#define EI_CLASS        4       /* 5:th byte: File class */
#define     ELFCLASSNONE 0      /* Invalid class */
#define     ELFCLASS32  1       /* 32-bit objects */
#define     ELFCLASS64  2       /* 64-bit objects */
#define     ELFCLASSNUM 3

#define EI_DATA         5       /* 6:th byte: Data encoding */
#define     ELFDATANONE 0       /* Unknown data format */
#define     ELFDATA2LSB 1       /* two's complement, little-endian */
#define     ELFDATA2MSB 2       /* two's complement, big-endian */

#define EI_VERSION      6       /* Version number of the ELF specification */
#define     EV_NONE     0       /* Invalid version */
#define     EV_CURRENT  1       /* Current version */
#define     EV_NUM      2

#define EI_OSABI        7       /* Operating system/ABI identification */
#define     ELFOSABI_SYSV   0   /* UNIX System V ABI */
#define     ELFOSABI_HPUX   1   /* HP-UX operating system */
#define     ELFOSABI_NETBSD     /* NetBSD ABI */
#define     ELFOSABI_LINUX      /* Linux ABI */
#define     ELFOSABI_SOLARIS    /* Solaris ABI */
#define     ELFOSABI_FREEBSD    /* FreeBSD ABI */
#define     ELFOSABI_ARM        /* ARM architecture ABI */
#define     ELFOSABI_STANDALONE 255 /* Stand-alone (embedded) application */

#define EI_ABIVERSION   8       /* ABI version */

#define EI_PAD          9       /* Start of padding bytes up to EI_NIDENT*/

#define ELFMAG          "\177ELF"
#define SELFMAG         4

/* e_type */
#define ET_NONE         0       /* Unknown file type */
#define ET_REL          1       /* A Relocatable file */
#define ET_EXEC         2       /* An Executable file */
#define ET_DYN          3       /* A Shared object file */
#define ET_CORE         4       /* A Core file */
#define ET_NUM          5

#define ET_LOOS         0xfe00  /* Operating system specific range */
#define ET_HIOS         0xfeff
#define ET_LOPROC       0xff00  /* Processor-specific range */
#define ET_HIPROC       0xffff

/* e_machine */
#define EM_NONE         0       /* No machine */
#define EM_M32          1       /* AT&T WE 32100 */
#define EM_SPARC        2       /* SPARC */
#define EM_386          3       /* Intel 80386 */
#define EM_68K          4       /* Motorola 68000 */
#define EM_88K          5       /* Motorola 88000 */
#define EM_486          6       /* Intel 80486 */
#define EM_860          7       /* Intel 80860 */
#define EM_MIPS         8       /* MIPS I Architecture */
#define EM_S370         9       /* Amdahl UTS on System/370 */
#define EM_MIPS_RS3_LE  10      /* MIPS RS3000 Little-endian */
#define EM_RS6000       11      /* IBM RS/6000 XXX reserved */
#define EM_PARISC       15      /* Hewlett-Packard PA-RISC */
#define EM_NCUBE        16      /* NCube XXX reserved */
#define EM_VPP500       17      /* Fujitsu VPP500 */
#define EM_SPARC32PLUS  18      /* Enhanced instruction set SPARC */
#define EM_960          19      /* Intel 80960 */
#define EM_PPC          20      /* PowerPC */
#define EM_V800         36      /* NEC V800 */
#define EM_FR20         37      /* Fujitsu FR20 */
#define EM_RH32         38      /* TRW RH-32 */
#define EM_RCE          39      /* Motorola RCE */
#define EM_ARM          40      /* Advanced RISC Machines ARM */
#define EM_ALPHA        41      /* DIGITAL Alpha */
#define EM_SH           42      /* Hitachi Super-H */
#define EM_SPARCV9      43      /* SPARC Version 9 */
#define EM_TRICORE      44      /* Siemens Tricore */
#define EM_ARC          45      /* Argonaut RISC Core */
#define EM_H8_300       46      /* Hitachi H8/300 */
#define EM_H8_300H      47      /* Hitachi H8/300H */
#define EM_H8S          48      /* Hitachi H8S */
#define EM_H8_500       49      /* Hitachi H8/500 */
#define EM_IA_64        50      /* Intel Merced Processor */
#define EM_MIPS_X       51      /* Stanford MIPS-X */
#define EM_COLDFIRE     52      /* Motorola Coldfire */
#define EM_68HC12       53      /* Motorola MC68HC12 */
#define EM_VAX          75      /* DIGITAL VAX */
#define EM_ALPHA_EXP    36902   /* used by NetBSD/alpha; obsolete */
#define EM_NUM          36903

/*
 * ELF Program Header
 */
struct elf_phdr {
    unsigned int    p_type;     /* entry type */
    unsigned int    p_offset;   /* file offset */
    unsigned int    p_vaddr;    /* virtual address */
    unsigned int    p_paddr;    /* physical address (reserved, 0) */
    unsigned int    p_filesz;   /* file size of segment (may be 0) */
    unsigned int    p_memsz;    /* memory size of segment (may be 0) */
    unsigned int    p_flags;    /* flags */
    unsigned int    p_align;    /* memory & file alignment */
};

/* p_type */
#define PT_NULL         0       /* Program header table entry unused */
#define PT_LOAD         1       /* Loadable program segment */
#define PT_DYNAMIC      2       /* Dynamic linking information */
#define PT_INTERP       3       /* Program interpreter */
#define PT_NOTE         4       /* Auxiliary information */
#define PT_SHLIB        5       /* Reserved, unspecified semantics */
#define PT_PHDR         6       /* Entry for header table itself */
#define PT_NUM          7
#define PT_LOPROC       0x70000000  /* Start of processor-specific semantics */
#define PT_HIPROC       0x7fffffff  /* end of processor-specific semantics */
#define PT_GNU_STACK            /* GNU stack extension */

/* p_flags */
#define PF_R            0x4     /* Segment is readable */
#define PF_W            0x2     /* Segment is writable */
#define PF_X            0x1     /* Segment is executable */
/* A text segment commonly have PF_X|PF_R, a data segment PF_X|PF_W and PF_R */

#define PF_MASKOS       0x0ff00000  /* Opersting system specific values */
#define PF_MASKPROC     0xf0000000  /* Processor-specific values */


#define PT_MIPS_REGINFO 0x70000000

/*
 * Section Headers
 */
struct elf_shdr {
    unsigned int    sh_name;        /* section name (.shstrtab index) */
    unsigned int    sh_type;        /* section type */
    unsigned int    sh_flags;       /* section flags */
    unsigned int    sh_addr;        /* virtual address */
    unsigned int    sh_offset;      /* file offset */
    unsigned int    sh_size;        /* section size */
    unsigned int    sh_link;        /* link to another */
    unsigned int    sh_info;        /* misc info */
    unsigned int    sh_addralign;   /* memory alignment */
    unsigned int    sh_entsize;     /* table entry size */
};

/* sh_type */
#define SHT_NULL        0       /* inactive */
#define SHT_PROGBITS    1       /* program defined contents */
#define SHT_SYMTAB      2       /* holds symbol table */
#define SHT_STRTAB      3       /* holds string table */
#define SHT_RELA        4       /* holds relocation info with explicit addends */
#define SHT_HASH        5       /* holds symbol hash table */
#define SHT_DYNAMIC     6       /* holds dynamic linking information */
#define SHT_NOTE        7       /* holds information marking */
#define SHT_NOBITS      8       /* holds a section that does not occupy space */
#define SHT_REL         9       /* holds relocation info without explicit addends */
#define SHT_SHLIB       10      /* reserved with unspecified semantics */
#define SHT_DYNSYM      11      /* holds a minimal set of dynamic linking symbols */
#define SHT_NUM         12

#define SHT_LOOS        0x60000000  /* Operating system specific range */
#define SHT_HIOS        0x6fffffff
#define SHT_LOPROC      0x70000000  /* Processor-specific range */
#define SHT_HIPROC      0x7fffffff
#define SHT_LOUSER      0x80000000  /* Application-specific range */
#define SHT_HIUSER      0xffffffff

/* sh_flags */
#define SHF_WRITE       0x1         /* Section contains writable data */
#define SHF_ALLOC       0x2         /* Section occupies memory */
#define SHF_EXECINSTR   0x4         /* Section contains executable insns */

#define SHF_MASKOS      0x0f000000  /* Operating system specific values */
#define SHF_MASKPROC    0xf0000000  /* Processor-specific values */

/*
 * Symbol Table
 */
struct elf_sym {
    unsigned int    st_name;    /* Symbol name (.symtab index) */
    unsigned int    st_value;   /* value of symbol */
    unsigned int    st_size;    /* size of symbol */
    unsigned char   st_info;    /* type / binding attrs */
    unsigned char   st_other;   /* unused */
    unsigned short  st_shndx;   /* section index of symbol */
};

/* Symbol Table index of the undefined symbol */
#define ELF_SYM_UNDEFINED   0

/* st_info: Symbol Bindings */
#define STB_LOCAL       0   /* local symbol */
#define STB_GLOBAL      1   /* global symbol */
#define STB_WEAK        2   /* weakly defined global symbol */
#define STB_NUM         3

#define STB_LOOS        10  /* Operating system specific range */
#define STB_HIOS        12
#define STB_LOPROC      13  /* Processor-specific range */
#define STB_HIPROC      15

/* st_info: Symbol Types */
#define STT_NOTYPE      0   /* Type not specified */
#define STT_OBJECT      1   /* Associated with a data object */
#define STT_FUNC        2   /* Associated with a function */
#define STT_SECTION     3   /* Associated with a section */
#define STT_FILE        4   /* Associated with a file name */
#define STT_NUM         5

#define STT_LOOS        10  /* Operating system specific range */
#define STT_HIOS        12
#define STT_LOPROC      13  /* Processor-specific range */
#define STT_HIPROC      15

/* st_info utility macros */
#define ELF_ST_BIND(info)       ((unsigned int)(info) >> 4)
#define ELF_ST_TYPE(info)       ((unsigned int)(info) & 0xf)
#define ELF_ST_INFO(bind,type)  ((unsigned char)(((bind) << 4) | ((type) & 0xf)))

/*
 * Special section indexes
 */
#define SHN_UNDEF       0       /* Undefined section */

#define SHN_LORESERVE   0xff00  /* Start of Reserved range */
#define SHN_ABS         0xfff1  /*  Absolute symbols */
#define SHN_COMMON      0xfff2  /*  Common symbols */
#define SHN_HIRESERVE   0xffff

#define SHN_LOPROC      0xff00  /* Start of Processor-specific range */
#define SHN_HIPROC      0xff1f
#define SHN_LOOS        0xff20  /* Operating system specific range */
#define SHN_HIOS        0xff3f

#define SHN_MIPS_ACOMMON 0xff00
#define SHN_MIPS_TEXT   0xff01
#define SHN_MIPS_DATA   0xff02
#define SHN_MIPS_SCOMMON 0xff03

/*
 * Relocation Entries
 */
struct elf_rel {
    unsigned int    r_offset;   /* where to do it */
    unsigned int    r_info;     /* index & type of relocation */
};

struct elf_rela {
    unsigned int    r_offset;   /* where to do it */
    unsigned int    r_info;     /* index & type of relocation */
    int             r_addend;   /* adjustment value */
};

/* r_info utility macros */
#define ELF_R_SYM(info)         ((info) >> 8)
#define ELF_R_TYPE(info)        ((info) & 0xff)
#define ELF_R_INFO(sym, type)   (((sym) << 8) + (unsigned char)(type))

/*
 * Dynamic Section structure array
 */
struct elf_dyn {
    unsigned int        d_tag;  /* entry tag value */
    union {
        unsigned int    d_ptr;
        unsigned int    d_val;
    } d_un;
};

/* d_tag */
#define DT_NULL         0   /* Marks end of dynamic array */
#define DT_NEEDED       1   /* Name of needed library (DT_STRTAB offset) */
#define DT_PLTRELSZ     2   /* Size, in bytes, of relocations in PLT */
#define DT_PLTGOT       3   /* Address of PLT and/or GOT */
#define DT_HASH         4   /* Address of symbol hash table */
#define DT_STRTAB       5   /* Address of string table */
#define DT_SYMTAB       6   /* Address of symbol table */
#define DT_RELA         7   /* Address of Rela relocation table */
#define DT_RELASZ       8   /* Size, in bytes, of DT_RELA table */
#define DT_RELAENT      9   /* Size, in bytes, of one DT_RELA entry */
#define DT_STRSZ        10  /* Size, in bytes, of DT_STRTAB table */
#define DT_SYMENT       11  /* Size, in bytes, of one DT_SYMTAB entry */
#define DT_INIT         12  /* Address of initialization function */
#define DT_FINI         13  /* Address of termination function */
#define DT_SONAME       14  /* Shared object name (DT_STRTAB offset) */
#define DT_RPATH        15  /* Library search path (DT_STRTAB offset) */
#define DT_SYMBOLIC     16  /* Start symbol search within local object */
#define DT_REL          17  /* Address of Rel relocation table */
#define DT_RELSZ        18  /* Size, in bytes, of DT_REL table */
#define DT_RELENT       19  /* Size, in bytes, of one DT_REL entry */
#define DT_PLTREL       20  /* Type of PLT relocation entries */
#define DT_DEBUG        21  /* Used for debugging; unspecified */
#define DT_TEXTREL      22  /* Relocations might modify non-writable seg */
#define DT_JMPREL       23  /* Address of relocations associated with PLT */
#define DT_BIND_NOW     24  /* Process all relocations at load-time */
#define DT_INIT_ARRAY   25  /* Address of initialization function array */
#define DT_FINI_ARRAY   26  /* Size, in bytes, of DT_INIT_ARRAY array */
#define DT_INIT_ARRAYSZ 27  /* Address of termination function array */
#define DT_FINI_ARRAYSZ 28  /* Size, in bytes, of DT_FINI_ARRAY array*/
#define DT_NUM          29

#define DT_LOOS     0x60000000  /* Operating system specific range */
#define DT_HIOS     0x6fffffff
#define DT_LOPROC   0x70000000  /* Processor-specific range */
#define DT_HIPROC   0x7fffffff

/*
 * Auxiliary Vectors
 */
struct elf_auxinfo {
    unsigned int    a_type;     /* 32-bit id */
    unsigned int    a_v;        /* 32-bit id */
};

/* a_type */
#define AT_NULL         0       /* Marks end of array */
#define AT_IGNORE       1       /* No meaning, a_un is undefined */
#define AT_EXECFD       2       /* Open file descriptor of object file */
#define AT_PHDR         3       /* &phdr[0] */
#define AT_PHENT        4       /* sizeof(phdr[0]) */
#define AT_PHNUM        5       /* # phdr entries */
#define AT_PAGESZ       6       /* PAGESIZE */
#define AT_BASE         7       /* Interpreter base addr */
#define AT_FLAGS        8       /* Processor flags */
#define AT_ENTRY        9       /* Entry address of executable */
#define AT_DCACHEBSIZE  10      /* Data cache block size */
#define AT_ICACHEBSIZE  11      /* Instruction cache block size */
#define AT_UCACHEBSIZE  12      /* Unified cache block size */

    /* Vendor specific */
#define AT_MIPS_NOTELF  10      /* XXX a_val != 0 -> MIPS XCOFF executable */

#define AT_SUN_UID      2000    /* euid */
#define AT_SUN_RUID     2001    /* ruid */
#define AT_SUN_GID      2002    /* egid */
#define AT_SUN_RGID     2003    /* rgid */

    /* Solaris kernel specific */
#define AT_SUN_LDELF    2004    /* dynamic linker's ELF header */
#define AT_SUN_LDSHDR   2005    /* dynamic linker's section header */
#define AT_SUN_LDNAME   2006    /* dynamic linker's name */
#define AT_SUN_LPGSIZE  2007    /* large pagesize */

    /* Other information */
#define AT_SUN_PLATFORM 2008    /* sysinfo(SI_PLATFORM) */
#define AT_SUN_HWCAP    2009    /* process hardware capabilities */
#define AT_SUN_IFLUSH   2010    /* do we need to flush the instruction cache? */
#define AT_SUN_CPU      2011    /* cpu name */
    /* ibcs2 emulation band aid */
#define AT_SUN_EMUL_ENTRY 2012  /* coff entry point */
#define AT_SUN_EMUL_EXECFD 2013 /* coff file descriptor */
    /* Executable's fully resolved name */
#define AT_SUN_EXECNAME 2014

/*
 * Note Headers
 */
struct elf_nhdr {

    unsigned int    n_namesz;
    unsigned int    n_descsz;
    unsigned int    n_type;
};

#define ELF_NOTE_TYPE_OSVERSION     1

/* NetBSD-specific note type: Emulation name.  desc is emul name string. */
#define ELF_NOTE_NETBSD_TYPE_EMULNAME 2

/* NetBSD-specific note name and description sizes */
#define ELF_NOTE_NETBSD_NAMESZ      7
#define ELF_NOTE_NETBSD_DESCSZ      4
/* NetBSD-specific note name */
#define ELF_NOTE_NETBSD_NAME        "NetBSD\0\0"

/* GNU-specific note name and description sizes */
#define ELF_NOTE_GNU_NAMESZ         4
#define ELF_NOTE_GNU_DESCSZ         4
/* GNU-specific note name */
#define ELF_NOTE_GNU_NAME           "GNU\0"

/* GNU-specific OS/version value stuff */
#define ELF_NOTE_GNU_OSMASK         (unsigned int)0xff000000
#define ELF_NOTE_GNU_OSLINUX        (unsigned int)0x01000000
#define ELF_NOTE_GNU_OSMACH         (unsigned int)0x00000000

#include <machine/elf_machdep.h>

#ifdef _KERNEL

#define ELF_AUX_ENTRIES     8       /* Size of aux array passed to loader */

struct elf_args {
    unsigned int    arg_entry;      /* program entry point */
    unsigned int    arg_interp;     /* Interpreter load address */
    unsigned int    arg_phaddr;     /* program header address */
    unsigned int    arg_phentsize;  /* Size of program header */
    unsigned int    arg_phnum;      /* Number of program headers */
};

#ifndef _LKM
#include "opt_execfmt.h"
#endif

int exec_elf_makecmds __P((struct proc *, struct exec_package *));
int elf_read_from __P((struct proc *, struct vnode *, u_long,
        caddr_t, int));
void    *elf_copyargs __P((struct exec_package *, struct ps_strings *,
        void *, void *));

/* common */
int exec_elf_setup_stack __P((struct proc *, struct exec_package *));

#endif /* _KERNEL */

#endif /* !_SYS_EXEC_ELF_H_ */
