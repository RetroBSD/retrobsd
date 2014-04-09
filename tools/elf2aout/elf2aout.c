/*
 * This program converts an elf executable to a BSD a.out executable.
 * The minimal symbol table is copied, but the debugging symbols and
 * other informational sections are not.
 *
 * Copyright (c) 1995 Ted Lemon (hereinafter referred to as the author)
 * Copyright (c) 2011 Serge Vakulenko
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <getopt.h>

/*
 * Header prepended to each a.out file.
 */
struct	exec {
	unsigned a_magic;	/* magic number */
#define OMAGIC      0407        /* old impure format */

        unsigned a_text;	/* size of text segment */
        unsigned a_data;	/* size of initialized data */
        unsigned a_bss;		/* size of uninitialized data */
        unsigned a_reltext;	/* size of text relocation info */
        unsigned a_reldata;	/* size of data relocation info */
        unsigned a_syms;	/* size of symbol table */
        unsigned a_entry; 	/* entry point */
};

struct	nlist {
	union {
		char *n_name;	/* In memory address of symbol name */
		unsigned n_strx; /* String table offset (file) */
	} n_un;
	u_char	n_type;		/* Type of symbol - see below */
	char	n_ovly;		/* Overlay number */
	u_int	n_value;	/* Symbol value */
};

/*
 * Simple values for n_type.
 */
#define	N_UNDF	0x00		/* undefined */
#define	N_ABS	0x01		/* absolute */
#define	N_TEXT	0x02		/* text segment */
#define	N_DATA	0x03		/* data segment */
#define	N_BSS	0x04		/* bss segment */
#define	N_REG	0x14		/* register symbol */
#define	N_FN	0x1f		/* file name */

#define	N_EXT	0x20		/* external (global) bit, OR'ed in */
#define	N_TYPE	0x1f		/* mask for all the type bits */

struct sect {
	/* should be unsigned long, but assume no a.out binaries on LP64 */
	uint32_t vaddr;
	uint32_t len;
};

/*
 * Standard ELF types, structures, and macros.
 */

/* The ELF file header.  This appears at the start of every ELF file.  */

#define EI_NIDENT (16)

typedef struct
{
  unsigned char	e_ident[EI_NIDENT];	/* Magic number and other info */
  uint16_t	e_type;			/* Object file type */
  uint16_t	e_machine;		/* Architecture */
  uint32_t	e_version;		/* Object file version */
  uint32_t	e_entry;		/* Entry point virtual address */
  uint32_t	e_phoff;		/* Program header table file offset */
  uint32_t	e_shoff;		/* Section header table file offset */
  uint32_t	e_flags;		/* Processor-specific flags */
  uint16_t	e_ehsize;		/* ELF header size in bytes */
  uint16_t	e_phentsize;		/* Program header table entry size */
  uint16_t	e_phnum;		/* Program header table entry count */
  uint16_t	e_shentsize;		/* Section header table entry size */
  uint16_t	e_shnum;		/* Section header table entry count */
  uint16_t	e_shstrndx;		/* Section header string table index */
} Elf32_Ehdr;

/* Program segment header.  */

typedef struct
{
  uint32_t	p_type;			/* Segment type */
  uint32_t	p_offset;		/* Segment file offset */
  uint32_t	p_vaddr;		/* Segment virtual address */
  uint32_t	p_paddr;		/* Segment physical address */
  uint32_t	p_filesz;		/* Segment size in file */
  uint32_t	p_memsz;		/* Segment size in memory */
  uint32_t	p_flags;		/* Segment flags */
  uint32_t	p_align;		/* Segment alignment */
} Elf32_Phdr;

/* Section header.  */

typedef struct
{
  uint32_t	sh_name;		/* Section name (string tbl index) */
  uint32_t	sh_type;		/* Section type */
  uint32_t	sh_flags;		/* Section flags */
  uint32_t	sh_addr;		/* Section virtual addr at execution */
  uint32_t	sh_offset;		/* Section file offset */
  uint32_t	sh_size;		/* Section size in bytes */
  uint32_t	sh_link;		/* Link to another section */
  uint32_t	sh_info;		/* Additional section information */
  uint32_t	sh_addralign;		/* Section alignment */
  uint32_t	sh_entsize;		/* Entry size if section holds table */
} Elf32_Shdr;

/* Symbol table entry.  */

