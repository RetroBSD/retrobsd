/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)mcount.s	1.1 (Berkeley) 5/2/87\0>
	.even
#endif LIBC_SCCS

/*
 * Count subroutine calls during simple (non-gprof) profiling.  This file is
 * appended to the compiled assembly output of mon.c.
 *
 * struct cnt {
 *	int	(*pc)();		/ address of function
 *	long	ncall;			/ number of times _foo entered
 * }
 *
 * mcount(cntp::r0)
 *	long	**cntp;
 *
 * Mcount is called with a pointer to a function's local call count pointer in
 * r0.  On first entry, mcount allocates a (struct cnt) and initializes it with
 * the function's base segment address and points the functions local call
 * counter pointer just past the structure's ncall field.  (we do this so we
 * can do long increments slightly faster)
 *
 * The C compiler generates the following preamble for every function
 * compiled with profiling:
 *
 *	.data
 *	1:	_foo+1
 *	.text
 *
 *	_foo:				/ unique name of foo (always in base)
 *	~foo:				/ real address of foo
 *	+0	jsr	r5,csv		/ perform standard C entry
 *	+4	mov	$1b,r0		/ point to local call counter pointer
 *	+8	jsr	pc,mcount	/ and have mcount do its thing
 *	+12	...			/ first `real' word of ~foo
 *
 * Function and ncall field addresses are always even so the "_foo+1" doesn't
 * destroy information and can be used to determine a first call to mcount.
 *
 * Note that because we now use functions' base segment address rather than
 * our own return address (as was done in the past) and because the call to
 * mcount is after that to csv, profiling now works for overlaid objects.
 * Only static routines in overlays which are assigned the same address by
 * the loader may be counted incorrectly.
 */
#include "../sys/SYS.h"
#undef	PROF

	.text
ASENTRY(mcount)
	tst	_countbase		/ buffer set up yet?
	beq	2f			/ nope, just exit
	mov	(r0),r1			/ cnt struct allocated yet?
	bit	$1,r1
	bne	3f			/ nope, grab one
1:	
	add	$1,-(r1)		/ increment *(*cnt-1)
	adc	-(r1)
2:
	rts	pc			/ and return
3:
	mov	_countbase,r1
	cmp	r1,_countend		/ no, out of cnt structs?
	bhis	3f			/   yes, output error message
	mov	(r0),(r1)		/ save _foo
	bic	$1,(r1)+
	cmp	(r1)+,(r1)+		/ move on to next cnt struct and
	mov	r1,_countbase		/   save in _countbase
	mov	r1,(r0)			/ save pointer to &ncall+1 in *cntp
	br	1b			/ increment ncall (to 1)
3:
	mov	$9f-8f,-(sp)		/ ran out cnt structs, output an
	mov	$8f,-(sp)		/   error message
	mov	$2,-(sp)
	tst	-(sp)			/ simulate return address stack
	SYS(write)			/   spacing and perform write (we
	add	$8.,sp			/   have to do the syscall because
	rts	pc			/   _write calls mcount)

.data
8:	<mcount: counter overflow\n>
9:
.text
