#include <stdio.h>
#include "defs.h"
#include "data.h"

/*
 * Some predefinitions:
 *
 * INTSIZE is the size of an integer in the target machine
 * BYTEOFF is the offset of an byte within an integer on the
 *         target machine. (ie: 8080,pdp11 = 0, 6809 = 1,
 *         360 = 3)
 * This compiler assumes that an integer is the SAME length as
 * a pointer - in fact, the compiler uses INTSIZE for both.
 */
#define BYTEOFF 0

/*
 * Print all assembler info before any code is generated.
 */
header()
{
    output_string ("#\tSmall C for MIPS32\n");
    output_string ("#\tRetroBSD Project\n");
    output_string ("#\n");
    output_string ("\t.set\tnoreorder\n");
    //output_line ("global\tTlneg");
    //output_line ("global\tTcase");
    //output_line ("global\tTeq");
    //output_line ("global\tTne");
    //output_line ("global\tTlt");
    //output_line ("global\tTle");
    //output_line ("global\tTgt");
    //output_line ("global\tTge");
    //output_line ("global\tTult");
    //output_line ("global\tTule");
    //output_line ("global\tTugt");
    //output_line ("global\tTuge");
    //output_line ("global\tTbool");
    //output_line ("global\tTmult");
    //output_line ("global\tTdiv");
    //output_line ("global\tTmod");
}

