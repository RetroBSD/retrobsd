/*
 * reloc - program to print out all of the a.out header, symbol table, and
 * relocation information in an a.out file.  Prints out more information
 * than ``nm'', and correlates a relocation info to symbol table entries if
 * it can.
 *
 * Author: Bruce Ediger, bediger@csn.net
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * without any conditions or restrictions.  This software is provided
 * ``as is'' without express or implied warranty.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <a.out.h>
#include <stab.h>

char *read_extent(FILE *fin, int rsize, int offset, int calling_lineno);
char *read_stringtable(FILE *fin, struct exec *exhdr, int *strtbl_sz);

void cleanup(FILE *fin, int count, ...);

void run_header(struct exec *exhdr, int extended_info);
void run_reloc(struct exec *exhdr, int size, struct relocation_info *data_reloc, struct nlist *symtabl, char *stringtable, int strtbl_sz, int correspondence);
void run_symtable(struct exec *exhdr, struct nlist *symbol_table, char *stringtable, int strtbl_sz, int print_symnames);

char *print_reloc_type(enum reloc_type type);
char *symbol_type(struct nlist *sym);
char *print_reloc_segment(int segnum);

extern char *optarg;
extern int   optind;

struct stat attributes;

int main(int ac, char **av)
{
	FILE *fin = NULL;
	struct exec *exhdr = NULL;
	char *fname = NULL;
	char *stringtable;
	int   strtbl_sz;
	struct relocation_info *data_reloc = NULL, *text_reloc = NULL;
	struct nlist *symbol_table = NULL;
	int show_header       = 0;
	int show_data_reloc   = 0;
	int show_text_reloc   = 0;
	int show_symbol_table = 0;
	int print_symnames    = 1;
	int correspondence    = 0;
	int hdr_info          = 0;
	int opt;

	while (-1 != (opt = getopt(ac, av, "hdtsacxn"))) {
		switch (opt) {
		case 'h': show_header = 1; break;
		case 'x': show_header = hdr_info = 1; break;  /* show_header implied */
		case 'd': show_data_reloc = 1; break;
		case 't': show_text_reloc = 1; break;
		case 's': show_symbol_table = 1; break;
		case 'c': correspondence = 1; break;
		case 'n': print_symnames = 0; break;
		case 'a':
			show_header = show_data_reloc = show_text_reloc = show_symbol_table = correspondence = hdr_info = 1;
			break;
		default: errout:
			fprintf(stderr, "%s: show exec header, relocation and symbol table data\n", av[0]);
			fprintf(stderr, "usage: %s [-h] [-d] [-t] [-s] [-c] [-x] [-a] <filename>\n", av[0]);
			fprintf(stderr, " -h: show exec header\n"
				" -x: show info derivable from exec header \n"
				" -d: show data segment relocations\n"
				" -t: show text segment relocations\n"
				" -c: show symbols that correspond to relocations\n"
				" -s: show symbol table\n"
				" -n: when showing symbol table, don't print symbol names\n"
				" -a: show it all\n");
			return 1;
			break;
		}
	}

	if (0 == show_header && 0 == show_data_reloc && 0 == show_text_reloc &&
		0 == show_symbol_table)
			show_header = show_data_reloc = show_text_reloc = show_symbol_table = correspondence = 1;

	if (optind < ac)
		fname = av[optind];

	if (NULL == fname) {
		fprintf(stderr, "Need a file to examine\n");
		goto errout;
	}

	if (NULL == (fin = fopen(fname, "r"))) {
		fprintf(stderr, "Couldn't open \"%s\" for reading: %s\n",
			fname, strerror(errno));
		return 1;
	}

	if (stat(fname, &attributes) < 0) {
		fprintf(stderr, "Problem on stat(2) of \"%s\": %s\n",
			fname, strerror(errno));
		fclose(fin);
		return 1;
	}

	exhdr = (struct exec *)read_extent(fin, sizeof(*exhdr), 0, __LINE__);

	if (NULL == exhdr) {
		cleanup(fin, 1, exhdr);
		return 2;
	}

	if (show_header) {
		puts("a.out header:");
		run_header(exhdr, hdr_info);
	}

	if (N_BADMAG(*exhdr)) {
		fprintf(stderr, "%s isn't a recognized NetBSD a.out file\n", fname);
		cleanup(fin, 1, exhdr);
		return 3;
	}

	stringtable  = read_stringtable(fin, exhdr, &strtbl_sz);
	data_reloc   = (struct relocation_info *)read_extent(fin, exhdr->a_drsize, N_DRELOFF(*exhdr), __LINE__);
	text_reloc   = (struct relocation_info *)read_extent(fin, exhdr->a_trsize, N_TRELOFF(*exhdr), __LINE__);
	symbol_table = (struct nlist *)          read_extent(fin, exhdr->a_syms,   N_SYMOFF(*exhdr), __LINE__);

	if (show_data_reloc) {
		puts("\nData relocations:");
		run_reloc(exhdr, exhdr->a_drsize, data_reloc, symbol_table, stringtable, strtbl_sz, correspondence);
	}

	if (show_text_reloc) {
		puts("\nText relocations:");
		run_reloc(exhdr, exhdr->a_trsize, text_reloc, symbol_table, stringtable, strtbl_sz, correspondence);
	}

	if (show_symbol_table) {
		puts("\nSymbol table:");
		run_symtable(exhdr, symbol_table, stringtable, strtbl_sz, print_symnames);
	}

	cleanup(fin, 5, exhdr, stringtable, data_reloc, text_reloc, symbol_table);

	return 0;
}

