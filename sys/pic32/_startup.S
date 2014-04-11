#
# Startup code for Microchip PIC32 microcontrollers.
# Using HID bootloader.
#
# Copyright (C) 2010 Serge Vakulenko, <serge@vak.ru>
#
# Permission to use, copy, modify, and distribute this software
# and its documentation for any purpose and without fee is hereby
# granted, provided that the above copyright notice appear in all
# copies and that both that the copyright notice and this
# permission notice and warranty disclaimer appear in supporting
# documentation, and that the name of the author not be used in
# advertising or publicity pertaining to distribution of the
# software without specific, written prior permission.
#
# The author disclaim all warranties with regard to this
# software, including all implied warranties of merchantability
# and fitness.  In no event shall the author be liable for any
# special, indirect or consequential damages or any damages
# whatsoever resulting from loss of use, data or profits, whether
# in an action of contract, negligence or other tortious action,
# arising out of or in connection with the use or performance of
# this software.
#
#include "machine/io.h"

#define UBASE	0x7f008000	/* User space base address */

		.set	noreorder
		.set	mips32r2
		.set	nomips16

		.extern	u
		.extern	u_end
		.extern	u0
		.extern	main
		.extern exception

#---------------------------------------
# Reset vector: main entry point
#
		.section .startup,"ax",@progbits
		.org	0
		.type	_reset_vector_, @function
_reset_vector_:	.globl	_reset_vector_

		.set	noat
		move    $1, $zero                       # Clear all regs
		move    $2, $zero
		move    $3, $zero
		move    $4, $zero
		move    $5, $zero
		move    $6, $zero
		move    $7, $zero
		move    $8, $zero
		move    $9, $zero
		move    $10, $zero
		move    $11, $zero
		move    $12, $zero
		move    $13, $zero
		move    $14, $zero
		move    $15, $zero
		move    $16, $zero
		move    $17, $zero
		move    $18, $zero
		move    $19, $zero
		move    $20, $zero
		move    $21, $zero
		move    $22, $zero
		move    $23, $zero
		move    $24, $zero
		move    $25, $zero
		move    $26, $zero
		move    $27, $zero
		move    $28, $zero
		move    $29, $zero
		move    $30, $zero
		move    $31, $zero
		mtlo	$zero
		mthi	$zero
		.set	at

		la	$sp, u_end - 16			# Stack at end of U area
                la      $a0, main
		jalr	$a0				# Jump to main()
		lui	$gp, 0x8000			# Set global pointer (delay slot)

                la      $k0, UBASE
		mtc0	$k0, $C0_EPC                    # Entry to user code.

		mfc0	$k0, $C0_STATUS
		ori	$k0, $k0, ST_UM | ST_EXL | ST_IE # Set user mode and enable interrupts
		mtc0	$k0, $C0_STATUS			# Put SR back
		ehb
		eret					# PC <= EPC; EXL <= 0
		nop					# just to be safe


#---------------------------------------
# Secondary entry point for RetroBSD bootloader.
#
		.section .exception,"ax",@progbits
_exception_base_: .globl _exception_base_

		.org	0
		.type	_entry_vector_, @function
_entry_vector_: .globl  _entry_vector_
                la      $k0, _reset_vector_
                jr      $k0
                nop

#---------------------------------------
# Exception vector: handle interrupts and exceptions
#
		.org	0x200
		.type	_exception_vector_, @function
_exception_vector_: .globl _exception_vector_

		mfc0	$k0, $C0_STATUS
		andi	$k1, $k0, ST_UM			# Check user mode
		beqz	$k1, kernel_exception
		move	$k1, $sp

		#
		# Exception in user mode: switch stack.
		#
user_exception:
		la	$sp, u_end                      # Stack at end of U area
kernel_exception:
		addi    $sp, -16-FRAME_WORDS*4		# Allocate space for registers
