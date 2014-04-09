/*
 * File expr.c: 2.2 (83/06/21,11:24:26)
 */

#include <stdio.h>
#include "defs.h"
#include "data.h"

//struct lvalue {
//	symbol_t *symbol ;		// symbol table address, or 0 for constant
//	int indirect ;			// type of indirect object, 0 for static object
//	int ptr_type ;			// type of pointer or array, 0 for other idents
//	int is_const ;			// true if constant expression
//	int const_val ;			// value of constant expression (& other uses)
//	TAG_SYMBOL *tagsym ;            // tag symbol address, 0 if not struct
//	int (*binop)() ;		// function address of highest/last binary operator
//	char *stage_add ;		// stage addess of "oper 0" code, else 0
//	int val_type ;			// type of value calculated
//} ;

//#define LVALUE struct lvalue

/**
 * unsigned operand ?
 */
nosign(lvalue_t *is) {
    symbol_t *ptr;

    if((is->ptr_type) ||
      ((ptr = is->symbol) && (ptr->type & UNSIGNED))) {
        return 1;
    }
    return 0;
}

/**
 * lval[0] - symbol table address, else 0 for constant
 * lval[1] - type indirect object to fetch, else 0 for static object
 * lval[2] - type pointer or array, else 0
 * @param comma
 * @return
 */
expression(int comma) {
        lvalue_t lval;
        int k;

        do {
                if (k = hier1 (&lval))
                        rvalue(&lval, k);
                if (!comma)
                        return;
        } while (match (","));
}

/**
 * assignment operators
 * @param lval
 * @return
 */
hier1 (lvalue_t *lval) {
        int     k;
        lvalue_t lval2[1];
        char    fc;

        k = hier1a (lval);
        if (match ("=")) {
                if (k == 0) {
                        needlval ();
                        return (0);
                }
                if (lval->indirect)
                        gen_push(k);
                if (k = hier1 (lval2))
                        k = rvalue(lval2, k);
                store (lval);
                return (0);
        } else {
                fc = ch();
                if  (match ("-=") ||
                    match ("+=") ||
                    match ("*=") ||
                    match ("/=") ||
                    match ("%=") ||
                    match (">>=") ||
                    match ("<<=") ||
                    match ("&=") ||
                    match ("^=") ||
                    match ("|=")) {
                        if (k == 0) {
                                needlval ();
                                return (0);
                        }
                        if (lval->indirect)
                                gen_push(k);
                        k = rvalue(lval, k);
                        gen_push(k);
                        if (k = hier1 (lval2))
                                k = rvalue(lval2, k);
                        switch (fc) {
                                case '-':       {
                                        if (dbltest(lval,lval2))
                                                gen_multiply_by_two();
                                        gen_sub();
                                        result (lval, lval2);
                                        break;
                                }
                                case '+':       {
                                        if (dbltest(lval,lval2))
                                                gen_multiply_by_two();
                                        gen_add (lval,lval2);
                                        result(lval,lval2);
                                        break;
                                }
                                case '*':       gen_mult (); break;
                                case '/':
                                    if(nosign(lval) || nosign(lval2)) {
                                        gen_udiv();
                                    } else {
                                        gen_div();
                                    }
                                    break;
                                case '%':
                                    if(nosign(lval) || nosign(lval2)) {
                                        gen_umod();
                                    } else {
                                        gen_mod();
                                    }
                                    break;
                                case '>': gen_arithm_shift_right (); break;
                                case '<': gen_arithm_shift_left(); break;
                                case '&': gen_and (); break;
                                case '^': gen_xor (); break;
                                case '|': gen_or (); break;
                        }
                        store (lval);
                        return (0);
                } else
                        return (k);
        }
}

/**
 * processes ? : expression
 * @param lval
 * @return 0 or 1, fetch or no fetch
 */
