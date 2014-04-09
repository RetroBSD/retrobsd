/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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

/*
 * C library -- _setjmp, _longjmp
 *
 *	_longjmp(a,v)
 * will generate a "return(v)" from
 * the last call to
 *	_setjmp(a)
 * by restoring registers from the stack,
 * The previous signal state is NOT restored.
 */
		.set	noreorder

		.type	_setjmp, @function
_setjmp:	.globl 	_setjmp

		sw	$s0, (0 * 4) ($a0)	# save register variables s0-s8
		sw	$s1, (1 * 4) ($a0)
		sw	$s2, (2 * 4) ($a0)
		sw	$s3, (3 * 4) ($a0)
		sw	$s4, (4 * 4) ($a0)
		sw	$s5, (5 * 4) ($a0)
		sw	$s6, (6 * 4) ($a0)
		sw	$s7, (7 * 4) ($a0)
		sw	$s8, (8 * 4) ($a0)	# frame pointer
		sw	$ra, (9 * 4) ($a0)	# return address
		sw	$gp, (10 * 4) ($a0)	# global data pointer
		sw	$sp, (11 * 4) ($a0)	# stack pointer

		# For compatibility with longjmp and siglongjmp
		sw	$zero, (12 * 4) ($a0)   # signal mask saved
		sw	$zero, (13 * 4) ($a0)   # signal mask

		j	$ra
		move	$v0, $zero		# return a zero for the setjmp call

		.type	_longjmp, @function
_longjmp:	.globl	_longjmp

		lw	$s0, (0 * 4) ($a0)	# restore register variables s0-s8
		lw	$s1, (1 * 4) ($a0)
		lw	$s2, (2 * 4) ($a0)
		lw	$s3, (3 * 4) ($a0)
		lw	$s4, (4 * 4) ($a0)
		lw	$s5, (5 * 4) ($a0)
		lw	$s6, (6 * 4) ($a0)
		lw	$s7, (7 * 4) ($a0)
		lw	$s8, (8 * 4) ($a0)	# frame pointer
		lw	$ra, (9 * 4) ($a0)	# return address
		lw	$gp, (10 * 4) ($a0)	# global data pointer
		lw	$sp, (11 * 4) ($a0)	# stack pointer

		j	$ra			# transfer back to setjmp()
		move	$v0, $a1		# get return value in 1st arg