typedef struct
{
  uint32_t	st_name;		/* Symbol name (string tbl index) */
  uint32_t	st_value;		/* Symbol value */
  uint32_t	st_size;		/* Symbol size */
  unsigned char	st_info;		/* Symbol type and binding */
  unsigned char	st_other;		/* Symbol visibility */
  uint16_t	st_shndx;		/* Section index */
} Elf32_Sym;

/* Legal values for p_type (segment type).  */

#define	PT_NULL		0		/* Program header table entry unused */
#define PT_LOAD		1		/* Loadable program segment */
#define PT_DYNAMIC	2		/* Dynamic linking information */
#define PT_INTERP	3		/* Program interpreter */
#define PT_NOTE		4		/* Auxiliary information */
#define PT_SHLIB	5		/* Reserved */
#define PT_PHDR		6		/* Entry for header table itself */
#define PT_TLS		7		/* Thread-local storage segment */
#define	PT_NUM		8		/* Number of defined types */
#define PT_LOOS		0x60000000	/* Start of OS-specific */
#define PT_GNU_EH_FRAME	0x6474e550	/* GCC .eh_frame_hdr segment */
#define PT_GNU_STACK	0x6474e551	/* Indicates stack executability */
#define PT_GNU_RELRO	0x6474e552	/* Read-only after relocation */
#define PT_LOSUNW	0x6ffffffa
#define PT_SUNWBSS	0x6ffffffa	/* Sun Specific segment */
#define PT_SUNWSTACK	0x6ffffffb	/* Stack segment */
#define PT_HISUNW	0x6fffffff
#define PT_HIOS		0x6fffffff	/* End of OS-specific */
#define PT_LOPROC	0x70000000	/* Start of processor-specific */
#define PT_HIPROC	0x7fffffff	/* End of processor-specific */

/* Legal values for p_type field of Elf32_Phdr.  */

#define PT_MIPS_REGINFO	0x70000000	/* Register usage information */
#define PT_MIPS_RTPROC  0x70000001	/* Runtime procedure table. */
#define PT_MIPS_OPTIONS 0x70000002

/* Legal values for p_flags (segment flags).  */

#define PF_X		(1 << 0)	/* Segment is executable */
#define PF_W		(1 << 1)	/* Segment is writable */
#define PF_R		(1 << 2)	/* Segment is readable */
#define PF_MASKOS	0x0ff00000	/* OS-specific */
#define PF_MASKPROC	0xf0000000	/* Processor-specific */

/* How to extract and insert information held in the st_info field.  */

#define ELF32_ST_BIND(val)		(((unsigned char) (val)) >> 4)
#define ELF32_ST_TYPE(val)		((val) & 0xf)
#define ELF32_ST_INFO(bind, type)	(((bind) << 4) + ((type) & 0xf))

/* Legal values for ST_TYPE subfield of st_info (symbol type).  */

#define STT_NOTYPE	0		/* Symbol type is unspecified */
#define STT_OBJECT	1		/* Symbol is a data object */
#define STT_FUNC	2		/* Symbol is a code object */
#define STT_SECTION	3		/* Symbol associated with a section */
#define STT_FILE	4		/* Symbol's name is file name */
#define STT_COMMON	5		/* Symbol is a common data object */
#define STT_TLS		6		/* Symbol is thread-local data object*/
#define	STT_NUM		7		/* Number of defined types.  */
#define STT_LOOS	10		/* Start of OS-specific */
#define STT_GNU_IFUNC	10		/* Symbol is indirect code object */
#define STT_HIOS	12		/* End of OS-specific */
#define STT_LOPROC	13		/* Start of processor-specific */
#define STT_HIPROC	15		/* End of processor-specific */

/* Special section indices.  */

#define SHN_UNDEF	0		/* Undefined section */
#define SHN_LORESERVE	0xff00		/* Start of reserved indices */
#define SHN_LOPROC	0xff00		/* Start of processor-specific */
#define SHN_BEFORE	0xff00		/* Order section before all others
					   (Solaris).  */
#define SHN_AFTER	0xff01		/* Order section after all others
					   (Solaris).  */
#define SHN_HIPROC	0xff1f		/* End of processor-specific */
#define SHN_LOOS	0xff20		/* Start of OS-specific */
#define SHN_HIOS	0xff3f		/* End of OS-specific */
#define SHN_ABS		0xfff1		/* Associated symbol is absolute */
#define SHN_COMMON	0xfff2		/* Associated symbol is common */
#define SHN_XINDEX	0xffff		/* Index is in extra table.  */
#define SHN_HIRESERVE	0xffff		/* End of reserved indices */