save_regs:
		sw	$k0, (16+FRAME_STATUS*4) ($sp)
                sw      $k1, (16+FRAME_SP*4) ($sp)

		.set	noat
		sw	$1, (16+FRAME_R1*4) ($sp)	# Save general registers
		sw	$2, (16+FRAME_R2*4) ($sp)
		sw	$3, (16+FRAME_R3*4) ($sp)
		sw	$4, (16+FRAME_R4*4) ($sp)
		sw	$5, (16+FRAME_R5*4) ($sp)
		sw	$6, (16+FRAME_R6*4) ($sp)
		sw	$7, (16+FRAME_R7*4) ($sp)
		sw	$8, (16+FRAME_R8*4) ($sp)
		sw	$9, (16+FRAME_R9*4) ($sp)
		sw	$10, (16+FRAME_R10*4) ($sp)
		sw	$11, (16+FRAME_R11*4) ($sp)
		sw	$12, (16+FRAME_R12*4) ($sp)
		sw	$13, (16+FRAME_R13*4) ($sp)
		sw	$14, (16+FRAME_R14*4) ($sp)
		sw	$15, (16+FRAME_R15*4) ($sp)
		sw	$16, (16+FRAME_R16*4) ($sp)
		sw	$17, (16+FRAME_R17*4) ($sp)
		sw	$18, (16+FRAME_R18*4) ($sp)
		sw	$19, (16+FRAME_R19*4) ($sp)
		sw	$20, (16+FRAME_R20*4) ($sp)
		sw	$21, (16+FRAME_R21*4) ($sp)
		sw	$22, (16+FRAME_R22*4) ($sp)
		sw	$23, (16+FRAME_R23*4) ($sp)
		sw	$24, (16+FRAME_R24*4) ($sp)
		sw	$25, (16+FRAME_R25*4) ($sp)
		# Skip $26 - K0
		# Skip $27 - K1
		sw	$28, (16+FRAME_GP*4) ($sp)
		# Skip $29 - SP
		sw	$30, (16+FRAME_FP*4) ($sp)
		sw	$31, (16+FRAME_RA*4) ($sp)
		.set	at

		mfhi	$k0				# Save special registers
		sw	$k0, (16+FRAME_HI*4) ($sp)

		mflo	$k0
		sw	$k0, (16+FRAME_LO*4) ($sp)

		mfc0	$k0, $C0_EPC
		sw	$k0, (16+FRAME_PC*4) ($sp)

		move	$a0, $sp
		addi	$a0, 16				# Arg 0: saved regs.
		jal	exception			# Call C code.
		lui	$gp, 0x8000			# Set global pointer (delay slot)

		#
		# Restore CPU state and return from interrupt.
		#
