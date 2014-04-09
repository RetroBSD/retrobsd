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
#define BYTEOFF 0

/*
 *      print all assembler info before any code is generated
 */
header ()
{
        outstr("#\tSmall C VAX\n#\tCoder (2.4,84/11/27)\n#");
        FEvers();
        nl ();
        ol (".globl\tlneg");
        ol (".globl\tcase");
        ol (".globl\teq");
        ol (".globl\tne");
        ol (".globl\tlt");
        ol (".globl\tle");
        ol (".globl\tgt");
        ol (".globl\tge");
        ol (".globl\tult");
        ol (".globl\tule");
        ol (".globl\tugt");
        ol (".globl\tuge");
        ol (".globl\tbool");
}

nl()
{
        if (needh) {
                ol (".word\t0");
                needh = 0;
        }
        if (needr0) {
                needr0 = 0;
                outstr(",r0");
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
        outbyte ('_');
}

/*
 *      print any assembler stuff needed after all code
 */
trailer ()
{
}

/*
 *      function prologue
 */
prologue ()
{
        ol (".align\t1");
}

/*
 *      text (code) segment
 */
gtext ()
{
        ol (".text");
}

/*
 *      data segment
 */
gdata ()
{
        ol (".data");
}

/*
 *  Output the variable symbol at scptr as an extrn or a public
 */
ppubext(scptr)
        char *scptr;
{
        if (scptr[STORAGE] == STATIC)
                return;
        ot (".globl\t");
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
        if ((sym[IDENT] != POINTER) & (sym[TYPE] == CCHAR)) {
                ot ("cvtbl\t");
                prefix ();
                outstr (sym + NAME);
        } else {
                ot ("movl\t");
                prefix ();
                outstr (sym + NAME);
        }
        outstr(",r0\n");
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
                ot ("moval\t");
                onum (glint(sym) - stkp);
                outstr ("(sp),r0\n");
        }
}

/*
 *      store the primary register into the specified static memory cell
 */
putmem (sym)
        char    *sym;
{
        if ((sym[IDENT] != POINTER) & (sym[TYPE] == CCHAR)) {
                ot ("cvtlb\tr0,");
        } else
                ot ("movl\tr0,");
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
        if (typeobj == CCHAR)
                ol ("cvtlb\tr0,*(sp)+");
        else
                ol ("movl\tr0,*(sp)+");
        stkp = stkp + INTSIZE;
}

/*
 *      fetch the specified object type indirect through the primary
 *      register into the primary register
 */
indirect (typeobj)
        char    typeobj;
{
        if (typeobj == CCHAR)
                ol ("cvtbl\t(r0),r0");
        else
                ol ("movl\t(r0),r0");
}

/*
 *      swap the primary and secondary registers
 */
swap ()
{
        ol ("movl\tr0,r2\n\tmovl\tr1,r0\n\tmovl\tr2,r1");
}

/*
 *      print partial instruction to get an immediate value into
 *      the primary register
 */
immed ()
{
        ot ("movl\t$");
        needr0 = 1;
}

/*
 *      push the primary register onto the stack
 */
gpush ()
{
        ol ("pushl\tr0");
        stkp = stkp - INTSIZE;
}

/*
 *      pop the top of the stack into the secondary register
 */
gpop ()
{
        ol ("movl\t(sp)+,r1");
        stkp = stkp + INTSIZE;
}

/*
 *      swap the primary register and the top of the stack
 */
swapstk ()
{
        ol ("popl\tr2\npushl\tr0\nmovl\tr2,r0");
}

/*
 *      call the specified subroutine name
 */