newline()
{
    output_byte ('\n');
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
 * Return size of an integer.
 */
intsize()
{
    return INTSIZE;
}

/*
 * Return offset of ls byte within word.
 * (ie: 8080 & pdp11 is 0, 6809 is 1, 360 is 3)
 */
byteoff()
{
    return BYTEOFF;
}

/*
 * Output internal generated label prefix.
 */
void output_label_prefix()
{
    output_string (".L");
}

/*
 * Output a label definition terminator.
 */
void output_label_terminator()
{
    output_string (":");
}

/*
 * Begin a comment line for the assembler.
 */
void gen_comment() {
    output_byte ('#');
}

/*
 * Print any assembler stuff needed after all code.
 */
trailer()
{
}

/*
 * Function prologue.
 */
fentry(int argtop)
{
    int i;

    /* Save register arguments to stack. */
    for (i=0; i<argtop; i+=4)
        fprintf(output, "\tsw\t$a%d, %d($sp)\n", i>>2, i);

    /* Allocate an empty call frame for 4 args.
     * Plus additional 4 bytes to save RA. */
    output_line("addiu\t$sp, -20");
    output_line("sw\t$ra, 16($sp)");
}

/*
 * Text (code) segment.
 */
code_segment_gtext()
{
    output_line (".text");
}

/*
 * Data segment.
 */
data_segment_gdata()
{
    output_line (".data");
}

char *inclib() {
#ifdef  cpm
        return("B:");
#endif
#ifdef  unix
#ifdef  INCDIR
        return(INCDIR);
#else
        return "";
#endif
#endif
}
/*
 * Output the variable symbol at scptr as an extrn or a public.
 */
void ppubext (symbol_t *scptr)
{
    if( scptr->storage == STATIC )
        return;
    output_string ("\t.globl\t");
    output_string (scptr);
    newline();
}

/*
 * Output the function symbol at scptr as an extrn or a public
 */
void fpubext (symbol_t *scptr)
{
    ppubext (scptr);
}

/*
 *  Output a decimal number to the assembler file.
 */
void output_number(int num)
{
    fprintf(output, "%d", num);
}

/*
 * Fetch a static memory cell into the primary register.
 */
void gen_get_memory(symbol_t *sym)
{
    output_string ("\tla\t$t0, ");
    output_string (sym->name);
    newline();
    if ((sym->identity != POINTER) & (sym->type & CCHAR)) {
		if(sym->type & UNSIGNED ) {
			output_line ("lbu\t$v0, 0($t0)");
		} else {
			output_line ("lb\t$v0, 0($t0)");
		}
    } else {
        output_line ("lw\t$v0, 0($t0)");
    }
}


/*
 * Fetch the address of the specified symbol into the primary register.
 */
int gen_get_location(symbol_t *sym)
{
    if( sym->storage == LSTATIC) {
        output_string ("\tla $v0, ");
        print_label(sym->offset);
        newline();
    } else {
	output_string("\taddiu\t$v0, $sp, ");
        output_number (sym->offset - stkp);
	newline();
    }
}

/*
 * Store the primary register into the specified static memory cell.
 */
void gen_put_memory(symbol_t *sym)
{
    output_string ("\tla\t$t0, ");
    output_string (sym->name);
    newline();
    if ((sym->identity != POINTER) & (sym->type & CCHAR)) {
        output_line ("sb\t$v0, 0($t0)");
    } else {
        output_line ("sw\t$v0, 0($t0)");
    }
}

/*
 * Store the specified object type in the primary register
 * at the address on the top of the stack.
 */
void gen_put_indirect(char typeobj)
{
    output_line ("lw\t$t1, 16($sp)");
    output_line ("addiu\t$sp, 4");
    if (typeobj & CCHAR)
        output_line ("sb\t$v0, 0($t1)");
    else
        output_line ("sw\t$v0, 0($t1)");
    stkp = stkp + INTSIZE;
}

/*
 * Fetch the specified object type indirect through the primary
 * register into the primary register.
 */
void gen_get_indirect(char typeobj, int reg)
{
    if (typeobj & CCHAR) {
		if( typeobj & UNSIGNED ) {
			output_line ("lbu\t$v0, 0($v0)");
		} else {
			output_line ("lb\t$v0, 0($v0)");
		}
	} else {
        output_line ("lw\t$v0, 0($v0)");
    }
}

/*
 * Swap the primary and secondary registers.
 */
gen_swap()
{
    output_line ("move\t$at, $v0\n\tmove\t$v0, $v1\n\tmove\t$v1, $at");
}

/*
 * Print partial instruction to get an immediate value into
 * the primary register.
 */
gen_immediate_a()
{
    output_string ("\tla\t$v0, ");
}

gen_immediate_c()
{
    output_string ("\tli\t$v0, ");
}

/*
 * Push the primary register onto the stack.
 */
gen_push()
{
    output_line ("addiu\t$sp, -4");
    output_line ("sw\t$v0, 16($sp)");
    stkp = stkp - INTSIZE;
}

/*
 * Pop the top of the stack into the secondary register.
 */
gen_pop()
{
    output_line ("lw\t$v1, 16($sp)");
    output_line ("addiu\t$sp, 4");
    stkp = stkp + INTSIZE;
}

/*
 * Swap the primary register and the top of the stack.
 */
gen_swap_stack()
{
    output_line ("move\t$t1, $v0");
    output_line ("lw\t$v0, 16($sp)");
    output_line ("sw\t$t1, 16($sp)");
}

/*
 * Call the specified subroutine name.
 */
gen_call (char * sname)
{
    output_string ("\tjal\t");
    if (*sname == '^') {
        output_string ("__sc_");
        sname++;
    }
    output_string (sname);
    newline();
    output_line ("nop");        /* fill delay slot */
}

/*
 * Return from subroutine.
 */
gen_ret()
{
    output_line("lw\t$ra, 16($sp)");
    output_line("jr\t$ra");
    output_line("addiu\t$sp, 20");
}

/*
 * Perform subroutine call to value on top of stack.
 */
callstk()
{
    output_line ("lw\t$t1, 16($sp)");
    output_line("jr\t$t1");
    output_line ("addiu\t$sp, 4");
    stkp = stkp + INTSIZE;
}

/*
 * Jump to specified internal label number.
 */
gen_jump (int label)
{
    output_string ("\tj\t");
    print_label (label);
    newline();
    output_line ("nop");
}

/*
 * Test the primary register and jump if false to label.
 */
gen_test_jump (int label, int ft)
{
    if (ft)
        output_string("\tbne\t$v0, $zero, ");
    else
        output_string("\tbeq\t$v0, $zero, ");
    print_label (label);
    newline();
    output_line("nop"); // fill delay slot
}

/*
 * Print pseudo-op to define a byte.
 */
gen_def_byte()
{
    output_string ("\t.byte\t");
}

/*
 * Print pseudo-op to define storage.
 */
gen_def_storage()
{
    output_string ("\t.space\t");
}

/*
 * Print pseudo-op to define a word.
 */
gen_def_word()
{
    output_string ("\t.word\t");
}

/*
 * Generate alignment to a word boundary.
 */
gen_align_word()
{
    output_string ("\t.align\t2\n");
}

/*
 * Modify the stack pointer to the new value indicated.
 */
gen_modify_stack (int newstkp)
{
    int k;

    k = newstkp - stkp;
    if (k % INTSIZE)
        error("Bad stack alignment (compiler error)");
    if (k == 0)
        return (newstkp);
    output_string ("\taddiu\t$sp, ");
    output_number (k);
    newline();
    return (newstkp);
}

/*
 * Multiply the primary register by INTSIZE.
 */
gen_multiply_by_two()
{
    output_line ("sll\t$v0, 2");
}

/*
 * Divide the primary register by INTSIZE.
 */
gen_divide_by_two()
{
    output_line ("sra\t$v0, 2");
}

/*
 * Case jump instruction.
 */
gen_jump_case()
{
    gen_call("^case");
}

/*
 * Add the primary and secondary registers.
 * If lval2 is int pointer and lval is int, scale lval.
 */
gen_add (int *lval, int *lval2)
{
    output_line ("lw\t$t1, 16($sp)");
    output_line ("addiu\t$sp, 4");
    if (dbltest (lval2, lval)) {
        output_line("sll\t$t1, 2");
    }
    output_line ("add\t$v0, $t1");
    stkp = stkp + INTSIZE;
}

/*
 * Subtract the primary register from the secondary. // *** from TOS
 */
gen_sub()
{
    output_line ("lw\t$t1, 16($sp)");
    output_line ("addiu\t$sp, 4");
    output_line ("sub\t$v0, $t1, $v0");
    stkp = stkp + INTSIZE;
}

/*
 * Multiply the primary and secondary registers.
 * (result in primary)
 */
gen_mult()
{
    output_line ("lw\t$t1, 16($sp)");
    output_line ("addiu\t$sp, 4");
    output_line ("mult\t$v0, $t1");
    output_line ("mflo\t$v0");
    //gcall ("^mult");
    stkp = stkp + INTSIZE;
}

/*
 * Divide the secondary register by the primary.
 * (quotient in primary, remainder in secondary)
 */
gen_div()
{
    output_line ("lw\t$t1, 16($sp)");
    output_line ("addiu\t$sp, 4");
    output_line ("div\t$t1, $v0");
    output_line ("mflo\t$v0");
    output_line ("mfhi\t$t1");
    //gcall ("^div");
    stkp = stkp + INTSIZE;
}

gen_udiv()
{
    output_line ("#FIXME genudiv");
    output_line ("lw\t$t1, 16($sp)");
    output_line ("addiu\t$sp, 4");
    output_line ("divu\t$t1, $v0");
    output_line ("mflo\t$v0");
    output_line ("mfhi\t$t1");
    //gcall ("^div");
    stkp = stkp + INTSIZE;
}

/*
 * Compute the remainder (mod) of the secondary register
 * divided by the primary register.
 * (remainder in primary, quotient in secondary)
 */
gen_mod()
{
    output_line ("lw\t$t1, 16($sp)");
    output_line ("addiu\t$sp, 4");
    output_line ("div\t$t1, $v0");
    output_line ("mflo\t$t1");
    output_line ("mfhi\t$v0");
    //gcall ("^mod");
    stkp = stkp + INTSIZE;
}

gen_umod()
{
    output_line ("#FIXME genumod");
    output_line ("lw\t$t1, 16($sp)");
    output_line ("addiu\t$sp, 4");
    output_line ("divu\t$t1, $v0");
    output_line ("mflo\t$t1");
    output_line ("mfhi\t$v0");
    //gcall ("^mod");
    stkp = stkp + INTSIZE;
}

/*
 * Inclusive 'or' the primary and secondary registers.
 */
gen_or()
{
    output_line ("lw\t$t1, 16($sp)");
    output_line ("addiu\t$sp, 4");
    output_line ("or\t$v0, $t1");
    //output_line ("or.l\t(%sp)+,%d0");
    stkp = stkp + INTSIZE;
}

/*
 * Exclusive 'or' the primary and secondary registers.
 */
gen_xor()
{
    output_line ("lw\t$t1, 16($sp)");
    output_line ("addiu\t$sp, 4");
    output_line ("xor\t$v0, $t1");
    //output_line ("mov.l\t(%sp)+,%d1");
    //output_line ("eor.l\t%d1,%d0");
    stkp = stkp + INTSIZE;
}

/*
 * 'And' the primary and secondary registers.
 */
gen_and()
{
    //output_line ("and.l\t(%sp)+,%d0");
    output_line ("lw\t$t1, 16($sp)");
    output_line ("addiu\t$sp, 4");
    output_line ("and\t$v0, $t1");
    stkp = stkp + INTSIZE;
}

/*
 * Arithmetic shift right the secondary register the number of
 * times in the primary register.
 * (results in primary register)
 */
gen_arithm_shift_right()
{
    output_line ("lw\t$t1, 16($sp)");
    output_line ("addiu\t$sp, 4");
    output_line ("srav\t$v0, $t1, $v0");
    stkp = stkp + INTSIZE;
}

/*
 * Arithmetic shift left the secondary register the number of
 * times in the primary register.
 * (results in primary register)
 */
gen_arithm_shift_left()
{
    output_line ("lw\t$t1, 16($sp)");
    output_line ("addiu\t$sp, 4");
    output_line ("sllv\t$v0, $t1, $v0");
    stkp = stkp + INTSIZE;
}

/*
 * Two's complement of primary register.
 */
gen_twos_complement()
{
    output_line ("sub\t$v0, $zero, $v0");
}

/*
 * Logical complement of primary register.
 */
gen_logical_negation()
{
    //gcall ("^lneg");
    output_line ("sltu\t$t1, $v0, $zero");
    output_line ("sltu\t$t2, $zero, $v0");
    output_line ("or\t$v0, $t1, $t2");
    output_line ("xori\t$v0, 1");
}

/*
 * One's complement of primary register.
 */
gen_complement()
{
    output_line ("addiu\t$t1, $zero, -1");
    output_line ("xor\t$v0, $t1");
}

/*
 * Convert primary register into logical value.
 */
gen_convert_primary_reg_value_to_bool()
{
    output_line ("sltu\t$t1, $v0, $zero");
    output_line ("sltu\t$t2, $zero, $v0");
    output_line ("or\t$v0, $t1, $t2");
    //gcall ("^bool");
}

/*
 * Increment the primary register by 1 if char, INTSIZE if int.
 */
gen_increment_primary_reg (lvalue_t *lval)
{
    if (lval->ptr_type & CINT)
	output_line("addiu\t$v0, 4");
    else
	output_line("addiu\t$v0, 1");
}

/*
 * Decrement the primary register by one if char, INTSIZE if int.
 */
gen_decrement_primary_reg (lvalue_t *lval)
{
    if (lval->ptr_type & CINT)
	output_line("addiu\t$v0, -4");
    else
	output_line("addiu\t$v0, -1");
}

/*
 * Following are the conditional operators.
 * They compare the secondary register against the primary register
 * and put a literl 1 in the primary if the condition is true,
 * otherwise they clear the primary register.
 */
///// BEEP BEEP actually, compare tos
/*
 * equal
 */
gen_equal()
{
    output_line("lw\t$t1, 16($sp)");
    output_line("sltu\t$t2, $v0, $t1");
    output_line("sltu\t$v0, $t1, $v0");
    output_line("or\t$v0, $t2");
    output_line("xori\t$v0, 1");
    output_line("addiu\t$sp, 4");
    //gcall ("^eq");
    stkp = stkp + INTSIZE;
}

/*
 * not equal
 */
gen_not_equal()
{
    output_line("lw\t$t1, 16($sp)");
    output_line("sltu\t$t2, $v0, $t1");
    output_line("sltu\t$v0, $t1, $v0");
    output_line("or\t$v0, $t2");
    output_line("addiu\t$sp, 4");
    //gcall ("^ne");
    stkp = stkp + INTSIZE;
}

/*
 * less than (signed) - TOS < primary
 */
gen_less_than()
{
    output_line("lw\t$t1, 16($sp)");
    output_line("addiu\t$sp, 4");
    output_line("slt\t$v0, $t1, $v0");
    //gcall ("^lt");
    stkp = stkp + INTSIZE;
}

/*
 * less than or equal (signed) TOS <= primary
 */
gen_less_or_equal()
{
    output_line("lw\t$t1, 16($sp)");
    output_line("addiu\t$sp, 4");
    output_line("slt\t$v0, $v0, $t1"); // primary < tos
    output_line("xori\t$v0, 1");  // primary >= tos
    //gcall ("^le");
    stkp = stkp + INTSIZE;
}

/*
 * greater than (signed) TOS > primary
 */
gen_greater_than()
{
    output_line("lw\t$t1, 16($sp)");
    output_line("addiu\t$sp, 4");
    output_line("slt\t$v0, $v0, $t1");   //pimary < TOS
    //output_line("xori\t$v0, 1");
    //gcall ("^gt");
    stkp = stkp + INTSIZE;
}

/*
 * greater than or equal (signed) TOS >= primary
 */
gen_greater_or_equal()
{
    output_line("lw\t$t1, 16($sp)");
    output_line("addiu\t$sp, 4");
    output_line("slt\t$v0, $t1, $v0");   //tos < primary
    output_line("xori\t$v0, 1");    //tos >= primary
    //gcall ("^ge");
    stkp = stkp + INTSIZE;
}

/*
 * less than (unsigned)
 */
gen_unsigned_less_than()
{
    output_line("lw\t$t1, 16($sp)");
    output_line("addiu\t$sp, 4");
    output_line("sltu\t$v0, $t1, $v0");
    //gcall ("^ult");
    stkp = stkp + INTSIZE;
}

/*
 * less than or equal (unsigned)
 */
gen_unsigned_less_or_equal()
{
    output_line("lw\t$t1, 16($sp)");
    output_line("addiu\t$sp, 4");
    output_line("sltu\t$v0, $v0, $t1"); // primary < tos
    output_line("xori\t$v0, 1");  // primary >= tos
    //gcall ("^ule");
    stkp = stkp + INTSIZE;
}

/*
 * greater than (unsigned)
 */
gen_usigned_greater_than()
{
    output_line("lw\t$t1, 16($sp)");
    output_line("addiu\t$sp, 4");
    output_line("sltu\t$v0, $v0, $t1");   //pimary < TOS
    //gcall ("^ugt");
    stkp = stkp + INTSIZE;
}

/*
 * greater than or equal (unsigned)
 */
gen_unsigned_greater_or_equal()
{
    output_line("lw\t$t1, 16($sp)");
    output_line("addiu\t$sp, 4");
    output_line("sltu\t$v0, $t1, $v0");   //tos < primary
    output_line("xori\t$v0, 1");    //tos >= primary
    //gcall ("^uge");
    stkp = stkp + INTSIZE;
}

/*
 * Put first 4 arguments to registers a0-a3.
 */
gnargs (nargs)
    int nargs;
{
    int i;

    if (nargs > 4) {
        error("Too many arguments in a function call (max 4 args supported)");
        nargs = 4;
    }
    for (i=0; i<nargs; i++)
        fprintf(output, "\tlw\t$a%d, %d($sp)\n", i, (nargs-1 - i) * 4 + 16);
}
