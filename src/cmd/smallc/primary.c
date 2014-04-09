/*
 * File primary.c: 2.4 (84/11/27,16:26:07)
 */
#include <stdio.h>
#include "defs.h"
#include "data.h"

primary (lvalue_t *lval)
{
        char    sname[NAMESIZE];
        int     num[1], k, symbol_table_idx, offset, reg;
        symbol_t *symbol;

        lval->ptr_type = 0;  /* clear pointer/array type */
        if (match ("(")) {
                k = hier1 (lval);
                needbrack (")");
                return (k);
        }
        if (amatch("sizeof", 6)) {
                needbrack("(");
                gen_immediate_c();
                if (amatch("int", 3)) output_number(INTSIZE);
                else if (amatch("char", 4)) output_number(1);
                else if (symname(sname)) {
                        if ((symbol_table_idx = findloc(sname)) ||
                                (symbol_table_idx = findglb(sname))) {
                                symbol = &symbol_table[symbol_table_idx];
                                if (symbol->storage == LSTATIC)
                                        error("sizeof local static");
                                offset = symbol->count;
                                if ((symbol->type & CINT) ||
                                        (symbol->identity == POINTER))
                                        offset *= INTSIZE;
                                output_number(offset);
                        } else {
                                error("sizeof undeclared variable");
                                output_number(0);
                        }
                } else {
                        error("sizeof only on simple type or variable");
                }
                needbrack(")");
                newline();
                lval->symbol = 0;
                lval->indirect = 0;
                return(0);
        }
        if (symname (sname)) {
                if (symbol_table_idx = findloc (sname)) {
                        symbol = &symbol_table[symbol_table_idx];
                        reg = gen_get_location (symbol);
                        lval->symbol = symbol;
                        lval->indirect =  symbol->type;
                        if (symbol->identity == ARRAY) {
                                //lval->ptr_type = symbol->type;
                                lval->ptr_type = 0;
                                return 0;
                        }
                        if (symbol->identity == POINTER) {
                                lval->indirect = UINT;
                                lval->ptr_type = symbol->type;
                        }
                        return reg;
                }
                if (symbol_table_idx = findglb (sname)) {
                        symbol = &symbol_table[symbol_table_idx];
                        lval->symbol = symbol;
                        switch (symbol->identity) {
                        case ARRAY:
                                gen_immediate_a ();
                                output_string (symbol->name);
                                newline ();
                                /* Fall through */
                        case FUNCTION:
                                lval->indirect = lval->ptr_type = symbol_table[symbol_table_idx].type;
                                lval->ptr_type = 0;
                                return 0;
                        default:
                                lval->indirect = 0;
                                if (symbol->identity == POINTER) {
                                        lval->ptr_type = symbol->type;
                                }
                                return 1;
                        }
                }
                blanks ();
                if (ch() != '(')
                        error("undeclared variable");
                symbol_table_idx = add_global (sname, FUNCTION, CINT, 0, PUBLIC, 1);
                symbol = &symbol_table[symbol_table_idx];
                lval->symbol = symbol;
                lval->indirect = 0;
                return 0;
        }
        if (constant (num)) {
                lval->symbol = 0;
                lval->indirect = 0;
                return 0;
        }
        error ("invalid expression");
        gen_immediate_c ();
        output_number(0);
        newline ();
        junk ();
        return 0;
}

/**
 * true if val1 -> int pointer or int array and val2 not pointer or array
 * @param val1
 * @param val2
 * @return
 */
dbltest (lvalue_t *val1, lvalue_t *val2)
{
        if (val1 == NULL)
                return (FALSE);
        if (!(val1->ptr_type & CINT))
                return (FALSE);
        if (val2->ptr_type)
                return (FALSE);
        return (TRUE);
}

/**
 * determine type of binary operation
 * @param lval
 * @param lval2
 * @return
 */
result (lvalue_t *lval, lvalue_t *lval2)
{
        if (lval->ptr_type && lval2->ptr_type)
                lval->ptr_type = 0;
        else if (lval2->ptr_type) {
                lval->symbol = lval2->symbol;
                lval->indirect = lval2->indirect;
                lval->ptr_type = lval2->ptr_type;
        }
}