hier1a (lvalue_t *lval) {
        int     k, lab1, lab2;
        lvalue_t lval2[1];

        k = hier1b (lval);
        blanks ();
        if (ch () != '?')
                return (k);
        if (k)
                k = rvalue(lval, k);
        for (;;)
                if (match ("?")) {
                        gen_test_jump (lab1 = getlabel (), FALSE);
                        if (k = hier1b (lval2))
                                k = rvalue(lval2, k);
                        gen_jump (lab2 = getlabel ());
                        print_label (lab1);
                        output_label_terminator ();
                        newline ();
                        blanks ();
                        if (!match (":")) {
                                error ("missing colon");
                                return (0);
                        }
                        if (k = hier1b (lval2))
                                k = rvalue(lval2, k);
                        print_label (lab2);
                        output_label_terminator ();
                        newline ();
                } else
                        return (0);
}

/**
 * processes logical or ||
 * @param lval
 * @return 0 or 1, fetch or no fetch
 */
hier1b (lvalue_t *lval) {
        int     k, lab;
        lvalue_t lval2[1];

        k = hier1c (lval);
        blanks ();
        if (!sstreq ("||"))
                return (k);
        if (k)
                k = rvalue(lval, k);
        for (;;)
                if (match ("||")) {
                        gen_test_jump (lab = getlabel (), TRUE);
                        if (k = hier1c (lval2))
                                k = rvalue(lval2, k);
                        print_label (lab);
                        output_label_terminator ();
                        newline ();
                        gen_convert_primary_reg_value_to_bool();
                } else
                        return (0);
}

/**
 * processes logical and &&
 * @param lval
 * @return 0 or 1, fetch or no fetch
 */
hier1c (lvalue_t *lval) {
        int     k, lab;
        lvalue_t lval2[1];

        k = hier2 (lval);
        blanks ();
        if (!sstreq ("&&"))
                return (k);
        if (k)
                k = rvalue(lval, k);
        for (;;)
                if (match ("&&")) {
                        gen_test_jump (lab = getlabel (), FALSE);
                        if (k = hier2 (lval2))
                                k = rvalue(lval2, k);
                        print_label (lab);
                        output_label_terminator ();
                        newline ();
                        gen_convert_primary_reg_value_to_bool();
                } else
                        return (0);
}

/**
 * processes bitwise or |
 * @param lval
 * @return 0 or 1, fetch or no fetch
 */
hier2 (lvalue_t *lval) {
        int     k;
        lvalue_t lval2[1];

        k = hier3 (lval);
        blanks ();
        if ((ch() != '|') | (nch() == '|') | (nch() == '='))
                return (k);
        if (k)
                k = rvalue(lval, k);
        for (;;) {
                if ((ch() == '|') & (nch() != '|') & (nch() != '=')) {
                        inbyte ();
                        gen_push(k);
                        if (k = hier3 (lval2))
                                k = rvalue(lval2, k);
                        gen_or ();
                        blanks();
                } else
                        return (0);
        }
}

/**
 * processes bitwise exclusive or
 * @param lval
 * @return 0 or 1, fetch or no fetch
 */
hier3 (lvalue_t *lval) {
        int     k;
        lvalue_t lval2[1];

        k = hier4 (lval);
        blanks ();
        if ((ch () != '^') | (nch() == '='))
                return (k);
        if (k)
                k = rvalue(lval, k);
        for (;;) {
                if ((ch() == '^') & (nch() != '=')){
                        inbyte ();
                        gen_push(k);
                        if (k = hier4 (lval2))
                                k = rvalue(lval2, k);
                        gen_xor ();
                        blanks();
                } else
                        return (0);
        }
}

/**
 * processes bitwise and &
 * @param lval
 * @return 0 or 1, fetch or no fetch
 */
