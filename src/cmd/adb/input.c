/*
 * Input routines
 */
#include "defs.h"

int lastc = EOR;

static char line[LINSIZ];

int
eol(c)
{
    return (c == EOR || c == ';');
}

int
rdc()
{
    do {
        readchar();
    } while (lastc == SP || lastc == TB );
    return lastc;
}

int
readchar()
{
    if (eof) {
        lastc = '\0';
    } else {
        if (lp == 0) {
            lp = line;
            do {
                eof = (read(infile, lp, 1) == 0);
                if (mkfault)
                    error((char *)0);
            } while (eof == 0 && *lp++ != EOR);
            *lp = 0;
            lp = line;
        }
        lastc = *lp;
        if (lastc)
            lp++;
    }
    return lastc;
}

int
nextchar()
{
    if (eol(rdc())) {
        lp--;
        return 0;
    }
    return lastc;
}

int
quotchar()
{
    if (readchar() == '\\')
        return readchar();
    if (lastc == '\'')
        return 0;
    return lastc;
}

void
getformat(deformat)
    char *deformat;
{
    register char   *fptr;
    register int    quote;

    fptr = deformat;
    quote = FALSE;
    while ((quote ? readchar()!=EOR : !eol(readchar()))) {
        if ((*fptr++ = lastc) == '"') {
            quote = ~quote;
        }
    }
    lp--;
    if (fptr != deformat) {
        *fptr++ = '\0';
    }
}
