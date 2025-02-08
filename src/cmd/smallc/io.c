/*      File io.c: 2.1 (83/03/20,16:02:07) */
/*% cc -O -c %
 *
 */

#include <stdio.h>
#include <string.h>
#include "defs.h"
#include "data.h"

void kill ()
{
        lptr = 0;
        line[lptr] = 0;
}

void readline ()
{
        int     k;

        for (;;) {
                if (feof (input))
                        return;
                kill ();
                while ((k = fgetc (input)) != EOF) {
                        if ((k == CR) || (k == LF) || (lptr >= LINEMAX))
                                break;
                        line[lptr++] = k;
                }
                line[lptr] = 0;
                if (lptr) {
                        if ((ctext) & (cmode)) {
                                gen_comment ();
                                output_string (line);
                                newline ();
                        }
                        lptr = 0;
                        return;
                }
        }
}

int inbyte ()
{
        while (ch () == 0) {
                if (feof (input))
                        return (0);
                readline ();
        }
        return (gch ());
}

int inchar ()
{
        if (ch () == 0)
                readline ();
        if (feof (input))
                return (0);
        return (gch ());
}

/**
 * gets current char from input line and moves to the next one
 * @return current char
 */
int gch ()
{
        int c = line[lptr];
        if (c == 0)
                return 0;
        lptr++;
        return c;
}

/**
 * returns next char
 * @return next char
 */
int nch ()
{
        int c = line[lptr];
        if (c == 0)
                return 0;
        return line[lptr+1];
}

/**
 * returns current char
 * @return current char
 */
int ch ()
{
        return line[lptr];
}
