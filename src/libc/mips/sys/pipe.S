/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "SYS.h"

ENTRY(pipe)
		.set	noreorder
		syscall	SYS_pipe
		j	1f		# return here on error
		nop
		.set	reorder

		sw	$v0, 0($a0)	# return here on success
		sw	$v1, 4($a0)
		move	$v0, $zero
		jr	$ra
1:
		lui	$t1, %hi(errno)
		sw      $t0, %lo(errno)($t1)
		jr	$ra
