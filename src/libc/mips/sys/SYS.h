/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <syscall.h>

#define	ENTRY(s)     s:	.globl s; \
			.type   s, @function

#define	SYS(s)		ENTRY(s); \
			.set	noreorder; \
			syscall	SYS_##s; \
			lui	$t1, %hi(errno); \
			sw      $t0, %lo(errno)($t1); \
			.set	reorder; \
			jr	$ra