/* Legal values for ST_BIND subfield of st_info (symbol binding).  */

#define STB_LOCAL	0		/* Local symbol */
#define STB_GLOBAL	1		/* Global symbol */
#define STB_WEAK	2		/* Weak symbol */
#define	STB_NUM		3		/* Number of defined types.  */
#define STB_LOOS	10		/* Start of OS-specific */
#define STB_GNU_UNIQUE	10		/* Unique symbol.  */
#define STB_HIOS	12		/* End of OS-specific */
#define STB_LOPROC	13		/* Start of processor-specific */
#define STB_HIPROC	15		/* End of processor-specific */

void	combine (struct sect *, struct sect *, int);
int	phcmp (const void *, const void *);
char   *save_read (int file, off_t offset, off_t len, const char *name);
void	copy (int, int, off_t, off_t);
void	translate_syms (int, int, off_t, off_t, off_t, off_t);

int    *symTypeTable;

int
main(int argc, char **argv)
{
	Elf32_Ehdr ex;
	Elf32_Phdr *ph;
	Elf32_Shdr *sh;
	char   *shstrtab;
	int     strtabix, symtabix;
	int     i;
	struct sect text, data, bss;
	struct exec aex;
	int     infile, outfile;
	uint32_t cur_vma = UINT32_MAX;
	int     verbose = 0;
	int     symflag = 0;

	strtabix = symtabix = 0;
	text.len = data.len = bss.len = 0;
	text.vaddr = data.vaddr = bss.vaddr = 0;

	/* Check args... */
	for (;;) {
		switch (getopt (argc, argv, "sv")) {
		case EOF:
			break;
		case 'v':
			++verbose;
			continue;
		case 's':
			++symflag;
			continue;
		default:
usage:                  fprintf(stderr,
                            "usage: elf2aout [-v] [-s] <elf executable> <a.out executable>\n");
                        exit(1);
		}
		break;
	}
	argc -= optind;
	argv += optind;
	if (argc != 2)
		goto usage;

	/* Try the input file... */
	if ((infile = open(argv[0], O_RDONLY)) < 0) {
		fprintf(stderr, "Can't open %s for read: %s\n",
		    argv[0], strerror(errno));
		exit(1);
	}
	/* Read the header, which is at the beginning of the file... */
	i = read(infile, &ex, sizeof ex);
	if (i != sizeof ex) {
		fprintf(stderr, "ex: %s: %s.\n",
		    argv[0], i ? strerror(errno) : "End of file reached");
		exit(1);
	}
	/* Read the program headers... */
	ph = (Elf32_Phdr *) save_read(infile, ex.e_phoff,
	    ex.e_phnum * sizeof(Elf32_Phdr), "ph");

	/* Read the section headers... */
	sh = (Elf32_Shdr *) save_read(infile, ex.e_shoff,
	    ex.e_shnum * sizeof(Elf32_Shdr), "sh");

	/* Read in the section string table. */
	shstrtab = save_read(infile, sh[ex.e_shstrndx].sh_offset,
	    sh[ex.e_shstrndx].sh_size, "shstrtab");

	/* Find space for a table matching ELF section indices to a.out symbol
	 * types. */
	symTypeTable = malloc(ex.e_shnum * sizeof(int));
	if (symTypeTable == NULL) {
		fprintf(stderr, "symTypeTable: can't allocate.\n");
		exit(1);
	}
	memset(symTypeTable, 0, ex.e_shnum * sizeof(int));

	/* Look for the symbol table and string table... Also map section
	 * indices to symbol types for a.out */
	for (i = 0; i < ex.e_shnum; i++) {
		char   *name = shstrtab + sh[i].sh_name;
		if (!strcmp(name, ".symtab"))
			symtabix = i;

		else if (!strcmp(name, ".strtab"))
                        strtabix = i;

                else if (!strcmp(name, ".text") || !strcmp(name, ".rodata"))
                        symTypeTable[i] = N_TEXT;

                else if (!strcmp(name, ".data") || !strcmp(name, ".sdata") ||
                         !strcmp(name, ".lit4") || !strcmp(name, ".lit8"))
                        symTypeTable[i] = N_DATA;

                else if (!strcmp(name, ".bss") || !strcmp(name, ".sbss"))
                        symTypeTable[i] = N_BSS;
	}

	/* Figure out if we can cram the program header into an a.out
	 * header... Basically, we can't handle anything but loadable
	 * segments, but we can ignore some kinds of segments.   We can't
	 * handle holes in the address space, and we handle start addresses
	 * other than 0x1000 by hoping that the loader will know where to load
	 * - a.out doesn't have an explicit load address.   Segments may be
	 * out of order, so we sort them first. */
	qsort(ph, ex.e_phnum, sizeof(Elf32_Phdr), phcmp);
	for (i = 0; i < ex.e_phnum; i++) {
		/* Section types we can ignore... */
		if (ph[i].p_type == PT_NULL || ph[i].p_type == PT_NOTE ||
		    ph[i].p_type == PT_PHDR || ph[i].p_type == PT_MIPS_REGINFO)
			continue;

		/* Section types we can't handle... */
		if (ph[i].p_type != PT_LOAD)
			errx(1, "Program header %d type %d can't be converted.", i, ph[i].p_type);
                if (verbose)
                        printf ("Section type=%x flags=%x vaddr=%x filesz=%x\n",
                            ph[i].p_type, ph[i].p_flags, ph[i].p_vaddr, ph[i].p_filesz);

		/* Writable (data) segment? */
		if (ph[i].p_flags & PF_W) {
			struct sect ndata, nbss;

			ndata.vaddr = ph[i].p_vaddr;
			ndata.len = ph[i].p_filesz;
			nbss.vaddr = ph[i].p_vaddr + ph[i].p_filesz;
			nbss.len = ph[i].p_memsz - ph[i].p_filesz;

			combine(&data, &ndata, 0);
			combine(&bss, &nbss, 1);
		} else {
			struct sect ntxt;

			ntxt.vaddr = ph[i].p_vaddr;
			ntxt.len = ph[i].p_filesz;

			combine(&text, &ntxt, 0);
		}
		/* Remember the lowest segment start address. */
		if (ph[i].p_vaddr < cur_vma)
			cur_vma = ph[i].p_vaddr;
	}
        if (! symflag) {
                /* Sections must be in order to be converted... */
                if (text.vaddr > data.vaddr || data.vaddr > bss.vaddr ||
                    text.vaddr + text.len > data.vaddr || data.vaddr + data.len > bss.vaddr) {
                        fprintf(stderr, "Sections ordering prevents a.out conversion.\n");
                        exit(1);
                }
        }

	/* If there's a data section but no text section, then the loader
	 * combined everything into one section.   That needs to be the text
	 * section, so just make the data section zero length following text. */
	if (data.len && text.len == 0) {
		text = data;
		data.vaddr = text.vaddr + text.len;
		data.len = 0;
	}
        if (verbose)
                printf ("Text vaddr = %x, data vaddr = %x\n", text.vaddr, data.vaddr);

	/* If there is a gap between text and data, we'll fill it when we copy
	 * the data, so update the length of the text segment as represented
	 * in a.out to reflect that, since a.out doesn't allow gaps in the
	 * program address space. */
	if (text.vaddr + text.len < data.vaddr) {
                if (verbose)
                        printf ("Update text len = %x (was %x)\n",
                                data.vaddr - text.vaddr, text.len);
		text.len = data.vaddr - text.vaddr;
        }

	/* We now have enough information to cons up an a.out header... */
	aex.a_magic = OMAGIC;
        if (! symflag) {
                aex.a_text = text.len;
                aex.a_data = data.len;
                aex.a_bss = bss.len;
                aex.a_entry = ex.e_entry;
                aex.a_syms = 0;
        } else {
                aex.a_text = 0;
                aex.a_data = 0;
                aex.a_bss = 0;
                aex.a_entry = 0;
                aex.a_syms = (sizeof(struct nlist) * (symtabix != -1 ?
                        sh[symtabix].sh_size / sizeof(Elf32_Sym) : 0));
        }
        aex.a_reltext = 0;
        aex.a_reldata = 0;
	if (verbose) {
                printf ("  magic: %#o\n", aex.a_magic);
                printf ("   text: %#x\n", aex.a_text);
                printf ("   data: %#x\n", aex.a_data);
                printf ("    bss: %#x\n", aex.a_bss);
                printf ("reltext: %#x\n", aex.a_reltext);
                printf ("reldata: %#x\n", aex.a_reldata);
                printf ("  entry: %#x\n", aex.a_entry);
                printf ("   syms: %u\n", aex.a_syms);
        }

	/* Make the output file... */
	if ((outfile = open(argv[1], O_WRONLY | O_CREAT, 0777)) < 0) {
		fprintf(stderr, "Unable to create %s: %s\n", argv[1], strerror(errno));
		exit(1);
	}
	/* Truncate file... */
	if (ftruncate(outfile, 0)) {
		warn("ftruncate %s", argv[1]);
	}
	/* Write the header... */
	i = write(outfile, &aex, sizeof aex);
	if (i != sizeof aex) {
		perror("aex: write");
		exit(1);
	}
        if (! symflag) {
                /* Copy the loadable sections.   Zero-fill any gaps less than 64k;
                 * complain about any zero-filling, and die if we're asked to
                 * zero-fill more than 64k. */
                for (i = 0; i < ex.e_phnum; i++) {
                        /* Unprocessable sections were handled above, so just verify
                         * that the section can be loaded before copying. */
                        if (ph[i].p_type == PT_LOAD && ph[i].p_filesz) {
                                if (cur_vma != ph[i].p_vaddr) {
                                        uint32_t gap = ph[i].p_vaddr - cur_vma;
                                        char    obuf[1024];
                                        if (gap > 65536)
                                                errx(1, "Intersegment gap (%ld bytes) too large.", (long) gap);
#ifdef DEBUG
                                        warnx("Warning: %ld byte intersegment gap.",
                                            (long)gap);
#endif
                                        memset(obuf, 0, sizeof obuf);
                                        while (gap) {
                                                int     count = write(outfile, obuf, (gap > sizeof obuf
                                                        ? sizeof obuf : gap));
                                                if (count < 0) {
                                                        fprintf(stderr, "Error writing gap: %s\n",
                                                            strerror(errno));
                                                        exit(1);
                                                }
                                                gap -= count;
                                        }
                                }
                                copy(outfile, infile, ph[i].p_offset, ph[i].p_filesz);
                                cur_vma = ph[i].p_vaddr + ph[i].p_filesz;
                        }
                }
        } else {
                /* Copy and translate the symbol table... */
                translate_syms(outfile, infile,
                        sh[symtabix].sh_offset, sh[symtabix].sh_size,
                        sh[strtabix].sh_offset, sh[strtabix].sh_size);
        }
	/* Looks like we won... */
	return 0;
}

