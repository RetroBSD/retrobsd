#include <stdio.h>
#include "defs.h"
#include "data.h"

int     needr0;
int     needh;

/*
 *      Some predefinitions:
 *
 *      INTSIZE is the size of an integer in the target machine
 *      BYTEOFF is the offset of an byte within an integer on the
 *              target machine. (ie: 8080,pdp11 = 0, 6809 = 1,
 *              360 = 3)
 *      This compiler assumes that an integer is the SAME length as
 *      a pointer - in fact, the compiler uses INTSIZE for both.
 */
#define INTSIZE 4
#define BYTEOFF 3

/*
 *      print all assembler info before any code is generated
 *
 */
header ()
{
        outstr("#\tSmall C M68000\n#\tCoder (1.2,84/11/28)\n#");
        FEvers();
        nl ();
        ol ("global\tTlneg");
        ol ("global\tTcase");
        ol ("global\tTeq");
        ol ("global\tTne");
        ol ("global\tTlt");
        ol ("global\tTle");
        ol ("global\tTgt");
        ol ("global\tTge");
        ol ("global\tTult");
        ol ("global\tTule");
        ol ("global\tTugt");
        ol ("global\tTuge");
        ol ("global\tTbool");
        ol ("global\tTmult");
        ol ("global\tTdiv");
        ol ("global\tTmod");
}

nl()
{
        if (needh) {
                ol ("word\t0");
                needh = 0;
        }
        if (needr0) {
                needr0 = 0;
                outstr(",%d0");
        }
        outbyte(EOL);
}

galign(t)
        int t;
{
        int sign;
        if (t < 0) {
                sign = 1;
                t = -t;
        } else
                sign = 0;
        t = (t + INTSIZE - 1) & ~(INTSIZE - 1);
        t = sign? -t: t;
        return (t);
}

/*
 *      return size of an integer
 */
intsize()
{
        return(INTSIZE);
}

/*
 *      return offset of ls byte within word
 *      (ie: 8080 & pdp11 is 0, 6809 is 1, 360 is 3)
 */
byteoff()
{
        return(BYTEOFF);
}

/*
 *      Output internal generated label prefix
 */
olprfix()
{
        outstr("LL");
}

/*
 *      Output a label definition terminator
 */
col ()
{
        outstr (":\n");
}

/*
 *      begin a comment line for the assembler
 *
 */
comment ()
{
        outbyte ('#');
}

/*
 *      Output a prefix in front of user labels
 */
prefix ()
{
/*      outbyte ('_'); */
}

/*
 *      print any assembler stuff needed after all code
 *
 */
trailer ()
{
}

/*
 *      function prologue
 */
prologue ()
{
        /* this is where we'd put splimit stuff */
}

/*
 *      text (code) segment
 */
gtext ()
{
        ol ("text");
}

/*
 *      data segment
 */
gdata ()
{
        ol ("data");
}

/*
 *  Output the variable symbol at scptr as an extrn or a public
 */
ppubext(scptr)
        char *scptr;
{
        if (scptr[STORAGE] == STATIC)
                return;
        ot ("global\t");
        prefix ();
        outstr (scptr);
        nl();
}

/*
 * Output the function symbol at scptr as an extrn or a public
 */
fpubext(scptr)
        char *scptr;
{
        ppubext(scptr);
}

/*
 *  Output a decimal number to the assembler file
 */
onum(num)
        int num;
{
        outdec(num);    /* pdp11 needs a "." here */
}

/*
 *      fetch a static memory cell into the primary register
 */
getmem (sym)
        char    *sym;
{
        int ischr;
        if ((sym[IDENT] != POINTER) & (sym[TYPE] == CCHAR)) {
                ischr = 1;
                ot ("mov.b\t");
                prefix ();
                outstr (sym + NAME);
        } else {
                ischr = 0;
                ot ("mov.l\t");
                prefix ();
                outstr (sym + NAME);
        }
        outstr(",%d0\n");
        if (ischr)
                ol ("ext.b\t%d0");
}

/*
 *      fetch the address of the specified symbol into the primary register
 */
getloc (sym)
        char    *sym;
{
        if (sym[STORAGE] == LSTATIC) {
                immed();
                printlabel(glint(sym));
                nl();
        } else {
                ot ("lea.l\t");
                onum (glint(sym) - stkp);
                outstr (",%a0\n");
                ol ("mov.l\t%a0,%d0");
        }
}

/*
 *      store the primary register into the specified static memory cell
 */
putmem (sym)
        char    *sym;
{
        if ((sym[IDENT] != POINTER) & (sym[TYPE] == CCHAR)) {
                ot ("mov.b\t%d0,");
        } else
                ot ("mov.l\t%d0,");
        prefix ();
        outstr (sym + NAME);
        nl ();
}