hier4 (lvalue_t *lval) {
        int     k;
        lvalue_t lval2[1];

        k = hier5 (lval);
        blanks ();
        if ((ch() != '&') | (nch() == '|') | (nch() == '='))
                return (k);
        if (k)
                k = rvalue(lval, k);
        for (;;) {
                if ((ch() == '&') & (nch() != '&') & (nch() != '=')) {
                        inbyte ();
                        gen_push(k);
                        if (k = hier5 (lval2))
                                k = rvalue(lval2, k);
                        gen_and ();
                        blanks();
                } else
                        return (0);
        }

}

/**
 * processes equal and not equal operators
 * @param lval
 * @return 0 or 1, fetch or no fetch
 */
hier5 (lvalue_t *lval) {
        int     k;
        lvalue_t lval2[1];

        k = hier6 (lval);
        blanks ();
        if (!sstreq ("==") &
            !sstreq ("!="))
                return (k);
        if (k)
                k = rvalue(lval, k);
        for (;;) {
                if (match ("==")) {
                        gen_push(k);
                        if (k = hier6 (lval2))
                                k = rvalue(lval2, k);
                        gen_equal ();
                } else if (match ("!=")) {
                        gen_push(k);
                        if (k = hier6 (lval2))
                                k = rvalue(lval2, k);
                        gen_not_equal ();
                } else
                        return (0);
        }

}

/**
 * comparison operators
 * @param lval
 * @return 0 or 1, fetch or no fetch
 */
hier6 (lvalue_t *lval) {
        int     k;
        lvalue_t lval2[1];

        k = hier7 (lval);
        blanks ();
        if (!sstreq ("<") &&
            !sstreq ("<=") &&
            !sstreq (">=") &&
            !sstreq (">"))
                return (k);
        if (sstreq ("<<") || sstreq (">>"))
                return (k);
        if (k)
                k = rvalue(lval, k);
        for (;;) {
                if (match ("<=")) {
                        gen_push(k);
                        if (k = hier7 (lval2))
                                k = rvalue(lval2, k);
                        if (nosign(lval) || nosign(lval2)) {
                                gen_unsigned_less_or_equal ();
                                continue;
                        }
                        gen_less_or_equal ();
                } else if (match (">=")) {
                        gen_push(k);
                        if (k = hier7 (lval2))
                                k = rvalue(lval2, k);
                        if (nosign(lval) || nosign(lval2)) {
                                gen_unsigned_greater_or_equal ();
                                continue;
                        }
                        gen_greater_or_equal();
                } else if ((sstreq ("<")) &&
                           !sstreq ("<<")) {
                        inbyte ();
                        gen_push(k);
                        if (k = hier7 (lval2))
                                k = rvalue(lval2, k);
                        if (nosign(lval) || nosign(lval2)) {
                                gen_unsigned_less_than ();
                                continue;
                        }
                        gen_less_than ();
                } else if ((sstreq (">")) &&
                           !sstreq (">>")) {
                        inbyte ();
                        gen_push(k);
                        if (k = hier7 (lval2))
                                k = rvalue(lval2, k);
                        if (nosign(lval) || nosign(lval2)) {
                                gen_usigned_greater_than ();
                                continue;
                        }
                        gen_greater_than();
                } else
                        return (0);
                blanks ();
        }

}

/**
 * bitwise left, right shift
 * @param lval
 * @return 0 or 1, fetch or no fetch
 */
hier7 (lvalue_t *lval) {
        int     k;
        lvalue_t lval2[1];

        k = hier8 (lval);
        blanks ();
        if (!sstreq (">>") &&
            !sstreq ("<<") || sstreq(">>=") || sstreq("<<="))
                return (k);
        if (k)
                k = rvalue(lval, k);
        for (;;) {
                if (sstreq(">>") && ! sstreq(">>=")) {
                        inbyte(); inbyte();
                        gen_push(k);
                        if (k = hier8 (lval2))
                                k = rvalue(lval2, k);
                        gen_arithm_shift_right ();
                } else if (sstreq("<<") && ! sstreq("<<=")) {
                        inbyte(); inbyte();
                        gen_push(k);
                        if (k = hier8 (lval2))
                                k = rvalue(lval2, k);
                        gen_arithm_shift_left();
                } else
                        return (0);
                blanks();
        }

}

