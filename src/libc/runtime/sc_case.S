        .set reorder
	.text
#
# $v0 = value to switch on
# 0($sp) = pointer to list of value,ptr cases
#         ended where ptr=0, value is used as pointer to jump to in default case
# looks like stack is popped as part of this
# FIXME - The assembler/linker only stores the bottom 16 bits
# of the labels in pair, so we construct the address by merging the 16 bits
# in the cell with the upper 16 bits in the return address of the code that
# called this. Is there a way to get the assembler linker to store the full
# address? If so, that should be used instead.
#
        .globl  __sc_case
__sc_case:
	lw	$t1, 16($sp)	# t1=pointer to list of value/ptr pairs
	addiu	$sp, $sp, 4	# pop stack that held pointer
1:
        lw      $t2, 0($t1)     # get value from pair
        lw      $t3, 4($t1)     # get ptr from pair
        beq     $t3, $zero, 2f

	beq     $t2, $v0, 3f

        addiu   $t1, $t1, 8     # t1 += size of pair
        j       1b

2:
        move	$t3, $t2
3:
	lui     $t2, 0xffff
	and	$t2, $t2, $ra
        or      $t3, $t3, $t2
	jr	$t3
