/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 */
#include "defs.h"

/* ========	storage allocation	======== */

char *getstak(int asize) /* allocate requested stack */
{
    register char *oldstak;
    register int size;

    size = round(asize, BYTESPERWORD);
    oldstak = stakbot;
    staktop = stakbot += size;
    return (oldstak);
}

/*
 * set up stack for local use
 * should be followed by `endstak'
 */
char *locstak()
{
    if (brkend - stakbot < BRKINCR) {
        if (setbrk(brkincr) == (char *)-1)
            error(nostack);
        if (brkincr < BRKMAX)
            brkincr += 256;
    }
    return (stakbot);
}

char *savstak()
{
    assert(staktop == stakbot);
    return (stakbot);
}

char *endstak(char *argp) /* tidy up after `locstak' */
{
    register char *oldstak;

    *argp++ = 0;
    oldstak = stakbot;
    stakbot = staktop = (char *)round(argp, BYTESPERWORD);
    return (oldstak);
}

void tdystak(char *x) /* try to bring stack back to x */
{
    while ((char *)(stakbsy) > (char *)(x)) {
        free(stakbsy);
        stakbsy = stakbsy->word;
    }
    staktop = stakbot = max((char *)(x), (char *)(stakbas));
    rmtemp((struct ionod *)x);
}

void stakchk()
{
    if ((brkend - stakbas) > BRKINCR + BRKINCR)
        setbrk(-BRKINCR);
}

char *cpystak(char *x)
{
    return (endstak(movstr(x, locstak())));
}