restore_regs:
		lw	$a0, (16+FRAME_LO*4) ($sp)	# Load HI, LO registers
		mtlo	$a0
		lw	$a0, (16+FRAME_HI*4) ($sp)
		mthi	$a0

		.set	noat
		lw	$1, (16+FRAME_R1*4) ($sp)	# Load general registers
		lw	$2, (16+FRAME_R2*4) ($sp)
		lw	$3, (16+FRAME_R3*4) ($sp)
		lw	$4, (16+FRAME_R4*4) ($sp)
		lw	$5, (16+FRAME_R5*4) ($sp)
		lw	$6, (16+FRAME_R6*4) ($sp)
		lw	$7, (16+FRAME_R7*4) ($sp)
		lw	$8, (16+FRAME_R8*4) ($sp)
		lw	$9, (16+FRAME_R9*4) ($sp)
		lw	$10, (16+FRAME_R10*4) ($sp)
		lw	$11, (16+FRAME_R11*4) ($sp)
		lw	$12, (16+FRAME_R12*4) ($sp)
		lw	$13, (16+FRAME_R13*4) ($sp)
		lw	$14, (16+FRAME_R14*4) ($sp)
		lw	$15, (16+FRAME_R15*4) ($sp)
		lw	$16, (16+FRAME_R16*4) ($sp)
		lw	$17, (16+FRAME_R17*4) ($sp)
		lw	$18, (16+FRAME_R18*4) ($sp)
		lw	$19, (16+FRAME_R19*4) ($sp)
		lw	$20, (16+FRAME_R20*4) ($sp)
		lw	$21, (16+FRAME_R21*4) ($sp)
		lw	$22, (16+FRAME_R22*4) ($sp)
		lw	$23, (16+FRAME_R23*4) ($sp)
		lw	$24, (16+FRAME_R24*4) ($sp)
		lw	$25, (16+FRAME_R25*4) ($sp)
		# Skip $26 - K0
		# Skip $27 - K1
		lw	$28, (16+FRAME_GP*4) ($sp)
		# Skip $29 - SP
		lw	$30, (16+FRAME_FP*4) ($sp)
		.set	at

                # Do not use k0/k1 here, as interrupts are still enabled
		lw	$31, (16+FRAME_STATUS*4) ($sp)	# K0 = saved status
		ori	$31, ST_EXL                     # Set EXL
		mtc0	$31, $C0_STATUS			# put SR back: disable interrupts
                ehb

		lw	$k0, (16+FRAME_PC*4) ($sp)	# K0 = EPC
		mtc0	$k0, $C0_EPC			# put PC in EPC
                ext     $k1, $31, 27, 1                 # get RP bit: single-step request

		lw	$31, (16+FRAME_RA*4) ($sp)
		lw	$sp, (16+FRAME_SP*4) ($sp)	# Restore stack

		# Return from exception
                bnez    $k1, debug_request              # single-step request
		ehb
		eret					# PC <= EPC; EXL <= 0
debug_request:
		sdbbp					# enter debug mode

#---------------------------------------
# Debug exception processing.
#
		.org	0x480
		.type	_debug_vector_, @function
_debug_vector_: .globl _debug_vector_

		mfc0	$k0, $C0_DEPC
		la	$k1, debug_request
		bne     $k0, $k1, single_step_done
		nop

                # single step request
		mfc0	$k0, $C0_DEBUG
		ori     $k0, DB_SST                     # set SST bit
		mtc0	$k0, $C0_DEBUG

		mfc0	$k1, $C0_EPC
		mtc0	$k1, $C0_DEPC                   # DEPC <= EPC
		mfc0	$k0, $C0_STATUS
		xori	$k0, ST_EXL                     # Clear EXL
		mtc0	$k0, $C0_STATUS
		ehb
		deret					# PC <= DEPC; DM <= 0

single_step_done:
		mtc0	$k0, $C0_EPC                    # EPC <= DEPC

		la      $k1, _exception_vector_
		mtc0	$k1, $C0_DEPC                   # DEPC <= exception handler

		mfc0	$k0, $C0_DEBUG
		sw	$k0, c0_debug                   # save Debug register
		ori     $k0, DB_SST
		xori    $k0, DB_SST                     # clear SST bit
		mtc0	$k0, $C0_DEBUG

		mfc0	$k1, $C0_STATUS
		ori	$k1, ST_EXL                     # Set EXL
		mtc0	$k1, $C0_STATUS
		ehb
		deret					# PC <= DEPC; DM <= 0

#---------------------------------------
# Icode is copied out to process 1 to exec /sbin/init.
# If the exec fails, process 1 exits.
#
		.globl	icode, icodeend
		.type	icode, @function
		.type	icodeend, @function
icode:
		la	$a0, UBASE
		move	$a1, $a0
		addi	$a0, etcinit - icode
		addi	$a1, argv - icode
		syscall	11				# SYS_execv
		move	$a0, $v0
		syscall	1				# SYS_exit
etcinit:
		.ascii	"/sbin/init\0"
initflags:
		.ascii	"-\0"                          # ASCII initflags
argv:
		.word	etcinit + 6 - icode + UBASE	# address of "init\0"
		.word	initflags - icode + UBASE	# init options
		.word	0