void
cleanup(FILE *fin, int count, ...)
{
	va_list ap;
	int i;

	if (NULL != fin)
		fclose(fin);

	va_start(ap, count);

	for (i = 0; i < count; ++i) {
		char *p = va_arg(ap, char *);

		if (NULL != p)
			free(p);
	}

	va_end(ap);
}

char *
read_stringtable(FILE *fin, struct exec *exhdr, int *strtbl_sz)
{
	unsigned long length;
	char *p = NULL;

	assert(NULL != fin);

	if (NULL == exhdr) {
		fprintf(stderr, "Trying to read string table, but got no a.out header\n");
		return NULL;
	}

	if (fseek(fin, N_STROFF(*exhdr), SEEK_SET) < 0) {
		fprintf(stderr, "Problem seeking to string table: %s\n",
			strerror(errno));
		return NULL;
	}

	if (fread(&length, sizeof(length), 1, fin) != 1) {
		fprintf(stderr, "Problem reading length of string table: %s\n",
			strerror(errno));
		return NULL;
	}

	if (length >= 4) {
		p = read_extent(fin, length, N_STROFF(*exhdr), __LINE__);
		*strtbl_sz = length;
		printf("\nstring table size %d\n", *strtbl_sz);
	} else  {
		fprintf(stderr, "Problem, length of string table read as %d,"
			" can't be right\n, ", length);
		*strtbl_sz = 0;
	}

	return p;
}


char *
read_extent(FILE *fin, int rsize, int offset, int calling_lineno)
{
	char *r = NULL;
	off_t potential_eof = (off_t)rsize + (off_t)offset;

	assert(NULL != fin);

	if (potential_eof > attributes.st_size) {
		fprintf(stderr, "Line %d, reading extent from %d, size %d:"
			" larger than file size %qd\n", calling_lineno, offset,
			rsize, attributes.st_size);

	} else if (fseek(fin, offset, SEEK_SET) < 0) {

		fprintf(stderr, "Line %d, seeking to extent at %d, file size %qd: %s\n",
			calling_lineno, offset, attributes.st_size, strerror(errno));

	} else {

		r = malloc(rsize);
		assert(NULL != r);

		if (fread(r, 1, rsize, fin) != rsize) {
			fprintf(stderr, "Line %d, problem reading in extent\n", calling_lineno);
			free(r);
			r = NULL;
		}
	}

	return r;
}