/*
 *      store the specified object type in the primary register
 *      at the address on the top of the stack
 */
putstk (typeobj)
        char    typeobj;
{
        ol ("mov.l\t(%sp)+,%a0");
        if (typeobj == CCHAR)
                ol ("mov.b\t%d0,(%a0)");
        else
                ol ("mov.l\t%d0,(%a0)");
        stkp = stkp + INTSIZE;
}

/*
 *      fetch the specified object type indirect through the primary
 *      register into the primary register
 */
indirect (typeobj)
        char    typeobj;
{
        ol ("mov.l\t%d0,%a0");
        if (typeobj == CCHAR)
                ol ("mov.b\t(%a0),%d0");
        else
                ol ("mov.l\t(%a0),%d0");
}

/*
 *      swap the primary and secondary registers
 */
swap ()
{
        ol ("mov.l\t%d0,%d2\n\tmov.l\t%d1,%d0\n\tmov.l\t%d2,%d1");
}

/*
 *      print partial instruction to get an immediate value into
 *      the primary register
 */
immed ()
{
        ot ("mov.l\t&");
        needr0 = 1;
}

/*
 *      push the primary register onto the stack
 */
gpush ()
{
        ol ("mov.l\t%d0,-(%sp)");
        stkp = stkp - INTSIZE;
}

/*
 *      pop the top of the stack into the secondary register
 */
gpop ()
{
        ol ("mov.l\t(%sp)+,%d1");
        stkp = stkp + INTSIZE;
}

/*
 *      swap the primary register and the top of the stack
 */
swapstk ()
{
        ol ("mov.l\t(%sp)+,%d2\nmov.l\t%d0,-(%sp)\nmov.l\t%d2,%d0");
}

/*
 *      call the specified subroutine name
 */
gcall (sname)
        char    *sname;
{
        if (*sname == '^') {
                ot ("jsr\tT");
                outstr (++sname);
        } else {
                ot ("jsr\t");
                prefix ();
                outstr (sname);
        }
        nl ();
}

/*
 *      return from subroutine
 */
gret ()
{
        ol ("rts");
}

/*
 *      perform subroutine call to value on top of stack
 */
callstk ()
{
        ol ("jsr\t(%sp)+");
        stkp = stkp + INTSIZE;
}

/*
 *      jump to specified internal label number
 */
jump (label)
        int     label;
{
        ot ("jmp\t");
        printlabel (label);
        nl ();
}

/*
 *      test the primary register and jump if false to label
 *
 */
testjump (label, ft)
        int label;
        int ft;
{
        ol ("cmp.l\t%d0,&0");
        if (ft)
                ot ("beq\t");
        else
                ot ("bne\t");
        printlabel (label);
        nl ();
}

/*
 *      print pseudo-op  to define a byte
 */
defbyte ()
{
        ot ("byte\t");
}

/*
 *      print pseudo-op to define storage
 */
defstorage ()
{
        ot ("space\t");
}

/*
 *      print pseudo-op to define a word
 */
defword ()
{
        ot ("long\t");
}

/*
 *      modify the stack pointer to the new value indicated
 */
modstk (newstkp)
        int     newstkp;
{
        int     k;

        k = newstkp - stkp;
        if (k % INTSIZE)
                error("Bad stack alignment (compiler error)");
        if (k == 0)
                return (newstkp);
        ot ("add.l\t&");
        onum (k);
        outstr (",sp");
        nl();
        return (newstkp);
}

/*
 *      multiply the primary register by INTSIZE
 */
gaslint ()
{
        ol ("asl.l\t&2,%d0");
}

/*
 *      divide the primary register by INTSIZE
 */
gasrint()
{
        ol ("asr.l\t&2,%d0");
}

/*
 *      Case jump instruction
 */
gjcase()
{
        gcall ("^case");
}

/*
 *      add the primary and secondary registers
 *      if lval2 is int pointer and lval is int, scale lval
 */
gadd (lval, lval2)
        int *lval, *lval2;
{
        if (dbltest (lval2, lval)) {
                ol ("asl.l\t&2,(%sp)");
        }
        ol ("add.l\t(%sp)+,%d0");
        stkp = stkp + INTSIZE;

}

/*
 *      subtract the primary register from the secondary
 */
gsub ()
{
        ol ("mov.l\t(%sp)+,%d2");
        ol ("sub.l\t%d0,%d2");
        ol ("mov.l\t%d2,%d0");
        stkp = stkp + INTSIZE;
}

