	.file	1 "hello.c"
	.section .mdebug.abi32
	.previous
	.gnu_attribute 4, 3
	.rdata
	.align	2
.LC0:
	.ascii	"Hello, World!\012\000"
	.section	.text.hello,"ax",@progbits
	.align	2
	.globl	hello
	.set	nomips16
	.set	nomicromips
	.ent	hello
	.type	hello, @function
hello:
	.frame	$fp,24,$31		# vars= 0, regs= 2/0, args= 16, gp= 0
	.mask	0xc0000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
# Begin mchp_output_function_prologue
# End mchp_output_function_prologue
	addiu	$sp,$sp,-24
	sw	$31,20($sp)
	sw	$fp,16($sp)
	move	$fp,$sp
	lui	$2,%hi(.LC0)
	addiu	$4,$2,%lo(.LC0)
	jal	puts
	nop

	move	$sp,$fp
	lw	$31,20($sp)
	lw	$fp,16($sp)
	addiu	$sp,$sp,24
	j	$31
	nop

	.set	macro
	.set	reorder
# Begin mchp_output_function_epilogue
# End mchp_output_function_epilogue
	.end	hello
	.size	hello, .-hello
	.ident	"GCC: (chipKIT) 4.5.1 chipKIT Compiler for PIC32 MCUs v1.30-20110506"
# Begin MCHP vector dispatch table
# End MCHP vector dispatch table
# MCHP configuration words