void
run_reloc(
	struct exec *exhdr,
	int size,
	struct relocation_info *reloc,
	struct nlist *symtabl,
	char *stringtable,
	int   strtbl_sz,
	int   correspondence
)
{
	int i, count;
	int max_ordinal;

	if (NULL == exhdr || NULL == symtabl || NULL == stringtable || NULL == reloc) {
		if (NULL == exhdr) fprintf(stderr, "a.out header NULL, ");
		if (NULL == symtabl) fprintf(stderr, "symbol table NULL, ");
		if (NULL == stringtable) fprintf(stderr, "string table NULL, ");
		if (NULL == reloc) fprintf(stderr, "relocation table NULL, ");
		fprintf(stderr, " can't possibly list the relocations meaningfully\n");
		return;
	}

	max_ordinal = exhdr->a_syms/sizeof(*symtabl);

	count = size/sizeof(*reloc);

	printf("%d relocations\n", count);

	for (i = 0; i < count; ++i) {
		printf("r_address 0x%x, symbol ordinal %d, ", reloc[i].r_address,
			reloc[i].r_symbolnum);

		if (reloc[i].r_extern)
			fputs("extern, ", stdout);
		else {
			printf("local, %s segment, ", print_reloc_segment(reloc[i].r_symbolnum));
		}

		printf(
			"r_addend 0x%x (%d), type %s (0x%x)",
			reloc[i].r_addend,
			reloc[i].r_addend,
			print_reloc_type(reloc[i].r_type), reloc[i].r_type
		);


		if (reloc[i].r_extern && correspondence && reloc[i].r_symbolnum <= max_ordinal) {
			char *sname = "";
			/* stuff from struct nlist associated with this relocation */

			if (symtabl[reloc[i].r_symbolnum].n_un.n_strx >= 4 &&
				symtabl[reloc[i].r_symbolnum].n_un.n_strx < strtbl_sz)
					sname = &stringtable[symtabl[reloc[i].r_symbolnum].n_un.n_strx];
			else
				fprintf(stderr,
					"relocation entry %d, symbol ordinal %d, n_strx is %d, "
					"string table size is %d\n",
					i, reloc[i].r_symbolnum, symtabl[reloc[i].r_symbolnum].n_un.n_strx,
					strtbl_sz);

			printf("\n  name \"%s\" (sym num %d), type 0x%x - %s, desc 0x%x, value 0x%x\n",
				sname,
				reloc[i].r_symbolnum,
				symtabl[reloc[i].r_symbolnum].n_type,
				symbol_type(&symtabl[reloc[i].r_symbolnum]),
				symtabl[reloc[i].r_symbolnum].n_desc,
				symtabl[reloc[i].r_symbolnum].n_value
			);
		} else
			fputc('\n', stdout);
	}
}

char *
print_reloc_type(enum reloc_type type)
{
	char *r = NULL;

	if (type == RELOC_8) {
		r = "RELOC_8, 8 bit simple, ";
	} else if (type == RELOC_16) {
		r = "RELOC_16, 16 bit simple, ";
	} else if (type == RELOC_32) {
		r = "RELOC_32, 32 bit simple, ";
	} else if (type == RELOC_DISP8) {
		r = "RELOC_DISP8, 8 bit PC relative, ";
	} else if (type == RELOC_DISP16) {
		r = "RELOC_DISP16, 16 bit PC relative, ";
	} else if (type == RELOC_DISP32) {
		r = "RELOC_DISP32, 32 bit PC relative, ";
	} else if (type == RELOC_WDISP30) {
		r = "RELOC_WDISP30, 30 bit PC relative CALL, ";
	} else if (type == RELOC_WDISP22) {
		r = "RELOC_WDISP22, 22 bit PC relative BRANCH, ";
	} else if (type == RELOC_HI22) {
		r = "RELOC_HI22, ";
	} else if (type == RELOC_22) {
		r = "RELOC_22, ";
	} else if (type == RELOC_13) {
		r = "RELOC_13, ";
	} else if (type == RELOC_LO10) {
		r = "RELOC_LO10, ";
	} else if (type == RELOC_BASE13) {
		r = "RELOC_BASE13, ";
	} else if (type == RELOC_BASE22) {
		r = "RELOC_BASE22, ";
	} else if (type == RELOC_PC10) {
		r = "RELOC_PC10, 10 bit PC relative PIC, ";
	} else if (type == RELOC_PC22) {
		r = "RELOC_PC22, 22 bit PC relative PIC, ";
	} else if (type == RELOC_JMP_TBL) {
		r = "RELOC_JMP_TBL, jump table relative PIC, ";
	} else if (type == RELOC_GLOB_DAT) {
		r = "RELOC_GLOB_DAT, rtld global data, ";
	} else if (type == RELOC_JMP_SLOT) {
		r = "RELOC_JMP_SLOT, rtld jump slot, ";
	} else if (type == RELOC_RELATIVE) {
		r = "RELOC_RELATIVE, rtld relative, ";
	} else {
		r = "some unknown reloc type..., ";
	}

	assert(NULL != r);

	return r;
}

