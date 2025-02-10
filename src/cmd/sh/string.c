/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 */
#include "defs.h"

/* ========     general purpose string handling ======== */

char *movstr(char *a, char *b)
{
    while ((*b++ = *a++))
        ;
    return (--b);
}

int any(char c, char *s)
{
    register char d;

    while ((d = *s++)) {
        if (d == c)
            return (TRUE);
    }
    return (FALSE);
}

int cf(char *s1, char *s2)
{
    while (*s1++ == *s2)
        if (*s2++ == '\0')
            return (0);
    return *--s1 - *s2;
}

int length(char *as)
{
    register char *s;

    if ((s = as))
        while (*s++)
            ;
    return (s - as);
}

char *movstrn(char *a, char *b, int n)
{
    while ((n-- > 0) && *a)
        *b++ = *a++;

    return (b);
}