gcall (sname)
        char    *sname;
{
        if (*sname == '^') {
                ot ("jsb\t");
                outstr (++sname);
        } else {
                ot ("jsb\t");
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
        ol ("rsb");
}

/*
 *      perform subroutine call to value on top of stack
 */
callstk ()
{
        ol ("jsb\t(sp)+");
        stkp = stkp + INTSIZE;
}

/*
 *      jump to specified internal label number
 *
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
 */
testjump (label, ft)
        int label;
        int ft;
{
        ol ("cmpl\tr0,$0");
        if (ft)
                ot ("jneq\t");
        else
                ot ("jeql\t");
        printlabel (label);
        nl ();
}

/*
 *      print pseudo-op  to define a byte
 */
defbyte ()
{
        ot (".byte\t");
}

/*
 *      print pseudo-op to define storage
 */
defstorage ()
{
        ot (".space\t");
}

/*
 *      print pseudo-op to define a word
 */
defword ()
{
        ot (".long\t");
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
        ot ("addl2\t$");
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
        ol ("ashl\t$2,r0,r0");
}

/*
 *      divide the primary register by INTSIZE
 */
gasrint()
{
        ol ("ashl\t$-2,r0,r0");
}

/*
 *      Case jump instruction
 */
gjcase()
{
        ot ("jmp\tcase");
        nl ();
}

/*
 *      add the primary and secondary registers
 *      if lval2 is int pointer and lval is int, scale lval
 */
gadd (lval, lval2)
        int *lval, *lval2;
{
        if (dbltest (lval2, lval)) {
                ol ("ashl\t$2,(sp),(sp)");
        }
        ol ("addl2\t(sp)+,r0");
        stkp = stkp + INTSIZE;
}

/*
 *      subtract the primary register from the secondary
 */
gsub ()
{
        ol ("subl3\tr0,(sp)+,r0");
        stkp = stkp + INTSIZE;
}

/*
 *      multiply the primary and secondary registers
 *      (result in primary)
 */
gmult ()
{
        ol ("mull2\t(sp)+,r0");
        stkp = stkp + INTSIZE;
}

/*
 *      divide the secondary register by the primary
 *      (quotient in primary, remainder in secondary)
 */
gdiv ()
{
        ol ("divl3\tr0,(sp)+,r0");
        stkp = stkp + INTSIZE;
}

/*
 *      compute the remainder (mod) of the secondary register
 *      divided by the primary register
 *      (remainder in primary, quotient in secondary)
 */
gmod ()
{
        ol ("movl\t(sp)+,r2\n\tmovl\t$0,r3\nediv\tr0,r2,r1,r0");
        stkp = stkp + INTSIZE;
}

/*
 *      inclusive 'or' the primary and secondary registers
 */
gor ()
{
        ol ("bisl2\t(sp)+,r0");
        stkp = stkp + INTSIZE;
}

/*
 *      exclusive 'or' the primary and secondary registers
 */
gxor ()
{
        ol ("xorl2\t(sp)+,r0");
        stkp = stkp + INTSIZE;
}

/*
 *      'and' the primary and secondary registers
 */
gand ()
{
        ol ("mcoml\t(sp)+,r1\n\tbicl2\tr1,r0");
        stkp = stkp + INTSIZE;
}

/*
 *      arithmetic shift right the secondary register the number of
 *      times in the primary register
 *      (results in primary register)
 */
gasr ()
{
        ol("mnegl\tr0,r0\n\tashl\tr0,(sp)+,r0");
        stkp = stkp + INTSIZE;
}

/*
 *      arithmetic shift left the secondary register the number of
 *      times in the primary register
 *      (results in primary register)
 */
gasl ()
{
        ol ("ashl\tr0,(sp)+,r0");
        stkp = stkp + INTSIZE;
}

/*
 *      two's complement of primary register
 */
gneg ()
{
        ol ("mnegl\tr0,r0");
}

/*
 *      logical complement of primary register
 *
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
        ol ("mcoml\tr0,r0");
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
                ol ("addl2\t$4,r0");
        else
                ol ("incl\tr0");
}

/*
 *      decrement the primary register by one if char, INTSIZE if int
 */
gdec (lval)
        int lval[];
{
        if (lval[2] == CINT)
                ol ("subl2\t$4,r0");
        else
                ol ("decl\tr0");
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
 *
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
 *
 */
guge ()
{
        gcall ("^uge");
        stkp = stkp + INTSIZE;
}

/*
 *      Squirrel away argument count in a register that modstk
 *      doesn't touch.
 */
gnargs(d)
        int d;
{
        ot ("movl\t$");
        onum(d);
        outstr (",r6\n");
}