char *
print_reloc_segment(int segnum)
{
	char *r = "unknown segment";

	switch (segnum) {
	case N_TEXT: r = "N_TEXT"; break;
	case N_DATA: r = "N_DATA"; break;
	case N_BSS:  r = "N_BSS";  break;
	}

	return r;
}

static char symbol_type_buffer[512];

char *
symbol_type(struct nlist *sym)
{
	unsigned char n_type = sym->n_type;

	symbol_type_buffer[0] = '\0';

/*
    N_EXT  = 0x01 = 0000 0001
	N_TYPE = 0x1e = 0001 1110
	N_STAB = 0xe0 = 1110 0000   <= not just a mask. if n_type field has
                                   one or more of these bits set, the whole
                                   n_type field is signifigant.
 */

	if (n_type & N_EXT)
		strcat(symbol_type_buffer, "external, ");

	if ((n_type & N_TYPE) & ~N_EXT) {
		switch ((n_type & N_TYPE) & ~N_EXT) {
		case N_UNDF  : strcat(symbol_type_buffer, "undefined, ");        break;
		case N_ABS   : strcat(symbol_type_buffer, "absolute address, "); break;
		case N_TEXT  : strcat(symbol_type_buffer, "text segment, ");     break;
		case N_DATA  : strcat(symbol_type_buffer, "data segment, ");     break;
		case N_BSS   : strcat(symbol_type_buffer, "bss segment, ");      break;
		case N_INDR  : strcat(symbol_type_buffer, "alias definition, "); break;
		case N_SIZE  : strcat(symbol_type_buffer, "symbol's size, ");    break;
		case N_COMM  : strcat(symbol_type_buffer, "common reference, "); break;
		case N_FN    :
			if (n_type & N_EXT)
				strcat(symbol_type_buffer, "file name, ");
			else
				strcat(symbol_type_buffer, "warning message, ");
			break;
		}
	}

	if ((n_type & N_STAB)) {
		switch (n_type & ~N_EXT) {
		case N_GSYM:  strcat(symbol_type_buffer, "global symbol, "); break;
		case N_FNAME: strcat(symbol_type_buffer, "F77 function name, "); break;
		case N_FUN:   strcat(symbol_type_buffer, "procedure name, "); break;
		case N_STSYM: strcat(symbol_type_buffer, "data segment variable, "); break;
		case N_LCSYM: strcat(symbol_type_buffer, "bss segment variable, "); break;
		case N_MAIN:  strcat(symbol_type_buffer, "main function name, "); break;
		case N_PC:    strcat(symbol_type_buffer, "global Pascal symbol, "); break;
		case N_RSYM:  strcat(symbol_type_buffer, "register variable, "); break;

		case N_DSLINE: case N_BSLINE: case N_SLINE:
			sprintf(
				&symbol_type_buffer[strlen(symbol_type_buffer)],
				"line %d at address 0x%x, ",
				sym->n_desc,
				sym->n_value
			);
			break;

		case N_SSYM: strcat(symbol_type_buffer, "structure/union element, "); break;
		case N_SO:   strcat(symbol_type_buffer, "main source file name, "); break;

		case N_LSYM:
			sprintf(
				&symbol_type_buffer[strlen(symbol_type_buffer)],
				"stack variable, [%%fp + %d]",
				sym->n_value
			);
			break;

		case N_BINCL: strcat(symbol_type_buffer, "include file beginning, "); break;
		case N_SOL:   strcat(symbol_type_buffer, "included source file name, "); break;

		case N_PSYM:
			sprintf(
				&symbol_type_buffer[strlen(symbol_type_buffer)],
				"parameter variable, [%%fp + %d]",
				sym->n_value
			);
			break;

		case N_EINCL: strcat(symbol_type_buffer, "include file end, "); break;
		case N_ENTRY: strcat(symbol_type_buffer, "alternate entry point, "); break;
		case N_LBRAC: strcat(symbol_type_buffer, "left bracket, "); break;
		case N_EXCL:  strcat(symbol_type_buffer, "deleted include file, "); break;
		case N_RBRAC: strcat(symbol_type_buffer, "right bracket, "); break;
		case N_BCOMM: strcat(symbol_type_buffer, "begin common, "); break;
		case N_ECOMM: strcat(symbol_type_buffer, "end common, "); break;
		case N_ECOML: strcat(symbol_type_buffer, "end common (local name), "); break;
		case N_LENG:  strcat(symbol_type_buffer, "length of preceding entry, "); break;
		}
	}

	if (' ' == symbol_type_buffer[strlen(symbol_type_buffer)-1])
		symbol_type_buffer[strlen(symbol_type_buffer)-1] = '\0';
	if (',' == symbol_type_buffer[strlen(symbol_type_buffer)-1])
		symbol_type_buffer[strlen(symbol_type_buffer)-1] = '\0';

	return symbol_type_buffer;
}

