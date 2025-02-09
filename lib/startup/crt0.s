	.text
	.text
	.globl	_start                          # -- Begin function _start
        .align  2
	.type	_start,@function
	.set	nomicromips
	.set	nomips16
	.ent	_start
_start:                                 # @_start
	.set	noreorder
	.set	nomacro
	.set	noat
	addiu	$sp, $sp, -24
	sw	$ra, 20($sp)                    # 4-byte Folded Spill

	lui	$gp, %hi(_gp)
	addiu	$gp, $gp, %lo(_gp)

	lui	$1, %hi(environ)
	blez	$4, $BB0_2
	sw	$6, %lo(environ)($1)

	lw	$3, 0($5)
	bnez	$3, $BB0_3
	nop
$BB0_2:
	jal	main
	nop
	jal	exit
	move	$4, $2
$BB0_3:
	lui	$2, %hi(__progname)
	sw	$3, %lo(__progname)($2)
	addiu	$3, $3, 1
	addiu	$7, $zero, 47
$BB0_4:                                 # =>This Inner Loop Header: Depth=1
	lbu	$8, -1($3)
	beq	$8, $7, $BB0_7
	nop
# %bb.5:                                #   in Loop: Header=BB0_4 Depth=1
	bnez	$8, $BB0_8
	nop
# %bb.6:
	j	$BB0_2
	nop
$BB0_7:                                 #   in Loop: Header=BB0_4 Depth=1
	sw	$3, %lo(__progname)($2)
$BB0_8:                                 #   in Loop: Header=BB0_4 Depth=1
	j	$BB0_4
	addiu	$3, $3, 1
	.set	at
	.set	macro
	.set	reorder
                                        # -- End function
	.type	$LC0,@object                   # @.str
	.section	.rodata.str1.1,"aMS",@progbits,1
        .align  2
$LC0:
        .ascii  "\000"

	.type	__progname,@object              # @__progname
	.data
	.globl	__progname
        .align  2
__progname:
        .word   $LC0

        .comm   environ,4,4