/**
 * addition, subtraction
 * @param lval
 * @return 0 or 1, fetch or no fetch
 */
hier8 (lvalue_t *lval) {
        int     k;
        lvalue_t lval2[1];

        k = hier9 (lval);
        blanks ();
        if ((ch () != '+') & (ch () != '-') | nch() == '=')
                return (k);
        if (k)
                k = rvalue(lval, k);
        for (;;) {
                if (match ("+")) {
                        gen_push(k);
                        if (k = hier9 (lval2))
                                k = rvalue(lval2, k);
                        /* if left is pointer and right is int, scale right */
                        if (dbltest (lval, lval2))
                                gen_multiply_by_two ();
                        /* will scale left if right int pointer and left int */
                        gen_add (lval,lval2);
                        result (lval, lval2);
                } else if (match ("-")) {
                        gen_push(k);
                        if (k = hier9 (lval2))
                                k = rvalue(lval2, k);
                        /* if dbl, can only be: pointer - int, or
                                                pointer - pointer, thus,
                                in first case, int is scaled up,
                                in second, result is scaled down. */
                        if (dbltest (lval, lval2))
                                gen_multiply_by_two ();
                        gen_sub ();
                        /* if both pointers, scale result */

                        /* the second major condition was added to fix &a[n]-&a[n] where a is an int array
                         * this was done be inspection and may cause other conditions to fail
                         * more testing is needed.
                         * Really, there are multiple problems when taking addresses of arrays
                         * This needs a lot more work, but it seems to fix the specific problem
                         * and does not introduce any bugs that I can assign to it */
                        if (((lval->ptr_type & CINT) && (lval2->ptr_type & CINT))
                            ||((!lval->symbol)&&(!lval2->symbol)&&(lval->ptr_type&CCHAR)&&(lval2->ptr_type&CCHAR)&&(lval->indirect & CINT) && (lval2->indirect & CINT))) {
                                gen_divide_by_two(); /* divide by intsize */
                        }
                        result (lval, lval2);
                } else
                        return (0);
        }
}

/**
 * multiplication, division, modulus
 * @param lval
 * @return 0 or 1, fetch or no fetch
 */
hier9 (lvalue_t *lval) {
        int     k;
        lvalue_t lval2[1];

        k = hier10 (lval);
        blanks ();
        if (((ch () != '*') && (ch () != '/') &&
                (ch () != '%')) || (nch() == '='))
                return (k);
        if (k)
                k = rvalue(lval, k);
        for (;;) {
                if (match ("*")) {
                        gen_push(k);
                        if (k = hier10 (lval2))
                                k = rvalue(lval2, k);
                        gen_mult ();
                } else if (match ("/")) {
                        gen_push(k);
                        if (k = hier10 (lval2))
                                k = rvalue(lval2, k);
                        if(nosign(lval) || nosign(lval2)) {
                            gen_udiv();
                        } else {
                            gen_div ();
                        }
                } else if (match ("%")) {
                        gen_push(k);
                        if (k = hier10 (lval2))
                                k = rvalue(lval2, k);
                        if(nosign(lval) || nosign(lval2)) {
                            gen_umod();
                        } else {
                            gen_mod ();
                        }
                } else
                        return (0);
        }

}

/**
 * increment, decrement, negation operators
 * @param lval
 * @return 0 or 1, fetch or no fetch
 */