icodeend:	nop

#---------------------------------------
# int setjmp (label_t *env);
#
# Setjmp(env) will save the process' current register variables, stack
# and program counter context and return a zero.
#
		.type	setjmp, @function
setjmp:		.globl	setjmp
		sw	$s0, (0 * 4) ($a0)		# save register variables s0-s8
		sw	$s1, (1 * 4) ($a0)
		sw	$s2, (2 * 4) ($a0)
		sw	$s3, (3 * 4) ($a0)
		sw	$s4, (4 * 4) ($a0)
		sw	$s5, (5 * 4) ($a0)
		sw	$s6, (6 * 4) ($a0)
		sw	$s7, (7 * 4) ($a0)
		sw	$s8, (8 * 4) ($a0)		# frame pointer
		sw	$ra, (9 * 4) ($a0)		# return address
		sw	$gp, (10 * 4) ($a0)		# global data pointer
		sw	$sp, (11 * 4) ($a0)		# stack pointer
		j	$ra
		move	$v0, $zero			# return a zero for the setjmp call

#---------------------------------------
# void longjmp (memaddr uaddr, label_t *env);
#
# Longjmp(uaddr, env) will generate a "return(1)" from the last
# call to setjmp(env) by mapping in the user structure pointed to by uaddr,
# restoring the context saved by setjmp in env and returning a "1".
# Note that registers are recovered statically from the env buffer.
# Stack is not used.
#
# This longjmp differs from the longjmp found in the standard library -
# it's actually closer to the resume routine of the 4.3BSD kernel.
#
		.type	longjmp, @function
longjmp:	.globl	longjmp

		di				# can't let anything in till we get a valid stack...

		la	$v0, u			# pointer to &u
		beq	$v0, $a0, 2f		# if uaddr == &u...
		nop				# ...no need to remap U area

		la	$a3, u_end		# pointer to &u + USIZE
		la	$a0, u0			# pointer to &u0

		lw	$v1, 0($v0)		# u.u_procp
		sw	$a0, 60($v1)		# u.u_procp->p_addr = &u0

		# exchange contents of u and u0
		move	$v1, $v0
1:
                lw	$t1, 0($v1)
                lw	$t0, 0($a0)
                sw	$t0, 0($v1)
                sw	$t1, 0($a0)
                lw	$t1, 4($v1)
                lw	$t0, 4($a0)
                sw	$t0, 4($v1)
                sw	$t1, 4($a0)
                lw	$t1, 8($v1)
                lw	$t0, 8($a0)
                sw	$t0, 8($v1)
                sw	$t1, 8($a0)
                lw	$t1, 12($v1)
                lw	$t0, 12($a0)
                sw	$t0, 12($v1)
                sw	$t1, 12($a0)
                addiu	$v1, $v1, 16
                bne	$a3, $v1, 1b
                addiu	$a0, $a0, 16

		lw	$v1, 0($v0)		# u.u_procp
		sw	$v0, 60($v1)		# u.u_procp->p_addr = &u
2:
		lw	$s0, (0 * 4) ($a1)	# restore register variables s0-s8
		lw	$s1, (1 * 4) ($a1)
		lw	$s2, (2 * 4) ($a1)
		lw	$s3, (3 * 4) ($a1)
		lw	$s4, (4 * 4) ($a1)
		lw	$s5, (5 * 4) ($a1)
		lw	$s6, (6 * 4) ($a1)
		lw	$s7, (7 * 4) ($a1)
		lw	$s8, (8 * 4) ($a1)	# frame pointer
		lw	$ra, (9 * 4) ($a1)	# return address
		lw	$gp, (10 * 4) ($a1)	# global data pointer
		lw	$sp, (11 * 4) ($a1)	# stack pointer

		ei				# release interrupts
		j	$ra			# transfer back to setjmp()
		li	$v0, 1			# return value of 1