/*
 * translate_syms (out, in, offset, size)
 *
 * Read the ELF symbol table from in at offset; translate it into a.out
 * nlist format and write it to out.
 */
void
translate_syms(int out, int in, off_t symoff, off_t symsize,
    off_t stroff, off_t strsize)
{
#define SYMS_PER_PASS	64
	Elf32_Sym inbuf[64];
	struct nlist outbuf[64];
	int     i, remaining, cur;
	char   *oldstrings;
	char   *newstrings, *nsp;
	int     newstringsize, stringsizebuf;

	/* Zero the unused fields in the output buffer.. */
	memset(outbuf, 0, sizeof outbuf);

	/* Find number of symbols to process... */
	remaining = symsize / sizeof(Elf32_Sym);

	/* Suck in the old string table... */
	oldstrings = save_read(in, stroff, strsize, "string table");

	/* Allocate space for the new one.   XXX We make the wild assumption
	 * that no two symbol table entries will point at the same place in
	 * the string table - if that assumption is bad, this could easily
	 * blow up. */
	newstringsize = strsize + remaining;
	newstrings = malloc(newstringsize);
	if (newstrings == NULL) {
		fprintf(stderr, "No memory for new string table!\n");
		exit(1);
	}
	/* Initialize the table pointer... */
	nsp = newstrings;

	/* Go the start of the ELF symbol table... */
	if (lseek(in, symoff, SEEK_SET) < 0) {
		perror("translate_syms: lseek");
		exit(1);
	}
	/* Translate and copy symbols... */
	while (remaining) {
		cur = remaining;
		if (cur > SYMS_PER_PASS)
			cur = SYMS_PER_PASS;
		remaining -= cur;
		if ((i = read(in, inbuf, cur * sizeof(Elf32_Sym)))
		    != cur * (ssize_t)sizeof(Elf32_Sym)) {
			if (i < 0)
				perror("translate_syms");
			else
				fprintf(stderr, "translate_syms: premature end of file.\n");
			exit(1);
		}
		/* Do the translation... */
		for (i = 0; i < cur; i++) {
			int     binding, type;

			/* Copy the symbol into the new table, but prepend an
			 * underscore. */
			*nsp = '_';
			strcpy(nsp + 1, oldstrings + inbuf[i].st_name);
			outbuf[i].n_un.n_strx = nsp - newstrings + 4;
			nsp += strlen(nsp) + 1;

			type = ELF32_ST_TYPE(inbuf[i].st_info);
			binding = ELF32_ST_BIND(inbuf[i].st_info);

			/* Convert ELF symbol type/section/etc info into a.out
			 * type info. */
			if (type == STT_FILE)
				outbuf[i].n_type = N_FN;

			else if (inbuf[i].st_shndx == SHN_UNDEF)
				outbuf[i].n_type = N_UNDF;

			else if (inbuf[i].st_shndx == SHN_ABS)
				outbuf[i].n_type = N_ABS;

			else if (inbuf[i].st_shndx == SHN_COMMON)
				outbuf[i].n_type = N_ABS /*N_COMM*/;

			else
				outbuf[i].n_type = symTypeTable[inbuf[i].st_shndx];

			if (binding == STB_GLOBAL)
				outbuf[i].n_type |= N_EXT;

			/* Symbol values in executables should be compatible. */
			outbuf[i].n_value = inbuf[i].st_value;
		}
		/* Write out the symbols... */
		if ((i = write(out, outbuf, cur * sizeof(struct nlist)))
		    != cur * (ssize_t)sizeof(struct nlist)) {
			fprintf(stderr, "translate_syms: write: %s\n", strerror(errno));
			exit(1);
		}
	}
	/* Write out the string table length... */
	stringsizebuf = newstringsize;
	if (write(out, &stringsizebuf, sizeof stringsizebuf)
	    != sizeof stringsizebuf) {
		fprintf(stderr,
		    "translate_syms: newstringsize: %s\n", strerror(errno));
		exit(1);
	}
	/* Write out the string table... */
	if (write(out, newstrings, newstringsize) != newstringsize) {
		fprintf(stderr, "translate_syms: newstrings: %s\n", strerror(errno));
		exit(1);
	}
}

