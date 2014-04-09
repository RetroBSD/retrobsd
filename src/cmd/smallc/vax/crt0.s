#       Small C VAX
#       Coder (2.1,83/04/05)
#       Front End (2.1,83/03/20)
        .globl  lneg
        .globl  case
        .globl  eq
        .globl  ne
        .globl  lt
        .globl  le
        .globl  gt
        .globl  ge
        .globl  ult
        .globl  ule
        .globl  ugt
        .globl  uge
        .globl  bool
        .text
##asm
# C runtime startoff
        .set    exit,1
.globl  start
.globl  _main
.globl  _exit
#
#       C language startup routine
start:
        .word   0x0000
        subl2   $8,sp
        movl    8(sp),4(sp)  #  argc
        movab   12(sp),r0
        movl    r0,(sp)  #  argv
        jsb     _main
        addl2   $8,sp
        pushl   r0
        chmk    $exit
#exit(x) int x; {
        .align  1
_exit:

#       x;
        moval   4(sp),r0
        movl    (r0),r0
##asm
        pushl   r0
        calls   $1,exit2
exit2:
        .word   0x0000
        chmk    $exit
#}
LL1:

        rsb
        .data
        .globl  _etext
        .globl  _edata
        .globl  _exit

#0 error(s) in compilation
#       literal pool:0
#       global pool:42
#       Macro pool:43
