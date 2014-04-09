/*
 * File data.c: 2.2 (84/11/27,16:26:13)
 */
#include <stdio.h>
#include "defs.h"

/* storage words */
symbol_t symbol_table[NUMBER_OF_GLOBALS + NUMBER_OF_LOCALS];
int global_table_index, rglobal_table_index;
int local_table_index;

loop_t  loopstack[WSTABSZ];
int     loop_table_index;

int     swstcase[SWSTSZ];
int     swstlab[SWSTSZ];
int     swstp;
char    litq[LITABSZ];
int     litptr;
char    line[LINESIZE];
int     lptr;

/* miscellaneous storage */

int     nxtlab,
        litlab,
        stkp,
        argstk,
        ncmp,
        errcnt,
        glbflag,
        verbose,
        ctext,
        cmode,
        lastst;

FILE    *input, *output;
int     inclsp;

int     current_symbol_table_idx;
int     *iptr;
int     fexitlab;
int     errfile;
int     errs;

char initials_table[INITIALS_SIZE];      // 5kB space for initialisation data
char *initials_table_ptr = 0;