void
copy(int out, int in, off_t offset, off_t size)
{
	char    ibuf[4096];
	int     remaining, cur, count;

	/* Go to the start of the ELF symbol table... */
	if (lseek(in, offset, SEEK_SET) < 0) {
		perror("copy: lseek");
		exit(1);
	}
	remaining = size;
	while (remaining) {
		cur = remaining;
		if (cur > (int)sizeof ibuf)
			cur = sizeof ibuf;
		remaining -= cur;
		if ((count = read(in, ibuf, cur)) != cur) {
			fprintf(stderr, "copy: read: %s\n",
			    count ? strerror(errno) : "premature end of file");
			exit(1);
		}
		if ((count = write(out, ibuf, cur)) != cur) {
			perror("copy: write");
			exit(1);
		}
	}
}

/*
 * Combine two segments, which must be contiguous.
 * If pad is true, it's okay for there to be padding between.
 */
void
combine(struct sect *base, struct sect *new, int pad)
{

	if (base->len == 0)
		*base = *new;
	else
		if (new->len) {
			if (base->vaddr + base->len != new->vaddr) {
				if (pad)
					base->len = new->vaddr - base->vaddr;
				else {
					fprintf(stderr,
					    "Non-contiguous data can't be converted.\n");
					exit(1);
				}
			}
			base->len += new->len;
		}
}

int
phcmp(const void *vh1, const void *vh2)
{
	const Elf32_Phdr *h1, *h2;

	h1 = (const Elf32_Phdr *)vh1;
	h2 = (const Elf32_Phdr *)vh2;

	if (h1->p_vaddr > h2->p_vaddr)
		return 1;
	else
		if (h1->p_vaddr < h2->p_vaddr)
			return -1;
		else
			return 0;
}

char *
save_read(int file, off_t offset, off_t len, const char *name)
{
	char   *tmp;
	int     count;
	off_t   off;
	if ((off = lseek(file, offset, SEEK_SET)) < 0) {
		fprintf(stderr, "%s: fseek: %s\n", name, strerror(errno));
		exit(1);
	}
	if ((tmp = malloc(len)) == NULL)
		errx(1, "%s: Can't allocate %ld bytes.", name, (long)len);
	count = read(file, tmp, len);
	if (count != len) {
		fprintf(stderr, "%s: read: %s.\n",
		    name, count ? strerror(errno) : "End of file reached");
		exit(1);
	}
	return tmp;
}