/*
 *      multiply the primary and secondary registers
 *      (result in primary)
 */
gmult ()
{
        gcall ("^mult");
        stkp = stkp + INTSIZE;
}

/*
 *      divide the secondary register by the primary
 *      (quotient in primary, remainder in secondary)
 */
gdiv ()
{
        gcall ("^div");
        stkp = stkp + INTSIZE;
}

/*
 *      compute the remainder (mod) of the secondary register
 *      divided by the primary register
 *      (remainder in primary, quotient in secondary)
 */
gmod ()
{
        gcall ("^mod");
        stkp = stkp + INTSIZE;
}

/*
 *      inclusive 'or' the primary and secondary registers
 */
gor ()
{
        ol ("or.l\t(%sp)+,%d0");
        stkp = stkp + INTSIZE;
}

/*
 *      exclusive 'or' the primary and secondary registers
 */
gxor ()
{
        ol ("mov.l\t(%sp)+,%d1");
        ol ("eor.l\t%d1,%d0");
        stkp = stkp + INTSIZE;
}

/*
 *      'and' the primary and secondary registers
 */
gand ()
{
        ol ("and.l\t(%sp)+,%d0");
        stkp = stkp + INTSIZE;
}

/*
 *      arithmetic shift right the secondary register the number of
 *      times in the primary register
 *      (results in primary register)
 */
gasr ()
{
        ol ("mov.l\t(%sp)+,%d1");
        ol ("asr.l\t%d0,%d1");
        ol ("mov.l\t%d1,%d0");
        stkp = stkp + INTSIZE;
}

/*
 *      arithmetic shift left the secondary register the number of
 *      times in the primary register
 *      (results in primary register)
 */
gasl ()
{
        ol ("mov.l\t(%sp)+,%d1");
        ol ("asl.l\t%d0,%d1");
        ol ("mov.l\t%d1,%d0");
        stkp = stkp + INTSIZE;
}

/*
 *      two's complement of primary register
 */
gneg ()
{
        ol ("neg.l\t%d0");
}

/*
 *      logical complement of primary register
 */
glneg ()
{
        gcall ("^lneg");
}

/*
 *      one's complement of primary register
 */
gcom ()
{
        ol ("not\t%d0");
}

/*
 *      convert primary register into logical value
 */
gbool ()
{
        gcall ("^bool");
}

/*
 *      increment the primary register by 1 if char, INTSIZE if int
 */
ginc (lval)
        int lval[];
{
        if (lval[2] == CINT)
                ol ("addq.l\t&4,%d0");
        else
                ol ("addq.l\t&1,%d0");
}

/*
 *      decrement the primary register by one if char, INTSIZE if int
 */
gdec (lval)
        int lval[];
{
        if (lval[2] == CINT)
                ol ("subq.l\t&4,%d0");
        else
                ol ("subq.l\t&1,%d0");
}

/*
 *      following are the conditional operators.
 *      they compare the secondary register against the primary register
 *      and put a literl 1 in the primary if the condition is true,
 *      otherwise they clear the primary register
 */

/*
 *      equal
 */
geq ()
{
        gcall ("^eq");
        stkp = stkp + INTSIZE;
}

/*
 *      not equal
 */
gne ()
{
        gcall ("^ne");
        stkp = stkp + INTSIZE;
}

/*
 *      less than (signed)
 */
glt ()
{
        gcall ("^lt");
        stkp = stkp + INTSIZE;
}

/*
 *      less than or equal (signed)
 */
gle ()
{
        gcall ("^le");
        stkp = stkp + INTSIZE;
}

/*
 *      greater than (signed)
 */
ggt ()
{
        gcall ("^gt");
        stkp = stkp + INTSIZE;
}

/*
 *      greater than or equal (signed)
 */
gge ()
{
        gcall ("^ge");
        stkp = stkp + INTSIZE;
}

/*
 *      less than (unsigned)
 */
gult ()
{
        gcall ("^ult");
        stkp = stkp + INTSIZE;
}

/*
 *      less than or equal (unsigned)
 */
gule ()
{
        gcall ("^ule");
        stkp = stkp + INTSIZE;
}

/*
 *      greater than (unsigned)
 */
gugt ()
{
        gcall ("^ugt");
        stkp = stkp + INTSIZE;
}

/*
 *      greater than or equal (unsigned)
 */
guge ()
{
        gcall ("^uge");
        stkp = stkp + INTSIZE;
}

/*
 *      Squirrel away argument count in a register that modstk/getloc/stloc
 *      doesn't touch.
 */
gnargs (d)
        int d;
{
        ot ("mov.l\t&");
        onum(d);
        outstr(",%d3\n");
}