constant (val)
        int     val[];
{
        if (number (val))
                gen_immediate_c ();
        else if (quoted_char (val))
                gen_immediate_c ();
        else if (quoted_string (val)) {
                gen_immediate_a ();
                print_label (litlab);
                output_byte ('+');
        } else
                return (0);
        output_number (val[0]);
        newline ();
        return (1);

}

number (val)
        int     val[];
{
        int     k, minus, base;
        char    c;

        k = minus = 1;
        while (k) {
                k = 0;
                if (match ("+"))
                        k = 1;
                if (match ("-")) {
                        minus = (-minus);
                        k = 1;
                }
        }
        if (!numeric (c = ch ()))
                return (0);
        if (match ("0x") || match ("0X"))
                while (numeric (c = ch ()) ||
                       (c >= 'a' && c <= 'f') ||
                       (c >= 'A' && c <= 'F')) {
                        inbyte ();
                        k = k * 16 +
                            (numeric (c) ? (c - '0') : ((c & 07) + 9));
                }
        else {
                base = (c == '0') ? 8 : 10;
                while (numeric (ch ())) {
                        c = inbyte ();
                        k = k * base + (c - '0');
                }
        }
        if (minus < 0)
                k = (-k);
        val[0] = k;
        if(k < 0) {
            return (UINT);
        } else {
            return (CINT);
        }
}

/**
 * Test if we have one char enclosed in single quotes
 * @param value returns the char found
 * @return 1 if we have, 0 otherwise
 */
quoted_char (int *value)
{
        int     k;
        char    c;

        k = 0;
        if (!match ("'"))
                return (0);
        while ((c = gch ()) != '\'') {
                c = (c == '\\') ? spechar(): c;
                k = (k & 255) << 8 | (c & 255);
        }
        *value = k;
        return (1);
}

/**
 * Test if we have string enclosed in double quotes. e.g. "abc".
 * Load the string into literal pool.
 * @param position returns beginning of the string
 * @return 1 if such string found, 0 otherwise
 */
quoted_string (int *position)
{
        char    c;

        if (!match ("\""))
                return (0);
        *position = litptr;
        while (ch () != '"') {
                if (ch () == 0)
                        break;
                if (litptr >= LITMAX) {
                        error ("string space exhausted");
                        while (!match ("\""))
                                if (gch () == 0)
                                        break;
                        return (1);
                }
                c = gch();
                litq[litptr++] = (c == '\\') ? spechar(): c;
        }
        gch ();
        litq[litptr++] = 0;
        return (1);
}

/**
 * decode special characters (preceeded by back slashes)
 */
spechar()
{
        char c;
        c = ch();

        if      (c == EOS) return 0;
        else if (c == 'n') c = LF;
        else if (c == 't') c = TAB;
        else if (c == 'r') c = CR;
        else if (c == 'f') c = FFEED;
        else if (c == 'b') c = BKSP;
        else if (c >= '0' && c <= '7') {
                int n = c - '0';
                gch();
                c = ch();
                if (c < '0' || c > '7')
                        return n;
                n = (n << 3) + c - '0';
                gch();
                c = ch();
                if (c < '0' || c > '7')
                        return n;
                n = (n << 3) + c - '0';
                gch();
                return n;
        }
        gch();
        return c;
}

/**
 * perform a function call
 * called from "hier11", this routine will either call the named
 * function, or if the supplied ptr is zero, will call the contents
 * of HL
 * @param ptr name of the function
 */
void callfunction (ptr)
        char    *ptr;
{
        int     nargs;

        nargs = 0;
        blanks ();
        if (ptr == 0)
                gen_push (HL_REG);
        while (!streq (line + lptr, ")")) {
                if (endst ())
                        break;
                expression (NO);
                if (ptr == 0)
                        gen_swap_stack ();
                gen_push (HL_REG);
                nargs = nargs + INTSIZE;
                if (!match (","))
                        break;
        }
        needbrack (")");
        // Generate an argument count to function calls.
        // This feature is arch-specific, disable it for now.
        // if (aflag)
        gnargs(nargs / INTSIZE);
        if (ptr)
                gen_call (ptr);
        else
                callstk ();
        stkp = gen_modify_stack (stkp + nargs);
}

needlval ()
{
        error ("must be lvalue");
}
