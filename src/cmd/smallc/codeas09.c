#include <stdio.h>
#include "defs.h"
#include "data.h"

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
#define INTSIZE 2
#define BYTEOFF 1

/*
 *      print all assembler info before any code is generated
 */
header ()
{
        outstr("|\tSmall C MC6809\n|\tCoder (2.4,84/11/27)\n|");
        FEvers();
        nl ();
        ol (".globl\tsmul,sdiv,smod,asr,asl,neg,lneg,case");
        ol (".globl\teq,ne,lt,le,gt,ge,ult,ule,ugt,uge,bool");
}

nl ()
{
        outbyte (EOL);
}

galign(t)
        int t;
{
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
        outstr ("=.\n");
}

/*
 *      begin a comment line for the assembler
 */
comment ()
{
        outbyte ('|');
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
        ol (".end");
}

/*
 *      function prologue
 */
prologue ()
{
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
        outbyte('.');
}

/*
 *      fetch a static memory cell into the primary register
 */
getmem (sym)
        char    *sym;
{
        if ((sym[IDENT] != POINTER) & (sym[TYPE] == CCHAR)) {
                ot ("ldb\t");
                prefix ();
                outstr (sym + NAME);
                nl ();
                ot ("sex");
                nl ();
        } else {
                ot ("ldd\t");
                prefix ();
                outstr (sym + NAME);
                nl ();
        }
}

/*
 *      fetch the address of the specified symbol into the primary register
 *
 */
getloc (sym)
        char    *sym;
{
        if (sym[STORAGE] == LSTATIC) {
                immed();
                printlabel(glint(sym));
                nl();
        } else {
                ot ("leay\t");
                onum (glint(sym) - stkp);
                outstr ("(s)\n\ttfr\ty,d\n");
        }
}

/*
 *      store the primary register into the specified static memory cell
 */
putmem (sym)
        char    *sym;
{
        if ((sym[IDENT] != POINTER) & (sym[TYPE] == CCHAR)) {
                ot ("stb\t");
        } else
                ot ("std\t");
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
                ol ("stb\t@(s)++");
        else
                ol ("std\t@(s)++");
        stkp = stkp + INTSIZE;
}

/*
 *      fetch the specified object type indirect through the primary
 *      register into the primary register
 */
indirect (typeobj)
        char    typeobj;
{
        ol("tfr\td,y");
        if (typeobj == CCHAR)
                ol ("ldb\t(y)\n\tsex");
        else
                ol ("ldd\t(y)");
}

/*
 *      swap the primary and secondary registers
 */
swap ()
{
        ol ("exg\td,x");
}

/*
 *      print partial instruction to get an immediate value into
 *      the primary register
 */
immed ()
{
        ot ("ldd\t#");
}

/*
 *      push the primary register onto the stack
 */
gpush ()
{
        ol ("pshs\td");
        stkp = stkp - INTSIZE;
}

/*
 *      pop the top of the stack into the secondary register
 */
gpop ()
{
        ol ("puls\td");
        stkp = stkp + INTSIZE;
}

/*
 *      swap the primary register and the top of the stack
 */
swapstk ()
{
        ol ("ldy\t(s)\nstd\t(s)\n\ttfr\ty,d");
}

/*
 *      call the specified subroutine name
 */
gcall (sname)
        char    *sname;
{
        ot ("jsr\t");
        if (*sname == '^')
                outstr (++sname);
        else {
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
        gpop();
        ol ("jsr\t(x)");
}

/*
 *      jump to specified internal label number
 */
jump (label)
        int     label;
{
        ot ("lbra\t");
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
        ol ("cmpd\t#0");
        if (ft)
                ot ("lbne\t");
        else
                ot ("lbeq\t");
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
        ot (".blkb\t");
}

/*
 *      print pseudo-op to define a word
 */
defword ()
{
        ot (".word\t");
}

/*
 *      modify the stack pointer to the new value indicated
 */
modstk (newstkp)
        int     newstkp;
{
        int     k;

        k = galign(newstkp - stkp);
        if (k == 0)
                return (newstkp);
        ot ("leas\t");
        onum (k);
        outstr ("(s)\n");
        return (newstkp);
}

/*
 *      multiply the primary register by INTSIZE
 */
gaslint ()
{
        ol ("aslb\n\trola");
}

/*
 *      divide the primary register by INTSIZE
 */
gasrint()
{
        ol ("asra\n\trorb");
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
                ol ("asl\t1(s)\n\trol\t(s)");
        }
        ol ("addd\t(s)++");
        stkp = stkp + INTSIZE;
}

/*
 *      subtract the primary register from the secondary
 */
gsub ()
{
        ol ("subd\t(s)++\n\tcoma\n\tcomb\n\taddd\t#1");
        stkp = stkp + INTSIZE;
}

/*
 *      multiply the primary and secondary registers
 *      (result in primary)
 */
gmult ()
{
        gcall ("^smul");
        stkp = stkp + INTSIZE;
}

/*
 *      divide the secondary register by the primary
 *      (quotient in primary, remainder in secondary)
 */
gdiv ()
{
        gcall ("^sdiv");
        stkp = stkp + INTSIZE;
}

/*
 *      compute the remainder (mod) of the secondary register
 *      divided by the primary register
 *      (remainder in primary, quotient in secondary)
 */
gmod ()
{
        gcall ("^smod");
        stkp = stkp + INTSIZE;
}

/*
 *      inclusive 'or' the primary and secondary registers
 */
gor ()
{
        ol ("ora\t(s)+\n\torb\t(s)+");
        stkp = stkp + INTSIZE;
}

/*
 *      exclusive 'or' the primary and secondary registers
 */
gxor ()
{
        ol ("eora\t(s)+\n\teorb\t(s)+");
        stkp = stkp + INTSIZE;
}

/*
 *      'and' the primary and secondary registers
 */
gand ()
{
        ol ("anda\t(s)+\n\tandb\t(s)+");
        stkp = stkp + INTSIZE;
}

/*
 *      arithmetic shift right the secondary register the number of
 *      times in the primary register
 *      (results in primary register)
 */
gasr ()
{
        gcall ("^asr");
        stkp = stkp + INTSIZE;
}

/*
 *      arithmetic shift left the secondary register the number of
 *      times in the primary register
 *      (results in primary register)
 */
gasl ()
{
        gcall ("^asl");
        stkp = stkp + INTSIZE;
}

/*
 *      two's complement of primary register
 */
gneg ()
{
        gcall ("^neg");
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
        ol ("coma\n\tcomb");
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
                ol ("addd\t#2");
        else
                ol ("addd\t#1");
}

/*
 *      decrement the primary register by one if char, INTSIZE if int
 */
gdec (lval)
        int lval[];
{
        if (lval[2] == CINT)
                ol ("subd\t#2");
        else
                ol ("subd\t#1");
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
        ot ("ldu\t#");
        onum(d);
        nl ();
}
