/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef	_NLIST_H_
#define	_NLIST_H_
#include <sys/types.h>

/*
 * Symbol table entry format.
 */
struct	nlist {
	char    *n_name;	/* In memory address of symbol name,
	                           or string table offset (file) */
	u_short	n_len;		/* Length of name in bytes */
	u_short	n_type;		/* Type of symbol - see below */
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
#define	N_STRNG	0x05		/* string segment (for assembler) */
#define	N_COMM	0x06		/* .comm segment (for assembler) */
#define	N_FN	0x1f		/* file name */

#define	N_TYPE	0x1f		/* mask for all the type bits */
#define	N_EXT	0x20		/* external (global) bit, OR'ed in */
#define	N_WEAK	0x40		/* weak reference bit, OR'ed in */
#define	N_LOC	0x80		/* local, for assembler */

/*
 * Get symbols from a file.
 */
int nlist (char *name, struct nlist *list);

/*
 * Get kernel symbols.
 */
int knlist (struct nlist *list);

#endif	/* !_NLIST_H_ */
