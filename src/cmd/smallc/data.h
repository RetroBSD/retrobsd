/*      File data.h: 2.2 (84/11/27,16:26:11) */

/* storage words */

extern symbol_t symbol_table[];
extern int global_table_index, rglobal_table_index;
extern int local_table_index;

extern  loop_t  loopstack[];
extern  int     loop_table_index;

extern  int     swstcase[];
extern  int     swstlab[];
extern  int     swstp;
extern  char    litq[];
extern  int     litptr;
extern  char    line[];
extern  int     lptr;

/* miscellaneous storage */

extern  int     nxtlab,
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

extern  FILE    *input, *output;
extern  int     inclsp;

extern  int     current_symbol_table_idx;
extern  int     *iptr;
extern  int     fexitlab;
extern  int     errfile;
extern  int     errs;

extern char initials_table[];
extern char *initials_table_ptr;
