#include "defs.h"

int
findroutine(cframe)
    long cframe;
{
    register int narg, inst;
    int lastpc, back2;
    char v;

    v = FALSE;
    localok = FALSE;
    lastpc = callpc;
    callpc = get(cframe+2, DSP);
    back2 = get(leng(callpc - 2), ISP);
    inst = get(leng(callpc-4), ISP);
    if (inst == 04737) {                        /* jsr pc, *$... */
        narg = 1;
    } else if ((inst & ~077) == 04700) {        /* jsr pc, ... */
        narg = 0;
        v = (inst != 04767);
    } else if ((back2 & ~077) == 04700) {
        narg = 0;
        v = TRUE;
    } else {
        errflg = NOCFN;
        return 0;
    }
    if (findsym((v ? lastpc : ((inst==04767 ? callpc : 0) + back2)), ISYM) == -1 && !v)
        symbol = NULL;
    else
        localok = TRUE;
    inst = get(leng(callpc), ISP);
    if (inst == 062706) {                       /* add $n, sp */
        narg += get(leng(callpc + 2), ISP) / 2;
        return narg;
    }
    if (inst == 05726 || inst == 010026)        /* tst (sp)+ or mov r0, (sp)+ */
        return narg + 1;
    if (inst == 022626) {                       /* cmp (sp)+, (sp)+ */
        return narg + 2;
    }
    return narg;
}
