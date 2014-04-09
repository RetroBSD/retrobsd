	.file	1 "hello.c"
	.section .mdebug.abi32
	.previous
	.gnu_attribute 4, 3
	.section	.rodata.str1.4,"aMS",@progbits,1
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
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
# Begin mchp_output_function_prologue
# End mchp_output_function_prologue
	lui	$4,%hi(.LC0)
	j	puts
	addiu	$4,$4,%lo(.LC0)

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
