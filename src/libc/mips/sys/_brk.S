/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "SYS.h"

ENTRY(_brk)
		.set	noreorder
		syscall	SYS_sbrk
		lui	$t1, %hi(errno)		# return here on error
		sw      $t0, %lo(errno)($t1)
		.set	reorder

		jr	$ra			# return here on success
