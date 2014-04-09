#define EOL 10
getchar() {
#asm
        movl    $0,r0
        pushl   $1
        pushal  buff
        pushl   $0
        calls   $3,Xread
        cvtbl   buff,r0
        .data
buff:   .space 1
        .text
#endasm

}

#asm
        .set    read,3
Xread:
        .word   0x0000
        chmk    $read
        bcc     noerror2
        jmp     cerror
noerror2:
        ret
cerror: bpt
#endasm

putchar (c) char c; {
        c;
#asm
        cvtlb   r0,buff
        pushl   $1
        pushal  buff
        pushl   $1
        calls   $3,Xwrite
        cvtbl   buff,r0
#endasm

}

#asm
        .set    write,4
Xwrite:
        .word   0x0000
        chmk    $write
        bcc     noerror
        jmp     cerror
noerror:
        ret
#endasm
