/*
 * File while.c: 2.1 (83/03/20,16:02:22)
 */
#include <stdio.h>
#include "defs.h"
#include "data.h"

addloop (loop_t *ptr)
{
    if (loop_table_index == WSTABSZ) {
        error ("too many active loops");
        return;
    }
    loopstack[loop_table_index++] = *ptr;
}

delloop ()
{
    if (readloop ()) {
        loop_table_index--;
    }
}

loop_t *readloop ()
{
    if (loop_table_index == 0) {
        error ("no active do/for/while/switch");
        return 0;
    }
    return &loopstack[loop_table_index - 1];
}

loop_t *findloop ()
{
    int i;

    for (i=loop_table_index; --i>= 0;) {
        if (loopstack[i].type != WSSWITCH)
            return &loopstack[i];
    }
    error ("no active do/for/while");
    return 0;
}

loop_t *readswitch ()
{
    loop_t *ptr;

    if (ptr = readloop ()) {
        if (ptr->type == WSSWITCH) {
            return ptr;
        }
    }
    return 0;
}

addcase (int val)
{
    int     lab;

    if (swstp == SWSTSZ) {
        error ("too many case labels");
        return;
    }
    swstcase[swstp] = val;
    swstlab[swstp++] = lab = getlabel ();
    print_label (lab);
    output_label_terminator ();
    newline ();
}
