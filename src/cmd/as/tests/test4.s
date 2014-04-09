	.file	1 "crt0.c"
	.section .mdebug.abi32
	.previous
	.gnu_attribute 4, 3
	.section	.text._start,"ax",@progbits
	.align	2
	.globl	_start
	.set	nomips16
	.set	nomicromips
	.ent	_start
	.type	_start, @function
_start:
	.frame	$sp,24,$31		# vars= 0, regs= 1/0, args= 16, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
# Begin mchp_output_function_prologue
# End mchp_output_function_prologue
	addiu	$sp,$sp,-24
	sw	$31,20($sp)
 #APP
 # 69 "crt0.c" 1
	la $gp, _gp
 # 0 "" 2
 #NO_APP
	.set	noreorder
	.set	nomacro
	blez	$4,.L2
	sw	$6,%gp_rel(environ)($28)
	.set	macro
	.set	reorder

	lw	$2,0($5)
	beq	$2,$0,.L2
	sw	$2,%gp_rel(__progname)($28)
	lb	$3,0($2)
	.set	noreorder
	.set	nomacro
	beq	$3,$0,.L2
	addiu	$2,$2,1
	.set	macro
	.set	reorder

	li	$7,47			# 0x2f
.L4:
	.set	noreorder
	.set	nomacro
	beql	$3,$7,.L3
	sw	$2,%gp_rel(__progname)($28)
	.set	macro
	.set	reorder

.L3:
	lb	$3,0($2)
	.set	noreorder
	.set	nomacro
	bne	$3,$0,.L4
	addiu	$2,$2,1
	.set	macro
	.set	reorder

.L2:
	jal	main
	.set	noreorder
	.set	nomacro
	jal	exit
	move	$4,$2
	.set	macro
	.set	reorder

# Begin mchp_output_function_epilogue
# End mchp_output_function_epilogue
	.end	_start
	.size	_start, .-_start
	.globl	__progname
	.section	.rodata.str1.4,"aMS",@progbits,1
	.align	2
.LC0:
	.ascii	"\000"
	.section	.sdata.__progname,"aw",@progbits
	.align	2
	.type	__progname, @object
	.size	__progname, 4
__progname:
	.word	.LC0

	.comm	environ,4,4
	.ident	"GCC: (chipKIT) 4.5.1 chipKIT Compiler for PIC32 MCUs v1.30-20110506"
# Begin MCHP vector dispatch table
# End MCHP vector dispatch table
# MCHP configuration words