void
run_symtable(
	struct exec *exhdr,
	struct nlist *symtabl,
	char *stringtable,
	int strtbl_sz,
	int sym_names)
{
	int i, count;

	if (NULL == exhdr || NULL == symtabl || NULL == stringtable) {
		if (NULL == exhdr) fprintf(stderr, "a.out header NULL, ");
		if (NULL == symtabl) fprintf(stderr, "symbol table NULL, ");
		if (NULL == stringtable) fprintf(stderr, "string table NULL, ");
		fprintf(stderr, " can't possibly list the symbol table meaningfully\n");
		return;
	}

	count = exhdr->a_syms/sizeof(*symtabl);

	printf("%d symbols\n", count);

	for (i = 0; i < count; ++i) {
		if (!sym_names)
			printf("%4d name offset (%d), type 0x%x - %s, other 0x%x, desc 0x%x, value 0x%x (%d)\n",
				i,
				symtabl[i].n_un.n_strx,
				symtabl[i].n_type,
				symbol_type(&symtabl[i]),
				symtabl[i].n_other,
				symtabl[i].n_desc,
				symtabl[i].n_value,
				symtabl[i].n_value
			);
		else {
			char *sname = "";

			if (symtabl[i].n_un.n_strx >= 0 && symtabl[i].n_un.n_strx < strtbl_sz)
				sname = &stringtable[symtabl[i].n_un.n_strx];

			printf("%4d name \"%s\" (%d), type 0x%x - %s, other 0x%x, desc 0x%x, value 0x%x (%d)\n",
				i,
				sname,
				symtabl[i].n_un.n_strx,
				symtabl[i].n_type,
				symbol_type(&symtabl[i]),
				symtabl[i].n_other,
				symtabl[i].n_desc,
				symtabl[i].n_value,
				symtabl[i].n_value
			);
		}
	}
}

