/*      File gen.c: 2.1 (83/03/20,16:02:06) */
/*% cc -O -c %
 *
 */

#include <stdio.h>
#include "defs.h"
#include "data.h"

/*
 *      return next available internal label number
 *
 */
getlabel ()
{
        return (nxtlab++);

}

/**
 * print specified number as label
 * @param label
 */
print_label (label)
int     label;
{
        output_label_prefix ();
        output_decimal (label);
}

/**
 * glabel - generate label
 * not used ?
 * @param lab label number
 */
glabel (lab)
char    *lab;
{
        output_string (lab);
        output_label_terminator ();
        newline ();
}

/**
 * gnlabel - generate numeric label
 * @param nlab label number
 * @return
 */
generate_label (nlab)
int     nlab;
{
        print_label (nlab);
        output_label_terminator ();
        newline ();
}

/**
 * outputs one byte
 * @param c
 * @return
 */
output_byte (c)
char    c;
{
        if (c == 0)
                return (0);
        fputc (c, output);
        return (c);
}

/**
 * outputs a string
 * @param ptr the string
 * @return
 */
output_string (ptr)
char    ptr[];
{
        int     k;
        k = 0;
        while (output_byte (ptr[k++]));
}

/**
 * outputs a tab
 * @return
 */
print_tab ()
{
        output_byte ('\t');
}

/**
 * output line
 * @param ptr
 * @return
 */
output_line (ptr)
char    ptr[];
{
        output_with_tab (ptr);
        newline ();
}

/**
 * tabbed output
 * @param ptr
 * @return
 */
output_with_tab (ptr)
char    ptr[];
{
        print_tab ();
        output_string (ptr);
}

/**
 * output decimal number
 * @param number
 * @return
 */
output_decimal (int number)
{
    fprintf(output, "%d", number);
}

/**
 * stores values into memory
 * @param lval TODO
 * @return
 */
store (lvalue_t *lval)
{
    if (lval->indirect == 0)
        gen_put_memory (lval->symbol);
    else
        gen_put_indirect (lval->indirect);
}

rvalue (lvalue_t *lval, int reg)
{
    if ((lval->symbol != 0) & (lval->indirect == 0))
        gen_get_memory (lval->symbol);
    else
        gen_get_indirect (lval->indirect, reg);
    return HL_REG;
}

/**
 * parses test part "(expression)" input and generates assembly for jump
 * @param label
 * @param ft
 * @return
 */
test (label, ft)
int     label,
        ft;
{
        needbrack ("(");
        expression (YES);
        needbrack (")");
        gen_test_jump (label, ft);
}