hier10 (lvalue_t *lval) {
        int     k;
        symbol_t *ptr;

        if (match ("++")) {
                if ((k = hier10 (lval)) == 0) {
                        needlval ();
                        return (0);
                }
                if (lval->indirect)
                        gen_push(k);
                k = rvalue(lval, k);
                gen_increment_primary_reg (lval);
                store (lval);
                return (0);
        } else if (match ("--")) {
                if ((k = hier10 (lval)) == 0) {
                        needlval ();
                        return (0);
                }
                if (lval->indirect)
                        gen_push(k);
                k = rvalue(lval, k);
                gen_decrement_primary_reg (lval);
                store (lval);
                return (0);
        } else if (match ("-")) {
                k = hier10 (lval);
                if (k)
                        k = rvalue(lval, k);
                gen_twos_complement();
                return (0);
        } else if (match ("~")) {
                k = hier10 (lval);
                if (k)
                        k = rvalue(lval, k);
                gen_complement ();
                return (0);
        } else if (match ("!")) {
                k = hier10 (lval);
                if (k)
                        k = rvalue(lval, k);
                gen_logical_negation();
                return (0);
        } else if (ch()=='*' && nch() != '=') {
                inbyte();
                k = hier10 (lval);
                if (k)
                        k = rvalue(lval, k);
                if (ptr = lval->symbol)
                        lval->indirect = ptr->type;
                else
                        lval->indirect = CINT;
                lval->ptr_type = 0;  // flag as not pointer or array
                return (1);
        } else if (ch()=='&' && nch()!='&' && nch()!='=') {
                inbyte();
                k = hier10 (lval);
                if (k == 0) {
                        error ("illegal address");
                        return (0);
                }
                ptr = lval->symbol;
                if( ptr ) { 
                        lval->ptr_type = ptr->type;
                        if (lval->indirect)
                            return (0);
                        /* global and non-array */
                        gen_immediate_a ();
                        output_string ((ptr = lval->symbol)->name);
                        newline ();
                        lval->indirect = ptr->type;
                } else {
                        lval->ptr_type = lval->indirect;
                        lval->indirect = 0;
                }
                return (0);
        } else {
                k = hier11 (lval);
                if (match ("++")) {
                        if (k == 0) {
                                needlval ();
                                return (0);
                        }
                        if (lval->indirect)
                                gen_push(k);
                        k = rvalue(lval, k);
                        gen_increment_primary_reg (lval);
                        store (lval);
                        gen_decrement_primary_reg (lval);
                        return (0);
                } else if (match ("--")) {
                        if (k == 0) {
                                needlval ();
                                return (0);
                        }
                        if (lval->indirect)
                                gen_push(k);
                        k = rvalue(lval, k);
                        gen_decrement_primary_reg (lval);
                        store (lval);
                        gen_increment_primary_reg (lval);
                        return (0);
                } else
                        return (k);
        }

}

/**
 * array subscripting
 * @param lval
 * @return 0 or 1, fetch or no fetch
 */
hier11 (lvalue_t *lval) {
        int     k;
        symbol_t *ptr;

        k = primary (lval);
        ptr = lval->symbol;
        blanks ();
        if ((ch () == '[') | (ch () == '('))
                for (;;) {
                        if (match ("[")) {
                                if (ptr == 0) {
                                        error ("can't subscript");
                                        junk ();
                                        needbrack ("]");
                                        return (0);
                                } else if (ptr->identity == POINTER)
                                        k = rvalue(lval, k);
                                else if (ptr->identity != ARRAY) {
                                        error ("can't subscript");
                                        k = 0;
                                }
                                gen_push(k);
                                expression (YES);
                                needbrack ("]");
                                if (ptr->type & CINT)
                                        gen_multiply_by_two ();
                                gen_add (NULL,NULL);
                                lval->symbol = 0;
                                lval->indirect = ptr->type;
                                k = HL_REG;
                        } else if (match ("(")) {
                                if (ptr == 0)
                                        callfunction (0);
                                else if (ptr->identity != FUNCTION) {
                                        k = rvalue(lval, k);
                                        callfunction (0);
                                } else
                                        callfunction (ptr);
                                lval->symbol = 0;
                                k = 0;
                        } else
                                return (k);
                }
        if (ptr == 0)
                return (k);
        if (ptr->identity == FUNCTION) {
                gen_immediate_a ();
                output_string (ptr);
                newline ();
                return (0);
        }
        return (k);
}