void
run_header(struct exec *exhdr, int extended_info)
{
	char *id = NULL;

	assert(NULL != exhdr);

	/* print raw values */
	printf(
		"\ta_midmag 0x%08x (mid %d, magic 0%o, flag 0x%x)\n"
		"\ta_text   0x%08x\n"
		"\ta_data   0x%08x\n"
		"\ta_bss    0x%08x\n"
		"\ta_syms   0x%08x\n"
		"\ta_entry  0x%08x\n"
		"\ta_trsize 0x%08x\n"
		"\ta_drsize 0x%08x\n",
		exhdr->a_midmag,
		N_GETMID(*exhdr), N_GETMAGIC(*exhdr), N_GETFLAG(*exhdr),
		exhdr->a_text,
		exhdr->a_data,
		exhdr->a_bss,
		exhdr->a_syms,
		exhdr->a_entry,
		exhdr->a_trsize,
		exhdr->a_drsize
	);

	printf(
		"magic number %04o: %s\n", N_GETMAGIC(*exhdr),
		N_GETMAGIC(*exhdr) == OMAGIC ?  "old impure format" :
		N_GETMAGIC(*exhdr) == NMAGIC ?  "read-only text" :
		N_GETMAGIC(*exhdr) == ZMAGIC ?  "demand load format" :
		N_GETMAGIC(*exhdr) == QMAGIC ?  "deprecated format" :
		"totally funky"
	);

	switch (N_GETMID(*exhdr)) {
	case MID_ZERO:    id = "unknown - implementation dependent"; break;
	case MID_SUN010:  id = "sun 68010/68020 binary";             break;
	case MID_SUN020:  id = "sun 68020-only binary";              break;
	case MID_PC386:   id = "386 PC binary. (so quoth BFD)";      break;
	case MID_HP200:   id = "hp200 (68010) BSD binary";           break;
	case MID_I386:    id = "i386 BSD binary";                    break;
	case MID_M68K:    id = "m68k BSD binary with 8K page sizes"; break;
	case MID_M68K4K:  id = "m68k BSD binary with 4K page sizes"; break;
	case MID_NS32532: id = "ns32532";                            break;
	case MID_SPARC:   id = "sparc";                              break;
	case MID_PMAX:    id = "pmax";                               break;
	case MID_VAX:     id = "vax";                                break;
	case MID_ALPHA:   id = "Alpha BSD binary";                   break;
	case MID_MIPS:    id = "big-endian MIPS";                    break;
	case MID_ARM6:    id = "ARM6";                               break;
	case MID_HP300:   id = "hp300 (68020+68881) BSD binary";     break;
	case MID_HPUX:    id = "hp200/300 HP-UX binary";             break;
	case MID_HPUX800: id = "hp800 HP-UX binary";                 break;
	default:
		id = "don't know"; break;
	}

	printf("type %d, %s\n", N_GETMID(*exhdr), id);

	/* this left shift seems a bit bogus */

	switch((N_GETFLAG(*exhdr) & EX_DPMASK)>>4) {
	case 0:
		id = "traditional executable or object file"; break;
	case 1:
		id = "object file contains PIC code"; break;
	case 2:
		id = "dynamic executable"; break;
	case 3:
		id = "position independent executable image"; break;
	default:
		id = NULL;
	}

	if (NULL != id)
		printf("flags: 0x%x, %s\n", N_GETFLAG(*exhdr), id);
	else
		printf("flags: 0x%x\n", N_GETFLAG(*exhdr));

	if (extended_info) {
		unsigned long txt_addr;
		unsigned long dat_addr;
		unsigned long bss_addr;

		/* N_TXTADDR and N_DATADDR macros DON'T WORK */
		if (N_GETMAGIC(*exhdr) == ZMAGIC) {
			txt_addr = __LDPGSZ;
			dat_addr = ((txt_addr + exhdr->a_text + __LDPGSZ - 1) & ~(__LDPGSZ - 1));
		} else if (N_GETMAGIC(*exhdr) == OMAGIC) {
			txt_addr = 0;
			dat_addr = txt_addr + exhdr->a_text;
		} else {
			txt_addr = 0xdeadbeef;
			dat_addr = 0xcafebabe;
		}

		bss_addr = dat_addr + exhdr->a_data;

		printf("	text segment size  = 0x%lx, text segment file offset = %ld\n", exhdr->a_text, N_TXTOFF(*exhdr));
		printf("	data segment size  = 0x%lx, data segment file offset = %ld\n", exhdr->a_data, N_DATOFF(*exhdr));
		printf("	bss  segment size  = 0x%lx\n", exhdr->a_bss);
		printf("	text segment relocation size  = 0x%lx, file offset   = %ld, %d text relocations\n",
			exhdr->a_trsize, N_TRELOFF(*exhdr), exhdr->a_trsize/sizeof(struct relocation_info));
		printf("	data segment relocation size  = 0x%lx, file offset   = %ld, %d data relocations\n",
			exhdr->a_drsize, N_DRELOFF(*exhdr), exhdr->a_drsize/sizeof(struct relocation_info));
		printf("	symbol table size  = 0x%lx, symbol table file offset = %ld (%d symbols)\n",
			exhdr->a_syms, N_SYMOFF(*exhdr), exhdr->a_syms/sizeof(struct nlist));
		printf("	string table file offset = 0x%lx (%d)\n", N_STROFF(*exhdr), N_STROFF(*exhdr));
		printf("	entry point  = 0x%lx\n", exhdr->a_entry);
		printf("	text address = 0x%lx\n\tdata address = 0x%lx\n"
			"\tbss address = 0x%lx\n",
			txt_addr, dat_addr, bss_addr
			/* N_TXTADDR(*exhdr), N_DATADDR(*exhdr), N_BSSADDR(*exhdr) */
		);
	}
}
