/*      VAX fopen, fclose, fgetc, fputc, feof
 * gawd is this gross - no buffering!
*/
#include <stdio.h>

static  feofed[20];
static  char    charbuf[1];
static  retcode;

fopen(filnam, mod) char *filnam, *mod; {
        if (*mod == 'w') {
                filnam;
#asm
                pushl   r0
                calls   $1,zunlink
#endasm
                filnam;
#asm
                pushl   $0644
                pushl   r0
                calls   $2,zcreat
                movl    r0,_retcode
#endasm
                if (retcode < 0) {
                        return(NULL);
                } else return(retcode);
        }
        filnam;
#asm
        pushl   $0      # read mode
        pushl   r0
        calls   $2,zopen
        movl    r0,_retcode
#endasm
        feofed[retcode] = 0;
        if (retcode < 0) return (NULL);
        else return(retcode);

}

fclose(unit) int unit; {
        unit;
#asm
        pushl   r0
        calls   $1,zclose
#endasm

}

fgetc(unit) int unit; {
        unit;
#asm
        pushl   $1
        pushl   $_charbuf
        pushl   r0
        calls   $3,zread
        movl    r0,_retcode
#endasm
        if (retcode <= 0) {
                feofed[unit] = 1;
                return(EOF);
        } else
                return(charbuf[0]);

}

fputc(c, unit) int c, unit; {
        charbuf[0] = c;
        unit;
#asm
        pushl   $1
        pushl   $_charbuf
        pushl   r0
        calls   $3,zwrite
#endasm
        return(c);

}

feof(unit) int unit; {
        if (feofed[unit]) return(1);
        else return(NULL);

}

/*      Assembler assists       */
#asm
        .set    unlink,10
        .set    creat,8
        .set    open,5
        .set    close,6
        .set    read,3
        .set    write,4
zunlink:
        .word   0x0000
        chmk    $unlink
        bcc     noerr
        jmp     cerror
zcreat:
        .word   0x0000
        chmk    $creat
        bcc     noerr
        jmp     cerror
zopen:
        .word   0x0000
        chmk    $open
        bcc     noerr
        jmp     cerror
zclose:
        .word   0x0000
        chmk    $close
        bcc     noerr
        jmp     cerror
zread:
        .word   0x0000
        chmk    $read
        bcc     noerr
        jmp     cerror
zwrite:
        .word   0x0000
        chmk    $write
        bcc     noerr
        jmp     cerror

cerror:
        mnegl   $1,r0
        ret
noerr:  ret
#endasm
