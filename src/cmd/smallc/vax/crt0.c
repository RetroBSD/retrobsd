#asm
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
#endasm
exit(x) int x; {
        x;
#asm
        pushl   r0
        calls   $1,exit2
exit2:
        .word   0x0000
        chmk    $exit
#endasm

}

