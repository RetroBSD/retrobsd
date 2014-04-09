/*      File error.c: 2.1 (83/03/20,16:02:00) */
/*% cc -O -c %
 *
 */

#include <stdio.h>
#include "defs.h"
#include "data.h"

error (ptr)
char    ptr[];
{
        FILE *tempfile;

        tempfile = output;
        output = stdout;
        doerror(ptr);
        output = tempfile;
        doerror(ptr);
        errcnt++;
}

doerror(ptr) char *ptr; {
        int k;
        gen_comment ();
        output_string (line);
        newline ();
        gen_comment ();
        k = 0;
        while (k < lptr) {
                if (line[k] == 9)
                        print_tab ();
                else
                        output_byte (' ');
                k++;
        }
        output_byte ('^');
        newline ();
        gen_comment ();
        output_string ("******  ");
        output_string (ptr);
        output_string ("  ******");
        newline ();
}

